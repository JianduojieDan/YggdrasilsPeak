// Copyright Epic Games, Inc. All Rights Reserved.

#include "Survival/ExpeditionComponent.h"

#include "Survival/SurvivalComponent.h"
#include "Survival/SurvivalConfig.h"
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

	Survival = GetOwner()->FindComponentByClass<USurvivalComponent>();
	if (Survival.IsValid())
	{
		Survival->OnDeath.AddDynamic(this, &UExpeditionComponent::HandleDeath);
	}
	StartTime = GetWorld()->GetTimeSeconds();
}

void UExpeditionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	CampCheckTimer -= DeltaTime;
	if (CampCheckTimer <= 0.f)
	{
		CampCheckTimer = 0.5f;
		UpdateCamps();
	}
	UpdateScreenEffect(DeltaTime);
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
