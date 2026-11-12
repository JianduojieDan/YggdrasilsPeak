// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LightningBolt.generated.h"

class UProceduralMeshComponent;
class UMaterialInstanceDynamic;

/**
 *  One lightning strike drawn by hand: a jagged main channel from the cloud base to the ground, and (depending on
 *  Branchiness) anything from no side branches to a whole tree of forks. It flickers with 1-4 return strokes down the
 *  same channel (the branches mostly light only on the first), then the channel cools to a slow fading afterglow, so a
 *  near strike is still faintly seen when its thunder arrives. Camera-facing ribbons, one additive unlit material
 *  (/Game/Environment/Realms/M_LightningBolt); the actor destroys itself when the glow is gone.
 *  RealmSky fires them in the Jotunheim storm; ygg.Storm.Strike [km] [branchiness] fires one by hand.
 */
UCLASS()
class YGGDRASILSPEAK_API ALightningBolt : public AActor
{
	GENERATED_BODY()

public:
	ALightningBolt();

	/** Builds and starts a strike. Branchiness 0 = a lone channel .. 1 = a heavy fork tree. Life = seconds until gone. */
	UFUNCTION(BlueprintCallable, Category="Lightning")
	void Fire(FVector From, FVector To, float Branchiness, float Life, float Brightness, int32 Seed);

	/** Checking in the editor: draws the strike as seen from Eye, Age seconds after it struck (no Tick needed). */
	UFUNCTION(BlueprintCallable, CallInEditor, Category="Lightning")
	void Preview(FVector Eye, float Age);

	/** How strongly the strike lights the world right now, 0..1 (the return strokes, not the afterglow). */
	float GetLight() const { return LightNow; }

	virtual void Tick(float DeltaSeconds) override;

private:
	struct FChannel
	{
		TArray<FVector> P;        // local points, from where the channel leaves its parent to its tip
		TArray<float> Reach;      // 0 at the cloud .. 1 at the ground: when the leader gets there
		float Width0 = 1.f, Width1 = 1.f;   // relative width at the start and the tip
		float Bright = 1.f;       // relative brightness
		float FadeAt = 1.f;       // share of the life this channel lasts (branches cool first)
		int32 Level = 0;          // 0 the main channel (and a forked second ground channel), 1.. branches
	};
	TArray<FChannel> Channels;
	TArray<float> Strokes;        // seconds after the strike: the first stroke and the return strokes
	float Age = 0.f, Life = 3.f, Brightness = 1.f, LightNow = 0.f;
	int32 Seed = 0;
	bool bFired = false;

	UPROPERTY(VisibleAnywhere) TObjectPtr<UProceduralMeshComponent> Mesh;
	UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> Mat;

	void Grow(FRandomStream& R, const FVector& A, const FVector& B, float Width0, float Width1, float Bright, float FadeAt,
		int32 Level, float Branchiness, float ReachA, float ReachB, float GroundZ);
	void Draw(const FVector& Eye);
	/** Stroke brightness (spikes) and afterglow at time T. */
	void Envelope(float T, float& OutStroke, float& OutGlow) const;
};
