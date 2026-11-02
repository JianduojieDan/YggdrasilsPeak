// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SeracCollapseZone.generated.h"

class UBoxComponent;
class USoundBase;
class UStaticMesh;

/**
 *  A stretch of the Icefall route under leaning seracs (Phase1_Design_Plan S3 danger table: "Serac collapse -
 *  lingering in a marked zone - cracking sound, falling ice chips - move through quickly - death if caught").
 *  The ends are marked by red pickets (Tools/RoutePlanning/place_hazards.py). Inside, the ice creaks and cracks
 *  and chips start to fall; stay longer than LingerSeconds and the groaning becomes a warning, then a collapse:
 *  blocks come down, and if you are still inside, you are buried. Moving through without stopping is safe.
 */
UCLASS()
class YGGDRASILSPEAK_API ASeracCollapseZone : public AActor
{
	GENERATED_BODY()

public:

	ASeracCollapseZone();

	UPROPERTY(VisibleAnywhere, Category="Serac Zone")
	TObjectPtr<UBoxComponent> Zone;

	/** Seconds inside before the warning (walking through takes ~15-20 s at Icefall pace). */
	UPROPERTY(EditAnywhere, Category="Serac Zone", meta=(ClampMin="1"))
	float LingerSeconds = 30.f;

	/** Seconds between the warning and the collapse: enough to run out if you move at once. */
	UPROPERTY(EditAnywhere, Category="Serac Zone", meta=(ClampMin="1"))
	float WarningSeconds = 5.f;

	/** After a collapse the zone is quiet this long. */
	UPROPERTY(EditAnywhere, Category="Serac Zone", meta=(ClampMin="0"))
	float RearmSeconds = 90.f;

	UPROPERTY(EditAnywhere, Category="Serac Zone")
	TArray<TObjectPtr<UStaticMesh>> BlockMeshes;

	UPROPERTY(EditAnywhere, Category="Serac Zone", meta=(ClampMin="0"))
	int32 BlocksPerCollapse = 7;

	UPROPERTY(EditAnywhere, Category="Serac Zone")
	TArray<TObjectPtr<USoundBase>> Cracks;
	UPROPERTY(EditAnywhere, Category="Serac Zone")
	TArray<TObjectPtr<USoundBase>> Groans;
	UPROPERTY(EditAnywhere, Category="Serac Zone")
	TArray<TObjectPtr<USoundBase>> CollapseSounds;

	virtual void Tick(float DeltaSeconds) override;

protected:

	virtual void BeginPlay() override;

private:

	UFUNCTION()
	void OnEnter(UPrimitiveComponent* Overlapped, AActor* Other, UPrimitiveComponent* OtherComp, int32 BodyIndex, bool bFromSweep, const FHitResult& Sweep);
	UFUNCTION()
	void OnLeave(UPrimitiveComponent* Overlapped, AActor* Other, UPrimitiveComponent* OtherComp, int32 BodyIndex);

	void PlayAround(const TArray<TObjectPtr<USoundBase>>& Sounds, float Volume, float Distance) const;
	void DropIce(int32 Count, float MinScale, float MaxScale, float Spread, float Life) const;
	void Say(const FText& Message, float Seconds) const;

	TWeakObjectPtr<APawn> Climber;
	float Timer = 0.f;
	float NextSound = 0.f;
	float NextChips = 0.f;
	float WarningLeft = -1.f;
	float Cooldown = 0.f;
	bool bWarnedEntry = false;
};
