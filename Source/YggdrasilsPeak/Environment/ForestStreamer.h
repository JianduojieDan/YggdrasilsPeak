// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/StreamableManager.h"
#include "ForestStreamer.generated.h"

class USkeletalMesh;
class UMaterialInterface;
class UInstancedSkinnedMeshComponent;

/** One tree variant and every place it stands. */
USTRUCT(BlueprintType)
struct FForestSet
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category="Forest")
	TSoftObjectPtr<USkeletalMesh> Mesh;

	/** Optional: replaces every material slot whose name contains "Foliage" (the gold ginkgo). */
	UPROPERTY(EditAnywhere, Category="Forest")
	TSoftObjectPtr<UMaterialInterface> FoliageMaterial;

	UPROPERTY(EditAnywhere, Category="Forest")
	TArray<FTransform> Instances;
};

/**
 *  The trees (Tools/Art/plant_forest.py), streamed by distance. The Megaplant trees are Nanite skeletal assemblies
 *  (~1 GB in memory); their meshes are soft references, loaded asynchronously while the climber is within LoadWithinCm
 *  of any tree and released (garbage collected) beyond UnloadBeyondCm.
 *  2026-11-13: trees now stand along the whole route, so the instances are bucketed into ChunkCm squares at BeginPlay
 *  and each chunk gets its own InstancedSkinnedMeshComponents only while the climber is within ShowWithinCm of it
 *  (removed beyond HideBeyondCm) - only the trees near the climber exist as components.
 *  Out of ray tracing, culled by distance (CullDistanceCm).
 */
UCLASS()
class YGGDRASILSPEAK_API AForestStreamer : public AActor
{
	GENERATED_BODY()

public:
	AForestStreamer();

	UPROPERTY(EditAnywhere, Category="Forest")
	TArray<FForestSet> Sets;

	/** The tree meshes are loaded while the climber is this close to any chunk... */
	UPROPERTY(EditAnywhere, Category="Forest")
	float LoadWithinCm = 900000.f;

	/** ...and released beyond this from every chunk. */
	UPROPERTY(EditAnywhere, Category="Forest")
	float UnloadBeyondCm = 1300000.f;

	/** Trees fade out at this distance from the camera. */
	UPROPERTY(EditAnywhere, Category="Forest")
	float CullDistanceCm = 800000.f;

	/** Side of a streaming chunk (each chunk = one component per tree variant: the CPU cost goes with their number). */
	UPROPERTY(EditAnywhere, Category="Forest")
	float ChunkCm = 600000.f;

	/** Trees sway in the wind (their components tick) only this close to the climber. */
	UPROPERTY(EditAnywhere, Category="Forest")
	float AnimateWithinCm = 150000.f;

	/** A chunk's trees exist while the climber is within this of its square, and go beyond HideBeyondCm. */
	UPROPERTY(EditAnywhere, Category="Forest")
	float ShowWithinCm = 850000.f;

	UPROPERTY(EditAnywhere, Category="Forest")
	float HideBeyondCm = 1000000.f;

	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	struct FChunk
	{
		int32 Set = 0;
		FBox2D Box = FBox2D(ForceInit);
		TArray<FTransform> Instances;
		int32 Component = INDEX_NONE;      // index into Spawned while shown
	};

	void Load();
	void OnLoaded();
	void Unload();
	void ShowChunk(FChunk& Chunk);
	void HideChunk(FChunk& Chunk);
	static float DistanceToBox(const FBox2D& Box, const FVector& Where);

	TArray<FChunk> Chunks;
	FStreamableManager Streamable;
	TSharedPtr<FStreamableHandle> Handle;
	UPROPERTY(Transient) TArray<TObjectPtr<UInstancedSkinnedMeshComponent>> Spawned;
	bool bWanted = false;
	bool bReady = false;
	float CheckIn = 0.f;
};
