// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "YggMirror.generated.h"

class UMaterialInstanceDynamic;
class UMaterialInterface;
class USceneCaptureComponent2D;
class UStaticMeshComponent;
class UTextureRenderTarget2D;

/**
 *  A standing mirror for checking the character (pose, outfit, held tool).
 *  Every frame the player's camera is reflected across the glass and a scene capture renders from there, clipped at
 *  the glass plane; the glass shows that image in screen space (flipped left-right), so it lines up with the real view
 *  exactly like a mirror. The capture only runs while a player is within CaptureDistance, so it costs nothing elsewhere.
 *  The first-person arms are owner-only, so the mirror shows the world body, i.e. what other players see.
 */
UCLASS()
class YGGDRASILSPEAK_API AYggMirror : public AActor
{
	GENERATED_BODY()

public:

	AYggMirror();

	/** Glass size, cm (width, height). */
	UPROPERTY(EditAnywhere, Category="Mirror")
	FVector2D Size = FVector2D(200.f, 200.f);

	/** Capture resolution as a fraction of the screen (lower = cheaper, blurrier). */
	UPROPERTY(EditAnywhere, Category="Mirror", meta=(ClampMin="0.1", ClampMax="1"))
	float ResolutionScale = 0.5f;

	/** The mirror only renders while the player's camera is closer than this, cm. */
	UPROPERTY(EditAnywhere, Category="Mirror", meta=(ClampMin="100"))
	float CaptureDistance = 1500.f;

	/** Unlit material sampling "Reflection" (texture parameter) at the flipped screen position. */
	UPROPERTY(EditAnywhere, Category="Mirror")
	TObjectPtr<UMaterialInterface> GlassMaterial;

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void Tick(float DeltaSeconds) override;

protected:

	virtual void BeginPlay() override;

private:

	UPROPERTY(VisibleAnywhere, Category="Mirror")
	TObjectPtr<UStaticMeshComponent> Glass;

	UPROPERTY(VisibleAnywhere, Category="Mirror")
	TObjectPtr<UStaticMeshComponent> Frame;

	UPROPERTY(VisibleAnywhere, Category="Mirror")
	TObjectPtr<USceneCaptureComponent2D> Capture;
	int32 FrameSkip = 0;

	UPROPERTY(Transient)
	TObjectPtr<UTextureRenderTarget2D> Target;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> GlassInstance;
};
