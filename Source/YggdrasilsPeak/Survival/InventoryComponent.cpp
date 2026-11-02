// Copyright Epic Games, Inc. All Rights Reserved.

#include "Survival/InventoryComponent.h"

#include "Survival/ExpeditionComponent.h"
#include "Survival/SurvivalComponent.h"
#include "GameFramework/Actor.h"

UInventoryComponent::UInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.5f;

	FPackItemDef Ration;
	Ration.DisplayName = NSLOCTEXT("Pack", "Ration", "ration");
	Ration.WeightKg = 0.6f;
	Ration.Satiety = 35.f;
	ItemDefs.Add(TEXT("Ration"), Ration);

	FPackItemDef Water;
	Water.DisplayName = NSLOCTEXT("Pack", "Water", "water bottle");
	Water.WeightKg = 1.f;
	Water.Hydration = 40.f;
	ItemDefs.Add(TEXT("Water"), Water);

	StartingItems.Add(TEXT("Ration"), 3);
	StartingItems.Add(TEXT("Water"), 2);
}

void UInventoryComponent::BeginPlay()
{
	Super::BeginPlay();
	Survival = GetOwner()->FindComponentByClass<USurvivalComponent>();
	Items = StartingItems;
	CheckpointItems = Items;
	OnChanged.Broadcast();
}

float UInventoryComponent::GetLoadKg() const
{
	float Kg = BaseGearKg;
	for (const TPair<FName, int32>& It : Items)
	{
		if (const FPackItemDef* Def = ItemDefs.Find(It.Key))
		{
			Kg += Def->WeightKg * It.Value;
		}
	}
	if (Survival.IsValid())
	{
		Kg += OxygenBottleKg * Survival->GetCarriedBottles();
	}
	return Kg;
}

bool UInventoryComponent::CanCarry(FName Item, int32 Count) const
{
	const FPackItemDef* Def = ItemDefs.Find(Item);
	return Def && CanCarryKg(Def->WeightKg * Count);
}

int32 UInventoryComponent::Add(FName Item, int32 Count)
{
	int32 Packed = 0;
	while (Packed < Count && CanCarry(Item))
	{
		Items.FindOrAdd(Item) += 1;
		++Packed;
	}
	if (Packed > 0)
	{
		OnChanged.Broadcast();
	}
	return Packed;
}

bool UInventoryComponent::Use(FName Item)
{
	const FPackItemDef* Def = ItemDefs.Find(Item);
	int32* Have = Items.Find(Item);
	if (!Def || !Survival.IsValid() || Survival->IsDead())
	{
		return false;
	}
	if (!Have || *Have <= 0)
	{
		Say(FText::Format(NSLOCTEXT("Pack", "NoneLeft", "No {0} left in the pack."), Def->DisplayName));
		return false;
	}
	// Don't waste supplies on a full stomach.
	const bool bUseful = (Def->Satiety > 0.f && Survival->GetStatValue(ESurvivalStat::Satiety) < Survival->GetStatMax(ESurvivalStat::Satiety) - 5.f)
		|| (Def->Hydration > 0.f && Survival->GetStatValue(ESurvivalStat::Hydration) < Survival->GetStatMax(ESurvivalStat::Hydration) - 5.f)
		|| Def->BodyTemperature > 0.f;
	if (!bUseful)
	{
		Say(NSLOCTEXT("Pack", "NotNeeded", "You don't need that right now."), 2.5f);
		return false;
	}

	*Have -= 1;
	if (*Have <= 0)
	{
		Items.Remove(Item);
	}
	if (Def->Satiety != 0.f)
	{
		Survival->AddToStat(ESurvivalStat::Satiety, Def->Satiety);
	}
	if (Def->Hydration != 0.f)
	{
		Survival->AddToStat(ESurvivalStat::Hydration, Def->Hydration);
	}
	if (Def->BodyTemperature != 0.f)
	{
		Survival->AddToStat(ESurvivalStat::BodyTemperature, Def->BodyTemperature);
	}
	Say(FText::Format(NSLOCTEXT("Pack", "Used", "Used a {0} ({1} left)."), Def->DisplayName, FText::AsNumber(GetCount(Item))), 3.f);
	OnChanged.Broadcast();
	return true;
}

float UInventoryComponent::GetSpeedMultiplier() const
{
	const float Over = FMath::Clamp((GetLoadKg() - ComfortableKg) / FMath::Max(MaxKg - ComfortableKg, 1.f), 0.f, 1.f);
	return FMath::Lerp(1.f, FullPackSpeedMultiplier, Over);
}

void UInventoryComponent::SetItems(const TMap<FName, int32>& NewItems)
{
	Items = NewItems;
	OnChanged.Broadcast();
}

void UInventoryComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (Survival.IsValid())
	{
		Survival->SetLoadSpeedMultiplier(GetSpeedMultiplier());
	}
}

void UInventoryComponent::Say(const FText& Message, float Seconds) const
{
	if (UExpeditionComponent* Expedition = GetOwner()->FindComponentByClass<UExpeditionComponent>())
	{
		Expedition->ShowMessage(Message, Seconds);
	}
}
