// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ExpeditionComponent.generated.h"

class AFixedRope;
class AStoryClue;
class ATrailCheckpoint;
class ARouteLandmark;
class USurvivalComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnExpeditionMessage, const FText&, Message, float, Seconds);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnExpeditionReading, const FText&, Title, const FText&, Body, bool, bOpen);

/**
 *  The expedition layer on top of survival: camps are shelters and checkpoints, the summit is the
 *  turning point, getting back down to the finish camp wins. On death the player is brought back at
 *  the last camp reached, with the pack as it was there; every camp is also saved to disk and the game
 *  continues from it next time (UYggSaveGame). Thin air also shows on screen (vignette, washed-out colour) through the
 *  owner's camera.
 */
UCLASS(ClassGroup=(Survival), meta=(BlueprintSpawnableComponent))
class YGGDRASILSPEAK_API UExpeditionComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	UExpeditionComponent();

	/** Beats per minute: rest ~64, up with thin air, effort, exhaustion and cold (drives the heartbeat on the view). */
	float GetHeartRate() const { return HeartRate; }

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

	/** Seconds on the mountain, this expedition (carried over through saves). */
	int32 GetPlaySeconds() const;
	/** The last camp or checkpoint reached (empty before the first). */
	const FString& GetCampName() const { return LastCheckpointName; }
	int32 GetDeaths() const { return Deaths; }

	UFUNCTION(BlueprintPure, Category="Expedition")
	bool IsExpeditionComplete() const { return bFinished; }

	UFUNCTION(BlueprintPure, Category="Expedition")
	ARouteLandmark* GetLastCamp() const { return LastCamp.Get(); }

	/** Camps ordered from lowest to highest (route order). */
	const TArray<TWeakObjectPtr<ARouteLandmark>>& GetCamps() const { return Camps; }

	/** Moves the player next to a camp (debug / respawn). */
	void TeleportToCamp(ARouteLandmark* Camp);

	/** Moves the player beside any checkpoint actor (camp or trail checkpoint). */
	void TeleportNear(AActor* Where);

	// ---- Story clues
	/** HUD listens to open / close the reading panel. */
	UPROPERTY(BlueprintAssignable, Category="Expedition")
	FOnExpeditionReading OnReading;

	/** A clue came into / left reading range (AStoryClue). */
	void SetReadableClue(AStoryClue* Clue);
	void ClearReadableClue(AStoryClue* Clue);

	/** E: reads the clue in range, or closes what is being read. */
	UFUNCTION(BlueprintCallable, Category="Expedition")
	void ToggleReading();

	/** In a camp's shelter: sleep - the screen goes dark, time passes (to 04:30 if it is afternoon or later, else
	 *  three hours), stamina, warmth and the mind come back, food and water go down. Outside: nothing. */
	UFUNCTION(BlueprintCallable, Category="Expedition")
	void Rest();

	UFUNCTION(BlueprintPure, Category="Expedition")
	bool HasFoundClue(FName ClueId) const { return FoundClues.Contains(ClueId); }

	/** Remembers a story event (kept with the found clues in the save, written at once). */
	void MarkStoryEvent(FName EventId) { if (!FoundClues.Contains(EventId)) { FoundClues.Add(EventId); WriteSave(); } }

protected:

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:

	UFUNCTION()
	void HandleDeath();

	void Respawn();
	/** After a death: the death screen (UYggMenuWidget::ShowDeath); Return calls Respawn. */
	void ShowDeathScreen();
	void UpdateCamps();
	/** Checkpoint (camp or trail checkpoint): pack, body and bottles for the next respawn, and the save game on disk. */
	void SaveCheckpoint(AActor* Where, const FString& Name);
	/** Writes the save game: the last checkpoint as it was, the clues as they are now. */
	void WriteSave();
	void UpdateTrailCheckpoints();
	/** Continues from the save game on disk, if there is one (ygg.Save.LoadOnStart). */
	void LoadSavedExpedition();
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
	TArray<TWeakObjectPtr<ATrailCheckpoint>> TrailCheckpoints;
	/** Where the player comes back after a death, and its name in the save game. */
	TWeakObjectPtr<AActor> LastCheckpoint;
	FString LastCheckpointName;
	TArray<float> CheckpointStats;
	TMap<FName, int32> CheckpointItems;
	TSet<FName> FoundClues;
	TWeakObjectPtr<AStoryClue> ReadableClue;
	bool bReading = false;
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
	FTimerHandle RestTimer;
	bool bResting = false;
	void FinishRest();
	float CampCheckTimer = 0.f;
	float DisplayedHypoxia = 0.f;
	// The body on the view (UpdateScreenEffect): M_PP_Body on the camera and a camera modifier, driven by a heart rate
	UPROPERTY(Transient)
	TObjectPtr<class UMaterialInstanceDynamic> BodyMID;
	TWeakObjectPtr<class UYggBodyCameraModifier> BodyCam;
	float HeartRate = 64.f;
	float HeartPhase = 0.f;
	float DisplayedFrost = 0.f;
	float DisplayedExhaust = 0.f;
	float DisplayedFaint = 0.f;
	bool bDeathZoneSaid = false;
	float StartTime = 0.f;
	int32 Deaths = 0;
	int32 CheckpointBottles = 0;
	/** Play time carried over from the save game. */
	float SavedPlaySeconds = 0.f;
	bool bSummitReached = false;
	bool bFinished = false;
};
