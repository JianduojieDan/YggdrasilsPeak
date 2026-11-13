// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "YggMenuBase.generated.h"

class APostProcessVolume;
class UButton;
class UCanvasPanel;
class UCanvasPanelSlot;
class UFont;
class UHorizontalBox;
class UImage;
class UMaterialInstanceDynamic;
class UTextBlock;
class UVerticalBox;
class UYggUIProxy;

/**
 *  The shared frame of every menu, v3 (2026-11-13, Docs/Development/UI_Redesign_2026-11-13.md): light, not lines.
 *  - Layers: what the subclass puts underneath (the start menu's views), the world blurred (on the mountain), drifting
 *    cloud shadow (M_UI_CloudVeil) darkest where the words stand, the summit's warm light, a darkening under a question.
 *  - One column on the left: an optional quiet line above, the serif title, a quiet line, one line of facts, the options.
 *    Options are words; the chosen one is lit (warm snow-white, a soft sun glow behind it), the others stand in shadow.
 *  - Optional: a side panel on the right (pause: the route of camps), a centred question, a centred title ("press any
 *    key"), rolling credits.
 *  - The start menu (2026-11-14): its title stands on its own at the top-left (FPage::bTitleTop), and after "press any
 *    key" the title glides there from the centre before the options come in one by one (GlideTitleIntoNextPage; any
 *    key or click jumps to the end; ygg.Menu.TitleFreeze holds it for pictures).
 *  - A death page (Fate) turns the live world behind it into what took you: an unbound post-process volume with
 *    M_PP_Fate (frost on the lens, the crevasse closing, the snow rising...), while the title's letters act it out.
 *  Mouse, keyboard (arrows / W S, Enter / Space, Esc) and gamepad (d-pad / stick, A, B) all work.
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
		FString Eyebrow;                            // a quiet line above the title, sentence case (rarely)
		FString Title, Line;
		FString Facts;                              // one line of facts under the quiet line ("6,230 m  ·  2 h 14 m ...")
		FString Context;                            // (v2's header; unused)
		TArray<TPair<FString, FString>> Stats;      // (v2; unused - pages say it in Facts)
		TArray<FString> Body;                       // credits: "#Heading" or a name line
		/** The right-hand panel: "#Heading", "=8,849|m" big numerals, "~line" under them, "@Camp|5,364 m|state"
		 *  (0 ahead, 1 passed, 2 here) for the route, "-" a gap, anything else a serif line. */
		TArray<FString> Side;
		TArray<FOption> Options;
		TFunction<void()> Back;                     // Esc / B; none = nothing
		TFunction<void()> AnyKey;                   // the splash: any key or click
		int32 Selected = 0;
		float TitleSize = 64.f;
		ETone Tone = ETone::Calm;
		bool bNineBeads = false;                    // (v2; unused)
		bool bEmblem = false;                       // (v2; unused)
		bool bModal = false;                        // a centred question with its answers side by side
		bool bSplash = false;                       // centred, no options: "press any key"
		bool bRoll = false;                         // the Body rolls up on the right (credits)
		bool bNumbered = false;                     // (v2; unused)
		bool bTitleTop = false;                     // the title on its own at the top-left, the options lower (start menu)
		FLinearColor AccentOverride = FLinearColor::Transparent;   // alpha 0: the tone decides
		uint8 Fate = 0;                             // YggFate: how the world and the title act out a death
	};

	/** Swaps the page (the column rebuilds and comes in again). */
	void SetPage(const FPage& Page);
	/** One menu at a time: input goes to it, the cursor shows. bUIOnly: no game input at all (start menu). */
	void Open(APlayerController* PC, int32 ZOrder, bool bUIOnly);
	void CloseMenu();
	/** Hides the words (the settings open over the start menu, its views stay). */
	void SetColumnVisible(bool bVisible);
	/** True while it fades out after closing (it is already gone for input and for FindOpen). */
	bool IsClosing() const { return bClosing; }
	/** Hides the HUD while a menu is open (and shows it again). */
	static void SetHudVisible(APlayerController* PC, bool bVisible);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual void NativeDestruct() override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply NativeOnKeyUp(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonDoubleClick(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

	/** Extra layers under the column (the start menu's views); the canvas fills the screen. */
	virtual void BuildBackground(UCanvasPanel* InCanvas) {}
	/** Called when the menu is about to be removed. */
	virtual void OnClosed() {}
	/** The next SetPage - a bTitleTop page replacing the title page - glides the title from where it stands to its place
	 *  at the top-left, then brings the options in one by one. Only that one page change; going back to it later is plain. */
	void GlideTitleIntoNextPage() { bGlideNext = true; }

	float VeilAlpha = 0.8f;                         // how dark the cloud shadow is where the words stand
	float ColumnX = 0.085f;                         // where the column stands, share of the width
	bool bShowHints = true;
	bool bHeader = true;                            // (v2; unused)
	bool bUIOnlyInput = false;
	FString HeaderRight;                            // (v2; unused)
	FString FooterLeft;                             // (v2; unused)
	UPROPERTY(Transient) TObjectPtr<UFont> Serif;
	UPROPERTY(Transient) TObjectPtr<UFont> Hand;
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
	void Activate(int32 Index);
	void AnimateFate();
	void Move(int32 Dir);
	void UpdateFateWorld();
	void RemoveFateWorld();
	FLinearColor Accent() const;
	void TickTitleGlide(const FVector2D& View, float W);
	void FinishTitleGlide();
	bool CanvasTopLeft(const UWidget* W, FVector2D& Out) const;

	UPROPERTY(Transient) TObjectPtr<UVerticalBox> Column;
	UPROPERTY(Transient) TObjectPtr<UCanvasPanelSlot> ColumnSlot;
	UPROPERTY(Transient) TObjectPtr<UVerticalBox> SidePanel;
	UPROPERTY(Transient) TObjectPtr<UVerticalBox> Roll;
	UPROPERTY(Transient) TArray<TObjectPtr<UImage>> RollMasks;
	UPROPERTY(Transient) TObjectPtr<UHorizontalBox> PromptRow;
	UPROPERTY(Transient) TObjectPtr<UHorizontalBox> PadRow;   // the same prompts as gamepad buttons
	UPROPERTY(Transient) TObjectPtr<UImage> DimLayer;
	UPROPERTY(Transient) TObjectPtr<UImage> ToneGlow;
	UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> VeilMID;
	UPROPERTY(Transient) TObjectPtr<class UBackgroundBlur> Blur;
	UPROPERTY(Transient) TObjectPtr<class UOverlay> TitleBox;
	UPROPERTY(Transient) TArray<TObjectPtr<UTextBlock>> TitleLetters;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> SplashPrompt;
	UPROPERTY(Transient) TArray<TObjectPtr<UButton>> OptButtons;
	UPROPERTY(Transient) TArray<TObjectPtr<UTextBlock>> OptTexts;
	UPROPERTY(Transient) TArray<TObjectPtr<UTextBlock>> OptSubs;
	UPROPERTY(Transient) TArray<TObjectPtr<UImage>> OptGlows;
	UPROPERTY(Transient) TArray<TObjectPtr<UYggUIProxy>> Proxies;
	UPROPERTY(Transient) TArray<TObjectPtr<UWidget>> SideItems;
	UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> FateMID;
	TWeakObjectPtr<APostProcessVolume> FateVolume;
	TArray<float> OptHot;                           // 0..1, eased
	int32 Selected = 0;
	float RollY = 0.f;
	bool bRollStarted = false;
	bool bPad = false;                              // the last input came from a gamepad
	float DimNow = 0.f;
	bool bMouseMoved = false;
	bool bBuilt = false;
	bool bClosing = false;
	float CloseT = 0.f;

	// the start menu's title at the top-left, and its glide there from the title page
	UPROPERTY(Transient) TObjectPtr<UVerticalBox> TopBlock;           // a bTitleTop page's title and quiet line
	UPROPERTY(Transient) TObjectPtr<UCanvasPanelSlot> TopBlockSlot;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> TitleText;            // the page's title (none for a death's letters)
	UPROPERTY(Transient) TObjectPtr<UTextBlock> LineText;             // the quiet line under it
	UPROPERTY(Transient) TObjectPtr<UTextBlock> GlideTitle;           // the title page's title on its way to the corner
	UPROPERTY(Transient) TArray<TObjectPtr<UWidget>> GlideGhosts;     // the title page's other words, fading where they stood
	TArray<float> GlideGhostAlpha;
	FVector2D GlideFrom = FVector2D::ZeroVector;    // where the gliding title began (canvas units, its top-left)
	float GlideT = -1.f;                            // seconds since the key that started the glide; < 0: no glide
	float GlideFromSize = 0.f;
	float GlideFromAlpha = 1.f;
	float GlideVeilFrom = 0.f;
	float TopColumnY = 0.f;                         // the options' top on a bTitleTop page (canvas units)
	bool bGlideNext = false;
	FKey GlideKey;                                  // the key that started or skipped the glide, while it is held
	float ActivateAfter = 0.f;                      // Age before which no option acts (just after the glide was skipped)
};
