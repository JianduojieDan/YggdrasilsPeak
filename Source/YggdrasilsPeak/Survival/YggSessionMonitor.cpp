#include "Survival/YggSessionMonitor.h"
#include "Survival/YggSettings.h"
#include "Survival/SurvivalComponent.h"
#include "Survival/ExpeditionComponent.h"
#include "Survival/SanityComponent.h"
#include "Survival/ClimbingComponent.h"
#include "Survival/YggTime.h"
#include "Environment/RealmSky.h"
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
}

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
			TEXT("LogGarbage"), TEXT("LogConsoleResponse"), TEXT("LogEngine"), TEXT("LogStats"), TEXT("LogStreamline"), TEXT("LogDLSS") };
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
	// every file is written by the worker (folders are made by the first write into them); the headers go first
	Worker = MakeShared<FYggMonitorWorker>(Dir, StartReal);
	Worker->Writes.Enqueue(TPair<FString, FString>(TEXT("!frames.csv"), TEXT("t,dt_ms,game_ms,render_ms,rhi_ms,gpu_ms,fps_presented,draws,prims_k,x_m,y_m,z_m,yaw,pitch,turn_dps,mouse_dx,mouse_dy,speed_ms,loading,streaming,paused,cursor,hitch\n")));
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
		TEXT("sg.PostProcessQuality"), TEXT("sg.TextureQuality"), TEXT("sg.EffectsQuality"), TEXT("sg.FoliageQuality") })
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
	const UGameInstance* GI = GetGameInstance();
	UWorld* World = GI ? GI->GetWorld() : nullptr;
	if (APlayerController* PC = GI ? GI->GetFirstLocalPlayerController() : nullptr)
	{
		if (PC->PlayerCameraManager) { Loc = PC->PlayerCameraManager->GetCameraLocation(); Rot = PC->PlayerCameraManager->GetCameraRotation(); }
		if (const APawn* P = PC->GetPawn()) { Speed = float(P->GetVelocity().Size()) / 100.f; }
		bCursor = PC->bShowMouseCursor;
		bPaused = World && World->IsPaused();
	}
	float Turn = 0.f;
	if (bHaveRot && Dt > 0.f)
	{
		const FRotator D = (Rot - LastRot).GetNormalized();
		Turn = FMath::Sqrt(D.Yaw * D.Yaw + D.Pitch * D.Pitch) / Dt;
	}
	LastRot = Rot; bHaveRot = true;
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

	FramesBuf += FString::Printf(TEXT("%.4f,%.2f,%.2f,%.2f,%.2f,%.2f,%.1f,%d,%.0f,%.1f,%.1f,%.1f,%.1f,%.1f,%.0f,%.0f,%.0f,%.2f,%d,%d,%d,%d,%d\n"),
		Now - StartReal, DtMs, GameMs, RenderMs, RhiMs, GpuMs, PresentedFps, GNumDrawCallsRHI[0], GNumPrimitivesDrawnRHI[0] / 1000.f,
		Loc.X / 100.f, Loc.Y / 100.f, Loc.Z / 100.f, Rot.Yaw, Rot.Pitch, Turn, MouseDx, MouseDy, Speed, bLoading ? 1 : 0, Streaming, bPaused ? 1 : 0, bCursor ? 1 : 0, bHitch ? 1 : (bSlow ? 2 : 0));
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
	Event(TEXT("hitch"), FString::Printf(TEXT("%.1f ms (game %.1f, render %.1f, gpu %.1f; %s)%s"), DtMs, GameMs, RenderMs, GpuMs, Bound, *Around));
	if (DtMs > 100.f && T - LastHitchShot > 10.0 && Shots < 60) { LastHitchShot = T; Shot(TEXT("hitch")); }
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
	if (Map != LastMap) { Event(TEXT("level"), Map); LastMap = Map; WriteSessionInfo(); }
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
