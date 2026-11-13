// Copyright Epic Games, Inc. All Rights Reserved.

#include "Environment/SeracCollapseZone.h"

#include "Survival/ExpeditionComponent.h"
#include "Survival/SurvivalComponent.h"
#include "Survival/YggTime.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/PackageName.h"
#include "Sound/SoundBase.h"

ASeracCollapseZone::ASeracCollapseZone()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;   // only while someone is inside
	Zone = CreateDefaultSubobject<UBoxComponent>(TEXT("Zone"));
	SetRootComponent(Zone);
	Zone->SetBoxExtent(FVector(2200.f, 1500.f, 2000.f));
	Zone->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
}

void ASeracCollapseZone::BeginPlay()
{
	Super::BeginPlay();
	auto Load = [](TArray<TObjectPtr<USoundBase>>& Into, const TCHAR* Name, int32 Count)
	{
		for (int32 i = 1; Into.Num() < Count && i <= Count; ++i)
		{
			const FString N = FString::Printf(TEXT("%s_%02d"), Name, i);
			// the sets are numbered up to a maximum; asking the loader for a missing one logs a warning in the packaged game
			if (!FPackageName::DoesPackageExist(FString(TEXT("/Game/Audio/Ice/")) + N)) { continue; }
			if (USoundBase* S = LoadObject<USoundBase>(nullptr, *(FString(TEXT("/Game/Audio/Ice/")) + N + TEXT(".") + N), nullptr, LOAD_NoWarn | LOAD_Quiet))
			{
				Into.Add(S);
			}
		}
	};
	Load(Cracks, TEXT("Ice_Crack"), 12);
	Load(Groans, TEXT("Ice_Groan"), 6);
	Load(CollapseSounds, TEXT("Serac_Collapse"), 4);
	for (int32 i = 1; BlockMeshes.Num() < 6 && i <= 6; ++i)
	{
		const FString N = FString::Printf(TEXT("SM_Serac_%02d"), i);
		if (UStaticMesh* M = LoadObject<UStaticMesh>(nullptr, *(FString(TEXT("/Game/Environment/Glacier/Seracs/")) + N + TEXT(".") + N)))
		{
			BlockMeshes.Add(M);
		}
	}
	Zone->OnComponentBeginOverlap.AddDynamic(this, &ASeracCollapseZone::OnEnter);
	Zone->OnComponentEndOverlap.AddDynamic(this, &ASeracCollapseZone::OnLeave);
}

void ASeracCollapseZone::OnEnter(UPrimitiveComponent*, AActor* Other, UPrimitiveComponent*, int32, bool, const FHitResult&)
{
	APawn* Pawn = Cast<APawn>(Other);
	if (!Pawn || !Pawn->IsPlayerControlled())
	{
		return;
	}
	Climber = Pawn;
	SetActorTickEnabled(true);
	NextSound = 1.5f;
	if (!bWarnedEntry)
	{
		bWarnedEntry = true;
		const float Tod = YggTime::Get(GetWorld());
		Say(Tod >= 1100.f && Tod < 1800.f
			? NSLOCTEXT("Serac", "EnterWarm", "Seracs lean over the route here, dripping in the afternoon sun. Don't stop until the red pickets.")
			: NSLOCTEXT("Serac", "Enter", "Seracs lean over the route here. Don't stop until the red pickets."), 5.f);
	}
}

void ASeracCollapseZone::OnLeave(UPrimitiveComponent*, AActor* Other, UPrimitiveComponent*, int32)
{
	if (Other && Other == Climber.Get() && WarningLeft < 0.f)
	{
		Climber.Reset();
	}
}

void ASeracCollapseZone::Say(const FText& Message, float Seconds) const
{
	if (Climber.IsValid())
	{
		if (UExpeditionComponent* Expedition = Climber->FindComponentByClass<UExpeditionComponent>())
		{
			Expedition->ShowMessage(Message, Seconds);
		}
	}
}

void ASeracCollapseZone::PlayAround(const TArray<TObjectPtr<USoundBase>>& Sounds, float Volume, float Distance) const
{
	if (Sounds.Num() == 0 || !Climber.IsValid())
	{
		return;
	}
	const FVector Where = Climber->GetActorLocation() + FRotator(0.f, FMath::FRandRange(0.f, 360.f), 0.f).Vector() * Distance + FVector(0.f, 0.f, FMath::FRandRange(300.f, 1200.f));
	UGameplayStatics::PlaySoundAtLocation(this, Sounds[FMath::RandRange(0, Sounds.Num() - 1)], Where, Volume, FMath::FRandRange(0.9f, 1.1f));
}

void ASeracCollapseZone::DropIce(int32 Count, float MinScale, float MaxScale, float Spread, float Life) const
{
	if (BlockMeshes.Num() == 0 || !Climber.IsValid())
	{
		return;
	}
	const FVector Centre = Climber->GetActorLocation() + Climber->GetVelocity().GetSafeNormal2D() * 300.f;
	for (int32 i = 0; i < Count; ++i)
	{
		const FVector At = Centre + FVector(FMath::FRandRange(-Spread, Spread), FMath::FRandRange(-Spread, Spread), FMath::FRandRange(900.f, 1600.f));
		FActorSpawnParameters P;
		P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		AStaticMeshActor* Block = GetWorld()->SpawnActor<AStaticMeshActor>(At, FRotator(FMath::FRandRange(0.f, 360.f), FMath::FRandRange(0.f, 360.f), 0.f), P);
		if (!Block)
		{
			continue;
		}
		UStaticMeshComponent* M = Block->GetStaticMeshComponent();
		M->SetMobility(EComponentMobility::Movable);
		M->SetStaticMesh(BlockMeshes[FMath::RandRange(0, BlockMeshes.Num() - 1)]);
		Block->SetActorScale3D(FVector(FMath::FRandRange(MinScale, MaxScale)));
		M->SetCollisionProfileName(TEXT("PhysicsActor"));
		M->SetSimulatePhysics(true);
		M->SetLinearDamping(0.05f);
		M->SetPhysicsLinearVelocity(FVector(0.f, 0.f, -600.f));
		Block->SetLifeSpan(Life);
	}
}

void ASeracCollapseZone::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	APawn* Pawn = Climber.Get();
	if (!Pawn)
	{
		// nobody inside: the strain eases off
		Timer = FMath::Max(0.f, Timer - DeltaSeconds * 2.f);
		Cooldown = FMath::Max(0.f, Cooldown - DeltaSeconds);
		if (Timer <= 0.f && Cooldown <= 0.f)
		{
			SetActorTickEnabled(false);
		}
		return;
	}
	const USurvivalComponent* Survival = Pawn->FindComponentByClass<USurvivalComponent>();
	if (Survival && Survival->IsDead())
	{
		Climber.Reset();
		WarningLeft = -1.f;
		return;
	}
	if (Cooldown > 0.f)
	{
		Cooldown -= DeltaSeconds;
		return;
	}

	// the warning: groans and cracks close by, chips raining; then the collapse
	if (WarningLeft >= 0.f)
	{
		WarningLeft -= DeltaSeconds;
		NextChips -= DeltaSeconds;
		if (NextChips <= 0.f)
		{
			NextChips = 0.35f;
			DropIce(2, 0.02f, 0.06f, 600.f, 6.f);
			PlayAround(Cracks, 1.2f, 600.f);
		}
		if (WarningLeft <= 0.f)
		{
			WarningLeft = -1.f;
			Cooldown = RearmSeconds;
			Timer = 0.f;
			PlayAround(CollapseSounds, 1.6f, 300.f);
			DropIce(BlocksPerCollapse, 0.12f, 0.35f, 900.f, 25.f);
			if (Zone->IsOverlappingActor(Pawn))
			{
				if (USurvivalComponent* S = Pawn->FindComponentByClass<USurvivalComponent>())
				{
					S->KillBy(NSLOCTEXT("Serac", "Buried", "The serac came down on you."));
				}
			}
			else
			{
				Say(NSLOCTEXT("Serac", "Escaped", "Behind you the ice tower comes down with a roar."), 4.f);
			}
			if (!Zone->IsOverlappingActor(Pawn))
			{
				Climber.Reset();
			}
		}
		return;
	}

	// inside and calm-ish: the longer you stay, the louder and closer the ice talks. The sun loosens the ice:
	// in the warm afternoon it gives way much sooner, in the cold early hours it holds longer.
	const float Tod = YggTime::Get(GetWorld());
	const float Warmth = Tod >= 1100.f && Tod < 1800.f ? 1.7f : (Tod < 800.f ? 0.7f : 1.f);
	Timer += DeltaSeconds * Warmth;
	const float Strain = FMath::Clamp(Timer / LingerSeconds, 0.f, 1.f);
	NextSound -= DeltaSeconds;
	if (NextSound <= 0.f)
	{
		NextSound = FMath::FRandRange(4.f, 7.f) * (1.f - 0.6f * Strain);
		if (FMath::FRand() < 0.35f)
		{
			PlayAround(Groans, 0.5f + 0.6f * Strain, FMath::Lerp(2500.f, 900.f, Strain));
		}
		else
		{
			PlayAround(Cracks, 0.4f + 0.7f * Strain, FMath::Lerp(2500.f, 900.f, Strain));
		}
	}
	if (Strain > 0.5f)
	{
		NextChips -= DeltaSeconds;
		if (NextChips <= 0.f)
		{
			NextChips = FMath::FRandRange(1.f, 2.5f) * (1.5f - Strain);
			DropIce(1, 0.02f, 0.05f, 700.f, 6.f);
		}
	}
	if (Timer >= LingerSeconds)
	{
		WarningLeft = WarningSeconds;
		NextChips = 0.f;
		PlayAround(Groans, 1.5f, 400.f);
		Say(NSLOCTEXT("Serac", "Warning", "The ice above you groans - MOVE!"), 3.f);
	}
}
