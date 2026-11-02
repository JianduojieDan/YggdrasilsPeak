// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "StoryClue.generated.h"

class USphereComponent;
class UStaticMeshComponent;

/**
 *  Something to find and read on the mountain: a journal page, a discovery in the rock. Close by, the HUD offers
 *  "[E] Read"; reading shows the text and records the clue for good (kept through deaths and in the save game,
 *  UExpeditionComponent). Main-route clues carry the story the endings depend on; optional ones add context.
 */
UCLASS()
class YGGDRASILSPEAK_API AStoryClue : public AActor
{
	GENERATED_BODY()

public:

	AStoryClue();

	/** Stable id for saving (never shown). */
	UPROPERTY(EditAnywhere, Category="Clue")
	FName ClueId = TEXT("Clue");

	UPROPERTY(EditAnywhere, Category="Clue")
	FText Title;

	UPROPERTY(EditAnywhere, Category="Clue", meta=(MultiLine="true"))
	FText Body;

	/** On the main route (needed to understand the endings) or an optional find. */
	UPROPERTY(EditAnywhere, Category="Clue")
	bool bMainRoute = true;

	UPROPERTY(VisibleAnywhere, Category="Clue")
	TObjectPtr<UStaticMeshComponent> Mesh;

	/** Within this distance (cm) the player can read it. */
	UPROPERTY(EditAnywhere, Category="Clue", meta=(ClampMin="50"))
	float ReadRadius = 300.f;

protected:

	virtual void NotifyActorBeginOverlap(AActor* OtherActor) override;
	virtual void NotifyActorEndOverlap(AActor* OtherActor) override;
	virtual void OnConstruction(const FTransform& Transform) override;

private:

	UPROPERTY(VisibleAnywhere, Category="Clue")
	TObjectPtr<USphereComponent> Trigger;
};
