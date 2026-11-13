#include "Survival/YggSessionMonitor.h"
#include "Survival/YggSettings.h"
#include "Survival/SurvivalComponent.h"
#include "Survival/ExpeditionComponent.h"
#include "Survival/SanityComponent.h"
#include "Survival/ClimbingComponent.h"
#include "Survival/YggTime.h"
#include "Environment/RealmSky.h"
#include "Async/Async.h"
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

static TAutoConsoleVariable<int32> CVarMonitor(TEXT("ygg.Monitor"),
#if UE_BUILD_SHIPPING
	0,
#else
	1,
#endif
	TEXT("1 = record the play session to Saved/Monitor (frames, hitches, input, log, body, screenshots), 0 = off. Read at start."));
static TAutoConsoleVariable<float> CVarMonitorShotEvery(TEXT("ygg.Monitor.ShotEvery"), 120.f,
	TEXT("Seconds between monitor screenshots, the first 20 s in (0 = only at big hitches). Each costs one short pause (the frame is read back)."));

namespace
{
	FString Csv(FString S) { S.ReplaceInline(TEXT("\""), TEXT("'")); S.ReplaceInline(TEXT("\n"), TEXT(" ")); S.ReplaceInline(TEXT("\r"), TEXT("")); return TEXT("\"") + S + TEXT("\""); }
	float Ms(uint32 Cycles) { return float(FPlatformTime::ToMilliseconds(Cycles)); }
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

/** The engine's log, filtered to what helps explain a session (and to what a player would never see). */
class FYggMonitorLog : public FOutputDevice
{
public:
	explicit FYggMonitorLog(UYggSessionMonitor* InOwner) : Owner(InOwner) { bAutoEmitLineTerminator = false; }
	virtual void Serialize(const TCHAR* V, ELogVerbosity::Type Verbosity, const FName& Category) override
	{
		if (!Owner.IsValid() || !IsInGameThread()) { return; }
		static const FName Keep[] = { TEXT("LogRenderer"), TEXT("LogStreaming"), TEXT("LogStaticMesh"), TEXT("LogShaders"), TEXT("LogRHI"),
			TEXT("LogD3D12RHI"), TEXT("LogAudio"), TEXT("LogNiagara"), TEXT("LogTemp"), TEXT("LogSurvival"), TEXT("LogTexture"), TEXT("LogContentStreaming"),
			TEXT("LogPSO"), TEXT("LogShaderCompilers"), TEXT("LogMaterial"), TEXT("LogLumen"), TEXT("LogNanite"), TEXT("LogViewport"), TEXT("LogLoad"), TEXT("LogWorld"),
			TEXT("LogGarbage"), TEXT("LogConsoleResponse"), TEXT("LogEngine"), TEXT("LogStats"), TEXT("LogStreamline"), TEXT("LogDLSS") };
		bool bKeep = Verbosity <= ELogVerbosity::Warning;
		for (const FName& K : Keep) { bKeep |= Category == K; }
		if (bKeep) { Owner->OnLogLine(FString::Printf(TEXT("%s: %s"), *Category.ToString(), V)); }
	}
private:
	TWeakObjectPtr<UYggSessionMonitor> Owner;
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
	IFileManager::Get().MakeDirectory(*(Dir / TEXT("shots")), true);
	StartReal = FPlatformTime::Seconds();
	StartEpoch = (FDateTime::UtcNow() - FDateTime(1970, 1, 1)).GetTotalSeconds();      // lines the system monitor up with the frames
	LastFlush = LastSecond = StartReal;
	bOn = true;
	FFileHelper::SaveStringToFile(TEXT("t,dt_ms,game_ms,render_ms,rhi_ms,gpu_ms,fps_presented,draws,prims_k,x_m,y_m,z_m,yaw,pitch,turn_dps,mouse_dx,mouse_dy,speed_ms,loading,streaming,paused,cursor,hitch\n"),
		*(Dir / TEXT("frames.csv")), FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
	FFileHelper::SaveStringToFile(TEXT("t,fps,fps_min,fps_low1,max_ms,fps_presented,mem_mb,map,alt_m,state,spo2,hr_bpm,core_c,stamina,satiety,hydration,health,sanity,exhausted,sheltered,on_rope,o2_bottle_s,bottles,storm,wind,realm,uds_time,air_c,keys,paused,cursor,focus,x_m,y_m\n"),
		*(Dir / TEXT("seconds.csv")), FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
	FFileHelper::SaveStringToFile(TEXT("t,type,detail\n"), *(Dir / TEXT("events.csv")), FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
	Recent.Init(16.7f, 240);
	if (FSlateApplication::IsInitialized())
	{
		Input = MakeShared<FYggMonitorInput>(this);
		FSlateApplication::Get().RegisterInputPreProcessor(Input);
	}
	ImageWrappers = &FModuleManager::LoadModuleChecked<IImageWrapperModule>(TEXT("ImageWrapper"));
	LogDevice = MakeShared<FYggMonitorLog>(this);
	GLog->AddOutputDevice(LogDevice.Get());
	// session.txt is written once the first level is up (viewport, player and settings exist), again on each new level
	Event(TEXT("session_start"), Dir);
	UE_LOG(LogTemp, Display, TEXT("Session monitor: recording to %s"), *Dir);
}

void UYggSessionMonitor::Deinitialize()
{
	if (bOn)
	{
		Event(TEXT("session_end"), FString::Printf(TEXT("%d frames, %d hitches, %d screenshots"), Frames, Hitches, Shots));
		Flush();
		if (Input.IsValid() && FSlateApplication::IsInitialized()) { FSlateApplication::Get().UnregisterInputPreProcessor(Input); }
		if (LogDevice.IsValid()) { GLog->RemoveOutputDevice(LogDevice.Get()); }
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
	FFileHelper::SaveStringToFile(S, *(Dir / TEXT("session.txt")), FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
}

void UYggSessionMonitor::Event(const TCHAR* Type, const FString& Detail)
{
	if (!bOn) { return; }
	EventsBuf += FString::Printf(TEXT("%.3f,%s,%s\n"), FPlatformTime::Seconds() - StartReal, Type, *Csv(Detail));
}

void UYggSessionMonitor::OnKey(const FString& Key, bool bDown)
{
	if (bDown) { if (!KeysDown.Contains(Key)) { KeysDown.Add(Key); ++SecKeys; Event(TEXT("key_down"), Key); } }
	else if (KeysDown.Remove(Key) > 0) { Event(TEXT("key_up"), Key); }
}

void UYggSessionMonitor::OnLogLine(const FString& Line)
{
	if (!bOn) { return; }
	const double T = FPlatformTime::Seconds() - StartReal;
	FString Clean = Line; Clean.TrimEndInline();
	if (Clean == LastLogLine) { ++LastLogRepeats; return; }               // fold repeats (one line spammed 200,000 times before)
	if (LastLogRepeats > 0) { LogBuf += FString::Printf(TEXT("%.3f  (previous line repeated %d more times)\n"), T, LastLogRepeats); LastLogRepeats = 0; }
	LastLogLine = Clean;
	if (Clean.StartsWith(TEXT("LogSurvival: Expedition: "))) { Event(TEXT("message"), Clean.Mid(25)); }
	LogBuf += FString::Printf(TEXT("%.3f  %s\n"), T, *Clean);
	RecentLog.Emplace(T, Clean);
	if (RecentLog.Num() > 64) { RecentLog.RemoveAt(0, RecentLog.Num() - 64); }
}

void UYggSessionMonitor::Tick(float DeltaTime)
{
	if (!bOn) { return; }
	const float Dt = float(FApp::GetDeltaTime());
	++Frames;
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
	const float GameMs = Ms(GGameThreadTime), RenderMs = Ms(GRenderThreadTime), RhiMs = Ms(GRHIThreadTime), GpuMs = Ms(RHIGetGPUFrameCycles(0));
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

	// hitch: over 33 ms, or over 2.5x the recent median. Column value 2 = slow because of the monitor's own screenshot
	// (the frame is read back), so the analyser can leave it out; level loads are told apart by the analyser.
	TArray<float> Sorted = Recent; Sorted.Sort();
	const float Median = Sorted[Sorted.Num() / 2];
	const bool bSlow = Frames > 5 && (DtMs > 33.4f || DtMs > Median * 2.5f) && DtMs > 20.f;
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
	SecondsBuf += FString::Printf(TEXT("%.1f,%.1f,%.1f,%.1f,%.1f,%.1f,%.0f,%s,%.0f,%s,%.1f,%.0f,%.2f,%.0f,%.0f,%.0f,%.0f,%.0f,%d,%d,%d,%.0f,%d,%.2f,%.2f,%s,%.2f,%.0f,%d,%d,%d,%d,%.0f,%.0f\n"),
		T, Fps, SecMaxMs > 0.f ? 1000.f / SecMaxMs : 0.f, Low1, SecMaxMs, PresentedFps, Mem.UsedPhysical / 1048576.0, *Map, Alt, *State, Spo2, Hr, Core, Stam, Sat, Hyd, Health, Sanity,
		Exhausted, Shelter, Rope, Bottle, Bottles, Storm, Wind, *Csv(Realm), World ? YggTime::Get(World) : 0.f, Air, SecKeys, bPaused ? 1 : 0, bCursor ? 1 : 0, FApp::HasFocus() ? 1 : 0, X, Y);
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
	const auto Append = [this](FString& Buf, const TCHAR* Name)
	{
		if (Buf.IsEmpty()) { return; }
		FFileHelper::SaveStringToFile(Buf, *(Dir / Name), FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM, &IFileManager::Get(), FILEWRITE_Append);
		Buf.Reset();
	};
	Append(FramesBuf, TEXT("frames.csv"));
	Append(SecondsBuf, TEXT("seconds.csv"));
	Append(EventsBuf, TEXT("events.csv"));
	Append(LogBuf, TEXT("log.txt"));
}
