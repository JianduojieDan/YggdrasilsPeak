// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tickable.h"
#include "YggQualitySubsystem.generated.h"

/**
 *  Keeps the game playable on any machine, starting from the player's own choices (Settings > Graphics):
 *  - Quality: Auto = the machine's tier (hardware benchmark, at most High, kept in [YggdrasilsPeak.Display] MachineTier);
 *    Low..Epic = fixed.
 *  - Upscaling: Auto = the render resolution follows the frame rate (50-85%); Native / Quality / Balanced / Performance
 *    = 100 / 67 / 58 / 50%.
 *  - While playing (not in the editor), if frames stay slow: resolution drops first (Upscaling Auto), then - only with
 *    Quality Auto - the tier, FOR THIS SESSION ONLY; both climb back when there is room. Nothing automatic is ever saved
 *    (2026-11-13: the old version saved every step down and never stepped up, so after a few hitches the game sat at Low
 *    and 50% for good - the "blurry snow" and settings that "did nothing").
 *  - When a world starts: everything small gets a draw distance by its size, so distant props are not rendered at all.
 *  Console: ygg.Quality <auto|0..3>, ygg.Cull <0|1>.
 */
UCLASS()
class YGGDRASILSPEAK_API UYggQualitySubsystem : public UGameInstanceSubsystem, public FTickableGameObject
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UYggQualitySubsystem, STATGROUP_Tickables); }
	virtual bool IsTickable() const override { return !IsTemplate(); }
	virtual ETickableTickType GetTickableTickType() const override { return IsTemplate() ? ETickableTickType::Never : ETickableTickType::Conditional; }

	static UYggQualitySubsystem* Get(UWorld* World);

	/** The player's choices: TierChoice -1 = Auto, 0..3 fixed; Upscaling 0 = Auto, 1 Native, 2 Quality, 3 Balanced, 4 Performance. */
	void Configure(int32 TierChoice, int32 UpscalingChoice);

	/** The tier in use right now (Auto: the machine's tier, lower while the session is struggling). */
	int32 GetTierInUse() const { return SessionTier; }
	int32 GetMachineTier() const { return MachineTier; }
	float GetPercent() const { return Percent; }

	void SetManualTier(int32 Tier);
	void SetAuto();
	static void ApplyDrawDistances(UWorld* World, bool bOn);

private:
	void ApplyTier(int32 Tier);
	void ApplyPercent();
	bool IsAutoTier() const { return ChosenTier < 0; }
	bool IsAutoPercent() const { return Upscaling == 0; }

	FDelegateHandle WorldHandle;
	int32 ChosenTier = -1, Upscaling = 0;
	int32 MachineTier = 2, SessionTier = 2;
	float Percent = 75.f;
	float Warmup = 25.f, WindowTime = 0.f, CalmTime = 0.f;
	int32 WindowFrames = 0;
	// 2026-11-14: frame times of the current window (judged by their median), slow windows in a row, and the tier the
	// next pause will switch to (-1 = none)
	TArray<float> WindowMs;
	int32 SlowWindows = 0, PendingTier = -1;
};
