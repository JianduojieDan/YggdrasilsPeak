// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Survival/YggMenuBase.h"
#include "YggMenuWidget.generated.h"

/**
 *  The menus on the mountain (2026-11-12 redesign; the frame and its look are UYggMenuBase):
 *  - Pause (Esc in a packaged game, P anywhere, Start on a pad): where you are and how long you have climbed;
 *    Resume / Settings / Start menu / Quit (the last two ask first).
 *  - Death: what took you, the height and the time; Return to the last camp / Start menu / Quit.
 *  - Summit (reaching the summit camp by the World Tree): the climb's numbers; Stay / Credits / Start menu / Quit.
 *  - Credits (from the summit and the start menu): GetCredits().
 *  Console: ygg.Menu pause|summit|death|credits (for checking).
 */
UCLASS()
class YGGDRASILSPEAK_API UYggMenuWidget : public UYggMenuBase
{
	GENERATED_BODY()

public:
	static void TogglePause(APlayerController* PC);
	static void ShowSummit(APlayerController* PC, int32 Seconds, int32 Deaths);
	/** The death screen; OnReturn brings the climber back to the last camp. */
	static void ShowDeath(APlayerController* PC, const FText& Cause, float Meters, int32 Seconds, int32 Deaths, const FString& Camp, TFunction<void()> OnReturn);
	static void ShowCredits(APlayerController* PC);
	static bool IsOpen(APlayerController* PC);
	/** Closes any open menu (one menu at a time). */
	static void CloseAll(APlayerController* PC);
	/** The credits page body ("#Heading" lines and name lines), shared with the start menu. */
	static TArray<FString> GetCredits();
	/** The rolling credits page (shared with the start menu). */
	static FPage CreditsPage();
	/** "Leave the mountain?" and other questions: Yes runs OnYes, No returns to the page before. */
	void Confirm(const FString& Title, const FString& Line, const FString& Yes, const FString& No, TFunction<void()> OnYes);

protected:
	virtual void OnClosed() override;

private:
	static UYggMenuWidget* Make(APlayerController* PC);
	void PausePage();
	void SummitPage();
	void Resume();
	void ToStartMenu();
	void Quit();
	bool bPause = false;
	FPage SummitSaved;
};
