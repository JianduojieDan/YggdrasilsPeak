// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "FirstPersonBodyComponent.generated.h"

class USkeletalMeshComponent;
class USpringArmComponent;

/**
 *  First-person body: the local player sees their own body (legs, arms, hands) with the head bone hidden
 *  so the eye-level camera doesn't look out through the inside of the skull.
 *  Hiding the head also removes it from the shadow, so a second, shadow-only copy of the mesh is spawned
 *  (invisible to the owner, full head, own animation instance) and the visible body stops casting shadows.
 *  Other players / cameras keep seeing the normal full mesh.
 *  The camera boom also follows the animated head (HeadFollow), so steps and breathing move the view a little.
 */
UCLASS(ClassGroup=(Survival), meta=(BlueprintSpawnableComponent))
class YGGDRASILSPEAK_API UFirstPersonBodyComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	UFirstPersonBodyComponent();

	/** Bone hidden for the owner (children are hidden with it). */
	UPROPERTY(EditAnywhere, Category="First Person")
	FName HiddenBone = TEXT("head");

	/** Keep the full shadow (with head) via a shadow-only mesh copy. */
	UPROPERTY(EditAnywhere, Category="First Person")
	bool bShadowProxy = true;

	/** 0 = camera fixed at the boom's default spot, 1 = camera rides fully on the animated head. */
	UPROPERTY(EditAnywhere, Category="First Person", meta=(ClampMin="0", ClampMax="1"))
	float HeadFollow = 0.6f;

	/** Eye position relative to the head bone, in actor space (X forward, Z up), cm. */
	UPROPERTY(EditAnywhere, Category="First Person")
	FVector EyeOffset = FVector(10.f, 0.f, 6.f);

	/** How fast the camera catches up with the head; lower = smoother sway. */
	UPROPERTY(EditAnywhere, Category="First Person", meta=(ClampMin="1"))
	float FollowSpeed = 18.f;

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:

	virtual void BeginPlay() override;

private:

	UPROPERTY(Transient)
	TObjectPtr<USkeletalMeshComponent> ShadowMesh;

	UPROPERTY(Transient)
	TObjectPtr<USpringArmComponent> CameraBoom;

	FVector BoomBaseLocation = FVector::ZeroVector;
};
