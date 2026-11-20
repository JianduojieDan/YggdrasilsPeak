// The robot player (2026-11-19): plays a chapter by itself in a standalone -game run, so a whole chapter can be checked
// unattended (story flow, stuck spots, the camera under abuse, FPS). Tools/Bot/run_bot.ps1 launches it.
//   ygg.Bot.Run chapter1 [fast]: starts the story (ygg.Story.Start Chapter1 with ygg.Story.Auto 1, so choices, cards,
//   inspects, press prompts and hand-holds answer themselves) and, during the free-walk stretches, walks the climber with
//   AddMovementInput to the next story place it needs: the first beat (file order) of Content/Story/<Chapter>.ygs that has
//   not fired, whose "after" beats are done and which waits for "near" a place or a person. A straight line first; once
//   that stops making progress, Tools/RoutePlanning/route.json leg 0 as the path. Looks at a beat's "look" target when the
//   beat is otherwise ready. Camera stress the rest of the time: fast yaw sweeps, straight down / straight up, small
//   random jitter (AddYawInput / AddPitchInput only: the view rig stays the one writer of the view, spec R1).
//   Stuck: no progress for ygg.Bot.StuckSec (25 s) -> logged, teleported 15 m ahead along the route. A beat that has been
//   the target for ygg.Bot.BeatCapMin (6 min) -> teleported to its place, 90 s later skipped.
//   Story progress comes from the story's own log lines ("Story: beat <id>", "... done", "Story: <chapter> finished"),
//   read by a log output device the bot registers (the story has no public getter for its beats).
//   Writes Saved/Bot/<yyyyMMdd_HHmmss>/report.md (rewritten every 60 s, final at the end, then done.txt): the beat
//   timeline (time, place, player position, FPS), stuck events, teleports, FPS summary, the camera stress done, the view
//   rig's audit, the CamRec incident count (Saved/Monitor/incidents/<session>, when that recorder exists), story warnings.
//   Then "quit" (ygg.Bot.Quit 1), at the chapter end or after ygg.Bot.CapMin (75) minutes of game time.
//   "fast": t.MaxFPS 0 and the walk at sprint.
// Console: ygg.Bot.Run [chapter1] [fast], ygg.Bot.Stop, ygg.Bot.Status; cvars ygg.Bot.* (see the .cpp).

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "YggPlayBot.generated.h"

class APawn;
class APlayerController;
class AYggStoryNPC;
class FYggBotLog;

UCLASS()
class YGGDRASILSPEAK_API UYggPlayBot : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Deinitialize() override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UYggPlayBot, STATGROUP_Tickables); }

	/** ygg.Bot.Run: Chapter as typed ("chapter1" -> Chapter1). */
	void Run(const FString& InChapter, bool bInFast);
	/** ygg.Bot.Stop (or the world going away): final report, no quit. */
	void StopRun(const FString& Why);
	bool IsBotRunning() const { return Phase != EPhase::Idle; }
	/** One line: phase, target, game time, beats fired (ygg.Bot.Status). */
	FString Status() const;
	/** The look the bot fed (deg/s) when it turned the view faster than 20 deg/s within the last HoldS real seconds; false
	 *  otherwise. The camera recorder (UYggCameraRecorder) counts it as input, so the bot's sweeps are not "spins". */
	bool GetLookFeed(double HoldS, float& OutYawRate, float& OutPitchRate) const;

private:
	enum class EPhase : uint8 { Idle, Waiting, Playing, Quitting };
	enum class ELook : uint8 { Follow, Sweep, Down, Up, Back };

	/** One beat of the chapter file, as far as the bot needs it, and what happened to it. */
	struct FBotBeat
	{
		FString Id;
		TArray<FString> After;       // "after" (and the "after" of each "any" sub-condition, for the dependency table)
		TArray<FString> TopAfter;    // only the beat's own "after": what must be done before it can fire
		TArray<FString> Flag, NoFlag;
		FString Near;                // a place or a person ("near", or the first "near" of an "any")
		float R = 600.f;
		FString Look;
		FString Posture;
		bool bNever = false;
		// what happened
		bool bFired = false, bDone = false, bSkipped = false, bForcedLook = false, bCapTeleported = false;
		double FiredGame = 0.0, FiredReal = 0.0, DoneGame = 0.0;
		FVector FiredPos = FVector::ZeroVector;
		FString FiredPlace;
		float FiredDist = -1.f;
		float SegFps = 0.f;
		float TargetTime = 0.f;      // seconds as the walk target (not counting the story holding the climber)
		float PostureWait = 0.f;     // seconds in walk while the beat wants a posture the bot cannot take
		float StandWait = 0.f;       // seconds standing still for it (arrived, or posed out of reach) without it firing
		FString SkipWhy;
	};

	struct FBotEvent { double Game = 0.0; FString Text; };
	struct FBotStuck { double Game = 0.0; FString Beat; FVector Pos = FVector::ZeroVector; float Dist = 0.f; FString Action; };
	struct FBotHitch { double Game = 0.0; float Ms = 0.f; FVector Pos = FVector::ZeroVector; FString Beat; };

	EPhase Phase = EPhase::Idle;
	FString Chapter;
	bool bFast = false;
	int32 Seed = 0;
	FRandomStream Rng;
	FString Dir;
	FString EndWhy;
	FDateTime StartUtc;
	double RealStart = 0.0;
	double GameT = 0.0;
	double LastReport = 0.0;
	double LastBeatGame = 0.0;
	float QuitLeft = 0.f;
	bool bStoryStarted = false;
	bool bStoryWasRunning = false;
	bool bChapterFinished = false;
	/** Game time the story stopped running (its "finished" log line may come a moment later); < 0 = running. */
	double StoryStopAt = -1.0;
	bool bSawDone = false;
	FString PrevMaxFps;
	bool bMaxFpsSet = false;

	// the chapter
	TArray<FBotBeat> Beats;
	TArray<TArray<bool>> Dep;        // Dep[a][b]: beat a waits (directly or not) for beat b
	TMap<FName, FVector> Places;
	TArray<FVector> Route;           // route.json leg 0
	TArray<float> RouteCum;          // distance along the route to each point (cm, 2D)
	TMap<FName, TWeakObjectPtr<AYggStoryNPC>> People;
	double PeopleScan = -100.0;

	// the walk
	int32 NavBeat = INDEX_NONE;
	int32 AimBeat = INDEX_NONE;
	FVector NavPos = FVector::ZeroVector;
	FVector AimPos = FVector::ZeroVector;
	FVector MoveDir = FVector::ZeroVector;
	/** The posture the walk target waits for (EYggView as a byte, 255 = none): Drive stops the moment the rig is in it. */
	uint8 NavWantPose = 255;
	bool bRouteMode = false;
	bool bHeld = false;
	bool bArrived = false;
	float Best = 0.f;
	double LastProgress = 0.0;
	double NextDecide = 0.0;
	double NextStuckCheck = 0.0;
	double NextSprint = 0.0;
	FVector LastPos = FVector::ZeroVector;
	bool bHaveLastPos = false;
	// where a story cut landed the climber: 3 s later the report says if he is no longer there (the physics pushed him)
	FVector CutPos = FVector::ZeroVector;
	double CutT = -1.0;
	double LastBotTeleport = -100.0;
	int32 Teleports = 0, CapTeleports = 0, PutBacks = 0, Jumps = 0;

	// the look
	ELook Look = ELook::Follow;
	double NextSweep = 0.0, NextDownUp = 0.0;
	float SweepLeft = 0.f, SweepRate = 0.f, HoldLeft = 0.f, StallFor = 0.f, PrevPitch = 0.f;
	FVector2D Jitter = FVector2D::ZeroVector, JitterGoal = FVector2D::ZeroVector;
	double NextJitter = 0.0, JitterWindowEnd = 0.0;
	bool bJitterOn = true;
	int32 Sweeps = 0, DownUps = 0, AimSeconds = 0;
	float AimAccum = 0.f;
	float MinPitchSeen = 0.f, MaxPitchSeen = 0.f;
	float FeedYaw = 0.f, FeedPitch = 0.f;   // the last look fed faster than 20 deg/s (deg/s) and when (real seconds)
	double FeedAt = -100.0;

	// the view rig (ygg.Bot.Rig 1)
	uint8 RigView = 0;
	float RigLid = 0.f;
	float EyeMin = 1e6f, EyeMax = -1e6f;
	float PosedSeconds = 0.f, LidShutSeconds = 0.f;
	FString LastAudit;

	// frames
	TArray<int32> Hist;              // 0.25 ms buckets up to 1 s
	int64 Frames = 0;
	double FrameSum = 0.0;
	float WorstMs = 0.f;
	int32 Over50 = 0, Over100 = 0, Over250 = 0;
	int64 SegFrames = 0;
	double SegTime = 0.0;
	TArray<FBotHitch> Hitches;       // the 12 worst

	// the record
	TArray<FBotEvent> Events;
	TArray<FBotStuck> Stucks;
	TMap<FString, int32> StoryWarnings;
	int32 StoryWarningCount = 0;
	int32 CamRecLogIncidents = 0;    // "CamRec: INCIDENT ..." log lines seen during the run
	TArray<FString> CamRecLines;     // the first 30 of them
	TSharedPtr<FYggBotLog> LogDevice;

	// setup
	bool LoadChapter();
	void LoadRoute();
	void BeginStory();
	void Finish(const FString& Why, bool bAllowQuit = true);
	void Note(const FString& Text);

	// every frame
	void DrainLog();
	void OnStoryLine(const FString& Line, bool bWarning);
	void RecordFrame(float RealDt, APawn* P);
	void ReadRig(APawn* P, float Dt);
	void Decide(APawn* P);
	void CheckStuck(APawn* P);
	void Drive(APawn* P);
	void LookAround(APlayerController* PC, float RealDt);

	// helpers
	int32 FindBeat(const FString& Id) const;
	bool AfterOk(const FBotBeat& B) const;
	bool Superseded(int32 Index) const;
	bool Resolve(const FString& Name, FVector& Out);
	void ScanPeople();
	int32 NearestRoute(const FVector& At) const;
	float RouteRemaining(const FVector& From, const FVector& To) const;
	FVector RouteAhead(const FVector& From, const FVector& To, float Dist, bool& bOk) const;
	bool TeleportTo(APawn* P, const FVector& Where, const FString& Why);
	FString NearestPlace(const FVector& At, float& OutDist) const;
	float Percentile(double Frac) const;
	FString BuildReport(bool bFinal) const;
	void WriteReport(bool bFinal);
	int32 CountIncidents(FString& OutWhere) const;
	FString BeatName(int32 Index) const { return Beats.IsValidIndex(Index) ? Beats[Index].Id : FString(TEXT("-")); }
};
