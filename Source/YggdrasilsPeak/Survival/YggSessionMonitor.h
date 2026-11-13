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

/**
 *  Writes Saved/Monitor/<date_time>/:
 *    session.txt  hardware, command line, every setting at the start (and later changes in events.csv)
 *    frames.csv   one row per frame: time, frame/game/render/RHI/GPU ms, presented FPS (DLSS frame generation), draw
 *                 calls, triangles, camera place and direction, turning speed, mouse movement, loading/streaming, flags
 *    seconds.csv  one row per second: FPS (avg, min, 1% low), the body (SpO2, heart, core, stamina, health...), what the
 *                 climber does, realm, time of day, air, memory, keys pressed, paused or in a menu
 *    events.csv   keys down/up, hitches (with the log lines around them), expedition messages, realm and settings
 *                 changes, level loads, screenshots
 *    log.txt      the engine's log lines worth reading (warnings, errors, rendering/streaming/shader/audio lines),
 *                 repeats folded into counts
 *    shots/       a screenshot every 60 s and at big hitches
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
	void OnLogLine(const FString& Line);

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
	FString LastLogLine;
	int32 LastLogRepeats = 0;
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

	TSharedPtr<FYggMonitorInput> Input;
	TSharedPtr<FYggMonitorLog> LogDevice;      // shared: its deleter is made in the .cpp where the type is complete
};
