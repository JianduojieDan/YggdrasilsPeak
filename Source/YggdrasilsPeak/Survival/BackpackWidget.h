// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BackpackWidget.generated.h"

class UBackgroundBlur;
class UCanvasPanel;
class UFont;
class UImage;
class UTextBlock;
class UTexture2D;
class UBackpackWidget;

/** One thing in (or on) the pack: a sketch the player can point at and click. */
UCLASS()
class YGGDRASILSPEAK_API UBackpackItemWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	void Setup(UBackpackWidget* InOwner, FName InItem, UTexture2D* Texture, float Size);

	FName Item;
	float Size = 100.f;
	float BaseAngle = 0.f;
	FVector2D Home = FVector2D::ZeroVector;
	/** Seconds after opening when it drops into the pack. */
	float Delay = 0.f;
	bool bHovered = false;
	float HoverAlpha = 0.f;

	UPROPERTY()
	TObjectPtr<UImage> Image;

protected:

	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeOnMouseEnter(const FGeometry& Geometry, const FPointerEvent& Event) override;
	virtual void NativeOnMouseLeave(const FPointerEvent& Event) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& Geometry, const FPointerEvent& Event) override;

private:

	TWeakObjectPtr<UBackpackWidget> Owner;
};

/**
 *  The pack screen (Tab): the world blurs and darkens, the expedition pack rises and its front panel folds open,
 *  and what you carry drops into it as field-journal sketches cut out of paper (Tools/UI: the items are drawn from
 *  the game's own meshes). Point at a thing to read its page on the right; click to eat, drink or open an oxygen
 *  bottle. The porter's spring scale on the left shows the load against the comfortable and maximum weight.
 *  The world keeps running while the pack is open. Built in code; art in /Game/UI/Backpack, fonts in /Game/UI/Fonts.
 */
UCLASS()
class YGGDRASILSPEAK_API UBackpackWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	/** Opens (adds to the viewport) or closes the pack for the owning player. */
	static void Toggle(APlayerController* PC);

	bool IsClosing() const { return bClosing; }

	void Select(FName Item);
	void HoverEnd(FName Item);
	void UseItem(FName Item);

protected:

	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& Geometry, float DeltaTime) override;
	virtual FReply NativeOnKeyDown(const FGeometry& Geometry, const FKeyEvent& Event) override;

private:

	void Build();
	void Close();
	/** Refills the compartment from the inventory (after using something, or when it changes). */
	void RefreshItems(bool bAnimate);
	void RefreshCard();
	void RefreshScale();
	UFUNCTION()
	void HandleInventoryChanged();

	UTexture2D* Tex(const TCHAR* Name) const;
	UTextBlock* MakeText(UFont* Font, int32 Size, const FLinearColor& Colour, float WrapAt = 0.f);
	UImage* AddImage(UTexture2D* Texture, const FVector2D& Centre, const FVector2D& Size);
	void SetLooseInput(bool bPackOpen);

	UPROPERTY()
	TObjectPtr<UCanvasPanel> Canvas;
	UPROPERTY()
	TObjectPtr<UBackgroundBlur> Blur;
	UPROPERTY()
	TObjectPtr<UImage> Vignette;
	UPROPERTY()
	TObjectPtr<UImage> Pack;
	UPROPERTY()
	TObjectPtr<UImage> Pocket;
	UPROPERTY()
	TArray<TObjectPtr<UTexture2D>> PackFrames;
	UPROPERTY()
	TArray<TObjectPtr<UBackpackItemWidget>> Items;
	UPROPERTY()
	TArray<TObjectPtr<UBackpackItemWidget>> Hotspots;

	UPROPERTY()
	TObjectPtr<UImage> Card;
	UPROPERTY()
	TObjectPtr<UImage> CardIcon;
	UPROPERTY()
	TObjectPtr<UTextBlock> CardTitle;
	UPROPERTY()
	TObjectPtr<UTextBlock> CardCount;
	UPROPERTY()
	TObjectPtr<UTextBlock> CardBody;
	UPROPERTY()
	TObjectPtr<UTextBlock> CardStats;
	UPROPERTY()
	TObjectPtr<UTextBlock> CardAction;

	UPROPERTY()
	TObjectPtr<UImage> Scale;
	UPROPERTY()
	TObjectPtr<UImage> ScaleMark;
	UPROPERTY()
	TObjectPtr<UTextBlock> ScaleText;
	UPROPERTY()
	TObjectPtr<UTextBlock> ScaleStatus;

	UPROPERTY()
	TObjectPtr<UTextBlock> Header;
	UPROPERTY()
	TObjectPtr<UTextBlock> SubHeader;
	UPROPERTY()
	TObjectPtr<UTextBlock> Footer;

	UPROPERTY()
	TObjectPtr<UFont> HandFont;
	UPROPERTY()
	TObjectPtr<UFont> BookFont;

	FName Selected;
	float OpenTime = 0.f;
	float CloseTime = 0.f;
	bool bClosing = false;
	float ShownLoadKg = 0.f;
	float ScaleVelocity = 0.f;
};
