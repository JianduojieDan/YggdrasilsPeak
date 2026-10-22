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
 *  Owns the player's body stats (health, stamina, satiety, hydration, body temperature, oxygen)
 *  and decides walk/sprint speed on a Character. Altitude (from world Z), uphill grade and camp
 *  shelter feed the simulation directly. HUD widgets read GetSnapshot() and the
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

	/** Extra cold exposure (0..1) on top of altitude, e.g. from weather. */
	UFUNCTION(BlueprintCallable, Category="Survival")
	void SetColdExposure(float Exposure01);

	/** Extra speed multiplier from weather (1 = calm, lower = fighting the wind). */
	UFUNCTION(BlueprintCallable, Category="Survival")
	void SetWeatherSpeedMultiplier(float Multiplier) { WeatherSpeedMultiplier = FMath::Clamp(Multiplier, 0.1f, 1.f); }

	/** Inside a camp: no cold exposure, faster recovery, a little more oxygen. */
	UFUNCTION(BlueprintCallable, Category="Survival")
	void SetSheltered(bool bInShelter);

	/** Starts (or tops up) breathing bottled oxygen. Seconds <= 0 uses one bottle from the config. */
	UFUNCTION(BlueprintCallable, Category="Survival")
	void UseOxygenBottle(float Seconds = 0.f);

	/** Adds a spare bottle to the pack. Returns false when the pack is already full. Spares are put on automatically when oxygen runs low. */
	UFUNCTION(BlueprintCallable, Category="Survival")
	bool AddCarriedBottle();

	UFUNCTION(BlueprintPure, Category="Survival")
	int32 GetCarriedBottles() const { return CarriedBottles; }

	/** Clipped to a fixed rope: uphill and wind cost less. */
	UFUNCTION(BlueprintCallable, Category="Survival")
	void SetOnRope(bool bClipped) { bOnRope = bClipped; }

	UFUNCTION(BlueprintPure, Category="Survival")
	bool IsOnRope() const { return bOnRope; }

	/** How deep the snow underfoot is, 0 (hard / none) .. 1 (deepest powder): slows walking and costs stamina. */
	UFUNCTION(BlueprintCallable, Category="Survival")
	void SetSnowDepth(float Depth01) { SnowDepth = FMath::Clamp(Depth01, 0.f, 1.f); }

	/** Brings the player back after death (or any time) with full health/stamina/oxygen and needs at NeedsPercent. */
	UFUNCTION(BlueprintCallable, Category="Survival")
	void Revive(float NeedsPercent = 60.f);

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

	/** Metres above sea level of the owner's feet. */
	UFUNCTION(BlueprintPure, Category="Survival")
	float GetElevationMeters() const;

	/** What killed the player, for the death message. Empty while alive. */
	UFUNCTION(BlueprintPure, Category="Survival")
	FText GetDeathCause() const { return DeathCause; }

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
	/** Uphill grade (rise over run, 0 when flat or downhill) in the direction the character is moving. */
	float GetUphillGrade() const;
	/** Speed multiplier from slope and oxygen, applied on top of walk/sprint speed. */
	float GetTerrainSpeedMultiplier() const;
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
	float Oxygen = 0.f;

	/** Values last sent to listeners, indexed by ESurvivalStat. */
	float LastNotified[6] = { -1.f, -1.f, -1.f, -1.f, -1.f, -1.f };

	float OxygenBottleRemaining = 0.f;
	/** Spare bottles in the pack; kept through death and revive. */
	int32 CarriedBottles = 0;
	float WeatherSpeedMultiplier = 1.f;
	bool bSheltered = false;
	bool bOnRope = false;
	float SnowDepth = 0.f;
	FText DeathCause;

	/** Bitmask of active ESurvivalWarning values. */
	uint32 ActiveWarnings = 0;

	float ColdExposure = 0.f;
	float StepAccumulator = 0.f;
	float LogTimer = 0.f;

	/** Seconds each damaging condition has persisted. */
	float StarvationTimer = 0.f;
	float DehydrationTimer = 0.f;
	float HypothermiaTimer = 0.f;
	float HypoxiaTimer = 0.f;

	bool bWasOnGround = true;
	bool bSprintRequested = false;
	bool bSprinting = false;
	bool bExhausted = false;
	bool bDead = false;
};
