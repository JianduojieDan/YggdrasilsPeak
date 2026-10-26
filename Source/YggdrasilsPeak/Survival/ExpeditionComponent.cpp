// Copyright Epic Games, Inc. All Rights Reserved.

#include "Survival/ExpeditionComponent.h"

#include "Survival/SnowTrailComponent.h"
#include "Survival/SpindriftComponent.h"
#include "Survival/SurvivalComponent.h"
#include "Survival/SurvivalConfig.h"
#include "Environment/FixedRope.h"
#include "Environment/RouteLandmark.h"
#include "Camera/CameraComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "TimerManager.h"

UExpeditionComponent::UExpeditionComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
}

void UExpeditionComponent::BeginPlay()
{
	Super::BeginPlay();

	for (TActorIterator<ARouteLandmark> It(GetWorld()); It; ++It)
	{
		Camps.Add(*It);
	}
	Camps.Sort([](const TWeakObjectPtr<ARouteLandmark>& A, const TWeakObjectPtr<ARouteLandmark>& B)
	{
		return A->GetActorLocation().Z < B->GetActorLocation().Z;
	});

	for (TActorIterator<AFixedRope> It(GetWorld()); It; ++It)
	{
		Ropes.Add(*It);
	}
	if (const ACharacter* Character = Cast<ACharacter>(GetOwner()))
	{
		DefaultWalkableFloorAngle = Character->GetCharacterMovement()->GetWalkableFloorAngle();
	}

	// Footprints for the local player (not part of the Blueprint, so it is added here).
	const APawn* Pawn = Cast<APawn>(GetOwner());
	if (Pawn && Pawn->IsLocallyControlled() && !GetOwner()->FindComponentByClass<USnowTrailComponent>())
	{
		USnowTrailComponent* Trails = NewObject<USnowTrailComponent>(GetOwner(), TEXT("SnowTrails"));
		Trails->RegisterComponent();
	}
	// Blowing snow around the local player, driven by the wind read in UpdateWeather.
	if (Pawn && Pawn->IsLocallyControlled() && !GetOwner()->FindComponentByClass<USpindriftComponent>())
	{
		USpindriftComponent* Spindrift = NewObject<USpindriftComponent>(GetOwner(), TEXT("Spindrift"));
		Spindrift->RegisterComponent();
	}

	Survival = GetOwner()->FindComponentByClass<USurvivalComponent>();
	if (Survival.IsValid())
	{
		Survival->OnDeath.AddDynamic(this, &UExpeditionComponent::HandleDeath);
	}
	StartTime = GetWorld()->GetTimeSeconds();

	// Ultra Dynamic Weather is a Blueprint; find it by class name so the code has no hard dependency on it.
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		if (It->GetClass()->GetName().Contains(TEXT("Ultra_Dynamic_Weather")))
		{
			WeatherActor = *It;
			break;
		}
	}
}

namespace
{
	FString Simplify(const FString& Name)
	{
		return Name.Replace(TEXT(" "), TEXT("")).ToLower();
	}

	FProperty* FindByDisplayName(const UClass* Class, const TCHAR* Name)
	{
		const FString Wanted = Simplify(Name);
		for (TFieldIterator<FProperty> It(Class); It; ++It)
		{
			if (Simplify(It->GetAuthoredName()) == Wanted || Simplify(It->GetName()) == Wanted)
			{
				return *It;
			}
		}
		return nullptr;
	}
}

bool UExpeditionComponent::ReadNumber(const UObject* Object, const TCHAR* Name, double& Out)
{
	const FProperty* Prop = Object ? FindByDisplayName(Object->GetClass(), Name) : nullptr;
	if (const FDoubleProperty* D = CastField<FDoubleProperty>(Prop))
	{
		Out = D->GetPropertyValue_InContainer(Object);
		return true;
	}
	if (const FFloatProperty* F = CastField<FFloatProperty>(Prop))
	{
		Out = F->GetPropertyValue_InContainer(Object);
		return true;
	}
	return false;
}

UObject* UExpeditionComponent::ReadObject(const UObject* Object, const TCHAR* Name)
{
	const FObjectPropertyBase* Prop = Object ? CastField<FObjectPropertyBase>(FindByDisplayName(Object->GetClass(), Name)) : nullptr;
	return Prop ? Prop->GetObjectPropertyValue_InContainer(Object) : nullptr;
}

void UExpeditionComponent::UpdateWeather()
{
	if (!Survival.IsValid())
	{
		return;
	}

	// Local state includes radial storms passing over the player; values are on UDW's 0..10 scale.
	const UObject* State = WeatherActor.IsValid() ? ReadObject(WeatherActor.Get(), TEXT("Local Weather State")) : nullptr;
	double Wind = 0.0, Snow = 0.0;
	if (State)
	{
		ReadNumber(State, TEXT("Wind Intensity"), Wind);
		ReadNumber(State, TEXT("Snow"), Snow);
		WindIntensity = float(Wind);
		double Direction = 0.0;
		if (ReadNumber(WeatherActor.Get(), TEXT("Wind Direction"), Direction))
		{
			WindDirection = float(Direction);
		}
	}
	const float NewStorm = FMath::Clamp(0.6f * float((Wind - 3.0) / 7.0) + 0.4f * float((Snow - 4.0) / 6.0), 0.f, 1.f);
	if (FMath::Abs(NewStorm - LoggedStorm) > 0.1f || (State == nullptr) != bLoggedNoState)
	{
		LoggedStorm = NewStorm;
		bLoggedNoState = State == nullptr;
		UE_LOG(LogSurvival, Log, TEXT("Weather: wind %.1f snow %.1f -> storm %.2f%s"), Wind, Snow, NewStorm, State ? TEXT("") : TEXT(" (no UDW local state found)"));
	}
	StormStrength = NewStorm;

	Survival->SetColdExposure(StormColdExposure * StormStrength);
	Survival->SetWeatherSpeedMultiplier(FMath::Lerp(1.f, StormSpeedMultiplier, StormStrength));

	if (!bStormWarned && StormStrength > 0.45f)
	{
		bStormWarned = true;
		Say(NSLOCTEXT("Expedition", "Storm", "A blizzard is coming in. Get to a camp or keep moving."), 7.f);
	}
	else if (bStormWarned && StormStrength < 0.2f)
	{
		bStormWarned = false;
		Say(NSLOCTEXT("Expedition", "StormOver", "The storm is passing."), 4.f);
	}
}

void UExpeditionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	CampCheckTimer -= DeltaTime;
	if (CampCheckTimer <= 0.f)
	{
		CampCheckTimer = 0.5f;
		UpdateCamps();
		UpdateWeather();
	}
	RopeCheckTimer -= DeltaTime;
	if (RopeCheckTimer <= 0.f)
	{
		RopeCheckTimer = 0.2f;
		UpdateRope();
	}
	UpdateScreenEffect(DeltaTime);
}

void UExpeditionComponent::UpdateRope()
{
	ACharacter* Character = Cast<ACharacter>(GetOwner());
	if (!Character || !Survival.IsValid() || Survival->IsDead())
	{
		return;
	}

	// Measure from the feet: the rope lies just above the snow.
	const FVector Feet = Character->GetActorLocation() - FVector(0.f, 0.f, Character->GetSimpleCollisionHalfHeight());
	// Letting go needs a bit more distance than clipping on, so walking along the edge does not flicker.
	const float Reach = bOnRope ? RopeClipDistance * 1.5f : RopeClipDistance;
	bool bClipped = false;
	for (const TWeakObjectPtr<AFixedRope>& Rope : Ropes)
	{
		if (Rope.IsValid() && Rope->IsNear(Feet, Reach) && Rope->DistanceToRope(Feet) < Reach)
		{
			bClipped = true;
			break;
		}
	}
	if (bClipped == bOnRope)
	{
		return;
	}

	bOnRope = bClipped;
	Survival->SetOnRope(bOnRope);
	Character->GetCharacterMovement()->SetWalkableFloorAngle(bOnRope ? FMath::Max(RopeWalkableFloorAngle, DefaultWalkableFloorAngle) : DefaultWalkableFloorAngle);

	if (bOnRope && !bRopeExplained)
	{
		bRopeExplained = true;
		Say(NSLOCTEXT("Expedition", "RopeFirst", "Clipped to the fixed rope. Follow it up."), 7.f);
	}
	else if (!bOnRope)
	{
		Say(NSLOCTEXT("Expedition", "RopeOff", "Off the rope. Find the orange line again."), 4.f);
	}
}

void UExpeditionComponent::UpdateCamps()
{
	if (!Survival.IsValid() || Survival->IsDead())
	{
		return;
	}

	const FVector Here = GetOwner()->GetActorLocation();
	ARouteLandmark* Inside = nullptr;
	for (const TWeakObjectPtr<ARouteLandmark>& Camp : Camps)
	{
		if (Camp.IsValid() && FVector::DistSquared2D(Here, Camp->GetActorLocation()) < FMath::Square(CampRadius))
		{
			Inside = Camp.Get();
			break;
		}
	}

	Survival->SetSheltered(Inside != nullptr);
	if (Inside == CurrentCamp.Get())
	{
		return;
	}
	CurrentCamp = Inside;
	if (!Inside)
	{
		return;
	}

	if (Inside->Kind == ERouteLandmarkKind::Summit && !bSummitReached)
	{
		bSummitReached = true;
		LastCamp = Inside;
		Say(FText::Format(NSLOCTEXT("Expedition", "Summit", "SUMMIT - {0}. Now get back down alive."), FText::FromString(Inside->LandmarkName)), 10.f);
		return;
	}

	if (Inside->bFinishLine && bSummitReached && !bFinished)
	{
		bFinished = true;
		const int32 Seconds = FMath::RoundToInt(GetWorld()->GetTimeSeconds() - StartTime);
		Say(FText::Format(NSLOCTEXT("Expedition", "Finished", "EXPEDITION COMPLETE - summit and back in {0}h {1}m, {2} deaths."),
			FText::AsNumber(Seconds / 3600), FText::AsNumber((Seconds / 60) % 60), FText::AsNumber(Deaths)), 20.f);
		LastCamp = Inside;
		return;
	}

	const bool bNew = LastCamp.Get() != Inside;
	LastCamp = Inside;
	Say(FText::Format(bNew ? NSLOCTEXT("Expedition", "CampNew", "{0} - shelter. Checkpoint saved.")
		: NSLOCTEXT("Expedition", "Camp", "{0} - shelter."), FText::FromString(Inside->LandmarkName)), 5.f);
}

void UExpeditionComponent::HandleDeath()
{
	++Deaths;
	if (ACharacter* Character = Cast<ACharacter>(GetOwner()))
	{
		Character->GetCharacterMovement()->DisableMovement();
	}

	const FText Cause = Survival.IsValid() && !Survival->GetDeathCause().IsEmpty() ? Survival->GetDeathCause()
		: NSLOCTEXT("Expedition", "Died", "You died.");
	const FText Where = LastCamp.IsValid() ? FText::FromString(LastCamp->LandmarkName) : NSLOCTEXT("Expedition", "Start", "the start");
	Say(FText::Format(NSLOCTEXT("Expedition", "Death", "{0} Back to {1}..."), Cause, Where), RespawnDelay + 2.f);

	GetWorld()->GetTimerManager().SetTimer(RespawnTimer, this, &UExpeditionComponent::Respawn, FMath::Max(RespawnDelay, 0.1f));
}

void UExpeditionComponent::Respawn()
{
	if (LastCamp.IsValid())
	{
		TeleportToCamp(LastCamp.Get());
	}
	if (ACharacter* Character = Cast<ACharacter>(GetOwner()))
	{
		Character->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	}
	if (Survival.IsValid())
	{
		Survival->Revive(RespawnNeedsPercent);
	}
}

void UExpeditionComponent::TeleportToCamp(ARouteLandmark* Camp)
{
	ACharacter* Character = Cast<ACharacter>(GetOwner());
	if (!Character || !Camp)
	{
		return;
	}
	// Beside the camp, a little above the ground so the capsule drops onto it.
	const FVector Target = Camp->GetActorLocation() + FVector(-400.f, 0.f, Character->GetSimpleCollisionHalfHeight() + 150.f);
	Character->TeleportTo(Target, Character->GetActorRotation(), false, true);
	Character->GetCharacterMovement()->StopMovementImmediately();
	CurrentCamp.Reset();
}

void UExpeditionComponent::UpdateScreenEffect(float DeltaTime)
{
	UCameraComponent* Camera = GetOwner()->FindComponentByClass<UCameraComponent>();
	if (!Camera || !Survival.IsValid())
	{
		return;
	}

	const USurvivalConfig* C = GetDefault<USurvivalConfig>();
	const float Oxygen = Survival->GetStatValue(ESurvivalStat::Oxygen);
	const float Target = HypoxiaScreenEffect * FMath::Clamp((C->OxygenFatigueBelow - Oxygen) / FMath::Max(C->OxygenFatigueBelow - C->OxygenDamageBelow, 1.f), 0.f, 1.f);
	DisplayedHypoxia = FMath::FInterpTo(DisplayedHypoxia, Target, DeltaTime, 1.5f);

	FPostProcessSettings& PP = Camera->PostProcessSettings;
	PP.bOverride_VignetteIntensity = true;
	PP.VignetteIntensity = FMath::Lerp(0.4f, 1.6f, DisplayedHypoxia);
	PP.bOverride_ColorSaturation = true;
	const float Saturation = FMath::Lerp(1.f, 0.45f, DisplayedHypoxia);
	PP.ColorSaturation = FVector4(Saturation, Saturation, Saturation, 1.f);
	Camera->PostProcessBlendWeight = 1.f;
}

void UExpeditionComponent::Say(const FText& Message, float Seconds)
{
	UE_LOG(LogSurvival, Log, TEXT("Expedition: %s"), *Message.ToString());
	OnMessage.Broadcast(Message, Seconds);
}

// ---- Console: jump between camps for testing ----

namespace
{
	FAutoConsoleCommandWithWorldAndArgs GExpeditionGoCommand(
		TEXT("ygg.Expedition.Go"),
		TEXT("ygg.Expedition.Go <index> - teleport to a camp, 0 = lowest (Dingboche) ... 8 = summit."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			const APlayerController* Controller = World ? World->GetFirstPlayerController() : nullptr;
			const APawn* Pawn = Controller ? Controller->GetPawn() : nullptr;
			UExpeditionComponent* Expedition = Pawn ? Pawn->FindComponentByClass<UExpeditionComponent>() : nullptr;
			if (!Expedition || Args.Num() < 1)
			{
				UE_LOG(LogSurvival, Warning, TEXT("Usage: ygg.Expedition.Go <index> (needs an ExpeditionComponent on the player)"));
				return;
			}
			const int32 Index = FCString::Atoi(*Args[0]);
			const TArray<TWeakObjectPtr<ARouteLandmark>>& Camps = Expedition->GetCamps();
			if (Camps.IsValidIndex(Index) && Camps[Index].IsValid())
			{
				Expedition->TeleportToCamp(Camps[Index].Get());
			}
		}));
}
