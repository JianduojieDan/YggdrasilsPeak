// Copyright Epic Games, Inc. All Rights Reserved.

#include "Survival/MountainAudioComponent.h"

#include "Survival/ExpeditionComponent.h"
#include "Environment/RealmSky.h"
#include "Survival/SurvivalComponent.h"
#include "Survival/SurvivalTypes.h"
#include "Components/AudioComponent.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundAttenuation.h"
#include "Sound/SoundBase.h"

static TAutoConsoleVariable<int32> CVarAudioLog(TEXT("ygg.Audio.Log"), 0, TEXT("1 = log the mountain sound mix (wind beds, effort, rope) every 2 s."));

static FAutoConsoleCommandWithWorld CmdAvalanche(TEXT("ygg.Avalanche"), TEXT("A distant avalanche now."),
	FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World)
	{
		if (APawn* Pawn = UGameplayStatics::GetPlayerPawn(World, 0))
		{
			if (UMountainAudioComponent* Audio = Pawn->FindComponentByClass<UMountainAudioComponent>())
			{
				Audio->Avalanche();
			}
		}
	}));

namespace
{
	USoundBase* LoadSound(const TCHAR* Dir, const FString& Name)
	{
		return LoadObject<USoundBase>(nullptr, *(FString(Dir) / Name + TEXT(".") + Name), nullptr, LOAD_NoWarn | LOAD_Quiet);
	}
	void LoadSet(TArray<TObjectPtr<USoundBase>>& Into, const TCHAR* Dir, const TCHAR* Prefix, int32 Max)
	{
		for (int32 i = 1; i <= Max; ++i)
		{
			if (USoundBase* S = LoadSound(Dir, FString::Printf(TEXT("%s_%02d"), Prefix, i)))
			{
				Into.Add(S);
			}
		}
	}
	USoundBase* Pick(const TArray<TObjectPtr<USoundBase>>& Set)
	{
		return Set.Num() > 0 ? Set[FMath::RandRange(0, Set.Num() - 1)].Get() : nullptr;
	}
	float SS(float A, float B, float X) { return FMath::SmoothStep(A, B, X); }
}

UMountainAudioComponent::UMountainAudioComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UMountainAudioComponent::BeginPlay()
{
	Super::BeginPlay();
	Expedition = GetOwner()->FindComponentByClass<UExpeditionComponent>();
	Survival = GetOwner()->FindComponentByClass<USurvivalComponent>();
	LoadSounds();

	// distant sounds come from a direction but do not fade with distance (they are far by design)
	FromDirection = NewObject<USoundAttenuation>(this);
	FSoundAttenuationSettings& A = FromDirection->Attenuation;
	A.bAttenuate = false;
	A.bSpatialize = true;
	A.StereoSpread = 200000.f;
	A.bAttenuateWithLPF = false;
	A.bEnableOcclusion = false;

	NextAvalanche = FMath::FRandRange(AvalancheInterval.X * 0.4f, AvalancheInterval.Y * 0.6f);
	MuteWeatherWind();
}

void UMountainAudioComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	for (UAudioComponent* Bed : Beds)
	{
		if (Bed) { Bed->Stop(); }
	}
	if (RopeSlide) { RopeSlide->Stop(); }
	Super::EndPlay(EndPlayReason);
}

UAudioComponent* UMountainAudioComponent::MakeLoop(USoundBase* Sound)
{
	if (!Sound)
	{
		return nullptr;
	}
	UAudioComponent* C = UGameplayStatics::CreateSound2D(this, Sound, 1.f, 1.f, 0.f, nullptr, false, false);
	if (C)
	{
		C->SetVolumeMultiplier(0.0001f);
		C->Play(FMath::FRandRange(0.f, Sound->GetDuration() * 0.9f));    // the beds never start in step
	}
	return C;
}

void UMountainAudioComponent::LoadSounds()
{
	static const TCHAR* BedNames[NumBeds] = { TEXT("Wind_Calm"), TEXT("Wind_Gusty"), TEXT("Wind_Blizzard"), TEXT("Wind_Whistle") };
	Beds.SetNum(NumBeds);
	for (int32 i = 0; i < NumBeds; ++i)
	{
		Beds[i] = MakeLoop(LoadSound(TEXT("/Game/Audio/Weather"), BedNames[i]));
	}
	LoadSet(Gusts, TEXT("/Game/Audio/Weather"), TEXT("Wind_Gust"), 4);
	LoadSet(Breaths, TEXT("/Game/Audio/Body"), TEXT("Breath_Phrase"), 12);
	LoadSet(Avalanches, TEXT("/Game/Audio/Ice"), TEXT("Avalanche"), 8);
	LoadSet(Clicks, TEXT("/Game/Audio/Gear"), TEXT("Carabiner_Click"), 8);
	LoadSet(ClipIns, TEXT("/Game/Audio/Gear"), TEXT("Carabiner_ClipIn"), 8);
	LoadSet(Tightens, TEXT("/Game/Audio/Gear"), TEXT("Rope_Tighten"), 8);
	if (USoundBase* Slide = LoadSound(TEXT("/Game/Audio/Gear"), TEXT("Rope_Slide_Loop")))
	{
		RopeSlide = UGameplayStatics::CreateSound2D(this, Slide, 1.f, 1.f, 0.f, nullptr, false, false);
	}
	UE_LOG(LogTemp, Display, TEXT("MountainAudio: beds %d/%d, breath phrases %d, avalanches %d, clicks %d, clip-ins %d, rope pulls %d, slide %s"),
		Beds.FilterByPredicate([](const TObjectPtr<UAudioComponent>& B) { return B != nullptr; }).Num(), int32(NumBeds),
		Breaths.Num(), Avalanches.Num(), Clicks.Num(), ClipIns.Num(), Tightens.Num(), RopeSlide ? TEXT("yes") : TEXT("no"));
}

void UMountainAudioComponent::MuteWeatherWind()
{
	// Ultra Dynamic Weather plays its own wind; ours replaces it (its rain and thunder stay as they are)
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		AActor* Weather = *It;
		if (!Weather->GetClass()->GetName().Contains(TEXT("Ultra_Dynamic_Weather")))
		{
			continue;
		}
		for (const TCHAR* Name : { TEXT("Wind Volume"), TEXT("Wind Whistling Volume") })
		{
			FProperty* P = Weather->GetClass()->FindPropertyByName(Name);
			if (FDoubleProperty* D = CastField<FDoubleProperty>(P)) { D->SetPropertyValue_InContainer(Weather, 0.0); }
			else if (FFloatProperty* F = CastField<FFloatProperty>(P)) { F->SetPropertyValue_InContainer(Weather, 0.f); }
		}
		UFunction* Apply = Weather->FindFunction(TEXT("Apply Sound Effects Volume Levels"));
		if (Apply && Apply->ParmsSize == 0)
		{
			Weather->ProcessEvent(Apply, nullptr);
		}
		UE_LOG(LogTemp, Display, TEXT("MountainAudio: weather wind sound off (apply function %s)"), Apply ? (Apply->ParmsSize == 0 ? TEXT("called") : TEXT("has parameters, skipped")) : TEXT("not found"));
		bWeatherMuted = true;
		return;
	}
}

void UMountainAudioComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (!bWeatherMuted && (MuteRetry -= DeltaTime) <= 0.f)
	{
		MuteRetry = 2.f;
		MuteWeatherWind();
	}
	UpdateWind(DeltaTime);
	UpdateBreath(DeltaTime);
	UpdateRope(DeltaTime);

	const float Elevation = Survival.IsValid() ? Survival->GetElevationMeters() : 0.f;
	if (Elevation > AvalancheAboveMeters && (NextAvalanche -= DeltaTime) <= 0.f)
	{
		NextAvalanche = FMath::FRandRange(AvalancheInterval.X, AvalancheInterval.Y);
		Avalanche();
	}

	if (CVarAudioLog.GetValueOnGameThread() > 0 && (LogTimer -= DeltaTime) <= 0.f)
	{
		LogTimer = 2.f;
		UE_LOG(LogTemp, Display, TEXT("MountainAudio: wind %.1f storm %.2f elev %.0f | calm %.2f gusty %.2f blizzard %.2f whistle %.2f muffle %.2f | effort %.2f | rope %.2f"),
			Expedition.IsValid() ? Expedition->GetWindIntensity() : -1.f, Expedition.IsValid() ? Expedition->GetStormStrength() : -1.f, Elevation,
			BedLevel[Calm], BedLevel[Gusty], BedLevel[Blizzard], BedLevel[Whistle], Muffle, Effort, RopeLevel);
	}
}

void UMountainAudioComponent::UpdateWind(float DeltaTime)
{
	float W = TourWind >= 0.f ? TourWind : Expedition.IsValid() ? Expedition->GetWindIntensity() : 3.f;       // 0..10
	float Storm = TourWind >= 0.f ? 0.f : Expedition.IsValid() ? Expedition->GetStormStrength() : 0.f;        // 0..1
	// A realm brings its own air, and the sound must arrive and leave WITH what you see (the weather system's wind
	// ramps slowly: the blizzard came late into Jotunheim and roared on into the next, calm realm)
	if (TourWind < 0.f)
	{
		if (!Sky.IsValid()) { Sky = ARealmSky::Get(GetWorld()); }
		float Blend = 0.f;
		const float RealmWind = Sky.IsValid() ? Sky->GetRealmWind(Blend) : -1.f;
		if (RealmWind >= 0.f && Blend > 0.f)
		{
			W = FMath::Lerp(W, RealmWind * 10.f, Blend);
			Storm = FMath::Lerp(Storm, 0.f, Blend);
		}
	}
	const float E = TourElevation >= 0.f ? TourElevation : Survival.IsValid() ? Survival->GetElevationMeters() : 5000.f;

	float Target[NumBeds];
	Target[Calm] = FMath::Lerp(0.6f, 0.2f, SS(3.f, 7.f, W));                          // the air itself, always a little there
	Target[Gusty] = SS(2.5f, 5.5f, W) * (1.f - 0.5f * SS(7.f, 9.5f, W)) * 0.8f;
	Target[Blizzard] = FMath::Max(SS(6.f, 9.5f, W), Storm) * 0.95f;
	Target[Whistle] = SS(4.f, 7.5f, W) * SS(5600.f, 7600.f, E) * 0.7f;               // only high and exposed

	const bool bSheltered = Survival.IsValid() && Survival->IsSheltered();
	Muffle = FMath::FInterpTo(Muffle, bSheltered ? 1.f : 0.f, DeltaTime, 1.5f);
	const float Overall = WindVolume * FMath::Lerp(0.55f, 1.f, SS(4000.f, 7000.f, E)) * FMath::Lerp(1.f, 0.4f, Muffle);
	for (int32 i = 0; i < NumBeds; ++i)
	{
		BedLevel[i] = FMath::FInterpTo(BedLevel[i], Target[i], DeltaTime, 1.2f);
		if (UAudioComponent* Bed = Beds.IsValidIndex(i) ? Beds[i].Get() : nullptr)
		{
			Bed->SetVolumeMultiplier(bTourMuteWind ? 0.0001f : FMath::Max(0.0001f, BedLevel[i] * Overall));
			Bed->SetLowPassFilterEnabled(Muffle > 0.01f);
			Bed->SetLowPassFilterFrequency(FMath::Lerp(20000.f, 700.f, Muffle));
		}
	}

	// single gusts on top in strong wind
	NextGust -= DeltaTime;
	if (NextGust <= 0.f)
	{
		NextGust = FMath::FRandRange(20.f, 50.f);
		if (W > 6.f && Gusts.Num() > 0 && !bTourMuteWind)
		{
			UGameplayStatics::PlaySound2D(this, Pick(Gusts), WindVolume * 0.6f * SS(6.f, 9.5f, W) * FMath::Lerp(1.f, 0.4f, Muffle), FMath::FRandRange(0.85f, 1.1f));
		}
	}
}

void UMountainAudioComponent::UpdateBreath(float DeltaTime)
{
	if (!Survival.IsValid() || Survival->IsDead())
	{
		return;
	}
	const float Stamina = Survival->GetStatNormalized(ESurvivalStat::Stamina);
	const FVector V = GetOwner()->GetVelocity();
	const float Speed = float(V.Size2D());
	const float Climb = FMath::Max(0.f, float(V.Z));
	const float Thin = SS(4500.f, 8500.f, Survival->GetElevationMeters());
	float Target = 0.45f * (1.f - Stamina) + 0.25f * SS(50.f, 450.f, Speed) + 0.2f * SS(10.f, 80.f, Climb)
		+ 0.35f * Thin * SS(30.f, 300.f, Speed) + 0.15f * Thin + (Survival->IsSprinting() ? 0.25f : 0.f);
	Target = TourEffort >= 0.f ? TourEffort : FMath::Clamp(Target, 0.f, 1.f);
	Effort = TourEffort >= 0.f ? TourEffort : FMath::FInterpTo(Effort, Target, DeltaTime, Target > Effort ? 0.8f : 0.25f);   // builds fast, settles slowly

	// Whole phrases of real breathing (several breaths each, their own rhythm), back to back while the effort lasts.
	// A phrase plays to its end; the next one is picked for the effort at that moment (sorted soft -> hard).
	NextBreath -= DeltaTime;
	if (NextBreath > 0.f || Breaths.Num() == 0)
	{
		return;
	}
	if (Effort < BreathFromEffort)
	{
		NextBreath = 0.5f;                                                  // at rest, low down: no audible breathing
		return;
	}
	const int32 N = Breaths.Num();
	int32 i = FMath::Clamp(FMath::RoundToInt(Effort * (N - 1)) + FMath::RandRange(-1, 1), 0, N - 1);
	if (N > 1 && i == LastBreath) { i = (i + 1 < N) ? i + 1 : i - 1; }    // never the same phrase twice in a row
	LastBreath = i;
	const float Volume = BreathVolume * FMath::Lerp(0.35f, 1.f, SS(BreathFromEffort, 0.9f, Effort));
	UGameplayStatics::PlaySound2D(this, Breaths[i], Volume);
	NextBreath = Breaths[i]->GetDuration() + FMath::FRandRange(0.05f, 0.4f);
}

void UMountainAudioComponent::UpdateRope(float DeltaTime)
{
	const bool bOn = Expedition.IsValid() && Expedition->IsOnRope();
	if (bOn != bWasOnRope)
	{
		bWasOnRope = bOn;
		// clipping in: a whole carabiner gesture (gate, clip, rattle) on its own -- the user heard the rope pull after it
		// as wrong (review round 2); unclipping: a single click. (The approved rope pulls stay loaded for later use.)
		USoundBase* Gesture = bOn ? Pick(ClipIns) : nullptr;
		if (USoundBase* Sound = Gesture ? Gesture : Pick(Clicks))
		{
			UGameplayStatics::PlaySound2D(this, Sound, RopeVolume * 0.8f, FMath::FRandRange(0.97f, 1.03f));
		}
		TightenIn = -1.f;
	}
	if (TightenIn > 0.f && (TightenIn -= DeltaTime) <= 0.f)
	{
		if (USoundBase* Pull = Pick(Tightens))
		{
			UGameplayStatics::PlaySound2D(this, Pull, RopeVolume * 0.7f, FMath::FRandRange(0.95f, 1.05f));
		}
	}
	if (!RopeSlide)
	{
		return;
	}
	const float Speed = float(GetOwner()->GetVelocity().Size());
	RopeLevel = FMath::FInterpTo(RopeLevel, bOn ? SS(40.f, 320.f, Speed) : 0.f, DeltaTime, 6.f);
	if (RopeLevel > 0.01f && !RopeSlide->IsPlaying())
	{
		RopeSlide->Play(FMath::FRandRange(0.f, 3.f));
	}
	else if (RopeLevel < 0.005f && RopeSlide->IsPlaying())
	{
		RopeSlide->Stop();
	}
	RopeSlide->SetVolumeMultiplier(FMath::Max(0.0001f, RopeLevel * RopeVolume * 0.35f));
	RopeSlide->SetPitchMultiplier(0.85f + 0.3f * SS(40.f, 450.f, Speed));
}

const TArray<TObjectPtr<USoundBase>>& UMountainAudioComponent::GetSet(const TCHAR* Prefix) const
{
	static const TArray<TObjectPtr<USoundBase>> None;
	const FString P(Prefix);
	if (P == TEXT("Breath_Phrase")) { return Breaths; }
	if (P == TEXT("Avalanche")) { return Avalanches; }
	if (P == TEXT("Carabiner_Click")) { return Clicks; }
	if (P == TEXT("Carabiner_ClipIn")) { return ClipIns; }
	if (P == TEXT("Rope_Tighten")) { return Tightens; }
	if (P == TEXT("Wind_Gust")) { return Gusts; }
	return None;
}

void UMountainAudioComponent::Avalanche(int32 Index)
{
	USoundBase* Roar = Avalanches.IsValidIndex(Index) ? Avalanches[Index].Get() : Pick(Avalanches);
	if (!Roar)
	{
		return;
	}
	// somewhere across the valley, a little above: a direction, not a distance
	const FVector At = GetOwner()->GetActorLocation() + FRotator(0.f, FMath::FRandRange(0.f, 360.f), 0.f).Vector() * 300000.f + FVector(0.f, 0.f, 50000.f);
	UGameplayStatics::PlaySoundAtLocation(this, Roar, At, FRotator::ZeroRotator, AvalancheVolume, FMath::FRandRange(0.9f, 1.05f), 0.f, FromDirection);
	UE_LOG(LogTemp, Display, TEXT("MountainAudio: distant avalanche (%s)"), *Roar->GetName());
}
