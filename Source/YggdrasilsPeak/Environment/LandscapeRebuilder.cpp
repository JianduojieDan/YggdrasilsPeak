// Copyright Epic Games, Inc. All Rights Reserved.

#include "LandscapeRebuilder.h"

#include "Engine/World.h"
#include "Landscape.h"
#include "LandscapeProxy.h"
#include "Async/ParallelFor.h"
#include "EngineUtils.h"
#include "Environment/FixedRope.h"
#include "Survival/SurvivalConfig.h"

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
