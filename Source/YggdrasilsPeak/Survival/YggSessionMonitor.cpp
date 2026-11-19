#include "Survival/YggSessionMonitor.h"
#include "Survival/YggSettings.h"
#include "Survival/SurvivalComponent.h"
#include "Survival/ExpeditionComponent.h"
#include "Survival/SanityComponent.h"
#include "Survival/ClimbingComponent.h"
#include "Survival/YggTime.h"
#include "Survival/YggViewRig.h"
#include "Environment/RealmSky.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Async/Async.h"
#include "Containers/Queue.h"
#include "HAL/PlatformFileManager.h"
#include "HAL/PlatformProcess.h"
#include "HAL/PlatformStackWalk.h"
#include "HAL/Runnable.h"
#include "HAL/RunnableThread.h"
#include "Blueprint/UserWidget.h"
#include "IImageWrapper.h"
#include "IImageWrapperModule.h"
#include "Modules/ModuleManager.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Framework/Application/IInputProcessor.h"
#include "Framework/Application/SlateApplication.h"
#include "Engine/GameInstance.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "ContentStreaming.h"
#include "DynamicRHI.h"
#include "RHIStats.h"
#include "RenderTimer.h"
#include "UnrealClient.h"
#include "HAL/FileManager.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformMemory.h"
#include "Misc/App.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/OutputDevice.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "UObject/UObjectGlobals.h"
#include <atomic>
#include <type_traits>
#include <utility>

static TAutoConsoleVariable<int32> CVarMonitor(TEXT("ygg.Monitor"),
#if UE_BUILD_SHIPPING
	0,
#else
	1,
#endif
	TEXT("1 = record the play session to Saved/Monitor (frames, hitches, input, log, body, screenshots), 0 = off. Read at start."));
static TAutoConsoleVariable<float> CVarMonitorShotEvery(TEXT("ygg.Monitor.ShotEvery"), 120.f,
	TEXT("Seconds between monitor screenshots, the first 20 s in (0 = only at big hitches). Each costs one short pause (the frame is read back)."));
static TAutoConsoleVariable<int32> CVarMonitorStallStacks(TEXT("ygg.Monitor.StallStacks"), 1,
	TEXT("1 = when the game thread is stuck for 0.4 s the monitor's watchdog takes its call stack (stalls.txt; the game thread is paused for a moment), 0 = only the time. Read at start."));

namespace
{
	// prefixed: this file and YggTelemetryComponent.cpp both had a Csv(), and a unity build puts both in one translation unit
	FString MonitorCsv(FString S) { S.ReplaceInline(TEXT("\""), TEXT("'")); S.ReplaceInline(TEXT("\n"), TEXT(" ")); S.ReplaceInline(TEXT("\r"), TEXT("")); return TEXT("\"") + S + TEXT("\""); }
	float MonitorMs(uint32 Cycles) { return float(FPlatformTime::ToMilliseconds(Cycles)); }

	// ---- the view rig (camera spec §10.1). The fixed API gives the posture, the eye height, the eyelids and Audit();
	// the takeover, the nudge and the leases need getters beyond it. They are used when the rig has them under these
	// names (GetScripted, IsNudging, GetLeaseMask), else the column says "-": this file compiles either way.
	template<typename T, typename = void> struct TMonitorRigHasScripted : std::false_type {};
	template<typename T> struct TMonitorRigHasScripted<T, std::void_t<decltype(std::declval<const T&>().GetScripted())>> : std::true_type {};
	template<typename T, typename = void> struct TMonitorRigHasNudging : std::false_type {};
	template<typename T> struct TMonitorRigHasNudging<T, std::void_t<decltype(std::declval<const T&>().IsNudging())>> : std::true_type {};
	template<typename T, typename = void> struct TMonitorRigHasLeaseMask : std::false_type {};
	template<typename T> struct TMonitorRigHasLeaseMask<T, std::void_t<decltype(std::declval<const T&>().GetLeaseMask())>> : std::true_type {};

	/** -1 unknown, 0 none, 1 fall, 2 knock, 3 fly */
	template<typename T> int32 MonitorRigScripted(const T* R)
	{
		if constexpr (TMonitorRigHasScripted<T>::value)
		{
			const int32 V = int32(R->GetScripted());
			return V == int32(EYggScripted::Fall) ? 1 : V == int32(EYggScripted::Knock) ? 2 : V == int32(EYggScripted::Fly) ? 3 : 0;
		}
		else
		{
			(void)R;
			return -1;
		}
	}
	/** -1 unknown, 0 / 1 */
	template<typename T> int32 MonitorRigNudging(const T* R)
	{
		if constexpr (TMonitorRigHasNudging<T>::value) { return R->IsNudging() ? 1 : 0; }
		else { (void)R; return -1; }
	}
	/** -1 unknown, else bit (1 << EYggLease) per lease held */
	template<typename T> int64 MonitorRigLeaseMask(const T* R)
	{
		if constexpr (TMonitorRigHasLeaseMask<T>::value) { return int64(uint32(R->GetLeaseMask())); }
		else { (void)R; return -1; }
	}

	const TCHAR* const MonitorScriptedNames[] = { TEXT("-"), TEXT("None"), TEXT("Fall"), TEXT("Knock"), TEXT("Fly") };   // index = code + 1
	const TCHAR* const MonitorPostureNames[] = { TEXT("-"), TEXT("stand"), TEXT("lie"), TEXT("sit"), TEXT("kneel1"), TEXT("kneel2"), TEXT("crawl") };

	/** -1 no rig, 0 stand, 1 lie, 2 sit, 3 kneel1, 4 kneel2, 5 crawl (the names V2 uses) */
	int32 MonitorPostureCode(EYggView V)
	{
		switch (V)
		{
		case EYggView::Walk: return 0;
		case EYggView::Lie: return 1;
		case EYggView::Sit: return 2;
		case EYggView::Kneel1: return 3;
		case EYggView::Kneel2: return 4;
		case EYggView::Crawl: return 5;
		default: return 0;
		}
	}

	FString MonitorLeaseNames(int64 Mask)
	{
		struct FLeaseName { EYggLease Kind; const TCHAR* Name; };
		static const FLeaseName Names[] = {
			{ EYggLease::Move, TEXT("Move") }, { EYggLease::Look, TEXT("Look") }, { EYggLease::Pace, TEXT("Pace") }, { EYggLease::Fall, TEXT("Fall") },
			{ EYggLease::Knock, TEXT("Knock") }, { EYggLease::Fly, TEXT("Fly") }, { EYggLease::Inspect, TEXT("Inspect") }, { EYggLease::Hands, TEXT("Hands") },
			{ EYggLease::Attention, TEXT("Attention") }, { EYggLease::FovBeat, TEXT("FovBeat") }, { EYggLease::StoryFov, TEXT("StoryFov") },
			{ EYggLease::Nudge, TEXT("Nudge") }, { EYggLease::Lids, TEXT("Lids") }, { EYggLease::Breath, TEXT("Breath") }, { EYggLease::Exposure, TEXT("Exposure") },
			{ EYggLease::Lens, TEXT("Lens") } };
		FString S;
		for (const FLeaseName& N : Names)
		{
			if (Mask & (int64(1) << int32(N.Kind))) { if (!S.IsEmpty()) { S += TEXT("|"); } S += N.Name; }
		}
		return S.IsEmpty() ? FString(TEXT("none")) : S;
	}

	/** Eye above the floor measured from the camera itself (no rig, or ygg.View.Rig 0): floor = capsule bottom - FloorDist. */
	float MonitorEyeAboveFloor(const APawn* P, const FVector& CamLoc)
	{
		const ACharacter* C = Cast<ACharacter>(P);
		const UCapsuleComponent* Cap = C ? C->GetCapsuleComponent() : nullptr;
		if (!Cap) { return -1.f; }
		double Floor = C->GetActorLocation().Z - Cap->GetScaledCapsuleHalfHeight();
		if (const UCharacterMovementComponent* M = C->GetCharacterMovement()) { if (M->CurrentFloor.bBlockingHit) { Floor -= M->CurrentFloor.FloorDist; } }
		return float(CamLoc.Z - Floor);
	}

	FString MonitorClock(double T) { return FString::Printf(TEXT("%d:%04.1f"), int32(T / 60.0), FMath::Fmod(T, 60.0)); }
}

/** Moments behind a failed check: runs of bad frames merged (a gap under Merge seconds joins them); the first 25 listed. */
struct FYggCamEpisode { double T0 = 0.0, T1 = 0.0; float Peak = 0.f; FString What; };
struct FYggCamEpisodes
{
	TArray<FYggCamEpisode> L;
	int32 Count = 0;
	double LastT = -100.0;
	void Add(double T, float Value, const TCHAR* Kind, const TCHAR* Context, bool bLow = false, double Merge = 0.5)
	{
		if (Count > 0 && T - LastT <= Merge)
		{
			if (L.Num() > 0 && L.Last().T1 >= LastT - 0.0001)
			{
				FYggCamEpisode& E = L.Last();
				E.T1 = T;
				E.Peak = bLow ? FMath::Min(E.Peak, Value) : FMath::Max(E.Peak, Value);
			}
		}
		else
		{
			++Count;
			if (L.Num() < 25) { FYggCamEpisode& E = L.AddDefaulted_GetRef(); E.T0 = E.T1 = T; E.Peak = Value; E.What = FString::Printf(TEXT("%s (%s)"), Kind, Context); }
		}
		LastT = T;
	}
};

/** The camera spec's §11 checks that a play session can measure, counted frame by frame from the values written to
 *  frames.csv (a handful of compares per frame: no traces, nothing allocated unless a moment gets listed). */
struct FYggCamChecks
{
	static constexpr int32 MaxList = 25;
	int64 PlayFrames = 0, RigFrames = 0;
	bool bScriptedKnown = false, bLeasesKnown = false, bNudgeKnown = false;
	// V1: no grab, no backflip - the view turns no faster than the story's caps while the mouse is still
	int64 StillFrames = 0, StillBad = 0, WindowBad = 0;
	FYggCamEpisodes V1;
	double WinT0 = -1.0; float WinYaw0 = 0.f, WinPitch0 = 0.f; int32 WinScr = -9;
	int64 PitchFrames = 0, PitchOut = 0; float PitchMin = 0.f, PitchMax = 0.f; double PitchOutT = -1.0;
	// V2: never stuck low
	int64 Moving = 0, Moving120 = 0, Moving135 = 0, MovingPosed = 0; float MovingEyeMin = 1e6f; double MovingEyeMinT = 0.0;
	FYggCamEpisodes V2Low, V2Posed;
	int32 StandWatchdog = 0, Repairs = 0; TArray<FString> RepairLines;
	// V4: leases release
	int32 Expired = 0; TArray<FString> ExpiredLines;
	// V5: cuts only in black
	int32 CutLines = 0, CutDark = 0, CutFrames = 0, Jumps = 0, AutoBlinks = 0, Teleports = 0;
	TArray<FString> CutList, JumpList, BlinkLines;
	// V6: nudge discipline
	int32 Nudges = 0, LastNudging = 0; TArray<double> NudgeT; TArray<FString> NudgeLines;
	// V7: a still view at rest
	double RestT0 = -1.0; float RestZMin = 0.f, RestZMax = 0.f; TArray<float> RestRanges;
	// V14: the fly-out
	bool bInFly = false; double FlyT0 = 0.0; float FlyRoll = 0.f, FlyTurn = 0.f, FlyDt = 0.f;
	float FlyRollAll = 0.f, FlyTurnAll = 0.f, FlyDtAll = 0.f; int32 FlyShots = 0, FlyPawnHidden = 0; double FlyPawnCheckT = -1.0;
	TArray<FString> FlyList;
	// V16: no stall with the eyes open
	int32 OpenStalls = 0; TArray<FString> StallList;
	// invariants (§4.6)
	int64 InvFrames = 0, RollBad = 0, FovBad = 0, EyeLow = 0; float RollMax = 0.f, FovLo = 0.f, FovHi = 0.f, EyeMin = 1e6f;
	double RollBadT = -1.0, FovBadT = -1.0, EyeLowT = -1.0;

	static void Keep(TArray<FString>& List, const FString& S) { if (List.Num() < MaxList) { List.Add(S.Left(240)); } }
};

/** Every key and mouse move, game and menus alike, before Slate routes them. */
class FYggMonitorInput : public IInputProcessor
{
public:
	explicit FYggMonitorInput(UYggSessionMonitor* InOwner) : Owner(InOwner) {}
	virtual void Tick(const float DeltaTime, FSlateApplication& SlateApp, TSharedRef<ICursor> Cursor) override {}
	virtual bool HandleKeyDownEvent(FSlateApplication& SlateApp, const FKeyEvent& E) override { if (!E.IsRepeat() && Owner.IsValid()) { Owner->OnKey(E.GetKey().ToString(), true); } return false; }
	virtual bool HandleKeyUpEvent(FSlateApplication& SlateApp, const FKeyEvent& E) override { if (Owner.IsValid()) { Owner->OnKey(E.GetKey().ToString(), false); } return false; }
	virtual bool HandleMouseButtonDownEvent(FSlateApplication& SlateApp, const FPointerEvent& E) override { if (Owner.IsValid()) { Owner->OnKey(E.GetEffectingButton().ToString(), true); } return false; }
	virtual bool HandleMouseButtonUpEvent(FSlateApplication& SlateApp, const FPointerEvent& E) override { if (Owner.IsValid()) { Owner->OnKey(E.GetEffectingButton().ToString(), false); } return false; }
	virtual bool HandleMouseMoveEvent(FSlateApplication& SlateApp, const FPointerEvent& E) override { if (Owner.IsValid()) { Owner->OnMouse(E.GetCursorDelta().X, E.GetCursorDelta().Y); } return false; }
	virtual bool HandleMouseWheelOrGestureEvent(FSlateApplication& SlateApp, const FPointerEvent& E, const FPointerEvent* G) override { if (Owner.IsValid()) { Owner->OnKey(E.GetWheelDelta() > 0.f ? TEXT("WheelUp") : TEXT("WheelDown"), true); } return false; }
	virtual const TCHAR* GetDebugName() const override { return TEXT("YggMonitorInput"); }
private:
	TWeakObjectPtr<UYggSessionMonitor> Owner;
};

/** The engine's log, filtered to what helps explain a session (and to what a player would never see). After start-up
 *  the engine hands lines over on its own logging thread (and any thread may log), so lines wait in a thread-safe queue
 *  that the game thread empties every frame (the first version only listened on the game thread and heard nothing). */
class FYggMonitorLog : public FOutputDevice
{
public:
	FYggMonitorLog() { bAutoEmitLineTerminator = false; }
	virtual bool CanBeUsedOnAnyThread() const override { return true; }
	virtual bool CanBeUsedOnMultipleThreads() const override { return true; }
	TQueue<TPair<double, FString>, EQueueMode::Mpsc> Lines;
	virtual void Serialize(const TCHAR* V, ELogVerbosity::Type Verbosity, const FName& Category) override
	{
		static const FName Keep[] = { TEXT("LogRenderer"), TEXT("LogStreaming"), TEXT("LogStaticMesh"), TEXT("LogShaders"), TEXT("LogRHI"),
			TEXT("LogD3D12RHI"), TEXT("LogAudio"), TEXT("LogNiagara"), TEXT("LogTemp"), TEXT("LogSurvival"), TEXT("LogTexture"), TEXT("LogContentStreaming"),
			TEXT("LogPSO"), TEXT("LogShaderCompilers"), TEXT("LogMaterial"), TEXT("LogLumen"), TEXT("LogNanite"), TEXT("LogViewport"), TEXT("LogLoad"), TEXT("LogWorld"),
			TEXT("LogGarbage"), TEXT("LogConsoleResponse"), TEXT("LogEngine"), TEXT("LogStats"), TEXT("LogStreamline"), TEXT("LogDLSS"),
		// the game's own (the view rig and the story may log under a category of their own: the camera checks read them)
		TEXT("LogYggdrasilsPeak"), TEXT("LogYggView"), TEXT("LogYggRig"), TEXT("LogYggStory") };
		bool bKeep = Verbosity <= ELogVerbosity::Warning;
		for (const FName& K : Keep) { bKeep |= Category == K; }
		if (bKeep) { Lines.Enqueue(TPair<double, FString>(FPlatformTime::Seconds(), FString::Printf(TEXT("%s: %s"), *Category.ToString(), V))); }
	}
};

/** One moment the game thread was stuck: when its last frame started, how long it had been stuck so far, and its call
 *  stack as raw addresses (the watchdog never reads symbols - that reads files - the worker names them later). */
struct FYggMonitorStall
{
	static constexpr uint32 MaxDepth = 48;
	double Beat = 0.0;
	float StuckFor = 0.f;
	uint32 Depth = 0;
	uint64 Pcs[MaxDepth] = {};
};

/** The monitor's file thread (2026-11-14, after the second monitored session froze for 2-5 s at a time with the game's
 *  CPU and the GPU idle while the separate system monitor stalled at the same moments - a storage stall):
 *   - writes every file, so the game thread never waits on the disk (the monitor itself used to append its CSVs on the
 *     game thread every 2 s - a disk stall became a game freeze);
 *   - names the watchdog's stacks into stalls.txt (and a "stall" event) once the game thread runs again: symbols are
 *     read from files and under the engine's stack-walk lock, so not while the stall (maybe a disk stall) is on;
 *   - disk probe: once a second a 4 KB write flushed to the drive, on the project drive and on C:, timed (seconds.csv;
 *     the slow ones also in stalls.txt and a "disk" event). */
class FYggMonitorWorker : public FRunnable
{
public:
	FYggMonitorWorker(const FString& InDir, double InStart)
		: Dir(InDir), TempProbe(FPaths::Combine(FPlatformProcess::UserTempDir(), TEXT("ygg_disk_probe.bin"))), StartReal(InStart) {}

	TQueue<TPair<FString, FString>, EQueueMode::Mpsc> Writes;   // game thread -> here: file ("!name" = start it over), text
	TQueue<TPair<FString, FString>, EQueueMode::Spsc> Notes;    // here -> game thread: event type, detail
	TQueue<FYggMonitorStall, EQueueMode::Spsc> Stalls;          // watchdog -> here
	std::atomic<double> Beat{0.0};                              // when the game thread last started a frame (0 = not yet)
	std::atomic<bool> bStop{false};
	std::atomic<float> ProbeProjectMs{0.f}, ProbeTempMs{0.f};

	virtual uint32 Run() override
	{
		double LastProbe = 0.0;
		while (!bStop.load())
		{
			WriteQueued();
			const double B = Beat.load();
			if (B > 0.0 && FPlatformTime::Seconds() - B < 0.4) { NameStalls(); }
			if (FPlatformTime::Seconds() - LastProbe >= 1.0) { LastProbe = FPlatformTime::Seconds(); Probe(); }
			FPlatformProcess::Sleep(0.05f);
		}
		WriteQueued();
		NameStalls();
		IFileManager::Get().Delete(*(Dir / TEXT("probe.bin")), false, false, true);
		IFileManager::Get().Delete(*TempProbe, false, false, true);
		return 0;
	}
	virtual void Stop() override { bStop.store(true); }

	/** Writes everything queued, in order. On this thread - on the game thread only if the thread could not be made. */
	void WriteQueued()
	{
		TPair<FString, FString> W;
		while (Writes.Dequeue(W))
		{
			const bool bReplace = W.Key.StartsWith(TEXT("!"));
			Append(bReplace ? W.Key.Mid(1) : W.Key, W.Value, bReplace);
		}
	}

private:
	void NameStalls()
	{
		FYggMonitorStall S;
		while (Stalls.Dequeue(S))
		{
			const double T0 = FPlatformTime::Seconds();
			FString Text = FString::Printf(TEXT("--- t=%.3f  game thread stuck for %.2f s so far%s"), S.Beat - StartReal, S.StuckFor, LINE_TERMINATOR);
			FString Top;
			for (uint32 i = 0; i < S.Depth; ++i)
			{
				FProgramCounterSymbolInfo Info;
				FPlatformStackWalk::ProgramCounterToSymbolInfo(S.Pcs[i], Info);
				const FString Module = FPaths::GetCleanFilename(FString(ANSI_TO_TCHAR(Info.ModuleName)));
				const FString Fn = Info.FunctionName[0] ? FString(ANSI_TO_TCHAR(Info.FunctionName)) : FString(TEXT("?"));
				// module + offset stays exact without symbols (an engine without its PDBs only gives the nearest exported name)
				Text += Info.OffsetInModule ? FString::Printf(TEXT("  %2u  %s+0x%llx  %s"), i, *Module, Info.OffsetInModule, *Fn)
					: FString::Printf(TEXT("  %2u  0x%016llx %s  %s"), i, S.Pcs[i], *Module, *Fn);
				if (Info.Filename[0]) { Text += FString::Printf(TEXT("  [%s:%d]"), *FPaths::GetCleanFilename(FString(ANSI_TO_TCHAR(Info.Filename))), Info.LineNumber); }
				Text += LINE_TERMINATOR;
				// the first frames outside the system's own waiting say what the game thread waits on
				const bool bSystem = Module.StartsWith(TEXT("ntdll")) || Module.StartsWith(TEXT("KERNELBASE")) || Module.StartsWith(TEXT("kernel32"));
				if (!bSystem && Top.Len() < 300) { Top += (Top.IsEmpty() ? TEXT("") : TEXT(" < ")) + Fn; }
			}
			if (S.Depth > 0) { Text += FString::Printf(TEXT("  (named in %.0f ms)%s"), (FPlatformTime::Seconds() - T0) * 1000.0, LINE_TERMINATOR); }
			Append(TEXT("stalls.txt"), Text, false);
			Notes.Enqueue(TPair<FString, FString>(TEXT("stall"), FString::Printf(TEXT("t=%.3f stuck %.2f s: %s"), S.Beat - StartReal, S.StuckFor, *Top)));
		}
	}

	void Probe()
	{
		static uint8 Bytes[4096] = {};
		for (int32 k = 0; k < 2; ++k)
		{
			const FString Path = k == 0 ? Dir / TEXT("probe.bin") : TempProbe;
			const double T0 = FPlatformTime::Seconds();
			if (IFileHandle* H = FPlatformFileManager::Get().GetPlatformFile().OpenWrite(*Path))
			{
				H->Write(Bytes, sizeof(Bytes));
				H->Flush(true);
				delete H;
			}
			const float Ms = float((FPlatformTime::Seconds() - T0) * 1000.0);
			(k == 0 ? ProbeProjectMs : ProbeTempMs).store(Ms);
			if (Ms > 150.f)
			{
				const FString D = FString::Printf(TEXT("t=%.3f %s write took %.0f ms"), T0 - StartReal, k == 0 ? TEXT("project drive") : TEXT("C: temp"), Ms);
				Notes.Enqueue(TPair<FString, FString>(TEXT("disk"), D));
				Append(TEXT("stalls.txt"), FString::Printf(TEXT("--- %s%s"), *D, LINE_TERMINATOR), false);
			}
		}
	}

	void Append(const FString& Name, const FString& Text, bool bReplace)
	{
		const double T0 = FPlatformTime::Seconds();
		FFileHelper::SaveStringToFile(Text, *(Dir / Name), FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM, &IFileManager::Get(), bReplace ? 0 : FILEWRITE_Append);
		const double Ms = (FPlatformTime::Seconds() - T0) * 1000.0;
		if (Ms > 200.0 && Name != TEXT("stalls.txt"))
		{
			Notes.Enqueue(TPair<FString, FString>(TEXT("disk"), FString::Printf(TEXT("t=%.3f writing %s took %.0f ms"), T0 - StartReal, *Name, Ms)));
		}
	}

	FString Dir, TempProbe;
	double StartReal;
};

/** The watchdog: its own thread, which never touches the disk (a stalled drive must not silence it, as it did when this
 *  ran on the file thread). When the game thread has not started a frame for 0.4 s it takes the game thread's call stack
 *  (the thread is suspended for a moment and its stack walked from memory; addresses only) and hands it to the worker,
 *  again every 1.5 s while the stall lasts: at most 3 a stall, stacks for the first 60 of a session, the times of all.
 *  The engine's own hitch detector is compiled out of editor builds (USE_HITCH_DETECTION needs !WITH_EDITORONLY_DATA). */
class FYggMonitorWatchdog : public FRunnable
{
public:
	FYggMonitorWatchdog(FYggMonitorWorker& InWorker, bool bInStacks) : Worker(InWorker), bStacks(bInStacks) {}
	std::atomic<bool> bStop{false};

	virtual uint32 Run() override
	{
		double StuckBeat = -1.0, NextTake = 0.0;
		int32 Taken = 0, WithStack = 0;
		while (!bStop.load())
		{
			const double Now = FPlatformTime::Seconds();
			const double B = Worker.Beat.load();
			// closing the game is one long frame of its own, not a stall
			if (B > 0.0 && Now - B > 0.4 && !IsEngineExitRequested())
			{
				if (StuckBeat != B) { StuckBeat = B; Taken = 0; NextTake = Now; }
				if (Taken < 3 && Now >= NextTake)
				{
					FYggMonitorStall S;
					S.Beat = B;
					S.StuckFor = float(Now - B);
					if (bStacks && WithStack < 60)
					{
						S.Depth = FPlatformStackWalk::CaptureThreadStackBackTrace(GGameThreadId, S.Pcs, FYggMonitorStall::MaxDepth);
						// the capture waits for the engine's stack-walk lock, which the worker holds while it names an earlier
						// stall (seconds the first time): a stack taken after the game thread moved on is not this stall's
						if (Worker.Beat.load() != B) { S.Depth = 0; }
						++WithStack;
					}
					Worker.Stalls.Enqueue(S);
					++Taken;
					NextTake = Now + 1.5;
				}
			}
			FPlatformProcess::Sleep(0.05f);
		}
		return 0;
	}
	virtual void Stop() override { bStop.store(true); }

private:
	FYggMonitorWorker& Worker;
	bool bStacks;
};

void UYggSessionMonitor::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	const bool bTestRun = FParse::Param(FCommandLine::Get(), TEXT("YggNoSave"));
	const bool bForce = FParse::Param(FCommandLine::Get(), TEXT("YggMonitor"));
	const bool bOff = FParse::Param(FCommandLine::Get(), TEXT("NoYggMonitor"));
	if (GIsEditor || bOff || (bTestRun && !bForce) || (CVarMonitor.GetValueOnGameThread() == 0 && !bForce))
	{
		return;
	}
	Start();
}

void UYggSessionMonitor::Start()
{
	Dir = FPaths::ProjectSavedDir() / TEXT("Monitor") / FDateTime::Now().ToString(TEXT("%Y%m%d_%H%M%S"));
	StartReal = FPlatformTime::Seconds();
	StartEpoch = (FDateTime::UtcNow() - FDateTime(1970, 1, 1)).GetTotalSeconds();      // lines the system monitor up with the frames
	LastFlush = LastSecond = StartReal;
	bOn = true;
	Recent.Init(16.7f, 240);
	Cam = MakeShared<FYggCamChecks>();
	LastCamReport = StartReal;
	// every file is written by the worker (folders are made by the first write into them); the headers go first.
	// The view columns (camera spec §10.2) come last, so older readers of the file keep their columns.
	Worker = MakeShared<FYggMonitorWorker>(Dir, StartReal);
	Worker->Writes.Enqueue(TPair<FString, FString>(TEXT("!frames.csv"), TEXT("t,dt_ms,game_ms,render_ms,rhi_ms,gpu_ms,fps_presented,draws,prims_k,x_m,y_m,z_m,yaw,pitch,turn_dps,mouse_dx,mouse_dy,speed_ms,loading,streaming,paused,cursor,hitch,")
		TEXT("roll,posture,eye_floor_cm,lid,story_fov,nudge,leases,cut,scripted\n")));
	Worker->Writes.Enqueue(TPair<FString, FString>(TEXT("!seconds.csv"), TEXT("t,fps,fps_min,fps_low1,max_ms,fps_presented,mem_mb,map,alt_m,state,spo2,hr_bpm,core_c,stamina,satiety,hydration,health,sanity,exhausted,sheltered,on_rope,o2_bottle_s,bottles,storm,wind,realm,uds_time,air_c,keys,paused,cursor,focus,x_m,y_m,disk_project_ms,disk_c_ms\n")));
	Worker->Writes.Enqueue(TPair<FString, FString>(TEXT("!events.csv"), TEXT("t,type,detail\n")));
	WorkerThread = FRunnableThread::Create(Worker.Get(), TEXT("YggMonitorWorker"), 0, TPri_BelowNormal);
	if (WorkerThread)
	{
		// normal priority: it sleeps nearly always, and must wake on time while every core is busy
		Watchdog = MakeShared<FYggMonitorWatchdog>(*Worker, CVarMonitorStallStacks.GetValueOnGameThread() != 0);
		WatchdogThread = FRunnableThread::Create(Watchdog.Get(), TEXT("YggMonitorWatchdog"), 0, TPri_Normal);
	}
	if (FSlateApplication::IsInitialized())
	{
		Input = MakeShared<FYggMonitorInput>(this);
		FSlateApplication::Get().RegisterInputPreProcessor(Input);
	}
	ImageWrappers = &FModuleManager::LoadModuleChecked<IImageWrapperModule>(TEXT("ImageWrapper"));
	LogDevice = MakeShared<FYggMonitorLog>();
	GLog->AddOutputDevice(LogDevice.Get());
	// session.txt is written once the first level is up (viewport, player and settings exist), again on each new level
	Event(TEXT("session_start"), Dir);
	UE_LOG(LogTemp, Display, TEXT("Session monitor: recording to %s"), *Dir);
}

void UYggSessionMonitor::Deinitialize()
{
	if (bOn)
	{
		// the watchdog first: from here on the game thread is busy closing the game, which is not a stall of the game
		if (WatchdogThread)
		{
			Watchdog->Stop();
			WatchdogThread->WaitForCompletion();
			delete WatchdogThread;
			WatchdogThread = nullptr;
		}
		Watchdog.Reset();
		Event(TEXT("session_end"), FString::Printf(TEXT("%d frames, %d hitches, %d screenshots"), Frames, Hitches, Shots));
		if (LogDevice.IsValid()) { GLog->RemoveOutputDevice(LogDevice.Get()); }   // the device itself lives on with this object
		DrainWorker();                                                         // a stall named a moment ago
		DrainLog();
		LogCounts.ValueSort([](int32 A, int32 B) { return A > B; });
		for (const TPair<FString, int32>& KV : LogCounts)
		{
			if (KV.Value > 20) { LogBuf += FString::Printf(TEXT("total %d x  %s%s"), KV.Value, *KV.Key, LINE_TERMINATOR); }
		}
		if (Worker.IsValid() && Cam.IsValid()) { Worker->Writes.Enqueue(TPair<FString, FString>(TEXT("!camera_checks.md"), CameraReport())); }
		Flush();
		// the worker writes what is still queued, then ends: the one time the game thread waits for the monitor's disk
		if (WorkerThread)
		{
			Worker->Stop();
			WorkerThread->WaitForCompletion();
			delete WorkerThread;
			WorkerThread = nullptr;
		}
		Worker.Reset();
		if (Input.IsValid() && FSlateApplication::IsInitialized()) { FSlateApplication::Get().UnregisterInputPreProcessor(Input); }
		if (ShotHandle.IsValid()) { UGameViewportClient::OnScreenshotCaptured().Remove(ShotHandle); ShotHandle.Reset(); }
		bOn = false;
	}
	Super::Deinitialize();
}

TStatId UYggSessionMonitor::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UYggSessionMonitor, STATGROUP_Tickables);
}

void UYggSessionMonitor::WriteSessionInfo()
{
	FString S;
	S += FString::Printf(TEXT("started   %s\n"), *FDateTime::Now().ToString());
	S += FString::Printf(TEXT("level     %s\n"), *LastMap);
	S += FString::Printf(TEXT("epoch     %.3f\n"), StartEpoch);
	S += FString::Printf(TEXT("cpu       %s (%d cores)\n"), *FPlatformMisc::GetCPUBrand(), FPlatformMisc::NumberOfCoresIncludingHyperthreads());
	S += FString::Printf(TEXT("gpu       %s\n"), *FPlatformMisc::GetPrimaryGPUBrand());
	S += FString::Printf(TEXT("ram       %.1f GB\n"), FPlatformMemory::GetConstants().TotalPhysical / 1073741824.0);
	S += FString::Printf(TEXT("build     %s\n"), LexToString(FApp::GetBuildConfiguration()));
	S += FString::Printf(TEXT("command   %s\n"), FCommandLine::Get());
	if (GEngine && GEngine->GameViewport)
	{
		FVector2D V; GEngine->GameViewport->GetViewportSize(V);
		S += FString::Printf(TEXT("viewport  %.0f x %.0f\n"), V.X, V.Y);
	}
	for (const TCHAR* Name : { TEXT("r.VSync"), TEXT("t.MaxFPS"), TEXT("r.ScreenPercentage"), TEXT("r.NGX.DLSS.Enable"), TEXT("r.Streamline.DLSSG.Enable"),
		TEXT("r.DynamicGlobalIlluminationMethod"), TEXT("r.ReflectionMethod"), TEXT("r.Shadow.Virtual.Enable"), TEXT("r.VolumetricRenderTarget.Mode"),
		TEXT("r.PSOPrecaching"), TEXT("r.PSOPrecache.ProxyCreationStrategy"), TEXT("r.Streaming.PoolSize"), TEXT("r.Nanite.Streaming.StreamingPoolSize"),
		TEXT("sg.ResolutionQuality"), TEXT("sg.ViewDistanceQuality"), TEXT("sg.ShadowQuality"), TEXT("sg.GlobalIlluminationQuality"), TEXT("sg.ReflectionQuality"),
		TEXT("sg.PostProcessQuality"), TEXT("sg.TextureQuality"), TEXT("sg.EffectsQuality"), TEXT("sg.FoliageQuality"),
		// the view rig's A/B switches (camera spec §10.1): which camera this session played with
		TEXT("ygg.View.Rig"), TEXT("ygg.View.HeadMotion"), TEXT("ygg.View.OneBody"), TEXT("ygg.View.SlipSlowMo") })
	{
		if (const IConsoleVariable* V = IConsoleManager::Get().FindConsoleVariable(Name)) { S += FString::Printf(TEXT("cvar      %s = %s\n"), Name, *V->GetString()); }
	}
	if (const UGameInstance* GI = GetGameInstance())
	{
		for (const TPair<FString, FString>& KV : YggSettings::EngineState(GI->GetWorld())) { S += FString::Printf(TEXT("setting   %s = %s\n"), *KV.Key, *KV.Value); }
	}
	if (Worker.IsValid()) { Worker->Writes.Enqueue(TPair<FString, FString>(TEXT("!session.txt"), S)); }
}

void UYggSessionMonitor::Event(const TCHAR* Type, const FString& Detail)
{
	if (!bOn) { return; }
	EventsBuf += FString::Printf(TEXT("%.3f,%s,%s\n"), FPlatformTime::Seconds() - StartReal, Type, *MonitorCsv(Detail));
}

void UYggSessionMonitor::OnKey(const FString& Key, bool bDown)
{
	if (bDown) { if (!KeysDown.Contains(Key)) { KeysDown.Add(Key); ++SecKeys; Event(TEXT("key_down"), Key); } }
	else if (KeysDown.Remove(Key) > 0) { Event(TEXT("key_up"), Key); }
}

void UYggSessionMonitor::DrainLog()
{
	TPair<double, FString> L;
	while (LogDevice.IsValid() && LogDevice->Lines.Dequeue(L)) { OnLogLine(L.Value, L.Key); }
}

void UYggSessionMonitor::OnLogLine(const FString& Line, double When)
{
	if (!bOn) { return; }
	const double T = FMath::Max(0.0, When - StartReal);
	FString Clean = Line; Clean.TrimEndInline();
	if (Clean.StartsWith(TEXT("LogSurvival: Expedition: "))) { Event(TEXT("message"), Clean.Mid(25)); }
	// a line seen more than 20 times is only counted from then on (log.txt ends with "total N x line"); the hitch context
	// below still gets every line with its time. (The first version also folded runs of one line into "repeated N more
	// times", which credited the runs of a suppressed line to the last line written: wrong counts in the first report.)
	int32& Seen = LogCounts.FindOrAdd(Clean);
	if (++Seen <= 20) { LogBuf += FString::Printf(TEXT("%.3f  %s\n"), T, *Clean); }
	CameraLogLine(Clean, T);
	RecentLog.Emplace(T, Clean);
	if (RecentLog.Num() > 64) { RecentLog.RemoveAt(0, RecentLog.Num() - 64); }
}

void UYggSessionMonitor::Tick(float DeltaTime)
{
	if (!bOn) { return; }
	const float Dt = float(FApp::GetDeltaTime());
	if (Worker.IsValid()) { Worker->Beat.store(FPlatformTime::Seconds()); }    // the watchdog's heartbeat
	++Frames;
	DrainWorker();
	DrainLog();                                                            // before the frame row: a hitch sees its log lines
	FrameRow(Dt);
	const double Now = FPlatformTime::Seconds();
	if (Now - LastSecond >= 1.0) { SecondRow(); PollChanges(); LastSecond = Now; }
	const float Every = CVarMonitorShotEvery.GetValueOnGameThread();
	if (Every > 0.f && Now - StartReal - LastShot >= Every) { Shot(TEXT("timer")); }
	if (Now - LastFlush >= 2.0) { Flush(); LastFlush = Now; }
	// the camera checks, rewritten now and then so a crash still leaves them (a few KB, written by the worker)
	if (Now - LastCamReport >= 30.0 && Worker.IsValid() && Cam.IsValid())
	{
		LastCamReport = Now;
		Worker->Writes.Enqueue(TPair<FString, FString>(TEXT("!camera_checks.md"), CameraReport()));
	}
}

void UYggSessionMonitor::FrameRow(float Dt)
{
	const float DtMs = Dt * 1000.f;
	const float GameMs = MonitorMs(GGameThreadTime), RenderMs = MonitorMs(GRenderThreadTime), RhiMs = MonitorMs(GRHIThreadTime), GpuMs = MonitorMs(RHIGetGPUFrameCycles(0));
	// the presented rate with DLSS frame generation, from the Streamline plugin when it is there (twice a second)
	const double Now = FPlatformTime::Seconds();
	if (Now - LastFg > 0.5)
	{
		LastFg = Now;
		PresentedFps = 0.f;
		static UClass* Lib = FindObject<UClass>(nullptr, TEXT("/Script/StreamlineDLSSGBlueprint.StreamlineLibraryDLSSG"));
		static UFunction* Timing = Lib ? Lib->FindFunctionByName(TEXT("GetDLSSGFrameTiming")) : nullptr;
		if (Timing)
		{
			struct { float FrameRateInHertz = 0.f; int32 FramesPresented = 0; } Parms;
			Lib->GetDefaultObject()->ProcessEvent(Timing, &Parms);
			if (Parms.FramesPresented > 1) { PresentedFps = Parms.FrameRateInHertz; }
		}
	}
	// where the camera is and how fast it turns
	FVector Loc = FVector::ZeroVector; FRotator Rot = FRotator::ZeroRotator; float Speed = 0.f; bool bPaused = false, bCursor = false;
	// the view (camera spec §10.2): the monitor ticks after the world (camera updated, the cut flag still set) and before
	// the viewport draws (which clears the flag)
	APawn* Pawn = nullptr; bool bPawnView = false, bPawnHidden = false, bCutFlag = false; float Fov = 0.f;
	const UGameInstance* GI = GetGameInstance();
	UWorld* World = GI ? GI->GetWorld() : nullptr;
	if (APlayerController* PC = GI ? GI->GetFirstLocalPlayerController() : nullptr)
	{
		if (APlayerCameraManager* PCM = PC->PlayerCameraManager)
		{
			Loc = PCM->GetCameraLocation(); Rot = PCM->GetCameraRotation();
			Fov = PCM->GetFOVAngle();
			bCutFlag = PCM->bGameCameraCutThisFrame != 0;
		}
		Pawn = PC->GetPawn();
		if (Pawn)
		{
			Speed = float(Pawn->GetVelocity().Size()) / 100.f;
			bPawnHidden = Pawn->IsHidden();
			bPawnView = PC->PlayerCameraManager && PC->PlayerCameraManager->GetViewTarget() == Pawn;
		}
		bCursor = PC->bShowMouseCursor;
		bPaused = World && World->IsPaused();
	}
	float Turn = 0.f;
	FRotator D = FRotator::ZeroRotator;
	if (bHaveRot && Dt > 0.f)
	{
		D = (Rot - LastRot).GetNormalized();
		Turn = FMath::Sqrt(D.Yaw * D.Yaw + D.Pitch * D.Pitch) / Dt;
	}
	const bool bHadPlace = bHaveRot;
	LastRot = Rot; bHaveRot = true;

	// the view rig's state; without a rig (or with ygg.View.Rig 0, the old socket camera) the eye is measured from the camera
	const bool bSamePawn = Pawn == RigPawn.Get();               // a new pawn (spawn, level change): last frame's view is not comparable
	UYggViewRigComponent* R = FindRig(Pawn);
	static IConsoleVariable* const RigCVar = IConsoleManager::Get().FindConsoleVariable(TEXT("ygg.View.Rig"));
	const bool bRigCamera = R && (!RigCVar || RigCVar->GetInt() != 0);
	int32 Posture = -1, Nudging = -1, Scripted = -1; float Eye = -1.f, Lid = 0.f; int64 Mask = -1;
	if (R)
	{
		Posture = MonitorPostureCode(R->GetView());
		Lid = R->GetLid();
		Nudging = MonitorRigNudging(R);
		Mask = MonitorRigLeaseMask(R);
		Scripted = MonitorRigScripted(R);
		if (bRigCamera) { Eye = R->GetEyeAboveFloor(); }
		if (Cam.IsValid())
		{
			++Cam->RigFrames;
			Cam->bScriptedKnown |= Scripted >= 0;
			Cam->bLeasesKnown |= Mask >= 0;
			Cam->bNudgeKnown |= Nudging >= 0;
		}
	}
	if (Eye < 0.f && bPawnView) { Eye = MonitorEyeAboveFloor(Pawn, Loc); }
	// without the rig's word, a view target other than the pawn is the fly-out (the only one in chapter 1)
	if (Scripted < 0 && Pawn && !bPawnView) { Scripted = 3; }
	// the lens over the settings FOV: story FOV + attention + FOV beat + heart FOV (the active pawn camera holds the setting)
	float StoryFov = 0.f;
	if (Fov > 0.f)
	{
		float Base = 0.f;
		if (bPawnView) { for (const TWeakObjectPtr<UCameraComponent>& C : PawnCams) { if (C.IsValid() && C->IsActive()) { Base = C->FieldOfView; break; } } }
		if (Base <= 0.f) { Base = YggSettings::FieldOfView() > 1.f ? FMath::Clamp(YggSettings::FieldOfView(), YggSettings::FovMin, YggSettings::FovMax) : 90.f; }
		StoryFov = Fov - Base;
	}
	// a cut: 1 = flagged by the camera manager, or told by the story's log line this frame; 2 = the camera jumped over 5 m in
	// one frame with no flag (a hard cut the spec forbids; not on the fly camera, which travels fast); 3 = a new pawn or the
	// first frame (not a cut, but nothing to compare with)
	int32 Cut = (bCutFlag || bCutLog) ? 1 : 0;
	if (Cut == 0 && (!bSamePawn || !bHadPlace)) { Cut = 3; }
	else if (Cut == 0 && !bLastCut && bPawnView && FVector::DistSquared(Loc, LastLoc) > FMath::Square(500.0)) { Cut = 2; }
	const bool bMouseNow = MouseDx == 0.f && MouseDy == 0.f;
	const bool bMouseStill = bMouseNow && bLastMouseStill;      // this frame and the last: input lands in the frame it arrives
	if (Mask != LastLeaseMask) { LastLeaseMask = Mask; LastLeaseText = Mask < 0 ? FString(TEXT("-")) : MonitorLeaseNames(Mask); }
	const double T = Now - StartReal;
	CameraFrame(T, Dt, bPaused, bCursor, bPawnView, Loc, Rot, D, Speed, Eye, Lid, StoryFov, Nudging, Cut, Scripted, Posture, bMouseStill, bPawnHidden);
	LastLid = Lid;                                              // the hitch event below wants it too (V16)
	bLastMouseStill = bMouseNow;
	bLastCut = Cut != 0;
	bCutLog = false;
	LastLoc = Loc;
	const bool bLoading = IsAsyncLoading();
	const int32 Streaming = IStreamingManager::Get().GetNumWantingResources();

	// hitch: see below. Column value 2 = slow because of the monitor's own screenshot
	// (the frame is read back), so the analyser can leave it out; level loads are told apart by the analyser.
	TArray<float> Sorted = Recent; Sorted.Sort();
	const float Median = Sorted[Sorted.Num() / 2];
	// twice the recent median, at least 25 ms: at 60 fps a 33 ms frame, at 120 fps 25 ms; with frame generation pacing
	// the rendered frames at 30 fps, 67 ms (the first rule, "over 33 ms", called every one of those frames a hitch)
	const bool bSlow = Frames > 5 && DtMs > FMath::Max(Median * 2.f, 25.f);
	const bool bOwnShot = Now < ShotQuietUntil;
	const bool bHitch = bSlow && !bOwnShot;
	Recent[RecentHead] = DtMs; RecentHead = (RecentHead + 1) % Recent.Num();

	FramesBuf += FString::Printf(TEXT("%.4f,%.2f,%.2f,%.2f,%.2f,%.2f,%.1f,%d,%.0f,%.1f,%.1f,%.1f,%.1f,%.1f,%.0f,%.0f,%.0f,%.2f,%d,%d,%d,%d,%d,%.2f,%s,%.1f,%.2f,%.2f,%s,%s,%d,%s\n"),
		Now - StartReal, DtMs, GameMs, RenderMs, RhiMs, GpuMs, PresentedFps, GNumDrawCallsRHI[0], GNumPrimitivesDrawnRHI[0] / 1000.f,
		Loc.X / 100.f, Loc.Y / 100.f, Loc.Z / 100.f, Rot.Yaw, Rot.Pitch, Turn, MouseDx, MouseDy, Speed, bLoading ? 1 : 0, Streaming, bPaused ? 1 : 0, bCursor ? 1 : 0, bHitch ? 1 : (bSlow ? 2 : 0),
		FRotator::NormalizeAxis(Rot.Roll), MonitorPostureNames[Posture + 1], Eye, Lid, StoryFov, Nudging < 0 ? TEXT("-") : (Nudging ? TEXT("1") : TEXT("0")),
		*LastLeaseText, Cut, MonitorScriptedNames[Scripted + 1]);
	MouseDx = MouseDy = 0.f;
	++SecFrames; SecSumMs += DtMs; SecMaxMs = FMath::Max(SecMaxMs, DtMs); SecFrameMs.Add(DtMs);
	if (bHitch) { Hitch(DtMs, GameMs, RenderMs, GpuMs); }
}

void UYggSessionMonitor::Hitch(float DtMs, float GameMs, float RenderMs, float GpuMs)
{
	++Hitches;
	const double T = FPlatformTime::Seconds() - StartReal;
	// which side was slow, and what the log said in the last half second
	const TCHAR* Bound = GpuMs >= FMath::Max(GameMs, RenderMs) ? TEXT("gpu") : (RenderMs >= GameMs ? TEXT("render thread") : TEXT("game thread"));
	FString Around;
	for (const TPair<double, FString>& L : RecentLog) { if (T - L.Key < 0.6) { Around += TEXT(" | ") + L.Value.Left(160); } }
	// the eyelids go last: analyze_session.py reads the part up to ")" with a fixed pattern
	Event(TEXT("hitch"), FString::Printf(TEXT("%.1f ms (game %.1f, render %.1f, gpu %.1f; %s)%s | lid %.2f"), DtMs, GameMs, RenderMs, GpuMs, Bound, *Around, LastLid));
	if (DtMs > 100.f && T - LastHitchShot > 10.0 && Shots < 60) { LastHitchShot = T; Shot(TEXT("hitch")); }
	// V16: a stall the player saw (eyes open, in play): the spec wants loads behind closed eyelids or black
	if (DtMs > 100.f && bLastPlay && LastLid < 0.99f && Cam.IsValid())
	{
		++Cam->OpenStalls;
		FYggCamChecks::Keep(Cam->StallList, FString::Printf(TEXT("%s  %.0f ms (%s), lid %.2f%s"), *MonitorClock(T), DtMs, Bound, LastLid,
			RecentLog.Num() ? *(TEXT("; last log: ") + RecentLog.Last().Value.Left(120)) : TEXT("")));
	}
}

void UYggSessionMonitor::Shot(const TCHAR* Why)
{
	const double T = FPlatformTime::Seconds() - StartReal;
	if (FString(Why) == TEXT("timer")) { LastShot = T; }
	if (ShotHandle.IsValid() && FPlatformTime::Seconds() > ShotQuietUntil + 5.0)            // a request that never came back
	{
		UGameViewportClient::OnScreenshotCaptured().Remove(ShotHandle);
		ShotHandle.Reset();
	}
	if (Shots >= 200 || ShotHandle.IsValid() || !ImageWrappers) { return; }
	// The engine's own screenshot saves a full PNG on the game thread (a 200 ms freeze, measured 2026-11-13). Instead the
	// pixels come to OnShotCaptured, which halves them and writes a JPEG on a worker thread: only the read-back remains.
	PendingShot = Dir / TEXT("shots") / FString::Printf(TEXT("%06.1f_%s.jpg"), T, Why);
	ShotHandle = UGameViewportClient::OnScreenshotCaptured().AddUObject(this, &UYggSessionMonitor::OnShotCaptured);
	FScreenshotRequest::RequestScreenshot(PendingShot, true, false);
	++Shots;
	ShotQuietUntil = FPlatformTime::Seconds() + 0.5;                       // the next frames' read-back is not a hitch of the game
	Event(TEXT("screenshot"), FPaths::GetCleanFilename(PendingShot));
}

void UYggSessionMonitor::OnShotCaptured(int32 W, int32 H, const TArray<FColor>& Px)
{
	UGameViewportClient::OnScreenshotCaptured().Remove(ShotHandle);
	ShotHandle.Reset();
	if (W <= 0 || H <= 0 || Px.Num() < W * H) { return; }
	const int32 Step = FMath::Max(1, FMath::DivideAndRoundUp(W, 1280));   // at most 1280 wide: enough to see the scene
	const int32 OW = W / Step, OH = H / Step;
	TArray<FColor> Small;
	Small.SetNumUninitialized(OW * OH);
	for (int32 Y = 0; Y < OH; ++Y)
	{
		const FColor* Row = Px.GetData() + int64(Y * Step) * W;
		for (int32 X = 0; X < OW; ++X) { Small[Y * OW + X] = Row[X * Step]; }
	}
	IImageWrapperModule* Mod = ImageWrappers;
	Async(EAsyncExecution::ThreadPool, [Mod, File = PendingShot, OW, OH, Small = MoveTemp(Small)]()
	{
		const TSharedPtr<IImageWrapper> Jpg = Mod->CreateImageWrapper(EImageFormat::JPEG);
		if (Jpg.IsValid() && Jpg->SetRaw(Small.GetData(), int64(Small.Num()) * sizeof(FColor), OW, OH, ERGBFormat::BGRA, 8))
		{
			FFileHelper::SaveArrayToFile(Jpg->GetCompressed(85), *File);
		}
	});
}

void UYggSessionMonitor::SecondRow()
{
	const double T = FPlatformTime::Seconds() - StartReal;
	SecFrameMs.Sort();
	const float Fps = SecSumMs > 0.f ? SecFrames * 1000.f / SecSumMs : 0.f;
	const float Low1 = SecFrameMs.Num() ? 1000.f / SecFrameMs[FMath::Clamp(int32(SecFrameMs.Num() * 0.99f), 0, SecFrameMs.Num() - 1)] : 0.f;
	const FPlatformMemoryStats Mem = FPlatformMemory::GetStats();
	const UGameInstance* GI = GetGameInstance();
	UWorld* World = GI ? GI->GetWorld() : nullptr;
	FString Map = World ? World->GetMapName() : FString(TEXT("-"));
	float Alt = 0.f, Spo2 = 0.f, Hr = 0.f, Core = 0.f, Stam = 0.f, Sat = 0.f, Hyd = 0.f, Health = 0.f, Sanity = 0.f, Air = 0.f, X = 0.f, Y = 0.f;
	float Bottle = 0.f, Storm = 0.f, Wind = 0.f;
	int32 Shelter = 0, Rope = 0, Exhausted = 0, Bottles = 0; FString State = TEXT("-"), Realm = TEXT("-");
	bool bPaused = false, bCursor = false;
	if (APlayerController* PC = GI ? GI->GetFirstLocalPlayerController() : nullptr)
	{
		bPaused = World && World->IsPaused(); bCursor = PC->bShowMouseCursor;
		if (const ACharacter* C = Cast<ACharacter>(PC->GetPawn()))
		{
			X = float(C->GetActorLocation().X / 100.0); Y = float(C->GetActorLocation().Y / 100.0);
			if (const USurvivalComponent* B = C->FindComponentByClass<USurvivalComponent>())
			{
				const FSurvivalSnapshot S = B->GetSnapshot();
				Alt = S.ElevationMeters; Spo2 = S.Oxygen; Core = S.BodyTemperature; Stam = S.Stamina; Sat = S.Satiety; Hyd = S.Hydration; Health = S.Health;
				Shelter = S.bSheltered; Rope = S.bOnRope; Exhausted = S.bExhausted; Bottle = S.OxygenBottleSeconds; Bottles = S.CarriedBottles;
				const UClimbingComponent* Cl = C->FindComponentByClass<UClimbingComponent>();
				State = S.bDead ? TEXT("dead") : (Cl && Cl->IsClimbing()) ? TEXT("climb") : (C->GetCharacterMovement() && C->GetCharacterMovement()->IsFalling()) ? TEXT("air")
					: S.bSprinting ? TEXT("sprint") : (C->GetVelocity().Size2D() > 30.0 ? TEXT("walk") : TEXT("rest"));
			}
			if (const UExpeditionComponent* E = C->FindComponentByClass<UExpeditionComponent>()) { Hr = E->GetHeartRate(); Storm = E->GetStormStrength(); Wind = E->GetWindIntensity(); }
			if (const USanityComponent* M = C->FindComponentByClass<USanityComponent>()) { Sanity = M->GetSanity(); }
		}
	}
	if (const ARealmSky* Sky = World ? ARealmSky::Get(World) : nullptr)
	{
		Realm = Sky->GetShownRealm() == ERealm::None ? FString(TEXT("none")) : FString(ARealmSky::GetWeatherInfo(Sky->GetShownIndex()).Name);
		Air = Sky->GetAirTemperature(Alt);
	}
	SecondsBuf += FString::Printf(TEXT("%.1f,%.1f,%.1f,%.1f,%.1f,%.1f,%.0f,%s,%.0f,%s,%.1f,%.0f,%.2f,%.0f,%.0f,%.0f,%.0f,%.0f,%d,%d,%d,%.0f,%d,%.2f,%.2f,%s,%.2f,%.0f,%d,%d,%d,%d,%.0f,%.0f,%.0f,%.0f\n"),
		T, Fps, SecMaxMs > 0.f ? 1000.f / SecMaxMs : 0.f, Low1, SecMaxMs, PresentedFps, Mem.UsedPhysical / 1048576.0, *Map, Alt, *State, Spo2, Hr, Core, Stam, Sat, Hyd, Health, Sanity,
		Exhausted, Shelter, Rope, Bottle, Bottles, Storm, Wind, *MonitorCsv(Realm), World ? YggTime::Get(World) : 0.f, Air, SecKeys, bPaused ? 1 : 0, bCursor ? 1 : 0, FApp::HasFocus() ? 1 : 0, X, Y,
		Worker.IsValid() ? Worker->ProbeProjectMs.load() : 0.f, Worker.IsValid() ? Worker->ProbeTempMs.load() : 0.f);
	SecFrames = 0; SecSumMs = 0.f; SecMaxMs = 0.f; SecKeys = 0; SecFrameMs.Reset();
}

void UYggSessionMonitor::PollChanges()
{
	const UGameInstance* GI = GetGameInstance();
	UWorld* World = GI ? GI->GetWorld() : nullptr;
	const FString Map = World ? World->GetMapName() : FString();
	if (Map != LastMap) { Event(TEXT("level"), Map); LastMap = Map; LevelT = FPlatformTime::Seconds() - StartReal; WriteSessionInfo(); }
	// the rig's own account (postures, leases with their beats, the watchdog): a "rig" event whenever its shape changes - the
	// digits (ages, heights) are left out of the comparison so a running timer does not log every second (V4 reads these)
	if (UYggViewRigComponent* R = Rig.Get())
	{
		const FString A = R->Audit();
		FString Shape;
		Shape.Reserve(A.Len());
		for (int32 i = 0; i < A.Len(); ++i) { if (!FChar::IsDigit(A[i])) { Shape.AppendChar(A[i]); } }
		if (Shape != LastAuditShape) { Event(TEXT("rig"), A); LastAuditShape = MoveTemp(Shape); }
	}
	FString Realm;
	if (const ARealmSky* Sky = World ? ARealmSky::Get(World) : nullptr)
	{
		Realm = Sky->GetShownRealm() == ERealm::None ? FString(TEXT("none")) : FString(ARealmSky::GetWeatherInfo(Sky->GetShownIndex()).Name);
	}
	if (Realm != LastRealm) { Event(TEXT("realm"), Realm); LastRealm = Realm; }
	if (APlayerController* PC = GI ? GI->GetFirstLocalPlayerController() : nullptr)
	{
		if (const APawn* P = PC->GetPawn())
		{
			if (const UExpeditionComponent* E = P->FindComponentByClass<UExpeditionComponent>())
			{
				if (E->GetDeaths() != LastDeaths) { if (LastDeaths >= 0) { Event(TEXT("death"), FString::Printf(TEXT("deaths %d"), E->GetDeaths())); Shot(TEXT("death")); } LastDeaths = E->GetDeaths(); }
				if (E->GetCampName() != LastCamp) { Event(TEXT("camp"), E->GetCampName()); LastCamp = E->GetCampName(); }
			}
		}
	}
	// which screens are up (pause menu, settings, pack, map, death screen, HUD...): every top-level widget by class
	TArray<UUserWidget*> Found;
	if (World) { UWidgetBlueprintLibrary::GetAllWidgetsOfClass(World, Found, UUserWidget::StaticClass(), true); }
	TArray<FString> Names;
	for (const UUserWidget* W : Found) { if (W && W->IsInViewport()) { FString N = W->GetClass()->GetName(); N.RemoveFromEnd(TEXT("_C")); Names.AddUnique(N); } }
	Names.Sort();
	const FString Ui = FString::Join(Names, TEXT(" "));
	if (Ui != LastUi) { Event(TEXT("ui"), Ui); LastUi = Ui; }
	const bool bFocus = FApp::HasFocus();
	if (bFocus != bLastFocus) { Event(TEXT("focus"), bFocus ? TEXT("game window active") : TEXT("game window lost focus (alt-tab or another window)")); bLastFocus = bFocus; }
	// settings: any change in the menu shows up here (key = value list, diffed)
	FString Now;
	for (const TPair<FString, FString>& KV : YggSettings::EngineState(World)) { Now += KV.Key + TEXT("=") + KV.Value + TEXT("; "); }
	if (!LastSettings.IsEmpty() && Now != LastSettings) { Event(TEXT("settings"), Now); }
	LastSettings = Now;
}

void UYggSessionMonitor::Flush()
{
	// the worker writes them (the game thread never waits on the disk)
	const auto Hand = [this](FString& Buf, const TCHAR* Name)
	{
		if (Buf.IsEmpty() || !Worker.IsValid()) { return; }
		const int32 Len = Buf.Len();
		Worker->Writes.Enqueue(TPair<FString, FString>(Name, MoveTemp(Buf)));
		Buf.Reset(Len + Len / 4);                                          // the next 2 s fit without growing frame by frame
	};
	Hand(FramesBuf, TEXT("frames.csv"));
	Hand(SecondsBuf, TEXT("seconds.csv"));
	Hand(EventsBuf, TEXT("events.csv"));
	Hand(LogBuf, TEXT("log.txt"));
	if (!WorkerThread && Worker.IsValid())
	{
		Worker->WriteQueued();                                             // no thread could be made: write here, as before
	}
}

void UYggSessionMonitor::DrainWorker()
{
	if (!Worker.IsValid()) { return; }
	TPair<FString, FString> N;
	while (Worker->Notes.Dequeue(N)) { Event(*N.Key, N.Value); }
}

// ---------------------------------------------------------------- the camera checks (camera spec §10.2, §11; 2026-11-19)

UYggViewRigComponent* UYggSessionMonitor::FindRig(APawn* Pawn)
{
	if (Pawn != RigPawn.Get())
	{
		// a new pawn (spawn, death and continue): its rig and cameras are looked up once, not every frame
		RigPawn = Pawn;
		Rig.Reset();
		NextRigFind = 0.0;
		PawnCams.Reset();
		if (Pawn)
		{
			TArray<UCameraComponent*> Cams;
			Pawn->GetComponents<UCameraComponent>(Cams);
			for (UCameraComponent* C : Cams) { PawnCams.Add(C); }
		}
	}
	if (!Pawn)
	{
		return nullptr;
	}
	// the rig is made in the body's BeginPlay, maybe after the first monitor frame: look again once a second until found
	if (!Rig.IsValid() && FPlatformTime::Seconds() >= NextRigFind)
	{
		NextRigFind = FPlatformTime::Seconds() + 1.0;
		Rig = UYggViewRigComponent::Find(Pawn);
	}
	return Rig.Get();
}

void UYggSessionMonitor::CameraFrame(double T, float Dt, bool bPaused, bool bCursor, bool bPawnView, const FVector& Loc, const FRotator& Rot, const FRotator& Delta,
	float Speed, float Eye, float Lid, float StoryFov, int32 Nudging, int32 Cut, int32 Scripted, int32 Posture, bool bMouseStill, bool bPawnHidden)
{
	if (!Cam.IsValid())
	{
		return;
	}
	FYggCamChecks& C = *Cam;
	const TCHAR* ScrName = MonitorScriptedNames[Scripted + 1];
	const TCHAR* PostureName = MonitorPostureNames[Posture + 1];
	// V5: every flagged cut; a jump only in play (a respawn behind the death screen's menu is not the story's cut)
	if (Cut == 1) { ++C.CutFrames; }
	if (Cut == 2 && !bPaused && !bCursor)
	{
		++C.Jumps;
		FYggCamChecks::Keep(C.JumpList, FString::Printf(TEXT("%s  camera jumped %.0f m with no cut flag (lid %.2f)"), *MonitorClock(T), FVector::Dist(Loc, LastLoc) / 100.0, Lid));
	}
	// what counts as play (as in analyze_session.py): not paused, no menu, not the first 6 s of a level
	const bool bPlay = !bPaused && !bCursor && T > 3.0 && T - LevelT > 6.0 && Dt > 0.f;
	bLastPlay = bPlay;
	if (!bPlay)
	{
		C.WinT0 = -1.0;
		C.RestT0 = -1.0;
		return;
	}
	++C.PlayFrames;
	const float Pitch = float(FRotator::NormalizeAxis(Rot.Pitch));
	const float Roll = FMath::Abs(float(FRotator::NormalizeAxis(Rot.Roll)));
	const bool bTakeover = Scripted == 1 || Scripted == 2;
	const bool bCutNear = Cut != 0 || bLastCut;

	// V1: with the mouse still, the view moves only as fast as the story may move it (§2 R5): nudge 20 deg/s yaw and 8 pitch,
	// the fall 120, the knock 15 deg in 0.14 s, the fly-out 12. Per frame: cap x dt + 1 deg (the spec's test; the 1 deg
	// takes the step bob, the breath and the landing nod). Over 0.5 s: cap + 2 deg/s (a rate the per-frame test cannot see
	// at high frame rates). A reflex (10 deg in 0.3 s) can show as one short per-frame moment at a 30 fps base.
	float CapYaw = 20.f, CapPitch = 8.f;
	if (Scripted == 1) { CapYaw = CapPitch = 120.f; }
	else if (Scripted == 2) { CapYaw = CapPitch = 110.f; }
	else if (Scripted == 3) { CapYaw = CapPitch = 12.f; }
	if (bMouseStill && !bCutNear)
	{
		++C.StillFrames;
		const float DY = FMath::Abs(float(Delta.Yaw)), DP = FMath::Abs(float(Delta.Pitch));
		if (DY > CapYaw * Dt + 1.f || DP > CapPitch * Dt + 1.f)
		{
			++C.StillBad;
			C.V1.Add(T, FMath::Max(DY, DP) / Dt, TEXT("per frame"), ScrName);
		}
		if (C.WinT0 < 0.0 || C.WinScr != Scripted)
		{
			C.WinT0 = T; C.WinYaw0 = float(Rot.Yaw); C.WinPitch0 = Pitch; C.WinScr = Scripted;
		}
		else if (T - C.WinT0 >= 0.5)
		{
			const float Span = float(T - C.WinT0);
			const float RY = FMath::Abs(float(FRotator::NormalizeAxis(Rot.Yaw - double(C.WinYaw0)))) / Span, RP = FMath::Abs(Pitch - C.WinPitch0) / Span;
			if (RY > CapYaw + 2.f || RP > CapPitch + 2.f)
			{
				++C.WindowBad;
				C.V1.Add(T, FMath::Max(RY, RP), TEXT("0.5 s rate"), ScrName);
			}
			C.WinT0 = T; C.WinYaw0 = float(Rot.Yaw); C.WinPitch0 = Pitch;
		}
	}
	else
	{
		C.WinT0 = -1.0;
	}
	// V1: pitch inside [-80, 82] except in the fall
	if (bPawnView && Scripted != 1)
	{
		C.PitchMin = C.PitchFrames ? FMath::Min(C.PitchMin, Pitch) : Pitch;
		C.PitchMax = C.PitchFrames ? FMath::Max(C.PitchMax, Pitch) : Pitch;
		++C.PitchFrames;
		if (Pitch < -80.5f || Pitch > 82.5f) { ++C.PitchOut; if (C.PitchOutT < 0.0) { C.PitchOutT = T; } }
	}

	// V2: walking means standing height - the knee-height bug of shot 7 (52 cm at 2 m/s for 48 s)
	if (bPawnView && !bCutNear && Speed > 0.6f && !bTakeover && Scripted != 3 && Eye >= 0.f)
	{
		++C.Moving;
		if (Eye >= 120.f) { ++C.Moving120; } else { C.V2Low.Add(T, Eye, TEXT("eye under 120 cm while moving"), PostureName, true, 1.0); }
		if (Eye >= 135.f) { ++C.Moving135; }
		if (Eye < C.MovingEyeMin) { C.MovingEyeMin = Eye; C.MovingEyeMinT = T; }
		if (Posture > 0 && Posture != 5) { ++C.MovingPosed; C.V2Posed.Add(T, Speed, TEXT("moving while posed"), PostureName, false, 1.0); }
	}

	// V6: nudges from the rig (when it tells them): each start counted, with its time for the 60 s rule
	if (Nudging >= 0)
	{
		if (Nudging == 1 && C.LastNudging == 0) { ++C.Nudges; if (C.NudgeT.Num() < 64) { C.NudgeT.Add(T); } }
		C.LastNudging = Nudging;
	}

	// V7: standing still with the mouse still, the eye barely moves (breathing at rest: +-0.2 cm); stretches of 3-10 s
	const bool bRest = bPawnView && !bCutNear && Speed < 0.05f && bMouseStill && Scripted <= 0 && Posture <= 0;
	if (bRest)
	{
		const float Z = float(Loc.Z);
		if (C.RestT0 < 0.0) { C.RestT0 = T; C.RestZMin = C.RestZMax = Z; }
		else
		{
			C.RestZMin = FMath::Min(C.RestZMin, Z); C.RestZMax = FMath::Max(C.RestZMax, Z);
			if (T - C.RestT0 >= 10.0) { if (C.RestRanges.Num() < 5000) { C.RestRanges.Add(C.RestZMax - C.RestZMin); } C.RestT0 = -1.0; }
		}
	}
	else if (C.RestT0 >= 0.0)
	{
		if (T - C.RestT0 >= 3.0 && C.RestRanges.Num() < 5000) { C.RestRanges.Add(C.RestZMax - C.RestZMin); }
		C.RestT0 = -1.0;
	}

	// V14: the fly-out - roll exactly 0, at most 12 deg/s, no frame over 50 ms, the pawn shown again afterwards
	if (Scripted == 3)
	{
		if (!C.bInFly) { C.bInFly = true; C.FlyT0 = T; C.FlyRoll = C.FlyTurn = C.FlyDt = 0.f; }
		else if (!bCutNear)
		{
			C.FlyRoll = FMath::Max(C.FlyRoll, Roll);
			C.FlyTurn = FMath::Max(C.FlyTurn, FMath::Sqrt(float(Delta.Yaw * Delta.Yaw + Delta.Pitch * Delta.Pitch)) / Dt);
			C.FlyDt = FMath::Max(C.FlyDt, Dt * 1000.f);
		}
	}
	else if (C.bInFly)
	{
		C.bInFly = false;
		++C.FlyShots;
		C.FlyRollAll = FMath::Max(C.FlyRollAll, C.FlyRoll);
		C.FlyTurnAll = FMath::Max(C.FlyTurnAll, C.FlyTurn);
		C.FlyDtAll = FMath::Max(C.FlyDtAll, C.FlyDt);
		FYggCamChecks::Keep(C.FlyList, FString::Printf(TEXT("%s - %s (%.1f s): roll max %.2f deg, turn max %.1f deg/s, slowest frame %.0f ms"),
			*MonitorClock(C.FlyT0), *MonitorClock(T), T - C.FlyT0, C.FlyRoll, C.FlyTurn, C.FlyDt));
		C.FlyPawnCheckT = T + 2.0;
	}
	if (C.FlyPawnCheckT > 0.0 && T >= C.FlyPawnCheckT)
	{
		if (bPawnHidden) { ++C.FlyPawnHidden; FYggCamChecks::Keep(C.FlyList, FString::Printf(TEXT("%s  the pawn is still hidden 2 s after the fly-out"), *MonitorClock(T))); }
		C.FlyPawnCheckT = -1.0;
	}

	// invariants (§4.6), on the player's own view
	if (bPawnView && !bCutNear)
	{
		++C.InvFrames;
		C.RollMax = FMath::Max(C.RollMax, Roll);
		if (Roll > (bTakeover ? 30.5f : 2.05f)) { ++C.RollBad; if (C.RollBadT < 0.0) { C.RollBadT = T; } }
		C.FovLo = C.InvFrames > 1 ? FMath::Min(C.FovLo, StoryFov) : StoryFov;
		C.FovHi = C.InvFrames > 1 ? FMath::Max(C.FovHi, StoryFov) : StoryFov;
		if (FMath::Abs(StoryFov) > 5.05f) { ++C.FovBad; if (C.FovBadT < 0.0) { C.FovBadT = T; } }
		if (Eye >= 0.f)
		{
			C.EyeMin = FMath::Min(C.EyeMin, Eye);
			if (Eye < (bTakeover ? 29.5f : 19.5f)) { ++C.EyeLow; if (C.EyeLowT < 0.0) { C.EyeLowT = T; } }
		}
	}
}

void UYggSessionMonitor::CameraLogLine(const FString& Line, double T)
{
	if (!Cam.IsValid())
	{
		return;
	}
	FYggCamChecks& C = *Cam;
	// the eyelids now: the story's cut runs on the first fully closed frame, so the lid is still closed when its line is read
	const float Lid = Rig.IsValid() ? Rig->GetLid() : LastLid;
	const FString Stamp = MonitorClock(T);
	if (Line.Contains(TEXT("camera cut")))
	{
		bCutLog = true;
		++C.CutLines;
		const bool bDark = Lid >= 0.99f;
		if (bDark) { ++C.CutDark; }
		FYggCamChecks::Keep(C.CutList, FString::Printf(TEXT("%s  lid %.2f%s  %s"), *Stamp, Lid, bDark ? TEXT("") : TEXT("  <- eyes open (under the black veil?)"), *Line));
	}
	if (Line.Contains(TEXT("Story: teleport"))) { ++C.Teleports; }
	if (Line.Contains(TEXT("blink"), ESearchCase::IgnoreCase) && (Line.Contains(TEXT("auto"), ESearchCase::IgnoreCase) || Line.Contains(TEXT("wrap"), ESearchCase::IgnoreCase)))
	{
		++C.AutoBlinks;
		FYggCamChecks::Keep(C.BlinkLines, Stamp + TEXT("  ") + Line);
	}
	const bool bRepair = Line.Contains(TEXT("Rig: repaired"));
	const bool bStand = Line.Contains(TEXT("standing watchdog"), ESearchCase::IgnoreCase) || (bRepair && (Line.Contains(TEXT("knee"), ESearchCase::IgnoreCase) || Line.Contains(TEXT("eye height"), ESearchCase::IgnoreCase)));
	if (bRepair || bStand)
	{
		if (bRepair) { ++C.Repairs; }
		if (bStand) { ++C.StandWatchdog; }
		FYggCamChecks::Keep(C.RepairLines, Stamp + TEXT("  ") + Line);
	}
	if (Line.Contains(TEXT("Rig: lease")) && Line.Contains(TEXT("expired")))
	{
		++C.Expired;
		FYggCamChecks::Keep(C.ExpiredLines, Stamp + TEXT("  ") + Line);
	}
	if (Line.Contains(TEXT("nudge"), ESearchCase::IgnoreCase) && (Line.Contains(TEXT("Rig")) || Line.Contains(TEXT("Story"))))
	{
		FYggCamChecks::Keep(C.NudgeLines, Stamp + TEXT("  ") + Line);
	}
}

FString UYggSessionMonitor::CameraReport() const
{
	if (!Cam.IsValid())
	{
		return FString();
	}
	const FYggCamChecks& C = *Cam;
	const double Now = FPlatformTime::Seconds() - StartReal;
	FString S;
	S += TEXT("# Camera checks (Chapter1_Camera_Spec.md §11)\n\n");
	S += FString::Printf(TEXT("Written by the game's session monitor at %s (rewritten every 30 s and at the end). Counted over play frames only: not paused, no menu, not the first 6 s of a level (%lld play frames).\n\n"),
		*MonitorClock(Now), C.PlayFrames);
	S += FString::Printf(TEXT("- View rig: %s.\n"), C.RigFrames > 0 ? TEXT("seen") : TEXT("**not seen** (old camera or ygg.View.Rig 0: the eye is measured from the camera, posture checks have no data)"));
	S += FString::Printf(TEXT("- Takeover state: %s.\n"), C.bScriptedKnown ? TEXT("from the rig (GetScripted)") : TEXT("**not told by the rig** - the fly-out is seen through the view target, but the fall and the knock count as plain walking (their fast turns show up under V1)"));
	S += FString::Printf(TEXT("- Leases: %s. Nudges: %s.\n\n"), C.bLeasesKnown ? TEXT("from the rig (GetLeaseMask)") : TEXT("not in frames.csv (the rig's Audit() is logged as `rig` events in events.csv when it changes)"),
		C.bNudgeKnown ? TEXT("from the rig (IsNudging)") : TEXT("only from the log lines below"));

	auto Pct = [](int64 A, int64 B) { return B > 0 ? 100.0 * double(A) / double(B) : 0.0; };
	auto Verdict = [](bool bData, bool bPass) { return !bData ? TEXT("NO DATA") : bPass ? TEXT("PASS") : TEXT("**FAIL**"); };
	auto At = [](double T) { return T >= 0.0 ? FString::Printf(TEXT(", first at %s"), *MonitorClock(T)) : FString(); };

	S += TEXT("| Check | Result | Measured | Pass when |\n|---|---|---|---|\n");
	S += FString::Printf(TEXT("| V1 view moves without the mouse | %s | %d moments; %lld of %lld still-mouse frames over the per-frame cap, %lld half-seconds over the rate cap | per frame <= cap x dt + 1 deg; per 0.5 s <= cap + 2 deg/s (nudge 20 yaw / 8 pitch, fall 120, knock 110, fly-out 12 deg/s) |\n"),
		Verdict(C.StillFrames > 0, C.StillBad + C.WindowBad == 0), C.V1.Count, C.StillBad, C.StillFrames, C.WindowBad);
	S += FString::Printf(TEXT("| V1 pitch range | %s | %.1f .. %.1f deg; %lld frames outside%s | within [-80, 82] outside the fall |\n"),
		Verdict(C.PitchFrames > 0, C.PitchOut == 0), C.PitchMin, C.PitchMax, C.PitchOut, *At(C.PitchOutT));
	S += FString::Printf(TEXT("| V2 eye height while moving | %s | %.2f%% >= 120 cm, %.2f%% >= 135 cm of %lld frames over 0.6 m/s; lowest %.0f cm at %s | 100%% >= 120 cm, 99%% >= 135 cm |\n"),
		Verdict(C.Moving > 0, C.Moving120 == C.Moving && Pct(C.Moving135, C.Moving) >= 99.0), Pct(C.Moving120, C.Moving), Pct(C.Moving135, C.Moving), C.Moving,
		C.Moving > 0 ? C.MovingEyeMin : 0.f, C.Moving > 0 ? *MonitorClock(C.MovingEyeMinT) : TEXT("-"));
	S += FString::Printf(TEXT("| V2 moving while posed | %s | %lld frames over 0.6 m/s not standing or crawling | 0 |\n"),
		Verdict(C.RigFrames > 0 && C.Moving > 0, C.MovingPosed == 0), C.MovingPosed);
	S += FString::Printf(TEXT("| V2 standing watchdog | %s | %d log lines (%d rig repairs in all) | 0 |\n"),
		Verdict(true, C.StandWatchdog == 0), C.StandWatchdog, C.Repairs);
	S += FString::Printf(TEXT("| V4 leases expired | %s | %d log lines | none unplanned |\n"), Verdict(true, C.Expired == 0), C.Expired);
	S += FString::Printf(TEXT("| V5 cuts only in black | %s | %d cut lines, %d with the eyes closed; %d flagged frames; %d unflagged camera jumps; %d teleports logged | every cut with lid >= 0.99 (or the black veil); no jumps; cuts = teleports + 2 |\n"),
		Verdict(C.CutLines + C.CutFrames + C.Jumps > 0, C.CutLines == C.CutDark && C.Jumps == 0), C.CutLines, C.CutDark, C.CutFrames, C.Jumps, C.Teleports);
	S += FString::Printf(TEXT("| V5 automatic blinks | %s | %d warnings | 0 in chapter 1 |\n"), Verdict(true, C.AutoBlinks == 0), C.AutoBlinks);
	{
		double Gap = 1e9;
		for (int32 i = 1; i < C.NudgeT.Num(); ++i) { Gap = FMath::Min(Gap, C.NudgeT[i] - C.NudgeT[i - 1]); }
		const FString GapText = C.NudgeT.Num() > 1 ? FString::Printf(TEXT(", closest %.0f s apart"), Gap) : FString();
		S += FString::Printf(TEXT("| V6 nudges | %s | %s | <= 3, >= 60 s apart |\n"), Verdict(C.bNudgeKnown, C.Nudges <= 3 && Gap >= 60.0),
			C.bNudgeKnown ? *FString::Printf(TEXT("%d nudges%s"), C.Nudges, *GapText) : *FString::Printf(TEXT("%d log lines mention a nudge (see below)"), C.NudgeLines.Num()));
	}
	{
		TArray<float> R = C.RestRanges;
		R.Sort();
		const float Median = R.Num() ? R[R.Num() / 2] : 0.f;
		S += FString::Printf(TEXT("| V7 still view at rest | %s | %d stretches of 3-10 s; eye Z range median %.2f cm, max %.2f cm | <= 0.4 cm at rest (breathing harder after effort or above 7,000 m is allowed more) |\n"),
			R.Num() == 0 ? TEXT("NO DATA") : Median <= 0.4f ? TEXT("PASS") : TEXT("CHECK"), R.Num(), Median, R.Num() ? R.Last() : 0.f);
	}
	S += FString::Printf(TEXT("| V14 fly-out | %s | %d shots; roll max %.2f deg, turn max %.1f deg/s, slowest frame %.0f ms; pawn still hidden after %d | roll 0, <= 12 deg/s, no frame over 50 ms, the pawn shown again |\n"),
		Verdict(C.FlyShots > 0, C.FlyRollAll <= 0.01f && C.FlyTurnAll <= 12.5f && C.FlyDtAll <= 50.f && C.FlyPawnHidden == 0), C.FlyShots, C.FlyRollAll, C.FlyTurnAll, C.FlyDtAll, C.FlyPawnHidden);
	S += FString::Printf(TEXT("| V16 stalls with the eyes open | %s | %d stalls over 100 ms in play with lid < 0.99 | 0 |\n"), Verdict(true, C.OpenStalls == 0), C.OpenStalls);
	S += FString::Printf(TEXT("| Invariant: roll | %s | max %.2f deg; %lld frames over%s | <= 2 deg (30 in a takeover) |\n"), Verdict(C.InvFrames > 0, C.RollBad == 0), C.RollMax, C.RollBad, *At(C.RollBadT));
	S += FString::Printf(TEXT("| Invariant: story FOV | %s | %.2f .. %.2f deg over the settings FOV; %lld frames outside%s | within +-5 deg (story FOV, attention, FOV beat, heart FOV together) |\n"),
		Verdict(C.InvFrames > 0, C.FovBad == 0), C.FovLo, C.FovHi, C.FovBad, *At(C.FovBadT));
	S += FString::Printf(TEXT("| Invariant: eye above the floor | %s | lowest %.0f cm; %lld frames under%s | >= 20 cm (30 in a takeover) |\n"),
		Verdict(C.EyeMin < 1e5f, C.EyeLow == 0), C.EyeMin < 1e5f ? C.EyeMin : 0.f, C.EyeLow, *At(C.EyeLowT));
	S += TEXT("\nNot measured here: V3 (`ygg.Story.CamTest`), V8 (the 0.2-0.4 Hz pitch band: an FFT of frames.csv pitch), V9-V13 (screenshots, the director's eye, `stat YggView`), V15 (`python -I Tools/Story/chapter1_story.py`), V17 (input safety).\n");

	auto Episodes = [&S](const TCHAR* Title, const FYggCamEpisodes& E, const TCHAR* Unit)
	{
		if (E.Count == 0) { return; }
		S += FString::Printf(TEXT("\n### %s (%d)\n\n"), Title, E.Count);
		for (const FYggCamEpisode& X : E.L)
		{
			S += FString::Printf(TEXT("- %s - %s  %s %.1f %s\n"), *MonitorClock(X.T0), *MonitorClock(X.T1), *X.What, X.Peak, Unit);
		}
	};
	auto Lines = [&S](const TCHAR* Title, const TArray<FString>& L, int32 Count)
	{
		if (L.Num() == 0) { return; }
		S += FString::Printf(TEXT("\n### %s (%d)\n\n"), Title, Count);
		for (const FString& X : L) { S += TEXT("- ") + X + TEXT("\n"); }
	};
	S += TEXT("\n## Moments\n");
	Episodes(TEXT("V1: the view moved faster than the story may move it (mouse still)"), C.V1, TEXT("deg/s peak"));
	Episodes(TEXT("V2: low eye while moving"), C.V2Low, TEXT("cm lowest"));
	Episodes(TEXT("V2: moving while posed"), C.V2Posed, TEXT("m/s peak"));
	Lines(TEXT("V2: rig repairs"), C.RepairLines, C.Repairs);
	Lines(TEXT("V4: expired leases"), C.ExpiredLines, C.Expired);
	Lines(TEXT("V5: camera cuts"), C.CutList, C.CutLines);
	Lines(TEXT("V5: unflagged camera jumps"), C.JumpList, C.Jumps);
	Lines(TEXT("V5: automatic blinks"), C.BlinkLines, C.AutoBlinks);
	if (C.NudgeT.Num() > 0)
	{
		S += FString::Printf(TEXT("\n### V6: nudges from the rig (%d)\n\n"), C.Nudges);
		for (double X : C.NudgeT) { S += TEXT("- ") + MonitorClock(X) + TEXT("\n"); }
	}
	Lines(TEXT("V6: log lines about nudges"), C.NudgeLines, C.NudgeLines.Num());
	Lines(TEXT("V14: fly-outs"), C.FlyList, C.FlyShots);
	Lines(TEXT("V16: stalls with the eyes open"), C.StallList, C.OpenStalls);
	return S;
}
