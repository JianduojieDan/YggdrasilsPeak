// Copyright Epic Games, Inc. All Rights Reserved.

#include "Environment/SnowDriftField.h"

#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

ASnowDriftField::ASnowDriftField()
{
	PrimaryActorTick.bCanEverTick = false;

	Drifts = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("Drifts"));
	RootComponent = Drifts;
	Drifts->SetMobility(EComponentMobility::Static);
	Drifts->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	// Small mounds: not worth drawing (or shadowing) beyond a few hundred metres.
	Drifts->SetCullDistances(0, 40000);
	Drifts->bAffectDistanceFieldLighting = false;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> Pile(TEXT("/Game/Fab/Megascans/3D/Snow_Pile_xgxhcjxqx/Raw/xgxhcjxqx_tier_0/StaticMeshes/xgxhcjxqx_tier_0.xgxhcjxqx_tier_0"));
	Drifts->SetStaticMesh(Pile.Object);
}
