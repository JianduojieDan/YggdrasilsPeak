// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InputCoreTypes.h"
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
 *  (UYggMenuBase / YggUIStyle). v3, 2026-11-13: light, not lines - six tabs as serif words between Q / E key caps (the
 *  open one lit); rows of a fixed width, label left in the serif, control right: choices are ‹ value ›, sliders a soft
 *  line with a sunlit fill, key bindings a key cap (click, then press a key). The chosen row is lit like a chosen
 *  option (warm words, a soft glow behind). What it does stands in a fixed panel on the right (the line, every choice
 *  with the current one lit) - it never moves the rows (to-do #62).
 *  Every value lives in YggSettings (saved in GameUserSettings.ini). 2026-11-21: a change edits a PENDING copy only (the row
 *  shows a sun bead and warm words); Apply (button, F, gamepad Y) asks "Apply these changes?" and applies all at once on Yes;
 *  leaving with pending changes asks "Discard changes?"; after Yes, changes that touch the screen (resolution, window,
 *  quality, upscaling, frame generation, VSync, frame limit) ask "Keep these settings?" and revert after 15 s or on Esc.
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
	/** ygg.Settings.Test: opens the menu and steps every row one notch and back through the menu's own change path,
	 *  logging (SETTINGSTEST) what changed in the engine for each and the frame time; rows that change nothing are flagged. */
	static void SelfTest(UWorld* World);
	/** ygg.Settings.Test.Pending (2026-11-21): the pending / apply / discard / keep-or-revert flow (SETTINGSTEST2 PASS/FAIL lines). */
	static void SelfTestPending(UWorld* World);

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
		// pending copy (2026-11-21): nothing is applied until Apply -> Yes
		bool bDirty = false;
		int32 PendI = 0;
		float PendF = 0.f;
		FKey PendKey;
		bool bScreen = false;               // a change the player must be able to undo if the screen goes wrong
		bool bHasDef = false;
		int32 DefI = 0;
		float DefF = 0.f;
		float MarkA = 0.f;
		UWidget* Mark = nullptr;
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
	// ---- pending changes, apply / discard / keep-or-revert (2026-11-21)
	enum class EModal : uint8 { None, Apply, Discard, Keep };
	struct FRevert { int32 Tab = 0, Row = 0, I = 0; float F = 0.f; };
	int32 EffInt(const FRow& R) const;
	float EffFloat(const FRow& R) const;
	FKey EffKey(const FRow& R) const;
	void SetPendingInt(FRow& R, int32 V);
	void SetPendingFloat(FRow& R, float V);
	void SetPendingKey(FRow& R, const FKey& NewKey);
	int32 PendingCount() const;
	FString RowValueText(const FRow& R, int32 I, float F, const FKey& K) const;
	void ResetPending(int32 Tab);
	void DiscardPending();
	void RequestClose();
	void RequestApply();
	void CommitPending(bool bAskKeep);
	void RevertKept();
	void BuildModal();
	void ShowModal(EModal M);
	void HideModal();
	void ModalAnswer(int32 Index);
	bool FindRowByLabel(const TCHAR* Label, int32& OutTab, int32& OutRow) const;
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
	EModal Modal = EModal::None;
	int32 ModalSel = 0;
	float ModalAge = 0.f;
	float ModalHot[2] = { 0.f, 0.f };
	double KeepEnd = 0.0;
	float KeepSeconds = 15.f;           // the self-test shortens it
	TArray<FRevert> Kept;               // old values of the screen rows just applied (what Revert puts back)
	int32 LastPendingCount = -1;

	UPROPERTY(Transient) TObjectPtr<UFont> Serif;
	UPROPERTY(Transient) TObjectPtr<UCanvasPanel> Canvas;
	UPROPERTY(Transient) TObjectPtr<UWidgetSwitcher> Pages;
	UPROPERTY(Transient) TObjectPtr<UVerticalBox> Desc;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> DescTitle;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> DescText;
	UPROPERTY(Transient) TObjectPtr<UVerticalBox> DescChoices;
	UPROPERTY(Transient) TObjectPtr<UImage> GrainImg;   // (v2; unused)
	UPROPERTY(Transient) TObjectPtr<class UMaterialInstanceDynamic> VeilMID;
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
	UPROPERTY(Transient) TObjectPtr<UTextBlock> ApplyText;
	UPROPERTY(Transient) TObjectPtr<UButton> ApplyBtn;
	UPROPERTY(Transient) TObjectPtr<class UOverlay> ModalRoot;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> ModalTitle;
	UPROPERTY(Transient) TObjectPtr<UVerticalBox> ModalBody;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> ModalCount;
	UPROPERTY(Transient) TArray<TObjectPtr<UTextBlock>> ModalOptTexts;
	UPROPERTY(Transient) TArray<TObjectPtr<UImage>> ModalOptGlows;
};

/**
 *  The comfort dot (settings "Comfort dot", camera spec §5.11, 2026-11-19): a small steady anchor in the centre of the
 *  screen for players whom moving views make unwell. Painted only (no widget tree, no ticking), over the HUD and under
 *  the pack (50), the story overlay (60: the eyelids and the black cover it) and the menus (80); hidden while a menu
 *  shows the mouse cursor. Added and removed by YggSettings::ApplyView, so it follows the setting at once.
 */
UCLASS()
class YGGDRASILSPEAK_API UYggComfortDot : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Shows the dot for the world's first player (made again after a level change), or removes it. */
	static void Apply(UWorld* World, bool bOn);
	/** Whether a dot is on screen now (for the settings self-test and the session monitor's settings list). */
	static bool IsShown();

protected:
	virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
};
