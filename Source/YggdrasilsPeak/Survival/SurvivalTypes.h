// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "SurvivalTypes.generated.h"

/** The body stats driven by USurvivalComponent. */
UENUM(BlueprintType)
enum class ESurvivalStat : uint8
{
	Health,
	Stamina,
	/** Higher is better: 100 = full, 0 = starving. */
	Satiety,
	/** Higher is better: 100 = fully hydrated, 0 = dehydrated. */
	Hydration,
	/** Core temperature in degrees Celsius. */
	BodyTemperature
};

/** Danger states the HUD should surface. Enter/exit use separate thresholds to avoid flicker. */
UENUM(BlueprintType)
enum class ESurvivalWarning : uint8
{
	Exhausted,
	Hungry,
	Thirsty,
	Cold,
	LowHealth
};

/** Read-only copy of the current survival state for HUD and debug tools. */
USTRUCT(BlueprintType)
struct FSurvivalSnapshot
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category="Survival")
	float Health = 0.f;

	UPROPERTY(BlueprintReadOnly, Category="Survival")
	float Stamina = 0.f;

	UPROPERTY(BlueprintReadOnly, Category="Survival")
	float Satiety = 0.f;

	UPROPERTY(BlueprintReadOnly, Category="Survival")
	float Hydration = 0.f;

	/** Degrees Celsius. */
	UPROPERTY(BlueprintReadOnly, Category="Survival")
	float BodyTemperature = 0.f;

	UPROPERTY(BlueprintReadOnly, Category="Survival")
	bool bSprinting = false;

	/** True from stamina reaching zero until it recovers past the resume threshold. */
	UPROPERTY(BlueprintReadOnly, Category="Survival")
	bool bExhausted = false;

	UPROPERTY(BlueprintReadOnly, Category="Survival")
	bool bDead = false;
};
