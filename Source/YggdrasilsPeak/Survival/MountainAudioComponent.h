// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MountainAudioComponent.generated.h"

class UAudioComponent;
class USoundAttenuation;
class USoundBase;
class UExpeditionComponent;
class USurvivalComponent;

/**
 *  The mountain's sound around the local player (sounds from Tools/Audio/build_game_sounds.py, /Game/Audio):
 *
 *  - Wind: four looping beds blended by the weather system's wind (0..10) and the storm: calm, gusty, blizzard, and a
 *    thin high whistle that only comes in high up and in strong wind. Inside a tent it is muffled. The weather
 *    system's own wind sound is turned down so the two do not double.
 *  - Breathing: whole phrases of real breathing (several breaths each, their natural rhythm kept) played back to back;
 *    the harder the effort (stamina spent, speed, climbing, thin air) the harder the phrase and the louder. Silent at
 *    rest down low.
 *  - Fixed rope: a whole carabiner gesture (gate, clip, rattle) and the rope pulling tight when you clip in, a click
 *    when you unclip, and the rope running through the carabiner while you move clipped.
 *  - Distant avalanches now and then high on the mountain, from a direction (console ygg.Avalanche).
 */
UCLASS(ClassGroup=(Survival), meta=(BlueprintSpawnableComponent))
class YGGDRASILSPEAK_API UMountainAudioComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	UMountainAudioComponent();

	UPROPERTY(EditAnywhere, Category="Audio", meta=(ClampMin="0", ClampMax="2"))
	float WindVolume = 0.75f;

	UPROPERTY(EditAnywhere, Category="Audio", meta=(ClampMin="0", ClampMax="2"))
	float BreathVolume = 0.6f;

	UPROPERTY(EditAnywhere, Category="Audio", meta=(ClampMin="0", ClampMax="2"))
	float RopeVolume = 0.7f;

	UPROPERTY(EditAnywhere, Category="Audio", meta=(ClampMin="0", ClampMax="2"))
	float AvalancheVolume = 0.8f;

	/** Seconds between distant avalanches (random in this range), only above AvalancheAboveMeters. */
	UPROPERTY(EditAnywhere, Category="Audio")
	FVector2D AvalancheInterval = FVector2D(240.f, 600.f);

	UPROPERTY(EditAnywhere, Category="Audio")
	float AvalancheAboveMeters = 4900.f;

	/** Below this effort (0..1) there is no audible breathing. */
	UPROPERTY(EditAnywhere, Category="Audio", meta=(ClampMin="0", ClampMax="1"))
	float BreathFromEffort = 0.15f;

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** A distant avalanche now, from a random direction. */
	void Avalanche();

	/** 0..1 how hard the climber is working (drives breathing). */
	float GetEffort() const { return Effort; }

protected:

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:

	void LoadSounds();
	void UpdateWind(float DeltaTime);
	void UpdateBreath(float DeltaTime);
	void UpdateRope(float DeltaTime);
	void MuteWeatherWind();
	UAudioComponent* MakeLoop(USoundBase* Sound);

	enum EBed { Calm, Gusty, Blizzard, Whistle, NumBeds };
	UPROPERTY(Transient) TArray<TObjectPtr<UAudioComponent>> Beds;
	float BedLevel[NumBeds] = { 0.f, 0.f, 0.f, 0.f };
	float Muffle = 0.f;

	UPROPERTY(Transient) TArray<TObjectPtr<USoundBase>> Breaths;      // whole phrases, soft -> hard
	UPROPERTY(Transient) TArray<TObjectPtr<USoundBase>> ClipIns;      // whole carabiner gestures
	UPROPERTY(Transient) TArray<TObjectPtr<USoundBase>> Avalanches;
	UPROPERTY(Transient) TArray<TObjectPtr<USoundBase>> Clicks;
	UPROPERTY(Transient) TArray<TObjectPtr<USoundBase>> Tightens;
	UPROPERTY(Transient) TArray<TObjectPtr<USoundBase>> Gusts;
	UPROPERTY(Transient) TObjectPtr<UAudioComponent> RopeSlide;
	UPROPERTY(Transient) TObjectPtr<USoundAttenuation> FromDirection;

	TWeakObjectPtr<UExpeditionComponent> Expedition;
	TWeakObjectPtr<USurvivalComponent> Survival;

	float Effort = 0.f;
	float NextBreath = 2.f;
	int32 LastBreath = -1;
	float NextAvalanche = 0.f;
	float NextGust = 30.f;
	bool bWasOnRope = false;
	float TightenIn = -1.f;
	float RopeLevel = 0.f;
	float MuteRetry = 0.f;
	bool bWeatherMuted = false;
	float LogTimer = 0.f;
};
