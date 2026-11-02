// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InventoryComponent.generated.h"

class USurvivalComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInventoryChanged);

/** One kind of supply the climber can carry (no crafting: things are packed, carried and used up). */
USTRUCT(BlueprintType)
struct FPackItemDef
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Pack")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Pack", meta=(ClampMin="0"))
	float WeightKg = 0.5f;

	/** Added to the survival stats when used (percent / degrees). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Pack")
	float Satiety = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Pack")
	float Hydration = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Pack")
	float BodyTemperature = 0.f;
};

/**
 *  The climber's pack: supplies by id and count, and the weight of everything carried (gear, supplies, oxygen
 *  bottles). A heavier pack slows the climber (through USurvivalComponent's speed) - carrying more is safer but
 *  slower, which is the preparation decision at every camp. Camps store a checkpoint copy; dying rolls the pack
 *  back to it (UExpeditionComponent).
 */
UCLASS(ClassGroup=(Survival), meta=(BlueprintSpawnableComponent))
class YGGDRASILSPEAK_API UInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	UInventoryComponent();

	/** What each supply id is. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Pack")
	TMap<FName, FPackItemDef> ItemDefs;

	/** Packed at the start of a new game. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Pack")
	TMap<FName, int32> StartingItems;

	/** Tent, stove, harness, axe, clothes in the pack: always carried. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Pack", meta=(ClampMin="0"))
	float BaseGearKg = 10.f;

	/** A full oxygen bottle (the survival component counts them). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Pack", meta=(ClampMin="0"))
	float OxygenBottleKg = 3.f;

	/** Up to this load the pack costs no speed... */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Pack", meta=(ClampMin="0"))
	float ComfortableKg = 16.f;

	/** ...and nothing more fits above this. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Pack", meta=(ClampMin="1"))
	float MaxKg = 26.f;

	/** Walking speed multiplier with a full pack (MaxKg). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Pack", meta=(ClampMin="0.1", ClampMax="1"))
	float FullPackSpeedMultiplier = 0.75f;

	UFUNCTION(BlueprintPure, Category="Pack")
	int32 GetCount(FName Item) const { return Items.FindRef(Item); }

	UFUNCTION(BlueprintPure, Category="Pack")
	float GetLoadKg() const;

	/** True if Count more of Item would still fit under MaxKg. */
	UFUNCTION(BlueprintPure, Category="Pack")
	bool CanCarry(FName Item, int32 Count = 1) const;

	/** True if ExtraKg more would still fit (oxygen bottles use this). */
	UFUNCTION(BlueprintPure, Category="Pack")
	bool CanCarryKg(float ExtraKg) const { return GetLoadKg() + ExtraKg <= MaxKg + KINDA_SMALL_NUMBER; }

	/** Adds up to Count of Item (as many as fit); returns how many were packed. */
	UFUNCTION(BlueprintCallable, Category="Pack")
	int32 Add(FName Item, int32 Count = 1);

	/** Eats / drinks one Item and applies its effect. False if none is left or it would do nothing. */
	UFUNCTION(BlueprintCallable, Category="Pack")
	bool Use(FName Item);

	/** Speed multiplier from the current load (1 up to ComfortableKg). */
	UFUNCTION(BlueprintPure, Category="Pack")
	float GetSpeedMultiplier() const;

	const TMap<FName, int32>& GetItems() const { return Items; }
	/** Replaces the pack's contents (save game / checkpoint restore). */
	void SetItems(const TMap<FName, int32>& NewItems);

	/** Copies the pack as the checkpoint (camps call this). */
	void SaveCheckpoint() { CheckpointItems = Items; }
	/** Rolls the pack back to the last checkpoint (after a death). */
	void RestoreCheckpoint() { SetItems(CheckpointItems); }

	UPROPERTY(BlueprintAssignable, Category="Pack")
	FOnInventoryChanged OnChanged;

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:

	virtual void BeginPlay() override;

private:

	void Say(const FText& Message, float Seconds = 4.f) const;

	TMap<FName, int32> Items;
	TMap<FName, int32> CheckpointItems;
	TWeakObjectPtr<USurvivalComponent> Survival;
};
