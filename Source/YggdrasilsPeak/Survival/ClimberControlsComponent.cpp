// Copyright Epic Games, Inc. All Rights Reserved.

#include "Survival/ClimberControlsComponent.h"

#include "Survival/YggSettings.h"
#include "Camera/PlayerCameraManager.h"
#include "Survival/BackpackWidget.h"
#include "Survival/SettingsWidget.h"
#include "Survival/YggMenuWidget.h"
#include "Survival/ExpeditionComponent.h"
#include "Survival/InventoryComponent.h"
#include "Survival/SurvivalComponent.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "InputAction.h"
#include "InputMappingContext.h"

UClimberControlsComponent::UClimberControlsComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	HUDClass = TSoftClassPtr<UUserWidget>(FSoftObjectPath(TEXT("/Game/UI/HUD/WBP_GameHUD.WBP_GameHUD_C")));
}

void UClimberControlsComponent::BeginPlay()
{
	Super::BeginPlay();

	const APawn* Pawn = Cast<APawn>(GetOwner());
	if (!Pawn || !Pawn->IsLocallyControlled())
	{
		return;
	}
	if (!SprintAction)
	{
		SprintAction = LoadObject<UInputAction>(nullptr, TEXT("/Game/Survival_Character/Demo/ThirdPerson/Input/Actions/IA_Sprint.IA_Sprint"));
	}
	CreateHUD();
	if (!TryBindInput())
	{
		SetComponentTickEnabled(true);
	}
}

void UClimberControlsComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (TryBindInput())
	{
		SetComponentTickEnabled(false);
	}
}

bool UClimberControlsComponent::TryBindInput()
{
	const APawn* Pawn = Cast<APawn>(GetOwner());
	const APlayerController* PC = Pawn ? Cast<APlayerController>(Pawn->GetController()) : nullptr;
	UEnhancedInputComponent* Input = Pawn ? Cast<UEnhancedInputComponent>(Pawn->InputComponent) : nullptr;
	if (!PC || !Input || !SprintAction)
	{
		return false;
	}

	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
	{
		SprintMapping = NewObject<UInputMappingContext>(this);
		SprintMapping->MapKey(SprintAction, YggSettings::KeyFor(TEXT("Sprint"), EKeys::LeftShift)); YggSettings::RegisterKey(TEXT("Sprint"), INVTEXT("Sprint"), EKeys::LeftShift, SprintMapping, SprintAction);
		SprintMapping->MapKey(SprintAction, EKeys::Gamepad_LeftThumbstick);
		EatAction = NewObject<UInputAction>(this, TEXT("IA_Eat"));
		DrinkAction = NewObject<UInputAction>(this, TEXT("IA_Drink"));
		SprintMapping->MapKey(EatAction, YggSettings::KeyFor(TEXT("Eat"), EKeys::One)); YggSettings::RegisterKey(TEXT("Eat"), INVTEXT("Eat"), EKeys::One, SprintMapping, EatAction);
		SprintMapping->MapKey(EatAction, EKeys::Gamepad_DPad_Left);
		SprintMapping->MapKey(DrinkAction, YggSettings::KeyFor(TEXT("Drink"), EKeys::Two)); YggSettings::RegisterKey(TEXT("Drink"), INVTEXT("Drink"), EKeys::Two, SprintMapping, DrinkAction);
		SprintMapping->MapKey(DrinkAction, EKeys::Gamepad_DPad_Right);
		InteractAction = NewObject<UInputAction>(this, TEXT("IA_Interact"));
		SprintMapping->MapKey(InteractAction, YggSettings::KeyFor(TEXT("Interact"), EKeys::E)); YggSettings::RegisterKey(TEXT("Interact"), INVTEXT("Read / use"), EKeys::E, SprintMapping, InteractAction);
		SprintMapping->MapKey(InteractAction, EKeys::Gamepad_FaceButton_Left);
		RestAction = NewObject<UInputAction>(this, TEXT("IA_Rest"));
		SprintMapping->MapKey(RestAction, YggSettings::KeyFor(TEXT("Rest"), EKeys::R)); YggSettings::RegisterKey(TEXT("Rest"), INVTEXT("Rest at camp"), EKeys::R, SprintMapping, RestAction);
		SprintMapping->MapKey(RestAction, EKeys::Gamepad_DPad_Down);
		Input->BindAction(RestAction, ETriggerEvent::Started, this, &UClimberControlsComponent::OnRest);
		PackAction = NewObject<UInputAction>(this, TEXT("IA_Pack"));
		SprintMapping->MapKey(PackAction, YggSettings::KeyFor(TEXT("Backpack"), EKeys::Tab)); YggSettings::RegisterKey(TEXT("Backpack"), INVTEXT("Backpack"), EKeys::Tab, SprintMapping, PackAction);
		SprintMapping->MapKey(PackAction, EKeys::Gamepad_Special_Left);
		Subsystem->AddMappingContext(SprintMapping, 1);
		Input->BindAction(PackAction, ETriggerEvent::Started, this, &UClimberControlsComponent::OnPack);
		SettingsAction = NewObject<UInputAction>(this, TEXT("IA_Settings"));
		SprintMapping->MapKey(SettingsAction, EKeys::F2);
		Input->BindAction(SettingsAction, ETriggerEvent::Started, this, &UClimberControlsComponent::OnSettings);
		YggSettings::ApplyAll(GetWorld());
		// whatever input mode a menu left on the viewport (the start menu's UI-only one survives the level change),
		// the climb starts with the game's own input (to-do #64)
		if (APlayerController* MutablePC = const_cast<APlayerController*>(PC))
		{
			MutablePC->SetInputMode(FInputModeGameOnly());
			MutablePC->SetShowMouseCursor(false);
		}
		// arriving from the start menu's loading screen: the mountain comes up out of black
		if (GetWorld()->WorldType == EWorldType::Game && PC->PlayerCameraManager) { PC->PlayerCameraManager->StartCameraFade(1.f, 0.f, 2.5f, FLinearColor::Black, false, false); }
		// pause menu: Esc in a packaged game (the editor keeps Esc to stop Play), P everywhere, Start on a pad
		PauseAction = NewObject<UInputAction>(this, TEXT("IA_Pause"));
		SprintMapping->MapKey(PauseAction, EKeys::Escape);
		SprintMapping->MapKey(PauseAction, YggSettings::KeyFor(TEXT("Pause"), EKeys::P)); YggSettings::RegisterKey(TEXT("Pause"), INVTEXT("Pause"), EKeys::P, SprintMapping, PauseAction);
		SprintMapping->MapKey(PauseAction, EKeys::Gamepad_Special_Right);
		Input->BindAction(PauseAction, ETriggerEvent::Started, this, &UClimberControlsComponent::OnPauseMenu);
		Input->BindAction(EatAction, ETriggerEvent::Started, this, &UClimberControlsComponent::OnEat);
		Input->BindAction(DrinkAction, ETriggerEvent::Started, this, &UClimberControlsComponent::OnDrink);
		Input->BindAction(InteractAction, ETriggerEvent::Started, this, &UClimberControlsComponent::OnInteract);
	}
	Input->BindAction(SprintAction, ETriggerEvent::Started, this, &UClimberControlsComponent::OnSprintStarted);
	Input->BindAction(SprintAction, ETriggerEvent::Completed, this, &UClimberControlsComponent::OnSprintStopped);
	Input->BindAction(SprintAction, ETriggerEvent::Canceled, this, &UClimberControlsComponent::OnSprintStopped);
	return true;
}

void UClimberControlsComponent::CreateHUD()
{
	APlayerController* PC = Cast<APlayerController>(Cast<APawn>(GetOwner())->GetController());
	UClass* WidgetClass = HUDClass.LoadSynchronous();
	if (!PC || !WidgetClass)
	{
		return;
	}
	// After a respawn the old HUD is still on screen.
	TArray<UUserWidget*> Existing;
	UWidgetBlueprintLibrary::GetAllWidgetsOfClass(this, Existing, WidgetClass, false);
	if (Existing.Num() == 0)
	{
		if (UUserWidget* Widget = CreateWidget<UUserWidget>(PC, WidgetClass))
		{
			Widget->AddToViewport();
		}
	}
}

void UClimberControlsComponent::OnSprintStarted()
{
	if (USurvivalComponent* Survival = GetOwner()->FindComponentByClass<USurvivalComponent>())
	{
		Survival->SetSprintRequested(true);
	}
}

void UClimberControlsComponent::OnSprintStopped()
{
	if (USurvivalComponent* Survival = GetOwner()->FindComponentByClass<USurvivalComponent>())
	{
		Survival->SetSprintRequested(false);
	}
}

void UClimberControlsComponent::OnEat()
{
	if (UInventoryComponent* Pack = GetOwner()->FindComponentByClass<UInventoryComponent>())
	{
		Pack->Use(TEXT("Ration"));
	}
}

void UClimberControlsComponent::OnDrink()
{
	if (UInventoryComponent* Pack = GetOwner()->FindComponentByClass<UInventoryComponent>())
	{
		Pack->Use(TEXT("Water"));
	}
}

void UClimberControlsComponent::OnRest()
{
	if (UExpeditionComponent* Expedition = GetOwner()->FindComponentByClass<UExpeditionComponent>())
	{
		Expedition->Rest();
	}
}

void UClimberControlsComponent::OnPack()
{
	UBackpackWidget::Toggle(Cast<APlayerController>(Cast<APawn>(GetOwner())->GetController()));
}

void UClimberControlsComponent::OnPauseMenu()
{
	APlayerController* PC = Cast<APlayerController>(Cast<APawn>(GetOwner())->GetController());
	TArray<UUserWidget*> Packs;
	UWidgetBlueprintLibrary::GetAllWidgetsOfClass(PC, Packs, UBackpackWidget::StaticClass(), true);
	if (Packs.Num() > 0) { return; }   // Esc closes the pack first
	UYggMenuWidget::TogglePause(PC);
}

void UClimberControlsComponent::OnSettings()
{
	APlayerController* PC = Cast<APlayerController>(Cast<APawn>(GetOwner())->GetController());
	if (UYggMenuWidget::IsOpen(PC)) { return; }   // a menu on screen (death, summit) opens its own settings
	USettingsWidget::Toggle(PC);
}

void UClimberControlsComponent::OnInteract()
{
	if (UExpeditionComponent* Expedition = GetOwner()->FindComponentByClass<UExpeditionComponent>())
	{
		Expedition->ToggleReading();
	}
}
