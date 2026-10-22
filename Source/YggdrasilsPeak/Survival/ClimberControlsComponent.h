// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ClimberControlsComponent.generated.h"

class UInputAction;
class UInputMappingContext;
class UUserWidget;

/**
 *  The climber's extras that are not part of a stock character Blueprint: sprint (Left Shift / left stick click,
 *  feeding USurvivalComponent::SetSprintRequested) and the survival HUD widget.
 *  Lets any character Blueprint (e.g. the First Person template character) become the climber just by adding
 *  this next to the Survival, Expedition and FirstPersonBody components - no Blueprint graph needed.
 *  Move / look / jump stay with the character Blueprint's own input.
 */
UCLASS(ClassGroup=(Survival), meta=(BlueprintSpawnableComponent))
class YGGDRASILSPEAK_API UClimberControlsComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	UClimberControlsComponent();

	/** Held to sprint. */
	UPROPERTY(EditAnywhere, Category="Climber Controls")
	TObjectPtr<UInputAction> SprintAction;

	/** HUD added to the owning player's viewport (skipped if one of this class is already on screen). */
	UPROPERTY(EditAnywhere, Category="Climber Controls")
	TSoftClassPtr<UUserWidget> HUDClass;

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:

	virtual void BeginPlay() override;

private:

	/** The pawn's input component may not exist yet at BeginPlay; this retries each tick until bound. */
	bool TryBindInput();
	void CreateHUD();

	void OnSprintStarted();
	void OnSprintStopped();

	/** Built at runtime with only the sprint keys, so it can't swallow the character's own move/look keys. */
	UPROPERTY(Transient)
	TObjectPtr<UInputMappingContext> SprintMapping;
};
