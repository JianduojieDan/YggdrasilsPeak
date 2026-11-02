// Copyright Epic Games, Inc. All Rights Reserved.

#include "Survival/ClimberControlsComponent.h"

#include "Survival/BackpackWidget.h"
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
		SprintMapping->MapKey(SprintAction, EKeys::LeftShift);
		SprintMapping->MapKey(SprintAction, EKeys::Gamepad_LeftThumbstick);
		EatAction = NewObject<UInputAction>(this, TEXT("IA_Eat"));
		DrinkAction = NewObject<UInputAction>(this, TEXT("IA_Drink"));
		SprintMapping->MapKey(EatAction, EKeys::One);
		SprintMapping->MapKey(EatAction, EKeys::Gamepad_DPad_Left);
		SprintMapping->MapKey(DrinkAction, EKeys::Two);
		SprintMapping->MapKey(DrinkAction, EKeys::Gamepad_DPad_Right);
		InteractAction = NewObject<UInputAction>(this, TEXT("IA_Interact"));
		SprintMapping->MapKey(InteractAction, EKeys::E);
		SprintMapping->MapKey(InteractAction, EKeys::Gamepad_FaceButton_Left);
		RestAction = NewObject<UInputAction>(this, TEXT("IA_Rest"));
		SprintMapping->MapKey(RestAction, EKeys::R);
		SprintMapping->MapKey(RestAction, EKeys::Gamepad_DPad_Down);
		Input->BindAction(RestAction, ETriggerEvent::Started, this, &UClimberControlsComponent::OnRest);
		PackAction = NewObject<UInputAction>(this, TEXT("IA_Pack"));
		SprintMapping->MapKey(PackAction, EKeys::Tab);
		SprintMapping->MapKey(PackAction, EKeys::Gamepad_Special_Left);
		Subsystem->AddMappingContext(SprintMapping, 1);
		Input->BindAction(PackAction, ETriggerEvent::Started, this, &UClimberControlsComponent::OnPack);
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

void UClimberControlsComponent::OnInteract()
{
	if (UExpeditionComponent* Expedition = GetOwner()->FindComponentByClass<UExpeditionComponent>())
	{
		Expedition->ToggleReading();
	}
}
