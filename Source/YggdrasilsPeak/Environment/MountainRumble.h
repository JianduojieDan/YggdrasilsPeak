// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MountainRumble.generated.h"

class UBoxComponent;
class USoundBase;

/**
 *  The Western Cwm's deep booms (Phase1_Design_Plan S4: "Níðhöggr's impacts booming in the mountain; the compass
 *  shakes harder. Could still be avalanches."). While you are inside the area, now and then something strikes far
 *  below: a deep boom from under your feet, the ground trembles (the view shakes), snow slides off the walls in
 *  the distance, and Father's compass jolts. The first two are commented on; after that they just happen.
 */
UCLASS()
class YGGDRASILSPEAK_API AMountainRumble : public AActor
{
	GENERATED_BODY()

public:

	AMountainRumble();

	UPROPERTY(VisibleAnywhere, Category="Rumble")
	TObjectPtr<UBoxComponent> Area;

	/** Seconds between booms (random in this range); the first comes soon after entering. */
	UPROPERTY(EditAnywhere, Category="Rumble")
	FVector2D Interval = FVector2D(45.f, 100.f);

	UPROPERTY(EditAnywhere, Category="Rumble")
	float FirstDelay = 12.f;

	/** View shake at the peak (degrees). */
	UPROPERTY(EditAnywhere, Category="Rumble")
	float ShakeDegrees = 1.6f;

	UPROPERTY(EditAnywhere, Category="Rumble")
	TArray<TObjectPtr<USoundBase>> Booms;

	virtual void Tick(float DeltaSeconds) override;

	/** Strike now (also console ygg.Boom). */
	UFUNCTION(BlueprintCallable, Category="Rumble")
	void Strike();

protected:

	virtual void BeginPlay() override;

private:

	TWeakObjectPtr<APawn> Climber;
	float Next = 0.f;
	float Shake = 0.f;
	float ShakeTime = 0.f;
	FRotator Applied = FRotator::ZeroRotator;
	int32 Count = 0;
};
