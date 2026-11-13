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
#include "Misc/ConfigCacheIni.h"

namespace
{
	constexpr float SlowMs = 17.5f;     // under ~57 fps: step down (the target is 60+)
	constexpr float RoomMs = 12.5f;     // over ~80 fps: climb back
	constexpr float MinPercent = 50.f, MaxPercent = 85.f;
	const TCHAR* const Sec = TEXT("YggdrasilsPeak.Display");

	bool ShouldAdapt() { return !GIsEditor; }   // the automatic steps never run in the editor; the choices apply everywhere
	float FixedPercent(int32 Up) { static const float P[] = { 75.f, 100.f, 67.f, 58.f, 50.f }; return P[FMath::Clamp(Up, 0, 4)]; }
	void SetCVar(const TCHAR* Name, float V) { if (IConsoleVariable* C = IConsoleManager::Get().FindConsoleVariable(Name)) { C->Set(V, ECVF_SetByConsole); } }
}

UYggQualitySubsystem* UYggQualitySubsystem::Get(UWorld* World)
{
	return World && World->GetGameInstance() ? World->GetGameInstance()->GetSubsystem<UYggQualitySubsystem>() : nullptr;
}

static FAutoConsoleCommandWithWorldAndArgs CmdQuality(TEXT("ygg.Quality"), TEXT("ygg.Quality <auto|0..3>: automatic quality, or a fixed tier (0 low .. 3 epic)"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		UYggQualitySubsystem* Q = UYggQualitySubsystem::Get(World);
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
	FWorldDelegates::OnWorldInitializedActors.AddLambda([](const FActorsInitializedParams& P)
	{
		if (P.World && P.World->IsGameWorld() && !GIsEditor) { ApplyDrawDistances(P.World, true); }
	});
	// the machine's tier: from the hardware benchmark once (at most High - Epic stays the player's choice), kept in our
	// own section so the engine's saved scalability (which a session may have lowered) never decides it
	int32 Saved = -1;
	if (GConfig->GetInt(Sec, TEXT("MachineTier"), Saved, GGameUserSettingsIni) && Saved >= 0)
	{
		MachineTier = FMath::Clamp(Saved, 0, 3);
	}
	else if (UGameUserSettings* S = GEngine ? GEngine->GetGameUserSettings() : nullptr)
	{
		S->LoadSettings();
		if (S->GetLastCPUBenchmarkResult() < 0.f) { S->RunHardwareBenchmark(); }
		S->ApplyHardwareBenchmarkResults();
		MachineTier = FMath::Clamp(S->GetOverallScalabilityLevel(), 0, 2);
		GConfig->SetInt(Sec, TEXT("MachineTier"), MachineTier, GGameUserSettingsIni);
		GConfig->Flush(false, GGameUserSettingsIni);
		UE_LOG(LogTemp, Display, TEXT("YggQuality: benchmark -> machine tier %d"), MachineTier);
	}
	SessionTier = MachineTier;
}

void UYggQualitySubsystem::Deinitialize()
{
	FWorldDelegates::OnWorldBeginTearDown.Remove(WorldHandle);
	Super::Deinitialize();
}

void UYggQualitySubsystem::Configure(int32 TierChoice, int32 UpscalingChoice)
{
	ChosenTier = FMath::Clamp(TierChoice, -1, 3);
	Upscaling = FMath::Clamp(UpscalingChoice, 0, 4);
	SessionTier = IsAutoTier() ? MachineTier : ChosenTier;
	Percent = FixedPercent(Upscaling);
	WindowTime = 0.f; WindowFrames = 0; CalmTime = 0.f; Warmup = 15.f; WindowMs.Reset(); SlowWindows = 0; PendingTier = -1;
	ApplyTier(SessionTier);
	ApplyPercent();
	UE_LOG(LogTemp, Display, TEXT("YggQuality: quality %s -> tier %d, upscaling %d -> %.0f%%%s"), IsAutoTier() ? TEXT("auto") : TEXT("fixed"), SessionTier,
		Upscaling, Percent, IsAutoPercent() ? TEXT(" (follows the frame rate)") : TEXT(""));
}

namespace
{
	// beyond the engine's scalability groups: Low and Medium draw Nanite a little coarser (half the triangles on screen)
	void ApplyTierExtras(int32 Tier)
	{
		SetCVar(TEXT("r.Nanite.MaxPixelsPerEdge"), Tier <= 1 ? 2.f : 1.f);
	}
}

void UYggQualitySubsystem::ApplyTier(int32 Tier)
{
	if (UGameUserSettings* S = GEngine ? GEngine->GetGameUserSettings() : nullptr)
	{
		S->SetOverallScalabilityLevel(FMath::Clamp(Tier, 0, 3));
		S->SetResolutionScaleValueEx(100.f);        // the render resolution is ours (r.ScreenPercentage), not the scalability group's
		S->ApplyNonResolutionSettings();            // applied, NOT saved (Settings saves the player's own choices)
	}
	ApplyTierExtras(Tier);
}

void UYggQualitySubsystem::ApplyPercent()
{
	SetCVar(TEXT("r.ScreenPercentage"), Percent);
	// in the editor's Play the viewport otherwise keeps the editor's own screen percentage and ignores the game's
	if (GIsEditor) { SetCVar(TEXT("r.Editor.Viewport.OverridePIEScreenPercentage"), 0.f); }
}

void UYggQualitySubsystem::SetManualTier(int32 Tier)
{
	Configure(FMath::Clamp(Tier, 0, 3), Upscaling);
}

void UYggQualitySubsystem::SetAuto()
{
	Configure(-1, Upscaling);
}

void UYggQualitySubsystem::Tick(float DeltaTime)
{
	if (!ShouldAdapt() || (!IsAutoTier() && !IsAutoPercent())) { return; }
	const UWorld* World = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr;
	if (!World) { return; }
	// Rules from the user's monitored session (2026-11-14). A change of quality tier rebuilds the sky light capture and
	// the shaders: a 0.5-0.7 s freeze with a flash of light, and the old 4 s average dropped the tier mid-play whenever
	// the sky filled the view, twice down to Low without the player knowing. Now:
	//  - a tier change is decided during play but carried out only while the game is paused (a menu hides it);
	//  - the judge is the MEDIAN frame time of 10 s windows (hitches do not count), slow for two windows in a row;
	//  - with frame generation nothing is judged: the rendered rate is paced by it (120 Hz with 4x is exactly 30 fps);
	//  - the warm-up counts real playing time (one long loading frame used to end it at once).
	const bool bPaused = UGameplayStatics::IsGamePaused(World);
	if (bPaused && PendingTier >= 0 && PendingTier != SessionTier)
	{
		UE_LOG(LogTemp, Display, TEXT("YggQuality: paused, switching to tier %d (machine tier %d)"), PendingTier, MachineTier);
		SessionTier = PendingTier;
		ApplyTier(SessionTier);
		PendingTier = -1;
		Warmup = 15.f;
	}
	if (bPaused) { return; }
	if (Warmup > 0.f) { Warmup -= FMath::Min(DeltaTime, 0.05f); return; }   // loading and shader hitches say nothing
	static IConsoleVariable* FrameGen = IConsoleManager::Get().FindConsoleVariable(TEXT("r.Streamline.DLSSG.Enable"));
	if (FrameGen && FrameGen->GetInt() != 0) { WindowTime = 0.f; WindowMs.Reset(); SlowWindows = 0; CalmTime = 0.f; return; }
	WindowTime += DeltaTime;
	WindowMs.Add(DeltaTime * 1000.f);
	if (WindowTime < 10.f) { return; }
	WindowMs.Sort();
	const float Ms = WindowMs[WindowMs.Num() / 2];
	WindowTime = 0.f; WindowMs.Reset();
	if (Ms > SlowMs)
	{
		CalmTime = 0.f;
		if (++SlowWindows < 2) { return; }
		SlowWindows = 0;
		if (IsAutoPercent() && Percent > MinPercent)
		{
			Percent = FMath::Max(MinPercent, Percent - 8.f);        // the render resolution is a light change: at once
			ApplyPercent();
			UE_LOG(LogTemp, Display, TEXT("YggQuality: median %.1f ms, resolution %.0f%%"), Ms, Percent);
		}
		else if (IsAutoTier() && SessionTier > 0 && PendingTier != SessionTier - 1)
		{
			PendingTier = SessionTier - 1;
			UE_LOG(LogTemp, Display, TEXT("YggQuality: median %.1f ms, tier %d at the next pause (machine tier %d)"), Ms, PendingTier, MachineTier);
		}
	}
	else if (Ms < RoomMs)
	{
		SlowWindows = 0;
		CalmTime += 10.f;
		if (IsAutoTier() && SessionTier < MachineTier && CalmTime >= 60.f && (!IsAutoPercent() || Percent >= MaxPercent))
		{
			CalmTime = 0.f;
			PendingTier = SessionTier + 1;
			UE_LOG(LogTemp, Display, TEXT("YggQuality: median %.1f ms, back to tier %d at the next pause"), Ms, PendingTier);
		}
		else if (IsAutoPercent() && CalmTime >= 20.f && Percent < MaxPercent)
		{
			CalmTime = 0.f;
			Percent = FMath::Min(MaxPercent, Percent + 4.f);
			ApplyPercent();
		}
	}
	else { CalmTime = 0.f; SlowWindows = 0; }
}
