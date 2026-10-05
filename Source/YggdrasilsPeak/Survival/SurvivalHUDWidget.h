// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Survival/SurvivalTypes.h"
#include "SurvivalHUDWidget.generated.h"

class UProgressBar;
class UTextBlock;
class UWidget;
class USurvivalComponent;

/**
 *  Presentation-only survival HUD. It finds the SurvivalComponent on the owning player's pawn,
 *  listens to its events and decides what to show, fade and pulse. It never calculates a stat.
 *
 *  The look (frosted glass panels, icons, fonts) lives in the Widget Blueprint derived from this
 *  class. Every bound widget below is optional: name a widget exactly like the property and it is
 *  picked up, leave it out and that part of the HUD is simply skipped.
 */
UCLASS(Abstract)
class YGGDRASILSPEAK_API USurvivalHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	// ---- Widgets bound by name from the Widget Blueprint ----

	/** Shown while sprinting and while stamina is recovering, then fades out. */
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UWidget> StaminaPanel;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UProgressBar> StaminaBar;

	/** Shown when health is low or just changed. */
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UWidget> HealthPanel;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UProgressBar> HealthBar;

	/** Warning icons: invisible normally, pulsing while their warning is active. */
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UWidget> HungerIcon;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UWidget> ThirstIcon;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UWidget> ColdIcon;

	/** Cold-to-comfortable indicator, shown only once the body has started to cool. */
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UWidget> TemperaturePanel;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UProgressBar> TemperatureBar;

	/** Full-screen frost along the screen edges; its opacity follows how cold the body is. */
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UWidget> FrostOverlay;

	/** Short message shown once when a warning starts. */
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UWidget> WarningPanel;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> WarningText;

	/** Tuning numbers for development. Shown while the console variable ygg.HUD.Debug is 1. */
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UWidget> DebugPanel;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> DebugText;

	// ---- Tuning ----

	/** How quickly bar fills catch up with the real value. */
	UPROPERTY(EditDefaultsOnly, Category="Survival HUD|Motion", meta=(ClampMin="0.1"))
	float BarInterpSpeed = 8.f;

	UPROPERTY(EditDefaultsOnly, Category="Survival HUD|Motion", meta=(ClampMin="0.1"))
	float FadeInSpeed = 8.f;

	UPROPERTY(EditDefaultsOnly, Category="Survival HUD|Motion", meta=(ClampMin="0.1"))
	float FadeOutSpeed = 2.f;

	/** Pulses per second of an active warning icon, in radians per second. */
	UPROPERTY(EditDefaultsOnly, Category="Survival HUD|Motion", meta=(ClampMin="0"))
	float PulseSpeed = 5.f;

	/** Seconds the stamina panel stays after stamina is full again. */
	UPROPERTY(EditDefaultsOnly, Category="Survival HUD|Timing", meta=(ClampMin="0"))
	float StaminaLingerSeconds = 1.5f;

	/** Seconds the health panel stays after health was last lost. */
	UPROPERTY(EditDefaultsOnly, Category="Survival HUD|Timing", meta=(ClampMin="0"))
	float HealthLingerSeconds = 3.f;

	/** Seconds a warning message stays on screen. */
	UPROPERTY(EditDefaultsOnly, Category="Survival HUD|Timing", meta=(ClampMin="0"))
	float WarningMessageSeconds = 4.f;

	/** The temperature indicator appears once normalised body temperature falls below this. */
	UPROPERTY(EditDefaultsOnly, Category="Survival HUD|Temperature", meta=(ClampMin="0", ClampMax="1"))
	float TemperatureShowBelow = 0.9f;

	/** Normalised temperature where frost starts and where it reaches full strength. */
	UPROPERTY(EditDefaultsOnly, Category="Survival HUD|Temperature", meta=(ClampMin="0", ClampMax="1"))
	float FrostStartsAt = 0.8f;

	UPROPERTY(EditDefaultsOnly, Category="Survival HUD|Temperature", meta=(ClampMin="0", ClampMax="1"))
	float FrostFullAt = 0.2f;

	UPROPERTY(EditDefaultsOnly, Category="Survival HUD|Temperature", meta=(ClampMin="0", ClampMax="1"))
	float FrostMaxOpacity = 0.7f;

	// ---- Colours ----

	UPROPERTY(EditDefaultsOnly, Category="Survival HUD|Colours")
	FLinearColor StaminaColor = FLinearColor(0.78f, 0.90f, 1.f, 0.95f);

	UPROPERTY(EditDefaultsOnly, Category="Survival HUD|Colours")
	FLinearColor ExhaustedColor = FLinearColor(1.f, 0.55f, 0.25f, 0.95f);

	UPROPERTY(EditDefaultsOnly, Category="Survival HUD|Colours")
	FLinearColor HealthColor = FLinearColor(0.85f, 0.95f, 0.90f, 0.95f);

	UPROPERTY(EditDefaultsOnly, Category="Survival HUD|Colours")
	FLinearColor LowHealthColor = FLinearColor(1.f, 0.30f, 0.30f, 0.95f);

	UPROPERTY(EditDefaultsOnly, Category="Survival HUD|Colours")
	FLinearColor TemperatureColor = FLinearColor(0.70f, 0.85f, 1.f, 0.95f);

	UPROPERTY(EditDefaultsOnly, Category="Survival HUD|Colours")
	FLinearColor ColdTemperatureColor = FLinearColor(0.35f, 0.60f, 1.f, 0.95f);

protected:

	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:

	/** Finds the component on the current pawn and (re)binds when the pawn changed. Cheap enough to call every frame. */
	void TryBind();
	void Unbind();
	/** Jumps every displayed value to the real one, so a fresh bind does not animate from zero. */
	void SnapToCurrentState();

	UFUNCTION()
	void HandleStatChanged(ESurvivalStat Stat, float NewValue, float OldValue);

	UFUNCTION()
	void HandleWarningChanged(ESurvivalWarning Warning, bool bActive);

	UFUNCTION()
	void HandleDeath();

	static void FadeWidget(UWidget* Widget, float TargetOpacity, float DeltaTime, float InSpeed, float OutSpeed);
	void UpdateDebugText(const FSurvivalSnapshot& Snapshot) const;

	TWeakObjectPtr<USurvivalComponent> Survival;

	float DisplayedStamina = 1.f;
	float DisplayedHealth = 1.f;
	float DisplayedTemperature = 1.f;

	float StaminaLinger = 0.f;
	float HealthLinger = 0.f;
	float WarningMessageTimer = 0.f;

	/** Eased 0..1 strength of each warning icon, indexed by ESurvivalWarning. */
	float WarningStrength[5] = { 0.f, 0.f, 0.f, 0.f, 0.f };

	float PulseClock = 0.f;
};
