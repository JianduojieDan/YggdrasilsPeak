// Copyright Epic Games, Inc. All Rights Reserved.

#include "LandscapeRebuilder.h"

#include "Engine/World.h"
#include "Landscape.h"
#include "LandscapeProxy.h"
#include "Async/ParallelFor.h"
#include "EngineUtils.h"
#include "Environment/FixedRope.h"
#include "Survival/SurvivalConfig.h"
#include "Dom/JsonObject.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

#if WITH_EDITOR
#include "LandscapeEdit.h"
#include "LandscapeInfo.h"
#include "LandscapeEditLayer.h"
#endif

ALandscapeRebuilder::ALandscapeRebuilder()
{
	PrimaryActorTick.bCanEverTick = false;
	bIsEditorOnlyActor = true;
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
}

#if WITH_EDITOR
void ALandscapeRebuilder::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	const FProperty* Prop = PropertyChangedEvent.MemberProperty;
	const bool bTuning = Prop && Prop->GetFName() != GET_MEMBER_NAME_CHECKED(ALandscapeRebuilder, bAutoRebuild)
		&& Prop->GetFName() != GET_MEMBER_NAME_CHECKED(ALandscapeRebuilder, bRebuildNow)
		&& Prop->GetFName() != GET_MEMBER_NAME_CHECKED(ALandscapeRebuilder, LastResult)
		&& Prop->GetOwnerClass() == ALandscapeRebuilder::StaticClass();
	// Interactive drags fire many times; only rebuild when the value is committed.
	if (bRebuildNow || (bAutoRebuild && bTuning && PropertyChangedEvent.ChangeType != EPropertyChangeType::Interactive))
	{
		bRebuildNow = false;
		RebuildLandscape();
	}
}
#endif

namespace
{
	/** Catmull-Rom through p1..p2 (passes exactly through the source samples). */
	double CatmullRom(double P0, double P1, double P2, double P3, double T)
	{
		const double T2 = T * T;
		const double T3 = T2 * T;
		return 0.5 * ((2.0 * P1) + (-P0 + P2) * T + (2.0 * P0 - 5.0 * P1 + 4.0 * P2 - P3) * T2 + (-P0 + 3.0 * P1 - 3.0 * P2 + P3) * T3);
	}

	double Hash(int32 X, int32 Y, int32 Seed)
	{
		uint32 H = uint32(X) * 374761393u + uint32(Y) * 668265263u + uint32(Seed) * 2246822519u;
		H = (H ^ (H >> 13)) * 1274126177u;
		return double(H ^ (H >> 16)) / 4294967295.0;
	}

	/** Smooth value noise in -1..1. */
	double ValueNoise(double X, double Y, int32 Seed)
	{
		const int32 IX = FMath::FloorToInt32(X), IY = FMath::FloorToInt32(Y);
		double FX = X - IX, FY = Y - IY;
		FX = FX * FX * (3.0 - 2.0 * FX);
		FY = FY * FY * (3.0 - 2.0 * FY);
		const double A = FMath::Lerp(Hash(IX, IY, Seed), Hash(IX + 1, IY, Seed), FX);
		const double B = FMath::Lerp(Hash(IX, IY + 1, Seed), Hash(IX + 1, IY + 1, Seed), FX);
		return FMath::Lerp(A, B, FY) * 2.0 - 1.0;
	}
}

void ALandscapeRebuilder::RebuildLandscape()
{
#if WITH_EDITOR
	UWorld* World = GetWorld();
	ULandscapeInfo* Info = SourceLandscape ? SourceLandscape->GetLandscapeInfo() : nullptr;
	if (!World || !Info)
	{
		LastResult = TEXT("No SourceLandscape.");
		return;
	}

	int32 MinX, MinY, MaxX, MaxY;
	if (!Info->GetLandscapeExtent(MinX, MinY, MaxX, MaxY))
	{
		LastResult = TEXT("Source landscape has no extent.");
		return;
	}
	const int32 SrcW = MaxX - MinX + 1;
	const int32 SrcH = MaxY - MinY + 1;

	const int32 QuadsPerComponent = SubsectionQuads * NumSubsections;
	const int32 NewQuadsX = (SrcW - 1) * Factor;
	const int32 NewQuadsY = (SrcH - 1) * Factor;
	if (QuadsPerComponent <= 0 || NewQuadsX % QuadsPerComponent != 0 || NewQuadsY % QuadsPerComponent != 0)
	{
		LastResult = FString::Printf(TEXT("%d x %d quads do not divide into components of %d quads."), NewQuadsX, NewQuadsY, QuadsPerComponent);
		return;
	}
	const int32 NewW = NewQuadsX + 1;
	const int32 NewH = NewQuadsY + 1;

	// Read the current (final) heights.
	TArray<uint16> Src;
	Src.SetNumZeroed(SrcW * SrcH);
	{
		FLandscapeEditDataInterface Edit(Info);
		int32 X1 = MinX, Y1 = MinY, X2 = MaxX, Y2 = MaxY;
		Edit.GetHeightData(X1, Y1, X2, Y2, Src.GetData(), SrcW);
	}
	auto S = [&](int32 X, int32 Y) -> double
	{
		return Src[FMath::Clamp(Y, 0, SrcH - 1) * SrcW + FMath::Clamp(X, 0, SrcW - 1)];
	};

	// Separable Catmull-Rom upsample: rows first, then columns.
	TArray<double> Rows;
	Rows.SetNumUninitialized(NewW * SrcH);
	for (int32 Y = 0; Y < SrcH; ++Y)
	{
		for (int32 X = 0; X < NewW; ++X)
		{
			const int32 I = X / Factor;
			const double T = double(X % Factor) / Factor;
			Rows[Y * NewW + X] = CatmullRom(S(I - 1, Y), S(I, Y), S(I + 1, Y), S(I + 2, Y), T);
		}
	}
	TArray<uint16> Heights;
	Heights.SetNumUninitialized(NewW * NewH);
	for (int32 Y = 0; Y < NewH; ++Y)
	{
		const int32 J = Y / Factor;
		const double T = double(Y % Factor) / Factor;
		for (int32 X = 0; X < NewW; ++X)
		{
			auto R = [&](int32 Row) { return Rows[FMath::Clamp(Row, 0, SrcH - 1) * NewW + X]; };
			Heights[Y * NewW + X] = (uint16)FMath::Clamp(FMath::RoundToInt(CatmullRom(R(J - 1), R(J), R(J + 1), R(J + 2), T)), 0, 65535);
		}
	}

	// Baked noise: fBm bumps mixed with ridged noise, octaves from NoiseSizeM down to ~2 vertex spacings,
	// amplitude raised on steep ground. All positions in metres so it doesn't depend on Factor.
	const FVector OldScaleForNoise = SourceLandscape->GetActorTransform().GetScale3D();
	const double SpacingM = OldScaleForNoise.X / Factor / 100.0;
	const double UnitsPerM = 100.0 / (OldScaleForNoise.Z / 128.0);
	if (NoiseHeightM > 0.f)
	{
		const TArray<uint16> Base = Heights;
		ParallelFor(NewH, [&](int32 Y)
		{
			for (int32 X = 0; X < NewW; ++X)
			{
				const int32 XL = FMath::Max(X - 1, 0), XR = FMath::Min(X + 1, NewW - 1);
				const int32 YU = FMath::Max(Y - 1, 0), YD = FMath::Min(Y + 1, NewH - 1);
				const double DX = (double(Base[Y * NewW + XR]) - Base[Y * NewW + XL]) / UnitsPerM / ((XR - XL) * SpacingM);
				const double DY = (double(Base[YD * NewW + X]) - Base[YU * NewW + X]) / UnitsPerM / ((YD - YU) * SpacingM);
				const double Slope = FMath::Sqrt(DX * DX + DY * DY); // rise over run
				const double Steep = FMath::SmoothStep(0.3, 1.2, Slope);

				double Sum = 0.0, Amp = 1.0, Norm = 0.0;
				double PX = X * SpacingM / NoiseSizeM, PY = Y * SpacingM / NoiseSizeM;
				for (int32 O = 0; O < 8 && NoiseSizeM / (1 << O) >= 2.0 * SpacingM; ++O)
				{
					const double N = ValueNoise(PX, PY, Seed + O);
					const double R = 1.0 - 2.0 * FMath::Abs(N); // ridges at noise zero-crossings
					Sum += Amp * FMath::Lerp(N, R, (double)Ridged);
					Norm += Amp;
					Amp *= 0.5;
					// Rotate + scale each octave so the grids don't line up.
					const double NX = (PX * 1.6 - PY * 1.2) + 17.3, NY = (PX * 1.2 + PY * 1.6) + 5.1;
					PX = NX; PY = NY;
				}
				const double OffsetM = (Norm > 0.0 ? Sum / Norm : 0.0) * NoiseHeightM * FMath::Lerp(1.0, (double)SteepMultiplier, Steep);
				const int32 I = Y * NewW + X;
				Heights[I] = (uint16)FMath::Clamp(FMath::RoundToInt(Base[I] + OffsetM * UnitsPerM), 0, 65535);
			}
		});
	}

	// Replace the previous result instead of stacking copies.
	if (BuiltLandscape && BuiltLandscape != SourceLandscape)
	{
		World->EditorDestroyActor(BuiltLandscape, true);
		BuiltLandscape = nullptr;
	}

	// New landscape: same corner and height scale, Factor times finer spacing.
	const FTransform OldTransform = SourceLandscape->GetActorTransform();
	const FVector OldScale = OldTransform.GetScale3D();
	const FVector Corner = OldTransform.TransformPosition(FVector(MinX, MinY, 0.0));

	ALandscape* NewLandscape = World->SpawnActor<ALandscape>(Corner, OldTransform.Rotator());
	NewLandscape->SetActorRelativeScale3D(FVector(OldScale.X / Factor, OldScale.Y / Factor, OldScale.Z));
	NewLandscape->LandscapeMaterial = SourceLandscape->LandscapeMaterial;
	NewLandscape->DefaultPhysMaterial = SourceLandscape->DefaultPhysMaterial;
	NewLandscape->StaticLightingLOD = FMath::DivideAndRoundUp(FMath::CeilLogTwo((NewW * NewH) / (2048 * 2048) + 1), (uint32)2);

	TMap<FGuid, TArray<uint16>> HeightData;
	HeightData.Add(FGuid(), MoveTemp(Heights));
	TMap<FGuid, TArray<FLandscapeImportLayerInfo>> LayerData;
	LayerData.Add(FGuid(), TArray<FLandscapeImportLayerInfo>());
	NewLandscape->Import(FGuid::NewGuid(), 0, 0, NewW - 1, NewH - 1, NumSubsections, SubsectionQuads, HeightData, TEXT(""),
		LayerData, ELandscapeImportAlphamapType::Additive, TArrayView<const FLandscapeLayer>());
	NewLandscape->SetActorLabel(TEXT("Landscape_Smooth"));
	// PCG_RockScatter samples the landscape by this tag (the hidden source must not be sampled too).
	NewLandscape->Tags.AddUnique(TEXT("PlayTerrain"));
	// bEnableNanite is protected; copy it through reflection.
	if (const FBoolProperty* NaniteProp = FindFProperty<FBoolProperty>(ALandscapeProxy::StaticClass(), TEXT("bEnableNanite")))
	{
		NaniteProp->SetPropertyValue_InContainer(NewLandscape, SourceLandscape->IsNaniteEnabled());
	}
	BuiltLandscape = NewLandscape;

	// Keep the old one for comparison, but out of the way.
	SourceLandscape->SetIsTemporarilyHiddenInEditor(true);
	SourceLandscape->SetActorHiddenInGame(true);
	SourceLandscape->SetActorEnableCollision(false);

	LastResult = FString::Printf(TEXT("Built %s: %d x %d verts from %d x %d."), *NewLandscape->GetName(), NewW, NewH, SrcW, SrcH);
	UE_LOG(LogTemp, Display, TEXT("LandscapeRebuilder: %s"), *LastResult);
#endif
}

void ALandscapeRebuilder::RaiseSummit()
{
#if WITH_EDITOR
	ALandscape* Landscape = Cast<ALandscape>(BuiltLandscape.Get());
	ULandscapeInfo* Info = Landscape ? Landscape->GetLandscapeInfo() : nullptr;
	int32 MinX, MinY, MaxX, MaxY;
	if (!Info || !Info->GetLandscapeExtent(MinX, MinY, MaxX, MaxY))
	{
		LastResult = TEXT("RaiseSummit: no BuiltLandscape.");
		return;
	}
	const int32 W = MaxX - MinX + 1;
	const int32 H = MaxY - MinY + 1;

	// Write into the base edit layer (the only one), then let the landscape re-merge its layers.
	TOptional<FScopedSetLandscapeEditingLayer> LayerScope;
	if (const ULandscapeEditLayerBase* Layer = Landscape->GetEditLayerConst(0))
	{
		LayerScope.Emplace(Landscape, Layer->GetGuid(), [Landscape] { Landscape->RequestLayersContentUpdateForceAll(); });
	}

	TArray<uint16> Heights;
	Heights.SetNumZeroed(W * H);
	FLandscapeEditDataInterface Edit(Info);
	{
		int32 X1 = MinX, Y1 = MinY, X2 = MaxX, Y2 = MaxY;
		Edit.GetHeightData(X1, Y1, X2, Y2, Heights.GetData(), W);
	}

	// Summit = centre of the highest samples (the top is a flat 65535 plateau when the range is used up).
	uint16 Top = 0;
	for (uint16 V : Heights) { Top = FMath::Max(Top, V); }
	FVector2D Peak = FVector2D::ZeroVector;
	int32 Count = 0;
	for (int32 Y = 0; Y < H; ++Y)
	{
		for (int32 X = 0; X < W; ++X)
		{
			if (Heights[Y * W + X] + 2 >= Top) { Peak += FVector2D(X, Y); ++Count; }
		}
	}
	Peak /= FMath::Max(Count, 1);

	const FTransform LandscapeTransform = Landscape->GetActorTransform();
	const FVector Scale = LandscapeTransform.GetScale3D();
	const double CmPerUnit = Scale.Z / 128.0;
	const USurvivalConfig* Cfg = GetDefault<USurvivalConfig>();
	const double TopZ = LandscapeTransform.GetLocation().Z + (double(Top) - 32768.0) * CmPerUnit;
	const double TargetZ = Cfg->ReferenceWorldZ + (SummitElevationM - Cfg->ReferenceElevationMeters) * 100.0;
	const double RaiseCm = TargetZ - TopZ;
	if (RaiseCm < 50.0)
	{
		LastResult = FString::Printf(TEXT("RaiseSummit: summit already at %.1f m, nothing to do."), Cfg->ReferenceElevationMeters + (TopZ - Cfg->ReferenceWorldZ) / 100.0);
		return;
	}

	// Make room at the top of the 16-bit range: store everything lower and move the actor up by the same amount.
	const double RaiseUnits = RaiseCm / CmPerUnit;
	const int32 Shift = FMath::Max(0, FMath::CeilToInt32(double(Top) + RaiseUnits - 65535.0 + 64.0));
	const double RadiusQuads = SummitRadiusM * 100.0 / Scale.X;
	auto Bump = [RadiusQuads](double DistQuads)
	{
		// 1 at the top, 0 at the edge, flat at both ends: a small standing spot and no crease where it meets the old slope.
		return 1.0 - FMath::SmoothStep(0.0, 1.0, DistQuads / RadiusQuads);
	};

	int32 Clamped = 0;
	for (int32 Y = 0; Y < H; ++Y)
	{
		for (int32 X = 0; X < W; ++X)
		{
			const double Dist = FVector2D::Distance(FVector2D(X, Y), Peak);
			const double NewValue = double(Heights[Y * W + X]) - Shift + (Dist < RadiusQuads ? RaiseUnits * Bump(Dist) : 0.0);
			Clamped += NewValue < 0.0 ? 1 : 0;
			Heights[Y * W + X] = (uint16)FMath::Clamp(FMath::RoundToInt(NewValue), 0, 65535);
		}
	}
	Edit.SetHeightData(MinX, MinY, MaxX, MaxY, Heights.GetData(), W, true);
	Edit.Flush();
	LayerScope.Reset();

	Landscape->Modify();
	Landscape->SetActorLocation(LandscapeTransform.GetLocation() + FVector(0.0, 0.0, Shift * CmPerUnit));

	// Lift what stands on the bump (summit props, the top of the last fixed rope) with the ground.
	const FVector PeakWorld = LandscapeTransform.TransformPosition(FVector(MinX + Peak.X, MinY + Peak.Y, 0.0));
	const double RadiusCm = SummitRadiusM * 100.0;
	auto LiftAt = [&](const FVector& P)
	{
		const double Dist = FVector2D::Distance(FVector2D(P), FVector2D(PeakWorld)) / Scale.X;
		return Dist < RadiusQuads ? RaiseCm * Bump(Dist) : 0.0;
	};
	int32 Moved = 0;
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		AActor* Actor = *It;
		if (Actor == this || Actor->IsA<ALandscapeProxy>())
		{
			continue;
		}
		if (AFixedRope* Rope = Cast<AFixedRope>(Actor))
		{
			bool bChanged = false;
			for (FVector& P : Rope->RopePoints)
			{
				const double Lift = LiftAt(P);
				if (Lift > 0.0) { if (!bChanged) { Rope->Modify(); } P.Z += Lift; bChanged = true; }
			}
			if (bChanged) { Rope->RerunConstructionScripts(); ++Moved; }
			continue;
		}
		FVector Origin, Extent;
		Actor->GetActorBounds(false, Origin, Extent);
		const double Lift = LiftAt(Actor->GetActorLocation());
		if (Lift > 0.0 && Extent.Size2D() < RadiusCm)
		{
			Actor->Modify();
			Actor->SetActorLocation(Actor->GetActorLocation() + FVector(0.0, 0.0, Lift));
			++Moved;
		}
	}

	LastResult = FString::Printf(TEXT("RaiseSummit: +%.1f m at the top (now %.2f m), stored heights shifted by %d (actor +%.1f m), %d valley samples clamped, %d actors lifted."),
		RaiseCm / 100.0, SummitElevationM, Shift, Shift * CmPerUnit / 100.0, Clamped, Moved);
	UE_LOG(LogTemp, Display, TEXT("LandscapeRebuilder: %s"), *LastResult);
#endif
}

void ALandscapeRebuilder::SculptHighRidges()
{
#if WITH_EDITOR
	ALandscape* Landscape = Cast<ALandscape>(BuiltLandscape.Get());
	ULandscapeInfo* Info = Landscape ? Landscape->GetLandscapeInfo() : nullptr;
	int32 MinX, MinY, MaxX, MaxY;
	if (!Info || !Info->GetLandscapeExtent(MinX, MinY, MaxX, MaxY))
	{
		LastResult = TEXT("SculptHighRidges: no BuiltLandscape.");
		return;
	}
	const int32 W = MaxX - MinX + 1;
	const int32 H = MaxY - MinY + 1;

	TOptional<FScopedSetLandscapeEditingLayer> LayerScope;
	if (const ULandscapeEditLayerBase* Layer = Landscape->GetEditLayerConst(0))
	{
		LayerScope.Emplace(Landscape, Layer->GetGuid(), [Landscape] { Landscape->RequestLayersContentUpdateForceAll(); });
	}
	TArray<uint16> Heights;
	Heights.SetNumZeroed(W * H);
	FLandscapeEditDataInterface Edit(Info);
	{
		int32 X1 = MinX, Y1 = MinY, X2 = MaxX, Y2 = MaxY;
		Edit.GetHeightData(X1, Y1, X2, Y2, Heights.GetData(), W);
	}

	const FTransform LandscapeTransform = Landscape->GetActorTransform();
	const FVector Scale = LandscapeTransform.GetScale3D();
	const double CmPerUnit = Scale.Z / 128.0;
	const double SpacingM = Scale.X / 100.0;
	const USurvivalConfig* Cfg = GetDefault<USurvivalConfig>();
	auto ElevationM = [&](double Value)
	{
		const double Z = LandscapeTransform.GetLocation().Z + (Value - 32768.0) * CmPerUnit;
		return Cfg->ReferenceElevationMeters + (Z - Cfg->ReferenceWorldZ) / 100.0;
	};
	uint16 Top = 0;
	FIntPoint Peak(0, 0);
	for (int32 I = 0; I < Heights.Num(); ++I) { if (Heights[I] > Top) { Top = Heights[I]; Peak = FIntPoint(I % W, I / W); } }

	// Ridged noise: 1 - |n| makes sharp crests and rounded troughs; two octaves, the second turned against the first.
	auto RidgeNoise = [&](double XM, double YM)
	{
		double Sum = 0.0, Norm = 0.0, Amp = 1.0, F = 1.0 / SculptWavelengthM;
		for (int32 O = 0; O < 2; ++O)
		{
			const double C = FMath::Cos(0.9 * O), S = FMath::Sin(0.9 * O);
			const double N = ValueNoise((XM * C - YM * S) * F, (XM * S + YM * C) * F, Seed + 101 + O);
			const double R = 1.0 - FMath::Abs(N);
			Sum += Amp * (R * R - 0.45);
			Norm += Amp;
			Amp *= 0.5;
			F *= 2.1;
		}
		return Sum / Norm;
	};
	TArray<float> Delta;                      // cm, per sample - also used to move props with the ground
	Delta.SetNumZeroed(W * H);
	const TArray<uint16> Base = Heights;
	ParallelFor(H, [&](int32 Y)
	{
		for (int32 X = 0; X < W; ++X)
		{
			const double V = Base[Y * W + X];
			const double Fade = FMath::SmoothStep(double(SculptAboveM), double(SculptFullAboveM), ElevationM(V));
			if (Fade <= 0.0) { continue; }
			const double Keep = FMath::SmoothStep(double(SculptKeepSummitM), SculptKeepSummitM * 3.0, FVector2D::Distance(FVector2D(X, Y), FVector2D(Peak)) * SpacingM);
			const auto At = [&](int32 DX, int32 DY) { return double(Base[FMath::Clamp(Y + DY, 0, H - 1) * W + FMath::Clamp(X + DX, 0, W - 1)]); };
			const double Slope = FVector2D(At(1, 0) - At(-1, 0), At(0, 1) - At(0, -1)).Size() * CmPerUnit / (2.0 * Scale.X);
			const double Steep = FMath::Lerp(0.25, 1.0, FMath::SmoothStep(0.25, 1.0, Slope));
			const double OffsetCm = RidgeNoise(X * SpacingM, Y * SpacingM) * SculptHeightM * 100.0 * Steep * Fade * Keep;
			Delta[Y * W + X] = float(OffsetCm);
			Heights[Y * W + X] = (uint16)FMath::Clamp(FMath::RoundToInt(V + OffsetCm / CmPerUnit), 0, 65535);
		}
	});
	Edit.SetHeightData(MinX, MinY, MaxX, MaxY, Heights.GetData(), W, true);
	Edit.Flush();
	LayerScope.Reset();
	Landscape->Modify();

	// Props, rope points: move with the ground under them (bilinear delta at their spot).
	auto DeltaAt = [&](const FVector& P)
	{
		const FVector L = LandscapeTransform.InverseTransformPosition(P) - FVector(MinX, MinY, 0.0);
		const int32 X0 = FMath::FloorToInt32(L.X), Y0 = FMath::FloorToInt32(L.Y);
		if (X0 < 0 || Y0 < 0 || X0 >= W - 1 || Y0 >= H - 1) { return 0.0; }
		const double FX = L.X - X0, FY = L.Y - Y0;
		const auto D = [&](int32 X, int32 Y) { return double(Delta[Y * W + X]); };
		return FMath::Lerp(FMath::Lerp(D(X0, Y0), D(X0 + 1, Y0), FX), FMath::Lerp(D(X0, Y0 + 1), D(X0 + 1, Y0 + 1), FX), FY);
	};
	int32 Moved = 0;
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		AActor* Actor = *It;
		if (Actor == this || Actor->IsA<ALandscapeProxy>())
		{
			continue;
		}
		if (AFixedRope* Rope = Cast<AFixedRope>(Actor))
		{
			bool bChanged = false;
			for (FVector& P : Rope->RopePoints)
			{
				const double D = DeltaAt(P);
				if (FMath::Abs(D) > 1.0) { if (!bChanged) { Rope->Modify(); } P.Z += D; bChanged = true; }
			}
			if (bChanged) { Rope->RerunConstructionScripts(); ++Moved; }
			continue;
		}
		FVector Origin, Extent;
		Actor->GetActorBounds(false, Origin, Extent);
		const double D = DeltaAt(Actor->GetActorLocation());
		if (FMath::Abs(D) > 1.0 && Extent.Size2D() < 5000.0)
		{
			Actor->Modify();
			Actor->SetActorLocation(Actor->GetActorLocation() + FVector(0.0, 0.0, D));
			++Moved;
		}
	}
	float MaxD = 0.f;
	for (float D : Delta) { MaxD = FMath::Max(MaxD, FMath::Abs(D)); }
	LastResult = FString::Printf(TEXT("SculptHighRidges: above %.0f m, up to %.1f m, %d actors moved with the ground."), SculptAboveM, MaxD / 100.0, Moved);
	UE_LOG(LogTemp, Display, TEXT("LandscapeRebuilder: %s"), *LastResult);
#endif
}

void ALandscapeRebuilder::ApplyTerrainMath()
{
#if WITH_EDITOR
	ALandscape* Landscape = Cast<ALandscape>(BuiltLandscape.Get());
	ULandscapeInfo* Info = Landscape ? Landscape->GetLandscapeInfo() : nullptr;
	int32 MinX, MinY, MaxX, MaxY;
	if (!Info || !Info->GetLandscapeExtent(MinX, MinY, MaxX, MaxY))
	{
		LastResult = TEXT("ApplyTerrainMath: no BuiltLandscape.");
		return;
	}
	const int32 W = MaxX - MinX + 1;
	const int32 H = MaxY - MinY + 1;

	TOptional<FScopedSetLandscapeEditingLayer> LayerScope;
	if (const ULandscapeEditLayerBase* Layer = Landscape->GetEditLayerConst(0))
	{
		LayerScope.Emplace(Landscape, Layer->GetGuid(), [Landscape] { Landscape->RequestLayersContentUpdateForceAll(); });
	}
	TArray<uint16> Current;
	Current.SetNumZeroed(W * H);
	FLandscapeEditDataInterface Edit(Info);
	{
		int32 X1 = MinX, Y1 = MinY, X2 = MaxX, Y2 = MaxY;
		Edit.GetHeightData(X1, Y1, X2, Y2, Current.GetData(), W);
	}

	// The clean heights: saved the first time, read back every time after, so each run replaces the last.
	const FString BasePath = FPaths::ProjectSavedDir() / TEXT("TerrainMathBase.bin");
	TArray<uint8> Bytes;
	TArray<uint16> Base;
	if (FFileHelper::LoadFileToArray(Bytes, *BasePath) && Bytes.Num() == W * H * int32(sizeof(uint16)))
	{
		Base.SetNumUninitialized(W * H);
		FMemory::Memcpy(Base.GetData(), Bytes.GetData(), Bytes.Num());
	}
	else
	{
		Base = Current;
		Bytes.SetNumUninitialized(W * H * sizeof(uint16));
		FMemory::Memcpy(Bytes.GetData(), Base.GetData(), Bytes.Num());
		if (!FFileHelper::SaveArrayToFile(Bytes, *BasePath))
		{
			LastResult = TEXT("ApplyTerrainMath: could not save the clean heights, nothing changed.");
			return;
		}
	}

	const FTransform LandscapeTransform = Landscape->GetActorTransform();
	const FVector Scale = LandscapeTransform.GetScale3D();
	const double CmPerUnit = Scale.Z / 128.0;
	const double SpacingM = Scale.X / 100.0;
	const USurvivalConfig* Cfg = GetDefault<USurvivalConfig>();
	auto ElevationM = [&](double Value)
	{
		const double Z = LandscapeTransform.GetLocation().Z + (Value - 32768.0) * CmPerUnit;
		return Cfg->ReferenceElevationMeters + (Z - Cfg->ReferenceWorldZ) / 100.0;
	};
	uint16 Top = 0;
	FIntPoint Peak(0, 0);
	for (int32 I = 0; I < Base.Num(); ++I) { if (Base[I] > Top) { Top = Base[I]; Peak = FIntPoint(I % W, I / W); } }

	// Protection map: 1 = full strength, 0 = untouched. Stamped around the route, ropes and camps.
	TArray<float> Protect;
	Protect.Init(1.f, W * H);
	auto Stamp = [&](const FVector& World, double ClearM, double FullM)
	{
		const FVector L = LandscapeTransform.InverseTransformPosition(World) - FVector(MinX, MinY, 0.0);
		const int32 Reach = FMath::CeilToInt32(FullM / SpacingM);
		for (int32 Y = FMath::Max(0, FMath::FloorToInt32(L.Y) - Reach); Y <= FMath::Min(H - 1, FMath::CeilToInt32(L.Y) + Reach); ++Y)
		{
			for (int32 X = FMath::Max(0, FMath::FloorToInt32(L.X) - Reach); X <= FMath::Min(W - 1, FMath::CeilToInt32(L.X) + Reach); ++X)
			{
				const double D = FVector2D(X - L.X, Y - L.Y).Size() * SpacingM;
				float& P = Protect[Y * W + X];
				P = FMath::Min(P, float(FMath::SmoothStep(ClearM, FullM, D)));
			}
		}
	};
	int32 RoutePoints = 0;
	FString RouteText;
	if (FFileHelper::LoadFileToString(RouteText, *(FPaths::ProjectDir() / TEXT("Tools/RoutePlanning/route.json"))))
	{
		TArray<TSharedPtr<FJsonValue>> Legs;
		const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(RouteText);
		if (FJsonSerializer::Deserialize(Reader, Legs))
		{
			for (const TSharedPtr<FJsonValue>& Leg : Legs)
			{
				const TArray<TSharedPtr<FJsonValue>>* Pts;
				if (!Leg->AsObject()->TryGetArrayField(TEXT("pts"), Pts)) { continue; }
				for (const TSharedPtr<FJsonValue>& Pt : *Pts)
				{
					const TArray<TSharedPtr<FJsonValue>>& XY = Pt->AsArray();
					if (XY.Num() >= 2)
					{
						Stamp(FVector(XY[0]->AsNumber(), XY[1]->AsNumber(), 0.0), MathRouteClearM, MathRouteClearM * 3.0);
						++RoutePoints;
					}
				}
			}
		}
	}
	if (RoutePoints == 0)
	{
		LastResult = TEXT("ApplyTerrainMath: route.json gave no points, nothing changed.");
		return;
	}
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		if (const AFixedRope* Rope = Cast<AFixedRope>(*It))
		{
			for (const FVector& P : Rope->RopePoints) { Stamp(P, MathRouteClearM, MathRouteClearM * 3.0); }
		}
		else if (It->GetClass()->GetName() == TEXT("RouteLandmark"))
		{
			Stamp(It->GetActorLocation(), MathCampClearM, MathCampClearM * 2.5);
		}
	}

	// Three octaves, each turned against the last. Sharpness blends smooth noise into ridged noise (sharp crests, round gullies).
	const double Sharp = MathSharpness;
	const double SmallestM = SpacingM * 2.0;
	auto Noise = [&](double XM, double YM)
	{
		double Sum = 0.0, Norm = 0.0, Amp = 1.0, F = 1.0 / MathScaleM;
		for (int32 O = 0; O < 3 && 1.0 / F >= SmallestM; ++O)
		{
			const double C = FMath::Cos(0.9 * O), S = FMath::Sin(0.9 * O);
			const double N = ValueNoise((XM * C - YM * S) * F + 31.7 * O, (XM * S + YM * C) * F - 17.3 * O, MathSeed + 211 + O);
			const double R = 1.0 - FMath::Abs(N);
			Sum += Amp * FMath::Lerp(N, (R * R - 0.45) * 2.0, Sharp);
			Norm += Amp;
			Amp *= 0.5;
			F *= 2.1;
		}
		return Norm > 0.0 ? Sum / Norm : 0.0;
	};
	TArray<uint16> Heights = Base;
	ParallelFor(H, [&](int32 Y)
	{
		for (int32 X = 0; X < W; ++X)
		{
			const double V = Base[Y * W + X];
			const double Fade = FMath::SmoothStep(double(MathAboveM), double(MathFullAboveM), ElevationM(V));
			if (Fade <= 0.0) { continue; }
			const double Keep = FMath::SmoothStep(double(MathKeepSummitM), MathKeepSummitM * 3.0, FVector2D::Distance(FVector2D(X, Y), FVector2D(Peak)) * SpacingM);
			const auto At = [&](int32 DX, int32 DY) { return double(Base[FMath::Clamp(Y + DY, 0, H - 1) * W + FMath::Clamp(X + DX, 0, W - 1)]); };
			const double Slope = FVector2D(At(1, 0) - At(-1, 0), At(0, 1) - At(0, -1)).Size() * CmPerUnit / (2.0 * Scale.X);
			const double Steep = FMath::Lerp(double(MathGentleGround), 1.0, FMath::SmoothStep(0.25, 1.0, Slope));
			const double OffsetCm = Noise(X * SpacingM, Y * SpacingM) * MathRoughnessM * 100.0 * Steep * Fade * Keep * Protect[Y * W + X];
			Heights[Y * W + X] = (uint16)FMath::Clamp(FMath::RoundToInt(V + OffsetCm / CmPerUnit), 0, 65535);
		}
	});
	// Round the high snow: repeated neighbour averaging, full strength high up, fading out below, the top itself kept.
	if (MathSmoothPasses > 0)
	{
		TArray<float> Soft;
		Soft.SetNumUninitialized(W * H);
		for (int32 I = 0; I < Soft.Num(); ++I) { Soft[I] = Heights[I]; }
		TArray<float> Weight;
		Weight.SetNumZeroed(W * H);
		ParallelFor(H, [&](int32 Y)
		{
			for (int32 X = 0; X < W; ++X)
			{
				const double High = FMath::SmoothStep(double(MathSmoothAboveM), double(MathSmoothFullAboveM), ElevationM(Base[Y * W + X]));
				// The walking route is rounded at every altitude; Protect is 0 on the line and around camps.
				Weight[Y * W + X] = FMath::Max(float(High), (1.f - Protect[Y * W + X]) * 0.85f);
			}
		});
		TArray<float> Next = Soft;
		for (int32 Pass = 0; Pass < MathSmoothPasses; ++Pass)
		{
			ParallelFor(H, [&](int32 Y)
			{
				for (int32 X = 0; X < W; ++X)
				{
					const float W0 = Weight[Y * W + X];
					if (W0 <= 0.f) { Next[Y * W + X] = Soft[Y * W + X]; continue; }
					const auto At = [&](int32 DX, int32 DY) { return Soft[FMath::Clamp(Y + DY, 0, H - 1) * W + FMath::Clamp(X + DX, 0, W - 1)]; };
					const float Avg = (At(-1, 0) + At(1, 0) + At(0, -1) + At(0, 1)) * 0.25f;
					Next[Y * W + X] = FMath::Lerp(Soft[Y * W + X], Avg, W0 * 0.7f);
				}
			});
			Swap(Soft, Next);
		}
		// Rounding lowers the top: a broad smooth rise brings it back to its true height.
		float SoftTop = 0.f;
		for (const float V : Soft) { SoftTop = FMath::Max(SoftTop, V); }
		const float Lost = float(Top) - SoftTop;
		ParallelFor(H, [&](int32 Y)
		{
			for (int32 X = 0; X < W; ++X)
			{
				const double D = FVector2D::Distance(FVector2D(X, Y), FVector2D(Peak)) * SpacingM / 220.0;
				Heights[Y * W + X] = (uint16)FMath::Clamp(FMath::RoundToInt(Soft[Y * W + X] + Lost * FMath::Exp(-D * D)), 0, 65535);
			}
		});
	}
	// Real camps stand on dug platforms: the ground under each tent is levelled, with a soft bank around it.
	int32 Pads = 0;
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		const FString Label = It->GetActorLabel();
		if (!It->ActorHasTag(TEXT("CampProp")) || Label.Contains(TEXT("KhumbuIcefall")) || !(Label.Contains(TEXT("_Tent")) || Label.Contains(TEXT("DiningTent")) || Label.Contains(TEXT("Stores")))) { continue; }
		FVector Origin, Extent;
		It->GetActorBounds(false, Origin, Extent);
		const FVector L = LandscapeTransform.InverseTransformPosition(Origin) - FVector(MinX, MinY, 0.0);
		const int32 CX = FMath::RoundToInt32(L.X), CY = FMath::RoundToInt32(L.Y);
		if (CX < 0 || CY < 0 || CX >= W || CY >= H) { continue; }
		const double Target = Heights[CY * W + CX];
		const double Full = Extent.Size2D() / 100.0 + 6.0, Edge = Full + 75.0;   // metres; a short bank made cliffs on the route where tents stand on steep ground
		const int32 Reach = FMath::CeilToInt32(Edge / SpacingM);
		for (int32 Y = FMath::Max(0, CY - Reach); Y <= FMath::Min(H - 1, CY + Reach); ++Y)
		{
			for (int32 X = FMath::Max(0, CX - Reach); X <= FMath::Min(W - 1, CX + Reach); ++X)
			{
				const double D = FVector2D(X - L.X, Y - L.Y).Size() * SpacingM;
				const double A = 1.0 - FMath::SmoothStep(Full, Edge, D);
				uint16& V = Heights[Y * W + X];
				V = (uint16)FMath::Clamp(FMath::RoundToInt(FMath::Lerp(double(V), Target, A)), 0, 65535);
			}
		}
		++Pads;
	}
	UE_LOG(LogTemp, Display, TEXT("LandscapeRebuilder: %d tent platforms"), Pads);
	Edit.SetHeightData(MinX, MinY, MaxX, MaxY, Heights.GetData(), W, true);
	Edit.Flush();
	LayerScope.Reset();
	Landscape->Modify();

	// Props and rope points move by the change since the current ground, so repeated runs stay seated.
	auto DeltaAt = [&](const FVector& P)
	{
		const FVector L = LandscapeTransform.InverseTransformPosition(P) - FVector(MinX, MinY, 0.0);
		const int32 X0 = FMath::FloorToInt32(L.X), Y0 = FMath::FloorToInt32(L.Y);
		if (X0 < 0 || Y0 < 0 || X0 >= W - 1 || Y0 >= H - 1) { return 0.0; }
		const double FX = L.X - X0, FY = L.Y - Y0;
		const auto D = [&](int32 X, int32 Y) { return (double(Heights[Y * W + X]) - double(Current[Y * W + X])) * CmPerUnit; };
		return FMath::Lerp(FMath::Lerp(D(X0, Y0), D(X0 + 1, Y0), FX), FMath::Lerp(D(X0, Y0 + 1), D(X0 + 1, Y0 + 1), FX), FY);
	};
	int32 Moved = 0;
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		AActor* Actor = *It;
		if (Actor == this || Actor->IsA<ALandscapeProxy>()) { continue; }
		if (AFixedRope* Rope = Cast<AFixedRope>(Actor))
		{
			bool bChanged = false;
			for (FVector& P : Rope->RopePoints)
			{
				const double D = DeltaAt(P);
				if (FMath::Abs(D) > 1.0) { if (!bChanged) { Rope->Modify(); } P.Z += D; bChanged = true; }
			}
			if (bChanged) { Rope->RerunConstructionScripts(); ++Moved; }
			continue;
		}
		FVector Origin, Extent;
		Actor->GetActorBounds(false, Origin, Extent);
		const double D = DeltaAt(Actor->GetActorLocation());
		if (FMath::Abs(D) > 1.0 && Extent.Size2D() < 5000.0)
		{
			Actor->Modify();
			Actor->SetActorLocation(Actor->GetActorLocation() + FVector(0.0, 0.0, D));
			++Moved;
		}
	}
	double MaxD = 0.0;
	for (int32 I = 0; I < Heights.Num(); ++I) { MaxD = FMath::Max(MaxD, FMath::Abs(double(Heights[I]) - double(Base[I])) * CmPerUnit); }
	LastResult = FString::Printf(TEXT("ApplyTerrainMath: up to %.1f m from clean terrain, %d route points kept clear, %d actors moved."), MaxD / 100.0, RoutePoints, Moved);
	UE_LOG(LogTemp, Display, TEXT("LandscapeRebuilder: %s"), *LastResult);
#endif
}

FString ALandscapeRebuilder::ApplyHeightPatch(const FString& File, bool bOnlyLower)
{
#if WITH_EDITOR
	ALandscape* Landscape = Cast<ALandscape>(BuiltLandscape.Get());
	ULandscapeInfo* Info = Landscape ? Landscape->GetLandscapeInfo() : nullptr;
	int32 MinX, MinY, MaxX, MaxY;
	if (!Info || !Info->GetLandscapeExtent(MinX, MinY, MaxX, MaxY)) { return LastResult = TEXT("ApplyHeightPatch: no BuiltLandscape."); }
	TArray<uint8> Bytes;
	if (!FFileHelper::LoadFileToArray(Bytes, *File) || Bytes.Num() < 16) { return LastResult = TEXT("ApplyHeightPatch: cannot read ") + File; }
	int32 Hdr[4];
	FMemory::Memcpy(Hdr, Bytes.GetData(), 16);
	const int32 PX = Hdr[0], PY = Hdr[1], PW = Hdr[2], PH = Hdr[3];
	if (PW <= 0 || PH <= 0 || Bytes.Num() != 16 + PW * PH * 4 || PX < 0 || PY < 0 || PX + PW > MaxX - MinX + 1 || PY + PH > MaxY - MinY + 1)
	{
		return LastResult = TEXT("ApplyHeightPatch: bad header or size.");
	}
	const float* Target = reinterpret_cast<const float*>(Bytes.GetData() + 16);

	TOptional<FScopedSetLandscapeEditingLayer> LayerScope;
	if (const ULandscapeEditLayerBase* Layer = Landscape->GetEditLayerConst(0))
	{
		LayerScope.Emplace(Landscape, Layer->GetGuid(), [Landscape] { Landscape->RequestLayersContentUpdateForceAll(); });
	}
	FLandscapeEditDataInterface Edit(Info);
	const int32 X1 = MinX + PX, Y1 = MinY + PY, X2 = X1 + PW - 1, Y2 = Y1 + PH - 1;
	TArray<uint16> Heights;
	Heights.SetNumZeroed(PW * PH);
	{
		int32 A = X1, B = Y1, C = X2, D = Y2;
		Edit.GetHeightData(A, B, C, D, Heights.GetData(), PW);
	}
	// undo file: the old heights of every vertex this patch touches
	TArray<uint8> Undo;
	Undo.SetNumUninitialized(16 + PW * PH * 4);
	FMemory::Memcpy(Undo.GetData(), Hdr, 16);
	float* UndoH = reinterpret_cast<float*>(Undo.GetData() + 16);
	int32 Changed = 0;
	double MaxDrop = 0.0;
	for (int32 I = 0; I < PW * PH; ++I)
	{
		const float T = Target[I];
		UndoH[I] = -1.f;
		if (T < 0.f) { continue; }
		const uint16 New = (uint16)FMath::Clamp(FMath::RoundToInt(T), 0, 65535);
		if (New == Heights[I] || (bOnlyLower && New > Heights[I])) { continue; }
		UndoH[I] = Heights[I];
		MaxDrop = FMath::Max(MaxDrop, double(Heights[I]) - double(New));
		Heights[I] = New;
		++Changed;
	}
	if (!FFileHelper::SaveArrayToFile(Undo, *(File + TEXT(".undo.bin")))) { return LastResult = TEXT("ApplyHeightPatch: could not write the undo file, nothing changed."); }
	Edit.SetHeightData(X1, Y1, X2, Y2, Heights.GetData(), PW, true);
	Edit.Flush();
	LayerScope.Reset();
	Landscape->Modify();
	const double CmPerUnit = Landscape->GetActorScale3D().Z / 128.0;
	LastResult = FString::Printf(TEXT("ApplyHeightPatch: %d vertices changed, deepest drop %.1f m."), Changed, MaxDrop * CmPerUnit / 100.0);
	UE_LOG(LogTemp, Display, TEXT("LandscapeRebuilder: %s"), *LastResult);
	return LastResult;
#else
	return FString();
#endif
}
