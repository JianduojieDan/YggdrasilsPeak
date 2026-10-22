// Copyright Epic Games, Inc. All Rights Reserved.

#include "Survival/SurvivalHUDWidget.h"
#include "Survival/SurvivalComponent.h"
#include "Survival/ExpeditionComponent.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"

namespace
{
	TAutoConsoleVariable<int32> CVarHudDebug(
		TEXT("ygg.HUD.Debug"),
		0,
		TEXT("1 shows the survival debug numbers on the HUD, 0 hides them."),
		ECVF_Default);

	/** Moves a bar fill towards its target value and tints it. */
	void UpdateBar(UProgressBar* Bar, float& Displayed, float Target, float Speed, float DeltaTime, const FLinearColor& Colour)
	{
		Displayed = FMath::FInterpTo(Displayed, Target, DeltaTime, Speed);
		if (Bar)
		{
			Bar->SetPercent(Displayed);
			Bar->SetFillColorAndOpacity(Colour);
		}
	}
}

void USurvivalHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// Everything starts hidden; the tick fades in only what is relevant.
	for (UWidget* Widget : { StaminaPanel.Get(), HealthPanel.Get(), HungerIcon.Get(), ThirstIcon.Get(), ColdIcon.Get(),
		TemperaturePanel.Get(), FrostOverlay.Get(), WarningPanel.Get(), DebugPanel.Get() })
	{
		if (Widget)
		{
			Widget->SetRenderOpacity(0.f);
		}
	}

	CreateAltitudeText();
	TryBind();
}

void USurvivalHUDWidget::CreateAltitudeText()
{
	UCanvasPanel* Canvas = WidgetTree ? Cast<UCanvasPanel>(WidgetTree->RootWidget) : nullptr;
	if (AltitudeText || !Canvas)
	{
		return;
	}

	AltitudeText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("AltitudeText"));
	FSlateFontInfo Font = AltitudeText->GetFont();
	Font.Size = 20;
	AltitudeText->SetFont(Font);
	AltitudeText->SetColorAndOpacity(FSlateColor(FLinearColor(0.9f, 0.95f, 1.f, 0.85f)));
	AltitudeText->SetShadowOffset(FVector2D(1.f, 1.f));
	AltitudeText->SetShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.6f));

	UCanvasPanelSlot* TextSlot = Canvas->AddChildToCanvas(AltitudeText);
	TextSlot->SetAnchors(FAnchors(0.f, 1.f));
	TextSlot->SetAlignment(FVector2D(0.f, 1.f));
	TextSlot->SetPosition(FVector2D(40.f, -40.f));
	TextSlot->SetAutoSize(true);
}

void USurvivalHUDWidget::NativeDestruct()
{
	Unbind();
	Super::NativeDestruct();
}

// ---- Binding ----

void USurvivalHUDWidget::TryBind()
{
	const APlayerController* Controller = GetOwningPlayer();
	const APawn* Pawn = Controller ? Controller->GetPawn() : nullptr;
	USurvivalComponent* Found = Pawn ? Pawn->FindComponentByClass<USurvivalComponent>() : nullptr;

	if (Found == Survival.Get())
	{
		return;
	}

	Unbind();
	Survival = Found;

	Expedition = Pawn ? Pawn->FindComponentByClass<UExpeditionComponent>() : nullptr;
	if (Expedition.IsValid())
	{
		Expedition->OnMessage.AddDynamic(this, &USurvivalHUDWidget::HandleExpeditionMessage);
	}

	if (Found)
	{
		Found->OnStatChanged.AddDynamic(this, &USurvivalHUDWidget::HandleStatChanged);
		Found->OnWarningChanged.AddDynamic(this, &USurvivalHUDWidget::HandleWarningChanged);
		Found->OnDeath.AddDynamic(this, &USurvivalHUDWidget::HandleDeath);
		SnapToCurrentState();
	}
}

void USurvivalHUDWidget::Unbind()
{
	if (USurvivalComponent* Bound = Survival.Get())
	{
		Bound->OnStatChanged.RemoveDynamic(this, &USurvivalHUDWidget::HandleStatChanged);
		Bound->OnWarningChanged.RemoveDynamic(this, &USurvivalHUDWidget::HandleWarningChanged);
		Bound->OnDeath.RemoveDynamic(this, &USurvivalHUDWidget::HandleDeath);
	}
	if (UExpeditionComponent* BoundExpedition = Expedition.Get())
	{
		BoundExpedition->OnMessage.RemoveDynamic(this, &USurvivalHUDWidget::HandleExpeditionMessage);
	}
	Survival.Reset();
	Expedition.Reset();
}

void USurvivalHUDWidget::SnapToCurrentState()
{
	const USurvivalComponent* Bound = Survival.Get();
	if (!Bound)
	{
		return;
	}

	DisplayedStamina = Bound->GetStatNormalized(ESurvivalStat::Stamina);
	DisplayedHealth = Bound->GetStatNormalized(ESurvivalStat::Health);
	DisplayedTemperature = Bound->GetStatNormalized(ESurvivalStat::BodyTemperature);

	StaminaLinger = 0.f;
	HealthLinger = 0.f;
	WarningMessageTimer = 0.f;

	const UEnum* WarningEnum = StaticEnum<ESurvivalWarning>();
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(WarningStrength); ++Index)
	{
		const ESurvivalWarning Warning = static_cast<ESurvivalWarning>(WarningEnum->GetValueByIndex(Index));
		WarningStrength[Index] = Bound->IsWarningActive(Warning) ? 1.f : 0.f;
	}
}

// ---- Events ----

void USurvivalHUDWidget::HandleStatChanged(ESurvivalStat Stat, float NewValue, float OldValue)
{
	if (Stat == ESurvivalStat::Health && NewValue < OldValue)
	{
		HealthLinger = HealthLingerSeconds;
	}
}

void USurvivalHUDWidget::HandleWarningChanged(ESurvivalWarning Warning, bool bActive)
{
	if (!bActive)
	{
		return;
	}

	// Announce a warning once, when it starts. The icon keeps pulsing for as long as it stays active.
	if (WarningText)
	{
		WarningText->SetText(USurvivalComponent::GetWarningText(Warning));
	}
	WarningMessageTimer = WarningMessageSeconds;
}

void USurvivalHUDWidget::HandleDeath()
{
	HealthLinger = FLT_MAX;
}

void USurvivalHUDWidget::HandleExpeditionMessage(const FText& Message, float Seconds)
{
	if (WarningText)
	{
		WarningText->SetText(Message);
	}
	WarningMessageTimer = Seconds;
	// Coming back from death: the health panel can go back to normal behaviour.
	if (HealthLinger == FLT_MAX)
	{
		HealthLinger = 0.f;
	}
}

// ---- Per-frame presentation ----

void USurvivalHUDWidget::FadeWidget(UWidget* Widget, float TargetOpacity, float DeltaTime, float InSpeed, float OutSpeed)
{
	if (!Widget)
	{
		return;
	}

	const float Current = Widget->GetRenderOpacity();
	const float Speed = TargetOpacity > Current ? InSpeed : OutSpeed;
	Widget->SetRenderOpacity(FMath::FInterpTo(Current, TargetOpacity, DeltaTime, Speed));
}

void USurvivalHUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	TryBind();

	const USurvivalComponent* Bound = Survival.Get();
	if (!Bound)
	{
		return;
	}

	const FSurvivalSnapshot Snapshot = Bound->GetSnapshot();
	PulseClock += InDeltaTime * PulseSpeed;

	// Stamina: visible while sprinting or recovering, then lingers briefly after it is full.
	const float StaminaNormalized = Bound->GetStatNormalized(ESurvivalStat::Stamina);
	if (Snapshot.bSprinting || StaminaNormalized < 0.99f)
	{
		StaminaLinger = StaminaLingerSeconds;
	}
	else
	{
		StaminaLinger = FMath::Max(0.f, StaminaLinger - InDeltaTime);
	}
	UpdateBar(StaminaBar, DisplayedStamina, StaminaNormalized, BarInterpSpeed, InDeltaTime,
		Snapshot.bExhausted ? ExhaustedColor : StaminaColor);
	FadeWidget(StaminaPanel, StaminaLinger > 0.f ? 1.f : 0.f, InDeltaTime, FadeInSpeed, FadeOutSpeed);

	// Health: visible while low or shortly after taking damage.
	const bool bLowHealth = Bound->IsWarningActive(ESurvivalWarning::LowHealth);
	HealthLinger = FMath::Max(0.f, HealthLinger - InDeltaTime);
	UpdateBar(HealthBar, DisplayedHealth, Bound->GetStatNormalized(ESurvivalStat::Health), BarInterpSpeed, InDeltaTime,
		bLowHealth ? LowHealthColor : HealthColor);
	FadeWidget(HealthPanel, (bLowHealth || HealthLinger > 0.f) ? 1.f : 0.f, InDeltaTime, FadeInSpeed, FadeOutSpeed);

	// Warning icons: fade with the warning, pulse while it is active.
	UWidget* const Icons[6] = { nullptr, HungerIcon.Get(), ThirstIcon.Get(), ColdIcon.Get(), nullptr, nullptr };
	const float Pulse = 0.65f + 0.35f * (0.5f + 0.5f * FMath::Sin(PulseClock));
	for (int32 Index = 0; Index < 6; ++Index)
	{
		const bool bActive = Bound->IsWarningActive(static_cast<ESurvivalWarning>(Index));
		WarningStrength[Index] = FMath::FInterpTo(WarningStrength[Index], bActive ? 1.f : 0.f, InDeltaTime, bActive ? FadeInSpeed : FadeOutSpeed);
		if (Icons[Index])
		{
			Icons[Index]->SetRenderOpacity(WarningStrength[Index] * (bActive ? Pulse : 1.f));
		}
	}

	// Temperature: an indicator once the body cools, frost along the edges when it gets dangerous.
	const float TemperatureNormalized = Bound->GetStatNormalized(ESurvivalStat::BodyTemperature);
	const bool bCold = Bound->IsWarningActive(ESurvivalWarning::Cold);
	UpdateBar(TemperatureBar, DisplayedTemperature, TemperatureNormalized, BarInterpSpeed, InDeltaTime,
		bCold ? ColdTemperatureColor : TemperatureColor);
	FadeWidget(TemperaturePanel, (bCold || TemperatureNormalized < TemperatureShowBelow) ? 1.f : 0.f, InDeltaTime, FadeInSpeed, FadeOutSpeed);

	const float FrostAmount = FMath::GetMappedRangeValueClamped(FVector2D(FrostStartsAt, FrostFullAt), FVector2D(0.f, 1.f), TemperatureNormalized);
	FadeWidget(FrostOverlay, FrostAmount * FrostMaxOpacity, InDeltaTime, 2.f, 1.f);

	// Warning message.
	WarningMessageTimer = FMath::Max(0.f, WarningMessageTimer - InDeltaTime);
	FadeWidget(WarningPanel, WarningMessageTimer > 0.f ? 1.f : 0.f, InDeltaTime, FadeInSpeed, FadeOutSpeed);

	UpdateAltitudeText(Snapshot);

	// Debug numbers.
	const bool bShowDebug = CVarHudDebug.GetValueOnGameThread() != 0;
	FadeWidget(DebugPanel, bShowDebug ? 1.f : 0.f, InDeltaTime, 20.f, 20.f);
	if (bShowDebug)
	{
		UpdateDebugText(Snapshot);
	}
}

void USurvivalHUDWidget::UpdateAltitudeText(const FSurvivalSnapshot& Snapshot) const
{
	if (!AltitudeText)
	{
		return;
	}

	FString Line = FString::Printf(TEXT("%s m     O2 %d%%"), *FText::AsNumber(FMath::RoundToInt(Snapshot.ElevationMeters)).ToString(),
		FMath::RoundToInt(Snapshot.Oxygen));
	if (Snapshot.OxygenBottleSeconds > 0.f)
	{
		Line += FString::Printf(TEXT("   bottle %d:%02d"), FMath::FloorToInt(Snapshot.OxygenBottleSeconds / 60.f), FMath::FloorToInt(Snapshot.OxygenBottleSeconds) % 60);
	}
	if (Snapshot.CarriedBottles > 0)
	{
		Line += FString::Printf(TEXT("   spare x%d"), Snapshot.CarriedBottles);
	}
	if (Snapshot.bOnRope)
	{
		Line += TEXT("   [rope]");
	}
	if (Snapshot.bSheltered)
	{
		Line += TEXT("   [camp]");
	}
	AltitudeText->SetText(FText::FromString(Line));

	// Oxygen colour: white when fine, amber when low, red when dangerous.
	const FLinearColor Colour = Snapshot.Oxygen > 72.f ? FLinearColor(0.9f, 0.95f, 1.f, 0.85f)
		: Snapshot.Oxygen > 55.f ? FLinearColor(1.f, 0.75f, 0.3f, 0.95f) : FLinearColor(1.f, 0.3f, 0.3f, 1.f);
	AltitudeText->SetColorAndOpacity(FSlateColor(Colour));
}

void USurvivalHUDWidget::UpdateDebugText(const FSurvivalSnapshot& Snapshot) const
{
	if (!DebugText)
	{
		return;
	}

	DebugText->SetText(FText::FromString(FString::Printf(
		TEXT("Health %.1f\nStamina %.1f\nSatiety %.1f\nHydration %.1f\nTemperature %.2f C\nOxygen %.1f%%  Elevation %.0f m\nSprinting %d  Exhausted %d  Dead %d"),
		Snapshot.Health, Snapshot.Stamina, Snapshot.Satiety, Snapshot.Hydration, Snapshot.BodyTemperature, Snapshot.Oxygen, Snapshot.ElevationMeters,
		Snapshot.bSprinting, Snapshot.bExhausted, Snapshot.bDead)));
}
