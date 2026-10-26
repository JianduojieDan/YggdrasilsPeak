// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ExpeditionComponent.generated.h"

class AFixedRope;
class ARouteLandmark;
class USurvivalComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnExpeditionMessage, const FText&, Message, float, Seconds);

/**
 *  The expedition layer on top of survival: camps are shelters and checkpoints, the summit is the
 *  turning point, getting back down to the finish camp wins. On death the player is brought back at
 *  the last camp reached. Thin air also shows on screen (vignette, washed-out colour) through the
 *  owner's camera.
 */
UCLASS(ClassGroup=(Survival), meta=(BlueprintSpawnableComponent))
class YGGDRASILSPEAK_API UExpeditionComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	UExpeditionComponent();

	/** Inside this distance (cm, horizontal) of a camp's centre you are sheltered and it becomes your checkpoint. */
	UPROPERTY(EditAnywhere, Category="Expedition", meta=(ClampMin="100"))
	float CampRadius = 1600.f;

	/** Seconds between death and coming back at the last camp. */
	UPROPERTY(EditAnywhere, Category="Expedition", meta=(ClampMin="0"))
	float RespawnDelay = 5.f;

	/** Food and water (percent) you come back with at least. */
	UPROPERTY(EditAnywhere, Category="Expedition", meta=(ClampMin="0", ClampMax="100"))
	float RespawnNeedsPercent = 60.f;

	/** Extra cold exposure (0..1) at full storm strength, on top of altitude. */
	UPROPERTY(EditAnywhere, Category="Expedition|Weather", meta=(ClampMin="0", ClampMax="1"))
	float StormColdExposure = 0.45f;

	/** Walking speed multiplier at full storm strength (wind and deep fresh snow). */
	UPROPERTY(EditAnywhere, Category="Expedition|Weather", meta=(ClampMin="0.1", ClampMax="1"))
	float StormSpeedMultiplier = 0.6f;

	/** Storm strength 0..1 right now, from the weather system's local wind and snow. */
	UFUNCTION(BlueprintPure, Category="Expedition|Weather")
	float GetStormStrength() const { return StormStrength; }

	/** Local wind on the weather system's 0..10 scale (calm ~2, gale 8+). */
	UFUNCTION(BlueprintPure, Category="Expedition|Weather")
	float GetWindIntensity() const { return WindIntensity; }

	/** Yaw (degrees, world) the wind blows towards. */
	UFUNCTION(BlueprintPure, Category="Expedition|Weather")
	float GetWindDirection() const { return WindDirection; }

	/** Within this distance (cm) of a fixed rope you are clipped in. */
	UPROPERTY(EditAnywhere, Category="Expedition|Fixed Rope", meta=(ClampMin="50"))
	float RopeClipDistance = 400.f;

	/** Steepest ground (degrees) you can walk up while clipped; the normal limit is the movement component's own. */
	UPROPERTY(EditAnywhere, Category="Expedition|Fixed Rope", meta=(ClampMin="0", ClampMax="85"))
	float RopeWalkableFloorAngle = 58.f;

	UFUNCTION(BlueprintPure, Category="Expedition|Fixed Rope")
	bool IsOnRope() const { return bOnRope; }

	/** Shows a message through the HUD (pickups and other actors use this too). */
	void ShowMessage(const FText& Message, float Seconds = 6.f) { Say(Message, Seconds); }

	/** Strength of the low-oxygen screen effect (0 disables it). */
	UPROPERTY(EditAnywhere, Category="Expedition", meta=(ClampMin="0", ClampMax="1"))
	float HypoxiaScreenEffect = 1.f;

	/** HUD listens to this for camp, summit, death and finish messages. */
	UPROPERTY(BlueprintAssignable, Category="Expedition")
	FOnExpeditionMessage OnMessage;

	UFUNCTION(BlueprintPure, Category="Expedition")
	bool HasReachedSummit() const { return bSummitReached; }

	UFUNCTION(BlueprintPure, Category="Expedition")
	bool IsExpeditionComplete() const { return bFinished; }

	UFUNCTION(BlueprintPure, Category="Expedition")
	ARouteLandmark* GetLastCamp() const { return LastCamp.Get(); }

	/** Camps ordered from lowest to highest (route order). */
	const TArray<TWeakObjectPtr<ARouteLandmark>>& GetCamps() const { return Camps; }

	/** Moves the player next to a camp (debug / respawn). */
	void TeleportToCamp(ARouteLandmark* Camp);

protected:

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:

	UFUNCTION()
	void HandleDeath();

	void Respawn();
	void UpdateCamps();
	/** Clips the player onto the nearest fixed rope when close enough, and unclips when they wander off. */
	void UpdateRope();
	void UpdateScreenEffect(float DeltaTime);
	void UpdateWeather();
	/** Reads a number property (float or double) from a Blueprint object by its display name, ignoring case and spaces. */
	static bool ReadNumber(const UObject* Object, const TCHAR* Name, double& Out);
	static UObject* ReadObject(const UObject* Object, const TCHAR* Name);
	void Say(const FText& Message, float Seconds = 6.f);

	TArray<TWeakObjectPtr<ARouteLandmark>> Camps;
	TArray<TWeakObjectPtr<AFixedRope>> Ropes;
	bool bOnRope = false;
	bool bRopeExplained = false;
	float DefaultWalkableFloorAngle = 0.f;
	float RopeCheckTimer = 0.f;
	TWeakObjectPtr<ARouteLandmark> LastCamp;
	TWeakObjectPtr<ARouteLandmark> CurrentCamp;
	TWeakObjectPtr<USurvivalComponent> Survival;
	TWeakObjectPtr<AActor> WeatherActor;
	float StormStrength = 0.f;
	/** Without the weather system: a moderate wind from the west-northwest, the prevailing winter wind on Everest (north = -Y). */
	float WindIntensity = 4.f;
	float WindDirection = 15.f;
	bool bStormWarned = false;
	float LoggedStorm = -1.f;
	bool bLoggedNoState = false;

	FTimerHandle RespawnTimer;
	float CampCheckTimer = 0.f;
	float DisplayedHypoxia = 0.f;
	float StartTime = 0.f;
	int32 Deaths = 0;
	bool bSummitReached = false;
	bool bFinished = false;
};
