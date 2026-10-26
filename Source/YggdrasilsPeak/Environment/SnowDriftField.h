// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SnowDriftField.generated.h"

class UHierarchicalInstancedStaticMeshComponent;

/**
 *  Wind-blown snow drifts piled on the downwind side of rocks and tents: one instanced snow mound per obstacle,
 *  stretched along the wind. The instances are baked in the editor (Tools/Snow/place_drifts.py traces the ground
 *  behind every PCG rock and camp tent); run it again after the rocks are regenerated.
 *  No collision: the drifts are low and you wade through them like the rest of the soft snow.
 */
UCLASS()
class YGGDRASILSPEAK_API ASnowDriftField : public AActor
{
	GENERATED_BODY()

public:

	ASnowDriftField();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Snow")
	TObjectPtr<UHierarchicalInstancedStaticMeshComponent> Drifts;
};
