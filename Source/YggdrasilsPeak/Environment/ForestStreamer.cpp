// Copyright Epic Games, Inc. All Rights Reserved.

#include "Environment/ForestStreamer.h"

#include "Components/InstancedSkinnedMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"

AForestStreamer::AForestStreamer()
{
	PrimaryActorTick.bCanEverTick = true;
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
}

void AForestStreamer::BeginPlay()
{
	Super::BeginPlay();
	Area = FBox2D(ForceInit);
	for (const FForestSet& S : Sets)
	{
		for (const FTransform& T : S.Instances)
		{
			Area += FVector2D(T.GetLocation());
		}
	}
	int32 Count = 0;
	for (const FForestSet& S : Sets) { Count += S.Instances.Num(); }
	UE_LOG(LogTemp, Display, TEXT("ForestStreamer: %d trees in %d sets, area %.1f x %.1f km"), Count, Sets.Num(), Area.GetSize().X / 100000.f, Area.GetSize().Y / 100000.f);
}

void AForestStreamer::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Unload();
	Super::EndPlay(EndPlayReason);
}

float AForestStreamer::DistanceToForest(const FVector& Where) const
{
	if (!Area.bIsValid)
	{
		return BIG_NUMBER;
	}
	const FVector2D P(Where);
	const FVector2D C(FMath::Clamp(P.X, Area.Min.X, Area.Max.X), FMath::Clamp(P.Y, Area.Min.Y, Area.Max.Y));
	return float(FVector2D::Distance(P, C));
}

void AForestStreamer::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if ((CheckIn -= DeltaSeconds) > 0.f)
	{
		return;
	}
	CheckIn = 1.f;
	const APawn* Pawn = UGameplayStatics::GetPlayerPawn(this, 0);
	if (!Pawn)
	{
		return;
	}
	const float D = DistanceToForest(Pawn->GetActorLocation());
	if (!bWanted && D < LoadWithinCm) { Load(); }
	else if (bWanted && D > UnloadBeyondCm) { Unload(); }
}

void AForestStreamer::Load()
{
	bWanted = true;
	TArray<FSoftObjectPath> Paths;
	for (const FForestSet& S : Sets)
	{
		if (!S.Mesh.IsNull()) { Paths.AddUnique(S.Mesh.ToSoftObjectPath()); }
		if (!S.FoliageMaterial.IsNull()) { Paths.AddUnique(S.FoliageMaterial.ToSoftObjectPath()); }
	}
	UE_LOG(LogTemp, Display, TEXT("ForestStreamer: loading %d assets"), Paths.Num());
	Handle = Streamable.RequestAsyncLoad(Paths, FStreamableDelegate::CreateUObject(this, &AForestStreamer::OnLoaded));
}

void AForestStreamer::OnLoaded()
{
	if (!bWanted || Spawned.Num() > 0)
	{
		return;
	}
	for (const FForestSet& S : Sets)
	{
		USkeletalMesh* Mesh = S.Mesh.Get();
		if (!Mesh || S.Instances.Num() == 0)
		{
			continue;
		}
		UInstancedSkinnedMeshComponent* C = NewObject<UInstancedSkinnedMeshComponent>(this);
		C->SetupAttachment(GetRootComponent());
		C->SetSkinnedAssetAndUpdate(Mesh, true);
		C->SetCastShadow(true);
		C->bVisibleInRayTracing = false;
		C->SetCullDistances(int32(CullDistanceCm * 0.8f), int32(CullDistanceCm));
		C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		C->RegisterComponent();
		if (UMaterialInterface* Gold = S.FoliageMaterial.Get())
		{
			const TArray<FSkeletalMaterial>& Mats = Mesh->GetMaterials();
			for (int32 i = 0; i < Mats.Num(); ++i)
			{
				if (Mats[i].MaterialInterface && Mats[i].MaterialInterface->GetName().Contains(TEXT("Foliage"))) { C->SetMaterial(i, Gold); }
			}
		}
		TArray<int32> Anim;
		Anim.Init(0, S.Instances.Num());
		C->AddInstances(S.Instances, Anim, false, true);
		Spawned.Add(C);
	}
	UE_LOG(LogTemp, Display, TEXT("ForestStreamer: forest in (%d components)"), Spawned.Num());
}

void AForestStreamer::Unload()
{
	bWanted = false;
	for (UInstancedSkinnedMeshComponent* C : Spawned)
	{
		if (C) { C->DestroyComponent(); }
	}
	const bool bHad = Spawned.Num() > 0;
	Spawned.Reset();
	if (Handle.IsValid())
	{
		Handle->ReleaseHandle();
		Handle.Reset();
	}
	if (bHad)
	{
		UE_LOG(LogTemp, Display, TEXT("ForestStreamer: forest out, meshes released"));
	}
}
