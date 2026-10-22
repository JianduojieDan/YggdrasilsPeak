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

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Orange(TEXT("/Game/Environment/Landmarks/MI_Landmark_Orange.MI_Landmark_Orange"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Metal(TEXT("/Game/Environment/Landmarks/MI_Landmark_Metal.MI_Landmark_Metal"));
	RopeMaterial = Orange.Object;
	StakeMaterial = Metal.Object;

	UStaticMesh* CylinderMesh = Cylinder.Object;
	auto MakeInstances = [this, CylinderMesh](const TCHAR* Name)
	{
		UInstancedStaticMeshComponent* Instances = CreateDefaultSubobject<UInstancedStaticMeshComponent>(Name);
		Instances->SetupAttachment(GetRootComponent());
		Instances->SetStaticMesh(CylinderMesh);
		Instances->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Instances->SetCastShadow(false);
		Instances->SetMobility(EComponentMobility::Static);
		return Instances;
	};
	Rope = MakeInstances(TEXT("Rope"));
	Stakes = MakeInstances(TEXT("Stakes"));
}

void AFixedRope::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	Rope->ClearInstances();
	Stakes->ClearInstances();
	Rope->SetMaterial(0, RopeMaterial);
	Stakes->SetMaterial(0, StakeMaterial);
	Rope->SetCullDistances(0, FMath::RoundToInt(DrawDistance));
	Stakes->SetCullDistances(0, FMath::RoundToInt(DrawDistance));

	UpdateBounds();

	// The engine cylinder is 100 cm tall and 100 cm across, centred on its pivot, along +Z.
	const float Thickness = RopeDiameter / 100.f;
	TArray<FTransform> RopePieces;
	RopePieces.Reserve(RopePoints.Num());
	for (int32 I = 0; I + 1 < RopePoints.Num(); ++I)
	{
		const FVector Span = RopePoints[I + 1] - RopePoints[I];
		const float Length = Span.Size();
		if (Length < KINDA_SMALL_NUMBER)
		{
			continue;
		}
		RopePieces.Emplace(FRotationMatrix::MakeFromZ(Span / Length).ToQuat(), RopePoints[I] + Span * 0.5f, FVector(Thickness, Thickness, Length / 100.f));
	}
	Rope->AddInstances(RopePieces, false, true);

	TArray<FTransform> StakePieces;
	const float StakeLength = StakeAbove + StakeBelow;
	for (int32 I = 0; I < RopePoints.Num(); I += FMath::Max(StakeEvery, 1))
	{
		const FVector Centre = RopePoints[I] + FVector(0.f, 0.f, (StakeAbove - StakeBelow) * 0.5f);
		StakePieces.Emplace(FQuat::Identity, Centre, FVector(0.04f, 0.04f, StakeLength / 100.f));
	}
	Stakes->AddInstances(StakePieces, false, true);
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
