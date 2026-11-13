// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "YggFootstepNotify.generated.h"

/**
 *  A foot touches the ground (placed in the walk and run clips at their foot-contact frames, Tools/Anim/add_footstep_notifies.py).
 *  Tells the owner's USnowTrailComponent, which plays the step for the ground under that foot. The animation decides
 *  when a step sounds, so rough rock under a swinging foot can no longer fake one (play-test 2026-11-13).
 */
UCLASS(meta=(DisplayName="Ygg Footstep"))
class YGGDRASILSPEAK_API UYggFootstepNotify : public UAnimNotify
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Footstep")
	bool bLeft = true;

	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
	virtual FString GetNotifyName_Implementation() const override { return bLeft ? TEXT("Step L") : TEXT("Step R"); }
};
