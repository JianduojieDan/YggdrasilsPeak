// Copyright Epic Games, Inc. All Rights Reserved.

#include "FixedRope.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

AFixedRope::AFixedRope()
{
	PrimaryActorTick.bCanEverTick = false;
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));

	// The look assets are NOT loaded here (ConstructorHelpers): objects the class default loads at startup are kept
	// out of garbage collection, and rebuilding the rope material in the editor then asserts (!IsRooted, 2026-11-13).
	// OnConstruction fills any that are unset from the default paths below.

	auto MakeInstances = [this](const TCHAR* Name, bool bShadow)
	{
		UInstancedStaticMeshComponent* Instances = CreateDefaultSubobject<UInstancedStaticMeshComponent>(Name);
		Instances->SetupAttachment(GetRootComponent());
		Instances->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Instances->SetCastShadow(bShadow);
		Instances->SetMobility(EComponentMobility::Static);
		Instances->bVisibleInRayTracing = false;      // thin, many instances: not worth the ray-tracing scene
		return Instances;
	};
	Rope = MakeInstances(TEXT("Rope"), true);
	OldRope = MakeInstances(TEXT("OldRope"), true);
	Stakes = MakeInstances(TEXT("Stakes"), true);
	Screws = MakeInstances(TEXT("Screws"), true);
	Pitons = MakeInstances(TEXT("Pitons"), true);
}

void AFixedRope::AddPieces(UInstancedStaticMeshComponent* Target, const TArray<FVector>& Points, float Diameter) const
{
	// The rope piece is 100 cm long and 100 cm across, centred on its pivot, along +Z (like the engine cylinder).
	const float Thickness = Diameter / 100.f;
	TArray<FTransform> Pieces;
	Pieces.Reserve(Points.Num());
	for (int32 I = 0; I + 1 < Points.Num(); ++I)
	{
		const FVector Span = Points[I + 1] - Points[I];
		const float Length = Span.Size();
		if (Length < KINDA_SMALL_NUMBER || Length > MaxPieceCm)
		{
			continue;
		}
		// a hair longer than the gap so the joints close where the rope bends
		Pieces.Emplace(FRotationMatrix::MakeFromZ(Span / Length).ToQuat(), Points[I] + Span * 0.5f, FVector(Thickness, Thickness, (Length + Diameter * 0.5f) / 100.f));
	}
	Target->AddInstances(Pieces, false, true);
}

void AFixedRope::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	UpdateBounds();
	UStaticMesh* Cylinder = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	auto Default = [](auto& Slot, const TCHAR* Path)
	{
		using T = typename TRemovePointer<decltype(Slot.Get())>::Type;
		if (!Slot)
		{
			Slot = LoadObject<T>(nullptr, Path);
		}
	};
	Default(RopeMesh, TEXT("/Game/Environment/FixedRope/SM_FixedRope_Piece.SM_FixedRope_Piece"));
	Default(PicketMesh, TEXT("/Game/Environment/FixedRope/SM_RopeAnchor_Picket.SM_RopeAnchor_Picket"));
	Default(ScrewMesh, TEXT("/Game/Environment/FixedRope/SM_RopeAnchor_Screw.SM_RopeAnchor_Screw"));
	Default(PitonMesh, TEXT("/Game/Environment/FixedRope/SM_RopeAnchor_Piton.SM_RopeAnchor_Piton"));
	Default(RopeMaterial, TEXT("/Game/Environment/FixedRope/MI_FixedRope_New.MI_FixedRope_New"));
	Default(OldRopeMaterial, TEXT("/Game/Environment/FixedRope/MI_FixedRope_Old.MI_FixedRope_Old"));

	for (UInstancedStaticMeshComponent* C : { Rope.Get(), OldRope.Get(), Stakes.Get(), Screws.Get(), Pitons.Get() })
	{
		C->ClearInstances();
	}
	Rope->SetStaticMesh(RopeMesh ? RopeMesh.Get() : Cylinder);
	OldRope->SetStaticMesh(RopeMesh ? RopeMesh.Get() : Cylinder);
	Stakes->SetStaticMesh(PicketMesh);
	Screws->SetStaticMesh(ScrewMesh);
	Pitons->SetStaticMesh(PitonMesh);
	Rope->SetMaterial(0, RopeMaterial);
	OldRope->SetMaterial(0, OldRopeMaterial ? OldRopeMaterial.Get() : RopeMaterial.Get());
	Rope->SetCullDistances(0, FMath::RoundToInt(DrawDistance));
	OldRope->SetCullDistances(0, FMath::RoundToInt(DrawDistance * 0.5f));
	for (UInstancedStaticMeshComponent* C : { Stakes.Get(), Screws.Get(), Pitons.Get() })
	{
		C->SetCullDistances(0, FMath::RoundToInt(AnchorDrawDistance));
	}

	if (LayPoints.Num() >= 2)
	{
		AddPieces(Rope, LayPoints, RopeDiameter);
		AddPieces(OldRope, OldLayPoints, OldRopeDiameter);
		Stakes->AddInstances(PicketAnchors, false, true);
		Screws->AddInstances(ScrewAnchors, false, true);
		Pitons->AddInstances(PitonAnchors, false, true);
	}
	else
	{
		// not laid yet (fresh from place_ropes): the plain route line
		AddPieces(Rope, RopePoints, RopeDiameter);
	}
}

void AFixedRope::BeginPlay()
{
	Super::BeginPlay();
	// Construction does not rerun for saved actors in a packaged game, and the bounds are not saved.
	UpdateBounds();
}

void AFixedRope::UpdateBounds()
{
	// FBox(TArray) reads element 0, so an empty rope (freshly spawned) needs its own case.
	RopeBounds = RopePoints.Num() > 0 ? FBox(RopePoints) : FBox(ForceInit);
}

float AFixedRope::DistanceToRope(const FVector& Location, FVector* OutDirection) const
{
	float BestSq = TNumericLimits<float>::Max();
	for (int32 I = 0; I + 1 < RopePoints.Num(); ++I)
	{
		const FVector Closest = FMath::ClosestPointOnSegment(Location, RopePoints[I], RopePoints[I + 1]);
		const float DistSq = FVector::DistSquared(Location, Closest);
		if (DistSq < BestSq)
		{
			BestSq = DistSq;
			if (OutDirection)
			{
				*OutDirection = (RopePoints[I + 1] - RopePoints[I]).GetSafeNormal();
			}
		}
	}
	return FMath::Sqrt(BestSq);
}
