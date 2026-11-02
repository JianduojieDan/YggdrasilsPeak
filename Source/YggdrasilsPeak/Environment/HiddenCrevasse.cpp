// Copyright Epic Games, Inc. All Rights Reserved.

#include "Environment/HiddenCrevasse.h"

#include "Survival/ExpeditionComponent.h"
#include "Survival/SanityComponent.h"
#include "Survival/SurvivalComponent.h"
#include "Components/BoxComponent.h"
#include "Components/DecalComponent.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "Sound/SoundBase.h"

AHiddenCrevasse::AHiddenCrevasse()
{
	PrimaryActorTick.bCanEverTick = false;
	Bridge = CreateDefaultSubobject<UBoxComponent>(TEXT("Bridge"));
	SetRootComponent(Bridge);
	Bridge->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	Sag = CreateDefaultSubobject<UDecalComponent>(TEXT("Sag"));
	Sag->SetupAttachment(Bridge);
	Sag->SetRelativeRotation(FRotator(-90.f, 0.f, 0.f));     // project straight down
	SetSize(2500.f, 260.f);
}

void AHiddenCrevasse::SetSize(float Length, float Width)
{
	Bridge->SetBoxExtent(FVector(Width * 0.5f, Length * 0.5f, 120.f));
	// decal box: X = projection depth, Y/Z = the footprint (rotated to point down)
	Sag->DecalSize = FVector(250.f, Length * 0.5f, Width * 0.5f + 60.f);
}

void AHiddenCrevasse::BeginPlay()
{
	Super::BeginPlay();
	if (!Sag->GetDecalMaterial())
	{
		Sag->SetDecalMaterial(LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Environment/Glacier/Crevasse/M_HiddenCrevasseSag.M_HiddenCrevasseSag")));
	}
	if (!CollapseSound)
	{
		CollapseSound = LoadObject<USoundBase>(nullptr, TEXT("/Game/Audio/Ice/Ice_Crack_03.Ice_Crack_03"));
	}
	Bridge->OnComponentBeginOverlap.AddDynamic(this, &AHiddenCrevasse::OnStep);
}

void AHiddenCrevasse::OnStep(UPrimitiveComponent*, AActor* Other, UPrimitiveComponent*, int32, bool, const FHitResult&)
{
	APawn* Pawn = Cast<APawn>(Other);
	if (!Pawn || !Pawn->IsPlayerControlled())
	{
		return;
	}
	UExpeditionComponent* Expedition = Pawn->FindComponentByClass<UExpeditionComponent>();
	if (Expedition && Expedition->IsOnRope())
	{
		if (!bWarnedRope)
		{
			bWarnedRope = true;
			Expedition->ShowMessage(NSLOCTEXT("Crevasse", "Sags", "The snow sags under your boots - a hidden crevasse. The rope holds you."), 4.f);
		}
		return;
	}
	if (CollapseSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, CollapseSound, Pawn->GetActorLocation(), 1.4f);
	}
	if (USanityComponent* Mind = Pawn->FindComponentByClass<USanityComponent>())
	{
		Mind->Shock(10.f);
	}
	if (USurvivalComponent* Survival = Pawn->FindComponentByClass<USurvivalComponent>())
	{
		Survival->KillBy(NSLOCTEXT("Crevasse", "Bridge", "The snow bridge gave way. You were off the rope."));
	}
}
