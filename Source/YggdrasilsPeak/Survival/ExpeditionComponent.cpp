// Copyright Epic Games, Inc. All Rights Reserved.

#include "Survival/ExpeditionComponent.h"
#include "Survival/YggMenuWidget.h"

#include "Survival/InventoryComponent.h"
#include "Survival/ClimbingComponent.h"
#include "Survival/CompassComponent.h"
#include "Survival/SanityComponent.h"
#include "Survival/SnowTrailComponent.h"
#include "Survival/SpindriftComponent.h"
#include "Survival/MountainAudioComponent.h"
#include "Survival/SurvivalComponent.h"
#include "Survival/SurvivalConfig.h"
#include "Survival/YggSaveGame.h"
#include "Survival/YggTime.h"
#include "Environment/FixedRope.h"
#include "Environment/TrailCheckpoint.h"
#include "Survival/StoryClue.h"
#include "Environment/RouteLandmark.h"
#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Environment/RealmSky.h"
#include "Survival/YggBodyCameraModifier.h"
#include "Survival/YggTelemetryComponent.h"
#include "Survival/YggSettings.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/WorldSettings.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

static TAutoConsoleVariable<int32> CVarLoadOnStart(TEXT("ygg.Save.LoadOnStart"), 1,
	TEXT("1 = starting the game continues from the last camp saved on disk, 0 = always a fresh start."));

static TAutoConsoleVariable<int32> CVarLoadInEditor(TEXT("ygg.Save.LoadInEditor"), 0,
	TEXT("0 = the editor's Play button ignores the save and starts at the PlayerStart, 1 = it continues from the save like the real game."));

// The body on the view, held at a value for screenshots and tuning (-1 = live)
static TAutoConsoleVariable<float> CVarBodyHypoxia(TEXT("ygg.Body.Hypoxia"), -1.f, TEXT("Hold the thin-air tunnel at 0..1 (-1 = live)."));
static TAutoConsoleVariable<float> CVarBodyFrost(TEXT("ygg.Body.Frost"), -1.f, TEXT("Hold the frost on the lens at 0..1 (-1 = live)."));
static TAutoConsoleVariable<float> CVarBodyExhaust(TEXT("ygg.Body.Exhaust"), -1.f, TEXT("Hold the spent swim and sway at 0..1 (-1 = live)."));
static TAutoConsoleVariable<float> CVarBodyFaint(TEXT("ygg.Body.Faint"), -1.f, TEXT("Hold the near-collapse grey waves at 0..1 (-1 = live)."));
static TAutoConsoleVariable<float> CVarBodyShiver(TEXT("ygg.Body.Shiver"), -1.f, TEXT("Hold the shiver at 0..1 (-1 = live)."));

static FAutoConsoleCommand CmdNewGame(TEXT("ygg.NewGame"), TEXT("Deletes the expedition save; the next start is a new game."),
	FConsoleCommandDelegate::CreateLambda([]()
	{
		UGameplayStatics::DeleteGameInSlot(UYggSaveGame::SlotName, 0);
		UE_LOG(LogTemp, Display, TEXT("Expedition save deleted - restart to begin a new game."));
	}));

UExpeditionComponent::UExpeditionComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
}

void UExpeditionComponent::BeginPlay()
{
	Super::BeginPlay();

	for (TActorIterator<ARouteLandmark> It(GetWorld()); It; ++It)
	{
		Camps.Add(*It);
	}
	Camps.Sort([](const TWeakObjectPtr<ARouteLandmark>& A, const TWeakObjectPtr<ARouteLandmark>& B)
	{
		return A->GetActorLocation().Z < B->GetActorLocation().Z;
	});

	for (TActorIterator<AFixedRope> It(GetWorld()); It; ++It)
	{
		Ropes.Add(*It);
	}
	for (TActorIterator<ATrailCheckpoint> It(GetWorld()); It; ++It)
	{
		TrailCheckpoints.Add(*It);
	}
	if (const ACharacter* Character = Cast<ACharacter>(GetOwner()))
	{
		DefaultWalkableFloorAngle = Character->GetCharacterMovement()->GetWalkableFloorAngle();
	}

	// Footprints for the local player (not part of the Blueprint, so it is added here).
	const APawn* Pawn = Cast<APawn>(GetOwner());
	if (Pawn && Pawn->IsLocallyControlled() && !GetOwner()->FindComponentByClass<USnowTrailComponent>())
	{
		USnowTrailComponent* Trails = NewObject<USnowTrailComponent>(GetOwner(), TEXT("SnowTrails"));
		Trails->RegisterComponent();
	}
	// The mind: sanity, hallucinations.
	if (Pawn && Pawn->IsLocallyControlled() && !GetOwner()->FindComponentByClass<USanityComponent>())
	{
		USanityComponent* Mind = NewObject<USanityComponent>(GetOwner(), TEXT("Sanity"));
		Mind->RegisterComponent();
	}
	// Father's rune compass, held up with Q.
	if (Pawn && Pawn->IsLocallyControlled() && !GetOwner()->FindComponentByClass<UCompassComponent>())
	{
		UCompassComponent* Compass = NewObject<UCompassComponent>(GetOwner(), TEXT("Compass"));
		Compass->RegisterComponent();
	}
	// Climbing steep faces (not part of the Blueprint either).
	if (Pawn && !GetOwner()->FindComponentByClass<UClimbingComponent>())
	{
		UClimbingComponent* Climbing = NewObject<UClimbingComponent>(GetOwner(), TEXT("Climbing"));
		Climbing->RegisterComponent();
	}
	// Blowing snow around the local player, driven by the wind read in UpdateWeather.
	if (Pawn && Pawn->IsLocallyControlled() && !GetOwner()->FindComponentByClass<USpindriftComponent>())
	{
		USpindriftComponent* Spindrift = NewObject<USpindriftComponent>(GetOwner(), TEXT("Spindrift"));
		Spindrift->RegisterComponent();
	}
	// The thesis data log (Development_Plan.md A3): one CSV row per second while ygg.Log is 1.
	if (Pawn && Pawn->IsLocallyControlled() && !GetOwner()->FindComponentByClass<UYggTelemetryComponent>())
	{
		UYggTelemetryComponent* DataLog = NewObject<UYggTelemetryComponent>(GetOwner(), TEXT("Telemetry"));
		DataLog->RegisterComponent();
	}
	// The mountain's sound around the local player: wind beds, breathing, rope clicks, distant avalanches.
	if (Pawn && Pawn->IsLocallyControlled() && !GetOwner()->FindComponentByClass<UMountainAudioComponent>())
	{
		UMountainAudioComponent* Audio = NewObject<UMountainAudioComponent>(GetOwner(), TEXT("MountainAudio"));
		Audio->RegisterComponent();
	}

	Survival = GetOwner()->FindComponentByClass<USurvivalComponent>();
	// The pack (supplies and weight), unless the Blueprint already has one.
	if (Pawn && Pawn->IsLocallyControlled() && !GetOwner()->FindComponentByClass<UInventoryComponent>())
	{
		UInventoryComponent* Pack = NewObject<UInventoryComponent>(GetOwner(), TEXT("Pack"));
		Pack->RegisterComponent();
	}
	if (Survival.IsValid())
	{
		Survival->OnDeath.AddDynamic(this, &UExpeditionComponent::HandleDeath);
	}
	StartTime = GetWorld()->GetTimeSeconds();
	// Play in the editor starts where the level says, so the world can be checked from anywhere; ygg.Save.LoadInEditor 1 tests the save itself.
	const bool bAllowLoad = GetWorld()->WorldType != EWorldType::PIE || CVarLoadInEditor.GetValueOnGameThread() != 0;
	if (bAllowLoad && CVarLoadOnStart.GetValueOnGameThread() != 0 && Pawn && Pawn->IsLocallyControlled())
	{
		GetWorld()->GetTimerManager().SetTimerForNextTick(this, &UExpeditionComponent::LoadSavedExpedition);
	}

	// Ultra Dynamic Weather is a Blueprint; find it by class name so the code has no hard dependency on it.
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		if (It->GetClass()->GetName().Contains(TEXT("Ultra_Dynamic_Weather")))
		{
			WeatherActor = *It;
			break;
		}
	}
}

namespace
{
	FString Simplify(const FString& Name)
	{
		return Name.Replace(TEXT(" "), TEXT("")).ToLower();
	}

	FProperty* FindByDisplayName(const UClass* Class, const TCHAR* Name)
	{
		const FString Wanted = Simplify(Name);
		for (TFieldIterator<FProperty> It(Class); It; ++It)
		{
			if (Simplify(It->GetAuthoredName()) == Wanted || Simplify(It->GetName()) == Wanted)
			{
				return *It;
			}
		}
		return nullptr;
	}
}

bool UExpeditionComponent::ReadNumber(const UObject* Object, const TCHAR* Name, double& Out)
{
	const FProperty* Prop = Object ? FindByDisplayName(Object->GetClass(), Name) : nullptr;
	if (const FDoubleProperty* D = CastField<FDoubleProperty>(Prop))
	{
		Out = D->GetPropertyValue_InContainer(Object);
		return true;
	}
	if (const FFloatProperty* F = CastField<FFloatProperty>(Prop))
	{
		Out = F->GetPropertyValue_InContainer(Object);
		return true;
	}
	return false;
}

UObject* UExpeditionComponent::ReadObject(const UObject* Object, const TCHAR* Name)
{
	const FObjectPropertyBase* Prop = Object ? CastField<FObjectPropertyBase>(FindByDisplayName(Object->GetClass(), Name)) : nullptr;
	return Prop ? Prop->GetObjectPropertyValue_InContainer(Object) : nullptr;
}

void UExpeditionComponent::UpdateWeather()
{
	if (!Survival.IsValid())
	{
		return;
	}

	// Local state includes radial storms passing over the player; values are on UDW's 0..10 scale.
	const UObject* State = WeatherActor.IsValid() ? ReadObject(WeatherActor.Get(), TEXT("Local Weather State")) : nullptr;
	double Wind = 0.0, Snow = 0.0;
	if (State)
	{
		ReadNumber(State, TEXT("Wind Intensity"), Wind);
		ReadNumber(State, TEXT("Snow"), Snow);
		WindIntensity = float(Wind);
		double Direction = 0.0;
		if (ReadNumber(WeatherActor.Get(), TEXT("Wind Direction"), Direction))
		{
			WindDirection = float(Direction);
		}
	}
	const float NewStorm = FMath::Clamp(0.6f * float((Wind - 3.0) / 7.0) + 0.4f * float((Snow - 4.0) / 6.0), 0.f, 1.f);
	if (FMath::Abs(NewStorm - LoggedStorm) > 0.1f || (State == nullptr) != bLoggedNoState)
	{
		LoggedStorm = NewStorm;
		bLoggedNoState = State == nullptr;
		UE_LOG(LogSurvival, Log, TEXT("Weather: wind %.1f snow %.1f -> storm %.2f%s"), Wind, Snow, NewStorm, State ? TEXT("") : TEXT(" (no UDW local state found)"));
	}
	StormStrength = NewStorm;

	Survival->SetColdExposure(StormColdExposure * StormStrength);
	Survival->SetWeatherSpeedMultiplier(FMath::Lerp(1.f, StormSpeedMultiplier, StormStrength));

	if (!bStormWarned && StormStrength > 0.45f)
	{
		bStormWarned = true;
		Say(NSLOCTEXT("Expedition", "Storm", "A blizzard is coming in. Get to a camp or keep moving."), 7.f);
	}
	else if (bStormWarned && StormStrength < 0.2f)
	{
		bStormWarned = false;
		Say(NSLOCTEXT("Expedition", "StormOver", "The storm is passing."), 4.f);
	}
}

void UExpeditionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Fell through the world (a gap in the terrain, pushed out of geometry): back to the last checkpoint before the
	// engine's kill height deletes the climber, which would leave the game without a player.
	if (GetOwner()->GetActorLocation().Z < GetWorld()->GetWorldSettings()->KillZ * 0.5f)
	{
		AActor* Back = LastCheckpoint.IsValid() ? LastCheckpoint.Get() : (Camps.Num() > 0 ? static_cast<AActor*>(Camps[0].Get()) : nullptr);
		if (Back)
		{
			TeleportNear(Back);
			Say(FText::Format(NSLOCTEXT("Expedition", "FellOut", "You fell out of the world - back to {0}."),
				FText::FromString(LastCheckpoint.IsValid() ? LastCheckpointName : Camps[0]->LandmarkName)), 5.f);
		}
	}

	CampCheckTimer -= DeltaTime;
	if (CampCheckTimer <= 0.f)
	{
		CampCheckTimer = 0.5f;
		UpdateCamps();
		UpdateTrailCheckpoints();
		UpdateWeather();
	}
	RopeCheckTimer -= DeltaTime;
	if (RopeCheckTimer <= 0.f)
	{
		RopeCheckTimer = 0.2f;
		UpdateRope();
	}
	UpdateScreenEffect(DeltaTime);
}

void UExpeditionComponent::UpdateRope()
{
	ACharacter* Character = Cast<ACharacter>(GetOwner());
	if (!Character || !Survival.IsValid() || Survival->IsDead())
	{
		return;
	}

	// Measure from the feet: the rope lies just above the snow.
	const FVector Feet = Character->GetActorLocation() - FVector(0.f, 0.f, Character->GetSimpleCollisionHalfHeight());
	// Letting go needs a bit more distance than clipping on, so walking along the edge does not flicker.
	const float Reach = bOnRope ? RopeClipDistance * 1.5f : RopeClipDistance;
	bool bClipped = false;
	for (const TWeakObjectPtr<AFixedRope>& Rope : Ropes)
	{
		if (Rope.IsValid() && Rope->IsNear(Feet, Reach) && Rope->DistanceToRope(Feet) < Reach)
		{
			bClipped = true;
			break;
		}
	}
	if (bClipped == bOnRope)
	{
		return;
	}

	bOnRope = bClipped;
	Survival->SetOnRope(bOnRope);
	Character->GetCharacterMovement()->SetWalkableFloorAngle(bOnRope ? FMath::Max(RopeWalkableFloorAngle, DefaultWalkableFloorAngle) : DefaultWalkableFloorAngle);

	if (bOnRope && !bRopeExplained)
	{
		bRopeExplained = true;
		Say(NSLOCTEXT("Expedition", "RopeFirst", "Clipped to the fixed rope. Follow it up."), 7.f);
	}
	else if (!bOnRope)
	{
		Say(NSLOCTEXT("Expedition", "RopeOff", "Off the rope. Find the yellow line again."), 4.f);
	}
}

void UExpeditionComponent::UpdateCamps()
{
	if (!Survival.IsValid() || Survival->IsDead())
	{
		return;
	}

	const FVector Here = GetOwner()->GetActorLocation();
	ARouteLandmark* Inside = nullptr;
	for (const TWeakObjectPtr<ARouteLandmark>& Camp : Camps)
	{
		if (Camp.IsValid() && FVector::DistSquared2D(Here, Camp->GetActorLocation()) < FMath::Square(CampRadius))
		{
			Inside = Camp.Get();
			break;
		}
	}

	Survival->SetSheltered(Inside != nullptr);
	if (Inside == CurrentCamp.Get())
	{
		return;
	}
	CurrentCamp = Inside;
	if (!Inside)
	{
		return;
	}

	if (Inside->Kind == ERouteLandmarkKind::Summit && !bSummitReached)
	{
		bSummitReached = true;
		LastCamp = Inside;
		SaveCheckpoint(Inside, Inside->LandmarkName);
		Say(FText::Format(NSLOCTEXT("Expedition", "Summit", "SUMMIT - {0}. Now get back down alive."), FText::FromString(Inside->LandmarkName)), 10.f);
		// the summit camp by the World Tree is the end of the climb: the summit screen (2026-11-11)
		if (const APawn* Pawn = Cast<APawn>(GetOwner()))
		{
			const int32 Seconds = FMath::RoundToInt(SavedPlaySeconds + GetWorld()->GetTimeSeconds() - StartTime);
			UYggMenuWidget::ShowSummit(Cast<APlayerController>(Pawn->GetController()), Seconds, Deaths);
		}
		return;
	}

	if (Inside->bFinishLine && bSummitReached && !bFinished)
	{
		bFinished = true;
		const int32 Seconds = FMath::RoundToInt(SavedPlaySeconds + GetWorld()->GetTimeSeconds() - StartTime);
		Say(FText::Format(NSLOCTEXT("Expedition", "Finished", "EXPEDITION COMPLETE - summit and back in {0}h {1}m, {2} deaths."),
			FText::AsNumber(Seconds / 3600), FText::AsNumber((Seconds / 60) % 60), FText::AsNumber(Deaths)), 20.f);
		LastCamp = Inside;
		SaveCheckpoint(Inside, Inside->LandmarkName);
		return;
	}

	const bool bNew = LastCamp.Get() != Inside;
	LastCamp = Inside;
	SaveCheckpoint(Inside, Inside->LandmarkName);
	Say(FText::Format(bNew ? NSLOCTEXT("Expedition", "CampNew", "{0} - shelter. Checkpoint saved.  [R] rest")
		: NSLOCTEXT("Expedition", "Camp", "{0} - shelter.  [R] rest"), FText::FromString(Inside->LandmarkName)), 5.f);
}

void UExpeditionComponent::HandleDeath()
{
	++Deaths;
	if (ACharacter* Character = Cast<ACharacter>(GetOwner()))
	{
		Character->GetCharacterMovement()->DisableMovement();
	}

	const FText Cause = Survival.IsValid() && !Survival->GetDeathCause().IsEmpty() ? Survival->GetDeathCause()
		: NSLOCTEXT("Expedition", "Died", "You died.");
	const FText Where = LastCheckpoint.IsValid() ? FText::FromString(LastCheckpointName) : NSLOCTEXT("Expedition", "Start", "the start");
	// the death screen after a breath (2026-11-12 menus): what took you, where, and back to the last camp from there
	const APawn* Pawn = Cast<APawn>(GetOwner());
	if (Pawn && Cast<APlayerController>(Pawn->GetController()))
	{
		GetWorld()->GetTimerManager().SetTimer(RespawnTimer, this, &UExpeditionComponent::ShowDeathScreen, 2.5f);
		return;
	}
	Say(FText::Format(NSLOCTEXT("Expedition", "Death", "{0} Back to {1}..."), Cause, Where), RespawnDelay + 2.f);
	GetWorld()->GetTimerManager().SetTimer(RespawnTimer, this, &UExpeditionComponent::Respawn, FMath::Max(RespawnDelay, 0.1f));
}

int32 UExpeditionComponent::GetPlaySeconds() const
{
	return FMath::RoundToInt(SavedPlaySeconds + GetWorld()->GetTimeSeconds() - StartTime);
}

void UExpeditionComponent::ShowDeathScreen()
{
	const APawn* Pawn = Cast<APawn>(GetOwner());
	APlayerController* PC = Pawn ? Cast<APlayerController>(Pawn->GetController()) : nullptr;
	const FText Cause = Survival.IsValid() && !Survival->GetDeathCause().IsEmpty() ? Survival->GetDeathCause()
		: NSLOCTEXT("Expedition", "Died", "You died.");
	const float Meters = Survival.IsValid() ? Survival->GetElevationMeters() : 0.f;
	TWeakObjectPtr<UExpeditionComponent> Self(this);
	UYggMenuWidget::ShowDeath(PC, Cause, Meters, GetPlaySeconds(), Deaths, LastCheckpoint.IsValid() ? LastCheckpointName : FString(),
		[Self]() { if (Self.IsValid()) { Self->Respawn(); } });
	if (!UYggMenuWidget::IsOpen(PC))
	{
		Respawn();
	}
}

void UExpeditionComponent::Respawn()
{
	if (LastCheckpoint.IsValid())
	{
		TeleportNear(LastCheckpoint.Get());
	}
	else if (Camps.Num() > 0 && Camps[0].IsValid())
	{
		TeleportNear(Camps[0].Get());   // no checkpoint yet: don't revive where you died (e.g. inside a crevasse)
	}
	if (ACharacter* Character = Cast<ACharacter>(GetOwner()))
	{
		Character->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	}
	if (Survival.IsValid())
	{
		Survival->Revive(RespawnNeedsPercent);
		Survival->SetCarriedBottles(CheckpointBottles);
	}
	// Supplies used or found since the camp are rolled back with you.
	if (UInventoryComponent* Pack = GetOwner()->FindComponentByClass<UInventoryComponent>())
	{
		Pack->RestoreCheckpoint();
	}
}

void UExpeditionComponent::SaveCheckpoint(AActor* Where, const FString& Name)
{
	LastCheckpoint = Where;
	LastCheckpointName = Name;
	if (UInventoryComponent* Pack = GetOwner()->FindComponentByClass<UInventoryComponent>())
	{
		Pack->SaveCheckpoint();
		CheckpointItems = Pack->GetItems();
	}
	CheckpointBottles = Survival.IsValid() ? Survival->GetCarriedBottles() : 0;
	CheckpointStats.Reset();
	if (Survival.IsValid())
	{
		for (uint8 i = 0; i <= uint8(ESurvivalStat::Oxygen); ++i)
		{
			CheckpointStats.Add(Survival->GetStatValue(ESurvivalStat(i)));
		}
	}
	WriteSave();
}

void UExpeditionComponent::WriteSave()
{
	UYggSaveGame* Save = Cast<UYggSaveGame>(UGameplayStatics::CreateSaveGameObject(UYggSaveGame::StaticClass()));
	Save->CampName = LastCheckpointName;
	Save->Stats = CheckpointStats;
	Save->Items = CheckpointItems;
	Save->CarriedBottles = CheckpointBottles;
	Save->bSummitReached = bSummitReached;
	Save->Deaths = Deaths;
	Save->PlaySeconds = SavedPlaySeconds + GetWorld()->GetTimeSeconds() - StartTime;
	Save->Clues = FoundClues.Array();
	UGameplayStatics::AsyncSaveGameToSlot(Save, UYggSaveGame::SlotName, 0);
}

void UExpeditionComponent::UpdateTrailCheckpoints()
{
	const FVector Here = GetOwner()->GetActorLocation();
	for (const TWeakObjectPtr<ATrailCheckpoint>& Point : TrailCheckpoints)
	{
		if (Point.IsValid() && Point.Get() != LastCheckpoint.Get()
			&& FVector::DistSquared2D(Here, Point->GetActorLocation()) < FMath::Square(Point->Radius))
		{
			SaveCheckpoint(Point.Get(), Point->CheckpointName);
			Say(FText::Format(NSLOCTEXT("Expedition", "TrailCheckpoint", "{0} - checkpoint saved."), FText::FromString(Point->CheckpointName)), 4.f);
			return;
		}
	}
}

void UExpeditionComponent::LoadSavedExpedition()
{
	const UYggSaveGame* Save = Cast<UYggSaveGame>(UGameplayStatics::LoadGameFromSlot(UYggSaveGame::SlotName, 0));
	if (!Save || Save->Version != GetDefault<UYggSaveGame>()->Version)
	{
		return;
	}
	// What was found stays found, even without a checkpoint yet.
	FoundClues = TSet<FName>(Save->Clues);

	AActor* Where = nullptr;
	for (const TWeakObjectPtr<ARouteLandmark>& It : Camps)
	{
		if (It.IsValid() && It->LandmarkName == Save->CampName)
		{
			Where = It.Get();
			LastCamp = It.Get();
		}
	}
	for (const TWeakObjectPtr<ATrailCheckpoint>& It : TrailCheckpoints)
	{
		if (!Where && It.IsValid() && It->CheckpointName == Save->CampName)
		{
			Where = It.Get();
		}
	}
	if (!Where)
	{
		return;
	}

	TeleportNear(Where);
	LastCheckpoint = Where;
	LastCheckpointName = Save->CampName;
	bSummitReached = Save->bSummitReached;
	Deaths = Save->Deaths;
	SavedPlaySeconds = Save->PlaySeconds;
	StartTime = GetWorld()->GetTimeSeconds();
	CheckpointStats = Save->Stats;
	CheckpointItems = Save->Items;
	CheckpointBottles = Save->CarriedBottles;
	if (Survival.IsValid())
	{
		for (int32 i = 0; i < Save->Stats.Num() && i <= int32(ESurvivalStat::Oxygen); ++i)
		{
			Survival->SetStat(ESurvivalStat(i), Save->Stats[i]);
		}
		Survival->SetCarriedBottles(Save->CarriedBottles);
	}
	if (UInventoryComponent* Pack = GetOwner()->FindComponentByClass<UInventoryComponent>())
	{
		Pack->SetItems(Save->Items);
		Pack->SaveCheckpoint();
	}
	Say(FText::Format(NSLOCTEXT("Expedition", "Continue", "Continuing from {0}. (Console: ygg.NewGame for a fresh start.)"),
		FText::FromString(Save->CampName)), 6.f);
}

void UExpeditionComponent::SetReadableClue(AStoryClue* Clue)
{
	ReadableClue = Clue;
	const bool bKnown = FoundClues.Contains(Clue->ClueId);
	Say(FText::Format(bKnown ? NSLOCTEXT("Expedition", "ReadAgain", "[E] Read again: {0}") : NSLOCTEXT("Expedition", "Read", "[E] Read: {0}"),
		Clue->Title), 4.f);
}

void UExpeditionComponent::ClearReadableClue(AStoryClue* Clue)
{
	if (ReadableClue.Get() != Clue)
	{
		return;
	}
	ReadableClue.Reset();
	if (bReading)
	{
		ToggleReading();
	}
}

void UExpeditionComponent::ToggleReading()
{
	if (bReading)
	{
		bReading = false;
		OnReading.Broadcast(FText::GetEmpty(), FText::GetEmpty(), false);
		return;
	}
	const AStoryClue* Clue = ReadableClue.Get();
	if (!Clue || (Survival.IsValid() && Survival->IsDead()))
	{
		return;
	}
	bReading = true;
	OnReading.Broadcast(Clue->Title, Clue->Body, true);
	if (!FoundClues.Contains(Clue->ClueId))
	{
		FoundClues.Add(Clue->ClueId);
		WriteSave();
	}
}

void UExpeditionComponent::TeleportToCamp(ARouteLandmark* Camp)
{
	TeleportNear(Camp);
}

void UExpeditionComponent::TeleportNear(AActor* Where)
{
	ACharacter* Character = Cast<ACharacter>(GetOwner());
	if (!Character || !Where)
	{
		return;
	}
	// Beside the checkpoint, a little above the ground so the capsule drops onto it.
	const FVector Target = Where->GetActorLocation() + FVector(-400.f, 0.f, Character->GetSimpleCollisionHalfHeight() + 150.f);
	Character->TeleportTo(Target, Character->GetActorRotation(), false, true);
	Character->GetCharacterMovement()->StopMovementImmediately();
	CurrentCamp.Reset();
}

void UExpeditionComponent::UpdateScreenEffect(float DeltaTime)
{
	UCameraComponent* Camera = GetOwner()->FindComponentByClass<UCameraComponent>();
	if (!Camera || !Survival.IsValid())
	{
		return;
	}

	const USurvivalConfig* C = GetDefault<USurvivalConfig>();
	const float Oxygen = Survival->GetStatValue(ESurvivalStat::Oxygen);
	const float Target = HypoxiaScreenEffect * FMath::Clamp((C->OxygenFatigueBelow - Oxygen) / FMath::Max(C->OxygenFatigueBelow - C->OxygenDamageBelow, 1.f), 0.f, 1.f);
	DisplayedHypoxia = FMath::FInterpTo(DisplayedHypoxia, Target, DeltaTime, 1.5f);

	FPostProcessSettings& PP = Camera->PostProcessSettings;
	PP.bOverride_VignetteIntensity = true;
	PP.VignetteIntensity = FMath::Lerp(0.4f, 0.9f, DisplayedHypoxia);   // the tunnel in M_PP_Body does the rest
	PP.bOverride_ColorSaturation = true;
	const float Saturation = FMath::Lerp(1.f, 0.55f, DisplayedHypoxia);
	PP.ColorSaturation = FVector4(Saturation, Saturation, Saturation, 1.f);
	Camera->PostProcessBlendWeight = 1.f;

	// ---- The body on the view (2026-11-13, "a death zone you can feel"): a heart that answers thin air, effort, exhaustion
	// and cold; its beat tightens the thin-air tunnel; frost grows on the lens with the cold; the view swims when spent and
	// greys out near collapse. One post-process pass (M_PP_Body, Tools/UI/shaders/pp_body.hlsl), weight 0 while fine.
	const FSurvivalSnapshot Snap = Survival->GetSnapshot();
	const ACharacter* Me = Cast<ACharacter>(GetOwner());
	const float Speed = Me ? float(Me->GetVelocity().Size2D()) : 0.f;
	const float Core = Snap.BodyTemperature;
	const float Effort = Snap.bSprinting ? 55.f : FMath::Clamp(Speed / 230.f, 0.f, 1.f) * 24.f;
	const float HeartTarget = FMath::Clamp(64.f + FMath::Max(0.f, 96.f - Oxygen) * 1.5f + Effort + (Snap.bExhausted ? 18.f : 0.f)
		+ FMath::Clamp((C->NormalTemperature - 1.f - Core) * 6.f, 0.f, 12.f), 48.f, 188.f);
	HeartRate = FMath::FInterpTo(HeartRate, HeartTarget, DeltaTime, HeartTarget > HeartRate ? 0.6f : 0.25f);
	HeartPhase = FMath::Fmod(HeartPhase + HeartRate / 60.f * DeltaTime, 1.f);
	const auto Bump = [](float X, float At, float W) { const float D = (X - At) / W; return FMath::Exp(-D * D); };
	const float Beat = FMath::Max(Bump(HeartPhase, 0.06f, 0.05f), 0.6f * Bump(HeartPhase, 0.26f, 0.05f));   // lub-dub

	// cold: the core (35.8 -> nothing, 33.2 -> full) and, out in killing air, the lens frosting at its edges
	float AirFrost = 0.f;
	if (const ARealmSky* Sky = ARealmSky::Get(GetWorld()))
	{
		AirFrost = FMath::Clamp((-40.f - Sky->GetAirTemperature(Snap.ElevationMeters)) / 60.f, 0.f, 0.45f);
	}
	const float FrostTarget = Snap.bSheltered ? 0.f : FMath::Max(FMath::Clamp((C->NormalTemperature - 0.8f - Core) / 2.6f, 0.f, 1.f), AirFrost);
	DisplayedFrost = FMath::FInterpTo(DisplayedFrost, FrostTarget, DeltaTime, FrostTarget > DisplayedFrost ? 0.25f : 0.5f);
	const float Stam = Survival->GetStatNormalized(ESurvivalStat::Stamina);
	const float ExTarget = Snap.bExhausted ? 1.f : FMath::Clamp((0.18f - Stam) / 0.18f, 0.f, 0.5f);
	DisplayedExhaust = FMath::FInterpTo(DisplayedExhaust, ExTarget, DeltaTime, 1.5f);
	DisplayedFaint = FMath::FInterpTo(DisplayedFaint, FMath::Clamp((C->OxygenDamageBelow - Oxygen) / 10.f, 0.f, 1.f), DeltaTime, 0.8f);

	// test switches (ygg.Body.*, -1 = off) hold a state for screenshots
	const auto Held = [](const TCHAR* Name, float Live) { const IConsoleVariable* V = IConsoleManager::Get().FindConsoleVariable(Name); return V && V->GetFloat() >= 0.f ? V->GetFloat() : Live; };
	float Hyp = Held(TEXT("ygg.Body.Hypoxia"), DisplayedHypoxia), Frost = Held(TEXT("ygg.Body.Frost"), DisplayedFrost);
	float Ex = Held(TEXT("ygg.Body.Exhaust"), DisplayedExhaust), Faint = Held(TEXT("ygg.Body.Faint"), DisplayedFaint);
	float Shiver = Held(TEXT("ygg.Body.Shiver"), FMath::Clamp((C->NormalTemperature - 1.2f - Core) / 2.2f, 0.f, 1.f));
	// Settings > Game > Body effects: Full, Reduced (half strength, no shaking or swaying), Off (the HUD still warns)
	const int32 BodyFx = YggSettings::GetInt(TEXT("BodyFx"), 0);
	const float FxScale = BodyFx == 2 ? 0.f : BodyFx == 1 ? 0.5f : 1.f;
	Hyp *= FxScale; Frost *= FxScale; Ex *= FxScale; Faint *= FxScale;
	if (BodyFx != 0) { Shiver = 0.f; }
	ShownHypoxia = Hyp; ShownFrost = Frost; ShownExhaust = Ex; ShownFaint = Faint;
	const float HeartShown = FMath::Max(Hyp, Ex * 0.7f);

	if (!BodyMID)
	{
		if (UMaterialInterface* M = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/UI/Materials/M_PP_Body.M_PP_Body"), nullptr, LOAD_NoWarn | LOAD_Quiet))
		{
			BodyMID = UMaterialInstanceDynamic::Create(M, this);
		}
	}
	if (BodyMID)
	{
		FVector2D View(16.f, 9.f);
		if (GEngine && GEngine->GameViewport) { GEngine->GameViewport->GetViewportSize(View); }
		BodyMID->SetScalarParameterValue(TEXT("T"), GetWorld()->GetTimeSeconds());
		BodyMID->SetScalarParameterValue(TEXT("Aspect"), View.Y > 0.f ? float(View.X / View.Y) : 1.7778f);
		BodyMID->SetScalarParameterValue(TEXT("Hyp"), Hyp);
		BodyMID->SetScalarParameterValue(TEXT("Beat"), Beat * HeartShown);
		BodyMID->SetScalarParameterValue(TEXT("Frost"), Frost);
		BodyMID->SetScalarParameterValue(TEXT("Exhaust"), Ex);
		BodyMID->SetScalarParameterValue(TEXT("Faint"), Faint);
		// weight 0 = the pass is not drawn: nothing is spent while the body is fine
		const float Weight = FMath::Max(FMath::Max(Hyp, Frost), FMath::Max(Ex, Faint)) > 0.01f ? 1.f : 0.f;
		FWeightedBlendable* Entry = PP.WeightedBlendables.Array.FindByPredicate([this](const FWeightedBlendable& B) { return B.Object == BodyMID; });
		if (Entry) { Entry->Weight = Weight; }
		else { PP.WeightedBlendables.Array.Add(FWeightedBlendable(Weight, BodyMID)); }
	}
	if (!BodyCam.IsValid())
	{
		const APawn* Pawn = Cast<APawn>(GetOwner());
		if (APlayerController* PC = Pawn ? Cast<APlayerController>(Pawn->GetController()) : nullptr)
		{
			if (PC->PlayerCameraManager) { BodyCam = Cast<UYggBodyCameraModifier>(PC->PlayerCameraManager->AddNewCameraModifier(UYggBodyCameraModifier::StaticClass())); }
		}
	}
	if (BodyCam.IsValid())
	{
		BodyCam->Shiver = Shiver;
		BodyCam->Sway = BodyFx == 0 ? Ex * 0.8f : 0.f;
		BodyCam->Pulse = Beat * HeartShown;
	}

	// the death zone: said once on the way up, again after going back below 7,850 m
	if (!bDeathZoneSaid && Snap.ElevationMeters >= 8000.f)
	{
		bDeathZoneSaid = true;
		Say(NSLOCTEXT("Expedition", "DeathZone", "8,000 metres. The death zone: up here the body is slowly dying."), 8.f);
	}
	else if (bDeathZoneSaid && Snap.ElevationMeters < 7850.f)
	{
		bDeathZoneSaid = false;
	}
}

void UExpeditionComponent::Say(const FText& Message, float Seconds)
{
	UE_LOG(LogSurvival, Log, TEXT("Expedition: %s"), *Message.ToString());
	OnMessage.Broadcast(Message, Seconds);
}

// ---- Console: jump between camps for testing ----

namespace
{
	FAutoConsoleCommandWithWorldAndArgs GExpeditionGoCommand(
		TEXT("ygg.Expedition.Go"),
		TEXT("ygg.Expedition.Go <index> - teleport to a camp, 0 = lowest (Dingboche) ... 8 = summit."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			const APlayerController* Controller = World ? World->GetFirstPlayerController() : nullptr;
			const APawn* Pawn = Controller ? Controller->GetPawn() : nullptr;
			UExpeditionComponent* Expedition = Pawn ? Pawn->FindComponentByClass<UExpeditionComponent>() : nullptr;
			if (!Expedition || Args.Num() < 1)
			{
				UE_LOG(LogSurvival, Warning, TEXT("Usage: ygg.Expedition.Go <index> (needs an ExpeditionComponent on the player)"));
				return;
			}
			const int32 Index = FCString::Atoi(*Args[0]);
			const TArray<TWeakObjectPtr<ARouteLandmark>>& Camps = Expedition->GetCamps();
			if (Camps.IsValidIndex(Index) && Camps[Index].IsValid())
			{
				Expedition->TeleportToCamp(Camps[Index].Get());
			}
		}));
}

void UExpeditionComponent::Rest()
{
	ACharacter* Character = Cast<ACharacter>(GetOwner());
	APlayerController* PC = Character ? Cast<APlayerController>(Character->GetController()) : nullptr;
	if (!Character || !Survival.IsValid() || bResting || Survival->IsDead())
	{
		return;
	}
	if (!Survival->IsSheltered())
	{
		Say(NSLOCTEXT("Expedition", "NoRest", "Nowhere to rest out here. Find a camp."), 3.f);
		return;
	}
	bResting = true;
	Character->GetCharacterMovement()->DisableMovement();
	if (PC && PC->PlayerCameraManager)
	{
		PC->PlayerCameraManager->StartCameraFade(0.f, 1.f, 1.5f, FLinearColor::Black, false, true);
	}
	GetWorld()->GetTimerManager().SetTimer(RestTimer, this, &UExpeditionComponent::FinishRest, 2.f);
}

void UExpeditionComponent::FinishRest()
{
	ACharacter* Character = Cast<ACharacter>(GetOwner());
	APlayerController* PC = Character ? Cast<APlayerController>(Character->GetController()) : nullptr;
	bResting = false;
	if (!Character || !Survival.IsValid())
	{
		return;
	}
	// afternoon or night: sleep until the cold early hours (the Icefall is safest then); otherwise a short rest
	const float Now = YggTime::Get(GetWorld());
	const bool bSleep = Now >= 1500.f || Now < 400.f;
	const float Wake = bSleep ? 450.f : Now + 300.f;
	const float Hours = FMath::Fmod(Wake - Now + 2400.f, 2400.f) / 100.f;
	YggTime::Set(GetWorld(), Wake);
	const USurvivalConfig* C = GetDefault<USurvivalConfig>();
	Survival->SetStat(ESurvivalStat::Stamina, Survival->GetStatMax(ESurvivalStat::Stamina));
	Survival->SetStat(ESurvivalStat::BodyTemperature, C->NormalTemperature);
	Survival->SetStat(ESurvivalStat::Satiety, FMath::Max(5.f, Survival->GetStatValue(ESurvivalStat::Satiety) - 2.5f * Hours));
	Survival->SetStat(ESurvivalStat::Hydration, FMath::Max(5.f, Survival->GetStatValue(ESurvivalStat::Hydration) - 3.f * Hours));
	Survival->SetStat(ESurvivalStat::Health, FMath::Min(Survival->GetStatMax(ESurvivalStat::Health), Survival->GetStatValue(ESurvivalStat::Health) + 5.f * Hours));
	if (USanityComponent* Mind = GetOwner()->FindComponentByClass<USanityComponent>())
	{
		Mind->SetSanity(Mind->GetSanity() + 6.f * Hours);
	}
	Character->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	if (PC && PC->PlayerCameraManager)
	{
		PC->PlayerCameraManager->StartCameraFade(1.f, 0.f, 2.5f, FLinearColor::Black, false, false);
	}
	WriteSave();
	Say(bSleep ? FText::Format(NSLOCTEXT("Expedition", "Slept", "You sleep badly, the tent cracking in the wind. {0} - cold, dark, the ice at its quietest. Time to go."), FText::FromString(YggTime::Clock(Wake)))
		: FText::Format(NSLOCTEXT("Expedition", "Rested", "You rest a few hours. {0}."), FText::FromString(YggTime::Clock(Wake))), 6.f);
}
