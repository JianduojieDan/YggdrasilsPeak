// The camera flight recorder (2026-11-19). The user's view sometimes spins 360 degrees vertically at the Dingboche camp;
// we must SEE what causes such things. Every frame it keeps the last few seconds of everything that can move the view
// (camera POV, control rotation, the raw mouse, the look-input flags, the pawn, the view target, the camera modifiers,
// the view rig's state) and, when something looks wrong, writes that buffer to disk with a screenshot and a guess at
// the culprit (which modifier / view target / rig state changed in the frames before).
//
//   Saved/Monitor/incidents/<session>/<n>_<kind>/frames.csv   the buffer (6 s before the incident, 0.5 s after)
//                                               /summary.txt  kind, values, likely culprit, changes, log lines, rig audits
//                                               /shot.jpg     the screen at the moment it was detected
//   Saved/Monitor/incidents/<session>/incidents.md            running index, one line per incident
//   log: "CamRec: INCIDENT <kind> ..." (Warning, so the session monitor's log.txt has it too)
//
// Incidents (ygg.CamRec.* cvars): the view turns faster than RateDeg while the mouse / stick is still (spin_pitch,
// spin_yaw), the pitch wraps across +-90 (pitch_wrap), |roll| > RollDeg outside a scripted fall / knock (roll), the eye is
// LowCm below normal while the pawn walks (low_eye), the view target is not the pawn for ViewTargetS (view_target), a
// frame over HitchMs (hitch). At most one per Cooldown seconds, Max per session.
//
// Console: ygg.CamRec 1/0 (-1 = auto: on in -game, off in the editor / -YggNoSave runs unless -YggCamRec),
//          ygg.CamRec.Dump (write the buffer now), ygg.CamRec.Test [pitch|yaw] [delay s] (fakes a 360 spin through
//          the control rotation for 1 s to prove the detection works).
// Cost: one struct copy into a ring per frame plus a short look back over the last 0.15 s; files are written on a
// worker thread.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "YggCameraRecorder.generated.h"

class APawn;
class APlayerController;
class UActorComponent;
class FYggCamRecInput;
class FYggCamRecLog;
class IImageWrapperModule;

/** One recorded frame (plain data: copied to a worker thread when an incident is written). */
struct FYggCamRecFrame
{
	static constexpr int32 MaxMods = 6;

	uint64 Seq = 0;
	double T = 0.0;                 // real seconds since the recorder started
	float Ms = 0.f;                 // real frame time
	float GameDt = 0.f;             // the world's delta (dilated)
	FVector CamLoc = FVector::ZeroVector;
	float CamPitch = 0.f, CamYaw = 0.f, CamRoll = 0.f;          // POV (camera manager cache, this frame)
	float CtrlPitch = 0.f, CtrlYaw = 0.f, CtrlRoll = 0.f;       // control rotation, normalised to -180..180
	float DCamPitch = 0.f, DCamYaw = 0.f, DCtrlPitch = 0.f, DCtrlYaw = 0.f;   // change since the last frame (0 on a cut)
	float Fov = 0.f;
	float MouseX = 0.f, MouseY = 0.f;                           // PlayerController::GetInputMouseDelta
	float RawX = 0.f, RawY = 0.f;                               // Slate mouse move this frame (pixels / raw counts)
	float StickX = 0.f, StickY = 0.f;                           // right stick
	float BotYaw = 0.f, BotPitch = 0.f;                         // look the robot player (ygg.Bot) fed, deg/s (0 = none)
	FVector PawnLoc = FVector::ZeroVector;
	FVector3f PawnVel = FVector3f::ZeroVector;
	float SpeedH = 0.f;
	uint8 MoveMode = 0, CustomMode = 0;
	FName ViewTarget;
	FName PawnName;
	float EyeMeasured = -1.f;       // camera Z minus the floor under the capsule (-1 = no floor)
	// the view rig (-1 = no rig / unknown)
	int8 Posture = -1;
	int8 Scripted = -1;             // 0 none, 1 fall, 2 knock, 3 fly
	int8 Nudging = -1;
	float RigEye = -1.f, RigLid = -1.f;
	int64 Leases = -1;
	int32 AuditId = -1;             // index into the recorder's audit history (changes only)
	// camera modifiers (first MaxMods)
	uint8 NumMods = 0;
	uint8 ModOffMask = 0;
	FName ModName[MaxMods];
	uint8 ModAlpha[MaxMods] = {};   // alpha * 255
	// flags
	bool bStill = false;            // no mouse, raw mouse, stick or robot-player look input this frame
	bool bBot = false;              // the robot player turned the view within the last 0.15 s (its input, not a spin)
	bool bLookIgnored = false, bMoveIgnored = false, bCursor = false;
	bool bCutPcm = false, bCutRig = false, bCut = false;        // bCut = deltas not measured this frame (cut, new pawn, resume)
	bool bVtIsPawn = true, bPawnHidden = false;
	bool bTest = false;             // ygg.CamRec.Test was writing the control rotation
	uint8 IncidentKind = 0;         // set on the frame an incident was detected (index into the kind names)
};

UCLASS()
class YGGDRASILSPEAK_API UYggCameraRecorder : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	/** The recorder of Ctx's world (null outside game / PIE worlds). */
	static UYggCameraRecorder* Get(const UObject* Ctx);

	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Tick(float DeltaTime) override;
	virtual bool IsTickable() const override { return true; }
	/** Pending incidents, the log queue and the screenshot timeout keep going under a pause menu. */
	virtual bool IsTickableWhenPaused() const override { return true; }
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UYggCameraRecorder, STATGROUP_Tickables); }

	/** ygg.CamRec resolved (-1 = auto). */
	static bool IsEnabled();
	/** ygg.CamRec.Dump: writes the buffer now as an incident of kind "dump" (no rate limit, not counted against Max). */
	void Dump(const FString& Why);
	/** ygg.CamRec.Test: from Delay seconds on (and never inside the warm-up), turns the control rotation 360 degrees about
	 *  pitch (or yaw) over 1 s, then puts it back. Frames carry test=1 and the summary names the test as the culprit. */
	void StartTest(bool bYaw, float Delay);

private:
	struct FPending
	{
		int32 N = 0;
		FString Kind, Values, Also, Dir;
		uint64 Seq = 0;
		double DetectT = 0.0, WriteAt = 0.0;
		int32 Suppressed = 0;
	};

	void OnPostActorTick(UWorld* InWorld, ELevelTick TickType, float DeltaSeconds);
	void Sample(float GameDt);
	void Detect(FYggCamRecFrame& F);
	void Fire(FYggCamRecFrame& F, uint8 Kind, const FString& Values, const FString& Also);
	void Queue(const FString& Kind, const FString& Values, const FString& Also, uint64 Seq, double T, float PostS);
	void WritePending(const FPending& P, bool bSync = false);
	void RequestShot(const FString& Path);
	void OnShotCaptured(int32 W, int32 H, const TArray<FColor>& Px);
	void DrainLog();
	UActorComponent* FindRig(APawn* Pawn);
	const FYggCamRecFrame* Back(int32 K) const;   // 0 = newest
	void ResetRing();

	// ring of frames
	TArray<FYggCamRecFrame> Ring;
	int32 Head = 0, Count = 0;
	uint64 NextSeq = 1;
	double StartReal = 0.0, LastReal = 0.0, BeginPlayReal = -1.0;
	bool bWasOn = false;
	bool bHavePrev = false;

	// episode state for the sustained checks
	double LowSince = -1.0, VtSince = -1.0, RollSince = -1.0;
	bool bLowFired = false, bVtFired = false, bRollFired = false;
	double LastIncidentT = -1000.0;
	double ScriptedT = -1000.0;     // last frame the rig was in a scripted fall / knock / fly-out (its roll fades out after)
	TMap<FString, int32> SuppressedKinds;

	// the view rig
	TWeakObjectPtr<UActorComponent> Rig;
	TWeakObjectPtr<APawn> RigPawn;
	double NextRigFind = 0.0, NextAudit = 0.0;
	FString LastAudit;
	TArray<TPair<double, FString>> Audits;   // (time, text) of each change; index = AuditBase + i
	int32 AuditBase = 0;

	// log lines of the game's own categories and every warning (time, line), the last ~8 s
	TArray<TPair<double, FString>> RecentLog;
	TSharedPtr<FYggCamRecLog> LogDevice;
	TSharedPtr<FYggCamRecInput> Input;

	// test injection
	bool bTestArmed = false, bTestRunning = false, bTestYaw = false;
	double TestAt = 0.0, TestStart = 0.0;
	FRotator TestFrom = FRotator::ZeroRotator;

	// incidents waiting for their after-frames, and the screenshot in flight
	TArray<FPending> Pending;
	FDelegateHandle ShotHandle;
	FString ShotPath;
	double ShotAt = 0.0;
	IImageWrapperModule* ImageWrappers = nullptr;
	FDelegateHandle PostTickHandle;
};
