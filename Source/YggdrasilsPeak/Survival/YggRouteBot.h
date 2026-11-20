// The robot player's route walk (2026-11-20, Development_Plan A4 "Demo route 1: Dingboche -> Base Camp -> Icefall ->
// Camp 1"): the standalone game walks route.json legs by itself and measures the way, so a whole stretch can be checked and
// timed unattended. The summit run (YggSummitBot) is the model; this one walks any run of legs and looks at the ground and
// the guidance instead of the body.
//   ygg.Bot.Route [from <leg>] [to <leg>] [fast] [speed <x>] [at <m>] [cap <min>] [every <m>] [time <hhmm>|-1] [noshots]
//   - jumps to the camp the first leg starts at (UExpeditionComponent::TeleportToCamp, camp index = leg index: 0 Dingboche,
//     1 Gorak Shep, 2 Base Camp, 3 Khumbu Icefall, 4 Camp 1), waits for the ground, then walks the legs from..to (default
//     0..3 = Dingboche -> Camp 1) with AddMovementInput: pure pursuit 7 m ahead along the joined polylines.
//   - "fast": sprints whenever stamina allows, t.MaxFPS 0. "speed 3": slomo 3 (game time runs 3x; every time in the report
//     is game time, which is what a player would take). "at 5200": testing a stretch, starts that far along by a teleport.
//   - every "every" metres (200) of route: the ground under the boots by simple (Pawn) and complex (Visibility) traces,
//     ygg.GroundProbe A4_<m> (Saved/Review/ground_probe.csv), a picture Saved/Review/A4/<run>_<m>m.png, the guidance
//     (fixed rope, flags / landmarks, clues, trail checkpoints in the next 100 m; the HUD's next camp against the camp the
//     leg leads to) and the next stretch scanned every 5 m for simple collision standing over the visible surface (an
//     invisible convex hull on a mesh blocked the summit route, 2026-11-20).
//   Checks while it walks (every 0.25 s): off the route (> 8 m for 3 s), falling / sliding over 0.75 s, the floor gap,
//   under the ground, stuck (sidestep + jump after 8 s, a 10 m teleport traced from 150 m above after 30 s; what blocks the
//   capsule is named, with its collision kind), death (the run ends). Food and water are topped up (a player eats).
//   Writes Saved/Bot/route_<yyyyMMdd_HHmmss>/report.md (progress.md every 60 s, done.txt at the end), samples.csv (one row
//   per second) and points.csv (one row per checkpoint); quits when done (ygg.Bot.Route.Quit).
// Tools/Bot/run_bot.ps1 -Chapter route [-RouteArgs "from 2 to 3"] runs it.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "YggRouteBot.generated.h"

class APawn;
class APlayerController;
class FYggRouteBotLog;

UCLASS()
class YGGDRASILSPEAK_API UYggRouteBot : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Deinitialize() override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UYggRouteBot, STATGROUP_Tickables); }

	void Run(const TArray<FString>& Args);
	void StopRun(const FString& Why);

private:
	enum class EPhase : uint8 { Idle, Waiting, Settle, Walk, Arrive, Quitting };

	struct FSample
	{
		double T = 0.0, Real = 0.0;
		int32 Leg = 0;
		float S = 0.f, Lat = 0.f, Elev = 0.f, Speed = 0.f, MaxSpeed = 0.f, FloorDist = 0.f, Fps = 0.f, Stamina = 0.f, SpO2 = 0.f;
		FVector Pos = FVector::ZeroVector;
		bool bFalling = false, bWalkable = false, bRope = false, bSprint = false;
	};
	struct FIssue { double T = 0.0; float S = 0.f; float Elev = 0.f; int32 Leg = 0; FVector Pos = FVector::ZeroVector; FString Kind, Text; };
	struct FShot { FString Stage, File; double T = 0.0; float Elev = 0.f; float S = 0.f; };
	struct FPoint
	{
		float S = 0.f, Elev = 0.f, Lat = 0.f; int32 Leg = 0; double T = 0.0; FVector Pos = FVector::ZeroVector;
		float FeetToSimple = 0.f, FeetToComplex = 0.f; FString GroundActor;
		float RopeM = -1.f, FlagM = -1.f, ClueM = -1.f, CheckM = -1.f; bool bClipped = false;
		FString HudNext, LegTarget; bool bHudAgrees = true;
		int32 SimpleOver = 0; FString SimpleOverWhat;
		float Fps = 0.f; FString Shot;
	};
	struct FCamp { FString Name, Short; FVector Pos = FVector::ZeroVector; };

	EPhase Phase = EPhase::Idle;
	// options
	int32 FromLeg = 0, ToLeg = 3;
	bool bFast = false, bShots = true;
	float SpeedX = 1.f, AtM = 0.f, CapMin = 240.f, EveryM = 200.f, HoldTime = 1100.f;
	// run
	FString RunName, Dir, ShotDir, EndWhy, PrevMaxFps;
	bool bMaxFpsSet = false;
	double GameT = 0.0, RealStart = 0.0, LastReport = 0.0, PhaseT = 0.0, WalkStartT = -1.0, WalkStartReal = 0.0, ArriveT = -1.0, DeathT = -1.0;
	FString DeathCause;
	TArray<FCamp> Camps;
	// the route (legs from..to joined) as a polyline; LegStartS[k] = where leg FromLeg + k begins (cm)
	TArray<FVector> Route;
	TArray<float> Cum;
	TArray<int32> PtLeg;
	TArray<float> LegStartS;
	TArray<double> LegStartT, LegStartReal;
	float RouteLen = 0.f;
	int32 Seg = 0, CurLeg = 0;
	float S = 0.f, Lat = 0.f, BestS = 0.f, NextPointS = 0.f, StartS = 0.f;
	double LastProgressT = 0.0;
	int32 Teleports = 0, Sidesteps = 0, Probes = 0, Falls = 0, Slides = 0, TopUps = 0;
	bool bBlockNoted = false, bOnLadderLine = false;
	int32 Detours = 0, SeracWarnings = 0;
	double SprintUntil = -100.0;
	double DetourUntil = -100.0;
	FVector DetourDir = FVector::ZeroVector;
	TArray<TWeakObjectPtr<AActor>> Crossings;
	FVector MoveDir = FVector::ZeroVector;
	float FallFor = 0.f, FallStartZ = 0.f, LostFor = 0.f, FloatFor = 0.f, UnderFor = 0.f;
	bool bLostNoted = false, bFloatNoted = false, bUnderNoted = false;
	float RopeOnSec = 0.f, RopeOffSec = 0.f, FallSec = 0.f, MaxLat = 0.f, MaxFloor = 0.f;
	FString LastCampSeen;
	// look
	float WantYaw = 0.f, WantPitch = -4.f;
	// record
	TArray<FSample> Samples;
	TArray<FIssue> Issues;
	TArray<FShot> Shots;
	TArray<FPoint> Points;
	TArray<FString> Events, LogLines;
	double NextSample = 0.0, NextCheck = 0.0, NextTopUp = 0.0;
	TArray<float> FrameMs;
	int32 FrameMark = 0;
	TSharedPtr<FYggRouteBotLog> LogDevice;
	FDelegateHandle ShotHandle;
	FString PendingShot;
	bool bShotPending = false;

	void Note(const FString& Text);
	void Issue(const FString& Kind, const FString& Text, APawn* P);
	bool LoadRoute();
	void Locate(const FVector& Pos);
	void Seed(const FVector& Pos);
	FVector PointAt(float AtS) const;
	FVector DirAt(float AtS) const;
	int32 LegAt(float AtS) const;
	void Begin(APawn* P);
	void Walk(APawn* P, float Dt);
	void Check(APawn* P, float Dt);
	void Sample(APawn* P);
	void Checkpoint(APawn* P);
	int32 ScanSimple(float FromS, float ToS, FString& What);
	FString HudNextCamp(APawn* P) const;
	FString Blocker(const struct FHitResult& H) const;
	void Look(APlayerController* PC, float Dt);
	bool TeleportAlong(APawn* P, float ToS, const FString& Why);
	void Shot(const FString& Stage, bool bUI);
	void OnShot(int32 W, int32 H, const TArray<FColor>& Pixels);
	void DrainLog();
	void Finish(const FString& Why);
	FString BuildReport(bool bFinal) const;
	void WriteFiles(bool bFinal);
};
