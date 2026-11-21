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
#include "Rendering/DrawElements.h"
#include "Styling/SlateBrush.h"

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

static FAutoConsoleCommandWithWorld CmdSettingsTestPending(TEXT("ygg.Settings.Test.Pending"),
	TEXT("Tests the pending / apply / discard / keep-or-revert flow of the settings and logs SETTINGSTEST2 PASS/FAIL lines."),
	FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World) { USettingsWidget::SelfTestPending(World); }));

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
			W->CommitPending(false);                            // changes are pending now: apply them as Apply -> Yes would
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
			W->CommitPending(false);
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
	if (USettingsWidget* S = FindOpenSettings(PC)) { S->RequestClose(); return; }
	Open(PC, nullptr, false);
}

void USettingsWidget::Open(APlayerController* PC, TFunction<void()> OnClosed, bool bInUIOnly)
{
	if (!PC) { return; }
	if (USettingsWidget* Old = FindOpenSettings(PC)) { Old->Close(); }   // (review) Close reverts a running keep / revert box
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
	// a screen change still waiting for "Keep" goes back (a forced close, the pause menu taking over...); pending edits are dropped
	if (Kept.Num() > 0) { RevertKept(); }
	DiscardPending();
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
		R.bHasDef = true; R.DefI = Def + Offset;
		const FString K = Key;
		R.Get = [K, Def, Offset]() { return GetInt(*K, Def) + Offset; };
		R.Set = [K, Offset, Apply](int32 V) { SetInt(*K, V - Offset); if (Apply) { Apply(); } };
		return R;
	};
	auto FloatRow = [](const TCHAR* Label, const TCHAR* Hint, const TCHAR* Key, float Def, float Min, float Max, float Step, TFunction<FString(float)> Fmt, TFunction<void()> Apply)
	{
		FRow R;
		R.Label = Label; R.Hint = Hint; R.Kind = EKind::Slider; R.Min = Min; R.Max = Max; R.Step = Step; R.Fmt = Fmt;
		R.bHasDef = true; R.DefF = Def;
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
		// to-do #65: where the player looks for it first. 80-110 since 2026-11-19 (camera spec §5.7; 90-110 before)
		FRow R = FloatRow(TEXT("FIELD OF VIEW"), TEXT("How much of the mountain you see at once. 80° is close and calm; 90° is the climber's own eyes; 110° shows the slopes around you."), TEXT("FieldOfView"), 0.f, FovMin, FovMax, 1.f,
			[](float V) { return FString::Printf(TEXT("%d°"), FMath::RoundToInt(V)); }, [W]() { ApplyView(W()); });
		R.DefF = 90.f;   // the climber's own eyes (the saved 0 = unset follows the camera)
		R.GetF = [Self]()
		{
			const float F = FieldOfView();
			if (F > 1.f) { return FMath::Clamp(F, FovMin, FovMax); }
			const APlayerController* PC = Self.IsValid() ? Self->GetOwningPlayer() : nullptr;
			const APawn* P = PC ? PC->GetPawn() : nullptr;
			const UCameraComponent* C = P ? P->FindComponentByClass<UCameraComponent>() : nullptr;
			return FMath::Clamp(C ? C->FieldOfView : 90.f, FovMin, FovMax);
		};
		Game.Rows.Add(R);
	}
	// how the camera feels (camera spec §5.11): stored here, read live by the view rig and the story
	Game.Rows.Add(IntRow(TEXT("HEAD MOTION"), TEXT("How much your head moves with your body: the dip of each step, the breath, the landing. Reduced keeps a little of the steps and nothing else; Off holds the view still - the steps are still heard."),
		{ TEXT("Full"), TEXT("Reduced"), TEXT("Off") }, TEXT("HeadMotion"), 0, 0, [W]() { ApplyView(W()); }));
	Game.Rows.Add(IntRow(TEXT("STORY CAMERA"), TEXT("Guided: now and then the story draws your eyes - a turn toward a sudden sound, a slow nudge toward something you have not seen, the fall. Gentle never moves your view for you; the fall becomes a closing of the eyes."),
		{ TEXT("Guided"), TEXT("Gentle") }, TEXT("StoryCamera"), 0, 0, [W]() { ApplyView(W()); }));
	Game.Rows.Add(IntRow(TEXT("HEARTBEAT ZOOM"), TEXT("At a few moments, and in the thinnest air, the view closes in a little with your heartbeat. Off keeps the field of view still."),
		{ TEXT("Off"), TEXT("On") }, TEXT("HeartbeatZoom"), 1, 0, [W]() { ApplyView(W()); }));
	Game.Rows.Add(IntRow(TEXT("UNITS"), TEXT("Height in metres or in feet, on the HUD and in the menus."), { TEXT("Metres"), TEXT("Feet") }, TEXT("Units"), 0, 0, nullptr));
	Game.Rows.Add(IntRow(TEXT("HUD"), TEXT("Everything shows every reading. Quiet keeps the screen empty until your body needs you - then only that."), { TEXT("Everything"), TEXT("Quiet") }, TEXT("Hud"), 0, 0, nullptr));
	Game.Rows.Add(IntRow(TEXT("HUD SIZE"), TEXT("How large the readings are on the screen."), { TEXT("Small"), TEXT("Normal"), TEXT("Large") }, TEXT("HudSize"), 1, 0, nullptr));
	Game.Rows.Add(IntRow(TEXT("FPS COUNTER"), TEXT("Frames per second in the top left corner. FPS: just the number. Frame times: the time per frame and the 1% low (the slowest frames). Detailed: also the GPU, game and render times."), { TEXT("Off"), TEXT("FPS"), TEXT("Frame times"), TEXT("Detailed") }, TEXT("FpsCounter"), 0, 0, nullptr));
	Game.Rows.Add(IntRow(TEXT("BODY EFFECTS"), TEXT("What thin air, cold and exhaustion do to your view: the tunnel, frost on the lens, the swaying, the grey-out. Reduced halves them and stops the shaking; Off leaves only the HUD's warnings."), { TEXT("Full"), TEXT("Reduced"), TEXT("Off") }, TEXT("BodyFx"), 0, 0, nullptr));
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
			// saved -1 auto, 0 off, 1..3 = 2x..4x: offset 1 puts Auto first (camera spec §5.10, 2026-11-19); the frame-limit
			// clamp (YggSettings::ApplyGraphics) is told here because the row would otherwise seem to do nothing under a limit
			Gfx.Rows.Add(IntRow(TEXT("FRAME GENERATION"), TEXT("DLSS adds frames between the rendered ones: smoother, a little more delay. Auto starts at 2× and rises to 3× or 4× only while the game holds 55 fps and the screen can show them. A frame limit of 120 allows 2× at most; 60 or 30 turns it off."),
				{ TEXT("Auto"), TEXT("Off"), TEXT("2×"), TEXT("3×"), TEXT("4×") }, TEXT("FrameGen"), -1, 1, [W]() { ApplyGraphics(W()); }));
		}
		if (IConsoleManager::Get().FindConsoleVariable(TEXT("t.Streamline.Reflex.Enable")))
		{
			Gfx.Rows.Add(IntRow(TEXT("LOW LATENCY"), TEXT("NVIDIA Reflex: the view answers the mouse sooner. Always on with frame generation."),
				{ TEXT("Off"), TEXT("On") }, TEXT("Reflex"), 1, 0, [W]() { ApplyGraphics(W()); }));
		}
	}
	Gfx.Rows.Add(IntRow(TEXT("RAY-TRACED LIGHT"), TEXT("Off is a little flatter and much lighter on the graphics card."), { TEXT("Off"), TEXT("On") }, TEXT("RayTracing"), 1, 0, [W]() { ApplyGraphics(W()); }));
	Gfx.Rows.Add(IntRow(TEXT("FOOTPRINTS"), TEXT("Off saves the most. High draws the prints twice as sharp."), { TEXT("Off"), TEXT("Normal"), TEXT("High") }, TEXT("Footprints"), 1, 0, [W]() { ApplyGraphics(W()); }));
	// Off by default since 2026-11-19 (camera spec §5.10); Low is a short, faint trail
	Gfx.Rows.Add(IntRow(TEXT("MOTION BLUR"), TEXT("The smear of fast turns. Off keeps every turn sharp; Low leaves a short, faint trail."), { TEXT("Off"), TEXT("Low") }, TEXT("MotionBlur"), 0, 0, [W]() { ApplyGraphics(W()); }));
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
		R.bHasDef = true; R.DefI = 0;
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
	Ctl.Rows.Add(FloatRow(TEXT("MOUSE SPEED"), TEXT("How fast the view turns with the mouse. A calmer speed keeps fast turns from smearing."), TEXT("Sensitivity"), DefaultSensitivity, 0.2f, 3.f, 0.05f,
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
	Acc.Rows.Add(IntRow(TEXT("COMFORT DOT"), TEXT("A small steady dot in the middle of the screen: something still for the eyes to rest on if moving views make you feel unwell."),
		{ TEXT("Off"), TEXT("On") }, TEXT("ComfortDot"), 0, 0, [W]() { ApplyView(W()); }));
	Acc.Rows.Add(IntRow(TEXT("LIGHTNING FLASHES"), TEXT("Soft: the storm's flashes light the world far less and the bolts glow gently."), { TEXT("Full"), TEXT("Soft") }, TEXT("SoftFlashes"), 0, 0, nullptr));
	Acc.Rows.Add(IntRow(TEXT("COLOUR VISION"), TEXT("Shifts the colours so red, green and blue warnings stay apart."),
		{ TEXT("Off"), TEXT("Protan"), TEXT("Deutan"), TEXT("Tritan") }, TEXT("ColourVision"), 0, 0, [W]() { ApplyAccessibility(W()); }));
	Tabs.Add(Acc);

	// the changes that can leave the screen unusable ask "Keep these settings?" after Apply -> Yes (and revert after 15 s)
	for (FTab& T : Tabs)
	{
		for (FRow& R : T.Rows)
		{
			static const TCHAR* const ScreenRows[] = { TEXT("QUALITY"), TEXT("UPSCALING"), TEXT("UPSCALER"), TEXT("FRAME GENERATION"), TEXT("WINDOW"), TEXT("RESOLUTION"), TEXT("VSYNC"), TEXT("FRAME LIMIT") };
			for (const TCHAR* L : ScreenRows) { if (R.Label == L) { R.bScreen = true; } }
		}
	}
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
	UTextBlock* AT = nullptr;
	ApplyBtn = TextButton(TEXT("Apply"), 19, AT, [this]() { RequestApply(); });
	ApplyText = AT;
	Foot->AddChildToHorizontalBox(ApplyBtn)->SetPadding(FMargin(24.f, 0.f, 18.f, 0.f));
	Foot->AddChildToHorizontalBox(YggUI::Words(WidgetTree, TEXT("·"), 19, YggUI::Shade))->SetPadding(FMargin(0.f, 0.f, 18.f, 0.f));
	UTextBlock* RT = nullptr;
	ResetBtn = TextButton(TEXT("Reset this tab"), 19, RT, [this]() { ResetPending(TabIndex); Refresh(); });
	ResetText = RT;
	Foot->AddChildToHorizontalBox(ResetBtn)->SetPadding(FMargin(0.f, 0.f, 18.f, 0.f));
	Foot->AddChildToHorizontalBox(YggUI::Words(WidgetTree, TEXT("·"), 19, YggUI::Shade))->SetPadding(FMargin(0.f, 0.f, 18.f, 0.f));
	UTextBlock* BT = nullptr;
	BackBtn = TextButton(TEXT("Back"), 19, BT, [this]() { RequestClose(); });
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
			? YggUI::Prompts(WidgetTree, { { TEXT("↑ ↓"), TEXT("Choose") }, { TEXT("← →"), TEXT("Change") }, { TEXT("Q E"), TEXT("Tab") }, { TEXT("F"), TEXT("Apply") }, { TEXT("Esc"), TEXT("Back") } })
			: YggUI::Prompts(WidgetTree, { { TEXT("D-pad"), TEXT("Choose") }, { TEXT("D-pad"), TEXT("Change") }, { TEXT("LB RB"), TEXT("Tab") }, { TEXT("Y"), TEXT("Apply") }, { TEXT("B"), TEXT("Back") } });
		Keys->SetVisibility(k == 0 ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		UCanvasPanelSlot* KS = Canvas->AddChildToCanvas(Keys);
		KS->SetAnchors(FAnchors(0.95f, 1.f));
		KS->SetAlignment(FVector2D(1.f, 0.5f));
		KS->SetPosition(FVector2D(0.f, -54.f));
		KS->SetAutoSize(true);
		(k == 0 ? KeysRow : PadKeysRow) = Keys;
	}
	BuildModal();
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
		{
			// a sun bead at the row's left edge: this row has a change waiting for Apply
			USizeBox* MB = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
			MB->SetWidthOverride(10.f);
			MB->SetHeightOverride(10.f);
			UImage* Bd = YggUI::Bead(WidgetTree, 6.f, YggUI::Sun);
			if (USizeBoxSlot* BS2 = Cast<USizeBoxSlot>(MB->SetContent(Bd))) { BS2->SetHorizontalAlignment(HAlign_Center); BS2->SetVerticalAlignment(VAlign_Center); }
			MB->SetRenderOpacity(0.f);
			MB->SetVisibility(ESlateVisibility::HitTestInvisible);
			UOverlaySlot* MS = O->AddChildToOverlay(MB);
			MS->SetHorizontalAlignment(HAlign_Left);
			MS->SetVerticalAlignment(VAlign_Center);
			MS->SetPadding(FMargin(6.f, 0.f, 0.f, 0.f));
			R.Mark = MB;
			Keep.Add(MB);
		}
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
			P->OnValue = [this, Tab, r](float V) { FRow& X = Tabs[Tab].Rows[r]; RowIndex = r; if (X.SetF) { SetPendingFloat(X, V); } };
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
	if (Modal != EModal::None) { return; }   // nothing changes behind a confirm page
	if (!Tabs.IsValidIndex(TabIndex) || !Tabs[TabIndex].Rows.IsValidIndex(RowIndex)) { return; }
	FRow& R = Tabs[TabIndex].Rows[RowIndex];
	switch (R.Kind)
	{
	case EKind::Choice:
		// a selector goes round (the pending copy only)
		if (R.Get && R.Set && R.Choices.Num() > 0) { SetPendingInt(R, (EffInt(R) + Dir + R.Choices.Num()) % R.Choices.Num()); }
		break;
	case EKind::Slider:
		if (R.SetF && R.SliderW)
		{
			const float V = FMath::Clamp(R.SliderW->GetValue() + Dir * R.Step, R.Min, R.Max);
			R.SliderW->SetValue(V);
			SetPendingFloat(R, V);
		}
		break;
	case EKind::Cycle:
		if (R.Get && R.Set) { SetPendingInt(R, FMath::Clamp(EffInt(R) + Dir, 0, int32(R.Max))); }
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
				const int32 Cur = EffInt(R);
				if (R.Value) { R.Value->SetText(FText::FromString(R.Choices.IsValidIndex(Cur) ? R.Choices[Cur] : FString())); }
			}
			else if (R.Kind == EKind::Slider && R.SliderW && R.GetF)
			{
				const float V = EffFloat(R);
				if (!FMath::IsNearlyEqual(R.SliderW->GetValue(), V, 0.001f)) { R.SliderW->SetValue(V); }
				if (R.Value && R.Fmt) { R.Value->SetText(FText::FromString(R.Fmt(V))); }
			}
			else if (R.Kind == EKind::Cycle && R.Value && R.Get && R.Fmt)
			{
				R.Value->SetText(FText::FromString(R.Fmt(float(EffInt(R)))));
			}
			else if (R.Kind == EKind::Key && R.Value)
			{
				const YggSettings::FBinding* B = YggSettings::Bindings().FindByPredicate([&R](const YggSettings::FBinding& X) { return X.Action == R.Action; });
				const bool bWait = t == TabIndex && r == Waiting;
				R.Value->SetText(FText::FromString(bWait ? FString(TEXT("PRESS A KEY")) : (B ? YggSettings::KeyName(EffKey(R)).ToUpper() : FString(TEXT("-")))));
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
		const int32 Cur = EffInt(R);
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
		R.MarkA = FMath::FInterpTo(R.MarkA, R.bDirty ? 1.f : 0.f, InDeltaTime, 14.f);
		if (R.Mark) { R.Mark->SetRenderOpacity(R.MarkA * (0.75f + 0.25f * Breathe)); }
		const FLinearColor ValueCol = FMath::Lerp(FMath::Lerp(YggUI::Mist, YggUI::Snow, R.Hot), YggUI::Sun, 0.8f * R.MarkA);
		if (R.Kind == EKind::Choice || R.Kind == EKind::Cycle)
		{
			if (R.Value) { R.Value->SetColorAndOpacity(FSlateColor(ValueCol)); }
		}
		if (R.Kind == EKind::Slider && R.SliderW)
		{
			const float V = R.SliderW->GetValue();
			if (R.Value && R.Fmt) { R.Value->SetText(FText::FromString(R.Fmt(V))); }
			if (R.Value) { R.Value->SetColorAndOpacity(FSlateColor(ValueCol)); }
			if (R.Fill) { R.Fill->SetDesiredSizeOverride(FVector2D(FMath::Max(1.f, 300.f * (V - R.Min) / FMath::Max(R.Max - R.Min, 0.0001f)), 2.f)); }
		}
		if (R.Kind == EKind::Key && R.Value)
		{
			if (r == Waiting) { R.Value->SetRenderOpacity(0.55f + 0.45f * FMath::Abs(FMath::Sin(Age * 3.f))); R.Value->SetColorAndOpacity(FSlateColor(YggUI::Sun)); }
			else { R.Value->SetRenderOpacity(1.f); R.Value->SetColorAndOpacity(FSlateColor(ValueCol)); }
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
	{
		const int32 Pend = PendingCount();
		if (Pend != LastPendingCount && ApplyText)
		{
			LastPendingCount = Pend;
			ApplyText->SetText(FText::FromString(Pend > 0 ? FString::Printf(TEXT("Apply (%d)"), Pend) : FString(TEXT("Apply"))));
		}
		if (ApplyText)
		{
			const UWidget* B = ApplyText->GetParent();
			ApplyText->SetColorAndOpacity(FSlateColor(Pend > 0 ? (B && B->IsHovered() ? YggUI::Snow : YggUI::Sun) : YggUI::Shade));
		}
	}
	// the question over everything (apply / discard / keep)
	if (ModalRoot && Modal != EModal::None)
	{
		ModalAge += InDeltaTime;
		ModalRoot->SetRenderOpacity(YggUI::Ease(ModalAge * 5.f));
		if (bMouseMoved)
		{
			for (int32 i = 0; i < ModalOptTexts.Num(); ++i)
			{
				const UWidget* B = ModalOptTexts[i] ? ModalOptTexts[i]->GetParent() : nullptr;
				if (B && B->IsHovered()) { ModalSel = i; }
			}
		}
		for (int32 i = 0; i < 2; ++i)
		{
			ModalHot[i] = FMath::FInterpTo(ModalHot[i], i == ModalSel ? 1.f : 0.f, InDeltaTime, 14.f);
			if (ModalOptTexts.IsValidIndex(i) && ModalOptTexts[i]) { ModalOptTexts[i]->SetColorAndOpacity(FSlateColor(FMath::Lerp(YggUI::Mist, YggUI::Snow, ModalHot[i]))); }
			if (ModalOptGlows.IsValidIndex(i) && ModalOptGlows[i]) { ModalOptGlows[i]->SetRenderOpacity(ModalHot[i] * Breathe); }
		}
		if (Modal == EModal::Keep)
		{
			const double Left = KeepEnd - FPlatformTime::Seconds();
			if (ModalCount) { ModalCount->SetText(FText::FromString(FString::Printf(TEXT("Reverting in %d s"), FMath::Max(0, FMath::CeilToInt(float(Left)))))); }
			if (Left <= 0.0) { RevertKept(); }
		}
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
	if (Modal != EModal::None)
	{
		// the question takes every key: left / right choose, Enter answers, Esc is the safe answer (No / keep editing / revert)
		if (K == EKeys::Left || K == EKeys::A || K == EKeys::Gamepad_DPad_Left || K == EKeys::Gamepad_LeftStick_Left) { ModalSel = 0; bMouseMoved = false; }
		else if (K == EKeys::Right || K == EKeys::D || K == EKeys::Gamepad_DPad_Right || K == EKeys::Gamepad_LeftStick_Right) { ModalSel = 1; bMouseMoved = false; }
		else if (K == EKeys::Enter || K == EKeys::SpaceBar || K == EKeys::Gamepad_FaceButton_Bottom) { ModalAnswer(ModalSel); }
		else if (K == EKeys::Escape || K == EKeys::Gamepad_FaceButton_Right) { ModalAnswer(1); }
		return FReply::Handled();
	}
	if (Waiting >= 0 && Tabs.IsValidIndex(TabIndex) && Tabs[TabIndex].Rows.IsValidIndex(Waiting))
	{
		if (K != EKeys::Escape) { SetPendingKey(Tabs[TabIndex].Rows[Waiting], K); }
		Waiting = -1;
		Refresh();
		return FReply::Handled();
	}
	if (K == EKeys::Escape || K == EKeys::F2 || K == EKeys::Gamepad_FaceButton_Right) { RequestClose(); return FReply::Handled(); }
	if (K == EKeys::F || K == EKeys::Gamepad_FaceButton_Top) { RequestApply(); return FReply::Handled(); }
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

// ---- pending changes (2026-11-21): a change edits a copy; Apply -> Yes applies and saves it all at once

int32 USettingsWidget::EffInt(const FRow& R) const { return R.bDirty ? R.PendI : (R.Get ? R.Get() : 0); }
float USettingsWidget::EffFloat(const FRow& R) const { return R.bDirty ? R.PendF : (R.GetF ? R.GetF() : R.Min); }
FKey USettingsWidget::EffKey(const FRow& R) const
{
	if (R.bDirty) { return R.PendKey; }
	const YggSettings::FBinding* B = YggSettings::Bindings().FindByPredicate([&R](const YggSettings::FBinding& X) { return X.Action == R.Action; });
	return B ? B->Key : FKey();
}

void USettingsWidget::SetPendingInt(FRow& R, int32 V)
{
	R.PendI = V;
	R.bDirty = !R.Get || R.Get() != V;
}

void USettingsWidget::SetPendingFloat(FRow& R, float V)
{
	R.PendF = V;
	R.bDirty = !R.GetF || !FMath::IsNearlyEqual(R.GetF(), V, 0.0005f);
}

void USettingsWidget::SetPendingKey(FRow& R, const FKey& NewKey)
{
	// the same keys Rebind refuses
	if (!NewKey.IsValid() || NewKey.IsMouseButton() || NewKey.IsGamepadKey() || NewKey == EKeys::Escape || NewKey == EKeys::F2) { return; }
	const FKey Old = EffKey(R);
	if (Old == NewKey) { return; }
	auto AppliedKey = [](const FRow& X) { const YggSettings::FBinding* B = YggSettings::Bindings().FindByPredicate([&X](const YggSettings::FBinding& Y) { return Y.Action == X.Action; }); return B ? B->Key : FKey(); };
	// a key another action already holds (applied or pending): the two swap
	for (FTab& T : Tabs)
	{
		for (FRow& O : T.Rows)
		{
			if (O.Kind != EKind::Key || &O == &R || EffKey(O) != NewKey) { continue; }
			O.PendKey = Old;
			O.bDirty = Old != AppliedKey(O);
		}
	}
	R.PendKey = NewKey;
	R.bDirty = NewKey != AppliedKey(R);
}

int32 USettingsWidget::PendingCount() const
{
	int32 N = 0;
	for (const FTab& T : Tabs) { for (const FRow& R : T.Rows) { if (R.bDirty) { ++N; } } }
	return N;
}

FString USettingsWidget::RowValueText(const FRow& R, int32 I, float F, const FKey& K) const
{
	switch (R.Kind)
	{
	case EKind::Choice: return R.Choices.IsValidIndex(I) ? R.Choices[I] : FString::FromInt(I);
	case EKind::Slider: return R.Fmt ? R.Fmt(F) : FString::Printf(TEXT("%.2f"), F);
	case EKind::Cycle: return R.Fmt ? R.Fmt(float(I)) : FString::FromInt(I);
	case EKind::Key: return YggSettings::KeyName(K).ToUpper();
	}
	return FString();
}

bool USettingsWidget::FindRowByLabel(const TCHAR* Label, int32& OutTab, int32& OutRow) const
{
	for (int32 t = 0; t < Tabs.Num(); ++t)
	{
		for (int32 r = 0; r < Tabs[t].Rows.Num(); ++r)
		{
			if (Tabs[t].Rows[r].Label == Label) { OutTab = t; OutRow = r; return true; }
		}
	}
	return false;
}

void USettingsWidget::ResetPending(int32 Tab)
{
	if (!Tabs.IsValidIndex(Tab)) { return; }
	for (FRow& R : Tabs[Tab].Rows)
	{
		switch (R.Kind)
		{
		case EKind::Choice:
		case EKind::Cycle:
			if (R.bHasDef) { SetPendingInt(R, R.DefI); }
			break;
		case EKind::Slider:
			if (R.bHasDef) { SetPendingFloat(R, FMath::Clamp(R.DefF, R.Min, R.Max)); }
			break;
		case EKind::Key:
		{
			const YggSettings::FBinding* B = YggSettings::Bindings().FindByPredicate([&R](const YggSettings::FBinding& X) { return X.Action == R.Action; });
			if (B) { R.PendKey = B->Default; R.bDirty = B->Default != B->Key; }
			break;
		}
		}
	}
	Waiting = -1;
}

void USettingsWidget::DiscardPending()
{
	for (FTab& T : Tabs) { for (FRow& R : T.Rows) { R.bDirty = false; } }
	Refresh();
}

void USettingsWidget::RequestClose()
{
	if (Modal == EModal::Keep) { RevertKept(); return; }
	if (Modal != EModal::None) { HideModal(); return; }
	Waiting = -1;
	if (PendingCount() > 0) { ShowModal(EModal::Discard); return; }
	Close();
}

void USettingsWidget::RequestApply()
{
	if (Modal != EModal::None) { return; }
	Waiting = -1;
	if (PendingCount() > 0) { ShowModal(EModal::Apply); }
}

void USettingsWidget::CommitPending(bool bAskKeep)
{
	Kept.Reset();
	for (int32 t = 0; t < Tabs.Num(); ++t)
	{
		for (int32 r = 0; r < Tabs[t].Rows.Num(); ++r)
		{
			FRow& R = Tabs[t].Rows[r];
			if (!R.bDirty) { continue; }
			if (R.Kind == EKind::Key)
			{
				YggSettings::Rebind(GetWorld(), R.Action, R.PendKey);
				R.bDirty = false;
				continue;
			}
			if (R.bScreen)
			{
				FRevert V;
				V.Tab = t; V.Row = r;
				V.I = R.Get ? R.Get() : 0;
				V.F = R.GetF ? R.GetF() : 0.f;
				Kept.Add(V);
			}
			R.bDirty = false;   // cleared first: the Set functions may read the row
			if (R.Kind == EKind::Slider) { if (R.SetF) { R.SetF(R.PendF); } }
			else if (R.Set) { R.Set(R.PendI); }
		}
	}
	Refresh();
	if (bAskKeep && Kept.Num() > 0)
	{
		KeepEnd = FPlatformTime::Seconds() + KeepSeconds;
		ShowModal(EModal::Keep);
	}
	else
	{
		Kept.Reset();
	}
}

void USettingsWidget::RevertKept()
{
	for (const FRevert& V : Kept)
	{
		if (!Tabs.IsValidIndex(V.Tab) || !Tabs[V.Tab].Rows.IsValidIndex(V.Row)) { continue; }
		FRow& R = Tabs[V.Tab].Rows[V.Row];
		R.bDirty = false;
		if (R.Kind == EKind::Slider) { if (R.SetF) { R.SetF(V.F); } }
		else if (R.Set) { R.Set(V.I); }
	}
	Kept.Reset();
	HideModal();
	Refresh();
}

void USettingsWidget::BuildModal()
{
	ModalRoot = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
	// the world and the rows step back under the question (it also takes every click)
	UImage* Dim = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
	FSlateBrush DB;
	DB.DrawAs = ESlateBrushDrawType::Box;
	DB.TintColor = FSlateColor(FLinearColor::White);
	Dim->SetBrush(DB);
	Dim->SetColorAndOpacity(YggUI::WithA(YggUI::InkC, 0.8f));
	UOverlaySlot* DS = ModalRoot->AddChildToOverlay(Dim);
	DS->SetHorizontalAlignment(HAlign_Fill);
	DS->SetVerticalAlignment(VAlign_Fill);
	UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	ModalTitle = YggUI::Words(WidgetTree, TEXT(""), 44, YggUI::Snow);
	ModalTitle->SetJustification(ETextJustify::Center);
	Box->AddChildToVerticalBox(ModalTitle)->SetHorizontalAlignment(HAlign_Center);
	ModalBody = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Box->AddChildToVerticalBox(ModalBody)->SetPadding(FMargin(0.f, 22.f, 0.f, 34.f));
	UHorizontalBox* Opts = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	for (int32 i = 0; i < 2; ++i)
	{
		UOverlay* O = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
		UImage* G = YggUI::Glow(WidgetTree, YggUI::WithA(YggUI::Sun, 0.26f), FVector2D(240.f, 80.f));
		G->SetRenderOpacity(0.f);
		UOverlaySlot* GS = O->AddChildToOverlay(G);
		GS->SetHorizontalAlignment(HAlign_Center);
		GS->SetVerticalAlignment(VAlign_Center);
		UTextBlock* T = nullptr;
		UButton* B = TextButton(TEXT(""), 27, T, [this, i]() { ModalAnswer(i); });
		UOverlaySlot* BS = O->AddChildToOverlay(B);
		BS->SetHorizontalAlignment(HAlign_Center);
		BS->SetVerticalAlignment(VAlign_Center);
		ModalOptTexts.Add(T);
		ModalOptGlows.Add(G);
		UHorizontalBoxSlot* S = Opts->AddChildToHorizontalBox(O);
		S->SetPadding(FMargin(i == 0 ? 0.f : 70.f, 0.f, 0.f, 0.f));
	}
	Box->AddChildToVerticalBox(Opts)->SetHorizontalAlignment(HAlign_Center);
	UOverlaySlot* BoxS = ModalRoot->AddChildToOverlay(Box);
	BoxS->SetHorizontalAlignment(HAlign_Center);
	BoxS->SetVerticalAlignment(VAlign_Center);
	ModalRoot->SetVisibility(ESlateVisibility::Collapsed);
	UCanvasPanelSlot* MS = Canvas->AddChildToCanvas(ModalRoot);
	MS->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
	MS->SetOffsets(FMargin(0.f));
	MS->SetZOrder(100);
}

void USettingsWidget::ShowModal(EModal M)
{
	if (!ModalRoot || M == EModal::None) { return; }
	Modal = M;
	ModalAge = 0.f;
	ModalCount = nullptr;
	ModalBody->ClearChildren();
	auto Line = [this](const FString& S, int32 Size, const FLinearColor& C) -> UTextBlock*
	{
		UTextBlock* T = YggUI::Words(WidgetTree, S, Size, C);
		T->SetJustification(ETextJustify::Center);
		ModalBody->AddChildToVerticalBox(T)->SetHorizontalAlignment(HAlign_Center);
		return T;
	};
	FString A, B;
	if (M == EModal::Apply)
	{
		ModalTitle->SetText(FText::FromString(TEXT("Apply these changes?")));
		A = TEXT("Yes"); B = TEXT("No");
		ModalSel = 0;
		int32 Shown = 0, Total = 0;
		for (const FTab& T : Tabs)
		{
			for (const FRow& R : T.Rows)
			{
				if (!R.bDirty) { continue; }
				++Total;
				if (Shown >= 12) { continue; }
				++Shown;
				const YggSettings::FBinding* KB = R.Kind == EKind::Key ? YggSettings::Bindings().FindByPredicate([&R](const YggSettings::FBinding& X) { return X.Action == R.Action; }) : nullptr;
				const FString Old = RowValueText(R, R.Get ? R.Get() : 0, R.GetF ? R.GetF() : 0.f, KB ? KB->Key : FKey());
				const FString New = RowValueText(R, R.PendI, R.PendF, R.PendKey);
				Line(FString::Printf(TEXT("%s: %s  ›  %s"), *SettingsSentence(R.Label), *Old, *New), 22, YggUI::Mist);
			}
		}
		if (Total > Shown) { Line(FString::Printf(TEXT("and %d more"), Total - Shown), 20, YggUI::Shade); }
	}
	else if (M == EModal::Discard)
	{
		ModalTitle->SetText(FText::FromString(TEXT("Discard changes?")));
		A = TEXT("Discard"); B = TEXT("Keep editing");
		ModalSel = 1;
		const int32 N = PendingCount();
		Line(FString::Printf(TEXT("%d change%s not applied yet."), N, N == 1 ? TEXT(" is") : TEXT("s are")), 22, YggUI::Mist);
	}
	else
	{
		ModalTitle->SetText(FText::FromString(TEXT("Keep these settings?")));
		A = TEXT("Keep"); B = TEXT("Revert");
		ModalSel = 1;
		ModalCount = Line(TEXT(""), 22, YggUI::Sun);
	}
	if (ModalOptTexts.Num() == 2) { ModalOptTexts[0]->SetText(FText::FromString(A)); ModalOptTexts[1]->SetText(FText::FromString(B)); }
	ModalHot[0] = ModalHot[1] = 0.f;
	ModalRoot->SetRenderOpacity(0.f);
	ModalRoot->SetVisibility(ESlateVisibility::Visible);
	SetKeyboardFocus();      // (2026-11-21 review) a clicked row button kept the focus and ate Enter / Space / A behind the page
}

void USettingsWidget::HideModal()
{
	Modal = EModal::None;
	if (ModalRoot) { ModalRoot->SetVisibility(ESlateVisibility::Collapsed); }
	SetKeyboardFocus();
}

void USettingsWidget::ModalAnswer(int32 Index)
{
	const EModal M = Modal;
	if (M == EModal::None) { return; }
	if (M == EModal::Apply)
	{
		HideModal();
		if (Index == 0) { CommitPending(true); }
	}
	else if (M == EModal::Discard)
	{
		HideModal();
		if (Index == 0) { DiscardPending(); Close(); }
	}
	else
	{
		if (Index == 0) { Kept.Reset(); HideModal(); }
		else { RevertKept(); }
	}
}

void USettingsWidget::SelfTestPending(UWorld* World)
{
	struct FState
	{
		TWeakObjectPtr<UWorld> World;
		TWeakObjectPtr<USettingsWidget> W;
		int32 Phase = 0, Pass = 0, Fail = 0, HudBefore = 0, VsBefore = 0;
		float Wait = 4.f;
	};
	TSharedRef<FState> S = MakeShared<FState>();
	S->World = World;
	UE_LOG(LogTemp, Display, TEXT("SETTINGSTEST2: starting in 4 s"));
	FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([S](float Dt) -> bool
	{
		UWorld* Wd = S->World.Get();
		if (!Wd) { return false; }
		if ((S->Wait -= Dt) > 0.f) { return true; }
		auto Chk = [S](bool b, const ANSICHAR* What) { (b ? S->Pass : S->Fail)++; UE_LOG(LogTemp, Display, TEXT("SETTINGSTEST2 %s: %s"), b ? TEXT("PASS") : TEXT("FAIL"), UTF8_TO_TCHAR(What)); };
		auto OpenIt = [S, Wd]() -> bool
		{
			APlayerController* PC = Wd->GetFirstPlayerController();
			if (!PC) { return false; }
			USettingsWidget::Open(PC, nullptr, false);
			TArray<UUserWidget*> Found;
			UWidgetBlueprintLibrary::GetAllWidgetsOfClass(PC, Found, USettingsWidget::StaticClass(), false);
			S->W.Reset();
			for (UUserWidget* U : Found) { if (U && U->IsInViewport()) { S->W = Cast<USettingsWidget>(U); } }
			return S->W.IsValid();
		};
		if (S->Phase == 0 || S->Phase == 3)
		{
			if (!OpenIt()) { UE_LOG(LogTemp, Warning, TEXT("SETTINGSTEST2: the settings did not open")); return false; }
			S->Wait = 1.f; ++S->Phase;
			return true;
		}
		USettingsWidget* W = S->W.Get();
		if (!W || W->Tabs.Num() == 0) { S->Wait = 0.5f; return true; }
		int32 HT = 0, HR = 0, VT = 0, VR = 0;
		if (!W->FindRowByLabel(TEXT("HUD SIZE"), HT, HR) || !W->FindRowByLabel(TEXT("VSYNC"), VT, VR))
		{
			UE_LOG(LogTemp, Warning, TEXT("SETTINGSTEST2: rows HUD SIZE / VSYNC not found"));
			W->Close();
			return false;
		}
		if (S->Phase == 1)
		{
			FRow& R = W->Tabs[HT].Rows[HR];
			S->HudBefore = R.Get();
			const TMap<FString, FString> Eng = YggSettings::EngineState(Wd);
			W->SetTab(HT); W->RowIndex = HR;
			W->Change(1);
			Chk(R.Get() == S->HudBefore && YggSettings::GetInt(TEXT("HudSize"), 1) == S->HudBefore, "a change applies nothing (value and saved setting unchanged)");
			Chk(R.bDirty && W->PendingCount() == 1, "the row is marked pending");
			{
				const TMap<FString, FString> Now = YggSettings::EngineState(Wd);
				bool bSame = Now.Num() == Eng.Num();
				for (const TPair<FString, FString>& P : Now) { const FString* O = Eng.Find(P.Key); if (!O || *O != P.Value) { bSame = false; } }
				Chk(bSame, "engine state unchanged by a pending change");
			}
			W->RequestApply();
			Chk(W->Modal == EModal::Apply, "Apply opens the confirm page");
			W->ModalAnswer(1);
			Chk(W->Modal == EModal::None && W->PendingCount() == 1 && R.Get() == S->HudBefore, "No keeps the pending choice and applies nothing");
			W->RequestApply();
			W->ModalAnswer(0);
			Chk(R.Get() == (S->HudBefore + 1) % 3 && YggSettings::GetInt(TEXT("HudSize"), 1) == (S->HudBefore + 1) % 3, "Yes applies and saves");
			Chk(W->PendingCount() == 0 && W->Modal == EModal::None, "nothing pending after Yes; no keep box for a non-screen setting");
			W->Change(-1); W->CommitPending(false);
			Chk(R.Get() == S->HudBefore, "restored");
			// leaving with a pending change
			W->Change(1);
			W->RequestClose();
			Chk(W->Modal == EModal::Discard, "leaving with a pending change asks to discard");
			W->ModalAnswer(1);
			Chk(W->Modal == EModal::None && W->PendingCount() == 1 && W->IsInViewport(), "Keep editing stays in the menu with the choice pending");
			W->RequestClose();
			W->ModalAnswer(0);
			Chk(YggSettings::GetInt(TEXT("HudSize"), 1) == S->HudBefore && !W->IsInViewport(), "Discard reverts the choice and leaves");
			S->Phase = 3; S->Wait = 1.f;
			return true;
		}
		if (S->Phase == 4)
		{
			FRow& R = W->Tabs[VT].Rows[VR];
			S->VsBefore = R.Get();
			W->KeepSeconds = 3.f;
			W->SetTab(VT); W->RowIndex = VR;
			W->Change(1);
			W->RequestApply();
			W->ModalAnswer(0);
			Chk(R.Get() != S->VsBefore, "a screen setting is applied by Yes");
			Chk(W->Modal == EModal::Keep, "a screen change asks Keep / Revert");
			S->Phase = 5; S->Wait = 4.5f;
			return true;
		}
		if (S->Phase == 5)
		{
			FRow& R = W->Tabs[VT].Rows[VR];
			Chk(W->Modal == EModal::None && R.Get() == S->VsBefore, "the screen change reverts by itself at 0");
			W->Change(1);
			W->RequestApply();
			W->ModalAnswer(0);
			Chk(W->Modal == EModal::Keep && R.Get() != S->VsBefore, "applied again");
			W->RequestClose();                                  // Esc on the keep box = Revert
			Chk(W->Modal == EModal::None && R.Get() == S->VsBefore, "Esc reverts");
			W->Close();
			UE_LOG(LogTemp, Display, TEXT("SETTINGSTEST2 done: %d passed, %d failed"), S->Pass, S->Fail);
			return false;
		}
		return false;
	}));
}

// ---- the comfort dot (camera spec §5.11)

namespace
{
	TWeakObjectPtr<UYggComfortDot>& ComfortDotInstance() { static TWeakObjectPtr<UYggComfortDot> D; return D; }
}

void UYggComfortDot::Apply(UWorld* World, bool bOn)
{
	TWeakObjectPtr<UYggComfortDot>& Dot = ComfortDotInstance();
	APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
	// a dot of another player or world (a level change, the start menu) is replaced
	if (Dot.IsValid() && (!bOn || Dot->GetOwningPlayer() != PC || !Dot->IsInViewport()))
	{
		Dot->RemoveFromParent();
		Dot.Reset();
	}
	if (!bOn || !PC || !PC->IsLocalController() || Dot.IsValid())
	{
		return;
	}
	UYggComfortDot* D = CreateWidget<UYggComfortDot>(PC, UYggComfortDot::StaticClass());
	if (!D)
	{
		return;
	}
	D->SetVisibility(ESlateVisibility::HitTestInvisible);   // never takes a click or the focus
	D->AddToViewport(5);                                    // over the HUD (0), under the pack (50), the story overlay (60) and the menus (80)
	Dot = D;
}

bool UYggComfortDot::IsShown()
{
	const TWeakObjectPtr<UYggComfortDot>& Dot = ComfortDotInstance();
	return Dot.IsValid() && Dot->IsInViewport();
}

int32 UYggComfortDot::NativePaint(const FPaintArgs& Args, const FGeometry& G, const FSlateRect& MyCullingRect, FSlateWindowElementList& Out, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	const int32 L = Super::NativePaint(Args, G, MyCullingRect, Out, LayerId, InWidgetStyle, bParentEnabled) + 1;
	// a menu with the cursor is up (pause, settings, the pack, the start menu): the dot steps aside
	const APlayerController* PC = GetOwningPlayer();
	if (!PC || PC->bShowMouseCursor)
	{
		return L;
	}
	static FSlateBrush Round = []()
	{
		FSlateBrush B;
		B.DrawAs = ESlateBrushDrawType::RoundedBox;
		B.OutlineSettings.RoundingType = ESlateBrushRoundingType::HalfHeightRadius;
		B.TintColor = FSlateColor(FLinearColor::White);
		return B;
	}();
	// about 5 px at 1080p, growing with the window; a soft dark ring keeps it readable on bright snow
	const FVector2f S = FVector2f(G.GetLocalSize());
	const float D = FMath::Max(4.f, 5.f * S.Y / 1080.f);
	const FVector2f C = S * 0.5f;
	auto Dot = [&](int32 Layer, float Size, const FLinearColor& Col)
	{
		FSlateDrawElement::MakeBox(Out, Layer, G.ToPaintGeometry(FVector2f(Size, Size), FSlateLayoutTransform(C - FVector2f(Size, Size) * 0.5f)), &Round, ESlateDrawEffect::None, Col);
	};
	Dot(L, D + 3.f, YggUI::WithA(YggUI::InkC, 0.35f));
	Dot(L + 1, D, YggUI::WithA(YggUI::Snow, 0.75f));
	return L + 1;
}
