// Copyright Epic Games, Inc. All Rights Reserved.

#include "Survival/OxygenPickup.h"

#include "Survival/ExpeditionComponent.h"
#include "Survival/SurvivalComponent.h"
#include "Survival/SurvivalConfig.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

AOxygenPickup::AOxygenPickup()
{
	PrimaryActorTick.bCanEverTick = false;

	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Tank(TEXT("/Game/Fab/Oxygen_Tank/oxygen_tank/StaticMeshes/oxygen_tank.oxygen_tank"));
	Bottle = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Bottle"));
	Bottle->SetupAttachment(GetRootComponent());
	Bottle->SetStaticMesh(Tank.Object);
	Bottle->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// The imported bottle sits about a metre from its pivot, so centre the trigger on the bottle itself.
	Trigger = CreateDefaultSubobject<USphereComponent>(TEXT("Trigger"));
	Trigger->SetupAttachment(Bottle);
	Trigger->SetRelativeLocation(Tank.Object ? Tank.Object->GetBounds().Origin : FVector::ZeroVector);
	Trigger->InitSphereRadius(110.f);
	Trigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Trigger->SetCollisionResponseToAllChannels(ECR_Ignore);
	Trigger->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	Trigger->SetGenerateOverlapEvents(true);
}

void AOxygenPickup::NotifyActorBeginOverlap(AActor* OtherActor)
{
	Super::NotifyActorBeginOverlap(OtherActor);

	const APawn* Pawn = Cast<APawn>(OtherActor);
	USurvivalComponent* Survival = Pawn && Pawn->IsPlayerControlled() ? Pawn->FindComponentByClass<USurvivalComponent>() : nullptr;
	if (!Survival || Survival->IsDead() || !Bottle->IsVisible())
	{
		return;
	}

	UExpeditionComponent* Expedition = Pawn->FindComponentByClass<UExpeditionComponent>();
	if (!Survival->AddCarriedBottle())
	{
		if (Expedition && GetWorld()->GetTimeSeconds() >= NextFullMessageTime)
		{
			NextFullMessageTime = GetWorld()->GetTimeSeconds() + 30.f;
			Expedition->ShowMessage(NSLOCTEXT("Expedition", "BottleFull", "Your pack is full of oxygen bottles."), 3.f);
		}
		return;
	}

	if (Expedition)
	{
		Expedition->ShowMessage(FText::Format(NSLOCTEXT("Expedition", "BottleTaken", "Oxygen bottle packed ({0}/{1}). It goes on by itself when the air gets thin."),
			FText::AsNumber(Survival->GetCarriedBottles()), FText::AsNumber(GetDefault<USurvivalConfig>()->MaxCarriedBottles)), 5.f);
	}
	SetAvailable(false);
	if (RespawnSeconds > 0.f)
	{
		GetWorldTimerManager().SetTimer(RespawnTimer, FTimerDelegate::CreateUObject(this, &AOxygenPickup::SetAvailable, true), RespawnSeconds, false);
	}
}

void AOxygenPickup::SetAvailable(bool bAvailable)
{
	Bottle->SetVisibility(bAvailable);
	Trigger->SetCollisionEnabled(bAvailable ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
}
