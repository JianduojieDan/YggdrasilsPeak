// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CompassComponent.generated.h"

class UInputAction;
class UInputMappingContext;
class UMaterialInstanceDynamic;
class UStaticMeshComponent;

/**
 *  Father's rune compass, held up in front of the view with Q (Phase1_Design_Plan: the compass from the parents'
 *  belongings; "the compass shakes harder with height"). Until the parents' abandoned camp it is an ordinary
 *  compass that points north. There, once, the needle swings off north towards the summit and holds (the first
 *  unusual event, Q5; saved as story event "CompassTwitch"). After that it is pulled more and more towards the
 *  summit the higher you climb, it shakes harder, and its runes glow while it points up the mountain.
 *  Kick() jolts it (impacts inside the mountain). Added at runtime by UExpeditionComponent.
 */
UCLASS(ClassGroup=(Survival), meta=(BlueprintSpawnableComponent))
class YGGDRASILSPEAK_API UCompassComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	UCompassComponent();

	/** Raises / lowers the compass. */
	UFUNCTION(BlueprintCallable, Category="Compass")
	void Toggle();

	UFUNCTION(BlueprintPure, Category="Compass")
	bool IsRaised() const { return bRaised; }

	/** A jolt: the needle swings and the runes flare (Strength 0..1). Raises the compass for a moment if it is down. */
	UFUNCTION(BlueprintCallable, Category="Compass")
	void Kick(float Strength, bool bShowIt = false);

	/** 0 = points north, 1 = points at the summit. */
	UFUNCTION(BlueprintPure, Category="Compass")
	float GetPull() const { return Pull; }

	/** World yaw of magnetic north (the map's north is -Y). */
	UPROPERTY(EditAnywhere, Category="Compass")
	float NorthYaw = -90.f;

	/** Where it is held, relative to the camera (cm) and its tilt towards the eye (degrees). */
	UPROPERTY(EditAnywhere, Category="Compass")
	FVector HeldOffset = FVector(27.f, 2.5f, -5.5f);

	UPROPERTY(EditAnywhere, Category="Compass")
	float HeldTilt = 42.f;

	UPROPERTY(EditAnywhere, Category="Compass")
	float HeldScale = 1.15f;

	/** Within this distance (cm) of the parents' camp the compass first turns towards the summit. */
	UPROPERTY(EditAnywhere, Category="Compass")
	float AwakenRadius = 2500.f;

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:

	virtual void BeginPlay() override;

private:

	bool TryBindInput();
	void Build();

	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> Case;
	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> Needle;
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> RuneGlow;
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> LidGlow;
	/** A soft light that lights only the held compass (lighting channel 1), so it reads in a dark storm (#66). */
	UPROPERTY(Transient)
	TObjectPtr<class UPointLightComponent> Fill;
	UPROPERTY(Transient)
	TObjectPtr<UInputAction> ToggleAction;
	UPROPERTY(Transient)
	TObjectPtr<UInputMappingContext> Mapping;

	FVector SummitLocation = FVector::ZeroVector;
	FVector ParentsCamp = FVector::ZeroVector;
	bool bHaveCamp = false;
	bool bInputBound = false;
	bool bRaised = false;
	bool bAwake = false;
	float RaiseAlpha = 0.f;
	float ShowFor = 0.f;
	float Pull = 0.f;
	float NeedleYaw = 0.f;
	float NeedleSpeed = 0.f;
	float Impulse = 0.f;
	float ShakeTime = 0.f;
};
