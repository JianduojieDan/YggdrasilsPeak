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
	Game.Rows.Add(IntRow(TEXT("HUD"), TEXT("Hidden keeps only the warnings and the messages: the mountain and nothing else."), { TEXT("Shown"), TEXT("Hidden") }, TEXT("Hud"), 0, 0, nullptr));
	Tabs.Add(Game);

	// ---- GRAPHICS
	FTab Gfx{ TEXT("Graphics") };
	Gfx.Rows.Add(IntRow(TEXT("QUALITY"), TEXT("Auto keeps the frame rate steady on this machine; a fixed level never changes."),
		{ TEXT("Auto"), TEXT("Low"), TEXT("Medium"), TEXT("High"), TEXT("Epic") }, TEXT("Graphics"), -1, 1, [W]() { ApplyGraphics(W()); }));
	Gfx.Rows.Add(IntRow(TEXT("UPSCALING"), bDLSS ? TEXT("NVIDIA DLSS renders fewer pixels and rebuilds the image.") : TEXT("TSR renders fewer pixels and rebuilds the image: the biggest saving."),
		{ TEXT("Auto"), TEXT("Native"), TEXT("Quality"), TEXT("Balanced"), TEXT("Performance") }, TEXT("Upscaling"), 0, 0, [W]() { ApplyGraphics(W()); }));
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
	UImage* Bd = nullptr;
	UButton* B = YggUI::Option(WidgetTree, Label, Size, OutText, Bd);
	if (Bd) { Bd->SetVisibility(ESlateVisibility::Collapsed); }
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
	Serif = YggUI::MakeSerif(this);
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
	// a full-screen menu: the whole world steps back a little, the veil darkens the side of the words
	{
		UImage* Base = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
		FSlateBrush BB;
		BB.DrawAs = ESlateBrushDrawType::Box;
		BB.TintColor = FSlateColor(FLinearColor::White);
		Base->SetBrush(BB);
		Base->SetColorAndOpacity(FLinearColor(YggUI::Ink.R, YggUI::Ink.G, YggUI::Ink.B, 0.45f));
		Base->SetVisibility(ESlateVisibility::HitTestInvisible);
		UCanvasPanelSlot* BS = Canvas->AddChildToCanvas(Base);
		BS->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
		BS->SetOffsets(FMargin(0.f));
	}
	YggUI::Layer(WidgetTree, Canvas, YggUI::VeilTexture(), YggUI::Ink);
	YggUI::Layer(WidgetTree, Canvas, YggUI::VignetteTexture(), FLinearColor(YggUI::Ink.R, YggUI::Ink.G, YggUI::Ink.B, 0.6f));

	UVerticalBox* Col = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Col->AddChildToVerticalBox(YggUI::Text(WidgetTree, TEXT("THE CLIMBER'S KIT"), 10, YggUI::Dim, 460, TEXT("Regular")))->SetPadding(FMargin(0.f, 0.f, 0.f, 6.f));
	Col->AddChildToVerticalBox(YggUI::Text(WidgetTree, TEXT("Settings"), 58, YggUI::Ivory, 0, TEXT("Regular"), Serif))->SetPadding(FMargin(0.f, 0.f, 0.f, 10.f));
	{
		UHorizontalBox* RuleRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		RuleRow->AddChildToHorizontalBox(YggUI::Line(WidgetTree, 300.f))->SetVerticalAlignment(VAlign_Center);
		UHorizontalBoxSlot* BS = RuleRow->AddChildToHorizontalBox(YggUI::Bead(WidgetTree, 5.f, YggUI::Gold));
		BS->SetVerticalAlignment(VAlign_Center);
		BS->SetPadding(FMargin(8.f, 0.f, 0.f, 0.f));
		Col->AddChildToVerticalBox(RuleRow)->SetPadding(FMargin(0.f, 0.f, 0.f, 30.f));
	}
	// tabs: small capitals between the Q and E key caps; the chosen one is lit and underlined in gold
	UHorizontalBox* TabRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	TabRow->AddChildToHorizontalBox(YggUI::KeyCap(WidgetTree, TEXT("Q")))->SetVerticalAlignment(VAlign_Top);
	for (int32 t = 0; t < Tabs.Num(); ++t)
	{
		UVerticalBox* TV = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		UTextBlock* TT = nullptr;
		UButton* TB = TextButton(Tabs[t].Name, 12, TT, [this, t]() { SetTab(t); });
		TV->AddChildToVerticalBox(TB);
		UImage* TL = YggUI::Line(WidgetTree, 10.f, YggUI::Gold, 2.f);
		UVerticalBoxSlot* LS = TV->AddChildToVerticalBox(TL);
		LS->SetHorizontalAlignment(HAlign_Fill);
		LS->SetPadding(FMargin(0.f, 7.f, 0.f, 0.f));
		TabTexts.Add(TT); TabLines.Add(TL); TabHot.Add(0.f);
		TabRow->AddChildToHorizontalBox(TV)->SetPadding(FMargin(t == 0 ? 24.f : 34.f, 0.f, 0.f, 0.f));
	}
	{
		UHorizontalBoxSlot* ES = TabRow->AddChildToHorizontalBox(YggUI::KeyCap(WidgetTree, TEXT("E")));
		ES->SetPadding(FMargin(28.f, 0.f, 0.f, 0.f));
		ES->SetVerticalAlignment(VAlign_Top);
	}
	Col->AddChildToVerticalBox(TabRow)->SetPadding(FMargin(0.f, 0.f, 0.f, 18.f));
	Col->AddChildToVerticalBox(YggUI::Line(WidgetTree, 860.f, YggUI::Rule))->SetPadding(FMargin(0.f, 0.f, 0.f, 4.f));

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
	ResetBtn = TextButton(TEXT("Reset this tab"), 11, RT, [this]() { YggSettings::ResetTab(GetWorld(), TabIndex); Refresh(); });
	ResetText = RT;
	Foot->AddChildToHorizontalBox(ResetBtn)->SetPadding(FMargin(0.f, 0.f, 18.f, 0.f));
	Foot->AddChildToHorizontalBox(YggUI::Text(WidgetTree, TEXT("·"), 11, YggUI::Faint, 0))->SetPadding(FMargin(0.f, 1.f, 18.f, 0.f));
	UTextBlock* BT = nullptr;
	BackBtn = TextButton(TEXT("Back"), 11, BT, [this]() { Close(); });
	BackText = BT;
	Foot->AddChildToHorizontalBox(BackBtn);
	Col->AddChildToVerticalBox(Foot)->SetPadding(FMargin(0.f, 26.f, 0.f, 0.f));

	UCanvasPanelSlot* CS = Canvas->AddChildToCanvas(Col);
	// the top stays put whichever tab is open (centred, the title jumped when a tab had fewer rows)
	CS->SetAnchors(FAnchors(0.085f, 0.5f));
	CS->SetAlignment(FVector2D(0.f, 0.f));
	CS->SetPosition(FVector2D(0.f, -405.f));
	CS->SetAutoSize(true);

	// what the chosen row does: a fixed place on the right, so nothing moves when the words change (to-do #62)
	Desc = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	DescTitle = YggUI::Text(WidgetTree, TEXT(""), 10, YggUI::Gold, 460, TEXT("Regular"));
	Desc->AddChildToVerticalBox(DescTitle);
	Desc->AddChildToVerticalBox(YggUI::Line(WidgetTree, 48.f, YggUI::Hair))->SetPadding(FMargin(0.f, 14.f, 0.f, 18.f));
	DescText = YggUI::Text(WidgetTree, TEXT(""), 19, YggUI::Ivory, 10, TEXT("Regular"), Serif);
	DescText->SetWrapTextAt(420.f);
	Desc->AddChildToVerticalBox(DescText);
	DescChoices = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	Desc->AddChildToVerticalBox(DescChoices)->SetPadding(FMargin(0.f, 26.f, 0.f, 0.f));
	{
		UCanvasPanelSlot* DS = Canvas->AddChildToCanvas(Desc);
		DS->SetAnchors(FAnchors(0.085f, 0.5f));
		DS->SetAlignment(FVector2D(0.f, 0.f));
		DS->SetPosition(FVector2D(960.f, -190.f));
		DS->SetSize(FVector2D(440.f, 420.f));
	}

	YggUI::FChrome C = YggUI::Chrome(WidgetTree, Canvas, 0.06f, true);
	if (C.Crumb) { C.Crumb->SetText(FText::FromString(TEXT("YGGDRASIL'S PEAK      /      SETTINGS"))); }
	if (C.Right) { C.Right->SetText(FText::FromString(TEXT("SAVED AS YOU CHANGE THEM"))); }
	if (C.Foot) { C.Foot->SetText(FText::FromString(TEXT("EVEREST  ·  THE SOUTH COL ROUTE"))); }
	UHorizontalBox* Keys = YggUI::Prompts(WidgetTree, { { TEXT("↑ ↓"), TEXT("Choose") }, { TEXT("← →"), TEXT("Change") }, { TEXT("Q E"), TEXT("Tab") }, { TEXT("Esc"), TEXT("Back") } });
	Keys->SetVisibility(ESlateVisibility::HitTestInvisible);
	UCanvasPanelSlot* KS = Canvas->AddChildToCanvas(Keys);
	KS->SetAnchors(FAnchors(0.94f, 1.f));
	KS->SetAlignment(FVector2D(1.f, 0.5f));
	KS->SetPosition(FVector2D(0.f, -50.f));
	KS->SetAutoSize(true);
	GrainImg = YggUI::Grain(WidgetTree, Canvas);
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
		Width->SetWidthOverride(860.f);
		UOverlay* O = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
		R.Band = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
		R.Band->SetBrush(YggUI::TextureBrush(YggUI::BandTexture(), FVector2D(256.f, 32.f)));
		R.Band->SetColorAndOpacity(FLinearColor(0.95f, 0.88f, 0.71f, 0.15f));
		R.Band->SetRenderOpacity(0.f);
		UOverlaySlot* BandS = O->AddChildToOverlay(R.Band);
		BandS->SetHorizontalAlignment(HAlign_Fill);
		BandS->SetVerticalAlignment(VAlign_Fill);
		UVerticalBox* V = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		R.Bar = YggUI::Line(WidgetTree, 2.f, YggUI::Gold, 22.f);
		R.Bar->SetRenderOpacity(0.f);
		UHorizontalBoxSlot* RB = Row->AddChildToHorizontalBox(R.Bar);
		RB->SetVerticalAlignment(VAlign_Center);
		RB->SetPadding(FMargin(0.f, 0.f, 16.f, 0.f));
		R.LabelW = YggUI::Text(WidgetTree, R.Label, 11, YggUI::Faint, 320, TEXT("Regular"));
		R.LabelW->SetMinDesiredWidth(300.f);
		Row->AddChildToHorizontalBox(R.LabelW)->SetVerticalAlignment(VAlign_Center);
		// the control, at the right end of the row
		UHorizontalBox* Ctl = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		switch (R.Kind)
		{
		case EKind::Choice:
		case EKind::Cycle:
		{
			// < VALUE > with a pip for every choice under the value
			UTextBlock* L = nullptr; UTextBlock* Rt = nullptr;
			UButton* Prev = TextButton(TEXT("‹"), 18, L, [this, r]() { RowIndex = r; Change(-1); });
			Ctl->AddChildToHorizontalBox(Prev)->SetVerticalAlignment(VAlign_Center);
			UVerticalBox* VB = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
			R.Value = YggUI::Text(WidgetTree, TEXT(""), 12, YggUI::Ivory, 240, TEXT("Regular"));
			R.Value->SetJustification(ETextJustify::Center);
			R.Value->SetMinDesiredWidth(210.f);
			VB->AddChildToVerticalBox(R.Value)->SetHorizontalAlignment(HAlign_Center);
			if (R.Kind == EKind::Choice && R.Choices.Num() > 1)
			{
				UHorizontalBox* PipRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
				for (int32 c = 0; c < R.Choices.Num(); ++c)
				{
					UImage* Pip = YggUI::Line(WidgetTree, 14.f, YggUI::Hair, 2.f);
					PipRow->AddChildToHorizontalBox(Pip)->SetPadding(FMargin(c == 0 ? 0.f : 4.f, 0.f, 0.f, 0.f));
					R.Pips.Add(Pip);
					Keep.Add(Pip);
				}
				UVerticalBoxSlot* PS = VB->AddChildToVerticalBox(PipRow);
				PS->SetHorizontalAlignment(HAlign_Center);
				PS->SetPadding(FMargin(0.f, 7.f, 0.f, 0.f));
			}
			UButton* Mid = Clickable(VB, [this, r]() { RowIndex = r; Change(1); });
			Ctl->AddChildToHorizontalBox(Mid)->SetVerticalAlignment(VAlign_Center);
			UButton* Next = TextButton(TEXT("›"), 18, Rt, [this, r]() { RowIndex = r; Change(1); });
			Ctl->AddChildToHorizontalBox(Next)->SetVerticalAlignment(VAlign_Center);
			Keep.Add(R.Value);
			break;
		}
		case EKind::Slider:
		{
			R.SliderW = YggUI::Slider(WidgetTree, 280.f);
			R.SliderW->SetMinValue(R.Min);
			R.SliderW->SetMaxValue(R.Max);
			R.SliderW->SetStepSize(R.Step);
			R.SliderW->SetValue(R.GetF ? R.GetF() : R.Min);
			UYggUIProxy* P = NewObject<UYggUIProxy>(this);
			P->OnValue = [this, Tab, r](float V) { FRow& X = Tabs[Tab].Rows[r]; RowIndex = r; if (X.SetF) { X.SetF(V); } };
			FScriptDelegate D; D.BindUFunction(P, TEXT("Value")); R.SliderW->OnValueChanged.Add(D);
			Proxies.Add(P);
			// a slider only asks for its thumb's size: give the hairline its length; a gold fill up to the bead
			UOverlay* Track = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
			R.Fill = YggUI::Line(WidgetTree, 1.f, YggUI::Gold, 2.f);
			UOverlaySlot* FS = Track->AddChildToOverlay(R.Fill);
			FS->SetHorizontalAlignment(HAlign_Left);
			FS->SetVerticalAlignment(VAlign_Center);
			USizeBox* SB = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
			SB->SetWidthOverride(280.f);
			SB->SetHeightOverride(24.f);
			SB->SetContent(R.SliderW);
			Track->AddChildToOverlay(SB);
			UHorizontalBoxSlot* S = Ctl->AddChildToHorizontalBox(Track);
			S->SetVerticalAlignment(VAlign_Center);
			S->SetPadding(FMargin(0.f, 0.f, 20.f, 0.f));
			R.Value = YggUI::Text(WidgetTree, TEXT(""), 15, YggUI::Ivory, 20);
			R.Value->SetMinDesiredWidth(64.f);
			R.Value->SetJustification(ETextJustify::Right);
			Ctl->AddChildToHorizontalBox(R.Value)->SetVerticalAlignment(VAlign_Center);
			Keep.Add(SB); Keep.Add(R.SliderW); Keep.Add(R.Value); Keep.Add(R.Fill); Keep.Add(Track);
			break;
		}
		case EKind::Key:
		{
			UBorder* Cap = YggUI::KeyCap(WidgetTree, TEXT("-"), 10);
			Cap->SetPadding(FMargin(14.f, 2.f, 14.f, 2.f));
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
		const bool bPips = R.Kind == EKind::Choice && R.Choices.Num() > 1;
		UVerticalBoxSlot* RowS = V->AddChildToVerticalBox(Row);
		const float Pad = R.Kind == EKind::Key ? 5.f : (bPips ? 7.f : 9.f);
		RowS->SetPadding(FMargin(0.f, Pad, 14.f, bPips ? 6.f : Pad));
		UImage* Under = YggUI::Line(WidgetTree, 8.f, YggUI::Rule);
		V->AddChildToVerticalBox(Under)->SetHorizontalAlignment(HAlign_Fill);
		O->AddChildToOverlay(V)->SetHorizontalAlignment(HAlign_Fill);
		Width->SetContent(O);
		Hit->SetContent(Width);
		R.Hit = Hit;
		Keep.Add(Hit); Keep.Add(R.LabelW); Keep.Add(R.Band); Keep.Add(R.Bar);
		Box->AddChildToVerticalBox(Hit);
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
				if (R.Value) { R.Value->SetText(FText::FromString(R.Choices.IsValidIndex(Cur) ? R.Choices[Cur].ToUpper() : FString())); }
				for (int32 c = 0; c < R.Pips.Num(); ++c)
				{
					if (R.Pips[c]) { R.Pips[c]->SetColorAndOpacity(c == Cur ? YggUI::Gold : FLinearColor(0.93f, 0.91f, 0.84f, 0.22f)); }
				}
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
	DescTitle->SetText(FText::FromString(R.Label));
	DescText->SetText(FText::FromString(R.Hint));
	DescChoices->ClearChildren();
	if (R.Kind == EKind::Choice && R.Get)
	{
		const int32 Cur = R.Get();
		for (int32 c = 0; c < R.Choices.Num(); ++c)
		{
			UHorizontalBox* L = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
			USizeBox* BB = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
			BB->SetWidthOverride(8.f);
			BB->SetHeightOverride(8.f);
			UImage* Bd = YggUI::Bead(WidgetTree, c == Cur ? 7.f : 4.f, c == Cur ? YggUI::Gold : FLinearColor(0.93f, 0.91f, 0.84f, 0.25f));
			if (USizeBoxSlot* BS = Cast<USizeBoxSlot>(BB->SetContent(Bd))) { BS->SetHorizontalAlignment(HAlign_Center); BS->SetVerticalAlignment(VAlign_Center); }
			UHorizontalBoxSlot* S = L->AddChildToHorizontalBox(BB);
			S->SetVerticalAlignment(VAlign_Center);
			S->SetPadding(FMargin(0.f, 0.f, 14.f, 0.f));
			L->AddChildToHorizontalBox(YggUI::Text(WidgetTree, R.Choices[c].ToUpper(), 10, c == Cur ? YggUI::Ivory : YggUI::Faint, 300, TEXT("Regular")))->SetVerticalAlignment(VAlign_Center);
			DescChoices->AddChildToVerticalBox(L)->SetPadding(FMargin(0.f, 0.f, 0.f, 10.f));
		}
	}
	else if (R.Kind == EKind::Key)
	{
		DescChoices->AddChildToVerticalBox(YggUI::Text(WidgetTree, TEXT("ENTER OR CLICK, THEN THE NEW KEY  ·  ESC KEEPS THE OLD ONE"), 9, YggUI::Faint, 300, TEXT("Regular")));
	}
}

void USettingsWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	Age += InDeltaTime;
	TabAge += InDeltaTime;
	DescAge += InDeltaTime;
	SetRenderOpacity(YggUI::Ease((Age - 0.15f) * 2.2f));   // a moment after the words under it begin to fade
	if (Pages) { Pages->SetRenderOpacity(YggUI::Ease(TabAge * 3.f)); Pages->SetRenderTranslation(FVector2D(-14.f * (1.f - YggUI::Ease(TabAge * 3.f)), 0.f)); }
	YggUI::JitterGrain(GrainImg, GrainClock, InDeltaTime);
	if (Blur) { Blur->SetBlurStrength(7.f * YggUI::Ease(Age * 2.5f)); }
	if (!Tabs.IsValidIndex(TabIndex)) { return; }
	TArray<FRow>& Rows = Tabs[TabIndex].Rows;
	if (bMouseMoved)
	{
		for (int32 r = 0; r < Rows.Num(); ++r) { if (Rows[r].Hit && Rows[r].Hit->IsHovered()) { RowIndex = r; } }
	}
	RowIndex = FMath::Clamp(RowIndex, 0, FMath::Max(Rows.Num() - 1, 0));
	for (int32 r = 0; r < Rows.Num(); ++r)
	{
		FRow& R = Rows[r];
		const bool bHot = r == RowIndex;
		R.Hot = FMath::FInterpTo(R.Hot, bHot ? 1.f : 0.f, InDeltaTime, 14.f);
		const float In = YggUI::Ease((TabAge - 0.03f * r) * 5.f);
		if (R.Hit) { R.Hit->SetRenderOpacity(In); R.Hit->SetRenderTranslation(FVector2D(5.f * R.Hot - 10.f * (1.f - In), 0.f)); }
		if (R.LabelW) { R.LabelW->SetColorAndOpacity(FSlateColor(FMath::Lerp(YggUI::Faint, YggUI::Ivory, R.Hot))); }
		if (R.Band) { R.Band->SetRenderOpacity(R.Hot); }
		if (R.Bar) { R.Bar->SetRenderOpacity(R.Hot); }
		if (R.Kind == EKind::Choice || R.Kind == EKind::Cycle)
		{
			if (R.Value) { R.Value->SetColorAndOpacity(FSlateColor(FMath::Lerp(YggUI::Dim, YggUI::Ivory, R.Hot))); }
		}
		if (R.Kind == EKind::Slider && R.SliderW)
		{
			const float V = R.SliderW->GetValue();
			if (R.Value && R.Fmt) { R.Value->SetText(FText::FromString(R.Fmt(V))); }
			if (R.Fill) { R.Fill->SetDesiredSizeOverride(FVector2D(FMath::Max(1.f, 280.f * (V - R.Min) / FMath::Max(R.Max - R.Min, 0.0001f)), 2.f)); }
		}
		if (R.Kind == EKind::Key && R.Value)
		{
			if (r == Waiting) { R.Value->SetRenderOpacity(0.55f + 0.45f * FMath::Abs(FMath::Sin(Age * 3.f))); R.Value->SetColorAndOpacity(FSlateColor(YggUI::Gold)); }
			else { R.Value->SetRenderOpacity(1.f); R.Value->SetColorAndOpacity(FSlateColor(FMath::Lerp(YggUI::Dim, YggUI::Ivory, R.Hot))); }
		}
	}
	for (int32 t = 0; t < TabTexts.Num(); ++t)
	{
		const UWidget* Btn = TabTexts[t] && TabTexts[t]->GetParent() ? TabTexts[t]->GetParent()->GetParent() : nullptr;
		TabHot[t] = FMath::FInterpTo(TabHot[t], t == TabIndex ? 1.f : 0.f, InDeltaTime, 12.f);
		if (TabTexts[t]) { TabTexts[t]->SetColorAndOpacity(FSlateColor(t == TabIndex || (Btn && Btn->IsHovered()) ? YggUI::Ivory : YggUI::Dim)); }
		if (TabLines[t]) { TabLines[t]->SetRenderOpacity(TabHot[t]); TabLines[t]->SetRenderScale(FVector2D(FMath::Max(TabHot[t], 0.01f), 1.f)); }
	}
	for (UTextBlock* T : { ResetText.Get(), BackText.Get() })
	{
		if (T) { const UWidget* B = T->GetParent() ? T->GetParent()->GetParent() : nullptr; T->SetColorAndOpacity(FSlateColor(B && B->IsHovered() ? YggUI::Ivory : YggUI::Dim)); }
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
	if (!InMouseEvent.GetCursorDelta().IsNearlyZero()) { bMouseMoved = true; }
	return Super::NativeOnMouseMove(InGeometry, InMouseEvent);
}
