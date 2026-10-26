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
 *  the forearm, the fingers close around the handle and the tool sits in that fist.
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
	float HandFollowSpeed = 16.f;

	/** 0 = hand straight along the forearm, 1 = knuckles level; the wrist never bends more than MaxWristBend. */
	UPROPERTY(EditAnywhere, Category="First Person|Held Tool", meta=(ClampMin="0", ClampMax="1"))
	float WristLevel = 0.6f;

	UPROPERTY(EditAnywhere, Category="First Person|Held Tool", meta=(ClampMin="0", ClampMax="45"))
	float MaxWristBend = 25.f;

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

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** Sinks the visible body, its shadow copy and the eye this far (cm) into the snow; collision is unchanged. */
	void SetSinkDepth(float Centimetres);

protected:

	virtual void BeginPlay() override;

private:

	/** Copies this frame's animated pose to ViewBody, raises the right arm with the tool in its fist and sinks the feet. */
	void PoseViewBody(float DeltaTime);

	/** Right arm, wrist, fingers and the tool. */
	void PoseToolArm(float DeltaTime);

	/**
	 *  Two-bone IK on ViewBody: Upper -> Lower -> End reaches EndLocation (component space) with EndRotation, bending towards Pole.
	 *  TwistToLower moves that share of the end's roll into the lower bone (a forearm turns along its length, not at the wrist).
	 */
	void BendLimb(FName UpperBone, FName LowerBone, FName EndBone, const FVector& EndLocation, const FQuat& EndRotation, const FVector& Pole, float MaxStretch, float TwistToLower = 0.f);

	/** Measures the hand in the reference pose: knuckle, thumb-side and palm directions in hand-bone space, and the finger chains. */
	void MeasureHand();

	/** Component-space transform of Bone on ViewBody, composing its local transforms up to the hand (HandXf). */
	FTransform ViewBoneUnderHand(int32 Bone, int32 Hand, const FTransform& HandXf) const;

	/** Hand rotation (component space) whose knuckles point along Knuckles and whose thumb side points along ThumbSide. */
	FQuat HandRotationFor(const FVector& Knuckles, const FVector& ThumbSide) const;

	inline static const FName LeftLegBones[3] = { TEXT("thigh_l"), TEXT("calf_l"), TEXT("foot_l") };
	inline static const FName RightLegBones[3] = { TEXT("thigh_r"), TEXT("calf_r"), TEXT("foot_r") };

	/** Hand geometry in hand-bone space (from the reference pose). */
	FVector HandKnuckleDir = FVector::ForwardVector;
	FVector HandThumbDir = FVector::UpVector;
	FVector HandPalmDir = FVector::RightVector;
	float HandLength = 10.f;

	/** Where the tool sits relative to the hand bone. */
	FTransform ToolInHand = FTransform::Identity;

	/** Finger chains (_01, _02, _03 bone indices); the thumb is the last one. */
	TArray<TArray<int32>> FingerChains;

	/** Fist position in component space, eased towards the camera-space rest spot. */
	FVector SmoothedGrip = FVector::ZeroVector;
	bool bHasSmoothedGrip = false;

	UPROPERTY(Transient)
	TObjectPtr<USkeletalMeshComponent> ShadowMesh;

	/** What the owner sees when holding a tool: a posable copy of the body. */
	UPROPERTY(Transient)
	TObjectPtr<UPoseableMeshComponent> ViewBody;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> HeldTool;

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
};
