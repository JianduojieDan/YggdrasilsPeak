// Copyright Epic Games, Inc. All Rights Reserved.

#include "FirstPersonBodyComponent.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/PoseableMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/GameViewportClient.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Character.h"
#include "Survival/ClimbingComponent.h"
#include "Survival/SnowTrailComponent.h"
#include "Engine/SkeletalMesh.h"
#include "HAL/IConsoleManager.h"
#include "TwoBoneIK.h"
#include "UObject/ConstructorHelpers.h"

static TAutoConsoleVariable<int32> CVarFeetLog(TEXT("ygg.Feet.Log"), 0,
	TEXT("Boots on the ground: 1 = log 4 times a second, 2 = every frame. Slope under the capsule and how much its round bottom lifts the body, ")
	TEXT("hips drop (and its target), powder sink; per boot the surface, the sole's gap to it after the fit (0 on rock, -SnowContactCm - sink in snow, ")
	TEXT("positive = floating), the plain animation's gap, the lift, the animated height, plant weight and tilt."));
static TAutoConsoleVariable<int32> CVarFeetWorldIK(TEXT("ygg.Feet.WorldIK"), 1,
	TEXT("1 = what others, mirrors and the shadow see is a posed copy of the body with its legs fitted to the ground like your own; 0 = the plain animated body (only lowered)."));
static TAutoConsoleVariable<int32> CVarFeetViewDrop(TEXT("ygg.Feet.ViewDrop"), 1,
	TEXT("1 = the first-person body and the eye come down with the hips on a slope (your legs stay under your jacket); 0 = they stay at the capsule's height (legs hang below on steep ground)."));

namespace
{
	// Testing the feet on slopes (the floating reports were on steep snow at Camp 3 and below the summit): stands the player
	// on the nearest steep but walkable patch of terrain, facing across the slope and looking down at the boots.
	FAutoConsoleCommandWithWorldAndArgs GFeetGoSteepCommand(
		TEXT("ygg.Feet.GoSteep"),
		TEXT("ygg.Feet.GoSteep [min slope deg = 35] [radius m = 80] [surface: 0 any, 1 snow, 2 rock] - stand on the nearest steep walkable slope, facing across it (testing the feet on slopes)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
			ACharacter* Character = PC ? Cast<ACharacter>(PC->GetPawn()) : nullptr;
			if (!Character || !Character->GetCharacterMovement() || !Character->GetCapsuleComponent())
			{
				return;
			}
			const float MinDeg = Args.Num() > 0 ? FCString::Atof(*Args[0]) : 35.f;
			const float Radius = (Args.Num() > 1 ? FCString::Atof(*Args[1]) : 80.f) * 100.f;
			const int32 Surface = Args.Num() > 2 ? FCString::Atoi(*Args[2]) : 0;
			// steeper than the walkable limit (44.8 deg, 58 clipped to a rope) the climber slides instead of standing
			const float MaxDeg = Character->GetCharacterMovement()->GetWalkableFloorAngle() - 2.f;
			if (MinDeg >= MaxDeg)
			{
				UE_LOG(LogTemp, Warning, TEXT("GoSteep: %.0f deg is past the walkable limit here (%.0f deg)"), MinDeg, MaxDeg);
				return;
			}
			struct FSteepProbe
			{
				FVector Point;
				FVector Normal;
				float Slope;
				bool bSnow;
				bool bRock;
			};
			const FVector Me = Character->GetActorLocation();
			FCollisionQueryParams Params(SCENE_QUERY_STAT(FeetGoSteep), false, Character);
			Params.bReturnPhysicalMaterial = true;
			Params.bReturnFaceIndex = true;   // rock faces of the detail tiles are known by their index (USnowTrailComponent::IsRock)
			const float Step = 200.f;
			const int32 N = FMath::Clamp(FMath::CeilToInt(Radius / Step), 1, 60);
			TMap<FIntPoint, FSteepProbe> Probes;
			for (int32 y = -N; y <= N; ++y)
			{
				for (int32 x = -N; x <= N; ++x)
				{
					const FVector P = Me + FVector(x * Step, y * Step, 0.f);
					FHitResult Hit;
					if (World->LineTraceSingleByChannel(Hit, P + FVector(0.f, 0.f, 20000.f), P - FVector(0.f, 0.f, 20000.f), ECC_Visibility, Params)
						&& USnowTrailComponent::IsTerrain(Hit))
					{
						const float Slope = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(float(Hit.ImpactNormal.Z), -1.f, 1.f)));
						Probes.Add(FIntPoint(x, y), FSteepProbe{ Hit.ImpactPoint, Hit.ImpactNormal, Slope, USnowTrailComponent::IsSnow(Hit), USnowTrailComponent::IsRock(Hit) });
					}
				}
			}
			// nearest spot in range whose four neighbours (2 m away) are steep too: a slope, not one tilted facet
			const FSteepProbe* Best = nullptr;
			double BestDistance = TNumericLimits<double>::Max();
			for (const TPair<FIntPoint, FSteepProbe>& Probe : Probes)
			{
				const FSteepProbe& S = Probe.Value;
				if (S.Slope < MinDeg || S.Slope > MaxDeg || (Surface == 1 && !S.bSnow) || (Surface == 2 && !S.bRock))
				{
					continue;
				}
				bool bSlope = true;
				for (const FIntPoint& Side : { FIntPoint(1, 0), FIntPoint(-1, 0), FIntPoint(0, 1), FIntPoint(0, -1) })
				{
					const FSteepProbe* Near = Probes.Find(Probe.Key + Side);
					if (!Near || Near->Slope < MinDeg - 8.f || Near->Slope > MaxDeg + 2.f)
					{
						bSlope = false;
						break;
					}
				}
				const double Distance = FVector::DistSquared2D(S.Point, Me);
				if (bSlope && Distance < BestDistance)
				{
					BestDistance = Distance;
					Best = &S;
				}
			}
			if (!Best)
			{
				UE_LOG(LogTemp, Warning, TEXT("GoSteep: no %s slope of %.0f-%.0f deg within %.0f m (%d probes)"),
					Surface == 1 ? TEXT("snow") : (Surface == 2 ? TEXT("rock") : TEXT("terrain")), MinDeg, MaxDeg, Radius / 100.f, Probes.Num());
				return;
			}
			const FVector Uphill = -FVector(Best->Normal.X, Best->Normal.Y, 0.f).GetSafeNormal();
			const float UphillYaw = float(Uphill.Rotation().Yaw);
			const float AcrossYaw = float(FRotator::NormalizeAxis(UphillYaw + 90.f));
			// the capsule's round bottom meets the slope uphill of its centre: start high enough to clear it, then drop a little
			const float Up = Character->GetSimpleCollisionHalfHeight() + Character->GetCapsuleComponent()->GetScaledCapsuleRadius() * FMath::Tan(FMath::DegreesToRadians(Best->Slope)) + 15.f;
			Character->TeleportTo(Best->Point + FVector(0.f, 0.f, Up), FRotator(0.f, AcrossYaw, 0.f));
			PC->SetControlRotation(FRotator(-35.f, AcrossYaw, 0.f));
			const FVector At = Character->GetActorLocation();
			UE_LOG(LogTemp, Display, TEXT("GoSteep: %.1f deg %s at %s. Again: BugItGo %.0f %.0f %.0f -35 %.0f 0. Walk with ygg.Snow.AutoWalk 0.5 and ygg.Snow.AutoYaw %.0f (across), %.0f (uphill) or %.0f (downhill)."),
				Best->Slope, Best->bRock ? TEXT("rock") : (Best->bSnow ? TEXT("snow") : TEXT("hard ground")), *Best->Point.ToCompactString(),
				At.X, At.Y, At.Z, AcrossYaw, AcrossYaw, UphillYaw, FRotator::NormalizeAxis(UphillYaw + 180.f));
		}));
}

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
	ClimbingComponent = Character->FindComponentByClass<UClimbingComponent>();

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
	for (int32 Side = 0; Side < 2; ++Side)
	{
		const FName* Bones = Side == 0 ? LeftLegBones : RightLegBones;
		FLegFit& Leg = Legs[Side];
		Leg.Thigh = Body->GetBoneIndex(Bones[0]);
		Leg.Knee = Body->GetBoneIndex(Bones[1]);
		Leg.Foot = Body->GetBoneIndex(Bones[2]);
		Leg.Hips = Leg.Thigh != INDEX_NONE ? Body->GetBoneIndex(Body->GetParentBone(Bones[0])) : INDEX_NONE;
	}
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
		bBodyHiddenFromOwner = Body->bOwnerNoSee || Body->FirstPersonPrimitiveType == EFirstPersonPrimitiveType::WorldSpaceRepresentation;
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

		// What others, mirrors and the shadow see: the animated world body can't have its legs bent onto a slope from code
		// (its foot rig never ran on these bone names), so a posed copy with the same fitted legs as FeetBody stands in for
		// it, and the world body keeps animating unseen (the copies, the camera and the footprints read its bones). Only with
		// the template layout, where the world body itself casts the shadow; otherwise ShadowMesh (its own animation) does
		// and the world body keeps the plain drop.
		if (bWorldBodyFootIK && TemplateArms)
		{
			WorldBody = NewObject<UPoseableMeshComponent>(Character, TEXT("WorldBodyOnGround"));
			WorldBody->SetupAttachment(Body);
			WorldBody->SetSkinnedAssetAndUpdate(Body->GetSkeletalMeshAsset());
			WorldBody->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			WorldBody->SetGenerateOverlapEvents(false);
			// seen, lit and shadowed exactly like the world body (SyncWorldBody keeps the visibility in step later)
			WorldBody->bOwnerNoSee = Body->bOwnerNoSee;
			WorldBody->bOnlyOwnerSee = Body->bOnlyOwnerSee;
			WorldBody->FirstPersonPrimitiveType = Body->FirstPersonPrimitiveType;
			WorldBody->CastShadow = false;   // hidden for now; a hidden copy with a hidden shadow would still draw one
			WorldBody->bCastDynamicShadow = Body->bCastDynamicShadow;
			WorldBody->bCastHiddenShadow = Body->bCastHiddenShadow;
			WorldBody->bCastContactShadow = Body->bCastContactShadow;
			WorldBody->bCastInsetShadow = Body->bCastInsetShadow;
			WorldBody->bCastFarShadow = Body->bCastFarShadow;
			WorldBody->bSelfShadowOnly = Body->bSelfShadowOnly;
			WorldBody->bReceivesDecals = Body->bReceivesDecals;
			WorldBody->bVisibleInRayTracing = Body->bVisibleInRayTracing;
			WorldBody->bRenderCustomDepth = Body->bRenderCustomDepth;
			WorldBody->CustomDepthStencilValue = Body->CustomDepthStencilValue;
			WorldBody->LightingChannels = Body->LightingChannels;
			WorldBody->BoundsScale = Body->BoundsScale;
			WorldBody->SetHiddenInGame(true);   // SyncWorldBody shows it and hides the world body in the same frame
			WorldBody->RegisterComponent();
			for (int32 Slot = 0; Slot < Body->GetNumMaterials(); ++Slot) { WorldBody->SetMaterial(Slot, Body->GetMaterial(Slot)); }
			bBodyCastShadow = Body->CastShadow;
			// unseen, the world body must still animate and move its bones every frame
			Body->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
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

	// Ground under each boot and the hips drop; the boots themselves are fitted below.
	UpdateFootDrop(DeltaTime);

	// After the camera has moved, so the fist sits at its spot in this frame's view.
	PoseViewBody(DeltaTime);

	if (Character)
	{
		SyncWorldBody(Character->GetMesh());
		PlaceFeet();
	}
	LogFeet(DeltaTime);
}

void UFirstPersonBodyComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// the world body gets its own look back (the posed copy stood in for it)
	if (bWorldBodyShown)
	{
		const ACharacter* Character = Cast<ACharacter>(GetOwner());
		if (USkeletalMeshComponent* Body = Character ? Character->GetMesh() : nullptr)
		{
			Body->SetHiddenInGame(false);
			Body->SetCastShadow(bBodyCastShadow);
		}
		bWorldBodyShown = false;
	}
	Super::EndPlay(EndPlayReason);
}

bool UFirstPersonBodyComponent::IsClimbing() const
{
	// ExpeditionComponent::BeginPlay adds the climbing component, which may come after this BeginPlay: until it is found,
	// look again (missing it, the drop moved the body every frame while climbing and undid ClimbingComponent::FitBodyToFace)
	if (!ClimbingComponent.IsValid())
	{
		if (const AActor* Owner = GetOwner())
		{
			ClimbingComponent = Owner->FindComponentByClass<UClimbingComponent>();
		}
	}
	const UClimbingComponent* Climbing = Cast<UClimbingComponent>(ClimbingComponent.Get());
	return Climbing && Climbing->IsClimbing();
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
	// Why the boots floated on steep slopes: the capsule rests on its round bottom, which meets a slope uphill of its centre,
	// so the whole body stands R * (1/cos(slope) - 1) higher than on flat ground (radius 34: 8 cm at 35 deg, 14 at 45, 30 at
	// 58 on a rope), and the downhill boot is lower again by the slope times its distance from the centre. The old drop
	// left out a boot more than 20 cm over the other while walking (on a slope, that is the planted downhill boot), gave up
	// when both were more than 30 cm up, and the world body had no knee to bend, so on steep ground it was hardly lowered.
	// Now each boot's ground is found once here (PlaceFeet uses it too), the hips come down until the lower boot reaches
	// its ground, and PlaceFeet lifts the other boot onto its own.
	ACharacter* Character = Cast<ACharacter>(GetOwner());
	USkeletalMeshComponent* Body = Character ? Character->GetMesh() : nullptr;
	const UCharacterMovementComponent* Movement = Character ? Character->GetCharacterMovement() : nullptr;
	for (FLegFit& Leg : Legs)
	{
		Leg.bHit = false;
	}
	if (!Body || !Movement || DeltaTime <= 0.f)
	{
		return;
	}
	if (IsClimbing())
	{
		// climbing places the body itself (ClimbingComponent::FitBodyToFace); the fit fades back in afterwards
		FeetAlpha = 0.f;
		return;
	}
	if (SoleHeightCm < 0.f)
	{
		SoleHeightCm = MeasureSoleHeight();
		UE_LOG(LogTemp, Display, TEXT("FirstPersonBody: legs %s/%s, sole height %.1f cm"), *LeftLegBones[0].ToString(), *LeftLegBones[2].ToString(), SoleHeightCm);
	}
	// movement switched off (dead, resting in a tent) still stands where it stopped: boots stay on the ground (easing the drop
	// out there lifted the body, and now the eye, off a slope while the death screen came up)
	const bool bOnGround = Movement->IsMovingOnGround() || Movement->MovementMode == MOVE_None;
	FeetAlpha = FMath::FInterpTo(FeetAlpha, bOnGround ? 1.f : 0.f, DeltaTime, 10.f);
	if (Movement->CurrentFloor.bBlockingHit)
	{
		FloorGapCm = Movement->CurrentFloor.FloorDist;
	}
	// The first-person body (the camera rides on its head) comes down with the hips, minus the drop flat ground already has
	// (the floor gap), so on flat ground the eye stays where it was and on a slope the legs stay under the jacket.
	auto FollowWithView = [this, DeltaTime]()
	{
		const float Target = TemplateArms && CVarFeetViewDrop.GetValueOnGameThread() != 0 ? FMath::Max(FootDrop - FloorGapCm, 0.f) : 0.f;
		ViewDrop = FMath::FInterpTo(ViewDrop, Target, DeltaTime, 6.f);
	};

	const TArray<FTransform>& Pose = Body->GetComponentSpaceTransforms();
	const FTransform BodyXf = Body->GetComponentTransform();
	for (FLegFit& Leg : Legs)
	{
		if (Pose.IsValidIndex(Leg.Foot))
		{
			// the animated sole over the animation's own flat floor: about 0 when planted, up in the swing
			Leg.AnimHeight = float(BodyXf.TransformPosition(Pose[Leg.Foot].GetLocation()).Z - BodyXf.GetLocation().Z) - SoleHeightCm;
		}
	}
	if (!bOnGround)
	{
		// in the air (a jump, a fall, sliding off ground too steep to stand on): no new drop, ease back to the plain body
		FootDropTarget = 0.f;
		FootDrop = FMath::FInterpTo(FootDrop, 0.f, DeltaTime, 4.f);
		for (FLegFit& Leg : Legs)
		{
			Leg.Alpha = FMath::FInterpTo(Leg.Alpha, 0.f, DeltaTime, 10.f);
		}
		FollowWithView();
		ApplyWorldBodyOffset();
		return;
	}

	// The animation's flat floor without the slope drop (already sunk into powder by SinkDepth), from the capsule itself,
	// so it is right whatever moved the body this frame.
	const USceneComponent* Parent = Body->GetAttachParent();
	const FTransform ParentXf = Parent ? Parent->GetComponentTransform() : FTransform::Identity;
	const double FloorZ = ParentXf.TransformPosition(WorldBodyBaseLocation - FVector(0.f, 0.f, SinkDepth)).Z;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(FootDrop), false, Character);
	Params.bReturnPhysicalMaterial = true;
	Params.bReturnFaceIndex = true;   // rock faces of the detail tiles are known by their index (USnowTrailComponent::IsRock)
	const float MinNormalZ = FMath::Cos(FMath::DegreesToRadians(65.f));
	// standing, a boot resting a little off the animation's floor must still reach (as before); walking, only one barely
	// off it, or every swing would pull the body down
	const bool bWalking = Character->GetVelocity().Size2D() > 40.f;
	const float RestFrom = bWalking ? 2.f : PlantedBelowCm;
	const float RestTo = bWalking ? 8.f : PlantedBelowCm + 12.f;
	float MaxNeed = 0.f, MinNeed = 0.f;
	int32 Count = 0;
	for (FLegFit& Leg : Legs)
	{
		if (!Pose.IsValidIndex(Leg.Foot))
		{
			continue;
		}
		const FVector Foot = BodyXf.TransformPosition(Pose[Leg.Foot].GetLocation());
		// straight down through the boot from about the capsule's bottom: free air even where the boot is sunk into a slope
		FHitResult Ground;
		const bool bGround = GetWorld()->LineTraceSingleByChannel(Ground, FVector(Foot.X, Foot.Y, FloorZ + 70.0),
			FVector(Foot.X, Foot.Y, FloorZ - MaxPelvisDropCm - 80.0), ECC_Visibility, Params) && Ground.ImpactNormal.Z >= MinNormalZ;
		// a boot over a drop or against a wall keeps the animation, eased out rather than cut
		Leg.Alpha = FMath::FInterpTo(Leg.Alpha, bGround ? 1.f : 0.f, DeltaTime, 12.f);
		if (!bGround)
		{
			continue;
		}
		const bool bSnow = USnowTrailComponent::IsSnow(Ground);
		const float Contact = bSnow ? SnowContactCm + SinkDepth : 0.f;
		// stepping from powder onto rock changes how deep the boot goes: ease it, don't pop
		Leg.Contact = Leg.bHasGround ? FMath::FInterpTo(Leg.Contact, Contact, DeltaTime, 15.f) : Contact;
		Leg.bHit = true;
		Leg.bHasGround = true;
		Leg.bSnow = bSnow;
		Leg.Point = Ground.ImpactPoint;
		Leg.Normal = Ground.ImpactNormal;
		Leg.Offset = float(Ground.ImpactPoint.Z - Leg.Contact - FloorZ);
		// how far the hips must come down for this boot to reach its ground (with its resting height); a boot in its swing
		// counts only with the ground under it
		const float Resting = Leg.AnimHeight * (1.f - FMath::SmoothStep(RestFrom, RestTo, Leg.AnimHeight));
		const float Need = Resting - Leg.Offset;
		MaxNeed = Count == 0 ? Need : FMath::Max(MaxNeed, Need);
		MinNeed = Count == 0 ? Need : FMath::Min(MinNeed, Need);
		++Count;
	}
	// The lower boot decides; the higher one is never lifted more than MaxFootLiftCm for it (at a ledge the lower one hangs).
	// Never raised above the plain body: a boot on higher ground is lifted instead.
	FootDropTarget = Count > 0 ? FMath::Clamp(FMath::Min(MaxNeed, MinNeed + MaxFootLiftCm), 0.f, MaxPelvisDropCm) : 0.f;
	// down quickly so the lower boot doesn't hang, up gently (the knees take it) so a slope doesn't pump the body each step
	FootDrop = FMath::FInterpTo(FootDrop, FootDropTarget, DeltaTime, FootDropTarget > FootDrop ? PelvisDropSpeed : PelvisRiseSpeed);
	FollowWithView();
	ApplyWorldBodyOffset();

	// for the log: each boot's gap with the plain (lowered) animation; PlaceFeet replaces FinalGap with the fitted one
	const FTransform LoweredXf = Body->GetComponentTransform();
	for (FLegFit& Leg : Legs)
	{
		if (Leg.bHasGround && Pose.IsValidIndex(Leg.Foot))
		{
			Leg.AnimGap = float(LoweredXf.TransformPosition(Pose[Leg.Foot].GetLocation()).Z - SoleHeightCm - Leg.Point.Z);
			Leg.FinalGap = Leg.AnimGap;
		}
	}
}

void UFirstPersonBodyComponent::ApplyWorldBodyOffset()
{
	ACharacter* Character = Cast<ACharacter>(GetOwner());
	USkeletalMeshComponent* Body = Character ? Character->GetMesh() : nullptr;
	if (!Body || IsClimbing())
	{
		return;   // climbing places the body itself (ClimbingComponent::FitBodyToFace)
	}
	const FVector Down(0.f, 0.f, SinkDepth + FootDrop);
	Body->SetRelativeLocation(WorldBodyBaseLocation - Down);
	if (ShadowMesh)
	{
		ShadowMesh->SetRelativeLocation(BodyBaseLocation - Down);
	}
	if (TemplateArms)
	{
		// the template's first-person body and the eye on its head, with the view body posed from it (ViewDrop)
		const FVector Sunk = BodyBaseLocation - FVector(0.f, 0.f, SinkDepth + ViewDrop);
		TemplateArms->SetRelativeLocation(Sunk);
		if (ViewBody)
		{
			ViewBody->SetRelativeLocation(Sunk);
		}
	}
}

void UFirstPersonBodyComponent::SetSinkDepth(float Centimetres)
{
	if (!AnimatedBody || FMath::IsNearlyEqual(Centimetres, SinkDepth, 0.05f))
	{
		return;
	}
	SinkDepth = Centimetres;
	const FVector Sunk = BodyBaseLocation - FVector(0.f, 0.f, SinkDepth + ViewDrop);   // ViewDrop stays 0 outside the template layout
	const ACharacter* Character = Cast<ACharacter>(GetOwner());
	if (Character && AnimatedBody != Character->GetMesh())
	{
		AnimatedBody->SetRelativeLocation(Sunk);   // the template's first-person mesh
	}
	if (ViewBody)
	{
		ViewBody->SetRelativeLocation(Sunk);
	}
	// The world body (shadow, what others see) and its shadow copy sink too, keeping the slope drop (this used to put the
	// world body back to the sink alone until the next drop update, whichever of the two ticked first).
	ApplyWorldBodyOffset();
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

void UFirstPersonBodyComponent::CopyBodyPose(UPoseableMeshComponent* Copy, USkeletalMeshComponent* Source)
{
	// CopyPoseFromSkeletalComponent builds a new bone array every call; the same mesh has the same bones, so copy in place
	const TArrayView<const FTransform> Local = Source->GetBoneSpaceTransformsView();
	if (Local.Num() == 0 || Local.Num() != Copy->BoneSpaceTransforms.Num() || Copy->GetSkinnedAsset() != Source->GetSkinnedAsset())
	{
		Copy->CopyPoseFromSkeletalComponent(Source);   // not set up yet, or another mesh: the engine's way
		return;
	}
	for (int32 Bone = 0; Bone < Local.Num(); ++Bone)
	{
		Copy->BoneSpaceTransforms[Bone] = Local[Bone];
	}
}

void UFirstPersonBodyComponent::SyncWorldBody(USkeletalMeshComponent* Body)
{
	if (!Body)
	{
		return;
	}
	const bool bShow = WorldBody && bWorldBodyFootIK && CVarFeetWorldIK.GetValueOnGameThread() != 0;
	if (bShow != bWorldBodyShown)
	{
		bWorldBodyShown = bShow;
		// a hidden component that casts a hidden shadow still draws it: each one's shadow goes with it
		if (WorldBody)
		{
			WorldBody->SetHiddenInGame(!bShow);
			WorldBody->SetCastShadow(bShow && bBodyCastShadow);
		}
		Body->SetHiddenInGame(bShow);
		Body->SetCastShadow(!bShow && bBodyCastShadow);
	}
	if (bShow)
	{
		// the climbing view hands the world body to the owner and back: the copy follows each change, and any new material
		if (WorldBody->bOwnerNoSee != Body->bOwnerNoSee)
		{
			WorldBody->SetOwnerNoSee(Body->bOwnerNoSee);
		}
		if (WorldBody->bOnlyOwnerSee != Body->bOnlyOwnerSee)
		{
			WorldBody->SetOnlyOwnerSee(Body->bOnlyOwnerSee);
		}
		if (WorldBody->FirstPersonPrimitiveType != Body->FirstPersonPrimitiveType)
		{
			WorldBody->SetFirstPersonPrimitiveType(Body->FirstPersonPrimitiveType);
		}
		for (int32 Slot = 0; Slot < Body->GetNumMaterials(); ++Slot)
		{
			UMaterialInterface* Material = Body->GetMaterial(Slot);
			if (WorldBody->GetMaterial(Slot) != Material)
			{
				WorldBody->SetMaterial(Slot, Material);
			}
		}
	}
	if (FeetBody)
	{
		// the owner's own legs step aside while the owner sees the whole body (the third-person climbing view), or the
		// two pairs of legs would overlap
		const bool bOwnerSeesBody = bBodyHiddenFromOwner && !Body->bOwnerNoSee
			&& Body->FirstPersonPrimitiveType != EFirstPersonPrimitiveType::WorldSpaceRepresentation;
		if (FeetBody->IsVisible() == bOwnerSeesBody)
		{
			FeetBody->SetVisibility(!bOwnerSeesBody);
		}
	}
}

void UFirstPersonBodyComponent::PlaceFeet()
{
	// The legs you can see, in both copies of the world body: FeetBody (your own legs, looking down) and WorldBody (third
	// person, mirrors, the shadow). A planted boot sits SnowContactCm (plus the powder sink) into snow and exactly on rock,
	// ice rubble, tents and props; a boot on ground higher than the lowered hips (the uphill one on a slope) is lifted onto
	// it, bending the knee; a boot in its swing keeps its animated height over the ground under it. One solve, written to
	// both copies, on the ground UpdateFootDrop found (no traces here).
	ACharacter* Character = Cast<ACharacter>(GetOwner());
	USkeletalMeshComponent* Source = Character ? Character->GetMesh() : nullptr;
	UPoseableMeshComponent* Copies[2] = { FeetBody.Get(), WorldBody.Get() };
	// a hidden copy needs no pose: FeetBody in the climbing view, WorldBody with ygg.Feet.WorldIK 0
	if (Copies[0] && !Copies[0]->IsVisible())
	{
		Copies[0] = nullptr;
	}
	if (!bWorldBodyShown)
	{
		Copies[1] = nullptr;
	}
	if (!Source || (!Copies[0] && !Copies[1]))
	{
		return;
	}
	for (UPoseableMeshComponent* Copy : Copies)
	{
		if (Copy)
		{
			CopyBodyPose(Copy, Source);
		}
	}

	const TArray<FTransform>& Pose = Source->GetComponentSpaceTransforms();
	const FTransform BodyXf = Source->GetComponentTransform();   // already lowered by UpdateFootDrop; both copies ride on it
	const FVector UpCS = BodyXf.InverseTransformVectorNoScale(FVector::UpVector);
	const FVector ForwardCS = BodyXf.InverseTransformVectorNoScale(Character->GetActorForwardVector());
	const float MaxTilt = FMath::DegreesToRadians(MaxFootTiltDeg);
	for (FLegFit& Leg : Legs)
	{
		Leg.Lift = 0.f;
		Leg.Weight = 0.f;
		Leg.TiltDeg = 0.f;
		const float Fit = FeetAlpha * Leg.Alpha;
		if (SoleHeightCm < 0.f || !Leg.bHasGround || Fit <= KINDA_SMALL_NUMBER
			|| !Pose.IsValidIndex(Leg.Thigh) || !Pose.IsValidIndex(Leg.Knee) || !Pose.IsValidIndex(Leg.Foot) || !Pose.IsValidIndex(Leg.Hips))
		{
			continue;
		}
		const FTransform& FootCS = Pose[Leg.Foot];
		const double SoleNow = BodyXf.TransformPosition(FootCS.GetLocation()).Z - SoleHeightCm;
		// planted (low in the animation): onto its ground; higher, fading back to its animated height over that ground
		const float Planted = 1.f - FMath::SmoothStep(PlantedBelowCm, PlantedBelowCm + 12.f, Leg.AnimHeight);
		const double WantSole = Leg.Point.Z - Leg.Contact + Leg.AnimHeight * (1.f - Planted);
		// the planted boot lies along the slope, at most MaxFootTiltDeg
		const FVector NormalCS = BodyXf.InverseTransformVectorNoScale(Leg.Normal).GetSafeNormal();
		const float Angle = FMath::Acos(FMath::Clamp(float(FVector::DotProduct(UpCS, NormalCS)), -1.f, 1.f));
		const float Tilt = FMath::Min(Angle, MaxTilt) * Planted * Fit;
		const FQuat TiltRotation = Angle > KINDA_SMALL_NUMBER ? FQuat::Slerp(FQuat::Identity, FQuat::FindBetweenNormals(UpCS, NormalCS), Tilt / Angle) : FQuat::Identity;
		// a tilted boot rests on its sole, not on the point straight under the ankle: the ankle sits Sole / cos(tilt) up
		const float TiltLift = SoleHeightCm * (1.f / FMath::Cos(Tilt) - 1.f);
		const float Lift = FMath::Clamp(float(WantSole - SoleNow), -25.f, MaxFootLiftCm) * Fit + TiltLift;
		const FVector TargetCS = FootCS.GetLocation() + BodyXf.InverseTransformVectorNoScale(FVector(0.f, 0.f, Lift));

		// two-bone solve (bone lengths kept, never stretched); the knee bends the way the animation bends it, and a
		// straight leg bends forward, never sideways
		const FTransform& OldUpper = Pose[Leg.Thigh];
		const FTransform& OldLower = Pose[Leg.Knee];
		const FVector ThighLocation = OldUpper.GetLocation();
		const FVector KneeLocation = OldLower.GetLocation();
		const FVector Bend = (KneeLocation - (ThighLocation + FootCS.GetLocation()) * 0.5f + ForwardCS * 3.f).GetSafeNormal();
		FVector NewKnee, NewFoot;
		AnimationCore::SolveTwoBoneIK(ThighLocation, KneeLocation, FootCS.GetLocation(), KneeLocation + Bend * 30.f, TargetCS, NewKnee, NewFoot, false, 1.0, 1.0);
		// swing thigh and shin onto the new knee and ankle
		const FQuat SwingUpper = FQuat::FindBetweenVectors(KneeLocation - ThighLocation, NewKnee - ThighLocation);
		FTransform NewUpper = OldUpper;
		NewUpper.SetRotation(SwingUpper * OldUpper.GetRotation());
		const FQuat SwingLower = FQuat::FindBetweenVectors(SwingUpper.RotateVector(FootCS.GetLocation() - KneeLocation), NewFoot - NewKnee);
		FTransform NewLower = OldLower;
		NewLower.SetLocation(NewKnee);
		NewLower.SetRotation(SwingLower * SwingUpper * OldLower.GetRotation());
		FTransform NewFootXf = FootCS;
		NewFootXf.SetLocation(NewFoot);
		NewFootXf.SetRotation(TiltRotation * FootCS.GetRotation());
		const FTransform ThighLocal = NewUpper.GetRelativeTransform(Pose[Leg.Hips]);
		const FTransform KneeLocal = NewLower.GetRelativeTransform(NewUpper);
		const FTransform FootLocal = NewFootXf.GetRelativeTransform(NewLower);
		for (UPoseableMeshComponent* Copy : Copies)
		{
			if (Copy && Copy->BoneSpaceTransforms.IsValidIndex(Leg.Thigh) && Copy->BoneSpaceTransforms.IsValidIndex(Leg.Knee) && Copy->BoneSpaceTransforms.IsValidIndex(Leg.Foot))
			{
				Copy->BoneSpaceTransforms[Leg.Thigh] = ThighLocal;
				Copy->BoneSpaceTransforms[Leg.Knee] = KneeLocal;
				Copy->BoneSpaceTransforms[Leg.Foot] = FootLocal;
			}
		}
		Leg.Lift = Lift;
		Leg.Weight = Planted * Fit;
		Leg.TiltDeg = FMath::RadiansToDegrees(Tilt);
		// what the log reports: the fitted sole over the surface (rock 0, snow -SnowContactCm - sink; positive = floating)
		Leg.FinalGap = float(BodyXf.TransformPosition(NewFoot).Z - SoleHeightCm / FMath::Cos(Tilt) - Leg.Point.Z);
	}

	// Into component space now, not at the copies' own tick early next frame: this frame's pose for this frame's body
	// position (a frame late, a planted boot slid by the distance walked in a frame).
	for (UPoseableMeshComponent* Copy : Copies)
	{
		if (Copy && Copy->IsRegistered())
		{
			Copy->RefreshBoneTransforms();
		}
	}
}

void UFirstPersonBodyComponent::LogFeet(float DeltaTime)
{
	const int32 Mode = CVarFeetLog.GetValueOnGameThread();
	if (Mode == 0)
	{
		FeetLogTimer = 0.f;
		return;
	}
	FeetLogTimer -= DeltaTime;
	if (Mode == 1 && FeetLogTimer > 0.f)
	{
		return;
	}
	FeetLogTimer = 0.25f;
	const ACharacter* Character = Cast<ACharacter>(GetOwner());
	const UCharacterMovementComponent* Movement = Character ? Character->GetCharacterMovement() : nullptr;
	const UCapsuleComponent* Capsule = Character ? Character->GetCapsuleComponent() : nullptr;
	if (!Movement || !Capsule)
	{
		return;
	}
	// the floor under the capsule, and how much higher than on flat ground its round bottom holds the body there
	const float FloorNormalZ = Movement->CurrentFloor.bBlockingHit ? float(Movement->CurrentFloor.HitResult.ImpactNormal.Z) : 1.f;
	const float Slope = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(FloorNormalZ, -1.f, 1.f)));
	const float CapsuleLift = Capsule->GetScaledCapsuleRadius() * (1.f / FMath::Max(FloorNormalZ, 0.2f) - 1.f);
	auto Describe = [](const FLegFit& Leg, const TCHAR* Name) -> FString
	{
		if (!Leg.bHasGround)
		{
			return FString::Printf(TEXT("%s no ground"), Name);
		}
		return FString::Printf(TEXT("%s %s gap %+.1f (anim %+.1f, lift %+.1f, target %+.1f from floor, height %.1f, planted %.2f, tilt %.0f)"), Name,
			Leg.bHit ? (Leg.bSnow ? TEXT("snow") : TEXT("hard")) : TEXT("lost"), Leg.FinalGap, Leg.AnimGap, Leg.Lift, Leg.Offset, Leg.AnimHeight, Leg.Weight, Leg.TiltDeg);
	};
	UE_LOG(LogTemp, Display, TEXT("Feet: %s, slope %.0f deg (capsule lifted %.1f + floor gap %.1f) | hips drop %.1f (target %.1f) eye drop %.1f sink %.1f fit %.2f world-ik %d | %s | %s"),
		IsClimbing() ? TEXT("climbing") : (Movement->IsMovingOnGround() ? TEXT("ground") : (Movement->MovementMode == MOVE_None ? TEXT("stopped") : TEXT("air"))),
		Slope, CapsuleLift, Movement->CurrentFloor.FloorDist, FootDrop, FootDropTarget, ViewDrop, SinkDepth, FeetAlpha, bWorldBodyShown ? 1 : 0, *Describe(Legs[0], TEXT("L")), *Describe(Legs[1], TEXT("R")));
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
