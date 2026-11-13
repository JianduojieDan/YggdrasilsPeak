// Copyright Epic Games, Inc. All Rights Reserved.

#include "Survival/CompassComponent.h"

#include "Survival/YggSettings.h"
#include "Environment/RouteLandmark.h"
#include "Survival/ExpeditionComponent.h"
#include "Survival/StoryClue.h"
#include "Survival/SurvivalComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Components/PointLightComponent.h"
#include "HAL/IConsoleManager.h"

namespace
{
	const FName CompassEvent(TEXT("CompassTwitch"));
}

static TAutoConsoleVariable<float> CVarCompassFill(TEXT("ygg.Compass.Fill"), 5.f, TEXT("Lumens of the soft light on the held compass (lights only the compass)."));
static TAutoConsoleVariable<int32> CVarCompassShow(TEXT("ygg.Compass.Show"), 0, TEXT("1 = hold the compass up (screenshots)."));

UCompassComponent::UCompassComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PostPhysics;     // after the camera has moved this frame
}

void UCompassComponent::BeginPlay()
{
	Super::BeginPlay();
	// the summit: the highest route landmark; the parents' camp: its story clue
	float Highest = -1e9f;
	for (TActorIterator<ARouteLandmark> It(GetWorld()); It; ++It)
	{
		if (It->GetActorLocation().Z > Highest)
		{
			Highest = It->GetActorLocation().Z;
			SummitLocation = It->GetActorLocation();
		}
	}
	for (TActorIterator<AStoryClue> It(GetWorld()); It; ++It)
	{
		if (It->ClueId.ToString().StartsWith(TEXT("ParentsCamp")))
		{
			ParentsCamp = It->GetActorLocation();
			bHaveCamp = true;
		}
	}
	Build();
}

void UCompassComponent::Build()
{
	ACharacter* Character = Cast<ACharacter>(GetOwner());
	UCameraComponent* Camera = Character ? Character->FindComponentByClass<UCameraComponent>() : nullptr;
	UStaticMesh* CaseMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Props/Compass/SM_Compass2_Case.SM_Compass2_Case"));
	UStaticMesh* NeedleMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Props/Compass/SM_Compass2_Needle.SM_Compass2_Needle"));
	if (!Camera || !CaseMesh || !NeedleMesh || !Character->IsLocallyControlled())
	{
		return;
	}
	Case = NewObject<UStaticMeshComponent>(Character, TEXT("HeldCompass"));
	Case->SetStaticMesh(CaseMesh);
	Case->SetupAttachment(Camera);
	Needle = NewObject<UStaticMeshComponent>(Character, TEXT("HeldCompassNeedle"));
	Needle->SetStaticMesh(NeedleMesh);
	Needle->SetupAttachment(Case);
	for (UStaticMeshComponent* C : { Case.Get(), Needle.Get() })
	{
		C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		C->SetCastShadow(false);
		C->SetFirstPersonPrimitiveType(EFirstPersonPrimitiveType::FirstPerson);   // never pokes into walls
		C->SetVisibility(false);
		C->RegisterComponent();
	}
	// its own soft light (backlog #66: in a storm, lit only by a dark sky, the dial went black): lights the compass and
	// nothing else (lighting channel 1 only, so no pool of light on the snow), on only while it is held up
	Case->SetLightingChannels(true, true, false);
	Needle->SetLightingChannels(true, true, false);
	// from the eyes, not above the lid (a light above it mirrored in the gold as a hot glare: "too shiny"), the cool
	// colour of the storm's own light, no reflection on metal, and only a few lumens: it lifts the compass to the
	// atmosphere around it instead of lighting it like a lamp ("let the compass fit the atmospheric light", 2026-11-13)
	Fill = NewObject<UPointLightComponent>(Character, TEXT("HeldCompassFill"));
	Fill->SetupAttachment(Camera);
	Fill->SetRelativeLocation(FVector(4.f, 0.f, 4.f));
	Fill->SetSpecularScale(0.f);
	Fill->SetIntensityUnits(ELightUnits::Lumens);
	Fill->SetIntensity(CVarCompassFill.GetValueOnGameThread());
	Fill->SetAttenuationRadius(60.f);
	Fill->SetSourceRadius(12.f);
	Fill->SetLightColor(FLinearColor(0.72f, 0.82f, 1.f));
	Fill->SetCastShadows(false);
	Fill->SetLightingChannels(false, true, false);
	Fill->SetVisibility(false);
	Fill->RegisterComponent();
	// the runes glow on the dial (M_CompassDial), the World Tree inside the lid with them (M_CompassLidIn)
	const int32 RuneSlot = CaseMesh->GetMaterialIndex(TEXT("dial"));
	if (RuneSlot != INDEX_NONE)
	{
		RuneGlow = Case->CreateDynamicMaterialInstance(RuneSlot);
	}
	const int32 LidSlot = CaseMesh->GetMaterialIndex(TEXT("lidin"));
	if (LidSlot != INDEX_NONE)
	{
		LidGlow = Case->CreateDynamicMaterialInstance(LidSlot);
	}
	if (const UExpeditionComponent* Expedition = Character->FindComponentByClass<UExpeditionComponent>())
	{
		bAwake = Expedition->HasFoundClue(CompassEvent);
	}
}

bool UCompassComponent::TryBindInput()
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
	ToggleAction = NewObject<UInputAction>(this, TEXT("IA_Compass"));
	Mapping->MapKey(ToggleAction, YggSettings::KeyFor(TEXT("Compass"), EKeys::Q)); YggSettings::RegisterKey(TEXT("Compass"), INVTEXT("Compass"), EKeys::Q, Mapping, ToggleAction);
	Mapping->MapKey(ToggleAction, EKeys::Gamepad_DPad_Up);
	Subsystem->AddMappingContext(Mapping, 2);
	Input->BindAction(ToggleAction, ETriggerEvent::Started, this, &UCompassComponent::Toggle);
	return true;
}

void UCompassComponent::Toggle()
{
	bRaised = !bRaised;
	ShowFor = 0.f;
}

void UCompassComponent::Kick(float Strength, bool bShowIt)
{
	Impulse = FMath::Max(Impulse, FMath::Clamp(Strength, 0.f, 1.f));
	NeedleSpeed += (FMath::RandBool() ? 1.f : -1.f) * 900.f * Strength;
	if (bShowIt && !bRaised)
	{
		ShowFor = 3.5f;
	}
}

void UCompassComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	ACharacter* Character = Cast<ACharacter>(GetOwner());
	if (!Character || !Case)
	{
		return;
	}
	if (!bInputBound)
	{
		bInputBound = TryBindInput();
	}

	// the parents' camp: the first time near it, the needle leaves north for the summit
	if (!bAwake && bHaveCamp && FVector::Dist2D(Character->GetActorLocation(), ParentsCamp) < AwakenRadius)
	{
		bAwake = true;
		Kick(1.f, true);
		ShowFor = 5.f;
		if (UExpeditionComponent* Expedition = Character->FindComponentByClass<UExpeditionComponent>())
		{
			Expedition->MarkStoryEvent(CompassEvent);
			Expedition->ShowMessage(NSLOCTEXT("Compass", "Awake", "Father's compass... the needle swings off north, up towards the summit - and holds there."), 6.f);
		}
	}

	// how strongly the summit pulls: nothing before the camp, then more with height
	const USurvivalComponent* Survival = Character->FindComponentByClass<USurvivalComponent>();
	const float Elevation = Survival ? Survival->GetElevationMeters() : 0.f;
	const float TargetPull = bAwake ? FMath::GetMappedRangeValueClamped(FVector2f(5150.f, 7000.f), FVector2f(0.35f, 1.f), Elevation) : 0.f;
	Pull = FMath::FInterpTo(Pull, TargetPull, DeltaTime, 0.5f);

	// raise / lower
	ShowFor = FMath::Max(0.f, ShowFor - DeltaTime);
	const bool bShow = bRaised || ShowFor > 0.f || CVarCompassShow.GetValueOnGameThread() > 0;
	RaiseAlpha = FMath::FInterpTo(RaiseAlpha, bShow ? 1.f : 0.f, DeltaTime, bShow ? 7.f : 9.f);
	const bool bVisible = RaiseAlpha > 0.01f;
	Case->SetVisibility(bVisible);
	Needle->SetVisibility(bVisible);
	if (Fill)
	{
		Fill->SetVisibility(bVisible);
		Fill->SetIntensity(CVarCompassFill.GetValueOnGameThread() * RaiseAlpha);
	}
	if (!bVisible)
	{
		return;
	}
	ShakeTime += DeltaTime;
	Impulse = FMath::FInterpTo(Impulse, 0.f, DeltaTime, 0.8f);
	// a little hand tremble, more with the cold and the altitude
	const float Tremble = 0.15f + 0.35f * Pull + Impulse;
	const FVector Wobble(0.f, FMath::Sin(ShakeTime * 2.1f) * 0.25f, FMath::Sin(ShakeTime * 1.3f) * 0.3f);
	const float Ease = 1.f - FMath::Pow(1.f - RaiseAlpha, 3.f);
	const FVector Hidden = HeldOffset + FVector(-6.f, 4.f, -26.f);
	Case->SetRelativeLocation(FMath::Lerp(Hidden, HeldOffset, Ease) + Wobble * Tremble);
	// turn the lid away from you first, then tilt the face towards the eye (in the camera's frame)
	const FQuat Turn = FRotator(0.f, 90.f, 0.f).Quaternion();
	const FQuat Tilt = FRotator(HeldTilt * Ease - 20.f * (1.f - Ease) + FMath::Sin(ShakeTime * 9.f) * Tremble, 0.f, FMath::Sin(ShakeTime * 7.f) * Tremble).Quaternion();
	Case->SetRelativeRotation(Tilt * Turn);
	Case->SetRelativeScale3D(FVector(HeldScale));

	// the needle: a damped spring towards its target bearing (north blended into the summit), shaking as it is pulled
	const float CamYaw = Case->GetAttachParent() ? Case->GetAttachParent()->GetComponentRotation().Yaw : 0.f;
	const float SummitYaw = (SummitLocation - Character->GetActorLocation()).Rotation().Yaw;
	const float Bearing = NorthYaw + FMath::FindDeltaAngleDegrees(NorthYaw, SummitYaw) * Pull;
	const float Shake = (FMath::PerlinNoise1D(ShakeTime * 3.f) * 4.f + FMath::PerlinNoise1D(ShakeTime * 17.f) * 1.5f) * (Pull * 3.f + Impulse * 6.f);
	const float Target = FMath::FindDeltaAngleDegrees(0.f, Bearing - CamYaw) + Shake;   // case turned +90 (lid away from you); the red half of the needle lies along the case's north
	const float Delta = FMath::FindDeltaAngleDegrees(NeedleYaw, Target);
	NeedleSpeed += (Delta * 60.f - NeedleSpeed * 6.f) * DeltaTime;
	NeedleYaw += NeedleSpeed * DeltaTime;
	Needle->SetRelativeRotation(FRotator(0.f, NeedleYaw, 0.f));

	// the runes wake while it points up the mountain
	if (RuneGlow)
	{
		const float Glow = Pull * (0.4f + 0.6f * FMath::Abs(FMath::Sin(ShakeTime * 1.4f))) + Impulse;
		RuneGlow->SetScalarParameterValue(TEXT("Glow"), Glow);
		if (LidGlow)
		{
			LidGlow->SetScalarParameterValue(TEXT("Glow"), Glow * 0.6f);
		}
	}
}
