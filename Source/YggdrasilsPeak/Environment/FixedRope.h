// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FixedRope.generated.h"

class UInstancedStaticMeshComponent;
class UMaterialInterface;
class UStaticMesh;

/**
 *  A fixed rope laid along the climbing route, the way the real ones on Everest lie (2026-11-13 rebuild; the old
 *  orange cylinders on posts floated): a braided rope (SM_FixedRope_Piece, MI_FixedRope_New) lying on the snow,
 *  sagging a little between anchors and riding over bumps, tied every 10-25 m to an anchor - an aluminium snow stake,
 *  an ice screw (icefall, Lhotse Face) or a rock peg (Yellow Band, Geneva Spur) - each with a carabiner the rope runs
 *  through. Beside it, here and there, faded ropes of earlier seasons half buried in the snow.
 *
 *  The lay (LayPoints, anchors, old ropes) is computed in the editor against the real ground by
 *  Tools/RoutePlanning/lay_fixed_ropes.py; this actor only draws it (instances: one draw per part per rope).
 *  RopePoints is what the clip-in uses (UExpeditionComponent); the lay script sets it to the laid rope.
 */
UCLASS()
class YGGDRASILSPEAK_API AFixedRope : public AActor
{
	GENERATED_BODY()

public:

	AFixedRope();

	/** Rope path in world space for the clip-in (distance and direction). Written by the route scripts. */
	UPROPERTY(EditAnywhere, Category="Fixed Rope")
	TArray<FVector> RopePoints;

	/** The rope as it lies, every ~50 cm (world space). Points further apart than MaxPieceCm start a new stretch. */
	UPROPERTY(EditAnywhere, Category="Fixed Rope|Lay")
	TArray<FVector> LayPoints;

	/** Old ropes of earlier seasons (same format; gaps where they are under the snow). */
	UPROPERTY(EditAnywhere, Category="Fixed Rope|Lay")
	TArray<FVector> OldLayPoints;

	/** Draw the old ropes? Off since 2026-11-13: from a few metres away they read as a black stick beside the real rope
	 *  (user: "a very strange black line next to the fixed rope"). The points are kept for a better look later. */
	UPROPERTY(EditAnywhere, Category="Fixed Rope|Lay")
	bool bShowOldRopes = false;

	UPROPERTY(EditAnywhere, Category="Fixed Rope|Lay")
	TArray<FTransform> PicketAnchors;

	UPROPERTY(EditAnywhere, Category="Fixed Rope|Lay")
	TArray<FTransform> ScrewAnchors;

	UPROPERTY(EditAnywhere, Category="Fixed Rope|Lay")
	TArray<FTransform> PitonAnchors;

	/** Rope thickness, cm (real fixed lines are 9-11 mm; a little more reads better from the climber's eyes). */
	UPROPERTY(EditAnywhere, Category="Fixed Rope", meta=(ClampMin="0.2"))
	float RopeDiameter = 1.4f;

	UPROPERTY(EditAnywhere, Category="Fixed Rope", meta=(ClampMin="0.2"))
	float OldRopeDiameter = 1.1f;

	/** Two lay points further apart than this are not joined (a gap in the rope, e.g. an old rope under the snow). */
	UPROPERTY(EditAnywhere, Category="Fixed Rope", meta=(ClampMin="10"))
	float MaxPieceCm = 150.f;

	/** Beyond this distance (cm) the rope is not drawn; anchors stop at AnchorDrawDistance. */
	UPROPERTY(EditAnywhere, Category="Fixed Rope", meta=(ClampMin="0"))
	float DrawDistance = 25000.f;

	UPROPERTY(EditAnywhere, Category="Fixed Rope", meta=(ClampMin="0"))
	float AnchorDrawDistance = 9000.f;

	UPROPERTY(EditAnywhere, Category="Fixed Rope|Look")
	TObjectPtr<UStaticMesh> RopeMesh;

	UPROPERTY(EditAnywhere, Category="Fixed Rope|Look")
	TObjectPtr<UMaterialInterface> RopeMaterial;

	UPROPERTY(EditAnywhere, Category="Fixed Rope|Look")
	TObjectPtr<UMaterialInterface> OldRopeMaterial;

	UPROPERTY(EditAnywhere, Category="Fixed Rope|Look")
	TObjectPtr<UStaticMesh> PicketMesh;

	UPROPERTY(EditAnywhere, Category="Fixed Rope|Look")
	TObjectPtr<UStaticMesh> ScrewMesh;

	UPROPERTY(EditAnywhere, Category="Fixed Rope|Look")
	TObjectPtr<UStaticMesh> PitonMesh;

	virtual void OnConstruction(const FTransform& Transform) override;

	/** Distance (cm) from Location to the nearest point on the rope; OutDirection gets the rope's direction there. */
	float DistanceToRope(const FVector& Location, FVector* OutDirection = nullptr) const;

	/** Quick reject: bounds of all rope points grown by Margin. */
	bool IsNear(const FVector& Location, float Margin) const { return RopeBounds.IsValid && RopeBounds.ExpandBy(Margin).IsInsideOrOn(Location); }

protected:

	virtual void BeginPlay() override;

private:

	void UpdateBounds();
	void AddPieces(UInstancedStaticMeshComponent* Target, const TArray<FVector>& Points, float Diameter) const;

	UPROPERTY(VisibleAnywhere, Category="Fixed Rope")
	TObjectPtr<UInstancedStaticMeshComponent> Rope;

	UPROPERTY(VisibleAnywhere, Category="Fixed Rope")
	TObjectPtr<UInstancedStaticMeshComponent> OldRope;

	/** Snow stakes (kept the old component name so saved ropes load cleanly). */
	UPROPERTY(VisibleAnywhere, Category="Fixed Rope")
	TObjectPtr<UInstancedStaticMeshComponent> Stakes;

	UPROPERTY(VisibleAnywhere, Category="Fixed Rope")
	TObjectPtr<UInstancedStaticMeshComponent> Screws;

	UPROPERTY(VisibleAnywhere, Category="Fixed Rope")
	TObjectPtr<UInstancedStaticMeshComponent> Pitons;

	FBox RopeBounds;
};
