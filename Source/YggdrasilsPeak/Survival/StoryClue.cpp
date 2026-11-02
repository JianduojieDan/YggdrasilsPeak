// Copyright Epic Games, Inc. All Rights Reserved.

#include "Survival/StoryClue.h"

#include "Survival/ExpeditionComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Pawn.h"

AStoryClue::AStoryClue()
{
	PrimaryActorTick.bCanEverTick = false;

	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(GetRootComponent());

	Trigger = CreateDefaultSubobject<USphereComponent>(TEXT("Trigger"));
	Trigger->SetupAttachment(GetRootComponent());
	Trigger->InitSphereRadius(ReadRadius);
	Trigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Trigger->SetCollisionResponseToAllChannels(ECR_Ignore);
	Trigger->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	Trigger->SetGenerateOverlapEvents(true);
}

void AStoryClue::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	Trigger->SetSphereRadius(ReadRadius);
}

void AStoryClue::NotifyActorBeginOverlap(AActor* OtherActor)
{
	Super::NotifyActorBeginOverlap(OtherActor);
	const APawn* Pawn = Cast<APawn>(OtherActor);
	if (UExpeditionComponent* Expedition = Pawn && Pawn->IsPlayerControlled() ? Pawn->FindComponentByClass<UExpeditionComponent>() : nullptr)
	{
		Expedition->SetReadableClue(this);
	}
}

void AStoryClue::NotifyActorEndOverlap(AActor* OtherActor)
{
	Super::NotifyActorEndOverlap(OtherActor);
	const APawn* Pawn = Cast<APawn>(OtherActor);
	if (UExpeditionComponent* Expedition = Pawn ? Pawn->FindComponentByClass<UExpeditionComponent>() : nullptr)
	{
		Expedition->ClearReadableClue(this);
	}
}
