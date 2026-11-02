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

	/** Soft round dab painted into the trail map where a lifted foot ploughs through deep snow (scalar parameter Depth). */
	UPROPERTY(EditAnywhere, Category="Snow Trail|Deformation")
	TObjectPtr<UMaterialInterface> TrailBrush;

	/** Softened copy of TrailMap read by the landscape (R: print with rounded walls, G: wide spread for the snow banks). */
	UPROPERTY(EditAnywhere, Category="Snow Trail|Deformation")
	TObjectPtr<UTextureRenderTarget2D> TrailSoftMap;

	/** Draws TrailSoftMap from TrailMap (texture parameter Trail, scalar Texel). */
	UPROPERTY(EditAnywhere, Category="Snow Trail|Deformation")
	TObjectPtr<UMaterialInterface> TrailBlur;

	/** Boot-sole stamp painted into the trail map under a planted foot (scalar parameters Depth, Mirror = 1 for the left boot). */
	UPROPERTY(EditAnywhere, Category="Snow Trail|Deformation")
	TObjectPtr<UMaterialInterface> BootBrush;

	/** Size of the boot print in the trail map (cm). */
	UPROPERTY(EditAnywhere, Category="Snow Trail|Deformation", meta=(ClampMin="5"))
	float BootLengthCm = 31.f;

	UPROPERTY(EditAnywhere, Category="Snow Trail|Deformation", meta=(ClampMin="3"))
	float BootWidthCm = 12.f;

	/** The sole's centre lies this far in front of the foot (ankle) bone (cm). */
	UPROPERTY(EditAnywhere, Category="Snow Trail|Deformation")
	float SoleForwardCm = 6.f;

	/** Side length of the square the trail map covers, centred near the player (cm). */
	UPROPERTY(EditAnywhere, Category="Snow Trail|Deformation", meta=(ClampMin="1000"))
	float TrailAreaCm = 5120.f;

	/** The map re-centres on the player when they get this far from its centre (cm). */
	UPROPERTY(EditAnywhere, Category="Snow Trail|Deformation", meta=(ClampMin="100"))
	float RecentreDistanceCm = 1600.f;

	/** Radius of the channel a lifted foot ploughs through deep snow between prints (cm). */
	UPROPERTY(EditAnywhere, Category="Snow Trail|Deformation", meta=(ClampMin="1"))
	float LegRadiusCm = 14.f;

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

	/** Deep trail: blowing snow halves its depth every this many seconds (nearly gone after ~5 min)... */
	UPROPERTY(EditAnywhere, Category="Snow Trail|Deformation", meta=(ClampMin="1"))
	float TrailHalfLifeSeconds = 45.f;

	/** ...and this fast in a storm above StormFillAbove (nearly gone after ~40 s). */
	UPROPERTY(EditAnywhere, Category="Snow Trail|Deformation", meta=(ClampMin="1"))
	float StormTrailHalfLifeSeconds = 7.f;

	/** Covers the whole trail map with "no trail" at opacity Amount (fills the trail in a little). */
	UPROPERTY(EditAnywhere, Category="Snow Trail|Deformation")
	TObjectPtr<UMaterialInterface> TrailFill;

	/** Print size: half length, half width, projection depth (cm). */
	UPROPERTY(EditAnywhere, Category="Snow Trail")
	FVector PrintHalfSize = FVector(16.f, 7.f, 20.f);

	/** A foot counts as planted in the lowest this fraction of its own up-down swing (measured per foot, so any skeleton or clip works)... */
	UPROPERTY(EditAnywhere, Category="Snow Trail", meta=(ClampMin="0", ClampMax="1"))
	float PlantFraction = 0.3f;

	/** ...and lifted again this fraction of the swing above that. */
	UPROPERTY(EditAnywhere, Category="Snow Trail", meta=(ClampMin="0", ClampMax="1"))
	float LiftHysteresis = 0.2f;

	/** Swing below this (cm) is treated as this, so a shuffling or standing foot's jitter never stamps prints. */
	UPROPERTY(EditAnywhere, Category="Snow Trail", meta=(ClampMin="0.5"))
	float MinSwingCm = 4.f;

	/** How fast the remembered lowest/highest foot heights relax towards each other (cm/s), so slopes and clip changes are followed. */
	UPROPERTY(EditAnywhere, Category="Snow Trail", meta=(ClampMin="0"))
	float SwingDriftCmPerSecond = 3.f;

	/** How far the body sinks into the softest snow (cm); hard crust, rock and props: none. */
	UPROPERTY(EditAnywhere, Category="Snow Trail|Sinking", meta=(ClampMin="0", ClampMax="60"))
	float MaxSinkCm = 12.f;

	/** Camps have trampled, packed snow: sinking is multiplied by this inside one. */
	UPROPERTY(EditAnywhere, Category="Snow Trail|Sinking", meta=(ClampMin="0", ClampMax="1"))
	float CampSinkMultiplier = 0.5f;

	/** How quickly the sink depth follows the ground under the player. */
	UPROPERTY(EditAnywhere, Category="Snow Trail|Sinking", meta=(ClampMin="0.1"))
	float SinkSpeed = 3.f;

	/** Softness (0 hard .. 1 deep powder) of the snow the player is standing on, 0 when not on snow. */
	float GetGroundSoftness() const { return GroundSoftness; }

	// ---- Footstep sounds (one per foot plant, chosen by the surface under the foot)
	/** Soft / wind-packed snow. Empty = Ultra Dynamic Sky's Snow_Compress recordings. */
	UPROPERTY(EditAnywhere, Category="Snow Trail|Footsteps")
	TArray<TObjectPtr<USoundBase>> SnowSteps;

	/** Crampons on bare glacier ice, hard crust and the seracs' ice. Empty = /Game/Audio/Footsteps/Step_Ice_*. */
	UPROPERTY(EditAnywhere, Category="Snow Trail|Footsteps")
	TArray<TObjectPtr<USoundBase>> IceSteps;

	/** Moraine rubble and rock. Empty = /Game/Audio/Footsteps/Step_Rubble_*. */
	UPROPERTY(EditAnywhere, Category="Snow Trail|Footsteps")
	TArray<TObjectPtr<USoundBase>> RubbleSteps;

	/** Crampons on an aluminium ladder rung. Empty = /Game/Audio/Footsteps/Step_Ladder_*. */
	UPROPERTY(EditAnywhere, Category="Snow Trail|Footsteps")
	TArray<TObjectPtr<USoundBase>> LadderSteps;

	/** Footstep loudness at walking speed (sprinting is a little louder, standing still quieter). */
	UPROPERTY(EditAnywhere, Category="Snow Trail|Footsteps", meta=(ClampMin="0", ClampMax="2"))
	float FootstepVolume = 0.5f;

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

	/** Bare glacier ice / debris (the landscape's Glacier zones below ~6000 m), where there is no snow to sink into. */
	static bool IsGlacierHard(const FVector& Location);

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
		/** Height of the foot bone above the snow surface under it, last time it was measured (cm). */
		float Height = 0.f;
		/** Lowest / highest foot height over the last steps (cm), relaxing towards each other at SwingDriftCmPerSecond. */
		float Lowest = 1000.f;
		float Highest = -1000.f;
	};

	/** Samples the snow under the player and eases the body's sink depth towards it. */
	void UpdateSinking(float DeltaTime);
	/** True when the hit is the landscape's snow (not its rock, not a prop). */
	static bool IsSnow(const FHitResult& Hit);
	void PlayFootstep(const FHitResult& Hit);
	void LoadDefaultFootsteps();
	int32 LastStepVariation = -1;
	/** Glacier rubble or bare ice (T_GlacierMask, below ~6000 m): no snow to sink into or leave prints in. */
	void UpdateFoot(FFootState& Foot, const FVector& WalkDirection, float DeltaTime);
	void StampPrint(const FHitResult& Hit, const FVector& WalkDirection, bool bLeft);
	void FillInPrints(float Seconds);

	/** Paints the legs into the trail map and keeps the map centred on the player. */
	void UpdateTrailMap(float DeltaTime);
	/** Lets blowing snow refill the deep trail a little (every FillInterval, so half floats can resolve the step). */
	void FillTrailMap(float DeltaTime);
	/** Moves the map's centre to the player, shifting what is already drawn so the trail stays in place. */
	void RecentreTrailMap(const FVector2D& NewCentre);
	void PublishTrailArea(bool bEnabled) const;
	/** Redraws TrailSoftMap from TrailMap. */
	void SoftenTrailMap();

	UPROPERTY(Transient)
	TObjectPtr<UTextureRenderTarget2D> TrailScratch;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> BrushInstance;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> BlurInstance;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> FillInstance;

	float FillTimer = 0.f;

	/** Right and left boot (separate instances: canvas draws are batched, so one instance can't switch Mirror between them). */
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> BootInstances[2];

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
