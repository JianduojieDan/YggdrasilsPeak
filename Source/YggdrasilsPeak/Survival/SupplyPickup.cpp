// Copyright Epic Games, Inc. All Rights Reserved.

#include "Survival/SupplyPickup.h"

#include "Survival/ExpeditionComponent.h"
#include "Survival/InventoryComponent.h"
#include "Survival/SurvivalComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "TimerManager.h"

ASupplyPickup::ASupplyPickup()
{
	PrimaryActorTick.bCanEverTick = false;

	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(GetRootComponent());
	Mesh->SetCollisionProfileName(TEXT("BlockAll"));

	Trigger = CreateDefaultSubobject<USphereComponent>(TEXT("Trigger"));
	Trigger->SetupAttachment(GetRootComponent());
	Trigger->InitSphereRadius(110.f);
	Trigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Trigger->SetCollisionResponseToAllChannels(ECR_Ignore);
	Trigger->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	Trigger->SetGenerateOverlapEvents(true);
}

void ASupplyPickup::NotifyActorBeginOverlap(AActor* OtherActor)
{
	Super::NotifyActorBeginOverlap(OtherActor);

	const APawn* Pawn = Cast<APawn>(OtherActor);
	UInventoryComponent* Pack = Pawn && Pawn->IsPlayerControlled() ? Pawn->FindComponentByClass<UInventoryComponent>() : nullptr;
	const USurvivalComponent* Survival = Pawn ? Pawn->FindComponentByClass<USurvivalComponent>() : nullptr;
	if (!Pack || (Survival && Survival->IsDead()) || !Mesh->IsVisible())
	{
		return;
	}
	if (Remaining <= 0)
	{
		Remaining = Count;
	}

	UExpeditionComponent* Expedition = Pawn->FindComponentByClass<UExpeditionComponent>();
	const FPackItemDef* Def = Pack->ItemDefs.Find(Item);
	const FText Name = Def ? Def->DisplayName : FText::FromName(Item);
	const int32 Packed = Pack->Add(Item, Remaining);
	if (Packed == 0)
	{
		if (Expedition && GetWorld()->GetTimeSeconds() >= NextFullMessageTime)
		{
			NextFullMessageTime = GetWorld()->GetTimeSeconds() + 30.f;
			Expedition->ShowMessage(FText::Format(NSLOCTEXT("Pack", "Full", "Your pack is too heavy for another {0} ({1} / {2} kg)."),
				Name, FText::AsNumber(FMath::RoundToInt(Pack->GetLoadKg())), FText::AsNumber(FMath::RoundToInt(Pack->MaxKg))), 4.f);
		}
		return;
	}

	Remaining -= Packed;
	if (Expedition)
	{
		Expedition->ShowMessage(FText::Format(NSLOCTEXT("Pack", "Packed", "Packed {0} × {1}. Pack {2} / {3} kg."), FText::AsNumber(Packed), Name,
			FText::AsNumber(FMath::RoundToInt(Pack->GetLoadKg())), FText::AsNumber(FMath::RoundToInt(Pack->MaxKg))), 4.f);
	}
	if (Remaining <= 0)
	{
		SetAvailable(false);
		if (RespawnSeconds > 0.f)
		{
			GetWorldTimerManager().SetTimer(RespawnTimer, FTimerDelegate::CreateUObject(this, &ASupplyPickup::SetAvailable, true), RespawnSeconds, false);
		}
	}
}

void ASupplyPickup::SetAvailable(bool bAvailable)
{
	Mesh->SetVisibility(bAvailable);
	Mesh->SetCollisionEnabled(bAvailable ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
	Trigger->SetCollisionEnabled(bAvailable ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
}
