// The view rig (2026-11-19, Docs/Development/Chapter1_Camera_Spec.md "Eyes on a Body"): the ONE writer of the first-person
// view. The story, the body effects and gameplay ask it; nobody else moves the camera, the control rotation or the view
// target (rule R1). It owns the eye position (terrain spring, sink, slope drop, neck, waist, step bob locked to the
// footstep sound, breathing, landing), the postures (lie, sit, kneel, crawl: no animation clips), the leases every story
// hold is made of (with a maximum lifetime, so nothing can stay stuck), the watchdog that puts a broken view back to
// plain walking, the eyelids (drawn by the story overlay), the scripted takeovers (fall, knock, the fly-out), inspect,
// hands and the walk-speed shaping. UYggViewModifier (same file) is its input and output side on the camera manager.
// Console: ygg.View.Audit, ygg.View.Rig 0 (old socket camera for A/B), ygg.View.* switches, stat YggView.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Camera/CameraModifier.h"
#include "Engine/EngineTypes.h"
#include "Engine/Scene.h"
#include "YggViewRig.generated.h"

class ACharacter;
class ACameraActor;
class APlayerCameraManager;
class APlayerController;
class UCameraComponent;
class UStaticMesh;
class UStaticMeshComponent;
class UTexture2D;
class UYggViewModifier;
class UFirstPersonBodyComponent;
class UMountainAudioComponent;
class USurvivalComponent;
class USnowTrailComponent;
class UWorldPartitionStreamingSourceComponent;
struct FMinimalViewInfo;

/** The view state: walking, or one of the postures (spec 4.1, 4.3). */
UENUM()
enum class EYggView : uint8 { Walk, Lie, Sit, Kneel1, Kneel2, Crawl };

/** A takeover of the view by the story (spec 6.8-6.10). */
UENUM()
enum class EYggScripted : uint8 { None, Fall, Knock, Fly };

/** Everything the story can hold (spec 4.4). */
UENUM()
enum class EYggLease : uint8 { Move, Look, Pace, Fall, Knock, Fly, Inspect, Hands, Attention, FovBeat, StoryFov, Nudge, Lids, Breath, Exposure, Lens };

/** Eyelid moves (spec 6.6). */
UENUM()
enum class EYggLidMode : uint8 { Blink, Closed, Open, Wake, Sleep };

/** One posture of the table in DefaultGame.ini (+Poses=...): the eye over the floor, its band, how long down and up take,
 *  the soft look limits around the posture's anchor yaw, and the prop shown while posed. */
USTRUCT()
struct FYggPoseDef
{
	GENERATED_BODY()

	/** stand, lie, sit, kneel1, kneel2, crawl */
	UPROPERTY() FName Name;
	UPROPERTY() float EyeCm = 156.f;
	/** Half width of the allowed eye band around EyeCm (cm). */
	UPROPERTY() float BandCm = 6.f;
	UPROPERTY() float DownS = 1.f;
	UPROPERTY() float UpS = 1.f;
	/** Yaw limit, +- degrees around the anchor (180 = free). */
	UPROPERTY() float YawHalf = 180.f;
	UPROPERTY() float PitchMin = -75.f;
	UPROPERTY() float PitchMax = 80.f;
	/** The standing body is hidden in this posture (FirstPersonBodyComponent::SetPostureView decides what shows). */
	UPROPERTY() bool bHideBody = false;
	/** Actor tag of a prop shown while posed (the sleeping bag); the story's "prop" overrides it. */
	UPROPERTY() FName Prop;
};

/** Where raised story hands rest (spec 9, view space cm: X forward, Y right, Z up; zero = that hand stays down). */
USTRUCT()
struct FYggHandsDef
{
	GENERATED_BODY()

	UPROPERTY() FName Name;
	UPROPERTY() FVector Left = FVector::ZeroVector;
	UPROPERTY() FVector Right = FVector::ZeroVector;
};

/** A story pose request (`pose` action, spec 7.1). */
USTRUCT()
struct FYggPoseReq
{
	GENERATED_BODY()

	UPROPERTY() EYggView Pose = EYggView::Walk;
	/** Transition length; 0 or less = the posture's own down / up time. */
	UPROPERTY() float Seconds = -1.f;
	/** Body and look-limit anchor (world yaw); unset = the current view yaw. */
	TOptional<float> FaceYaw;
	/** Instant (only behind closed eyelids, or on a camera-cut frame; otherwise played and logged). */
	UPROPERTY() bool bSnap = false;
	/** Wait for the player's first move or look input (or Auto seconds) before starting. */
	UPROPERTY() bool bAfterInput = false;
	/** With bAfterInput: start anyway after this many seconds (0 = only on input). */
	UPROPERTY() float Auto = 0.f;
	/** Sit -> Crawl: the place the crawl goes to (tent_out); moving while sitting crawls there instead of standing. */
	TOptional<FVector> Exit;
	/** Exposure overshoot (EV) when standing up at the exit (the dawn reveal, +0.4). */
	UPROPERTY() float ExitEv = 0.f;
	/** Actor tag shown while posed (none = the posture table's prop). */
	UPROPERTY() FName Prop;
};

/** An object held up to the eye (`inspect`, spec 6.11). The rig keeps the state and turns the object with the mouse;
 *  the story overlay draws the Front / Back card and the captions; a Mesh is shown by the rig itself in 3D. */
USTRUCT()
struct FYggInspect
{
	GENERATED_BODY()

	UPROPERTY() FName Id;
	UPROPERTY() TObjectPtr<UTexture2D> Front = nullptr;
	UPROPERTY() TObjectPtr<UTexture2D> Back = nullptr;
	UPROPERTY() FText Text;
	UPROPERTY() FText BackText;
	/** Cannot be put away until its back has faced the eye. */
	UPROPERTY() bool bNeedBack = false;
	UPROPERTY() TObjectPtr<UStaticMesh> Mesh = nullptr;
};

UCLASS(Config=Game, ClassGroup=(Survival))
class YGGDRASILSPEAK_API UYggViewRigComponent : public UActorComponent
{
	GENERATED_BODY()

	friend class UYggViewModifier;

public:
	UYggViewRigComponent();

	/** The rig of Ctx's pawn (an actor, a component on it) or of the first local player of Ctx's world; null if none. */
	static UYggViewRigComponent* Find(const UObject* Ctx);
	/** Creates and registers the rig on a locally controlled character (FirstPersonBodyComponent::BeginPlay calls it) and
	 *  adds the UYggViewModifier to its camera manager; returns the existing one when there is one. */
	static UYggViewRigComponent* EnsureFor(ACharacter* Character);

	// ---- gameplay feeds

	/** Body effects (UExpeditionComponent, every frame): Shiver, Exhaust, Hypoxia 0..1, Beat = the raw lub-dub 0..1, PackKg. */
	void SetBody(float Shiver, float Exhaust, float Beat, float Hypoxia, float PackKg);
	/** A footstep sound just played (USnowTrailComponent::OnStep): the bob's phase locks to it. */
	void OnStep(bool bLeft, float SpeedCmS, uint8 Surface);
	UFUNCTION() void OnLanded(const FHitResult& Hit);
	/** A named exposure term in EV (e.g. "sanity"); 0 removes it. Summed into the rig's one exposure channel. */
	void SetExposureBias(FName Source, float Ev);
	/** A decaying tremor of the view (MountainRumble): Deg at the start, gone after Seconds. */
	void AddShake(float Deg, float Seconds);
	/** SurvivalComponent's per-tick walk speed goes through here: 0 when posed / held / inspecting / in a takeover, the
	 *  crawl speed in Crawl, capped by a Pace lease. */
	float ShapeWalkSpeed(float Target) const;

	// ---- story (Owner = the beat index; -1 = posture system, -2 = fallbacks)

	void RequestPose(int32 Owner, const FYggPoseReq& R);
	EYggView GetView() const { return View; }
	/** Eye height over the floor (capsule bottom minus the floor gap), cm. */
	float GetEyeAboveFloor() const { return EyeAboveFloor; }
	/** 0 = open, 1 = closed (the story overlay draws the lids from this). */
	float GetLid() const { return Lid; }

	/** Returns the lease id. MaxSec <= 0 = the kind's own maximum (spec 4.4); larger values are capped by it. Taking a lease
	 *  this owner already holds refreshes it (keep is sticky). */
	int32 AcquireLease(int32 Owner, EYggLease Kind, float MaxSec, bool bKeep = false);
	void ReleaseLease(int32 Owner, EYggLease Kind);
	/** A beat's run ended: everything it held, except "keep" leases. */
	void ReleaseOwner(int32 Owner);
	/** Everything, keep leases too; story FOV / attention / exposure fade over Blend seconds. Usually followed by HardReset. */
	void ReleaseAll(float Blend);

	/** The last-resort pull of the gaze (spec 6.2). False = over budget, too fast or Gentle: the caller lures instead. */
	bool Nudge(int32 Owner, const FVector& Target, float MaxYaw, float Rate, float PitchMax);
	/** Orienting reflex to a sudden loud sound: Deg toward Source over 0.3 s, only 30-120 deg off-centre, 20 s cooldown. */
	void Reflex(const FVector& Source, float Deg);
	/** While a hand is held: a yaw bias of at most Rate deg/s toward Toward's heading (null / 0 = off). */
	void SetHandPull(TWeakObjectPtr<AActor> Toward, float Rate);
	/** Eyelids. T = close time (Blink, Closed, Sleep) or open time (Open, Wake); Hold = Blink's closed hold. OnClosed runs on
	 *  the first fully closed frame (a teleport, a pose snap, spawns behind the lids). */
	void SetLids(int32 Owner, EYggLidMode Mode, float T, float Hold, TFunction<void()> OnClosed);
	/** The prologue slip (spec 6.8). Side: -1 left, +1 right (toward the drop). */
	void Fall(int32 Owner, float Seconds, float Side);
	/** The risky porter pass (spec 6.9). Side: +1 thrown to the right (yaw +15, roll +8), -1 to the left. */
	void Knock(int32 Owner, float Side);
	/** The one cinematic (spec 6.10): FlyCam along Path, looking along Look, T seconds, FOV Fov.X -> Fov.Y, the lids open
	 *  over Open seconds once the camera has cut to it. */
	void FlyOut(int32 Owner, const TArray<FVector>& Path, const TArray<FVector>& Look, float T, FVector2D Fov, float Open);
	/** Story FOV term, clamped to +-3 deg, eased over at least 1.5 s; 0 eases out and lets the lease go. */
	void SetStoryFov(int32 Owner, float Deg, float T);
	void SetStoryFov(int32 Owner, float Deg, float T, bool bKeep);
	/** Attention zoom: only while the player is still (spec 6.13); released over 1.2 s on any input. */
	void Attention(int32 Owner, float Deg, float T);
	/** The heartbeat push (once per chapter: the plume). Off with Heartbeat zoom Off. */
	void FovBeat(int32 Owner, float Deg, float Bpm, float T);
	/** Exposure overshoot: up to Ev over T, held Hold seconds, then back (tau T). */
	void StoryExposure(int32 Owner, float Ev, float T, float Hold);
	/** The breath catches: forces the breathing value B for T seconds. */
	void StoryBreath(int32 Owner, float B, float T);
	void BeginInspect(int32 Owner, const FYggInspect& I);
	bool IsInspecting() const { return bInspect; }
	/** Story hands (compass, photo, bottle, khata, dolma); NAME_None = hands down. */
	void SetHands(int32 Owner, FName Item);
	/** Walk cap in m/s as a lease; Ms <= 0 releases it. T <= 0 = until released (600 s at most). */
	void SetPace(int32 Owner, float Ms, float T, bool bKeep);
	/** Every teleport, fly-out end, JumpTo, Stop, death, continue: springs snapped, posture set (snap), takeovers 0, time
	 *  dilation and audio pitch 1, view target the pawn and the pawn shown, plane constraint off, story FOV 0, backstops
	 *  and the controller-yaw flag restored. The control rotation is set to Facing (pitch normalised, roll 0). The
	 *  eyelids are NOT opened: the caller decides. */
	void HardReset(const FRotator& Facing, EYggView Pose = EYggView::Walk);
	/** One multi-line report of the rig (ygg.View.Audit). */
	FString Audit() const;

	virtual void TickComponent(float Dt, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override; // TG_PostPhysics

	// ---- extra read-outs (for the story overlay, the session monitor, the body)

	EYggScripted GetScripted() const { return Scripted; }
	/** Standing eye minus the posture's eye now (cm, 0 when standing): how far the first-person mesh is lowered. */
	float GetPostureDrop() const;
	bool IsPosed() const { return View != EYggView::Walk || PoseH < StandEyeCm - 1.f; }
	bool HasLease(EYggLease Kind) const;
	bool IsNudging() const { return bNudge; }
	/** Bit (1 << EYggLease) per lease held (the session monitor's leases column). */
	uint32 GetLeaseMask() const;
	/** Nudges spent in this chapter (budget NudgeBudget). */
	int32 GetNudgesUsed() const { return NudgesUsed; }
	/** A new chapter starts with a fresh nudge budget (the story's Start calls this). */
	void ResetChapterBudget();
	/** Current story-side FOV terms summed (deg), for the monitor. */
	float GetStoryFovTotal() const { return StoryFov + AttnValue + FovBeatValue; }
	/** The inspect state for the overlay: the object, how far it is raised (0..1), its turn (pitch, yaw) and whether its
	 *  back is toward the eye (then show Back / BackText). */
	const FYggInspect& GetInspect() const { return Insp; }
	float GetInspectAlpha() const { return InspAlpha; }
	FRotator GetInspectRotation() const { return FRotator(InspPitch, InspYaw, 0.f); }
	bool IsInspectBackFacing() const;
	bool CanCloseInspect() const { return !Insp.bNeedBack || bBackSeen; }
	FText GetInspectCaption() const;
	FName GetHandsItem() const { return HandsItem; }
	/** A camera cut happened this frame or the last (teleport, fly-out start / end): for the monitor's `cut` column. */
	bool WasCutRecently() const;

	// ---- tunables (DefaultGame.ini [/Script/YggdrasilsPeak.YggViewRigComponent], spec 9)

	// eye, terrain, neck
	UPROPERTY(Config) float StandEyeCm = 156.f;
	UPROPERTY(Config) float SinkToEye = 1.f;
	/** At most this much of the snow sink lowers the eye (cm). */
	UPROPERTY(Config) float SinkEyeMaxCm = 10.f;
	UPROPERTY(Config) float FootDropToEye = 0.5f;
	UPROPERTY(Config) float FootDropMaxCm = 10.f;
	UPROPERTY(Config) float FootDropTauDown = 0.5f;
	UPROPERTY(Config) float FootDropTauUp = 1.2f;
	UPROPERTY(Config) float TerrainSpringHz = 1.8f;
	UPROPERTY(Config) float TerrainMaxLagCm = 15.f;
	UPROPERTY(Config) float NeckFwdCm = 8.f;
	UPROPERTY(Config) float NeckUpCm = 7.5f;
	UPROPERTY(Config) float NeckScale = 0.8f;
	/** Waist lean from / full at this many degrees below level. */
	UPROPERTY(Config) float WaistLeanFromDeg = 45.f;
	UPROPERTY(Config) float WaistLeanFullDeg = 75.f;
	UPROPERTY(Config) float WaistLeanFwdCm = 8.f;
	UPROPERTY(Config) float WaistLeanDownCm = 3.5f;
	UPROPERTY(Config) float EyeHorizMaxCm = 24.f;
	UPROPERTY(Config) float EyeFloorMinCm = 20.f;
	UPROPERTY(Config) float EyeFloorMinTakeoverCm = 30.f;
	UPROPERTY(Config) float EyeTopMarginCm = 5.f;
	UPROPERTY(Config) float StandWatchdogDropCm = 36.f;
	UPROPERTY(Config) float StandWatchdogSpeedCmS = 60.f;
	UPROPERTY(Config) float StandWatchdogSec = 0.5f;
	UPROPERTY(Config) float BackstopPitchMin = -78.f;
	UPROPERTY(Config) float BackstopPitchMax = 82.f;
	UPROPERTY(Config) float AbsPitchMin = -80.f;
	UPROPERTY(Config) float AbsPitchMax = 82.f;
	UPROPERTY(Config) float RollMaxDeg = 2.f;
	UPROPERTY(Config) float RollMaxFallDeg = 30.f;
	UPROPERTY(Config) float RollMaxKnockDeg = 10.f;

	// postures
	UPROPERTY(Config) TArray<FYggPoseDef> Poses;
	/** Share of the height change done at 60% of the time (the S-curve). */
	UPROPERTY(Config) float PoseCurveAt60 = 0.7f;
	UPROPERTY(Config) float PoseSettleCm = 1.5f;
	UPROPERTY(Config) float GazeBendDeg = -6.f;
	UPROPERTY(Config) float PoseEnterMaxSpeedCmS = 20.f;
	UPROPERTY(Config) float BodyTurnToFaceSec = 0.6f;
	UPROPERTY(Config) float StandOnMoveHoldSec = 0.3f;
	UPROPERTY(Config) float WalkResumeFrac = 0.85f;
	UPROPERTY(Config) float StandBodyTurnDegS = 360.f;
	UPROPERTY(Config) float StandBodyTurnMaxSec = 0.5f;
	UPROPERTY(Config) float LimitTweenSec = 1.f;
	/** A limit tween never drags the view faster than this (deg/s, peak): longer tweens for bigger gaps. */
	UPROPERTY(Config) float LimitDragMaxDegS = 45.f;
	UPROPERTY(Config) float SoftEdgeDeg = 12.f;
	UPROPERTY(Config) float SitShuffleAfterSec = 0.6f;
	UPROPERTY(Config) float SitShuffleDegS = 60.f;
	UPROPERTY(Config) float CrawlSpeedCmS = 40.f;
	UPROPERTY(Config) float CrawlDipCm = 0.8f;
	UPROPERTY(Config) float CrawlDipEveryCm = 40.f;
	UPROPERTY(Config) float CrawlExitRadiusCm = 60.f;
	UPROPERTY(Config) float CrawlFallbackSec = 90.f;
	UPROPERTY(Config) float CrawlStandSec = 1.4f;
	UPROPERTY(Config) float CrawlExitEvTau = 2.f;

	// bob (tables by ground speed, cm/s -> peak-to-peak cm / roll deg)
	UPROPERTY(Config) TArray<float> BobSpeedCmS;
	UPROPERTY(Config) TArray<float> BobVerticalCm;
	UPROPERTY(Config) TArray<float> BobLateralCm;
	UPROPERTY(Config) TArray<float> BobRollDeg;
	UPROPERTY(Config) float BobTroughPhase = 0.07f;
	UPROPERTY(Config) float BobPeriodMin = 0.3f;
	UPROPERTY(Config) float BobPeriodMax = 1.1f;
	UPROPERTY(Config) float BobMinStepGap = 0.2f;
	UPROPERTY(Config) float BobFallbackAfterSec = 0.8f;
	UPROPERTY(Config) float BobFallbackSpeedCmS = 30.f;
	/** Fallback steps: one every half of this stride (cm of travel). */
	UPROPERTY(Config) float BobFallbackStrideCm = 130.f;
	UPROPERTY(Config) float BobDecayAfterPeriods = 1.2f;
	UPROPERTY(Config) float BobDecayHalfLife = 0.25f;
	UPROPERTY(Config) float RestStepBelowCmS = 45.f;
	UPROPERTY(Config) float RestStepDipCm = 1.2f;
	UPROPERTY(Config) float RestStepDipSec = 0.12f;
	UPROPERTY(Config) float RestStepHoldSec = 0.25f;
	UPROPERTY(Config) float RestStepRiseSec = 0.35f;
	UPROPERTY(Config) float BobJitter = 0.15f;
	UPROPERTY(Config) float BobJitterFatigue = 0.75f;
	UPROPERTY(Config) float DownhillFromDeg = 8.f;
	UPROPERTY(Config) float DownhillDipScale = 1.3f;
	UPROPERTY(Config) float DownhillNodDeg = 0.4f;
	UPROPERTY(Config) float RockDipMinCm = 0.5f;
	UPROPERTY(Config) float RockDipMaxCm = 1.f;
	/** OnStep's Surface value that means rock / rubble (EYggStepSurface in SnowTrailComponent.h: 0 snow, 1 ice, 2 rock, 3 ladder). */
	UPROPERTY(Config) int32 RockSurfaceId = 2;
	UPROPERTY(Config) float FixationCm = 1000.f;
	UPROPERTY(Config) float StartStopNodDeg = 0.4f;
	UPROPERTY(Config) float StartStopOmega = 9.f;
	UPROPERTY(Config) float StartStopZeta = 0.7f;
	UPROPERTY(Config) float StartStopSpeedCmS = 30.f;

	// breathing and body
	UPROPERTY(Config) float AltGainFromM = 3000.f;
	UPROPERTY(Config) float AltGainRangeM = 5400.f;
	UPROPERTY(Config) float BreathFloorMax = 0.6f;
	UPROPERTY(Config) float BreathFloorFromM = 7000.f;
	UPROPERTY(Config) float BreathFloorRangeM = 1400.f;
	UPROPERTY(Config) float BreathRiseTau = 5.f;
	UPROPERTY(Config) float BreathFallTau = 25.f;
	UPROPERTY(Config) float BreathThreshold = 0.15f;
	UPROPERTY(Config) float BreathIdlePitchDeg = 0.06f;
	UPROPERTY(Config) float BreathIdleZCm = 0.2f;
	UPROPERTY(Config) float BreathIdleHz = 0.25f;
	UPROPERTY(Config) float BreathHzBase = 0.45f;
	UPROPERTY(Config) float BreathHzPerB = 0.3f;
	UPROPERTY(Config) float BreathPitchPerB = 0.55f;
	UPROPERTY(Config) float BreathZPerB = 0.5f;
	UPROPERTY(Config) float ExhaustHangPitchDeg = -3.f;
	UPROPERTY(Config) float ExhaustHangCm = 3.f;
	UPROPERTY(Config) float ExhaustTau = 4.f;
	UPROPERTY(Config) float ExhaustSwayDeg = 0.35f;
	UPROPERTY(Config) float ShiverDeg = 0.12f;
	UPROPERTY(Config) float ShiverHzMin = 5.f;
	UPROPERTY(Config) float ShiverHzMax = 8.f;
	UPROPERTY(Config) float ShiverFpsFrac = 0.25f;
	UPROPERTY(Config) float HeartFovDeg = 0.4f;
	UPROPERTY(Config) float HeartFovFromM = 7000.f;

	// landing, slope, pack, turning
	UPROPERTY(Config) float LandThresholdCmS = 150.f;
	UPROPERTY(Config) float LandDipPer100 = 1.5f;
	UPROPERTY(Config) float LandDipMaxCm = 10.f;
	UPROPERTY(Config) float LandDownSec = 0.1f;
	UPROPERTY(Config) float LandOmega = 14.f;
	UPROPERTY(Config) float LandZeta = 0.65f;
	UPROPERTY(Config) float LandSlowAboveM = 5000.f;
	UPROPERTY(Config) float LandSlowFactor = 1.3f;
	UPROPERTY(Config) float LandNodPerCm = 0.25f;
	UPROPERTY(Config) float LandNodMaxDeg = 2.f;
	UPROPERTY(Config) float SlopeFromDeg = 8.f;
	UPROPERTY(Config) float SlopeFullDeg = 20.f;
	UPROPERTY(Config) float SlopeCapDeg = 25.f;
	UPROPERTY(Config) float SlopeUpFwdCm = 6.f;
	UPROPERTY(Config) float SlopeUpDownCm = 4.f;
	UPROPERTY(Config) float SlopeDownBackCm = 2.5f;
	UPROPERTY(Config) float SlopeTau = 1.75f;
	UPROPERTY(Config) float PackFromKg = 3.f;
	UPROPERTY(Config) float PackCmPerKg = 0.25f;
	UPROPERTY(Config) float PackMaxCm = 6.f;
	UPROPERTY(Config) float PackTau = 1.5f;
	UPROPERTY(Config) float PackLandFromKg = 15.f;
	UPROPERTY(Config) float PackLandPerKg = 0.02f;
	UPROPERTY(Config) float TurnLeanK = 0.f;
	UPROPERTY(Config) float TurnLeanMaxDeg = 0.8f;
	UPROPERTY(Config) float TurnLeanTau = 0.15f;
	UPROPERTY(Config) float DeadZoneSpeedCmS = 10.f;
	UPROPERTY(Config) float DeadZoneDeg = 45.f;
	UPROPERTY(Config) float DeadZoneCatchUpDegS = 240.f;

	// FOV and screen
	UPROPERTY(Config) float StoryFovMaxDeg = 3.f;
	UPROPERTY(Config) float StoryFovMinEase = 1.5f;
	/** All rig FOV terms together stay within the settings FOV +- this. */
	UPROPERTY(Config) float ViewFovMaxDeg = 5.f;
	UPROPERTY(Config) float AttentionStillCmS = 20.f;
	UPROPERTY(Config) float AttentionStillDegS = 5.f;
	UPROPERTY(Config) float AttentionStillSec = 1.2f;
	UPROPERTY(Config) float AttentionReleaseSec = 1.2f;
	UPROPERTY(Config) float FovBeatPerBeat = 0.6f;
	UPROPERTY(Config) float FovBeatIn = 1.5f;
	UPROPERTY(Config) float FovBeatOut = 2.f;
	UPROPERTY(Config) float FovBeatCap = 3.f;
	UPROPERTY(Config) float InspectFovDeg = -3.f;
	UPROPERTY(Config) float SteadyFromDeg = 20.f;
	UPROPERTY(Config) float SteadyFullDeg = 40.f;
	UPROPERTY(Config) float SteadyExposureSpeed = 0.05f;
	UPROPERTY(Config) float WakeEv = 0.5f;
	UPROPERTY(Config) float WakeEvTau = 1.5f;
	UPROPERTY(Config) float InspectDarken = 0.25f;
	UPROPERTY(Config) float InspectDesaturate = 0.2f;

	// story guidance, eyelids, takeovers, inspect
	UPROPERTY(Config) float NudgeK = 1.4f;
	UPROPERTY(Config) float NudgePitchRate = 8.f;
	UPROPERTY(Config) float NudgeRamp = 0.4f;
	UPROPERTY(Config) float NudgeMaxSec = 3.f;
	UPROPERTY(Config) float NudgeYieldDegS = 30.f;
	UPROPERTY(Config) float NudgeCancelDegS = 60.f;
	UPROPERTY(Config) float NudgeCancelSec = 0.1f;
	UPROPERTY(Config) float NudgeDoneYaw = 15.f;
	UPROPERTY(Config) float NudgeDonePitch = 10.f;
	UPROPERTY(Config) int32 NudgeBudget = 5;
	UPROPERTY(Config) float NudgeGapSec = 60.f;
	UPROPERTY(Config) float NudgeMaxSpeedCmS = 150.f;
	UPROPERTY(Config) float ReflexSec = 0.3f;
	UPROPERTY(Config) float ReflexMinOffDeg = 30.f;
	UPROPERTY(Config) float ReflexMaxOffDeg = 120.f;
	UPROPERTY(Config) float ReflexCooldownSec = 20.f;
	UPROPERTY(Config) float HandPullK = 1.f;
	UPROPERTY(Config) float BlinkCloseSec = 0.15f;
	UPROPERTY(Config) float BlinkOpenSec = 0.6f;
	/** A fully closed lid whose lease ended stays shut this long (s, hitches not counted) so the next beat can take over. */
	UPROPERTY(Config) float LidGraceSec = 2.5f;
	UPROPERTY(Config) float FallAudioPitch = 0.8f;
	UPROPERTY(Config) float KnockSec = 0.14f;
	UPROPERTY(Config) float KnockBackSec = 0.5f;
	UPROPERTY(Config) float KnockYawDeg = 15.f;
	UPROPERTY(Config) float KnockRollDeg = 8.f;
	UPROPERTY(Config) float KnockDownCm = 20.f;
	UPROPERTY(Config) float KnockPitchDeg = -10.f;
	UPROPERTY(Config) float FlyMaxTurnDegS = 12.f;
	UPROPERTY(Config) float FlyCloseSec = 2.f;
	UPROPERTY(Config) float InspectDistCm = 40.f;
	UPROPERTY(Config) float InspectBelowDeg = 18.f;
	UPROPERTY(Config) float InspectRiseSec = 0.3f;
	UPROPERTY(Config) float InspectPocketSec = 0.25f;
	UPROPERTY(Config) float InspectMouse = 0.35f;
	UPROPERTY(Config) float InspectPitchMax = 60.f;
	UPROPERTY(Config) float InspectBackDot = -0.3f;
	UPROPERTY(Config) float InspectBackSec = 0.5f;
	UPROPERTY(Config) TArray<FYggHandsDef> Hands;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	enum class EPhase : uint8 { Holding, Entering, Leaving };
	enum class EInspPhase : uint8 { Rising, Open, Pocket };

	struct FLease
	{
		int32 Id = 0;
		int32 Owner = 0;
		EYggLease Kind = EYggLease::Move;
		double Start = 0.0;
		float MaxSec = 0.f;
		bool bKeep = false;
		/** Ends on its own clock by design (a timed pace): no "expired" warning. */
		bool bQuiet = false;
	};

	struct FExposurePulse
	{
		float Ev = 0.f, Rise = 0.f, Hold = 0.f, Tau = 1.f, T = 0.f;
		/** A story overshoot: holds only while its Exposure lease lives. */
		bool bStory = false;
	};

	// ---- links
	TWeakObjectPtr<ACharacter> Char;
	UPROPERTY(Transient) TObjectPtr<UCameraComponent> Camera;
	UPROPERTY(Transient) TObjectPtr<UYggViewModifier> Modifier;
	TWeakObjectPtr<APlayerCameraManager> ModPCM;
	TWeakObjectPtr<USceneComponent> OldCamParent;
	FName OldCamSocket;
	FTransform OldCamRelative = FTransform::Identity;
	bool bCamReparented = false;
	bool bRigOn = false;
	float SavedPitchMin = -70.f, SavedPitchMax = 80.f;
	bool bBackstopSet = false;
	TWeakObjectPtr<UFirstPersonBodyComponent> Body;
	TWeakObjectPtr<UMountainAudioComponent> Audio;
	TWeakObjectPtr<USurvivalComponent> Survival;
	TWeakObjectPtr<USnowTrailComponent> Snow;
	TWeakObjectPtr<UActorComponent> Climbing;
	bool bStepBound = false;
	double NextLookup = 0.0;

	// ---- clocks (Clock: real seconds while the game runs, so leases and story timing ignore slow motion and pauses)
	double Clock = 0.0;
	float RealDt = 0.f;

	// ---- feeds of this frame
	float Speed2D = 0.f;
	bool bOnGround = true;
	float GradeDeg = 0.f;
	float AltM = 4400.f;
	float FeedShiver = 0.f, FeedExhaust = 0.f, FeedBeat = 0.f, FeedHyp = 0.f, FeedPackKg = 0.f;
	float HeadMotion = 1.f;
	bool bGentle = false;
	bool bHeartZoom = true;
	double NextSettingsRead = 0.0;

	// ---- eye
	FVector Eye = FVector::ZeroVector;
	bool bEyeValid = false;
	float EyeAboveFloor = 156.f;
	float TerrainZ = 0.f, TerrainV = 0.f;
	bool bTerrainInit = false;
	float FootDropSm = 0.f;
	float SlopeFwdSm = 0.f, SlopeDownSm = 0.f, PackStoop = 0.f, ExhaustHang = 0.f;
	float WatchdogLift = 0.f, LowTime = 0.f;
	/** The eye over the capsule bottom before the watchdog's lift (the knee-height detector reads it). */
	float EyeAboveCapRaw = 156.f;
	bool bWatchdogTripped = false;

	// ---- posture
	EYggView View = EYggView::Walk;
	EPhase Phase = EPhase::Holding;
	float PoseH = 156.f, PoseFrom = 156.f, PoseTo = 156.f, PoseT = 0.f, PoseLen = 0.f;
	float PoseCurveExp = 0.883f;
	float AnchorYaw = 0.f;
	float GazeBend = 0.f;
	bool bBodyTurning = false;
	float BodyTurnFrom = 0.f, BodyTurnTo = 0.f, BodyTurnT = 0.f;
	bool bCatchUp = false;
	float CatchUpT = 0.f, CatchUpRate = 360.f;
	bool bSavedCtrlYaw = true;
	bool bCtrlYawOwned = false;
	bool bDeadZoneTurning = false;
	float LimPitchMin = -75.f, LimPitchMax = 80.f, LimYawHalf = 180.f;
	float LimFromMin = -75.f, LimFromMax = 80.f, LimFromYaw = 180.f;
	float LimToMin = -75.f, LimToMax = 80.f, LimToYaw = 180.f;
	float LimT = 0.f, LimLen = 0.f;
	bool bPending = false;
	FYggPoseReq Pending;
	int32 PendingOwner = -1;
	double PendingSince = 0.0;
	bool bHasExit = false;
	FVector ExitPos = FVector::ZeroVector;
	float ExitEv = 0.f;
	FName PoseProp;
	FName ShownProp;
	FVector CrawlDir = FVector::ForwardVector;
	double CrawlSince = 0.0;
	float CrawlTravel = 0.f;
	bool bCrawlFallback = false;
	float MoveHeld = 0.f;
	double LastLookInput = -100.0;
	float LastInputRate = 0.f;
	float ShuffleTime = 0.f;

	// ---- leases
	TArray<FLease> Leases;
	int32 NextLeaseId = 1;
	int32 LastOwner[16] = {};

	// ---- step bob
	double LastStepTime = -100.0;
	double LastRealStepTime = -100.0;
	/** The bob's running phase in steps (even whole numbers = left plants) and the whole step the last footstep locked to. */
	double BobPsi = 0.0;
	double StepBase = 0.0;
	float StepPeriod = 0.5f;
	float StepA = 0.f, StepL = 0.f, StepR = 0.f;
	float BobAmp = 0.f;
	float RestW = 0.f;
	float FallbackDist = 0.f;
	float RockDipCm = 0.f;
	double RockDipAt = -100.0;
	float BobZ = 0.f, BobY = 0.f, BobRoll = 0.f, BobPitch = 0.f;
	float NodX = 0.f, NodV = 0.f;
	bool bWasMoving = false;

	// ---- landing
	float LandZ = 0.f, LandV = 0.f, LandStart = 0.f, LandTarget = 0.f, LandDownT = -1.f;
	float LastFallVz = 0.f;
	double LastLandTime = -100.0;

	// ---- breathing, exhaustion, shiver, shake, turn lean
	float BSmooth = 0.f, BPhase = 0.f, BFreq = 0.25f, BAmpPitch = 0.06f, BAmpZ = 0.2f;
	float StoryB = -1.f;
	float BreathPitch = 0.f, BreathZ = 0.f;
	float SwayP = 0.f, SwayY = 0.f, ShivP = 0.f, ShivY = 0.f, ShakeP = 0.f, ShakeY = 0.f;
	float SwayClock = 0.f;
	float ShivPhase[4] = {};
	float ShivFreq[2] = { 6.f, 7.f };
	float ShakeDeg = 0.f, ShakeLen = 0.f, ShakeT = 0.f;
	float LastCtrlYaw = 0.f;
	bool bHasLastCtrlYaw = false;
	float TurnRate = 0.f, TurnLean = 0.f;
	float StillTime = 0.f;
	float ViewPitchOff = 0.f, ViewYawOff = 0.f, ViewRollOff = 0.f;

	// ---- FOV terms and exposure
	float StoryFov = 0.f, StoryFovFrom = 0.f, StoryFovTo = 0.f, StoryFovT = 0.f, StoryFovLen = 0.f;
	int32 StoryFovOwner = -1;
	float AttnValue = 0.f, AttnTarget = 0.f, AttnIn = 4.f, AttnLeft = 0.f;
	int32 AttnOwner = -1;
	bool bAttn = false;
	float FovBeatValue = 0.f, FovBeatT = 0.f, FovBeatDeg = 0.f, FovBeatBpm = 96.f, FovBeatHold = 7.f;
	int32 FovBeatOwner = -1;
	bool bFovBeat = false;
	/** >= 0: the beat was cut short and goes out from this envelope value. */
	float FovBeatOutFrom = -1.f;
	float HeartFov = 0.f;
	float BlendOutSec = 1.f;
	TArray<FExposurePulse> Pulses;
	TMap<FName, float> ExposureSources;
	float ExposureSum = 0.f;
	FPostProcessSettings PPSteady;
	FPostProcessSettings PPBias;
	FPostProcessSettings PPInspect;

	// ---- eyelids
	float Lid = 0.f;
	EYggLidMode LidMode = EYggLidMode::Open;
	TArray<FVector2f> LidKeys;   // (time, lid) from the moment the mode was set
	float LidT = 0.f;
	bool bLidMoving = false;
	bool bLidRepair = false;
	int32 LidOwner = -1;
	float LidNoLeaseTime = 0.f;
	TFunction<void()> LidOnClosed;

	// ---- scripted takeovers
	EYggScripted Scripted = EYggScripted::None;
	int32 ScriptOwner = -1;
	float ScriptT = 0.f, ScriptLen = 0.f, ScriptSide = 1.f, FallZScale = 1.f;
	bool bFallLidsSlammed = false;
	FVector TakeLoc = FVector::ZeroVector;   // (forward, right, up) cm in the view's yaw frame
	FRotator TakeRot = FRotator::ZeroRotator;
	float TakeFade = 0.f;                    // > 0: offsets easing back to 0 over this many seconds left
	FVector TakeFadeLoc = FVector::ZeroVector;
	FRotator TakeFadeRot = FRotator::ZeroRotator;
	float TakeFadeLen = 0.3f;
	float Dilation = 1.f;
	bool bAudioPitched = false;
	float LastAudioPitch = 1.f;

	// ---- fly-out
	UPROPERTY(Transient) TObjectPtr<ACameraActor> FlyCam;
	UPROPERTY(Transient) TObjectPtr<UWorldPartitionStreamingSourceComponent> FlyStreaming;
	TArray<FVector> FlyPath, FlyLook;
	TArray<float> FlyPathArc, FlyLookArc;   // cumulative arc length (0..1) at even curve-parameter samples
	TArray<float> FlySampleU, FlySampleT;  // timing samples: arc fraction and time
	float FlyT = 0.f, FlyLen = 0.f, FlyOpen = 1.5f, FlyFovNow = 70.f;
	FVector2D FlyFov = FVector2D(70.f, 50.f);
	int32 FlyOwner = -1;
	bool bFlyWaiting = false, bFlyActive = false, bFlyEnding = false;

	// ---- nudge, reflex, hand pull
	bool bNudge = false;
	FVector NudgeTarget = FVector::ZeroVector;
	float NudgeMaxYaw = 45.f, NudgeRate = 20.f, NudgePitchMax = 8.f, NudgeT = 0.f, NudgeYawDone = 0.f, NudgePitchDone = 0.f, NudgeFastT = 0.f;
	int32 NudgeOwner = -1;
	int32 NudgesUsed = 0;
	double LastNudgeTime = -1000.0;
	bool bReflex = false;
	float ReflexT = 0.f, ReflexDone = 0.f;
	FVector2D ReflexTotal = FVector2D::ZeroVector;
	double LastReflexTime = -1000.0;
	TWeakObjectPtr<AActor> PullActor;
	float PullRate = 0.f;

	// ---- inspect
	UPROPERTY(Transient) FYggInspect Insp;
	UPROPERTY(Transient) TObjectPtr<UStaticMeshComponent> InspMesh;
	bool bInspect = false;
	EInspPhase InspPhase = EInspPhase::Rising;
	float InspT = 0.f, InspAlpha = 0.f, InspYaw = 0.f, InspPitch = 0.f, BackT = 0.f;
	bool bBackSeen = false;
	int32 InspOwner = -1;

	// ---- hands, pace, jump, body pushes
	FName HandsItem;
	int32 HandsOwner = -1;
	float PaceCmS = -1.f;
	bool bJumpBlocked = false;
	bool bJumpWasAllowed = true;
	int32 PushedOneBody = -1;
	EYggView PushedBodyView = EYggView::Walk;
	float PushedBodyDrop = -1.f;

	// ---- misc
	double LastCutTime = -100.0;
	double LastHardReset = -100.0;
	TArray<FString> RecentRepairs;
	bool bRepairingViewTarget = false;
	/** The fly-out hid the pawn (the watchdog only undoes the rig's own hide). */
	bool bPawnHiddenByRig = false;

	// ---- steps of the tick
	void LookupSources();
	void ApplyRigToggle();
	void TickLeases();
	void UpdateFeeds(float Dt);
	void TickLids(float Dt);
	void TickPosture(float Dt);
	void TickBodyYaw(float Dt);
	void TickLimits(float Dt);
	void TickBob(float Dt);
	void TickBreath(float Dt);
	void TickLandingAndNods(float Dt);
	void TickScripted(float Dt);
	void TickFly(float Dt);
	void TickInspect(float Dt);
	void TickLens(float Dt);
	void Watchdog(float Dt);
	void ComposeEye(float Dt);
	void ComposeViewOffsets(float Dt);
	void PushBody();
	void DrawDebug() const;

	// ---- helpers
	const FYggPoseDef& PoseDef(EYggView V) const;
	void EnterPose(int32 Owner, const FYggPoseReq& R);
	void SnapPose(EYggView V, float Yaw);
	void StandUp(float Seconds);
	void StartCrawl();
	void BecomeWalk();
	void SetLimitsTo(float MinP, float MaxP, float YawHalf, float NewAnchor, bool bSnap);
	void ShowProp(FName Tag, bool bShow);
	void SetCtrlYawOwned(bool bOwned);
	void SetBodyYaw(float Yaw);
	void DoStep(bool bLeft, float SpeedCmS, uint8 Surface, bool bSynth);
	void StartFly();
	void EndFly(bool bCut);
	void BuildFlyTables(float T);
	FVector FlyPathAt(float ArcU) const;
	FVector FlyLookAt(float ArcU) const;
	void EndNudge(const TCHAR* Why);
	void SetDilation(float D, float AudioPitch);
	void Repair(const FString& What, EYggLease Kind);
	void LeaseLog(const TCHAR* What, const FLease& L) const;
	void QuietRelease(int32 Owner, EYggLease Kind);
	float LeaseCap(EYggLease Kind, bool bKeep) const;
	bool IsLookLocked() const;
	bool CanGuide() const;
	bool InTakeover() const { return Scripted == EYggScripted::Fall || Scripted == EYggScripted::Knock || TakeFade > 0.f; }
	APlayerController* GetPC() const;
	APlayerCameraManager* GetPCM() const;
	void EnsureModifier();
	void Cut();

	// ---- the modifier's two sides
	void ProcessRotation(AActor* ViewTarget, float Dt, FRotator& ViewRot, FRotator& Delta);
	void ApplyView(APlayerCameraManager* PCM, float Dt, FMinimalViewInfo& POV);
};

/** The rig's input and output side on the camera manager: soft look limits, look lock, inspect capture, nudge, reflex
 *  and hand pull as deltas (ProcessViewRotation); view-only rotation offsets, FOV terms and the one exposure channel
 *  (ModifyCamera). Priority 0. For a view target other than the pawn everything passes through (the fly camera gets
 *  its FOV). */
UCLASS()
class YGGDRASILSPEAK_API UYggViewModifier : public UCameraModifier
{
	GENERATED_BODY()

public:
	UYggViewModifier();

	TWeakObjectPtr<UYggViewRigComponent> Rig;

	virtual bool ProcessViewRotation(AActor* ViewTarget, float Dt, FRotator& OutViewRotation, FRotator& OutDeltaRot) override;
	virtual bool ModifyCamera(float Dt, FMinimalViewInfo& InOutPOV) override;

	APlayerCameraManager* GetCameraOwner() const { return CameraOwner; }
};
