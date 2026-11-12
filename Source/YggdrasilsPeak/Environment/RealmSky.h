// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RealmSky.generated.h"

class UStaticMeshComponent;
class UMaterialInstanceDynamic;

/** The nine realms (Docs/ArtDirection/realms, Nine Realms brief). */
UENUM(BlueprintType)
enum class ERealm : uint8
{
	Midgard, Niflheim, Muspelheim, Jotunheim, Alfheim, Vanaheim, Svartalfheim, Hel, Asgard,
	None UMETA(Hidden)
};

/** What the player is told about a weather. */
struct FRealmWeatherInfo { const TCHAR* Name; const TCHAR* Warning; int32 Danger; };

/** One realm's mood. Defaults are neutral: nothing changes the world's own light. Table in RealmSky.cpp. */
struct FRealmLook
{
	float Active = 0.f;                        // 0 = the world's own values
	float SunScale = 1.f;
	FLinearColor SunColor = FLinearColor::White;
	float SkyScale = 1.f;
	FLinearColor SkyColor = FLinearColor::White;
	float FogDensity = 0.005f, FogFalloff = 0.012f, FogBelow = 0.f, FogCutoff = 1.f;   // FogBelow: layer base, cm under the player; FogCutoff 0: fog covers the sky
	FLinearColor FogColor = FLinearColor::White;
	float Saturation = 1.f, Contrast = 1.f;
	FLinearColor Gain = FLinearColor::White;
	float PlanetAlpha = 1.f;                   // how much of the planet the air lets through
	float Temp = 6500.f, Exposure = 0.f, Bloom = 0.675f, Vignette = 0.4f;
	float CloudCover = 0.f, CloudSpeed = 5.f, CloudScale = 6.f;   // deck: 0..1, m/s, km per billow
	FLinearColor CloudDark = FLinearColor(0.3f, 0.32f, 0.36f), CloudLight = FLinearColor::White;
	float Wind = -1.f;                         // 0..1, spindrift strength; below 0 = the world's own
	float HealthDrain = 0.f;
	float Cold = 0.f, StaminaDrain = 0.f, SanityGain = 0.f;   // per second, on the body and mind: degrees C lost, stamina lost, sanity gained
	static FRealmLook Lerp(const FRealmLook& A, const FRealmLook& B, float T);
};

/**
 *  The realm planets in the sky. Ordinarily one realm is visible at a time (the sign of its weather); the summit shows
 *  all nine together. Owns the Realm_* planet actors placed by Tools/Realms/place_realms.py (found by label), fades
 *  their brightness in and out, and keeps them centred on the player so walking never moves them.
 *  Console: ygg.Realm <name|none|all>  - show one realm, hide all, or the summit view.
 *  The realm weather itself (fog, ember snow, aurora, eclipse, ...) is driven separately; this is only the sky sign.
 */
UCLASS()
class YGGDRASILSPEAK_API ARealmSky : public AActor
{
	GENERATED_BODY()

public:
	ARealmSky();

	/** Show one realm (fades the others out), or None to clear the sky. */
	UFUNCTION(BlueprintCallable, Category="Realms")
	void ShowRealm(ERealm Realm);

	/** The summit payoff: all nine at once. */
	UFUNCTION(BlueprintCallable, Category="Realms")
	void ShowAll();

	UFUNCTION(BlueprintPure, Category="Realms")
	ERealm GetShownRealm() const { return Shown; }

	/** Seconds for a planet to fade fully in or out. */
	UPROPERTY(EditAnywhere, Category="Realms", meta=(ClampMin="0.1"))
	float FadeSeconds = 6.f;

	static ARealmSky* Get(UWorld* World);

	/** Re-reads Config/RealmLooks.json and re-applies the shown realm (ygg.Realm reload). */
	void ReloadLooks();

	/** Random weather: the realms come and go in a shuffled bag, 10-12 minutes each, one after another. */
	void SetScheduled(bool bOn);
	void ForecastNow();
	static const FRealmWeatherInfo& GetWeatherInfo(int32 Index);
	static int32 NumWeathers();
	/** Sound asset name (/Game/Audio/UI) for the weather wheel landing on this realm; more intense for more danger. */
	static const FString& GetLandingSound(int32 Index);
	bool HasForecast() const { return DemoIndex >= 0 ? bDemoAnnounced : (bScheduled && bForecast && bAnnounced && !bAtSummit); }
	int32 GetForecastIndex() const { return int32(NextRealm); }
	/** The weather the wheel lands on: the forecast, or a realm named by hand. */
	int32 GetWheelIndex() const { return WheelIndex; }
	void PlayWheel(ERealm Realm) { WheelIndex = int32(Realm); ++ForecastSerial; }
	/** The sound tour (snd): the wheel spins and lands on this weather even while a forecast is up. */
	void SpinWheelTo(int32 Index) { WheelIndex = Index; bHandWheel = true; ++ForecastSerial; }
	/** True once after SpinWheelTo: the HUD shows WheelIndex instead of the forecast. */
	bool TakeHandWheel() { const bool b = bHandWheel; bHandWheel = false; return b; }
	/** One lightning strike now, Km away towards the storm: a visible bolt and a flash, its thunder at the speed of sound
	 *  (Index = which thunder, -1 random; bFlashOnly: no thunder, the caller plays its own). */
	void Strike(int32 ThunderIndex = -1, float Km = 1.5f, bool bFlashOnly = false, float Branchiness = -1.f);
	/** A burst of Count strikes at once at different distances, as the Jotunheim storm makes them. */
	void StormBurst(int32 Count);
	/** How a thunder Km away sounds: quieter, duller (low-pass Hz) and a little lower the further it struck. */
	static void ThunderVoice(float Km, float& OutVolume, float& OutPitch, float& OutLowPassHz);
	/** The realm's wind 0..1 (below 0 = the world's own) and how much the realm holds the air now (0..1). */
	float GetRealmWind(float& OutBlend) const { OutBlend = FMath::Min(Look.Active * 4.f, 1.f); return Look.Wind; }
	bool IsDemo() const { return DemoIndex >= 0; }
	int32 GetShownIndex() const { return int32(Shown); }
	void LogStatus() const;
	float GetForecastSeconds() const { return DemoIndex >= 0 ? DemoLeft : WeatherLeft; }
	int32 GetForecastSerial() const { return ForecastSerial; }
	float GetSkyHoursSinceShown() const;

	/** ygg.Realm route: the realm changes as the player reaches each route landmark (prototype, off by default). */
	bool bFollowRoute = false;

	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void BeginPlay() override;

private:
	struct FPlanet
	{
		TWeakObjectPtr<AActor> Actor;
		TArray<TWeakObjectPtr<UMaterialInstanceDynamic>> Materials;
		float BaseBright = 2.f;
		float Fade = 0.f;
		float Target = 0.f;
		FVector BaseScale = FVector::OneVector;
		FVector Offset = FVector::ZeroVector;   // from the view point it was placed around
	};
	TArray<FPlanet> Planets;                   // index = ERealm, plus Hel's trail at the end
	ERealm Shown = ERealm::None;
	bool bTimeLapseOn = false;
	bool bAll = false;
	FVector PlacedAround = FVector::ZeroVector;
	FRealmLook Look;                           // what is on screen now, gliding towards the shown realm's row
	float TransitionTau = 3.5f;                // ~10 s from one realm's mood to the next
	bool bLookOn = false, bBasesKnown = false;
	void ApplyLook(float DeltaSeconds, const FVector& Around);
	UPROPERTY(VisibleAnywhere) TObjectPtr<class UPostProcessComponent> Grade;
	TWeakObjectPtr<class UDirectionalLightComponent> Sun;
	TWeakObjectPtr<class USkyLightComponent> SkyLight;
	TWeakObjectPtr<class UExponentialHeightFogComponent> Fog;
	float SunBase = 5.f, SkyBase = 1.5f, FogBase = 0.005f, FalloffBase = 0.012f, CutoffBase = 0.f;
	FLinearColor SunColor = FLinearColor::White, FogColorBase = FLinearColor::White;
	FVector FogLocation = FVector::ZeroVector;
	float NextFlash = 4.f, FlashLeft = 0.f, ThunderIn = 0.f;
	bool bHandWheel = false;
	struct FStrike
	{
		float FlashIn = 0.f;      // s until its flash
		float ThunderIn = 0.f;    // s from the flash to its thunder (distance / speed of sound)
		FVector Dir = FVector::ForwardVector;
		float Km = 1.f;
		int32 Thunder = -1;
		bool bFlashed = false;
		bool bFlashOnly = false;
		float Branchiness = -1.f; // 0 a lone channel .. 1 a heavy fork tree; below 0 = drawn by chance
	};
	struct FBoltRef { TWeakObjectPtr<class ALightningBolt> Bolt; float Power = 1.f; FVector Dir = FVector::ForwardVector; };
	TArray<FBoltRef> Bolts;                    // the hand-drawn strikes still glowing (they light the world on their strokes)
	TArray<FStrike> Strikes;
	float FlashPower = 0.f;                    // 0..1, how bright the current flash is (close strikes brighter)
	FVector FlashDir = FVector::ForwardVector;
	void QueueStrike(float Delay, float Km, int32 ThunderIndex, bool bFlashOnly, float Branchiness = -1.f);
	void TickStrikes(float DeltaSeconds);
	void DoFlash(const FStrike& S);
	void LoadThunders();
	UPROPERTY(Transient) TObjectPtr<class UDirectionalLightComponent> LightningLight;
	UPROPERTY(Transient) TObjectPtr<class UNiagaraSystem> BoltSystem;
	UPROPERTY(Transient) TObjectPtr<class UNiagaraSystem> CloudFlashSystem;
	UPROPERTY(Transient) TArray<TObjectPtr<class USoundBase>> Thunders;   // /Game/Audio/Weather/Thunder_NN
	int32 LastThunder = -1;
	bool bSkySaved = false;
	double BaseCloud = 0.9;
	void SetTargets();
	void TickWeather(float DeltaSeconds, const FVector& Around);
	void TickTour(float DeltaSeconds);
	void TickDemo(float DeltaSeconds);
	int32 DemoIndex = -1, WheelIndex = 0;
	bool bDemoAnnounced = false;
	float DemoLeft = 0.f, SavedTau = 3.5f, SavedFade = 6.f;
	bool bStartTimeSet = false;
	TWeakObjectPtr<AActor> SkyCached;
	TArray<FString> PerfSteps;
	float PerfTime = 0.f, PerfFrame = 0.f, PerfGpu = 0.f, PerfRender = 0.f;
	int32 PerfFrames = 0;
	int32 TourIndex = -1, TourFrames = 0;
	float TourTime = 0.f, TourFrame = 0.f, TourGame = 0.f, TourRender = 0.f, TourGpu = 0.f;
	ERealm DrawFromBag();
	TArray<int32> Bag;
	bool bScheduled = true, bForecast = false, bAnnounced = false, bAtSummit = false;
	ERealm NextRealm = ERealm::Midgard;
	float WeatherLeft = 240.f, ForecastLeft = 45.f, AnnounceIn = -1.f;
	int32 ForecastSerial = 0;
	float SkyHours = 0.f, ShownAtHours = 0.f;
	double LastTod = -1.0;
	// Where a lone realm planet stands: on the sun by day, on the moon by night (smoothed: the moon light moves in steps)
	TWeakObjectPtr<class UDirectionalLightComponent> MoonLight;
	FVector SmoothMoon = FVector::ZeroVector;
	FVector PlanetAnchor(FVector& OutDir, float DeltaSeconds);   // returns (horizon fade, night dim, unused)
	void MakeAsgardRings();
	UPROPERTY(Transient) TObjectPtr<UStaticMeshComponent> Ring;
	UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> RingMat;
	float RingScale = 7.f, RingTilt = 30.f, RingRoll = 35.f;   // 7: rings out to 3.5 planet radii, starting just above the surface
	float SunHide = 0.f;
	TWeakObjectPtr<class UMaterialInstanceDynamic> SkySphereMID;
	TWeakObjectPtr<class UStaticMeshComponent> SkySphere;
	float SunBrightBase = -1.f, SunHighlightBase = -1.f, SunRadiusBase = 0.0214f;
	bool bSunOverridden = false;
	FDelegateHandle PostTickHandle;
	void ApplySunHide();
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	FLinearColor SunDiskBase = FLinearColor::White;
	UPROPERTY(Transient) TObjectPtr<UStaticMeshComponent> Deck;
	UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> DeckMat;
	void ChangeWeather();
	FString LastWeather;
	void UpdateRouteRealm(const FVector& Where);
	int32 RouteIndex = -1;
	bool bInHel = false;
	void SetParticles(TObjectPtr<class UNiagaraComponent>& Comp, const TCHAR* Path, bool bOn);
	UPROPERTY(Transient) TArray<TObjectPtr<class UNiagaraComponent>> AirFx;
};
