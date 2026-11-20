// The story machine (2026-11-14): a chapter is a JSON file of beats (Content/Story/<Chapter>.ygs: .ygs so the editor never tries to import it). Each beat waits for its
// "when" (start / after a beat / near a place or person / looked at / a flag / a posture) and then plays its "do" list of actions
// (eyelids, titles, lines, choices, NPCs, postures, sounds, journal page...). No C++ per scene: the chapter is data.
// Camera (Docs/Development/Chapter1_Camera_Spec.md, 2026-11-19): the story never moves the view itself. Every camera effect is a
// request to the view rig (UYggViewRigComponent: postures, eyelids, leases, nudges, fall, fly-out, inspect); the only absolute
// control-rotation write left is a teleport's facing, behind closed eyelids, on a camera-cut frame (spec R1, R2, R7).
// Console: ygg.Story.Start [file], ygg.Story.Jump <beat>, ygg.Story.Choose <n>, ygg.Story.Stop, ygg.Story.CamTest,
// ygg.Cam.Frame <place> [pose].

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GameFramework/Character.h"
#include "Blueprint/UserWidget.h"
#include "YggStory.generated.h"

class FJsonObject;
class FJsonValue;
class UAudioComponent;
class UTexture2D;
class UStaticMeshComponent;
class USpotLightComponent;
class USoundAttenuation;
class UYggViewRigComponent;
class APawn;
struct FStreamableHandle;

/** A person in the story: the climber body with its walk blend, steered by the story (idle, follow, go to, lead, sit, present). */
UCLASS()
class YGGDRASILSPEAK_API AYggStoryNPC : public ACharacter
{
	GENERATED_BODY()

public:
	AYggStoryNPC();
	virtual void Tick(float DeltaSeconds) override;

	/** Present (spec 6.5): stands in front of the player's view at PresentDist and faces him - people come into your view,
	 *  the view is never dragged to them. */
	enum class EMode : uint8 { Idle, Follow, GoTo, Lead, Sit, Present };
	EMode Mode = EMode::Idle;
	FName Id;
	FVector Goal = FVector::ZeroVector;
	/** Waypoints after Goal (GoTo / Lead): a straight line between far places leaves the trail (porters, 2026-11-19 audit). */
	TArray<FVector> Path;
	float FollowBehind = 250.f;
	float FollowSide = 120.f;
	float Speed = 1.f;
	float PresentDist = 200.f;
	/** talk_side: while this person's line is on screen (TalkLeft > 0), a follower walks this far at the player's side
	 *  (30 cm behind), so a 30 degree glance finds the face. 0 = off. */
	float TalkSide = 0.f;
	float TalkLeft = 0.f;
	bool bArrived = false;
	/** Lead / hand: walks only while this is true (the hand is held, or the player is close enough). */
	bool bMayWalk = true;
	bool bFacePlayer = true;

	void Dress(const FLinearColor& Tint, float Scale, bool bHood);
	/** MetaHuman look from Content/Story/Cast.ygs: the body (walks with ABP_Unarmed_MH), face, grooms and the Blender
	 *  garment following the body. False when the person is not in the cast file yet (then Dress() is used). */
	bool DressAs(const FString& Look, float Scale);
	/** Close to the last goal (waypoints left = not there yet). */
	bool ReachedGoal(float Radius = 120.f) const;
	/** npc_face: head and torso turn to a point (or keep turning to an actor) over about 0.5 s, held while standing still,
	 *  for Seconds (< 0 = until the next mode change). */
	void FaceTo(const FVector& At, AActor* Actor, float Seconds);
	void ClearFace() { FaceLeft = 0.f; FaceActor = nullptr; }

private:
	float StuckTime = 0.f;
	float GroundCheck = 0.f;
	float FallTime = 0.f;      // seconds in the air: past 0.4 s look far for the ground, hover if there is none
	bool bHover = false;       // no collision under this person: walk on without gravity at the height between start and goal
	FVector HoverFrom = FVector::ZeroVector;
	FVector LastPos = FVector::ZeroVector;
	/** Follow forward from the player's walking direction, not his view: every look turn swung the followers to the other
	 *  side (2026-11-19 audit). */
	FVector FollowFwd = FVector::ZeroVector;
	// present: the goal is recomputed only when the view turns > 25 degrees or the player moves > 1 m
	FVector PresentFrom = FVector::ZeroVector;
	float PresentYaw = 0.f;
	bool bPresentGoal = false;
	// steering: a capsule sweep 4x a second while walking; a blocked way turns the walk 60 degrees to the free side
	float SteerCheck = 0.f;
	float SteerYaw = 0.f;
	// facing
	FVector FaceAt = FVector::ZeroVector;
	TWeakObjectPtr<AActor> FaceActor;
	float FaceLeft = 0.f;
	// far away: the MetaHuman pose ticks only while drawn (two grown-ups ticking 5 km behind for an hour, audit)
	float LodCheck = 0.f;
	bool bTickFar = false;
	bool bMetaHuman = false;
	EMode LastMode = EMode::Idle;
	int32 HopFails = 0;
	float DebugTime = 0.f;
	void DebugLog(float DeltaSeconds);
	/** The MetaHuman walk: the body plays BS_Idle_Walk_Run_MH itself (single node), fed with the speed and the walk
	 *  direction. ABP_Unarmed_MH (retargeted from the mannequin's) runs CR_Mannequin_FootIK, whose foot effectors read
	 *  ik_foot_l / ik_foot_r - bones the MetaHuman skeleton does not have - so the feet were pinned in a standing pose
	 *  and the people glided (play-test 2026-11-20). */
	bool bBlendWalk = false;
	float BlendSpeed = 0.f;
	void TickWalkBlend(float DeltaSeconds);
	/** Follow (2026-11-20 redesign): walks the player's breadcrumb trail (UYggStorySubsystem::Crumbs) at FollowBehind
	 *  cm behind him along it, matching his pace. TrailCursor = absolute index of the next crumb to reach. */
	void TickFollow(float DeltaSeconds, APawn* P, const FVector& Me, const FVector& PLoc, FVector& Target, bool& bMove);
	int32 TrailCursor = -1;
	int32 TrailEpochSeen = -1;
	bool bFollowStopped = true;
	double RestartAt = -1.0;
	float FollowSpeed = 0.f;     // smoothed walking pace while following (cm/s)
	float WantSpeed = -1.f;      // this frame's pace for the follow walk (< 0 = the old Speed scale)
	/** Feet in the snow: a foot that stops moving (planted) presses a print into the player's trail map, within 60 m. */
	void TickFeet(float DeltaSeconds, APawn* P);
	bool bFeetBones = false;
	bool bFeetInit = false;
	bool bFootPlanted[2] = { true, true };
	float FootRatio[2] = { 0.f, 0.f };
	FVector FootLast[2] = { FVector::ZeroVector, FVector::ZeroVector };
	float SinceStamp = 0.f;
	int32 Stamps = 0;
	TWeakObjectPtr<class USnowTrailComponent> PlayerSnow;
	/** Stuck: hop round the obstacle onto drawn ground where the capsule fits (never a tent roof or a chorten). */
	bool HopAside(const FVector& Me, const FVector& Dir, bool bAnySurface);
	UPROPERTY(Transient)
	TArray<TObjectPtr<USceneComponent>> LookParts;
};

/** Words, choices, titles, captions, eyelids, the journal page and the black veil, drawn in the HUD's language (no boxes, serif).
 *  No full-screen card veil any more: captions sit in the lower third on a soft gradient (spec 5.9, 6.14). */
UCLASS()
class YGGDRASILSPEAK_API UYggStoryOverlay : public UUserWidget
{
	GENERATED_BODY()

public:
	/** The black of the title card and the end page (fade), shared with closed eyelids. */
	float Black = 0.f;
	FLinearColor BlackColor = FLinearColor(0.004f, 0.006f, 0.01f, 1.f);
	/** Eyelids: 0 open, 1 closed (the view rig's value, copied each story tick). */
	float Lid = 0.f;
	FString Title, Sub;
	float TitleA = 0.f;
	FString Who, Line;
	float LineA = 0.f;
	FString Hint;
	float HintA = 0.f;
	/** holdkey: 0..1 progress line under the hint (< 0 = none). */
	float HoldProgress = -1.f;
	FString Prompt;
	TArray<FString> Options;
	float ChoiceA = 0.f;
	float ChoiceTimeLeft = -1.f;
	FString PageHead;
	TArray<FString> Page;
	float PageA = 0.f;
	/** caption: lower third on a soft gradient; "lowleft" = small, lower left ("Day 1 / 49"). */
	FString Caption;
	float CaptionA = 0.f;
	bool bCaptionLowLeft = false;
	/** A look trigger's "[E] Open" prompt (spec 6.1 press). */
	FString PressPrompt;
	float PressA = 0.f;
	/** Inspect (spec 6.11): the captions always (front text, back text once the back faces you); the photo card when the
	 *  object is a texture (a mesh is held in 3D by the view rig), turned with the mouse by the rig (TurnX / TurnY = the
	 *  card's foreshortening), or with [F] when there is no rig. */
	FString InspectText, InspectBackText;
	bool bInspectBack = false;
	bool bInspectCard = false;
	/** The background darkens here only without the rig (the rig darkens it in post-process). */
	bool bInspectDarken = false;
	/** need_back and the back not seen yet: the hint says to turn it. */
	bool bInspectNeedBack = false;
	bool bInspectFlipKey = false;
	float InspectTurnX = 1.f, InspectTurnY = 1.f;
	float InspectA = 0.f;
	UPROPERTY(Transient) TObjectPtr<UTexture2D> InspectFront;
	UPROPERTY(Transient) TObjectPtr<UTexture2D> InspectBackImage;
	/** /Game/UI/T_Eyelid (Tools/UI/make_eyelid.py): opaque top row fading to a soft curved edge at the bottom; mirrored for the
	 *  lower lid. Missing = a stacked gradient. */
	UPROPERTY(Transient) TObjectPtr<UTexture2D> LidTex;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
};

UCLASS()
class YGGDRASILSPEAK_API UYggStorySubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UYggStorySubsystem, STATGROUP_Tickables); }

	bool Start(const FString& Chapter);
	void Stop();
	void JumpTo(const FString& BeatId);
	void Choose(int32 Option);
	/** ExpeditionComponent suppresses its own survival messages ("Off the rope", camp lines) while a chapter plays. */
	bool IsRunning() const { return bRunning; }
	bool HasFlag(FName Flag) const { return Flags.Contains(Flag); }
	/** A choice is on screen: keys 1-4 answer it, so ClimberControls must not eat or drink with them (audit: "1" ate a ration). */
	bool IsChoiceOpen() const { return bChoiceOpen; }
	/** Death / continue (ExpeditionComponent, before its Rig->ReleaseAll(0) + HardReset): drops every camera hold the story
	 *  has - its leases, the hand-hold, the teleport check, closed eyelids. The beats keep running. */
	void ReleaseAll();

	/** ygg.Story.CamTest (spec 11 V3): drives the view rig through every posture, takeover and reset, checks it comes back. */
	void StartCamTest();
	/** ygg.Cam.Frame <place> [pose] (spec V13): cut to a place, take the pose, save Saved/Review/frames/<place>[_<pose>].png. */
	void StartFrameShot(const FString& Place, const FString& Pose);

	/** New game from the menu: the next base_map start plays chapter 1 (set by the start menu, read once). */
	static bool bPendingNewGame;

	/** The player's walked path for the followers: a point every ~60 cm (Crumbs, actor locations) and the distance
	 *  walked up to each (CrumbLen). Crumbs[i] has the absolute index TrailFirst + i (old points are dropped from the
	 *  front); TrailEpoch changes when the trail starts over (a teleport or a cut). */
	TArray<FVector> Crumbs;
	TArray<float> CrumbLen;
	int32 TrailFirst = 0;
	int32 TrailEpoch = 0;
	void RecordTrail();

private:
	struct FBeat
	{
		FString Id;
		TSharedPtr<FJsonObject> When;
		TArray<TSharedPtr<FJsonValue>> Do;
		bool bFired = false;
		bool bDone = false;
		double FiredAt = 0.0;
		double DoneAt = 0.0;
		double CondSince = -1.0;   // T0 of the look escalation: when every other condition first held
		// look trigger state (spec 6.1)
		float LookHeld = 0.f;      // seconds inside the cone (dwell)
		float SeenHeld = 0.f;      // seconds inside the inner 60% of the frame (seen)
		bool bCompanion = false, bLure = false, bNudge = false, bPromptShown = false, bPromptHint = false;
	};
	struct FRun
	{
		int32 Beat = INDEX_NONE;
		int32 Step = 0;
		float Wait = 0.f;
		bool bBlocked = false;     // waiting for a choice, an inspect, a hand-hold, an arrival, a held key
		double BlockedAt = 0.0;    // story clock when it blocked (per run: two blocked runs had one shared clock)
		FString BlockKind;
		TSharedPtr<FJsonObject> BlockAction;
		float BlockValue = 0.f;    // holdkey: seconds held
	};
	struct FTween { float* Value = nullptr; float From = 0.f, To = 0.f, Time = 0.f, Length = 0.f; bool bPulse = false; };
	/** A blink's "during" list: runs on the first fully closed frame (spec 6.6). */
	struct FDuring { int32 Serial = 0; int32 Beat = INDEX_NONE; TArray<TSharedPtr<FJsonValue>> Actions; bool bReady = false; double Since = 0.0; float MaxWait = 1.f; };
	/** A lease the story asked the rig for (Kind = EYggLease as a byte, so this header needs no rig include). */
	struct FStoryLease { int32 Owner = INDEX_NONE; uint8 Kind = 0; bool bKeep = false; };
	/** One step of ygg.Story.CamTest. */
	struct FCamStep { FString Name; float Wait = 1.f; TFunction<void()> Do; TFunction<void(float)> OnTick; TFunction<FString()> Check; };

	bool bRunning = false;
	FString ChapterName;
	TArray<FBeat> Beats;
	TArray<FRun> Runs;
	TSet<FName> Flags;
	TMap<FName, FTransform> Places;
	TMap<FName, TWeakObjectPtr<AYggStoryNPC>> People;
	TArray<FTween> Tweens;
	int32 Day = 1;
	/** Story clock, real seconds (it keeps running at real speed through the slip's slow motion, spec 6.8). */
	double Clock = 0.0;
	// story clock of the day: held at TimeValue (0..2400) and advanced at TimeRate game minutes per real second
	bool bTimeHold = false;
	float TimeValue = 0.f;
	float TimeRate = 0.f;

	UPROPERTY(Transient) TObjectPtr<UYggStoryOverlay> Overlay;
	UPROPERTY(Transient) TArray<TObjectPtr<UAudioComponent>> Sounds;
	/** Spatial falloff for story sounds by radius (cm): the wave assets carry no attenuation, so "at" did nothing (audit). */
	UPROPERTY(Transient) TMap<int32, TObjectPtr<USoundAttenuation>> Attenuations;
	UPROPERTY(Transient) TObjectPtr<USpotLightComponent> Headlamp;
	UPROPERTY(Transient) TObjectPtr<UStaticMeshComponent> WornKhata;
	UPROPERTY(Transient) TArray<TObjectPtr<AActor>> SpawnedLights;

	TWeakObjectPtr<UYggViewRigComponent> RigCache;
	TArray<FStoryLease> StoryLeases;
	TArray<FDuring> Durings;
	int32 DuringSerial = 0;
	bool bInDuring = false;
	/** The story asked for closed eyelids (closed / sleep): a teleport carrying "cut" may go, and the black veil holds the
	 *  dark once they are shut (a page can outlast the rig's 8 s lid lease). */
	bool bLidsShutRequested = false;
	// eyelids without a view rig: the same curves, run here and drawn by the overlay
	uint8 LocalLidMode = 0;
	float LocalLidTime = -1.f, LocalLidT = 0.f, LocalLidHold = 0.f, LocalLidFrom = 0.f, LocalLid = 0.f;

	// hand-hold
	TWeakObjectPtr<AYggStoryNPC> HandNPC;
	int32 HandOwner = INDEX_NONE;
	float HandPull = 8.f;
	bool bHandHeld = false;
	bool bHandActive = false;
	// a story teleport is checked for a moment afterwards: put back if something moved the climber away, and held in
	// the air (flying, no gravity) until the ground there has loaded (a jump across the map, 2026-11-19 play-test)
	FVector PendingTeleport = FVector::ZeroVector;
	FVector PendingTeleportLast = FVector::ZeroVector;   // where the climber was last frame (a jump of metres = not walking)
	float PendingTeleportYaw = 0.f, PendingTeleportTime = 0.f;
	bool bHeldForGround = false;
	bool bPendingReached = false;   // the climber has stood at the spot: from then on only a story Move lease or a jump puts him back
	void UpdatePendingTeleport(float Dt);
	/** Ends the check: a climber still held in the air for the ground falls normally again. */
	void ClearPendingTeleport();

	int32 ChoiceResult = -1;
	bool bChoiceOpen = false;
	bool bCardClose = false;
	/** The chapter ended in the dark: the overlay stays a moment after Stop and its black fades out over 2 s, so the people
	 *  removed at the end never vanish on screen and the world does not pop in (audit: end of chapter). */
	bool bEndFade = false;
	// a hint that waits for the player to stop ("idle": seconds without input); cancelled once he walks
	FString IdleHint;
	float IdleHintAfter = 0.f, IdleHintShow = 5.f;
	double IdleHintSince = 0.0, LastInputAt = 0.0;
	FRotator LastView = FRotator::ZeroRotator;
	int32 PromptBeat = INDEX_NONE;
	// material parameters tweened on tagged actors ("param" action): up to 8 at once
	float ParamValue[8] = {};
	FName ParamTag[8];
	FName ParamName[8];
	TArray<TWeakObjectPtr<class UMaterialInstanceDynamic>> ParamMIDs[8];
	// lights faded by the story ("light" action): up to 4 at once
	float LightK[4] = {};
	float LightBase[4] = {};
	FName LightName[4];
	TArray<TWeakObjectPtr<class ULightComponent>> LightComps[4];
	/** NPC looks loaded in the background at the chapter start (spec 6.16): DressAs then finds them in memory. */
	TSharedPtr<FStreamableHandle> PreloadHandle;
	/** ygg.Climb.Camera while the chapter plays (0: a grab never swaps to the climbing arm); restored at Stop. */
	FString ClimbCameraBefore;
	bool bClimbCameraSet = false;
	/** The chapter file's "preload_cast" (default true): false = Start does not load the whole cast (the summit beat has no people). */
	bool bPreloadCast = true;

	// tools
	TArray<FCamStep> CamSteps;
	int32 CamStep = INDEX_NONE;
	float CamStepTime = 0.f;
	float CamBaseFov = 90.f;
	TArray<FString> CamResults;
	FString FramePlace, FramePose;
	float FrameTime = -1.f;
	bool bFrameShot = false;

	bool LoadChapter(const FString& Chapter);
	bool LoadPlaces(const TSharedPtr<FJsonObject>& Root);
	bool Ready() const;
	UYggViewRigComponent* Rig();
	void EnsureOverlay();
	/** Story seconds: the real frame time while the world is slowed or sped (slip slow motion), else the game delta. */
	float StoryDt(float DeltaTime) const;
	void TickTriggers(float Dt);
	/** every condition but the look: after, delay, flags, near / far, posture, any [...] */
	bool BasicConditions(const TSharedPtr<FJsonObject>& W, double ParentDone, const FString& BeatId) const;
	bool Conditions(FBeat& Beat, int32 Index, float Dt, bool& bLookPart);
	void ResetLook(FBeat& Beat, int32 Index);
	void TickRuns(float Dt);
	/** Runs one action; returns the seconds to wait after it (0 = go on), or -1 when it blocks until something happens. */
	float DoAction(FRun& Run, const TSharedPtr<FJsonObject>& A);
	/** Removed keys (spec 7.3): logged as errors and played as their safe equivalent. True when handled. */
	bool LegacyAction(FRun& Run, const TSharedPtr<FJsonObject>& A, float& OutWait);
	bool Unblocked(FRun& Run, float Dt);
	void EndRun(int32 Beat);
	/** The hand-hold is over: pull, hands and pace released, the hint faded (it stayed up after the hold, audit). */
	void EndHand();
	bool BeatHas(int32 Beat, const TCHAR* Key) const;
	void TickTweens(float Dt);
	void TickHand(float Dt);
	void TickHints(float Dt);
	void TickLights(float Dt);
	void TickInspect(float Dt);
	void TickTools(float Dt);
	void TickCamTest(float Dt);
	void TickFrameShot(float Dt);

	// eyelids
	float StartLids(int32 Owner, const FString& Mode, float T, float Hold, const TArray<TSharedPtr<FJsonValue>>& During);
	void RunDurings();
	void TickLocalLids(float Dt);
	float CurrentLid() const;
	/** Eyelids or the black veil at 99% or more, or inside a blink's "during": a cut may happen now (spec R7). */
	bool EyesShut() const;
	/** The cut frame (spec 6.7): camera cut flag, teleport, facing, HardReset, log. */
	void CutTo(const FString& PlaceName, const FVector& At, const FRotator& Facing, float Up, int32 Pose);
	/** Rig->ReleaseAll + HardReset at the current facing (JumpTo, Stop, the chapter end). */
	void ResetCamera(float Blend);
	void TrackLease(int32 Owner, uint8 Kind, bool bKeep);
	/** A timed effect (attention, heartbeat push, exposure, breath, fly-out) runs its own length even when its beat ends
	 *  first: its lease is re-taken as keep, capped at Seconds (flags_seen's attention died with its one-line beat). */
	void KeepTimed(int32 Owner, uint8 Kind, float Seconds);
	void ReleaseTracked(uint8 Kind, int32 ExceptOwner = INDEX_NONE);
	bool StoryHolds(uint8 Kind) const;

	void PlayLure(const TSharedPtr<FJsonObject>& Lure, const FVector& Target, const FString& BeatId);
	UAudioComponent* PlaySound(const FString& Path, const FVector* At, float Vol, float Falloff);
	USoundAttenuation* Attenuation(float Falloff);
	void Preload(const TArray<FString>& Looks);
	void SetLight(const FString& Name, bool bOn, float Seconds);
	void Wear(const FString& Item, bool bOn);
	void Compass(const TSharedPtr<FJsonObject>& Params);
	void ReleaseCompass();
	void TalkBy(const FString& Who, float Seconds);
	void ShowCaption(const FString& Text, float Seconds, bool bLowLeft);

	bool Where(const FString& Name, FVector& Out, FRotator* OutRot = nullptr) const;
	AYggStoryNPC* Person(const FString& Name) const;
	void Tween(float& Value, float To, float Seconds, bool bPulse = false);
	void CancelTween(float& Value);
	bool IsTweening(const float& Value) const;
	FString Expand(const FString& Text) const;
	class APawn* Player() const;
	class APlayerController* PC() const;
};
