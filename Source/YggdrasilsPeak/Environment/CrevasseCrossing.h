// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CrevasseCrossing.generated.h"

class UBoxComponent;
class UStaticMeshComponent;

/**
 *  A crevasse across the route with an aluminium ladder over it (Khumbu Icefall, Western Cwm). Two ice lips with
 *  snow ramps on the outside and sheer walls into a dark slot; the ladder rests on both lips. On the ladder you
 *  walk slowly, cannot sprint and the view sways (more in wind); dropping into the slot kills you like any other
 *  death (back to the last checkpoint). The layout (lips, ladder, slot) is placed by
 *  Tools/RoutePlanning/place_crevasses.py; this actor only handles the gameplay.
 *  Local frame: X = along the route (the ladder), Y = along the crevasse, origin = centre of the gap at ladder height.
 */
UCLASS()
class YGGDRASILSPEAK_API ACrevasseCrossing : public AActor
{
	GENERATED_BODY()

public:

	ACrevasseCrossing();

	UPROPERTY(VisibleAnywhere, Category="Crevasse")
	TObjectPtr<UStaticMeshComponent> LipNear;

	UPROPERTY(VisibleAnywhere, Category="Crevasse")
	TObjectPtr<UStaticMeshComponent> LipFar;

	UPROPERTY(VisibleAnywhere, Category="Crevasse")
	TObjectPtr<UStaticMeshComponent> Ladder;

	/** Dark floor of the slot, hides the terrain at the bottom. */
	UPROPERTY(VisibleAnywhere, Category="Crevasse")
	TObjectPtr<UStaticMeshComponent> Abyss;

	/** On the ladder: careful footing. */
	UPROPERTY(VisibleAnywhere, Category="Crevasse")
	TObjectPtr<UBoxComponent> LadderZone;

	/** Inside the slot below the lips: a fatal fall. */
	UPROPERTY(VisibleAnywhere, Category="Crevasse")
	TObjectPtr<UBoxComponent> FallZone;

	/** Walking speed on the ladder (times normal). */
	UPROPERTY(EditAnywhere, Category="Crevasse", meta=(ClampMin="0.1", ClampMax="1"))
	float LadderSpeedMultiplier = 0.45f;

	/** View roll on the ladder (degrees) when calm and standing; grows with walking speed and wind. */
	UPROPERTY(EditAnywhere, Category="Crevasse", meta=(ClampMin="0"))
	float SwayDegrees = 0.8f;

	/** Extra sway at full wind (degrees). */
	UPROPERTY(EditAnywhere, Category="Crevasse", meta=(ClampMin="0"))
	float WindSwayDegrees = 2.5f;

protected:

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;

private:

	UFUNCTION()
	void OnLadderBegin(UPrimitiveComponent* Overlapped, AActor* Other, UPrimitiveComponent* OtherComp, int32 BodyIndex, bool bFromSweep, const FHitResult& Sweep);

	UFUNCTION()
	void OnLadderEnd(UPrimitiveComponent* Overlapped, AActor* Other, UPrimitiveComponent* OtherComp, int32 BodyIndex);

	UFUNCTION()
	void OnFall(UPrimitiveComponent* Overlapped, AActor* Other, UPrimitiveComponent* OtherComp, int32 BodyIndex, bool bFromSweep, const FHitResult& Sweep);

	void SetOnLadder(class APawn* Pawn, bool bOn);

	TWeakObjectPtr<class APawn> Climber;
	float SwayTime = 0.f;
	float SwayRoll = 0.f;
	bool bWarned = false;
};
