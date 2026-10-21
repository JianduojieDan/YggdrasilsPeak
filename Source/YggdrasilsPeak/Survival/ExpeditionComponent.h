// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ExpeditionComponent.generated.h"

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
	void UpdateScreenEffect(float DeltaTime);
	void Say(const FText& Message, float Seconds = 6.f);

	TArray<TWeakObjectPtr<ARouteLandmark>> Camps;
	TWeakObjectPtr<ARouteLandmark> LastCamp;
	TWeakObjectPtr<ARouteLandmark> CurrentCamp;
	TWeakObjectPtr<USurvivalComponent> Survival;

	FTimerHandle RespawnTimer;
	float CampCheckTimer = 0.f;
	float DisplayedHypoxia = 0.f;
	float StartTime = 0.f;
	int32 Deaths = 0;
	bool bSummitReached = false;
	bool bFinished = false;
};
