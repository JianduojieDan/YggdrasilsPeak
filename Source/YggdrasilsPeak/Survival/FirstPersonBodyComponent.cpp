// Copyright Epic Games, Inc. All Rights Reserved.

#include "FirstPersonBodyComponent.h"

#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Character.h"

UFirstPersonBodyComponent::UFirstPersonBodyComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	// After animation so the head bone is this frame's pose.
	PrimaryComponentTick.TickGroup = TG_PostUpdateWork;
}

void UFirstPersonBodyComponent::BeginPlay()
{
	Super::BeginPlay();

	ACharacter* Character = Cast<ACharacter>(GetOwner());
	USkeletalMeshComponent* Body = Character ? Character->GetMesh() : nullptr;
	if (!Body || !Character->IsLocallyControlled())
	{
		return;
	}

	CameraBoom = Character->FindComponentByClass<USpringArmComponent>();
	if (CameraBoom && HeadFollow > 0.f)
	{
		BoomBaseLocation = CameraBoom->GetRelativeLocation();
		SetComponentTickEnabled(true);
	}

	// The owner sees their body, minus the head.
	Body->SetOwnerNoSee(false);
	Body->HideBoneByName(HiddenBone, PBO_None);

	if (!bShadowProxy)
	{
		return;
	}

	// Shadow-only copy with the head intact. It runs its own anim instance: a leader-pose follower would
	// inherit the hidden head bone.
	ShadowMesh = NewObject<USkeletalMeshComponent>(Character, TEXT("FirstPersonShadowMesh"));
	ShadowMesh->SetupAttachment(Body->GetAttachParent());
	ShadowMesh->SetRelativeTransform(Body->GetRelativeTransform());
	ShadowMesh->SetSkeletalMesh(Body->GetSkeletalMeshAsset());
	ShadowMesh->SetAnimInstanceClass(Body->GetAnimClass());
	ShadowMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ShadowMesh->SetOwnerNoSee(true);
	ShadowMesh->bCastHiddenShadow = true;
	ShadowMesh->SetCastShadow(true);
	ShadowMesh->RegisterComponent();

	Body->SetCastShadow(false);
}

void UFirstPersonBodyComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	ACharacter* Character = Cast<ACharacter>(GetOwner());
	// The shadow copy keeps the head bone; the visible body has it hidden.
	USkeletalMeshComponent* HeadSource = ShadowMesh ? ShadowMesh.Get() : (Character ? Character->GetMesh() : nullptr);
	if (!CameraBoom || !HeadSource || !CameraBoom->GetAttachParent())
	{
		return;
	}

	// Boom location is relative to its parent (the capsule), so work in that space.
	const FTransform& ParentXf = CameraBoom->GetAttachParent()->GetComponentTransform();
	const FVector Head = ParentXf.InverseTransformPosition(HeadSource->GetSocketLocation(HiddenBone));
	const FVector Target = FMath::Lerp(BoomBaseLocation, Head + EyeOffset, HeadFollow);
	CameraBoom->SetRelativeLocation(FMath::VInterpTo(CameraBoom->GetRelativeLocation(), Target, DeltaTime, FollowSpeed));
}
