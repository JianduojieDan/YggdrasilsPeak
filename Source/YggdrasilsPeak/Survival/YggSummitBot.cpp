// The robot player's death-zone run: see YggSummitBot.h. Tools/Bot/run_bot.ps1 -Chapter summit runs it; Tools/Bot/README.md.

#include "Survival/YggSummitBot.h"
#include "Survival/ExpeditionComponent.h"
#include "Survival/SurvivalComponent.h"
#include "Survival/OxygenPickup.h"
#include "Survival/YggMenuWidget.h"
#include "Survival/YggViewRig.h"
#include "Environment/FixedRope.h"
#include "Environment/RouteLandmark.h"
#include "Story/YggStory.h"

#include "Async/Async.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Containers/Queue.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/InputSettings.h"
#include "GameFramework/PlayerController.h"
#include "HAL/FileManager.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformTime.h"
#include "IImageWrapper.h"
#include "IImageWrapperModule.h"
#include "Misc/App.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/OutputDevice.h"
#include "Misc/OutputDeviceRedirector.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "UnrealClient.h"

DEFINE_LOG_CATEGORY_STATIC(LogYggSummitBot, Log, All);

/** The expedition's own lines (the death-zone message, summit, rope, bottles) and the story's, handed to the game thread. */
class FYggSummitBotLog : public FOutputDevice
{
public:
	FYggSummitBotLog() { bAutoEmitLineTerminator = false; }
	virtual bool CanBeUsedOnAnyThread() const override { return true; }
	virtual bool CanBeUsedOnMultipleThreads() const override { return true; }
	TQueue<FString, EQueueMode::Mpsc> Lines;
	virtual void Serialize(const TCHAR* V, ELogVerbosity::Type Verbosity, const FName& Category) override
	{
		if (!V) { return; }
		if (FCString::Strncmp(V, TEXT("Expedition"), 10) == 0 || FCString::Strncmp(V, TEXT("Story"), 5) == 0 || FCString::Strncmp(V, TEXT("Rig:"), 4) == 0
			|| FCString::Strncmp(V, TEXT("Put on a spare"), 14) == 0 || FCString::Strncmp(V, TEXT("CamRec: INCIDENT"), 16) == 0)
		{
			Lines.Enqueue(FString(V));
		}
	}
};

namespace
{
	TAutoConsoleVariable<int32> CVarSummitBotQuit(TEXT("ygg.Bot.Summit.Quit"), 1, TEXT("Summit robot: 1 = quit the game when the run ends."));

	FAutoConsoleCommandWithWorldAndArgs SummitBotCmd(TEXT("ygg.Bot.Summit"),
		TEXT("ygg.Bot.Summit [fast] [noo2] [stay <min>] [down <m>] [cap <min>] - the robot climbs from the South Col to the summit along route.json leg 7, measures it, writes Saved/Bot/summit_<time>/report.md and quits."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			UYggSummitBot* Bot = World ? World->GetSubsystem<UYggSummitBot>() : nullptr;
			if (!Bot) { UE_LOG(LogYggSummitBot, Warning, TEXT("SummitBot: no robot in this world (run it in the game or PIE)")); return; }
			Bot->Run(Args);
		}));
	FAutoConsoleCommandWithWorld SummitBotStop(TEXT("ygg.Bot.Summit.Stop"), TEXT("Stop the summit robot: final report, no quit."),
		FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World)
		{
			if (UYggSummitBot* Bot = World ? World->GetSubsystem<UYggSummitBot>() : nullptr) { Bot->StopRun(TEXT("stopped (ygg.Bot.Summit.Stop)")); }
		}));

	FString SummitBotClock(double S)
	{
		const int32 T = FMath::Max(0, FMath::RoundToInt(S));
		return T >= 3600 ? FString::Printf(TEXT("%d:%02d:%02d"), T / 3600, (T / 60) % 60, T % 60) : FString::Printf(TEXT("%d:%02d"), T / 60, T % 60);
	}
	FString SummitBotCm(const FVector& V) { return FString::Printf(TEXT("%.0f %.0f %.0f"), V.X, V.Y, V.Z); }
	const TCHAR* PhaseName(uint8 P)
	{
		static const TCHAR* N[] = { TEXT("idle"), TEXT("waiting"), TEXT("settle"), TEXT("up"), TEXT("summit"), TEXT("stay"), TEXT("down"), TEXT("quitting") };
		return P < UE_ARRAY_COUNT(N) ? N[P] : TEXT("?");
	}
	/** The South Col leg of route.json. */
	constexpr int32 SummitLeg = 7;
}

// ------------------------------------------------------------------------------------------------ lifetime

bool UYggSummitBot::ShouldCreateSubsystem(UObject* Outer) const
{
	const UWorld* W = Cast<UWorld>(Outer);
	return W && (W->WorldType == EWorldType::Game || W->WorldType == EWorldType::PIE) && Super::ShouldCreateSubsystem(Outer);
}

void UYggSummitBot::Deinitialize()
{
	if (Phase != EPhase::Idle && Phase != EPhase::Quitting) { Finish(TEXT("the world closed during the run")); }
	if (LogDevice.IsValid() && GLog) { GLog->RemoveOutputDevice(LogDevice.Get()); }
	if (ShotHandle.IsValid()) { UGameViewportClient::OnScreenshotCaptured().Remove(ShotHandle); ShotHandle.Reset(); }
	if (bShotPending && FScreenshotRequest::IsScreenshotRequested()) { FScreenshotRequest::Reset(); }   // (2026-11-20)
	bShotPending = false;
	Phase = EPhase::Idle;
	Super::Deinitialize();
}

void UYggSummitBot::Run(const TArray<FString>& Args)
{
	if (Phase != EPhase::Idle) { UE_LOG(LogYggSummitBot, Warning, TEXT("SummitBot: already running")); return; }
	// the run teleports to the South Col and would save that checkpoint over the player's save (2026-11-20)
	if (!FParse::Param(FCommandLine::Get(), TEXT("YggNoSave")))
	{
		UE_LOG(LogYggSummitBot, Warning, TEXT("SummitBot: needs -YggNoSave (it would overwrite your save). Use Tools/Bot/run_bot.ps1 -Chapter summit."));
		return;
	}
	bFast = bNoO2 = false;
	StayMin = 0.f; DownM = 300.f; CapMin = 60.f; FromM = 0.f;
	for (int32 i = 0; i < Args.Num(); ++i)
	{
		const FString A = Args[i].ToLower();
		const float Next = i + 1 < Args.Num() ? FCString::Atof(*Args[i + 1]) : 0.f;
		if (A == TEXT("fast")) { bFast = true; }
		else if (A == TEXT("noo2")) { bNoO2 = true; }
		else if (A == TEXT("stay")) { StayMin = FMath::Max(0.f, Next); ++i; }
		else if (A == TEXT("down")) { DownM = FMath::Max(0.f, Next); ++i; }
		else if (A == TEXT("cap")) { CapMin = FMath::Max(5.f, Next); ++i; }
		else if (A == TEXT("from")) { FromM = FMath::Max(0.f, Next); ++i; }
	}
	GameT = 0.0; RealStart = FPlatformTime::Seconds(); LastReport = RealStart; PhaseT = 0.0;
	UpStartT = SummitT = MenuT = DownStartT = DownEndT = DeathT = DeathZoneMsgT = StayStartT = -1.0;
	FirstHypT = FirstFrostT = FirstFaintT = FirstDamageT = FirstExhT = -1.0;
	Samples.Reset(); Issues.Reset(); Shots.Reset(); Events.Reset(); ExpeditionLines.Reset(); FrameMs.Reset(); OffRope.Reset();
	ProbeMs.Reset(); SkipFrameStats = 0;
	// a second run in the same session starts clean (2026-11-20)
	NextSample = NextCheck = 0.0; bShotPending = false; PendingShot.Reset();
	if (ShotHandle.IsValid()) { UGameViewportClient::OnScreenshotCaptured().Remove(ShotHandle); ShotHandle.Reset(); }
	Teleports = Sidesteps = Probes = Falls = 0;
	RopeOnSec = RopeOffSec = FallSec = MaxLat = MaxFloor = 0.f;
	MaxHyp = MaxFrost = MaxExh = MaxFaint = MaxHeart = 0.f; MinSpO2 = 100.f; MinHealth = 100.f; MinCore = 40.f;
	bSawMoveLease = bSawNudge = bSawAttention = bBeatStarted = bBeatDone = false;
	SummitStep = 0; BeatLog.Reset(); EndWhy.Reset();
	bLostNoted = bFloatNoted = bUnderNoted = false; OffRopeFrom = -1.f; FallFor = LostFor = FloatFor = UnderFor = 0.f;
	bLookFree = false; bHaveTree = false; TreeInfo.Reset();
	const FString Stamp = FDateTime::Now().ToString(TEXT("%Y%m%d_%H%M%S"));
	RunName = FString::Printf(TEXT("summit_%s%s%s"), *Stamp, bNoO2 ? TEXT("_noo2") : TEXT("_o2"), bFast ? TEXT("_fast") : TEXT(""));
	Dir = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("Bot") / RunName);
	ShotDir = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("Review") / TEXT("A5"));
	IFileManager::Get().MakeDirectory(*Dir, true);
	IFileManager::Get().MakeDirectory(*ShotDir, true);
	if (!LoadRoute()) { Phase = EPhase::Waiting; Finish(TEXT("cannot read route.json leg 7")); return; }
	if (!LogDevice.IsValid()) { LogDevice = MakeShared<FYggSummitBotLog>(); }
	else { FString Stale; while (LogDevice->Lines.Dequeue(Stale)) {} }
	if (GLog) { GLog->AddOutputDevice(LogDevice.Get()); }
	if (IConsoleVariable* V = IConsoleManager::Get().FindConsoleVariable(TEXT("ygg.Story.Debug"))) { V->Set(TEXT("1"), ECVF_SetByConsole); }
	if (bFast)
	{
		if (IConsoleVariable* V = IConsoleManager::Get().FindConsoleVariable(TEXT("t.MaxFPS"))) { PrevMaxFps = V->GetString(); bMaxFpsSet = true; V->Set(TEXT("0"), ECVF_SetByConsole); }
	}
	Phase = EPhase::Waiting;
	Note(FString::Printf(TEXT("run %s: %s, %s, stay %.0f min, down %.0f m, cap %.0f min; route leg 7 %d points, %.0f m"),
		*RunName, bNoO2 ? TEXT("no oxygen") : TEXT("bottled oxygen"), bFast ? TEXT("sprint when possible") : TEXT("walk"), StayMin, DownM, CapMin, Route.Num(), RouteLen / 100.f));
}

void UYggSummitBot::StopRun(const FString& Why)
{
	if (Phase != EPhase::Idle && Phase != EPhase::Quitting)
	{
		CVarSummitBotQuit->Set(0, ECVF_SetByConsole);
		Finish(Why);
	}
}

void UYggSummitBot::Note(const FString& Text)
{
	const FString L = FString::Printf(TEXT("%s  %s"), *SummitBotClock(GameT), *Text);
	if (Events.Num() < 2000) { Events.Add(L); }
	UE_LOG(LogYggSummitBot, Display, TEXT("SummitBot %s"), *L);
}

void UYggSummitBot::Issue(const FString& Kind, const FString& Text, APawn* P)
{
	FIssue I;
	I.T = GameT; I.S = S / 100.f; I.Kind = Kind; I.Text = Text;
	if (P) { I.Pos = P->GetActorLocation(); }
	if (const USurvivalComponent* Sv = P ? P->FindComponentByClass<USurvivalComponent>() : nullptr) { I.Elev = Sv->GetSnapshot().ElevationMeters; }
	if (Issues.Num() < 500) { Issues.Add(I); }
	Note(FString::Printf(TEXT("ISSUE %s at %.0f m along (%.0f m a.s.l.): %s"), *Kind, I.S, I.Elev, *Text));
}

// ------------------------------------------------------------------------------------------------ the route

bool UYggSummitBot::LoadRoute()
{
	Route.Reset(); Cum.Reset();
	FString Raw;
	TArray<TSharedPtr<FJsonValue>> Legs;
	const FString Path = FPaths::ProjectDir() / TEXT("Tools/RoutePlanning/route.json");
	if (!FFileHelper::LoadFileToString(Raw, *Path) || !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Raw), Legs) || !Legs.IsValidIndex(SummitLeg)
		|| !Legs[SummitLeg].IsValid() || Legs[SummitLeg]->Type != EJson::Object)
	{
		return false;
	}
	const TArray<TSharedPtr<FJsonValue>>* Pts;
	if (!Legs[SummitLeg]->AsObject()->TryGetArrayField(TEXT("pts"), Pts)) { return false; }
	for (const TSharedPtr<FJsonValue>& V : *Pts)
	{
		if (!V.IsValid() || V->Type != EJson::Array) { continue; }
		const TArray<TSharedPtr<FJsonValue>>& A = V->AsArray();
		if (A.Num() >= 3) { Route.Add(FVector(A[0]->AsNumber(), A[1]->AsNumber(), A[2]->AsNumber())); }
	}
	Cum.SetNum(Route.Num());
	for (int32 i = 0; i < Route.Num(); ++i) { Cum[i] = i == 0 ? 0.f : Cum[i - 1] + float(FVector::Dist2D(Route[i - 1], Route[i])); }
	RouteLen = Route.Num() > 1 ? Cum.Last() : 0.f;
	return Route.Num() > 1;
}

void UYggSummitBot::Locate(const FVector& Pos, bool bDown)
{
	// the nearest segment in a window around the last one (a switchback's other arm is never taken for this one)
	const int32 Lo = FMath::Max(0, Seg - 4), Hi = FMath::Min(Route.Num() - 2, Seg + 6);
	float BestD = 1e12f; int32 BestI = Seg; float BestT = 0.f;
	for (int32 i = Lo; i <= Hi; ++i)
	{
		const FVector2D A(Route[i]), B(Route[i + 1]), P(Pos);
		const FVector2D AB = B - A;
		const float L2 = float(AB.SizeSquared());
		const float T = L2 > 1.f ? FMath::Clamp(float(FVector2D::DotProduct(P - A, AB)) / L2, 0.f, 1.f) : 0.f;
		const float D = float(FVector2D::Distance(P, A + AB * T));
		if (D < BestD - 1.f) { BestD = D; BestI = i; BestT = T; }
	}
	Seg = BestI;
	S = Cum[BestI] + (Cum[BestI + 1] - Cum[BestI]) * BestT;
	Lat = BestD;
}

FVector UYggSummitBot::PointAt(float AtS) const
{
	AtS = FMath::Clamp(AtS, 0.f, RouteLen);
	int32 i = 0;
	while (i < Route.Num() - 2 && Cum[i + 1] < AtS) { ++i; }
	const float L = FMath::Max(Cum[i + 1] - Cum[i], 1.f);
	return FMath::Lerp(Route[i], Route[i + 1], FMath::Clamp((AtS - Cum[i]) / L, 0.f, 1.f));
}

// ------------------------------------------------------------------------------------------------ every frame

void UYggSummitBot::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (Phase == EPhase::Idle) { return; }
	UWorld* W = GetWorld();
	APlayerController* PC = W ? W->GetFirstPlayerController() : nullptr;
	APawn* P = PC ? PC->GetPawn() : nullptr;
	const float RealDt = float(FApp::GetDeltaTime());
	if (Phase == EPhase::Quitting)
	{
		PhaseT += RealDt;
		if (PhaseT > 2.0)
		{
			Phase = EPhase::Idle;
			if (PC) { PC->ConsoleCommand(TEXT("quit")); } else { FPlatformMisc::RequestExit(false, TEXT("ygg.Bot.Summit")); }
		}
		return;
	}
	GameT += DeltaTime;
	PhaseT += DeltaTime;
	DrainLog();
	// a picture that never comes back (a failed capture gives no callback) must not block all later ones (2026-11-20)
	if (bShotPending && FPlatformTime::Seconds() - ShotAt > 5.0)
	{
		UGameViewportClient::OnScreenshotCaptured().Remove(ShotHandle);
		ShotHandle.Reset();
		bShotPending = false;
		Note(FString::Printf(TEXT("picture %s lost (no capture in 5 s)"), *PendingShot));
		PendingShot.Reset();
	}
	// the frames after a ground probe are the tool's own freeze, and the camp jump / settle is loading: not game frames (2026-11-20)
	if (Phase != EPhase::Waiting && Phase != EPhase::Settle && RealDt > 0.f)
	{
		if (SkipFrameStats > 0) { --SkipFrameStats; ProbeMs.Add(RealDt * 1000.f); }
		else if (FrameMs.Num() < 400000) { FrameMs.Add(RealDt * 1000.f); }
	}
	if (GameT > CapMin * 60.0) { Finish(FString::Printf(TEXT("cap: %.0f min of game time"), CapMin)); return; }

	if (Phase == EPhase::Waiting)
	{
		if (P && GameT >= 3.0) { Begin(P); }
		else if (FPlatformTime::Seconds() - RealStart > 240.0) { Finish(TEXT("no player pawn after 4 minutes")); }
		return;
	}
	if (!P) { return; }
	USurvivalComponent* Sv = P->FindComponentByClass<USurvivalComponent>();

	// no oxygen means none: the South Col pickups would put bottles in the pack (and on, once saturation falls)
	if (bNoO2 && Sv && (Sv->GetCarriedBottles() > 0 || Sv->GetBottleSecondsLeft() > 0.f)) { Sv->SetCarriedBottles(0); Sv->ClearOxygenBottle(); }
	// with oxygen the kit is one bottle on and one spare (17 kg pack): the camp's eight pickups would fill the pack to three
	// spares (23 kg, walk x0.83), which no summit push carries
	if (!bNoO2 && Sv && Phase != EPhase::Settle && Sv->GetCarriedBottles() > 1) { Sv->SetCarriedBottles(1); }

	// death: the time, the place, the cause; the death screen's picture; then the report
	if (Sv && Sv->IsDead())
	{
		if (DeathT < 0.0)
		{
			DeathT = GameT;
			DeathElev = Sv->GetSnapshot().ElevationMeters;
			DeathCause = Sv->GetDeathCause().ToString();
			MoveDir = FVector::ZeroVector;
			Note(FString::Printf(TEXT("DEATH at %.0f m a.s.l., %.0f m along: %s (%s after the start of the climb%s)"), DeathElev, S / 100.f, *DeathCause,
				*SummitBotClock(UpStartT >= 0.0 ? GameT - UpStartT : 0.0), StayStartT >= 0.0 ? *FString::Printf(TEXT(", %s after stopping at the summit"), *SummitBotClock(GameT - StayStartT)) : TEXT("")));
		}
		if (GameT - DeathT > 3.5 && !Shots.ContainsByPredicate([](const FShot& X) { return X.Stage == TEXT("death"); })) { Shot(TEXT("death"), true); }
		if (GameT - DeathT > 5.0) { Finish(FString::Printf(TEXT("died: %s"), *DeathCause)); }
		return;
	}

	switch (Phase)
	{
	case EPhase::Settle:
	{
		const ACharacter* C = Cast<ACharacter>(P);
		const bool bGround = C && C->GetCharacterMovement() && C->GetCharacterMovement()->IsMovingOnGround();
		if (PhaseT > 8.0 && bGround)
		{
			Locate(P->GetActorLocation(), false);
			Note(FString::Printf(TEXT("on the ground at %s, %.0f m a.s.l., %.0f m along leg 7, %.1f m off it"), *SummitBotCm(P->GetActorLocation()),
				Sv ? Sv->GetSnapshot().ElevationMeters : 0.f, S / 100.f, Lat / 100.f));
			if (Sv)
			{
				if (bNoO2) { Sv->SetCarriedBottles(0); Sv->ClearOxygenBottle(); }
				else { Sv->ClearOxygenBottle(); Sv->UseOxygenBottle(); Sv->SetCarriedBottles(1); }
				StartHealth = Sv->GetSnapshot().Health;
				StartElev = Sv->GetSnapshot().ElevationMeters;
			}
			Shot(TEXT("1_southcol"), true);
			// testing a stretch: a teleport up the route first (the times are then not the demo's)
			if (FromM > 0.f && TeleportAlong(P, FromM * 100.f, TEXT("from")))
			{
				Seg = 0;
				while (Seg < Route.Num() - 2 && Cum[Seg + 1] < FromM * 100.f) { ++Seg; }
				Locate(P->GetActorLocation(), false);
			}
			StartS = S;
			Phase = EPhase::Up;
			PhaseT = 0.0;
			UpStartT = GameT;
			UpStartReal = FPlatformTime::Seconds();
			BestS = S;
			LastProgressT = GameT;
			NextGroundProbeS = FMath::CeilToFloat(S / 10000.f) * 10000.f;
			Note(FString::Printf(TEXT("climb starts: %s"), bNoO2 ? TEXT("no bottle, none carried") : TEXT("a full bottle on (30 min), one spare")));
		}
		else if (PhaseT > 40.0) { Finish(TEXT("never reached the ground at the South Col")); return; }
		break;
	}
	case EPhase::Up:
	{
		Walk(P, DeltaTime, false);
		const UExpeditionComponent* Ex = P->FindComponentByClass<UExpeditionComponent>();
		if (Ex && Ex->HasReachedSummit())
		{
			SummitT = GameT;
			SummitReal = FPlatformTime::Seconds();
			SummitS = S;
			SummitElev = Sv ? Sv->GetSnapshot().ElevationMeters : 0.f;
			MoveDir = FVector::ZeroVector;
			if (Sv) { Sv->SetSprintRequested(false); }
			if (OffRopeFrom >= 0.f) { OffRope.Add(FVector2D(OffRopeFrom, S / 100.f)); OffRopeFrom = -1.f; }
			Note(FString::Printf(TEXT("SUMMIT reached after %s game time (%s real), %.0f m along, %.0f m a.s.l."), *SummitBotClock(SummitT - UpStartT),
				*SummitBotClock(SummitReal - UpStartReal), S / 100.f, Sv ? Sv->GetSnapshot().ElevationMeters : 0.f));
			Phase = EPhase::Summit;
			PhaseT = 0.0;
			SummitStep = 0;
			SummitStepT = GameT;
			bLookFree = true;   // the beat's nudge and attention belong to the view rig: no look input from here
		}
		else if (S >= RouteLen - 150.f && PhaseT > 5.0 && GameT - LastProgressT > 20.0)
		{
			Issue(TEXT("summit"), TEXT("route end reached but the summit camp did not trigger in 20 s"), P);
			Phase = EPhase::Summit; PhaseT = 0.0; SummitStep = 0; SummitStepT = GameT; bLookFree = true;
		}
		break;
	}
	case EPhase::Summit:
		Check(P, DeltaTime, false);
		SummitTick(P, PC, DeltaTime);
		break;
	case EPhase::Stay:
		Check(P, DeltaTime, false);
		if (GameT - StayStartT > StayMin * 60.0) { Finish(FString::Printf(TEXT("summit run finished: alive after %.0f min at the summit"), StayMin)); return; }
		break;
	case EPhase::Down:
		Walk(P, DeltaTime, true);
		if (S <= FMath::Max(0.f, DownStartS - DownM * 100.f) + 100.f || S <= 200.f)
		{
			DownEndT = GameT;
			Note(FString::Printf(TEXT("down %.0f m along the route in %s"), (DownStartS - S) / 100.f, *SummitBotClock(DownEndT - DownStartT)));
			Shot(TEXT("8_descent_end"), true);
			Finish(TEXT("summit run finished: summit reached and back down the route"));
			return;
		}
		if (DownStartT >= 0.0 && GameT - DownStartT > 25.0 && !Shots.ContainsByPredicate([](const FShot& X) { return X.Stage == TEXT("7_descent"); }))
		{
			Shot(TEXT("7_descent"), true);
		}
		break;
	default: break;
	}
	if (GameT >= NextSample) { NextSample = GameT + 1.0; Sample(P); }
	if (PC) { Look(PC, FMath::Min(RealDt, 0.1f)); }
	const double Now = FPlatformTime::Seconds();
	if (Now - LastReport > 60.0) { LastReport = Now; WriteFiles(false); }
}

void UYggSummitBot::Begin(APawn* P)
{
	UExpeditionComponent* Ex = P->FindComponentByClass<UExpeditionComponent>();
	if (!Ex) { Finish(TEXT("no ExpeditionComponent on the player")); return; }
	// the South Col camp: by name, else the camp below the summit (ygg.Expedition.Go 7 on the 9-camp route)
	ARouteLandmark* Col = nullptr;
	const TArray<TWeakObjectPtr<ARouteLandmark>>& Camps = Ex->GetCamps();
	FString List;
	for (const TWeakObjectPtr<ARouteLandmark>& C : Camps)
	{
		if (!C.IsValid()) { continue; }
		List += FString::Printf(TEXT("%s%s"), List.IsEmpty() ? TEXT("") : TEXT(", "), *C->LandmarkName);
		if (!Col && C->LandmarkName.Contains(TEXT("South Col"))) { Col = C.Get(); }
	}
	if (!Col && Camps.Num() >= 2 && Camps[Camps.Num() - 2].IsValid()) { Col = Camps[Camps.Num() - 2].Get(); }
	Note(FString::Printf(TEXT("%d camps: %s"), Camps.Num(), *List));
	if (!Col) { Finish(TEXT("no South Col camp")); return; }
	SouthColName = Col->LandmarkName;
	PickupsAtCol = 0;
	for (TActorIterator<AOxygenPickup> It(GetWorld()); It; ++It)
	{
		PickupsAtCol += FVector::Dist2D(It->GetActorLocation(), Col->GetActorLocation()) < 4000.0 ? 1 : 0;
	}
	FindTree();
	Ex->TeleportToCamp(Col);
	Seg = 0;
	Note(FString::Printf(TEXT("camp jump to %s (%s); oxygen pickups within 40 m: %d"), *SouthColName, *SummitBotCm(Col->GetActorLocation()), PickupsAtCol));
	Phase = EPhase::Settle;
	PhaseT = 0.0;
}

void UYggSummitBot::FindTree()
{
	// the World Tree: the static mesh SM_Yggdrasil_* on the summit (Tools/Art/world_tree_summit.py)
	FBox Box(ForceInit);
	FString Names;
	TreeActors.Reset();
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		TInlineComponentArray<UStaticMeshComponent*> Comps(*It);
		for (UStaticMeshComponent* C : Comps)
		{
			const UStaticMesh* M = C ? C->GetStaticMesh() : nullptr;
			if (M && M->GetName().Contains(TEXT("Yggdrasil")))
			{
				Box += C->Bounds.GetBox();
				TreeActors.AddUnique(*It);
				if (Names.Len() < 200) { Names += FString::Printf(TEXT("%s%s/%s"), Names.IsEmpty() ? TEXT("") : TEXT(", "), *It->GetName(), *M->GetName()); }
			}
		}
	}
	bHaveTree = Box.IsValid != 0;
	if (bHaveTree)
	{
		const FVector Ctr = Box.GetCenter(), Ext = Box.GetExtent();
		// the trunk a quarter of the way up: what a climber at its roots looks up at
		TreeAim = FVector(Ctr.X, Ctr.Y, Box.Min.Z + (Box.Max.Z - Box.Min.Z) * 0.25);
		TreeInfo = FString::Printf(TEXT("%s: bounds centre %s, size %.0f x %.0f x %.0f m, bottom %.0f m, top %.0f m (world Z cm %.0f..%.0f)"), *Names, *SummitBotCm(Ctr),
			Ext.X / 50.0, Ext.Y / 50.0, Ext.Z / 50.0, Box.Min.Z, Box.Max.Z, Box.Min.Z, Box.Max.Z);
	}
	else { TreeInfo = TEXT("no SM_Yggdrasil mesh found in the level"); }
	Note(FString::Printf(TEXT("World Tree: %s"), *TreeInfo));
}

void UYggSummitBot::Walk(APawn* P, float Dt, bool bDown)
{
	Locate(P->GetActorLocation(), bDown);
	const float Ahead = 700.f;
	const FVector Target = PointAt(bDown ? S - Ahead : S + Ahead);
	FVector Go = Target - P->GetActorLocation();
	Go.Z = 0.f;
	Go = Go.GetSafeNormal();
	// progress: further along (up) or back along (down)
	const float Prog = bDown ? -S : S;
	if (Prog > BestS + 100.f) { BestS = Prog; LastProgressT = GameT; }
	const double NoProg = GameT - LastProgressT;
	ACharacter* C = Cast<ACharacter>(P);
	if (NoProg > 8.0)
	{
		// stuck: a sidestep (2 s of every 6, alternating sides) and a jump, then a 10 m teleport along the route
		const double Ph = NoProg - 8.0;
		if (C && Ph <= Dt * 1.5)
		{
			// what holds the climber: the capsule swept 1.5 m towards the target (the report names it)
			UCapsuleComponent* Cap = C->GetCapsuleComponent();
			FHitResult B;
			FCollisionQueryParams BQ(SCENE_QUERY_STAT(YggSummitBotBlock), false, P);
			const FVector From = P->GetActorLocation();
			if (GetWorld()->SweepSingleByChannel(B, From, From + Go * 150.f, FQuat::Identity, ECC_Pawn, FCollisionShape::MakeCapsule(Cap->GetScaledCapsuleRadius(), Cap->GetScaledCapsuleHalfHeight()), BQ))
			{
				Note(FString::Printf(TEXT("stuck at %.0f m along: blocked by %s / %s (normal z %.2f, %.0f cm over the capsule bottom, %.0f cm ahead)"), S / 100.f,
					B.GetActor() ? *B.GetActor()->GetActorNameOrLabel() : TEXT("?"), *GetNameSafe(B.GetComponent()), B.ImpactNormal.Z,
					B.ImpactPoint.Z - (From.Z - Cap->GetScaledCapsuleHalfHeight()), B.Distance));
			}
			else { Note(FString::Printf(TEXT("stuck at %.0f m along: nothing in the way 1.5 m ahead"), S / 100.f)); }
		}
		if (FMath::Fmod(Ph, 6.0) < 2.0) { Go = Go.RotateAngleAxis((int32(Ph / 6.0) % 2) == 0 ? 70.f : -70.f, FVector::UpVector); }
		if (C && FMath::Fmod(Ph, 6.0) < Dt * 1.5) { C->Jump(); ++Sidesteps; }
		if (NoProg > 30.0)
		{
			Issue(TEXT("stuck"), FString::Printf(TEXT("no progress for %.0f s at %s: teleported 10 m %s the route"), NoProg, *SummitBotCm(P->GetActorLocation()), bDown ? TEXT("down") : TEXT("up")), P);
			TeleportAlong(P, bDown ? S - 1000.f : S + 1000.f, TEXT("stuck"));
			LastProgressT = GameT;
			BestS = bDown ? -S : S;
		}
	}
	MoveDir = Go;
	P->AddMovementInput(MoveDir, 1.f);
	if (bFast)
	{
		if (USurvivalComponent* Sv = P->FindComponentByClass<USurvivalComponent>()) { Sv->SetSprintRequested(true); }
	}
	// the visible surface under the boots every 100 m of route (Saved/Review/ground_probe.csv)
	if ((!bDown && S >= NextGroundProbeS) || (bDown && S <= NextGroundProbeS))
	{
		NextGroundProbeS = bDown ? S - 10000.f : S + 10000.f;
		if (GEngine) { GEngine->Exec(GetWorld(), *FString::Printf(TEXT("ygg.GroundProbe A5_%s_%04.0fm"), bDown ? TEXT("down") : TEXT("up"), S / 100.f)); ++Probes; SkipFrameStats = 2; }
	}
	Check(P, Dt, true);
}

void UYggSummitBot::Check(APawn* P, float Dt, bool bWalking)
{
	if (GameT < NextCheck) { return; }
	const float Step = 0.25f;
	NextCheck = GameT + Step;
	ACharacter* C = Cast<ACharacter>(P);
	UCharacterMovementComponent* M = C ? C->GetCharacterMovement() : nullptr;
	const UExpeditionComponent* Ex = P->FindComponentByClass<UExpeditionComponent>();
	USurvivalComponent* Sv = P->FindComponentByClass<USurvivalComponent>();
	if (!M || !Ex || !Sv) { return; }
	const FSurvivalSnapshot Snap = Sv->GetSnapshot();
	const float Half = C->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	const FVector Pos = P->GetActorLocation();
	const float Bottom = float(Pos.Z) - Half;

	// falling / sliding: MOVE_Falling over 0.75 s is reported (a step down a rock is shorter)
	if (M->IsFalling())
	{
		if (FallFor == 0.f) { FallStartZ = float(Pos.Z); }
		FallFor += Step;
		FallSec += Step;
	}
	else
	{
		if (FallFor >= 0.75f)
		{
			++Falls;
			Issue(TEXT("fall"), FString::Printf(TEXT("falling / sliding for %.2f s, %.1f m down"), FallFor, (FallStartZ - float(Pos.Z)) / 100.f), P);
		}
		FallFor = 0.f;
	}
	// the floor gap while walking (the movement component keeps it at about 2 cm)
	if (M->IsMovingOnGround() && M->CurrentFloor.bBlockingHit)
	{
		MaxFloor = FMath::Max(MaxFloor, M->CurrentFloor.FloorDist);
		FloatFor = M->CurrentFloor.FloorDist > 10.f ? FloatFor + Step : 0.f;
		if (FloatFor > 1.f && !bFloatNoted) { bFloatNoted = true; Issue(TEXT("float"), FString::Printf(TEXT("floor gap %.1f cm for 1 s while walking"), M->CurrentFloor.FloorDist), P); }
		if (FloatFor == 0.f) { bFloatNoted = false; }
	}
	// under the ground: the first surface from 30 m above is well over the capsule's bottom
	{
		FHitResult H;
		FCollisionQueryParams Q(SCENE_QUERY_STAT(YggSummitBotUnder), false, P);
		if (GetWorld()->LineTraceSingleByChannel(H, Pos + FVector(0, 0, 3000), Pos - FVector(0, 0, 3000), ECC_Pawn, Q) && H.ImpactPoint.Z > Bottom + 80.f
			&& !Cast<UInstancedStaticMeshComponent>(H.GetComponent()))   // an arch of rock or root overhead is not ground
		{
			UnderFor += Step;
			if (UnderFor > 1.f && !bUnderNoted) { bUnderNoted = true; Issue(TEXT("under"), FString::Printf(TEXT("ground (%s) %.0f cm above the capsule's bottom"), *GetNameSafe(H.GetComponent()), H.ImpactPoint.Z - Bottom), P); }
		}
		else { UnderFor = 0.f; bUnderNoted = false; }
	}
	if (bWalking)
	{
		// lost: more than 8 m off the route (where the rope lies) for 3 s
		MaxLat = FMath::Max(MaxLat, Lat / 100.f);
		LostFor = Lat > 800.f ? LostFor + Step : 0.f;
		if (LostFor > 3.f && !bLostNoted) { bLostNoted = true; Issue(TEXT("lost"), FString::Printf(TEXT("%.1f m off the route for 3 s"), Lat / 100.f), P); }
		if (LostFor == 0.f) { bLostNoted = false; }
		// the fixed rope: clipped (the expedition's own state) or not, and the stretches without it
		const bool bRope = Ex->IsOnRope();
		(bRope ? RopeOnSec : RopeOffSec) += Step;
		if (!bRope && OffRopeFrom < 0.f) { OffRopeFrom = S / 100.f; }
		if (bRope && OffRopeFrom >= 0.f) { OffRope.Add(FVector2D(OffRopeFrom, S / 100.f)); OffRopeFrom = -1.f; }
	}
	// the body
	float Hyp, Frost, Exh, Faint;
	Ex->GetBodyEffects(Hyp, Frost, Exh, Faint);
	const auto First = [this](double& At, bool bNow) { if (bNow && At < 0.0) { At = GameT; } };
	First(FirstHypT, Hyp > 0.05f); First(FirstFrostT, Frost > 0.05f); First(FirstExhT, Exh > 0.05f); First(FirstFaintT, Faint > 0.05f);
	First(FirstDamageT, Snap.Health < StartHealth - 0.5f);
	MaxHyp = FMath::Max(MaxHyp, Hyp); MaxFrost = FMath::Max(MaxFrost, Frost); MaxExh = FMath::Max(MaxExh, Exh); MaxFaint = FMath::Max(MaxFaint, Faint);
	MaxHeart = FMath::Max(MaxHeart, Ex->GetHeartRate());
	MinSpO2 = FMath::Min(MinSpO2, Snap.Oxygen); MinHealth = FMath::Min(MinHealth, Snap.Health); MinCore = FMath::Min(MinCore, Snap.BodyTemperature);
	// pictures on the way: the death zone (after its message) and three heights
	if (Phase == EPhase::Up)
	{
		if (DeathZoneMsgT >= 0.0 && GameT - DeathZoneMsgT > 2.0 && !Shots.ContainsByPredicate([](const FShot& X) { return X.Stage == TEXT("2_deathzone"); })) { Shot(TEXT("2_deathzone"), true); }
		static const int32 Heights[] = { 8300, 8600 };
		for (int32 Hm : Heights)
		{
			const FString Stage = FString::Printf(TEXT("3_climb_%d"), Hm);
			if (Snap.ElevationMeters >= Hm && !Shots.ContainsByPredicate([&Stage](const FShot& X) { return X.Stage == Stage; })) { Shot(Stage, true); }
		}
	}
}

void UYggSummitBot::Sample(APawn* P)
{
	ACharacter* C = Cast<ACharacter>(P);
	UCharacterMovementComponent* M = C ? C->GetCharacterMovement() : nullptr;
	const UExpeditionComponent* Ex = P->FindComponentByClass<UExpeditionComponent>();
	const USurvivalComponent* Sv = P->FindComponentByClass<USurvivalComponent>();
	if (!M || !Ex || !Sv) { return; }
	const FSurvivalSnapshot Snap = Sv->GetSnapshot();
	FSample X;
	X.T = GameT; X.Real = FPlatformTime::Seconds() - RealStart; X.Phase = uint8(Phase);
	X.S = S / 100.f; X.Lat = Lat / 100.f; X.Pos = P->GetActorLocation(); X.Elev = Snap.ElevationMeters;
	X.Speed = float(P->GetVelocity().Size2D()); X.MaxSpeed = M->MaxWalkSpeed;
	X.bFalling = M->IsFalling(); X.bWalkable = M->CurrentFloor.IsWalkableFloor(); X.FloorDist = M->CurrentFloor.FloorDist;
	X.bRope = Ex->IsOnRope(); X.bSheltered = Snap.bSheltered; X.bSprint = Snap.bSprinting;
	const FVector Feet = X.Pos - FVector(0, 0, C->GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
	float RD = 1e9f;
	for (TActorIterator<AFixedRope> It(GetWorld()); It; ++It) { if (It->IsNear(Feet, 3000.f)) { RD = FMath::Min(RD, It->DistanceToRope(Feet)); } }
	X.RopeDist = RD < 1e8f ? RD / 100.f : -1.f;
	X.SpO2 = Snap.Oxygen; X.Heart = Ex->GetHeartRate(); X.Health = Snap.Health; X.Stamina = Snap.Stamina; X.Core = Snap.BodyTemperature;
	X.Bottle = Snap.OxygenBottleSeconds; X.Bottles = Snap.CarriedBottles; X.Storm = Ex->GetStormStrength();
	Ex->GetBodyEffects(X.Hyp, X.Frost, X.Exh, X.Faint);
	if (Samples.Num() < 20000) { Samples.Add(X); }
}

void UYggSummitBot::SummitTick(APawn* P, APlayerController* PC, float Dt)
{
	// what the beat does with the view, read from the rig (the bot gives no input meanwhile)
	if (const UYggViewRigComponent* R = UYggViewRigComponent::Find(P))
	{
		bSawMoveLease |= R->HasLease(EYggLease::Move);
		bSawNudge |= R->IsNudging();
		bSawAttention |= R->HasLease(EYggLease::Attention);
	}
	const UWorld* W = GetWorld();
	const UYggStorySubsystem* Story = W ? W->GetSubsystem<UYggStorySubsystem>() : nullptr;
	const bool bMenu = PC && UYggMenuWidget::IsOpen(PC);
	const double In = GameT - SummitStepT;
	auto NextStep = [this]() { ++SummitStep; SummitStepT = GameT; };
	switch (SummitStep)
	{
	case 0:   // the beat: its subtitle a few seconds in, then wait for the summit screen
		bBeatStarted |= Story && Story->IsRunning();
		if (In > 3.0 && !Shots.ContainsByPredicate([](const FShot& X) { return X.Stage == TEXT("4_summit_beat"); })) { Shot(TEXT("4_summit_beat"), true); }
		if (bMenu)
		{
			MenuT = GameT;
			Note(FString::Printf(TEXT("summit screen after %.1f s (beat %s; Move lease %d, nudge %d, attention %d)"), MenuT - SummitT,
				bBeatStarted ? TEXT("played") : TEXT("did not start"), bSawMoveLease ? 1 : 0, bSawNudge ? 1 : 0, bSawAttention ? 1 : 0));
			NextStep();
		}
		else if (In > 30.0) { Issue(TEXT("summit"), TEXT("no summit screen 30 s after the summit"), P); NextStep(); }
		break;
	case 1:
		if (In > 2.0) { Shot(TEXT("5_summit_screen"), true); NextStep(); }
		break;
	case 2:
		if (In > 1.0)
		{
			if (PC) { UYggMenuWidget::CloseAll(PC); }   // "Stay at the summit"
			Note(TEXT("summit screen closed (Stay at the summit)"));
			bLookFree = false;
			// look at the World Tree's trunk from where the climber stands
			if (bHaveTree)
			{
				// the trunk 60 m above the eye (the mesh's roots reach far down the peak, so its bounds start below the summit)
				const FVector Aim(TreeAim.X, TreeAim.Y, FMath::Max(TreeAim.Z, P->GetActorLocation().Z + 6000.0));
				const FRotator R = (Aim - P->GetActorLocation()).Rotation();
				WantYaw = float(R.Yaw);
				WantPitch = FMath::Clamp(float(FRotator::NormalizeAxis(R.Pitch)), -10.f, 35.f);
			}
			else { WantPitch = 25.f; }
			NextStep();
		}
		break;
	case 3:
		if (In > 5.0) { Shot(TEXT("6_worldtree"), false); WantPitch = 50.f; NextStep(); }
		break;
	case 4:
		if (In > 4.0)
		{
			Shot(TEXT("6_worldtree_up"), false);
			// the view back down the route (the way home)
			const FVector Back = PointAt(S - 6000.f) - P->GetActorLocation();
			WantYaw = float(Back.Rotation().Yaw);
			WantPitch = -12.f;
			NextStep();
		}
		break;
	case 5:
		if (In > 5.0)
		{
			Shot(TEXT("6_summit_view_down"), false);
			NextStep();
			if (StayMin > 0.f)
			{
				Phase = EPhase::Stay; PhaseT = 0.0; StayStartT = GameT;
				Note(FString::Printf(TEXT("staying at the summit for up to %.0f min"), StayMin));
			}
			else
			{
				Phase = EPhase::Down; PhaseT = 0.0; DownStartT = GameT;
				Locate(P->GetActorLocation(), true);
				DownStartS = S; BestS = -S; LastProgressT = GameT; NextGroundProbeS = S - 5000.f;
				Note(FString::Printf(TEXT("down: from %.0f m along, %.0f m to go"), S / 100.f, DownM));
			}
		}
		break;
	default: break;
	}
}

void UYggSummitBot::Look(APlayerController* PC, float Dt)
{
	if (Dt <= 0.f || bLookFree || DeathT >= 0.0) { return; }
	if ((Phase == EPhase::Up || Phase == EPhase::Down) && !MoveDir.IsNearlyZero()) { WantYaw = float(MoveDir.Rotation().Yaw); WantPitch = Phase == EPhase::Down ? -14.f : -4.f; }
	const FRotator CR = PC->GetControlRotation();
	const float EY = float(FRotator::NormalizeAxis(double(WantYaw) - CR.Yaw));
	const float EP = WantPitch - float(FRotator::NormalizeAxis(CR.Pitch));
	// at most 60 deg/s: well under the camera recorder's spin limit (120)
	const float DY = FMath::Abs(EY) > 1.f ? FMath::Clamp(EY * 2.f * Dt, -60.f * Dt, 60.f * Dt) : 0.f;
	const float DP = FMath::Abs(EP) > 1.f ? FMath::Clamp(EP * 2.f * Dt, -40.f * Dt, 40.f * Dt) : 0.f;
	float YS = 1.f, PS = 1.f;
	if (GetDefault<UInputSettings>()->bEnableLegacyInputScales)
	{
PRAGMA_DISABLE_DEPRECATION_WARNINGS
		YS = PC->GetDeprecatedInputYawScale();
		PS = PC->GetDeprecatedInputPitchScale();
PRAGMA_ENABLE_DEPRECATION_WARNINGS
	}
	if (FMath::Abs(YS) < 0.01f) { YS = 1.f; }
	if (FMath::Abs(PS) < 0.01f) { PS = 1.f; }
	if (DY != 0.f) { PC->AddYawInput(DY / YS); }
	if (DP != 0.f) { PC->AddPitchInput(DP / PS); }
}

bool UYggSummitBot::TeleportAlong(APawn* P, float ToS, const FString& Why)
{
	ACharacter* C = Cast<ACharacter>(P);
	if (!C) { return false; }
	// route.json's heights are the ground when the route was planned: near the top they are 60-80 m under the raised
	// summit (a teleport to them dropped the climber inside the mountain, 2026-11-20). So the ground is traced from 150 m
	// above the higher of the route and the climber, and without ground there is no teleport.
	FVector At = PointAt(ToS);
	At.Z = FMath::Max(At.Z, P->GetActorLocation().Z);
	FHitResult H;
	FCollisionQueryParams Q(SCENE_QUERY_STAT(YggSummitBotTp), false, P);
	// the World Tree (and its roots) is solid to this trace: a teleport landed on the roots 10 m above the route (2026-11-20)
	for (const TWeakObjectPtr<AActor>& A : TreeActors) { if (A.IsValid()) { Q.AddIgnoredActor(A.Get()); } }
	if (!GetWorld()->LineTraceSingleByChannel(H, At + FVector(0, 0, 15000), At - FVector(0, 0, 20000), ECC_Pawn, Q))
	{
		Note(FString::Printf(TEXT("teleport (%s) refused: no ground under %s"), *Why, *SummitBotCm(At)));
		return false;
	}
	const FVector To = H.ImpactPoint + FVector(0, 0, C->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + 10.f);
	if (!P->SetActorLocation(To, false, nullptr, ETeleportType::TeleportPhysics)) { return false; }
	C->GetCharacterMovement()->StopMovementImmediately();
	APlayerController* PC = Cast<APlayerController>(P->GetController());
	if (PC && PC->PlayerCameraManager) { PC->PlayerCameraManager->SetGameCameraCutThisFrame(); }
	if (UYggViewRigComponent* R = UYggViewRigComponent::Find(P)) { if (R->GetView() == EYggView::Walk) { R->HardReset(PC ? PC->GetControlRotation() : P->GetActorRotation(), EYggView::Walk); } }
	++Teleports;
	Note(FString::Printf(TEXT("teleport (%s) to %s, onto %s / %s"), *Why, *SummitBotCm(To), H.GetActor() ? *H.GetActor()->GetActorNameOrLabel() : TEXT("?"), *GetNameSafe(H.GetComponent())));
	return true;
}

// ------------------------------------------------------------------------------------------------ pictures

void UYggSummitBot::Shot(const FString& Stage, bool bUI)
{
	if (bShotPending)
	{
		// one at a time: a shot asked for while another is on its way is taken a moment later by its caller's next check
		Note(FString::Printf(TEXT("picture %s skipped (another one pending)"), *Stage));
		return;
	}
	FShot X;
	X.Stage = Stage;
	X.File = ShotDir / FString::Printf(TEXT("%s_%s.png"), *RunName, *Stage);
	X.T = GameT;
	if (const APawn* P = GetWorld() && GetWorld()->GetFirstPlayerController() ? GetWorld()->GetFirstPlayerController()->GetPawn() : nullptr)
	{
		if (const USurvivalComponent* Sv = P->FindComponentByClass<USurvivalComponent>()) { X.Elev = Sv->GetSnapshot().ElevationMeters; }
	}
	Shots.Add(X);
	PendingShot = X.File;
	bShotPending = true;
	ShotAt = FPlatformTime::Seconds();
	// the pixels come to OnShot and a worker writes the PNG (the engine's own PNG write freezes the game thread)
	ShotHandle = UGameViewportClient::OnScreenshotCaptured().AddUObject(this, &UYggSummitBot::OnShot);
	FScreenshotRequest::RequestScreenshot(PendingShot, bUI, false);
	Note(FString::Printf(TEXT("picture %s%s"), *Stage, bUI ? TEXT(" (with the HUD)") : TEXT("")));
}

void UYggSummitBot::OnShot(int32 ShotW, int32 ShotH, const TArray<FColor>& Pixels)
{
	if (ShotHandle.IsValid()) { UGameViewportClient::OnScreenshotCaptured().Remove(ShotHandle); ShotHandle.Reset(); }
	const FString File = PendingShot;
	bShotPending = false;
	PendingShot.Reset();
	if (ShotW <= 0 || ShotH <= 0 || Pixels.Num() < ShotW * ShotH || File.IsEmpty()) { return; }
	TArray<FColor> Copy = Pixels;
	for (FColor& Px : Copy) { Px.A = 255; }   // the scene's alpha would make the PNG see-through
	Async(EAsyncExecution::ThreadPool, [File, ShotW, ShotH, Copy = MoveTemp(Copy)]()
	{
		IImageWrapperModule& Mod = FModuleManager::LoadModuleChecked<IImageWrapperModule>(FName(TEXT("ImageWrapper")));
		const TSharedPtr<IImageWrapper> Png = Mod.CreateImageWrapper(EImageFormat::PNG);
		if (Png.IsValid() && Png->SetRaw(Copy.GetData(), int64(Copy.Num()) * sizeof(FColor), ShotW, ShotH, ERGBFormat::BGRA, 8))
		{
			FFileHelper::SaveArrayToFile(Png->GetCompressed(), *File);
		}
	});
}

// ------------------------------------------------------------------------------------------------ the log

void UYggSummitBot::DrainLog()
{
	if (!LogDevice.IsValid()) { return; }
	FString Full;
	int32 Budget = 300;
	while (Budget-- > 0 && LogDevice->Lines.Dequeue(Full))
	{
		int32 Nl;
		const FString L = Full.FindChar(TEXT('\n'), Nl) ? Full.Left(Nl).TrimEnd() : Full.TrimEnd();
		if (ExpeditionLines.Num() < 400) { ExpeditionLines.Add(FString::Printf(TEXT("%s  %s"), *SummitBotClock(GameT), *L.Left(240))); }
		if (DeathZoneMsgT < 0.0 && L.Contains(TEXT("death zone")))
		{
			DeathZoneMsgT = GameT;
			DeathZoneMsg = L;
			bDeathZoneHeld = L.Contains(TEXT("held"));
			const APawn* P = GetWorld() && GetWorld()->GetFirstPlayerController() ? GetWorld()->GetFirstPlayerController()->GetPawn() : nullptr;
			const USurvivalComponent* Sv = P ? P->FindComponentByClass<USurvivalComponent>() : nullptr;
			DeathZoneMsgElev = Sv ? Sv->GetSnapshot().ElevationMeters : 0.f;
			Note(FString::Printf(TEXT("death-zone message at %.0f m a.s.l.%s"), DeathZoneMsgElev, bDeathZoneHeld ? TEXT(" (HELD BACK: a story was running)") : TEXT("")));
		}
		if (L.StartsWith(TEXT("Expedition: summit beat"))) { BeatLog += (BeatLog.IsEmpty() ? TEXT("") : TEXT("; ")) + L.Mid(12); bBeatDone |= L.Contains(TEXT("done")); }
	}
}

// ------------------------------------------------------------------------------------------------ the report

void UYggSummitBot::Finish(const FString& Why)
{
	if (Phase == EPhase::Idle || Phase == EPhase::Quitting) { return; }
	EndWhy = Why;
	Note(FString::Printf(TEXT("end: %s"), *Why));
	if (OffRopeFrom >= 0.f) { OffRope.Add(FVector2D(OffRopeFrom, S / 100.f)); OffRopeFrom = -1.f; }
	UWorld* W = GetWorld();
	APawn* P = W && W->GetFirstPlayerController() ? W->GetFirstPlayerController()->GetPawn() : nullptr;
	if (USurvivalComponent* Sv = P ? P->FindComponentByClass<USurvivalComponent>() : nullptr) { Sv->SetSprintRequested(false); }
	DrainLog();
	WriteFiles(true);
	if (LogDevice.IsValid() && GLog) { GLog->RemoveOutputDevice(LogDevice.Get()); }
	if (bMaxFpsSet)
	{
		if (IConsoleVariable* V = IConsoleManager::Get().FindConsoleVariable(TEXT("t.MaxFPS"))) { V->Set(*PrevMaxFps, ECVF_SetByConsole); }
		bMaxFpsSet = false;
	}
	if (CVarSummitBotQuit.GetValueOnGameThread() != 0) { Phase = EPhase::Quitting; PhaseT = 0.0; }
	else { Phase = EPhase::Idle; }
}

FString UYggSummitBot::BuildReport(bool bFinal) const
{
	TStringBuilder<16384> R;
	R.Appendf(TEXT("# Summit robot run %s\n\n"), *RunName);
	R.Appendf(TEXT("- Result: %s\n"), bFinal ? *EndWhy : TEXT("(running)"));
	R.Appendf(TEXT("- Now: %s, game %s, %.0f m along leg 7 of %.0f m\n"), PhaseName(uint8(Phase)), *SummitBotClock(GameT), S / 100.f, RouteLen / 100.f);
	R.Appendf(TEXT("- Mode: %s, %s; stay %.0f min, down %.0f m\n"), bNoO2 ? TEXT("no oxygen") : TEXT("bottled oxygen (a full bottle on at the start + one spare, never more)"),
		bFast ? TEXT("sprinting whenever stamina allows (fast)") : TEXT("plain walk (no sprint)"), StayMin, DownM);
	R.Appendf(TEXT("- Start: camp jump to %s; oxygen pickups within 40 m of it: %d\n"), *SouthColName, PickupsAtCol);
	R.Appendf(TEXT("- World Tree: %s\n\n"), *TreeInfo);

	R.Append(TEXT("## Times\n\n| | game | real |\n|---|---|---|\n"));
	if (UpStartT >= 0.0 && SummitT >= 0.0)
	{
		R.Appendf(TEXT("| South Col -> summit | **%s** | %s |\n"), *SummitBotClock(SummitT - UpStartT), *SummitBotClock(SummitReal - UpStartReal));
		R.Appendf(TEXT("| summit beat -> summit screen | %.1f s | |\n"), MenuT >= 0.0 ? MenuT - SummitT : -1.0);
	}
	else if (UpStartT >= 0.0) { R.Appendf(TEXT("| climbing (summit not reached) | %s | |\n"), *SummitBotClock(GameT - UpStartT)); }
	if (DownStartT >= 0.0) { R.Appendf(TEXT("| down %.0f m of route | %s | |\n"), (DownStartS - S) / 100.f, *SummitBotClock((DownEndT >= 0.0 ? DownEndT : GameT) - DownStartT)); }
	if (StayStartT >= 0.0) { R.Appendf(TEXT("| standing at the summit | %s | |\n"), *SummitBotClock((DeathT >= 0.0 ? DeathT : GameT) - StayStartT)); }
	if (DeathT >= 0.0) { R.Appendf(TEXT("| death after the climb started | **%s** (%.0f m a.s.l.: %s) | |\n"), *SummitBotClock(DeathT - UpStartT), DeathElev, *DeathCause); }
	if (UpStartT >= 0.0 && SummitT >= 0.0 && SummitT > UpStartT)
	{
		const float ClimbSec = float(SummitT - UpStartT);
		R.Appendf(TEXT("\nAverage along the route %.2f m/s; %.0f m of height in %.1f min.\n"), RouteLen / 100.f / ClimbSec,
			SummitElev - StartElev, ClimbSec / 60.f);
	}

	R.Append(TEXT("\n## Route, rope, ground\n\n"));
	R.Appendf(TEXT("- Clipped to a fixed rope %.0f s, off %.0f s while walking (%.0f%% on)\n"), RopeOnSec, RopeOffSec, RopeOnSec + RopeOffSec > 0.f ? 100.f * RopeOnSec / (RopeOnSec + RopeOffSec) : 0.f);
	R.Append(TEXT("- Off-rope stretches (m along leg 7): "));
	for (int32 i = 0; i < OffRope.Num() && i < 40; ++i) { R.Appendf(TEXT("%s%.0f-%.0f"), i ? TEXT(", ") : TEXT(""), OffRope[i].X, OffRope[i].Y); }
	if (OffRope.Num() == 0) { R.Append(TEXT("none")); }
	R.Appendf(TEXT("\n- Furthest off the route: %.1f m; falls / slides over 0.75 s: %d (%.1f s falling in all); largest floor gap while walking %.1f cm\n"), MaxLat, Falls, FallSec, MaxFloor);
	R.Appendf(TEXT("- Stuck teleports: %d; sidestep jumps: %d; ground probes: %d (Saved/Review/ground_probe.csv, labels A5_*)\n"), Teleports, Sidesteps, Probes);

	R.Append(TEXT("\n## Body\n\n"));
	R.Appendf(TEXT("- Death-zone message: %s\n"), DeathZoneMsgT >= 0.0 ? *FString::Printf(TEXT("%s at %.0f m (%s)%s"), *SummitBotClock(DeathZoneMsgT), DeathZoneMsgElev, *DeathZoneMsg.Left(120), bDeathZoneHeld ? TEXT(" HELD BACK") : TEXT("")) : TEXT("not seen"));
	const auto At = [this](double T) { return T >= 0.0 ? FString::Printf(TEXT("from %s"), *SummitBotClock(T - FMath::Max(0.0, UpStartT))) : FString(TEXT("never")); };
	R.Appendf(TEXT("- On the view (0..1, max): thin-air tunnel %.2f (%s), frost %.2f (%s), spent %.2f (%s), faint %.2f (%s)\n"), MaxHyp, *At(FirstHypT), MaxFrost, *At(FirstFrostT),
		MaxExh, *At(FirstExhT), MaxFaint, *At(FirstFaintT));
	R.Appendf(TEXT("- Lowest SpO2 %.1f %%, highest heart rate %.0f bpm, lowest health %.1f (first damage %s), lowest core %.2f C\n"), MinSpO2, MaxHeart, MinHealth, *At(FirstDamageT), MinCore);

	// the climb by height bands
	R.Append(TEXT("\n| m a.s.l. | game time | m along | SpO2 | heart | health | stamina | core | bottle s | rope | tunnel | frost | speed m/s |\n|---|---|---|---|---|---|---|---|---|---|---|---|---|\n"));
	int32 NextBand = 7900;
	for (const FSample& X : Samples)
	{
		if (X.Phase != uint8(EPhase::Up) || X.Elev < NextBand) { continue; }
		R.Appendf(TEXT("| %.0f | %s | %.0f | %.1f | %.0f | %.1f | %.0f | %.2f | %.0f | %d | %.2f | %.2f | %.2f |\n"), X.Elev, *SummitBotClock(X.T - UpStartT), X.S, X.SpO2, X.Heart, X.Health, X.Stamina, X.Core,
			X.Bottle, X.bRope ? 1 : 0, X.Hyp, X.Frost, X.Speed / 100.f);
		NextBand = (int32(X.Elev) / 100 + 1) * 100;
	}
	if (StayStartT >= 0.0)
	{
		R.Append(TEXT("\nAt the summit, every 30 s:\n\n| standing | SpO2 | heart | health | core | faint |\n|---|---|---|---|---|---|\n"));
		double NextT = StayStartT;
		for (const FSample& X : Samples)
		{
			if (X.Phase != uint8(EPhase::Stay) || X.T < NextT) { continue; }
			R.Appendf(TEXT("| %s | %.1f | %.0f | %.1f | %.2f | %.2f |\n"), *SummitBotClock(X.T - StayStartT), X.SpO2, X.Heart, X.Health, X.Core, X.Faint);
			NextT = X.T + 30.0;
		}
	}

	R.Append(TEXT("\n## Summit beat\n\n"));
	R.Appendf(TEXT("- Story chapter Summit running: %s; %s\n"), bBeatStarted ? TEXT("yes") : TEXT("no"), BeatLog.IsEmpty() ? TEXT("(no beat log line)") : *BeatLog);
	R.Appendf(TEXT("- View rig during the beat: Move lease %s, nudge %s, attention %s\n"), bSawMoveLease ? TEXT("yes") : TEXT("no"), bSawNudge ? TEXT("yes") : TEXT("no"), bSawAttention ? TEXT("yes") : TEXT("no"));

	R.Append(TEXT("\n## Issues\n\n"));
	if (Issues.Num() == 0) { R.Append(TEXT("none\n")); }
	for (const FIssue& I : Issues) { R.Appendf(TEXT("- %s %s at %.0f m along, %.0f m a.s.l. (%s): %s\n"), *SummitBotClock(I.T), *I.Kind, I.S, I.Elev, *SummitBotCm(I.Pos), *I.Text); }

	// frames
	if (FrameMs.Num() > 10)
	{
		TArray<float> F = FrameMs;
		F.Sort();
		double Sum = 0.0;
		for (float V : F) { Sum += V; }
		const float P99 = F[FMath::Clamp(int32(F.Num() * 0.99), 0, F.Num() - 1)];
		const float Med = F[F.Num() / 2];
		R.Appendf(TEXT("\n## Frames\n\n- %d frames: average %.1f fps, median %.1f ms, 99th percentile %.1f ms (1%% low %.1f fps), worst %.0f ms\n"), F.Num(), 1000.0 * F.Num() / Sum, Med, P99, 1000.f / P99, F.Last());
		if (ProbeMs.Num() > 0)
		{
			float WorstProbe = 0.f;
			for (float V : ProbeMs) { WorstProbe = FMath::Max(WorstProbe, V); }
			R.Appendf(TEXT("- ground-probe frames (left out above): %d, worst %.0f ms\n"), ProbeMs.Num(), WorstProbe);
		}
	}

	R.Append(TEXT("\n## Pictures\n\n"));
	for (const FShot& X : Shots) { R.Appendf(TEXT("- %s  %s (%.0f m): %s\n"), *SummitBotClock(X.T), *X.Stage, X.Elev, *X.File); }
	R.Append(TEXT("\n## Expedition and story lines\n\n"));
	for (const FString& L : ExpeditionLines) { R.Appendf(TEXT("- %s\n"), *L); }
	R.Append(TEXT("\n## Events\n\n"));
	for (const FString& L : Events) { R.Appendf(TEXT("- %s\n"), *L); }
	return FString(R.ToString());
}

void UYggSummitBot::WriteFiles(bool bFinal)
{
	if (Dir.IsEmpty()) { return; }
	FFileHelper::SaveStringToFile(BuildReport(bFinal), *(Dir / (bFinal ? TEXT("report.md") : TEXT("progress.md"))), FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
	TStringBuilder<65536> C;
	C.Append(TEXT("t,real,phase,s_m,lat_m,x,y,z,elev_m,speed_cms,maxwalk_cms,falling,walkable,floor_cm,rope,rope_m,sheltered,sprint,spo2,heart,health,stamina,core,bottle_s,bottles,storm,hyp,frost,exhaust,faint\n"));
	for (const FSample& X : Samples)
	{
		C.Appendf(TEXT("%.1f,%.1f,%s,%.1f,%.2f,%.0f,%.0f,%.0f,%.1f,%.0f,%.0f,%d,%d,%.1f,%d,%.2f,%d,%d,%.2f,%.1f,%.2f,%.1f,%.3f,%.0f,%d,%.2f,%.3f,%.3f,%.3f,%.3f\n"),
			X.T, X.Real, PhaseName(X.Phase), X.S, X.Lat, X.Pos.X, X.Pos.Y, X.Pos.Z, X.Elev, X.Speed, X.MaxSpeed, X.bFalling ? 1 : 0, X.bWalkable ? 1 : 0, X.FloorDist,
			X.bRope ? 1 : 0, X.RopeDist, X.bSheltered ? 1 : 0, X.bSprint ? 1 : 0, X.SpO2, X.Heart, X.Health, X.Stamina, X.Core, X.Bottle, X.Bottles, X.Storm, X.Hyp, X.Frost, X.Exh, X.Faint);
	}
	FFileHelper::SaveStringToFile(FString(C.ToString()), *(Dir / TEXT("samples.csv")), FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
	if (bFinal) { FFileHelper::SaveStringToFile(EndWhy + TEXT("\n"), *(Dir / TEXT("done.txt"))); }
}
