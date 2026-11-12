// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SettingsWidget.generated.h"

class UButton;
class UCanvasPanel;
class UFont;
class UImage;
class USlider;
class UTextBlock;
class UVerticalBox;
class UWidgetSwitcher;
class UYggUIProxy;

/**
 *  Settings (F2 on the mountain, from the pause menu and the start menu; console ygg.Settings), in the menus' language
 *  (UYggMenuBase / YggUIStyle). v2, 2026-11-13: the same frame of hairlines, grain and key caps as the menus; six tabs
 *  - GAME, GRAPHICS, DISPLAY, SOUND, CONTROLS, ACCESSIBILITY - with a gold underline and Q / E key caps; rows of a
 *  fixed width (label left, control right, a quiet rule under each): choices are selectors (‹ value ›, one pip per
 *  choice), sliders a hairline with a gold fill and light numerals, key bindings a key cap (click, then press a key).
 *  The chosen row sits in the band of soft light. What it does stands in a fixed panel on the right (title, the line,
 *  every choice with the current one marked) - it never moves the rows (to-do #62).
 *  Every value lives in YggSettings (saved in GameUserSettings.ini) and applies at once.
 *  Keys: Up/Down rows, Left/Right change, Q/E or the bumpers change tab, Enter rebinds a key, Esc goes back.
 */
UCLASS()
class YGGDRASILSPEAK_API USettingsWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** F2: opens on the mountain, or closes. */
	static void Toggle(APlayerController* PC);
	/** Opens; OnClosed runs when it closes (the pause menu comes back). bUIOnly for the start menu. */
	static void Open(APlayerController* PC, TFunction<void()> OnClosed, bool bUIOnly = false);
	static void CloseIfOpen(APlayerController* PC);
	/** Shows a tab (0 game .. 5 accessibility). */
	void ShowTab(int32 Tab) { if (Tabs.Num() > 0) { SetTab(Tab); } else { PendingTab = Tab; } }
	/** Old entry point, kept for callers: applies every saved setting. */
	static void ApplySaved(UWorld* World);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

private:
	enum class EKind : uint8 { Choice, Slider, Cycle, Key };
	struct FRow
	{
		FString Label, Hint;
		EKind Kind = EKind::Choice;
		TArray<FString> Choices;
		TFunction<int32()> Get;
		TFunction<void(int32)> Set;
		float Min = 0.f, Max = 1.f, Step = 0.05f;
		TFunction<float()> GetF;
		TFunction<void(float)> SetF;
		TFunction<FString(float)> Fmt;
		FName Action;
		// built widgets
		TArray<UImage*> Pips;
		USlider* SliderW = nullptr;
		UImage* Fill = nullptr;
		UTextBlock* Value = nullptr;
		UTextBlock* LabelW = nullptr;
		UImage* Band = nullptr;
		UImage* Bar = nullptr;
		UWidget* Hit = nullptr;
		float Hot = 0.f;
	};
	struct FTab { FString Name; TArray<FRow> Rows; };

	void Build();
	void DefineTabs();
	void BuildTabPage(int32 Tab, UVerticalBox* Box);
	void SetTab(int32 Tab);
	void Change(int32 Dir);
	void Close();
	void Refresh();
	void ShowDescription();
	UButton* TextButton(const FString& Label, int32 Size, UTextBlock*& OutText, TFunction<void()> OnClick);
	UButton* Clickable(UWidget* Content, TFunction<void()> OnClick);

	TArray<FTab> Tabs;
	int32 TabIndex = 0;
	int32 PendingTab = 0;
	int32 RowIndex = 0;
	int32 Waiting = -1;                 // row waiting for a key (rebinding)
	int32 DescTab = -1, DescRow = -1;   // what the description panel shows
	float Age = 0.f, TabAge = 0.f, DescAge = 0.f, GrainClock = 0.f;
	bool bMouseMoved = false;
	bool bUIOnly = false;
	bool bDescDirty = true;
	bool bPad = false;
	TFunction<void()> OnClosedFn;
	TArray<TWeakObjectPtr<class UYggMenuBase>> Hidden;   // menus whose words step aside while the settings are open
	TArray<float> TabHot;

	UPROPERTY(Transient) TObjectPtr<UFont> Serif;
	UPROPERTY(Transient) TObjectPtr<UCanvasPanel> Canvas;
	UPROPERTY(Transient) TObjectPtr<UWidgetSwitcher> Pages;
	UPROPERTY(Transient) TObjectPtr<UVerticalBox> Desc;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> DescTitle;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> DescText;
	UPROPERTY(Transient) TObjectPtr<UVerticalBox> DescChoices;
	UPROPERTY(Transient) TObjectPtr<UImage> GrainImg;
	UPROPERTY(Transient) TObjectPtr<UWidget> KeysRow;
	UPROPERTY(Transient) TObjectPtr<UWidget> PadKeysRow;
	UPROPERTY(Transient) TObjectPtr<class UBackgroundBlur> Blur;
	UPROPERTY(Transient) TArray<TObjectPtr<UTextBlock>> TabTexts;
	UPROPERTY(Transient) TArray<TObjectPtr<UImage>> TabLines;
	UPROPERTY(Transient) TArray<TObjectPtr<UYggUIProxy>> Proxies;
	UPROPERTY(Transient) TArray<TObjectPtr<UWidget>> Keep;   // keeps every built widget referenced by the rows alive
	UPROPERTY(Transient) TObjectPtr<UTextBlock> ResetText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> BackText;
	UPROPERTY(Transient) TObjectPtr<UButton> ResetBtn;
	UPROPERTY(Transient) TObjectPtr<UButton> BackBtn;
};
