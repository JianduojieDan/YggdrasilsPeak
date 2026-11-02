// Copyright Epic Games, Inc. All Rights Reserved.

#include "Environment/MountainRumble.h"

#include "Survival/CompassComponent.h"
#include "Survival/ExpeditionComponent.h"
#include "Components/BoxComponent.h"
#include "EngineUtils.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

static FAutoConsoleCommandWithWorld CmdBoom(TEXT("ygg.Boom"), TEXT("A boom inside the mountain now (Western Cwm rumble)."),
	FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World)
	{
		for (TActorIterator<AMountainRumble> It(World); It; ++It)
		{
			It->Strike();
			return;
		}
	}));

AMountainRumble::AMountainRumble()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.f;
	Area = CreateDefaultSubobject<UBoxComponent>(TEXT("Area"));
	SetRootComponent(Area);
	Area->SetBoxExtent(FVector(50000.f, 30000.f, 30000.f));
	Area->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AMountainRumble::BeginPlay()
{
	Super::BeginPlay();
	for (int32 i = 1; Booms.Num() < 3 && i <= 3; ++i)
	{
		const FString N = FString::Printf(TEXT("Mountain_Boom_%02d"), i);
		if (USoundBase* S = LoadObject<USoundBase>(nullptr, *(FString(TEXT("/Game/Audio/Ice/")) + N + TEXT(".") + N)))
		{
			Booms.Add(S);
		}
	}
	Next = FirstDelay;
}

void AMountainRumble::Strike()
{
	if (!Climber.IsValid())
	{
		Climber = UGameplayStatics::GetPlayerPawn(this, 0);
	}
	APawn* Pawn = Climber.Get();
	if (!Pawn)
	{
		return;
	}
	++Count;
	// from deep below and a little to one side: felt more than heard
	const FVector Under = Pawn->GetActorLocation() + FRotator(0.f, FMath::FRandRange(0.f, 360.f), 0.f).Vector() * 1500.f - FVector(0.f, 0.f, 3000.f);
	if (Booms.Num() > 0)
	{
		UGameplayStatics::PlaySoundAtLocation(this, Booms[FMath::RandRange(0, Booms.Num() - 1)], Under, 1.6f, FMath::FRandRange(0.85f, 1.05f));
	}
	Shake = 1.f;
	ShakeTime = 0.f;
	if (UCompassComponent* Compass = Pawn->FindComponentByClass<UCompassComponent>())
	{
		Compass->Kick(0.8f, Count <= 2);
	}
	if (UExpeditionComponent* Expedition = Pawn->FindComponentByClass<UExpeditionComponent>())
	{
		if (Count == 1)
		{
			Expedition->ShowMessage(NSLOCTEXT("Rumble", "First", "Deep inside the mountain something strikes. Once. The ground shivers under your boots... then nothing."), 6.f);
			Expedition->MarkStoryEvent(TEXT("FirstBoom"));
		}
		else if (Count == 2)
		{
			Expedition->ShowMessage(NSLOCTEXT("Rumble", "Second", "Again. An avalanche somewhere? It came from below."), 5.f);
		}
	}
}

void AMountainRumble::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	APawn* Pawn = UGameplayStatics::GetPlayerPawn(this, 0);
	AController* Controller = Pawn ? Pawn->GetController() : nullptr;

	// the trembling: a decaying shake of the view, taken back out exactly as it was put in
	if (Controller && (Shake > 0.f || !Applied.IsZero()))
	{
		ShakeTime += DeltaSeconds;
		Shake = FMath::Max(0.f, Shake - DeltaSeconds / 3.f);
		const float A = ShakeDegrees * Shake * Shake;
		const FRotator Want(FMath::PerlinNoise1D(ShakeTime * 11.f) * A, FMath::PerlinNoise1D(ShakeTime * 9.f + 7.f) * A * 0.6f, FMath::PerlinNoise1D(ShakeTime * 13.f + 3.f) * A * 0.8f);
		FRotator R = Controller->GetControlRotation();
		R += Want - Applied;
		Applied = Want;
		Controller->SetControlRotation(R);
	}

	if (!Pawn)
	{
		return;
	}
	const FVector Local = Area->GetComponentTransform().InverseTransformPosition(Pawn->GetActorLocation());
	const FVector Extent = Area->GetUnscaledBoxExtent();
	const bool bInside = FMath::Abs(Local.X) < Extent.X && FMath::Abs(Local.Y) < Extent.Y && FMath::Abs(Local.Z) < Extent.Z;
	if (!bInside)
	{
		return;
	}
	Climber = Pawn;
	Next -= DeltaSeconds;
	if (Next <= 0.f)
	{
		Next = FMath::FRandRange(Interval.X, Interval.Y);
		Strike();
	}
}
