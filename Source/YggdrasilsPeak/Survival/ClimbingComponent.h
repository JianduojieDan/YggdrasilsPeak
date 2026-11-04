// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ClimbingComponent.generated.h"

class UAnimSequence;
class UInputAction;
class UInputMappingContext;
class USkeletalMeshComponent;

/** What the hands and crampons are on. Each grips differently (speed, stamina, slips). */
UENUM(BlueprintType)
enum class EClimbSurface : uint8
{
	None,
	Rock,
	HardSnow,
	Ice,
	SoftSnow
};

/** How one surface climbs. */
USTRUCT(BlueprintType)
struct FClimbSurfaceRules
{
	GENERATED_BODY()

	/** Climbing speed (times ClimbSpeed). */
	UPROPERTY(EditAnywhere, Category="Climbing")
	float Speed = 1.f;

	/** Stamina drain (times StaminaPerSecond). */
	UPROPERTY(EditAnywhere, Category="Climbing")
	float Drain = 1.f;

	/** Chance per second of a slip while stamina is low (below SlipBelowStamina). */
	UPROPERTY(EditAnywhere, Category="Climbing")
	float SlipChance = 0.f;

	/** Shown when you first grab it. */
	UPROPERTY(EditAnywhere, Category="Climbing")
	FText Hint;
};

/**
 *  Climbing steep ground: rock faces, ice walls (seracs, crevasse lips, the Lhotse Face), hard snow and soft snow.
 *  Space at a face steeper than MinWallAngle grabs it; WASD climbs along the face (W up, S down, A/D sideways),
 *  Space again lunges up, C lets go. Every surface climbs differently (FClimbSurfaceRules: speed, stamina drain,
 *  slips when tired) and the clipped-in fixed rope makes it cheaper and arrests falls. Topping out over a ledge
 *  pulls you up onto it. Stamina is the survival stamina: when it runs out the grip goes. Long falls hurt, very long
 *  ones kill (less when clipped in). Animations: Mixamo climbing clips retargeted to the climber, played on the
 *  body and first-person meshes while climbing. Added at runtime by UExpeditionComponent.
 */
UCLASS(ClassGroup=(Survival), meta=(BlueprintSpawnableComponent))
class YGGDRASILSPEAK_API UClimbingComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	UClimbingComponent();

	UFUNCTION(BlueprintPure, Category="Climbing")
	bool IsClimbing() const { return bClimbing; }

	UFUNCTION(BlueprintPure, Category="Climbing")
	EClimbSurface GetSurface() const { return Surface; }

	/** Grab the face in front, if there is one. True if climbing now. */
	UFUNCTION(BlueprintCallable, Category="Climbing")
	bool TryGrab();

	UFUNCTION(BlueprintCallable, Category="Climbing")
	void LetGo(bool bPushOff);

	/** Ground at least this steep (degrees) is a face you climb rather than walk. */
	UPROPERTY(EditAnywhere, Category="Climbing", meta=(ClampMin="30", ClampMax="90"))
	float MinWallAngle = 48.f;

	/** How far in front of the chest a face can be grabbed (cm, beyond the capsule). */
	UPROPERTY(EditAnywhere, Category="Climbing", meta=(ClampMin="10"))
	float Reach = 70.f;

	/** Climbing speed on good rock (cm/s). */
	UPROPERTY(EditAnywhere, Category="Climbing", meta=(ClampMin="10"))
	float ClimbSpeed = 70.f;

	/** Stamina per second while climbing on good rock (hanging still costs half). */
	UPROPERTY(EditAnywhere, Category="Climbing", meta=(ClampMin="0"))
	float StaminaPerSecond = 0.8f;

	/** Space while climbing: a quick move up for this much stamina. */
	UPROPERTY(EditAnywhere, Category="Climbing", meta=(ClampMin="0"))
	float LungeStamina = 8.f;

	/** Slips can happen below this stamina (percent of max). */
	UPROPERTY(EditAnywhere, Category="Climbing", meta=(ClampMin="0", ClampMax="100"))
	float SlipBelowStamina = 35.f;

	/** On grabbing, the view tilts up to at least this pitch (degrees): you look at your hands and the next hold. */
	UPROPERTY(EditAnywhere, Category="Climbing", meta=(ClampMin="0", ClampMax="80"))
	float LookUpOnGrab = 12.f;

	/** The climbing clips hold the hands closer to the body than the real face is: the body is slid in each frame
	 *  until the hands are this far off the surface (cm) - they touch the ice / rock. */
	UPROPERTY(EditAnywhere, Category="Climbing", meta=(ClampMin="0"))
	float HandContactGap = 3.f;

	/** ...and moved up or down so the lower boot rests this far above the ground under it (cm): never in it, never
	 *  hovering over a snow slope. */
	UPROPERTY(EditAnywhere, Category="Climbing", meta=(ClampMin="0"))
	float FootClearance = 3.f;

	/** Clipped into a fixed rope: stamina drain times this. */
	UPROPERTY(EditAnywhere, Category="Climbing", meta=(ClampMin="0", ClampMax="1"))
	float RopeDrain = 0.6f;

	UPROPERTY(EditAnywhere, Category="Climbing")
	TMap<EClimbSurface, FClimbSurfaceRules> Rules;

	/** Falls longer than this (m) hurt... */
	UPROPERTY(EditAnywhere, Category="Climbing|Falls", meta=(ClampMin="0"))
	float SafeFallMeters = 7.f;

	/** ...this much health per metre beyond it... */
	UPROPERTY(EditAnywhere, Category="Climbing|Falls", meta=(ClampMin="0"))
	float HealthPerMeter = 5.f;

	/** ...and falls this long kill (unless clipped into a rope). */
	UPROPERTY(EditAnywhere, Category="Climbing|Falls", meta=(ClampMin="1"))
	float DeadlyFallMeters = 26.f;

	/** While climbing the view pulls out over the shoulder (cm behind the climber) so you see the body on the face. */
	UPROPERTY(EditAnywhere, Category="Climbing|Camera", meta=(ClampMin="0"))
	float ClimbCameraDistance = 320.f;

	UPROPERTY(EditAnywhere, Category="Climbing|Camera")
	FVector ClimbCameraOffset = FVector(0.f, 55.f, -25.f);

	UPROPERTY(EditAnywhere, Category="Climbing|Animation")
	TObjectPtr<UAnimSequence> ClimbUpAnim;
	UPROPERTY(EditAnywhere, Category="Climbing|Animation")
	TObjectPtr<UAnimSequence> ClimbDownAnim;
	UPROPERTY(EditAnywhere, Category="Climbing|Animation")
	TObjectPtr<UAnimSequence> SlopeAnim;
	UPROPERTY(EditAnywhere, Category="Climbing|Animation")
	TObjectPtr<UAnimSequence> HangAnim;
	UPROPERTY(EditAnywhere, Category="Climbing|Animation")
	TObjectPtr<UAnimSequence> ShimmyLeftAnim;
	UPROPERTY(EditAnywhere, Category="Climbing|Animation")
	TObjectPtr<UAnimSequence> ShimmyRightAnim;
	UPROPERTY(EditAnywhere, Category="Climbing|Animation")
	TObjectPtr<UAnimSequence> TopOutAnim;

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:

	virtual void BeginPlay() override;

private:

	/** Face in front of the chest (or at the given height offset): hit, true if it is steep enough to climb. */
	bool TraceFace(const FVector& Direction, float HeightOffset, FHitResult& OutHit) const;
	EClimbSurface Classify(const FHitResult& Hit) const;
	const FClimbSurfaceRules& RulesFor(EClimbSurface S) const;
	bool TryTopOut();
	void PlayClip(UAnimSequence* Clip, float Rate);
	void RestoreAnimation();
	void SpendStamina(float Amount);
	float StaminaPercent() const;
	void Say(const FText& Message, float Seconds = 3.f) const;
	bool TryBindInput();
	void OnClimbPressed();
	void OnLetGoPressed();
	void OnClimbReleased();

	UFUNCTION()
	void HandleLanded(const FHitResult& Hit);

	bool bClimbing = false;
	bool bToppingOut = false;
	EClimbSurface Surface = EClimbSurface::None;
	FVector WallNormal = FVector::ForwardVector;
	float LungeTime = 0.f;
	float SlipTime = 0.f;
	float TopOutTime = 0.f;
	FVector TopOutFrom = FVector::ZeroVector;
	FVector TopOutTo = FVector::ZeroVector;
	float FallApexZ = 0.f;
	bool bTrackingFall = false;
	bool bSavedUseControllerYaw = true;
	TSet<EClimbSurface> HintsShown;
	bool bInputBound = false;

	UPROPERTY(Transient)
	TObjectPtr<UAnimSequence> CurrentClip;

	// third-person view while climbing
	void SetClimbView(bool bOn);
	void UpdateClimbView(float DeltaTime);
	UPROPERTY(Transient)
	TObjectPtr<class USpringArmComponent> ClimbArm;
	UPROPERTY(Transient)
	TObjectPtr<class UCameraComponent> ClimbCamera;
	TWeakObjectPtr<class UCameraComponent> FirstPersonCamera;
	bool bClimbView = false;
	float MeshShift = 0.f;
	float MeshLift = 0.f;            // body up (+) / down (-) from the capsule so the boots meet the ground
	TArray<FVector> MeshBaseLocations;
	void FitBodyToFace(const FHitResult& Face, float DeltaTime);
	bool bBodyWasOwnerNoSee = true;
	EFirstPersonPrimitiveType BodyPrimitiveType = EFirstPersonPrimitiveType::WorldSpaceRepresentation;

	/** Meshes we took over from their anim blueprints, and the classes to give back. */
	TArray<TPair<TWeakObjectPtr<USkeletalMeshComponent>, TSubclassOf<UAnimInstance>>> TakenMeshes;

	UPROPERTY(Transient)
	TObjectPtr<UInputAction> ClimbAction;
	UPROPERTY(Transient)
	TObjectPtr<UInputAction> LetGoAction;
	UPROPERTY(Transient)
	TObjectPtr<UInputMappingContext> Mapping;
};
