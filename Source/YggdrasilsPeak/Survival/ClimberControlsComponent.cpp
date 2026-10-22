// Copyright Epic Games, Inc. All Rights Reserved.

#include "Survival/ClimberControlsComponent.h"

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
		Subsystem->AddMappingContext(SprintMapping, 1);
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
