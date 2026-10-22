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
 *  With a HeldToolMesh the owner's view body is a posable copy of the animated body: the tool sits at a fixed
 *  spot low in the view and the right arm is bent onto its handle with two-bone IK, so the hand stays visible.
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
	float HeldToolScale = 1.4f;

	/** Where the hand grips the tool, in camera space (X forward, Y right, Z up), cm. Low on the right, just inside the view. */
	UPROPERTY(EditAnywhere, Category="First Person|Held Tool")
	FVector GripInView = FVector(38.f, 17.f, -13.f);

	/** Camera-space directions of the tool's pick (mesh +X) and of its handle end (mesh +Y): head up and forward. */
	UPROPERTY(EditAnywhere, Category="First Person|Held Tool")
	FVector PickDirectionInView = FVector(0.75f, 0.f, -0.65f);

	UPROPERTY(EditAnywhere, Category="First Person|Held Tool")
	FVector HandleDirectionInView = FVector(-0.65f, -0.05f, -0.75f);

	/** Point on the tool mesh (mesh space, unscaled) that sits in the palm: the middle of the rubber handle. */
	UPROPERTY(EditAnywhere, Category="First Person|Held Tool")
	FVector ToolGripPoint = FVector(0.f, 14.f, 0.f);

	/** The tool's rotation relative to the hand bone: handle across the palm, pick along the knuckles... */
	UPROPERTY(EditAnywhere, Category="First Person|Held Tool")
	FRotator ToolRotationInHand = FRotator(0.f, 180.f, 90.f);

	/** ...and the palm point, in hand-bone space, where ToolGripPoint goes. */
	UPROPERTY(EditAnywhere, Category="First Person|Held Tool")
	FVector PalmInHand = FVector(-7.f, 4.f, 0.f);

	/** The camera sits ahead of the shoulders, so the view-only arm may stretch this much to reach the tool. */
	UPROPERTY(EditAnywhere, Category="First Person|Held Tool", meta=(ClampMin="1", ClampMax="1.6"))
	float ArmMaxStretch = 1.3f;

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

	/** Copies this frame's animated pose to ViewBody, bends the right arm onto the held tool and sinks the feet. */
	void PoseViewBody();

	/** Two-bone IK on ViewBody: Upper -> Lower -> End reaches EndLocation (component space) with EndRotation, bending towards Pole. */
	void BendLimb(FName UpperBone, FName LowerBone, FName EndBone, const FVector& EndLocation, const FQuat& EndRotation, const FVector& Pole, float MaxStretch);

	inline static const FName LeftLegBones[3] = { TEXT("thigh_l"), TEXT("calf_l"), TEXT("foot_l") };
	inline static const FName RightLegBones[3] = { TEXT("thigh_r"), TEXT("calf_r"), TEXT("foot_r") };

	UPROPERTY(Transient)
	TObjectPtr<USkeletalMeshComponent> ShadowMesh;

	/** What the owner sees when holding a tool: a posable copy of the body. */
	UPROPERTY(Transient)
	TObjectPtr<UPoseableMeshComponent> ViewBody;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> HeldTool;

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
