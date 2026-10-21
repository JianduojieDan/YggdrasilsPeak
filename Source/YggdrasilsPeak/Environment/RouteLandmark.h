// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RouteLandmark.generated.h"

class UStaticMesh;
class UStaticMeshComponent;
class UMaterialInterface;
class UTextRenderComponent;

UENUM(BlueprintType)
enum class ERouteLandmarkKind : uint8
{
	/** A few tents and a prayer-flag pole. */
	Camp,
	/** Camp plus a ladder laid across the ice. */
	Icefall,
	/** Prayer flags strung around a snow cairn, no tents. */
	Summit
};

/**
 *  Placeholder route marker built from engine basic shapes (no art assets needed): tents, a pole with
 *  prayer flags, an optional ladder. Every piece is snapped to the ground below it when the actor is built.
 *  The name label is editor-only, so screenshots show which camp is which but the game stays clean.
 */
UCLASS()
class YGGDRASILSPEAK_API ARouteLandmark : public AActor
{
	GENERATED_BODY()

public:

	ARouteLandmark();

	UPROPERTY(EditAnywhere, Category="Landmark")
	FString LandmarkName = TEXT("Camp");

	UPROPERTY(EditAnywhere, Category="Landmark")
	ERouteLandmarkKind Kind = ERouteLandmarkKind::Camp;

	UPROPERTY(EditAnywhere, Category="Landmark", meta=(ClampMin="0", ClampMax="12"))
	int32 NumTents = 4;

	/** Reaching this camp after the summit completes the expedition (Everest Base Camp). */
	UPROPERTY(EditAnywhere, Category="Landmark")
	bool bFinishLine = false;

	/** Tent box scale (length, width, height) before the 45-degree roll; 1 = 1 m cube. */
	UPROPERTY(EditAnywhere, Category="Landmark")
	FVector TentScale = FVector(2.6f, 1.7f, 1.7f);

	/** Prayer flag size in metres (width, height). */
	UPROPERTY(EditAnywhere, Category="Landmark")
	FVector2D FlagSize = FVector2D(0.45f, 0.32f);

	/** Changes the tent layout and colours. */
	UPROPERTY(EditAnywhere, Category="Landmark")
	int32 Seed = 1;

	virtual void OnConstruction(const FTransform& Transform) override;

private:

	UStaticMeshComponent* AddPiece(UStaticMesh* Mesh, UMaterialInterface* Material, const FVector& LocalXY, float HeightAboveGround,
		const FRotator& Rotation, const FVector& Scale, bool bCollide);

	/** Ground height (world Z) under a world XY, or the actor's Z if nothing is hit. */
	float GroundZ(const FVector2D& WorldXY) const;

	UPROPERTY(VisibleAnywhere, Category="Landmark")
	TObjectPtr<UTextRenderComponent> Label;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UStaticMeshComponent>> Pieces;

	UPROPERTY()
	TObjectPtr<UStaticMesh> CubeMesh;

	UPROPERTY()
	TObjectPtr<UStaticMesh> CylinderMesh;

	UPROPERTY()
	TObjectPtr<UStaticMesh> PlaneMesh;

	UPROPERTY()
	TObjectPtr<UStaticMesh> SphereMesh;
};
