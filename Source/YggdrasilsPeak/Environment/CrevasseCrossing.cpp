// Copyright Epic Games, Inc. All Rights Reserved.

#include "Environment/CrevasseCrossing.h"

#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/Controller.h"
#include "Survival/ExpeditionComponent.h"
#include "Survival/SurvivalComponent.h"

ACrevasseCrossing::ACrevasseCrossing()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;   // only while someone is on the ladder
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));

	auto MakeMesh = [this](const TCHAR* Name, bool bCollide)
	{
		UStaticMeshComponent* M = CreateDefaultSubobject<UStaticMeshComponent>(Name);
		M->SetupAttachment(GetRootComponent());
		M->SetCollisionProfileName(bCollide ? UCollisionProfile::BlockAll_ProfileName : UCollisionProfile::NoCollision_ProfileName);
		return M;
	};
	LipNear = MakeMesh(TEXT("LipNear"), true);
	LipFar = MakeMesh(TEXT("LipFar"), true);
	Ladder = MakeMesh(TEXT("Ladder"), true);
	Abyss = MakeMesh(TEXT("Abyss"), false);
	Abyss->SetCastShadow(false);

	LadderZone = CreateDefaultSubobject<UBoxComponent>(TEXT("LadderZone"));
	LadderZone->SetupAttachment(GetRootComponent());
	LadderZone->SetBoxExtent(FVector(400.f, 60.f, 120.f));
	LadderZone->SetCollisionProfileName(TEXT("OverlapAllDynamic"));

	FallZone = CreateDefaultSubobject<UBoxComponent>(TEXT("FallZone"));
	FallZone->SetupAttachment(GetRootComponent());
	FallZone->SetBoxExtent(FVector(150.f, 1500.f, 300.f));
	FallZone->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
}

void ACrevasseCrossing::BeginPlay()
{
	Super::BeginPlay();
	LadderZone->OnComponentBeginOverlap.AddDynamic(this, &ACrevasseCrossing::OnLadderBegin);
	LadderZone->OnComponentEndOverlap.AddDynamic(this, &ACrevasseCrossing::OnLadderEnd);
	FallZone->OnComponentBeginOverlap.AddDynamic(this, &ACrevasseCrossing::OnFall);
}

void ACrevasseCrossing::EndPlay(const EEndPlayReason::Type Reason)
{
	if (Climber.IsValid())
	{
		SetOnLadder(Climber.Get(), false);
	}
	Super::EndPlay(Reason);
}

void ACrevasseCrossing::OnLadderBegin(UPrimitiveComponent*, AActor* Other, UPrimitiveComponent*, int32, bool, const FHitResult&)
{
	APawn* Pawn = Cast<APawn>(Other);
	if (Pawn && Pawn->IsPlayerControlled())
	{
		SetOnLadder(Pawn, true);
	}
}

void ACrevasseCrossing::OnLadderEnd(UPrimitiveComponent*, AActor* Other, UPrimitiveComponent*, int32)
{
	if (Other && Other == Climber.Get())
	{
		SetOnLadder(Climber.Get(), false);
	}
}

void ACrevasseCrossing::OnFall(UPrimitiveComponent*, AActor* Other, UPrimitiveComponent*, int32, bool, const FHitResult&)
{
	APawn* Pawn = Cast<APawn>(Other);
	if (!Pawn || !Pawn->IsPlayerControlled())
	{
		return;
	}
	if (USurvivalComponent* Survival = Pawn->FindComponentByClass<USurvivalComponent>())
	{
		Survival->KillBy(NSLOCTEXT("Crevasse", "Fell", "You fell into a crevasse."));
	}
}

void ACrevasseCrossing::SetOnLadder(APawn* Pawn, bool bOn)
{
	if (USurvivalComponent* Survival = Pawn->FindComponentByClass<USurvivalComponent>())
	{
		Survival->SetFootingSpeedMultiplier(bOn ? LadderSpeedMultiplier : 1.f);
	}
	if (bOn)
	{
		Climber = Pawn;
		SetActorTickEnabled(true);
		if (!bWarned)
		{
			bWarned = true;
			if (UExpeditionComponent* Expedition = Pawn->FindComponentByClass<UExpeditionComponent>())
			{
				Expedition->ShowMessage(NSLOCTEXT("Crevasse", "Ladder", "Ladder over a crevasse - slow steps, don't look down."), 4.f);
			}
		}
		return;
	}
	// level the view again
	if (AController* Controller = Pawn->GetController())
	{
		FRotator R = Controller->GetControlRotation();
		R.Roll = 0.f;
		Controller->SetControlRotation(R);
	}
	SwayRoll = 0.f;
	Climber.Reset();
	SetActorTickEnabled(false);
}

void ACrevasseCrossing::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	APawn* Pawn = Climber.Get();
	AController* Controller = Pawn ? Pawn->GetController() : nullptr;
	if (!Controller)
	{
		return;
	}
	// The ladder flexes under each step and the wind pushes: two slow sines, stronger when moving and in wind.
	const UExpeditionComponent* Expedition = Pawn->FindComponentByClass<UExpeditionComponent>();
	const float Wind = Expedition ? FMath::Clamp(Expedition->GetWindIntensity(), 0.f, 1.f) : 0.f;
	const float Moving = FMath::Clamp(Pawn->GetVelocity().Size2D() / 150.f, 0.f, 1.f);
	SwayTime += DeltaSeconds;
	const float Amp = SwayDegrees * (0.5f + Moving) + WindSwayDegrees * Wind;
	const float Target = Amp * (0.7f * FMath::Sin(SwayTime * 1.7f) + 0.3f * FMath::Sin(SwayTime * 4.3f + 1.1f));
	SwayRoll = FMath::FInterpTo(SwayRoll, Target, DeltaSeconds, 4.f);
	FRotator R = Controller->GetControlRotation();
	R.Roll = SwayRoll;
	Controller->SetControlRotation(R);
}
