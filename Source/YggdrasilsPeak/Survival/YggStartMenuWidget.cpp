// Copyright Epic Games, Inc. All Rights Reserved.

#include "Survival/YggStartMenuWidget.h"

#include "Survival/SettingsWidget.h"
#include "Survival/YggMenuWidget.h"
#include "Survival/YggSaveGame.h"
#include "Survival/YggSettings.h"
#include "Survival/YggUIStyle.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Rendering/DrawElements.h"

namespace
{
	constexpr float SlideSeconds = 17.f;
	constexpr float BlendSeconds = 3.5f;
	FSlateBrush& FlakeBrush()
	{
		static FSlateBrush B;
		if (!B.GetResourceObject()) { B = YggUI::TextureBrush(YggUI::GlowTexture(), FVector2D(64.f, 64.f)); }
		return B;
	}
}

UYggStartMenuWidget* UYggStartMenuWidget::Show(APlayerController* PC)
{
	if (!PC) { return nullptr; }
	UYggStartMenuWidget* W = CreateWidget<UYggStartMenuWidget>(PC, UYggStartMenuWidget::StaticClass());
	if (!W) { return nullptr; }
	W->VeilAlpha = 0.82f;
	W->ColumnX = 0.08f;
	W->bHeader = false;
	W->FooterLeft = TEXT("Everest  ·  8,849 m  ·  the South Col route");
	// the title on its own first (once a session), then the menu
	static bool bSplashShown = false;
	if (!bSplashShown) { bSplashShown = true; W->SplashPage(); }
	else { W->MainPage(); }
	W->Open(PC, 50, true);
	return W;
}

void UYggStartMenuWidget::BuildBackground(UCanvasPanel* InCanvas)
{
	// our own mountain, captured in the game (Tools/UI/capture_backdrops.py); the old photo if none are there yet
	for (int32 i = 1; i <= 8; ++i)
	{
		const FString N = FString::Printf(TEXT("T_Backdrop_%02d"), i);
		if (UTexture2D* T = LoadObject<UTexture2D>(nullptr, *FString::Printf(TEXT("/Game/UI/StartMenu/Backdrops/%s.%s"), *N, *N), nullptr, LOAD_NoWarn | LOAD_Quiet)) { Backdrops.Add(T); }
	}
	if (Backdrops.Num() == 0)
	{
		if (UTexture2D* T = LoadObject<UTexture2D>(nullptr, TEXT("/Game/UI/StartMenu/weichao-deng-eyn0LjpNWV4-unsplash.weichao-deng-eyn0LjpNWV4-unsplash"), nullptr, LOAD_NoWarn | LOAD_Quiet)) { Backdrops.Add(T); }
	}
	auto MakeBack = [this, InCanvas](const TCHAR* Name)
	{
		UImage* I = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), Name);
		UCanvasPanelSlot* S = InCanvas->AddChildToCanvas(I);
		S->SetAnchors(FAnchors(0.5f, 0.5f));
		S->SetAlignment(FVector2D(0.5f, 0.5f));
		return I;
	};
	BackA = MakeBack(TEXT("BackA"));
	BackB = MakeBack(TEXT("BackB"));
	if (Backdrops.Num() > 0) { BackA->SetBrushFromTexture(Backdrops[0]); }
	if (Backdrops.Num() > 1) { BackB->SetBrushFromTexture(Backdrops[1]); }
	BackB->SetRenderOpacity(0.f);

	// snow drifting across the view: a few big soft flakes near, many small far ones
	FRandomStream R(7);
	for (int32 i = 0; i < 150; ++i)
	{
		FFlake F;
		const float Depth = FMath::Pow(R.FRand(), 2.2f);   // most are far
		F.P = FVector2f(R.FRand(), R.FRand());
		F.Size = FMath::Lerp(2.f, 11.f, Depth);
		F.Speed = FMath::Lerp(0.012f, 0.06f, Depth);
		F.Sway = R.FRandRange(0.004f, 0.02f);
		F.Phase = R.FRandRange(0.f, 6.28f);
		F.Alpha = FMath::Lerp(0.18f, 0.5f, Depth) * R.FRandRange(0.6f, 1.f);
		Flakes.Add(F);
	}
}

void UYggStartMenuWidget::SplashPage()
{
	FPage Pg;
	Pg.Eyebrow = TEXT("A CLIMB BENEATH THE WORLD TREE");
	Pg.Title = TEXT("Yggdrasil's Peak");
	Pg.TitleSize = 104.f;
	Pg.bNineBeads = true;
	Pg.bEmblem = true;
	Pg.bSplash = true;
	Pg.AnyKey = [this]() { MainPage(); };
	SetPage(Pg);
}

void UYggStartMenuWidget::MainPage()
{
	FPage Pg;
	Pg.Eyebrow = TEXT("A CLIMB BENEATH THE WORLD TREE");
	Pg.Title = TEXT("Yggdrasil's Peak");
	Pg.TitleSize = 82.f;
	Pg.bNineBeads = true;
	Pg.bEmblem = true;
	Pg.Line = TEXT("“Was it worth it?”");
	const UYggSaveGame* Save = UGameplayStatics::DoesSaveGameExist(UYggSaveGame::SlotName, 0)
		? Cast<UYggSaveGame>(UGameplayStatics::LoadGameFromSlot(UYggSaveGame::SlotName, 0)) : nullptr;
	if (Save && Save->Version != GetDefault<UYggSaveGame>()->Version) { Save = nullptr; }
	if (Save)
	{
		const int32 S = FMath::RoundToInt(Save->PlaySeconds);
		FString Sub = FString::Printf(TEXT("%s   ·   %d h %02d m"), Save->CampName.IsEmpty() ? TEXT("The start") : *Save->CampName, S / 3600, (S / 60) % 60);
		if (Save->Deaths > 0) { Sub += FString::Printf(TEXT("   ·   %d %s"), Save->Deaths, Save->Deaths == 1 ? TEXT("fall") : TEXT("falls")); }
		Pg.Options.Add({ TEXT("Continue"), Sub, [this]() { Begin(false); } });
	}
	const bool bHasSave = Save != nullptr;
	Pg.Options.Add({ TEXT("New expedition"), TEXT("Dingboche, 4,410 m"), [this, bHasSave]()
		{
			if (!bHasSave) { Begin(true); return; }
			const FPage Before = Page;
			FPage C;
			C.Eyebrow = Before.Eyebrow;
			C.Title = TEXT("Begin again?");
			C.TitleSize = 48.f;
			C.bModal = true;
			C.Line = TEXT("The climb you have saved will be lost.");
			C.Options = { { TEXT("Begin again"), FString(), [this]() { Begin(true); } }, { TEXT("Keep my climb"), FString(), [this, Before]() { SetPage(Before); } } };
			C.Selected = 1;
			C.Back = [this, Before]() { SetPage(Before); };
			SetPage(C);
		} });
	Pg.Options.Add({ TEXT("Settings"), FString(), [this]()
		{
			TWeakObjectPtr<UYggStartMenuWidget> Self(this);
			SetColumnVisible(false);
			USettingsWidget::Open(GetOwningPlayer(), [Self]()
			{
				if (!Self.IsValid()) { return; }
				Self->SetColumnVisible(true);
				if (APlayerController* PC = Self->GetOwningPlayer())
				{
					FInputModeUIOnly Mode;
					Mode.SetWidgetToFocus(Self->TakeWidget());
					PC->SetInputMode(Mode);
				}
				Self->SetKeyboardFocus();
			}, true);
		} });
	Pg.Options.Add({ TEXT("Credits"), FString(), [this]()
		{
			const FPage Before = Page;
			FPage C = UYggMenuWidget::CreditsPage();
			C.Options = { { TEXT("Back"), FString(), [this, Before]() { SetPage(Before); } } };
			C.Back = [this, Before]() { SetPage(Before); };
			SetPage(C);
		} });
	Pg.Options.Add({ TEXT("Quit"), FString(), [this]() { UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, false); } });
	SetPage(Pg);
}

void UYggStartMenuWidget::Begin(bool bNew)
{
	if (Leaving >= 0.f) { return; }
	if (bNew) { UGameplayStatics::DeleteGameInSlot(UYggSaveGame::SlotName, 0); }
	bLeaveNew = bNew;
	Leaving = 0.f;
}

void UYggStartMenuWidget::PlaceBackdrop(UImage* Img, UTexture2D* Tex, float Zoom, const FVector2D& Pan, const FVector2D& View)
{
	if (!Img || !Tex) { return; }
	// cover the whole view without stretching, a little larger so it can drift
	const float Aspect = float(Tex->GetSizeX()) / FMath::Max(1.f, float(Tex->GetSizeY()));
	FVector2D Size(View.X, View.X / Aspect);
	if (Size.Y < View.Y) { Size = FVector2D(View.Y * Aspect, View.Y); }
	Size *= Zoom;
	if (UCanvasPanelSlot* S = Cast<UCanvasPanelSlot>(Img->Slot))
	{
		S->SetSize(Size);
		S->SetPosition(Pan * (Size - View) * 0.5f);
	}
}

void UYggStartMenuWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	const FVector2D View = MyGeometry.GetLocalSize();
	Slide += InDeltaTime;
	// slow drift: each view zooms in a touch and slides one way, then dissolves into the next
	auto Drift = [](int32 Index, float T, float& OutZoom, FVector2D& OutPan)
	{
		const float K = FMath::Clamp(T / (SlideSeconds + BlendSeconds), 0.f, 1.f);
		OutZoom = FMath::Lerp(1.05f, 1.13f, K);
		const float A = Index * 2.39996f;   // golden angle: every view drifts its own way
		OutPan = FVector2D(FMath::Cos(A), FMath::Sin(A) * 0.5f) * FMath::Lerp(-0.6f, 0.6f, K);
	};
	if (Backdrops.Num() > 0)
	{
		float Z; FVector2D Pn;
		Drift(Current, Slide, Z, Pn);
		PlaceBackdrop(BackA, Backdrops[Current % Backdrops.Num()], Z, Pn, View);
		if (Backdrops.Num() > 1)
		{
			const int32 Next = (Current + 1) % Backdrops.Num();
			const float Into = Slide - SlideSeconds;
			Drift(Next, FMath::Max(Into, 0.f), Z, Pn);
			PlaceBackdrop(BackB, Backdrops[Next], Z, Pn, View);
			BackB->SetRenderOpacity(FMath::SmoothStep(0.f, BlendSeconds, Into));
			if (Into >= BlendSeconds)
			{
				// the next view becomes the current one
				Current = Next;
				Slide = BlendSeconds;
				BackA->SetBrushFromTexture(Backdrops[Current]);
				BackB->SetBrushFromTexture(Backdrops[(Current + 1) % Backdrops.Num()]);
				BackB->SetRenderOpacity(0.f);
			}
		}
	}
	for (FFlake& F : Flakes)
	{
		F.P.Y += F.Speed * InDeltaTime;
		F.P.X += (0.01f + F.Sway * FMath::Sin(Age * 0.7f + F.Phase)) * InDeltaTime;
		if (F.P.Y > 1.05f) { F.P.Y -= 1.1f; F.P.X = FMath::FRand(); }
		if (F.P.X > 1.05f) { F.P.X -= 1.1f; }
	}
	if (Leaving >= 0.f)
	{
		Leaving += InDeltaTime;
		SetRenderOpacity(FMath::Clamp(1.f - Leaving / 1.1f, 0.f, 1.f));
		if (Leaving > 1.2f)
		{
			Leaving = -100.f;
			// the start menu's UI-only input lives on the game viewport and would follow us onto the mountain
			// (to-do #64: the player could not walk or look after the menus)
			if (APlayerController* PC = GetOwningPlayer()) { PC->SetInputMode(FInputModeGameOnly()); PC->SetShowMouseCursor(false); }
			UGameplayStatics::OpenLevel(this, FName(TEXT("/Game/LevelPrototyping/base_map")));
		}
	}
}

int32 UYggStartMenuWidget::NativePaint(const FPaintArgs& Args, const FGeometry& G, const FSlateRect& Cull,
	FSlateWindowElementList& Out, int32 LayerId, const FWidgetStyle& Style, bool bParentEnabled) const
{
	int32 Layer = Super::NativePaint(Args, G, Cull, Out, LayerId, Style, bParentEnabled) + 1;
	const FVector2D Size = G.GetLocalSize();
	const FSlateBrush& B = FlakeBrush();
	const float Fade = FMath::Clamp(Age / 2.f, 0.f, 1.f) * Style.GetColorAndOpacityTint().A;
	for (const FFlake& F : Flakes)
	{
		const FVector2D P(F.P.X * Size.X, F.P.Y * Size.Y);
		FSlateDrawElement::MakeBox(Out, Layer, G.ToPaintGeometry(FVector2f(F.Size, F.Size), FSlateLayoutTransform(FVector2f(P) - FVector2f(F.Size * 0.5f))),
			&B, ESlateDrawEffect::None, FLinearColor(0.95f, 0.97f, 1.f, F.Alpha * Fade));
	}
	return Layer;
}

// ---------------------------------------------------------------------------------------------------------------

AYggMenuGameMode::AYggMenuGameMode()
{
	DefaultPawnClass = nullptr;
}

void AYggMenuGameMode::StartPlay()
{
	// the old Blueprint start menu (BP_MenuBootstrap made WBP_StartMenu) goes before it can begin
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		if (It->GetClass()->GetName().Contains(TEXT("MenuBootstrap"))) { It->Destroy(); }
	}
	Super::StartPlay();
	YggSettings::ApplyAll(GetWorld());
	if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
	{
		UYggStartMenuWidget::Show(PC);
	}
}
