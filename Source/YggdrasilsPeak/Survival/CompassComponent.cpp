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
#include "Components/SpotLightComponent.h"
#include "HAL/IConsoleManager.h"

namespace
{
	const FName CompassEvent(TEXT("CompassTwitch"));
}

static TAutoConsoleVariable<float> CVarCompassFill(TEXT("ygg.Compass.Fill"), 2.f, TEXT("Lumens of the soft light on the held compass (lights only the compass). 5 made it read as a lamp in the tent (2026-11-21)."));
static TAutoConsoleVariable<float> CVarCompassLampSpill(TEXT("ygg.Compass.LampSpill"), 1.3f, TEXT("Under the player's headlamp the compass is lit by its fill only, this many times ygg.Compass.Fill, in the lamp's colour (2026-11-21)."));
static TAutoConsoleVariable<float> CVarCompassWear(TEXT("ygg.Compass.Wear"), 1.f, TEXT("0 = the polished brass as made (rough 0.33, read as a shiny toy), 1 = worn, hand-rubbed satin brass and needles (2026-11-21). Read when the compass is built."));
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
	// worn metal (2026-11-21, "the compass is too shiny, make it look natural"): the brass was a polished mirror (roughness
	// 0.33) that threw back every bright wall, the jewel and needles near-glass. A carried, hand-rubbed compass is satin:
	// rougher and a touch darker, so it takes the light around it instead of shining like a lamp.
	const float Wear = FMath::Clamp(CVarCompassWear.GetValueOnGameThread(), 0.f, 1.f);
	if (Wear > 0.f)
	{
		struct FWorn { const TCHAR* Slot; float Rough; float Dark; };
		static const FWorn WornParts[] = { { TEXT("brass"), 0.5f, 0.68f }, { TEXT("jewel"), 0.38f, 0.85f }, { TEXT("needle_n"), 0.55f, 0.9f }, { TEXT("needle_s"), 0.5f, 0.9f } };
		for (UStaticMeshComponent* C : { Case.Get(), Needle.Get() })
		{
			const UStaticMesh* SM = C->GetStaticMesh();
			for (const FWorn& W : WornParts)
			{
				const int32 Slot = SM ? SM->GetMaterialIndex(FName(W.Slot)) : INDEX_NONE;
				if (Slot == INDEX_NONE) { continue; }
				UMaterialInstanceDynamic* MID = C->CreateDynamicMaterialInstance(Slot);
				if (!MID) { continue; }
				float Rough = 0.3f;
				MID->GetScalarParameterValue(FHashedMaterialParameterInfo(TEXT("Roughness")), Rough);
				MID->SetScalarParameterValue(TEXT("Roughness"), FMath::Lerp(Rough, FMath::Max(Rough, W.Rough), Wear));
				FLinearColor Col(0.5f, 0.5f, 0.5f);
				if (MID->GetVectorParameterValue(FHashedMaterialParameterInfo(TEXT("BaseColor")), Col))
				{
					MID->SetVectorParameterValue(TEXT("BaseColor"), Col * FMath::Lerp(1.f, W.Dark, Wear));
				}
			}
		}
	}
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

void UCompassComponent::SetStoryTarget(FVector Target, float Tremble)
{
	const float Was = StoryTremble;
	StoryTremble = FMath::Clamp(Tremble, 0.f, 1.f);
	if (StoryTremble > 0.f)
	{
		StoryTarget = Target;
		if (Was <= 0.f)
		{
			// it wakes in the hand: a jolt, then it swings over
			Kick(0.5f * StoryTremble, false);
		}
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

	// the story's target swings the needle over (and back when released) over about a second
	StoryPull = FMath::FInterpTo(StoryPull, StoryTremble > 0.f ? 1.f : 0.f, DeltaTime, 2.5f);

	// raise / lower (held up while the story has the needle)
	ShowFor = FMath::Max(0.f, ShowFor - DeltaTime);
	const bool bShow = bRaised || ShowFor > 0.f || StoryTremble > 0.f || CVarCompassShow.GetValueOnGameThread() > 0;
	RaiseAlpha = FMath::FInterpTo(RaiseAlpha, bShow ? 1.f : 0.f, DeltaTime, bShow ? 7.f : 9.f);
	const bool bVisible = RaiseAlpha > 0.01f;
	Case->SetVisibility(bVisible);
	Needle->SetVisibility(bVisible);
	if (Fill)
	{
		// (2026-11-21) under the player's story headlamp the compass, held 30 cm in front of it, took the full spot: a
		// blown-out lid and dial, "like the compass is the flashlight itself". While the headlamp shines the compass leaves
		// its light channel and the fill takes the lamp's warm colour at a few lumens with no shine - the soft spill a
		// real headlamp throws on something held under it.
		if (!HeadlampRef.IsValid())
		{
			TInlineComponentArray<USpotLightComponent*> Spots(Character);
			for (USpotLightComponent* S : Spots) { if (S->GetFName() == TEXT("StoryHeadlamp")) { HeadlampRef = S; break; } }
		}
		const bool bLamp = HeadlampRef.IsValid() && HeadlampRef->IsVisible() && HeadlampRef->Intensity > 0.05f;
		if (bLamp != bUnderLamp)
		{
			bUnderLamp = bLamp;
			Case->SetLightingChannels(!bLamp, true, false);
			Needle->SetLightingChannels(!bLamp, true, false);
			Fill->SetLightColor(bLamp ? FLinearColor(1.f, 0.9f, 0.78f) : FLinearColor(0.72f, 0.82f, 1.f));
		}
		Fill->SetVisibility(bVisible);
		Fill->SetIntensity(CVarCompassFill.GetValueOnGameThread() * RaiseAlpha * (bUnderLamp ? CVarCompassLampSpill.GetValueOnGameThread() : 1.f));
	}
	if (!bVisible)
	{
		return;
	}
	ShakeTime += DeltaTime;
	Impulse = FMath::FInterpTo(Impulse, 0.f, DeltaTime, 0.8f);
	// a little hand tremble, more with the cold and the altitude, and with whatever pulls the needle in the story
	const float Tremble = 0.15f + 0.35f * Pull + Impulse + 0.5f * StoryTremble * StoryPull;
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
	float Bearing = NorthYaw + FMath::FindDeltaAngleDegrees(NorthYaw, SummitYaw) * Pull;
	if (StoryPull > 0.001f)
	{
		// the story's pull wins over north and the summit (angles taken the short way round)
		const float StoryYaw = (StoryTarget - Character->GetActorLocation()).Rotation().Yaw;
		Bearing += FMath::FindDeltaAngleDegrees(Bearing, StoryYaw) * StoryPull;
	}
	const float Shake = (FMath::PerlinNoise1D(ShakeTime * 3.f) * 4.f + FMath::PerlinNoise1D(ShakeTime * 17.f) * 1.5f)
		* (Pull * 3.f + Impulse * 6.f + StoryTremble * StoryPull * 4.f);
	const float Target = FMath::FindDeltaAngleDegrees(0.f, Bearing - CamYaw) + Shake;   // case turned +90 (lid away from you); the red half of the needle lies along the case's north
	const float Delta = FMath::FindDeltaAngleDegrees(NeedleYaw, Target);
	NeedleSpeed += (Delta * 60.f - NeedleSpeed * 6.f) * DeltaTime;
	NeedleYaw += NeedleSpeed * DeltaTime;
	Needle->SetRelativeRotation(FRotator(0.f, NeedleYaw, 0.f));

	// the runes wake while it points up the mountain
	if (RuneGlow)
	{
		const float Wake = FMath::Max(Pull, StoryTremble * StoryPull);
		const float Glow = Wake * (0.4f + 0.6f * FMath::Abs(FMath::Sin(ShakeTime * 1.4f))) + Impulse;
		RuneGlow->SetScalarParameterValue(TEXT("Glow"), Glow);
		if (LidGlow)
		{
			LidGlow->SetScalarParameterValue(TEXT("Glow"), Glow * 0.6f);
		}
	}
}
