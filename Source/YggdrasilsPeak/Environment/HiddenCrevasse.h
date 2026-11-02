// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HiddenCrevasse.generated.h"

class UBoxComponent;
class UDecalComponent;
class USoundBase;

/**
 *  A crevasse under a snow bridge, off the roped route (Phase1_Design_Plan hazard table: "Hidden crevasse -
 *  unroped off-route - sagging snow, faint blue - stay on route - fall"). The only warning is a long faint sag in
 *  the snow, a little bluer than around it. Step onto it unroped and the bridge gives way; clipped into a rope it
 *  only sags and the rope holds. Placed by Tools/RoutePlanning/place_hazards.py.
 */
UCLASS()
class YGGDRASILSPEAK_API AHiddenCrevasse : public AActor
{
	GENERATED_BODY()

public:

	AHiddenCrevasse();

	UPROPERTY(VisibleAnywhere, Category="Hidden Crevasse")
	TObjectPtr<UBoxComponent> Bridge;

	/** The sag in the snow (a soft blue-grey stripe). */
	UPROPERTY(VisibleAnywhere, Category="Hidden Crevasse")
	TObjectPtr<UDecalComponent> Sag;

	UPROPERTY(EditAnywhere, Category="Hidden Crevasse")
	TObjectPtr<USoundBase> CollapseSound;

	/** Sets the bridge's size (cm) and its decal together. */
	UFUNCTION(BlueprintCallable, Category="Hidden Crevasse")
	void SetSize(float Length, float Width);

protected:

	virtual void BeginPlay() override;

private:

	UFUNCTION()
	void OnStep(UPrimitiveComponent* Overlapped, AActor* Other, UPrimitiveComponent* OtherComp, int32 BodyIndex, bool bFromSweep, const FHitResult& Sweep);

	bool bWarnedRope = false;
};
