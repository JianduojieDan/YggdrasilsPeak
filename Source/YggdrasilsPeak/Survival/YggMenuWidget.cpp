// Copyright Epic Games, Inc. All Rights Reserved.

#include "Survival/YggMenuWidget.h"

#include "Survival/ExpeditionComponent.h"
#include "Survival/SettingsWidget.h"
#include "Survival/SurvivalComponent.h"
#include "Survival/YggSettings.h"
#include "Survival/YggFateFx.h"
#include "Survival/YggUIStyle.h"
#include "Environment/RouteLandmark.h"
#include "GameFramework/Character.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"

static FAutoConsoleCommandWithWorldAndArgs CmdMenu(TEXT("ygg.Menu"), TEXT("ygg.Menu pause|summit|death|credits - opens a menu (for checking)."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
		const FString A = Args.Num() > 0 ? Args[0] : FString();
		if (A == TEXT("summit")) { UYggMenuWidget::ShowSummit(PC, 5 * 3600 + 42 * 60, 2); }
		else if (A == TEXT("death"))
		{
			// ygg.Menu death [cold|oxygen|fall|avalanche|thirst|hunger|lost]
			const FString W = Args.Num() > 1 ? Args[1] : FString(TEXT("cold"));
			const FString Cause = W == TEXT("oxygen") ? TEXT("Your lungs ran out of oxygen.") : W == TEXT("fall") ? TEXT("You fell into a crevasse.")
				: W == TEXT("avalanche") ? TEXT("An avalanche buried you.") : W == TEXT("thirst") ? TEXT("You died of thirst.")
				: W == TEXT("hunger") ? TEXT("You starved on the mountain.") : W == TEXT("lost") ? TEXT("The mountain took you.") : TEXT("You froze to death.");
			UYggMenuWidget::ShowDeath(PC, FText::FromString(Cause), 6230.f, 2 * 3600 + 14 * 60, 1, TEXT("Camp II"), []() {});
		}
		else if (A == TEXT("credits")) { UYggMenuWidget::ShowCredits(PC); }
		else { UYggMenuWidget::TogglePause(PC); }
	}));

namespace
{
	UYggMenuWidget* FindOpen(APlayerController* PC)
	{
		TArray<UUserWidget*> Open;
		UWidgetBlueprintLibrary::GetAllWidgetsOfClass(PC, Open, UYggMenuWidget::StaticClass(), true);
		for (UUserWidget* W : Open) { if (UYggMenuWidget* M = Cast<UYggMenuWidget>(W); M && !M->IsClosing()) { return M; } }   // one fading out is gone
		return nullptr;
	}

	FString Duration(int32 Seconds)
	{
		return FString::Printf(TEXT("%d h %02d m"), Seconds / 3600, (Seconds / 60) % 60);
	}

	FString Height(float Meters)
	{
		return FString::Printf(TEXT("%s %s"), *YggSettings::AltitudeNumber(Meters), YggSettings::AltitudeUnit());
	}

	FString MenuFalls(int32 N)
	{
		return N == 0 ? FString(TEXT("no falls")) : FString::Printf(TEXT("%d %s"), N, N == 1 ? TEXT("fall") : TEXT("falls"));
	}

	/** A camp's name without the height it carries in brackets ("Camp 3 (7160 m)"). */
	FString MenuCamp(const FString& In)
	{
		const int32 Br = In.Find(TEXT("("));
		return Br > 0 ? In.Left(Br).TrimStartAndEnd() : In;
	}

	const TCHAR* MenuDot = TEXT("   ·   ");

	/** One word for what took you, from the cause line. */
	FString DeathWord(const FString& Cause)
	{
		const FString C = Cause.ToLower();
		if (C.Contains(TEXT("froze")) || C.Contains(TEXT("cold"))) { return TEXT("Frozen"); }
		if (C.Contains(TEXT("oxygen")) || C.Contains(TEXT("breath"))) { return TEXT("Breathless"); }
		if (C.Contains(TEXT("thirst"))) { return TEXT("Parched"); }
		if (C.Contains(TEXT("starv")) || C.Contains(TEXT("hunger"))) { return TEXT("Starved"); }
		if (C.Contains(TEXT("fell")) || C.Contains(TEXT("fall")) || C.Contains(TEXT("crevasse"))) { return TEXT("Fallen"); }
		if (C.Contains(TEXT("avalanche")) || C.Contains(TEXT("serac")) || C.Contains(TEXT("ice"))) { return TEXT("Buried"); }
		return TEXT("Lost");
	}
}

TArray<FString> UYggMenuWidget::GetCredits()
{
	return {
		TEXT("#A game by"), TEXT("JianduojieDan"),
		TEXT("#The mountain"), TEXT("Everest at 1:1, from Terrain Tiles on AWS (Mapzen / Tilezen: SRTM and others)"), TEXT("Glacier outlines © OpenStreetMap contributors"),
		TEXT("#Sky and weather"), TEXT("Ultra Dynamic Sky and Ultra Dynamic Weather, Everett Gunther"),
		TEXT("#Rocks, ice and forest"), TEXT("Quixel Megascans and Fab, Megaplant Library"),
		TEXT("#Sound"), TEXT("Field recordings by the author; BigSoundBank; OpenGameArt"),
		TEXT("#Type"), TEXT("Alegreya, Juan Pablo del Peral (Huerta Tipográfica)  ·  Roboto"),
		TEXT("#Made with"), TEXT("Unreal Engine 5, Blender  ·  assisted by Claude and ChatGPT"),
	};
}

UYggMenuBase::FPage UYggMenuWidget::CreditsPage()
{
	FPage C;
	C.Title = TEXT("Credits");
	C.TitleSize = 72.f;
	C.Body = GetCredits();
	C.bRoll = true;
	return C;
}

UYggMenuWidget* UYggMenuWidget::Make(APlayerController* PC)
{
	if (!PC)
	{
		return nullptr;
	}
	USettingsWidget::CloseIfOpen(PC);
	if (UYggMenuWidget* Old = FindOpen(PC)) { Old->RemoveFromParent(); }
	return CreateWidget<UYggMenuWidget>(PC, UYggMenuWidget::StaticClass());
}

bool UYggMenuWidget::IsOpen(APlayerController* PC) { return PC && FindOpen(PC) != nullptr; }

void UYggMenuWidget::CloseAll(APlayerController* PC)
{
	if (UYggMenuWidget* Open = PC ? FindOpen(PC) : nullptr) { Open->CloseMenu(); }
}

void UYggMenuWidget::TogglePause(APlayerController* PC)
{
	if (!PC)
	{
		return;
	}
	if (UYggMenuWidget* Open = FindOpen(PC))
	{
		if (Open->bPause) { Open->Resume(); }
		return;
	}
	UYggMenuWidget* W = Make(PC);
	if (!W) { return; }
	W->bPause = true;
	W->HeaderRight = TEXT("Everest  ·  South Col route");
	W->FooterLeft = TEXT("Your climb is kept at every camp");
	W->PausePage();
	W->Open(PC, 70, false);
	UGameplayStatics::SetGamePaused(PC, true);
}

void UYggMenuWidget::PausePage()
{
	APlayerController* PC = GetOwningPlayer();
	const APawn* P = PC ? PC->GetPawn() : nullptr;
	const USurvivalComponent* Body = P ? P->FindComponentByClass<USurvivalComponent>() : nullptr;
	const UExpeditionComponent* Exp = P ? P->FindComponentByClass<UExpeditionComponent>() : nullptr;
	FPage Pg;
	// the title is where you are: the last camp you reached (the start below the first), the facts under it
	Pg.Eyebrow = TEXT("Paused");
	Pg.Title = Exp && Exp->GetLastCamp() ? MenuCamp(Exp->GetLastCamp()->LandmarkName) : FString(TEXT("On the mountain"));
	Pg.TitleSize = 76.f;
	{
		TArray<FString> F;
		if (Body) { F.Add(Height(Body->GetElevationMeters())); }
		if (Exp) { F.Add(Duration(Exp->GetPlaySeconds()) + TEXT(" on the mountain")); F.Add(MenuFalls(Exp->GetDeaths())); }
		Pg.Facts = FString::Join(F, MenuDot);
	}
	// the right-hand panel: the route of camps, the highest at the top, a light where you are
	if (Exp && Body && Exp->GetCamps().Num() > 0)
	{
		Pg.Side.Add(TEXT("#The route"));
		const float FeetZ = P->GetActorLocation().Z - P->GetSimpleCollisionHalfHeight();
		const TArray<TWeakObjectPtr<ARouteLandmark>>& Camps = Exp->GetCamps();
		const int32 Here = Camps.IndexOfByKey(TWeakObjectPtr<ARouteLandmark>(Exp->GetLastCamp()));
		for (int32 i = Camps.Num() - 1; i >= 0; --i)
		{
			const ARouteLandmark* C = Camps[i].Get();
			if (!C) { continue; }
			const float M = Body->GetElevationMeters() + (C->GetActorLocation().Z - FeetZ) / 100.f;
			const int32 State = i == Here ? 2 : (Here >= 0 && i < Here ? 1 : 0);
			// the names carry their height in brackets ("Camp 3 (7160 m)"): ours is measured, show the name alone
			Pg.Side.Add(FString::Printf(TEXT("@%s|%s %s|%d"), *MenuCamp(C->LandmarkName), *YggSettings::AltitudeNumber(FMath::RoundToFloat(M / 10.f) * 10.f), YggSettings::AltitudeUnit(), State));
		}
	}
	Pg.Options = {
		{ TEXT("Resume"), FString(), [this]() { Resume(); } },
		{ TEXT("Settings"), FString(), [this]()
			{
				APlayerController* C = GetOwningPlayer();
				RemoveFromParent();
				USettingsWidget::Open(C, [C]() { UYggMenuWidget::TogglePause(C); });
			} },
		{ TEXT("Start menu"), FString(), [this]()
			{
				Confirm(TEXT("Leave the mountain?"), TEXT("Your climb is kept at the last camp you reached."), TEXT("Leave"), TEXT("Stay"), [this]() { ToStartMenu(); });
			} },
		{ TEXT("Quit"), FString(), [this]()
			{
				Confirm(TEXT("Quit the game?"), TEXT("Your climb is kept at the last camp you reached."), TEXT("Quit"), TEXT("Stay"), [this]() { Quit(); });
			} },
	};
	Pg.Back = [this]() { Resume(); };
	SetPage(Pg);
}

void UYggMenuWidget::Confirm(const FString& InTitle, const FString& InLine, const FString& Yes, const FString& No, TFunction<void()> OnYes)
{
	const FPage Before = Page;
	FPage Pg;
	Pg.Tone = Before.Tone;
	Pg.Title = InTitle;
	Pg.Line = InLine;
	Pg.TitleSize = 56.f;
	Pg.bModal = true;
	Pg.Options = { { Yes, FString(), OnYes }, { No, FString(), [this, Before]() { SetPage(Before); } } };
	Pg.Selected = 1;   // the safe answer first
	Pg.Back = [this, Before]() { SetPage(Before); };
	SetPage(Pg);
}

void UYggMenuWidget::ShowSummit(APlayerController* PC, int32 Seconds, int32 Deaths)
{
	UYggMenuWidget* W = Make(PC);
	if (!W) { return; }
	FPage& Pg = W->SummitSaved;
	Pg.Title = TEXT("The Summit");
	Pg.TitleSize = 104.f;
	Pg.Tone = ETone::Summit;
	Pg.Line = TEXT("You stand beneath the World Tree.");
	Pg.Facts = Height(8849.f) + MenuDot + Duration(Seconds) + TEXT(" on the mountain") + MenuDot + MenuFalls(Deaths);
	Pg.Options = {
		{ TEXT("Stay at the summit"), FString(), [W]() { W->CloseMenu(); } },
		{ TEXT("Credits"), FString(), [W]()
			{
				FPage C = CreditsPage();
				C.Tone = ETone::Summit;
				C.Options = { { TEXT("Back"), FString(), [W]() { W->SetPage(W->SummitSaved); } } };
				C.Back = [W]() { W->SetPage(W->SummitSaved); };
				W->SetPage(C);
			} },
		{ TEXT("Start menu"), FString(), [W]() { W->ToStartMenu(); } },
		{ TEXT("Quit"), FString(), [W]() { W->Quit(); } },
	};
	Pg.Back = [W]() { W->CloseMenu(); };
	W->SetPage(Pg);
	W->Open(PC, 70, false);
}

void UYggMenuWidget::ShowDeath(APlayerController* PC, const FText& Cause, float Meters, int32 Seconds, int32 Deaths, const FString& Camp, TFunction<void()> OnReturn)
{
	UYggMenuWidget* W = Make(PC);
	if (!W) { return; }
	FPage Pg;
	Pg.Title = DeathWord(Cause.ToString());
	Pg.Tone = ETone::Death;
	// each way of dying turns the world its own way (M_PP_Fate) and lights the chosen word in its own colour
	struct FFateLook { const TCHAR* Word; uint8 Fate; FLinearColor Accent; };
	static const FFateLook Looks[] = {
		{ TEXT("Frozen"), YggFate::Frozen, YggUI::Ice },
		{ TEXT("Breathless"), YggFate::Breathless, YggUI::Hex(170, 182, 240) },
		{ TEXT("Fallen"), YggFate::Fallen, YggUI::Hex(150, 196, 230) },
		{ TEXT("Buried"), YggFate::Buried, YggUI::Hex(225, 234, 246) },
		{ TEXT("Parched"), YggFate::Parched, YggUI::Hex(236, 184, 120) },
		{ TEXT("Starved"), YggFate::Starved, YggUI::Hex(200, 196, 188) },
		{ TEXT("Lost"), YggFate::Lost, YggUI::Hex(228, 228, 222) },
	};
	for (const FFateLook& F : Looks) { if (Pg.Title == F.Word) { Pg.Fate = F.Fate; Pg.AccentOverride = F.Accent; } }
	Pg.TitleSize = 120.f;
	Pg.Line = Cause.ToString();
	Pg.Facts = Height(Meters) + MenuDot + Duration(Seconds) + TEXT(" on the mountain") + MenuDot + MenuFalls(Deaths);
	const FString Back = Camp.IsEmpty() ? TEXT("Return to the start") : FString::Printf(TEXT("Return to %s"), *Camp);
	Pg.Options = {
		{ Back, TEXT("Your pack as it was at the camp"), [W, OnReturn]() { W->CloseMenu(); if (OnReturn) { OnReturn(); } } },
		{ TEXT("Start menu"), FString(), [W]() { W->ToStartMenu(); } },
		{ TEXT("Quit"), FString(), [W]() { W->Quit(); } },
	};
	W->bShowHints = true;
	W->SetPage(Pg);
	W->Open(PC, 70, false);
}

void UYggMenuWidget::ShowCredits(APlayerController* PC)
{
	UYggMenuWidget* W = Make(PC);
	if (!W) { return; }
	FPage C = CreditsPage();
	C.Options = { { TEXT("Back"), FString(), [W]() { W->CloseMenu(); } } };
	C.Back = [W]() { W->CloseMenu(); };
	W->SetPage(C);
	W->Open(PC, 70, false);
}

void UYggMenuWidget::Resume()
{
	CloseMenu();
}

void UYggMenuWidget::OnClosed()
{
	if (bPause)
	{
		if (APlayerController* PC = GetOwningPlayer()) { UGameplayStatics::SetGamePaused(PC, false); }
	}
}

void UYggMenuWidget::ToStartMenu()
{
	if (APlayerController* PC = GetOwningPlayer()) { UGameplayStatics::SetGamePaused(PC, false); }
	UGameplayStatics::OpenLevel(this, FName(TEXT("/Game/UI/Menu/L_MainMenu")));
}

void UYggMenuWidget::Quit()
{
	UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, false);
}
