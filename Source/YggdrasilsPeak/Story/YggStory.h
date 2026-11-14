// The story machine (2026-11-14): a chapter is a JSON file of beats (Content/Story/<Chapter>.ygs: .ygs so the editor never tries to import it). Each beat waits for its
// "when" (start / after a beat / near a place or person / looked at / a flag) and then plays its "do" list of actions
// (fades, titles, lines, choices, NPCs, camera moves, sounds, journal page...). No C++ per scene: the chapter is data.
// Console: ygg.Story.Start [file], ygg.Story.Jump <beat>, ygg.Story.Choose <n>, ygg.Story.Stop.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Camera/CameraModifier.h"
#include "GameFramework/Character.h"
#include "Blueprint/UserWidget.h"
#include "YggStory.generated.h"

class FJsonObject;
class FJsonValue;
class ACameraActor;
class UAudioComponent;
class UTexture2D;

/** Camera moves the story adds on top of the player's view: kneel (down), slip tumble, field-of-view pulse. */
UCLASS()
class YGGDRASILSPEAK_API UYggStoryCameraModifier : public UCameraModifier
{
	GENERATED_BODY()

public:
	float DownCm = 0.f;
	float FovAdd = 0.f;
	FRotator Spin = FRotator::ZeroRotator;
	virtual bool ModifyCamera(float DeltaTime, FMinimalViewInfo& InOutPOV) override;
};

/** A person in the story: the climber body with its walk blend, steered by the story (idle, follow, go to, lead, sit). */
UCLASS()
class YGGDRASILSPEAK_API AYggStoryNPC : public ACharacter
{
	GENERATED_BODY()

public:
	AYggStoryNPC();
	virtual void Tick(float DeltaSeconds) override;

	enum class EMode : uint8 { Idle, Follow, GoTo, Lead, Sit };
	EMode Mode = EMode::Idle;
	FName Id;
	FVector Goal = FVector::ZeroVector;
	float FollowBehind = 250.f;
	float FollowSide = 120.f;
	float Speed = 1.f;
	bool bArrived = false;
	/** Lead / hand: walks only while this is true (the hand is held, or the player is close enough). */
	bool bMayWalk = true;
	bool bFacePlayer = true;

	void Dress(const FLinearColor& Tint, float Scale, bool bHood);
	bool ReachedGoal(float Radius = 120.f) const;

private:
	float StuckTime = 0.f;
	FVector LastPos = FVector::ZeroVector;
};

/** Words, choices, titles, the journal page and the black veil, drawn in the HUD's language (no boxes, veils, serif). */
UCLASS()
class YGGDRASILSPEAK_API UYggStoryOverlay : public UUserWidget
{
	GENERATED_BODY()

public:
	float Black = 0.f;
	FLinearColor BlackColor = FLinearColor(0.004f, 0.006f, 0.01f, 1.f);
	FString Title, Sub;
	float TitleA = 0.f;
	FString Who, Line;
	float LineA = 0.f;
	FString Hint;
	float HintA = 0.f;
	FString Prompt;
	TArray<FString> Options;
	float ChoiceA = 0.f;
	float ChoiceTimeLeft = -1.f;
	FString PageHead;
	TArray<FString> Page;
	float PageA = 0.f;
	FString CardTitle, CardText;
	TObjectPtr<UTexture2D> CardImage;
	float CardA = 0.f;

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
	bool IsRunning() const { return bRunning; }
	bool HasFlag(FName Flag) const { return Flags.Contains(Flag); }

	/** New game from the menu: the next base_map start plays chapter 1 (set by the start menu, read once). */
	static bool bPendingNewGame;

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
		double CondSince = -1.0;   // when the non-look conditions first held (look timeouts count from here)
	};
	struct FRun
	{
		int32 Beat = INDEX_NONE;
		int32 Step = 0;
		float Wait = 0.f;
		bool bBlocked = false;     // waiting for a choice, a card, a hand-hold, an arrival
		FString BlockKind;
		TSharedPtr<FJsonObject> BlockAction;
	};
	struct FTween { float* Value = nullptr; float From = 0.f, To = 0.f, Time = 0.f, Length = 0.f; bool bPulse = false; };

	bool bRunning = false;
	FString ChapterName;
	TArray<FBeat> Beats;
	TArray<FRun> Runs;
	TSet<FName> Flags;
	TMap<FName, FTransform> Places;
	TMap<FName, TWeakObjectPtr<AYggStoryNPC>> People;
	TArray<FTween> Tweens;
	int32 Day = 1;
	double Clock = 0.0;
	// story clock: held at TimeValue (0..2400) and advanced at TimeRate game minutes per real second
	bool bTimeHold = false;
	float TimeValue = 0.f;
	float TimeRate = 0.f;

	UPROPERTY(Transient) TObjectPtr<UYggStoryOverlay> Overlay;
	UPROPERTY(Transient) TObjectPtr<UYggStoryCameraModifier> CamMod;
	UPROPERTY(Transient) TObjectPtr<ACameraActor> FlyCam;
	UPROPERTY(Transient) TArray<TObjectPtr<UAudioComponent>> Sounds;

	// fly-out camera
	FVector FlyFrom = FVector::ZeroVector, FlyTo = FVector::ZeroVector;
	FRotator FlyRotFrom, FlyRotTo;
	float FlyTime = 0.f, FlyLength = 0.f;
	// gaze pull
	FRotator GazeFrom, GazeTo;
	float GazeTime = 0.f, GazeLength = 0.f;
	// hand-hold
	TWeakObjectPtr<AYggStoryNPC> HandNPC;
	bool bHandHintShown = false;
	// tumble
	float TumbleTime = 0.f, TumbleLength = 0.f;
	// locks
	bool bMoveLocked = false, bLookLocked = false;
	int32 ChoiceResult = -1;
	bool bCardClose = false;
	double BlockedSince = 0.0;
	// material parameters tweened on tagged actors ("param" action): up to 8 at once
	float ParamValue[8] = {};
	FName ParamTag[8];
	FName ParamName[8];
	TArray<TWeakObjectPtr<class UMaterialInstanceDynamic>> ParamMIDs[8];

	bool LoadChapter(const FString& Chapter);
	bool Ready() const;
	void TickTriggers();
	bool Conditions(FBeat& Beat, bool& bLookPart);
	void TickRuns(float Dt);
	/** Runs one action; returns the seconds to wait after it (0 = go on), or -1 when it blocks until something happens. */
	float DoAction(FRun& Run, const TSharedPtr<FJsonObject>& A);
	bool Unblocked(FRun& Run, float Dt);
	void TickTweens(float Dt);
	void TickCamera(float Dt);
	void TickHand(float Dt);

	bool Where(const FString& Name, FVector& Out, FRotator* OutRot = nullptr) const;
	AYggStoryNPC* Person(const FString& Name) const;
	void Tween(float& Value, float To, float Seconds, bool bPulse = false);
	void SetLocks(bool bMove, bool bLook);
	FString Expand(const FString& Text) const;
	class APawn* Player() const;
	class APlayerController* PC() const;
};
