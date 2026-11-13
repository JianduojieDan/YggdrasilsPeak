// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Styling/SlateBrush.h"
#include "BackpackWidget.generated.h"

class UBackgroundBlur;
class UCanvasPanel;
class UImage;
class UTexture2D;

/**
 *  The pack screen (Tab), v5 (2026-11-14, the user's pick "pack_hud_2" of Tools/UI/pack_hud_mockups.py): an ordinary
 *  inventory drawn in the HUD's own language (HUD v8) - Roboto, light numerals, small tracked caps labels, white with a
 *  tight shadow, fine underline bars, line icons, red only for danger, no boxes or frames - over the blurred, dimmed
 *  game. The list on the left (grouped Food and water / Gear / Notes), the chosen thing's page on the right with the
 *  pack's weight above it. Hover or W/S chooses, click or Enter eats, drinks or opens an oxygen bottle.
 *  The world keeps running while the pack is open. The blur and the dark are widgets; everything else is painted
 *  (NativePaint), laid out on the mockup's 1920 x 1080 and scaled to the screen.
 */
UCLASS()
class YGGDRASILSPEAK_API UBackpackWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	/** Opens (adds to the viewport) or closes the pack for the owning player. */
	static void Toggle(APlayerController* PC);
	/** Closes the pack if it is open (a menu opening above it: death, pause, summit), 2026-11-14 review. */
	static void CloseIfOpen(APlayerController* PC);

	bool IsClosing() const { return bClosing; }

	void Select(FName Item);
	void UseItem(FName Item);

protected:

	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& Geometry, float DeltaTime) override;
	virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
	virtual FReply NativeOnKeyDown(const FGeometry& Geometry, const FKeyEvent& Event) override;
	virtual FReply NativeOnMouseMove(const FGeometry& Geometry, const FPointerEvent& Event) override;
	virtual void NativeOnMouseLeave(const FPointerEvent& Event) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& Geometry, const FPointerEvent& Event) override;
	virtual FReply NativeOnMouseButtonDoubleClick(const FGeometry& Geometry, const FPointerEvent& Event) override;

private:

	void Build();
	void Close();
	/** Re-reads the counts and weights from the inventory (after using something, when it changes, once a second);
	 *  bPages also counts the journal's pages found (an actor walk: only on open and when the pack changes). */
	void RefreshRows(bool bPages);
	/** Re-writes the chosen thing's page. */
	void RefreshPage();
	/** Takes the pictures and icons that have streamed in (they never block a frame). */
	void TakeArt();
	/** The list row under a point of this widget, or -1. */
	int32 RowAt(const FGeometry& Geometry, const FVector2f& ScreenPos) const;
	UFUNCTION()
	void HandleInventoryChanged();
	UFUNCTION()
	void HandleExpeditionMessage(const FText& Message, float Seconds);
	void SetLooseInput(bool bPackOpen);

	UPROPERTY()
	TObjectPtr<UCanvasPanel> Canvas;
	UPROPERTY()
	TObjectPtr<UBackgroundBlur> Blur;
	UPROPERTY()
	TObjectPtr<UImage> Dim;
	/** The streamed-in textures the brushes below draw, held while the pack is open. */
	UPROPERTY()
	TArray<TObjectPtr<UTexture2D>> HeldArt;

	/** One line of the list, as painted. */
	struct FPackRow
	{
		FName Item;
		int32 Kit = 0;           // which line icon
		int32 Group = 0;
		bool bFirstOfGroup = false;
		float Top = 0.f;         // in the mockup's pixels (1080 lines)
		FString Name;
		FString Count;           // "6", "0 / 3", "2 pages"; empty when not counted
		FString Weight;          // "0.6 kg"
		bool bDanger = false;    // none left: the count turns red
	};
	TArray<FPackRow> Rows;
	/** How lit each row is (eases towards the chosen one). */
	TArray<float> RowLight;
	TArray<FString> JournalPages;

	/** The chosen thing's page, as painted. */
	struct FPackPage
	{
		FString Group, Title;
		int32 Effect = -1;       // index of its HUD icon (food, drop, lungs), -1 none
		FString EffectValue;     // "+40", "30 min"
		FString Count, CountUnit;
		bool bDanger = false;
		FString Weight;          // "1.0 kg each"
		FString Body;
		FString Danger, DangerHint;
		FString Action, ActionHint;
	} Page;
	int32 PageRow = -1;
	int32 BodySerial = 0;

	/** The description wrapped to the page's width (made in the paint when the text or the scale changed). */
	mutable TArray<FString> BodyLines;
	mutable int32 BodyLinesSerial = -1;
	mutable float BodyLinesScale = -1.f;

	/** Pictures (one per row) and effect icons; sized once, never resized (the paint keeps pointers to them). */
	TArray<FSlateBrush> Pictures;
	TArray<FSlateBrush> EffectIcons;
	bool bArtReady = false;

	FString KeysLine;
	FString PackLine, PackMaxLine, PackNote;
	FString MessageText;
	float MessageTime = 0.f;

	FName Selected;
	/** The step key last pressed while the pack is open: only its repeats step on (see NativeOnKeyDown). */
	FKey HeldStepKey;
	int32 HoverRow = -1;
	float OpenTime = 0.f;
	float CloseTime = 0.f;
	float PageAge = 0.f;     // since the page last changed (it comes in again)
	float ContentAlpha = 0.f;
	float LastBlur = -1.f;
	float RefreshClock = 0.f;
	bool bClosing = false;
	float ShownLoadKg = 0.f;
	float ScaleVelocity = 0.f;
	float LoadKg = 0.f;
	float MaxKg = 26.f;
	float ComfortKg = 16.f;
};
