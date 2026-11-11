// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "YggMenuWidget.generated.h"

class UButton;
class UFont;
class UImage;
class UTextBlock;
class UVerticalBox;

/**
 *  The in-game menus after the start menu, in the HUD's language (YggUIStyle.h: no boxes, a soft veil, serif title,
 *  hairline with a bead, letter-spaced small capitals, light numerals with small units, text options marked by a bead):
 *  - Pause (Esc in a packaged game, P anywhere; Esc in the editor stops Play): Resume / Settings / Start menu / Quit.
 *  - Summit (reaching the summit camp by the World Tree, the end of the climb): altitude, time and falls, then
 *    Stay at the summit / Start menu / Quit.
 */
UCLASS()
class YGGDRASILSPEAK_API UYggMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	enum class EAction : uint8 { Resume, Settings, StartMenu, Quit };

	static void TogglePause(APlayerController* PC);
	static void ShowSummit(APlayerController* PC, int32 Seconds, int32 Deaths);
	static bool IsOpen(APlayerController* PC);
	/** Closes any open menu (one menu at a time: settings over the summit screen overlapped). */
	static void CloseAll(APlayerController* PC);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	void Build();
	void Do(EAction Action);
	void Close();

	UFUNCTION() void OnB0();
	UFUNCTION() void OnB1();
	UFUNCTION() void OnB2();
	UFUNCTION() void OnB3();

	FString Eyebrow, Title, SubLine;
	TArray<TPair<FString, FString>> Stats;      // value+unit, label
	TArray<TPair<FString, EAction>> Buttons;
	bool bPause = false;
	float FadeIn = 0.f;
	UPROPERTY(Transient) TObjectPtr<UFont> Serif;
	UPROPERTY(Transient) TArray<TObjectPtr<UButton>> OptButtons;
	UPROPERTY(Transient) TArray<TObjectPtr<UTextBlock>> OptTexts;
	UPROPERTY(Transient) TArray<TObjectPtr<UImage>> OptBeads;
};
