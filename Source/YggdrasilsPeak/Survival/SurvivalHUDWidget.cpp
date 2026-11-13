// Copyright Epic Games, Inc. All Rights Reserved.

#include "Survival/SurvivalHUDWidget.h"
#include "Survival/YggSettings.h"
#include "Survival/YggUIStyle.h"
#include "Survival/InventoryComponent.h"
#include "Survival/SurvivalComponent.h"
#include "Survival/ExpeditionComponent.h"
#include "Environment/RouteLandmark.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "Environment/RealmSky.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"
#include "Engine/Font.h"
#include "Engine/Texture2D.h"
#include "Engine/FontFace.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "DynamicRHI.h"
#include "RenderTimer.h"
#include "Misc/App.h"

namespace
{
	// The weather wheel's motion, shared by the drawing and its tick sounds: steady spin, then one continuous
	// slow-down (same speed where they join), then a small settle onto the real result.
	constexpr float WheelSector = 14.f, WheelSpin = 1.0f, WheelSlow = 0.8f, WheelStop = WheelSpin + WheelSlow;
	float WheelTurnAt(float T)   // degrees still to turn at time T
	{
		const float Round = WheelSector * ARealmSky::NumWeathers();
		const float Speed = 2.f * Round / (WheelSpin + WheelSlow / 3.f);
		const float Left = T < WheelSpin ? Speed * (WheelSpin - T) + Speed * WheelSlow / 3.f
			: Speed * WheelSlow / 3.f * FMath::Pow(1.f - FMath::Clamp((T - WheelSpin) / WheelSlow, 0.f, 1.f), 3.f);
		const float After = FMath::Max(0.f, T - WheelStop);
		return Left + (After > 0.f ? 0.8f * FMath::Exp(-After * 7.f) * FMath::Sin(After * 20.f) : 0.f);
	}

	TAutoConsoleVariable<int32> CVarHudDebug(
		TEXT("ygg.HUD.Debug"),
		0,
		TEXT("1 shows the survival debug numbers on the HUD, 0 hides them."),
		ECVF_Default);

	const FSlateBrush* Hud6Brush(const TCHAR* Name);   // the HUD's line icons (below)

	/** The wheel's soft ticks (Wheel_Tick_01..08, cut from the user's wheel recording). */
	void HudLoadWheelTicks(TArray<TObjectPtr<USoundBase>>& Into)
	{
		for (int32 i = 1; i <= 8; ++i)
		{
			const FString N = FString::Printf(TEXT("Wheel_Tick_%02d"), i);
			if (USoundBase* S = LoadObject<USoundBase>(nullptr, *(TEXT("/Game/Audio/UI/") + N + TEXT(".") + N), nullptr, LOAD_NoWarn | LOAD_Quiet)) { Into.Add(S); }
		}
	}

	/** A realm's landing sound by name (ARealmSky::GetLandingSound), else the plain chime. */
	USoundBase* HudLoadLanding(const FString& Name)
	{
		USoundBase* S = LoadObject<USoundBase>(nullptr, *FString::Printf(TEXT("/Game/Audio/UI/%s.%s"), *Name, *Name), nullptr, LOAD_NoWarn | LOAD_Quiet);
		return S ? S : LoadObject<USoundBase>(nullptr, TEXT("/Game/Audio/UI/UI_Chime_01.UI_Chime_01"), nullptr, LOAD_NoWarn | LOAD_Quiet);
	}

	/** Moves a bar fill towards its target value and tints it and its icon. */
	void UpdateBar(UProgressBar* Bar, UImage* Glyph, float& Displayed, float Target, float Speed, float DeltaTime, const FLinearColor& Colour)
	{
		Displayed = FMath::FInterpTo(Displayed, Target, DeltaTime, Speed);
		if (Bar)
		{
			Bar->SetPercent(Displayed);
			Bar->SetFillColorAndOpacity(Colour);
		}
		if (Glyph)
		{
			Glyph->SetColorAndOpacity(Colour);
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
			SetFadedOpacity(Widget, 0.f);
		}
	}

	// The whole HUD is painted (NativePaint); of the old widgets only the frost and the reading panel stay
	if (UCanvasPanel* Canvas = WidgetTree ? Cast<UCanvasPanel>(WidgetTree->RootWidget) : nullptr)
	{
		for (UWidget* Child : Canvas->GetAllChildren())
		{
			if (Child && Child != FrostOverlay && Child != DebugPanel) { Child->SetVisibility(ESlateVisibility::Collapsed); }
		}
	}
	TryBind();

	if (!SerifFont)
	{
		if (UFontFace* Face = LoadObject<UFontFace>(nullptr, TEXT("/Game/UI/Fonts/FF_Alegreya.FF_Alegreya")))
		{
			SerifFont = NewObject<UFont>(this);
			SerifFont->FontCacheType = EFontCacheType::Runtime;
			FTypefaceEntry& Entry = SerifFont->GetMutableInternalCompositeFont().DefaultTypeface.Fonts.AddDefaulted_GetRef();
			Entry.Name = TEXT("Regular");
			Entry.Font = FFontData(Face);
		}
	}

	// What the HUD shows or plays later is loaded now, with the level (2026-11-14: the icons at the first look at the
	// watch, the wheel's ticks at its first spin and its landing sound when it stopped were each a LoadObject mid-play -
	// the game thread waited for the disk, a freeze whenever the drive stalled). The pause menu's and the death page's
	// assets are kept from here too (YggUI::KeepMenuAssets).
	for (const TCHAR* Icon : { TEXT("Lungs"), TEXT("Thermo"), TEXT("Heart"), TEXT("Food"), TEXT("Drop"), TEXT("Tent") })
	{
		Hud6Brush(Icon);
	}
	if (!bWheelSoundsLoaded)
	{
		bWheelSoundsLoaded = true;
		HudLoadWheelTicks(WheelTicks);
	}
	for (int32 i = 0; i < ARealmSky::NumWeathers(); ++i)
	{
		const FString& Name = ARealmSky::GetLandingSound(i);
		TObjectPtr<USoundBase>& Landing = WheelLandings.FindOrAdd(Name);
		if (!Landing) { Landing = HudLoadLanding(Name); }
	}
	YggUI::KeepMenuAssets();
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

void USurvivalHUDWidget::CreateReadingPanel()
{
	UCanvasPanel* Canvas = WidgetTree ? Cast<UCanvasPanel>(WidgetTree->RootWidget) : nullptr;
	if (ReadingPanel || !Canvas)
	{
		return;
	}
	// v3: no box - a soft pool of shadow behind the words, the page in the serif
	ReadingPanel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("ReadingPanel"));
	ReadingPanel->SetBrush(YggUI::TextureBrush(YggUI::GlowTexture(), FVector2D(64.f, 64.f)));
	ReadingPanel->SetBrushColor(YggUI::WithA(YggUI::InkC, 0.9f));
	ReadingPanel->SetPadding(FMargin(110.f, 80.f));

	UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ReadingBox"));
	ReadingPanel->SetContent(Box);
	ReadingTitle = YggUI::Words(WidgetTree, TEXT(""), 36, YggUI::Snow);
	ReadingBody = YggUI::Words(WidgetTree, TEXT(""), 21, YggUI::Snow);
	ReadingTitle->SetAutoWrapText(true);
	ReadingBody->SetAutoWrapText(true);
	Box->AddChildToVerticalBox(ReadingTitle)->SetPadding(FMargin(0.f, 0.f, 0.f, 16.f));
	Box->AddChildToVerticalBox(ReadingBody);
	UTextBlock* Hint = YggUI::Words(WidgetTree, TEXT("E  to close"), 16, YggUI::Mist);
	Box->AddChildToVerticalBox(Hint)->SetPadding(FMargin(0.f, 22.f, 0.f, 0.f));

	UCanvasPanelSlot* PanelSlot = Canvas->AddChildToCanvas(ReadingPanel);
	PanelSlot->SetAnchors(FAnchors(0.5f, 0.5f));
	PanelSlot->SetAlignment(FVector2D(0.5f, 0.5f));
	PanelSlot->SetSize(FVector2D(720.f, 0.f));
	PanelSlot->SetAutoSize(true);
	ReadingPanel->SetVisibility(ESlateVisibility::Collapsed);
}

void USurvivalHUDWidget::HandleReading(const FText& Title, const FText& Body, bool bOpen)
{
	CreateReadingPanel();
	if (!ReadingPanel)
	{
		return;
	}
	if (bOpen)
	{
		ReadingTitle->SetText(Title);
		ReadingBody->SetText(Body);
		// wrap the body at the panel's width (auto-sized panels otherwise grow to one long line)
		ReadingBody->SetWrapTextAt(660.f);
		ReadingTitle->SetWrapTextAt(660.f);
	}
	ReadingPanel->SetVisibility(bOpen ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
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
		Expedition->OnReading.AddDynamic(this, &USurvivalHUDWidget::HandleReading);
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
		BoundExpedition->OnReading.RemoveDynamic(this, &USurvivalHUDWidget::HandleReading);
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
	Paint.Subtitle = USurvivalComponent::GetWarningText(Warning).ToString();
	WarningMessageTimer = WarningMessageSeconds;
}

void USurvivalHUDWidget::HandleDeath()
{
	HealthLinger = FLT_MAX;
}

void USurvivalHUDWidget::HandleExpeditionMessage(const FText& Message, float Seconds)
{
	Paint.Subtitle = Message.ToString();
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
	SetFadedOpacity(Widget, FMath::FInterpTo(Current, TargetOpacity, DeltaTime, Speed));
}

void USurvivalHUDWidget::SetFadedOpacity(UWidget* Widget, float Opacity)
{
	// The panels are BackgroundBlurs: their blur ignores render opacity, so a faded-out panel would still leave an
	// empty blurred pill on screen. Hide it outright once it is (nearly) transparent; Hidden keeps the stack layout.
	Widget->SetRenderOpacity(Opacity);
	Widget->SetVisibility(Opacity > 0.01f ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
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
	Paint.SubtitleTimer = WarningMessageTimer;
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
	UpdateBar(StaminaBar, StaminaGlyph, DisplayedStamina, StaminaNormalized, BarInterpSpeed, InDeltaTime,
		Snapshot.bExhausted ? ExhaustedColor : StaminaColor);
	
	// Health: visible while low or shortly after taking damage.
	const bool bLowHealth = Bound->IsWarningActive(ESurvivalWarning::LowHealth);
	HealthLinger = FMath::Max(0.f, HealthLinger - InDeltaTime);
	UpdateBar(HealthBar, HealthGlyph, DisplayedHealth, Bound->GetStatNormalized(ESurvivalStat::Health), BarInterpSpeed, InDeltaTime,
		bLowHealth ? LowHealthColor : HealthColor);
	
	// Warning icons: fade with the warning, pulse while it is active.
	UWidget* const Icons[6] = { nullptr, HungerIcon.Get(), ThirstIcon.Get(), ColdIcon.Get(), nullptr, nullptr };
	const float Pulse = 0.65f + 0.35f * (0.5f + 0.5f * FMath::Sin(PulseClock));
	for (int32 Index = 0; Index < 6; ++Index)
	{
		const bool bActive = Bound->IsWarningActive(static_cast<ESurvivalWarning>(Index));
		WarningStrength[Index] = FMath::FInterpTo(WarningStrength[Index], bActive ? 1.f : 0.f, InDeltaTime, bActive ? FadeInSpeed : FadeOutSpeed);
	}
	IconPulse = Pulse;
	Paint.Snapshot = Snapshot;
	Paint.Temperature = DisplayedTemperature; Paint.Health = DisplayedHealth; Paint.Stamina = DisplayedStamina;
	Paint.Satiety = FMath::FInterpTo(Paint.Satiety, Bound->GetStatNormalized(ESurvivalStat::Satiety), InDeltaTime, BarInterpSpeed);
	Paint.Hydration = FMath::FInterpTo(Paint.Hydration, Bound->GetStatNormalized(ESurvivalStat::Hydration), InDeltaTime, BarInterpSpeed);
	Paint.bExhausted = Snapshot.bExhausted; Paint.bLowHealth = bLowHealth;
	// the air eases towards the realm's, so a change of weather is seen coming in the numbers
	if (const ARealmSky* Sky = ARealmSky::Get(GetWorld()))
	{
		const float Target = Sky->GetAirTemperature(Snapshot.ElevationMeters);
		Paint.AirC = Paint.bAirKnown ? FMath::FInterpTo(Paint.AirC, Target, InDeltaTime, 0.6f) : Target;
		Paint.bAirKnown = true;
		Paint.WeatherNow = Sky->GetShownRealm() == ERealm::None ? -1 : Sky->GetShownIndex();
	}
	Paint.Extreme = FMath::Max(FMath::Clamp((-Paint.AirC - 35.f) / 50.f, 0.f, 1.f), FMath::Clamp((Paint.AirC - 30.f) / 25.f, 0.f, 1.f));
	// FPS monitor: real frames per second (averaged over half a second), the 1% low from the last 600 frames
	{
		const float Dt = FMath::Max(float(FApp::GetDeltaTime()), 1e-4f);
		FpsTime += Dt;
		++FpsFrames;
		if (FrameHistory.Num() < 600) { FrameHistory.Add(Dt); }
		else { FrameHistory[FrameHead] = Dt; FrameHead = (FrameHead + 1) % 600; }
		if (FpsTime >= 0.5f)
		{
			Paint.RenderedFps = FpsFrames / FpsTime;
			Paint.Fps = Paint.RenderedFps;
			Paint.FrameMs = 1000.f * FpsTime / FpsFrames;
			// DLSS frame generation presents more frames than the game renders: ask the Streamline plugin for the real
			// presented rate (2026-11-13, user: "with DLSS the FPS count shows only 30"). Through reflection, so the game
			// still builds and runs without the NVIDIA plugin.
			{
				static UClass* Lib = FindObject<UClass>(nullptr, TEXT("/Script/StreamlineDLSSGBlueprint.StreamlineLibraryDLSSG"));
				static UFunction* Timing = Lib ? Lib->FindFunctionByName(TEXT("GetDLSSGFrameTiming")) : nullptr;
				Paint.bFrameGen = false;
				if (Timing && Timing->ParmsSize >= int32(sizeof(float) + sizeof(int32)))
				{
					struct { float FrameRateInHertz = 0.f; int32 FramesPresented = 0; } Parms;
					Lib->GetDefaultObject()->ProcessEvent(Timing, &Parms);
					if (Parms.FramesPresented > 1 && Parms.FrameRateInHertz > Paint.RenderedFps * 1.2f)
					{
						Paint.Fps = Parms.FrameRateInHertz;
						Paint.bFrameGen = true;
					}
				}
			}
			TArray<float> Sorted = FrameHistory;
			Sorted.Sort();
			Paint.LowFps = 1.f / Sorted[FMath::Clamp(FMath::FloorToInt(Sorted.Num() * 0.99f), 0, Sorted.Num() - 1)];
			Paint.GpuMs = float(FPlatformTime::ToMilliseconds(RHIGetGPUFrameCycles(0)));
			Paint.GameMs = float(FPlatformTime::ToMilliseconds(GGameThreadTime));
			Paint.DrawMs = float(FPlatformTime::ToMilliseconds(GRenderThreadTime));
			FpsTime = 0.f;
			FpsFrames = 0;
		}
	}
	// the camps for the route line (names and heights), refreshed now and then
	if ((Paint.CampsAge += InDeltaTime) > 2.f)
	{
		Paint.CampsAge = 0.f;
		Paint.Camps.Reset();
		const APawn* Me = GetOwningPlayerPawn();
		if (const UExpeditionComponent* Exp = Me ? Me->FindComponentByClass<UExpeditionComponent>() : nullptr)
		{
			for (const TWeakObjectPtr<ARouteLandmark>& C : Exp->GetCamps())
			{
				if (!C.IsValid()) { continue; }
				FString Name = C->LandmarkName;
				Name.Split(TEXT(" - "), &Name, nullptr); Name.Split(TEXT(" ("), &Name, nullptr);           // "South Col - Camp 4" -> "South Col"
				Name.ReplaceInline(TEXT("Everest "), TEXT(""));
				Paint.Camps.Add({ Name, Snapshot.ElevationMeters + float(C->GetActorLocation().Z - Me->GetActorLocation().Z) / 100.f });
			}
			Paint.Camps.Sort([](const FPaintState::FCampMark& A, const FPaintState::FCampMark& B) { return A.Elev < B.Elev; });
		}
	}
	// Quiet HUD: the watch comes up only while something vital needs the climber (a warning, thin air, a bottle running)
	{
		bool bNeeded = !YggSettings::HudHidden() || bLowHealth || Snapshot.bExhausted || Snapshot.Oxygen < 72.f || Snapshot.OxygenBottleSeconds > 0.f;
		for (int32 W = 1; W <= 3; ++W) { bNeeded |= WarningStrength[W] > 0.5f; }
		Paint.WatchShown = FMath::FInterpTo(Paint.WatchShown, bNeeded ? 1.f : 0.f, InDeltaTime, bNeeded ? 4.f : 1.2f);
	}
	if (const APawn* Pawn = GetOwningPlayerPawn())
	{
		if (const UInventoryComponent* Pack = Pawn->FindComponentByClass<UInventoryComponent>())
		{
			Paint.PackKg = FMath::RoundToInt(Pack->GetLoadKg()); Paint.PackMax = FMath::RoundToInt(Pack->MaxKg);
			Paint.Food = Pack->GetCount(TEXT("Ration")); Paint.Water = Pack->GetCount(TEXT("Water"));
		}
	}
	TickWheel(InDeltaTime);
	// Temperature: an indicator once the body cools, frost along the edges when it gets dangerous.
	const float TemperatureNormalized = Bound->GetStatNormalized(ESurvivalStat::BodyTemperature);
	const bool bCold = Bound->IsWarningActive(ESurvivalWarning::Cold);
	UpdateBar(TemperatureBar, TemperatureGlyph, DisplayedTemperature, TemperatureNormalized, BarInterpSpeed, InDeltaTime,
		bCold ? ColdTemperatureColor : TemperatureColor);
	
	const float FrostAmount = FMath::GetMappedRangeValueClamped(FVector2D(FrostStartsAt, FrostFullAt), FVector2D(0.f, 1.f), TemperatureNormalized);
	FadeWidget(FrostOverlay, FrostAmount * FrostMaxOpacity, InDeltaTime, 2.f, 1.f);

	// Warning message.
	WarningMessageTimer = FMath::Max(0.f, WarningMessageTimer - InDeltaTime);
	

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

	const FString Altitude = FString::Printf(TEXT("%s %s"), *YggSettings::AltitudeNumber(Snapshot.ElevationMeters), YggSettings::AltitudeUnit());
	const FString Oxygen = FString::Printf(TEXT("%d%%"), FMath::RoundToInt(Snapshot.Oxygen));

	TArray<FString> Status;
	if (Snapshot.OxygenBottleSeconds > 0.f)
	{
		Status.Add(FString::Printf(TEXT("bottle %d:%02d"), FMath::FloorToInt(Snapshot.OxygenBottleSeconds / 60.f), FMath::FloorToInt(Snapshot.OxygenBottleSeconds) % 60));
	}
	if (Snapshot.CarriedBottles > 0)
	{
		Status.Add(FString::Printf(TEXT("spare ×%d"), Snapshot.CarriedBottles));
	}
	if (Snapshot.bOnRope)
	{
		Status.Add(TEXT("rope"));
	}
	if (Snapshot.bSheltered)
	{
		Status.Add(TEXT("camp"));
	}
	if (const APawn* Pawn = GetOwningPlayerPawn())
	{
		if (const UInventoryComponent* Pack = Pawn->FindComponentByClass<UInventoryComponent>())
		{
			Status.Add(FString::Printf(TEXT("pack %d/%d kg  ·  food %d [1]  ·  water %d [2]"), FMath::RoundToInt(Pack->GetLoadKg()),
				FMath::RoundToInt(Pack->MaxKg), Pack->GetCount(TEXT("Ration")), Pack->GetCount(TEXT("Water"))));
		}
	}
	const FString StatusLine = FString::Join(Status, TEXT("  ·  "));

	// Oxygen colour: white when fine, amber when low, red when dangerous.
	const FLinearColor Colour = Snapshot.Oxygen > 72.f ? FLinearColor(0.93f, 0.96f, 1.f, 0.95f)
		: Snapshot.Oxygen > 55.f ? FLinearColor(1.f, 0.75f, 0.3f, 0.95f) : FLinearColor(1.f, 0.3f, 0.3f, 1.f);

	if (!OxygenText)
	{
		// Single-line fallback.
		FString Line = Altitude + TEXT("  ·  O2 ") + Oxygen;
		if (!StatusLine.IsEmpty())
		{
			Line += TEXT("  ·  ") + StatusLine;
		}
		AltitudeText->SetText(FText::FromString(Line));
		AltitudeText->SetColorAndOpacity(FSlateColor(Colour));
		return;
	}

	AltitudeText->SetText(FText::FromString(Altitude));
	OxygenText->SetText(FText::FromString(Oxygen));
	OxygenText->SetColorAndOpacity(FSlateColor(Colour));
	if (OxygenGlyph)
	{
		OxygenGlyph->SetColorAndOpacity(Colour);
	}
	if (StatusText)
	{
		StatusText->SetText(FText::FromString(StatusLine));
		StatusText->SetVisibility(StatusLine.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	}
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

// ---- The painted HUD ----

namespace
{
	// v3 palette (Docs/Development/UI_Redesign_2026-11-13.md), under the HUD's old names so the wheel and icons follow
	const FLinearColor Ivory = FLinearColor::FromSRGBColor(FColor(238, 232, 220));     // Snow
	const FLinearColor Quiet = FLinearColor::FromSRGBColor(FColor(169, 180, 194));     // Mist
	const FLinearColor IceBlue = FLinearColor::FromSRGBColor(FColor(156, 199, 232));   // Ice
	const FLinearColor Coral = FLinearColor::FromSRGBColor(FColor(224, 122, 102));     // Ember
	const FLinearColor Champagne = FLinearColor::FromSRGBColor(FColor(244, 184, 132)); // Sun
	const FLinearColor Amber = FLinearColor::FromSRGBColor(FColor(240, 170, 104));     // a warmer Sun: caution
	const FLinearColor ShadowCol(0.0f, 0.005f, 0.02f, 0.45f);

	struct FPen
	{
		const FGeometry& G; FSlateWindowElementList& Out; int32 Layer; float Alpha;
		FLinearColor A(const FLinearColor& C) const { return FLinearColor(C.R, C.G, C.B, C.A * Alpha); }
		void Line(const TArray<FVector2f>& P, const FLinearColor& C, float T = 1.6f) const
		{
			TArray<FVector2f> S; for (const FVector2f& X : P) { S.Add(X + FVector2f(0.f, 1.f)); }
			FSlateDrawElement::MakeLines(Out, Layer, G.ToPaintGeometry(), S, ESlateDrawEffect::None, A(ShadowCol), true, T + 1.f);
			FSlateDrawElement::MakeLines(Out, Layer + 1, G.ToPaintGeometry(), P, ESlateDrawEffect::None, A(C), true, T);
		}
		void Rect(FVector2f Pos, FVector2f Size, const FLinearColor& C) const
		{
			FSlateDrawElement::MakeBox(Out, Layer + 1, G.ToPaintGeometry(FVector2f(Size), FSlateLayoutTransform(Pos)), FCoreStyle::Get().GetBrush("WhiteBrush"), ESlateDrawEffect::None, A(C));
		}
		float Width(const FString& T, const FSlateFontInfo& F) const { return FSlateApplication::Get().GetRenderer()->GetFontMeasureService()->Measure(T, F).X; }
		void Text(const FString& T, FVector2f Pos, const FSlateFontInfo& F, const FLinearColor& C) const
		{
			// v4 (2026-11-13, user: the shadow pools behind the readings were "not natural"): a thin dark edge that follows
			// each letter (the font's own outline) - readable on bright snow with nothing painted around the words
			FSlateFontInfo O = F;
			// 1 px (2 only on the big numerals), light: a heavier edge read as stamped letters in the close-ups
			O.OutlineSettings.OutlineSize = F.Size >= 34 ? 2 : 1;
			O.OutlineSettings.OutlineColor = A(FLinearColor(0.0f, 0.004f, 0.012f, 0.36f));
			O.OutlineSettings.bSeparateFillAlpha = true;
			FSlateDrawElement::MakeText(Out, Layer + 1, G.ToPaintGeometry(FVector2f(1.f, 1.f), FSlateLayoutTransform(Pos)), T, O, ESlateDrawEffect::None, A(C));
		}
		// v5: text with a soft shadow a pixel or two under it (no outline, no panel) - reads on bright snow and on a bright storm sky
		void TextS(const FString& T, FVector2f Pos, const FSlateFontInfo& F, const FLinearColor& C, float Shadow = 0.5f) const
		{
			const FLinearColor Ink(0.f, 0.02f, 0.06f, 1.f);
			FSlateDrawElement::MakeText(Out, Layer, G.ToPaintGeometry(FVector2f(1.f, 1.f), FSlateLayoutTransform(Pos + FVector2f(1.f, 2.f))), T, F, ESlateDrawEffect::None, A(FLinearColor(Ink.R, Ink.G, Ink.B, C.A * Shadow * 0.32f)));
			FSlateDrawElement::MakeText(Out, Layer, G.ToPaintGeometry(FVector2f(1.f, 1.f), FSlateLayoutTransform(Pos + FVector2f(0.6f, 1.f))), T, F, ESlateDrawEffect::None, A(FLinearColor(Ink.R, Ink.G, Ink.B, C.A * Shadow * 0.55f)));
			FSlateDrawElement::MakeText(Out, Layer + 1, G.ToPaintGeometry(FVector2f(1.f, 1.f), FSlateLayoutTransform(Pos)), T, F, ESlateDrawEffect::None, A(C));
		}
		void TextSOnBase(const FString& T, float X, float BaseY, const FSlateFontInfo& F, const FLinearColor& C, float Shadow = 0.5f) const { TextS(T, FVector2f(X, BaseY - Base(F)), F, C, Shadow); }
		void TextSRightOnBase(const FString& T, float Right, float BaseY, const FSlateFontInfo& F, const FLinearColor& C, float Shadow = 0.5f) const { TextSOnBase(T, Right - Width(T, F), BaseY, F, C, Shadow); }
		// a small diamond (you, on the route line)
		void Diamond(FVector2f At, float R, const FLinearColor& C) const
		{
			const float Side = R * 1.41421f;
			FSlateDrawElement::MakeBox(Out, Layer + 1, G.ToPaintGeometry(FVector2f(Side, Side), FSlateLayoutTransform(At - FVector2f(Side * 0.5f, Side * 0.5f)),
				FSlateRenderTransform(FQuat2D(FMath::DegreesToRadians(45.f))), FVector2f(0.5f, 0.5f)), FCoreStyle::Get().GetBrush("WhiteBrush"), ESlateDrawEffect::None, A(C));
		}
		// an arc of a ring, Deg0..Deg1 clockwise from 12 o'clock, with a dark edge one pixel wider on each side
		void Arc(FVector2f At, float R, float Deg0, float Deg1, const FLinearColor& C, float T) const
		{
			if (Deg1 - Deg0 < 0.2f) { return; }
			TArray<FVector2f> Pts;
			const int32 N = FMath::Max(2, FMath::CeilToInt((Deg1 - Deg0) / 4.f));
			for (int32 i = 0; i <= N; ++i)
			{
				const float D = FMath::DegreesToRadians(FMath::Lerp(Deg0, Deg1, i / float(N)) - 90.f);
				Pts.Add(At + FVector2f(FMath::Cos(D), FMath::Sin(D)) * R);
			}
			FSlateDrawElement::MakeLines(Out, Layer, G.ToPaintGeometry(), Pts, ESlateDrawEffect::None, A(FLinearColor(0.f, 0.004f, 0.012f, 0.42f * C.A)), true, T + 2.f);
			FSlateDrawElement::MakeLines(Out, Layer + 1, G.ToPaintGeometry(), Pts, ESlateDrawEffect::None, A(C), true, T);
		}
		void TextRight(const FString& T, FVector2f Right, const FSlateFontInfo& F, const FLinearColor& C) const { Text(T, Right - FVector2f(Width(T, F), 0.f), F, C); }
		// distance from the top of a text box to its baseline: mixed sizes share one baseline
		float Base(const FSlateFontInfo& F) const
		{
			const TSharedRef<FSlateFontMeasure> FM = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
			return FM->GetMaxCharacterHeight(F) + FM->GetBaseline(F);
		}
		void TextOnBase(const FString& T, float X, float BaseY, const FSlateFontInfo& F, const FLinearColor& C) const { Text(T, FVector2f(X, BaseY - Base(F)), F, C); }
		// a soft glow (the light over a bar's fill)
		void Glow(FVector2f Pos, FVector2f Size, const FLinearColor& C) const
		{
			static FSlateBrush B;
			if (!B.GetResourceObject()) { B = YggUI::TextureBrush(YggUI::GlowTexture(), FVector2D(64.f, 64.f)); }
			FSlateDrawElement::MakeBox(Out, Layer, G.ToPaintGeometry(FVector2f(Size), FSlateLayoutTransform(Pos)), &B, ESlateDrawEffect::None, A(C));
		}
		// a key's name in a soft outline, its right edge at Right; returns its width
		float Key(const FString& K, FVector2f Right, const FSlateFontInfo& F) const
		{
			const float W = Width(K, F) + 10.f, H = 16.f;
			const FVector2f TL = Right - FVector2f(W, 0.f);
			const TArray<FVector2f> Box = { TL, TL + FVector2f(W, 0.f), TL + FVector2f(W, H), TL + FVector2f(0.f, H), TL };
			FSlateDrawElement::MakeLines(Out, Layer + 1, G.ToPaintGeometry(), Box, ESlateDrawEffect::None, A(FLinearColor(Quiet.R, Quiet.G, Quiet.B, 0.45f)), true, 1.f);
			FSlateDrawElement::MakeText(Out, Layer + 1, G.ToPaintGeometry(FVector2f(1.f, 1.f), FSlateLayoutTransform(TL + FVector2f(5.f, 1.f))), K, F, ESlateDrawEffect::None, A(Ivory));
			return W;
		}
		// a small filled bead (the end of a bar)
		void Bead(FVector2f At, float R, const FLinearColor& C) const { Circle(At, R * 0.5f, C, R); }
		void TextCentre(const FString& T, FVector2f Mid, const FSlateFontInfo& F, const FLinearColor& C) const { Text(T, Mid - FVector2f(Width(T, F) * 0.5f, 0.f), F, C); }
		void Shape(FVector2f At, float S, std::initializer_list<FVector2f> Unit, const FLinearColor& C, float T = 1.6f) const
		{
			TArray<FVector2f> P; for (const FVector2f& U : Unit) { P.Add(At + U * S); }
			Line(P, C, T);
		}
		void Circle(FVector2f At, float R, const FLinearColor& C, float T = 1.6f, float From = 0.f, float To = 360.f) const
		{
			TArray<FVector2f> P; const int32 N = 24;
			for (int32 i = 0; i <= N; ++i) { const float A0 = FMath::DegreesToRadians(FMath::Lerp(From, To, i / float(N))); P.Add(At + FVector2f(FMath::Cos(A0), FMath::Sin(A0)) * R); }
			Line(P, C, T);
		}
	};

	// One outline icon family: same stroke, same box (unit -1..1)
	enum class EIcon { Thermo, Heart, Bolt, Bowl, Drop, Flake, Mountain, Lungs, Sun, Mist, Ember, Storm, Pearl, Wave, Eclipse, Star, Ring };
	void DrawIcon(const FPen& P, EIcon I, FVector2f At, float S, const FLinearColor& C, float T = 1.6f)
	{
		switch (I)
		{
		case EIcon::Thermo:
			P.Circle(At + FVector2f(0.f, 0.55f) * S, 0.38f * S, C, T, -60.f, 240.f);
			P.Shape(At, S, { {-0.18f, 0.22f}, {-0.18f, -0.75f}, {0.f, -0.95f}, {0.18f, -0.75f}, {0.18f, 0.22f} }, C, T); break;
		case EIcon::Heart:
			P.Shape(At, S, { {0.f, 0.85f}, {-0.85f, -0.05f}, {-0.85f, -0.45f}, {-0.55f, -0.75f}, {-0.2f, -0.75f}, {0.f, -0.45f}, {0.2f, -0.75f}, {0.55f, -0.75f}, {0.85f, -0.45f}, {0.85f, -0.05f}, {0.f, 0.85f} }, C, T); break;
		case EIcon::Bolt:
			P.Shape(At, S, { {0.2f, -0.95f}, {-0.5f, 0.15f}, {-0.02f, 0.15f}, {-0.2f, 0.95f}, {0.5f, -0.2f}, {0.02f, -0.2f}, {0.2f, -0.95f} }, C, T); break;
		case EIcon::Bowl:
			P.Shape(At, S, { {-0.9f, -0.05f}, {0.9f, -0.05f}, {0.6f, 0.65f}, {-0.6f, 0.65f}, {-0.9f, -0.05f} }, C, T);
			P.Shape(At, S, { {-0.3f, -0.8f}, {-0.15f, -0.55f}, {-0.3f, -0.3f} }, C, T); P.Shape(At, S, { {0.25f, -0.8f}, {0.4f, -0.55f}, {0.25f, -0.3f} }, C, T); break;
		case EIcon::Drop:
			P.Circle(At + FVector2f(0.f, 0.3f) * S, 0.55f * S, C, T, -25.f, 205.f);
			P.Shape(At, S, { {-0.5f, 0.07f}, {0.f, -0.95f}, {0.5f, 0.07f} }, C, T); break;
		case EIcon::Flake:
			for (int32 k = 0; k < 3; ++k)
			{
				const float A0 = FMath::DegreesToRadians(30.f + 60.f * k); const FVector2f D(FMath::Cos(A0), FMath::Sin(A0));
				P.Shape(At, S, { -D * 0.95f, D * 0.95f }, C, T);
			}
			break;
		case EIcon::Mountain:
			P.Shape(At, S, { {-1.2f, 0.7f}, {-0.35f, -0.65f}, {0.05f, -0.05f}, {0.45f, -0.4f}, {1.2f, 0.7f}, {-1.2f, 0.7f} }, C, T);
			P.Shape(At, S, { {-0.62f, -0.2f}, {-0.35f, -0.05f}, {-0.15f, -0.3f} }, C, T); break;
		case EIcon::Lungs:
			P.Shape(At, S, { {0.f, -0.95f}, {0.f, -0.1f}, {-0.3f, 0.15f} }, C, T); P.Shape(At, S, { {0.f, -0.1f}, {0.3f, 0.15f} }, C, T);
			P.Shape(At, S, { {-0.2f, -0.45f}, {-0.75f, 0.05f}, {-0.95f, 0.75f}, {-0.3f, 0.75f}, {-0.2f, -0.45f} }, C, T);
			P.Shape(At, S, { {0.2f, -0.45f}, {0.75f, 0.05f}, {0.95f, 0.75f}, {0.3f, 0.75f}, {0.2f, -0.45f} }, C, T); break;
		case EIcon::Sun:
			P.Circle(At, 0.45f * S, C, T);
			for (int32 k = 0; k < 8; ++k) { const float A0 = FMath::DegreesToRadians(45.f * k); const FVector2f D(FMath::Cos(A0), FMath::Sin(A0)); P.Shape(At, S, { D * 0.65f, D * 0.98f }, C, T); }
			break;
		case EIcon::Mist:
			P.Shape(At, S, { {-0.95f, -0.45f}, {0.45f, -0.45f} }, C, T); P.Shape(At, S, { {-0.55f, 0.f}, {0.95f, 0.f} }, C, T); P.Shape(At, S, { {-0.95f, 0.45f}, {0.3f, 0.45f} }, C, T); break;
		case EIcon::Ember:
			P.Shape(At, S, { {0.f, -0.95f}, {0.5f, -0.15f}, {0.55f, 0.35f}, {0.2f, 0.85f}, {-0.2f, 0.85f}, {-0.55f, 0.35f}, {-0.35f, -0.1f}, {-0.15f, 0.1f}, {0.f, -0.95f} }, C, T); break;
		case EIcon::Storm:
			DrawIcon(P, EIcon::Flake, At - FVector2f(0.35f, 0.35f) * S, 0.55f * S, C, T);
			P.Shape(At, S, { {-0.95f, 0.4f}, {0.55f, 0.4f}, {0.85f, 0.15f}, {0.6f, -0.05f} }, C, T); P.Shape(At, S, { {-0.6f, 0.75f}, {0.7f, 0.75f} }, C, T); break;
		case EIcon::Pearl:
			P.Shape(At, S, { {0.f, -0.95f}, {0.2f, -0.2f}, {0.95f, 0.f}, {0.2f, 0.2f}, {0.f, 0.95f}, {-0.2f, 0.2f}, {-0.95f, 0.f}, {-0.2f, -0.2f}, {0.f, -0.95f} }, C, T); break;
		case EIcon::Wave:
			for (float Y : { -0.4f, 0.1f, 0.6f }) { P.Shape(At, S, { {-0.95f, Y}, {-0.5f, Y - 0.25f}, {0.f, Y}, {0.5f, Y + 0.2f}, {0.95f, Y - 0.1f} }, C, T); }
			break;
		case EIcon::Eclipse:
			P.Circle(At, 0.6f * S, C, T); P.Circle(At + FVector2f(0.22f, -0.1f) * S, 0.5f * S, C, T, 30.f, 250.f); break;
		case EIcon::Star:
			P.Shape(At, S, { {0.75f, -0.8f}, {-0.4f, 0.4f} }, C, T); P.Shape(At, S, { {-0.1f, -0.75f}, {-0.75f, 0.f} }, C, T);
			DrawIcon(P, EIcon::Pearl, At + FVector2f(-0.45f, 0.5f) * S, 0.42f * S, C, T); break;
		case EIcon::Ring:
			P.Circle(At, 0.45f * S, C, T);
			P.Shape(At, S, { {-0.45f, 0.1f}, {-0.98f, 0.32f}, {-0.6f, 0.42f}, {0.6f, -0.42f}, {0.98f, -0.32f}, {0.45f, -0.1f} }, C, T); break;
		}
	}
	const EIcon WeatherIcons[9] = { EIcon::Sun, EIcon::Mist, EIcon::Ember, EIcon::Storm, EIcon::Pearl, EIcon::Wave, EIcon::Eclipse, EIcon::Star, EIcon::Ring };

	// Type: a light sans everywhere, letter-spaced small capitals for labels; the serif only names the weather
	FSlateFontInfo Sans(const TCHAR* Style, int32 Size, int32 Track = 0)
	{
		FSlateFontInfo F = FCoreStyle::GetDefaultFontStyle(FName(Style), Size);
		F.LetterSpacing = Track;
		return F;
	}
	FLinearColor WithA(const FLinearColor& C, float A) { return FLinearColor(C.R, C.G, C.B, A); }
	/** HUD v8 line icons (Tools/UI/hud_icons.py -> /Game/UI/HUD/Icons6/T_HUD6_<Name>), white, tinted when drawn. Kept for
	 *  the session; the brushes never move (the element list holds pointers to them until the frame is drawn). */
	const FSlateBrush* Hud6Brush(const TCHAR* Name)
	{
		static TMap<FName, TSharedPtr<FSlateBrush>> Cache;
		TSharedPtr<FSlateBrush>& B = Cache.FindOrAdd(FName(Name));
		if (!B.IsValid())
		{
			B = MakeShared<FSlateBrush>();
			const FString Path = FString::Printf(TEXT("/Game/UI/HUD/Icons6/T_HUD6_%s.T_HUD6_%s"), Name, Name);
			if (UTexture2D* T = LoadObject<UTexture2D>(nullptr, *Path, nullptr, LOAD_NoWarn | LOAD_Quiet))
			{
				T->AddToRoot();
				*B = YggUI::TextureBrush(T, FVector2D(128.f, 128.f));
			}
		}
		return B->GetResourceObject() ? B.Get() : nullptr;
	}
	/** "VEIL OF MIST" -> "Veil of Mist" (the weather names are stored in capitals; v3 has no shouting). */
	FString HudTitle(const FString& In)
	{
		TArray<FString> W;
		In.ToLower().ParseIntoArray(W, TEXT(" "));
		for (int32 i = 0; i < W.Num(); ++i)
		{
			if (W[i].Len() == 0) { continue; }
			if (i > 0 && (W[i] == TEXT("of") || W[i] == TEXT("the") || W[i] == TEXT("and"))) { continue; }
			W[i][0] = FChar::ToUpper(W[i][0]);
		}
		return FString::Join(W, TEXT(" "));
	}
}

void USurvivalHUDWidget::TickWheel(float DeltaTime)
{
	ARealmSky* Sky = ARealmSky::Get(GetWorld());
	if (!Sky) { return; }
	const bool bHas = Sky->HasForecast();
	if (Sky->GetForecastSerial() != WheelSerial)
	{
		WheelSerial = Sky->GetForecastSerial();
		const bool bByHand = Sky->TakeHandWheel();
		Paint.WheelIndex = (bHas && !bByHand) ? Sky->GetForecastIndex() : Sky->GetWheelIndex();
		WheelTime = 0.f;
		bWheelOn = true;
		WheelDivider = FMath::FloorToInt(WheelTurnAt(0.f) / WheelSector + 0.5f);
		bWheelChimed = false;
	}
	if (bWheelOn)
	{
		if (!bWheelSoundsLoaded)                                           // normally done in NativeConstruct
		{
			bWheelSoundsLoaded = true;
			HudLoadWheelTicks(WheelTicks);
		}
		// a divider sits half a sector from each icon: one tick each time one passes the pointer (the settle is far smaller)
		const float T = WheelTime + DeltaTime;
		const int32 Divider = FMath::FloorToInt(WheelTurnAt(T) / WheelSector + 0.5f);
		if (Divider != WheelDivider && WheelTicks.Num() > 0 && T < WheelStop + 0.3f)
		{
			const float Fast = FMath::Clamp(1.f - T / WheelStop, 0.f, 1.f);
			UGameplayStatics::PlaySound2D(this, WheelTicks[FMath::RandRange(0, WheelTicks.Num() - 1)], FMath::Lerp(0.45f, 0.3f, Fast), FMath::FRandRange(0.97f, 1.03f) * FMath::Lerp(1.f, 1.06f, Fast));
		}
		WheelDivider = Divider;
		if (!bWheelChimed && T >= WheelStop + 0.05f)
		{
			bWheelChimed = true;
			// fate decided: the realm's own landing sound, heavier the more dangerous the weather it brings
			const FString& Name = ARealmSky::GetLandingSound(Paint.WheelIndex);
			TObjectPtr<USoundBase>& Landing = WheelLandings.FindOrAdd(Name);
			if (!Landing)                                                  // loaded in NativeConstruct unless the names changed since
			{
				Landing = HudLoadLanding(Name);
			}
			if (Landing)
			{
				UGameplayStatics::PlaySound2D(this, Landing, 0.8f);
				UE_LOG(LogTemp, Display, TEXT("Wheel landed on %s (danger %d): %s"), ARealmSky::GetWeatherInfo(Paint.WheelIndex).Name, ARealmSky::GetWeatherInfo(Paint.WheelIndex).Danger, *Landing->GetName());
			}
		}
	}
	// In the presentation the weather now is named quietly between wheels; in play the view stays clean
	const int32 Shown = Sky->IsDemo() ? Sky->GetShownIndex() : -1;
	if (Shown >= 0 && Shown < ARealmSky::NumWeathers() && !bWheelOn) { Paint.DemoIndex = Shown; Paint.DemoCaption = FMath::Min(1.f, Paint.DemoCaption + DeltaTime * 2.f); }
	else { Paint.DemoCaption = FMath::Max(0.f, Paint.DemoCaption - DeltaTime * 4.f); }
	if (bWheelOn) { WheelTime += DeltaTime; if (WheelTime > WheelTotal) { bWheelOn = false; } }
	Paint.bForecast = bHas;
	Paint.ForecastIndex = bHas ? Sky->GetForecastIndex() : 0;
	Paint.ForecastSeconds = bHas ? Sky->GetForecastSeconds() : 0.f;
}

int32 USurvivalHUDWidget::NativePaint(const FPaintArgs& Args, const FGeometry& G, const FSlateRect& Cull, FSlateWindowElementList& Out, int32 LayerId, const FWidgetStyle& Style, bool bParentEnabled) const
{
	int32 Layer = Super::NativePaint(Args, G, Cull, Out, LayerId, Style, bParentEnabled) + 1;
	if (!Survival.IsValid()) { return Layer; }
	const FVector2f Size = FVector2f(G.GetLocalSize());
	// Settings > Game > HUD: Everything (default) shows every reading; Quiet keeps the screen empty until something vital
	// needs the climber (the watch fades in, Paint.WatchShown). Messages and the weather wheel show in both.
	const bool bQuiet = YggSettings::HudHidden();
	const FPen P{ G, Out, Layer, 1.f };
	const float M = 56.f;
	const float Pulse = IconPulse;
	// v3 (2026-11-13): the menus' language - the serif in sentence case, soft lines of light, words for what the body
	// lacks; no tracked capitals, no outline icons in the readings (the weather keeps its icons)
	const FSlateFontInfo Word = YggUI::SerifInfo(18);
	const FSlateFontInfo Small = YggUI::SerifInfo(17);
	const FSlateFontInfo BigNum = YggUI::SerifInfo(42);
	const FSlateFontInfo BigUnit = YggUI::SerifInfo(21);
	const FSlateFontInfo MidNum = YggUI::SerifInfo(23);
	const FSlateFontInfo KeyF = Sans(TEXT("Medium"), 9, 60);
	const FSlateFontInfo Line = YggUI::SerifInfo(19);
	const auto Serif = [](int32 Sz, int32 Track) { (void)Track; return YggUI::SerifInfo(Sz); };

	// v8 (2026-11-13): the Altimeter developed (Tools/UI/hud_v8_mockup.py). The user: "what we had before is acceptable,
	// develop based on that... modernize, minimalist, futuristic, elegant". One typeface (Roboto), white and one red (danger
	// only), line icons instead of words, the next camp beside the altitude, the route as camps on a line, the weather and
	// the air top right. The air runs -100..+60 C with the realms; extreme air grows and turns red there (the second
	// warning beside the wheel). A light shadow under everything; no panels, no halos.
	const FSurvivalSnapshot& Snap = Paint.Snapshot;
	// drawn for 1080 lines; Settings > Game > HUD size (user 2026-11-13: "the UI at the bottom left is too large" - Normal
	// is now about three quarters of the first v8 size)
	const int32 HudSize = YggSettings::GetInt(TEXT("HudSize"), 1);
	const float S = FMath::Clamp(Size.Y / 1080.f, 0.75f, 1.6f) * (HudSize == 0 ? 0.62f : HudSize == 2 ? 0.92f : 0.76f);
	const float Wa = Paint.WatchShown;
	const FLinearColor White = FLinearColor::FromSRGBColor(FColor(244, 246, 248));
	const FLinearColor Red = FLinearColor::FromSRGBColor(FColor(238, 70, 58));
	constexpr float Soft = 0.72f;                                           // the second voice: units, hints, what is not yet climbed
	// Slate sizes are points: px * 0.75; LetterSpacing in thousandths of an em
	const auto Roboto = [S](const TCHAR* Face, float Px, int32 Track = 0)
	{
		FSlateFontInfo F = FCoreStyle::GetDefaultFontStyle(FName(Face), FMath::Max(6, FMath::RoundToInt(Px * 0.75f * S)));
		F.LetterSpacing = Track;
		return F;
	};
	const auto Icon = [&](const FPen& Pn, const TCHAR* Name, FVector2f TopLeft, float H, const FLinearColor& C)
	{
		if (const FSlateBrush* B = Hud6Brush(Name))
		{
			FSlateDrawElement::MakeBox(Out, Pn.Layer, G.ToPaintGeometry(FVector2f(H, H), FSlateLayoutTransform(TopLeft + FVector2f(0.5f, 1.f))), B, ESlateDrawEffect::None, Pn.A(FLinearColor(0.f, 0.024f, 0.063f, 0.45f * C.A)));
			FSlateDrawElement::MakeBox(Out, Pn.Layer + 1, G.ToPaintGeometry(FVector2f(H, H), FSlateLayoutTransform(TopLeft)), B, ESlateDrawEffect::None, Pn.A(C));
		}
	};
	const float X0 = M;
	const float LY = Size.Y - 58.f * S;                                     // the route line
	const float TY = LY - 158.f * S;                                        // baseline of the readings
	const float BlockW = 470.f * S;                                         // the bottom-left block's one width (readings = route line)

	// what needs you, most urgent first
	struct FNeed { FString Word, Hint; float Strength; };
	TArray<FNeed> Needs;
	const float Cold = WarningStrength[3], Hungry = WarningStrength[1], Thirsty = WarningStrength[2];
	if (Cold > 0.02f) { Needs.Add({ TEXT("Freezing"), TEXT("find shelter or warm up"), Cold }); }
	if (Snap.Oxygen < 60.f) { Needs.Add({ TEXT("Thin air"), Snap.CarriedBottles > 0 ? TEXT("slow down, or open a bottle") : TEXT("slow down"), FMath::Clamp((60.f - Snap.Oxygen) / 8.f, 0.4f, 1.f) }); }
	if (Paint.bLowHealth) { Needs.Add({ TEXT("Hurt"), TEXT("rest at a camp"), 1.f }); }
	if (Thirsty > 0.02f) { Needs.Add({ TEXT("Thirsty"), FString::Printf(TEXT("drink  %s"), *YggSettings::KeyLabel(TEXT("Drink"), EKeys::Two)), Thirsty }); }
	if (Hungry > 0.02f) { Needs.Add({ TEXT("Hungry"), FString::Printf(TEXT("eat  %s"), *YggSettings::KeyLabel(TEXT("Eat"), EKeys::One)), Hungry }); }
	if (Paint.bExhausted) { Needs.Add({ TEXT("Out of breath"), TEXT("stand still a moment"), 1.f }); }

	if (Wa > 0.01f)
	{
		const FPen W{ G, Out, Layer, Wa };
		// ---- five readings: an icon and a light numeral on one baseline, a fine line under both
		struct FRead { const TCHAR* Icon; FString Value, Unit; float V; bool bAlert; };
		const FRead Reads[5] = {
			{ TEXT("Lungs"), FString::FromInt(FMath::RoundToInt(Snap.Oxygen)), TEXT("%"), Snap.Oxygen / 100.f, Snap.Oxygen < 60.f },
			{ TEXT("Thermo"), FString::Printf(TEXT("%.1f°"), Snap.BodyTemperature), TEXT(""), Paint.Temperature, Cold > 0.5f },
			{ TEXT("Heart"), FString::FromInt(FMath::RoundToInt(Paint.Health * 100.f)), TEXT(""), Paint.Health, Paint.bLowHealth },
			{ TEXT("Food"), FString::FromInt(FMath::RoundToInt(Paint.Satiety * 100.f)), TEXT(""), Paint.Satiety, Hungry > 0.5f },
			{ TEXT("Drop"), FString::FromInt(FMath::RoundToInt(Paint.Hydration * 100.f)), TEXT(""), Paint.Hydration, Thirsty > 0.5f } };
		// one width for the whole block (user 2026-11-13: the five readings must be as long as the altitude and route
		// below): the last reading's line ends exactly where the route line ends
		const FSlateFontInfo ValF = Roboto(TEXT("Light"), 28.f), UnitF = Roboto(TEXT("Regular"), 14.f);
		const float BarW = 72.f * S, Step = (BlockW - BarW) / 4.f, IconH = 20.f * S, BarH = FMath::Max(2.f, 2.f * S);
		float CX = X0;
		for (const FRead& R : Reads)
		{
			if (bQuiet && !R.bAlert) { continue; }                          // Quiet: only what needs you
			const FLinearColor C = R.bAlert ? Red : White;
			const float V = FMath::Clamp(R.V, 0.f, 1.f);
			if (R.bAlert) { W.Glow(FVector2f(CX - 6.f * S, TY + 4.f * S), FVector2f(BarW * V + 12.f * S, 16.f * S), WithA(Red, 0.55f * (0.7f + 0.3f * Pulse))); }
			Icon(W, R.Icon, FVector2f(CX, TY - IconH), IconH, WithA(C, R.bAlert ? 1.f : 0.92f));
			const float NX = CX + IconH + 9.f * S;
			W.TextSOnBase(R.Value, NX, TY, ValF, C, 0.6f);
			if (!R.Unit.IsEmpty()) { W.TextSOnBase(R.Unit, NX + W.Width(R.Value, ValF) + 2.f * S, TY, UnitF, WithA(C, 0.8f), 0.6f); }
			W.Rect(FVector2f(CX, TY + 11.f * S + 1.f), FVector2f(BarW, BarH), FLinearColor(0.f, 0.024f, 0.063f, 0.2f));   // the line's own shadow
			W.Rect(FVector2f(CX, TY + 11.f * S), FVector2f(BarW, BarH), WithA(White, 0.3f));
			W.Rect(FVector2f(CX, TY + 11.f * S), FVector2f(BarW * V, BarH), WithA(C, 0.95f));
			CX += Step;
		}
		if (!bQuiet)
		{
			const FSlateFontInfo CapF = Roboto(TEXT("Medium"), 11.f, 220);
			// ---- the altitude, the hero
			const FSlateFontInfo AltF = Roboto(TEXT("Light"), 80.f, -25), UnitBig = Roboto(TEXT("Light"), 26.f);
			const FString Alt = YggSettings::AltitudeNumber(Snap.ElevationMeters);
			W.TextSOnBase(TEXT("ALTITUDE"), X0, LY - 104.f * S, CapF, WithA(White, Soft), 0.6f);
			if (Snap.ElevationMeters >= 8000.f)          // above 8,000 m: the death zone, named beside the label
			{
				W.TextSOnBase(TEXT("DEATH ZONE"), X0 + W.Width(TEXT("ALTITUDE"), CapF) + 14.f * S, LY - 104.f * S, CapF, WithA(Red, 0.75f + 0.25f * Pulse), 0.6f);
			}
			W.TextSOnBase(Alt, X0 - 4.f * S, LY - 22.f * S, AltF, WithA(White, 0.98f), 0.5f);
			const float AX = X0 - 4.f * S + W.Width(Alt, AltF);
			W.TextSOnBase(YggSettings::AltitudeUnit(), AX + 7.f * S, LY - 22.f * S, UnitBig, WithA(White, Soft), 0.5f);
			// ---- the next camp, its own column beside the altitude
			const FPaintState::FCampMark* Next = Paint.Camps.FindByPredicate([&](const FPaintState::FCampMark& C) { return C.Elev > Snap.ElevationMeters + 15.f; });
			const float DX = AX + 7.f * S + W.Width(YggSettings::AltitudeUnit(), UnitBig) + 30.f * S;
			W.Line({ FVector2f(DX, LY - 110.f * S), FVector2f(DX, LY - 24.f * S) }, WithA(White, 0.3f), 1.f);
			const float NC = DX + 26.f * S;
			W.TextSOnBase(Next ? TEXT("NEXT CAMP") : TEXT("SUMMIT"), NC, LY - 104.f * S, CapF, WithA(White, Soft), 0.6f);
			Icon(W, TEXT("Tent"), FVector2f(NC, LY - 72.f * S), 20.f * S, WithA(White, 0.95f));
			const FSlateFontInfo NameF = Roboto(TEXT("Regular"), 20.f), MetreF = Roboto(TEXT("Light"), 20.f);
			W.TextSOnBase(Next ? Next->Name : FString(TEXT("Reached")), NC + 30.f * S, LY - 54.f * S, NameF, WithA(White, 0.97f), 0.6f);
			if (Next)
			{
				W.TextSOnBase(FString::Printf(TEXT("+%s %s"), *YggSettings::AltitudeNumber(Next->Elev - Snap.ElevationMeters), YggSettings::AltitudeUnit()),
					NC, LY - 22.f * S, MetreF, WithA(White, Soft), 0.6f);
			}
			// ---- the route: Dingboche to the summit, a dot per camp, the climbed part lit, you a ring, the summit a triangle
			if (Paint.Camps.Num() >= 2)
			{
				const float Lo = Paint.Camps[0].Elev, Hi = FMath::Max(Paint.Camps.Last().Elev, Lo + 1.f), LW = BlockW;
				const auto LX = [&](float E) { return X0 + FMath::Clamp((E - Lo) / (Hi - Lo), 0.f, 1.f) * LW; };
				const float You = LX(Snap.ElevationMeters);
				W.Line({ FVector2f(You, LY), FVector2f(X0 + LW, LY) }, WithA(White, 0.35f), 1.f);
				W.Glow(FVector2f(X0 - 4.f * S, LY - 6.f * S), FVector2f(You - X0 + 8.f * S, 12.f * S), WithA(White, 0.22f));
				W.Line({ FVector2f(X0, LY), FVector2f(You, LY) }, WithA(White, 0.95f), FMath::Max(2.f, 2.f * S));
				for (int32 i = 0; i < Paint.Camps.Num() - 1; ++i)
				{
					const FPaintState::FCampMark& C = Paint.Camps[i];
					if (C.Elev <= Snap.ElevationMeters) { W.Bead(FVector2f(LX(C.Elev), LY), 3.f * S, WithA(White, 0.95f)); }
					else { W.Circle(FVector2f(LX(C.Elev), LY), 3.f * S, WithA(White, 0.5f), 1.f); }
				}
				const float SX = LX(Paint.Camps.Last().Elev);
				W.Line({ FVector2f(SX - 5.f * S, LY + 3.f * S), FVector2f(SX, LY - 6.f * S), FVector2f(SX + 5.f * S, LY + 3.f * S), FVector2f(SX - 5.f * S, LY + 3.f * S) }, WithA(White, 0.6f), 1.f);
				W.Circle(FVector2f(You, LY), 6.5f * S, WithA(White, 1.f), FMath::Max(1.5f, 2.f * S));
			}
		}
		// ---- what needs you: one line above the readings, the only sentence on the left
		if (Needs.Num())
		{
			const FNeed& N = Needs[0];
			const float AY = TY - 52.f * S, K = FMath::Clamp(N.Strength, 0.f, 1.f);
			const FSlateFontInfo WordF = Roboto(TEXT("Medium"), 16.f), HintF = Roboto(TEXT("Regular"), 16.f);
			W.Bead(FVector2f(X0 + 4.f * S, AY - 5.f * S), 3.5f * S, WithA(Red, K * (0.75f + 0.25f * Pulse)));
			W.TextSOnBase(N.Word, X0 + 16.f * S, AY, WordF, WithA(Red, K), 0.6f);
			W.TextSOnBase(N.Hint, X0 + 16.f * S + W.Width(N.Word, WordF) + 10.f * S, AY, HintF, WithA(White, 0.88f * K), 0.6f);
			if (Needs.Num() > 1)
			{
				FString More;
				for (int32 i = 1; i < Needs.Num(); ++i) { More += (i > 1 ? TEXT("   ·   ") : TEXT("")) + Needs[i].Word; }
				W.TextSOnBase(More, X0 + 16.f * S, AY - 24.f * S, HintF, WithA(Red, 0.7f), 0.6f);
			}
		}
	}

	// ---- top right: the weather and the air. Extreme air grows and turns red (with the wheel, the second warning);
	// in Quiet it shows only then
	const float RX = Size.X - M;
	const float AirShown = bQuiet ? FMath::Clamp((Paint.Extreme - 0.25f) * 2.f, 0.f, 1.f) : 1.f;
	float WeatherBottom = 60.f * S;
	if (AirShown > 0.01f && Paint.bAirKnown)
	{
		const FPen T{ G, Out, Layer, AirShown };
		const float E = Paint.Extreme, Hot = Paint.AirC > 0.f ? 1.f : 0.f;
		const float FY = 60.f * S;
		// heat is red; deep cold is a dark purple on a breath of frost (user: "+60 and -100 are two completely different
		// scenarios... when it is minus 100 we should use the cold color, maybe dark purple")
		const FLinearColor Violet = FLinearColor::FromSRGBColor(FColor(104, 54, 206));
		const FLinearColor Accent = Hot > 0.f ? Red : Violet;
		const FLinearColor TC = FMath::Lerp(White, Accent, FMath::SmoothStep(0.f, 0.35f, E));
		const FSlateFontInfo TF = Roboto(TEXT("Light"), FMath::Lerp(26.f, 46.f, FMath::SmoothStep(0.f, 1.f, E)));
		const int32 Deg = FMath::RoundToInt(Paint.AirC);
		const FString TStr = FString::Printf(TEXT("%s%d°"), Deg < 0 ? TEXT("−") : TEXT(""), FMath::Abs(Deg));
		const float TW = T.Width(TStr, TF);
		if (E > 0.05f)
		{
			if (Hot > 0.f)
			{
				// a soft dark under the number (red on a red Emberfall sky still stands out), then a breath of red light
				T.Glow(FVector2f(RX - TW - 30.f * S, FY - 56.f * S), FVector2f(TW + 60.f * S, 84.f * S), FLinearColor(0.f, 0.01f, 0.03f, 0.42f * E));
				T.Glow(FVector2f(RX - TW - 12.f * S, FY - 40.f * S), FVector2f(TW + 24.f * S, 50.f * S), WithA(Red, 0.22f * E * (0.6f + 0.4f * Pulse)));
			}
			else
			{
				// frost: a pale glow that breathes, so the dark purple reads on a dark storm sky
				T.Glow(FVector2f(RX - TW - 34.f * S, FY - 60.f * S), FVector2f(TW + 68.f * S, 92.f * S), FLinearColor(0.84f, 0.88f, 1.f, 0.4f * E * (0.85f + 0.15f * Pulse)));
			}
		}
		T.TextSOnBase(TStr, RX - TW, FY, TF, TC, FMath::Lerp(0.6f, 0.95f, E));
		float X = RX - TW - 16.f * S;
		T.Line({ FVector2f(X, FY - 22.f * S), FVector2f(X, FY + 2.f * S) }, WithA(White, 0.35f), 1.f);
		X -= 16.f * S;
		const FSlateFontInfo WF = Roboto(TEXT("Light"), 22.f);
		const FString WName = Paint.WeatherNow >= 0 ? HudTitle(ARealmSky::GetWeatherInfo(Paint.WeatherNow).Name) : FString(TEXT("Clear"));
		const float WW = T.Width(WName, WF);
		T.TextSOnBase(WName, X - WW, FY, WF, WithA(White, 0.95f), 0.6f);
		DrawIcon(T, WeatherIcons[FMath::Max(0, Paint.WeatherNow)], FVector2f(X - WW - 20.f * S, FY - 8.f * S), 8.5f * S, WithA(White, 0.9f), 1.2f);
		WeatherBottom = FY;
		if (E > 0.45f)
		{
			const FSlateFontInfo CapF = Roboto(TEXT("Medium"), 13.f, 220);
			const FString Cap = Hot > 0.f ? TEXT("EXTREME HEAT") : TEXT("EXTREME COLD");
			WeatherBottom = FY + 26.f * S;
			T.TextSRightOnBase(Cap, RX, WeatherBottom, CapF, WithA(Accent, FMath::Clamp((E - 0.45f) * 4.f, 0.f, 1.f) * (0.85f + 0.15f * Pulse)), 0.95f);
		}
	}
	// what comes next, once the wheel has told it: under the weather, small
	if (Paint.bForecast && !bWheelOn && !bQuiet)
	{
		const bool bDanger = ARealmSky::GetWeatherInfo(Paint.ForecastIndex).Danger > 0;
		const FString Next = FString::Printf(TEXT("%s in %d:%02d"), *HudTitle(ARealmSky::GetWeatherInfo(Paint.ForecastIndex).Name),
			FMath::FloorToInt(Paint.ForecastSeconds / 60.f), FMath::FloorToInt(Paint.ForecastSeconds) % 60);
		const FSlateFontInfo NF = Roboto(TEXT("Regular"), 15.f);
		const float NY = WeatherBottom + 28.f * S, NW = P.Width(Next, NF);
		const FLinearColor C = bDanger ? WithA(Red, 0.95f) : WithA(White, Soft);
		P.TextSRightOnBase(Next, RX, NY, NF, C, 0.6f);
		DrawIcon(P, WeatherIcons[Paint.ForecastIndex], FVector2f(RX - NW - 14.f * S, NY - 5.f * S), 6.f * S, C, 1.1f);
	}

	// ---- breath: a thin line under the centre of the view, only while it is spent or coming back
	if (Paint.Stamina < 0.985f || Snap.bSprinting)
	{
		const float V = FMath::Clamp(Paint.Stamina, 0.f, 1.f), HW = 90.f * S, BY = Size.Y - 34.f * S;
		const FLinearColor C = Paint.bExhausted ? WithA(Red, 0.75f + 0.25f * Pulse) : WithA(White, 0.95f);
		P.Line({ FVector2f(Size.X * 0.5f - HW, BY), FVector2f(Size.X * 0.5f + HW, BY) }, WithA(White, 0.25f), 1.f);
		P.Line({ FVector2f(Size.X * 0.5f - HW * V, BY), FVector2f(Size.X * 0.5f + HW * V, BY) }, C, FMath::Max(2.f, 2.f * S));
	}

	// ---- bottom right (Everything only): the state of things, then what is in the pack
	if (!bQuiet)
	{
		const FSlateFontInfo KitF = Roboto(TEXT("Regular"), 15.f);
		TArray<FString> Line2;
		if (Snap.bSheltered) { Line2.Add(TEXT("at camp")); }
		if (Snap.bOnRope) { Line2.Add(TEXT("on the rope")); }
		if (Snap.OxygenBottleSeconds > 0.f) { Line2.Add(FString::Printf(TEXT("bottle %d:%02d"), FMath::FloorToInt(Snap.OxygenBottleSeconds / 60.f), FMath::FloorToInt(Snap.OxygenBottleSeconds) % 60)); }
		if (Snap.CarriedBottles > 0) { Line2.Add(FString::Printf(TEXT("%d spare bottle%s"), Snap.CarriedBottles, Snap.CarriedBottles > 1 ? TEXT("s") : TEXT(""))); }
		Line2.Add(FString::Printf(TEXT("pack %d / %d kg"), Paint.PackKg, Paint.PackMax));
		P.TextSRightOnBase(FString::Join(Line2, TEXT("   ·   ")), RX, LY - 26.f * S, KitF, WithA(White, 0.78f), 0.6f);
		P.TextSRightOnBase(FString::Printf(TEXT("food %d  %s      water %d  %s"), Paint.Food, *YggSettings::KeyLabel(TEXT("Eat"), EKeys::One),
			Paint.Water, *YggSettings::KeyLabel(TEXT("Drink"), EKeys::Two)), RX, LY + 5.f * S, KitF, WithA(White, 0.78f), 0.6f);
	}

	// ---- top left: the FPS monitor (Settings > Game > FPS counter), small and quiet, the frame rate first
	if (const int32 FpsLevel = YggSettings::GetInt(TEXT("FpsCounter"), 0); FpsLevel > 0 && Paint.Fps > 0.f)
	{
		const FSlateFontInfo NumF = Roboto(TEXT("Light"), 24.f), CapF = Roboto(TEXT("Medium"), 10.f, 200), SmallF = Roboto(TEXT("Regular"), 13.f);
		const float FX = M * 0.6f, FY = 40.f * S;
		const FString Num = FString::FromInt(FMath::RoundToInt(Paint.Fps));
		const FLinearColor NumC = Paint.Fps < 30.f ? Red : WithA(White, 0.92f);
		P.TextSOnBase(Num, FX, FY, NumF, NumC, 0.6f);
		float X = FX + P.Width(Num, NumF) + 6.f * S;
		P.TextSOnBase(TEXT("FPS"), X, FY, CapF, WithA(White, Soft), 0.6f);
		X += P.Width(TEXT("FPS"), CapF) + 18.f * S;
		if (FpsLevel >= 2)
		{
			const FString More = Paint.bFrameGen
				? FString::Printf(TEXT("rendered %d   \u00b7   %.1f ms   \u00b7   1%% low %d"), FMath::RoundToInt(Paint.RenderedFps), Paint.FrameMs, FMath::RoundToInt(Paint.LowFps))
				: FString::Printf(TEXT("%.1f ms   \u00b7   1%% low %d"), Paint.FrameMs, FMath::RoundToInt(Paint.LowFps));
			P.TextSOnBase(More, X, FY, SmallF, WithA(White, Soft), 0.6f);
		}
		if (FpsLevel >= 3)
		{
			const FString Detail = FString::Printf(TEXT("GPU %.1f ms   \u00b7   game %.1f ms   \u00b7   render %.1f ms"), Paint.GpuMs, Paint.GameMs, Paint.DrawMs);
			P.TextSOnBase(Detail, FX, FY + 20.f * S, SmallF, WithA(White, Soft * 0.9f), 0.6f);
		}
	}

	// ---- bottom centre: a subtitle, in the HUD's own type, no box
	if (Paint.SubtitleTimer > 0.f && !Paint.Subtitle.IsEmpty())
	{
		const FPen PS{ G, Out, Layer, FMath::Clamp(Paint.SubtitleTimer * 2.f, 0.f, 1.f) };
		const FSlateFontInfo SF = Roboto(TEXT("Regular"), 21.f);
		PS.TextS(Paint.Subtitle, FVector2f(Size.X * 0.5f - PS.Width(Paint.Subtitle, SF) * 0.5f, Size.Y - 122.f * S - PS.Base(SF)), SF, WithA(White, 0.97f), 0.6f);
	}

	// Presentation only: the weather now, under the top edge, in the serif
	if (Paint.DemoCaption > 0.f && Paint.DemoIndex >= 0)
	{
		const FPen DC{ G, Out, Layer, Paint.DemoCaption * 0.9f };
		const FVector2f At(Size.X * 0.5f, 44.f + (1.f - Paint.DemoCaption) * -8.f);
		DrawIcon(DC, WeatherIcons[Paint.DemoIndex], At, 12.f, Ivory, 1.3f);
		DC.TextCentre(HudTitle(ARealmSky::GetWeatherInfo(Paint.DemoIndex).Name), At + FVector2f(0.f, 20.f), Serif(22, 0), Ivory);
	}

	// Top centre: the wheel. Its centre stays above the screen: a shallow arc, three sectors, one fixed pointer.
	if (bWheelOn)
	{
		const float T = WheelTime;
		const float In = FMath::Clamp(T / 0.3f, 0.f, 1.f), Fade = FMath::Clamp((WheelTotal - T) / 0.4f, 0.f, 1.f);
		const float InEased = 1.f - FMath::Square(1.f - In);
		const FPen W{ G, Out, Layer, InEased * Fade };
		const float Rad = 1150.f, CX = Size.X * 0.5f, Slide = (1.f - InEased) * -22.f, CY = 132.f - Rad + Slide;
		const FVector2f Hub(CX, CY);
		const float Sector = WheelSector, Round = Sector * ARealmSky::NumWeathers();   // a big wheel: three sectors fit the arc that shows
		constexpr float Stop = WheelStop;
		const float After = FMath::Max(0.f, T - Stop);
		const float Turn = WheelTurnAt(T);
		const float Stopped = FMath::Clamp(After / 0.25f, 0.f, 1.f);
		const float Land = FMath::Clamp(After / 0.45f, 0.f, 1.f);
		const FRealmWeatherInfo& Info = ARealmSky::GetWeatherInfo(Paint.WheelIndex);
		const FLinearColor Verdict = Info.Danger >= 2 ? Coral : Info.Danger == 1 ? Amber : Ivory;
		const auto Arm = [](float Deg) { const float R = FMath::DegreesToRadians(90.f - Deg); return FVector2f(FMath::Cos(R), FMath::Sin(R)); };
		// the arcs thin out towards their ends instead of stopping
		const auto Arc = [&](float R, float Alpha, float Thick)
		{
			for (int32 s = 0; s < 4; ++s)
			{
				const float A0 = 6.f + s * 4.25f, A1 = A0 + 4.25f, K = 1.f - s / 4.f;
				W.Circle(Hub, R, WithA(Ivory, Alpha * K), Thick, 90.f - A1, 90.f - A0);
				W.Circle(Hub, R, WithA(Ivory, Alpha * K), Thick, 90.f + A0, 90.f + A1);
			}
			W.Circle(Hub, R, WithA(Ivory, Alpha), Thick, 90.f - 6.f, 90.f + 6.f);
		};
		const float Inner = Rad - 118.f;
		Arc(Rad, 0.62f, 1.f);
		Arc(Inner, 0.16f, 1.f);
		// fine ticks: the outer ring turns with the wheel, the inner slowly against it
		const float TickStep = Sector / 4.f;
		const float OuterPhase = FMath::Fmod(Turn, TickStep), InnerPhase = FMath::Fmod(-Turn * 0.35f, TickStep);
		for (int32 k = -6; k <= 6; ++k)
		{
			const float O = k * TickStep + OuterPhase, I = k * TickStep + InnerPhase;
			const float OA = 0.32f * FMath::Clamp((23.f - FMath::Abs(O)) / 9.f, 0.f, 1.f), IA = 0.16f * FMath::Clamp((23.f - FMath::Abs(I)) / 9.f, 0.f, 1.f);
			W.Line({ Hub + Arm(O) * (Rad - 5.f), Hub + Arm(O) * Rad }, WithA(Ivory, OA), 1.f);
			W.Line({ Hub + Arm(I) * Inner, Hub + Arm(I) * (Inner + 3.f) }, WithA(Ivory, IA), 1.f);
		}

		for (int32 i = 0; i < ARealmSky::NumWeathers(); ++i)
		{
			const float Delta = FMath::Fmod((i - Paint.WheelIndex) * Sector + Turn + Round * 8.f + Round * 0.5f, Round) - Round * 0.5f;
			if (FMath::Abs(Delta) > 23.f) { continue; }
			const float Edge = FMath::Clamp((23.f - FMath::Abs(Delta)) / 7.f, 0.f, 1.f);         // sectors arrive and leave softly
			const float Near = 1.f - FMath::Clamp(FMath::Abs(Delta) / (Sector * 0.5f), 0.f, 1.f);
			const bool bChosen = i == Paint.WheelIndex;
			const float Lit = FMath::Lerp(0.45f + 0.4f * Near, bChosen ? 1.f : 0.3f, Stopped);    // the two neighbours dim once it lands
			const FLinearColor Tint = bChosen ? FMath::Lerp(Ivory, Verdict, Stopped) : Ivory;
			const FLinearColor C = WithA(Tint, Lit * Edge);
			const FVector2f IconAt = Hub + Arm(Delta) * (Rad - 82.f);
			const float Beat = bChosen ? FMath::Sin(Land * PI) * 0.18f : 0.f;                       // one beat as it lands
			DrawIcon(W, WeatherIcons[i], IconAt, FMath::Lerp(11.f, 13.5f, Near) * (1.f + Beat), C, 1.25f);
			if (bChosen && Land > 0.f && Land < 1.f)
			{
				W.Circle(IconAt, FMath::Lerp(15.f, 34.f, Land), WithA(Verdict, 0.45f * (1.f - Land)), 1.f);
			}
			// the small spaced name gives way to the serif one, so the type never jumps
			const FSlateFontInfo WSmall = YggUI::SerifInfo(15), Large = YggUI::SerifInfo(25);
			const float SmallAlpha = bChosen ? 1.f - Stopped : 1.f;
			if (SmallAlpha > 0.f) { W.TextCentre(HudTitle(ARealmSky::GetWeatherInfo(i).Name), IconAt + FVector2f(0.f, 24.f), WSmall, WithA(C, C.A * SmallAlpha)); }
			if (bChosen && Stopped > 0.f) { W.TextCentre(HudTitle(Info.Name), IconAt + FVector2f(0.f, 21.f), Large, WithA(C, Stopped)); }
			// dividers: only a short mark on the outer ring, so the sectors stay open (no boxes)
			const FVector2f DD = Arm(Delta + Sector * 0.5f);
			W.Line({ Hub + DD * (Rad - 14.f), Hub + DD * Rad }, WithA(Ivory, 0.34f * Edge), 1.f);
		}
		// The chosen sector's arc opens from the pointer, in the colour of the verdict
		if (Stopped > 0.f)
		{
			const float Half = 6.5f * (1.f - FMath::Square(1.f - Stopped));
			W.Circle(Hub, Rad, Verdict, 1.6f, 90.f - Half, 90.f + Half);
		}
		// The pointer: a small open diamond that stays put but flicks as each divider passes under it
		const float PY = CY + Rad;
		const float Past = FMath::Fmod(Turn + Sector * 0.5f, Sector);
		const float Flick = FMath::Exp(-Past * 1.4f) * 4.f * (1.f - Stopped);
		W.Shape(FVector2f(CX - Flick, PY + 7.f), 1.f, { {0.f, -5.f}, {4.f, 0.f}, {0.f, 5.f}, {-4.f, 0.f}, {0.f, -5.f} }, FMath::Lerp(Ivory, Verdict, Stopped), 1.1f);
		// Result line under the wheel: what comes, plainly, and when (a realm named by hand has no countdown)
		const FPen WR{ G, Out, Layer, InEased * Fade * Stopped };
		const FString When = Paint.bForecast ? FString::Printf(TEXT("   ·   %d:%02d"), FMath::FloorToInt(Paint.ForecastSeconds / 60.f), FMath::FloorToInt(Paint.ForecastSeconds) % 60) : FString();
		WR.TextCentre(FString(Info.Warning) + When, FVector2f(CX, PY + 20.f + (1.f - Stopped) * 8.f), Line, Info.Danger > 0 ? Verdict : WithA(Quiet, 0.85f));
	}
	return Layer + 2;
}
