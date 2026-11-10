// Copyright Epic Games, Inc. All Rights Reserved.

#include "FirstPersonBodyComponent.h"

#include "Camera/CameraComponent.h"
#include "Components/PoseableMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/GameViewportClient.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Character.h"
#include "Survival/ClimbingComponent.h"
#include "Survival/SnowTrailComponent.h"
#include "Engine/SkeletalMesh.h"
#include "HAL/IConsoleManager.h"
#include "TwoBoneIK.h"

static TAutoConsoleVariable<int32> CVarFeetLog(TEXT("ygg.Feet.Log"), 0, TEXT("1 logs each view-body foot: animated sole height over the ground, plant weight, surface."));
#include "UObject/ConstructorHelpers.h"

UFirstPersonBodyComponent::UFirstPersonBodyComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	// After animation so the head bone is this frame's pose.
	PrimaryComponentTick.TickGroup = TG_PostUpdateWork;

	// No held tool by default: the First Person template's arms and animation are used as they are.
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
		// Culling uses the reference-pose box (an arms-only mesh: down at the hips), not where the arms are posed in front
		// of the camera, so the hands vanished whenever the hips were out of view. Grow the box to cover the view.
		ViewBody->SetBoundsScale(4.f);
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

		// The tool rides in the view body's right fist; PoseArm places it every frame. The left hand rests empty.
		ViewCamera = Camera;
		Arms[0] = FViewArm();
		Arms[0].Upper = UpperArmBone; Arms[0].Lower = LowerArmBone; Arms[0].Hand = HandBone; Arms[0].Suffix = TEXT("r");
		Arms[0].Side = 1.f; Arms[0].bHoldsTool = true;
		Arms[0].RestInView = GripInView; Arms[0].ElbowInView = ElbowDirectionInView;
		Arms[0].FingerCurl = FingerCurl; Arms[0].ThumbCurl = ThumbCurl;
		Arms[1] = FViewArm();
		Arms[1].Upper = TEXT("upperarm_l"); Arms[1].Lower = TEXT("lowerarm_l"); Arms[1].Hand = TEXT("hand_l"); Arms[1].Suffix = TEXT("l");
		Arms[1].Side = -1.f;
		Arms[1].RestInView = LeftHandInView; Arms[1].ElbowInView = LeftElbowDirectionInView;
		Arms[1].FingerCurl = LeftFingerCurl; Arms[1].ThumbCurl = LeftThumbCurl;
		MeasureHand(Arms[0]);
		MeasureHand(Arms[1]);

		// Handle across the fist along the thumb side, head leaning forward by ToolTiltForward, pick along the knuckles.
		const FViewArm& Tool = Arms[0];
		const float Tilt = FMath::DegreesToRadians(ToolTiltForward);
		const FVector Head = Tool.ThumbDir * FMath::Cos(Tilt) + Tool.KnuckleDir * FMath::Sin(Tilt);
		const FVector Pick = Tool.KnuckleDir * FMath::Cos(Tilt) - Tool.ThumbDir * FMath::Sin(Tilt);
		const FQuat ToolRotation = FRotationMatrix::MakeFromXY(Pick, -Head).ToQuat();
		const FVector Grip = Tool.KnuckleDir * Tool.Length * GripAlongHand + Tool.PalmDir * GripPalmOffset;
		ToolInHand = FTransform(ToolRotation, Grip - ToolRotation.RotateVector(ToolGripPoint * HeldToolScale), FVector(HeldToolScale));

		// The same tool in the world body's hand, for everyone else (and mirrors / shadows); hidden from the owner,
		// who sees the first-person one.
		WorldTool = NewObject<UStaticMeshComponent>(Character, TEXT("WorldHeldTool"));
		WorldTool->SetupAttachment(Body, HandBone);
		WorldTool->SetStaticMesh(HeldToolMesh);
		WorldTool->SetRelativeTransform(ToolInHand);
		WorldTool->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		WorldTool->SetOwnerNoSee(true);
		WorldTool->bCastHiddenShadow = true;
		WorldTool->RegisterComponent();
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
		// No tool: the template's own first-person arms and animation are shown untouched, minus the head (a full
		// character mesh on FirstPersonMesh would otherwise put its hair in front of the camera).
		TemplateArms->HideBoneByName(HiddenBone, PBO_None);
	}
	else
	{
		// The owner sees their body, minus the head.
		Body->SetOwnerNoSee(false);
		Body->HideBoneByName(HiddenBone, PBO_None);
	}

	// Own legs in world space: the first-person body (template arms mesh or view body) is drawn with a narrower FOV and
	// pulled towards the camera, so its boots float over the ground and can never press into snow. The owner sees the
	// world body's hips and legs instead (a posable copy, feet placed on the ground), the first-person body without legs.
	ResolveLegBones(Body);
	if ((TemplateArms || ViewBody) && Body->GetBoneIndex(LeftLegBones[0]) != INDEX_NONE && Body->GetBoneIndex(RightLegBones[0]) != INDEX_NONE)
	{
		FeetBody = NewObject<UPoseableMeshComponent>(Character, TEXT("FirstPersonFeet"));
		FeetBody->SetupAttachment(Body);
		FeetBody->SetSkinnedAssetAndUpdate(Body->GetSkeletalMeshAsset());
		FeetBody->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		FeetBody->SetOnlyOwnerSee(true);
		FeetBody->SetCastShadow(false);   // the world body / shadow copy casts the shadow
		FeetBody->SetBoundsScale(2.f);
		FeetBody->RegisterComponent();
		for (int32 Slot = 0; Slot < Body->GetNumMaterials(); ++Slot) { FeetBody->SetMaterial(Slot, Body->GetMaterial(Slot)); }
		// hips and legs only: hide every other branch that leaves the hips (spine, and with it arms and head)
		const FName Hips = Body->GetParentBone(LeftLegBones[0]);
		const FReferenceSkeleton& Ref = Body->GetSkeletalMeshAsset()->GetRefSkeleton();
		const int32 HipsIndex = Ref.FindBoneIndex(Hips);
		for (int32 Bone = 0; Bone < Ref.GetNum(); ++Bone)
		{
			const FName Name = Ref.GetBoneName(Bone);
			if (Ref.GetParentIndex(Bone) == HipsIndex && Name != LeftLegBones[0] && Name != RightLegBones[0]) { FeetBody->HideBoneByName(Name, PBO_None); }
		}
		for (USkinnedMeshComponent* FirstPersonBody : { static_cast<USkinnedMeshComponent*>(TemplateArms.Get()), static_cast<USkinnedMeshComponent*>(ViewBody.Get()) })
		{
			if (FirstPersonBody)
			{
				FirstPersonBody->HideBoneByName(LeftLegBones[0], PBO_None);
				FirstPersonBody->HideBoneByName(RightLegBones[0], PBO_None);
			}
		}
		SetComponentTickEnabled(true);
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

	UpdateFootDrop(DeltaTime);

	// After the camera has moved, so the fist sits at its spot in this frame's view.
	PoseViewBody(DeltaTime);

	if (FeetBody && Character)
	{
		FeetBody->CopyPoseFromSkeletalComponent(Character->GetMesh());
		PlaceFeet();
		FeetBody->MarkRefreshTransformDirty();
	}
}

void UFirstPersonBodyComponent::ResolveLegBones(const USkinnedMeshComponent* Mesh)
{
	if (!Mesh || Mesh->GetBoneIndex(LeftLegBones[0]) != INDEX_NONE) { return; }
	// Mixamo / AI-rigged skeletons (the climber): with or without the "mixamorig:" prefix
	for (const TCHAR* Prefix : { TEXT(""), TEXT("mixamorig:") })
	{
		if (Mesh->GetBoneIndex(FName(FString(Prefix) + TEXT("LeftUpLeg"))) != INDEX_NONE)
		{
			const TCHAR* Names[2][3] = { { TEXT("LeftUpLeg"), TEXT("LeftLeg"), TEXT("LeftFoot") }, { TEXT("RightUpLeg"), TEXT("RightLeg"), TEXT("RightFoot") } };
			for (int32 i = 0; i < 3; ++i)
			{
				LeftLegBones[i] = FName(FString(Prefix) + Names[0][i]);
				RightLegBones[i] = FName(FString(Prefix) + Names[1][i]);
			}
			return;
		}
	}
}

void UFirstPersonBodyComponent::UpdateFootDrop(float DeltaTime)
{
	// On a slope the capsule rests on its curve and the animation assumes flat ground, so the downhill boot hangs in the air.
	// Lower the world body towards the ground; the uphill boot goes a little into the snow, which suits deep snow.
	ACharacter* Character = Cast<ACharacter>(GetOwner());
	USkeletalMeshComponent* Mesh = Character ? Character->GetMesh() : nullptr;
	const UCharacterMovementComponent* Movement = Character ? Character->GetCharacterMovement() : nullptr;
	const UClimbingComponent* Climbing = Character ? Character->FindComponentByClass<UClimbingComponent>() : nullptr;
	if (!Mesh || !Movement || (Climbing && Climbing->IsClimbing()) || !(Movement->IsMovingOnGround() || Movement->IsFalling()))
	{
		return;   // climbing places the body itself; standing on a too-steep slope counts as falling, and still needs its boots down
	}
	if (SoleHeightCm < 0.f)
	{
		SoleHeightCm = MeasureSoleHeight();
		UE_LOG(LogTemp, Display, TEXT("FirstPersonBody: legs %s/%s, sole height %.1f cm"), *LeftLegBones[0].ToString(), *LeftLegBones[2].ToString(), SoleHeightCm);
	}
	constexpr float Share = 0.9f, MaxDrop = 120.f;
	// Per foot: how far its sole is above where it should rest (SnowContactCm into snow, exactly on rock and props)
	float Gaps[2] = { 0.f, 0.f };
	int32 Count = 0;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(FootDrop), false, Character);
	Params.bReturnPhysicalMaterial = true;
	Params.bReturnFaceIndex = true;
	for (const FName* Leg : { LeftLegBones, RightLegBones })
	{
		if (Mesh->GetBoneIndex(Leg[2]) == INDEX_NONE) { continue; }
		const FVector Foot = Mesh->GetSocketLocation(Leg[2]);
		FHitResult Ground;
		if (GetWorld()->LineTraceSingleByChannel(Ground, Foot + FVector(0.f, 0.f, 60.f), Foot - FVector(0.f, 0.f, 200.f), ECC_Visibility, Params))
		{
			Gaps[Count++] = float(Foot.Z - Ground.ImpactPoint.Z) - SoleHeightCm + (USnowTrailComponent::IsSnow(Ground) ? SnowContactCm : 0.f);
		}
	}
	if (Count == 0)
	{
		return;
	}
	// Standing, both feet decide (on a slope the downhill one must reach); walking, a foot clearly higher than the
	// other is in its swing and is left out, or the body would sink with every step.
	const float Low = Count == 2 ? FMath::Min(Gaps[0], Gaps[1]) : Gaps[0];
	const bool bWalking = Character->GetVelocity().Size2D() > 40.f;
	float Gap = Low;
	for (int32 i = 0; i < Count; ++i) { if (!bWalking || Gaps[i] < Low + 20.f) { Gap = FMath::Max(Gap, Gaps[i]); } }
	// a boot well clear of the ground is in the air (a jump, a fall, a teleport): never lower the body after it
	Gap = FMath::Min(Gap, 30.f);
	if (Low > 30.f) { Gap = 0.f; }
	if (CVarFeetLog.GetValueOnGameThread() != 0)
	{
		UE_LOG(LogTemp, Display, TEXT("FootDrop: gaps %.1f %.1f gap %.1f drop %.1f sink %.1f"), Gaps[0], Count > 1 ? Gaps[1] : 0.f, Gap, FootDrop, SinkDepth);
	}
	// The trace reads the body where it is now, so the gap is measured on top of the current drop.
	const float Want = FMath::Clamp(FootDrop + Gap * Share, 0.f, MaxDrop);
	FootDrop = FMath::FInterpTo(FootDrop, Gap > 0.f ? Want : FMath::Max(FootDrop + Gap, 0.f), DeltaTime, 6.f);
	const FVector Down(0.f, 0.f, SinkDepth + FootDrop);
	Mesh->SetRelativeLocation(WorldBodyBaseLocation - Down);
	if (ShadowMesh)
	{
		ShadowMesh->SetRelativeLocation(BodyBaseLocation - Down);
	}
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

	UpdateHandMotion(DeltaTime);
	PoseArm(Arms[0], DeltaTime);
	if (bShowLeftHand)
	{
		PoseArm(Arms[1], DeltaTime);
	}

	ViewBody->MarkRefreshTransformDirty();
}

float UFirstPersonBodyComponent::MeasureSoleHeight() const
{
	// How high the ankle bone sits above the boot's sole: the mannequin foot rig assumes its own (lower) ankle, so the
	// climber's boots floated. Read from this mesh: ankle height in the reference pose over the lowest point of the mesh.
	const ACharacter* Character = Cast<ACharacter>(GetOwner());
	const USkeletalMesh* Mesh = Character && Character->GetMesh() ? Character->GetMesh()->GetSkeletalMeshAsset() : nullptr;
	if (!Mesh) { return 10.f; }
	const FReferenceSkeleton& Ref = Mesh->GetRefSkeleton();
	int32 Bone = Ref.FindBoneIndex(LeftLegBones[2]);
	if (Bone == INDEX_NONE) { return 10.f; }
	FTransform Xf = Ref.GetRefBonePose()[Bone];
	for (int32 Parent = Ref.GetParentIndex(Bone); Parent != INDEX_NONE; Parent = Ref.GetParentIndex(Parent)) { Xf = Xf * Ref.GetRefBonePose()[Parent]; }
	const FBoxSphereBounds Bounds = Mesh->GetImportedBounds();
	const float Sole = float(Xf.GetLocation().Z - (Bounds.Origin.Z - Bounds.BoxExtent.Z));
	return FMath::Clamp(Sole, 3.f, 25.f);
}

void UFirstPersonBodyComponent::PlaceFeet()
{
	// What you see looking down: a planted boot sits a little INTO snow (SnowContactCm, plus the body's sink in powder),
	// so it clearly touches; on rock, ice rubble, tents and props it stands exactly on the surface. A boot in the air
	// keeps the animation. Works on FeetBody (posable copy of the world body, which UpdateFootDrop has already lowered).
	const ACharacter* Character = Cast<ACharacter>(GetOwner());
	const USkeletalMeshComponent* Source = Character ? Character->GetMesh() : nullptr;
	if (!Source || !FeetBody || SoleHeightCm < 0.f)
	{
		return;
	}
	const TArray<FTransform>& Pose = Source->GetComponentSpaceTransforms();
	const FTransform BodyXf = FeetBody->GetComponentTransform();
	const FVector UpCS = BodyXf.InverseTransformVectorNoScale(FVector::UpVector);
	FCollisionQueryParams Params(SCENE_QUERY_STAT(FootPlace), false, GetOwner());
	Params.bReturnPhysicalMaterial = true;
	Params.bReturnFaceIndex = true;   // rock faces of the detail tiles are known by their index (USnowTrailComponent::IsRock)
	for (const FName* Leg : { LeftLegBones, RightLegBones })
	{
		const int32 Thigh = Source->GetBoneIndex(Leg[0]);
		const int32 Knee = Source->GetBoneIndex(Leg[1]);
		const int32 Foot = Source->GetBoneIndex(Leg[2]);
		const int32 Hips = Thigh != INDEX_NONE ? Source->GetBoneIndex(Source->GetParentBone(Leg[0])) : INDEX_NONE;
		if (!Pose.IsValidIndex(Thigh) || !Pose.IsValidIndex(Knee) || !Pose.IsValidIndex(Foot) || !Pose.IsValidIndex(Hips)
			|| !FeetBody->BoneSpaceTransforms.IsValidIndex(Foot))
		{
			continue;
		}
		const FTransform& FootCS = Pose[Foot];
		const FVector FootWS = BodyXf.TransformPosition(FootCS.GetLocation());
		FHitResult Ground;
		if (!GetWorld()->LineTraceSingleByChannel(Ground, FootWS + FVector(0.f, 0.f, 70.f), FootWS - FVector(0.f, 0.f, 120.f), ECC_Visibility, Params))
		{
			continue;
		}
		const bool bSnow = USnowTrailComponent::IsSnow(Ground);
		const float Sink = bSnow ? SnowContactCm + SinkDepth : 0.f;
		const float Above = float(FootWS.Z - SoleHeightCm - Ground.ImpactPoint.Z);   // the animated sole over the ground
		const float Weight = 1.f - FMath::SmoothStep(PlantedBelowCm, PlantedBelowCm + 12.f, Above);
		if (CVarFeetLog.GetValueOnGameThread() != 0)
		{
			UE_LOG(LogTemp, Display, TEXT("Feet: %s above %.1f weight %.2f %s sink %.1f"), *Leg[2].ToString(), Above, Weight, bSnow ? TEXT("snow") : TEXT("hard"), Sink);
		}
		if (Weight <= 0.f)
		{
			continue;
		}
		const float DeltaZ = (-Above - Sink) * Weight;
		const FVector TargetCS = FootCS.GetLocation() + BodyXf.InverseTransformVectorNoScale(FVector(0.f, 0.f, DeltaZ));
		// the planted boot follows the slope, at most 25 degrees
		const FVector NormalCS = BodyXf.InverseTransformVectorNoScale(Ground.ImpactNormal).GetSafeNormal();
		const float Angle = FMath::Acos(FMath::Clamp(float(FVector::DotProduct(UpCS, NormalCS)), -1.f, 1.f));
		const float Limit = Angle > KINDA_SMALL_NUMBER ? FMath::Min(1.f, FMath::DegreesToRadians(25.f) / Angle) : 1.f;
		const FQuat Tilt = FQuat::Slerp(FQuat::Identity, FQuat::FindBetweenNormals(UpCS, NormalCS), Limit * Weight);
		const FVector KneeLocation = Pose[Knee].GetLocation();
		const FVector Bend = (KneeLocation - (Pose[Thigh].GetLocation() + FootCS.GetLocation()) * 0.5f).GetSafeNormal();
		// two-bone solve (no stretch), then swing thigh and shin onto the new knee and foot
		const FTransform& OldUpper = Pose[Thigh];
		const FTransform& OldLower = Pose[Knee];
		FVector NewKnee, NewFoot;
		AnimationCore::SolveTwoBoneIK(OldUpper.GetLocation(), OldLower.GetLocation(), FootCS.GetLocation(), KneeLocation + Bend * 30.f, TargetCS, NewKnee, NewFoot, false, 1.0, 1.0);
		const FQuat SwingUpper = FQuat::FindBetweenVectors(OldLower.GetLocation() - OldUpper.GetLocation(), NewKnee - OldUpper.GetLocation());
		FTransform NewUpper = OldUpper;
		NewUpper.SetRotation(SwingUpper * OldUpper.GetRotation());
		const FQuat SwingLower = FQuat::FindBetweenVectors(SwingUpper.RotateVector(FootCS.GetLocation() - OldLower.GetLocation()), NewFoot - NewKnee);
		FTransform NewLower = OldLower;
		NewLower.SetLocation(NewKnee);
		NewLower.SetRotation(SwingLower * SwingUpper * OldLower.GetRotation());
		FTransform NewFootXf = FootCS;
		NewFootXf.SetLocation(NewFoot);
		NewFootXf.SetRotation(Tilt * FootCS.GetRotation());
		FeetBody->BoneSpaceTransforms[Thigh] = NewUpper.GetRelativeTransform(Pose[Hips]);
		FeetBody->BoneSpaceTransforms[Knee] = NewLower.GetRelativeTransform(NewUpper);
		FeetBody->BoneSpaceTransforms[Foot] = NewFootXf.GetRelativeTransform(NewLower);
	}
}

void UFirstPersonBodyComponent::MeasureHand(FViewArm& Arm)
{
	const FReferenceSkeleton& Ref = ViewBody->GetSkinnedAsset()->GetRefSkeleton();
	const TArray<FTransform>& RefPose = Ref.GetRefBonePose();
	const int32 Hand = Ref.FindBoneIndex(Arm.Hand);
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

	const TOptional<FVector> Middle = InHand(*(TEXT("middle_01_") + Arm.Suffix));
	const TOptional<FVector> Index = InHand(*(TEXT("index_01_") + Arm.Suffix));
	const TOptional<FVector> Pinky = InHand(*(TEXT("pinky_01_") + Arm.Suffix));
	if (Middle && Index && Pinky)
	{
		Arm.Length = Middle->Size();
		Arm.KnuckleDir = Middle->GetSafeNormal();
		Arm.ThumbDir = FVector::VectorPlaneProject(*Index - *Pinky, Arm.KnuckleDir).GetSafeNormal();
		// Right hand: knuckles forward and thumb up leaves the palm facing left; the left hand is the mirror.
		Arm.PalmDir = FVector::CrossProduct(Arm.KnuckleDir, Arm.ThumbDir) * Arm.Side;
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("FirstPersonBody: no finger bones under %s, the grip will be approximate"), *Arm.Hand.ToString());
	}

	// Reference pose in component space, for rebuilding the arm bones from directions plus a shared palm roll.
	auto InComponent = [&](int32 Bone)
	{
		FTransform Xf = RefPose[Bone];
		for (int32 Parent = Ref.GetParentIndex(Bone); Parent != INDEX_NONE; Parent = Ref.GetParentIndex(Parent))
		{
			Xf = Xf * RefPose[Parent];
		}
		return Xf;
	};
	const int32 UpperBone = Ref.FindBoneIndex(Arm.Upper);
	const int32 LowerBone = Ref.FindBoneIndex(Arm.Lower);
	if (Hand != INDEX_NONE && UpperBone != INDEX_NONE && LowerBone != INDEX_NONE)
	{
		const FTransform RefUpper = InComponent(UpperBone);
		const FTransform RefLower = InComponent(LowerBone);
		const FTransform RefHand = InComponent(Hand);
		Arm.RefPalm = RefHand.TransformVectorNoScale(Arm.PalmDir);
		Arm.RefUpperDir = (RefLower.GetLocation() - RefUpper.GetLocation()).GetSafeNormal();
		Arm.RefLowerDir = (RefHand.GetLocation() - RefLower.GetLocation()).GetSafeNormal();
		Arm.RefUpperRotation = RefUpper.GetRotation();
		Arm.RefLowerRotation = RefLower.GetRotation();
	}

	Arm.Fingers.Reset();
	for (const TCHAR* Finger : { TEXT("index"), TEXT("middle"), TEXT("ring"), TEXT("pinky"), TEXT("thumb") })
	{
		TArray<int32>& Chain = Arm.Fingers.AddDefaulted_GetRef();
		for (int32 Joint = 1; Joint <= 3; ++Joint)
		{
			Chain.Add(Ref.FindBoneIndex(*FString::Printf(TEXT("%s_0%d_%s"), Finger, Joint, *Arm.Suffix)));
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

void UFirstPersonBodyComponent::UpdateHandMotion(float DeltaTime)
{
	const ACharacter* Character = Cast<ACharacter>(GetOwner());
	if (!Character || DeltaTime <= 0.f)
	{
		return;
	}
	const UCharacterMovementComponent* Movement = Character->GetCharacterMovement();
	const bool bFalling = Movement && Movement->IsFalling();
	const float Speed = Character->GetVelocity().Size2D();

	// Walk rhythm follows the distance actually covered; the motion fades in and out with speed (none in the air).
	const float WalkTarget = bFalling ? 0.f : FMath::Clamp(Speed / WalkSpeedForFullBob, 0.f, 1.f);
	WalkAlpha = FMath::FInterpTo(WalkAlpha, WalkTarget, DeltaTime, 6.f);
	SprintAlpha = FMath::FInterpTo(SprintAlpha, (!bFalling && Speed > SprintSpeed) ? 1.f : 0.f, DeltaTime, 5.f);
	if (!bFalling)
	{
		StridePhase = FMath::Fmod(StridePhase + Speed * DeltaTime / StrideLength * UE_TWO_PI, UE_TWO_PI);
	}
	BreathPhase = FMath::Fmod(BreathPhase + BreathRate * DeltaTime * UE_TWO_PI, UE_TWO_PI);

	// Turning: the hands lag against the turn (view yaw right -> hands drift left), springing back when it stops.
	const FRotator View = Character->GetControlRotation();
	FVector2D LagTarget = FVector2D::ZeroVector;
	if (bHasLastViewRotation)
	{
		const float YawRate = FRotator::NormalizeAxis(View.Yaw - LastViewRotation.Yaw) / DeltaTime;
		const float PitchRate = FRotator::NormalizeAxis(View.Pitch - LastViewRotation.Pitch) / DeltaTime;
		LagTarget = FVector2D(-YawRate, -PitchRate) * (TurnSway / 100.f);
		LagTarget = LagTarget.ClampAxes(-TurnSwayMax, TurnSwayMax);
	}
	LastViewRotation = View;
	bHasLastViewRotation = true;
	TurnLag = FMath::Vector2DInterpTo(TurnLag, LagTarget, DeltaTime, 8.f);

	// Landing: kick the hands down by the fall speed, then a damped spring brings them back.
	if (bFalling)
	{
		LastFallSpeed = FMath::Max(0.f, -Character->GetVelocity().Z);
	}
	else if (bWasFalling)
	{
		DipVelocity -= LastFallSpeed / 1000.f * LandingDip * 12.f;
	}
	bWasFalling = bFalling;
	DipVelocity += (-DipOffset * 120.f - DipVelocity * 14.f) * DeltaTime;
	DipOffset = FMath::Clamp(DipOffset + DipVelocity * DeltaTime, -LandingDip * 2.f, LandingDip);
}

FVector UFirstPersonBodyComponent::HandTargetInView(const FViewArm& Arm) const
{
	FVector Target = Arm.RestInView;

	// Rest height from the real screen shape, so a wide monitor (short vertical view) doesn't push the hands off the bottom.
	UGameViewportClient* Viewport = GetWorld() ? GetWorld()->GetGameViewport() : nullptr;
	if (HandScreenDrop > 0.f && Viewport && ViewCamera)
	{
		FVector2D Size;
		Viewport->GetViewportSize(Size);
		if (Size.X > 1.f && Size.Y > 1.f)
		{
			const float Fov = bFirstPersonRendering ? FirstPersonFieldOfView : ViewCamera->FieldOfView;
			const float HalfVertical = FMath::Atan(FMath::Tan(FMath::DegreesToRadians(Fov * 0.5f)) * Size.Y / Size.X);
			Target.Z = -Target.X * FMath::Tan(HalfVertical * HandScreenDrop);
		}
	}

	// Two bobs per stride (one per footfall); the arms swing in opposition, the tool hand a little less.
	const float Swing = Arm.bHoldsTool ? 0.6f : 1.f;
	Target.X += FMath::Sin(StridePhase) * WalkSwing * Swing * Arm.Side * WalkAlpha;
	Target.Z -= FMath::Abs(FMath::Sin(StridePhase)) * WalkBob * WalkAlpha;
	Target.Y += FMath::Cos(StridePhase) * WalkSway * WalkAlpha;

	// Breathing shows when standing still and fades out while walking.
	Target.Z += FMath::Sin(BreathPhase) * BreathAmount * (1.f - WalkAlpha);

	Target.Y += TurnLag.X;
	Target.Z += TurnLag.Y + DipOffset;

	// Running: hands drop and pull in towards the body.
	Target.Z -= SprintDrop * SprintAlpha;
	Target.X -= SprintDrop * 0.6f * SprintAlpha;
	return Target;
}

FQuat UFirstPersonBodyComponent::HandRotationFor(const FViewArm& Arm, const FVector& Knuckles, const FVector& ThumbSide)
{
	const FQuat InHand = FRotationMatrix::MakeFromXY(Arm.KnuckleDir, Arm.ThumbDir).ToQuat();
	const FQuat Wanted = FRotationMatrix::MakeFromXY(Knuckles, ThumbSide).ToQuat();
	return Wanted * InHand.Inverse();
}

void UFirstPersonBodyComponent::PoseArm(FViewArm& Arm, float DeltaTime)
{
	const TArray<FTransform>& Pose = AnimatedBody->GetComponentSpaceTransforms();
	const int32 Upper = AnimatedBody->GetBoneIndex(Arm.Upper);
	const int32 Lower = AnimatedBody->GetBoneIndex(Arm.Lower);
	const int32 Hand = AnimatedBody->GetBoneIndex(Arm.Hand);
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

	// The fist eases towards its spot in the view, so turning the head leaves the hands trailing a little.
	const FVector GripTarget = Camera.TransformPosition(HandTargetInView(Arm));
	FVector& SmoothedGrip = Arm.SmoothedGrip;
	if (!Arm.bHasSmoothedGrip || DeltaTime <= 0.f)
	{
		SmoothedGrip = GripTarget;
		Arm.bHasSmoothedGrip = true;
	}
	else
	{
		SmoothedGrip = FMath::VInterpTo(SmoothedGrip, GripTarget, DeltaTime, HandFollowSpeed);
		SmoothedGrip = GripTarget + (SmoothedGrip - GripTarget).GetClampedToMaxSize(3.f);
	}

	const FVector Shoulder = Pose[Upper].GetLocation();
	const FVector Pole = (Shoulder + SmoothedGrip) * 0.5f + Camera.TransformVectorNoScale(Arm.ElbowInView.GetSafeNormal()) * 40.f;
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
		// "Inside" = towards the body's centre line: left of the view for the right hand, right of it for the left.
		FVector Inside = FVector::CrossProduct(Knuckles, Thumb);
		if (FVector::DotProduct(Inside, CameraRight) * Arm.Side > 0.f)
		{
			Inside = -Inside;
		}
		const float Inward = FMath::DegreesToRadians(ThumbInward);
		Thumb = Thumb * FMath::Cos(Inward) + Inside * FMath::Sin(Inward);
		return HandRotationFor(Arm, Knuckles, Thumb);
	};

	// The grip is offset from the wrist, and the wrist's rotation follows the forearm: settle the two in a few passes.
	const FVector GripInHandSpace = Arm.KnuckleDir * Arm.Length * GripAlongHand + Arm.PalmDir * GripPalmOffset;
	FQuat HandRotation = HandFor((SmoothedGrip - Shoulder).GetSafeNormal());
	FVector Elbow = Pose[Lower].GetLocation();
	FVector Reached = SmoothedGrip;
	for (int32 Pass = 0; Pass < 3; ++Pass)
	{
		const FVector Wrist = SmoothedGrip - HandRotation.RotateVector(GripInHandSpace);
		// Bone lengths kept: out of reach, the arm straightens and the fist stops short.
		AnimationCore::SolveTwoBoneIK(Shoulder, Shoulder + FVector(UpperLength, 0.f, 0.f), Shoulder + FVector(UpperLength + LowerLength, 0.f, 0.f),
			Pole, Wrist, Elbow, Reached, false, 1.0, 1.0);
		HandRotation = HandFor((Reached - Elbow).GetSafeNormal());
	}

	// Upper arm, forearm and hand all take their roll from the same palm direction, so nothing between shoulder and
	// fingers twists against its neighbour (swinging each bone separately from the animation's pose wrung the arm).
	const FVector PalmNow = HandRotation.RotateVector(Arm.PalmDir);
	auto Aim = [&PalmNow, &Arm](const FVector& Dir, const FVector& RefDir, const FQuat& RefRotation)
	{
		const FVector Side = FVector::VectorPlaneProject(PalmNow, Dir).GetSafeNormal();
		const FVector RefSide = FVector::VectorPlaneProject(Arm.RefPalm, RefDir).GetSafeNormal();
		return FRotationMatrix::MakeFromXY(Dir, Side).ToQuat() * FRotationMatrix::MakeFromXY(RefDir, RefSide).ToQuat().Inverse() * RefRotation;
	};
	const int32 Clavicle = Ref.GetParentIndex(Upper);
	const FTransform NewUpper(Aim((Elbow - Shoulder).GetSafeNormal(), Arm.RefUpperDir, Arm.RefUpperRotation), Shoulder, Pose[Upper].GetScale3D());
	const FTransform NewLower(Aim((Reached - Elbow).GetSafeNormal(), Arm.RefLowerDir, Arm.RefLowerRotation), Elbow, Pose[Lower].GetScale3D());
	const FTransform HandXf(HandRotation, Reached, Pose[Hand].GetScale3D());
	ViewBody->BoneSpaceTransforms[Upper] = NewUpper.GetRelativeTransform(Pose[Clavicle]);
	ViewBody->BoneSpaceTransforms[Lower] = NewLower.GetRelativeTransform(NewUpper);
	ViewBody->BoneSpaceTransforms[Hand] = HandXf.GetRelativeTransform(NewLower);

	// Close the fingers round the handle and wrap the thumb over them.
	const FVector Palm = HandXf.TransformVectorNoScale(Arm.PalmDir);
	const FVector KnuckleAxis = HandXf.TransformVectorNoScale(Arm.KnuckleDir);
	const FVector ThumbAxis = HandXf.TransformVectorNoScale(Arm.ThumbDir);
	for (int32 Finger = 0; Finger < Arm.Fingers.Num(); ++Finger)
	{
		const TArray<int32>& Chain = Arm.Fingers[Finger];
		if (Chain.Num() != 3)
		{
			continue;
		}
		const bool bThumb = Finger == Arm.Fingers.Num() - 1;
		const FVector Curl = bThumb ? Arm.ThumbCurl : Arm.FingerCurl;

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

	if (Arm.bHoldsTool)
	{
		HeldTool->SetRelativeTransform(ToolInHand * HandXf);
	}
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
