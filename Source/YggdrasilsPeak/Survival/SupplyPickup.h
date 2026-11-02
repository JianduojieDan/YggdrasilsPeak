// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SupplyPickup.generated.h"

class USphereComponent;
class UStaticMeshComponent;

/**
 *  Food or water lying at a camp or an old cache. Walking over it packs it (UInventoryComponent), as much as
 *  the pack's weight limit allows; what does not fit stays on the ground. Comes back after RespawnSeconds so a
 *  player who died can restock.
 */
UCLASS()
class YGGDRASILSPEAK_API ASupplyPickup : public AActor
{
	GENERATED_BODY()

public:

	ASupplyPickup();

	/** Supply id (UInventoryComponent::ItemDefs), e.g. Ration or Water. */
	UPROPERTY(EditAnywhere, Category="Supply")
	FName Item = TEXT("Ration");

	UPROPERTY(EditAnywhere, Category="Supply", meta=(ClampMin="1"))
	int32 Count = 2;

	/** Seconds until a taken supply is back (0 = never). */
	UPROPERTY(EditAnywhere, Category="Supply", meta=(ClampMin="0"))
	float RespawnSeconds = 900.f;

	UPROPERTY(VisibleAnywhere, Category="Supply")
	TObjectPtr<UStaticMeshComponent> Mesh;

protected:

	virtual void NotifyActorBeginOverlap(AActor* OtherActor) override;

private:

	void SetAvailable(bool bAvailable);

	UPROPERTY(VisibleAnywhere, Category="Supply")
	TObjectPtr<USphereComponent> Trigger;

	int32 Remaining = 0;
	FTimerHandle RespawnTimer;
	float NextFullMessageTime = 0.f;
};
