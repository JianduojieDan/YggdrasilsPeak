// Copyright Epic Games, Inc. All Rights Reserved.

#include "Survival/YggMenuWidget.h"

#include "Survival/SettingsWidget.h"
#include "Survival/YggUIStyle.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "HAL/IConsoleManager.h"

static FAutoConsoleCommandWithWorldAndArgs CmdMenu(TEXT("ygg.Menu"), TEXT("ygg.Menu pause|summit - opens a menu (for checking)."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
		if (Args.Num() > 0 && Args[0] == TEXT("summit")) { UYggMenuWidget::ShowSummit(PC, 5 * 3600 + 42 * 60, 2); }
		else { UYggMenuWidget::TogglePause(PC); }
	}));

namespace
{
	UYggMenuWidget* FindOpen(APlayerController* PC)
	{
		TArray<UUserWidget*> Open;
		UWidgetBlueprintLibrary::GetAllWidgetsOfClass(PC, Open, UYggMenuWidget::StaticClass(), true);
		return Open.Num() > 0 ? Cast<UYggMenuWidget>(Open[0]) : nullptr;
	}
	void Loose(APlayerController* PC, UUserWidget* W, bool bOpen)
	{
		PC->SetShowMouseCursor(bOpen);
		PC->SetIgnoreLookInput(bOpen);
		PC->SetIgnoreMoveInput(bOpen);
		if (bOpen)
		{
			FInputModeGameAndUI Mode;
			Mode.SetWidgetToFocus(W->TakeWidget());
			Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
			Mode.SetHideCursorDuringCapture(false);
			PC->SetInputMode(Mode);
			W->SetKeyboardFocus();
		}
		else
		{
			PC->SetInputMode(FInputModeGameOnly());
		}
	}
}

bool UYggMenuWidget::IsOpen(APlayerController* PC)
{
	return PC && FindOpen(PC) != nullptr;
}

void UYggMenuWidget::CloseAll(APlayerController* PC)
{
	if (UYggMenuWidget* Open = PC ? FindOpen(PC) : nullptr) { Open->Close(); }
}

void UYggMenuWidget::TogglePause(APlayerController* PC)
{
	if (!PC)
	{
		return;
	}
	if (UYggMenuWidget* Open = FindOpen(PC))
	{
		if (Open->bPause) { Open->Do(EAction::Resume); }
		return;
	}
	UYggMenuWidget* W = CreateWidget<UYggMenuWidget>(PC, UYggMenuWidget::StaticClass());
	if (!W)
	{
		return;
	}
	USettingsWidget::CloseIfOpen(PC);
	W->SetIsFocusable(true);
	W->Eyebrow = TEXT("THE MOUNTAIN WAITS");
	W->Title = TEXT("Paused");
	W->Buttons = { { TEXT("Resume"), EAction::Resume }, { TEXT("Settings"), EAction::Settings }, { TEXT("Start menu"), EAction::StartMenu }, { TEXT("Quit"), EAction::Quit } };
	W->bPause = true;
	W->AddToViewport(70);
	Loose(PC, W, true);
	UGameplayStatics::SetGamePaused(PC, true);
}

void UYggMenuWidget::ShowSummit(APlayerController* PC, int32 Seconds, int32 Deaths)
{
	if (!PC || FindOpen(PC))
	{
		return;
	}
	UYggMenuWidget* W = CreateWidget<UYggMenuWidget>(PC, UYggMenuWidget::StaticClass());
	if (!W)
	{
		return;
	}
	USettingsWidget::CloseIfOpen(PC);
	W->SetIsFocusable(true);
	W->Eyebrow = TEXT("YGGDRASIL'S PEAK");
	W->Title = TEXT("The Summit");
	W->SubLine = TEXT("You stand beneath the World Tree.");
	W->Stats = {
		{ TEXT("8,849|m"), TEXT("ALTITUDE") },
		{ FString::Printf(TEXT("%d|h  %02d|m"), Seconds / 3600, (Seconds / 60) % 60), TEXT("ON THE MOUNTAIN") },
		{ FString::Printf(TEXT("%d|"), Deaths), TEXT("FALLS") } };
	W->Buttons = { { TEXT("Stay at the summit"), EAction::Resume }, { TEXT("Start menu"), EAction::StartMenu }, { TEXT("Quit"), EAction::Quit } };
	W->AddToViewport(70);
	Loose(PC, W, true);
}

TSharedRef<SWidget> UYggMenuWidget::RebuildWidget()
{
	if (!WidgetTree->RootWidget)
	{
		Build();
	}
	return Super::RebuildWidget();
}

void UYggMenuWidget::Build()
{
	using namespace YggUI;
	UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Canvas"));
	WidgetTree->RootWidget = Canvas;
	Serif = MakeSerif(this);

	// a soft veil over the world: the scene stays visible, the text stays readable
	UBorder* Veil = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Veil"));
	Veil->SetBrushColor(FLinearColor(0.004f, 0.006f, 0.012f, bPause ? 0.55f : 0.5f));
	UCanvasPanelSlot* VS = Canvas->AddChildToCanvas(Veil);
	VS->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
	VS->SetOffsets(FMargin(0.f));

	UVerticalBox* Col = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	auto Add = [Col](UWidget* W, float Gap)
	{
		UVerticalBoxSlot* S = Col->AddChildToVerticalBox(W);
		S->SetHorizontalAlignment(HAlign_Center);
		S->SetPadding(FMargin(0.f, 0.f, 0.f, Gap));
	};
	Add(Text(WidgetTree, Eyebrow, 10, Dim, 420, TEXT("Regular")), 6.f);
	Add(Text(WidgetTree, Title, 52, Ivory, 0, TEXT("Regular"), Serif), 10.f);
	Add(Divider(WidgetTree, 70.f), SubLine.IsEmpty() ? 34.f : 18.f);
	if (!SubLine.IsEmpty())
	{
		Add(Text(WidgetTree, SubLine, 17, Ivory, 20, TEXT("Regular"), Serif), 30.f);
	}
	if (Stats.Num() > 0)
	{
		UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		for (int32 i = 0; i < Stats.Num(); ++i)
		{
			UVerticalBox* Cell = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
			// "8,849|m" -> light numerals with a small unit on their baseline (as the HUD's altitude)
			UHorizontalBox* Num = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
			TArray<FString> Parts;
			Stats[i].Key.ParseIntoArray(Parts, TEXT("|"), false);
			for (int32 p = 0; p < Parts.Num(); ++p)
			{
				FString Piece = Parts[p];
				FString Unit;
				if (p > 0) { Unit = Piece.Left(Piece.Find(TEXT(" ")) >= 0 ? Piece.Find(TEXT(" ")) : Piece.Len()); Piece = Piece.Mid(Unit.Len()); }
				if (!Unit.IsEmpty())
				{
					{ UHorizontalBoxSlot* US = Num->AddChildToHorizontalBox(Text(WidgetTree, Unit, 15, Dim, 120)); US->SetVerticalAlignment(VAlign_Bottom); US->SetPadding(FMargin(0.f, 0.f, 0.f, 4.f)); }
				}
				if (!Piece.TrimStartAndEnd().IsEmpty())
				{
					UHorizontalBoxSlot* S = Num->AddChildToHorizontalBox(Text(WidgetTree, Piece.TrimStartAndEnd(), 34, Ivory, 10));
					S->SetVerticalAlignment(VAlign_Bottom);
					S->SetPadding(FMargin(p > 0 ? 10.f : 0.f, 0.f, 3.f, 0.f));
				}
			}
			Cell->AddChildToVerticalBox(Num)->SetHorizontalAlignment(HAlign_Center);
			UVerticalBoxSlot* LS = Cell->AddChildToVerticalBox(Text(WidgetTree, Stats[i].Value, 9, Faint, 300, TEXT("Regular")));
			LS->SetHorizontalAlignment(HAlign_Center);
			LS->SetPadding(FMargin(0.f, 4.f, 0.f, 0.f));
			UHorizontalBoxSlot* CS = Row->AddChildToHorizontalBox(Cell);
			CS->SetPadding(FMargin(i == 0 ? 0.f : 30.f, 0.f, 30.f, 0.f));
			if (i + 1 < Stats.Num())
			{
				UImage* Sep = Line(WidgetTree, 1.f, Hair, 34.f);
				Row->AddChildToHorizontalBox(Sep)->SetVerticalAlignment(VAlign_Center);
			}
		}
		Add(Row, 40.f);
	}
	static const FName Handlers[] = { TEXT("OnB0"), TEXT("OnB1"), TEXT("OnB2"), TEXT("OnB3") };
	for (int32 i = 0; i < Buttons.Num() && i < 4; ++i)
	{
		UTextBlock* T = nullptr; UImage* B = nullptr;
		UButton* Btn = Option(WidgetTree, Buttons[i].Key, 12, T, B);
		FScriptDelegate D;
		D.BindUFunction(this, Handlers[i]);
		Btn->OnClicked.Add(D);
		OptButtons.Add(Btn); OptTexts.Add(T); OptBeads.Add(B);
		Add(Btn, 12.f);
	}
	UCanvasPanelSlot* CS = Canvas->AddChildToCanvas(Col);
	CS->SetAnchors(FAnchors(0.5f, 0.46f));
	CS->SetAlignment(FVector2D(0.5f, 0.5f));
	CS->SetAutoSize(true);
	SetRenderOpacity(0.f);
}

void UYggMenuWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	FadeIn = FMath::Min(1.f, FadeIn + InDeltaTime * 2.5f);   // ticks while paused (UI), fades in like the HUD captions
	SetRenderOpacity(FadeIn * FadeIn * (3.f - 2.f * FadeIn));
	// the hovered option brightens and gets the bead (the first one has it until the mouse moves onto another)
	int32 Hot = 0;
	for (int32 i = 0; i < OptButtons.Num(); ++i) { if (OptButtons[i] && OptButtons[i]->IsHovered()) { Hot = i; } }
	for (int32 i = 0; i < OptTexts.Num(); ++i)
	{
		if (OptTexts[i]) { OptTexts[i]->SetColorAndOpacity(FSlateColor(i == Hot ? YggUI::Ivory : YggUI::Dim)); }
		if (OptBeads[i]) { OptBeads[i]->SetRenderOpacity(i == Hot ? 1.f : 0.f); }
	}
}

FReply UYggMenuWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.GetKey() == EKeys::Escape || InKeyEvent.GetKey() == EKeys::P)
	{
		Do(EAction::Resume);
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void UYggMenuWidget::Close()
{
	if (APlayerController* PC = GetOwningPlayer())
	{
		Loose(PC, this, false);
		if (bPause) { UGameplayStatics::SetGamePaused(PC, false); }
	}
	RemoveFromParent();
}

void UYggMenuWidget::Do(EAction Action)
{
	APlayerController* PC = GetOwningPlayer();
	switch (Action)
	{
	case EAction::Resume:
		Close();
		break;
	case EAction::Settings:
		Close();
		USettingsWidget::Toggle(PC);
		break;
	case EAction::StartMenu:
		if (PC) { UGameplayStatics::SetGamePaused(PC, false); }
		UGameplayStatics::OpenLevel(this, FName(TEXT("/Game/UI/StartMenu/L_StartMenu")));
		break;
	case EAction::Quit:
		UKismetSystemLibrary::QuitGame(this, PC, EQuitPreference::Quit, false);
		break;
	}
}

void UYggMenuWidget::OnB0() { if (Buttons.IsValidIndex(0)) { Do(Buttons[0].Value); } }
void UYggMenuWidget::OnB1() { if (Buttons.IsValidIndex(1)) { Do(Buttons[1].Value); } }
void UYggMenuWidget::OnB2() { if (Buttons.IsValidIndex(2)) { Do(Buttons[2].Value); } }
void UYggMenuWidget::OnB3() { if (Buttons.IsValidIndex(3)) { Do(Buttons[3].Value); } }
