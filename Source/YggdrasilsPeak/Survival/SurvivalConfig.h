// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "SurvivalConfig.generated.h"

/**
 *  All tunable survival numbers in one place. Create a DataAsset of this class to override
 *  the defaults, or leave USurvivalComponent::Config empty to use the defaults below.
 *  Values are prototype starting points, not balanced parameters.
 */
UCLASS(BlueprintType)
class YGGDRASILSPEAK_API USurvivalConfig : public UDataAsset
{
	GENERATED_BODY()

public:

	// ---- Simulation ----

	/** The survival simulation advances in fixed steps so results do not depend on frame rate. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Simulation", meta=(ClampMin="0.01", ClampMax="1.0"))
	float FixedStepSeconds = 0.1f;

	// ---- Movement ----

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Movement", meta=(ClampMin="0"))
	float WalkSpeed = 230.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Movement", meta=(ClampMin="0"))
	float SprintSpeed = 500.f;

	/** Horizontal speed above which the character counts as moving for recovery and drain rules. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Movement", meta=(ClampMin="0"))
	float MovingSpeedThreshold = 10.f;

	// ---- Maximums ----

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Maximums", meta=(ClampMin="1"))
	float MaxHealth = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Maximums", meta=(ClampMin="1"))
	float MaxStamina = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Maximums", meta=(ClampMin="1"))
	float MaxSatiety = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Maximums", meta=(ClampMin="1"))
	float MaxHydration = 100.f;

	// ---- Stamina ----

	/** Stamina lost per second while sprinting. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stamina", meta=(ClampMin="0"))
	float SprintStaminaDrainPerSecond = 12.f;

	/** Stamina regained per second while standing still. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stamina", meta=(ClampMin="0"))
	float IdleStaminaRecoverPerSecond = 15.f;

	/** Stamina regained per second while walking. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stamina", meta=(ClampMin="0"))
	float WalkStaminaRecoverPerSecond = 8.f;

	/** After hitting zero, sprinting stays blocked until stamina recovers to this value. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stamina", meta=(ClampMin="0"))
	float SprintResumeStamina = 20.f;

	/** Stamina recovery is multiplied by this while hungry, thirsty or cold. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Stamina", meta=(ClampMin="0", ClampMax="1"))
	float StaminaRecoveryPenaltyMultiplier = 0.5f;

	// ---- Satiety and hydration ----

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Needs", meta=(ClampMin="0"))
	float SatietyDecayPerSecond = 0.05f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Needs", meta=(ClampMin="0"))
	float HydrationDecayPerSecond = 0.08f;

	/** Decay is multiplied by this while walking. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Needs", meta=(ClampMin="0"))
	float WalkNeedsMultiplier = 1.5f;

	/** Decay is multiplied by this while sprinting. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Needs", meta=(ClampMin="0"))
	float SprintNeedsMultiplier = 3.f;

	// ---- Body temperature (degrees Celsius) ----

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Temperature")
	float NormalTemperature = 36.6f;

	/** The lowest temperature the stat can reach; used as 0 when normalising for the HUD. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Temperature")
	float MinTemperature = 30.f;

	/** Temperature the body drifts towards at full cold exposure. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Temperature")
	float FullExposureTargetTemperature = 31.f;

	/** Degrees per second lost while below the exposure target. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Temperature", meta=(ClampMin="0"))
	float CoolingPerSecond = 0.02f;

	/** Degrees per second regained while the target is warmer than the body. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Temperature", meta=(ClampMin="0"))
	float WarmingPerSecond = 0.05f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Temperature")
	float ColdWarningEnter = 35.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Temperature")
	float ColdWarningExit = 35.8f;

	/** At or below this temperature the body takes hypothermia damage after the grace period. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Temperature")
	float HypothermiaDamageBelow = 33.f;

	// ---- Health ----

	/** Seconds a damaging condition must persist before it starts hurting, so a player is not locked out instantly. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Health", meta=(ClampMin="0"))
	float DamageGraceSeconds = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Health", meta=(ClampMin="0"))
	float StarvationDamagePerSecond = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Health", meta=(ClampMin="0"))
	float DehydrationDamagePerSecond = 0.8f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Health", meta=(ClampMin="0"))
	float HypothermiaDamagePerSecond = 0.8f;

	/** Health regained per second when no damaging condition is active and food and water are above the minimum below. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Health", meta=(ClampMin="0"))
	float HealthRegenPerSecond = 0.2f;

	/** Satiety and hydration (percent of max) both need to be above this for health to regenerate. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Health", meta=(ClampMin="0", ClampMax="100"))
	float RegenMinNeedsPercent = 50.f;

	// ---- Warning thresholds (percent of max) ----

	/** A need or stat warning turns on at or below this percent... */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Warnings", meta=(ClampMin="0", ClampMax="100"))
	float WarningEnterPercent = 25.f;

	/** ...and turns off only once it is back at or above this percent. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Warnings", meta=(ClampMin="0", ClampMax="100"))
	float WarningExitPercent = 35.f;
};
