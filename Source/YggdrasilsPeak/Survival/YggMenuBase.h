// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "YggMenuBase.generated.h"

class UButton;
class UCanvasPanel;
class UCanvasPanelSlot;
class UFont;
class UHorizontalBox;
class UImage;
class UTextBlock;
class UVerticalBox;
class UYggUIProxy;

/**
 *  The shared frame of every menu, in the HUD's language (YggUIStyle.h). v2, 2026-11-13 ("industrial level"):
 *  - Layers: what the subclass puts underneath (the start menu's mountain), the veil (dark on the left where the words
 *    stand, thinning to the right), a vignette, the tone's light (warm for the summit, a cold dark for a death), the
 *    frame of hairlines (header: the game's name / where you are; footer: a quiet line, key caps), and a film grain.
 *  - One column, left: eyebrow, emblem (optional), the serif title, a hairline ending in a bead (or nine for the nine
 *    realms), a serif line, light numerals with small units, then numbered options. The chosen option sits in a band
 *    of soft light with an accent bar, lit and moved in a little.
 *  - Optional: a side panel on the right (pause: altitude, the route of camps, time), a centred dialog (confirm),
 *    a centred splash (the start menu's "press any key"), rolling credits.
 *  Mouse, keyboard (arrows / W S, Enter / Space, Esc) and gamepad (d-pad / stick, A, B) all work.
 *  Pages: a widget can swap its content (SetPage) - the column fades and slides in again each time.
 */
UCLASS(Abstract)
class YGGDRASILSPEAK_API UYggMenuBase : public UUserWidget
{
	GENERATED_BODY()

public:
	enum class ETone : uint8 { Calm, Death, Summit };
	struct FOption
	{
		FString Label;
		FString Sub;
		TFunction<void()> Do;
	};
	struct FPage
	{
		FString Eyebrow, Title, Line;
		FString Context;                            // the header's "YGGDRASIL'S PEAK / <Context>"
		TArray<TPair<FString, FString>> Stats;      // "8,849|m" -> numerals + unit, label
		TArray<FString> Body;                       // credits: "#Heading" or a name line
		/** The right-hand panel: "#HEADING", "=8,849|m" big numerals, "~label" under them, "@Camp|5,364 m|state"
		 *  (0 ahead, 1 passed, 2 here) for the route, "-" a hairline, anything else a serif line. */
		TArray<FString> Side;
		TArray<FOption> Options;
		TFunction<void()> Back;                     // Esc / B; none = nothing
		TFunction<void()> AnyKey;                   // the splash: any key or click
		int32 Selected = 0;
		float TitleSize = 54.f;
		ETone Tone = ETone::Calm;
		bool bNineBeads = false;                    // the start menu: nine beads for the nine realms under the title
		bool bEmblem = false;                       // the World Tree emblem over the title
		bool bModal = false;                        // a centred question with its answers side by side
		bool bSplash = false;                       // centred, no options: "press any key"
		bool bRoll = false;                         // the Body rolls up on the right (credits)
		bool bNumbered = true;                      // 01 02 03 before the options
	};

	/** Swaps the page (the column rebuilds and comes in again). */
	void SetPage(const FPage& Page);
	/** One menu at a time: input goes to it, the cursor shows. bUIOnly: no game input at all (start menu). */
	void Open(APlayerController* PC, int32 ZOrder, bool bUIOnly);
	void CloseMenu();
	/** Hides the words (the settings open over the start menu, its mountain stays). */
	void SetColumnVisible(bool bVisible);
	/** True while it fades out after closing (it is already gone for input and for FindOpen). */
	bool IsClosing() const { return bClosing; }
	/** Hides the HUD while a menu is open (and shows it again). */
	static void SetHudVisible(APlayerController* PC, bool bVisible);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnFocusLost(const FFocusEvent& InFocusEvent) override;

	/** Extra layers under the column (the start menu's mountain); the canvas fills the screen. */
	virtual void BuildBackground(UCanvasPanel* InCanvas) {}
	/** Called when the menu is about to be removed. */
	virtual void OnClosed() {}

	float VeilAlpha = 1.f;                          // how strong the veil is (the start menu's backdrop wants less)
	float ColumnX = 0.085f;                         // where the column stands, share of the width
	bool bShowHints = true;
	bool bHeader = true;                            // the header line (the start menu has the title instead)
	bool bUIOnlyInput = false;
	FString HeaderRight;                            // top right: where you are
	FString FooterLeft;                             // bottom left: a quiet line
	UPROPERTY(Transient) TObjectPtr<UFont> Serif;
	UPROPERTY(Transient) TObjectPtr<UCanvasPanel> Canvas;
	FPage Page;
	float Age = 0.f;                                // since the widget opened
	float Words = 1.f, WordsTarget = 1.f;           // the column's own fade (settings over the start menu)
	UPROPERTY(Transient) TArray<TObjectPtr<UWidget>> FadeWithWords;   // other words that fade with the column
	float PageAge = 0.f;                            // since the page changed

private:
	void BuildFrame();
	void BuildColumn();
	void BuildSide();
	void BuildRoll();
	void BuildPrompts();
	UWidget* Numbers(const FString& Key, int32 Big, int32 Small, bool bRight);
	void Activate(int32 Index);
	void Move(int32 Dir);
	FLinearColor Accent() const;

	UPROPERTY(Transient) TObjectPtr<UVerticalBox> Column;
	UPROPERTY(Transient) TObjectPtr<UCanvasPanelSlot> ColumnSlot;
	UPROPERTY(Transient) TObjectPtr<UVerticalBox> SidePanel;
	UPROPERTY(Transient) TObjectPtr<UVerticalBox> Roll;
	UPROPERTY(Transient) TArray<TObjectPtr<UImage>> RollMasks;
	UPROPERTY(Transient) TObjectPtr<UHorizontalBox> PromptRow;
	UPROPERTY(Transient) TObjectPtr<UHorizontalBox> PadRow;   // the same prompts as gamepad buttons
	UPROPERTY(Transient) TObjectPtr<UImage> DimLayer;
	UPROPERTY(Transient) TObjectPtr<UImage> ToneGlow;
	UPROPERTY(Transient) TObjectPtr<UImage> RightVeil;
	UPROPERTY(Transient) TObjectPtr<class UBackgroundBlur> Blur;
	UPROPERTY(Transient) TObjectPtr<UImage> GrainImg;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> CrumbText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> SplashPrompt;
	UPROPERTY(Transient) TArray<TObjectPtr<UButton>> OptButtons;
	UPROPERTY(Transient) TArray<TObjectPtr<UTextBlock>> OptTexts;
	UPROPERTY(Transient) TArray<TObjectPtr<UTextBlock>> OptSubs;
	UPROPERTY(Transient) TArray<TObjectPtr<UTextBlock>> OptNums;
	UPROPERTY(Transient) TArray<TObjectPtr<UImage>> OptBands;
	UPROPERTY(Transient) TArray<TObjectPtr<UImage>> OptBars;
	UPROPERTY(Transient) TArray<TObjectPtr<UYggUIProxy>> Proxies;
	UPROPERTY(Transient) TArray<TObjectPtr<UImage>> RealmBeads;
	UPROPERTY(Transient) TArray<TObjectPtr<UWidget>> SideItems;
	TArray<float> OptHot;                           // 0..1, eased
	int32 Selected = 0;
	float RollY = 0.f;
	bool bRollStarted = false;
	bool bPad = false;                              // the last input came from a gamepad
	float GrainClock = 0.f;
	float DimNow = 0.f;
	bool bMouseMoved = false;
	bool bBuilt = false;
	bool bClosing = false;
	float CloseT = 0.f;
};
