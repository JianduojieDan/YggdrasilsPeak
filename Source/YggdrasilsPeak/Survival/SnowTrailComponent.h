// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SnowTrailComponent.generated.h"

class UDecalComponent;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class UMaterialParameterCollection;
class UNiagaraSystem;
class UTextureRenderTarget2D;
class USkeletalMeshComponent;

/**
 *  The owner's trail through the snow.
 *
 *  Deformation (default, ygg.Snow.Deform 0/1): a trail map follows the player (TrailAreaCm across). Every frame
 *  the feet paint into it wherever they are in snow, and the landscape material pushes its snow down by that map,
 *  so walking leaves a real trench (the same idea as Ghost of Yotei's displacement buffer). Old trail outside the
 *  area is forgotten.
 *  Also: each step kicks up a powder puff, the body sinks into deep snow, and deep snow is reported to survival.
 *  The older boot-print decals are still here behind ygg.Snow.Trails 0/1 (off by default).
 */
UCLASS(ClassGroup=(Survival), meta=(BlueprintSpawnableComponent))
class YGGDRASILSPEAK_API USnowTrailComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	USnowTrailComponent();

	/** Decal material for one boot print (decal colour: R = left foot, A = depth 0..1). */
	UPROPERTY(EditAnywhere, Category="Snow Trail")
	TObjectPtr<UMaterialInterface> FootprintMaterial;

	// ---- Deformation (trail map) ----

	/** Trail map (R: 0 untouched .. 1 fully pushed down) read by the landscape material. */
	UPROPERTY(EditAnywhere, Category="Snow Trail|Deformation")
	TObjectPtr<UTextureRenderTarget2D> TrailMap;

	/** TrailArea = (centre X, centre Y, side length, on/off) for the landscape material. */
	UPROPERTY(EditAnywhere, Category="Snow Trail|Deformation")
	TObjectPtr<UMaterialParameterCollection> TrailParameters;

	/** Soft round dab painted into the trail map under each foot (scalar parameter Depth). */
	UPROPERTY(EditAnywhere, Category="Snow Trail|Deformation")
	TObjectPtr<UMaterialInterface> TrailBrush;

	/** Side length of the square the trail map covers, centred near the player (cm). */
	UPROPERTY(EditAnywhere, Category="Snow Trail|Deformation", meta=(ClampMin="1000"))
	float TrailAreaCm = 6400.f;

	/** The map re-centres on the player when they get this far from its centre (cm). */
	UPROPERTY(EditAnywhere, Category="Snow Trail|Deformation", meta=(ClampMin="100"))
	float RecentreDistanceCm = 1600.f;

	/** Radius of the hole each leg pushes through the snow (cm). */
	UPROPERTY(EditAnywhere, Category="Snow Trail|Deformation", meta=(ClampMin="1"))
	float LegRadiusCm = 22.f;

	/** Small burst of powder kicked up by each step (scaled by how soft the snow is). */
	UPROPERTY(EditAnywhere, Category="Snow Trail")
	TObjectPtr<UNiagaraSystem> StepPuff;

	/** Most prints alive at once; the oldest is reused. */
	UPROPERTY(EditAnywhere, Category="Snow Trail", meta=(ClampMin="1", ClampMax="2000"))
	int32 MaxPrints = 300;

	/** Seconds a print stays before it starts fading, and how long the fade takes. */
	UPROPERTY(EditAnywhere, Category="Snow Trail", meta=(ClampMin="0"))
	float PrintLifeSeconds = 300.f;

	UPROPERTY(EditAnywhere, Category="Snow Trail", meta=(ClampMin="0"))
	float PrintFadeSeconds = 30.f;

	/** Above this storm strength existing prints fill in over StormFillSeconds. */
	UPROPERTY(EditAnywhere, Category="Snow Trail", meta=(ClampMin="0", ClampMax="1"))
	float StormFillAbove = 0.5f;

	UPROPERTY(EditAnywhere, Category="Snow Trail", meta=(ClampMin="0"))
	float StormFillSeconds = 40.f;

	/** Print size: half length, half width, projection depth (cm). */
	UPROPERTY(EditAnywhere, Category="Snow Trail")
	FVector PrintHalfSize = FVector(16.f, 7.f, 20.f);

	/** A foot bone counts as planted below this height above the ground under it (cm) and lifted again above it + 4 (planted feet sit ~7 cm up, lifted ones 13-22). */
	UPROPERTY(EditAnywhere, Category="Snow Trail", meta=(ClampMin="0"))
	float PlantHeight = 10.f;

	/** How far the body sinks into the softest snow (cm); hard crust, rock and props: none. */
	UPROPERTY(EditAnywhere, Category="Snow Trail|Sinking", meta=(ClampMin="0", ClampMax="60"))
	float MaxSinkCm = 20.f;

	/** Camps have trampled, packed snow: sinking is multiplied by this inside one. */
	UPROPERTY(EditAnywhere, Category="Snow Trail|Sinking", meta=(ClampMin="0", ClampMax="1"))
	float CampSinkMultiplier = 0.5f;

	/** How quickly the sink depth follows the ground under the player. */
	UPROPERTY(EditAnywhere, Category="Snow Trail|Sinking", meta=(ClampMin="0.1"))
	float SinkSpeed = 3.f;

	/** Softness (0 hard .. 1 deep powder) of the snow the player is standing on, 0 when not on snow. */
	float GetGroundSoftness() const { return GroundSoftness; }

	UPROPERTY(EditAnywhere, Category="Snow Trail")
	FName LeftFootBone = TEXT("foot_l");

	UPROPERTY(EditAnywhere, Category="Snow Trail")
	FName RightFootBone = TEXT("foot_r");

	/**
	 *  How soft the snow is at a point, 0 = hard wind crust / ice, 1 = deep powder. Mirrors the landscape
	 *  material's zones (deep below ~6700 m and on gentle slopes, hard above ~7000 m and on steep ground)
	 *  so what you see, what you sink into and how fast you walk agree.
	 */
	static float SnowSoftness(const FVector& Location, const FVector& GroundNormal);

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:

	struct FFootState
	{
		FName Bone;
		bool bPlanted = true;
		bool bLeft = false;
	};

	/** Samples the snow under the player and eases the body's sink depth towards it. */
	void UpdateSinking(float DeltaTime);
	/** True when the hit is the landscape's snow (not its rock, not a prop). */
	static bool IsSnow(const FHitResult& Hit);
	void UpdateFoot(FFootState& Foot, const FVector& WalkDirection);
	void StampPrint(const FHitResult& Hit, const FVector& WalkDirection, bool bLeft);
	void FillInPrints(float Seconds);

	/** Paints the legs into the trail map and keeps the map centred on the player. */
	void UpdateTrailMap();
	/** Moves the map's centre to the player, shifting what is already drawn so the trail stays in place. */
	void RecentreTrailMap(const FVector2D& NewCentre);
	void PublishTrailArea(bool bEnabled) const;

	UPROPERTY(Transient)
	TObjectPtr<UTextureRenderTarget2D> TrailScratch;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> BrushInstance;

	FVector2D TrailCentre = FVector2D::ZeroVector;
	bool bTrailReady = false;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UDecalComponent>> Pool;

	UPROPERTY(Transient)
	TObjectPtr<USkeletalMeshComponent> Body;

	FFootState Feet[2];
	int32 NextPrint = 0;
	bool bStormFilling = false;

	float GroundSoftness = 0.f;
	float TargetSink = 0.f;
	float CurrentSink = 0.f;
	float SinkCheckTimer = 0.f;
};
