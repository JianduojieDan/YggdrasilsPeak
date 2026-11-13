// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Survival/SurvivalTypes.h"
#include "SurvivalHUDWidget.generated.h"

class UBorder;
class UImage;
class UProgressBar;
class UTextBlock;
class UWidget;
class USurvivalComponent;
class UExpeditionComponent;

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

	/** Altimeter readout. Created at runtime (bottom left) when the Widget Blueprint has none; without
	 *  OxygenText it shows the whole line (altitude, oxygen and status). */
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> AltitudeText;

	/** Blood oxygen, tinted white / amber / red with its icon. */
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> OxygenText;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UImage> OxygenGlyph;

	/** Bottle time, spare bottles, rope and camp; collapsed when there is nothing to say. */
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> StatusText;

	/** Icons in front of the bars; they take the bar's colour. */
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UImage> StaminaGlyph;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UImage> HealthGlyph;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UImage> TemperatureGlyph;

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
	FLinearColor StaminaColor = FLinearColor(1.f, 0.84f, 0.48f, 0.95f);

	UPROPERTY(EditDefaultsOnly, Category="Survival HUD|Colours")
	FLinearColor ExhaustedColor = FLinearColor(1.f, 0.48f, 0.22f, 0.95f);

	UPROPERTY(EditDefaultsOnly, Category="Survival HUD|Colours")
	FLinearColor HealthColor = FLinearColor(1.f, 0.50f, 0.56f, 0.95f);

	UPROPERTY(EditDefaultsOnly, Category="Survival HUD|Colours")
	FLinearColor LowHealthColor = FLinearColor(1.f, 0.22f, 0.25f, 1.f);

	UPROPERTY(EditDefaultsOnly, Category="Survival HUD|Colours")
	FLinearColor TemperatureColor = FLinearColor(0.62f, 0.86f, 1.f, 0.95f);

	UPROPERTY(EditDefaultsOnly, Category="Survival HUD|Colours")
	FLinearColor ColdTemperatureColor = FLinearColor(0.38f, 0.62f, 1.f, 0.95f);

protected:

	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

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

	UFUNCTION()
	void HandleExpeditionMessage(const FText& Message, float Seconds);

	/** Journal pages and discoveries (UExpeditionComponent::OnReading): a panel in the middle of the screen. */
	UFUNCTION()
	void HandleReading(const FText& Title, const FText& Body, bool bOpen);
	void CreateReadingPanel();

	UPROPERTY(Transient)
	TObjectPtr<UBorder> ReadingPanel;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> ReadingTitle;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> ReadingBody;

	void CreateAltitudeText();
	void UpdateAltitudeText(const FSurvivalSnapshot& Snapshot) const;

	static void FadeWidget(UWidget* Widget, float TargetOpacity, float DeltaTime, float InSpeed, float OutSpeed);

	/** Sets the opacity and hides the widget completely when it is (nearly) transparent. */
	static void SetFadedOpacity(UWidget* Widget, float Opacity);
	void UpdateDebugText(const FSurvivalSnapshot& Snapshot) const;

	TWeakObjectPtr<USurvivalComponent> Survival;
	TWeakObjectPtr<UExpeditionComponent> Expedition;

	float DisplayedStamina = 1.f;
	float DisplayedHealth = 1.f;
	float DisplayedTemperature = 1.f;

	float StaminaLinger = 0.f;
	float HealthLinger = 0.f;
	float WarningMessageTimer = 0.f;

	/** Eased 0..1 strength of each warning icon, indexed by ESurvivalWarning. */
	float WarningStrength[6] = { 0.f, 0.f, 0.f, 0.f, 0.f, 0.f };

	float PulseClock = 0.f;
	float IconPulse = 1.f;

	/** What NativePaint draws: read from the game in the tick. */
	struct FPaintState
	{
		FSurvivalSnapshot Snapshot;
		float Temperature = 1.f, Health = 1.f, Stamina = 1.f, Satiety = 1.f, Hydration = 1.f;
		/** Quiet HUD (Settings > Game > HUD): how far the readouts have faded in (1 = shown). */
		float WatchShown = 1.f;
		/** The route's camps for the altimeter's route line: name and height (m), low to high. */
		struct FCampMark { FString Name; float Elev = 0.f; };
		TArray<FCampMark> Camps;
		float CampsAge = 99.f;
		float AirC = 0.f;                // the air, eased towards ARealmSky::GetAirTemperature
		bool bAirKnown = false;
		int32 WeatherNow = -1;           // the shown realm, -1 none
		float Extreme = 0.f;             // 0..1, how extreme the air is (from -35 down to -85, from +30 up to +55)
		// FPS monitor (Settings > Game > FPS counter): frames per second over half a second, 1% low over the last ~5 s,
		// and the GPU / game-thread / render-thread times
		float Fps = 0.f, FrameMs = 0.f, LowFps = 0.f, GpuMs = 0.f, GameMs = 0.f, DrawMs = 0.f;
		bool bExhausted = false, bLowHealth = false;
		int32 PackKg = 0, PackMax = 0, Food = 0, Water = 0;
		FString Subtitle;
		float SubtitleTimer = 0.f;
		bool bForecast = false;
		int32 ForecastIndex = 0;
		int32 WheelIndex = 0;
		int32 DemoIndex = -1;
		float DemoCaption = 0.f;
		float ForecastSeconds = 0.f;
	} Paint;

	/** Serif face for the weather names only (Alegreya, already in the project for the pack's book pages). */
	UPROPERTY(Transient)
	TObjectPtr<class UFont> SerifFont;

	void TickWheel(float DeltaTime);
	int32 WheelSerial = 0;
	float FpsTime = 0.f;
	int32 FpsFrames = 0;
	TArray<float> FrameHistory;
	int32 FrameHead = 0;
	float WheelTime = 0.f;
	bool bWheelOn = false;
	static constexpr float WheelTotal = 4.2f;   // in 0.3, spin 1.0, slow 0.8, result ~2, fade 0.4

	/** A soft tick each time a divider passes the pointer (cut from the user's wheel recording); when it lands, the
	 *  realm's own landing sound (ARealmSky::GetLandingSound: more intense for more dangerous weather). */
	UPROPERTY(Transient)
	TArray<TObjectPtr<class USoundBase>> WheelTicks;
	UPROPERTY(Transient)
	TMap<FString, TObjectPtr<class USoundBase>> WheelLandings;
	bool bWheelSoundsLoaded = false;
	int32 WheelDivider = 0;
	bool bWheelChimed = false;
};
