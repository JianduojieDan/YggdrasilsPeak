// Copyright Epic Games, Inc. All Rights Reserved.

#include "Environment/ConstrainedScatter.h"

#include "Environment/FixedRope.h"
#include "Environment/RouteLandmark.h"
#include "Survival/OxygenPickup.h"
#include "Survival/SupplyPickup.h"
#include "Survival/SurvivalConfig.h"
#include "Components/BoxComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "HAL/PlatformTime.h"
#include "LandscapeProxy.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace
{
	const TCHAR* RejectName(EScatterReject Reason)
	{
		switch (Reason)
		{
		case EScatterReject::RouteCorridor: return TEXT("route_corridor");
		case EScatterReject::ProtectedSpot: return TEXT("protected_spot");
		case EScatterReject::NoGround:      return TEXT("no_ground");
		case EScatterReject::Overlap:       return TEXT("overlap");
		case EScatterReject::Elevation:     return TEXT("elevation");
		case EScatterReject::Slope:         return TEXT("slope");
		case EScatterReject::SurfaceMask:   return TEXT("surface_mask");
		case EScatterReject::FarFromRoute:  return TEXT("far_from_route");
		default:                            return TEXT("?");
		}
	}

	FString LabelOf(const AActor* Actor)
	{
#if WITH_EDITOR
		return Actor->GetActorLabel();
#else
		return Actor->GetName();
#endif
	}

	/** Heightmap grid the masks are drawn on (see Tools/RoutePlanning/glac_mask.py). */
	constexpr double GridOrigin = -504000.0;
	constexpr double GridExtent = 3374.24 * 1009.0;
}

AConstrainedScatter::AConstrainedScatter()
{
	PrimaryActorTick.bCanEverTick = false;
	Area = CreateDefaultSubobject<UBoxComponent>(TEXT("Area"));
	Area->SetBoxExtent(FVector(50000.f, 50000.f, 50000.f));
	Area->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetRootComponent(Area);
}

void AConstrainedScatter::GatherProtected()
{
	Ropes.Reset();
	Camps.Reset();
	Spots.Reset();
	SpotBoxes.Reset();
	const FBox Box = Area->Bounds.GetBox().ExpandBy(FMath::Max(CampClearCm, RouteCorridorCm));
	for (TActorIterator<AFixedRope> It(GetWorld()); It; ++It)
	{
		Ropes.Add(*It);
	}
	for (TActorIterator<ARouteLandmark> It(GetWorld()); It; ++It)
	{
		if (Box.IsInsideXY(It->GetActorLocation()))
		{
			Camps.Add(It->GetActorLocation());
		}
	}
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		if ((It->IsA<ASupplyPickup>() || It->IsA<AOxygenPickup>() || It->ActorHasTag(TEXT("ScatterProtect"))) && Box.IsInsideXY(It->GetActorLocation()))
		{
			Spots.Add(It->GetActorLocation());
		}
		// whole structures (crevasse crossings): keep clear of their footprint, not just a point
		if (It->ActorHasTag(TEXT("ScatterProtectBox")))
		{
			const FBox Bounds = It->GetComponentsBoundingBox(true);
			if (Bounds.IsValid && Box.IntersectXY(Bounds))
			{
				SpotBoxes.Add(Bounds);
			}
		}
	}

	MaskPixels.Reset();
	MaskSize = 0;
#if WITH_EDITORONLY_DATA
	if (SurfaceMask && SurfaceMask->Source.IsValid() && SurfaceMask->Source.GetFormat() == TSF_G8)
	{
		TArray64<uint8> Data;
		SurfaceMask->Source.GetMipData(Data, 0);
		MaskSize = SurfaceMask->Source.GetSizeX();
		MaskPixels.Append(Data.GetData(), int32(Data.Num()));
	}
#endif
}

EScatterReject AConstrainedScatter::CheckProtected(const FVector& Location, float Radius) const
{
	for (const TWeakObjectPtr<AFixedRope>& Rope : Ropes)
	{
		if (Rope.IsValid() && Rope->IsNear(Location, RouteCorridorCm + Radius) && Rope->DistanceToRope(Location) < RouteCorridorCm + Radius)
		{
			return EScatterReject::RouteCorridor;
		}
	}
	for (const FVector& Camp : Camps)
	{
		if (FVector::DistSquared2D(Location, Camp) < FMath::Square(CampClearCm + Radius))
		{
			return EScatterReject::ProtectedSpot;
		}
	}
	for (const FBox& SpotBox : SpotBoxes)
	{
		if (SpotBox.ExpandBy(FVector(Radius + SpotClearCm * 0.25f, Radius + SpotClearCm * 0.25f, 0.f)).IsInsideXY(Location))
		{
			return EScatterReject::ProtectedSpot;
		}
	}
	for (const FVector& Spot : Spots)
	{
		if (FVector::DistSquared2D(Location, Spot) < FMath::Square(SpotClearCm + Radius))
		{
			return EScatterReject::ProtectedSpot;
		}
	}
	return EScatterReject::Count;
}

bool AConstrainedScatter::SampleMask(const FVector& Location, float& Out) const
{
	if (MaskSize <= 0)
	{
		return false;
	}
	const int32 X = FMath::FloorToInt32((Location.X - GridOrigin) / GridExtent * MaskSize);
	const int32 Y = FMath::FloorToInt32((Location.Y - GridOrigin) / GridExtent * MaskSize);
	Out = (X >= 0 && Y >= 0 && X < MaskSize && Y < MaskSize) ? MaskPixels[Y * MaskSize + X] / 255.f : 0.f;
	return true;
}

UHierarchicalInstancedStaticMeshComponent* AConstrainedScatter::GetInstancer(int32 MeshIndex)
{
	while (Instancers.Num() <= MeshIndex)
	{
		UHierarchicalInstancedStaticMeshComponent* H = NewObject<UHierarchicalInstancedStaticMeshComponent>(this,
			*FString::Printf(TEXT("Placements_%d"), Instancers.Num()), RF_Transactional);
		H->SetupAttachment(GetRootComponent());
		H->SetMobility(EComponentMobility::Static);
		H->SetCollisionProfileName(TEXT("BlockAll"));
		AddInstanceComponent(H);
		H->RegisterComponent();
		Instancers.Add(H);
	}
	UHierarchicalInstancedStaticMeshComponent* H = Instancers[MeshIndex];
	H->SetStaticMesh(Meshes[MeshIndex].Mesh);
	return H;
}

FVector2D AConstrainedScatter::FootprintOf(const FBoxSphereBounds& MeshBounds, const FTransform& T, float& OutRadius)
{
	const FBox World = MeshBounds.GetBox().TransformBy(T);
	OutRadius = float(FMath::Max(World.GetExtent().X, World.GetExtent().Y));
	return FVector2D(World.GetCenter());
}

void AConstrainedScatter::Clear()
{
	for (UHierarchicalInstancedStaticMeshComponent* H : Instancers)
	{
		if (H)
		{
			H->ClearInstances();
		}
	}
}

void AConstrainedScatter::Generate()
{
	Modify();
	Clear();
	GatherProtected();
	const double Start = FPlatformTime::Seconds();

	float TotalWeight = 0.f;
	for (const FScatterMesh& M : Meshes)
	{
		TotalWeight += M.Mesh ? M.Weight : 0.f;
	}
	LastRejected.Init(0, int32(EScatterReject::Count));
	LastAccepted = 0;
	if (TotalWeight <= 0.f)
	{
		UE_LOG(LogTemp, Warning, TEXT("%s: no meshes to place."), *GetName());
		return;
	}

	const USurvivalConfig* Cfg = GetDefault<USurvivalConfig>();
	const FBox Box = Area->Bounds.GetBox();
	const float MaxSlopeCos = FMath::Cos(FMath::DegreesToRadians(MaxSlopeDeg));
	FRandomStream Rng(Seed);
	TArray<FPlaced> Placed;
	// spatial hash for the overlap rule
	const float Cell = 2000.f;
	TMultiMap<FIntPoint, int32> Grid;
	auto CellOf = [Cell](const FVector2D& P) { return FIntPoint(FMath::FloorToInt32(P.X / Cell), FMath::FloorToInt32(P.Y / Cell)); };

	const int32 MaxAttempts = TargetCount * AttemptsPerPlacement;
	int32 Attempts = 0;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(ConstrainedScatter), false, this);
	while (LastAccepted < TargetCount && Attempts < MaxAttempts)       // rule 5: density
	{
		++Attempts;
		// pick mesh and size first: the footprint radius takes part in every distance rule
		float Pick = Rng.FRand() * TotalWeight;
		int32 MeshIndex = 0;
		for (; MeshIndex < Meshes.Num() - 1; ++MeshIndex)
		{
			Pick -= Meshes[MeshIndex].Mesh ? Meshes[MeshIndex].Weight : 0.f;
			if (Pick <= 0.f)
			{
				break;
			}
		}
		const FScatterMesh& M = Meshes[MeshIndex];
		if (!M.Mesh)
		{
			continue;
		}
		const float Scale = Rng.FRandRange(M.MinScale, M.MaxScale);
		const FBoxSphereBounds MB = M.Mesh->GetBounds();
		// pre-check footprint around the pivot, lean included (the exact one, tilt included, is tested once it stands)
		const float Radius = (FVector2D(MB.Origin.X, MB.Origin.Y).Size() + FMath::Max(MB.BoxExtent.X, MB.BoxExtent.Y)) * Scale;
		const FVector Try(Rng.FRandRange(Box.Min.X, Box.Max.X), Rng.FRandRange(Box.Min.Y, Box.Max.Y), Box.Max.Z);

		// 1 + 2 (cheap, on the 2D point)
		const EScatterReject Protected = CheckProtected(FVector(Try.X, Try.Y, Box.GetCenter().Z), Radius);
		if (Protected != EScatterReject::Count)
		{
			++LastRejected[int32(Protected)];
			continue;
		}
		// 3: on the landscape
		FHitResult Hit;
		if (!GetWorld()->LineTraceSingleByChannel(Hit, Try, FVector(Try.X, Try.Y, Box.Min.Z), ECC_Visibility, Params)
			|| !Cast<ALandscapeProxy>(Hit.GetActor()))
		{
			++LastRejected[int32(EScatterReject::NoGround)];
			continue;
		}
		// 4: suitability
		const float ElevationM = Cfg->ReferenceElevationMeters + float(Hit.ImpactPoint.Z - Cfg->ReferenceWorldZ) / 100.f;
		if (ElevationM < MinElevationM || ElevationM > MaxElevationM)
		{
			++LastRejected[int32(EScatterReject::Elevation)];
			continue;
		}
		if (Hit.ImpactNormal.Z < MaxSlopeCos)
		{
			++LastRejected[int32(EScatterReject::Slope)];
			continue;
		}
		float MaskValue = 1.f;
		if (SampleMask(Hit.ImpactPoint, MaskValue) && MaskValue < MaskThreshold)
		{
			++LastRejected[int32(EScatterReject::SurfaceMask)];
			continue;
		}

		if (MaxRouteDistanceCm > 0.f)
		{
			bool bNear = false;
			for (const TWeakObjectPtr<AFixedRope>& Rope : Ropes)
			{
				if (Rope.IsValid() && Rope->IsNear(Hit.ImpactPoint, MaxRouteDistanceCm) && Rope->DistanceToRope(Hit.ImpactPoint) < MaxRouteDistanceCm)
				{
					bNear = true;
					break;
				}
			}
			if (!bNear)
			{
				++LastRejected[int32(EScatterReject::FarFromRoute)];
				continue;
			}
		}

		// the instance as it will stand: tilted half-way to the ground normal, random yaw, sunk a little
		const FQuat Tilt = FQuat::FindBetweenNormals(FVector::UpVector, FMath::Lerp(FVector::UpVector, Hit.ImpactNormal, 0.5f).GetSafeNormal());
		const FQuat Yaw(FVector::UpVector, Rng.FRandRange(0.f, UE_TWO_PI));
		const float Sink = MB.BoxExtent.Z * 2.f * Scale * M.EmbedFraction;
		const FTransform T(Tilt * Yaw, Hit.ImpactPoint - Hit.ImpactNormal * Sink - (MB.Origin.Z - MB.BoxExtent.Z) * Scale * FVector::UpVector, FVector(Scale));
		// its real footprint: leaning meshes and the tilt move the top sideways, far beyond the pivot
		float Footprint = 0.f;
		const FVector2D P = FootprintOf(MB, T, Footprint);
		const FVector Ground(P, Hit.ImpactPoint.Z);

		// 1 again with the real footprint and the ground height (ropes are 3D lines)
		if (CheckProtected(Ground, Footprint) != EScatterReject::Count)
		{
			++LastRejected[int32(CheckProtected(Ground, Footprint))];
			continue;
		}
		// 3: overlap
		bool bOverlap = false;
		const FIntPoint C = CellOf(P);
		for (int32 dx = -1; dx <= 1 && !bOverlap; ++dx)
		{
			for (int32 dy = -1; dy <= 1 && !bOverlap; ++dy)
			{
				for (auto It = Grid.CreateConstKeyIterator(C + FIntPoint(dx, dy)); It; ++It)
				{
					const FPlaced& O = Placed[It.Value()];
					if (FVector2D::DistSquared(O.Location, P) < FMath::Square(O.Radius + Footprint + MinGapCm))
					{
						bOverlap = true;
						break;
					}
				}
			}
		}
		if (bOverlap)
		{
			++LastRejected[int32(EScatterReject::Overlap)];
			continue;
		}

		GetInstancer(MeshIndex)->AddInstance(T, true);
		Grid.Add(C, Placed.Add({ P, Footprint, float(Hit.ImpactPoint.Z) }));
		++LastAccepted;
	}

	LastMilliseconds = float((FPlatformTime::Seconds() - Start) * 1000.0);
	int32 Violations = 0;
	for (const FPlaced& O : Placed)
	{
		Violations += CheckProtected(FVector(O.Location, O.GroundZ), O.Radius) != EScatterReject::Count ? 1 : 0;
	}
	WriteReport(Attempts, Violations);
	UE_LOG(LogTemp, Display, TEXT("%s seed %d: %d / %d placed in %.1f ms from %d candidates, report %s"), *GetName(), Seed, LastAccepted, TargetCount,
		LastMilliseconds, Attempts, *LastReportPath);
}

void AConstrainedScatter::Verify()
{
	GatherProtected();
	int32 Total = 0;
	TArray<int32> Broken;
	Broken.Init(0, int32(EScatterReject::Count));
	for (UHierarchicalInstancedStaticMeshComponent* H : Instancers)
	{
		for (int32 i = 0; H && i < H->GetInstanceCount(); ++i)
		{
			FTransform T;
			H->GetInstanceTransform(i, T, true);
			float Footprint = 0.f;
			const FVector2D P = FootprintOf(H->GetStaticMesh()->GetBounds(), T, Footprint);
			const EScatterReject R = CheckProtected(FVector(P, T.GetLocation().Z), Footprint);
			++Total;
			if (R != EScatterReject::Count)
			{
				++Broken[int32(R)];
			}
		}
	}
	UE_LOG(LogTemp, Display, TEXT("%s verify: %d placements, %d in the route corridor, %d on protected spots"), *GetName(), Total,
		Broken[int32(EScatterReject::RouteCorridor)], Broken[int32(EScatterReject::ProtectedSpot)]);
}

void AConstrainedScatter::ShowProtectedAreas()
{
	GatherProtected();
	const float Life = 60.f;
	const FBox Box = Area->Bounds.GetBox();
	for (const TWeakObjectPtr<AFixedRope>& Rope : Ropes)
	{
		for (int32 i = 0; Rope.IsValid() && i + 1 < Rope->RopePoints.Num(); i += 2)
		{
			if (Box.ExpandBy(RouteCorridorCm).IsInsideXY(Rope->RopePoints[i]))
			{
				DrawDebugCylinder(GetWorld(), Rope->RopePoints[i], Rope->RopePoints[i] + FVector(0, 0, 300), RouteCorridorCm, 12, FColor::Orange, false, Life);
			}
		}
	}
	for (const FVector& Camp : Camps)
	{
		DrawDebugCylinder(GetWorld(), Camp, Camp + FVector(0, 0, 500), CampClearCm, 32, FColor::Green, false, Life);
	}
	for (const FVector& Spot : Spots)
	{
		DrawDebugCylinder(GetWorld(), Spot, Spot + FVector(0, 0, 300), SpotClearCm, 16, FColor::Cyan, false, Life);
	}
	DrawDebugBox(GetWorld(), Box.GetCenter(), Box.GetExtent(), FColor::White, false, Life);
}

void AConstrainedScatter::WriteReport(int32 Attempts, int32 Violations)
{
	FString Rejected;
	int32 RejectedTotal = 0;
	for (int32 i = 0; i < LastRejected.Num(); ++i)
	{
		Rejected += FString::Printf(TEXT("%s\"%s\": %d"), i ? TEXT(", ") : TEXT(""), RejectName(EScatterReject(i)), LastRejected[i]);
		RejectedTotal += LastRejected[i];
	}
	const FBox Box = Area->Bounds.GetBox();
	const FString Json = FString::Printf(TEXT(
		"{\n  \"actor\": \"%s\",\n  \"seed\": %d,\n  \"target\": %d,\n  \"attempts\": %d,\n  \"accepted\": %d,\n  \"rejected_total\": %d,\n"
		"  \"rejected\": {%s},\n  \"milliseconds\": %.2f,\n  \"post_check_violations\": %d,\n"
		"  \"settings\": {\"route_corridor_cm\": %.0f, \"camp_clear_cm\": %.0f, \"spot_clear_cm\": %.0f, \"min_gap_cm\": %.0f,"
		" \"elevation_m\": [%.0f, %.0f], \"max_slope_deg\": %.1f, \"mask\": \"%s\", \"mask_threshold\": %.2f, \"max_route_distance_cm\": %.0f, \"meshes\": %d},\n"
		"  \"area\": {\"min\": [%.0f, %.0f, %.0f], \"max\": [%.0f, %.0f, %.0f]},\n  \"protected\": {\"ropes\": %d, \"camps\": %d, \"spots\": %d}\n}\n"),
		*LabelOf(this), Seed, TargetCount, Attempts, LastAccepted, RejectedTotal, *Rejected, LastMilliseconds, Violations,
		RouteCorridorCm, CampClearCm, SpotClearCm, MinGapCm, MinElevationM, MaxElevationM, MaxSlopeDeg,
		SurfaceMask ? *SurfaceMask->GetName() : TEXT(""), MaskThreshold, MaxRouteDistanceCm, Meshes.Num(),
		Box.Min.X, Box.Min.Y, Box.Min.Z, Box.Max.X, Box.Max.Y, Box.Max.Z, Ropes.Num(), Camps.Num(), Spots.Num());
	const FString Path = FPaths::ProjectSavedDir() / TEXT("ScatterReports") / FString::Printf(TEXT("%s_seed%d.json"), *LabelOf(this), Seed);
	FFileHelper::SaveStringToFile(Json, *Path);
	LastReportPath = FPaths::ConvertRelativePathToFull(Path);
}
