// The robot player's death-zone run (2026-11-20, Development_Plan A5 "South Col -> Summit"): the standalone game climbs
// route leg 7 by itself and measures it, so the whole stretch can be checked and timed unattended.
//   ygg.Bot.Summit [fast] [noo2] [stay <min>] [down <m>] [cap <min>]
//   - jumps to the South Col camp (UExpeditionComponent::TeleportToCamp, what ygg.Expedition.Go does), waits for the
//     ground and the streaming, then walks Tools/RoutePlanning/route.json leg 7 (South Col -> Summit) with
//     AddMovementInput: pure pursuit 7 m ahead along the polyline, which is where the fixed rope lies.
//   - oxygen: by default one bottle is put on at the start and one spare carried (a player who took the South Col
//     bottles); "noo2" takes every bottle away (and keeps them away: the camp's pickups would hand them back).
//   - "fast": sprints whenever the body lets it (stamina), t.MaxFPS 0. Without it: a plain walk.
//   - at the summit: waits for the summit beat (Content/Story/Summit.ygs) and the summit screen, takes the pictures,
//     closes the screen ("Stay at the summit"), looks at the World Tree. Then "stay <min>" stands there (the no-oxygen
//     death test) or, by default, walks back down leg 7 for "down" metres (300).
//   Checks while it walks (every 0.25 s): the distance off the route, the fixed rope (UExpeditionComponent::IsOnRope and
//   the distance to the nearest rope), falling / sliding (MOVE_Falling), the floor gap, under the ground (a trace from
//   above finds ground over the capsule), stuck (no progress: sidestep and jump, then a 10 m teleport along the route,
//   each one reported). The body: SpO2, heart rate, health, stamina, core, bottle, the body effects on the view, and the
//   death-zone message (read from the log). ygg.GroundProbe every 100 m of route (Saved/Review/ground_probe.csv).
//   Writes Saved/Bot/summit_<yyyyMMdd_HHmmss>/report.md (progress.md every 60 s, done.txt at the end), samples.csv (one
//   row per second) and pictures to Saved/Review/A5/<run>_<stage>.png; quits when done (ygg.Bot.Quit).
// Tools/Bot/run_bot.ps1 -Chapter summit [-Fast] [-Extra ...] [-SummitArgs "noo2 stay 10"] runs it.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "YggSummitBot.generated.h"

class AActor;
class APawn;
class APlayerController;
class FYggSummitBotLog;

UCLASS()
class YGGDRASILSPEAK_API UYggSummitBot : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Deinitialize() override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UYggSummitBot, STATGROUP_Tickables); }

	void Run(const TArray<FString>& Args);
	void StopRun(const FString& Why);
	bool IsBotRunning() const { return Phase != EPhase::Idle; }

private:
	enum class EPhase : uint8 { Idle, Waiting, Settle, Up, Summit, Stay, Down, Quitting };

	struct FSample
	{
		double T = 0.0, Real = 0.0;
		uint8 Phase = 0;
		float S = 0.f, Lat = 0.f, Elev = 0.f, Speed = 0.f, MaxSpeed = 0.f, FloorDist = 0.f, RopeDist = 0.f;
		FVector Pos = FVector::ZeroVector;
		bool bFalling = false, bWalkable = false, bRope = false, bSheltered = false, bSprint = false;
		float SpO2 = 0.f, Heart = 0.f, Health = 0.f, Stamina = 0.f, Core = 0.f, Bottle = 0.f, Storm = 0.f;
		int32 Bottles = 0;
		float Hyp = 0.f, Frost = 0.f, Exh = 0.f, Faint = 0.f;
	};
	struct FIssue { double T = 0.0; float S = 0.f; float Elev = 0.f; FVector Pos = FVector::ZeroVector; FString Kind, Text; };
	struct FShot { FString Stage, File; double T = 0.0; float Elev = 0.f; };

	EPhase Phase = EPhase::Idle;
	// options
	bool bFast = false, bNoO2 = false;
	float StayMin = 0.f, DownM = 300.f, CapMin = 60.f;
	/** Testing: start the climb this far along leg 7 (m) by a teleport after the South Col (0 = from the camp). */
	float FromM = 0.f;
	// run
	FString RunName, Dir, ShotDir, EndWhy;
	double GameT = 0.0, RealStart = 0.0, LastReport = 0.0, PhaseT = 0.0;
	double UpStartT = -1.0, UpStartReal = 0.0, SummitT = -1.0, SummitReal = 0.0, MenuT = -1.0, DownStartT = -1.0, DownEndT = -1.0;
	double DeathT = -1.0, DeathZoneMsgT = -1.0, StayStartT = -1.0;
	float DeathZoneMsgElev = 0.f, DeathElev = 0.f, DownStartS = 0.f;
	FString DeathCause, DeathZoneMsg, SouthColName;
	bool bDeathZoneHeld = false;
	int32 PickupsAtCol = 0;
	FString PrevMaxFps;
	bool bMaxFpsSet = false;
	FString TreeInfo;
	FVector TreeAim = FVector::ZeroVector;
	bool bHaveTree = false;
	// the actors that make up the World Tree (the stuck-teleport trace ignores them, 2026-11-20)
	TArray<TWeakObjectPtr<AActor>> TreeActors;
	// the route (leg 7) as a 2D polyline
	TArray<FVector> Route;
	TArray<float> Cum;
	float RouteLen = 0.f;
	int32 Seg = 0;
	float S = 0.f, Lat = 0.f, BestS = 0.f;
	double LastProgressT = 0.0, NextGroundProbeS = 0.f;
	int32 Teleports = 0, Sidesteps = 0, Probes = 0;
	FVector MoveDir = FVector::ZeroVector;
	// ground and rope state
	float FallFor = 0.f, FallStartZ = 0.f, OffRopeFrom = -1.f, LostFor = 0.f, FloatFor = 0.f, UnderFor = 0.f;
	bool bLostNoted = false, bFloatNoted = false, bUnderNoted = false;
	float RopeOnSec = 0.f, RopeOffSec = 0.f, FallSec = 0.f, MaxLat = 0.f, MaxFloor = 0.f;
	int32 Falls = 0;
	TArray<FVector2D> OffRope;      // (from s, to s) in metres, while walking the route
	// body
	float MaxHyp = 0.f, MaxFrost = 0.f, MaxExh = 0.f, MaxFaint = 0.f, MaxHeart = 0.f, MinSpO2 = 100.f, MinHealth = 100.f, MinCore = 40.f;
	double FirstHypT = -1.0, FirstFrostT = -1.0, FirstFaintT = -1.0, FirstDamageT = -1.0, FirstExhT = -1.0;
	float StartHealth = 100.f, StartElev = 0.f, SummitElev = 0.f, StartS = 0.f, SummitS = 0.f;
	// the summit
	bool bSawMoveLease = false, bSawNudge = false, bSawAttention = false, bBeatStarted = false, bBeatDone = false;
	int32 SummitStep = 0;
	double SummitStepT = 0.0;
	FString BeatLog;
	// look
	float WantYaw = 0.f, WantPitch = -6.f;
	bool bLookFree = false;
	// record
	TArray<FSample> Samples;
	TArray<FIssue> Issues;
	TArray<FShot> Shots;
	TArray<FString> Events;
	TArray<FString> ExpeditionLines;
	double NextSample = 0.0, NextCheck = 0.0;
	TArray<float> FrameMs;
	// frames right after a ground probe (it freezes the game 4-10 s): kept apart so they do not pollute the frame statistics (2026-11-20)
	TArray<float> ProbeMs;
	int32 SkipFrameStats = 0;
	TSharedPtr<FYggSummitBotLog> LogDevice;
	FDelegateHandle ShotHandle;
	FString PendingShot;
	bool bShotPending = false;
	double ShotAt = 0.0;   // real time the pending picture was asked for (2026-11-20)

	void Note(const FString& Text);
	void Issue(const FString& Kind, const FString& Text, APawn* P);
	bool LoadRoute();
	void Locate(const FVector& Pos, bool bDown);
	FVector PointAt(float AtS) const;
	void Begin(APawn* P);
	void Walk(APawn* P, float Dt, bool bDown);
	void Check(APawn* P, float Dt, bool bWalking);
	void Sample(APawn* P);
	void SummitTick(APawn* P, APlayerController* PC, float Dt);
	void Look(APlayerController* PC, float Dt);
	bool TeleportAlong(APawn* P, float ToS, const FString& Why);
	void Shot(const FString& Stage, bool bUI);
	void OnShot(int32 W, int32 H, const TArray<FColor>& Pixels);
	void FindTree();
	void DrainLog();
	void Finish(const FString& Why);
	FString BuildReport(bool bFinal) const;
	void WriteFiles(bool bFinal);
};
