// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SpindriftComponent.generated.h"

class UExpeditionComponent;
class UMaterialParameterCollection;
class UNiagaraComponent;
class UNiagaraSystem;

/**
 *  Blowing snow (spindrift) around the local player, driven by the weather system's wind.
 *
 *  Two layers: powder streaming along the snow surface ("snow snakes", drawn by the landscape material from
 *  MPC_Wind) and airborne gusts (a Niagara system that follows the player). Both strengthen with the wind
 *  and with altitude, so the high mountain is wilder than the valleys.
 *
 *  The streaks are sampled in a frame turned towards the wind around an anchor near the player. When the wind
 *  turns or the player walks far, the anchor moves and the scroll offset is corrected, so the pattern under
 *  the player never jumps.
 */
UCLASS(ClassGroup=(Survival), meta=(BlueprintSpawnableComponent))
class YGGDRASILSPEAK_API USpindriftComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	USpindriftComponent();

	UPROPERTY(EditAnywhere, Category="Spindrift")
	TObjectPtr<UNiagaraSystem> GustEffect;

	UPROPERTY(EditAnywhere, Category="Spindrift")
	TObjectPtr<UMaterialParameterCollection> WindCollection;

	/** Wind speed (m/s) at the weather system's intensity 0 and 10. */
	UPROPERTY(EditAnywhere, Category="Spindrift")
	FVector2D WindSpeedRange = FVector2D(2.f, 24.f);

	/** Weather wind intensity where blowing snow starts and where it is at full strength. */
	UPROPERTY(EditAnywhere, Category="Spindrift")
	FVector2D IntensityRange = FVector2D(2.5f, 8.f);

	/** Extra strength high up: added from 0 at the first elevation (m) to the amount at the second. */
	UPROPERTY(EditAnywhere, Category="Spindrift")
	FVector2D AltitudeRange = FVector2D(5200.f, 8000.f);

	UPROPERTY(EditAnywhere, Category="Spindrift", meta=(ClampMin="0", ClampMax="1"))
	float AltitudeBoost = 0.35f;

	/** Surface streaks move at this fraction of the wind (the powder is dragged along the snow). */
	UPROPERTY(EditAnywhere, Category="Spindrift", meta=(ClampMin="0", ClampMax="1"))
	float SurfaceSpeedFraction = 0.6f;

	/** Streak texture tile size (cm) along and across the wind; the landscape material must use the same. */
	UPROPERTY(EditAnywhere, Category="Spindrift")
	FVector2D StreakTileCm = FVector2D(3200.f, 800.f);

	/** Seconds for strength and direction to follow a change in the weather. */
	UPROPERTY(EditAnywhere, Category="Spindrift", meta=(ClampMin="0.1"))
	float Smoothing = 3.f;

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	float GetStrength() const { return Strength; }

protected:

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:

	/** Wind direction the streak frame currently uses, and the frame's anchor/scroll (cm). */
	FVector2D FrameDirection = FVector2D(1.f, 0.f);
	FVector2D Anchor = FVector2D::ZeroVector;
	FVector2D Scroll = FVector2D::ZeroVector;
	bool bHasFrame = false;

	FVector2D Direction = FVector2D(1.f, 0.f);
	float DirectionAngle = 0.f;
	float Age = 0.f;
	float Strength = 0.f;
	float SpeedCm = 0.f;

	TWeakObjectPtr<UExpeditionComponent> Expedition;

	UPROPERTY(Transient)
	TObjectPtr<UNiagaraComponent> Gusts;
};
