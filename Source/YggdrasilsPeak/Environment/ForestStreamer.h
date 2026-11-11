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
 *  The valley forest (Tools/Art/plant_forest.py) streamed by distance (2026-11-11 RAM pass): the Megaplant trees are
 *  Nanite skeletal assemblies, ~1 GB in memory, and only the valley below Dingboche has any. Their meshes are soft
 *  references, loaded asynchronously when the climber comes within LoadWithinCm of the forest and released (garbage
 *  collected) beyond UnloadBeyondCm. Instances are InstancedSkinnedMeshComponents, out of ray tracing, culled by distance.
 */
UCLASS()
class YGGDRASILSPEAK_API AForestStreamer : public AActor
{
	GENERATED_BODY()

public:
	AForestStreamer();

	UPROPERTY(EditAnywhere, Category="Forest")
	TArray<FForestSet> Sets;

	UPROPERTY(EditAnywhere, Category="Forest")
	float LoadWithinCm = 900000.f;

	UPROPERTY(EditAnywhere, Category="Forest")
	float UnloadBeyondCm = 1100000.f;

	/** Trees fade out at this distance from the camera. */
	UPROPERTY(EditAnywhere, Category="Forest")
	float CullDistanceCm = 1000000.f;

	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void Load();
	void OnLoaded();
	void Unload();
	float DistanceToForest(const FVector& Where) const;

	FBox2D Area;
	FStreamableManager Streamable;
	TSharedPtr<FStreamableHandle> Handle;
	UPROPERTY(Transient) TArray<TObjectPtr<UInstancedSkinnedMeshComponent>> Spawned;
	bool bWanted = false;
	float CheckIn = 0.f;
};
