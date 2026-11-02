// Copyright Epic Games, Inc. All Rights Reserved.

#include "Environment/TrailCheckpoint.h"

#include "Components/StaticMeshComponent.h"

ATrailCheckpoint::ATrailCheckpoint()
{
	PrimaryActorTick.bCanEverTick = false;
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
	Marker = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Marker"));
	Marker->SetupAttachment(GetRootComponent());
	Marker->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}
