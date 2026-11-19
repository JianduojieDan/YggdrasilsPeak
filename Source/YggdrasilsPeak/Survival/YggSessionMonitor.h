// The play-session monitor (2026-11-13, user: "write a script to monitor me play the game... monitor literally everything,
// only then you understand what is going on"). Records a whole session so it can be analysed afterwards
// (Tools/Monitor/analyze_session.py): every frame's timing, every hitch with its context, every key and mouse move, the
// engine's own log, the climber's body and place every second, settings, menus, weather, and screenshots.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tickable.h"
#include "YggSessionMonitor.generated.h"

class FYggMonitorInput;
class FYggMonitorLog;
class FYggMonitorWorker;
class FYggMonitorWatchdog;
class FRunnableThread;
struct FYggCamChecks;
class UYggViewRigComponent;
class UCameraComponent;
class APawn;

/**
 *  Writes Saved/Monitor/<date_time>/:
 *    session.txt  hardware, command line, every setting at the start (and later changes in events.csv)
 *    frames.csv   one row per frame: time, frame/game/render/RHI/GPU ms, presented FPS (DLSS frame generation), draw
 *                 calls, triangles, camera place and direction, turning speed, mouse movement, loading/streaming, flags;
 *                 (2026-11-19, camera spec §10.2) the view: roll, posture, eye above the floor (cm), eyelids, the lens
 *                 offset over the settings FOV (story_fov), nudge, leases, camera cut (1 flagged, 2 an unflagged jump
 *                 of the camera over 5 m, 3 a new pawn or level) and the scripted takeover ("-" = the rig cannot tell)
 *    camera_checks.md  the camera spec's §11 automatic checks (V1, V2, V4-V7, V14, V16, invariants) with PASS/FAIL,
 *                 rewritten every 30 s and at the end (Tools/Monitor/analyze_session.py can fold it into report.md)
 *    seconds.csv  one row per second: FPS (avg, min, 1% low), the body (SpO2, heart, core, stamina, health...), what the
 *                 climber does, realm, time of day, air, memory, keys pressed, paused or in a menu
 *    events.csv   keys down/up, hitches (with the log lines around them), expedition messages, realm and settings
 *                 changes, level loads, screenshots
 *    log.txt      the engine's log lines worth reading (warnings, errors, rendering/streaming/shader/audio lines),
 *                 repeats folded into counts
 *    shots/       a screenshot every 120 s, at big hitches and deaths
 *    stalls.txt   (2026-11-14) where the game thread was stuck whenever it had not started a frame for 0.4 s: call
 *                 stacks taken by a watchdog thread while it is stuck (named once the game runs again); also slow
 *                 disk writes found by the disk probe. ygg.Monitor.StallStacks 0 = only the times.
 *  All files are written by a worker thread (the game thread never waits on the disk), which also probes the disk once a
 *  second. The watchdog is a thread of its own that never touches the disk, so a stalled drive cannot silence it.
 *  On while ygg.Monitor is 1 (default in development builds; test runs with -YggNoSave are off unless -YggMonitor).
 */
UCLASS()
class YGGDRASILSPEAK_API UYggSessionMonitor : public UGameInstanceSubsystem, public FTickableGameObject
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// FTickableGameObject: every frame, paused or not, menus included
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual bool IsTickable() const override { return bOn; }
	virtual bool IsTickableWhenPaused() const override { return true; }
	virtual bool IsTickableInEditor() const override { return false; }
	virtual ETickableTickType GetTickableTickType() const override { return ETickableTickType::Conditional; }

	/** Anything worth a line in events.csv (called by the input and log recorders, and by game code). */
	void Event(const TCHAR* Type, const FString& Detail);
	void OnKey(const FString& Key, bool bDown);
	void OnMouse(float Dx, float Dy) { MouseDx += Dx; MouseDy += Dy; }
	void OnLogLine(const FString& Line, double When);

private:
	void Start();
	void WriteSessionInfo();
	void FrameRow(float Dt);
	void SecondRow();
	void Hitch(float Dt, float GameMs, float RenderMs, float GpuMs);
	void Shot(const TCHAR* Why);
	void OnShotCaptured(int32 W, int32 H, const TArray<FColor>& Px);
	void Flush();
	void PollChanges();
	void DrainLog();
	void DrainWorker();
	/** The view rig of the player's pawn (found again when the pawn changes, retried once a second while missing). */
	UYggViewRigComponent* FindRig(APawn* Pawn);
	/** One frame of the camera checks (camera spec §11), from the values just written to frames.csv. */
	void CameraFrame(double T, float Dt, bool bPaused, bool bCursor, bool bPawnView, const FVector& Loc, const FRotator& Rot, const FRotator& Delta,
		float Speed, float Eye, float Lid, float StoryFov, int32 Nudging, int32 Cut, int32 Scripted, int32 Posture, bool bMouseStill, bool bPawnHidden);
	/** A log line the camera checks count (cuts, repairs, expired leases, automatic blinks, nudges, teleports). */
	void CameraLogLine(const FString& Line, double T);
	/** camera_checks.md: the §11 checks as a table with PASS / FAIL / INFO and the moments behind each. */
	FString CameraReport() const;

	bool bOn = false;
	FString Dir;
	FString FramesBuf, SecondsBuf, EventsBuf, LogBuf;
	double StartReal = 0.0, StartEpoch = 0.0;
	double LastFlush = 0.0, LastSecond = 0.0, LastShot = -100.0, LastHitchShot = -100.0;
	int32 Shots = 0, Hitches = 0, Frames = 0;
	// this second
	int32 SecFrames = 0, SecKeys = 0;
	float SecMaxMs = 0.f, SecSumMs = 0.f;
	TArray<float> SecFrameMs;
	// rolling frame-time history for the hitch threshold
	TArray<float> Recent;
	int32 RecentHead = 0;
	// camera and input
	FRotator LastRot = FRotator::ZeroRotator;
	bool bHaveRot = false;
	float MouseDx = 0.f, MouseDy = 0.f;
	TSet<FString> KeysDown;
	// recent log lines (for the hitch context) and repeat folding
	TArray<TPair<double, FString>> RecentLog;
	TMap<FString, int32> LogCounts;
	// state seen last poll
	FString LastSettings, LastRealm, LastMap, LastCamp, LastUi;
	int32 LastDeaths = -1;
	bool bLastFocus = true;
	float PresentedFps = 0.f;
	double LastFg = 0.0;
	double ShotQuietUntil = 0.0;
	FString PendingShot;
	FDelegateHandle ShotHandle;
	class IImageWrapperModule* ImageWrappers = nullptr;
	// the view rig and the camera checks (2026-11-19)
	TWeakObjectPtr<UYggViewRigComponent> Rig;
	TWeakObjectPtr<APawn> RigPawn;
	double NextRigFind = 0.0;
	TArray<TWeakObjectPtr<UCameraComponent>> PawnCams;  // the pawn's cameras: the active one holds the settings FOV
	double LevelT = -100.0;                             // when the current level appeared (its first 6 s are loading)
	bool bCutLog = false;                               // a "camera cut" log line arrived this frame
	bool bLastMouseStill = false, bLastCut = false;
	FVector LastLoc = FVector::ZeroVector;              // last frame's camera place (an unflagged jump of over 5 m is a hard cut)
	float LastLid = 0.f;                                // for the hitch events (a stall with the eyes open, V16)
	bool bLastPlay = false;
	int64 LastLeaseMask = -2;
	FString LastLeaseText, LastAuditShape;
	double LastCamReport = 0.0;
	TSharedPtr<FYggCamChecks> Cam;                      // shared: its deleter is made in the .cpp where the type is complete

	TSharedPtr<FYggMonitorInput> Input;
	TSharedPtr<FYggMonitorLog> LogDevice;      // shared: its deleter is made in the .cpp where the type is complete
	TSharedPtr<FYggMonitorWorker> Worker;      // writes the files, names the stall stacks, probes the disk (own thread)
	FRunnableThread* WorkerThread = nullptr;
	TSharedPtr<FYggMonitorWatchdog> Watchdog;  // notices a stuck game thread and takes its stack (own thread, no disk)
	FRunnableThread* WatchdogThread = nullptr;
};
