// Copyright Epic Games, Inc. All Rights Reserved.

#include "Survival/SanityComponent.h"

#include "Environment/ParentsGhosts.h"
#include "Survival/CompassComponent.h"
#include "Survival/ExpeditionComponent.h"
#include "Survival/SurvivalComponent.h"
#include "Camera/CameraComponent.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

static TAutoConsoleVariable<float> CVarSanity(TEXT("ygg.Sanity"), -1.f, TEXT("Forces sanity to this value (0..100) for testing; -1 = off."));

USanityComponent::USanityComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void USanityComponent::BeginPlay()
{
	Super::BeginPlay();
	for (int32 i = 1; Whispers.Num() < 6 && i <= 6; ++i)
	{
		const FString N = FString::Printf(TEXT("Whisper_%02d"), i);
		if (USoundBase* S = LoadObject<USoundBase>(nullptr, *(FString(TEXT("/Game/Audio/Mind/")) + N + TEXT(".") + N)))
		{
			Whispers.Add(S);
		}
	}
	// the steps that follow you are the same boots in snow as your own (falls back to the sky pack's snow sounds)
	for (int32 i = 1; FollowingSteps.Num() < 8 && i <= 8; ++i)
	{
		const FString N = FString::Printf(TEXT("Step_Snow_%02d"), i);
		if (USoundBase* S = LoadObject<USoundBase>(nullptr, *(FString(TEXT("/Game/Audio/Footsteps/")) + N + TEXT(".") + N), nullptr, LOAD_NoWarn | LOAD_Quiet))
		{
			FollowingSteps.Add(S);
		}
	}
	for (int32 i = 1; FollowingSteps.Num() < 6 && i <= 6; ++i)
	{
		const FString N = FString::Printf(TEXT("Snow_Compress_%d"), i);
		if (USoundBase* S = LoadObject<USoundBase>(nullptr, *(FString(TEXT("/Game/Content_From_Internet/UltraDynamicSky/Sound/Snow/")) + N + TEXT(".") + N)))
		{
			FollowingSteps.Add(S);
		}
	}
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		if (It->GetClass()->GetName().Contains(TEXT("Ultra_Dynamic_Sky")))
		{
			Sky = *It;
			break;
		}
	}
}

float USanityComponent::TimeOfDay() const
{
	// Ultra Dynamic Sky's "Time of Day": 0..2400 (hours * 100)
	if (!Sky)
	{
		return 1200.f;
	}
	const FProperty* P = Sky->GetClass()->FindPropertyByName(TEXT("Time of Day"));
	if (const FDoubleProperty* D = CastField<FDoubleProperty>(P))
	{
		return float(D->GetPropertyValue_InContainer(Sky));
	}
	if (const FFloatProperty* F = CastField<FFloatProperty>(P))
	{
		return F->GetPropertyValue_InContainer(Sky);
	}
	return 1200.f;
}

void USanityComponent::Hallucinate()
{
	ACharacter* Character = Cast<ACharacter>(GetOwner());
	UExpeditionComponent* Expedition = Character->FindComponentByClass<UExpeditionComponent>();
	const FVector Behind = Character->GetActorLocation() - Character->GetControlRotation().Vector().GetSafeNormal2D() * 250.f;
	const float Roll = FMath::FRand();
	// low enough, and not seen them for a while: the parents, walking up through the snow ahead
	if (Sanity < 30.f && GhostCooldown <= 0.f && FMath::FRand() < 0.5f)
	{
		if (AParentsGhosts::SpawnAhead(GetWorld()))
		{
			GhostCooldown = 180.f;
			if (!bSawGhosts && Expedition)
			{
				bSawGhosts = true;
				Expedition->ShowMessage(NSLOCTEXT("Sanity", "Ghosts", "Two figures above you in the snow, climbing side by side. You know that walk."), 6.f);
				Expedition->MarkStoryEvent(TEXT("SawParents"));
			}
			Shock(4.f);
			return;
		}
	}
	if (Roll < 0.45f && Whispers.Num() > 0)
	{
		// a whisper just behind one shoulder
		const FVector At = Behind + FRotator(0.f, Character->GetControlRotation().Yaw + (FMath::RandBool() ? 90.f : -90.f), 0.f).Vector() * 80.f + FVector(0.f, 0.f, 60.f);
		UGameplayStatics::PlaySoundAtLocation(this, Whispers[FMath::RandRange(0, Whispers.Num() - 1)], At, 0.55f, FMath::FRandRange(0.85f, 1.05f));
		static bool bSaid = false;
		if (!bSaid && Expedition)
		{
			bSaid = true;
			Expedition->ShowMessage(NSLOCTEXT("Sanity", "Name", "...did someone just say your name?"), 4.f);
		}
	}
	else if (Roll < 0.8f && FollowingSteps.Num() > 0)
	{
		// someone walking behind you, coming closer - and then nobody there
		StepsLeft = FMath::RandRange(4, 7);
		NextStep = 0.f;
		StepFrom = Character->GetActorLocation() - Character->GetControlRotation().Vector().GetSafeNormal2D() * 700.f;
	}
	else if (Sanity < 35.f)
	{
		// the compass forgets which way is anything
		if (UCompassComponent* Compass = Character->FindComponentByClass<UCompassComponent>())
		{
			Compass->Kick(1.f, false);
		}
		Pulse = 1.f;
	}
}

void USanityComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	ACharacter* Character = Cast<ACharacter>(GetOwner());
	const USurvivalComponent* Survival = Character ? Character->FindComponentByClass<USurvivalComponent>() : nullptr;
	const UExpeditionComponent* Expedition = Character ? Character->FindComponentByClass<UExpeditionComponent>() : nullptr;
	if (!Survival || !Character->IsLocallyControlled())
	{
		return;
	}
	Time += DeltaTime;
	GhostCooldown = FMath::Max(0.f, GhostCooldown - DeltaTime);

	if (Survival->IsDead())
	{
		Sanity = FMath::Max(Sanity, 75.f);    // you come back to yourself at the checkpoint
		return;
	}
	// drains and recovery, per minute
	float Rate = 0.f;
	const float Elevation = Survival->GetElevationMeters();
	Rate -= FMath::Max(0.f, Elevation - StartAltitude) / 1000.f * AltitudeDrain;
	if (Survival->GetStatValue(ESurvivalStat::Oxygen) < 75.f) Rate -= HypoxiaDrain;
	const float Tod = TimeOfDay();
	if (Tod < 600.f || Tod > 1900.f) Rate -= NightDrain;
	if (Expedition && Expedition->GetWindIntensity() > 0.6f) Rate -= StormDrain;
	if (Survival->IsSheltered()) Rate = ShelterRecovery;
	Sanity = FMath::Clamp(Sanity + Rate * DeltaTime / 60.f, 0.f, 100.f);
	if (CVarSanity.GetValueOnGameThread() >= 0.f)
	{
		Sanity = CVarSanity.GetValueOnGameThread();
	}

	// things that are not there: more often the lower it goes
	if (Sanity < 60.f)
	{
		NextEvent -= DeltaTime;
		if (NextEvent <= 0.f)
		{
			NextEvent = FMath::Lerp(25.f, 90.f, FMath::Clamp((Sanity - 10.f) / 50.f, 0.f, 1.f)) * FMath::FRandRange(0.7f, 1.3f);
			Hallucinate();
		}
	}
	if (StepsLeft > 0)
	{
		NextStep -= DeltaTime;
		if (NextStep <= 0.f && FollowingSteps.Num() > 0)
		{
			NextStep = 0.55f;
			const FVector To = Character->GetActorLocation();
			StepFrom = FMath::Lerp(StepFrom, To, 0.18f);
			UGameplayStatics::PlaySoundAtLocation(this, FollowingSteps[FMath::RandRange(0, FollowingSteps.Num() - 1)], StepFrom, 0.7f, FMath::FRandRange(0.9f, 1.05f));
			--StepsLeft;
		}
	}

	// the view: colour fringes at the edges and grain as the mind frays, the light "breathing" below 35
	UCameraComponent* Camera = Character->FindComponentByClass<UCameraComponent>();
	if (!Camera)
	{
		return;
	}
	const float Target = FMath::Clamp((60.f - Sanity) / 60.f, 0.f, 1.f);
	Distortion = FMath::FInterpTo(Distortion, Target, DeltaTime, 0.5f);
	Pulse = FMath::FInterpTo(Pulse, 0.f, DeltaTime, 0.6f);
	const float Breath = Sanity < 35.f ? (0.5f + 0.5f * FMath::Sin(Time * 0.9f)) * FMath::Clamp((35.f - Sanity) / 35.f, 0.f, 1.f) : 0.f;
	FPostProcessSettings& PP = Camera->PostProcessSettings;
	PP.bOverride_SceneFringeIntensity = true;
	PP.SceneFringeIntensity = Distortion * 2.2f + Pulse * 3.f + Breath * 1.f;   // more would read as a glitch, not a mind
	PP.bOverride_FilmGrainIntensity = true;
	PP.FilmGrainIntensity = Distortion * 0.35f + Pulse * 0.3f;
	// exposure only while it breathes, so the level's own exposure is left alone otherwise
	const float Dim = 0.35f * Breath + 0.4f * Pulse;
	PP.bOverride_AutoExposureBias = Dim > 0.01f;
	PP.AutoExposureBias = -Dim;
}
