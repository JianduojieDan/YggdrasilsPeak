// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tickable.h"
#include "YggQualitySubsystem.generated.h"

/**
 *  Keeps the game playable on any machine (never in the editor):
 *  - first launch: the hardware benchmark picks the quality tier, saved for next time;
 *  - while playing: if frames stay slow, resolution drops first, then the quality tier; resolution climbs back when there is room;
 *  - when a world starts: everything small gets a draw distance by its size, so distant props are not rendered at all.
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

	void SetManualTier(int32 Tier);
	void SetAuto();
	static void ApplyDrawDistances(UWorld* World, bool bOn);

private:
	void ApplyPercent();
	FDelegateHandle WorldHandle;
	bool bAuto = true;
	float Percent = 67.f;
	float Warmup = 25.f, WindowTime = 0.f, CalmTime = 0.f;
	int32 WindowFrames = 0;
};
