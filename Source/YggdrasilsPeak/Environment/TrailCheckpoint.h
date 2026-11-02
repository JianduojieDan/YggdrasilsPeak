// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TrailCheckpoint.generated.h"

/**
 *  A checkpoint between camps, in the middle of long or technical sections (Icefall, Lhotse Face, summit ridge),
 *  so a fall does not cost the whole section again. Passing it saves like a camp (UExpeditionComponent) but it is
 *  no shelter. Marked in the world by a small cairn with a prayer flag stick, if a mesh is set.
 */
UCLASS()
class YGGDRASILSPEAK_API ATrailCheckpoint : public AActor
{
	GENERATED_BODY()

public:

	ATrailCheckpoint();

	/** Shown on the HUD and stored in the save game; must be unique. */
	UPROPERTY(EditAnywhere, Category="Checkpoint")
	FString CheckpointName = TEXT("Checkpoint");

	/** Passing within this distance (cm, horizontal) saves. */
	UPROPERTY(EditAnywhere, Category="Checkpoint", meta=(ClampMin="100"))
	float Radius = 800.f;

	UPROPERTY(VisibleAnywhere, Category="Checkpoint")
	TObjectPtr<class UStaticMeshComponent> Marker;
};
