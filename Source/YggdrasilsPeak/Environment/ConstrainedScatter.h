// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ConstrainedScatter.generated.h"

class UBoxComponent;
class UHierarchicalInstancedStaticMeshComponent;
class UStaticMesh;
class UTexture2D;

/** One mesh the scatter may place, with its share and size range. */
USTRUCT(BlueprintType)
struct FScatterMesh
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category="Scatter")
	TObjectPtr<UStaticMesh> Mesh;

	/** Relative chance of picking this mesh. */
	UPROPERTY(EditAnywhere, Category="Scatter", meta=(ClampMin="0"))
	float Weight = 1.f;

	UPROPERTY(EditAnywhere, Category="Scatter", meta=(ClampMin="0.01"))
	float MinScale = 0.8f;

	UPROPERTY(EditAnywhere, Category="Scatter", meta=(ClampMin="0.01"))
	float MaxScale = 1.4f;

	/** Sunk this fraction of its height into the ground (hides the cut base on slopes). */
	UPROPERTY(EditAnywhere, Category="Scatter", meta=(ClampMin="0", ClampMax="0.9"))
	float EmbedFraction = 0.15f;

	/** Random extra lean, up to this many degrees, in a random direction. */
	UPROPERTY(EditAnywhere, Category="Scatter", meta=(ClampMin="0", ClampMax="45"))
	float MaxLeanDeg = 0.f;

	/** 0 = uniform scale; 1 = widths/heights vary roughly 0.5x..2x (slabs to towers). */
	UPROPERTY(EditAnywhere, Category="Scatter", meta=(ClampMin="0", ClampMax="1"))
	float ShapeVariance = 0.f;
};

/** Why candidates were rejected, in rule priority order (thesis evaluation: one count per reason). */
UENUM()
enum class EScatterReject : uint8
{
	RouteCorridor,   // 1. route access
	ProtectedSpot,   // 2. camps, supplies, story spots
	NoGround,        // 3. valid placement: nothing under it / not the landscape
	Overlap,         // 3. valid placement: too close to another placement
	Elevation,       // 4. environmental suitability
	Slope,           // 4.
	SurfaceMask,     // 4. (e.g. outside the glacier)
	FarFromRoute,    // 4. outside the band along the route (MaxRouteDistanceCm)
	Count UMETA(Hidden)
};

/**
 *  Constraint-based procedural dressing (Phase 1 plan, section 9): places meshes inside its box from a seed,
 *  testing each candidate against rules in priority order - (1) keep the climbing route open (fixed ropes,
 *  corridor), (2) keep camps / pickups / story spots clear, (3) valid placement (on the landscape, no overlap),
 *  (4) environmental suitability (elevation, slope, surface mask), (5) density. A candidate failing any rule is
 *  rejected, never forced. Runs in the editor (Generate); the result is saved with the level, so the game pays
 *  nothing and a seed reproduces the same layout. Every run writes a report (Saved/ScatterReports/*.json):
 *  seed, settings, accepted / rejected counts by reason, time, and a re-check of the result against the rules.
 */
UCLASS()
class YGGDRASILSPEAK_API AConstrainedScatter : public AActor
{
	GENERATED_BODY()

public:

	AConstrainedScatter();

	UPROPERTY(EditAnywhere, Category="Scatter")
	TArray<FScatterMesh> Meshes;

	UPROPERTY(EditAnywhere, Category="Scatter")
	int32 Seed = 1;

	/** How many placements to aim for (rule 5: stops when reached). */
	UPROPERTY(EditAnywhere, Category="Scatter", meta=(ClampMin="0"))
	int32 TargetCount = 300;

	/** Candidates tried per wanted placement before giving up (low density means the rules are tight here). */
	UPROPERTY(EditAnywhere, Category="Scatter", meta=(ClampMin="1"))
	int32 AttemptsPerPlacement = 20;

	// ---- 1. Route access
	/** Nothing within this distance (cm, horizontal) of a fixed rope. */
	UPROPERTY(EditAnywhere, Category="Scatter|1 Route")
	float RouteCorridorCm = 900.f;

	// ---- 2. Protected spots
	/** Clear radius around camps (cm). */
	UPROPERTY(EditAnywhere, Category="Scatter|2 Protected")
	float CampClearCm = 2500.f;

	/** Clear radius around pickups and actors tagged ScatterProtect (cm); a quarter of it around the bounds of actors tagged ScatterProtectBox. */
	UPROPERTY(EditAnywhere, Category="Scatter|2 Protected")
	float SpotClearCm = 600.f;

	// ---- 3. Valid placement
	/** Extra gap between placements on top of their footprints (cm). */
	UPROPERTY(EditAnywhere, Category="Scatter|3 Placement")
	float MinGapCm = 150.f;

	// ---- 4. Suitability
	UPROPERTY(EditAnywhere, Category="Scatter|4 Suitability")
	float MinElevationM = 0.f;

	UPROPERTY(EditAnywhere, Category="Scatter|4 Suitability")
	float MaxElevationM = 9000.f;

	/** Steepest ground (degrees) a placement may stand on. */
	UPROPERTY(EditAnywhere, Category="Scatter|4 Suitability", meta=(ClampMin="0", ClampMax="90"))
	float MaxSlopeDeg = 40.f;

	/** Optional surface mask over the whole heightmap (e.g. T_GlacierMask); placements need at least MaskThreshold. */
	UPROPERTY(EditAnywhere, Category="Scatter|4 Suitability")
	TObjectPtr<UTexture2D> SurfaceMask;

	UPROPERTY(EditAnywhere, Category="Scatter|4 Suitability", meta=(ClampMin="0", ClampMax="1"))
	float MaskThreshold = 0.5f;

	/** Only within this distance (cm) of a fixed rope - dresses the climbing line itself (0 = anywhere). */
	UPROPERTY(EditAnywhere, Category="Scatter|4 Suitability", meta=(ClampMin="0"))
	float MaxRouteDistanceCm = 0.f;

	/** Places the meshes (replacing the last result) and writes the report. */
	UFUNCTION(CallInEditor, Category="Scatter")
	void Generate();

	/** Removes all placements. */
	UFUNCTION(CallInEditor, Category="Scatter")
	void Clear();

	/** Re-checks the current placements against rules 1-3 (e.g. after ropes or camps moved) and logs violations. */
	UFUNCTION(CallInEditor, Category="Scatter")
	void Verify();

	/** Draws the route corridor and protected circles for a while (editor debug). */
	UFUNCTION(CallInEditor, Category="Scatter")
	void ShowProtectedAreas();

	/** Last run, for scripts: accepted, rejected per reason, milliseconds. */
	UPROPERTY(VisibleAnywhere, Category="Scatter|Report")
	int32 LastAccepted = 0;

	UPROPERTY(VisibleAnywhere, Category="Scatter|Report")
	TArray<int32> LastRejected;

	UPROPERTY(VisibleAnywhere, Category="Scatter|Report")
	float LastMilliseconds = 0.f;

	UPROPERTY(VisibleAnywhere, Category="Scatter|Report")
	FString LastReportPath;

private:

	struct FPlaced
	{
		/** Centre and radius of the instance's world-space footprint (lean and tilt included). */
		FVector2D Location;
		float Radius;
		float GroundZ;
	};

	/** Horizontal centre and radius of a placed mesh's world bounds. */
	static FVector2D FootprintOf(const FBoxSphereBounds& MeshBounds, const FTransform& T, float& OutRadius);

	void GatherProtected();
	/** Rule 1/2 for a point: the reason it is blocked, or Count if free. */
	EScatterReject CheckProtected(const FVector& Location, float Radius) const;
	bool SampleMask(const FVector& Location, float& Out) const;
	UHierarchicalInstancedStaticMeshComponent* GetInstancer(int32 MeshIndex);
	void WriteReport(int32 Attempts, int32 Violations);

	UPROPERTY(VisibleAnywhere, Category="Scatter")
	TObjectPtr<UBoxComponent> Area;

	UPROPERTY()
	TArray<TObjectPtr<UHierarchicalInstancedStaticMeshComponent>> Instancers;

	TArray<TWeakObjectPtr<class AFixedRope>> Ropes;
	TArray<FVector> Camps;
	TArray<FVector> Spots;
	TArray<FBox> SpotBoxes;
	TArray<uint8> MaskPixels;
	int32 MaskSize = 0;
};
