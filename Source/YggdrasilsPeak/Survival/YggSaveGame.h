// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "YggSaveGame.generated.h"

/**
 *  The expedition checkpoint on disk, written at every camp (UExpeditionComponent): where you are, your body
 *  and your pack as they were when you arrived. Starting the game continues from it (console ygg.NewGame
 *  deletes it). Story clues found are kept here too, even across deaths.
 */
UCLASS()
class YGGDRASILSPEAK_API UYggSaveGame : public USaveGame
{
	GENERATED_BODY()

public:

	static constexpr const TCHAR* SlotName = TEXT("Expedition");

	/** Bumped when the layout changes; older saves are ignored. */
	UPROPERTY()
	int32 Version = 1;

	/** Name of the checkpoint: ARouteLandmark::LandmarkName or ATrailCheckpoint::CheckpointName. */
	UPROPERTY()
	FString CampName;

	/** Survival stats by ESurvivalStat order. */
	UPROPERTY()
	TArray<float> Stats;

	UPROPERTY()
	TMap<FName, int32> Items;

	UPROPERTY()
	int32 CarriedBottles = 0;

	/** Altitude the climber is adapted to (m, physiology model, 2026-11-14); below 0 = not saved yet. */
	UPROPERTY()
	float AdaptedAltitude = -1.f;

	UPROPERTY()
	bool bSummitReached = false;

	UPROPERTY()
	int32 Deaths = 0;

	/** Seconds played before this save (the expedition timer carries on from here). */
	UPROPERTY()
	float PlaySeconds = 0.f;

	/** Story clues read (AStoryClue::ClueId); kept through deaths. */
	UPROPERTY()
	TArray<FName> Clues;
};
