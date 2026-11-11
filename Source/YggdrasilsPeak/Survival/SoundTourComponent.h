// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/ActorComponent.h"
#include "SoundTourComponent.generated.h"

class UAudioComponent;
class UFont;
class USoundAttenuation;
class USoundBase;
class UTextBlock;

/** The sound tour's panel: a dark card on the left of the screen, rebuilt in code (no widget asset). */
UCLASS()
class YGGDRASILSPEAK_API USoundTourWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetLines(const FString& InHeader, const FString& InTitle, const FString& InWhere, const FString& InStatus, const FString& InVerdict);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	void Build();
	static void Put(UTextBlock* Text, const FString& Value);

	UPROPERTY(Transient) TObjectPtr<UTextBlock> Header;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> Title;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> Where;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> Status;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> Verdict;
	UPROPERTY(Transient) TObjectPtr<UFont> Serif;
};

/**
 *  The in-game sound tour: every sound the game uses, heard inside the game while you play (console: snd).
 *
 *  Group by group, in the order you meet them on the mountain, it first plays a SCENE through the game's own systems
 *  (you walk and every step is that surface, the wind rises from calm to a storm, breathing gets harder, a lightning
 *  flash then its thunder, a serac zone breaking, clipping into the rope, the real weather wheel spinning), then every
 *  sound on its own, at the place and loudness the game gives it, over the live wind. You judge each by ear:
 *  G good, B bad, U unsure; arrows to move. Verdicts go to Docs/Development/Sound_Review_InGame.md (and
 *  Saved/AudioWork/review_ingame.json, so a later "snd new" plays only what is new, changed or not yet good).
 *
 *  Console:  snd        start (or show the keys again)       snd new     only new / changed / not yet good
 *            snd 7      jump to group 7                      snd wind    jump to the first group named like that
 *            snd note <words>   a note on the last sound you rated      snd bad <words>   bad, with why
 *            snd 0      end the tour (everything goes back to normal)
 */
UCLASS(ClassGroup=(Survival))
class YGGDRASILSPEAK_API USoundTourComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USoundTourComponent();

	/** The console command: snd [0|new|all|<group number>|<word>|note ..|good|bad ..|unsure|next|back|again]. */
	static void Command(UWorld* World, const TArray<FString>& Args);

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	enum class EKind : uint8
	{
		// scenes: the game's own systems playing a group the way you meet it
		WalkScene, BreathScene, WindScene, ThunderScene, SeracScene, RopeScene, WheelScene,
		// single sounds, each where and how loud the game plays it
		Step, Flat, Around, Behind, Below, Thunder, Avalanche, Loop, WindBed, Landing
	};

	struct FGroup
	{
		FString Title;
		FString Where;
	};

	struct FStep
	{
		int32 Group = 0;
		EKind Kind = EKind::Flat;
		FString Key;            // in the review file
		FString Title;
		FString Hint;
		USoundBase* Sound = nullptr;   // kept alive by Keep
		FString AssetFile;      // .uasset on disk, for the "changed since you judged it" check
		float Volume = 1.f;
		float Pitch = 1.f;
		float Distance = 0.f;
		int32 Index = 0;        // scene surface, weather index...
		float Seconds = 0.f;    // how long it plays (0 = open-ended)
	};

	void Begin(bool bOnlyNew);
	void Finish();
	void BuildSteps();
	void AddGroup(const FString& Title, const FString& Where);
	FStep& AddStep(EKind Kind, const FString& Key, const FString& Title, const FString& Hint);
	USoundBase* Load(const TCHAR* Folder, const FString& Name, FString* OutFile = nullptr);
	void AddSet(EKind Kind, const TCHAR* Folder, const TCHAR* Prefix, int32 Max, float Volume, float Distance, const FString& Hint);

	void Enter(int32 Index);
	void Leave();
	void Go(int32 Delta);
	void NextGroup();
	void Jump(const FString& Word);
	void Rate(const TCHAR* Verdict, const FString& Note);
	void AddNote(const FString& Note);

	void TickStep(float DeltaTime);
	void UpdatePanel();
	void ReadKeys();

	UAudioComponent* Play2D(USoundBase* Sound, float Volume, float Pitch = 1.f);
	UAudioComponent* PlayAt(USoundBase* Sound, const FVector& At, float Volume, float Pitch = 1.f, USoundAttenuation* Attenuation = nullptr);
	void PlayAround(USoundBase* Sound, float Volume, float Distance);
	void PlayThunder(USoundBase* Sound);
	void PlayAvalanche(USoundBase* Sound);
	void StopAll();
	USoundBase* PickFrom(const TCHAR* Prefix) const;
	FVector Feet() const;

	FString HashOf(const FStep& Step) const;
	bool IsApproved(const FStep& Step) const;
	void LoadReview();
	void SaveReview() const;

	TArray<FGroup> Groups;
	TArray<FStep> Steps;     // what this tour plays (snd new: only what still needs your ear)
	TArray<FStep> All;       // every sound, for the review file
	mutable TMap<FString, FString> FileHashes;
	UPROPERTY(Transient) TArray<TObjectPtr<USoundBase>> Keep;
	TArray<TWeakObjectPtr<UAudioComponent>> Playing;
	UPROPERTY(Transient) TObjectPtr<USoundAttenuation> FromDirection;
	UPROPERTY(Transient) TObjectPtr<USoundTourWidget> Panel;
	TWeakObjectPtr<AActor> StormPlanet;

	TSharedPtr<class FJsonObject> Review;
	FString LastRated;
	FString LastRatedTitle;
	float SavedFlash = 0.f;

	bool bRunning = false;
	int32 Current = -1;
	float T = 0.f, PrevT = 0.f;
	float Gauge = -1.f;      // what a scene shows: wind 0..10, effort 0..1
	FString SceneLine;

	// the game's own loudness, read from its components at the start
	float FootstepVolume = 0.5f, WindVolume = 0.75f, BreathVolume = 0.6f, RopeVolume = 0.7f, AvalancheVolume = 0.8f;
};
