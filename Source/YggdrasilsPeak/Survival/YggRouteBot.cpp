// The robot player's route walk: see YggRouteBot.h. Tools/Bot/run_bot.ps1 -Chapter route runs it; Tools/Bot/README.md.

#include "Survival/YggRouteBot.h"
#include "Survival/ExpeditionComponent.h"
#include "Survival/SurvivalComponent.h"
#include "Survival/SurvivalTypes.h"
#include "Survival/StoryClue.h"
#include "Survival/YggTime.h"
#include "Survival/YggViewRig.h"
#include "Environment/CrevasseCrossing.h"
#include "Environment/FixedRope.h"
#include "Environment/RouteLandmark.h"
#include "Environment/TrailCheckpoint.h"

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
#include "GameFramework/WorldSettings.h"
#include "HAL/FileManager.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformTime.h"
#include "IImageWrapper.h"
#include "IImageWrapperModule.h"
#include "Misc/App.h"
#include "Misc/FileHelper.h"
#include "Misc/OutputDevice.h"
#include "Misc/OutputDeviceRedirector.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"
#include "PhysicsEngine/BodySetup.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "UnrealClient.h"

DEFINE_LOG_CATEGORY_STATIC(LogYggRouteBot, Log, All);

/** The expedition's own lines (camps reached, rope, deaths) handed to the game thread. */
class FYggRouteBotLog : public FOutputDevice
{
public:
	FYggRouteBotLog() { bAutoEmitLineTerminator = false; }
	virtual bool CanBeUsedOnAnyThread() const override { return true; }
	virtual bool CanBeUsedOnMultipleThreads() const override { return true; }
	TQueue<FString, EQueueMode::Mpsc> Lines;
	virtual void Serialize(const TCHAR* V, ELogVerbosity::Type Verbosity, const FName& Category) override
	{
		if (!V) { return; }
		if (FCString::Strncmp(V, TEXT("Expedition"), 10) == 0 || FCString::Strncmp(V, TEXT("CamRec: INCIDENT"), 16) == 0
			|| FCString::Strncmp(V, TEXT("GroundProbe"), 11) == 0)
		{
			Lines.Enqueue(FString(V));
		}
	}
};

namespace
{
	TAutoConsoleVariable<int32> CVarRouteBotQuit(TEXT("ygg.Bot.Route.Quit"), 1, TEXT("Route robot: 1 = quit the game when the run ends."));

	FAutoConsoleCommandWithWorldAndArgs RouteBotCmd(TEXT("ygg.Bot.Route"),
		TEXT("ygg.Bot.Route [from <leg>] [to <leg>] [fast] [speed <x>] [at <m>] [cap <min>] [every <m>] [time <hhmm>|-1] [noshots] - the robot walks route.json legs from..to (default 0..3, Dingboche -> Camp 1), checks the ground and the guidance every 200 m, writes Saved/Bot/route_<time>/report.md and quits."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			UYggRouteBot* Bot = World ? World->GetSubsystem<UYggRouteBot>() : nullptr;
			if (!Bot) { UE_LOG(LogYggRouteBot, Warning, TEXT("RouteBot: no robot in this world (run it in the game or PIE)")); return; }
			Bot->Run(Args);
		}));
	FAutoConsoleCommandWithWorld RouteBotStop(TEXT("ygg.Bot.Route.Stop"), TEXT("Stop the route robot: final report, no quit."),
		FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World)
		{
			if (UYggRouteBot* Bot = World ? World->GetSubsystem<UYggRouteBot>() : nullptr) { Bot->StopRun(TEXT("stopped (ygg.Bot.Route.Stop)")); }
		}));

	FString Clock(double S)
	{
		const int32 T = FMath::Max(0, FMath::RoundToInt(S));
		return T >= 3600 ? FString::Printf(TEXT("%d:%02d:%02d"), T / 3600, (T / 60) % 60, T % 60) : FString::Printf(TEXT("%d:%02d"), T / 60, T % 60);
	}
	FString RouteBotCm(const FVector& V) { return FString::Printf(TEXT("%.0f %.0f %.0f"), V.X, V.Y, V.Z); }
	FString ShortCamp(FString Name)
	{
		Name.Split(TEXT(" - "), &Name, nullptr); Name.Split(TEXT(" ("), &Name, nullptr);   // as the HUD writes it
		Name.ReplaceInline(TEXT("Everest "), TEXT(""));
		return Name;
	}
	FString Csv(FString V) { V.ReplaceInline(TEXT(","), TEXT(";")); V.ReplaceInline(TEXT("\n"), TEXT(" ")); return V; }
}

// ------------------------------------------------------------------------------------------------ lifetime

bool UYggRouteBot::ShouldCreateSubsystem(UObject* Outer) const
{
	const UWorld* W = Cast<UWorld>(Outer);
	return W && (W->WorldType == EWorldType::Game || W->WorldType == EWorldType::PIE) && Super::ShouldCreateSubsystem(Outer);
}

void UYggRouteBot::Deinitialize()
{
	if (Phase != EPhase::Idle && Phase != EPhase::Quitting) { Finish(TEXT("the world closed during the run")); }
	if (LogDevice.IsValid() && GLog) { GLog->RemoveOutputDevice(LogDevice.Get()); }
	if (ShotHandle.IsValid()) { UGameViewportClient::OnScreenshotCaptured().Remove(ShotHandle); ShotHandle.Reset(); }
	Phase = EPhase::Idle;
	Super::Deinitialize();
}

void UYggRouteBot::Run(const TArray<FString>& Args)
{
	if (Phase != EPhase::Idle) { UE_LOG(LogYggRouteBot, Warning, TEXT("RouteBot: already running")); return; }
	FromLeg = 0; ToLeg = 3; bFast = false; bShots = true;
	SpeedX = 1.f; AtM = 0.f; CapMin = 240.f; EveryM = 200.f; HoldTime = 1100.f;
	for (int32 i = 0; i < Args.Num(); ++i)
	{
		const FString A = Args[i].ToLower();
		const float Next = i + 1 < Args.Num() ? FCString::Atof(*Args[i + 1]) : 0.f;
		if (A == TEXT("fast")) { bFast = true; }
		else if (A == TEXT("noshots")) { bShots = false; }
		else if (A == TEXT("from")) { FromLeg = FMath::Clamp(int32(Next), 0, 7); ++i; }
		else if (A == TEXT("to")) { ToLeg = FMath::Clamp(int32(Next), 0, 7); ++i; }
		else if (A == TEXT("speed")) { SpeedX = FMath::Clamp(Next, 0.25f, 8.f); ++i; }
		else if (A == TEXT("at")) { AtM = FMath::Max(0.f, Next); ++i; }
		else if (A == TEXT("cap")) { CapMin = FMath::Max(5.f, Next); ++i; }
		else if (A == TEXT("every")) { EveryM = FMath::Max(25.f, Next); ++i; }
		else if (A == TEXT("time")) { HoldTime = Next; ++i; }
	}
	ToLeg = FMath::Max(ToLeg, FromLeg);
	GameT = 0.0; RealStart = FPlatformTime::Seconds(); LastReport = RealStart; PhaseT = 0.0;
	WalkStartT = ArriveT = DeathT = -1.0;
	Samples.Reset(); Issues.Reset(); Shots.Reset(); Points.Reset(); Events.Reset(); LogLines.Reset(); FrameMs.Reset(); FrameMark = 0;
	LegStartT.Reset(); LegStartReal.Reset(); Camps.Reset();
	Teleports = Sidesteps = Probes = Falls = Slides = TopUps = Detours = SeracWarnings = 0; bBlockNoted = false; DetourUntil = -100.0; SprintUntil = -100.0;
	RopeOnSec = RopeOffSec = FallSec = MaxLat = MaxFloor = 0.f;
	bLostNoted = bFloatNoted = bUnderNoted = false; FallFor = LostFor = FloatFor = UnderFor = 0.f;
	EndWhy.Reset(); DeathCause.Reset(); LastCampSeen.Reset();
	NextSample = NextCheck = NextTopUp = 0.0;
	const FString Stamp = FDateTime::Now().ToString(TEXT("%Y%m%d_%H%M%S"));
	RunName = FString::Printf(TEXT("route_%s_L%d-%d%s"), *Stamp, FromLeg, ToLeg, bFast ? TEXT("_fast") : TEXT(""));
	Dir = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("Bot") / RunName);
	ShotDir = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("Review") / TEXT("A4"));
	IFileManager::Get().MakeDirectory(*Dir, true);
	IFileManager::Get().MakeDirectory(*ShotDir, true);
	if (!LoadRoute()) { Phase = EPhase::Waiting; Finish(FString::Printf(TEXT("cannot read route.json legs %d..%d"), FromLeg, ToLeg)); return; }
	if (!LogDevice.IsValid()) { LogDevice = MakeShared<FYggRouteBotLog>(); }
	else { FString Stale; while (LogDevice->Lines.Dequeue(Stale)) {} }
	if (GLog) { GLog->AddOutputDevice(LogDevice.Get()); }
	if (bFast)
	{
		if (IConsoleVariable* V = IConsoleManager::Get().FindConsoleVariable(TEXT("t.MaxFPS"))) { PrevMaxFps = V->GetString(); bMaxFpsSet = true; V->Set(TEXT("0"), ECVF_SetByConsole); }
	}
	Phase = EPhase::Waiting;
	Note(FString::Printf(TEXT("run %s: legs %d..%d, %d points, %.0f m; %s, game speed x%.2f, a checkpoint every %.0f m%s, cap %.0f min"),
		*RunName, FromLeg, ToLeg, Route.Num(), RouteLen / 100.f, bFast ? TEXT("sprint when possible") : TEXT("walk"), SpeedX, EveryM,
		bShots ? TEXT(" with a picture") : TEXT(""), CapMin));
}

void UYggRouteBot::StopRun(const FString& Why)
{
	if (Phase != EPhase::Idle && Phase != EPhase::Quitting)
	{
		CVarRouteBotQuit->Set(0, ECVF_SetByConsole);
		Finish(Why);
	}
}

void UYggRouteBot::Note(const FString& Text)
{
	const FString L = FString::Printf(TEXT("%s  %s"), *Clock(GameT), *Text);
	if (Events.Num() < 3000) { Events.Add(L); }
	UE_LOG(LogYggRouteBot, Display, TEXT("RouteBot %s"), *L);
}

void UYggRouteBot::Issue(const FString& Kind, const FString& Text, APawn* P)
{
	FIssue I;
	I.T = GameT; I.S = S / 100.f; I.Kind = Kind; I.Text = Text; I.Leg = CurLeg;
	if (P) { I.Pos = P->GetActorLocation(); }
	if (const USurvivalComponent* Sv = P ? P->FindComponentByClass<USurvivalComponent>() : nullptr) { I.Elev = Sv->GetSnapshot().ElevationMeters; }
	if (Issues.Num() < 1000) { Issues.Add(I); }
	Note(FString::Printf(TEXT("ISSUE %s at %.0f m along (leg %d, %.0f m a.s.l.): %s"), *Kind, I.S, CurLeg, I.Elev, *Text));
}

// ------------------------------------------------------------------------------------------------ the route

bool UYggRouteBot::LoadRoute()
{
	Route.Reset(); Cum.Reset(); PtLeg.Reset(); LegStartS.Reset();
	FString Raw;
	TArray<TSharedPtr<FJsonValue>> Legs;
	const FString Path = FPaths::ProjectDir() / TEXT("Tools/RoutePlanning/route.json");
	if (!FFileHelper::LoadFileToString(Raw, *Path) || !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Raw), Legs)) { return false; }
	for (int32 L = FromLeg; L <= ToLeg; ++L)
	{
		if (!Legs.IsValidIndex(L) || !Legs[L].IsValid() || Legs[L]->Type != EJson::Object) { return false; }
		const TArray<TSharedPtr<FJsonValue>>* Pts;
		if (!Legs[L]->AsObject()->TryGetArrayField(TEXT("pts"), Pts)) { return false; }
		LegStartS.Add(float(Route.Num()));   // the index for now: turned into a distance below
		for (const TSharedPtr<FJsonValue>& V : *Pts)
		{
			if (!V.IsValid() || V->Type != EJson::Array) { continue; }
			const TArray<TSharedPtr<FJsonValue>>& A = V->AsArray();
			if (A.Num() < 3) { continue; }
			const FVector P(A[0]->AsNumber(), A[1]->AsNumber(), A[2]->AsNumber());
			// the joint between two legs: a leg starts a few metres off the last one's end (on the camp's other side), and
			// walking back to it made the robot turn on the spot at Gorak Shep (2026-11-20); its first points near the joint go
			if (Route.Num() && (FVector::Dist2D(Route.Last(), P) < 50.0 || (PtLeg.Last() != L && FVector::Dist2D(Route.Last(), P) < 1500.0))) { continue; }
			Route.Add(P);
			PtLeg.Add(L);
		}
	}
	Cum.SetNum(Route.Num());
	for (int32 i = 0; i < Route.Num(); ++i) { Cum[i] = i == 0 ? 0.f : Cum[i - 1] + float(FVector::Dist2D(Route[i - 1], Route[i])); }
	RouteLen = Route.Num() > 1 ? Cum.Last() : 0.f;
	for (float& L : LegStartS) { L = Cum.IsValidIndex(int32(L)) ? Cum[int32(L)] : RouteLen; }
	return Route.Num() > 1;
}

void UYggRouteBot::Locate(const FVector& Pos)
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
	CurLeg = LegAt(S);
}

void UYggRouteBot::Seed(const FVector& Pos)
{
	// after a camp jump or a teleport: the nearest segment of the whole route
	float BestD = 1e12f; int32 BestI = 0;
	for (int32 i = 0; i + 1 < Route.Num(); ++i)
	{
		const FVector2D A(Route[i]), B(Route[i + 1]), P(Pos);
		const FVector2D AB = B - A;
		const float L2 = float(AB.SizeSquared());
		const float T = L2 > 1.f ? FMath::Clamp(float(FVector2D::DotProduct(P - A, AB)) / L2, 0.f, 1.f) : 0.f;
		const float D = float(FVector2D::Distance(P, A + AB * T));
		if (D < BestD) { BestD = D; BestI = i; }
	}
	Seg = BestI;
	Locate(Pos);
}

FVector UYggRouteBot::PointAt(float AtS) const
{
	AtS = FMath::Clamp(AtS, 0.f, RouteLen);
	int32 i = 0;
	while (i < Route.Num() - 2 && Cum[i + 1] < AtS) { ++i; }
	const float L = FMath::Max(Cum[i + 1] - Cum[i], 1.f);
	return FMath::Lerp(Route[i], Route[i + 1], FMath::Clamp((AtS - Cum[i]) / L, 0.f, 1.f));
}

FVector UYggRouteBot::DirAt(float AtS) const
{
	FVector D = PointAt(AtS + 1500.f) - PointAt(AtS - 500.f);
	D.Z = 0.f;
	return D.GetSafeNormal();
}

int32 UYggRouteBot::LegAt(float AtS) const
{
	int32 L = FromLeg;
	for (int32 k = 0; k < LegStartS.Num(); ++k) { if (AtS >= LegStartS[k] - 1.f) { L = FromLeg + k; } }
	return L;
}

// ------------------------------------------------------------------------------------------------ every frame

void UYggRouteBot::Tick(float DeltaTime)
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
			if (PC) { PC->ConsoleCommand(TEXT("quit")); } else { FPlatformMisc::RequestExit(false, TEXT("ygg.Bot.Route")); }
		}
		return;
	}
	GameT += DeltaTime;
	PhaseT += DeltaTime;
	DrainLog();
	// the frame's real time (time dilation stretches game time, not the frame)
	const float FrameReal = RealDt * 1000.f;
	if (Phase == EPhase::Walk && FrameReal > 0.f && FrameMs.Num() < 2000000) { FrameMs.Add(FrameReal); }
	if (GameT > CapMin * 60.0) { Finish(FString::Printf(TEXT("cap: %.0f min of game time"), CapMin)); return; }

	if (Phase == EPhase::Waiting)
	{
		if (P && GameT >= 3.0) { Begin(P); }
		else if (FPlatformTime::Seconds() - RealStart > 240.0) { Finish(TEXT("no player pawn after 4 minutes")); }
		return;
	}
	if (!P) { return; }
	USurvivalComponent* Sv = P->FindComponentByClass<USurvivalComponent>();
	UExpeditionComponent* Ex = P->FindComponentByClass<UExpeditionComponent>();

	// a player eats and drinks on a walk of hours: the bot tops food and water up (the walk is what is measured)
	if (Sv && GameT >= NextTopUp)
	{
		NextTopUp = GameT + 30.0;
		const FSurvivalSnapshot Snap = Sv->GetSnapshot();
		if (Snap.Satiety < 40.f) { Sv->SetStat(ESurvivalStat::Satiety, 90.f); ++TopUps; }
		if (Snap.Hydration < 40.f) { Sv->SetStat(ESurvivalStat::Hydration, 90.f); ++TopUps; }
	}
	// the camp the expedition counts as reached
	if (Ex && Ex->GetCampName() != LastCampSeen)
	{
		LastCampSeen = Ex->GetCampName();
		if (!LastCampSeen.IsEmpty()) { Note(FString::Printf(TEXT("checkpoint: %s"), *LastCampSeen)); }
	}

	if (Sv && Sv->IsDead())
	{
		if (DeathT < 0.0)
		{
			DeathT = GameT;
			DeathCause = Sv->GetDeathCause().ToString();
			MoveDir = FVector::ZeroVector;
			Issue(TEXT("death"), FString::Printf(TEXT("died: %s"), *DeathCause), P);
		}
		if (GameT - DeathT > 3.5 && !Shots.ContainsByPredicate([](const FShot& X) { return X.Stage == TEXT("death"); })) { Shot(TEXT("death"), true); }
		if (GameT - DeathT > 5.0) { Finish(FString::Printf(TEXT("died at %.0f m along: %s"), S / 100.f, *DeathCause)); }
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
			Seed(P->GetActorLocation());
			Note(FString::Printf(TEXT("on the ground at %s, %.0f m a.s.l., %.0f m along, %.1f m off the route"), *RouteBotCm(P->GetActorLocation()),
				Sv ? Sv->GetSnapshot().ElevationMeters : 0.f, S / 100.f, Lat / 100.f));
			if (AtM > 0.f && TeleportAlong(P, AtM * 100.f, TEXT("at"))) { Seed(P->GetActorLocation()); }
			// game speed: the world's time dilation (the "slomo" console command is a cheat and does nothing in the game build)
			if (SpeedX != 1.f) { if (AWorldSettings* WS = W->GetWorldSettings()) { WS->MaxGlobalTimeDilation = FMath::Max(WS->MaxGlobalTimeDilation, SpeedX); WS->SetTimeDilation(SpeedX); } }
			StartS = S;
			BestS = S;
			LastProgressT = GameT;
			NextPointS = FMath::FloorToFloat(S / (EveryM * 100.f)) * EveryM * 100.f;   // a checkpoint right at the start
			WalkStartT = GameT;
			WalkStartReal = FPlatformTime::Seconds();
			LegStartT.Init(-1.0, LegStartS.Num());
			LegStartReal.Init(0.0, LegStartS.Num());
			const int32 K = CurLeg - FromLeg;
			if (LegStartT.IsValidIndex(K)) { LegStartT[K] = GameT; LegStartReal[K] = WalkStartReal; }
			Phase = EPhase::Walk;
			PhaseT = 0.0;
			Note(TEXT("walk starts"));
		}
		else if (PhaseT > 40.0) { Finish(TEXT("never reached the ground at the start camp")); return; }
		break;
	}
	case EPhase::Walk:
	{
		// the view rig's HardReset (each stuck teleport) puts the time dilation back to 1: keep the game speed
		if (SpeedX != 1.f) { if (AWorldSettings* WS = W->GetWorldSettings()) { if (!FMath::IsNearlyEqual(WS->TimeDilation, SpeedX, 0.01f)) { WS->SetTimeDilation(SpeedX); } } }
		const int32 Before = CurLeg;
		Walk(P, DeltaTime);
		if (CurLeg != Before)
		{
			const int32 K = CurLeg - FromLeg;
			if (LegStartT.IsValidIndex(K) && LegStartT[K] < 0.0) { LegStartT[K] = GameT; LegStartReal[K] = FPlatformTime::Seconds(); }
			Note(FString::Printf(TEXT("leg %d begins at %.0f m along (%s)"), CurLeg, S / 100.f, Camps.IsValidIndex(CurLeg) ? *Camps[CurLeg].Name : TEXT("?")));
			if (bShots) { Shot(FString::Printf(TEXT("leg%d_start"), CurLeg), true); }
		}
		if (S >= RouteLen - 150.f)
		{
			ArriveT = GameT;
			MoveDir = FVector::ZeroVector;
			if (Sv) { Sv->SetSprintRequested(false); }
			Note(FString::Printf(TEXT("ROUTE END reached after %s game time (%s real)"), *Clock(ArriveT - WalkStartT), *Clock(FPlatformTime::Seconds() - WalkStartReal)));
			Phase = EPhase::Arrive;
			PhaseT = 0.0;
		}
		break;
	}
	case EPhase::Arrive:
	{
		Check(P, DeltaTime);
		const FString Want = Camps.IsValidIndex(ToLeg + 1) ? Camps[ToLeg + 1].Name : FString();
		const bool bReached = Ex && !Want.IsEmpty() && Ex->GetCampName() == Want;
		if (PhaseT > 3.0 && bShots && !Shots.ContainsByPredicate([](const FShot& X) { return X.Stage == TEXT("arrive"); })) { Shot(TEXT("arrive"), true); }
		if (bReached && PhaseT > 5.0) { Finish(FString::Printf(TEXT("route finished: %s reached"), *Want)); return; }
		if (PhaseT > 25.0)
		{
			Issue(TEXT("camp"), FString::Printf(TEXT("route end reached but the expedition's checkpoint is '%s', not '%s'"), Ex ? *Ex->GetCampName() : TEXT("?"), *Want), P);
			Finish(TEXT("route finished: the route end reached (camp checkpoint missing)"));
			return;
		}
		break;
	}
	default: break;
	}
	if (GameT >= NextSample) { NextSample = GameT + 1.0; Sample(P); }
	if (PC) { Look(PC, FMath::Min(RealDt, 0.1f)); }
	const double Now = FPlatformTime::Seconds();
	if (Now - LastReport > 60.0) { LastReport = Now; WriteFiles(false); }
}

void UYggRouteBot::Begin(APawn* P)
{
	UExpeditionComponent* Ex = P->FindComponentByClass<UExpeditionComponent>();
	if (!Ex) { Finish(TEXT("no ExpeditionComponent on the player")); return; }
	FString List;
	for (const TWeakObjectPtr<ARouteLandmark>& C : Ex->GetCamps())
	{
		if (!C.IsValid()) { continue; }
		FCamp X; X.Name = C->LandmarkName; X.Short = ShortCamp(C->LandmarkName); X.Pos = C->GetActorLocation();
		Camps.Add(X);
		List += FString::Printf(TEXT("%s%d %s"), List.IsEmpty() ? TEXT("") : TEXT(", "), Camps.Num() - 1, *X.Name);
	}
	Note(FString::Printf(TEXT("%d camps: %s"), Camps.Num(), *List));
	const TArray<TWeakObjectPtr<ARouteLandmark>>& Raw = Ex->GetCamps();
	if (!Raw.IsValidIndex(FromLeg) || !Raw[FromLeg].IsValid()) { Finish(FString::Printf(TEXT("no camp %d to start from"), FromLeg)); return; }
	if (HoldTime >= 0.f) { YggTime::Set(GetWorld(), HoldTime); }
	Crossings.Reset();
	for (TActorIterator<ACrevasseCrossing> It(GetWorld()); It; ++It) { Crossings.Add(*It); }
	Ex->TeleportToCamp(Raw[FromLeg].Get());
	Seg = 0;
	Note(FString::Printf(TEXT("camp jump to %s (%s)"), *Raw[FromLeg]->LandmarkName, *RouteBotCm(Raw[FromLeg]->GetActorLocation())));
	Phase = EPhase::Settle;
	PhaseT = 0.0;
}

FString UYggRouteBot::Blocker(const FHitResult& H) const
{
	const UPrimitiveComponent* C = H.GetComponent();
	FString Kind = TEXT("");
	if (const UStaticMeshComponent* SM = Cast<UStaticMeshComponent>(C))
	{
		const UStaticMesh* M = SM->GetStaticMesh();
		const UBodySetup* B = M ? M->GetBodySetup() : nullptr;
		if (B)
		{
			const TCHAR* Flag = B->CollisionTraceFlag == CTF_UseComplexAsSimple ? TEXT("complex as simple")
				: B->CollisionTraceFlag == CTF_UseSimpleAsComplex ? TEXT("simple as complex") : TEXT("simple + complex");
			Kind = FString::Printf(TEXT(" [mesh %s: %s; simple = %d convex, %d box, %d sphere, %d capsule]"), *M->GetName(), Flag,
				B->AggGeom.ConvexElems.Num(), B->AggGeom.BoxElems.Num(), B->AggGeom.SphereElems.Num(), B->AggGeom.SphylElems.Num());
		}
	}
	return FString::Printf(TEXT("%s / %s%s"), H.GetActor() ? *H.GetActor()->GetActorNameOrLabel() : TEXT("?"), *GetNameSafe(C), *Kind);
}

void UYggRouteBot::Walk(APawn* P, float Dt)
{
	Locate(P->GetActorLocation());
	// the icefall path is a 1.5 m trench between blocks on the route's 20 m grid: a short look-ahead keeps to its corners
	const float Ahead = CurLeg >= 2 ? 300.f : 700.f;
	FVector Target = PointAt(S + Ahead);
	// a crevasse ladder: line up on its axis 15 m before it and walk straight over (the fixed rope does the same; the
	// route polyline turns on a 20 m grid, and cutting its corner over Crevasse_1_leg2 dropped the robot into the slot)
	const FVector Pos = P->GetActorLocation();
	const FVector Fwd = DirAt(S);
	bool bNearLadder = false;
	for (const TWeakObjectPtr<AActor>& Cr : Crossings)
	{
		if (!Cr.IsValid()) { continue; }
		FVector Ax = Cr->GetActorForwardVector(); Ax.Z = 0.f; Ax = Ax.GetSafeNormal();
		if (FVector::DotProduct(Ax, Fwd) < 0.f) { Ax = -Ax; }
		const FVector D = Pos - Cr->GetActorLocation();
		const float Along = float(FVector::DotProduct(FVector(D.X, D.Y, 0.0), Ax));
		const float Side = float(FVector::CrossProduct(Ax, FVector(D.X, D.Y, 0.0)).Z);
		if (Along > -1500.f && Along < 500.f && FMath::Abs(Side) < 800.f)
		{
			// aim 1.5 m ahead on the axis (3 m let it reach the ladder still 1 m to the side: off the rungs into the slot)
			Target = Cr->GetActorLocation() + Ax * (Along + 150.f);
			bNearLadder = true;
			if (!bOnLadderLine) { bOnLadderLine = true; Note(FString::Printf(TEXT("lining up on %s (%.1f m to the side)"), *Cr->GetActorNameOrLabel(), Side / 100.f)); }
			break;
		}
	}
	bOnLadderLine &= bNearLadder;
	FVector Go = Target - P->GetActorLocation();
	Go.Z = 0.f;
	Go = Go.GetSafeNormal();
	if (S > BestS + 100.f) { BestS = S; LastProgressT = GameT; bBlockNoted = false; }
	const double NoProg = GameT - LastProgressT;
	ACharacter* C = Cast<ACharacter>(P);
	// a player walks round a stone or a tent: after 3 s held by an object (not the ground), 2.5 s to its freer side
	if (C && !bNearLadder && NoProg > 3.0 && NoProg < 8.0 && GameT >= DetourUntil && GameT - DetourUntil > 1.0)
	{
		UCapsuleComponent* Cap = C->GetCapsuleComponent();
		const FCollisionShape Sh = FCollisionShape::MakeCapsule(Cap->GetScaledCapsuleRadius(), Cap->GetScaledCapsuleHalfHeight() * 0.8f);
		FCollisionQueryParams DQ(SCENE_QUERY_STAT(YggRouteBotDetour), false, P);
		const FVector From = Pos + FVector(0, 0, Cap->GetScaledCapsuleHalfHeight() * 0.2f);
		FHitResult B;
		if (GetWorld()->SweepSingleByChannel(B, From, From + Go * 150.f, FQuat::Identity, ECC_Pawn, Sh, DQ) && B.GetActor() && B.ImpactNormal.Z < 0.7f
			&& !B.GetActor()->GetActorNameOrLabel().StartsWith(TEXT("DetailTile_")) && !B.GetActor()->GetActorNameOrLabel().StartsWith(TEXT("Landscape")))
		{
			float Free[2] = { 0.f, 0.f };
			for (int32 k = 0; k < 2; ++k)
			{
				const FVector D = Go.RotateAngleAxis(k == 0 ? 65.f : -65.f, FVector::UpVector);
				FHitResult HS;
				Free[k] = GetWorld()->SweepSingleByChannel(HS, From, From + D * 250.f, FQuat::Identity, ECC_Pawn, Sh, DQ) ? HS.Distance : 250.f;
			}
			DetourDir = Go.RotateAngleAxis(Free[0] >= Free[1] ? 65.f : -65.f, FVector::UpVector);
			DetourUntil = GameT + 2.5;
			++Detours;
			Note(FString::Printf(TEXT("walking round %s at %.0f m along"), *B.GetActor()->GetActorNameOrLabel(), S / 100.f));
		}
	}
	if (GameT < DetourUntil) { Go = DetourDir; }
	// the snow ramp up to a ladder can be steeper than the walkable angle at its foot (Crevasse_2_leg2, 46 deg over the
	// first 25 cm): held there for 2 s, hop up it as a player would, straight along the axis
	if (C && bNearLadder && NoProg > 2.0 && FMath::Fmod(NoProg, 1.5) < Dt * 1.5) { C->Jump(); ++Sidesteps; }
	if (NoProg > 8.0)
	{
		// stuck: a sidestep (2 s of every 6, alternating sides) and a jump, then a 10 m teleport along the route
		const double Ph = NoProg - 8.0;
		if (C && !bBlockNoted)
		{
			bBlockNoted = true;
			// what holds the climber: the capsule (simple collision, as the movement sees it) swept 1.5 m towards the target
			UCapsuleComponent* Cap = C->GetCapsuleComponent();
			FHitResult B;
			FCollisionQueryParams BQ(SCENE_QUERY_STAT(YggRouteBotBlock), false, P);
			const FVector From = P->GetActorLocation();
			if (GetWorld()->SweepSingleByChannel(B, From, From + Go * 150.f, FQuat::Identity, ECC_Pawn, FCollisionShape::MakeCapsule(Cap->GetScaledCapsuleRadius(), Cap->GetScaledCapsuleHalfHeight()), BQ))
			{
				Issue(TEXT("blocked"), FString::Printf(TEXT("blocked by %s (normal z %.2f, %.0f cm over the capsule bottom, %.0f cm ahead) at %s"), *Blocker(B), B.ImpactNormal.Z,
					B.ImpactPoint.Z - (From.Z - Cap->GetScaledCapsuleHalfHeight()), B.Distance, *RouteBotCm(From)), P);
			}
			else { Note(FString::Printf(TEXT("stuck at %.0f m along: nothing in the way 1.5 m ahead"), S / 100.f)); }
		}
		if (!bNearLadder && FMath::Fmod(Ph, 6.0) < 2.0) { Go = Go.RotateAngleAxis((int32(Ph / 6.0) % 2) == 0 ? 70.f : -70.f, FVector::UpVector); }
		if (C && FMath::Fmod(Ph, 6.0) < Dt * 1.5) { C->Jump(); ++Sidesteps; }
		if (NoProg > 30.0)
		{
			Issue(TEXT("stuck"), FString::Printf(TEXT("no progress for %.0f s at %s: teleported 10 m along the route"), NoProg, *RouteBotCm(P->GetActorLocation())), P);
			if (TeleportAlong(P, S + 1000.f, TEXT("stuck"))) { Seed(P->GetActorLocation()); }
			LastProgressT = GameT;
			BestS = S;
		}
	}
	MoveDir = Go;
	P->AddMovementInput(MoveDir, 1.f);
	// sprint: always with "fast"; otherwise for 8 s after a serac zone's "MOVE!" (a player runs when the ice groans)
	if (USurvivalComponent* Sv = P->FindComponentByClass<USurvivalComponent>()) { Sv->SetSprintRequested(bFast || GameT < SprintUntil); }
	if (S >= NextPointS)
	{
		NextPointS += EveryM * 100.f;
		Checkpoint(P);
	}
	Check(P, Dt);
}

void UYggRouteBot::Check(APawn* P, float Dt)
{
	if (GameT < NextCheck) { return; }
	const float Step = 0.25f;
	NextCheck = GameT + Step;
	ACharacter* C = Cast<ACharacter>(P);
	UCharacterMovementComponent* M = C ? C->GetCharacterMovement() : nullptr;
	const UExpeditionComponent* Ex = P->FindComponentByClass<UExpeditionComponent>();
	if (!M || !Ex) { return; }
	const float Half = C->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	const FVector Pos = P->GetActorLocation();
	const float Bottom = float(Pos.Z) - Half;

	if (M->IsFalling())
	{
		if (FallFor == 0.f) { FallStartZ = float(Pos.Z); }
		FallFor += Step;
		FallSec += Step;
	}
	else
	{
		if (FallFor >= 0.75f && (FallStartZ - float(Pos.Z) > 100.f || FallFor >= 1.5f))
		{
			++Falls;
			Issue(TEXT("fall"), FString::Printf(TEXT("falling / sliding for %.2f s, %.1f m down"), FallFor, (FallStartZ - float(Pos.Z)) / 100.f), P);
		}
		else if (FallFor >= 0.75f) { ++Slides; }
		FallFor = 0.f;
	}
	if (M->IsMovingOnGround() && M->CurrentFloor.bBlockingHit)
	{
		MaxFloor = FMath::Max(MaxFloor, M->CurrentFloor.FloorDist);
		FloatFor = M->CurrentFloor.FloorDist > 10.f ? FloatFor + Step : 0.f;
		if (FloatFor > 1.f && !bFloatNoted) { bFloatNoted = true; Issue(TEXT("float"), FString::Printf(TEXT("floor gap %.1f cm for 1 s while walking"), M->CurrentFloor.FloorDist), P); }
		if (FloatFor == 0.f) { bFloatNoted = false; }
	}
	{
		FHitResult H;
		FCollisionQueryParams Q(SCENE_QUERY_STAT(YggRouteBotUnder), false, P);
		if (GetWorld()->LineTraceSingleByChannel(H, Pos + FVector(0, 0, 3000), Pos - FVector(0, 0, 3000), ECC_Pawn, Q) && H.ImpactPoint.Z > Bottom + 80.f
			&& !Cast<UInstancedStaticMeshComponent>(H.GetComponent()))
		{
			UnderFor += Step;
			if (UnderFor > 1.f && !bUnderNoted) { bUnderNoted = true; Issue(TEXT("under"), FString::Printf(TEXT("ground (%s) %.0f cm above the capsule's bottom"), *Blocker(H), H.ImpactPoint.Z - Bottom), P); }
		}
		else { UnderFor = 0.f; bUnderNoted = false; }
	}
	if (Phase == EPhase::Walk)
	{
		MaxLat = FMath::Max(MaxLat, Lat / 100.f);
		LostFor = Lat > 800.f ? LostFor + Step : 0.f;
		if (LostFor > 3.f && !bLostNoted) { bLostNoted = true; Issue(TEXT("lost"), FString::Printf(TEXT("%.1f m off the route for 3 s"), Lat / 100.f), P); }
		if (LostFor == 0.f) { bLostNoted = false; }
		(Ex->IsOnRope() ? RopeOnSec : RopeOffSec) += Step;
	}
}

void UYggRouteBot::Sample(APawn* P)
{
	ACharacter* C = Cast<ACharacter>(P);
	UCharacterMovementComponent* M = C ? C->GetCharacterMovement() : nullptr;
	const UExpeditionComponent* Ex = P->FindComponentByClass<UExpeditionComponent>();
	const USurvivalComponent* Sv = P->FindComponentByClass<USurvivalComponent>();
	if (!M || !Ex || !Sv) { return; }
	const FSurvivalSnapshot Snap = Sv->GetSnapshot();
	FSample X;
	X.T = GameT; X.Real = FPlatformTime::Seconds() - RealStart; X.Leg = CurLeg;
	X.S = S / 100.f; X.Lat = Lat / 100.f; X.Pos = P->GetActorLocation(); X.Elev = Snap.ElevationMeters;
	X.Speed = float(P->GetVelocity().Size2D()); X.MaxSpeed = M->MaxWalkSpeed;
	X.bFalling = M->IsFalling(); X.bWalkable = M->CurrentFloor.IsWalkableFloor(); X.FloorDist = M->CurrentFloor.FloorDist;
	X.bRope = Ex->IsOnRope(); X.bSprint = Snap.bSprinting; X.Stamina = Snap.Stamina; X.SpO2 = Snap.Oxygen;
	float Sum = 0.f; int32 N = 0;
	for (int32 i = FrameMs.Num() - 1; i >= 0 && N < 60; --i, ++N) { Sum += FrameMs[i]; }
	X.Fps = N && Sum > 0.f ? 1000.f * N / Sum : 0.f;
	if (Samples.Num() < 40000) { Samples.Add(X); }
}

FString UYggRouteBot::HudNextCamp(APawn* P) const
{
	// the HUD's rule (USurvivalHUDWidget): the camp after the expedition's last camp; before any camp, the first camp more
	// than 15 m above the climber
	const USurvivalComponent* Sv = P ? P->FindComponentByClass<USurvivalComponent>() : nullptr;
	const UExpeditionComponent* Ex = P ? P->FindComponentByClass<UExpeditionComponent>() : nullptr;
	if (!Sv || !Ex) { return TEXT("?"); }
	if (const ARouteLandmark* Last = Ex->GetLastCamp())
	{
		const int32 I = Camps.IndexOfByPredicate([Last](const FCamp& C) { return C.Name == Last->LandmarkName; });
		if (I != INDEX_NONE) { return Camps.IsValidIndex(I + 1) ? Camps[I + 1].Short : FString(TEXT("SUMMIT")); }
	}
	const float Me = Sv->GetSnapshot().ElevationMeters;
	for (const FCamp& C : Camps)
	{
		if (Me + float(C.Pos.Z - P->GetActorLocation().Z) / 100.f > Me + 15.f) { return C.Short; }
	}
	return TEXT("SUMMIT");
}

int32 UYggRouteBot::ScanSimple(float FromS, float ToS, FString& What)
{
	// the climber's capsule meets simple collision; the eye sees the triangles. Where the first simple hit from above stands
	// well over the first visible one, something invisible is in the way (the World Tree roots' single convex hull, A5).
	int32 N = 0;
	FCollisionQueryParams QS(SCENE_QUERY_STAT(YggRouteBotSimple), false);
	FCollisionQueryParams QC(SCENE_QUERY_STAT(YggRouteBotComplex), true);
	if (const APlayerController* PC = GetWorld()->GetFirstPlayerController()) { if (PC->GetPawn()) { QS.AddIgnoredActor(PC->GetPawn()); QC.AddIgnoredActor(PC->GetPawn()); } }
	for (float At = FromS; At < ToS && At <= RouteLen; At += 500.f)
	{
		const FVector Pt = PointAt(At);
		const FVector Top = Pt + FVector(0, 0, 15000), Bot = Pt - FVector(0, 0, 20000);
		FHitResult HS, HC;
		const bool bS = GetWorld()->LineTraceSingleByChannel(HS, Top, Bot, ECC_Pawn, QS);
		const bool bC = GetWorld()->LineTraceSingleByChannel(HC, Top, Bot, ECC_Visibility, QC);
		// a crevasse ladder's hull is its walkway over the rungs (the eye sees the slot floor through them): not an issue
		if (bS && bC && HS.ImpactPoint.Z - HC.ImpactPoint.Z > 100.0 && !Cast<ACrevasseCrossing>(HS.GetActor()))
		{
			++N;
			if (What.Len() < 400)
			{
				What += FString::Printf(TEXT("%s%.0f m: %.0f cm over the visible surface (%s)"), What.IsEmpty() ? TEXT("") : TEXT("; "), At / 100.f,
					HS.ImpactPoint.Z - HC.ImpactPoint.Z, *Blocker(HS));
			}
		}
	}
	return N;
}

void UYggRouteBot::Checkpoint(APawn* P)
{
	ACharacter* C = Cast<ACharacter>(P);
	const USurvivalComponent* Sv = P->FindComponentByClass<USurvivalComponent>();
	const UExpeditionComponent* Ex = P->FindComponentByClass<UExpeditionComponent>();
	if (!C || !Sv || !Ex) { return; }
	UWorld* W = GetWorld();
	FPoint X;
	X.S = S / 100.f; X.Leg = CurLeg; X.T = GameT; X.Pos = P->GetActorLocation(); X.Lat = Lat / 100.f; X.Elev = Sv->GetSnapshot().ElevationMeters;
	const float Half = C->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	const FVector Feet = X.Pos - FVector(0, 0, Half);
	// the ground under the boots: simple (what the capsule stands on) and complex (what the eye sees)
	{
		FCollisionQueryParams QS(SCENE_QUERY_STAT(YggRouteBotFeetS), false, P), QC(SCENE_QUERY_STAT(YggRouteBotFeetC), true, P);
		FHitResult HS, HC;
		X.FeetToSimple = W->LineTraceSingleByChannel(HS, X.Pos, X.Pos - FVector(0, 0, 5000), ECC_Pawn, QS) ? float(Feet.Z - HS.ImpactPoint.Z) : -999.f;
		X.FeetToComplex = W->LineTraceSingleByChannel(HC, Feet + FVector(0, 0, 150), Feet - FVector(0, 0, 5000), ECC_Visibility, QC) ? float(Feet.Z - HC.ImpactPoint.Z) : -999.f;
		X.GroundActor = HS.GetActor() ? HS.GetActor()->GetActorNameOrLabel() : TEXT("none");
		if (FMath::Abs(X.FeetToSimple - X.FeetToComplex) > 30.f && X.FeetToSimple > -900.f && X.FeetToComplex > -900.f)
		{
			Issue(TEXT("ground"), FString::Printf(TEXT("boots %.0f cm over the collision but %.0f cm over the visible surface (%s)"), X.FeetToSimple, X.FeetToComplex, *X.GroundActor), P);
		}
	}
	if (GEngine) { GEngine->Exec(W, *FString::Printf(TEXT("ygg.GroundProbe A4_%s_%05.0fm"), *RunName.Mid(6, 15), X.S)); ++Probes; }
	// guidance in the next 100 m
	const FVector Fwd = DirAt(S);
	X.bClipped = Ex->IsOnRope();
	for (float Ah = 0.f; Ah <= 10000.f && X.RopeM < 0.f; Ah += 1000.f)
	{
		const FVector Pt = PointAt(S + Ah);
		for (TActorIterator<AFixedRope> It(W); It; ++It)
		{
			if (It->IsNear(Pt, 3000.f) && It->DistanceToRope(Pt) < 400.f) { X.RopeM = Ah / 100.f; break; }
		}
	}
	const auto Ahead = [&](UClass* Cls) -> float
	{
		float Best = -1.f;
		for (TActorIterator<AActor> It(W, Cls); It; ++It)
		{
			const FVector D = It->GetActorLocation() - X.Pos;
			const float Along = float(FVector::DotProduct(FVector(D.X, D.Y, 0.0), Fwd));
			const float Side = float(FMath::Abs(FVector::CrossProduct(FVector(D.X, D.Y, 0.0), Fwd).Z));
			if (Along > -500.f && Along < 10000.f && Side < 3000.f && (Best < 0.f || Along / 100.f < Best)) { Best = FMath::Max(0.f, Along / 100.f); }
		}
		return Best;
	};
	X.FlagM = Ahead(ARouteLandmark::StaticClass());
	X.ClueM = Ahead(AStoryClue::StaticClass());
	X.CheckM = Ahead(ATrailCheckpoint::StaticClass());
	// the HUD's next camp against the camp this leg leads to
	X.HudNext = HudNextCamp(P);
	X.LegTarget = Camps.IsValidIndex(CurLeg + 1) ? Camps[CurLeg + 1].Short : TEXT("?");
	// inside the leg's end camp already (its checkpoint taken): the HUD rightly names the camp after it
	if (Ex->GetLastCamp() && Camps.IsValidIndex(CurLeg + 2) && Ex->GetLastCamp()->LandmarkName == Camps[CurLeg + 1].Name) { X.LegTarget = Camps[CurLeg + 2].Short; }
	X.bHudAgrees = X.HudNext == X.LegTarget;
	if (!X.bHudAgrees)
	{
		const bool bNew = !Points.Num() || Points.Last().bHudAgrees || Points.Last().HudNext != X.HudNext;
		if (bNew) { Issue(TEXT("guidance"), FString::Printf(TEXT("HUD says next camp '%s' while the route leads to '%s'"), *X.HudNext, *X.LegTarget), P); }
	}
	if (X.RopeM < 0.f && X.FlagM < 0.f && X.ClueM < 0.f && X.CheckM < 0.f && CurLeg >= 2)
	{
		Issue(TEXT("guidance"), TEXT("no rope, flag, clue or trail checkpoint in the next 100 m"), P);
	}
	// invisible simple collision over the next stretch
	X.SimpleOver = ScanSimple(S, S + EveryM * 100.f, X.SimpleOverWhat);
	if (X.SimpleOver > 0) { Issue(TEXT("simple"), FString::Printf(TEXT("simple collision over the visible surface at %d of %d spots ahead: %s"), X.SimpleOver, int32(EveryM / 5.f), *X.SimpleOverWhat), P); }
	// frames since the previous checkpoint
	float Sum = 0.f; int32 N = 0;
	for (int32 i = FrameMark; i < FrameMs.Num(); ++i) { Sum += FrameMs[i]; ++N; }
	X.Fps = N && Sum > 0.f ? 1000.f * N / Sum : 0.f;
	FrameMark = FrameMs.Num();
	if (HoldTime >= 0.f) { YggTime::Set(W, HoldTime); }
	if (bShots)
	{
		const FString Stage = FString::Printf(TEXT("%05.0fm"), X.S);
		Shot(Stage, true);
		X.Shot = FString::Printf(TEXT("%s_%s.png"), *RunName, *Stage);
	}
	Points.Add(X);
}

void UYggRouteBot::Look(APlayerController* PC, float Dt)
{
	if (Dt <= 0.f || DeathT >= 0.0) { return; }
	if (Phase == EPhase::Walk && !MoveDir.IsNearlyZero()) { WantYaw = float(MoveDir.Rotation().Yaw); WantPitch = -4.f; }
	const FRotator CR = PC->GetControlRotation();
	const float EY = float(FRotator::NormalizeAxis(double(WantYaw) - CR.Yaw));
	const float EP = WantPitch - float(FRotator::NormalizeAxis(CR.Pitch));
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

bool UYggRouteBot::TeleportAlong(APawn* P, float ToS, const FString& Why)
{
	ACharacter* C = Cast<ACharacter>(P);
	if (!C) { return false; }
	// route.json's heights are the ground when the route was planned; the ground is traced from 150 m above the higher of
	// the route and the climber, and without ground there is no teleport (as the summit robot)
	FVector At = PointAt(ToS);
	At.Z = FMath::Max(At.Z, P->GetActorLocation().Z);
	FHitResult H;
	FCollisionQueryParams Q(SCENE_QUERY_STAT(YggRouteBotTp), false, P);
	if (!GetWorld()->LineTraceSingleByChannel(H, At + FVector(0, 0, 15000), At - FVector(0, 0, 20000), ECC_Pawn, Q))
	{
		Note(FString::Printf(TEXT("teleport (%s) refused: no ground under %s"), *Why, *RouteBotCm(At)));
		return false;
	}
	const FVector To = H.ImpactPoint + FVector(0, 0, C->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + 10.f);
	if (!P->SetActorLocation(To, false, nullptr, ETeleportType::TeleportPhysics)) { return false; }
	C->GetCharacterMovement()->StopMovementImmediately();
	APlayerController* PC = Cast<APlayerController>(P->GetController());
	if (PC && PC->PlayerCameraManager) { PC->PlayerCameraManager->SetGameCameraCutThisFrame(); }
	if (UYggViewRigComponent* R = UYggViewRigComponent::Find(P)) { if (R->GetView() == EYggView::Walk) { R->HardReset(PC ? PC->GetControlRotation() : P->GetActorRotation(), EYggView::Walk); } }
	if (Why != TEXT("at")) { ++Teleports; }
	Note(FString::Printf(TEXT("teleport (%s) to %s, onto %s"), *Why, *RouteBotCm(To), *Blocker(H)));
	return true;
}

// ------------------------------------------------------------------------------------------------ pictures

void UYggRouteBot::Shot(const FString& Stage, bool bUI)
{
	if (bShotPending)
	{
		Note(FString::Printf(TEXT("picture %s skipped (another one pending)"), *Stage));
		return;
	}
	FShot X;
	X.Stage = Stage;
	X.File = ShotDir / FString::Printf(TEXT("%s_%s.png"), *RunName, *Stage);
	X.T = GameT;
	X.S = S / 100.f;
	if (const APawn* P = GetWorld() && GetWorld()->GetFirstPlayerController() ? GetWorld()->GetFirstPlayerController()->GetPawn() : nullptr)
	{
		if (const USurvivalComponent* Sv = P->FindComponentByClass<USurvivalComponent>()) { X.Elev = Sv->GetSnapshot().ElevationMeters; }
	}
	Shots.Add(X);
	PendingShot = X.File;
	bShotPending = true;
	// the pixels come to OnShot and a worker writes the PNG (the engine's own PNG write freezes the game thread)
	ShotHandle = UGameViewportClient::OnScreenshotCaptured().AddUObject(this, &UYggRouteBot::OnShot);
	FScreenshotRequest::RequestScreenshot(PendingShot, bUI, false);
}

void UYggRouteBot::OnShot(int32 ShotW, int32 ShotH, const TArray<FColor>& Pixels)
{
	if (ShotHandle.IsValid()) { UGameViewportClient::OnScreenshotCaptured().Remove(ShotHandle); ShotHandle.Reset(); }
	const FString File = PendingShot;
	bShotPending = false;
	PendingShot.Reset();
	if (ShotW <= 0 || ShotH <= 0 || Pixels.Num() < ShotW * ShotH || File.IsEmpty()) { return; }
	TArray<FColor> Copy = Pixels;
	for (FColor& Px : Copy) { Px.A = 255; }
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

void UYggRouteBot::DrainLog()
{
	if (!LogDevice.IsValid()) { return; }
	FString Full;
	int32 Budget = 300;
	while (Budget-- > 0 && LogDevice->Lines.Dequeue(Full))
	{
		int32 Nl;
		const FString L = Full.FindChar(TEXT('\n'), Nl) ? Full.Left(Nl).TrimEnd() : Full.TrimEnd();
		if (LogLines.Num() < 600) { LogLines.Add(FString::Printf(TEXT("%s  %s"), *Clock(GameT), *L.Left(260))); }
		if (L.Contains(TEXT("MOVE!"))) { SprintUntil = GameT + 8.0; Note(TEXT("serac warning: sprinting for 8 s")); ++SeracWarnings; }
	}
}

// ------------------------------------------------------------------------------------------------ the report

void UYggRouteBot::Finish(const FString& Why)
{
	if (Phase == EPhase::Idle || Phase == EPhase::Quitting) { return; }
	EndWhy = Why;
	Note(FString::Printf(TEXT("end: %s"), *Why));
	UWorld* W = GetWorld();
	APlayerController* PC = W ? W->GetFirstPlayerController() : nullptr;
	APawn* P = PC ? PC->GetPawn() : nullptr;
	if (USurvivalComponent* Sv = P ? P->FindComponentByClass<USurvivalComponent>() : nullptr) { Sv->SetSprintRequested(false); }
	if (SpeedX != 1.f && W) { if (AWorldSettings* WS = W->GetWorldSettings()) { WS->SetTimeDilation(1.f); } }
	DrainLog();
	WriteFiles(true);
	if (LogDevice.IsValid() && GLog) { GLog->RemoveOutputDevice(LogDevice.Get()); }
	if (bMaxFpsSet)
	{
		if (IConsoleVariable* V = IConsoleManager::Get().FindConsoleVariable(TEXT("t.MaxFPS"))) { V->Set(*PrevMaxFps, ECVF_SetByConsole); }
		bMaxFpsSet = false;
	}
	if (CVarRouteBotQuit.GetValueOnGameThread() != 0) { Phase = EPhase::Quitting; PhaseT = 0.0; }
	else { Phase = EPhase::Idle; }
}

FString UYggRouteBot::BuildReport(bool bFinal) const
{
	TStringBuilder<65536> R;
	R.Appendf(TEXT("# Route robot run %s\n\n"), *RunName);
	R.Appendf(TEXT("- Result: %s\n"), bFinal ? *EndWhy : TEXT("(running)"));
	R.Appendf(TEXT("- Now: game %s, %.0f m along of %.0f m (leg %d)\n"), *Clock(GameT), S / 100.f, RouteLen / 100.f, CurLeg);
	R.Appendf(TEXT("- Legs %d..%d: %s -> %s; %s; game speed x%.2f (times below are game time = a player's time)%s\n"), FromLeg, ToLeg,
		Camps.IsValidIndex(FromLeg) ? *Camps[FromLeg].Name : TEXT("?"), Camps.IsValidIndex(ToLeg + 1) ? *Camps[ToLeg + 1].Name : TEXT("?"),
		bFast ? TEXT("sprinting whenever stamina allows") : TEXT("plain walk"), SpeedX, AtM > 0.f ? *FString::Printf(TEXT("; started %.0f m along by a teleport"), AtM) : TEXT(""));
	R.Appendf(TEXT("- Stuck teleports: **%d**; walk-rounds: %d; sidestep jumps: %d; falls (over 0.75 s and 1 m, or over 1.5 s): %d, short slides: %d (%.1f s falling in all); furthest off the route %.1f m; largest floor gap %.1f cm\n"),
		Teleports, Detours, Sidesteps, Falls, Slides, FallSec, MaxLat, MaxFloor);
	R.Appendf(TEXT("- Serac warnings (sprinted out): %d; clipped to a fixed rope %.0f s of %.0f s walking; food / water top-ups: %d; ground probes: %d (Saved/Review/ground_probe.csv, labels A4_*)\n\n"),
		SeracWarnings, RopeOnSec, RopeOnSec + RopeOffSec, TopUps, Probes);

	R.Append(TEXT("## Times\n\n| leg | from | game | real |\n|---|---|---|---|\n"));
	const double EndT = ArriveT >= 0.0 ? ArriveT : GameT;
	for (int32 k = 0; k < LegStartT.Num(); ++k)
	{
		if (LegStartT[k] < 0.0) { continue; }
		const double To = k + 1 < LegStartT.Num() && LegStartT[k + 1] >= 0.0 ? LegStartT[k + 1] : EndT;
		R.Appendf(TEXT("| %d | %s | %s | |\n"), FromLeg + k, Camps.IsValidIndex(FromLeg + k) ? *Camps[FromLeg + k].Name : TEXT("?"), *Clock(To - LegStartT[k]));
	}
	if (WalkStartT >= 0.0)
	{
		R.Appendf(TEXT("| **all** | | **%s** | %s |\n"), *Clock(EndT - WalkStartT), *Clock(FPlatformTime::Seconds() - WalkStartReal));
		const double WalkSec = EndT - WalkStartT;
		if (WalkSec > 1.0) { R.Appendf(TEXT("\nAverage %.2f m/s over %.0f m.\n"), (S - StartS) / 100.f / WalkSec, (S - StartS) / 100.f); }
	}

	// issues by kind
	R.Append(TEXT("\n## Issues\n\n"));
	TMap<FString, int32> ByKind;
	for (const FIssue& I : Issues) { ByKind.FindOrAdd(I.Kind)++; }
	if (!Issues.Num()) { R.Append(TEXT("none\n")); }
	else
	{
		for (const TPair<FString, int32>& K : ByKind) { R.Appendf(TEXT("- %s: %d\n"), *K.Key, K.Value); }
		R.Append(TEXT("\n| game | m along | leg | m a.s.l. | kind | what | where (BugItGo) |\n|---|---|---|---|---|---|---|\n"));
		for (const FIssue& I : Issues)
		{
			R.Appendf(TEXT("| %s | %.0f | %d | %.0f | %s | %s | %s |\n"), *Clock(I.T), I.S, I.Leg, I.Elev, *I.Kind, *I.Text.Replace(TEXT("|"), TEXT("/")), *RouteBotCm(I.Pos));
		}
	}

	// frames
	R.Append(TEXT("\n## Frames (real time)\n\n"));
	if (FrameMs.Num() > 100)
	{
		TArray<float> F = FrameMs;
		F.Sort();
		double Sum = 0.0; for (float V : F) { Sum += V; }
		const float Avg = float(Sum / F.Num());
		const float P99 = F[FMath::Min(F.Num() - 1, int32(F.Num() * 0.99))];
		R.Appendf(TEXT("- %d frames: average %.1f fps (%.2f ms), median %.2f ms, 99th percentile %.2f ms (1%% low %.0f fps), worst %.1f ms\n"),
			F.Num(), 1000.f / Avg, Avg, F[F.Num() / 2], P99, 1000.f / P99, F.Last());
		const FPoint* Worst = nullptr;
		for (const FPoint& Pt : Points) { if (Pt.Fps > 0.f && (!Worst || Pt.Fps < Worst->Fps)) { Worst = &Pt; } }
		if (Worst) { R.Appendf(TEXT("- Slowest stretch: the %.0f m before %.0f m along (leg %d, %.0f m a.s.l.): %.0f fps; BugItGo %s\n"), EveryM, Worst->S, Worst->Leg, Worst->Elev, Worst->Fps, *RouteBotCm(Worst->Pos)); }
	}

	R.Append(TEXT("\n## Checkpoints\n\n| m along | leg | game | m a.s.l. | off route m | boots-collision cm | boots-visible cm | ground | rope / flag / clue / checkpoint ahead m | clipped | HUD next | leg to | simple over | fps | picture |\n|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|\n"));
	for (const FPoint& Pt : Points)
	{
		R.Appendf(TEXT("| %.0f | %d | %s | %.0f | %.1f | %.0f | %.0f | %s | %.0f / %.0f / %.0f / %.0f | %s | %s%s | %s | %d | %.0f | %s |\n"), Pt.S, Pt.Leg, *Clock(Pt.T), Pt.Elev, Pt.Lat,
			Pt.FeetToSimple, Pt.FeetToComplex, *Pt.GroundActor, Pt.RopeM, Pt.FlagM, Pt.ClueM, Pt.CheckM, Pt.bClipped ? TEXT("yes") : TEXT("no"),
			*Pt.HudNext, Pt.bHudAgrees ? TEXT("") : TEXT(" (!)"), *Pt.LegTarget, Pt.SimpleOver, Pt.Fps, *Pt.Shot);
	}
	R.Append(TEXT("\n## Expedition log lines\n\n```\n"));
	for (int32 i = FMath::Max(0, LogLines.Num() - 120); i < LogLines.Num(); ++i) { R.Appendf(TEXT("%s\n"), *LogLines[i]); }
	R.Append(TEXT("```\n\n## Events\n\n```\n"));
	for (int32 i = FMath::Max(0, Events.Num() - 400); i < Events.Num(); ++i) { R.Appendf(TEXT("%s\n"), *Events[i]); }
	R.Append(TEXT("```\n"));
	return FString(R.ToString());
}

void UYggRouteBot::WriteFiles(bool bFinal)
{
	FFileHelper::SaveStringToFile(BuildReport(bFinal), *(Dir / (bFinal ? TEXT("report.md") : TEXT("progress.md"))), FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
	TStringBuilder<262144> C;
	C.Append(TEXT("t,real,leg,s_m,lat_m,x,y,z,elev_m,speed,max_speed,falling,walkable,floor_cm,rope,sprint,stamina,spo2,fps\n"));
	for (const FSample& X : Samples)
	{
		C.Appendf(TEXT("%.1f,%.1f,%d,%.1f,%.2f,%.0f,%.0f,%.0f,%.1f,%.0f,%.0f,%d,%d,%.1f,%d,%d,%.0f,%.1f,%.0f\n"), X.T, X.Real, X.Leg, X.S, X.Lat, X.Pos.X, X.Pos.Y, X.Pos.Z, X.Elev,
			X.Speed, X.MaxSpeed, X.bFalling ? 1 : 0, X.bWalkable ? 1 : 0, X.FloorDist, X.bRope ? 1 : 0, X.bSprint ? 1 : 0, X.Stamina, X.SpO2, X.Fps);
	}
	FFileHelper::SaveStringToFile(FString(C.ToString()), *(Dir / TEXT("samples.csv")), FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
	TStringBuilder<65536> Pc;
	Pc.Append(TEXT("s_m,leg,t,elev_m,lat_m,x,y,z,boots_collision_cm,boots_visible_cm,ground,rope_m,flag_m,clue_m,checkpoint_m,clipped,hud_next,leg_to,hud_agrees,simple_over,fps,picture\n"));
	for (const FPoint& X : Points)
	{
		Pc.Appendf(TEXT("%.0f,%d,%.1f,%.1f,%.2f,%.0f,%.0f,%.0f,%.0f,%.0f,%s,%.0f,%.0f,%.0f,%.0f,%d,%s,%s,%d,%d,%.0f,%s\n"), X.S, X.Leg, X.T, X.Elev, X.Lat, X.Pos.X, X.Pos.Y, X.Pos.Z,
			X.FeetToSimple, X.FeetToComplex, *Csv(X.GroundActor), X.RopeM, X.FlagM, X.ClueM, X.CheckM, X.bClipped ? 1 : 0, *Csv(X.HudNext), *Csv(X.LegTarget), X.bHudAgrees ? 1 : 0,
			X.SimpleOver, X.Fps, *Csv(X.Shot));
	}
	FFileHelper::SaveStringToFile(FString(Pc.ToString()), *(Dir / TEXT("points.csv")), FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
	if (bFinal) { FFileHelper::SaveStringToFile(EndWhy, *(Dir / TEXT("done.txt")), FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM); }
}
