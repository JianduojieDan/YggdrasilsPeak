// Copyright Epic Games, Inc. All Rights Reserved.

#include "Survival/YggFootstepNotify.h"

#include "Survival/SnowTrailComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Actor.h"

void UYggFootstepNotify::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);
	AActor* Owner = MeshComp ? MeshComp->GetOwner() : nullptr;
	if (!Owner) { return; }
	if (USnowTrailComponent* Trail = Owner->FindComponentByClass<USnowTrailComponent>())
	{
		Trail->OnFootContact(bLeft, MeshComp);
	}
}
