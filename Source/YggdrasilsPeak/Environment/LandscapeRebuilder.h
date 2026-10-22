// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LandscapeRebuilder.generated.h"

class ALandscapeProxy;

/**
 *  Editor-only helper: rebuilds a landscape at a higher resolution with smooth (Catmull-Rom) heights.
 *  Same footprint and heights at the original samples, but the coarse flat triangles become curved
 *  terrain, which removes straight lighting/shadow creases. Optional baked noise (rolling bumps plus
 *  ridged gullies, stronger on steep ground) breaks up the remaining smooth planes.
 *  The old landscape is hidden, not deleted.
 *
 *  Use: place in the level, pick SourceLandscape, then tick bRebuildNow (or press Rebuild Landscape).
 *  With bAutoRebuild, changing any Rebuild/Noise value rebuilds right away. Each rebuild replaces the previous result.
 */
UCLASS(NotBlueprintable)
class YGGDRASILSPEAK_API ALandscapeRebuilder : public AActor
{
	GENERATED_BODY()

public:

	ALandscapeRebuilder();

	UPROPERTY(EditAnywhere, Category="Rebuild")
	TObjectPtr<ALandscapeProxy> SourceLandscape;

	/** Each source quad becomes Factor x Factor quads. */
	UPROPERTY(EditAnywhere, Category="Rebuild", meta=(ClampMin="2", ClampMax="8"))
	int32 Factor = 2;

	/** Section layout of the new landscape; (source quads * Factor) must divide by SubsectionQuads * NumSubsections. */
	UPROPERTY(EditAnywhere, Category="Rebuild")
	int32 SubsectionQuads = 63;

	UPROPERTY(EditAnywhere, Category="Rebuild")
	int32 NumSubsections = 2;

	/** Height of the rolling bumps on flat ground, in metres. 0 = no noise. */
	UPROPERTY(EditAnywhere, Category="Noise", meta=(ClampMin="0"))
	float NoiseHeightM = 2.5f;

	/** Size of the largest bumps, in metres. Smaller octaves go down to the vertex spacing. */
	UPROPERTY(EditAnywhere, Category="Noise", meta=(ClampMin="10"))
	float NoiseSizeM = 300.f;

	/** Noise height multiplier on steep ground (lerped in by slope). Rock faces get rougher than snow fields. */
	UPROPERTY(EditAnywhere, Category="Noise", meta=(ClampMin="0"))
	float SteepMultiplier = 3.f;

	/** 0 = only soft bumps, 1 = only sharp ridges/gullies. */
	UPROPERTY(EditAnywhere, Category="Noise", meta=(ClampMin="0", ClampMax="1"))
	float Ridged = 0.5f;

	UPROPERTY(EditAnywhere, Category="Noise")
	int32 Seed = 1337;

	/** Setting this to true runs the rebuild once and resets it. */
	UPROPERTY(EditAnywhere, Category="Rebuild")
	bool bRebuildNow = false;

	/** Rebuild whenever a Rebuild/Noise value changes. */
	UPROPERTY(EditAnywhere, Category="Rebuild")
	bool bAutoRebuild = true;

	/** Landscape made by the last rebuild; deleted before the next one. */
	UPROPERTY(VisibleAnywhere, Category="Rebuild")
	TObjectPtr<ALandscapeProxy> BuiltLandscape;

	/** Result of the last rebuild. */
	UPROPERTY(VisibleAnywhere, Category="Rebuild")
	FString LastResult;

	UFUNCTION(CallInEditor, Category="Rebuild")
	void RebuildLandscape();

	/** Height the highest point of BuiltLandscape is raised to, in metres above sea level (Everest: 8848.86). */
	UPROPERTY(EditAnywhere, Category="Summit")
	float SummitElevationM = 8848.86f;

	/** Radius of the smooth bump that lifts the summit, in metres. Props and rope points inside it are lifted with the ground. */
	UPROPERTY(EditAnywhere, Category="Summit", meta=(ClampMin="50"))
	float SummitRadiusM = 450.f;

	/**
	 *  The 34 m/px source heightmap averages Everest's sharp top down to ~8762 m (and the 16-bit range is already used up there).
	 *  This shifts BuiltLandscape's stored heights down and its actor up by the same amount (world heights unchanged, only the
	 *  lowest valley floors far outside the route get clamped), then adds a smooth bump so the top reaches SummitElevationM.
	 *  Run once; running again finds the summit already high enough and does nothing.
	 */
	UFUNCTION(CallInEditor, Category="Summit")
	void RaiseSummit();

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
};
