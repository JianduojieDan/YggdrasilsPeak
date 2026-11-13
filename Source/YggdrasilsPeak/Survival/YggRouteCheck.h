// The route check (2026-11-14, overnight plan: "no walking for route checks"). Walking the 28 km route would take
// hours, so the game jumps along it instead: every SpacingM metres it puts the climber on the route, waits for the
// streaming to settle and checks everything there - the ground (holes, missing or different collision, too steep to
// stand), every object within 40 m (floating, buried, props a climber would walk through), whether a fixed rope, flag,
// clue or checkpoint shows the way ahead - and takes a picture along the route (and optionally one back).
// Tools/Route/route_check.ps1 runs it in the standalone game; Tools/Route/analyze_route_check.py writes the report.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tickable.h"
#include "Async/Future.h"
#include "Engine/HitResult.h"
#include "Tasks/Pipe.h"
#include "UObject/ObjectKey.h"
#include "YggRouteCheck.generated.h"

class AActor;
class ACharacter;
class UPrimitiveComponent;
class UStaticMeshComponent;
class UInstancedStaticMeshComponent;
class UWorld;
class IImageWrapperModule;

/** One place to check: a point on the route polyline. */
struct FYggRouteCheckPoint
{
	int32 Leg = 0;
	double LegDistM = 0.0;
	double RunDistM = 0.0;
	/** On the route polyline; Z = the ground when the route was planned (Tools/RoutePlanning/plan_route.py). */
	FVector Pos = FVector::ZeroVector;
	/** Along the route (towards the polyline 30 m ahead) and the route's climb over those 30 m. */
	float Yaw = 0.f;
	float GradeDeg = 0.f;
	/** The 5 m samples of the stretch since the previous point (holes between points are found there). */
	int32 SubFirst = 0;
	int32 SubNum = 0;
};

/** Tools/RoutePlanning/route.json cut into points (read and built on a worker thread). */
struct FYggRouteCheckPlan
{
	TArray<FYggRouteCheckPoint> Points;
	TArray<FVector> Sub;
	TMap<int32, FString> LegNames;
	FString Error;
};

/** A plain static mesh near the route, found once at the start (instanced meshes are asked at each point). */
struct FYggRouteCheckStatic
{
	TWeakObjectPtr<UStaticMeshComponent> Comp;
	FBox Box = FBox(ForceInit);
	uint8 Kind = 0;
};

/** One object to measure at the current point. */
struct FYggRouteCheckJob
{
	/** Null for a tree (the forest is instanced skeletal meshes without collision: measured at its root). */
	TWeakObjectPtr<UStaticMeshComponent> Comp;
	int32 Instance = INDEX_NONE;
	FBox Box = FBox(ForceInit);
	uint8 Kind = 0;
	float DistM = 0.f;
	int32 Tree = INDEX_NONE;
	/** Its entry in UYggRouteCheck::Measured (taken out again when MaxObjects leaves it unmeasured). */
	uint64 Key = 0;
};

/** Every surface straight down at one spot: the mountain's own (detail tile, landscape) and what overhangs it. */
struct FYggRouteCheckColumn
{
	bool bTerrain = false;
	bool bTile = false;
	bool bLand = false;
	double TerrainZ = 0.0;
	double TileZ = 0.0;
	double LandZ = 0.0;
	FString Overhead;
};

/** The steps of one point (and the run around them). */
enum class EYggRouteCheckStep : uint8 { Idle, Loading, Index, WaitPlay, Move, Settle, Measure, Objects, Shot, TurnBack, ShotBack, Next, Quit };

/**
 *  ygg.RouteCheck [SpacingM=200] [FirstLeg=0] [LastLeg=all] [Shots=1]   (or Spacing=, First=, Last=, Shots= in any order)
 *  ygg.RouteCheck stop
 *  Writes Saved/RouteCheck/<yyyyMMdd_HHmmss>/:
 *    points.csv   one row per point: the ground (what, height, slope, walkable, capsule fits, player vs visible
 *                 collision, tile vs landscape, holes in the stretch since the last point), guidance ahead, object
 *                 counts, frame time, the picture names and a BugItGo to stand there
 *    objects.csv  every static / instanced mesh (and tree) within ygg.RouteCheck.Radius: its lowest contact vs what is
 *                 under it (floating), how much of it is under the ground (buried), whether a climber walks through it
 *    shots/       NNN_leg<L>_<dist>m.jpg along the route (and _back.jpg with Shots=2)
 *    log.txt, and done.txt when finished (ygg.RouteCheck.QuitWhenDone 1 then quits the game)
 *  The climber flies with collision off and the body simulation stopped, so nothing can hurt or kill it; all work is
 *  spread over frames and every file is written by a worker (a task pipe), never by the game thread.
 */
UCLASS()
class YGGDRASILSPEAK_API UYggRouteCheck : public UGameInstanceSubsystem, public FTickableGameObject
{
	GENERATED_BODY()

public:
	virtual void Deinitialize() override;

	// FTickableGameObject: the state machine, one step per frame (paused or not)
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual bool IsTickable() const override { return Step != EYggRouteCheckStep::Idle; }
	virtual bool IsTickableWhenPaused() const override { return true; }
	virtual bool IsTickableInEditor() const override { return false; }
	virtual ETickableTickType GetTickableTickType() const override { return ETickableTickType::Conditional; }

	void Begin(const TArray<FString>& Args, UWorld* World);
	void Stop(const FString& Why);
	bool IsRunning() const { return Step != EYggRouteCheckStep::Idle; }

private:
	void SetStep(EYggRouteCheckStep NewStep);
	void Finish(const FString& Status);
	void Write(const TCHAR* Name, const FString& Text, bool bReplace);
	void Log(const FString& Line);
	ACharacter* GetClimber(UWorld* World) const;
	void TakePlayer(ACharacter* Me);
	void ReleasePlayer();
	void IndexActor(AActor* A);
	bool IsTerrainCached(const FHitResult& Hit);
	bool TileCovers(double X, double Y) const;
	bool FindStand(UWorld* World, double X, double Y, double ZGuess, const AActor* Me, FHitResult& OutHit, FYggRouteCheckColumn& OutColumn);
	void PlaceClimber(UWorld* World, ACharacter* Me);
	void MeasurePoint(UWorld* World, ACharacter* Me);
	void GatherObjects();
	void MeasureObject(UWorld* World, const AActor* Me, const FYggRouteCheckJob& Job);
	void MeasureTree(UWorld* World, const AActor* Me, const FYggRouteCheckJob& Job);
	void WritePoint();
	void Look(ACharacter* Me, bool bBack) const;
	void RequestShot(const FString& RelPath);
	void OnShotCaptured(int32 ShotW, int32 ShotH, const TArray<FColor>& Pixels);
	void ClearShot();

	EYggRouteCheckStep Step = EYggRouteCheckStep::Idle;
	double StepStart = 0.0;
	bool bEntered = false;
	TWeakObjectPtr<UWorld> WorldRef;
	FString Dir;
	TUniquePtr<UE::Tasks::FPipe> Pipe;
	IImageWrapperModule* ImageWrappers = nullptr;
	double RunStart = 0.0;

	// arguments
	float SpacingM = 200.f;
	int32 FirstLeg = 0;
	int32 LastLeg = 999;
	int32 Shots = 1;

	// the route
	TFuture<FYggRouteCheckPlan> PlanFuture;
	FYggRouteCheckPlan Plan;
	int32 Cur = 0;

	// what is in the level (built once, a few hundred actors a frame)
	TArray<TWeakObjectPtr<AActor>> ToIndex;
	int32 IndexNext = 0;
	TArray<FYggRouteCheckStatic> Statics;
	TMap<FIntPoint, TArray<int32>> StaticGrid;
	TArray<TWeakObjectPtr<UInstancedStaticMeshComponent>> Instanced;
	TArray<uint8> InstancedKind;
	TArray<FBox> TileBoxes;
	TArray<FVector> RopePts, FlagPts, CluePts, CheckpointPts, TreePts;
	TArray<int32> TreeSet;
	TArray<FString> TreeNames;
	int32 SkippedHuge = 0;
	TSet<uint64> Measured;
	TMap<TObjectKey<UPrimitiveComponent>, bool> TerrainOf;
	TArray<FHitResult> Scratch;

	// the current point
	FVector StandPoint = FVector::ZeroVector;
	FString PointCols;
	FString PointBugIt;
	bool bPointIssue = false;
	TArray<FYggRouteCheckJob> Jobs;
	int32 JobNext = 0;
	FString ObjRows;
	int32 PtObjects = 0, PtFloating = 0, PtBuried = 0, PtWalkInto = 0;
	float FrameSum = 0.f, FrameMax = 0.f;
	int32 FrameCount = 0;
	int32 StreamingLeft = 0;
	double SettleSeconds = 0.0;
	FString ShotName, BackName;

	// totals
	int32 TotPoints = 0, TotIssuePoints = 0, TotObjects = 0, TotFloating = 0, TotBuried = 0, TotWalkInto = 0, TotShots = 0;

	// the picture being taken
	FString PendingShot;
	FDelegateHandle ShotHandle;
	bool bShotDone = false;

	// what was changed on the climber (put back when the game is not quit afterwards)
	bool bChangedPlayer = false;
	bool bSurvivalTickWas = true;
	TArray<float> HeldWas;
	/** Actors attached to the climber (child actors keep their own collision when the climber's is off): every trace ignores them. */
	TArray<TWeakObjectPtr<const AActor>> ClimberParts;
};
