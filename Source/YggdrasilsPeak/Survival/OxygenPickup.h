// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "OxygenPickup.generated.h"

class USphereComponent;
class UStaticMeshComponent;

/**
 *  A spare oxygen bottle lying at a camp. Walking over it puts it in the pack (if there is room);
 *  spares are put on automatically when the air gets too thin (USurvivalComponent).
 *  Taken bottles come back after RespawnSeconds so a player who died can stock up again.
 */
UCLASS()
class YGGDRASILSPEAK_API AOxygenPickup : public AActor
{
	GENERATED_BODY()

public:

	AOxygenPickup();

	/** Seconds until a taken bottle is back (0 = never). */
	UPROPERTY(EditAnywhere, Category="Oxygen Pickup", meta=(ClampMin="0"))
	float RespawnSeconds = 600.f;

protected:

	virtual void NotifyActorBeginOverlap(AActor* OtherActor) override;

private:

	void SetAvailable(bool bAvailable);

	UPROPERTY(VisibleAnywhere, Category="Oxygen Pickup")
	TObjectPtr<USphereComponent> Trigger;

	UPROPERTY(VisibleAnywhere, Category="Oxygen Pickup")
	TObjectPtr<UStaticMeshComponent> Bottle;

	FTimerHandle RespawnTimer;
	/** So a full pack does not repeat its message every step. */
	float NextFullMessageTime = 0.f;
};
