// Copyright Epic Games, Inc. All Rights Reserved.

#include "FirstPersonBodyComponent.h"

#include "Camera/CameraComponent.h"
#include "Components/PoseableMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Character.h"
#include "TwoBoneIK.h"
#include "UObject/ConstructorHelpers.h"

UFirstPersonBodyComponent::UFirstPersonBodyComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	// After animation so the head bone is this frame's pose.
	PrimaryComponentTick.TickGroup = TG_PostUpdateWork;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> IceAxe(TEXT("/Game/Fab/Climbing_Axe_for_Blender/climbing_axe_422_packed/StaticMeshes/climbing_axe_422_packed.climbing_axe_422_packed"));
	HeldToolMesh = IceAxe.Object;
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
	// First Person template layout: Mesh is the world-space body (shadow, others see it) and a separate "FirstPersonMesh"
	// runs the first-person animation (ABP_FP_Copy + arm warp). Then the view body copies that mesh's pose instead.
	TArray<USkeletalMeshComponent*> Meshes;
	Character->GetComponents(Meshes);
	for (USkeletalMeshComponent* Mesh : Meshes)
	{
		if (Mesh != Body && Mesh->GetName().Contains(TEXT("FirstPersonMesh")))
		{
			TemplateArms = Mesh;
		}
	}
	AnimatedBody = TemplateArms ? TemplateArms.Get() : Body;
	BodyBaseLocation = AnimatedBody->GetRelativeLocation();
	WorldBodyBaseLocation = Body->GetRelativeLocation();

	CameraBoom = Character->FindComponentByClass<USpringArmComponent>();
	if (CameraBoom && HeadFollow > 0.f)
	{
		BoomBaseLocation = CameraBoom->GetRelativeLocation();
		SetComponentTickEnabled(true);
	}

	UCameraComponent* Camera = Character->FindComponentByClass<UCameraComponent>();
	if (HeldToolMesh && Camera && AnimatedBody->GetBoneIndex(HandBone) != INDEX_NONE)
	{
		// The owner sees a posable copy (head hidden, right arm on the tool); the animated body is for everyone else.
		ViewBody = NewObject<UPoseableMeshComponent>(Character, TEXT("FirstPersonViewBody"));
		ViewBody->SetupAttachment(AnimatedBody->GetAttachParent(), AnimatedBody->GetAttachSocketName());
		ViewBody->SetRelativeTransform(AnimatedBody->GetRelativeTransform());
		ViewBody->SetSkinnedAssetAndUpdate(AnimatedBody->GetSkeletalMeshAsset());
		ViewBody->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		ViewBody->SetOnlyOwnerSee(true);
		ViewBody->SetCastShadow(false);
		ViewBody->RegisterComponent();
		ViewBody->HideBoneByName(HiddenBone, PBO_None);
		for (int32 Slot = 0; Slot < AnimatedBody->GetNumMaterials(); ++Slot)
		{
			ViewBody->SetMaterial(Slot, AnimatedBody->GetMaterial(Slot));
		}

		// Not rendered for the owner, but its pose still drives the view body and the camera.
		AnimatedBody->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
		if (TemplateArms)
		{
			// The template's world body already hides itself from the owner and casts the shadow.
			TemplateArms->SetVisibility(false);
		}
		else
		{
			Body->SetOwnerNoSee(true);
		}

		HeldTool = NewObject<UStaticMeshComponent>(Character, TEXT("HeldTool"));
		HeldTool->SetupAttachment(Camera);
		HeldTool->SetStaticMesh(HeldToolMesh);
		HeldTool->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		HeldTool->SetOnlyOwnerSee(true);
		HeldTool->SetCastShadow(false);
		const FQuat ToolRotation = FRotationMatrix::MakeFromXY(PickDirectionInView, HandleDirectionInView).ToQuat();
		HeldTool->SetRelativeTransform(FTransform(ToolRotation, GripInView - ToolRotation.RotateVector(ToolGripPoint * HeldToolScale), FVector(HeldToolScale)));
		HeldTool->RegisterComponent();

		if (bFirstPersonRendering)
		{
			ViewBody->SetFirstPersonPrimitiveType(EFirstPersonPrimitiveType::FirstPerson);
			HeldTool->SetFirstPersonPrimitiveType(EFirstPersonPrimitiveType::FirstPerson);
			Camera->SetEnableFirstPersonFieldOfView(true);
			Camera->SetFirstPersonFieldOfView(FirstPersonFieldOfView);
			Camera->SetEnableFirstPersonScale(true);
			Camera->SetFirstPersonScale(FirstPersonScale);
		}

		SetComponentTickEnabled(true);
	}
	else if (TemplateArms)
	{
		// No tool: the template's own first-person arms and animation are shown untouched.
	}
	else
	{
		// The owner sees their body, minus the head.
		Body->SetOwnerNoSee(false);
		Body->HideBoneByName(HiddenBone, PBO_None);
	}

	if (!bShadowProxy || TemplateArms)
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
	if (ViewBody && bFirstPersonRendering)
	{
		// The world-space stand-in for the first-person body: owner-no-see, casts the shadow (needed for VSM).
		ShadowMesh->SetFirstPersonPrimitiveType(EFirstPersonPrimitiveType::WorldSpaceRepresentation);
	}
	ShadowMesh->RegisterComponent();

	Body->SetCastShadow(false);
}

void UFirstPersonBodyComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	ACharacter* Character = Cast<ACharacter>(GetOwner());
	// The shadow copy keeps the head bone; the visible body may have it hidden.
	USkeletalMeshComponent* HeadSource = ShadowMesh ? ShadowMesh.Get() : (Character ? Character->GetMesh() : nullptr);
	if (CameraBoom && HeadSource && CameraBoom->GetAttachParent() && HeadFollow > 0.f)
	{
		// Boom location is relative to its parent (the capsule), so work in that space.
		const FTransform& ParentXf = CameraBoom->GetAttachParent()->GetComponentTransform();
		const FVector Head = ParentXf.InverseTransformPosition(HeadSource->GetSocketLocation(HiddenBone));
		// The head already sinks with the body; the boom's resting spot has to sink with it.
		const FVector Target = FMath::Lerp(BoomBaseLocation - FVector(0.f, 0.f, SinkDepth), Head + EyeOffset, HeadFollow);
		CameraBoom->SetRelativeLocation(FMath::VInterpTo(CameraBoom->GetRelativeLocation(), Target, DeltaTime, FollowSpeed));
	}

	// After the camera has moved, so the hand lands exactly on the tool this frame.
	PoseViewBody();
}

void UFirstPersonBodyComponent::SetSinkDepth(float Centimetres)
{
	if (!AnimatedBody || FMath::IsNearlyEqual(Centimetres, SinkDepth, 0.05f))
	{
		return;
	}
	SinkDepth = Centimetres;
	const FVector Sunk = BodyBaseLocation - FVector(0.f, 0.f, SinkDepth);
	AnimatedBody->SetRelativeLocation(Sunk);
	if (TemplateArms)
	{
		// The world body (shadow, what others see) sinks too; with the template layout it is a separate mesh.
		if (ACharacter* Character = Cast<ACharacter>(GetOwner()))
		{
			Character->GetMesh()->SetRelativeLocation(WorldBodyBaseLocation - FVector(0.f, 0.f, SinkDepth));
		}
	}
	if (ViewBody)
	{
		ViewBody->SetRelativeLocation(Sunk);
	}
	if (ShadowMesh)
	{
		ShadowMesh->SetRelativeLocation(Sunk);
	}
}

void UFirstPersonBodyComponent::PoseViewBody()
{
	if (!ViewBody || !AnimatedBody || !HeldTool)
	{
		return;
	}

	ViewBody->CopyPoseFromSkeletalComponent(AnimatedBody);
	const TArray<FTransform>& Pose = AnimatedBody->GetComponentSpaceTransforms();
	const int32 Hand = AnimatedBody->GetBoneIndex(HandBone);
	if (!Pose.IsValidIndex(Hand) || !ViewBody->BoneSpaceTransforms.IsValidIndex(Hand))
	{
		return;
	}

	// Right arm: the hand goes where the tool sits in its palm.
	const FQuat ToolInHandRotation = ToolRotationInHand.Quaternion();
	const FTransform ToolInHand(ToolInHandRotation, PalmInHand - ToolInHandRotation.RotateVector(ToolGripPoint * HeldToolScale), FVector(HeldToolScale));
	const FTransform HandTarget = (ToolInHand.Inverse() * HeldTool->GetComponentTransform()).GetRelativeTransform(AnimatedBody->GetComponentTransform());
	// Keep the elbow bending the way the animation had it, pushed a little down.
	const int32 Elbow = AnimatedBody->GetBoneIndex(LowerArmBone);
	if (Pose.IsValidIndex(Elbow))
	{
		BendLimb(UpperArmBone, LowerArmBone, HandBone, HandTarget.GetLocation(), HandTarget.GetRotation(), Pose[Elbow].GetLocation() + FVector(0.f, 0.f, -20.f), ArmMaxStretch);
	}

	// Legs: the animation's foot IK keeps the boots on top of the snow even when the body is sunk; push them
	// back down by the sink depth so they disappear into it.
	if (SinkDepth > 0.5f)
	{
		for (const FName* Leg : { LeftLegBones, RightLegBones })
		{
			const int32 Thigh = AnimatedBody->GetBoneIndex(Leg[0]);
			const int32 Knee = AnimatedBody->GetBoneIndex(Leg[1]);
			const int32 Foot = AnimatedBody->GetBoneIndex(Leg[2]);
			if (!Pose.IsValidIndex(Thigh) || !Pose.IsValidIndex(Knee) || !Pose.IsValidIndex(Foot))
			{
				continue;
			}
			const FVector KneeLocation = Pose[Knee].GetLocation();
			const FVector Bend = (KneeLocation - (Pose[Thigh].GetLocation() + Pose[Foot].GetLocation()) * 0.5f).GetSafeNormal();
			BendLimb(Leg[0], Leg[1], Leg[2], Pose[Foot].GetLocation() - FVector(0.f, 0.f, SinkDepth), Pose[Foot].GetRotation(), KneeLocation + Bend * 30.f, 1.f);
		}
	}

	ViewBody->MarkRefreshTransformDirty();
}

void UFirstPersonBodyComponent::BendLimb(FName UpperBone, FName LowerBone, FName EndBone, const FVector& EndLocation, const FQuat& EndRotation, const FVector& Pole, float MaxStretch)
{
	const int32 Upper = AnimatedBody->GetBoneIndex(UpperBone);
	const int32 Lower = AnimatedBody->GetBoneIndex(LowerBone);
	const int32 End = AnimatedBody->GetBoneIndex(EndBone);
	const int32 Parent = Upper != INDEX_NONE ? AnimatedBody->GetBoneIndex(AnimatedBody->GetParentBone(UpperBone)) : INDEX_NONE;
	const TArray<FTransform>& Pose = AnimatedBody->GetComponentSpaceTransforms();
	if (Parent == INDEX_NONE || !Pose.IsValidIndex(Lower) || !Pose.IsValidIndex(End) || !ViewBody->BoneSpaceTransforms.IsValidIndex(End))
	{
		return;
	}

	const FTransform& OldUpper = Pose[Upper];
	const FTransform& OldLower = Pose[Lower];
	const FTransform& OldEnd = Pose[End];
	FVector NewJoint, NewEnd;
	AnimationCore::SolveTwoBoneIK(OldUpper.GetLocation(), OldLower.GetLocation(), OldEnd.GetLocation(), Pole, EndLocation,
		NewJoint, NewEnd, MaxStretch > 1.f, 1.0, MaxStretch);

	// Swing each bone from where the animation pointed it to where the solver put the next joint.
	const FQuat SwingUpper = FQuat::FindBetweenVectors(OldLower.GetLocation() - OldUpper.GetLocation(), NewJoint - OldUpper.GetLocation());
	FTransform NewUpper = OldUpper;
	NewUpper.SetRotation(SwingUpper * OldUpper.GetRotation());

	const FQuat SwingLower = FQuat::FindBetweenVectors(SwingUpper.RotateVector(OldEnd.GetLocation() - OldLower.GetLocation()), NewEnd - NewJoint);
	FTransform NewLower = OldLower;
	NewLower.SetLocation(NewJoint);
	NewLower.SetRotation(SwingLower * SwingUpper * OldLower.GetRotation());

	FTransform NewEndXf = OldEnd;
	NewEndXf.SetLocation(NewEnd);
	NewEndXf.SetRotation(EndRotation);

	ViewBody->BoneSpaceTransforms[Upper] = NewUpper.GetRelativeTransform(Pose[Parent]);
	ViewBody->BoneSpaceTransforms[Lower] = NewLower.GetRelativeTransform(NewUpper);
	ViewBody->BoneSpaceTransforms[End] = NewEndXf.GetRelativeTransform(NewLower);
}
