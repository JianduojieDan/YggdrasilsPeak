// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ParentsGhosts.generated.h"

class UMaterialInstanceDynamic;
class USkeletalMeshComponent;

/**
 *  "The parents' ghosts walk in the blizzard" (original setting; Phase1_Design_Plan S6). Two pale figures - the
 *  climber's own model in a translucent, rim-lit ghost material - walking up the mountain side by side some way
 *  ahead. They fade in, climb on without looking back, and fade out when you come close or after a while.
 *  Spawned by USanityComponent when the mind is frayed (also console ygg.Ghosts).
 */
UCLASS()
class YGGDRASILSPEAK_API AParentsGhosts : public AActor
{
	GENERATED_BODY()

public:

	AParentsGhosts();

	UPROPERTY(VisibleAnywhere, Category="Ghosts")
	TObjectPtr<USkeletalMeshComponent> Father;

	UPROPERTY(VisibleAnywhere, Category="Ghosts")
	TObjectPtr<USkeletalMeshComponent> Mother;

	/** Walking speed (cm/s), seconds they stay, and how close you can get before they are gone (cm). */
	UPROPERTY(EditAnywhere, Category="Ghosts")
	float WalkSpeed = 85.f;

	UPROPERTY(EditAnywhere, Category="Ghosts")
	float Lifetime = 13.f;

	UPROPERTY(EditAnywhere, Category="Ghosts")
	float VanishDistance = 1600.f;

	virtual void Tick(float DeltaSeconds) override;

	/** Puts a pair ahead of the player, along their view. */
	static AParentsGhosts* SpawnAhead(UWorld* World, float Distance = 3500.f);

	/** Loads (and keeps) the ghosts' mesh, walk and material now, so a pair spawned mid-play never waits for the disk.
	 *  Called by USanityComponent at BeginPlay. */
	static void Preload();

protected:

	virtual void BeginPlay() override;

private:

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> Ghostly;

	float Age = 0.f;
	float Fade = 0.f;
	bool bLeaving = false;
};
