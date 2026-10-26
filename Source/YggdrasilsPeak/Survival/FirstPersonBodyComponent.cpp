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

		// The tool rides in the view body's fist; PoseToolArm places it every frame.
		ViewCamera = Camera;
		MeasureHand();
		HeldTool = NewObject<UStaticMeshComponent>(Character, TEXT("HeldTool"));
		HeldTool->SetupAttachment(ViewBody);
		HeldTool->SetStaticMesh(HeldToolMesh);
		HeldTool->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		HeldTool->SetOnlyOwnerSee(true);
		HeldTool->SetCastShadow(false);
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

	// After the camera has moved, so the fist sits at its spot in this frame's view.
	PoseViewBody(DeltaTime);
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

void UFirstPersonBodyComponent::PoseViewBody(float DeltaTime)
{
	if (!ViewBody || !AnimatedBody || !HeldTool)
	{
		return;
	}

	ViewBody->CopyPoseFromSkeletalComponent(AnimatedBody);
	const TArray<FTransform>& Pose = AnimatedBody->GetComponentSpaceTransforms();

	PoseToolArm(DeltaTime);

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

void UFirstPersonBodyComponent::MeasureHand()
{
	const FReferenceSkeleton& Ref = ViewBody->GetSkinnedAsset()->GetRefSkeleton();
	const TArray<FTransform>& RefPose = Ref.GetRefBonePose();
	const int32 Hand = Ref.FindBoneIndex(HandBone);
	// Reference-pose position of a bone below the hand, in hand-bone space.
	auto InHand = [&](const TCHAR* Name) -> TOptional<FVector>
	{
		const int32 Bone = Ref.FindBoneIndex(Name);
		if (Bone == INDEX_NONE)
		{
			return {};
		}
		FTransform Xf = RefPose[Bone];
		for (int32 Parent = Ref.GetParentIndex(Bone); Parent != Hand; Parent = Ref.GetParentIndex(Parent))
		{
			if (Parent == INDEX_NONE)
			{
				return {};
			}
			Xf = Xf * RefPose[Parent];
		}
		return Xf.GetLocation();
	};

	const TOptional<FVector> Middle = InHand(TEXT("middle_01_r"));
	const TOptional<FVector> Index = InHand(TEXT("index_01_r"));
	const TOptional<FVector> Pinky = InHand(TEXT("pinky_01_r"));
	if (Middle && Index && Pinky)
	{
		HandLength = Middle->Size();
		HandKnuckleDir = Middle->GetSafeNormal();
		HandThumbDir = FVector::VectorPlaneProject(*Index - *Pinky, HandKnuckleDir).GetSafeNormal();
		// Right hand: knuckles forward and thumb up leaves the palm facing left.
		HandPalmDir = FVector::CrossProduct(HandKnuckleDir, HandThumbDir);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("FirstPersonBody: no finger bones under %s, the tool grip will be approximate"), *HandBone.ToString());
	}

	// Handle across the fist along the thumb side, head leaning forward by ToolTiltForward, pick along the knuckles.
	const float Tilt = FMath::DegreesToRadians(ToolTiltForward);
	const FVector Head = HandThumbDir * FMath::Cos(Tilt) + HandKnuckleDir * FMath::Sin(Tilt);
	const FVector Pick = HandKnuckleDir * FMath::Cos(Tilt) - HandThumbDir * FMath::Sin(Tilt);
	const FQuat ToolRotation = FRotationMatrix::MakeFromXY(Pick, -Head).ToQuat();
	const FVector Grip = HandKnuckleDir * HandLength * GripAlongHand + HandPalmDir * GripPalmOffset;
	ToolInHand = FTransform(ToolRotation, Grip - ToolRotation.RotateVector(ToolGripPoint * HeldToolScale), FVector(HeldToolScale));

	FingerChains.Reset();
	for (const TCHAR* Finger : { TEXT("index"), TEXT("middle"), TEXT("ring"), TEXT("pinky"), TEXT("thumb") })
	{
		TArray<int32>& Chain = FingerChains.AddDefaulted_GetRef();
		for (int32 Joint = 1; Joint <= 3; ++Joint)
		{
			Chain.Add(Ref.FindBoneIndex(*FString::Printf(TEXT("%s_0%d_r"), Finger, Joint)));
		}
		if (Chain.Contains(INDEX_NONE))
		{
			Chain.Reset();
		}
	}
}

FTransform UFirstPersonBodyComponent::ViewBoneUnderHand(int32 Bone, int32 Hand, const FTransform& HandXf) const
{
	const FReferenceSkeleton& Ref = ViewBody->GetSkinnedAsset()->GetRefSkeleton();
	FTransform Xf = ViewBody->BoneSpaceTransforms[Bone];
	for (int32 Parent = Ref.GetParentIndex(Bone); Parent != Hand && Parent != INDEX_NONE; Parent = Ref.GetParentIndex(Parent))
	{
		Xf = Xf * ViewBody->BoneSpaceTransforms[Parent];
	}
	return Xf * HandXf;
}

FQuat UFirstPersonBodyComponent::HandRotationFor(const FVector& Knuckles, const FVector& ThumbSide) const
{
	const FQuat InHand = FRotationMatrix::MakeFromXY(HandKnuckleDir, HandThumbDir).ToQuat();
	const FQuat Wanted = FRotationMatrix::MakeFromXY(Knuckles, ThumbSide).ToQuat();
	return Wanted * InHand.Inverse();
}

void UFirstPersonBodyComponent::PoseToolArm(float DeltaTime)
{
	const TArray<FTransform>& Pose = AnimatedBody->GetComponentSpaceTransforms();
	const int32 Upper = AnimatedBody->GetBoneIndex(UpperArmBone);
	const int32 Lower = AnimatedBody->GetBoneIndex(LowerArmBone);
	const int32 Hand = AnimatedBody->GetBoneIndex(HandBone);
	const FReferenceSkeleton& Ref = ViewBody->GetSkinnedAsset()->GetRefSkeleton();
	if (!ViewCamera || !Pose.IsValidIndex(Upper) || !Pose.IsValidIndex(Lower) || !Pose.IsValidIndex(Hand) || !ViewBody->BoneSpaceTransforms.IsValidIndex(Hand)
		|| Ref.GetParentIndex(Hand) != Lower || !Pose.IsValidIndex(Ref.GetParentIndex(Upper)))
	{
		return;
	}

	// Everything below is in the view body's component space.
	const FTransform Camera = ViewCamera->GetComponentTransform().GetRelativeTransform(ViewBody->GetComponentTransform());
	const FVector Up = FVector::UpVector;
	const FVector CameraRight = Camera.GetUnitAxis(EAxis::Y);

	// The fist eases towards its spot in the view, so turning the head leaves the axe trailing a little.
	const FVector GripTarget = Camera.TransformPosition(GripInView);
	if (!bHasSmoothedGrip || DeltaTime <= 0.f)
	{
		SmoothedGrip = GripTarget;
		bHasSmoothedGrip = true;
	}
	else
	{
		SmoothedGrip = FMath::VInterpTo(SmoothedGrip, GripTarget, DeltaTime, HandFollowSpeed);
		SmoothedGrip = GripTarget + (SmoothedGrip - GripTarget).GetClampedToMaxSize(12.f);
	}

	const FVector Shoulder = Pose[Upper].GetLocation();
	const FVector Pole = (Shoulder + SmoothedGrip) * 0.5f + Camera.TransformVectorNoScale(ElbowDirectionInView.GetSafeNormal()) * 40.f;
	const float UpperLength = (Pose[Lower].GetLocation() - Shoulder).Size();
	const float LowerLength = (Pose[Hand].GetLocation() - Pose[Lower].GetLocation()).Size();

	// Hand in line with the forearm (levelled a little, at most MaxWristBend), thumb up and turned slightly in.
	auto HandFor = [&](const FVector& Forearm) -> FQuat
	{
		FVector Knuckles = (Forearm - Up * FVector::DotProduct(Forearm, Up) * WristLevel).GetSafeNormal();
		const float Bend = FMath::Acos(FMath::Clamp(FVector::DotProduct(Knuckles, Forearm), -1.f, 1.f));
		const float MaxBend = FMath::DegreesToRadians(MaxWristBend);
		if (Bend > MaxBend)
		{
			Knuckles = FQuat::Slerp(FQuat::Identity, FQuat::FindBetweenNormals(Forearm, Knuckles), MaxBend / Bend).RotateVector(Forearm);
		}
		FVector Thumb = FVector::VectorPlaneProject(Up, Knuckles).GetSafeNormal();
		FVector Inside = FVector::CrossProduct(Knuckles, Thumb);
		if (FVector::DotProduct(Inside, CameraRight) > 0.f)
		{
			Inside = -Inside;
		}
		const float Inward = FMath::DegreesToRadians(ThumbInward);
		Thumb = Thumb * FMath::Cos(Inward) + Inside * FMath::Sin(Inward);
		return HandRotationFor(Knuckles, Thumb);
	};

	// The grip is offset from the wrist, and the wrist's rotation follows the forearm: settle the two in a few passes.
	const FVector GripInHandSpace = HandKnuckleDir * HandLength * GripAlongHand + HandPalmDir * GripPalmOffset;
	FQuat HandRotation = HandFor((SmoothedGrip - Shoulder).GetSafeNormal());
	FVector Wrist = SmoothedGrip;
	for (int32 Pass = 0; Pass < 3; ++Pass)
	{
		Wrist = SmoothedGrip - HandRotation.RotateVector(GripInHandSpace);
		FVector Elbow, Reached;
		AnimationCore::SolveTwoBoneIK(Shoulder, Shoulder + FVector(UpperLength, 0.f, 0.f), Shoulder + FVector(UpperLength + LowerLength, 0.f, 0.f),
			Pole, Wrist, Elbow, Reached, false, 1.0, 1.0);
		HandRotation = HandFor((Reached - Elbow).GetSafeNormal());
	}

	// Bone lengths kept: out of reach, the arm straightens and the fist stops short.
	BendLimb(UpperArmBone, LowerArmBone, HandBone, Wrist, HandRotation, Pole, 1.f, 0.6f);

	// Where the hand really ended up (the forearm took part of its roll, so rebuild it from the bones).
	const FTransform HandXf = ViewBody->BoneSpaceTransforms[Hand] * ViewBody->BoneSpaceTransforms[Lower] * ViewBody->BoneSpaceTransforms[Upper] * Pose[Ref.GetParentIndex(Upper)];

	// Close the fingers round the handle and wrap the thumb over them.
	const FVector Palm = HandXf.TransformVectorNoScale(HandPalmDir);
	const FVector KnuckleAxis = HandXf.TransformVectorNoScale(HandKnuckleDir);
	const FVector ThumbAxis = HandXf.TransformVectorNoScale(HandThumbDir);
	for (int32 Finger = 0; Finger < FingerChains.Num(); ++Finger)
	{
		const TArray<int32>& Chain = FingerChains[Finger];
		if (Chain.Num() != 3)
		{
			continue;
		}
		const bool bThumb = Finger == FingerChains.Num() - 1;
		const FVector Curl = bThumb ? ThumbCurl : FingerCurl;

		// Fingers hinge about the thumb-side axis; the thumb folds about an axis across its own first bone.
		FVector Axis = ThumbAxis;
		FVector Towards = Palm;
		FVector Along = KnuckleAxis;
		if (bThumb)
		{
			Along = (ViewBoneUnderHand(Chain[1], Hand, HandXf).GetLocation() - ViewBoneUnderHand(Chain[0], Hand, HandXf).GetLocation()).GetSafeNormal();
			Towards = (Palm - ThumbAxis * 0.5f).GetSafeNormal();
			Axis = FVector::CrossProduct(Along, Towards).GetSafeNormal();
		}
		// Curl the way that moves the finger into the palm.
		const float Sign = FVector::DotProduct(FQuat(Axis, 0.3f).RotateVector(Along) - Along, Towards) >= 0.f ? 1.f : -1.f;

		for (int32 Joint = 0; Joint < 3; ++Joint)
		{
			const int32 Bone = Chain[Joint];
			const int32 Parent = Ref.GetParentIndex(Bone);
			const FTransform ParentXf = Parent == Hand ? HandXf : ViewBoneUnderHand(Parent, Hand, HandXf);
			FTransform Xf = ViewBody->BoneSpaceTransforms[Bone] * ParentXf;
			Xf.SetRotation(FQuat(Axis, Sign * FMath::DegreesToRadians(Curl[Joint])) * Xf.GetRotation());
			ViewBody->BoneSpaceTransforms[Bone] = Xf.GetRelativeTransform(ParentXf);
		}
	}

	HeldTool->SetRelativeTransform(ToolInHand * HandXf);
}

void UFirstPersonBodyComponent::BendLimb(FName UpperBone, FName LowerBone, FName EndBone, const FVector& EndLocation, const FQuat& EndRotation, const FVector& Pole, float MaxStretch, float TwistToLower)
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

	if (TwistToLower > 0.f)
	{
		// Roll the lower bone about its own length by part of the end's roll, so the joint at the end twists less.
		FQuat Swing, Twist;
		(EndRotation * (SwingLower * SwingUpper * OldEnd.GetRotation()).Inverse()).ToSwingTwist((NewEnd - NewJoint).GetSafeNormal(), Swing, Twist);
		NewLower.SetRotation(FQuat::Slerp(FQuat::Identity, Twist, TwistToLower) * NewLower.GetRotation());
	}

	FTransform NewEndXf = OldEnd;
	NewEndXf.SetLocation(NewEnd);
	NewEndXf.SetRotation(EndRotation);

	ViewBody->BoneSpaceTransforms[Upper] = NewUpper.GetRelativeTransform(Pose[Parent]);
	ViewBody->BoneSpaceTransforms[Lower] = NewLower.GetRelativeTransform(NewUpper);
	ViewBody->BoneSpaceTransforms[End] = NewEndXf.GetRelativeTransform(NewLower);
}
