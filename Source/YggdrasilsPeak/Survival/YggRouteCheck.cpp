#include "Survival/YggRouteCheck.h"
#include "Survival/ClimbingComponent.h"
#include "Survival/OxygenPickup.h"
#include "Survival/SnowTrailComponent.h"
#include "Survival/StoryClue.h"
#include "Survival/SupplyPickup.h"
#include "Survival/SurvivalComponent.h"
#include "Survival/YggTime.h"
#include "Environment/FixedRope.h"
#include "Environment/ForestStreamer.h"
#include "Environment/RouteLandmark.h"
#include "Environment/SnowDriftField.h"
#include "Environment/TrailCheckpoint.h"
#include "Async/Async.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/CapsuleComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "ContentStreaming.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Engine/GameInstance.h"
#include "Engine/GameViewportClient.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "HAL/FileManager.h"
#include "HAL/IConsoleManager.h"
#include "IImageWrapper.h"
#include "IImageWrapperModule.h"
#include "LandscapeProxy.h"
#include "Misc/App.h"
#include "Misc/DateTime.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "UnrealClient.h"
#include "UObject/UObjectGlobals.h"

static TAutoConsoleVariable<int32> CVarRouteCheckQuit(TEXT("ygg.RouteCheck.QuitWhenDone"), 0,
	TEXT("1 = ygg.RouteCheck quits the game when it is done or stopped (the automated run, Tools/Route/route_check.ps1)."));
static TAutoConsoleVariable<float> CVarRouteCheckSettle(TEXT("ygg.RouteCheck.Settle"), 3.f,
	TEXT("Seconds ygg.RouteCheck waits at each point before measuring (streaming); up to 3x longer while textures still stream in."));
static TAutoConsoleVariable<float> CVarRouteCheckFirstWait(TEXT("ygg.RouteCheck.FirstWait"), 15.f,
	TEXT("Seconds after the level appears before ygg.RouteCheck moves to its first point (the arrival fade, the first streaming)."));
static TAutoConsoleVariable<float> CVarRouteCheckRadius(TEXT("ygg.RouteCheck.Radius"), 40.f,
	TEXT("Metres around each point in which ygg.RouteCheck measures every object."));
static TAutoConsoleVariable<int32> CVarRouteCheckMaxObjects(TEXT("ygg.RouteCheck.MaxObjects"), 400,
	TEXT("Most objects ygg.RouteCheck measures at one point, the nearest first."));
static TAutoConsoleVariable<float> CVarRouteCheckTime(TEXT("ygg.RouteCheck.Time"), 1100.f,
	TEXT("Time of day (0..2400) ygg.RouteCheck holds at every point so the pictures compare; below 0 = let the clock run."));

namespace
{
	enum ERouteCheckKind : uint8 { RCK_Prop, RCK_Rock, RCK_Ice, RCK_Tent, RCK_Camp, RCK_Pickup, RCK_Clue, RCK_Checkpoint, RCK_Landmark, RCK_Hanging, RCK_Drift, RCK_Snow, RCK_Tree, RCK_Num };
	const TCHAR* const RouteCheckKindNames[RCK_Num] = { TEXT("prop"), TEXT("rock"), TEXT("ice"), TEXT("tent"), TEXT("camp"), TEXT("pickup"), TEXT("clue"),
		TEXT("checkpoint"), TEXT("landmark"), TEXT("hanging"), TEXT("drift"), TEXT("snow"), TEXT("tree") };

	constexpr double RouteCheckCell = 10000.0;        // the static-mesh grid: 100 m squares
	constexpr double RouteCheckFloatCm = 10.0;        // an object whose closest contact is further than this off what is under it floats
	constexpr double RouteCheckSubStepCm = 500.0;     // the stretch between two points is sampled every 5 m (holes between points)

	// Held at 0 while the check runs, so no frost, thin-air tunnel, swim or blowing snow lies over the pictures
	const TCHAR* const RouteCheckHeldVars[] = { TEXT("ygg.Body.Hypoxia"), TEXT("ygg.Body.Frost"), TEXT("ygg.Body.Exhaust"), TEXT("ygg.Body.Faint"),
		TEXT("ygg.Body.Shiver"), TEXT("ygg.Snow.Spindrift") };

	// Not objects to seat (the same lists as Tools/Placement/ground_lib.py): sky, weather, tools, volumes, hazards' zones,
	// the far terrain, the World Tree and the realms. Ladders over crevasses span air by design.
	const TCHAR* const RouteCheckSkipClass[] = { TEXT("Ultra_Dynamic"), TEXT("CameraActor"), TEXT("CrevasseCrossing"), TEXT("HiddenCrevasse"),
		TEXT("SeracCollapseZone"), TEXT("Landscape"), TEXT("LoadingArrival"), TEXT("NiagaraActor"), TEXT("PlayerStart"), TEXT("BlockingVolume"),
		TEXT("PCGVolume"), TEXT("PCGWorldActor"), TEXT("RealmSky"), TEXT("LightningBolt"), TEXT("ParentsGhosts"), TEXT("YggMirror"),
		TEXT("SkyAtmosphere"), TEXT("SkyLight"), TEXT("HeightFog"), TEXT("VolumetricCloud") };
	// (no "DetailTile" here, unlike ground_lib: the tiles must reach IndexActor's mesh check, which records where they are)
	const TCHAR* const RouteCheckSkipLabel[] = { TEXT("Distant"), TEXT("Far"), TEXT("Horizon"), TEXT("WorldTree"), TEXT("Realm"),
		TEXT("Cube"), TEXT("IceCave"), TEXT("Landscape"), TEXT("GameShotCam") };
	const TCHAR* const RouteCheckSkipFolder[] = { TEXT("Environment/WorldTree"), TEXT("Realms"), TEXT("Distant"), TEXT("Route/IceCave"),
		TEXT("Route/Crevasses"), TEXT("Tools") };

	FString RouteCheckCsv(FString S)
	{
		S.ReplaceInline(TEXT("\""), TEXT("'"));
		S.ReplaceInline(TEXT("\n"), TEXT(" "));
		S.ReplaceInline(TEXT("\r"), TEXT(""));
		return TEXT("\"") + S + TEXT("\"");
	}

	// empty when there is nothing to say (the analyser reads an empty cell as "no value")
	FString RouteCheckF0(bool bHave, double V) { return bHave ? FString::Printf(TEXT("%.0f"), V) : FString(); }
	FString RouteCheckF1(bool bHave, double V) { return bHave ? FString::Printf(TEXT("%.1f"), V) : FString(); }
	FString RouteCheckF2(bool bHave, double V) { return bHave ? FString::Printf(TEXT("%.2f"), V) : FString(); }

	/** The name the level editor shows (labels exist in development builds), else the object name. */
	FString RouteCheckLabel(const AActor* A)
	{
		if (!A)
		{
			return TEXT("-");
		}
#if WITH_EDITOR
		const FString& Label = A->GetActorLabel(false);
		if (!Label.IsEmpty())
		{
			return Label;
		}
#endif
		return A->GetName();
	}

	FString RouteCheckFolder(const AActor* A)
	{
#if WITH_EDITOR
		return A ? A->GetFolderPath().ToString() : FString();
#else
		return FString();
#endif
	}

	/** The actor at the root of what A hangs on (an axe attached to the climber belongs to the climber). */
	const AActor* RouteCheckRoot(const AActor* A)
	{
		while (A && A->GetAttachParentActor())
		{
			A = A->GetAttachParentActor();
		}
		return A;
	}

	/** Horizontal distance from P to the nearest point of B (0 inside it). */
	double RouteCheckDist2D(const FBox& B, const FVector& P)
	{
		const double Dx = FMath::Max3(B.Min.X - P.X, 0.0, P.X - B.Max.X);
		const double Dy = FMath::Max3(B.Min.Y - P.Y, 0.0, P.Y - B.Max.Y);
		return FMath::Sqrt(Dx * Dx + Dy * Dy);
	}

	FString RouteCheckBugItGo(const FVector& Cam, const FVector& LookAt)
	{
		const FRotator R = (LookAt - Cam).Rotation();
		return FString::Printf(TEXT("BugItGo %.0f %.0f %.0f %.1f %.1f 0"), Cam.X, Cam.Y, Cam.Z, R.Pitch, R.Yaw);
	}

	/** Looking a little down the route's own slope, so the ground ahead fills the lower half of the picture. */
	float RouteCheckPitch(float GradeDeg)
	{
		return FMath::Clamp(GradeDeg * 0.6f - 6.f, -30.f, 20.f);
	}

	uint8 RouteCheckKindOf(const AActor* A, const FString& MeshPath, const FString& Label, bool bCampProp)
	{
		if (Cast<ARouteLandmark>(A)) { return RCK_Landmark; }
		if (Cast<AOxygenPickup>(A) || Cast<ASupplyPickup>(A)) { return RCK_Pickup; }
		if (Cast<AStoryClue>(A)) { return RCK_Clue; }
		if (Cast<ATrailCheckpoint>(A)) { return RCK_Checkpoint; }
		const FString S = (MeshPath + TEXT(" ") + Label).ToLower();
		// the drifts have no collision on purpose: low, and waded through like the rest of the soft snow
		if (Cast<ASnowDriftField>(A) || S.Contains(TEXT("drift"))) { return RCK_Drift; }
		// prayer flags, their cords and poles hang in the air by design (Tools/RoutePlanning/seat_camp_props.py skips them too)
		static const TCHAR* const Hanging[] = { TEXT("flag"), TEXT("cord"), TEXT("pole"), TEXT("banner"), TEXT("altartop"), TEXT("prayer") };
		for (const TCHAR* H : Hanging)
		{
			if (S.Contains(H)) { return RCK_Hanging; }
		}
		if (S.Contains(TEXT("tent")) && !S.Contains(TEXT("content"))) { return RCK_Tent; }
		if (bCampProp) { return RCK_Camp; }
		if (S.Contains(TEXT("serac")) || S.Contains(TEXT("ice_cliff")) || S.Contains(TEXT("icecliff")) || S.Contains(TEXT("ice_block"))
			|| S.Contains(TEXT("iceblock")) || S.Contains(TEXT("/glacier/"))) { return RCK_Ice; }
		if (S.Contains(TEXT("snow_pile")) || S.Contains(TEXT("snowpile")) || S.Contains(TEXT("embankment"))) { return RCK_Snow; }
		if (S.Contains(TEXT("rock")) || S.Contains(TEXT("stone")) || S.Contains(TEXT("boulder"))) { return RCK_Rock; }
		return RCK_Prop;
	}

	/** The nearest of Pts that shows the way from From: within 100 m and in front (or within 8 m any way). Metres, or -1. */
	float RouteCheckAhead(const TArray<FVector>& Pts, const FVector& From, const FVector& Fwd, FVector& OutAt)
	{
		float Best = -1.f;
		for (const FVector& P : Pts)
		{
			const FVector D = P - From;
			const double H = D.Size2D();
			if (H > 10000.0 || FMath::Abs(D.Z) > 5000.0)
			{
				continue;
			}
			if (H > 800.0 && D.X * Fwd.X + D.Y * Fwd.Y < 0.2 * H)
			{
				continue;   // behind or off to the side
			}
			if (Best < 0.f || H < Best * 100.0)
			{
				Best = float(H / 100.0);
				OutAt = P;
			}
		}
		return Best;
	}

	/** route.json -> points every SpacingM metres (horizontal) along each chosen leg, plus 5 m samples between them. */
	FYggRouteCheckPlan RouteCheckBuildPlan(const FString& Text, float SpacingM, int32 FirstLeg, int32 LastLeg)
	{
		FYggRouteCheckPlan Plan;
		TArray<TSharedPtr<FJsonValue>> Legs;
		if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Legs) || Legs.Num() == 0)
		{
			Plan.Error = TEXT("route.json is not a list of legs");
			return Plan;
		}
		const double Spacing = FMath::Max(10.0, double(SpacingM)) * 100.0;
		const double SubStep = FMath::Min(RouteCheckSubStepCm, Spacing);
		double Run = 0.0;
		for (const TSharedPtr<FJsonValue>& LegValue : Legs)
		{
			TSharedPtr<FJsonObject> Leg;
			if (LegValue.IsValid() && LegValue->Type == EJson::Object)
			{
				Leg = LegValue->AsObject();
			}
			const TArray<TSharedPtr<FJsonValue>>* Pts = nullptr;
			if (!Leg.IsValid() || !Leg->TryGetArrayField(TEXT("pts"), Pts) || !Pts)
			{
				continue;
			}
			double LegNum = 0.0;
			Leg->TryGetNumberField(TEXT("leg"), LegNum);
			const int32 LegNo = FMath::RoundToInt32(LegNum);
			TArray<FVector> P;
			for (const TSharedPtr<FJsonValue>& V : *Pts)
			{
				if (!V.IsValid() || V->Type != EJson::Array)
				{
					continue;
				}
				const TArray<TSharedPtr<FJsonValue>>& A = V->AsArray();
				if (A.Num() >= 3)
				{
					P.Add(FVector(A[0]->AsNumber(), A[1]->AsNumber(), A[2]->AsNumber()));
				}
			}
			if (LegNo < FirstLeg || LegNo > LastLeg || P.Num() < 2)
			{
				continue;
			}
			FString Name;
			Leg->TryGetStringField(TEXT("name"), Name);
			Plan.LegNames.Add(LegNo, Name);

			TArray<double> Cum;
			Cum.SetNumZeroed(P.Num());
			for (int32 i = 1; i < P.Num(); ++i)
			{
				Cum[i] = Cum[i - 1] + FVector::Dist2D(P[i - 1], P[i]);
			}
			const double Len = Cum.Last();
			const auto At = [&P, &Cum, Len](double D) -> FVector
			{
				D = FMath::Clamp(D, 0.0, Len);
				int32 i = 1;
				while (i < P.Num() - 1 && Cum[i] < D)
				{
					++i;
				}
				const double Seg = FMath::Max(Cum[i] - Cum[i - 1], 1e-3);
				return FMath::Lerp(P[i - 1], P[i], FMath::Clamp((D - Cum[i - 1]) / Seg, 0.0, 1.0));
			};
			// every Spacing from the leg's start (a camp), and its end when the last stretch is long enough (the summit)
			TArray<double> Ds;
			for (double D = 0.0; D <= Len + 1.0; D += Spacing)
			{
				Ds.Add(D);
			}
			if (Len - Ds.Last() > 0.3 * Spacing)
			{
				Ds.Add(Len);
			}
			for (int32 k = 0; k < Ds.Num(); ++k)
			{
				const double D = Ds[k];
				FYggRouteCheckPoint& Pt = Plan.Points.AddDefaulted_GetRef();
				Pt.Leg = LegNo;
				Pt.LegDistM = D / 100.0;
				Pt.RunDistM = (Run + D) / 100.0;
				Pt.Pos = At(D);
				const double Ahead = FMath::Min(D + 3000.0, Len);
				const double Behind = FMath::Max(Ahead - 3000.0, 0.0);
				const FVector A0 = At(Behind), A1 = At(Ahead);
				Pt.Yaw = float(FMath::RadiansToDegrees(FMath::Atan2(A1.Y - A0.Y, A1.X - A0.X)));
				Pt.GradeDeg = float(FMath::RadiansToDegrees(FMath::Atan2(A1.Z - A0.Z, FMath::Max(FVector::Dist2D(A0, A1), 1.0))));
				Pt.SubFirst = Plan.Sub.Num();
				if (k > 0)
				{
					for (double S = Ds[k - 1] + SubStep; S < D - 1.0; S += SubStep)
					{
						Plan.Sub.Add(At(S));
					}
				}
				Pt.SubNum = Plan.Sub.Num() - Pt.SubFirst;
			}
			Run += Len;
		}
		return Plan;
	}
}

static FAutoConsoleCommandWithWorldAndArgs CmdRouteCheck(TEXT("ygg.RouteCheck"),
	TEXT("ygg.RouteCheck [SpacingM=200] [FirstLeg=0] [LastLeg=all] [Shots=1] - jumps along the route and checks the ground, every object within ")
	TEXT("40 m and the guidance ahead at each point, with a picture (0 none, 1 along the route, 2 also back). Saved/RouteCheck/<time>/. ygg.RouteCheck stop ends it."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		UGameInstance* GI = World ? World->GetGameInstance() : nullptr;
		UYggRouteCheck* Check = GI ? GI->GetSubsystem<UYggRouteCheck>() : nullptr;
		if (!Check)
		{
			UE_LOG(LogTemp, Warning, TEXT("RouteCheck: needs a running game (Play, or the standalone -game)."));
			return;
		}
		if (Args.Num() > 0 && Args[0].Equals(TEXT("stop"), ESearchCase::IgnoreCase))
		{
			Check->Stop(TEXT("ygg.RouteCheck stop"));
			return;
		}
		Check->Begin(Args, World);
	}));

void UYggRouteCheck::Begin(const TArray<FString>& Args, UWorld* World)
{
	if (Step != EYggRouteCheckStep::Idle)
	{
		UE_LOG(LogTemp, Warning, TEXT("RouteCheck: already running (ygg.RouteCheck stop ends it)."));
		return;
	}
	if (!World)
	{
		return;
	}
	// SpacingM FirstLeg LastLeg Shots in that order, or Spacing= First= Last= Shots= in any order
	SpacingM = 200.f;
	FirstLeg = 0;
	LastLeg = 999;
	Shots = 1;
	int32 Positional = 0;
	for (const FString& Arg : Args)
	{
		FString Key, Value;
		int32 Slot = INDEX_NONE;
		if (Arg.Split(TEXT("="), &Key, &Value))
		{
			Slot = Key.StartsWith(TEXT("Sp"), ESearchCase::IgnoreCase) ? 0 : Key.StartsWith(TEXT("F"), ESearchCase::IgnoreCase) ? 1
				: Key.StartsWith(TEXT("L"), ESearchCase::IgnoreCase) ? 2 : Key.StartsWith(TEXT("Sh"), ESearchCase::IgnoreCase) ? 3 : INDEX_NONE;
		}
		else
		{
			Value = Arg;
			Slot = Positional++;
		}
		switch (Slot)
		{
		case 0: SpacingM = FCString::Atof(*Value); break;
		case 1: FirstLeg = FCString::Atoi(*Value); break;
		case 2: LastLeg = Value.Equals(TEXT("all"), ESearchCase::IgnoreCase) ? 999 : FCString::Atoi(*Value); break;
		case 3: Shots = FCString::Atoi(*Value); break;
		default: break;
		}
	}
	SpacingM = FMath::Clamp(SpacingM, 10.f, 5000.f);
	Shots = FMath::Clamp(Shots, 0, 2);

	if (Pipe.IsValid())
	{
		Pipe->WaitUntilEmpty();   // the last run's files (long written by now)
	}
	Pipe = MakeUnique<UE::Tasks::FPipe>(TEXT("YggRouteCheckFiles"));
	ImageWrappers = &FModuleManager::LoadModuleChecked<IImageWrapperModule>(TEXT("ImageWrapper"));
	WorldRef = World;
	RunStart = FPlatformTime::Seconds();
	Dir = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("RouteCheck") / FDateTime::Now().ToString(TEXT("%Y%m%d_%H%M%S")));
	IFileManager::Get().MakeDirectory(*(Dir / TEXT("shots")), true);

	Plan = FYggRouteCheckPlan();
	Cur = 0;
	ToIndex.Reset();
	Statics.Reset();
	StaticGrid.Reset();
	Instanced.Reset();
	InstancedKind.Reset();
	TileBoxes.Reset();
	RopePts.Reset();
	FlagPts.Reset();
	CluePts.Reset();
	CheckpointPts.Reset();
	TreePts.Reset();
	TreeSet.Reset();
	TreeNames.Reset();
	SkippedHuge = 0;
	Measured.Reset();
	TerrainOf.Reset();
	ClimberParts.Reset();
	TotPoints = TotIssuePoints = TotObjects = TotFloating = TotBuried = TotWalkInto = TotShots = 0;

	Write(TEXT("points.csv"), TEXT("idx,leg,leg_name,leg_dist_m,run_dist_m,route_x,route_y,route_z,yaw,stand_z,dz_route_cm,ground,surface,ground_actor,ground_class,")
		TEXT("slope_deg,grade_deg,normal_slope_deg,walkable,capsule_blocked,pawn_dz_cm,tile_z,land_z,land_minus_tile_cm,overhead,issues,")
		TEXT("seg_samples,seg_no_ground,seg_holes,seg_no_tile,seg_steep,seg_max_step_cm,seg_bad_x,seg_bad_y,")
		TEXT("rope_ahead_m,rope_los,flag_ahead_m,flag_los,clue_ahead_m,clue_los,checkpoint_ahead_m,guided,")
		TEXT("objects,floating,buried,walk_into,streaming_left,settle_s,frame_ms,frame_ms_max,shot,shot_back,bugitgo\n"), true);
	Write(TEXT("objects.csv"), TEXT("point,leg,leg_dist_m,dist_m,actor,class,component,mesh,instance,kind,x,y,z,size_x_cm,size_y_cm,height_cm,")
		TEXT("method,cells,gap_cm,terrain_gap_cm,pen_cm,frac_under,air_share,on,blocks_player,walk_into,floating,buried,bugitgo\n"), true);
	Write(TEXT("log.txt"), FString::Printf(TEXT("route check %s\n"), *FDateTime::Now().ToString()), true);
	Log(FString::Printf(TEXT("ygg.RouteCheck spacing %.0f m, legs %d..%d, shots %d -> %s"), SpacingM, FirstLeg, LastLeg, Shots, *Dir));
	Log(FString::Printf(TEXT("radius %.0f m, settle %.1f s, first wait %.0f s, max objects %d, time %.0f, quit when done %d"),
		CVarRouteCheckRadius.GetValueOnGameThread(), CVarRouteCheckSettle.GetValueOnGameThread(), CVarRouteCheckFirstWait.GetValueOnGameThread(),
		CVarRouteCheckMaxObjects.GetValueOnGameThread(), CVarRouteCheckTime.GetValueOnGameThread(), CVarRouteCheckQuit.GetValueOnGameThread()));

	// the route is read and cut on a worker: the game thread never waits on the disk
	const FString RouteFile = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / TEXT("Tools/RoutePlanning/route.json"));
	const float Spacing = SpacingM;
	const int32 First = FirstLeg, Last = LastLeg;
	PlanFuture = Async(EAsyncExecution::ThreadPool, [RouteFile, Spacing, First, Last]() -> FYggRouteCheckPlan
	{
		FString Text;
		if (!FFileHelper::LoadFileToString(Text, *RouteFile))
		{
			FYggRouteCheckPlan Failed;
			Failed.Error = TEXT("cannot read ") + RouteFile;
			return Failed;
		}
		return RouteCheckBuildPlan(Text, Spacing, First, Last);
	});
	SetStep(EYggRouteCheckStep::Loading);
}

void UYggRouteCheck::Stop(const FString& Why)
{
	if (Step == EYggRouteCheckStep::Idle || Step == EYggRouteCheckStep::Quit)
	{
		return;
	}
	Finish(TEXT("stopped: ") + Why);
}

void UYggRouteCheck::Deinitialize()
{
	if (Step != EYggRouteCheckStep::Idle && Step != EYggRouteCheckStep::Quit)
	{
		Write(TEXT("done.txt"), FString::Printf(TEXT("status stopped: the game closed\npoints %d of %d\nfolder %s\n"), TotPoints, Plan.Points.Num(), *Dir), true);
	}
	Step = EYggRouteCheckStep::Idle;
	ClearShot();
	if (Pipe.IsValid())
	{
		Pipe->WaitUntilEmpty();
		Pipe.Reset();
	}
	Super::Deinitialize();
}

TStatId UYggRouteCheck::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UYggRouteCheck, STATGROUP_Tickables);
}

void UYggRouteCheck::SetStep(EYggRouteCheckStep NewStep)
{
	Step = NewStep;
	StepStart = FPlatformTime::Seconds();
	bEntered = false;
}

void UYggRouteCheck::Write(const TCHAR* Name, const FString& Text, bool bReplace)
{
	if (!Pipe.IsValid() || Dir.IsEmpty())
	{
		return;
	}
	// one pipe: the writes happen in order, one after another, on a worker
	Pipe->Launch(TEXT("YggRouteCheckWrite"), [File = Dir / Name, Text, bReplace]()
	{
		FFileHelper::SaveStringToFile(Text, *File, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM, &IFileManager::Get(), bReplace ? 0 : FILEWRITE_Append);
	});
}

void UYggRouteCheck::Log(const FString& Line)
{
	UE_LOG(LogTemp, Display, TEXT("RouteCheck: %s"), *Line);
	Write(TEXT("log.txt"), FString::Printf(TEXT("%8.1f  %s\n"), FPlatformTime::Seconds() - RunStart, *Line), false);
}

ACharacter* UYggRouteCheck::GetClimber(UWorld* World) const
{
	APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
	return PC ? Cast<ACharacter>(PC->GetPawn()) : nullptr;
}

void UYggRouteCheck::Finish(const FString& Status)
{
	ClearShot();
	const double Minutes = (FPlatformTime::Seconds() - RunStart) / 60.0;
	Log(FString::Printf(TEXT("finished (%s): %d of %d points, %d with issues; %d objects: %d floating, %d buried, %d walked through; %d pictures; %.1f min"),
		*Status, TotPoints, Plan.Points.Num(), TotIssuePoints, TotObjects, TotFloating, TotBuried, TotWalkInto, TotShots, Minutes));
	Write(TEXT("done.txt"), FString::Printf(TEXT("status %s\npoints %d of %d\npoints_with_issues %d\nobjects %d\nfloating %d\nburied %d\nwalk_into %d\nshots %d\nminutes %.1f\nfolder %s\n"),
		*Status, TotPoints, Plan.Points.Num(), TotIssuePoints, TotObjects, TotFloating, TotBuried, TotWalkInto, TotShots, Minutes, *Dir), true);
	const bool bQuit = CVarRouteCheckQuit.GetValueOnGameThread() != 0;
	if (!bQuit)
	{
		ReleasePlayer();
	}
	// the route and the level index are only needed while running
	PlanFuture = TFuture<FYggRouteCheckPlan>();
	Plan = FYggRouteCheckPlan();
	Jobs.Empty();
	ToIndex.Empty();
	Statics.Empty();
	StaticGrid.Empty();
	Instanced.Empty();
	InstancedKind.Empty();
	TileBoxes.Empty();
	RopePts.Empty();
	FlagPts.Empty();
	CluePts.Empty();
	CheckpointPts.Empty();
	TreePts.Empty();
	TreeSet.Empty();
	TreeNames.Empty();
	Measured.Empty();
	TerrainOf.Empty();
	Scratch.Empty();
	ObjRows.Empty();
	if (bQuit)
	{
		SetStep(EYggRouteCheckStep::Quit);
	}
	else
	{
		Step = EYggRouteCheckStep::Idle;
	}
}

void UYggRouteCheck::TakePlayer(ACharacter* Me)
{
	if (USurvivalComponent* Body = Me->FindComponentByClass<USurvivalComponent>())
	{
		bSurvivalTickWas = Body->IsComponentTickEnabled();
		Body->SetComponentTickEnabled(false);   // the body stays as it is: no thin air, cold or hunger while jumping about
		if (Body->IsDead())
		{
			Body->Revive(100.f);
		}
	}
	if (UClimbingComponent* Climb = Me->FindComponentByClass<UClimbingComponent>())
	{
		Climb->ForgetFall();
	}
	// no landings and none of the hazards' overlap triggers (crevasses, serac fall, snow bridges): nothing can hurt it
	Me->SetActorEnableCollision(false);
	// what hangs on it keeps its own collision: the traces must not find the ground on its kit
	TArray<AActor*> Attached;
	Me->GetAttachedActors(Attached, true, true);
	ClimberParts.Reset();
	for (AActor* Part : Attached)
	{
		ClimberParts.Add(Part);
	}
	if (UCharacterMovementComponent* Move = Me->GetCharacterMovement())
	{
		Move->bCheatFlying = true;
		Move->SetMovementMode(MOVE_Flying);
		Move->StopMovementImmediately();
	}
	HeldWas.Reset();
	for (const TCHAR* Name : RouteCheckHeldVars)
	{
		IConsoleVariable* Var = IConsoleManager::Get().FindConsoleVariable(Name);
		HeldWas.Add(Var ? Var->GetFloat() : -1.f);
		if (Var)
		{
			Var->Set(0.f, ECVF_SetByConsole);
		}
	}
	bChangedPlayer = true;
	Log(TEXT("climber taken: flying, collision off, body simulation paused; frost, thin-air tunnel and blowing snow held off"));
}

void UYggRouteCheck::ReleasePlayer()
{
	if (!bChangedPlayer)
	{
		return;
	}
	bChangedPlayer = false;
	const int32 NumHeld = int32(UE_ARRAY_COUNT(RouteCheckHeldVars));
	for (int32 i = 0; i < NumHeld && i < HeldWas.Num(); ++i)
	{
		if (IConsoleVariable* Var = IConsoleManager::Get().FindConsoleVariable(RouteCheckHeldVars[i]))
		{
			Var->Set(HeldWas[i], ECVF_SetByConsole);
		}
	}
	ACharacter* Me = GetClimber(WorldRef.Get());
	if (!Me)
	{
		return;
	}
	if (USurvivalComponent* Body = Me->FindComponentByClass<USurvivalComponent>())
	{
		Body->SetComponentTickEnabled(bSurvivalTickWas);
	}
	if (UClimbingComponent* Climb = Me->FindComponentByClass<UClimbingComponent>())
	{
		Climb->ForgetFall();   // the last jump is not a fall
	}
	Me->SetActorEnableCollision(true);
	if (UCharacterMovementComponent* Move = Me->GetCharacterMovement())
	{
		Move->bCheatFlying = false;
		Move->SetMovementMode(MOVE_Walking);   // it stands 2 cm above the last point's ground
	}
}

void UYggRouteCheck::IndexActor(AActor* A)
{
	if (const AFixedRope* Rope = Cast<AFixedRope>(A))
	{
		// guidance only: the rope pieces are laid on the snow by their own tool (Tools/RoutePlanning/lay_fixed_ropes.py)
		const bool bClip = Rope->RopePoints.Num() > 0;
		const TArray<FVector>& Pts = bClip ? Rope->RopePoints : Rope->LayPoints;
		for (int32 i = 0; i < Pts.Num(); i += bClip ? 1 : 8)
		{
			RopePts.Add(Pts[i]);
		}
		return;
	}
	if (const AForestStreamer* Forest = Cast<AForestStreamer>(A))
	{
		// the trees exist as components only near the climber: their places come from the streamer's own list
		for (const FForestSet& Set : Forest->Sets)
		{
			const int32 Name = TreeNames.AddUnique(Set.Mesh.GetAssetName());
			for (const FTransform& T : Set.Instances)
			{
				TreePts.Add(T.GetLocation());
				TreeSet.Add(Name);
			}
		}
		return;
	}
	if (Cast<AStoryClue>(A))
	{
		CluePts.Add(A->GetActorLocation());
	}
	else if (Cast<ATrailCheckpoint>(A))
	{
		CheckpointPts.Add(A->GetActorLocation());
	}
	else if (Cast<ARouteLandmark>(A))
	{
		FlagPts.Add(A->GetActorLocation());   // a camp: tents and a prayer-flag pole
	}
	if (Cast<APawn>(RouteCheckRoot(A)))
	{
		return;   // the climber and what it carries
	}
	const FString Class = A->GetClass()->GetName();
	for (const TCHAR* Skip : RouteCheckSkipClass)
	{
		if (Class.Contains(Skip)) { return; }
	}
	const FString Label = RouteCheckLabel(A);
	for (const TCHAR* Skip : RouteCheckSkipLabel)
	{
		if (Label.StartsWith(Skip)) { return; }
	}
	const FString Folder = RouteCheckFolder(A);
	for (const TCHAR* Skip : RouteCheckSkipFolder)
	{
		if (Folder.StartsWith(Skip)) { return; }
	}
	if (A->IsHidden() || A->GetActorLocation().Z > 600000.0)
	{
		return;   // hidden, or a template parked far above the world
	}
	const bool bCampProp = A->ActorHasTag(TEXT("CampProp"));
	TInlineComponentArray<UStaticMeshComponent*> Comps(A);
	for (UStaticMeshComponent* C : Comps)
	{
		const UStaticMesh* Mesh = C ? C->GetStaticMesh() : nullptr;
		if (!Mesh || !C->IsRegistered())
		{
			continue;
		}
		const FString Path = Mesh->GetPathName();
		if (Path.StartsWith(TEXT("/Game/Environment/DetailTiles/")))
		{
			TileBoxes.Add(C->Bounds.GetBox());   // the ground itself: where a tile's collision should be found
			continue;
		}
		if (!C->IsVisible() || C->bHiddenInGame)
		{
			continue;
		}
		const uint8 Kind = RouteCheckKindOf(A, Path, Label, bCampProp);
		if (UInstancedStaticMeshComponent* Ism = Cast<UInstancedStaticMeshComponent>(C))
		{
			// the route's 200k stones and the ice blocks: their instances near each point are asked for there
			Instanced.Add(Ism);
			InstancedKind.Add(Kind);
			continue;
		}
		if (C->Bounds.SphereRadius > 15000.0)
		{
			++SkippedHuge;   // far terrain, glacier sheets: ground, not objects
			continue;
		}
		if (Kind == RCK_Hanging && (Path.Contains(TEXT("flag"), ESearchCase::IgnoreCase) || Label.Contains(TEXT("flag"), ESearchCase::IgnoreCase)))
		{
			FlagPts.Add(C->Bounds.Origin);
		}
		FYggRouteCheckStatic& S = Statics.AddDefaulted_GetRef();
		S.Comp = C;
		S.Box = C->Bounds.GetBox();
		S.Kind = Kind;
		const int32 Id = Statics.Num() - 1;
		for (int32 X = FMath::FloorToInt32(S.Box.Min.X / RouteCheckCell); X <= FMath::FloorToInt32(S.Box.Max.X / RouteCheckCell); ++X)
		{
			for (int32 Y = FMath::FloorToInt32(S.Box.Min.Y / RouteCheckCell); Y <= FMath::FloorToInt32(S.Box.Max.Y / RouteCheckCell); ++Y)
			{
				StaticGrid.FindOrAdd(FIntPoint(X, Y)).Add(Id);
			}
		}
	}
}

bool UYggRouteCheck::IsTerrainCached(const FHitResult& Hit)
{
	const UPrimitiveComponent* C = Hit.GetComponent();
	if (!C)
	{
		return false;
	}
	// the mountain itself (the landscape or a route detail tile); asked thousands of times a point, worked out once
	const TObjectKey<UPrimitiveComponent> Key(C);
	if (const bool* Known = TerrainOf.Find(Key))
	{
		return *Known;
	}
	const bool bTerrain = USnowTrailComponent::IsTerrain(Hit);
	TerrainOf.Add(Key, bTerrain);
	return bTerrain;
}

bool UYggRouteCheck::TileCovers(double X, double Y) const
{
	for (const FBox& B : TileBoxes)
	{
		if (X >= B.Min.X && X <= B.Max.X && Y >= B.Min.Y && Y <= B.Max.Y)
		{
			return true;
		}
	}
	return false;
}

bool UYggRouteCheck::FindStand(UWorld* World, double X, double Y, double ZGuess, const AActor* Me, FHitResult& OutHit, FYggRouteCheckColumn& Col)
{
	Col = FYggRouteCheckColumn();
	FCollisionQueryParams Q(SCENE_QUERY_STAT(YggRouteCheckGround), true, Me);
	Q.AddIgnoredActors(ClimberParts);
	Q.bReturnFaceIndex = true;   // rock faces of the detail tiles are known by their index (USnowTrailComponent::IsRock)
	Q.bReturnPhysicalMaterial = true;
	const FVector Top(X, Y, ZGuess + 10000.0), Bottom(X, Y, ZGuess - 30000.0);
	// every surface in the column (object queries return them all, top down): the tile and the landscape under it
	FCollisionObjectQueryParams All(ECC_WorldStatic);
	All.AddObjectTypesToQuery(ECC_WorldDynamic);
	Scratch.Reset();
	World->LineTraceMultiByObjectType(Scratch, Top, Bottom, All, Q);
	for (const FHitResult& H : Scratch)
	{
		if (!IsTerrainCached(H))
		{
			continue;
		}
		const double Z = H.ImpactPoint.Z;
		if (!Col.bTerrain) { Col.bTerrain = true; Col.TerrainZ = Z; }
		if (Cast<ALandscapeProxy>(H.GetActor()))
		{
			if (!Col.bLand) { Col.bLand = true; Col.LandZ = Z; }
		}
		else if (!Col.bTile)
		{
			Col.bTile = true;
			Col.TileZ = Z;
		}
	}
	// what is seen from above, the way the camera sees it
	if (!World->LineTraceSingleByChannel(OutHit, Top, Bottom, ECC_Visibility, Q))
	{
		return false;
	}
	// something well above the mountain's own surface (a serac top, the ice-cave roof) is overhead, not underfoot
	if (Col.bTerrain && !IsTerrainCached(OutHit) && OutHit.ImpactPoint.Z > Col.TerrainZ + 300.0)
	{
		Col.Overhead = RouteCheckLabel(OutHit.GetActor());
		FHitResult Under;
		if (World->LineTraceSingleByChannel(Under, FVector(X, Y, Col.TerrainZ + 200.0), Bottom, ECC_Visibility, Q))
		{
			OutHit = Under;
		}
	}
	return true;
}

void UYggRouteCheck::Look(ACharacter* Me, bool bBack) const
{
	APlayerController* PC = Me ? Cast<APlayerController>(Me->GetController()) : nullptr;
	if (!PC || !Plan.Points.IsValidIndex(Cur))
	{
		return;
	}
	const FYggRouteCheckPoint& P = Plan.Points[Cur];
	PC->SetControlRotation(FRotator(RouteCheckPitch(bBack ? -P.GradeDeg : P.GradeDeg), P.Yaw + (bBack ? 180.f : 0.f), 0.f));
}

void UYggRouteCheck::PlaceClimber(UWorld* World, ACharacter* Me)
{
	const FYggRouteCheckPoint& P = Plan.Points[Cur];
	if (USurvivalComponent* Body = Me->FindComponentByClass<USurvivalComponent>())
	{
		if (Body->IsDead())
		{
			Body->Revive(100.f);
			Log(FString::Printf(TEXT("point %d: the climber was dead, revived"), Cur));
		}
	}
	FHitResult Hit;
	FYggRouteCheckColumn Col;
	const bool bGround = FindStand(World, P.Pos.X, P.Pos.Y, P.Pos.Z, Me, Hit, Col);
	// standing on the ground, so the camera is at eye height: flying with no speed, it stays exactly there
	const double Z = (bGround ? double(Hit.ImpactPoint.Z) : P.Pos.Z) + Me->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + 2.0;
	Me->TeleportTo(FVector(P.Pos.X, P.Pos.Y, Z), FRotator(0.f, P.Yaw, 0.f), false, true);
	if (UCharacterMovementComponent* Move = Me->GetCharacterMovement())
	{
		if (Move->MovementMode != MOVE_Flying)
		{
			Move->SetMovementMode(MOVE_Flying);
		}
		Move->StopMovementImmediately();
	}
	Look(Me, false);
	const float Time = CVarRouteCheckTime.GetValueOnGameThread();
	if (Time >= 0.f)
	{
		YggTime::Set(World, Time);
	}
	FrameSum = FrameMax = 0.f;
	FrameCount = 0;
	StreamingLeft = 0;
	ShotName.Reset();
	BackName.Reset();
}

void UYggRouteCheck::MeasurePoint(UWorld* World, ACharacter* Me)
{
	const FYggRouteCheckPoint& P = Plan.Points[Cur];
	UCharacterMovementComponent* Move = Me->GetCharacterMovement();
	const float HalfHeight = Me->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	const float Radius = Me->GetCapsuleComponent()->GetScaledCapsuleRadius();
	const double WalkableZ = Move ? double(Move->GetWalkableFloorZ()) : 0.71;
	const double WalkableDeg = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(WalkableZ, 0.0, 1.0)));
	TArray<FString> Issues;
	bPointIssue = false;
	const auto Issue = [&Issues, this](const TCHAR* What, bool bSerious)
	{
		Issues.Add(What);
		if (bSerious) { bPointIssue = true; }
	};

	// ---- the ground under the point
	FHitResult Hit;
	FYggRouteCheckColumn Col;
	const bool bGround = FindStand(World, P.Pos.X, P.Pos.Y, P.Pos.Z, Me, Hit, Col);
	const FVector Stand = bGround ? FVector(Hit.ImpactPoint) : P.Pos;
	StandPoint = Stand;
	const bool bTerrainHit = bGround && IsTerrainCached(Hit);
	const TCHAR* GroundKind = !bGround ? TEXT("none") : !bTerrainHit ? TEXT("object") : Cast<ALandscapeProxy>(Hit.GetActor()) ? TEXT("landscape") : TEXT("tile");
	const TCHAR* Surface = !bGround ? TEXT("-") : !bTerrainHit ? TEXT("object")
		: USnowTrailComponent::IsRock(Hit) ? TEXT("rock") : USnowTrailComponent::IsGlacierHard(Hit.ImpactPoint) ? TEXT("glacier") : TEXT("snow");
	const FString GroundActor = bGround ? RouteCheckLabel(Hit.GetActor()) : FString(TEXT("-"));
	const FString GroundClass = bGround && Hit.GetActor() ? Hit.GetActor()->GetClass()->GetName() : FString(TEXT("-"));

	// slope of the ground plane over 2 m (one triangle's normal on rock can be steep where the ground is not)
	FCollisionQueryParams QV(SCENE_QUERY_STAT(YggRouteCheckSlope), true, Me);
	QV.AddIgnoredActors(ClimberParts);
	const auto ZAt = [World, &QV, &Stand](double X, double Y, double& OutZ)
	{
		FHitResult H;
		if (World->LineTraceSingleByChannel(H, FVector(X, Y, Stand.Z + 300.0), FVector(X, Y, Stand.Z - 300.0), ECC_Visibility, QV))
		{
			OutZ = H.ImpactPoint.Z;
		}
	};
	double ZE = Stand.Z, ZW = Stand.Z, ZN = Stand.Z, ZS = Stand.Z;
	ZAt(Stand.X + 100.0, Stand.Y, ZE);
	ZAt(Stand.X - 100.0, Stand.Y, ZW);
	ZAt(Stand.X, Stand.Y + 100.0, ZN);
	ZAt(Stand.X, Stand.Y - 100.0, ZS);
	const double Dx = (ZE - ZW) / 200.0, Dy = (ZN - ZS) / 200.0;
	const double SlopeDeg = FMath::RadiansToDegrees(FMath::Atan(FMath::Sqrt(Dx * Dx + Dy * Dy)));
	const FVector Fwd = FRotator(0.f, P.Yaw, 0.f).Vector();
	const double GradeDeg = FMath::RadiansToDegrees(FMath::Atan(Dx * Fwd.X + Dy * Fwd.Y));
	const double NormalDeg = bGround ? FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(double(Hit.ImpactNormal.Z), -1.0, 1.0))) : 0.0;
	const bool bWalkable = bGround && SlopeDeg <= WalkableDeg;

	// the climber's own collision: does the capsule fit here, and does it stand where the eye sees the ground? On a slope
	// a capsule rests on its round bottom, higher than on the flat (r / cos slope), or the test would touch the slope itself
	FCollisionQueryParams QP(SCENE_QUERY_STAT(YggRouteCheckPawn), false, Me);
	QP.AddIgnoredActors(ClimberParts);
	const double Lift = HalfHeight - Radius + Radius / FMath::Max(FMath::Cos(FMath::DegreesToRadians(SlopeDeg)), 0.3) + 3.0;
	const bool bBlocked = bGround && World->OverlapBlockingTestByChannel(Stand + FVector(0.0, 0.0, Lift), FQuat::Identity, ECC_Pawn,
		FCollisionShape::MakeCapsule(FMath::Max(Radius - 2.f, 5.f), FMath::Max(HalfHeight - 2.f, 10.f)), QP);
	FHitResult PawnHit;
	const bool bPawn = bGround && World->LineTraceSingleByChannel(PawnHit, Stand + FVector(0.0, 0.0, 200.0), Stand - FVector(0.0, 0.0, 5000.0), ECC_Pawn, QP);
	const double PawnDz = bPawn ? PawnHit.ImpactPoint.Z - Stand.Z : 0.0;

	if (!bGround)
	{
		Issue(TEXT("no_ground"), true);
	}
	else
	{
		if (!bPawn) { Issue(TEXT("pawn_falls_through"), true); }
		else if (PawnDz < -15.0) { Issue(TEXT("pawn_sinks"), true); }      // what is seen is not solid (walked through)
		else if (PawnDz > 15.0) { Issue(TEXT("pawn_floats"), true); }      // invisible collision above what is seen
		if (!bWalkable) { Issue(TEXT("steep"), true); }
		if (bBlocked) { Issue(TEXT("capsule_blocked"), true); }
		if (!bTerrainHit) { Issue(TEXT("on_object"), false); }
		if (FMath::Abs(Stand.Z - P.Pos.Z) > 300.0) { Issue(TEXT("far_from_route_z"), false); }
	}
	const bool bTileHere = TileCovers(P.Pos.X, P.Pos.Y);
	if (bTileHere && !Col.bTile) { Issue(TEXT("tile_hole"), true); }        // a tile is here but its collision is not
	if (!bTileHere) { Issue(TEXT("no_tile"), true); }
	if (Col.bTile && Col.bLand && Col.LandZ > Col.TileZ + 20.0) { Issue(TEXT("land_above_tile"), true); }

	// ---- the stretch since the previous point, every 5 m: holes, missing tiles, steps, steep ground
	int32 SegNoGround = 0, SegHoles = 0, SegNoTile = 0, SegSteep = 0;
	double SegMaxStep = 0.0, PrevZ = 0.0;
	bool bPrev = false, bSegBad = false;
	FVector SegBad = FVector::ZeroVector;
	FCollisionQueryParams QS(SCENE_QUERY_STAT(YggRouteCheckStretch), true, Me);
	QS.AddIgnoredActors(ClimberParts);
	FCollisionObjectQueryParams All(ECC_WorldStatic);
	All.AddObjectTypesToQuery(ECC_WorldDynamic);
	for (int32 i = 0; i < P.SubNum; ++i)
	{
		const FVector S = Plan.Sub[P.SubFirst + i];
		Scratch.Reset();
		World->LineTraceMultiByObjectType(Scratch, FVector(S.X, S.Y, S.Z + 10000.0), FVector(S.X, S.Y, S.Z - 30000.0), All, QS);
		const FHitResult* Ground = nullptr;
		bool bTileHit = false;
		for (const FHitResult& H : Scratch)
		{
			if (!IsTerrainCached(H)) { continue; }
			if (!Ground) { Ground = &H; }
			if (!Cast<ALandscapeProxy>(H.GetActor())) { bTileHit = true; }
		}
		const bool bCovered = TileCovers(S.X, S.Y);
		bool bBad = false;
		if (!Ground)
		{
			++SegNoGround;
			bBad = true;
		}
		else
		{
			if (bCovered && !bTileHit) { ++SegHoles; bBad = true; }   // the ray went through the tile to the landscape under it
			if (!bCovered) { ++SegNoTile; }
			if (Ground->ImpactNormal.Z < WalkableZ) { ++SegSteep; }
			if (bPrev) { SegMaxStep = FMath::Max(SegMaxStep, FMath::Abs(Ground->ImpactPoint.Z - PrevZ)); }
			PrevZ = Ground->ImpactPoint.Z;
			bPrev = true;
		}
		if (bBad && !bSegBad)
		{
			bSegBad = true;
			SegBad = S;
		}
	}
	if (SegNoGround > 0 || SegHoles > 0)
	{
		bPointIssue = true;
	}

	PointCols = FString::Printf(TEXT("%d,%d,%s,%.0f,%.0f,%.0f,%.0f,%.0f,%.1f,%s,%s,%s,%s,%s,%s,%.1f,%.1f,%.1f,%d,%d,%s,%s,%s,%s,%s,%s,%d,%d,%d,%d,%d,%.0f,%s,%s,"),
		Cur, P.Leg, *RouteCheckCsv(Plan.LegNames.FindRef(P.Leg)), P.LegDistM, P.RunDistM, P.Pos.X, P.Pos.Y, P.Pos.Z, P.Yaw,
		*RouteCheckF1(bGround, Stand.Z), *RouteCheckF1(bGround, Stand.Z - P.Pos.Z), GroundKind, Surface, *RouteCheckCsv(GroundActor), *RouteCheckCsv(GroundClass),
		SlopeDeg, GradeDeg, NormalDeg, bWalkable ? 1 : 0, bBlocked ? 1 : 0, *RouteCheckF1(bPawn, PawnDz),
		*RouteCheckF1(Col.bTile, Col.TileZ), *RouteCheckF1(Col.bLand, Col.LandZ), *RouteCheckF1(Col.bTile && Col.bLand, Col.LandZ - Col.TileZ),
		*RouteCheckCsv(Col.Overhead), *RouteCheckCsv(FString::Join(Issues, TEXT(";"))),
		P.SubNum, SegNoGround, SegHoles, SegNoTile, SegSteep, SegMaxStep, *RouteCheckF0(bSegBad, SegBad.X), *RouteCheckF0(bSegBad, SegBad.Y));

	// ---- guidance: the nearest rope, flag, clue and checkpoint ahead, and whether the eye can see it
	APlayerController* PC = Cast<APlayerController>(Me->GetController());
	const FVector Eye = PC && PC->PlayerCameraManager ? PC->PlayerCameraManager->GetCameraLocation() : Stand + FVector(0.0, 0.0, 165.0);
	FCollisionQueryParams QSight(SCENE_QUERY_STAT(YggRouteCheckSight), true, Me);
	QSight.AddIgnoredActors(ClimberParts);
	const auto Guide = [&](const TArray<FVector>& Pts, FString& OutDist, FString& OutSeen) -> bool
	{
		FVector At = FVector::ZeroVector;
		const float D = RouteCheckAhead(Pts, Stand, Fwd, At);
		if (D < 0.f)
		{
			OutDist.Reset();
			OutSeen.Reset();
			return false;
		}
		OutDist = FString::Printf(TEXT("%.0f"), D);
		FHitResult H;
		const FVector To = At + FVector(0.0, 0.0, 40.0);
		const bool bSeen = !World->LineTraceSingleByChannel(H, Eye, To, ECC_Visibility, QSight) || FVector::Dist(H.ImpactPoint, To) < 150.0;
		OutSeen = bSeen ? TEXT("1") : TEXT("0");
		return true;
	};
	FString RopeD, RopeS, FlagD, FlagS, ClueD, ClueS, CheckD, CheckS;
	const bool bRope = Guide(RopePts, RopeD, RopeS);
	const bool bFlag = Guide(FlagPts, FlagD, FlagS);
	const bool bClue = Guide(CluePts, ClueD, ClueS);
	const bool bCheck = Guide(CheckpointPts, CheckD, CheckS);
	PointCols += FString::Printf(TEXT("%s,%s,%s,%s,%s,%s,%s,%d"), *RopeD, *RopeS, *FlagD, *FlagS, *ClueD, *ClueS, *CheckD, (bRope || bFlag || bClue || bCheck) ? 1 : 0);

	PointBugIt = FString::Printf(TEXT("BugItGo %.0f %.0f %.0f %.1f %.1f 0"), Stand.X, Stand.Y, Stand.Z + HalfHeight + 5.0, RouteCheckPitch(P.GradeDeg), P.Yaw);
}

void UYggRouteCheck::GatherObjects()
{
	Jobs.Reset();
	const double R = FMath::Max(1.f, CVarRouteCheckRadius.GetValueOnGameThread()) * 100.0;
	const FVector C = StandPoint;
	const auto KeyOf = [](const UObject* O, int32 Instance) { return (uint64(O ? O->GetUniqueID() : 0u) << 32) | uint64(uint32(Instance + 1)); };

	// plain static meshes, from the grid
	for (int32 X = FMath::FloorToInt32((C.X - R) / RouteCheckCell); X <= FMath::FloorToInt32((C.X + R) / RouteCheckCell); ++X)
	{
		for (int32 Y = FMath::FloorToInt32((C.Y - R) / RouteCheckCell); Y <= FMath::FloorToInt32((C.Y + R) / RouteCheckCell); ++Y)
		{
			const TArray<int32>* Ids = StaticGrid.Find(FIntPoint(X, Y));
			if (!Ids)
			{
				continue;
			}
			for (const int32 Id : *Ids)
			{
				const FYggRouteCheckStatic& S = Statics[Id];
				UStaticMeshComponent* Comp = S.Comp.Get();
				if (!Comp)
				{
					continue;
				}
				const double D = RouteCheckDist2D(S.Box, C);
				const uint64 Key = KeyOf(Comp, INDEX_NONE);
				if (D > R || FMath::Abs(S.Box.GetCenter().Z - C.Z) > 30000.0 || Measured.Contains(Key))
				{
					continue;   // too far, or already measured from an earlier point
				}
				Measured.Add(Key);
				FYggRouteCheckJob& J = Jobs.AddDefaulted_GetRef();
				J.Key = Key;
				J.Comp = Comp;
				J.Box = S.Box;
				J.Kind = S.Kind;
				J.DistM = float(D / 100.0);
			}
		}
	}

	// instances near the point (their components' trees answer this fast)
	const FBox Query(C - FVector(R, R, 30000.0), C + FVector(R, R, 30000.0));
	for (int32 i = 0; i < Instanced.Num(); ++i)
	{
		UInstancedStaticMeshComponent* Ism = Instanced[i].Get();
		const UStaticMesh* Mesh = Ism ? Ism->GetStaticMesh() : nullptr;
		if (!Mesh || !Ism->Bounds.GetBox().Intersect(Query))
		{
			continue;
		}
		const FBox Local = Mesh->GetBoundingBox();
		const TArray<int32> Near = Ism->GetInstancesOverlappingBox(Query, true);
		for (const int32 Index : Near)
		{
			FTransform T;
			if (!Ism->GetInstanceTransform(Index, T, true))
			{
				continue;
			}
			const FBox B = Local.TransformBy(T);
			const double D = RouteCheckDist2D(B, C);
			const uint64 Key = KeyOf(Ism, Index);
			if (D > R || B.GetExtent().GetMax() > 15000.0 || Measured.Contains(Key))
			{
				continue;
			}
			Measured.Add(Key);
			FYggRouteCheckJob& J = Jobs.AddDefaulted_GetRef();
			J.Key = Key;
			J.Comp = Ism;
			J.Instance = Index;
			J.Box = B;
			J.Kind = InstancedKind[i];
			J.DistM = float(D / 100.0);
		}
	}

	// trees, from the forest's own list
	for (int32 i = 0; i < TreePts.Num(); ++i)
	{
		const FVector& T = TreePts[i];
		const double D = FVector::Dist2D(T, C);
		const uint64 Key = (uint64(0xFFFFFFFFu) << 32) | uint64(uint32(i));
		if (D > R || FMath::Abs(T.Z - C.Z) > 30000.0 || Measured.Contains(Key))
		{
			continue;
		}
		Measured.Add(Key);
		FYggRouteCheckJob& J = Jobs.AddDefaulted_GetRef();
		J.Key = Key;
		J.Tree = i;
		J.Kind = RCK_Tree;
		J.Box = FBox(T - FVector(50.0, 50.0, 0.0), T + FVector(50.0, 50.0, 1000.0));
		J.DistM = float(D / 100.0);
	}

	Jobs.Sort([](const FYggRouteCheckJob& A, const FYggRouteCheckJob& B) { return A.DistM < B.DistM; });
	const int32 Max = FMath::Max(1, CVarRouteCheckMaxObjects.GetValueOnGameThread());
	if (Jobs.Num() > Max)
	{
		Log(FString::Printf(TEXT("point %d: %d objects within reach, only the nearest %d measured"), Cur, Jobs.Num(), Max));
		// the ones left out were not measured: the next point may still reach them
		for (int32 i = Max; i < Jobs.Num(); ++i)
		{
			Measured.Remove(Jobs[i].Key);
		}
		Jobs.SetNum(Max);
	}
}

void UYggRouteCheck::MeasureObject(UWorld* World, const AActor* Me, const FYggRouteCheckJob& J)
{
	UStaticMeshComponent* Comp = J.Comp.Get();
	if (!Comp)
	{
		return;   // gone since it was found
	}
	const FYggRouteCheckPoint& P = Plan.Points[Cur];
	const FBox& B = J.Box;
	const FVector Size = B.GetSize();
	FCollisionQueryParams Q(SCENE_QUERY_STAT(YggRouteCheckObject), true, Me);
	Q.AddIgnoredActors(ClimberParts);
	FCollisionObjectQueryParams Everything(ECC_WorldStatic);
	Everything.AddObjectTypesToQuery(ECC_WorldDynamic);
	const FCollisionObjectQueryParams Own(Comp->GetCollisionObjectType());
	const bool bQuery = CollisionEnabledHasQuery(Comp->GetCollisionEnabled());
	const bool bBlocks = bQuery && Comp->GetCollisionResponseToChannel(ECC_Pawn) == ECR_Block;
	// a camp's cords and flags hang by design (no collision); its tents, pole and cairn block and must stand on the ground
	const bool bHangs = J.Kind == RCK_Hanging || (J.Kind == RCK_Landmark && !bBlocks);
	const int32 N = FMath::Max(Size.X, Size.Y) > 400.0 ? 4 : 3;
	const auto IsSelf = [Comp, &J](const FHitResult& H) { return H.GetComponent() == Comp && (J.Instance == INDEX_NONE || H.Item == J.Instance); };

	// A grid of cells under it. In each: its underside (a ray up from below it, keeping only its own hit; without
	// collision the bounding box bottom, which is never higher than the mesh), its top, the mountain's surface and
	// whatever it rests on (another rock, a box, a serac) between the two.
	int32 Cells = 0, AirCells = 0;
	double MinGap = 0.0, MinTerrainGap = 0.0, MaxPen = 0.0, SumBuried = 0.0, SumColumn = 0.0, SumTerrain = 0.0;
	FString On;
	const auto RunCells = [&](bool bUseTrace) -> int32
	{
		Cells = AirCells = 0;
		MinGap = MinTerrainGap = TNumericLimits<double>::Max();
		MaxPen = -TNumericLimits<double>::Max();
		SumBuried = SumColumn = SumTerrain = 0.0;
		On = TEXT("-");
		for (int32 IY = 0; IY < N; ++IY)
		{
			for (int32 IX = 0; IX < N; ++IX)
			{
				const double X = B.Min.X + (IX + 0.5) / N * Size.X;
				const double Y = B.Min.Y + (IY + 0.5) / N * Size.Y;
				double Under = B.Min.Z, Top = B.Max.Z;
				if (bUseTrace)
				{
					Scratch.Reset();
					World->LineTraceMultiByObjectType(Scratch, FVector(X, Y, B.Min.Z - 100.0), FVector(X, Y, B.Max.Z + 100.0), Own, Q);
					bool bOwn = false;
					for (const FHitResult& H : Scratch)
					{
						if (IsSelf(H)) { Under = H.ImpactPoint.Z; bOwn = true; break; }
					}
					if (!bOwn)
					{
						continue;   // this cell is outside the mesh's real outline
					}
				}
				Scratch.Reset();
				World->LineTraceMultiByObjectType(Scratch, FVector(X, Y, B.Max.Z + 200.0), FVector(X, Y, B.Min.Z - 5000.0), Everything, Q);
				bool bTerrain = false, bSupport = false, bTop = false;
				double TerrainZ = 0.0, SupportZ = 0.0;
				FString SupportName;
				for (const FHitResult& H : Scratch)
				{
					const double Z = H.ImpactPoint.Z;
					if (IsSelf(H))
					{
						if (bUseTrace && !bTop) { Top = Z; bTop = true; }
						continue;
					}
					if (IsTerrainCached(H))
					{
						TerrainZ = Z;   // top down: the first terrain hit is the surface
						bTerrain = true;
						break;
					}
					// what it rests on: the first other object under its underside or, traced, inside its own column below
					// its top here (a stone sunk into a boulder, an ice block set into a glacier sheet): that is contact,
					// not a gap down to the ground under the boulder or the sheet
					const bool bInside = bUseTrace && bTop && Z <= Top + 2.0;
					if (!bSupport && (bInside || Z <= Under + 2.0))
					{
						bSupport = true;
						SupportZ = Z;
						SupportName = RouteCheckLabel(H.GetActor());
					}
				}
				if (!bTerrain)
				{
					continue;   // no ground within 50 m below it
				}
				const double Support = bSupport ? SupportZ : TerrainZ;
				const double Gap = Under - Support;
				++Cells;
				if (Gap < MinGap)
				{
					MinGap = Gap;
					On = bSupport ? SupportName : FString(TEXT("terrain"));
				}
				MinTerrainGap = FMath::Min(MinTerrainGap, Under - TerrainZ);
				MaxPen = FMath::Max(MaxPen, TerrainZ - Under);
				AirCells += Gap > RouteCheckFloatCm ? 1 : 0;
				const double Column = FMath::Max(Top - Under, 1.0);
				SumColumn += Column;
				SumBuried += FMath::Clamp(TerrainZ - Under, 0.0, Column);
				SumTerrain += TerrainZ;
			}
		}
		return Cells;
	};
	const bool bTraced = bQuery && RunCells(true) > 0;
	if (!bTraced)
	{
		RunCells(false);
	}

	const bool bHave = Cells > 0;
	const double FracUnder = SumColumn > 0.0 ? SumBuried / SumColumn : 0.0;
	const double AboveGround = bHave ? B.Max.Z - SumTerrain / Cells : 0.0;
	// a bounding box bottom above the ground floats for certain (the mesh is never lower than its box)
	const bool bFloating = bHave && !bHangs && MinGap > RouteCheckFloatCm;
	const bool bBuried = bHave && FracUnder > 0.5;
	// no collision for the climber, standing on the ground, taller and wider than a stone: walked straight through
	const bool bWalkInto = bHave && !bBlocks && !bHangs && J.Kind != RCK_Drift && AboveGround > 50.0 && FMath::Min(Size.X, Size.Y) > 30.0 && MinGap < 50.0;
	const AActor* Owner = Comp->GetOwner();
	const UStaticMesh* Mesh = Comp->GetStaticMesh();
	const FVector Center = B.GetCenter();
	FVector Away = (StandPoint - Center).GetSafeNormal2D();
	if (Away.IsNearlyZero())
	{
		Away = FVector(1.0, 0.0, 0.0);
	}
	const FVector Cam = Center + Away * (FMath::Max(B.GetExtent().Size2D(), 100.0) + 500.0) + FVector(0.0, 0.0, B.GetExtent().Z + 150.0);
	ObjRows += FString::Printf(TEXT("%d,%d,%.0f,%.1f,%s,%s,%s,%s,%d,%s,%.0f,%.0f,%.0f,%.0f,%.0f,%.0f,%s,%d,%s,%s,%s,%s,%s,%s,%d,%d,%d,%d,%s\n"),
		Cur, P.Leg, P.LegDistM, J.DistM, *RouteCheckCsv(RouteCheckLabel(Owner)), *RouteCheckCsv(Owner ? Owner->GetClass()->GetName() : FString(TEXT("-"))),
		*RouteCheckCsv(Comp->GetName()), *RouteCheckCsv(Mesh ? Mesh->GetName() : FString(TEXT("-"))), J.Instance, RouteCheckKindNames[J.Kind],
		Center.X, Center.Y, Center.Z, Size.X, Size.Y, Size.Z, !bHave ? TEXT("none") : bTraced ? TEXT("trace") : TEXT("bounds"), Cells,
		*RouteCheckF1(bHave, MinGap), *RouteCheckF1(bHave, MinTerrainGap), *RouteCheckF1(bHave, MaxPen), *RouteCheckF2(bHave, FracUnder),
		*RouteCheckF2(bHave, double(AirCells) / FMath::Max(Cells, 1)), *RouteCheckCsv(On),
		bBlocks ? 1 : 0, bWalkInto ? 1 : 0, bFloating ? 1 : 0, bBuried ? 1 : 0, *RouteCheckCsv(RouteCheckBugItGo(Cam, Center)));
	++PtObjects;
	++TotObjects;
	if (bFloating) { ++PtFloating; ++TotFloating; }
	if (bBuried) { ++PtBuried; ++TotBuried; }
	if (bWalkInto) { ++PtWalkInto; ++TotWalkInto; }
}

void UYggRouteCheck::MeasureTree(UWorld* World, const AActor* Me, const FYggRouteCheckJob& J)
{
	if (!TreePts.IsValidIndex(J.Tree))
	{
		return;
	}
	const FYggRouteCheckPoint& P = Plan.Points[Cur];
	const FVector Root = TreePts[J.Tree];
	FCollisionQueryParams Q(SCENE_QUERY_STAT(YggRouteCheckTree), true, Me);
	Q.AddIgnoredActors(ClimberParts);
	FCollisionObjectQueryParams Everything(ECC_WorldStatic);
	Everything.AddObjectTypesToQuery(ECC_WorldDynamic);
	Scratch.Reset();
	World->LineTraceMultiByObjectType(Scratch, Root + FVector(0.0, 0.0, 3000.0), Root - FVector(0.0, 0.0, 5000.0), Everything, Q);
	bool bTerrain = false;
	double TerrainZ = 0.0;
	for (const FHitResult& H : Scratch)
	{
		if (IsTerrainCached(H))
		{
			TerrainZ = H.ImpactPoint.Z;
			bTerrain = true;
			break;
		}
	}
	// the trees have no collision at all (AForestStreamer): each is walked through; its root should sit at or just under the ground
	const double Gap = Root.Z - TerrainZ;
	const bool bFloating = bTerrain && Gap > RouteCheckFloatCm;
	const FString Mesh = TreeSet.IsValidIndex(J.Tree) && TreeNames.IsValidIndex(TreeSet[J.Tree]) ? TreeNames[TreeSet[J.Tree]] : FString(TEXT("-"));
	FVector Away = (StandPoint - Root).GetSafeNormal2D();
	if (Away.IsNearlyZero())
	{
		Away = FVector(1.0, 0.0, 0.0);
	}
	const FVector Cam = Root + Away * 800.0 + FVector(0.0, 0.0, 200.0);
	ObjRows += FString::Printf(TEXT("%d,%d,%.0f,%.1f,%s,%s,%s,%s,%d,%s,%.0f,%.0f,%.0f,,,,%s,%d,%s,%s,%s,,,%s,0,1,%d,0,%s\n"),
		Cur, P.Leg, P.LegDistM, J.DistM, TEXT("ForestStreamer"), TEXT("ForestStreamer"), TEXT("tree"), *RouteCheckCsv(Mesh), J.Tree, RouteCheckKindNames[RCK_Tree],
		Root.X, Root.Y, Root.Z, bTerrain ? TEXT("root") : TEXT("none"), bTerrain ? 1 : 0,
		*RouteCheckF1(bTerrain, Gap), *RouteCheckF1(bTerrain, Gap), *RouteCheckF1(bTerrain, -Gap), bTerrain ? TEXT("terrain") : TEXT("-"),
		bFloating ? 1 : 0, *RouteCheckCsv(RouteCheckBugItGo(Cam, Root + FVector(0.0, 0.0, 100.0))));
	++PtObjects;
	++TotObjects;
	if (bFloating) { ++PtFloating; ++TotFloating; }
}

void UYggRouteCheck::WritePoint()
{
	const FYggRouteCheckPoint& P = Plan.Points[Cur];
	const float FrameMs = FrameCount > 0 ? FrameSum / FrameCount * 1000.f : 0.f;
	Write(TEXT("points.csv"), PointCols + FString::Printf(TEXT(",%d,%d,%d,%d,%d,%.1f,%.2f,%.2f,%s,%s,%s\n"),
		PtObjects, PtFloating, PtBuried, PtWalkInto, StreamingLeft, SettleSeconds, FrameMs, FrameMax * 1000.f, *ShotName, *BackName, *RouteCheckCsv(PointBugIt)), false);
	if (!ObjRows.IsEmpty())
	{
		Write(TEXT("objects.csv"), ObjRows, false);
		ObjRows.Reset();
	}
	++TotPoints;
	TotIssuePoints += bPointIssue ? 1 : 0;
	Log(FString::Printf(TEXT("point %d/%d leg %d %.0f m: %d objects (%d floating, %d buried, %d walked through)%s, %.1f ms, settled in %.1f s"),
		Cur + 1, Plan.Points.Num(), P.Leg, P.LegDistM, PtObjects, PtFloating, PtBuried, PtWalkInto, bPointIssue ? TEXT(", ground issue") : TEXT(""), FrameMs, SettleSeconds));
}

void UYggRouteCheck::RequestShot(const FString& RelPath)
{
	ClearShot();
	bShotDone = false;
	PendingShot = Dir / RelPath;
	// The engine's own screenshot saves a PNG on the game thread (a 200 ms freeze); the pixels come to OnShotCaptured
	// instead and a worker writes the JPEG. Without the UI: the scene only, no HUD or menus.
	ShotHandle = UGameViewportClient::OnScreenshotCaptured().AddUObject(this, &UYggRouteCheck::OnShotCaptured);
	FScreenshotRequest::RequestScreenshot(PendingShot, false, false);
}

void UYggRouteCheck::ClearShot()
{
	if (ShotHandle.IsValid())
	{
		UGameViewportClient::OnScreenshotCaptured().Remove(ShotHandle);
		ShotHandle.Reset();
	}
	PendingShot.Reset();
}

void UYggRouteCheck::OnShotCaptured(int32 ShotW, int32 ShotH, const TArray<FColor>& Pixels)
{
	const FString File = PendingShot;
	ClearShot();
	bShotDone = true;
	if (ShotW <= 0 || ShotH <= 0 || Pixels.Num() < ShotW * ShotH || !Pipe.IsValid() || !ImageWrappers || File.IsEmpty())
	{
		return;
	}
	const int32 Skip = FMath::Max(1, FMath::DivideAndRoundUp(ShotW, 1280));   // at most 1280 wide: enough to see the scene
	const int32 OW = ShotW / Skip, OH = ShotH / Skip;
	TArray<FColor> Small;
	Small.SetNumUninitialized(OW * OH);
	for (int32 Y = 0; Y < OH; ++Y)
	{
		const FColor* Row = Pixels.GetData() + int64(Y * Skip) * ShotW;
		for (int32 X = 0; X < OW; ++X)
		{
			Small[Y * OW + X] = Row[X * Skip];
		}
	}
	IImageWrapperModule* Mod = ImageWrappers;
	Pipe->Launch(TEXT("YggRouteCheckShot"), [Mod, File, OW, OH, Small = MoveTemp(Small)]()
	{
		const TSharedPtr<IImageWrapper> Jpg = Mod->CreateImageWrapper(EImageFormat::JPEG);
		if (Jpg.IsValid() && Jpg->SetRaw(Small.GetData(), int64(Small.Num()) * sizeof(FColor), OW, OH, ERGBFormat::BGRA, 8))
		{
			FFileHelper::SaveArrayToFile(Jpg->GetCompressed(85), *File);
		}
	});
	++TotShots;
}

void UYggRouteCheck::Tick(float DeltaTime)
{
	if (Step == EYggRouteCheckStep::Quit)
	{
		// the files first (done.txt is the last of them), then out
		if (Pipe.IsValid() && Pipe->HasWork() && FPlatformTime::Seconds() - StepStart < 30.0)
		{
			return;
		}
		Step = EYggRouteCheckStep::Idle;
		FPlatformMisc::RequestExit(false, TEXT("ygg.RouteCheck"));
		return;
	}
	UWorld* World = WorldRef.Get();
	if (!World)
	{
		Stop(TEXT("the level went away"));
		return;
	}
	const double In = FPlatformTime::Seconds() - StepStart;
	const bool bFirst = !bEntered;
	bEntered = true;
	switch (Step)
	{
	case EYggRouteCheckStep::Loading:
	{
		if (!PlanFuture.IsValid())
		{
			Stop(TEXT("the route was never read"));
			return;
		}
		if (!PlanFuture.IsReady())
		{
			break;
		}
		Plan = PlanFuture.Consume();
		if (!Plan.Error.IsEmpty() || Plan.Points.Num() == 0)
		{
			Stop(Plan.Error.IsEmpty() ? FString(TEXT("no route points in the chosen legs")) : Plan.Error);
			return;
		}
		Log(FString::Printf(TEXT("route: %d points on %d legs, %d stretch samples"), Plan.Points.Num(), Plan.LegNames.Num(), Plan.Sub.Num()));
		ToIndex.Reset();
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			ToIndex.Add(*It);
		}
		IndexNext = 0;
		SetStep(EYggRouteCheckStep::Index);
		break;
	}
	case EYggRouteCheckStep::Index:
	{
		// a few hundred actors a frame, so the start does not freeze
		const int32 End = FMath::Min(IndexNext + 300, ToIndex.Num());
		for (; IndexNext < End; ++IndexNext)
		{
			if (AActor* A = ToIndex[IndexNext].Get())
			{
				IndexActor(A);
			}
		}
		if (IndexNext >= ToIndex.Num())
		{
			Log(FString::Printf(TEXT("level: %d actors, %d static meshes, %d instanced meshes, %d detail tiles, %d rope points, %d flags, %d clues, %d checkpoints, %d trees (%d meshes over 150 m left out)"),
				ToIndex.Num(), Statics.Num(), Instanced.Num(), TileBoxes.Num(), RopePts.Num(), FlagPts.Num(), CluePts.Num(), CheckpointPts.Num(), TreePts.Num(), SkippedHuge));
			ToIndex.Empty();
			SetStep(EYggRouteCheckStep::WaitPlay);
		}
		break;
	}
	case EYggRouteCheckStep::WaitPlay:
	{
		ACharacter* Me = GetClimber(World);
		if (!Me || World->GetRealTimeSeconds() < CVarRouteCheckFirstWait.GetValueOnGameThread())
		{
			if (In > 300.0)
			{
				Stop(TEXT("no climber to move after 5 minutes"));
				return;
			}
			break;
		}
		TakePlayer(Me);
		Cur = 0;
		SetStep(EYggRouteCheckStep::Move);
		break;
	}
	case EYggRouteCheckStep::Move:
	{
		ACharacter* Me = GetClimber(World);
		if (!Me)
		{
			Stop(TEXT("the climber went away"));
			return;
		}
		PlaceClimber(World, Me);
		SetStep(EYggRouteCheckStep::Settle);
		break;
	}
	case EYggRouteCheckStep::Settle:
	{
		const double Settle = FMath::Max(0.5, double(CVarRouteCheckSettle.GetValueOnGameThread()));
		if (In > Settle - 1.0)
		{
			// the frame time here once things have settled: a map of what the route costs
			const float Dt = float(FApp::GetDeltaTime());
			FrameSum += Dt;
			FrameMax = FMath::Max(FrameMax, Dt);
			++FrameCount;
		}
		StreamingLeft = IStreamingManager::Get().GetNumWantingResources();
		if (In >= Settle && ((StreamingLeft == 0 && !IsAsyncLoading()) || In >= Settle * 3.0))
		{
			SettleSeconds = In;
			SetStep(EYggRouteCheckStep::Measure);
		}
		break;
	}
	case EYggRouteCheckStep::Measure:
	{
		ACharacter* Me = GetClimber(World);
		if (!Me)
		{
			Stop(TEXT("the climber went away"));
			return;
		}
		MeasurePoint(World, Me);
		GatherObjects();
		JobNext = 0;
		PtObjects = PtFloating = PtBuried = PtWalkInto = 0;
		ObjRows.Reset();
		SetStep(EYggRouteCheckStep::Objects);
		break;
	}
	case EYggRouteCheckStep::Objects:
	{
		// a dozen objects a frame (~200 short traces)
		const AActor* Me = GetClimber(World);
		const int32 End = FMath::Min(JobNext + 12, Jobs.Num());
		for (; JobNext < End; ++JobNext)
		{
			if (Jobs[JobNext].Kind == RCK_Tree)
			{
				MeasureTree(World, Me, Jobs[JobNext]);
			}
			else
			{
				MeasureObject(World, Me, Jobs[JobNext]);
			}
		}
		if (JobNext >= Jobs.Num())
		{
			SetStep(Shots > 0 ? EYggRouteCheckStep::Shot : EYggRouteCheckStep::Next);
		}
		break;
	}
	case EYggRouteCheckStep::Shot:
	case EYggRouteCheckStep::ShotBack:
	{
		const bool bBack = Step == EYggRouteCheckStep::ShotBack;
		FString& Name = bBack ? BackName : ShotName;
		if (bFirst)
		{
			const FYggRouteCheckPoint& P = Plan.Points[Cur];
			Name = FString::Printf(TEXT("shots/%03d_leg%d_%05.0fm%s.jpg"), Cur, P.Leg, P.LegDistM, bBack ? TEXT("_back") : TEXT(""));
			RequestShot(Name);
			break;
		}
		if (!bShotDone && In < 5.0)
		{
			break;
		}
		if (!bShotDone)
		{
			Log(FString::Printf(TEXT("point %d: no picture came back (HDR output?)"), Cur));
			Name.Reset();
		}
		ClearShot();
		SetStep(!bBack && Shots >= 2 ? EYggRouteCheckStep::TurnBack : EYggRouteCheckStep::Next);
		break;
	}
	case EYggRouteCheckStep::TurnBack:
	{
		if (bFirst)
		{
			Look(GetClimber(World), true);
		}
		if (In >= 1.0)
		{
			SetStep(EYggRouteCheckStep::ShotBack);   // a second for the picture to settle after the turn (TSR, streaming)
		}
		break;
	}
	case EYggRouteCheckStep::Next:
	{
		WritePoint();
		++Cur;
		if (Cur >= Plan.Points.Num())
		{
			Finish(TEXT("done"));
			return;
		}
		SetStep(EYggRouteCheckStep::Move);
		break;
	}
	default:
		break;
	}
}
