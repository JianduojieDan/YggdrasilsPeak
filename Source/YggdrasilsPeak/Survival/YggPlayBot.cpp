// The robot player: see YggPlayBot.h. Tools/Bot/run_bot.ps1 runs it in the standalone game; Tools/Bot/README.md.

#include "Survival/YggPlayBot.h"
#include "Survival/YggViewRig.h"
#include "Survival/SurvivalComponent.h"
#include "Story/YggStory.h"

#include "Algo/BinarySearch.h"
#include "Async/Async.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/CapsuleComponent.h"
#include "Containers/Queue.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/InputSettings.h"
#include "GameFramework/PlayerController.h"
#include "HAL/FileManager.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformTime.h"
#include "Misc/App.h"
#include "Misc/FileHelper.h"
#include "Misc/OutputDevice.h"
#include "Misc/OutputDeviceRedirector.h"
#include "Misc/Paths.h"
#include "Misc/StringBuilder.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

DEFINE_LOG_CATEGORY_STATIC(LogYggBot, Log, All);

/** Hands the story's log lines (and view / story warnings) to the game thread; called on any thread. */
class FYggBotLog : public FOutputDevice
{
public:
	FYggBotLog() { bAutoEmitLineTerminator = false; }
	virtual bool CanBeUsedOnAnyThread() const override { return true; }
	virtual bool CanBeUsedOnMultipleThreads() const override { return true; }
	/** (is a warning or error, the text) */
	TQueue<TPair<bool, FString>, EQueueMode::Mpsc> Lines;
	virtual void Serialize(const TCHAR* V, ELogVerbosity::Type Verbosity, const FName& Category) override
	{
		if (!V) { return; }
		static const FName ViewCat(TEXT("LogYggView")), RigCat(TEXT("LogYggRig")), StoryCat(TEXT("LogYggStory"));
		const bool bWarn = (Verbosity & ELogVerbosity::VerbosityMask) <= ELogVerbosity::Warning;
		const bool bStory = FCString::Strncmp(V, TEXT("Story"), 5) == 0;
		const bool bCamRec = FCString::Strncmp(V, TEXT("CamRec: INCIDENT"), 16) == 0;   // the camera recorder, this run
		if (bStory || bCamRec || (bWarn && (Category == ViewCat || Category == RigCat || Category == StoryCat)))
		{
			Lines.Enqueue(TPair<bool, FString>(bWarn, FString(V)));
		}
	}
};

namespace
{
	TAutoConsoleVariable<float> CVarBotCapMin(TEXT("ygg.Bot.CapMin"), 75.f, TEXT("Robot player: total cap, minutes of game time; then the report and quit."));
	TAutoConsoleVariable<float> CVarBotStuckSec(TEXT("ygg.Bot.StuckSec"), 25.f, TEXT("Robot player: seconds without progress before it teleports 15 m ahead along the route."));
	TAutoConsoleVariable<float> CVarBotBeatCapMin(TEXT("ygg.Bot.BeatCapMin"), 6.f, TEXT("Robot player: minutes a beat may be the walk target; then a teleport to its place, 90 s later it is skipped."));
	TAutoConsoleVariable<float> CVarBotStallMin(TEXT("ygg.Bot.StallMin"), 15.f, TEXT("Robot player: minutes without any beat firing before the run ends as stalled."));
	TAutoConsoleVariable<int32> CVarBotCamera(TEXT("ygg.Bot.Camera"), 1, TEXT("Robot player: 1 = camera stress (yaw sweeps, straight down / up, jitter); 0 = only looks where it walks and at look targets."));
	TAutoConsoleVariable<int32> CVarBotQuit(TEXT("ygg.Bot.Quit"), 1, TEXT("Robot player: 1 = quit the game when the run ends (not after ygg.Bot.Stop)."));
	TAutoConsoleVariable<int32> CVarBotRig(TEXT("ygg.Bot.Rig"), 1, TEXT("Robot player: 1 = read the view rig (posture, eyelids, eye height, audit); 0 = never touch it."));
	TAutoConsoleVariable<int32> CVarBotHardReset(TEXT("ygg.Bot.HardReset"), 1, TEXT("Robot player: 1 = a stuck teleport calls the view rig's HardReset (springs snapped, like every teleport) when walking with open eyes."));
	TAutoConsoleVariable<int32> CVarBotPath(TEXT("ygg.Bot.Path"), 0, TEXT("Robot player: 0 = straight line first, the route.json path once stuck; 1 = the route path whenever the target is far."));
	TAutoConsoleVariable<int32> CVarBotSeed(TEXT("ygg.Bot.Seed"), 0, TEXT("Robot player: random seed of the camera stress (0 = from the clock; the report names the seed used)."));

	FAutoConsoleCommandWithWorldAndArgs BotCmdRun(TEXT("ygg.Bot.Run"),
		TEXT("ygg.Bot.Run [chapter1] [fast] - the robot plays the chapter by itself (story auto-answers, walks to the next story place, camera stress), writes Saved/Bot/<time>/report.md, then quits (ygg.Bot.Quit). fast = t.MaxFPS 0 and sprint."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			UYggPlayBot* Bot = World ? World->GetSubsystem<UYggPlayBot>() : nullptr;
			if (!Bot) { UE_LOG(LogYggBot, Warning, TEXT("Bot: no robot in this world (run it in the game or PIE)")); return; }
			FString Chapter = TEXT("Chapter1");
			bool bFast = false;
			for (const FString& A : Args)
			{
				if (A.Equals(TEXT("fast"), ESearchCase::IgnoreCase)) { bFast = true; }
				else if (!A.IsEmpty()) { Chapter = A; }
			}
			Bot->Run(Chapter, bFast);
		}));
	FAutoConsoleCommandWithWorld BotCmdStop(TEXT("ygg.Bot.Stop"), TEXT("Stop the robot player: final report, no quit."),
		FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World)
		{
			if (UYggPlayBot* Bot = World ? World->GetSubsystem<UYggPlayBot>() : nullptr) { Bot->StopRun(TEXT("stopped (ygg.Bot.Stop)")); }
		}));
	FAutoConsoleCommandWithWorld BotCmdStatus(TEXT("ygg.Bot.Status"), TEXT("Log the robot player's state."),
		FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World)
		{
			if (UYggPlayBot* Bot = World ? World->GetSubsystem<UYggPlayBot>() : nullptr) { UE_LOG(LogYggBot, Display, TEXT("Bot: %s"), *Bot->Status()); }
		}));

	TArray<FString> JStrs(const TSharedPtr<FJsonObject>& O, const TCHAR* Key)
	{
		TArray<FString> Out;
		if (!O.IsValid()) { return Out; }
		const TArray<TSharedPtr<FJsonValue>>* Arr;
		FString S;
		if (O->TryGetArrayField(Key, Arr))
		{
			for (const TSharedPtr<FJsonValue>& V : *Arr) { if (V.IsValid() && V->Type == EJson::String) { Out.Add(V->AsString()); } }
		}
		else if (O->TryGetStringField(Key, S) && !S.IsEmpty()) { Out.Add(S); }
		return Out;
	}
	FString JStr(const TSharedPtr<FJsonObject>& O, const TCHAR* Key)
	{
		FString S;
		if (O.IsValid()) { O->TryGetStringField(Key, S); }
		return S;
	}
	float JNum(const TSharedPtr<FJsonObject>& O, const TCHAR* Key, float Def)
	{
		double D;
		return O.IsValid() && O->TryGetNumberField(Key, D) ? float(D) : Def;
	}
	/** The story's posture names -> EYggView as a byte (stand / walk = 0); 255 = unknown. */
	uint8 PoseByte(const FString& In)
	{
		const FString N = In.ToLower();
		if (N == TEXT("stand") || N == TEXT("walk")) { return uint8(EYggView::Walk); }
		if (N == TEXT("lie")) { return uint8(EYggView::Lie); }
		if (N == TEXT("sit")) { return uint8(EYggView::Sit); }
		if (N == TEXT("kneel1")) { return uint8(EYggView::Kneel1); }
		if (N == TEXT("kneel2")) { return uint8(EYggView::Kneel2); }
		if (N == TEXT("crawl")) { return uint8(EYggView::Crawl); }
		return 255;
	}
	FString Clock(double S)
	{
		const int32 T = FMath::Max(0, FMath::RoundToInt(S));
		return T >= 3600 ? FString::Printf(TEXT("%d:%02d:%02d"), T / 3600, (T / 60) % 60, T % 60) : FString::Printf(TEXT("%d:%02d"), T / 60, T % 60);
	}
	FString Cm(const FVector& V) { return FString::Printf(TEXT("%.0f %.0f %.0f"), V.X, V.Y, V.Z); }
	void SetCVar(const TCHAR* Name, const TCHAR* Value)
	{
		if (IConsoleVariable* V = IConsoleManager::Get().FindConsoleVariable(Name)) { V->Set(Value, ECVF_SetByConsole); }
		else { UE_LOG(LogYggBot, Warning, TEXT("Bot: no console variable %s"), Name); }
	}
}

// ------------------------------------------------------------------------------------------------ lifetime

bool UYggPlayBot::ShouldCreateSubsystem(UObject* Outer) const
{
	const UWorld* W = Cast<UWorld>(Outer);
	return W && (W->WorldType == EWorldType::Game || W->WorldType == EWorldType::PIE) && Super::ShouldCreateSubsystem(Outer);
}

void UYggPlayBot::Deinitialize()
{
	if (Phase == EPhase::Waiting || Phase == EPhase::Playing) { Finish(TEXT("the world closed during the run"), false); }
	if (LogDevice.IsValid() && GLog) { GLog->RemoveOutputDevice(LogDevice.Get()); }   // the device itself lives on with this object
	Phase = EPhase::Idle;
	Super::Deinitialize();
}

void UYggPlayBot::Run(const FString& InChapter, bool bInFast)
{
	if (Phase != EPhase::Idle) { UE_LOG(LogYggBot, Warning, TEXT("Bot: already running (%s)"), *Status()); return; }
	Chapter = InChapter.IsEmpty() ? FString(TEXT("Chapter1")) : InChapter;
	Chapter[0] = FChar::ToUpper(Chapter[0]);
	bFast = bInFast;
	Seed = CVarBotSeed.GetValueOnGameThread() != 0 ? CVarBotSeed.GetValueOnGameThread() : int32(FDateTime::Now().GetTicks() % 1000000);
	Rng.Initialize(Seed);
	StartUtc = FDateTime::UtcNow();
	RealStart = FPlatformTime::Seconds();
	LastReport = RealStart;
	GameT = 0.0;
	LastBeatGame = 0.0;
	EndWhy.Reset();
	bStoryStarted = bStoryWasRunning = bChapterFinished = bSawDone = false;
	StoryStopAt = -1.0;
	Beats.Reset(); Dep.Reset(); Places.Reset(); Route.Reset(); RouteCum.Reset(); People.Reset();
	PeopleScan = -100.0;
	NavBeat = AimBeat = INDEX_NONE;
	MoveDir = FVector::ZeroVector;
	NavWantPose = 255;
	Best = 1e12f;
	FeedYaw = FeedPitch = 0.f;
	FeedAt = -100.0;
	CamRecLogIncidents = 0;
	CamRecLines.Reset();
	bRouteMode = bHeld = bArrived = bHaveLastPos = false;
	LastProgress = NextDecide = NextStuckCheck = NextSprint = 0.0;
	LastBotTeleport = -100.0;
	Teleports = CapTeleports = PutBacks = Jumps = 0;
	Look = ELook::Follow;
	Sweeps = DownUps = AimSeconds = 0;
	AimAccum = 0.f;
	MinPitchSeen = MaxPitchSeen = 0.f;
	RigView = 0; RigLid = 0.f; EyeMin = 1e6f; EyeMax = -1e6f; PosedSeconds = LidShutSeconds = 0.f;
	LastAudit.Reset();
	Hist.Init(0, 4000);
	Frames = 0; FrameSum = 0.0; WorstMs = 0.f; Over50 = Over100 = Over250 = 0; SegFrames = 0; SegTime = 0.0;
	Hitches.Reset(); Events.Reset(); Stucks.Reset(); StoryWarnings.Reset(); StoryWarningCount = 0;

	Dir = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("Bot") / FDateTime::Now().ToString(TEXT("%Y%m%d_%H%M%S")));
	IFileManager::Get().MakeDirectory(*Dir, true);
	Phase = EPhase::Waiting;
	Note(FString::Printf(TEXT("run %s%s, seed %d, folder %s"), *Chapter, bFast ? TEXT(" fast") : TEXT(""), Seed, *Dir));

	if (!LoadChapter()) { Finish(FString::Printf(TEXT("cannot read Content/Story/%s.ygs"), *Chapter)); return; }
	LoadRoute();
	if (!LogDevice.IsValid()) { LogDevice = MakeShared<FYggBotLog>(); }
	else { TPair<bool, FString> Stale; while (LogDevice->Lines.Dequeue(Stale)) {} }   // lines left from an earlier run
	if (GLog) { GLog->AddOutputDevice(LogDevice.Get()); }
}

void UYggPlayBot::StopRun(const FString& Why)
{
	if (Phase == EPhase::Waiting || Phase == EPhase::Playing) { Finish(Why, false); }
}

FString UYggPlayBot::Status() const
{
	int32 Fired = 0;
	for (const FBotBeat& B : Beats) { Fired += B.bFired ? 1 : 0; }
	static const TCHAR* PhaseNames[] = { TEXT("idle"), TEXT("waiting for the player"), TEXT("playing"), TEXT("quitting") };
	return FString::Printf(TEXT("%s, %s, game %s, beats %d/%d, target %s%s, aim %s, teleports %d, stuck events %d"),
		PhaseNames[uint8(Phase)], *Chapter, *Clock(GameT), Fired, Beats.Num(), *BeatName(NavBeat), bRouteMode ? TEXT(" (route)") : TEXT(""),
		*BeatName(AimBeat), Teleports, Stucks.Num());
}

bool UYggPlayBot::GetLookFeed(double HoldS, float& OutYawRate, float& OutPitchRate) const
{
	if (Phase != EPhase::Playing || FPlatformTime::Seconds() - FeedAt > HoldS) { return false; }
	OutYawRate = FeedYaw;
	OutPitchRate = FeedPitch;
	return true;
}

void UYggPlayBot::Note(const FString& Text)
{
	if (Events.Num() < 3000) { Events.Add({ GameT, Text }); }
	UE_LOG(LogYggBot, Display, TEXT("Bot %s: %s"), *Clock(GameT), *Text);
}

// ------------------------------------------------------------------------------------------------ the chapter

bool UYggPlayBot::LoadChapter()
{
	const FString Path = FPaths::ProjectContentDir() / TEXT("Story") / (Chapter + TEXT(".ygs"));
	FString Raw;
	TSharedPtr<FJsonObject> Root;
	if (!FFileHelper::LoadFileToString(Raw, *Path) || !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Raw), Root) || !Root.IsValid())
	{
		return false;
	}
	auto ReadPlaces = [this](const TSharedPtr<FJsonObject>& Obj)
	{
		if (!Obj.IsValid()) { return; }
		for (const auto& KV : Obj->Values)
		{
			if (!KV.Value.IsValid() || KV.Value->Type != EJson::Array) { continue; }
			const TArray<TSharedPtr<FJsonValue>>& A = KV.Value->AsArray();
			if (A.Num() >= 3) { Places.Add(FName(*KV.Key), FVector(A[0]->AsNumber(), A[1]->AsNumber(), A[2]->AsNumber())); }
		}
	};
	const TSharedPtr<FJsonObject>* P;
	if (Root->TryGetObjectField(TEXT("places"), P)) { ReadPlaces(*P); }
	// the world builder's places file wins, as in the story (UYggStorySubsystem::LoadPlaces)
	const FString File = JStr(Root, TEXT("places_file"));
	if (!File.IsEmpty())
	{
		FString PRaw;
		TSharedPtr<FJsonObject> PF;
		if (FFileHelper::LoadFileToString(PRaw, *(FPaths::ProjectContentDir() / TEXT("Story") / File))
			&& FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(PRaw), PF) && PF.IsValid())
		{
			const TSharedPtr<FJsonObject>* Inner;
			ReadPlaces(PF->TryGetObjectField(TEXT("places"), Inner) ? *Inner : PF);
		}
	}
	const TArray<TSharedPtr<FJsonValue>>* Arr;
	if (Root->TryGetArrayField(TEXT("beats"), Arr))
	{
		for (const TSharedPtr<FJsonValue>& V : *Arr)
		{
			if (!V.IsValid() || V->Type != EJson::Object) { continue; }
			const TSharedPtr<FJsonObject> O = V->AsObject();
			FBotBeat B;
			B.Id = JStr(O, TEXT("id"));
			const TSharedPtr<FJsonObject>* WP;
			if (O->TryGetObjectField(TEXT("when"), WP) && WP->IsValid())
			{
				const TSharedPtr<FJsonObject>& W = *WP;
				bool bNever = false;
				W->TryGetBoolField(TEXT("never"), bNever);
				B.bNever = bNever;
				B.TopAfter = JStrs(W, TEXT("after"));
				B.After = B.TopAfter;
				B.Flag = JStrs(W, TEXT("flag"));
				B.NoFlag = JStrs(W, TEXT("noflag"));
				B.Near = JStr(W, TEXT("near"));
				B.R = JNum(W, TEXT("r"), 600.f);
				B.Look = JStr(W, TEXT("look"));
				B.Posture = JStr(W, TEXT("posture"));
				const TArray<TSharedPtr<FJsonValue>>* Any;
				if (W->TryGetArrayField(TEXT("any"), Any))
				{
					for (const TSharedPtr<FJsonValue>& S : *Any)
					{
						if (!S.IsValid() || S->Type != EJson::Object) { continue; }
						const TSharedPtr<FJsonObject> SO = S->AsObject();
						B.After.Append(JStrs(SO, TEXT("after")));
						if (B.Near.IsEmpty() && !JStr(SO, TEXT("near")).IsEmpty()) { B.Near = JStr(SO, TEXT("near")); B.R = JNum(SO, TEXT("r"), 600.f); }
					}
				}
			}
			if (!B.Id.IsEmpty()) { Beats.Add(MoveTemp(B)); }
		}
	}
	// which beat waits for which (a later beat that fired without waiting for an open one means the walk went past it)
	const int32 N = Beats.Num();
	Dep.Init(TArray<bool>(), N);
	for (int32 a = 0; a < N; ++a)
	{
		Dep[a].Init(false, N);
		for (const FString& Id : Beats[a].After) { const int32 b = FindBeat(Id); if (b != INDEX_NONE) { Dep[a][b] = true; } }
	}
	for (int32 k = 0; k < N; ++k) { for (int32 a = 0; a < N; ++a) { if (Dep[a][k]) { for (int32 b = 0; b < N; ++b) { if (Dep[k][b]) { Dep[a][b] = true; } } } } }
	Note(FString::Printf(TEXT("%s: %d beats, %d places"), *Path, Beats.Num(), Places.Num()));
	return Beats.Num() > 0;
}

void UYggPlayBot::LoadRoute()
{
	FString Raw;
	TArray<TSharedPtr<FJsonValue>> Legs;
	const FString Path = FPaths::ProjectDir() / TEXT("Tools/RoutePlanning/route.json");
	if (FFileHelper::LoadFileToString(Raw, *Path) && FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Raw), Legs) && Legs.Num() > 0
		&& Legs[0].IsValid() && Legs[0]->Type == EJson::Object)
	{
		const TArray<TSharedPtr<FJsonValue>>* Pts;
		if (Legs[0]->AsObject()->TryGetArrayField(TEXT("pts"), Pts))
		{
			for (const TSharedPtr<FJsonValue>& V : *Pts)
			{
				if (!V.IsValid() || V->Type != EJson::Array) { continue; }
				const TArray<TSharedPtr<FJsonValue>>& A = V->AsArray();
				if (A.Num() >= 3) { Route.Add(FVector(A[0]->AsNumber(), A[1]->AsNumber(), A[2]->AsNumber())); }
			}
		}
	}
	RouteCum.SetNum(Route.Num());
	for (int32 i = 0; i < Route.Num(); ++i) { RouteCum[i] = i == 0 ? 0.f : RouteCum[i - 1] + float(FVector::Dist2D(Route[i - 1], Route[i])); }
	Note(Route.Num() > 1 ? FString::Printf(TEXT("route leg 0: %d points, %.1f km"), Route.Num(), RouteCum.Last() / 100000.f)
		: FString::Printf(TEXT("no route (%s): straight lines only"), *Path));
}

void UYggPlayBot::BeginStory()
{
	UWorld* W = GetWorld();
	UYggStorySubsystem* S = W ? W->GetSubsystem<UYggStorySubsystem>() : nullptr;
	if (!S) { Finish(TEXT("no story subsystem in this world")); return; }
	SetCVar(TEXT("ygg.Story.Auto"), TEXT("1"));
	SetCVar(TEXT("ygg.Story.Debug"), TEXT("1"));   // "Story: beat <id> done" lines: the bot knows when an "after" is met
	if (bFast)
	{
		if (IConsoleVariable* V = IConsoleManager::Get().FindConsoleVariable(TEXT("t.MaxFPS"))) { PrevMaxFps = V->GetString(); bMaxFpsSet = true; V->Set(TEXT("0"), ECVF_SetByConsole); }
	}
	if (!S->Start(Chapter)) { Finish(FString::Printf(TEXT("the story did not start %s"), *Chapter)); return; }
	bStoryStarted = true;
	Phase = EPhase::Playing;
	LastBeatGame = GameT;
	LastProgress = GameT;
	NextSweep = GameT + Rng.FRandRange(20.f, 35.f);
	NextDownUp = GameT + Rng.FRandRange(45.f, 70.f);
	JitterWindowEnd = GameT + 5.0;
	Note(FString::Printf(TEXT("story started: ygg.Story.Start %s, ygg.Story.Auto 1%s"), *Chapter, bFast ? TEXT(", t.MaxFPS 0, sprint") : TEXT("")));
}

void UYggPlayBot::Finish(const FString& Why, bool bAllowQuit)
{
	if (Phase != EPhase::Waiting && Phase != EPhase::Playing) { return; }
	EndWhy = Why;
	Note(FString::Printf(TEXT("end: %s"), *Why));
	MoveDir = FVector::ZeroVector;
	UWorld* W = GetWorld();
	APawn* P = W && W->GetFirstPlayerController() ? W->GetFirstPlayerController()->GetPawn() : nullptr;
	if (P && CVarBotRig.GetValueOnGameThread() != 0)
	{
		if (const UYggViewRigComponent* R = UYggViewRigComponent::Find(P)) { LastAudit = R->Audit(); }
	}
	if (P && bFast)
	{
		if (USurvivalComponent* S = P->FindComponentByClass<USurvivalComponent>()) { S->SetSprintRequested(false); }
	}
	DrainLog();
	WriteReport(true);
	if (LogDevice.IsValid() && GLog) { GLog->RemoveOutputDevice(LogDevice.Get()); }
	if (bMaxFpsSet)
	{
		if (IConsoleVariable* V = IConsoleManager::Get().FindConsoleVariable(TEXT("t.MaxFPS"))) { V->Set(*PrevMaxFps, ECVF_SetByConsole); }
		bMaxFpsSet = false;
	}
	if (bAllowQuit && CVarBotQuit.GetValueOnGameThread() != 0) { Phase = EPhase::Quitting; QuitLeft = 2.f; }
	else { Phase = EPhase::Idle; }
}

// ------------------------------------------------------------------------------------------------ every frame

void UYggPlayBot::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (Phase == EPhase::Idle) { return; }
	TRACE_CPUPROFILER_EVENT_SCOPE(UYggPlayBot_Tick);
	const float RealDt = float(FApp::GetDeltaTime());
	UWorld* W = GetWorld();
	APlayerController* PC = W ? W->GetFirstPlayerController() : nullptr;
	APawn* P = PC ? PC->GetPawn() : nullptr;
	if (Phase == EPhase::Quitting)
	{
		if ((QuitLeft -= RealDt) <= 0.f)
		{
			Phase = EPhase::Idle;
			UE_LOG(LogYggBot, Display, TEXT("Bot: quit"));
			if (PC) { PC->ConsoleCommand(TEXT("quit")); }
			else { FPlatformMisc::RequestExit(false, TEXT("ygg.Bot")); }
		}
		return;
	}
	GameT += DeltaTime;
	DrainLog();
	if (Phase == EPhase::Idle) { return; }
	RecordFrame(RealDt, P);
	if (Phase == EPhase::Waiting)
	{
		if (P && GameT >= 2.0) { BeginStory(); }
		else if (FPlatformTime::Seconds() - RealStart > 240.0) { Finish(TEXT("no player pawn after 4 minutes")); }
		return;
	}

	UYggStorySubsystem* S = W ? W->GetSubsystem<UYggStorySubsystem>() : nullptr;
	const bool bStoryOn = S && S->IsRunning();
	bStoryWasRunning |= bStoryOn;
	if (bChapterFinished) { Finish(FString::Printf(TEXT("%s finished"), *Chapter)); return; }
	// stopped without the "finished" line: give the log a moment to deliver it
	if (bStoryWasRunning && !bStoryOn && StoryStopAt < 0.0) { StoryStopAt = GameT; }
	if (bStoryOn) { StoryStopAt = -1.0; }
	if (StoryStopAt >= 0.0 && GameT - StoryStopAt > 3.0) { Finish(TEXT("the story stopped before the chapter end")); return; }
	if (!bStoryWasRunning && GameT > 30.0) { Finish(TEXT("the story never ran")); return; }
	if (GameT > CVarBotCapMin.GetValueOnGameThread() * 60.0) { Finish(FString::Printf(TEXT("total cap: %.0f min of game time"), CVarBotCapMin.GetValueOnGameThread())); return; }
	if (GameT - LastBeatGame > CVarBotStallMin.GetValueOnGameThread() * 60.0)
	{
		Finish(FString::Printf(TEXT("stalled: no beat for %.0f min (last target %s)"), CVarBotStallMin.GetValueOnGameThread(), *BeatName(NavBeat)));
		return;
	}

	if (P)
	{
		ReadRig(P, DeltaTime);
		if (GameT >= NextDecide) { NextDecide = GameT + 0.25; Decide(P); }
		if (Phase != EPhase::Playing) { return; }
		Drive(P);
	}
	if (PC) { LookAround(PC, FMath::Min(RealDt, 0.1f)); }
	const double Now = FPlatformTime::Seconds();
	if (Now - LastReport > 60.0) { LastReport = Now; WriteReport(false); }
}

void UYggPlayBot::DrainLog()
{
	if (!LogDevice.IsValid()) { return; }
	TPair<bool, FString> L;
	int32 Budget = 500;
	while (Budget-- > 0 && LogDevice->Lines.Dequeue(L)) { OnStoryLine(L.Value, L.Key); }
}

void UYggPlayBot::OnStoryLine(const FString& Full, bool bWarning)
{
	if (Phase == EPhase::Idle) { return; }
	int32 Nl;
	const FString Line = Full.FindChar(TEXT('\n'), Nl) ? Full.Left(Nl).TrimEnd() : Full.TrimEnd();
	if (Line.StartsWith(TEXT("CamRec: INCIDENT")))
	{
		++CamRecLogIncidents;
		if (CamRecLines.Num() < 30) { CamRecLines.Add(FString::Printf(TEXT("%s  %s"), *Clock(GameT), *Line.Mid(8).Left(220))); }
		return;
	}
	if (bWarning)
	{
		++StoryWarningCount;
		if (StoryWarnings.Num() < 200 || StoryWarnings.Contains(Line.Left(140))) { StoryWarnings.FindOrAdd(Line.Left(140))++; }
	}
	static const FString BeatPrefix = TEXT("Story: beat ");
	if (Line.StartsWith(BeatPrefix))
	{
		const FString Rest = Line.Mid(BeatPrefix.Len());
		int32 Sp;
		const FString Id = Rest.FindChar(TEXT(' '), Sp) ? Rest.Left(Sp) : Rest;
		const FString Tail = Sp != INDEX_NONE ? Rest.Mid(Sp + 1) : FString();
		const int32 i = FindBeat(Id);
		if (i == INDEX_NONE) { Note(FString::Printf(TEXT("story beat %s is not in the bot's copy of the chapter"), *Id)); return; }
		FBotBeat& B = Beats[i];
		if (Tail.StartsWith(TEXT("done")))
		{
			B.bDone = true;
			B.DoneGame = GameT;
			bSawDone = true;
			return;
		}
		if (Tail.StartsWith(TEXT("fires without the look"))) { B.bForcedLook = true; Note(FString::Printf(TEXT("%s fires without the look"), *Id)); return; }
		if (!Tail.IsEmpty() || B.bFired) { return; }
		B.bFired = true;
		B.FiredGame = GameT;
		B.FiredReal = FPlatformTime::Seconds() - RealStart;
		B.SegFps = SegTime > 0.0 ? float(double(SegFrames) / SegTime) : 0.f;
		SegFrames = 0;
		SegTime = 0.0;
		LastBeatGame = GameT;
		UWorld* W = GetWorld();
		const APawn* P = W && W->GetFirstPlayerController() ? W->GetFirstPlayerController()->GetPawn() : nullptr;
		if (P)
		{
			B.FiredPos = P->GetActorLocation();
			FVector At;
			if (!B.Near.IsEmpty() && Resolve(B.Near, At)) { B.FiredPlace = B.Near; B.FiredDist = float(FVector::Dist(B.FiredPos, At)); }
			else { float D = 0.f; B.FiredPlace = NearestPlace(B.FiredPos, D); B.FiredDist = D; }
		}
		Note(FString::Printf(TEXT("beat %s fired at %s (%s)"), *Id, *Cm(B.FiredPos), *B.FiredPlace));
		return;
	}
	if (Line.StartsWith(TEXT("Story: ")) && Line.EndsWith(TEXT(" finished"))) { bChapterFinished = true; return; }
	if (Line.Contains(TEXT("climber moved away from the teleport spot")))
	{
		++PutBacks;
		Note(GameT - LastBotTeleport < 3.0 ? TEXT("the story put the climber back right after a bot teleport (teleport undone)") : TEXT("the story put the climber back on its teleport spot"));
		return;
	}
	if (Line.StartsWith(TEXT("Story: camera cut")) || Line.StartsWith(TEXT("Story: jump to"))) { Note(Line); }
}

void UYggPlayBot::RecordFrame(float RealDt, APawn* P)
{
	if (RealDt <= 0.f) { return; }
	const float Ms = RealDt * 1000.f;
	++Frames;
	FrameSum += RealDt;
	++SegFrames;
	SegTime += RealDt;
	if (Hist.Num() == 4000) { Hist[FMath::Clamp(int32(Ms * 4.f), 0, 3999)]++; }
	WorstMs = FMath::Max(WorstMs, Ms);
	Over50 += Ms > 50.f ? 1 : 0;
	Over100 += Ms > 100.f ? 1 : 0;
	Over250 += Ms > 250.f ? 1 : 0;
	if (Ms > 50.f && GameT > 15.0)
	{
		int32 Min = INDEX_NONE;
		for (int32 i = 0; i < Hitches.Num(); ++i) { if (Min == INDEX_NONE || Hitches[i].Ms < Hitches[Min].Ms) { Min = i; } }
		FBotHitch H{ GameT, Ms, P ? P->GetActorLocation() : FVector::ZeroVector, BeatName(NavBeat) };
		if (Hitches.Num() < 12) { Hitches.Add(H); }
		else if (Hitches[Min].Ms < Ms) { Hitches[Min] = H; }
	}
}

void UYggPlayBot::ReadRig(APawn* P, float Dt)
{
	if (CVarBotRig.GetValueOnGameThread() == 0) { RigView = 0; RigLid = 0.f; return; }
	const UYggViewRigComponent* R = UYggViewRigComponent::Find(P);
	if (!R) { RigView = 0; RigLid = 0.f; return; }
	RigView = uint8(R->GetView());
	RigLid = R->GetLid();
	if (GameT > 5.0 && RigView == uint8(EYggView::Walk) && RigLid < 0.5f)
	{
		const float Eye = R->GetEyeAboveFloor();
		EyeMin = FMath::Min(EyeMin, Eye);
		EyeMax = FMath::Max(EyeMax, Eye);
	}
	PosedSeconds += RigView != uint8(EYggView::Walk) ? Dt : 0.f;
	LidShutSeconds += RigLid > 0.98f ? Dt : 0.f;
}

// ------------------------------------------------------------------------------------------------ the walk

void UYggPlayBot::Decide(APawn* P)
{
	const float Step = 0.25f;
	if (GameT - PeopleScan > 2.0) { ScanPeople(); }
	const FVector Pos = P->GetActorLocation();
	if (bHaveLastPos && FVector::Dist(Pos, LastPos) > 5000.0 && GameT - LastBotTeleport > 1.0)
	{
		++Jumps;
		Note(FString::Printf(TEXT("the climber jumped %.0f m (story cut, respawn...) to %s"), FVector::Dist(Pos, LastPos) / 100.0, *Cm(Pos)));
		LastProgress = GameT;
		Best = 1e12f;
	}
	LastPos = Pos;
	bHaveLastPos = true;
	const ACharacter* C = Cast<ACharacter>(P);
	const float MaxSpeed = C && C->GetCharacterMovement() ? C->GetCharacterMovement()->GetMaxSpeed() : 600.f;
	// held: the story stops the walk (inspect, choice, Move lease) or the eyes are shut. A posture is not "held": walk input
	// is what stands the climber up.
	bHeld = (MaxSpeed < 10.f && RigView == uint8(EYggView::Walk)) || RigLid > 0.5f;

	// the walk target: the first open beat (file order) that waits near a place or a person
	int32 NewNav = INDEX_NONE;
	FVector NewPos = FVector::ZeroVector;
	for (int32 i = 0; i < Beats.Num(); ++i)
	{
		FBotBeat& B = Beats[i];
		if (B.bFired || B.bSkipped || B.bNever || B.Near.IsEmpty() || !AfterOk(B)) { continue; }
		if (Superseded(i))
		{
			B.bSkipped = true;
			B.SkipWhy = TEXT("passed: a later beat that does not wait for it fired 30 s ago");
			Note(FString::Printf(TEXT("beat %s skipped (%s)"), *B.Id, *B.SkipWhy));
			continue;
		}
		FVector At;
		if (!Resolve(B.Near, At)) { continue; }
		NewNav = i;
		NewPos = At;
		break;
	}
	if (NewNav != NavBeat)
	{
		NavBeat = NewNav;
		bArrived = false;
		LastProgress = GameT;
		if (NavBeat != INDEX_NONE)
		{
			const float D = float(FVector::Dist2D(Pos, NewPos));
			bRouteMode = CVarBotPath.GetValueOnGameThread() == 1 && Route.Num() > 1 && D > 6000.f;
			Best = bRouteMode ? RouteRemaining(Pos, NewPos) : D;
			Note(FString::Printf(TEXT("target: beat %s, near %s (r %.0f m), %.0f m away"), *Beats[NavBeat].Id, *Beats[NavBeat].Near, Beats[NavBeat].R / 100.f, D / 100.f));
		}
	}
	NavPos = NewPos;

	// the look target: the first open beat with a "look" whose other conditions hold
	AimBeat = INDEX_NONE;
	for (int32 i = 0; i < Beats.Num(); ++i)
	{
		const FBotBeat& B = Beats[i];
		if (B.bFired || B.bSkipped || B.bNever || B.Look.IsEmpty() || !AfterOk(B)) { continue; }
		FVector At;
		if (!B.Near.IsEmpty() && (!Resolve(B.Near, At) || FVector::Dist(Pos, At) > B.R)) { continue; }
		if (Resolve(B.Look, AimPos)) { AimBeat = i; break; }
	}

	MoveDir = FVector::ZeroVector;
	if (NavBeat == INDEX_NONE) { return; }
	FBotBeat& B = Beats[NavBeat];
	const float D = float(FVector::Dist2D(Pos, NavPos));
	bArrived = D <= FMath::Max(B.R * 0.6f, 80.f);
	// a posture the beat waits for: sit / kneel / crawl the bot cannot take itself (the story puts the climber there)
	bool bPoseBlock = false, bSitUp = false;
	const uint8 Want = B.Posture.IsEmpty() ? uint8(255) : PoseByte(B.Posture);
	NavWantPose = Want != uint8(EYggView::Walk) ? Want : uint8(255);
	if (Want != 255 && Want != uint8(EYggView::Walk))
	{
		if (RigView == Want) { bPoseBlock = true; B.PostureWait = 0.f; }
		// lying: a held move input sits the climber up (the rig's Lie -> Sit); Drive lets go once the posture is reached
		else if (RigView == uint8(EYggView::Lie) && Want != uint8(EYggView::Lie)) { bSitUp = true; }
		else if (RigView == uint8(EYggView::Walk))
		{
			B.PostureWait += Step;
			if (B.PostureWait > 20.f)
			{
				B.bSkipped = true;
				B.SkipWhy = FString::Printf(TEXT("wants posture %s, the climber walks (the bot cannot take it)"), *B.Posture);
				Note(FString::Printf(TEXT("beat %s skipped (%s)"), *B.Id, *B.SkipWhy));
				NavBeat = INDEX_NONE;
				return;
			}
		}
		else { bPoseBlock = true; }   // another posture on its way (lie -> sit): wait for it
	}
	if (!bHeld) { B.TargetTime += Step; }
	// there (or posed and unable to walk there) and it does not fire: its look / press / posture never comes (2 min; 30 s
	// when a posture keeps the climber out of reach)
	const bool bInReach = bArrived || D <= B.R;
	B.StandWait = !bHeld && (bArrived || bPoseBlock) ? B.StandWait + Step : 0.f;
	if (B.StandWait > (bInReach ? 120.f : 30.f))
	{
		B.bSkipped = true;
		B.SkipWhy = bInReach ? TEXT("in reach, but it did not fire in 2 min") : TEXT("posed out of its reach for 30 s");
		Note(FString::Printf(TEXT("beat %s skipped (%s)"), *B.Id, *B.SkipWhy));
		NavBeat = INDEX_NONE;
		return;
	}

	// the per-beat cap: a teleport to the place, then 90 s, then skipped
	const float Cap = CVarBotBeatCapMin.GetValueOnGameThread() * 60.f;
	if (!bHeld && B.TargetTime > Cap)
	{
		if (!B.bCapTeleported)
		{
			B.bCapTeleported = true;
			B.TargetTime = FMath::Max(0.f, Cap - 90.f);
			const FVector Off = (Pos - NavPos).GetSafeNormal2D() * FMath::Min(100.f, B.R * 0.4f);
			const FVector To = NavPos + Off;
			FBotStuck S{ GameT, B.Id, Pos, D, FString() };
			const bool bTeleported = TeleportTo(P, To, FString::Printf(TEXT("beat cap %.0f min"), Cap / 60.f));
			S.Action = bTeleported ? FString::Printf(TEXT("beat cap %.0f min: teleport to %s"), Cap / 60.f, *B.Near) : FString(TEXT("beat cap: teleport failed"));
			Stucks.Add(S);
			CapTeleports += bTeleported ? 1 : 0;
		}
		else
		{
			B.bSkipped = true;
			B.SkipWhy = TEXT("beat cap: not fired 90 s after the teleport to its place");
			Note(FString::Printf(TEXT("beat %s skipped (%s)"), *B.Id, *B.SkipWhy));
			Stucks.Add({ GameT, B.Id, Pos, D, TEXT("beat cap: skipped") });
			NavBeat = INDEX_NONE;
		}
		return;
	}

	if (!bHeld && bSitUp)
	{
		const FVector Fwd = (NavPos - Pos).GetSafeNormal2D();
		MoveDir = Fwd.IsNearlyZero() ? P->GetActorForwardVector().GetSafeNormal2D() : Fwd;
	}
	else if (!bHeld && !bArrived && !bPoseBlock)
	{
		FVector Steer = NavPos - Pos;
		// crawling out of a tent: the rig stands the climber up only at the tent's exit, so crawl to the door first
		if (RigView == uint8(EYggView::Crawl))
		{
			static const TCHAR* const Doors[] = { TEXT("tent_out"), TEXT("tent_door") };
			for (const TCHAR* Door : Doors)
			{
				const FVector* At = Places.Find(FName(Door));
				if (At && FVector::Dist2D(Pos, *At) < 2000.0 && FVector::Dist2D(Pos, *At) > 40.0) { Steer = *At - Pos; break; }
			}
		}
		if (bRouteMode && D > 4000.f && RigView != uint8(EYggView::Crawl))
		{
			bool bOk = false;
			const FVector Ahead = RouteAhead(Pos, NavPos, 2500.f, bOk);
			if (bOk) { Steer = Ahead - Pos; }
		}
		Steer.Z = 0.f;
		Steer = Steer.GetSafeNormal();
		// a little sidestep while stuck: 2 s every 6 s, alternating sides
		const double NoProg = GameT - LastProgress;
		if (NoProg > 10.0)
		{
			const double Ph = NoProg - 10.0;
			if (FMath::Fmod(Ph, 6.0) < 2.0)
			{
				const float Side = (int32(Ph / 6.0) % 2) == 0 ? 75.f : -75.f;
				Steer = Steer.RotateAngleAxis(Side, FVector::UpVector);
			}
		}
		MoveDir = Steer;
	}
	if (GameT >= NextStuckCheck) { NextStuckCheck = GameT + 1.0; CheckStuck(P); }
}

void UYggPlayBot::CheckStuck(APawn* P)
{
	// lying / sitting / kneeling the climber does not travel (a held move input only changes the posture): never "stuck"
	const bool bStoryPose = RigView != uint8(EYggView::Walk) && RigView != uint8(EYggView::Crawl);
	if (NavBeat == INDEX_NONE || bHeld || bArrived || MoveDir.IsNearlyZero() || bStoryPose)
	{
		LastProgress = GameT;
		Best = 1e12f;
		return;
	}
	const FVector Pos = P->GetActorLocation();
	const float D = float(FVector::Dist2D(Pos, NavPos));
	const float Metric = bRouteMode ? RouteRemaining(Pos, NavPos) : D;
	if (Metric < Best - 150.f) { Best = Metric; LastProgress = GameT; return; }
	const double NoProg = GameT - LastProgress;
	if (!bRouteMode && NoProg > 8.0 && Route.Num() > 1 && D > 3000.f)
	{
		bRouteMode = true;
		Best = RouteRemaining(Pos, NavPos);
		Stucks.Add({ GameT, Beats[NavBeat].Id, Pos, D, TEXT("no progress 8 s: following the route.json path") });
		Note(FString::Printf(TEXT("no progress 8 s toward %s: route path from here"), *Beats[NavBeat].Near));
		return;
	}
	if (NoProg > CVarBotStuckSec.GetValueOnGameThread())
	{
		bool bOk = false;
		FVector To = RouteAhead(Pos, NavPos, 1500.f, bOk);
		if (!bOk)
		{
			const float Len = FMath::Min(1500.f, D - 150.f);
			To = Pos + (NavPos - Pos).GetSafeNormal2D() * Len;
			bOk = Len > 50.f;
		}
		FBotStuck S{ GameT, Beats[NavBeat].Id, Pos, D, FString() };
		if (bOk && TeleportTo(P, To, TEXT("stuck")))
		{
			S.Action = FString::Printf(TEXT("no progress %.0f s: teleport 15 m ahead to %s"), NoProg, *Cm(P->GetActorLocation()));
		}
		else { S.Action = FString::Printf(TEXT("no progress %.0f s: nowhere to teleport (target %.0f m away), waiting for the beat cap"), NoProg, D / 100.f); }
		Stucks.Add(S);
		Note(S.Action);
		LastProgress = GameT;
		Best = 1e12f;
	}
}

void UYggPlayBot::Drive(APawn* P)
{
	const bool bPosedAsWanted = NavWantPose != 255 && RigView == NavWantPose;
	if (!MoveDir.IsNearlyZero() && !bPosedAsWanted) { P->AddMovementInput(MoveDir, 1.f); }
	if (bFast && GameT >= NextSprint)
	{
		NextSprint = GameT + 2.0;
		if (USurvivalComponent* S = P->FindComponentByClass<USurvivalComponent>()) { S->SetSprintRequested(!MoveDir.IsNearlyZero()); }
	}
}

bool UYggPlayBot::TeleportTo(APawn* P, const FVector& Where, const FString& Why)
{
	UWorld* W = GetWorld();
	if (!W || !P) { return false; }
	ACharacter* C = Cast<ACharacter>(P);
	const float Half = C && C->GetCapsuleComponent() ? C->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() : 90.f;
	FCollisionQueryParams Q(SCENE_QUERY_STAT(YggBotGround), false, P);
	// from 3 m above first (no tent roof, no overhang); from 30 m above when the spot is under the ground there
	FHitResult Low, High;
	const bool bLow = W->LineTraceSingleByChannel(Low, Where + FVector(0, 0, 300), Where - FVector(0, 0, 8000), ECC_Pawn, Q);
	const bool bHigh = W->LineTraceSingleByChannel(High, Where + FVector(0, 0, 3000), Where - FVector(0, 0, 8000), ECC_Pawn, Q);
	FVector Ground = Where;
	if (bLow && FMath::Abs(Low.ImpactPoint.Z - Where.Z) < 600.f) { Ground = Low.ImpactPoint; }
	else if (bHigh) { Ground = High.ImpactPoint; }
	else if (bLow) { Ground = Low.ImpactPoint; }
	const FVector To = Ground + FVector(0, 0, Half + 10.f);
	const FVector From = P->GetActorLocation();
	if (!P->SetActorLocation(To, false, nullptr, ETeleportType::TeleportPhysics)) { return false; }
	if (C && C->GetCharacterMovement()) { C->GetCharacterMovement()->StopMovementImmediately(); }
	APlayerController* PC = Cast<APlayerController>(P->GetController());
	if (PC && PC->PlayerCameraManager) { PC->PlayerCameraManager->SetGameCameraCutThisFrame(); }
	// every teleport snaps the view rig's springs (spec: HardReset after every teleport), unless a posture or the eyelids
	// belong to the story right now
	if (CVarBotRig.GetValueOnGameThread() != 0 && CVarBotHardReset.GetValueOnGameThread() != 0)
	{
		UYggViewRigComponent* R = UYggViewRigComponent::Find(P);
		// walking, or crawling (the bot's own walk input started the crawl; teleported away from the tent it would never
		// reach the exit that ends it); story postures and shut eyelids are left alone
		if (R && (R->GetView() == EYggView::Walk || R->GetView() == EYggView::Crawl) && R->GetLid() < 0.5f)
		{
			R->HardReset(PC ? PC->GetControlRotation() : P->GetActorRotation(), EYggView::Walk);
		}
	}
	LastBotTeleport = GameT;
	LastPos = To;
	++Teleports;
	Note(FString::Printf(TEXT("teleport (%s): %s -> %s%s"), *Why, *Cm(From), *Cm(To), (bLow || bHigh) ? TEXT("") : TEXT(" (no ground found there)")));
	return true;
}

// ------------------------------------------------------------------------------------------------ the look

void UYggPlayBot::LookAround(APlayerController* PC, float Dt)
{
	if (Dt <= 0.f) { return; }
	const FRotator CR = PC->GetControlRotation();
	const float Pitch = float(FRotator::NormalizeAxis(CR.Pitch));
	const float Yaw = float(CR.Yaw);
	MinPitchSeen = FMath::Min(MinPitchSeen, Pitch);
	MaxPitchSeen = FMath::Max(MaxPitchSeen, Pitch);
	const bool bStress = CVarBotCamera.GetValueOnGameThread() != 0 && RigLid < 0.5f && RigView != uint8(EYggView::Lie);
	float DY = 0.f, DP = 0.f;   // degrees this frame

	if (AimBeat != INDEX_NONE)
	{
		FVector Eye;
		FRotator Unused;
		PC->GetPlayerViewPoint(Eye, Unused);
		const FRotator Want = (AimPos - Eye).Rotation();
		const float EY = float(FRotator::NormalizeAxis(Want.Yaw - double(Yaw)));
		const float EP = float(FRotator::NormalizeAxis(Want.Pitch)) - Pitch;
		DY = FMath::Abs(EY) > 0.5f ? FMath::Clamp(EY * 5.f * Dt, -120.f * Dt, 120.f * Dt) : 0.f;
		DP = FMath::Abs(EP) > 0.5f ? FMath::Clamp(EP * 5.f * Dt, -60.f * Dt, 60.f * Dt) : 0.f;
		if (Look == ELook::Sweep) { Look = ELook::Follow; }
		NextSweep = FMath::Max(NextSweep, GameT + 6.0);
		NextDownUp = FMath::Max(NextDownUp, GameT + 10.0);
		if ((AimAccum += Dt) >= 1.f) { AimAccum -= 1.f; ++AimSeconds; }
	}
	else
	{
		switch (Look)
		{
		case ELook::Follow:
		{
			// a walking player looks roughly where he goes, a little down
			if (!MoveDir.IsNearlyZero())
			{
				const float EY = float(FRotator::NormalizeAxis(MoveDir.Rotation().Yaw - double(Yaw)));
				DY = FMath::Abs(EY) > 2.f ? FMath::Clamp(EY * 1.5f * Dt, -90.f * Dt, 90.f * Dt) : 0.f;
			}
			const float EP = -6.f - Pitch;
			DP = FMath::Abs(EP) > 2.f ? FMath::Clamp(EP * 1.5f * Dt, -40.f * Dt, 40.f * Dt) : 0.f;
			if (bStress && GameT >= NextSweep)
			{
				Look = ELook::Sweep;
				SweepLeft = Rng.FRandRange(270.f, 540.f);
				SweepRate = Rng.FRandRange(360.f, 720.f) * (Rng.FRand() < 0.5f ? -1.f : 1.f);
				++Sweeps;
			}
			else if (bStress && GameT >= NextDownUp)
			{
				Look = ELook::Down;
				HoldLeft = 1.2f;
				StallFor = 0.f;
				++DownUps;
			}
			break;
		}
		case ELook::Sweep:
			DY = SweepRate * Dt;
			SweepLeft -= FMath::Abs(DY);
			if (SweepLeft <= 0.f || !bStress) { Look = ELook::Follow; NextSweep = GameT + Rng.FRandRange(25.f, 45.f); }
			break;
		case ELook::Down:
		case ELook::Up:
		{
			const float Target = Look == ELook::Down ? -89.f : 89.f;
			const float E = Target - Pitch;
			DP = FMath::Clamp(E * 6.f * Dt, -200.f * Dt, 200.f * Dt);
			// reached, or the view rig's pitch limit stops it
			if (FMath::Abs(E) < 2.f || StallFor > 0.4f)
			{
				if ((HoldLeft -= Dt) <= 0.f)
				{
					if (Look == ELook::Down) { Look = ELook::Up; HoldLeft = 1.2f; StallFor = 0.f; }
					else { Look = ELook::Back; StallFor = 0.f; }
				}
			}
			if (!bStress) { Look = ELook::Back; StallFor = 0.f; }
			break;
		}
		case ELook::Back:
		{
			const float E = -6.f - Pitch;
			DP = FMath::Clamp(E * 4.f * Dt, -150.f * Dt, 150.f * Dt);
			if (FMath::Abs(E) < 3.f || StallFor > 0.6f) { Look = ELook::Follow; NextDownUp = GameT + Rng.FRandRange(70.f, 110.f); }
			break;
		}
		}
		// small random look jitter, in windows (on 8-15 s, off 4-8 s, so "the player is still" happens too)
		if (bStress && Look == ELook::Follow)
		{
			if (GameT >= JitterWindowEnd)
			{
				bJitterOn = !bJitterOn;
				JitterWindowEnd = GameT + (bJitterOn ? Rng.FRandRange(8.f, 15.f) : Rng.FRandRange(4.f, 8.f));
			}
			if (bJitterOn)
			{
				if (GameT >= NextJitter) { NextJitter = GameT + 0.3; JitterGoal = FVector2D(Rng.FRandRange(-6.f, 6.f), Rng.FRandRange(-3.f, 3.f)); }
				Jitter += (JitterGoal - Jitter) * FMath::Min(1.f, Dt * 4.f);
				DY += float(Jitter.X) * Dt;
				DP += float(Jitter.Y) * Dt;
			}
		}
	}
	const bool bPitching = Look == ELook::Down || Look == ELook::Up || Look == ELook::Back;
	StallFor = bPitching && FMath::Abs(Pitch - PrevPitch) < 0.05f ? StallFor + Dt : 0.f;
	PrevPitch = Pitch;

	// AddYawInput / AddPitchInput scale by the legacy input scales when those are on (DefaultInput.ini: on): undo it, so
	// the numbers above are degrees
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
	if (FMath::Max(FMath::Abs(DY), FMath::Abs(DP)) > 20.f * Dt)
	{
		FeedYaw = DY / Dt;
		FeedPitch = DP / Dt;
		FeedAt = FPlatformTime::Seconds();
	}
}

// ------------------------------------------------------------------------------------------------ helpers

int32 UYggPlayBot::FindBeat(const FString& Id) const
{
	for (int32 i = 0; i < Beats.Num(); ++i) { if (Beats[i].Id == Id) { return i; } }
	return INDEX_NONE;
}

bool UYggPlayBot::AfterOk(const FBotBeat& B) const
{
	for (const FString& A : B.TopAfter)
	{
		const int32 i = FindBeat(A);
		if (i == INDEX_NONE) { return false; }
		const FBotBeat& O = Beats[i];
		// done (the story's debug line), or fired a while ago when no "done" line ever came
		const bool bOk = O.bDone || (O.bFired && GameT - O.FiredGame > (bSawDone ? 300.0 : 20.0));
		if (!bOk) { return false; }
	}
	if (B.Flag.Num() || B.NoFlag.Num())
	{
		const UWorld* W = GetWorld();
		const UYggStorySubsystem* S = W ? W->GetSubsystem<UYggStorySubsystem>() : nullptr;
		if (S)
		{
			for (const FString& F : B.Flag) { if (!S->HasFlag(FName(*F))) { return false; } }
			for (const FString& F : B.NoFlag) { if (S->HasFlag(FName(*F))) { return false; } }
		}
	}
	return true;
}

bool UYggPlayBot::Superseded(int32 Index) const
{
	for (int32 j = Index + 1; j < Beats.Num(); ++j)
	{
		const FBotBeat& C = Beats[j];
		if (C.bFired && GameT - C.FiredGame > 30.0 && Dep.IsValidIndex(j) && Dep[j].IsValidIndex(Index) && !Dep[j][Index]) { return true; }
	}
	return false;
}

void UYggPlayBot::ScanPeople()
{
	PeopleScan = GameT;
	People.Reset();
	if (UWorld* W = GetWorld())
	{
		for (TActorIterator<AYggStoryNPC> It(W); It; ++It) { if (!It->Id.IsNone()) { People.Add(It->Id, TWeakObjectPtr<AYggStoryNPC>(*It)); } }
	}
}

bool UYggPlayBot::Resolve(const FString& Name, FVector& Out)
{
	if (Name.IsEmpty() || Name == TEXT("player")) { return false; }
	const FName Key(*Name);
	// people first, as the story's Where()
	const TWeakObjectPtr<AYggStoryNPC>* N = People.Find(Key);
	if ((!N || !N->IsValid()) && !Places.Contains(Key) && GameT - PeopleScan > 1.0) { ScanPeople(); N = People.Find(Key); }
	if (N && N->IsValid()) { Out = (*N)->GetActorLocation(); return true; }
	if (const FVector* V = Places.Find(Key)) { Out = *V; return true; }
	return false;
}

FString UYggPlayBot::NearestPlace(const FVector& At, float& OutDist) const
{
	FName BestKey;
	double BestD = 1e18;
	for (const TPair<FName, FVector>& KV : Places)
	{
		const double D = FVector::Dist(At, KV.Value);
		if (D < BestD) { BestD = D; BestKey = KV.Key; }
	}
	OutDist = BestKey.IsNone() ? -1.f : float(BestD);
	return BestKey.IsNone() ? FString(TEXT("-")) : BestKey.ToString();
}

int32 UYggPlayBot::NearestRoute(const FVector& At) const
{
	int32 BestI = INDEX_NONE;
	double BestD = 1e18;
	for (int32 i = 0; i < Route.Num(); ++i)
	{
		const double D = FVector::DistSquared2D(At, Route[i]);
		if (D < BestD) { BestD = D; BestI = i; }
	}
	return BestI;
}

float UYggPlayBot::RouteRemaining(const FVector& From, const FVector& To) const
{
	if (Route.Num() < 2) { return float(FVector::Dist2D(From, To)); }
	const int32 a = NearestRoute(From), b = NearestRoute(To);
	return FMath::Abs(RouteCum[b] - RouteCum[a]) + float(FVector::Dist2D(From, Route[a])) + float(FVector::Dist2D(To, Route[b]));
}

FVector UYggPlayBot::RouteAhead(const FVector& From, const FVector& To, float Dist, bool& bOk) const
{
	bOk = false;
	if (Route.Num() < 2) { return From; }
	const int32 a = NearestRoute(From), b = NearestRoute(To);
	// the route helps only when both ends are close to it and the target is not at the same route point
	if (a == b || FVector::Dist2D(From, Route[a]) > 15000.0 || FVector::Dist2D(To, Route[b]) > 30000.0) { return From; }
	const float S0 = RouteCum[a], ST = RouteCum[b];
	const float S = ST > S0 ? FMath::Min(S0 + Dist, ST) : FMath::Max(S0 - Dist, ST);
	const int32 k = FMath::Clamp(int32(Algo::UpperBound(RouteCum, S)) - 1, 0, Route.Num() - 2);
	const float Seg = RouteCum[k + 1] - RouteCum[k];
	const float T = Seg > 1.f ? FMath::Clamp((S - RouteCum[k]) / Seg, 0.f, 1.f) : 0.f;
	bOk = true;
	return FMath::Lerp(Route[k], Route[k + 1], T);
}

float UYggPlayBot::Percentile(double Frac) const
{
	if (Frames <= 0 || Hist.Num() != 4000) { return 0.f; }
	const int64 Want = int64(Frac * double(Frames));
	int64 Sum = 0;
	for (int32 i = 0; i < Hist.Num(); ++i)
	{
		Sum += Hist[i];
		if (Sum > Want) { return (float(i) + 0.5f) / 4.f; }
	}
	return 1000.f;
}

int32 UYggPlayBot::CountIncidents(FString& OutWhere) const
{
	// the camera recorder (CamRec) writes Saved/Monitor/incidents/<session>/: the session folder of this run is the newest
	// one written since the game started (a few minutes before the bot)
	const FString Root = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("Monitor") / TEXT("incidents"));
	IFileManager& FM = IFileManager::Get();
	if (!FM.DirectoryExists(*Root)) { OutWhere = Root; return -1; }
	const FDateTime Since = StartUtc - FTimespan::FromMinutes(10.0);
	TArray<FString> Sessions;
	FM.FindFiles(Sessions, *(Root / TEXT("*")), false, true);
	FString Newest;
	FDateTime NewestT = FDateTime::MinValue();
	for (const FString& S : Sessions)
	{
		const FDateTime T = FM.GetTimeStamp(*(Root / S));
		if (T >= Since && T > NewestT) { NewestT = T; Newest = S; }
	}
	const FString Folder = Newest.IsEmpty() ? Root : Root / Newest;
	OutWhere = Folder;
	TArray<FString> Dirs, Files;
	FM.FindFiles(Dirs, *(Folder / TEXT("*")), false, true);
	FM.FindFiles(Files, *(Folder / TEXT("*")), true, false);
	if (Newest.IsEmpty())
	{
		// no session folder: incidents written straight into the root since the run started
		int32 N = 0;
		for (const FString& F : Files) { N += FM.GetTimeStamp(*(Root / F)) >= StartUtc ? 1 : 0; }
		return N;
	}
	if (Dirs.Num() > 0) { return Dirs.Num(); }
	int32 N = 0;
	for (const FString& F : Files) { N += F.EndsWith(TEXT(".md")) || F.StartsWith(TEXT("index")) ? 0 : 1; }
	return N;
}

// ------------------------------------------------------------------------------------------------ the report

FString UYggPlayBot::BuildReport(bool bFinal) const
{
	TStringBuilder<4096> S;
	int32 Fired = 0;
	for (const FBotBeat& B : Beats) { Fired += B.bFired ? 1 : 0; }
	const double Real = FPlatformTime::Seconds() - RealStart;
	S.Appendf(TEXT("# Robot run: %s%s\n\n"), *Chapter, bFast ? TEXT(" (fast)") : TEXT(""));
	S.Appendf(TEXT("- **Result:** %s\n"), bFinal ? *EndWhy : TEXT("still running (progress report, rewritten every 60 s)"));
	S.Appendf(TEXT("- Beats fired: %d / %d\n"), Fired, Beats.Num());
	S.Appendf(TEXT("- Game time %s, real time %s; started %s (local), seed %d (ygg.Bot.Seed %d repeats the camera stress)\n"),
		*Clock(GameT), *Clock(Real), *(StartUtc + (FDateTime::Now() - FDateTime::UtcNow())).ToString(TEXT("%Y-%m-%d %H:%M:%S")), Seed, Seed);
	S.Appendf(TEXT("- Teleports by the bot: %d (stuck) + %d (beat cap); story put-backs: %d; other jumps of the climber (story cuts, respawns): %d\n"),
		Teleports - CapTeleports, CapTeleports, PutBacks, Jumps);
	if (!bFinal) { S.Appendf(TEXT("- Now: %s\n"), *Status()); }
	S.Append(TEXT("\n## Beats\n\n| # | Beat | Fired (game) | Real | Place | Player at (cm) | From place (m) | FPS since the last beat | Note |\n|---|---|---|---|---|---|---|---|---|\n"));
	for (int32 i = 0; i < Beats.Num(); ++i)
	{
		const FBotBeat& B = Beats[i];
		FString Info;
		if (B.bForcedLook) { Info += TEXT("fired without the look; "); }
		if (B.bCapTeleported) { Info += TEXT("cap teleport; "); }
		if (B.bDone && B.bFired) { Info += FString::Printf(TEXT("done %s; "), *Clock(B.DoneGame)); }
		if (B.bSkipped) { Info += TEXT("**skipped**: ") + B.SkipWhy; }
		else if (!B.bFired)
		{
			Info += B.bNever ? TEXT("never (by design)") : (AfterOk(B) ? TEXT("**not fired** (ready)") : TEXT("not fired (waiting for an earlier beat)"));
			if (B.TargetTime > 0.f) { Info += FString::Printf(TEXT(", walk target %.0f s"), B.TargetTime); }
		}
		if (B.bFired)
		{
			S.Appendf(TEXT("| %d | %s | %s | %s | %s | %s | %.1f | %.0f | %s |\n"), i + 1, *B.Id, *Clock(B.FiredGame), *Clock(B.FiredReal), *B.FiredPlace,
				*Cm(B.FiredPos), B.FiredDist >= 0.f ? B.FiredDist / 100.f : -1.f, B.SegFps, *Info);
		}
		else
		{
			S.Appendf(TEXT("| %d | %s | - | - | %s | - | - | - | %s |\n"), i + 1, *B.Id, B.Near.IsEmpty() ? TEXT("-") : *B.Near, *Info);
		}
	}
	S.Appendf(TEXT("\n## Stuck events (%d)\n\n"), Stucks.Num());
	if (Stucks.Num() == 0) { S.Append(TEXT("None.\n")); }
	else
	{
		S.Append(TEXT("| Game | Target beat | Player at (cm) | Target (m) | What the bot did |\n|---|---|---|---|---|\n"));
		for (const FBotStuck& St : Stucks) { S.Appendf(TEXT("| %s | %s | %s | %.0f | %s |\n"), *Clock(St.Game), *St.Beat, *Cm(St.Pos), St.Dist / 100.f, *St.Action); }
	}
	S.Append(TEXT("\n## FPS\n\n"));
	if (Frames > 0 && FrameSum > 0.0)
	{
		const float P50 = Percentile(0.5), P99 = Percentile(0.99), P999 = Percentile(0.999);
		S.Appendf(TEXT("- %lld frames, average **%.1f FPS** (%.2f ms), median %.1f FPS\n"), Frames, double(Frames) / FrameSum, 1000.0 * FrameSum / double(Frames), P50 > 0.f ? 1000.f / P50 : 0.f);
		S.Appendf(TEXT("- 1%% low %.1f FPS (%.1f ms), 0.1%% low %.1f FPS (%.1f ms), worst frame %.0f ms\n"), P99 > 0.f ? 1000.f / P99 : 0.f, P99, P999 > 0.f ? 1000.f / P999 : 0.f, P999, WorstMs);
		S.Appendf(TEXT("- frames over 50 ms: %d, over 100 ms: %d, over 250 ms: %d%s\n"), Over50, Over100, Over250, bFast ? TEXT(" (t.MaxFPS 0)") : TEXT(""));
		if (Hitches.Num())
		{
			TArray<FBotHitch> H = Hitches;
			H.Sort([](const FBotHitch& A, const FBotHitch& B) { return A.Ms > B.Ms; });
			S.Append(TEXT("\nWorst hitches (after the first 15 s):\n\n| Game | ms | Player at (cm) | Target beat |\n|---|---|---|---|\n"));
			for (const FBotHitch& X : H) { S.Appendf(TEXT("| %s | %.0f | %s | %s |\n"), *Clock(X.Game), X.Ms, *Cm(X.Pos), *X.Beat); }
		}
	}
	else { S.Append(TEXT("No frames recorded.\n")); }
	S.Append(TEXT("\n## Camera\n\n"));
	S.Appendf(TEXT("- Stress (ygg.Bot.Camera %d): %d fast yaw sweeps, %d straight down / up, jitter windows; aimed at look targets %d s\n"),
		CVarBotCamera.GetValueOnGameThread(), Sweeps, DownUps, AimSeconds);
	S.Appendf(TEXT("- Control pitch reached: %.1f (down) .. %.1f (up) degrees\n"), MinPitchSeen, MaxPitchSeen);
	if (CVarBotRig.GetValueOnGameThread() != 0)
	{
		S.Appendf(TEXT("- View rig: posed %.0f s, eyelids shut %.0f s, eye above the floor while walking %s\n"), PosedSeconds, LidShutSeconds,
			EyeMin <= EyeMax ? *FString::Printf(TEXT("%.0f .. %.0f cm"), EyeMin, EyeMax) : TEXT("not seen"));
	}
	FString Where;
	const int32 Inc = CountIncidents(Where);
	S.Appendf(TEXT("- **CamRec incidents in this run's log: %d**\n"), CamRecLogIncidents);
	if (Inc >= 0) { S.Appendf(TEXT("- CamRec folders: %d (%s)\n"), Inc, *Where); }
	else { S.Appendf(TEXT("- CamRec folders: none (%s does not exist: is ygg.CamRec on? -YggNoSave turns it off unless -YggCamRec)\n"), *Where); }
	for (const FString& L : CamRecLines) { S.Appendf(TEXT("  - %s\n"), *L); }
	if (!LastAudit.IsEmpty()) { S.Appendf(TEXT("\nView rig audit at the end:\n\n```\n%s\n```\n"), *LastAudit); }
	S.Appendf(TEXT("\n## Story and view warnings (%d)\n\n"), StoryWarningCount);
	if (StoryWarnings.Num() == 0) { S.Append(TEXT("None.\n")); }
	else
	{
		TArray<TPair<FString, int32>> W;
		for (const TPair<FString, int32>& KV : StoryWarnings) { W.Add(KV); }
		W.Sort([](const TPair<FString, int32>& A, const TPair<FString, int32>& B) { return A.Value > B.Value; });
		for (int32 i = 0; i < W.Num() && i < 25; ++i) { S.Appendf(TEXT("- %dx %s\n"), W[i].Value, *W[i].Key); }
	}
	S.Appendf(TEXT("\n## Event log (%d)\n\n```\n"), Events.Num());
	const int32 First = FMath::Max(0, Events.Num() - 400);
	if (First > 0) { S.Appendf(TEXT("... %d earlier events left out\n"), First); }
	for (int32 i = First; i < Events.Num(); ++i) { S.Appendf(TEXT("%8s  %s\n"), *Clock(Events[i].Game), *Events[i].Text); }
	S.Append(TEXT("```\n\nThe engine log of this run: Saved/Logs/Bot.log (or %LOCALAPPDATA%/UnrealEngine/5.8/Saved/Logs/Bot.log). Bot lines start with \"Bot \".\n"));
	return FString(S.ToView());
}

void UYggPlayBot::WriteReport(bool bFinal)
{
	if (Dir.IsEmpty()) { return; }
	const FString Text = BuildReport(bFinal);
	if (bFinal)
	{
		FFileHelper::SaveStringToFile(Text, *(Dir / TEXT("report.md")), FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
		FFileHelper::SaveStringToFile(EndWhy + TEXT("\n"), *(Dir / TEXT("done.txt")), FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
		UE_LOG(LogYggBot, Display, TEXT("Bot: report %s"), *(Dir / TEXT("report.md")));
	}
	else
	{
		// off the game thread (a slow disk must not hitch the run); its own file, so it never overwrites the final report
		const FString Path = Dir / TEXT("progress.md");
		Async(EAsyncExecution::ThreadPool, [Text, Path]() { FFileHelper::SaveStringToFile(Text, *Path, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM); });
	}
}
