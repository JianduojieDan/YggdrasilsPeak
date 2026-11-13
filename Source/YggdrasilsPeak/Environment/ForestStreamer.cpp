// Copyright Epic Games, Inc. All Rights Reserved.

#include "Environment/ForestStreamer.h"

#include "Components/InstancedSkinnedMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"

static TAutoConsoleVariable<int32> CVarForest(TEXT("ygg.Forest"), 1, TEXT("0 = no trees (measuring what the forest costs), 1 = trees."));
static TAutoConsoleVariable<float> CVarForestAnimM(TEXT("ygg.Forest.AnimM"), -1.f, TEXT("Trees sway within this many metres (-1 = the actor's AnimateWithinCm, 0 = all still)."));
static TAutoConsoleVariable<float> CVarForestKm(TEXT("ygg.Forest.Km"), 0.f, TEXT("How far trees are drawn, km (0 = by Settings > View distance: 2.5 / 4 / 6 / 8 km)."));

namespace
{
	// 2026-11-13: 38k trees along the route cost ~2.3 ms of CPU at Dingboche with every chunk within 8.5 km shown; the
	// distance now follows the View distance setting (sg.ViewDistanceQuality)
	float ForestReachCm()
	{
		if (const float Km = CVarForestKm.GetValueOnGameThread(); Km > 0.f) { return Km * 100000.f; }
		static const float ByLevel[] = { 250000.f, 400000.f, 600000.f, 800000.f };
		const IConsoleVariable* V = IConsoleManager::Get().FindConsoleVariable(TEXT("sg.ViewDistanceQuality"));
		return ByLevel[FMath::Clamp(V ? V->GetInt() : 2, 0, 3)];
	}
}

AForestStreamer::AForestStreamer()
{
	PrimaryActorTick.bCanEverTick = true;
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
}

void AForestStreamer::BeginPlay()
{
	Super::BeginPlay();
	Chunks.Reset();
	TMap<FIntVector, int32> Index;           // (cell x, cell y, set) -> chunk
	int32 Count = 0;
	for (int32 S = 0; S < Sets.Num(); ++S)
	{
		for (const FTransform& T : Sets[S].Instances)
		{
			const FVector L = T.GetLocation();
			const FIntVector Key(FMath::FloorToInt(L.X / ChunkCm), FMath::FloorToInt(L.Y / ChunkCm), S);
			int32* Found = Index.Find(Key);
			if (!Found)
			{
				FChunk& C = Chunks.AddDefaulted_GetRef();
				C.Set = S;
				Found = &Index.Add(Key, Chunks.Num() - 1);
			}
			FChunk& C = Chunks[*Found];
			C.Instances.Add(T);
			C.Box += FVector2D(L);
			++Count;
		}
	}
	UE_LOG(LogTemp, Display, TEXT("ForestStreamer: %d trees in %d sets, %d chunks of %.1f km"), Count, Sets.Num(), Chunks.Num(), ChunkCm / 100000.f);
}

void AForestStreamer::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Unload();
	Super::EndPlay(EndPlayReason);
}

float AForestStreamer::DistanceToBox(const FBox2D& Box, const FVector& Where)
{
	if (!Box.bIsValid)
	{
		return BIG_NUMBER;
	}
	const FVector2D P(Where);
	const FVector2D C(FMath::Clamp(P.X, Box.Min.X, Box.Max.X), FMath::Clamp(P.Y, Box.Min.Y, Box.Max.Y));
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
	if (CVarForest.GetValueOnGameThread() == 0)
	{
		if (bWanted) { Unload(); }
		return;
	}
	const APawn* Pawn = UGameplayStatics::GetPlayerPawn(this, 0);
	if (!Pawn)
	{
		return;
	}
	const FVector Where = Pawn->GetActorLocation();
	float Nearest = BIG_NUMBER;
	for (const FChunk& C : Chunks)
	{
		Nearest = FMath::Min(Nearest, DistanceToBox(C.Box, Where));
	}
	if (!bWanted && Nearest < LoadWithinCm) { Load(); }
	else if (bWanted && Nearest > UnloadBeyondCm) { Unload(); return; }
	if (!bReady)
	{
		return;
	}
	const float Reach = ForestReachCm();
	if (!FMath::IsNearlyEqual(Reach, CullDistanceCm))
	{
		CullDistanceCm = Reach;
		for (UInstancedSkinnedMeshComponent* C : Spawned) { if (C) { C->SetCullDistances(int32(Reach * 0.8f), int32(Reach)); } }
	}
	for (FChunk& C : Chunks)
	{
		const float D = DistanceToBox(C.Box, Where);
		if (C.Component == INDEX_NONE && D < Reach + 50000.f) { ShowChunk(C); }
		else if (C.Component != INDEX_NONE && D > Reach + 200000.f) { HideChunk(C); }
		// the wind only moves the trees near the climber: a still tree at 2 km looks the same and costs nothing to animate
		if (C.Component != INDEX_NONE && Spawned.IsValidIndex(C.Component) && Spawned[C.Component])
		{
			// measured 2026-11-13: the sway WAS the whole cost of the forest (~4 ms of game thread at Dingboche: 93 fps vs
			// 120 fps still, 119 without trees) - so the trees stand still unless Effects is Epic
			const float AnimM = CVarForestAnimM.GetValueOnGameThread();
			const IConsoleVariable* Fx = IConsoleManager::Get().FindConsoleVariable(TEXT("sg.EffectsQuality"));
			const float Within = AnimM >= 0.f ? AnimM * 100.f : ((Fx && Fx->GetInt() >= 3) ? AnimateWithinCm : 0.f);
			Spawned[C.Component]->SetComponentTickEnabled(D < Within);
		}
	}
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
	if (!bWanted)
	{
		return;
	}
	bReady = true;
	CheckIn = 0.f;                            // place the near chunks on the next tick
	UE_LOG(LogTemp, Display, TEXT("ForestStreamer: tree meshes in"));
}

void AForestStreamer::ShowChunk(FChunk& Chunk)
{
	const FForestSet& S = Sets[Chunk.Set];
	USkeletalMesh* Mesh = S.Mesh.Get();
	if (!Mesh || Chunk.Instances.Num() == 0)
	{
		return;
	}
	UInstancedSkinnedMeshComponent* C = NewObject<UInstancedSkinnedMeshComponent>(this);
	C->SetupAttachment(GetRootComponent());
	C->SetSkinnedAssetAndUpdate(Mesh, true);
	C->SetCastShadow(true);
	C->bVisibleInRayTracing = false;
	C->SetCullDistances(int32(CullDistanceCm * 0.8f), int32(CullDistanceCm));
	C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	C->SetComponentTickEnabled(false);                 // still until Tick decides (sway costs, see Tick)
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
	Anim.Init(0, Chunk.Instances.Num());
	C->AddInstances(Chunk.Instances, Anim, false, true);
	int32 Slot = Spawned.Find(nullptr);
	if (Slot == INDEX_NONE) { Slot = Spawned.Add(C); } else { Spawned[Slot] = C; }
	Chunk.Component = Slot;
}

void AForestStreamer::HideChunk(FChunk& Chunk)
{
	if (Spawned.IsValidIndex(Chunk.Component) && Spawned[Chunk.Component])
	{
		Spawned[Chunk.Component]->DestroyComponent();
		Spawned[Chunk.Component] = nullptr;
	}
	Chunk.Component = INDEX_NONE;
}

void AForestStreamer::Unload()
{
	bWanted = false;
	bReady = false;
	for (FChunk& C : Chunks)
	{
		HideChunk(C);
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
