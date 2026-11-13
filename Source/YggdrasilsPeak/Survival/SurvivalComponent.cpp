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
	constexpr int32 NumSurvivalStats = 6;

	/** A tick that took longer than this many fixed steps drops the backlog instead of fast-forwarding. */
	constexpr int32 MaxStepsPerTick = 5;

	TAutoConsoleVariable<float> CVarSurvivalLogEvery(
		TEXT("ygg.Survival.LogEvery"),
		0.f,
		TEXT("Seconds between automatic survival stat dumps to the log (0 = off). For testing without looking at the HUD."),
		ECVF_Default);

	FORCEINLINE uint32 WarningBit(ESurvivalWarning Warning)
	{
		return 1u << static_cast<uint32>(Warning);
	}

	TAutoConsoleVariable<int32> CVarPhysio(
		TEXT("ygg.Physio"),
		-1,
		TEXT("Oxygen model: -1 = the config's choice (physiology on), 1 = physiology (air pressure, acclimatisation), 0 = the old linear rule."),
		ECVF_Default);

	/** Alveolar CO2 (Torr) by altitude (km): acclimatised (Grocott 2009 at 8.4 km; points between are estimates) and
	 *  just arrived (estimates). Same tables as Tools/Thesis/physio_model.py. */
	const float PhysioAcclKm[] = { 0.f, 3.f, 4.5f, 5.3f, 6.4f, 7.2f, 8.f, 8.4f, 8.85f };
	const float PhysioAcclCO2[] = { 40.f, 33.f, 27.5f, 24.f, 20.f, 17.f, 14.5f, 13.3f, 12.2f };
	const float PhysioAcuteKm[] = { 0.f, 3.f, 4.5f, 5.3f, 6.4f, 7.2f, 8.f, 8.85f };
	const float PhysioAcuteCO2[] = { 40.f, 36.f, 32.5f, 30.f, 27.f, 25.f, 23.f, 21.f };

	float PhysioTable(const float* X, const float* Y, int32 N, float V)
	{
		if (V <= X[0]) { return Y[0]; }
		for (int32 i = 1; i < N; ++i)
		{
			if (V <= X[i]) { return FMath::Lerp(Y[i - 1], Y[i], (V - X[i - 1]) / (X[i] - X[i - 1])); }
		}
		return Y[N - 1];
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

bool USurvivalComponent::UsesPhysiology() const
{
	const int32 Override = CVarPhysio.GetValueOnGameThread();
	return Override >= 0 ? Override > 0 : GetCfg()->bPhysiologyModel;
}

float USurvivalComponent::GetOxygenFatigueBelow() const
{
	return UsesPhysiology() ? GetCfg()->PhysioFatigueBelow : GetCfg()->OxygenFatigueBelow;
}

float USurvivalComponent::GetOxygenDamageBelow() const
{
	return UsesPhysiology() ? GetCfg()->PhysioDamageBelow : GetCfg()->OxygenDamageBelow;
}

float USurvivalComponent::PhysioSpO2(float AltitudeMeters, float Acclimatised, float GapTorr)
{
	// barometric pressure (West 1996) -> inspired O2 -> alveolar gas equation (R 0.8) -> arterial O2 -> blood pH from CO2
	// -> Severinghaus 1979 virtual PO2 and saturation curve (core at 37 C)
	const float H = FMath::Max(AltitudeMeters, 0.f) / 1000.f;
	const float A = FMath::Clamp(Acclimatised, 0.f, 1.f);
	const float PB = FMath::Exp(6.63268f - 0.1112f * H - 0.00149f * H * H);
	const float PIO2 = 0.2093f * (PB - 47.f);
	const float PaCO2 = FMath::Lerp(PhysioTable(PhysioAcuteKm, PhysioAcuteCO2, UE_ARRAY_COUNT(PhysioAcuteKm), H),
		PhysioTable(PhysioAcclKm, PhysioAcclCO2, UE_ARRAY_COUNT(PhysioAcclKm), H), A);
	const float PAO2 = FMath::Max(PIO2 - PaCO2 / 0.8f, 1.f);
	const float PaO2 = FMath::Max(PAO2 - GapTorr, 1.f);
	const float PH = 7.40f + FMath::Lerp(0.008f, 0.0049f, A) * (40.f - PaCO2);
	const float PV = PaO2 * FMath::Pow(10.f, 0.40f * (PH - 7.40f) + 0.06f * FMath::LogX(10.f, 40.f / PaCO2));
	return 100.f / (23400.f / (PV * PV * PV + 150.f * PV) + 1.f);
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

	DetectJump();

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

	const float LogEvery = CVarSurvivalLogEvery.GetValueOnGameThread();
	if (LogEvery > 0.f)
	{
		LogTimer += DeltaTime;
		if (LogTimer >= LogEvery)
		{
			LogTimer = 0.f;
			DebugDump();
		}
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

	const bool bGrounded = !IsAirborne();
	const float Grade = (bMoving && bGrounded) ? GetUphillGrade() * (bOnRope ? C->RopeGradeMultiplier : 1.f) : 0.f;

	// Altitude: 0 at the bottom of each effect's range, 1 at the summit.
	const float Elevation = GetElevationMeters();
	const float ColdAltitude = FMath::Clamp((Elevation - C->ColdStartsAtMeters) / FMath::Max(C->SummitMeters - C->ColdStartsAtMeters, 1.f), 0.f, 1.f);
	const float ThinAir = FMath::Clamp((Elevation - C->OxygenDropStartsAtMeters) / FMath::Max(C->SummitMeters - C->OxygenDropStartsAtMeters, 1.f), 0.f, 1.f);

	// Oxygen drifts towards what the altitude and the current effort allow.
	OxygenBottleRemaining = FMath::Max(0.f, OxygenBottleRemaining - Dt);
	// A spare bottle goes on by itself when the air gets too thin; the player only has to carry them.
	const bool bPhysio = UsesPhysiology();
	if (OxygenBottleRemaining <= 0.f && CarriedBottles > 0 && Oxygen < (bPhysio ? C->PhysioAutoBottleBelow : C->AutoBottleBelowOxygen))
	{
		--CarriedBottles;
		OxygenBottleRemaining = C->OxygenBottleSeconds;
		UE_LOG(LogSurvival, Log, TEXT("Put on a spare oxygen bottle (%d left)."), CarriedBottles);
	}
	float OxygenTarget = 100.f;
	if (bPhysio)
	{
		// acclimatisation: the adapted altitude creeps up while you are higher (fast resting in a camp's shelter), never
		// into the death zone, and slowly falls when you stay well below it
		if (AdaptedAltitude < 0.f) { AdaptedAltitude = C->StartAdaptedMeters; }
		const float Reach = FMath::Min(Elevation, C->AdaptMaxMeters);
		if (AdaptedAltitude < Reach)
		{
			const float Rate = (bSheltered && !bMoving) ? C->AdaptRestMetersPerSecond : C->AdaptMovingMetersPerSecond;
			AdaptedAltitude = FMath::Min(Reach, AdaptedAltitude + Rate * Dt);
		}
		else if (Elevation < AdaptedAltitude - 300.f)
		{
			AdaptedAltitude = FMath::Max(Elevation, AdaptedAltitude - C->AdaptLossMetersPerSecond * Dt);
		}
		// bottled oxygen = a lower felt altitude; effort widens the alveolar-arterial gap
		const float Breathed = OxygenBottleRemaining > 0.f ? FMath::Max(0.f, Elevation - C->BottleAltitudeDropMeters) : Elevation;
		const float Acclim = FMath::Clamp(1.f - (Breathed - AdaptedAltitude) / FMath::Max(C->AdaptSpanMeters, 1.f), C->PhysioMinAcclimatised, 1.f);
		const float Gap = C->GapRestTorr + (bSprinting ? C->GapSprintTorr : (bMoving ? C->GapWalkTorr : 0.f)) + C->GapPerGradeTorr * Grade;
		OxygenTarget = PhysioSpO2(Breathed, Acclim, Gap) + (bSheltered ? C->PhysioShelterBonus : 0.f);
	}
	else
	{
		const float Effort = bSprinting ? C->SprintOxygenCost : (bMoving ? C->WalkOxygenCost : 0.f);
		OxygenTarget = 100.f - ThinAir * ((100.f - C->RestingOxygenAtSummit) + Effort + C->UphillOxygenCostPerGrade * Grade);
		if (OxygenBottleRemaining > 0.f)
		{
			OxygenTarget += C->BottleOxygenBonus;
		}
		if (bSheltered)
		{
			OxygenTarget += C->ShelterOxygenBonus;
		}
	}
	OxygenTarget = FMath::Clamp(OxygenTarget, 0.f, 100.f);
	Oxygen = FMath::FInterpConstantTo(Oxygen, OxygenTarget, Dt, C->OxygenChangePerSecond);

	// 0 while oxygen is fine, 1 at the hypoxia damage threshold.
	const float OxygenFatigue = FMath::Clamp((GetOxygenFatigueBelow() - Oxygen) / FMath::Max(GetOxygenFatigueBelow() - GetOxygenDamageBelow(), 1.f), 0.f, 1.f);

	// 2. Update stats from activity and environment.
	const float NeedsMultiplier = bSprinting ? C->SprintNeedsMultiplier : (bMoving ? C->WalkNeedsMultiplier : 1.f);
	Satiety = FMath::Max(0.f, Satiety - C->SatietyDecayPerSecond * NeedsMultiplier * Dt);
	Hydration = FMath::Max(0.f, Hydration - C->HydrationDecayPerSecond * NeedsMultiplier * Dt);
	if (bSheltered)
	{
		// Resting in camp: eating and melting snow for water.
		const float Refill = C->ShelterNeedsRecoveryPerSecond * 0.01f * Dt;
		Satiety = FMath::Min(C->MaxSatiety, Satiety + C->MaxSatiety * Refill);
		Hydration = FMath::Min(C->MaxHydration, Hydration + C->MaxHydration * Refill);
	}

	// Wading through deep powder is hard work.
	const float SnowDrain = bMoving ? C->DeepSnowStaminaDrainPerSecond * SnowDepth : 0.f;
	if (bSprinting)
	{
		Stamina -= (C->SprintStaminaDrainPerSecond + SnowDrain) * Dt;
	}
	else if (!IsAirborne() && !bClimbing)   // hanging on a face is no rest (UClimbingComponent spends stamina itself)
	{
		float Recover = bMoving ? C->WalkStaminaRecoverPerSecond : C->IdleStaminaRecoverPerSecond;
		if (bPenalised)
		{
			Recover *= C->StaminaRecoveryPenaltyMultiplier;
		}
		Recover *= FMath::Lerp(1.f, C->StaminaRecoveryPenaltyMultiplier, OxygenFatigue);
		if (bSheltered)
		{
			Recover *= C->ShelterStaminaRecoveryMultiplier;
		}
		// Climbing costs more than walking can recover.
		Stamina += (Recover - C->UphillStaminaDrainPerGrade * Grade - SnowDrain) * Dt;
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

	float Exposure = 0.f;
	if (!bSheltered)
	{
		Exposure = FMath::Clamp(ColdAltitude * C->ColdExposureAtSummit + ColdExposure, 0.f, 1.f);
		if (bMoving)
		{
			Exposure *= C->MovingColdMultiplier;
		}
	}
	const float TargetTemperature = FMath::Lerp(C->NormalTemperature, C->FullExposureTargetTemperature, Exposure);
	const float WeatherNow = bSheltered ? 0.f : WeatherCold;
	const float TemperatureRate = TargetTemperature < BodyTemperature ? C->CoolingPerSecond
		: (WeatherNow > 0.f ? 0.f : C->WarmingPerSecond * (bSheltered ? C->ShelterWarmingMultiplier : 1.f));
	BodyTemperature = FMath::Clamp(FMath::FInterpConstantTo(BodyTemperature, TargetTemperature, Dt, TemperatureRate) - WeatherNow * Dt,
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
	const bool bHypoxic = Oxygen <= GetOxygenDamageBelow();

	const float StarveDamage = ConditionDamage(StarvationTimer, bStarving, C->StarvationDamagePerSecond);
	const float ThirstDamage = ConditionDamage(DehydrationTimer, bDehydrated, C->DehydrationDamagePerSecond);
	const float ColdDamage = ConditionDamage(HypothermiaTimer, bHypothermic, C->HypothermiaDamagePerSecond);
	const float AirDamage = ConditionDamage(HypoxiaTimer, bHypoxic, C->HypoxiaDamagePerSecond);
	const float Damage = StarveDamage + ThirstDamage + ColdDamage + AirDamage;

	// Remember the biggest killer for the death message.
	if (Damage > 0.f)
	{
		const float Worst = FMath::Max(FMath::Max(StarveDamage, ThirstDamage), FMath::Max(ColdDamage, AirDamage));
		DeathCause = Worst == ColdDamage ? NSLOCTEXT("Survival", "DiedCold", "You froze to death.")
			: Worst == AirDamage ? NSLOCTEXT("Survival", "DiedOxygen", "You collapsed from lack of oxygen.")
			: Worst == ThirstDamage ? NSLOCTEXT("Survival", "DiedThirst", "You died of thirst.")
			: NSLOCTEXT("Survival", "DiedHunger", "You starved.");
	}

	if (Damage > 0.f)
	{
		Health = FMath::Max(0.f, Health - Damage);
	}
	else if (!bStarving && !bDehydrated && !bHypothermic && !bHypoxic)
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
	bSprinting = bSprintRequested && !bDead && !bExhausted && Stamina > 0.f && FootingSpeedMultiplier >= 1.f && IsMoving();
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
	WithHysteresis(ESurvivalWarning::LowOxygen, Oxygen, UsesPhysiology() ? C->PhysioWarningEnter : C->OxygenWarningEnter, UsesPhysiology() ? C->PhysioWarningExit : C->OxygenWarningExit);
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

void USurvivalComponent::KillBy(const FText& Cause)
{
	if (!bDead)
	{
		DeathCause = Cause;
		Die();
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

bool USurvivalComponent::IsAirborne() const
{
	const ACharacter* Character = Cast<ACharacter>(GetOwner());
	const UCharacterMovementComponent* Movement = Character ? Character->GetCharacterMovement() : nullptr;
	return Movement && Movement->IsFalling();
}

void USurvivalComponent::DetectJump()
{
	const ACharacter* Character = Cast<ACharacter>(GetOwner());
	const UCharacterMovementComponent* Movement = Character ? Character->GetCharacterMovement() : nullptr;
	if (!Movement)
	{
		return;
	}

	const bool bOnGround = Movement->IsMovingOnGround();
	// Leaving the ground upward is a jump; walking off a ledge starts with zero or downward velocity.
	const bool bJumped = bWasOnGround && Movement->IsFalling() && Movement->Velocity.Z > 0.f;
	if (bOnGround || Movement->IsFalling())
	{
		bWasOnGround = bOnGround;
	}

	if (bJumped)
	{
		const USurvivalConfig* C = GetCfg();
		Stamina = FMath::Max(0.f, Stamina - C->JumpStaminaCost);
		if (Stamina <= 0.f)
		{
			bExhausted = true;
		}
		UpdateWarnings();
		NotifyChanges();
	}
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
	const float Target = (bSprinting ? C->SprintSpeed : C->WalkSpeed) * GetTerrainSpeedMultiplier();
	if (!FMath::IsNearlyEqual(Movement->MaxWalkSpeed, Target))
	{
		Movement->MaxWalkSpeed = Target;
	}
}

float USurvivalComponent::GetElevationMeters() const
{
	const USurvivalConfig* C = GetCfg();
	const ACharacter* Character = Cast<ACharacter>(GetOwner());
	if (!Character)
	{
		return 0.f;
	}
	// Feet, not capsule centre.
	const float FeetZ = Character->GetActorLocation().Z - Character->GetSimpleCollisionHalfHeight();
	return C->ReferenceElevationMeters + (FeetZ - C->ReferenceWorldZ) / 100.f;
}

float USurvivalComponent::GetUphillGrade() const
{
	const ACharacter* Character = Cast<ACharacter>(GetOwner());
	const UCharacterMovementComponent* Movement = Character ? Character->GetCharacterMovement() : nullptr;
	if (!Movement || !Movement->CurrentFloor.IsWalkableFloor())
	{
		return 0.f;
	}

	const FVector Normal = Movement->CurrentFloor.HitResult.ImpactNormal;
	const FVector Direction = Movement->Velocity.GetSafeNormal2D();
	if (Normal.Z < KINDA_SMALL_NUMBER || Direction.IsNearlyZero())
	{
		return 0.f;
	}
	// The floor rises along -Normal.xy; its rise per unit of run in our direction is -(N.xy . dir) / N.z.
	return FMath::Max(0.f, -FVector::DotProduct(FVector(Normal.X, Normal.Y, 0.f), Direction) / Normal.Z);
}

float USurvivalComponent::GetTerrainSpeedMultiplier() const
{
	const USurvivalConfig* C = GetCfg();
	const float Grade = GetUphillGrade() * (bOnRope ? C->RopeGradeMultiplier : 1.f);
	const float Uphill = 1.f / (1.f + C->UphillSlowdownPerGrade * Grade);
	const float OxygenFatigue = FMath::Clamp((GetOxygenFatigueBelow() - Oxygen) / FMath::Max(GetOxygenFatigueBelow() - GetOxygenDamageBelow(), 1.f), 0.f, 1.f);
	const float Weather = bOnRope ? FMath::Lerp(WeatherSpeedMultiplier, 1.f, C->RopeWeatherRelief) : WeatherSpeedMultiplier;
	const float Snow = FMath::Lerp(1.f, C->DeepSnowSpeedMultiplier, SnowDepth);
	return Uphill * FMath::Lerp(1.f, C->LowOxygenSpeedMultiplier, OxygenFatigue) * Weather * Snow * LoadSpeedMultiplier * FootingSpeedMultiplier;
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
	Oxygen = 100.f;
	OxygenBottleRemaining = 0.f;
	DeathCause = FText::GetEmpty();

	StarvationTimer = 0.f;
	HypoxiaTimer = 0.f;
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
	case ESurvivalStat::Oxygen:          return Oxygen;
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
	case ESurvivalStat::Oxygen:          return 100.f;
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
	case ESurvivalStat::Oxygen:          Oxygen = FMath::Clamp(Value, 0.f, 100.f); break;
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
	Snapshot.Oxygen = Oxygen;
	Snapshot.ElevationMeters = GetElevationMeters();
	Snapshot.OxygenBottleSeconds = OxygenBottleRemaining;
	Snapshot.CarriedBottles = CarriedBottles;
	Snapshot.bSheltered = bSheltered;
	Snapshot.bOnRope = bOnRope;
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
	case ESurvivalWarning::LowOxygen: return NSLOCTEXT("Survival", "LowOxygen", "The air is too thin. Slow down, go lower or use bottled oxygen.");
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

void USurvivalComponent::SetSheltered(bool bInShelter)
{
	bSheltered = bInShelter;
}

void USurvivalComponent::UseOxygenBottle(float Seconds)
{
	OxygenBottleRemaining += Seconds > 0.f ? Seconds : GetCfg()->OxygenBottleSeconds;
}

bool USurvivalComponent::AddCarriedBottle()
{
	if (CarriedBottles >= GetCfg()->MaxCarriedBottles)
	{
		return false;
	}
	++CarriedBottles;
	return true;
}

void USurvivalComponent::SetCarriedBottles(int32 Count)
{
	CarriedBottles = FMath::Clamp(Count, 0, GetCfg()->MaxCarriedBottles);
}

void USurvivalComponent::Revive(float NeedsPercent)
{
	const float KeepSatiety = Satiety;
	const float KeepHydration = Hydration;
	ResetStats();
	const USurvivalConfig* C = GetCfg();
	// Coming back costs food and water, but never leaves you with less than NeedsPercent.
	Satiety = FMath::Max(KeepSatiety, C->MaxSatiety * NeedsPercent * 0.01f);
	Hydration = FMath::Max(KeepHydration, C->MaxHydration * NeedsPercent * 0.01f);
	UpdateWarnings();
	NotifyChanges();
	ApplyMovementSpeed();
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
	UE_LOG(LogSurvival, Log, TEXT("[%s] Health %.1f | Stamina %.1f | Satiety %.1f | Hydration %.1f | Temp %.2fC | O2 %.1f | %.0f m | Sheltered %d | Rope %d | Snow %.2f | Spare %d | Bottle %.0fs | Sprinting %d | Exhausted %d | Dead %d | ColdExposure %.2f"),
		*GetNameSafe(GetOwner()), Health, Stamina, Satiety, Hydration, BodyTemperature, Oxygen, GetElevationMeters(),
		bSheltered, bOnRope, SnowDepth, CarriedBottles, OxygenBottleRemaining, bSprinting, bExhausted, bDead, ColdExposure);

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
		TEXT("ygg.Survival.Set <Health|Stamina|Satiety|Hydration|BodyTemperature|Oxygen> <value> - set a survival stat on the local player."),
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
				UE_LOG(LogSurvival, Warning, TEXT("Usage: ygg.Survival.Set <Health|Stamina|Satiety|Hydration|BodyTemperature|Oxygen> <value>"));
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

	// 2026-11-14: the physiology model's saturation at every camp (rest / walk / climb, with a bottle, just arrived), and
	// the local climber's adapted altitude. ygg.Physio.Adapt <m> sets the adaptation for testing.
	FAutoConsoleCommandWithWorldAndArgs GPhysioSweepCommand(
		TEXT("ygg.Physio.Sweep"),
		TEXT("ygg.Physio.Sweep - print the physiology model's SpO2 at every camp altitude."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>&, UWorld* World)
		{
			const USurvivalConfig* C = GetDefault<USurvivalConfig>();
			struct FCamp { const TCHAR* Name; float M; };
			const FCamp Camps[] = { { TEXT("Dingboche"), 4410.f }, { TEXT("Base Camp"), 5364.f }, { TEXT("Camp 1"), 6065.f }, { TEXT("Camp 2"), 6400.f },
				{ TEXT("Camp 3"), 7160.f }, { TEXT("South Col"), 7906.f }, { TEXT("8,400 m"), 8400.f }, { TEXT("Summit"), 8849.f } };
			UE_LOG(LogSurvival, Display, TEXT("Physio sweep (acclimatised; thresholds fatigue %.0f, warning %.0f, damage %.0f):  rest  walk  climb | bottle walk climb | arrived rest"),
				C->PhysioFatigueBelow, C->PhysioWarningEnter, C->PhysioDamageBelow);
			for (const FCamp& K : Camps)
			{
				const float Rest = USurvivalComponent::PhysioSpO2(K.M, 1.f, C->GapRestTorr);
				const float Walk = USurvivalComponent::PhysioSpO2(K.M, 1.f, C->GapRestTorr + C->GapWalkTorr);
				const float Climb = USurvivalComponent::PhysioSpO2(K.M, 1.f, C->GapRestTorr + C->GapSprintTorr + C->GapPerGradeTorr);
				const float B = FMath::Max(0.f, K.M - C->BottleAltitudeDropMeters);
				UE_LOG(LogSurvival, Display, TEXT("  %-10s %5.0f m   %5.1f %5.1f %5.1f |  %5.1f %5.1f |  %5.1f"), K.Name, K.M, Rest, Walk, Climb,
					USurvivalComponent::PhysioSpO2(B, 1.f, C->GapRestTorr + C->GapWalkTorr), USurvivalComponent::PhysioSpO2(B, 1.f, C->GapRestTorr + C->GapSprintTorr + C->GapPerGradeTorr),
					USurvivalComponent::PhysioSpO2(K.M, 0.f, C->GapRestTorr));
			}
			if (USurvivalComponent* Survival = FindLocalSurvival(World))
			{
				UE_LOG(LogSurvival, Display, TEXT("  you: %.0f m, adapted to %.0f m, SpO2 %.1f, model %s"), Survival->GetElevationMeters(),
					Survival->GetAdaptedAltitude(), Survival->GetStatValue(ESurvivalStat::Oxygen), Survival->UsesPhysiology() ? TEXT("physiology") : TEXT("linear"));
			}
		}));

	FAutoConsoleCommandWithWorldAndArgs GPhysioAdaptCommand(
		TEXT("ygg.Physio.Adapt"),
		TEXT("ygg.Physio.Adapt <metres> - set the altitude the local climber is adapted to."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (USurvivalComponent* Survival = FindLocalSurvival(World))
			{
				if (Args.Num() >= 1) { Survival->SetAdaptedAltitude(FCString::Atof(*Args[0])); }
				UE_LOG(LogSurvival, Display, TEXT("Adapted to %.0f m"), Survival->GetAdaptedAltitude());
			}
		}));

	FAutoConsoleCommandWithWorldAndArgs GSurvivalBottleCommand(
		TEXT("ygg.Survival.Bottle"),
		TEXT("ygg.Survival.Bottle [seconds] - start breathing bottled oxygen (default: one bottle)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			if (USurvivalComponent* Survival = FindLocalSurvival(World))
			{
				Survival->UseOxygenBottle(Args.Num() > 0 ? FCString::Atof(*Args[0]) : 0.f);
				Survival->DebugDump();
			}
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

	// One switch for a visual check of the whole HUD and the weather effects.
	FAutoConsoleCommandWithWorldAndArgs GTestAllCommand(
		TEXT("ygg.Test.All"),
		TEXT("ygg.Test.All [0] - show everything at once: all HUD bars and warning icons, frost on the screen, blowing snow. 0 = back to normal."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			USurvivalComponent* Survival = FindLocalSurvival(World);
			if (!Survival)
			{
				UE_LOG(LogSurvival, Warning, TEXT("ygg.Test.All needs a SurvivalComponent on the local player (press Play first)."));
				return;
			}
			const bool bOn = Args.Num() == 0 || FCString::Atoi(*Args[0]) != 0;
			IConsoleVariable* Spindrift = IConsoleManager::Get().FindConsoleVariable(TEXT("ygg.Snow.Spindrift"));
			if (bOn)
			{
				Survival->DebugSetStat(ESurvivalStat::Satiety, 10.f);
				Survival->DebugSetStat(ESurvivalStat::Hydration, 10.f);
				Survival->DebugSetStat(ESurvivalStat::Health, 20.f);
				Survival->DebugSetStat(ESurvivalStat::Stamina, 40.f);
				Survival->DebugSetStat(ESurvivalStat::BodyTemperature, 32.f);
				Survival->SetColdExposure(1.f);
				if (Spindrift)
				{
					Spindrift->Set(1.f, ECVF_SetByConsole);
				}
			}
			else
			{
				Survival->SetColdExposure(0.f);
				Survival->DebugResetAll();
				if (Spindrift)
				{
					Spindrift->Set(-1.f, ECVF_SetByConsole);
				}
			}
			Survival->DebugDump();
		}));
}
