// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Survival/YggMenuBase.h"
#include "YggStartMenuWidget.generated.h"

class UImage;
class UTexture2D;

/**
 *  The start menu (2026-11-12 redesign, replacing the old WBP_StartMenu built before the HUD existed).
 *  Our own mountain fills the screen - views captured in the game (/Game/UI/StartMenu/Backdrops/T_Backdrop_NN),
 *  each drifting slowly and dissolving into the next - with snow falling across it. On the left, in the menus'
 *  language: the title, nine beads for the nine realms with a wandering gold light, the question of the story,
 *  then Continue (the last camp, time and falls) / New expedition / Settings / Credits / Quit.
 *  Starting fades to black, then the loading screen (UYggLoadingScreen) carries you to the mountain.
 */
UCLASS()
class YGGDRASILSPEAK_API UYggStartMenuWidget : public UYggMenuBase
{
	GENERATED_BODY()

public:
	static UYggStartMenuWidget* Show(APlayerController* PC);

protected:
	virtual void BuildBackground(UCanvasPanel* InCanvas) override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

private:
	void SplashPage();
	void MainPage();
	void Begin(bool bNew);
	void PlaceBackdrop(UImage* Img, UTexture2D* Tex, float Zoom, const FVector2D& Pan, const FVector2D& View);

	UPROPERTY(Transient) TArray<TObjectPtr<UTexture2D>> Backdrops;
	UPROPERTY(Transient) TObjectPtr<UImage> BackA;
	UPROPERTY(Transient) TObjectPtr<UImage> BackB;
	UPROPERTY(Transient) TObjectPtr<UImage> Black;
	int32 Current = 0;
	float Slide = 0.f;                  // seconds into the current view
	float Leaving = -1.f;               // >= 0 while fading out to start
	bool bLeaveNew = false;
	struct FFlake { FVector2f P; float Size, Speed, Sway, Phase, Alpha; };
	TArray<FFlake> Flakes;
};

/** The start level's game mode: no pawn, the start menu on screen, the old Blueprint menu removed. */
UCLASS()
class YGGDRASILSPEAK_API AYggMenuGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AYggMenuGameMode();
	virtual void StartPlay() override;
};
