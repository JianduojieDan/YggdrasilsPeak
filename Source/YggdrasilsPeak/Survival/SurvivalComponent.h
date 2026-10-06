// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SurvivalTypes.h"
#include "SurvivalComponent.generated.h"

class USurvivalConfig;

DECLARE_LOG_CATEGORY_EXTERN(LogSurvival, Log, All);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnSurvivalStatChanged, ESurvivalStat, Stat, float, NewValue, float, OldValue);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSurvivalWarningChanged, ESurvivalWarning, Warning, bool, bActive);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSurvivalDeath);

/**
 *  Owns the player's body stats (health, stamina, satiety, hydration, body temperature)
 *  and decides walk/sprint speed on a Character. HUD widgets read GetSnapshot() and the
 *  change events; they never calculate stats themselves.
 *
 *  The simulation runs in fixed steps, stops while the game is paused, and fires death once.
 */
UCLASS(ClassGroup=(Survival), meta=(BlueprintSpawnableComponent))
class YGGDRASILSPEAK_API USurvivalComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	USurvivalComponent();

	/** Optional tuning asset. When empty a transient copy of the class defaults is used. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Survival")
	TObjectPtr<USurvivalConfig> Config;

	// ---- Events ----

	/** Fired when a stat changes by a visible amount. */
	UPROPERTY(BlueprintAssignable, Category="Survival")
	FOnSurvivalStatChanged OnStatChanged;

	/** Fired when a warning turns on or off. */
	UPROPERTY(BlueprintAssignable, Category="Survival")
	FOnSurvivalWarningChanged OnWarningChanged;

	/** Fired once when health reaches zero. */
	UPROPERTY(BlueprintAssignable, Category="Survival")
	FOnSurvivalDeath OnDeath;

	// ---- Input from the character ----

	/** Call with true when the sprint input starts and false when it ends or is cancelled. */
	UFUNCTION(BlueprintCallable, Category="Survival")
	void SetSprintRequested(bool bRequested);

	/** Cold exposure from 0 (comfortable) to 1 (full exposure). Temporary input until the environment service exists. */
	UFUNCTION(BlueprintCallable, Category="Survival")
	void SetColdExposure(float Exposure01);

	// ---- Reading state ----

	UFUNCTION(BlueprintPure, Category="Survival")
	FSurvivalSnapshot GetSnapshot() const;

	UFUNCTION(BlueprintPure, Category="Survival")
	float GetStatValue(ESurvivalStat Stat) const;

	UFUNCTION(BlueprintPure, Category="Survival")
	float GetStatMax(ESurvivalStat Stat) const;

	/** 0 to 1, with 0 empty/critical and 1 full. Body temperature is normalised between its minimum and normal value. */
	UFUNCTION(BlueprintPure, Category="Survival")
	float GetStatNormalized(ESurvivalStat Stat) const;

	UFUNCTION(BlueprintPure, Category="Survival")
	bool IsWarningActive(ESurvivalWarning Warning) const;

	UFUNCTION(BlueprintPure, Category="Survival")
	bool IsSprinting() const { return bSprinting; }

	UFUNCTION(BlueprintPure, Category="Survival")
	bool IsDead() const { return bDead; }

	/** Short player-facing text for a warning, including what to do about it. */
	UFUNCTION(BlueprintPure, Category="Survival")
	static FText GetWarningText(ESurvivalWarning Warning);

	// ---- Changing state ----

	/** Single entry point for damage from falls, rocks, environment and conditions. Ignored after death. */
	UFUNCTION(BlueprintCallable, Category="Survival")
	void ApplyDamage(float Amount);

	/** Adds (or subtracts, if negative) to a stat, clamped to its range. Intended for items and debug tools. */
	UFUNCTION(BlueprintCallable, Category="Survival")
	void AddToStat(ESurvivalStat Stat, float Delta);

	/** Sets a stat directly, clamped to its range. Intended for debug tools and tests. */
	UFUNCTION(BlueprintCallable, Category="Survival|Debug")
	void DebugSetStat(ESurvivalStat Stat, float Value);

	/** Restores every stat to its starting value and clears death. */
	UFUNCTION(BlueprintCallable, Category="Survival|Debug")
	void DebugResetAll();

	/** Logs all stats and the active warnings. */
	UFUNCTION(BlueprintCallable, Category="Survival|Debug")
	void DebugDump() const;

protected:

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:

	/** Advances the simulation by exactly one fixed step. */
	void StepSimulation(float Dt);
	void UpdateWarnings();
	/** Recomputes whether the character is sprinting right now from input, stamina and movement. */
	void RefreshSprintState();
	void ApplyMovementSpeed() const;
	const USurvivalConfig* GetCfg() const;
	void ResetStats();
	void SetStatInternal(ESurvivalStat Stat, float Value);
	void SetWarning(ESurvivalWarning Warning, bool bActive);
	void NotifyChanges();
	void Die();
	bool IsMoving() const;
	bool IsAirborne() const;
	/** Spends stamina once when the character leaves the ground moving upward. Runs every frame so no jump is missed. */
	void DetectJump();

	/** The config actually in use (assigned asset, or a transient default copy). */
	UPROPERTY(Transient)
	TObjectPtr<USurvivalConfig> ActiveConfig;

	float Health = 0.f;
	float Stamina = 0.f;
	float Satiety = 0.f;
	float Hydration = 0.f;
	float BodyTemperature = 0.f;

	/** Values last sent to listeners, indexed by ESurvivalStat. */
	float LastNotified[5] = { -1.f, -1.f, -1.f, -1.f, -1.f };

	/** Bitmask of active ESurvivalWarning values. */
	uint32 ActiveWarnings = 0;

	float ColdExposure = 0.f;
	float StepAccumulator = 0.f;

	/** Seconds each damaging condition has persisted. */
	float StarvationTimer = 0.f;
	float DehydrationTimer = 0.f;
	float HypothermiaTimer = 0.f;

	bool bWasOnGround = true;
	bool bSprintRequested = false;
	bool bSprinting = false;
	bool bExhausted = false;
	bool bDead = false;
};
