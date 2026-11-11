// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SettingsWidget.generated.h"

class UButton;
class UTextBlock;
class UVerticalBox;
class UFont;

/**
 *  Display settings (F2 in the game, console ygg.Settings). A dark card in the middle of the screen, built in code.
 *  - Footprints: Off (no trail map, no trench: cheapest) / Normal (2.5 cm per texel) / High (1.25 cm per texel).
 *  - Graphics: Auto (YggQualitySubsystem keeps the frame rate) or a fixed tier Low / Medium / High / Epic.
 *  Choices are saved in GameUserSettings.ini [YggdrasilsPeak.Display] and applied at every start.
 */
UCLASS()
class YGGDRASILSPEAK_API USettingsWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	static void Toggle(APlayerController* PC);
	static void CloseIfOpen(APlayerController* PC);
	/** Applies the saved choices (called when the climber spawns). */
	static void ApplySaved(UWorld* World);
	/** 0 off, 1 normal, 2 high. */
	static int32 GetFootprints();
	/** -1 auto, 0..3 fixed tier. */
	static int32 GetGraphics();
	/** 0 auto (dynamic in the game, Quality in the editor), 1 native, 2 quality 67%, 3 balanced 58%, 4 performance 50%. DLSS when its plugin is installed, TSR otherwise. */
	static int32 GetUpscaling();
	/** 0 = 60, 1 = 120, 2 = off. */
	static int32 GetFrameLimit();
	/** 1 = hardware ray-traced Lumen, 0 = software Lumen (cheaper). */
	static int32 GetRayTracing();
	/** Applies upscaling, frame limit and ray tracing now. */
	static void ApplyGraphicsOptions(UWorld* World);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	void Build();
	void Refresh();
	void SetLoose(bool bOpen);
	void Close();
	static void SetFootprints(UWorld* World, int32 Value);
	static void SetGraphics(UWorld* World, int32 Value);
	void AddRow(UVerticalBox* Box, const FString& Label, const TArray<TPair<FString, FName>>& Options, int32 Group, const FString& Hint);
	static int32 Current(int32 Group);

	UFUNCTION() void OnFootOff();
	UFUNCTION() void OnFootNormal();
	UFUNCTION() void OnFootHigh();
	UFUNCTION() void OnGfxAuto();
	UFUNCTION() void OnGfxLow();
	UFUNCTION() void OnGfxMedium();
	UFUNCTION() void OnGfxHigh();
	UFUNCTION() void OnGfxEpic();
	UFUNCTION() void OnCloseClicked();
	UFUNCTION() void OnUpAuto();
	UFUNCTION() void OnUpNative();
	UFUNCTION() void OnUpQuality();
	UFUNCTION() void OnUpBalanced();
	UFUNCTION() void OnUpPerformance();
	UFUNCTION() void OnFps60();
	UFUNCTION() void OnFps120();
	UFUNCTION() void OnFpsOff();
	UFUNCTION() void OnRtOn();
	UFUNCTION() void OnRtOff();
	static void SetInt(const TCHAR* Key, int32 Value);

	UPROPERTY(Transient) TObjectPtr<UFont> Serif;
	UPROPERTY(Transient) TArray<TObjectPtr<UButton>> OptButtons;
	UPROPERTY(Transient) TArray<TObjectPtr<UTextBlock>> OptTexts;
	UPROPERTY(Transient) TArray<TObjectPtr<class UImage>> OptBeads;
	TArray<int32> OptGroup, OptIndex;
};
