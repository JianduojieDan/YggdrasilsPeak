// Copyright Epic Games, Inc. All Rights Reserved.

#include "RouteLandmark.h"

#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	UMaterialInterface* LandmarkMaterial(const TCHAR* Name)
	{
		return LoadObject<UMaterialInterface>(nullptr, *FString::Printf(TEXT("/Game/Environment/Landmarks/MI_Landmark_%s.MI_Landmark_%s"), Name, Name));
	}
}

ARouteLandmark::ARouteLandmark()
{
	PrimaryActorTick.bCanEverTick = false;
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));

	Label = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Label"));
	Label->SetupAttachment(GetRootComponent());
	Label->SetRelativeLocation(FVector(0.f, 0.f, 900.f));
	Label->SetHorizontalAlignment(EHTA_Center);
	Label->SetWorldSize(300.f);
	Label->SetTextRenderColor(FColor(255, 200, 40));
	Label->SetHiddenInGame(true);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Plane(TEXT("/Engine/BasicShapes/Plane.Plane"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	CubeMesh = Cube.Object;
	CylinderMesh = Cylinder.Object;
	PlaneMesh = Plane.Object;
	SphereMesh = Sphere.Object;
}

float ARouteLandmark::GroundZ(const FVector2D& WorldXY) const
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return GetActorLocation().Z;
	}
	FCollisionQueryParams Params(SCENE_QUERY_STAT(RouteLandmarkGround), true, this);
	FHitResult Hit;
	const FVector Start(WorldXY, GetActorLocation().Z + 20000.0);
	const FVector End(WorldXY, GetActorLocation().Z - 20000.0);
	return World->LineTraceSingleByChannel(Hit, Start, End, ECC_WorldStatic, Params) ? Hit.ImpactPoint.Z : GetActorLocation().Z;
}

UStaticMeshComponent* ARouteLandmark::AddPiece(UStaticMesh* Mesh, UMaterialInterface* Material, const FVector& LocalXY, float HeightAboveGround,
	const FRotator& Rotation, const FVector& Scale, bool bCollide)
{
	UStaticMeshComponent* Piece = NewObject<UStaticMeshComponent>(this);
	Piece->CreationMethod = EComponentCreationMethod::UserConstructionScript;
	Piece->SetStaticMesh(Mesh);
	if (Material)
	{
		Piece->SetMaterial(0, Material);
	}
	Piece->SetCollisionProfileName(bCollide ? UCollisionProfile::BlockAll_ProfileName : UCollisionProfile::NoCollision_ProfileName);
	Piece->SetupAttachment(GetRootComponent());

	// Snap to the ground under this piece; yaw follows the actor, Z is absolute.
	const FVector World = GetActorTransform().TransformPosition(FVector(LocalXY.X, LocalXY.Y, 0.0));
	const FVector GroundPoint(World.X, World.Y, GroundZ(FVector2D(World)) + HeightAboveGround);
	Piece->SetWorldTransform(FTransform(GetActorRotation() + Rotation, GroundPoint, Scale));
	Piece->RegisterComponent();
	AddInstanceComponent(Piece);
	Pieces.Add(Piece);
	return Piece;
}

void ARouteLandmark::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	// Native actors don't get their construction-made components cleaned up like Blueprints do, and the
	// pieces are saved with the level, so remove every previously built piece before rebuilding.
	TInlineComponentArray<UStaticMeshComponent*> Old(this);
	for (UStaticMeshComponent* Piece : Old)
	{
		if (Piece && Piece->CreationMethod != EComponentCreationMethod::Native)
		{
			RemoveInstanceComponent(Piece);
			Piece->DestroyComponent();
		}
	}
	Pieces.Reset();
	Label->SetText(FText::FromString(LandmarkName));

	UMaterialInterface* TentColors[] = { LandmarkMaterial(TEXT("Orange")), LandmarkMaterial(TEXT("Yellow")), LandmarkMaterial(TEXT("Red")), LandmarkMaterial(TEXT("Green")) };
	UMaterialInterface* FlagColors[] = { LandmarkMaterial(TEXT("Blue")), LandmarkMaterial(TEXT("White")), LandmarkMaterial(TEXT("Red")), LandmarkMaterial(TEXT("Green")), LandmarkMaterial(TEXT("Yellow")) };
	UMaterialInterface* Wood = LandmarkMaterial(TEXT("Wood"));
	UMaterialInterface* Metal = LandmarkMaterial(TEXT("Metal"));
	UMaterialInterface* White = LandmarkMaterial(TEXT("White"));

	FRandomStream Rand(Seed);

	// Tents: a cube rolled 45 degrees and half buried reads as a ridge tent (~1.1 m tall, 2.2 m long).
	const int32 Tents = Kind == ERouteLandmarkKind::Summit ? 0 : NumTents;
	for (int32 I = 0; I < Tents; ++I)
	{
		const float Angle = (I / float(FMath::Max(Tents, 1))) * 2.f * PI + Rand.FRandRange(-0.3f, 0.3f);
		const float Radius = Rand.FRandRange(500.f, 1100.f);
		const FVector Pos(FMath::Cos(Angle) * Radius, FMath::Sin(Angle) * Radius, 0.f);
		// Centre slightly above ground so the ridge stands ~TentScale.Z * 0.7 m + 0.2 m tall.
		AddPiece(CubeMesh, TentColors[I % UE_ARRAY_COUNT(TentColors)], Pos, 20.f,
			FRotator(0.f, Rand.FRandRange(0.f, 180.f), 45.f), TentScale, true);
	}

	// Pole with a string of prayer flags running down to the snow.
	const float PoleHeight = 450.f;
	AddPiece(CylinderMesh, Wood, FVector::ZeroVector, PoleHeight * 0.5f, FRotator::ZeroRotator, FVector(0.1f, 0.1f, PoleHeight / 100.f), true);
	const int32 Strings = Kind == ERouteLandmarkKind::Summit ? 6 : 2;
	for (int32 S = 0; S < Strings; ++S)
	{
		const float Dir = (S / float(Strings)) * 2.f * PI + 0.4f;
		const FVector Anchor(FMath::Cos(Dir) * 900.f, FMath::Sin(Dir) * 900.f, 0.f);
		const int32 Flags = 14;
		for (int32 F = 1; F <= Flags; ++F)
		{
			const float T = F / float(Flags + 1);
			// Height above local ground: from the pole top down to the anchor, with a little sag.
			const float Height = FMath::Lerp(PoleHeight, 40.f, T) - FMath::Sin(T * PI) * 60.f;
			AddPiece(PlaneMesh, FlagColors[F % UE_ARRAY_COUNT(FlagColors)], Anchor * T, Height,
				FRotator(0.f, FMath::RadiansToDegrees(Dir) + 90.f, 90.f), FVector(FlagSize.X, FlagSize.Y, 1.f), false);
		}
	}

	if (Kind == ERouteLandmarkKind::Summit)
	{
		// Snow cairn under the flags.
		AddPiece(SphereMesh, White, FVector::ZeroVector, 20.f, FRotator::ZeroRotator, FVector(2.2f, 2.2f, 1.2f), true);
	}

	if (Kind == ERouteLandmarkKind::Icefall)
	{
		// Aluminium ladder laid across the ice: two rails, rungs every 35 cm.
		const FVector LadderPos(1500.f, 0.f, 0.f);
		const float Length = 600.f;
		for (int32 Side = -1; Side <= 1; Side += 2)
		{
			AddPiece(CylinderMesh, Metal, LadderPos + FVector(0.f, Side * 25.f, 0.f), 15.f, FRotator(90.f, 0.f, 0.f), FVector(0.05f, 0.05f, Length / 100.f), true);
		}
		for (float X = -Length * 0.5f + 20.f; X < Length * 0.5f; X += 35.f)
		{
			AddPiece(CylinderMesh, Metal, LadderPos + FVector(X, 0.f, 0.f), 15.f, FRotator(0.f, 0.f, 90.f), FVector(0.03f, 0.03f, 0.5f), true);
		}
	}
}
