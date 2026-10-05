// Copyright Epic Games, Inc. All Rights Reserved.

#include "Survival/SurvivalComponent.h"
#include "Survival/SurvivalConfig.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"

DEFINE_LOG_CATEGORY(LogSurvival);

namespace
{
	constexpr int32 NumSurvivalStats = 5;

	/** A tick that took longer than this many fixed steps drops the backlog instead of fast-forwarding. */
	constexpr int32 MaxStepsPerTick = 5;

	FORCEINLINE uint32 WarningBit(ESurvivalWarning Warning)
	{
		return 1u << static_cast<uint32>(Warning);
	}
}

USurvivalComponent::USurvivalComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
}

const USurvivalConfig* USurvivalComponent::GetCfg() const
{
	return ActiveConfig ? ActiveConfig.Get() : GetDefault<USurvivalConfig>();
}

void USurvivalComponent::BeginPlay()
{
	Super::BeginPlay();

	ActiveConfig = Config ? Config.Get() : NewObject<USurvivalConfig>(this, NAME_None, RF_Transient);

	ResetStats();
	UpdateWarnings();
	NotifyChanges();
	ApplyMovementSpeed();
}

void USurvivalComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bDead)
	{
		return;
	}

	const float Step = FMath::Max(GetCfg()->FixedStepSeconds, KINDA_SMALL_NUMBER);

	StepAccumulator += DeltaTime;
	int32 StepsRun = 0;
	while (StepAccumulator >= Step && StepsRun < MaxStepsPerTick && !bDead)
	{
		StepSimulation(Step);
		StepAccumulator -= Step;
		++StepsRun;
	}

	if (StepsRun == MaxStepsPerTick)
	{
		StepAccumulator = 0.f;
	}

	// Sprint eligibility and speed are re-checked every frame so input feels immediate.
	RefreshSprintState();
	ApplyMovementSpeed();
}

// ---- Simulation ----

void USurvivalComponent::StepSimulation(float Dt)
{
	const USurvivalConfig* C = GetCfg();

	// 1. Gather what the character is doing.
	const bool bMoving = IsMoving();
	RefreshSprintState();

	const bool bPenalised = IsWarningActive(ESurvivalWarning::Hungry)
		|| IsWarningActive(ESurvivalWarning::Thirsty)
		|| IsWarningActive(ESurvivalWarning::Cold);

	// 2. Update stats from activity and environment.
	const float NeedsMultiplier = bSprinting ? C->SprintNeedsMultiplier : (bMoving ? C->WalkNeedsMultiplier : 1.f);
	Satiety = FMath::Max(0.f, Satiety - C->SatietyDecayPerSecond * NeedsMultiplier * Dt);
	Hydration = FMath::Max(0.f, Hydration - C->HydrationDecayPerSecond * NeedsMultiplier * Dt);

	if (bSprinting)
	{
		Stamina -= C->SprintStaminaDrainPerSecond * Dt;
	}
	else
	{
		float Recover = bMoving ? C->WalkStaminaRecoverPerSecond : C->IdleStaminaRecoverPerSecond;
		if (bPenalised)
		{
			Recover *= C->StaminaRecoveryPenaltyMultiplier;
		}
		Stamina += Recover * Dt;
	}
	Stamina = FMath::Clamp(Stamina, 0.f, C->MaxStamina);

	if (Stamina <= 0.f)
	{
		bExhausted = true;
		bSprinting = false;
	}
	else if (bExhausted && Stamina >= C->SprintResumeStamina)
	{
		bExhausted = false;
	}

	const float TargetTemperature = FMath::Lerp(C->NormalTemperature, C->FullExposureTargetTemperature, ColdExposure);
	const float TemperatureRate = TargetTemperature < BodyTemperature ? C->CoolingPerSecond : C->WarmingPerSecond;
	BodyTemperature = FMath::Clamp(FMath::FInterpConstantTo(BodyTemperature, TargetTemperature, Dt, TemperatureRate),
		C->MinTemperature, C->NormalTemperature);

	// 3. Apply stage effects: damaging conditions only hurt after a grace period.
	auto ConditionDamage = [Dt, C](float& Timer, bool bActive, float DamagePerSecond)
	{
		if (!bActive)
		{
			// Recover twice as fast as it built up so a short relief gives a real window.
			Timer = FMath::Max(0.f, Timer - Dt * 2.f);
			return 0.f;
		}
		Timer += Dt;
		return Timer >= C->DamageGraceSeconds ? DamagePerSecond * Dt : 0.f;
	};

	const bool bStarving = Satiety <= 0.f;
	const bool bDehydrated = Hydration <= 0.f;
	const bool bHypothermic = BodyTemperature <= C->HypothermiaDamageBelow;

	const float Damage = ConditionDamage(StarvationTimer, bStarving, C->StarvationDamagePerSecond)
		+ ConditionDamage(DehydrationTimer, bDehydrated, C->DehydrationDamagePerSecond)
		+ ConditionDamage(HypothermiaTimer, bHypothermic, C->HypothermiaDamagePerSecond);

	if (Damage > 0.f)
	{
		Health = FMath::Max(0.f, Health - Damage);
	}
	else if (!bStarving && !bDehydrated && !bHypothermic)
	{
		const float RegenMinSatiety = C->MaxSatiety * C->RegenMinNeedsPercent * 0.01f;
		const float RegenMinHydration = C->MaxHydration * C->RegenMinNeedsPercent * 0.01f;
		if (Satiety > RegenMinSatiety && Hydration > RegenMinHydration)
		{
			Health = FMath::Min(C->MaxHealth, Health + C->HealthRegenPerSecond * Dt);
		}
	}

	// 4. Warnings, then 5. tell listeners.
	UpdateWarnings();
	NotifyChanges();

	if (Health <= 0.f)
	{
		Die();
	}
}

void USurvivalComponent::RefreshSprintState()
{
	bSprinting = bSprintRequested && !bDead && !bExhausted && Stamina > 0.f && IsMoving();
}

void USurvivalComponent::UpdateWarnings()
{
	const USurvivalConfig* C = GetCfg();

	auto WithHysteresis = [this](ESurvivalWarning Warning, float Value, float EnterAtOrBelow, float ExitAtOrAbove)
	{
		const bool bActive = IsWarningActive(Warning);
		if (!bActive && Value <= EnterAtOrBelow)
		{
			SetWarning(Warning, true);
		}
		else if (bActive && Value >= ExitAtOrAbove)
		{
			SetWarning(Warning, false);
		}
	};

	const float Enter = C->WarningEnterPercent;
	const float Exit = C->WarningExitPercent;

	SetWarning(ESurvivalWarning::Exhausted, bExhausted);
	WithHysteresis(ESurvivalWarning::Hungry, Satiety / C->MaxSatiety * 100.f, Enter, Exit);
	WithHysteresis(ESurvivalWarning::Thirsty, Hydration / C->MaxHydration * 100.f, Enter, Exit);
	WithHysteresis(ESurvivalWarning::LowHealth, Health / C->MaxHealth * 100.f, Enter, Exit);
	WithHysteresis(ESurvivalWarning::Cold, BodyTemperature, C->ColdWarningEnter, C->ColdWarningExit);
}

void USurvivalComponent::SetWarning(ESurvivalWarning Warning, bool bActive)
{
	const uint32 Bit = WarningBit(Warning);
	if (((ActiveWarnings & Bit) != 0) == bActive)
	{
		return;
	}

	ActiveWarnings = bActive ? (ActiveWarnings | Bit) : (ActiveWarnings & ~Bit);
	OnWarningChanged.Broadcast(Warning, bActive);
}

void USurvivalComponent::NotifyChanges()
{
	for (int32 Index = 0; Index < NumSurvivalStats; ++Index)
	{
		const ESurvivalStat Stat = static_cast<ESurvivalStat>(Index);
		const float Value = GetStatValue(Stat);
		if (FMath::Abs(Value - LastNotified[Index]) < 0.01f)
		{
			continue;
		}

		const float Old = LastNotified[Index] < 0.f ? Value : LastNotified[Index];
		LastNotified[Index] = Value;
		OnStatChanged.Broadcast(Stat, Value, Old);
	}
}

void USurvivalComponent::Die()
{
	if (bDead)
	{
		return;
	}

	bDead = true;
	bSprinting = false;
	Health = 0.f;

	UpdateWarnings();
	NotifyChanges();
	ApplyMovementSpeed();

	UE_LOG(LogSurvival, Log, TEXT("%s died."), *GetNameSafe(GetOwner()));
	OnDeath.Broadcast();
}

bool USurvivalComponent::IsMoving() const
{
	const ACharacter* Character = Cast<ACharacter>(GetOwner());
	const UCharacterMovementComponent* Movement = Character ? Character->GetCharacterMovement() : nullptr;
	if (!Movement)
	{
		return false;
	}

	const float Threshold = GetCfg()->MovingSpeedThreshold;
	return Movement->Velocity.SizeSquared2D() > FMath::Square(Threshold);
}

void USurvivalComponent::ApplyMovementSpeed() const
{
	const ACharacter* Character = Cast<ACharacter>(GetOwner());
	UCharacterMovementComponent* Movement = Character ? Character->GetCharacterMovement() : nullptr;
	if (!Movement)
	{
		return;
	}

	const USurvivalConfig* C = GetCfg();
	const float Target = bSprinting ? C->SprintSpeed : C->WalkSpeed;
	if (!FMath::IsNearlyEqual(Movement->MaxWalkSpeed, Target))
	{
		Movement->MaxWalkSpeed = Target;
	}
}

// ---- Stat access ----

void USurvivalComponent::ResetStats()
{
	const USurvivalConfig* C = GetCfg();

	Health = C->MaxHealth;
	Stamina = C->MaxStamina;
	Satiety = C->MaxSatiety;
	Hydration = C->MaxHydration;
	BodyTemperature = C->NormalTemperature;

	StarvationTimer = 0.f;
	DehydrationTimer = 0.f;
	HypothermiaTimer = 0.f;
	StepAccumulator = 0.f;

	bSprinting = false;
	bExhausted = false;
	bDead = false;
}

float USurvivalComponent::GetStatValue(ESurvivalStat Stat) const
{
	switch (Stat)
	{
	case ESurvivalStat::Health:          return Health;
	case ESurvivalStat::Stamina:         return Stamina;
	case ESurvivalStat::Satiety:         return Satiety;
	case ESurvivalStat::Hydration:       return Hydration;
	case ESurvivalStat::BodyTemperature: return BodyTemperature;
	}
	return 0.f;
}

float USurvivalComponent::GetStatMax(ESurvivalStat Stat) const
{
	const USurvivalConfig* C = GetCfg();
	switch (Stat)
	{
	case ESurvivalStat::Health:          return C->MaxHealth;
	case ESurvivalStat::Stamina:         return C->MaxStamina;
	case ESurvivalStat::Satiety:         return C->MaxSatiety;
	case ESurvivalStat::Hydration:       return C->MaxHydration;
	case ESurvivalStat::BodyTemperature: return C->NormalTemperature;
	}
	return 0.f;
}

float USurvivalComponent::GetStatNormalized(ESurvivalStat Stat) const
{
	const USurvivalConfig* C = GetCfg();
	const float Value = GetStatValue(Stat);

	if (Stat == ESurvivalStat::BodyTemperature)
	{
		const float Range = C->NormalTemperature - C->MinTemperature;
		return Range > KINDA_SMALL_NUMBER ? FMath::Clamp((Value - C->MinTemperature) / Range, 0.f, 1.f) : 1.f;
	}

	const float Max = GetStatMax(Stat);
	return Max > KINDA_SMALL_NUMBER ? FMath::Clamp(Value / Max, 0.f, 1.f) : 0.f;
}

void USurvivalComponent::SetStatInternal(ESurvivalStat Stat, float Value)
{
	const USurvivalConfig* C = GetCfg();
	switch (Stat)
	{
	case ESurvivalStat::Health:          Health = FMath::Clamp(Value, 0.f, C->MaxHealth); break;
	case ESurvivalStat::Stamina:         Stamina = FMath::Clamp(Value, 0.f, C->MaxStamina); break;
	case ESurvivalStat::Satiety:         Satiety = FMath::Clamp(Value, 0.f, C->MaxSatiety); break;
	case ESurvivalStat::Hydration:       Hydration = FMath::Clamp(Value, 0.f, C->MaxHydration); break;
	case ESurvivalStat::BodyTemperature: BodyTemperature = FMath::Clamp(Value, C->MinTemperature, C->NormalTemperature); break;
	}
}

FSurvivalSnapshot USurvivalComponent::GetSnapshot() const
{
	FSurvivalSnapshot Snapshot;
	Snapshot.Health = Health;
	Snapshot.Stamina = Stamina;
	Snapshot.Satiety = Satiety;
	Snapshot.Hydration = Hydration;
	Snapshot.BodyTemperature = BodyTemperature;
	Snapshot.bSprinting = bSprinting;
	Snapshot.bExhausted = bExhausted;
	Snapshot.bDead = bDead;
	return Snapshot;
}

bool USurvivalComponent::IsWarningActive(ESurvivalWarning Warning) const
{
	return (ActiveWarnings & WarningBit(Warning)) != 0;
}

FText USurvivalComponent::GetWarningText(ESurvivalWarning Warning)
{
	switch (Warning)
	{
	case ESurvivalWarning::Exhausted: return NSLOCTEXT("Survival", "Exhausted", "Out of breath. Stop and rest.");
	case ESurvivalWarning::Hungry:    return NSLOCTEXT("Survival", "Hungry", "You are hungry. Find something to eat.");
	case ESurvivalWarning::Thirsty:   return NSLOCTEXT("Survival", "Thirsty", "You are thirsty. Find water to drink.");
	case ESurvivalWarning::Cold:      return NSLOCTEXT("Survival", "Cold", "You are freezing. Find shelter or make camp.");
	case ESurvivalWarning::LowHealth: return NSLOCTEXT("Survival", "LowHealth", "You are badly hurt. Deal with what is hurting you.");
	}
	return FText::GetEmpty();
}

// ---- Input and external changes ----

void USurvivalComponent::SetSprintRequested(bool bRequested)
{
	bSprintRequested = bRequested;
	RefreshSprintState();
	ApplyMovementSpeed();
}

void USurvivalComponent::SetColdExposure(float Exposure01)
{
	ColdExposure = FMath::Clamp(Exposure01, 0.f, 1.f);
}

void USurvivalComponent::ApplyDamage(float Amount)
{
	if (bDead || Amount <= 0.f)
	{
		return;
	}

	SetStatInternal(ESurvivalStat::Health, Health - Amount);
	UpdateWarnings();
	NotifyChanges();

	if (Health <= 0.f)
	{
		Die();
	}
}

void USurvivalComponent::AddToStat(ESurvivalStat Stat, float Delta)
{
	if (bDead)
	{
		return;
	}

	SetStatInternal(Stat, GetStatValue(Stat) + Delta);
	UpdateWarnings();
	NotifyChanges();

	if (Health <= 0.f)
	{
		Die();
	}
}

void USurvivalComponent::DebugSetStat(ESurvivalStat Stat, float Value)
{
	if (bDead)
	{
		return;
	}

	SetStatInternal(Stat, Value);
	UpdateWarnings();
	NotifyChanges();

	if (Health <= 0.f)
	{
		Die();
	}
}

void USurvivalComponent::DebugResetAll()
{
	ResetStats();
	ColdExposure = 0.f;
	UpdateWarnings();
	NotifyChanges();
	ApplyMovementSpeed();
}

void USurvivalComponent::DebugDump() const
{
	UE_LOG(LogSurvival, Log, TEXT("[%s] Health %.1f | Stamina %.1f | Satiety %.1f | Hydration %.1f | Temp %.2fC | Sprinting %d | Exhausted %d | Dead %d | ColdExposure %.2f"),
		*GetNameSafe(GetOwner()), Health, Stamina, Satiety, Hydration, BodyTemperature,
		bSprinting, bExhausted, bDead, ColdExposure);

	const UEnum* WarningEnum = StaticEnum<ESurvivalWarning>();
	for (int32 Index = 0; Index < WarningEnum->NumEnums() - 1; ++Index)
	{
		const ESurvivalWarning Warning = static_cast<ESurvivalWarning>(WarningEnum->GetValueByIndex(Index));
		if (IsWarningActive(Warning))
		{
			UE_LOG(LogSurvival, Log, TEXT("  warning: %s"), *WarningEnum->GetNameStringByIndex(Index));
		}
	}
}

// ---- Console debug commands (work in PIE: open the console with ~) ----

namespace
{
	USurvivalComponent* FindLocalSurvival(UWorld* World)
	{
		const APlayerController* Controller = World ? World->GetFirstPlayerController() : nullptr;
		const APawn* Pawn = Controller ? Controller->GetPawn() : nullptr;
		return Pawn ? Pawn->FindComponentByClass<USurvivalComponent>() : nullptr;
	}

	FAutoConsoleCommandWithWorldAndArgs GSurvivalSetCommand(
		TEXT("ygg.Survival.Set"),
		TEXT("ygg.Survival.Set <Health|Stamina|Satiety|Hydration|BodyTemperature> <value> - set a survival stat on the local player."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			USurvivalComponent* Survival = FindLocalSurvival(World);
			if (!Survival)
			{
				UE_LOG(LogSurvival, Warning, TEXT("No SurvivalComponent on the local player pawn."));
				return;
			}

			const int64 StatValue = Args.Num() >= 2 ? StaticEnum<ESurvivalStat>()->GetValueByNameString(Args[0]) : INDEX_NONE;
			if (StatValue == INDEX_NONE)
			{
				UE_LOG(LogSurvival, Warning, TEXT("Usage: ygg.Survival.Set <Health|Stamina|Satiety|Hydration|BodyTemperature> <value>"));
				return;
			}

			Survival->DebugSetStat(static_cast<ESurvivalStat>(StatValue), FCString::Atof(*Args[1]));
			Survival->DebugDump();
		}));

	FAutoConsoleCommandWithWorldAndArgs GSurvivalColdCommand(
		TEXT("ygg.Survival.Cold"),
		TEXT("ygg.Survival.Cold <0..1> - set cold exposure on the local player."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			USurvivalComponent* Survival = FindLocalSurvival(World);
			if (!Survival || Args.Num() < 1)
			{
				UE_LOG(LogSurvival, Warning, TEXT("Usage: ygg.Survival.Cold <0..1> (needs a SurvivalComponent on the local pawn)"));
				return;
			}

			Survival->SetColdExposure(FCString::Atof(*Args[0]));
			Survival->DebugDump();
		}));

	FAutoConsoleCommandWithWorld GSurvivalDumpCommand(
		TEXT("ygg.Survival.Dump"),
		TEXT("Log the local player's survival stats and active warnings."),
		FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World)
		{
			if (USurvivalComponent* Survival = FindLocalSurvival(World))
			{
				Survival->DebugDump();
			}
			else
			{
				UE_LOG(LogSurvival, Warning, TEXT("No SurvivalComponent on the local player pawn."));
			}
		}));

	FAutoConsoleCommandWithWorld GSurvivalResetCommand(
		TEXT("ygg.Survival.Reset"),
		TEXT("Restore all survival stats of the local player and clear death."),
		FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World)
		{
			if (USurvivalComponent* Survival = FindLocalSurvival(World))
			{
				Survival->DebugResetAll();
				Survival->DebugDump();
			}
		}));
}
