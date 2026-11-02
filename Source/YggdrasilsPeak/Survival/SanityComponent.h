// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SanityComponent.generated.h"

class USoundBase;

/**
 *  The mind (Phase1_Design_Plan: "Sanity and oxygen ... hypoxia distorts vision; is this path the parents' guidance,
 *  or death's lure?"). 100 = clear-headed. It drains with altitude above the Western Cwm, thin blood oxygen, night,
 *  storms and being out in them for long, and comes back in shelter. Nothing on the HUD: you notice it in the
 *  world. Below 60 something whispers just behind you and footsteps follow you; below 35 the edges of the view
 *  fringe and swim, the light breathes, the compass needle spins for a moment; below 12 you stumble.
 *  Added at runtime by UExpeditionComponent.
 */
UCLASS(ClassGroup=(Survival), meta=(BlueprintSpawnableComponent))
class YGGDRASILSPEAK_API USanityComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	USanityComponent();

	UFUNCTION(BlueprintPure, Category="Sanity")
	float GetSanity() const { return Sanity; }

	UFUNCTION(BlueprintCallable, Category="Sanity")
	void SetSanity(float Value) { Sanity = FMath::Clamp(Value, 0.f, 100.f); }

	/** A shock (a death nearby, a ghost, a page from the journal): takes this much at once. */
	UFUNCTION(BlueprintCallable, Category="Sanity")
	void Shock(float Amount) { Sanity = FMath::Clamp(Sanity - Amount, 0.f, 100.f); }

	/** Per minute, per 1000 m above StartAltitude. */
	UPROPERTY(EditAnywhere, Category="Sanity")
	float AltitudeDrain = 1.2f;

	UPROPERTY(EditAnywhere, Category="Sanity")
	float StartAltitude = 6000.f;

	/** Per minute while blood oxygen is below 75 %. */
	UPROPERTY(EditAnywhere, Category="Sanity")
	float HypoxiaDrain = 2.f;

	/** Per minute at night (UDS time of day before 06:00 or after 19:00). */
	UPROPERTY(EditAnywhere, Category="Sanity")
	float NightDrain = 0.8f;

	/** Per minute in strong wind / storm. */
	UPROPERTY(EditAnywhere, Category="Sanity")
	float StormDrain = 1.f;

	/** Per minute in shelter (camps). */
	UPROPERTY(EditAnywhere, Category="Sanity")
	float ShelterRecovery = 8.f;

	UPROPERTY(EditAnywhere, Category="Sanity")
	TArray<TObjectPtr<USoundBase>> Whispers;

	UPROPERTY(EditAnywhere, Category="Sanity")
	TArray<TObjectPtr<USoundBase>> FollowingSteps;

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:

	virtual void BeginPlay() override;

private:

	float TimeOfDay() const;
	void Hallucinate();

	UPROPERTY(Transient)
	TObjectPtr<AActor> Sky;

	float Sanity = 100.f;
	float NextEvent = 30.f;
	float Distortion = 0.f;
	float Pulse = 0.f;
	float Time = 0.f;
	int32 StepsLeft = 0;
	float NextStep = 0.f;
	FVector StepFrom = FVector::ZeroVector;
	float GhostCooldown = 60.f;
	bool bSawGhosts = false;
};
