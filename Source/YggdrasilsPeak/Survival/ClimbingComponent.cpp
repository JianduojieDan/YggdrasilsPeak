// Copyright Epic Games, Inc. All Rights Reserved.

#include "Survival/ClimbingComponent.h"

#include "Survival/ExpeditionComponent.h"
#include "Survival/SnowTrailComponent.h"
#include "Survival/SurvivalComponent.h"
#include "Animation/AnimSequence.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "LandscapeProxy.h"
#include "Materials/MaterialInterface.h"
#include "PhysicalMaterials/PhysicalMaterial.h"

namespace
{
	UAnimSequence* LoadClip(const TCHAR* Name)
	{
		const FString Path = FString::Printf(TEXT("/Game/Characters/Climber_Rigged/Anims/Climb_%s_mixamo_com_Climber.Climb_%s_mixamo_com_Climber"), Name, Name);
		return LoadObject<UAnimSequence>(nullptr, *Path);
	}
}

UClimbingComponent::UClimbingComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;

	auto Add = [this](EClimbSurface S, float Speed, float Drain, float Slip, const FText& Hint)
	{
		FClimbSurfaceRules R;
		R.Speed = Speed; R.Drain = Drain; R.SlipChance = Slip; R.Hint = Hint;
		Rules.Add(S, R);
	};
	Add(EClimbSurface::Rock, 1.f, 1.f, 0.f, NSLOCTEXT("Climb", "Rock", "Rock - good holds."));
	Add(EClimbSurface::HardSnow, 0.85f, 1.15f, 0.12f, NSLOCTEXT("Climb", "HardSnow", "Hard snow - kick each step in."));
	Add(EClimbSurface::Ice, 0.7f, 1.45f, 0.25f, NSLOCTEXT("Climb", "Ice", "Ice - axe and front points. It tires you fast."));
	Add(EClimbSurface::SoftSnow, 0.45f, 1.9f, 0.35f, NSLOCTEXT("Climb", "SoftSnow", "Soft snow - it barely holds you."));
}

void UClimbingComponent::BeginPlay()
{
	Super::BeginPlay();
	ClimbUpAnim = ClimbUpAnim ? ClimbUpAnim.Get() : LoadClip(TEXT("Up_Wall"));
	ClimbDownAnim = ClimbDownAnim ? ClimbDownAnim.Get() : LoadClip(TEXT("Down_Wall"));
	SlopeAnim = SlopeAnim ? SlopeAnim.Get() : LoadClip(TEXT("Slope"));
	HangAnim = HangAnim ? HangAnim.Get() : LoadClip(TEXT("HangIdle"));
	ShimmyLeftAnim = ShimmyLeftAnim ? ShimmyLeftAnim.Get() : LoadClip(TEXT("ShimmyLeft"));
	ShimmyRightAnim = ShimmyRightAnim ? ShimmyRightAnim.Get() : LoadClip(TEXT("ShimmyRight"));
	TopOutAnim = TopOutAnim ? TopOutAnim.Get() : LoadClip(TEXT("TopOut"));

	if (ACharacter* Character = Cast<ACharacter>(GetOwner()))
	{
		// over-the-shoulder camera for climbing, idle until a face is grabbed
		FirstPersonCamera = Character->FindComponentByClass<UCameraComponent>();
		ClimbArm = NewObject<USpringArmComponent>(Character, TEXT("ClimbArm"));
		ClimbArm->SetupAttachment(Character->GetRootComponent());
		ClimbArm->SetRelativeLocation(FVector(0.f, 0.f, 60.f));
		ClimbArm->TargetArmLength = 0.f;
		ClimbArm->SocketOffset = FVector::ZeroVector;
		ClimbArm->bUsePawnControlRotation = true;
		ClimbArm->bDoCollisionTest = true;
		ClimbArm->ProbeSize = 14.f;
		ClimbArm->bEnableCameraLag = true;
		ClimbArm->CameraLagSpeed = 8.f;
		ClimbArm->RegisterComponent();
		ClimbCamera = NewObject<UCameraComponent>(Character, TEXT("ClimbCamera"));
		ClimbCamera->SetupAttachment(ClimbArm, USpringArmComponent::SocketName);
		ClimbCamera->bUsePawnControlRotation = false;
		ClimbCamera->SetAutoActivate(false);
		ClimbCamera->RegisterComponent();
		ClimbCamera->Deactivate();
		Character->LandedDelegate.AddDynamic(this, &UClimbingComponent::HandleLanded);
		// our velocity is set before the movement component moves the character each frame
		Character->GetCharacterMovement()->AddTickPrerequisiteComponent(this);
	}
}

bool UClimbingComponent::TryBindInput()
{
	const APawn* Pawn = Cast<APawn>(GetOwner());
	const APlayerController* PC = Pawn ? Cast<APlayerController>(Pawn->GetController()) : nullptr;
	UEnhancedInputComponent* Input = Pawn ? Cast<UEnhancedInputComponent>(Pawn->InputComponent) : nullptr;
	UEnhancedInputLocalPlayerSubsystem* Subsystem = PC ? ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()) : nullptr;
	if (!Input || !Subsystem)
	{
		return false;
	}
	Mapping = NewObject<UInputMappingContext>(this);
	ClimbAction = NewObject<UInputAction>(this, TEXT("IA_Climb"));
	LetGoAction = NewObject<UInputAction>(this, TEXT("IA_LetGo"));
	Mapping->MapKey(ClimbAction, EKeys::SpaceBar);
	Mapping->MapKey(ClimbAction, EKeys::Gamepad_FaceButton_Bottom);
	Mapping->MapKey(LetGoAction, EKeys::C);
	Mapping->MapKey(LetGoAction, EKeys::Gamepad_FaceButton_Right);
	Subsystem->AddMappingContext(Mapping, 2);
	Input->BindAction(ClimbAction, ETriggerEvent::Started, this, &UClimbingComponent::OnClimbPressed);
	Input->BindAction(LetGoAction, ETriggerEvent::Started, this, &UClimbingComponent::OnLetGoPressed);
	Input->BindAction(ClimbAction, ETriggerEvent::Completed, this, &UClimbingComponent::OnClimbReleased);
	return true;
}

void UClimbingComponent::OnClimbPressed()
{
	if (!bClimbing)
	{
		// Space is also jump: this mapping sits above the character's own, so jump from here when there is no
		// face to grab (otherwise the jump never arrives)
		if (!TryGrab())
		{
			if (ACharacter* Character = Cast<ACharacter>(GetOwner()))
			{
				Character->Jump();
			}
		}
		return;
	}
	// a quick move up the face
	if (!bToppingOut && LungeTime <= 0.f && StaminaPercent() > LungeStamina * 0.5f)
	{
		SpendStamina(LungeStamina);
		LungeTime = 0.35f;
	}
}

void UClimbingComponent::OnClimbReleased()
{
	if (ACharacter* Character = Cast<ACharacter>(GetOwner()))
	{
		Character->StopJumping();
	}
}

void UClimbingComponent::OnLetGoPressed()
{
	if (bClimbing && !bToppingOut)
	{
		LetGo(true);
	}
}

const FClimbSurfaceRules& UClimbingComponent::RulesFor(EClimbSurface S) const
{
	static const FClimbSurfaceRules Default;
	const FClimbSurfaceRules* R = Rules.Find(S);
	return R ? *R : Default;
}

bool UClimbingComponent::TraceFace(const FVector& Direction, float HeightOffset, FHitResult& OutHit) const
{
	const ACharacter* Character = Cast<ACharacter>(GetOwner());
	const float Radius = Character->GetCapsuleComponent()->GetScaledCapsuleRadius();
	const FVector Start = Character->GetActorLocation() + FVector(0.f, 0.f, 25.f + HeightOffset);
	FCollisionQueryParams Params(SCENE_QUERY_STAT(ClimbFace), true, Character);
	Params.bReturnPhysicalMaterial = true;
	Params.bReturnFaceIndex = true;   // rock faces of the detail tiles are known by their index (USnowTrailComponent::IsRock)
	if (!GetWorld()->SweepSingleByChannel(OutHit, Start, Start + Direction * (Radius + Reach), FQuat::Identity, ECC_Visibility,
		FCollisionShape::MakeSphere(12.f), Params))
	{
		return false;
	}
	// steep enough to be a face (overhangs too), not something you would just walk up
	return OutHit.ImpactNormal.Z < FMath::Cos(FMath::DegreesToRadians(MinWallAngle));
}

EClimbSurface UClimbingComponent::Classify(const FHitResult& Hit) const
{
	if (USnowTrailComponent::IsTerrain(Hit))   // the landscape or a route detail tile
	{
		if (USnowTrailComponent::IsRock(Hit))
		{
			return EClimbSurface::Rock;
		}
		if (USnowTrailComponent::IsGlacierHard(Hit.ImpactPoint))
		{
			return EClimbSurface::Ice;
		}
		return USnowTrailComponent::SnowSoftness(Hit.ImpactPoint, Hit.ImpactNormal) < 0.35f ? EClimbSurface::HardSnow : EClimbSurface::SoftSnow;
	}
	// meshes: seracs, crevasse lips and anything iced are ice, the rest is rock
	if (const UPrimitiveComponent* Comp = Hit.GetComponent())
	{
		const UMaterialInterface* Mat = Comp->GetMaterial(0);
		const FString Name = Mat ? Mat->GetName() : FString();
		if (Name.Contains(TEXT("Ice")) || Name.Contains(TEXT("Serac")) || Name.Contains(TEXT("Crevasse")))
		{
			return EClimbSurface::Ice;
		}
		if (Name.Contains(TEXT("Snow")))
		{
			return EClimbSurface::HardSnow;
		}
	}
	return EClimbSurface::Rock;
}

bool UClimbingComponent::TryGrab()
{
	ACharacter* Character = Cast<ACharacter>(GetOwner());
	USurvivalComponent* Survival = Character ? Character->FindComponentByClass<USurvivalComponent>() : nullptr;
	if (!Character || bClimbing || (Survival && Survival->IsDead()))
	{
		return false;
	}
	// look along the view (flattened): the face you are looking at
	FRotator View = Character->GetControlRotation();
	View.Pitch = 0.f;
	FHitResult Hit;
	if (!TraceFace(View.Vector(), 0.f, Hit) && !TraceFace(View.Vector(), 60.f, Hit))
	{
		return false;
	}
	if (StaminaPercent() < 5.f)
	{
		Say(NSLOCTEXT("Climb", "TooTired", "Too tired to hold on."));
		return false;
	}
	bClimbing = true;
	WallNormal = Hit.ImpactNormal;
	Surface = Classify(Hit);
	UCharacterMovementComponent* Move = Character->GetCharacterMovement();
	Character->StopJumping();
	Move->StopMovementImmediately();
	Move->SetMovementMode(MOVE_Flying);
	Move->BrakingDecelerationFlying = 0.f;
	bSavedUseControllerYaw = Character->bUseControllerRotationYaw;
	Character->bUseControllerRotationYaw = false;
	if (Survival)
	{
		Survival->SetSprintRequested(false);
		Survival->SetClimbing(true);
	}
	bTrackingFall = false;
	CurrentClip = nullptr;
	// look up the face a little (the camera pulls out over the shoulder; LookUpOnGrab is the pitch to start from)
	if (AController* Controller = Character->GetController())
	{
		FRotator R = Controller->GetControlRotation();
		const float Pitch = FRotator::NormalizeAxis(R.Pitch);
		if (Pitch < LookUpOnGrab)
		{
			R.Pitch = LookUpOnGrab;
			Controller->SetControlRotation(R);
		}
	}
	if (!HintsShown.Contains(Surface))
	{
		HintsShown.Add(Surface);
		Say(FText::Format(NSLOCTEXT("Climb", "Grab", "{0}  [WASD] climb  [Space] lunge  [C] let go"), RulesFor(Surface).Hint), 5.f);
	}
	return true;
}

void UClimbingComponent::LetGo(bool bPushOff)
{
	ACharacter* Character = Cast<ACharacter>(GetOwner());
	if (!bClimbing || !Character)
	{
		return;
	}
	bClimbing = false;
	bToppingOut = false;
	Surface = EClimbSurface::None;
	Character->bUseControllerRotationYaw = bSavedUseControllerYaw;
	if (USurvivalComponent* Survival = Character->FindComponentByClass<USurvivalComponent>())
	{
		Survival->SetClimbing(false);
	}
	UCharacterMovementComponent* Move = Character->GetCharacterMovement();
	Move->SetMovementMode(MOVE_Falling);
	Move->Velocity = bPushOff ? WallNormal * 150.f : FVector::ZeroVector;
	FallApexZ = Character->GetActorLocation().Z;
	bTrackingFall = true;
	RestoreAnimation();
}

void UClimbingComponent::SpendStamina(float Amount)
{
	if (USurvivalComponent* Survival = GetOwner()->FindComponentByClass<USurvivalComponent>())
	{
		Survival->SetStat(ESurvivalStat::Stamina, FMath::Max(0.f, Survival->GetStatValue(ESurvivalStat::Stamina) - Amount));
	}
}

float UClimbingComponent::StaminaPercent() const
{
	const USurvivalComponent* Survival = GetOwner()->FindComponentByClass<USurvivalComponent>();
	return Survival ? Survival->GetStatNormalized(ESurvivalStat::Stamina) * 100.f : 100.f;
}

void UClimbingComponent::Say(const FText& Message, float Seconds) const
{
	if (UExpeditionComponent* Expedition = GetOwner()->FindComponentByClass<UExpeditionComponent>())
	{
		Expedition->ShowMessage(Message, Seconds);
	}
}

void UClimbingComponent::PlayClip(UAnimSequence* Clip, float Rate)
{
	if (!Clip)
	{
		return;
	}
	ACharacter* Character = Cast<ACharacter>(GetOwner());
	if (TakenMeshes.Num() == 0)
	{
		// every skeletal mesh on the climber that shares the clip's skeleton (body + first-person body)
		TArray<USkeletalMeshComponent*> Meshes;
		Character->GetComponents<USkeletalMeshComponent>(Meshes);
		for (USkeletalMeshComponent* M : Meshes)
		{
			if (M->GetSkeletalMeshAsset() && M->GetSkeletalMeshAsset()->GetSkeleton() == Clip->GetSkeleton())
			{
				TakenMeshes.Emplace(M, M->GetAnimClass());
				MeshBaseLocations.Add(M->GetRelativeLocation());
			}
		}
	}
	for (const auto& Taken : TakenMeshes)
	{
		if (USkeletalMeshComponent* M = Taken.Key.Get())
		{
			if (CurrentClip != Clip)
			{
				M->PlayAnimation(Clip, true);
			}
			M->SetPlayRate(Rate);
		}
	}
	CurrentClip = Clip;
}

void UClimbingComponent::SetClimbView(bool bOn)
{
	ACharacter* Character = Cast<ACharacter>(GetOwner());
	if (!Character || !ClimbCamera || !FirstPersonCamera.IsValid() || bClimbView == bOn)
	{
		return;
	}
	bClimbView = bOn;
	if (bOn)
	{
		// start the arm where the eyes are, so the pull-out is smooth
		ClimbArm->TargetArmLength = 0.f;
		ClimbArm->SocketOffset = FVector::ZeroVector;
		FirstPersonCamera->Deactivate();
		ClimbCamera->Activate();
	}
	TArray<USkeletalMeshComponent*> Meshes;
	Character->GetComponents<USkeletalMeshComponent>(Meshes);
	for (USkeletalMeshComponent* M : Meshes)
	{
		if (M == Character->GetMesh())
		{
			// the first-person setup marks the body "world space representation", which the owner never sees
			if (bOn)
			{
				bBodyWasOwnerNoSee = M->bOwnerNoSee;
				BodyPrimitiveType = M->FirstPersonPrimitiveType;
				M->SetFirstPersonPrimitiveType(EFirstPersonPrimitiveType::None);
			}
			else
			{
				M->SetFirstPersonPrimitiveType(BodyPrimitiveType);
			}
			M->SetOwnerNoSee(bOn ? false : bBodyWasOwnerNoSee);   // the body you climb with, seen from behind
		}
		else if (M->GetName().Contains(TEXT("FirstPerson")))
		{
			M->SetVisibility(!bOn, true);  // the first-person body would float in front of the camera
		}
	}
	if (!bOn)
	{
		ClimbCamera->Deactivate();
		FirstPersonCamera->Activate();
	}
}

void UClimbingComponent::UpdateClimbView(float DeltaTime)
{
	if (!ClimbArm)
	{
		return;
	}
	const bool bWant = bClimbing;
	const float Length = FMath::FInterpTo(ClimbArm->TargetArmLength, bWant ? ClimbCameraDistance : 0.f, DeltaTime, bWant ? 3.f : 6.f);
	ClimbArm->TargetArmLength = Length;
	ClimbArm->SocketOffset = ClimbCameraOffset * (Length / FMath::Max(ClimbCameraDistance, 1.f));
	if (bWant)
	{
		SetClimbView(true);
	}
	else if (bClimbView && Length < 25.f)
	{
		SetClimbView(false);           // back into the eyes once the camera has glided home
	}
}

void UClimbingComponent::RestoreAnimation()
{
	for (int32 i = 0; i < TakenMeshes.Num(); ++i)
	{
		const auto& Taken = TakenMeshes[i];
		if (USkeletalMeshComponent* M = Taken.Key.Get())
		{
			if (MeshBaseLocations.IsValidIndex(i))
			{
				M->SetRelativeLocation(MeshBaseLocations[i]);
			}
			M->SetPlayRate(1.f);
			M->SetAnimationMode(EAnimationMode::AnimationBlueprint);
			M->SetAnimInstanceClass(Taken.Value);
		}
	}
	TakenMeshes.Reset();
	MeshBaseLocations.Reset();
	MeshShift = 0.f;
	MeshLift = 0.f;
	CurrentClip = nullptr;
}

void UClimbingComponent::FitBodyToFace(const FHitResult& Face, float DeltaTime)
{
	ACharacter* Character = Cast<ACharacter>(GetOwner());
	USkeletalMeshComponent* Body = Character ? Character->GetMesh() : nullptr;
	if (!Body || TakenMeshes.Num() == 0)
	{
		return;
	}
	// hands onto the face: the gripping hand (the one nearest the surface) rests on it, the other may reach
	float Off = 1e9f;
	int32 N = 0;
	for (const FName Hand : { FName(TEXT("LeftHand")), FName(TEXT("RightHand")) })
	{
		if (Body->GetBoneIndex(Hand) != INDEX_NONE)
		{
			Off = FMath::Min(Off, float(FVector::DotProduct(Body->GetSocketLocation(Hand) - Face.ImpactPoint, Face.ImpactNormal)));
			++N;
		}
	}
	if (N > 0)
	{
		MeshShift = FMath::Clamp(MeshShift + (Off - HandContactGap) * FMath::Min(1.f, DeltaTime * 6.f), 0.f, 70.f);
	}
	// boots on the ground: the body moves up or down (relative to the capsule) until the lower boot rests just above
	// what is under it. Only lifting the whole climber - and never lowering - left them hovering over snow slopes,
	// with the lift adding up as they moved.
	float Lowest = 1e9f;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(ClimbFeet), false, Character);
	for (const FName Foot : { FName(TEXT("LeftFoot")), FName(TEXT("RightFoot")) })
	{
		if (Body->GetBoneIndex(Foot) == INDEX_NONE)
		{
			continue;
		}
		const FVector F = Body->GetSocketLocation(Foot);
		FHitResult Ground;
		if (GetWorld()->LineTraceSingleByChannel(Ground, F + FVector(0.f, 0.f, 80.f), F - FVector(0.f, 0.f, 300.f), ECC_Visibility, Params)
			&& Ground.ImpactNormal.Z > 0.5f)
		{
			Lowest = FMath::Min(Lowest, float(F.Z - Ground.ImpactPoint.Z));
		}
	}
	if (Lowest > 60.f)
	{
		// up a wall, the ground is well below the boots: they stand on the face, not on it - drift back to neutral
		MeshLift = FMath::FInterpTo(MeshLift, 0.f, DeltaTime, 3.f);
	}
	else
	{
		// up fast (a quick step, not a pop), down gently; the feet traces read last frame's lift, so ease toward it
		const float Want = FootClearance - Lowest;
		const float Step = Want > 0.f ? FMath::Min(Want, 400.f * DeltaTime) : FMath::Max(Want, -150.f * DeltaTime);
		MeshLift = FMath::Clamp(MeshLift + Step, -60.f, 80.f);
	}
	// the capsule faces the wall (actor yaw), so +X in its space is towards the face
	for (int32 i = 0; i < TakenMeshes.Num(); ++i)
	{
		if (USkeletalMeshComponent* M = TakenMeshes[i].Key.Get())
		{
			M->SetRelativeLocation(MeshBaseLocations[i] + FVector(MeshShift, 0.f, MeshLift));
		}
	}
}

bool UClimbingComponent::TryTopOut()
{
	// the face ends at head height and there is walkable ground just over the edge: pull up onto it
	ACharacter* Character = Cast<ACharacter>(GetOwner());
	const float HalfHeight = Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	const FVector Into = -FVector(WallNormal.X, WallNormal.Y, 0.f).GetSafeNormal();
	FHitResult Above;
	if (TraceFace(Into, 95.f, Above))
	{
		return false;      // still wall up there
	}
	const FVector Probe = Character->GetActorLocation() + Into * 70.f + FVector(0.f, 0.f, HalfHeight + 120.f);
	FHitResult Ledge;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(ClimbLedge), false, Character);
	if (!GetWorld()->LineTraceSingleByChannel(Ledge, Probe, Probe - FVector(0.f, 0.f, HalfHeight + 220.f), ECC_Visibility, Params)
		|| !Character->GetCharacterMovement()->IsWalkable(Ledge))
	{
		return false;
	}
	const FVector Target = Ledge.ImpactPoint + FVector(0.f, 0.f, HalfHeight + 4.f);
	// room to stand there?
	if (GetWorld()->OverlapAnyTestByChannel(Target, FQuat::Identity, ECC_Pawn, Character->GetCapsuleComponent()->GetCollisionShape(-2.f), Params))
	{
		return false;
	}
	bToppingOut = true;
	TopOutTime = 0.f;
	TopOutFrom = Character->GetActorLocation();
	TopOutTo = Target;
	PlayClip(TopOutAnim, 1.6f);
	return true;
}

void UClimbingComponent::HandleLanded(const FHitResult& Hit)
{
	ACharacter* Character = Cast<ACharacter>(GetOwner());
	if (!bTrackingFall || !Character)
	{
		return;
	}
	bTrackingFall = false;
	const float Meters = (FallApexZ - Character->GetActorLocation().Z) / 100.f;
	USurvivalComponent* Survival = Character->FindComponentByClass<USurvivalComponent>();
	const UExpeditionComponent* Expedition = Character->FindComponentByClass<UExpeditionComponent>();
	const bool bRope = Expedition && Expedition->IsOnRope();
	if (!Survival || Meters <= SafeFallMeters)
	{
		return;
	}
	if (Meters >= DeadlyFallMeters && !bRope)
	{
		Survival->KillBy(FText::Format(NSLOCTEXT("Climb", "FellDead", "You fell {0} metres."), FText::AsNumber(FMath::RoundToInt(Meters))));
		return;
	}
	const float Damage = (Meters - SafeFallMeters) * HealthPerMeter * (bRope ? 0.3f : 1.f);
	Survival->SetStat(ESurvivalStat::Health, FMath::Max(1.f, Survival->GetStatValue(ESurvivalStat::Health) - Damage));
	Say(bRope ? NSLOCTEXT("Climb", "RopeCaught", "The rope caught you. Bruised, but whole.")
		: NSLOCTEXT("Climb", "HardLanding", "A hard landing."), 3.f);
}

void UClimbingComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	ACharacter* Character = Cast<ACharacter>(GetOwner());
	if (!Character)
	{
		return;
	}
	if (!bInputBound && Character->IsLocallyControlled())
	{
		bInputBound = TryBindInput();
	}
	UCharacterMovementComponent* Move = Character->GetCharacterMovement();
	UpdateClimbView(DeltaTime);

	// falls: from the highest point of any fall (also walking off an edge)
	if (!bClimbing)
	{
		if (Move->IsFalling())
		{
			if (!bTrackingFall)
			{
				bTrackingFall = true;
				FallApexZ = Character->GetActorLocation().Z;
			}
			FallApexZ = FMath::Max(FallApexZ, float(Character->GetActorLocation().Z));
		}
		return;
	}

	USurvivalComponent* Survival = Character->FindComponentByClass<USurvivalComponent>();
	if (Survival && Survival->IsDead())
	{
		LetGo(false);
		return;
	}

	// pulling up over a ledge: an eased move up, then over
	if (bToppingOut)
	{
		TopOutTime += DeltaTime;
		const float T = FMath::Clamp(TopOutTime / 1.1f, 0.f, 1.f);
		const float Up = FMath::SmoothStep(0.f, 0.6f, T), Over = FMath::SmoothStep(0.45f, 1.f, T);
		FVector P = TopOutFrom;
		P.Z = FMath::Lerp(TopOutFrom.Z, TopOutTo.Z, Up);
		P.X = FMath::Lerp(TopOutFrom.X, TopOutTo.X, Over);
		P.Y = FMath::Lerp(TopOutFrom.Y, TopOutTo.Y, Over);
		Character->SetActorLocation(P, false);
		Move->Velocity = FVector::ZeroVector;
		if (T >= 1.f)
		{
			bClimbing = false;
			bToppingOut = false;
			Surface = EClimbSurface::None;
			Character->bUseControllerRotationYaw = bSavedUseControllerYaw;
			Move->SetMovementMode(MOVE_Walking);
			if (Survival)
			{
				Survival->SetClimbing(false);
			}
			RestoreAnimation();
		}
		return;
	}

	// keep hold of the face: find it again behind the hands
	const FVector Into = -FVector(WallNormal.X, WallNormal.Y, 0.f).GetSafeNormal();
	FHitResult Hit;
	const bool bFace = TraceFace(Into, 0.f, Hit) || TraceFace(Into, 50.f, Hit) || TraceFace(Into, -40.f, Hit);
	if (!bFace)
	{
		// the face ended: over the top if there is a ledge, otherwise the ground got gentle enough to walk
		if (!TryTopOut())
		{
			FHitResult Ground;
			if (GetWorld()->SweepSingleByChannel(Ground, Character->GetActorLocation(), Character->GetActorLocation() + Into * (Reach + 40.f),
				FQuat::Identity, ECC_Visibility, FCollisionShape::MakeSphere(12.f), FCollisionQueryParams(SCENE_QUERY_STAT(ClimbEnd), false, Character))
				&& Move->IsWalkable(Ground))
			{
				LetGo(false);
				Move->SetMovementMode(MOVE_Walking);
			}
			else
			{
				LetGo(false);
			}
		}
		return;
	}
	WallNormal = FMath::VInterpTo(WallNormal, Hit.ImpactNormal, DeltaTime, 8.f).GetSafeNormal();
	FitBodyToFace(Hit, DeltaTime);
	const EClimbSurface Now = Classify(Hit);
	if (Now != Surface)
	{
		Surface = Now;
		if (!HintsShown.Contains(Surface))
		{
			HintsShown.Add(Surface);
			Say(RulesFor(Surface).Hint, 4.f);
		}
	}
	const FClimbSurfaceRules& R = RulesFor(Surface);

	// input in the face's own frame: W up the face, S down, A/D across
	const FVector Input = Character->ConsumeMovementInputVector();
	FRotator Yaw = Character->GetControlRotation();
	Yaw.Pitch = Yaw.Roll = 0.f;
	const float Fwd = FVector::DotProduct(Input, Yaw.Vector());
	const float Side = FVector::DotProduct(Input, FRotationMatrix(Yaw).GetUnitAxis(EAxis::Y));
	FVector UpFace = (FVector::UpVector - WallNormal * FVector::DotProduct(FVector::UpVector, WallNormal)).GetSafeNormal();
	const FVector RightFace = FVector::CrossProduct(WallNormal, UpFace).GetSafeNormal();   // right as you face the wall
	const bool bRope = Character->FindComponentByClass<UExpeditionComponent>() && Character->FindComponentByClass<UExpeditionComponent>()->IsOnRope();

	// tired climbers slip
	SlipTime -= DeltaTime;
	if (SlipTime <= 0.f && StaminaPercent() < SlipBelowStamina && FMath::FRand() < R.SlipChance * DeltaTime)
	{
		SlipTime = 0.5f;
		Say(NSLOCTEXT("Climb", "Slip", "Slipped!"), 1.5f);
		SpendStamina(6.f);
	}
	LungeTime -= DeltaTime;
	float Speed = ClimbSpeed * R.Speed;
	FVector Velocity = (UpFace * Fwd + RightFace * Side).GetClampedToMaxSize(1.f) * Speed;
	if (LungeTime > 0.f)
	{
		Velocity += UpFace * Speed * 2.5f;
	}
	if (SlipTime > 0.f)
	{
		Velocity = -UpFace * 220.f;
	}
	// hold the body at arm's length from the face
	const float Radius = Character->GetCapsuleComponent()->GetScaledCapsuleRadius();
	const float Gap = Hit.Distance - Radius;
	Velocity += Into * FMath::Clamp((Gap - 8.f) * 6.f, -120.f, 120.f);   // horizontal only: on an overhang it must not lift
	Move->Velocity = Velocity;
	Character->SetActorRotation(FRotator(0.f, (-WallNormal).Rotation().Yaw, 0.f));

	// stamina: moving costs the full rate, hanging still half, on a rope less
	const bool bMoving = FMath::Abs(Fwd) + FMath::Abs(Side) > 0.1f;
	SpendStamina(StaminaPerSecond * R.Drain * (bMoving ? 1.f : 0.5f) * (bRope ? RopeDrain : 1.f) * DeltaTime);
	if (StaminaPercent() <= 0.5f)
	{
		Say(NSLOCTEXT("Climb", "Spent", "Your grip gives out."), 3.f);
		LetGo(false);
		return;
	}

	// the clip for what the body is doing: wall moves on steep faces, the slope clip on snow under ~70 deg
	const bool bSlope = (Surface == EClimbSurface::SoftSnow || Surface == EClimbSurface::HardSnow) && WallNormal.Z > 0.34f;
	UAnimSequence* Clip = HangAnim;
	float Rate = 1.f;
	if (Fwd > 0.1f || LungeTime > 0.f)
	{
		Clip = bSlope ? SlopeAnim : ClimbUpAnim;
		Rate = FMath::Max(0.5f, R.Speed) * (LungeTime > 0.f ? 1.8f : 1.f);
	}
	else if (Fwd < -0.1f)
	{
		Clip = ClimbDownAnim;
		Rate = FMath::Max(0.5f, R.Speed);
	}
	else if (FMath::Abs(Side) > 0.1f)
	{
		Clip = Side > 0.f ? ShimmyRightAnim : ShimmyLeftAnim;
		Rate = FMath::Max(0.5f, R.Speed);
	}
	PlayClip(Clip, Rate);

	// climbing up into a ledge: pull over
	if (Fwd > 0.1f)
	{
		TryTopOut();
	}
}
