// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FixedRope.generated.h"

class UInstancedStaticMeshComponent;
class UMaterialInterface;
class UStaticMesh;

/**
 *  A fixed rope laid along the climbing route: the rope runs through RopePoints (world space, already
 *  following the ground) and is tied to a stake every few points. Both are drawn as instances of one
 *  cylinder each, so a whole leg of the route is two draw calls.
 *  Players near the rope are clipped in (see UExpeditionComponent), which makes steep ground walkable.
 */
UCLASS()
class YGGDRASILSPEAK_API AFixedRope : public AActor
{
	GENERATED_BODY()

public:

	AFixedRope();

	/** Rope path in world space, a few metres apart. Usually written by the route-planning script. */
	UPROPERTY(EditAnywhere, Category="Fixed Rope")
	TArray<FVector> RopePoints;

	/** A stake is placed at every Nth rope point. */
	UPROPERTY(EditAnywhere, Category="Fixed Rope", meta=(ClampMin="1"))
	int32 StakeEvery = 5;

	/** Rope thickness, cm. */
	UPROPERTY(EditAnywhere, Category="Fixed Rope", meta=(ClampMin="0.2"))
	float RopeDiameter = 2.f;

	/** How far the stake top stands above the rope point, and how deep it goes into the snow, cm. */
	UPROPERTY(EditAnywhere, Category="Fixed Rope", meta=(ClampMin="0"))
	float StakeAbove = 25.f;

	UPROPERTY(EditAnywhere, Category="Fixed Rope", meta=(ClampMin="0"))
	float StakeBelow = 60.f;

	/** Beyond this distance (cm) rope and stakes are not drawn. */
	UPROPERTY(EditAnywhere, Category="Fixed Rope", meta=(ClampMin="0"))
	float DrawDistance = 40000.f;

	UPROPERTY(EditAnywhere, Category="Fixed Rope")
	TObjectPtr<UMaterialInterface> RopeMaterial;

	UPROPERTY(EditAnywhere, Category="Fixed Rope")
	TObjectPtr<UMaterialInterface> StakeMaterial;

	virtual void OnConstruction(const FTransform& Transform) override;

	/** Distance (cm) from Location to the nearest point on the rope; OutDirection gets the rope's direction there. */
	float DistanceToRope(const FVector& Location, FVector* OutDirection = nullptr) const;

	/** Quick reject: bounds of all rope points grown by Margin. */
	bool IsNear(const FVector& Location, float Margin) const { return RopeBounds.IsValid && RopeBounds.ExpandBy(Margin).IsInsideOrOn(Location); }

protected:

	virtual void BeginPlay() override;

private:

	void UpdateBounds();

	UPROPERTY(VisibleAnywhere, Category="Fixed Rope")
	TObjectPtr<UInstancedStaticMeshComponent> Rope;

	UPROPERTY(VisibleAnywhere, Category="Fixed Rope")
	TObjectPtr<UInstancedStaticMeshComponent> Stakes;

	FBox RopeBounds;
};
