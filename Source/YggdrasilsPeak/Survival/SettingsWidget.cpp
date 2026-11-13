// Copyright Epic Games, Inc. All Rights Reserved.

#include "Survival/SettingsWidget.h"

#include "Survival/YggMenuWidget.h"
#include "Survival/YggSettings.h"
#include "Survival/YggUIProxy.h"
#include "Survival/YggUIStyle.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Camera/CameraComponent.h"
#include "Components/BackgroundBlur.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBoxSlot.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/Slider.h"
#include "Survival/YggMenuBase.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/WidgetSwitcher.h"
#include "Engine/Engine.h"
#include "GameFramework/GameUserSettings.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Containers/Ticker.h"

static FAutoConsoleCommandWithWorldAndArgs CmdSettings(TEXT("ygg.Settings"), TEXT("ygg.Settings [tab 0-5]: opens or closes the settings (same as F2); a tab number opens that tab."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
		if (!PC) { return; }
		if (Args.Num() == 0) { USettingsWidget::Toggle(PC); return; }
		TArray<UUserWidget*> Open;
		UWidgetBlueprintLibrary::GetAllWidgetsOfClass(PC, Open, USettingsWidget::StaticClass(), true);
		if (Open.Num() == 0) { USettingsWidget::Open(PC, nullptr, false); UWidgetBlueprintLibrary::GetAllWidgetsOfClass(PC, Open, USettingsWidget::StaticClass(), true); }
		if (Open.Num() > 0) { Cast<USettingsWidget>(Open[0])->ShowTab(FCString::Atoi(*Args[0])); }
	}));

namespace
{
	float SEase(float T) { T = FMath::Clamp(T, 0.f, 1.f); return T * T * (3.f - 2.f * T); }
	/** "MOUSE SPEED" -> "Mouse speed" (the rows are defined in capitals; v3 speaks in sentence case). Acronyms stay. */
	FString SettingsSentence(const FString& In)
	{
		static const TCHAR* Keep[] = { TEXT("HUD"), TEXT("FOV"), TEXT("DLSS"), TEXT("FSR"), TEXT("TSR"), TEXT("FPS"), TEXT("UI"), TEXT("O2") };
		TArray<FString> W;
		In.ParseIntoArray(W, TEXT(" "));
		for (int32 i = 0; i < W.Num(); ++i)
		{
			bool bKeep = false;
			for (const TCHAR* K : Keep) { if (W[i].Equals(K, ESearchCase::CaseSensitive)) { bKeep = true; } }
			if (bKeep) { continue; }
			W[i] = W[i].ToLower();
			if (i == 0 && W[i].Len() > 0) { W[i][0] = FChar::ToUpper(W[i][0]); }
		}
		return FString::Join(W, TEXT(" "));
	}
	USettingsWidget* FindOpenSettings(APlayerController* PC)
	{
		TArray<UUserWidget*> Open;
		UWidgetBlueprintLibrary::GetAllWidgetsOfClass(PC, Open, USettingsWidget::StaticClass(), true);
		return Open.Num() > 0 ? Cast<USettingsWidget>(Open[0]) : nullptr;
	}
	UGameUserSettings* GUS() { return GEngine ? GEngine->GetGameUserSettings() : nullptr; }
	TArray<FIntPoint> Resolutions()
	{
		TArray<FIntPoint> R;
		UKismetSystemLibrary::GetSupportedFullscreenResolutions(R);
		if (R.Num() == 0) { R = { FIntPoint(1280, 720), FIntPoint(1600, 900), FIntPoint(1920, 1080), FIntPoint(2560, 1440), FIntPoint(3840, 2160) }; }
		return R;
	}
}

void USettingsWidget::ApplySaved(UWorld* World) { YggSettings::ApplyAll(World); }

// ygg.Later <seconds> <command...>: runs a console command after a while of play (automated shots: open the pack, walk...;
// -ExecCmds run before the world and the player exist)
static FAutoConsoleCommandWithWorldAndArgs CmdLater(TEXT("ygg.Later"), TEXT("ygg.Later <seconds> <command ...>: runs the command after that many seconds."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		if (Args.Num() < 2 || !World) { return; }
		const float Delay = FCString::Atof(*Args[0]);
		FString Cmd;
		for (int32 i = 1; i < Args.Num(); ++i) { Cmd += (i > 1 ? TEXT(" ") : TEXT("")) + Args[i]; }
		TWeakObjectPtr<UWorld> W = World;
		TSharedRef<float> Left = MakeShared<float>(Delay);
		FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([W, Cmd, Left](float Dt) -> bool
		{
			if ((*Left -= Dt) > 0.f) { return true; }
			if (UWorld* Wd = W.Get())
			{
				if (APlayerController* PC = Wd->GetFirstPlayerController()) { PC->ConsoleCommand(Cmd); }
			}
			return false;
		}));
	}));

static FAutoConsoleCommandWithWorld CmdSettingsTest(TEXT("ygg.Settings.Test"),
	TEXT("Steps every settings row one notch and back through the menu and logs what changed in the engine (SETTINGSTEST)."),
	FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World) { USettingsWidget::SelfTest(World); }));

void USettingsWidget::SelfTest(UWorld* World)
{
	struct FState
	{
		TWeakObjectPtr<UWorld> World;
		TWeakObjectPtr<USettingsWidget> W;
		int32 Tab = 0, Row = 0, Phase = 0, Dir = 1;
		float Clock = 0.f, Wait = 6.f, FrameSum = 0.f;
		int32 Frames = 0, Tested = 0;
		float MsBefore = 0.f;
		TMap<FString, FString> Before;
		FString ValueBefore;
		TArray<FString> NoEffect;
	};
	TSharedRef<FState> S = MakeShared<FState>();
	S->World = World;
	UE_LOG(LogTemp, Display, TEXT("SETTINGSTEST: starting in 6 s"));
	FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([S](float Dt) -> bool
	{
		UWorld* Wd = S->World.Get();
		if (!Wd) { return false; }
		S->FrameSum += Dt; ++S->Frames;
		if ((S->Wait -= Dt) > 0.f) { return true; }
		APlayerController* PC = Wd->GetFirstPlayerController();
		if (!S->W.IsValid())
		{
			if (!PC) { S->Wait = 1.f; return true; }
			USettingsWidget::Open(PC, nullptr, false);
			TArray<UUserWidget*> Found;
			UWidgetBlueprintLibrary::GetAllWidgetsOfClass(PC, Found, USettingsWidget::StaticClass(), false);
			if (Found.Num() == 0) { UE_LOG(LogTemp, Warning, TEXT("SETTINGSTEST: the settings did not open")); return false; }
			S->W = Cast<USettingsWidget>(Found[0]);
			S->Wait = 1.f;
			return true;
		}
		USettingsWidget* W = S->W.Get();
		if (W->Tabs.Num() == 0) { S->Wait = 0.5f; return true; }
		// next testable row
		while (W->Tabs.IsValidIndex(S->Tab) && (!W->Tabs[S->Tab].Rows.IsValidIndex(S->Row) || W->Tabs[S->Tab].Rows[S->Row].Kind == EKind::Key))
		{
			if (!W->Tabs[S->Tab].Rows.IsValidIndex(S->Row)) { ++S->Tab; S->Row = 0; } else { ++S->Row; }
		}
		if (!W->Tabs.IsValidIndex(S->Tab))
		{
			UE_LOG(LogTemp, Display, TEXT("SETTINGSTEST done: %d rows, %d with no engine effect: %s"), S->Tested, S->NoEffect.Num(), *FString::Join(S->NoEffect, TEXT(", ")));
			W->Close();
			return false;
		}
		FRow& R = W->Tabs[S->Tab].Rows[S->Row];
		auto ValueText = [&R]()
		{
			if (R.Kind == EKind::Choice && R.Get) { const int32 V = R.Get(); return R.Choices.IsValidIndex(V) ? R.Choices[V] : FString::FromInt(V); }
			if (R.Kind == EKind::Slider && R.GetF) { return R.Fmt ? R.Fmt(R.GetF()) : FString::SanitizeFloat(R.GetF()); }
			if (R.Kind == EKind::Cycle && R.Get) { return R.Fmt ? R.Fmt(float(R.Get())) : FString::FromInt(R.Get()); }
			return FString();
		};
		if (S->Phase == 0)
		{
			W->SetTab(S->Tab);
			W->RowIndex = S->Row;
			S->Before = YggSettings::EngineState(Wd);
			S->ValueBefore = ValueText();
			S->MsBefore = S->Frames > 0 ? 1000.f * S->FrameSum / S->Frames : 0.f;
			// a slider already at its top is stepped down (and back up) instead
			S->Dir = (R.Kind == EKind::Slider && R.GetF && R.GetF() >= R.Max - 0.001f) ? -1 : +1;
			W->Change(S->Dir);
			S->FrameSum = 0.f; S->Frames = 0;
			S->Phase = 1; S->Wait = 1.5f;
			return true;
		}
		if (S->Phase == 1)
		{
			const TMap<FString, FString> After = YggSettings::EngineState(Wd);
			TArray<FString> Diff;
			for (const TPair<FString, FString>& P : After)
			{
				const FString* B = S->Before.Find(P.Key);
				if (!B || *B != P.Value) { Diff.Add(FString::Printf(TEXT("%s %s->%s"), *P.Key, B ? **B : TEXT("-"), *P.Value)); }
			}
			const float Ms = S->Frames > 0 ? 1000.f * S->FrameSum / S->Frames : 0.f;
			const FString Name = FString::Printf(TEXT("[%s] %s"), *W->Tabs[S->Tab].Name, *R.Label);
			UE_LOG(LogTemp, Display, TEXT("SETTINGSTEST %s: %s -> %s | %s | frame %.1f -> %.1f ms"), *Name, *S->ValueBefore, *ValueText(),
				Diff.Num() ? *FString::Join(Diff, TEXT(", ")) : TEXT("NO ENGINE CHANGE"), S->MsBefore, Ms);
			if (Diff.Num() == 0) { S->NoEffect.Add(Name); }
			++S->Tested;
			W->RowIndex = S->Row;
			W->Change(-S->Dir);                                 // and back
			S->FrameSum = 0.f; S->Frames = 0;
			S->Phase = 0; S->Wait = 1.0f; ++S->Row;
			return true;
		}
		return true;
	}));
}

void USettingsWidget::Toggle(APlayerController* PC)
{
	if (!PC) { return; }
	if (USettingsWidget* S = FindOpenSettings(PC)) { S->Close(); return; }
	Open(PC, nullptr, false);
}

void USettingsWidget::Open(APlayerController* PC, TFunction<void()> OnClosed, bool bInUIOnly)
{
	if (!PC) { return; }
	if (USettingsWidget* Old = FindOpenSettings(PC)) { Old->RemoveFromParent(); }
	if (!bInUIOnly) { UYggMenuWidget::CloseAll(PC); }
	USettingsWidget* S = CreateWidget<USettingsWidget>(PC, USettingsWidget::StaticClass());
	if (!S) { return; }
	// any menu still on screen (the start menu) keeps its picture but lends its place to the settings
	TArray<UUserWidget*> Under;
	UWidgetBlueprintLibrary::GetAllWidgetsOfClass(PC, Under, UYggMenuBase::StaticClass(), true);
	for (UUserWidget* U : Under) { Cast<UYggMenuBase>(U)->SetColumnVisible(false); S->Hidden.Add(Cast<UYggMenuBase>(U)); }
	S->OnClosedFn = OnClosed;
	S->bUIOnly = bInUIOnly;
	S->SetIsFocusable(true);
	S->AddToViewport(80);
	UYggMenuBase::SetHudVisible(PC, false);
	PC->SetShowMouseCursor(true);
	// the controller counts these: lock once, however many menus open on top of each other
	if (!PC->IsLookInputIgnored()) { PC->SetIgnoreLookInput(true); }
	if (!PC->IsMoveInputIgnored()) { PC->SetIgnoreMoveInput(true); }
	if (bInUIOnly)
	{
		FInputModeUIOnly Mode;
		Mode.SetWidgetToFocus(S->TakeWidget());
		PC->SetInputMode(Mode);
	}
	else
	{
		FInputModeGameAndUI Mode;
		Mode.SetWidgetToFocus(S->TakeWidget());
		Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		Mode.SetHideCursorDuringCapture(false);
		PC->SetInputMode(Mode);
	}
	S->SetKeyboardFocus();
}

void USettingsWidget::CloseIfOpen(APlayerController* PC)
{
	if (USettingsWidget* S = PC ? FindOpenSettings(PC) : nullptr) { S->OnClosedFn = nullptr; S->Close(); }
}

void USettingsWidget::Close()
{
	APlayerController* PC = GetOwningPlayer();
	TFunction<void()> After = OnClosedFn;
	OnClosedFn = nullptr;
	if (PC && !bUIOnly && !After)
	{
		PC->SetShowMouseCursor(false);
		PC->ResetIgnoreLookInput();
		PC->ResetIgnoreMoveInput();
		PC->SetInputMode(FInputModeGameOnly());
		UYggMenuBase::SetHudVisible(PC, true);
	}
	RemoveFromParent();
	for (const TWeakObjectPtr<UYggMenuBase>& U : Hidden) { if (U.IsValid()) { U->SetColumnVisible(true); } }
	if (After) { After(); }
}

TSharedRef<SWidget> USettingsWidget::RebuildWidget()
{
	if (!WidgetTree->RootWidget)
	{
		DefineTabs();
		Build();
	}
	return Super::RebuildWidget();
}

void USettingsWidget::DefineTabs()
{
	using namespace YggSettings;
	auto IntRow = [](const TCHAR* Label, const TCHAR* Hint, TArray<FString> Choices, const TCHAR* Key, int32 Def, int32 Offset, TFunction<void()> Apply)
	{
		FRow R;
		R.Label = Label; R.Hint = Hint; R.Choices = Choices;
		const FString K = Key;
		R.Get = [K, Def, Offset]() { return GetInt(*K, Def) + Offset; };
		R.Set = [K, Offset, Apply](int32 V) { SetInt(*K, V - Offset); if (Apply) { Apply(); } };
		return R;
	};
	auto FloatRow = [](const TCHAR* Label, const TCHAR* Hint, const TCHAR* Key, float Def, float Min, float Max, float Step, TFunction<FString(float)> Fmt, TFunction<void()> Apply)
	{
		FRow R;
		R.Label = Label; R.Hint = Hint; R.Kind = EKind::Slider; R.Min = Min; R.Max = Max; R.Step = Step; R.Fmt = Fmt;
		const FString K = Key;
		R.GetF = [K, Def]() { return GetFloat(*K, Def); };
		R.SetF = [K, Apply](float V) { SetFloat(*K, V); if (Apply) { Apply(); } };
		return R;
	};
	TWeakObjectPtr<USettingsWidget> Self(this);
	auto W = [Self]() { return Self.IsValid() ? Self->GetWorld() : nullptr; };
	const bool bDLSS = IConsoleManager::Get().FindConsoleVariable(TEXT("r.NGX.DLSS.Enable")) != nullptr;
	auto Pct = [](float V) { return FString::Printf(TEXT("%d%%"), FMath::RoundToInt(V * 100.f)); };

	// ---- GAME
	FTab Game{ TEXT("Game") };
	{
		// to-do #65: 90-110, where the player looks for it first
		FRow R = FloatRow(TEXT("FIELD OF VIEW"), TEXT("How much of the mountain you see at once. 90° is close and steady; 110° shows the slopes around you."), TEXT("FieldOfView"), 0.f, 90.f, 110.f, 1.f,
			[](float V) { return FString::Printf(TEXT("%d°"), FMath::RoundToInt(V)); }, [W]() { ApplyView(W()); });
		R.GetF = [Self]()
		{
			const float F = FieldOfView();
			if (F > 1.f) { return FMath::Clamp(F, 90.f, 110.f); }
			const APlayerController* PC = Self.IsValid() ? Self->GetOwningPlayer() : nullptr;
			const APawn* P = PC ? PC->GetPawn() : nullptr;
			const UCameraComponent* C = P ? P->FindComponentByClass<UCameraComponent>() : nullptr;
			return FMath::Clamp(C ? C->FieldOfView : 90.f, 90.f, 110.f);
		};
		Game.Rows.Add(R);
	}
	Game.Rows.Add(IntRow(TEXT("UNITS"), TEXT("Height in metres or in feet, on the HUD and in the menus."), { TEXT("Metres"), TEXT("Feet") }, TEXT("Units"), 0, 0, nullptr));
	Game.Rows.Add(IntRow(TEXT("HUD"), TEXT("Everything shows every reading. Quiet keeps the screen empty until your body needs you - then only that."), { TEXT("Everything"), TEXT("Quiet") }, TEXT("Hud"), 0, 0, nullptr));
	Tabs.Add(Game);

	// ---- GRAPHICS
	FTab Gfx{ TEXT("Graphics") };
	Gfx.Rows.Add(IntRow(TEXT("QUALITY"), TEXT("Auto keeps the frame rate steady on this machine; a fixed level never changes."),
		{ TEXT("Auto"), TEXT("Low"), TEXT("Medium"), TEXT("High"), TEXT("Epic") }, TEXT("Graphics"), -1, 1, [W]() { ApplyGraphics(W()); }));
	{
		const TArray<FString> Levels = { TEXT("Preset"), TEXT("Low"), TEXT("Medium"), TEXT("High"), TEXT("Epic") };
		Gfx.Rows.Add(IntRow(TEXT("SHADOWS"), TEXT("How sharp and how far the shadows reach. Preset follows Quality; the biggest cost after the light itself."), Levels, TEXT("GfxShadows"), 0, 0, [W]() { ApplyGraphics(W()); }));
		Gfx.Rows.Add(IntRow(TEXT("VIEW DISTANCE"), TEXT("How far the small things - rocks, tents, flags - are drawn. Preset follows Quality."), Levels, TEXT("GfxViewDistance"), 0, 0, [W]() { ApplyGraphics(W()); }));
		Gfx.Rows.Add(IntRow(TEXT("EFFECTS"), TEXT("Snow, spindrift, storms and the realms' lights; on Epic the trees near you sway in the wind (costs a little). Preset follows Quality."), Levels, TEXT("GfxEffects"), 0, 0, [W]() { ApplyGraphics(W()); }));
		Gfx.Rows.Add(IntRow(TEXT("TEXTURES"), TEXT("Sharper surfaces up close, more video memory. Preset follows Quality."), Levels, TEXT("GfxTextures"), 0, 0, [W]() { ApplyGraphics(W()); }));
	}
	Gfx.Rows.Add(IntRow(TEXT("UPSCALING"), bDLSS ? TEXT("Renders fewer pixels and rebuilds the image. Auto follows the frame rate; Native with DLSS is DLAA, the sharpest.") : TEXT("TSR renders fewer pixels and rebuilds the image: the biggest saving. Auto follows the frame rate."),
		{ TEXT("Auto"), TEXT("Native"), TEXT("Quality"), TEXT("Balanced"), TEXT("Performance") }, TEXT("Upscaling"), 0, 0, [W]() { ApplyGraphics(W()); }));
	if (bDLSS)
	{
		Gfx.Rows.Add(IntRow(TEXT("UPSCALER"), TEXT("NVIDIA DLSS rebuilds the picture with the graphics card's AI cores; TSR is Unreal's own."),
			{ TEXT("DLSS"), TEXT("TSR") }, TEXT("Upscaler"), 0, 0, [W]() { ApplyGraphics(W()); }));
		if (HasFrameGeneration())
		{
			Gfx.Rows.Add(IntRow(TEXT("FRAME GENERATION"), TEXT("DLSS adds frames between the rendered ones: much smoother, a little more delay. Needs the DLSS upscaler; RTX 40 and 50 cards."),
				{ TEXT("Off"), TEXT("2×"), TEXT("3×"), TEXT("4×") }, TEXT("FrameGen"), 0, 0, [W]() { ApplyGraphics(W()); }));
		}
		if (IConsoleManager::Get().FindConsoleVariable(TEXT("t.Streamline.Reflex.Enable")))
		{
			Gfx.Rows.Add(IntRow(TEXT("LOW LATENCY"), TEXT("NVIDIA Reflex: the view answers the mouse sooner. Always on with frame generation."),
				{ TEXT("Off"), TEXT("On") }, TEXT("Reflex"), 1, 0, [W]() { ApplyGraphics(W()); }));
		}
	}
	Gfx.Rows.Add(IntRow(TEXT("RAY-TRACED LIGHT"), TEXT("Off is a little flatter and much lighter on the graphics card."), { TEXT("Off"), TEXT("On") }, TEXT("RayTracing"), 1, 0, [W]() { ApplyGraphics(W()); }));
	Gfx.Rows.Add(IntRow(TEXT("FOOTPRINTS"), TEXT("Off saves the most. High draws the prints twice as sharp."), { TEXT("Off"), TEXT("Normal"), TEXT("High") }, TEXT("Footprints"), 1, 0, [W]() { ApplyGraphics(W()); }));
	Gfx.Rows.Add(IntRow(TEXT("MOTION BLUR"), TEXT("The smear of fast turns."), { TEXT("Off"), TEXT("On") }, TEXT("MotionBlur"), 1, 0, [W]() { ApplyGraphics(W()); }));
	Tabs.Add(Gfx);

	// ---- DISPLAY
	FTab Disp{ TEXT("Display") };
	{
		FRow R;
		R.Label = TEXT("WINDOW"); R.Hint = TEXT("Borderless covers the screen like full screen and switches away faster.");
		R.Choices = { TEXT("Full screen"), TEXT("Borderless"), TEXT("Window") };
		R.Get = []() { const UGameUserSettings* U = GUS(); if (!U) { return 1; } const EWindowMode::Type M = U->GetFullscreenMode(); return M == EWindowMode::Fullscreen ? 0 : M == EWindowMode::WindowedFullscreen ? 1 : 2; };
		R.Set = [](int32 V) { if (UGameUserSettings* U = GUS()) { U->SetFullscreenMode(V == 0 ? EWindowMode::Fullscreen : V == 1 ? EWindowMode::WindowedFullscreen : EWindowMode::Windowed); if (!GIsEditor) { U->ApplySettings(false); } else { U->SaveSettings(); } } };
		Disp.Rows.Add(R);
	}
	{
		FRow R;
		R.Label = TEXT("RESOLUTION"); R.Hint = TEXT("The size of the picture. Upscaling renders below it and rebuilds it."); R.Kind = EKind::Cycle;
		R.Get = []() { const UGameUserSettings* U = GUS(); const TArray<FIntPoint> L = Resolutions(); const FIntPoint C = U ? U->GetScreenResolution() : FIntPoint(1920, 1080);
			int32 Best = 0; for (int32 i = 0; i < L.Num(); ++i) { if (L[i] == C) { return i; } if (FMath::Abs(L[i].X - C.X) < FMath::Abs(L[Best].X - C.X)) { Best = i; } } return Best; };
		R.Set = [](int32 V) { const TArray<FIntPoint> L = Resolutions(); if (UGameUserSettings* U = GUS()) { U->SetScreenResolution(L[FMath::Clamp(V, 0, L.Num() - 1)]); if (!GIsEditor) { U->ApplySettings(false); } else { U->SaveSettings(); } } };
		R.Fmt = [](float V) { const TArray<FIntPoint> L = Resolutions(); const FIntPoint P = L[FMath::Clamp(int32(V), 0, L.Num() - 1)]; return FString::Printf(TEXT("%d × %d"), P.X, P.Y); };
		R.Max = float(Resolutions().Num() - 1);
		Disp.Rows.Add(R);
	}
	{
		FRow R;
		R.Label = TEXT("VSYNC"); R.Hint = TEXT("No tearing, a little more delay."); R.Choices = { TEXT("Off"), TEXT("On") };
		R.Get = []() { const UGameUserSettings* U = GUS(); return U && U->IsVSyncEnabled() ? 1 : 0; };
		R.Set = [W](int32 V) { if (UGameUserSettings* U = GUS()) { U->SetVSyncEnabled(V == 1); U->SaveSettings(); } ApplyDisplay(W()); };
		Disp.Rows.Add(R);
	}
	{
		FRow R = IntRow(TEXT("FRAME LIMIT"), TEXT("A lower limit keeps a laptop cooler and quieter."), { TEXT("30"), TEXT("60"), TEXT("120"), TEXT("Off") }, TEXT("FrameCap"), 3, 0, [W]() { ApplyDisplay(W()); });
		R.Get = []() { return FrameCap(); };
		Disp.Rows.Add(R);
	}
	Disp.Rows.Add(FloatRow(TEXT("BRIGHTNESS"), TEXT("Lift it until the darkest rock in the night still shows a little."), TEXT("Brightness"), 0.f, -1.f, 1.f, 0.05f,
		[](float V) { return FString::Printf(TEXT("%d%%"), FMath::RoundToInt(100.f + V * 50.f)); }, [W]() { ApplyDisplay(W()); }));
	Tabs.Add(Disp);

	// ---- SOUND
	FTab Snd{ TEXT("Sound") };
	const TCHAR* const SndLabel[] = { TEXT("MASTER"), TEXT("EFFECTS"), TEXT("WEATHER"), TEXT("VOICES"), TEXT("INTERFACE") };
	const TCHAR* const SndHint[] = { TEXT("Everything you hear."), TEXT("Steps, breath, ropes, ice."), TEXT("Wind, storms and thunder."),
		TEXT("The whispers on the mountain."), TEXT("The weather wheel and its chimes.") };
	for (int32 i = 0; i < int32(ESound::Num); ++i)
	{
		Snd.Rows.Add(FloatRow(SndLabel[i], SndHint[i], SoundKey(ESound(i)), 1.f, 0.f, 1.f, 0.05f, Pct, [W]() { ApplySound(W()); }));
	}
	Snd.Rows.Add(IntRow(TEXT("IN THE BACKGROUND"), TEXT("Keep playing sound while the game is not the window in front."), { TEXT("Mute"), TEXT("Play") }, TEXT("SoundInBackground"), 1, 0, [W]() { ApplySound(W()); }));
	Tabs.Add(Snd);

	// ---- CONTROLS
	FTab Ctl{ TEXT("Controls") };
	Ctl.Rows.Add(FloatRow(TEXT("MOUSE SPEED"), TEXT("How fast the view turns with the mouse."), TEXT("Sensitivity"), 1.f, 0.2f, 3.f, 0.05f,
		[](float V) { return FString::Printf(TEXT("%.2f×"), V); }, [W]() { ApplyControls(W()); }));
	Ctl.Rows.Add(IntRow(TEXT("INVERT LOOK"), TEXT("Push the mouse up to look down."), { TEXT("Off"), TEXT("On") }, TEXT("InvertY"), 0, 0, [W]() { ApplyControls(W()); }));
	for (const FBinding& B : Bindings())
	{
		FRow R;
		R.Label = B.Label.ToString().ToUpper(); R.Kind = EKind::Key; R.Action = B.Action;
		R.Hint = TEXT("Click, then press the new key. A key already in use swaps with it.");
		Ctl.Rows.Add(R);
	}
	Tabs.Add(Ctl);

	// ---- ACCESSIBILITY
	FTab Acc{ TEXT("Accessibility") };
	Acc.Rows.Add(FloatRow(TEXT("INTERFACE SIZE"), TEXT("The HUD and the menus, larger or smaller."), TEXT("InterfaceScale"), 1.f, 0.8f, 1.3f, 0.05f, Pct, [W]() { ApplyAccessibility(W()); }));
	Acc.Rows.Add(IntRow(TEXT("LIGHTNING FLASHES"), TEXT("Soft: the storm's flashes light the world far less and the bolts glow gently."), { TEXT("Full"), TEXT("Soft") }, TEXT("SoftFlashes"), 0, 0, nullptr));
	Acc.Rows.Add(IntRow(TEXT("COLOUR VISION"), TEXT("Shifts the colours so red, green and blue warnings stay apart."),
		{ TEXT("Off"), TEXT("Protan"), TEXT("Deutan"), TEXT("Tritan") }, TEXT("ColourVision"), 0, 0, [W]() { ApplyAccessibility(W()); }));
	Tabs.Add(Acc);
}

UButton* USettingsWidget::TextButton(const FString& Label, int32 Size, UTextBlock*& OutText, TFunction<void()> OnClick)
{
	UButton* B = YggUI::Bare(WidgetTree);
	OutText = YggUI::Words(WidgetTree, Label, Size, YggUI::Mist);
	B->SetContent(OutText);
	UYggUIProxy* P = NewObject<UYggUIProxy>(this);
	P->OnClick = OnClick;
	FScriptDelegate D;
	D.BindUFunction(P, TEXT("Click"));
	B->OnClicked.Add(D);
	Proxies.Add(P);
	return B;
}

UButton* USettingsWidget::Clickable(UWidget* Content, TFunction<void()> OnClick)
{
	UButton* B = YggUI::Bare(WidgetTree);
	B->SetContent(Content);
	UYggUIProxy* P = NewObject<UYggUIProxy>(this);
	P->OnClick = OnClick;
	FScriptDelegate D;
	D.BindUFunction(P, TEXT("Click"));
	B->OnClicked.Add(D);
	Proxies.Add(P);
	return B;
}

void USettingsWidget::Build()
{
	Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Canvas"));
	WidgetTree->RootWidget = Canvas;
	Serif = YggUI::SerifFont();
	// on the mountain the world behind goes soft (not over the start menu's pictures)
	if (!bUIOnly)
	{
		Blur = WidgetTree->ConstructWidget<UBackgroundBlur>(UBackgroundBlur::StaticClass());
		Blur->SetBlurStrength(0.f);
		Blur->SetVisibility(ESlateVisibility::HitTestInvisible);
		UCanvasPanelSlot* BlS = Canvas->AddChildToCanvas(Blur);
		BlS->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
		BlS->SetOffsets(FMargin(0.f));
	}
	// a full-screen menu: the whole world steps back a little, cloud shadow darkens the side of the words
	{
		UImage* Base = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
		FSlateBrush BB;
		BB.DrawAs = ESlateBrushDrawType::Box;
		BB.TintColor = FSlateColor(FLinearColor::White);
		Base->SetBrush(BB);
		Base->SetColorAndOpacity(YggUI::WithA(YggUI::InkC, 0.3f));
		Base->SetVisibility(ESlateVisibility::HitTestInvisible);
		UCanvasPanelSlot* BS = Canvas->AddChildToCanvas(Base);
		BS->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
		BS->SetOffsets(FMargin(0.f));
	}
	UMaterialInstanceDynamic* MID = nullptr;
	YggUI::CloudVeil(WidgetTree, Canvas, this, 0.8f, 0.5f, MID);
	VeilMID = MID;

	UVerticalBox* Col = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Col->AddChildToVerticalBox(YggUI::Words(WidgetTree, TEXT("Settings"), 76, YggUI::Snow))->SetPadding(FMargin(0.f, 0.f, 0.f, 28.f));
	// tabs: serif words between the Q and E key caps; the open one is lit
	UHorizontalBox* TabRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	TabRow->AddChildToHorizontalBox(YggUI::KeyCap(WidgetTree, TEXT("Q")))->SetVerticalAlignment(VAlign_Center);
	for (int32 t = 0; t < Tabs.Num(); ++t)
	{
		UOverlay* TO = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
		UImage* TL = YggUI::Glow(WidgetTree, YggUI::WithA(YggUI::Sun, 0.26f), FVector2D(200.f, 80.f));
		TL->SetRenderOpacity(0.f);
		UOverlaySlot* GS = TO->AddChildToOverlay(TL);
		GS->SetHorizontalAlignment(HAlign_Center);
		GS->SetVerticalAlignment(VAlign_Center);
		UTextBlock* TT = nullptr;
		UButton* TB = TextButton(Tabs[t].Name, 23, TT, [this, t]() { SetTab(t); });
		UOverlaySlot* BS = TO->AddChildToOverlay(TB);
		BS->SetHorizontalAlignment(HAlign_Center);
		BS->SetVerticalAlignment(VAlign_Center);
		TabTexts.Add(TT); TabLines.Add(TL); TabHot.Add(0.f);
		UHorizontalBoxSlot* TS = TabRow->AddChildToHorizontalBox(TO);
		TS->SetPadding(FMargin(t == 0 ? 26.f : 34.f, 0.f, 0.f, 0.f));
		TS->SetVerticalAlignment(VAlign_Center);
	}
	{
		UHorizontalBoxSlot* ES = TabRow->AddChildToHorizontalBox(YggUI::KeyCap(WidgetTree, TEXT("E")));
		ES->SetPadding(FMargin(30.f, 0.f, 0.f, 0.f));
		ES->SetVerticalAlignment(VAlign_Center);
	}
	Col->AddChildToVerticalBox(TabRow)->SetPadding(FMargin(0.f, 0.f, 0.f, 24.f));

	Pages = WidgetTree->ConstructWidget<UWidgetSwitcher>(UWidgetSwitcher::StaticClass());
	for (int32 t = 0; t < Tabs.Num(); ++t)
	{
		UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		BuildTabPage(t, Box);
		Pages->AddChild(Box);
	}
	Col->AddChildToVerticalBox(Pages);
	UHorizontalBox* Foot = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	UTextBlock* RT = nullptr;
	ResetBtn = TextButton(TEXT("Reset this tab"), 19, RT, [this]() { YggSettings::ResetTab(GetWorld(), TabIndex); Refresh(); });
	ResetText = RT;
	Foot->AddChildToHorizontalBox(ResetBtn)->SetPadding(FMargin(24.f, 0.f, 18.f, 0.f));
	Foot->AddChildToHorizontalBox(YggUI::Words(WidgetTree, TEXT("·"), 19, YggUI::Shade))->SetPadding(FMargin(0.f, 0.f, 18.f, 0.f));
	UTextBlock* BT = nullptr;
	BackBtn = TextButton(TEXT("Back"), 19, BT, [this]() { Close(); });
	BackText = BT;
	Foot->AddChildToHorizontalBox(BackBtn);
	Col->AddChildToVerticalBox(Foot)->SetPadding(FMargin(0.f, 30.f, 0.f, 0.f));

	UCanvasPanelSlot* CS = Canvas->AddChildToCanvas(Col);
	// the top stays put whichever tab is open (centred, the title jumped when a tab had fewer rows)
	CS->SetAnchors(FAnchors(0.085f, 0.5f));
	CS->SetAlignment(FVector2D(0.f, 0.f));
	CS->SetPosition(FVector2D(-24.f, -420.f));
	CS->SetAutoSize(true);

	// what the chosen row does: a fixed place on the right, so nothing moves when the words change (to-do #62)
	Desc = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	DescTitle = YggUI::Words(WidgetTree, TEXT(""), 19, YggUI::Sun);
	Desc->AddChildToVerticalBox(DescTitle)->SetPadding(FMargin(0.f, 0.f, 0.f, 12.f));
	DescText = YggUI::Words(WidgetTree, TEXT(""), 25, YggUI::Snow);
	DescText->SetWrapTextAt(460.f);
	Desc->AddChildToVerticalBox(DescText);
	DescChoices = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Desc->AddChildToVerticalBox(DescChoices)->SetPadding(FMargin(0.f, 30.f, 0.f, 0.f));
	{
		UCanvasPanelSlot* DS = Canvas->AddChildToCanvas(Desc);
		DS->SetAnchors(FAnchors(0.085f, 0.5f));
		DS->SetAlignment(FVector2D(0.f, 0.f));
		DS->SetPosition(FVector2D(1000.f, -200.f));
		DS->SetSize(FVector2D(480.f, 460.f));
	}

	for (int32 k = 0; k < 2; ++k)
	{
		UHorizontalBox* Keys = k == 0
			? YggUI::Prompts(WidgetTree, { { TEXT("↑ ↓"), TEXT("Choose") }, { TEXT("← →"), TEXT("Change") }, { TEXT("Q E"), TEXT("Tab") }, { TEXT("Esc"), TEXT("Back") } })
			: YggUI::Prompts(WidgetTree, { { TEXT("D-pad"), TEXT("Choose") }, { TEXT("D-pad"), TEXT("Change") }, { TEXT("LB RB"), TEXT("Tab") }, { TEXT("B"), TEXT("Back") } });
		Keys->SetVisibility(k == 0 ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		UCanvasPanelSlot* KS = Canvas->AddChildToCanvas(Keys);
		KS->SetAnchors(FAnchors(0.95f, 1.f));
		KS->SetAlignment(FVector2D(1.f, 0.5f));
		KS->SetPosition(FVector2D(0.f, -54.f));
		KS->SetAutoSize(true);
		(k == 0 ? KeysRow : PadKeysRow) = Keys;
	}
	SetTab(PendingTab);
	SetRenderOpacity(0.f);
}

void USettingsWidget::BuildTabPage(int32 Tab, UVerticalBox* Box)
{
	for (int32 r = 0; r < Tabs[Tab].Rows.Num(); ++r)
	{
		FRow& R = Tabs[Tab].Rows[r];
		UBorder* Hit = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
		Hit->SetBrushColor(FLinearColor::Transparent);
		Hit->SetPadding(FMargin(0.f));
		USizeBox* Width = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		Width->SetWidthOverride(900.f);
		UOverlay* O = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
		// the chosen row is lit: a soft glow behind its words, like a chosen option
		R.Band = YggUI::Glow(WidgetTree, YggUI::WithA(YggUI::Sun, 0.22f), FVector2D(560.f, 84.f));
		R.Band->SetRenderOpacity(0.f);
		R.Band->SetRenderTranslation(FVector2D(250.f, 0.f));   // the 8 px glow sits at the left: its light centres on the label
		UOverlaySlot* BandS = O->AddChildToOverlay(R.Band);
		BandS->SetHorizontalAlignment(HAlign_Left);
		BandS->SetVerticalAlignment(VAlign_Center);
		R.Bar = nullptr;
		UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		R.LabelW = YggUI::Words(WidgetTree, SettingsSentence(R.Label), 21, YggUI::Mist);
		R.LabelW->SetMinDesiredWidth(330.f);
		UHorizontalBoxSlot* LS = Row->AddChildToHorizontalBox(R.LabelW);
		LS->SetVerticalAlignment(VAlign_Center);
		LS->SetPadding(FMargin(24.f, 0.f, 0.f, 0.f));
		// the control, at the right end of the row
		UHorizontalBox* Ctl = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		switch (R.Kind)
		{
		case EKind::Choice:
		case EKind::Cycle:
		{
			UTextBlock* L = nullptr; UTextBlock* Rt = nullptr;
			UButton* Prev = TextButton(TEXT("‹"), 26, L, [this, r]() { RowIndex = r; Change(-1); });
			Ctl->AddChildToHorizontalBox(Prev)->SetVerticalAlignment(VAlign_Center);
			R.Value = YggUI::Words(WidgetTree, TEXT(""), 20, YggUI::Snow);
			R.Value->SetJustification(ETextJustify::Center);
			R.Value->SetMinDesiredWidth(230.f);
			UButton* Mid = Clickable(R.Value, [this, r]() { RowIndex = r; Change(1); });
			Ctl->AddChildToHorizontalBox(Mid)->SetVerticalAlignment(VAlign_Center);
			UButton* Next = TextButton(TEXT("›"), 26, Rt, [this, r]() { RowIndex = r; Change(1); });
			Ctl->AddChildToHorizontalBox(Next)->SetVerticalAlignment(VAlign_Center);
			Keep.Add(R.Value);
			if (L) { Keep.Add(L); }
			if (Rt) { Keep.Add(Rt); }
			break;
		}
		case EKind::Slider:
		{
			R.SliderW = YggUI::Slider(WidgetTree, 300.f);
			R.SliderW->SetMinValue(R.Min);
			R.SliderW->SetMaxValue(R.Max);
			R.SliderW->SetStepSize(R.Step);
			R.SliderW->SetValue(R.GetF ? R.GetF() : R.Min);
			UYggUIProxy* P = NewObject<UYggUIProxy>(this);
			P->OnValue = [this, Tab, r](float V) { FRow& X = Tabs[Tab].Rows[r]; RowIndex = r; if (X.SetF) { X.SetF(V); } };
			FScriptDelegate D; D.BindUFunction(P, TEXT("Value")); R.SliderW->OnValueChanged.Add(D);
			Proxies.Add(P);
			// a slider only asks for its thumb's size: give the line its length; a sunlit fill up to the bead
			UOverlay* Track = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
			R.Fill = YggUI::Line(WidgetTree, 1.f, YggUI::Sun, 2.f);
			UOverlaySlot* FS = Track->AddChildToOverlay(R.Fill);
			FS->SetHorizontalAlignment(HAlign_Left);
			FS->SetVerticalAlignment(VAlign_Center);
			USizeBox* SB = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
			SB->SetWidthOverride(300.f);
			SB->SetHeightOverride(26.f);
			SB->SetContent(R.SliderW);
			Track->AddChildToOverlay(SB);
			UHorizontalBoxSlot* S = Ctl->AddChildToHorizontalBox(Track);
			S->SetVerticalAlignment(VAlign_Center);
			S->SetPadding(FMargin(0.f, 0.f, 22.f, 0.f));
			R.Value = YggUI::Words(WidgetTree, TEXT(""), 20, YggUI::Snow);
			R.Value->SetMinDesiredWidth(70.f);
			R.Value->SetJustification(ETextJustify::Right);
			Ctl->AddChildToHorizontalBox(R.Value)->SetVerticalAlignment(VAlign_Center);
			Keep.Add(SB); Keep.Add(R.SliderW); Keep.Add(R.Value); Keep.Add(R.Fill); Keep.Add(Track);
			break;
		}
		case EKind::Key:
		{
			UBorder* Cap = YggUI::KeyCap(WidgetTree, TEXT("-"), 10);
			Cap->SetPadding(FMargin(14.f, 3.f, 14.f, 3.f));
			R.Value = Cast<UTextBlock>(Cap->GetContent());
			if (R.Value) { R.Value->SetMinDesiredWidth(90.f); R.Value->SetJustification(ETextJustify::Center); }
			UButton* KB = Clickable(Cap, [this, r]() { RowIndex = r; Waiting = r; SetKeyboardFocus(); Refresh(); });
			Ctl->AddChildToHorizontalBox(KB)->SetVerticalAlignment(VAlign_Center);
			Keep.Add(Cap);
			if (R.Value) { Keep.Add(R.Value); }
			break;
		}
		}
		UHorizontalBoxSlot* CtlS = Row->AddChildToHorizontalBox(Ctl);
		CtlS->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		CtlS->SetHorizontalAlignment(HAlign_Right);
		CtlS->SetVerticalAlignment(VAlign_Center);
		UOverlaySlot* RS = O->AddChildToOverlay(Row);
		RS->SetHorizontalAlignment(HAlign_Fill);
		RS->SetVerticalAlignment(VAlign_Center);
		RS->SetPadding(FMargin(0.f, R.Kind == EKind::Key ? 6.f : 8.f, 18.f, R.Kind == EKind::Key ? 6.f : 8.f));
		Width->SetContent(O);
		Hit->SetContent(Width);
		R.Hit = Hit;
		Keep.Add(Hit); Keep.Add(R.LabelW); Keep.Add(R.Band);
		Box->AddChildToVerticalBox(Hit)->SetHorizontalAlignment(HAlign_Left);   // the row keeps its own width (the tabs above are wider)
	}
}

void USettingsWidget::SetTab(int32 Tab)
{
	TabIndex = (Tab + Tabs.Num()) % Tabs.Num();
	RowIndex = 0;
	Waiting = -1;
	TabAge = 0.f;
	if (Pages) { Pages->SetActiveWidgetIndex(TabIndex); }
	Refresh();
}

void USettingsWidget::Change(int32 Dir)
{
	if (!Tabs.IsValidIndex(TabIndex) || !Tabs[TabIndex].Rows.IsValidIndex(RowIndex)) { return; }
	FRow& R = Tabs[TabIndex].Rows[RowIndex];
	switch (R.Kind)
	{
	case EKind::Choice:
		// a selector goes round
		if (R.Get && R.Set && R.Choices.Num() > 0) { R.Set((R.Get() + Dir + R.Choices.Num()) % R.Choices.Num()); }
		break;
	case EKind::Slider:
		if (R.SetF && R.SliderW)
		{
			const float V = FMath::Clamp(R.SliderW->GetValue() + Dir * R.Step, R.Min, R.Max);
			R.SliderW->SetValue(V);
			R.SetF(V);
		}
		break;
	case EKind::Cycle:
		if (R.Get && R.Set) { R.Set(FMath::Clamp(R.Get() + Dir, 0, int32(R.Max))); }
		break;
	default:
		break;
	}
	Refresh();
}

void USettingsWidget::Refresh()
{
	for (int32 t = 0; t < Tabs.Num(); ++t)
	{
		for (int32 r = 0; r < Tabs[t].Rows.Num(); ++r)
		{
			FRow& R = Tabs[t].Rows[r];
			if (R.Kind == EKind::Choice && R.Get)
			{
				const int32 Cur = R.Get();
				if (R.Value) { R.Value->SetText(FText::FromString(R.Choices.IsValidIndex(Cur) ? R.Choices[Cur] : FString())); }
			}
			else if (R.Kind == EKind::Slider && R.SliderW && R.GetF)
			{
				const float V = R.GetF();
				if (!FMath::IsNearlyEqual(R.SliderW->GetValue(), V, 0.001f)) { R.SliderW->SetValue(V); }
				if (R.Value && R.Fmt) { R.Value->SetText(FText::FromString(R.Fmt(V))); }
			}
			else if (R.Kind == EKind::Cycle && R.Value && R.Get && R.Fmt)
			{
				R.Value->SetText(FText::FromString(R.Fmt(float(R.Get()))));
			}
			else if (R.Kind == EKind::Key && R.Value)
			{
				const YggSettings::FBinding* B = YggSettings::Bindings().FindByPredicate([&R](const YggSettings::FBinding& X) { return X.Action == R.Action; });
				const bool bWait = t == TabIndex && r == Waiting;
				R.Value->SetText(FText::FromString(bWait ? FString(TEXT("PRESS A KEY")) : (B ? YggSettings::KeyName(B->Key).ToUpper() : FString(TEXT("-")))));
			}
		}
	}
	bDescDirty = true;   // the panel lists the choices: redraw it
}

void USettingsWidget::ShowDescription()
{
	if (!Tabs.IsValidIndex(TabIndex) || !Tabs[TabIndex].Rows.IsValidIndex(RowIndex) || !DescTitle) { return; }
	const FRow& R = Tabs[TabIndex].Rows[RowIndex];
	const bool bMoved = DescTab != TabIndex || DescRow != RowIndex;
	if (!bMoved && !bDescDirty) { return; }
	if (bMoved) { DescAge = 0.f; }
	bDescDirty = false;
	DescTab = TabIndex;
	DescRow = RowIndex;
	DescTitle->SetText(FText::FromString(SettingsSentence(R.Label)));
	DescText->SetText(FText::FromString(R.Hint));
	DescChoices->ClearChildren();
	if (R.Kind == EKind::Choice && R.Get)
	{
		// every choice, the current one lit
		const int32 Cur = R.Get();
		for (int32 c = 0; c < R.Choices.Num(); ++c)
		{
			UHorizontalBox* L = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
			USizeBox* BB = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
			BB->SetWidthOverride(10.f);
			BB->SetHeightOverride(10.f);
			if (c == Cur)
			{
				UImage* Bd = YggUI::Bead(WidgetTree, 6.f, YggUI::Sun);
				if (USizeBoxSlot* BS = Cast<USizeBoxSlot>(BB->SetContent(Bd))) { BS->SetHorizontalAlignment(HAlign_Center); BS->SetVerticalAlignment(VAlign_Center); }
			}
			UHorizontalBoxSlot* S = L->AddChildToHorizontalBox(BB);
			S->SetVerticalAlignment(VAlign_Center);
			S->SetPadding(FMargin(0.f, 0.f, 14.f, 0.f));
			L->AddChildToHorizontalBox(YggUI::Words(WidgetTree, R.Choices[c], 20, c == Cur ? YggUI::Snow : YggUI::Shade))->SetVerticalAlignment(VAlign_Center);
			DescChoices->AddChildToVerticalBox(L)->SetPadding(FMargin(0.f, 0.f, 0.f, 6.f));
		}
	}
	else if (R.Kind == EKind::Key)
	{
		UTextBlock* T = YggUI::Words(WidgetTree, TEXT("Enter or click, then the new key. Esc keeps the old one."), 18, YggUI::Shade);
		T->SetWrapTextAt(460.f);
		DescChoices->AddChildToVerticalBox(T);
	}
}

void USettingsWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	Age += InDeltaTime;
	TabAge += InDeltaTime;
	DescAge += InDeltaTime;
	SetRenderOpacity(YggUI::Ease((Age - 0.15f) * 2.2f));   // a moment after the words under it begin to fade
	YggUI::TickVeil(VeilMID, Age, MyGeometry.GetLocalSize());
	if (Pages) { Pages->SetRenderOpacity(YggUI::Ease(TabAge * 3.f)); Pages->SetRenderTranslation(FVector2D(-14.f * (1.f - YggUI::Ease(TabAge * 3.f)), 0.f)); }
	if (Blur) { Blur->SetBlurStrength(7.f * YggUI::Ease(Age * 2.5f)); }
	if (KeysRow && PadKeysRow)
	{
		KeysRow->SetVisibility(bPad ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
		PadKeysRow->SetVisibility(bPad ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
	if (!Tabs.IsValidIndex(TabIndex)) { return; }
	TArray<FRow>& Rows = Tabs[TabIndex].Rows;
	if (bMouseMoved)
	{
		for (int32 r = 0; r < Rows.Num(); ++r) { if (Rows[r].Hit && Rows[r].Hit->IsHovered()) { RowIndex = r; } }
	}
	RowIndex = FMath::Clamp(RowIndex, 0, FMath::Max(Rows.Num() - 1, 0));
	const float Breathe = 0.86f + 0.14f * FMath::Sin(Age * 0.8f + 1.3f);
	for (int32 r = 0; r < Rows.Num(); ++r)
	{
		FRow& R = Rows[r];
		const bool bHot = r == RowIndex;
		R.Hot = FMath::FInterpTo(R.Hot, bHot ? 1.f : 0.f, InDeltaTime, 12.f);
		const float In = YggUI::Ease((TabAge - 0.03f * r) * 5.f);
		if (R.Hit) { R.Hit->SetRenderOpacity(In); R.Hit->SetRenderTranslation(FVector2D(6.f * R.Hot - 10.f * (1.f - In), 0.f)); }
		if (R.LabelW) { R.LabelW->SetColorAndOpacity(FSlateColor(FMath::Lerp(YggUI::Mist, YggUI::Snow, R.Hot))); }
		if (R.Band) { R.Band->SetRenderOpacity(R.Hot * Breathe); }
		if (R.Kind == EKind::Choice || R.Kind == EKind::Cycle)
		{
			if (R.Value) { R.Value->SetColorAndOpacity(FSlateColor(FMath::Lerp(YggUI::Mist, YggUI::Snow, R.Hot))); }
		}
		if (R.Kind == EKind::Slider && R.SliderW)
		{
			const float V = R.SliderW->GetValue();
			if (R.Value && R.Fmt) { R.Value->SetText(FText::FromString(R.Fmt(V))); }
			if (R.Value) { R.Value->SetColorAndOpacity(FSlateColor(FMath::Lerp(YggUI::Mist, YggUI::Snow, R.Hot))); }
			if (R.Fill) { R.Fill->SetDesiredSizeOverride(FVector2D(FMath::Max(1.f, 300.f * (V - R.Min) / FMath::Max(R.Max - R.Min, 0.0001f)), 2.f)); }
		}
		if (R.Kind == EKind::Key && R.Value)
		{
			if (r == Waiting) { R.Value->SetRenderOpacity(0.55f + 0.45f * FMath::Abs(FMath::Sin(Age * 3.f))); R.Value->SetColorAndOpacity(FSlateColor(YggUI::Sun)); }
			else { R.Value->SetRenderOpacity(1.f); R.Value->SetColorAndOpacity(FSlateColor(FMath::Lerp(YggUI::Mist, YggUI::Snow, R.Hot))); }
		}
	}
	for (int32 t = 0; t < TabTexts.Num(); ++t)
	{
		const UWidget* Btn = TabTexts[t] ? TabTexts[t]->GetParent() : nullptr;
		TabHot[t] = FMath::FInterpTo(TabHot[t], t == TabIndex ? 1.f : 0.f, InDeltaTime, 12.f);
		if (TabTexts[t]) { TabTexts[t]->SetColorAndOpacity(FSlateColor(t == TabIndex ? YggUI::Snow : (Btn && Btn->IsHovered() ? FMath::Lerp(YggUI::Mist, YggUI::Snow, 0.5f) : YggUI::Mist))); }
		if (TabLines[t]) { TabLines[t]->SetRenderOpacity(TabHot[t] * Breathe); }
	}
	for (UTextBlock* T : { ResetText.Get(), BackText.Get() })
	{
		if (T) { const UWidget* B = T->GetParent(); T->SetColorAndOpacity(FSlateColor(B && B->IsHovered() ? YggUI::Snow : YggUI::Mist)); }
	}
	ShowDescription();
	if (Desc)
	{
		const float D = YggUI::Ease(DescAge * 4.f);
		Desc->SetRenderOpacity(D);
		Desc->SetRenderTranslation(FVector2D(10.f * (1.f - D), 0.f));
	}
}

FReply USettingsWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey K = InKeyEvent.GetKey();
	bPad = K.IsGamepadKey();
	if (Waiting >= 0 && Tabs.IsValidIndex(TabIndex) && Tabs[TabIndex].Rows.IsValidIndex(Waiting))
	{
		if (K != EKeys::Escape) { YggSettings::Rebind(GetWorld(), Tabs[TabIndex].Rows[Waiting].Action, K); }
		Waiting = -1;
		Refresh();
		return FReply::Handled();
	}
	if (K == EKeys::Escape || K == EKeys::F2 || K == EKeys::Gamepad_FaceButton_Right) { Close(); return FReply::Handled(); }
	if (!Tabs.IsValidIndex(TabIndex)) { return FReply::Handled(); }
	if (K == EKeys::Up || K == EKeys::W || K == EKeys::Gamepad_DPad_Up || K == EKeys::Gamepad_LeftStick_Up) { RowIndex = FMath::Max(RowIndex - 1, 0); bMouseMoved = false; return FReply::Handled(); }
	if (K == EKeys::Down || K == EKeys::S || K == EKeys::Gamepad_DPad_Down || K == EKeys::Gamepad_LeftStick_Down) { RowIndex = FMath::Min(RowIndex + 1, Tabs[TabIndex].Rows.Num() - 1); bMouseMoved = false; return FReply::Handled(); }
	if (K == EKeys::Left || K == EKeys::A || K == EKeys::Gamepad_DPad_Left || K == EKeys::Gamepad_LeftStick_Left) { Change(-1); return FReply::Handled(); }
	if (K == EKeys::Right || K == EKeys::D || K == EKeys::Gamepad_DPad_Right || K == EKeys::Gamepad_LeftStick_Right) { Change(1); return FReply::Handled(); }
	if (K == EKeys::Q || K == EKeys::Gamepad_LeftShoulder) { SetTab(TabIndex - 1); return FReply::Handled(); }
	if (K == EKeys::E || K == EKeys::Gamepad_RightShoulder || K == EKeys::Tab) { SetTab(TabIndex + 1); return FReply::Handled(); }
	if ((K == EKeys::Enter || K == EKeys::Gamepad_FaceButton_Bottom) && Tabs[TabIndex].Rows.IsValidIndex(RowIndex) && Tabs[TabIndex].Rows[RowIndex].Kind == EKind::Key)
	{
		Waiting = RowIndex;
		Refresh();
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

FReply USettingsWidget::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (!InMouseEvent.GetCursorDelta().IsNearlyZero()) { bMouseMoved = true; bPad = false; }
	return Super::NativeOnMouseMove(InGeometry, InMouseEvent);
}
