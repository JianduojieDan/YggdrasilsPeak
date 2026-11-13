// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "FirstPersonBodyComponent.generated.h"

class UCameraComponent;
class UPoseableMeshComponent;
class USkeletalMeshComponent;
class USpringArmComponent;
class UStaticMesh;
class UStaticMeshComponent;

/**
 *  First-person body: the local player sees their own body (legs, arms, hands) with the head bone hidden
 *  so the eye-level camera doesn't look out through the inside of the skull.
 *  Hiding the head also removes it from the shadow, so a second, shadow-only copy of the mesh is spawned
 *  (invisible to the owner, full head, own animation instance) and the visible body stops casting shadows.
 *  Other players / cameras keep seeing the normal full mesh.
 *  The camera boom also follows the animated head (HeadFollow), so steps and breathing move the view a little.
 *
 *  With a HeldToolMesh the owner's view body is a posable copy of the animated body. The right arm is raised so the fist
 *  rests low on the right of the view (two-bone IK, bone lengths kept, never stretched), the hand stays in line with
 *  the forearm, the fingers close around the handle and the tool sits in that fist. The left arm is raised the same way,
 *  mirrored and empty-handed, so both hands are always on screen.
 *
 *  Feet: the capsule stands higher on a slope than on flat ground (its round bottom meets the slope uphill of its centre),
 *  so the world body is lowered until the lower boot reaches its ground and each boot is fitted to the ground under it
 *  (the uphill knee bends), in the owner's own legs (FeetBody) and in what others, mirrors and the shadow see (WorldBody).
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

	/**
	 *  Engine first-person rendering (as in the UE 5.6+ First Person template): the view body and the held tool are drawn
	 *  with their own field of view and pulled towards the camera by FirstPersonScale, so arms and axe never sink into
	 *  snow or rock walls and don't stretch at the screen edges. The hidden world body / shadow copy casts the shadow.
	 */
	UPROPERTY(EditAnywhere, Category="First Person|Rendering")
	bool bFirstPersonRendering = true;

	/** Horizontal FOV for the view body and tool; 90 = same as the world camera, so they keep their old on-screen size. */
	UPROPERTY(EditAnywhere, Category="First Person|Rendering", meta=(ClampMin="40", ClampMax="120"))
	float FirstPersonFieldOfView = 90.f;

	/** How far the view body and tool are pulled towards the camera (1 = not at all). */
	UPROPERTY(EditAnywhere, Category="First Person|Rendering", meta=(ClampMin="0.1", ClampMax="1"))
	float FirstPersonScale = 0.6f;

	// ---- Held tool (ice axe in the right hand) ----

	/** Tool carried in the right hand. Empty = no tool, arms follow the animation only. */
	UPROPERTY(EditAnywhere, Category="First Person|Held Tool")
	TObjectPtr<UStaticMesh> HeldToolMesh;

	UPROPERTY(EditAnywhere, Category="First Person|Held Tool", meta=(ClampMin="0.1"))
	float HeldToolScale = 1.0f;

	/**
	 *  Where the fist (the middle of the handle) rests, in camera space (X forward, Y right, Z up), cm: low on the right.
	 *  It must stay within arm's reach of the shoulder (~9 cm behind the template camera); the arm is never stretched,
	 *  it just stops short.
	 */
	UPROPERTY(EditAnywhere, Category="First Person|Held Tool")
	FVector GripInView = FVector(40.f, 18.f, -19.f);

	/** Camera-space direction the elbow points: down and a little out to the right (turns the pick to face forward). */
	UPROPERTY(EditAnywhere, Category="First Person|Held Tool")
	FVector ElbowDirectionInView = FVector(0.f, 0.45f, -1.f);

	/** How fast the hand catches up when the view turns; lower = the axe trails more. */
	UPROPERTY(EditAnywhere, Category="First Person|Held Tool", meta=(ClampMin="1"))
	float HandFollowSpeed = 40.f;

	/** 0 = hand straight along the forearm, 1 = knuckles level; the wrist never bends more than MaxWristBend. */
	UPROPERTY(EditAnywhere, Category="First Person|Held Tool", meta=(ClampMin="0", ClampMax="1"))
	float WristLevel = 0.3f;

	UPROPERTY(EditAnywhere, Category="First Person|Held Tool", meta=(ClampMin="0", ClampMax="45"))
	float MaxWristBend = 12.f;

	/** Thumb turned in towards the body from straight up, degrees (a relaxed forearm is slightly turned in). */
	UPROPERTY(EditAnywhere, Category="First Person|Held Tool", meta=(ClampMin="-45", ClampMax="60"))
	float ThumbInward = 15.f;

	/** How far the axe head leans forward in the fist from straight up, degrees. */
	UPROPERTY(EditAnywhere, Category="First Person|Held Tool", meta=(ClampMin="-30", ClampMax="70"))
	float ToolTiltForward = 65.f;

	/** Point on the tool mesh (mesh space, unscaled) that sits in the fist: the middle of the rubber handle. Head = mesh -Y, pick = +X. */
	UPROPERTY(EditAnywhere, Category="First Person|Held Tool")
	FVector ToolGripPoint = FVector(0.f, 14.f, 0.f);

	/** Handle centre in the fist: this fraction of the way from the wrist to the knuckles... */
	UPROPERTY(EditAnywhere, Category="First Person|Held Tool", meta=(ClampMin="0", ClampMax="1.2"))
	float GripAlongHand = 0.65f;

	/** ...and this far (cm) off the hand bone towards the palm. */
	UPROPERTY(EditAnywhere, Category="First Person|Held Tool")
	float GripPalmOffset = 4.f;

	/** Finger curl around the handle per joint (_01, _02, _03), degrees. */
	UPROPERTY(EditAnywhere, Category="First Person|Held Tool")
	FVector FingerCurl = FVector(70.f, 85.f, 50.f);

	/** Thumb wrap over the fingers per joint (_01, _02, _03), degrees. */
	UPROPERTY(EditAnywhere, Category="First Person|Held Tool")
	FVector ThumbCurl = FVector(15.f, 30.f, 30.f);

	UPROPERTY(EditAnywhere, Category="First Person|Held Tool")
	FName UpperArmBone = TEXT("upperarm_r");

	UPROPERTY(EditAnywhere, Category="First Person|Held Tool")
	FName LowerArmBone = TEXT("lowerarm_r");

	UPROPERTY(EditAnywhere, Category="First Person|Held Tool")
	FName HandBone = TEXT("hand_r");

	// ---- Free (left) hand ----

	/** Keep the left hand in view too (low on the left, mirroring the tool hand), so both hands are always on screen. */
	UPROPERTY(EditAnywhere, Category="First Person|Left Hand")
	bool bShowLeftHand = true;

	/** Where the left hand rests, in camera space (X forward, Y right, Z up), cm. */
	UPROPERTY(EditAnywhere, Category="First Person|Left Hand")
	FVector LeftHandInView = FVector(40.f, -20.f, -21.f);

	/** Camera-space direction the left elbow points: down and a little out to the left. */
	UPROPERTY(EditAnywhere, Category="First Person|Left Hand")
	FVector LeftElbowDirectionInView = FVector(0.f, -0.45f, -1.f);

	/** Relaxed, half-closed fingers per joint (_01, _02, _03), degrees. */
	UPROPERTY(EditAnywhere, Category="First Person|Left Hand")
	FVector LeftFingerCurl = FVector(45.f, 60.f, 40.f);

	UPROPERTY(EditAnywhere, Category="First Person|Left Hand")
	FVector LeftThumbCurl = FVector(15.f, 25.f, 20.f);

	// ---- Procedural hand motion (both hands) ----

	/**
	 *  Rest height of the hands as a fraction of the way from the view centre to the bottom edge (0 = use the Z of
	 *  GripInView / LeftHandInView as is). Measured from the real viewport each frame, so the hands stay on screen on
	 *  wide and ultrawide monitors alike.
	 */
	UPROPERTY(EditAnywhere, Category="First Person|Hand Motion", meta=(ClampMin="0", ClampMax="0.95"))
	float HandScreenDrop = 0.7f;

	/** Walking: up-down bob of both hands (cm at full walk speed; two bobs per stride). */
	UPROPERTY(EditAnywhere, Category="First Person|Hand Motion", meta=(ClampMin="0"))
	float WalkBob = 1.2f;

	/** Walking: the hands swing forward/back in opposition, like arms swinging (cm). */
	UPROPERTY(EditAnywhere, Category="First Person|Hand Motion", meta=(ClampMin="0"))
	float WalkSwing = 1.6f;

	/** Walking: side-to-side sway as the weight shifts from foot to foot (cm). */
	UPROPERTY(EditAnywhere, Category="First Person|Hand Motion", meta=(ClampMin="0"))
	float WalkSway = 0.8f;

	/** Distance covered per full stride (two steps), cm: sets the bob rhythm from the walking speed. */
	UPROPERTY(EditAnywhere, Category="First Person|Hand Motion", meta=(ClampMin="20"))
	float StrideLength = 150.f;

	/** Speed (cm/s) at which the walk motion reaches full size. */
	UPROPERTY(EditAnywhere, Category="First Person|Hand Motion", meta=(ClampMin="1"))
	float WalkSpeedForFullBob = 300.f;

	/** Standing: slow breathing rise and fall (cm) and breaths per second. */
	UPROPERTY(EditAnywhere, Category="First Person|Hand Motion", meta=(ClampMin="0"))
	float BreathAmount = 0.4f;

	UPROPERTY(EditAnywhere, Category="First Person|Hand Motion", meta=(ClampMin="0"))
	float BreathRate = 0.28f;

	/** Turning the view: the hands lag behind by this many cm per 100 deg/s of turn, up to TurnSwayMax. */
	UPROPERTY(EditAnywhere, Category="First Person|Hand Motion", meta=(ClampMin="0"))
	float TurnSway = 0.f;

	UPROPERTY(EditAnywhere, Category="First Person|Hand Motion", meta=(ClampMin="0"))
	float TurnSwayMax = 4.f;

	/** Landing: the hands dip this many cm per 1000 cm/s of fall speed and spring back. */
	UPROPERTY(EditAnywhere, Category="First Person|Hand Motion", meta=(ClampMin="0"))
	float LandingDip = 6.f;

	/** Running (above SprintSpeed cm/s): the hands drop and pull in this many cm. */
	UPROPERTY(EditAnywhere, Category="First Person|Hand Motion", meta=(ClampMin="0"))
	float SprintDrop = 3.f;

	UPROPERTY(EditAnywhere, Category="First Person|Hand Motion", meta=(ClampMin="1"))
	float SprintSpeed = 450.f;

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** Sinks the visible body, its shadow copy and the eye this far (cm) into the snow; collision is unchanged. */
	void SetSinkDepth(float Centimetres);

	/** A planted boot goes this far into snow (cm) so it clearly touches it; on rock it stands on the surface. */
	UPROPERTY(EditAnywhere, Category="First Person|Feet", meta=(ClampMin="0", ClampMax="10"))
	float SnowContactCm = 1.5f;

	/** An animated sole lower than this over the animation's own flat floor (cm) counts as planted; higher, it fades back
	 *  to the animation's height over the ground under it. */
	UPROPERTY(EditAnywhere, Category="First Person|Feet", meta=(ClampMin="0"))
	float PlantedBelowCm = 10.f;

	/** A planted boot turns to lie along a slope, at most this far from level (degrees). */
	UPROPERTY(EditAnywhere, Category="First Person|Feet", meta=(ClampMin="0", ClampMax="60"))
	float MaxFootTiltDeg = 35.f;

	/** How far the hips may come down so the lower boot reaches sloping ground (cm). */
	UPROPERTY(EditAnywhere, Category="First Person|Feet", meta=(ClampMin="0", ClampMax="150"))
	float MaxPelvisDropCm = 70.f;

	/** How far a boot may be lifted above where the animation put it, onto higher ground (cm); past it, the lower boot hangs. */
	UPROPERTY(EditAnywhere, Category="First Person|Feet", meta=(ClampMin="0", ClampMax="90"))
	float MaxFootLiftCm = 50.f;

	/** How quickly the hips come down when the lower boot needs to reach further, and go back up (higher = quicker).
	 *  Down fast so no boot hangs, up gently so walking on a slope doesn't pump the body with every step. */
	UPROPERTY(EditAnywhere, Category="First Person|Feet", meta=(ClampMin="0.5"))
	float PelvisDropSpeed = 12.f;

	UPROPERTY(EditAnywhere, Category="First Person|Feet", meta=(ClampMin="0.5"))
	float PelvisRiseSpeed = 5.f;

	/** The body others, mirrors and the shadow see becomes a posed copy whose legs are fitted to the ground like the owner's
	 *  own (the animated world body can't bend its legs: its foot rig never ran on these bone names). Test: ygg.Feet.WorldIK. */
	UPROPERTY(EditAnywhere, Category="First Person|Feet")
	bool bWorldBodyFootIK = true;

protected:

	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:

	/** One arm held up into the view: its bones, where its hand rests and the hand's measured geometry. */
	struct FViewArm
	{
		FName Upper, Lower, Hand;
		/** Bone name suffix: "r" or "l". */
		FString Suffix;
		/** +1 right arm, -1 left arm (mirrors the palm and which way "inside" is). */
		float Side = 1.f;
		bool bHoldsTool = false;
		FVector RestInView = FVector::ZeroVector;
		FVector ElbowInView = FVector::ZeroVector;
		FVector FingerCurl = FVector::ZeroVector;
		FVector ThumbCurl = FVector::ZeroVector;

		/** Hand geometry in hand-bone space (from the reference pose). */
		FVector KnuckleDir = FVector::ForwardVector;
		FVector ThumbDir = FVector::UpVector;
		FVector PalmDir = FVector::RightVector;
		float Length = 10.f;

		/** Finger chains (_01, _02, _03 bone indices); the thumb is the last one. */
		TArray<TArray<int32>> Fingers;

		/** Reference pose in component space: palm direction, upper/lower arm directions and rotations. */
		FVector RefPalm = FVector::RightVector;
		FVector RefUpperDir = FVector::ForwardVector;
		FVector RefLowerDir = FVector::ForwardVector;
		FQuat RefUpperRotation = FQuat::Identity;
		FQuat RefLowerRotation = FQuat::Identity;

		/** Hand position in component space, eased towards the camera-space rest spot. */
		FVector SmoothedGrip = FVector::ZeroVector;
		bool bHasSmoothedGrip = false;
	};

	/** Copies this frame's animated pose to ViewBody, raises the arms (tool in the right fist) and sinks the feet. */
	void PoseViewBody(float DeltaTime);

	/** One arm, its wrist, fingers and (for the tool hand) the tool. */
	void PoseArm(FViewArm& Arm, float DeltaTime);

	/** Advances the procedural hand motion (walk bob/swing, breathing, turn lag, landing dip, sprint) for this frame. */
	void UpdateHandMotion(float DeltaTime);

	/** Where an arm's hand rests in camera space this frame: its rest spot, kept on screen, plus the hand motion. */
	FVector HandTargetInView(const FViewArm& Arm) const;

	// Procedural hand motion state.
	float StridePhase = 0.f;
	float BreathPhase = 0.f;
	float WalkAlpha = 0.f;
	float SprintAlpha = 0.f;
	FVector2D TurnLag = FVector2D::ZeroVector;
	FRotator LastViewRotation = FRotator::ZeroRotator;
	bool bHasLastViewRotation = false;
	float DipOffset = 0.f;
	float DipVelocity = 0.f;
	bool bWasFalling = false;
	float LastFallSpeed = 0.f;

	/**
	 *  Two-bone IK on ViewBody: Upper -> Lower -> End reaches EndLocation (component space) with EndRotation, bending towards Pole.
	 *  TwistToLower moves that share of the end's roll into the lower bone (a forearm turns along its length, not at the wrist).
	 */
	void BendLimb(FName UpperBone, FName LowerBone, FName EndBone, const FVector& EndLocation, const FQuat& EndRotation, const FVector& Pole, float MaxStretch, float TwistToLower = 0.f);

	/** Measures the hand in the reference pose: knuckle, thumb-side and palm directions in hand-bone space, and the finger chains. */
	void MeasureHand(FViewArm& Arm);

	/** Component-space transform of Bone on ViewBody, composing its local transforms up to the hand (HandXf). */
	FTransform ViewBoneUnderHand(int32 Bone, int32 Hand, const FTransform& HandXf) const;

	/** Hand rotation (component space) whose knuckles point along Knuckles and whose thumb side points along ThumbSide. */
	static FQuat HandRotationFor(const FViewArm& Arm, const FVector& Knuckles, const FVector& ThumbSide);

	// thigh, knee, ankle: mannequin names by default, Mixamo names (the climber) found at BeginPlay
	FName LeftLegBones[3] = { TEXT("thigh_l"), TEXT("calf_l"), TEXT("foot_l") };
	FName RightLegBones[3] = { TEXT("thigh_r"), TEXT("calf_r"), TEXT("foot_r") };
	void ResolveLegBones(const USkinnedMeshComponent* Mesh);

	/** [0] = tool (right) arm, [1] = free left arm. */
	FViewArm Arms[2];

	/** Where the tool sits relative to the hand bone. */
	FTransform ToolInHand = FTransform::Identity;

	UPROPERTY(Transient)
	TObjectPtr<USkeletalMeshComponent> ShadowMesh;

	/** What the owner sees when holding a tool: a posable copy of the body. */
	UPROPERTY(Transient)
	TObjectPtr<UPoseableMeshComponent> ViewBody;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> HeldTool;

	/** The tool in the world body's hand: what others and mirrors see. */
	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> WorldTool;

	UPROPERTY(Transient)
	TObjectPtr<UCameraComponent> ViewCamera;

	UPROPERTY(Transient)
	TObjectPtr<USkeletalMeshComponent> AnimatedBody;

	/** First Person template's arms mesh (component named FirstPersonMesh), when the owner has one. */
	UPROPERTY(Transient)
	TObjectPtr<USkeletalMeshComponent> TemplateArms;

	/** The world body's relative location before sinking (differs from BodyBaseLocation with the template layout). */
	FVector WorldBodyBaseLocation = FVector::ZeroVector;

	UPROPERTY(Transient)
	TObjectPtr<USpringArmComponent> CameraBoom;

	FVector BoomBaseLocation = FVector::ZeroVector;

	/** The body's relative location before any sinking. */
	FVector BodyBaseLocation = FVector::ZeroVector;
	float SinkDepth = 0.f;

	/** What the owner sees of their own legs: a world-space copy of the world body (hips and legs only). The first-person
	 *  body is drawn with its own FOV and pulled towards the camera, so its boots could never meet the ground. */
	UPROPERTY(Transient)
	TObjectPtr<UPoseableMeshComponent> FeetBody;

	/** What others, mirrors and the shadow see in place of the world body (which keeps animating, hidden): a posed copy with
	 *  the same fitted legs as FeetBody. Only with the template layout, where the world body itself casts the shadow. */
	UPROPERTY(Transient)
	TObjectPtr<UPoseableMeshComponent> WorldBody;

	/** WorldBody is on screen and the world body hidden (ygg.Feet.WorldIK can switch back). */
	bool bWorldBodyShown = false;
	/** The world body's own shadow setting, given back when it is shown again. */
	bool bBodyCastShadow = true;
	/** The owner normally doesn't see the world body (first-person setup); only then does FeetBody step aside when they do. */
	bool bBodyHiddenFromOwner = false;

	/** One leg: its bones on the world body and this frame's ground under its boot (found once, used by the drop and the fit). */
	struct FLegFit
	{
		int32 Thigh = INDEX_NONE;
		int32 Knee = INDEX_NONE;
		int32 Foot = INDEX_NONE;
		int32 Hips = INDEX_NONE;

		/** Ground found under the boot this frame / ever (the last ground is kept while the fit eases out). */
		bool bHit = false;
		bool bHasGround = false;
		bool bSnow = false;
		FVector Point = FVector::ZeroVector;
		FVector Normal = FVector::UpVector;
		/** How far the sole rests below the surface (cm): SnowContactCm plus the powder sink in snow, 0 on rock and props. */
		float Contact = 0.f;
		/** The sole's target relative to the animation's floor without the slope drop (cm, negative = lower). */
		float Offset = 0.f;
		/** The animated sole over the animation's own flat floor (cm): about 0 when planted, up in the swing. */
		float AnimHeight = 0.f;
		/** 0..1: this boot is fitted to its ground (eases out over a drop or a wall). */
		float Alpha = 0.f;

		/** For ygg.Feet.Log: the sole's gap to the surface after the fit / with the plain (lowered) animation, the lift,
		 *  the plant weight and the tilt. */
		float FinalGap = 0.f;
		float AnimGap = 0.f;
		float Lift = 0.f;
		float Weight = 0.f;
		float TiltDeg = 0.f;
	};

	/** [0] = left leg, [1] = right leg. */
	FLegFit Legs[2];

	/** Fits the boots of FeetBody and WorldBody to the ground found by UpdateFootDrop: one solve, written to both. */
	void PlaceFeet();
	float MeasureSoleHeight() const;
	float SoleHeightCm = -1.f;

	/** How far the world body is lowered so its lower boot meets sloping ground (cm), and where that is heading. */
	float FootDrop = 0.f;
	float FootDropTarget = 0.f;
	/** 0..1: how much the feet are fitted to the ground (fades out in the air and while climbing). */
	float FeetAlpha = 0.f;
	void UpdateFootDrop(float DeltaTime);

	/** How far the first-person body, and with it the eye on its head, comes down with the hips (cm): the drop beyond what
	 *  flat ground gives, eased. Without it the owner's legs (FeetBody) hung below the first-person jacket on a slope.
	 *  Template layout only. Test: ygg.Feet.ViewDrop. */
	float ViewDrop = 0.f;
	/** CharacterMovement's floor gap, kept from the last frame it was walking (the flat-ground part of the drop). */
	float FloorGapCm = 0.f;

	/** The world body at its base spot, minus the powder sink and the slope drop (unless climbing, which places it itself). */
	void ApplyWorldBodyOffset();

	/** Shows WorldBody or the world body (ygg.Feet.WorldIK), keeps the copy looking like it, and hides FeetBody while the
	 *  owner sees the whole body (the third-person climbing view). */
	void SyncWorldBody(USkeletalMeshComponent* Body);

	/** Copies the animated pose onto a posable copy of the same mesh, in place (no allocation each frame). */
	static void CopyBodyPose(UPoseableMeshComponent* Copy, USkeletalMeshComponent* Source);

	/** The ClimbingComponent, if the owner has one (kept as its base class so this header needs no climbing include).
	 *  ExpeditionComponent adds it at run time, possibly after this BeginPlay, so IsClimbing looks it up until found. */
	mutable TWeakObjectPtr<UActorComponent> ClimbingComponent;
	bool IsClimbing() const;

	/** ygg.Feet.Log: slope, drop and each boot's gap. */
	void LogFeet(float DeltaTime);
	float FeetLogTimer = 0.f;
};
