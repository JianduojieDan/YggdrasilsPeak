// Copyright Epic Games, Inc. All Rights Reserved.

#include "Environment/YggQualitySubsystem.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/GameUserSettings.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	constexpr float SlowMs = 17.5f;     // under ~57 fps: step down (2026-11-11: the target is 60+, was 36)
	constexpr float RoomMs = 12.5f;     // over ~80 fps: resolution may climb back
	constexpr float MinPercent = 50.f, MaxPercent = 80.f;

	bool ShouldRun() { return !GIsEditor; }
	UYggQualitySubsystem* FindSubsystem(UWorld* World) { return World && World->GetGameInstance() ? World->GetGameInstance()->GetSubsystem<UYggQualitySubsystem>() : nullptr; }
}

static FAutoConsoleCommandWithWorldAndArgs CmdQuality(TEXT("ygg.Quality"), TEXT("ygg.Quality <auto|0..3>: automatic quality, or a fixed tier (0 low .. 3 epic)"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		UYggQualitySubsystem* Q = FindSubsystem(World);
		if (!Q || Args.Num() == 0) { return; }
		if (Args[0].Equals(TEXT("auto"), ESearchCase::IgnoreCase)) { Q->SetAuto(); } else { Q->SetManualTier(FCString::Atoi(*Args[0])); }
	}));

static FAutoConsoleCommandWithWorldAndArgs CmdCull(TEXT("ygg.Cull"), TEXT("ygg.Cull <0|1>: size-based draw distances off or on"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		UYggQualitySubsystem::ApplyDrawDistances(World, Args.Num() == 0 || FCString::Atoi(*Args[0]) != 0);
	}));

void UYggQualitySubsystem::ApplyDrawDistances(UWorld* World, bool bOn)
{
	if (!World) { return; }
	int32 Count = 0;
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		// the sky and the far terrain are meant to be seen from anywhere
		const FString Label = It->GetActorNameOrLabel();
		if (Label.StartsWith(TEXT("Realm_")) || Label.StartsWith(TEXT("Far")) || Label.Contains(TEXT("Distant")) || Label.Contains(TEXT("Horizon")) || It->GetClass()->GetName().Contains(TEXT("Ultra_Dynamic"))) { continue; }
		TInlineComponentArray<UStaticMeshComponent*> Meshes(*It);
		for (UStaticMeshComponent* M : Meshes)
		{
			if (UInstancedStaticMeshComponent* I = Cast<UInstancedStaticMeshComponent>(M))
			{
				// each piece by its own distance: a rope or a rock field spans kilometres. Rocks shape the view, ropes do not.
				const bool bRope = Label.Contains(TEXT("Rope"));
				I->SetCullDistances(bOn ? (bRope ? 25000 : 300000) : 0, bOn ? (bRope ? 30000 : 350000) : 0);
				++Count;
				continue;
			}
			const float Radius = M->Bounds.SphereRadius;
			if (Radius > 3000.f) { continue; }                    // large landforms stay
			const float Dist = Radius < 100.f ? 15000.f : Radius < 500.f ? 40000.f : 120000.f;   // 150 m, 400 m, 1.2 km
			M->SetCachedMaxDrawDistance(bOn ? Dist : 0.f);
			M->LDMaxDrawDistance = bOn ? Dist : 0.f;
			M->MarkRenderStateDirty();
			++Count;
		}
	}
	UE_LOG(LogTemp, Display, TEXT("YggQuality: draw distances %s on %d meshes"), bOn ? TEXT("on") : TEXT("off"), Count);
}

void UYggQualitySubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	if (!ShouldRun()) { return; }
	WorldHandle = FWorldDelegates::OnWorldBeginTearDown.AddLambda([](UWorld*) {});
	FWorldDelegates::OnWorldInitializedActors.AddLambda([](const FActorsInitializedParams& P)
	{
		if (P.World && P.World->IsGameWorld()) { ApplyDrawDistances(P.World, true); }
	});
	if (UGameUserSettings* S = GEngine ? GEngine->GetGameUserSettings() : nullptr)
	{
		S->LoadSettings();
		if (S->GetLastCPUBenchmarkResult() < 0.f)
		{
			S->RunHardwareBenchmark();
			S->ApplyHardwareBenchmarkResults();
			// Epic's Lumen guide: Epic quality targets 30 fps, High 60. Epic stays a choice, never the automatic pick.
			if (S->GetOverallScalabilityLevel() > 2) { S->SetOverallScalabilityLevel(2); S->ApplySettings(false); }
			S->SaveSettings();
			UE_LOG(LogTemp, Display, TEXT("YggQuality: benchmark picked tier %d"), S->GetOverallScalabilityLevel());
		}
	}
	ApplyPercent();
}

void UYggQualitySubsystem::Deinitialize()
{
	FWorldDelegates::OnWorldBeginTearDown.Remove(WorldHandle);
	Super::Deinitialize();
}

namespace
{
	// beyond the engine's scalability groups: Low and Medium draw Nanite a little coarser (half the triangles on
	// screen; the geometry raster cost 5.8 ms of a 4K frame on 2026-11-12)
	void ApplyTierExtras(int32 Tier)
	{
		if (IConsoleVariable* V = IConsoleManager::Get().FindConsoleVariable(TEXT("r.Nanite.MaxPixelsPerEdge")))
		{
			V->Set(Tier <= 1 ? 2.f : 1.f, ECVF_SetByConsole);
		}
	}
}

void UYggQualitySubsystem::ApplyPercent()
{
	if (const UGameUserSettings* S = GEngine ? GEngine->GetGameUserSettings() : nullptr) { ApplyTierExtras(S->GetOverallScalabilityLevel()); }
	if (IConsoleVariable* V = IConsoleManager::Get().FindConsoleVariable(TEXT("r.ScreenPercentage")))
	{
		V->Set(Percent, ECVF_SetByConsole);   // above the scalability group, which would force 100
	}
}

void UYggQualitySubsystem::SetManualTier(int32 Tier)
{
	bAuto = false;
	if (UGameUserSettings* S = GEngine->GetGameUserSettings())
	{
		S->SetOverallScalabilityLevel(FMath::Clamp(Tier, 0, 3));
		S->ApplySettings(false);
	}
	ApplyPercent();
}

void UYggQualitySubsystem::SetAuto()
{
	bAuto = true;
	WindowTime = 0.f; WindowFrames = 0; Warmup = 5.f;
}

void UYggQualitySubsystem::Tick(float DeltaTime)
{
	if (!ShouldRun() || !bAuto) { return; }
	const UWorld* World = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr;
	if (!World || UGameplayStatics::IsGamePaused(World)) { return; }
	if (Warmup > 0.f) { Warmup -= DeltaTime; return; }   // loading and shader hitches say nothing
	WindowTime += DeltaTime; ++WindowFrames;
	if (WindowTime < 4.f) { return; }
	const float Ms = WindowTime * 1000.f / WindowFrames;
	WindowTime = 0.f; WindowFrames = 0;
	UGameUserSettings* S = GEngine->GetGameUserSettings();
	if (Ms > SlowMs)
	{
		CalmTime = 0.f;
		if (Percent > MinPercent)
		{
			Percent = FMath::Max(MinPercent, Percent - 8.f);
			ApplyPercent();
			UE_LOG(LogTemp, Display, TEXT("YggQuality: %.1f ms, resolution %.0f%%"), Ms, Percent);
		}
		else if (S && S->GetOverallScalabilityLevel() > 0)
		{
			const int32 Tier = S->GetOverallScalabilityLevel() - 1;
			S->SetOverallScalabilityLevel(Tier);
			S->ApplySettings(false);
			S->SaveSettings();
			Percent = 67.f;
			ApplyPercent();
			Warmup = 6.f;
			UE_LOG(LogTemp, Display, TEXT("YggQuality: %.1f ms, quality tier %d"), Ms, Tier);
		}
	}
	else if (Ms < RoomMs)
	{
		CalmTime += 4.f;
		if (CalmTime >= 20.f && Percent < MaxPercent)
		{
			CalmTime = 0.f;
			Percent = FMath::Min(MaxPercent, Percent + 4.f);
			ApplyPercent();
		}
	}
	else { CalmTime = 0.f; }
}
