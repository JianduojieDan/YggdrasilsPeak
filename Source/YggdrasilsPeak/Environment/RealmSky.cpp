// Copyright Epic Games, Inc. All Rights Reserved.

#include "Environment/RealmSky.h"

#include "Components/DirectionalLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PostProcessComponent.h"
#include "Components/SkyLightComponent.h"
#include "Sound/SoundBase.h"
#include "EngineUtils.h"
#include "Engine/StaticMeshActor.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Environment/RouteLandmark.h"
#include "Dom/JsonObject.h"
#include "Misc/FileHelper.h"
#include "Misc/OutputDeviceNull.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "RHI.h"
#include "RenderTimer.h"
#include "NiagaraComponent.h"
#include "Survival/SanityComponent.h"
#include "Survival/SurvivalComponent.h"
#include "Survival/YggTime.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "UObject/UnrealType.h"

namespace
{
	const TCHAR* RealmLabels[] = { TEXT("Midgard"), TEXT("Niflheim"), TEXT("Muspelheim"), TEXT("Jotunheim"), TEXT("Alfheim"),
		TEXT("Vanaheim"), TEXT("Svartalfheim"), TEXT("Hel"), TEXT("Asgard"), TEXT("HelTrail"), TEXT("AsgardPillar"), TEXT("AlfheimBow") };
	constexpr int32 NumRealms = 9;
	FString GOrdinaryWeather = TEXT("Clear_Skies");   // the Ultra Dynamic preset used between realms
	constexpr int32 HelTrail = 9;
	constexpr int32 AsgardPillar = 10;
	constexpr int32 AlfheimBow = 11;
	constexpr int32 NumSlots = 12;
	constexpr double BowDistance = 100000.0;   // Tools/Realms/place_prism_bow.py: the spot is this far along the disc's +Z
	constexpr double BowSpotRadius = 12000.0;  // the bow only shows from within 120 m of the spot
}

namespace
{
	void LoadLookOverrides();

	// Apparent diameter (degrees) and direction, one row per realm. Yaw 12 looks up the Western Cwm from the showcase spot past Camp 1.
	struct FPlanetSky { float Degrees, Yaw, Pitch; };
	FPlanetSky GPlanetSky[9] = {
		{ 24.f, 22.f, 26.f },   // Midgard
		{ 38.f, 5.f, 24.f },    // Niflheim
		{ 15.f, 22.f, 25.f },   // Muspelheim
		{ 72.f, 12.f, 36.f },   // Jotunheim: a wall in the sky
		{ 9.f, 14.f, 25.f },    // Alfheim
		{ 30.f, 2.f, 30.f },    // Vanaheim
		{ 4.5f, 12.f, 30.f },   // Svartalfheim: a small hard disc
		{ 18.f, 25.f, 22.f },   // Hel
		{ 56.f, 20.f, 34.f },   // Asgard
	};
	const FPlanetSky& PlanetSky(int32 Index) { return GPlanetSky[Index]; }
}

static TAutoConsoleVariable<float> CVarRealmDemo(TEXT("ygg.Realm.Demo"), 0.f,
	TEXT("Presentation: shows all nine realms in order, this many seconds each, looping (set before or during Play). 0 = back to the random schedule."));

static TAutoConsoleVariable<int32> CVarRealmLook(TEXT("ygg.Realm.LookAt"), 0, TEXT("Checking the sky: holds the camera on 1 the sun, 2 the moon, 3 the realm planet."));
static TAutoConsoleVariable<float> CVarRealmStart(TEXT("ygg.Realm.StartTime"), -1.f, TEXT("Checking the sky: the clock starts here, 0..2400 (set on the command line)."));

static TAutoConsoleVariable<float> CVarTimeLapse(TEXT("ygg.Realm.TimeLapse"), 0.f, TEXT("Watching the sky: one full 24 hour day takes this many seconds (15 = a day in 15 s). 0 = normal time."));

// Short forms for checking: "r 3" or "r mu" shows a realm, "r 0" ordinary weather, "r a" all nine, "tl 15" a day in 15 s, "tl 0" normal time.
static FAutoConsoleCommandWithWorldAndArgs CmdRealmShort(TEXT("r"),
	TEXT("r <1-9|mi ni mu jo al va sv he as|0|a>: 1 Midgard 2 Niflheim 3 Muspelheim 4 Jotunheim 5 Alfheim 6 Vanaheim 7 Svartalfheim 8 Hel 9 Asgard, 0 none, a all"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		if (Args.Num() == 0 || !GEngine) { return; }
		static const TCHAR* const Short[] = { TEXT("mi"), TEXT("ni"), TEXT("mu"), TEXT("jo"), TEXT("al"), TEXT("va"), TEXT("sv"), TEXT("he"), TEXT("as") };
		FString Name;
		if (Args[0] == TEXT("0") || Args[0].Equals(TEXT("no"), ESearchCase::IgnoreCase)) { Name = TEXT("none"); }
		else if (Args[0].Equals(TEXT("a"), ESearchCase::IgnoreCase)) { Name = TEXT("all"); }
		else
		{
			const int32 N = FCString::Atoi(*Args[0]);
			for (int32 i = 0; i < NumRealms; ++i)
			{
				if (N == i + 1 || Args[0].Equals(Short[i], ESearchCase::IgnoreCase)) { Name = RealmLabels[i]; }
			}
		}
		if (!Name.IsEmpty()) { GEngine->Exec(World, *(TEXT("ygg.Realm ") + Name)); }
	}));

static FAutoConsoleCommandWithWorldAndArgs CmdTimeLapseShort(TEXT("tl"), TEXT("tl <seconds>: one 24 hour day takes this long; tl 0 is normal time"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld*)
	{
		CVarTimeLapse->Set(Args.Num() ? FCString::Atof(*Args[0]) : 0.f, ECVF_SetByConsole);
	}));

static FAutoConsoleCommandWithWorldAndArgs CmdRealm(TEXT("ygg.Realm"),
	TEXT("Shows a realm planet in the sky: ygg.Realm <Midgard|Niflheim|Muspelheim|Jotunheim|Alfheim|Vanaheim|Svartalfheim|Hel|Asgard|none|all>"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		ARealmSky* Sky = ARealmSky::Get(World);
		if (!Sky || Args.Num() == 0)
		{
			return;
		}
		if (Args[0].Equals(TEXT("reload"), ESearchCase::IgnoreCase)) { Sky->ReloadLooks(); return; }
		if (Args[0].Equals(TEXT("auto"), ESearchCase::IgnoreCase)) { Sky->SetScheduled(true); return; }
		if (Args[0].Equals(TEXT("status"), ESearchCase::IgnoreCase)) { Sky->LogStatus(); return; }
		if (Args[0].Equals(TEXT("demo"), ESearchCase::IgnoreCase)) { CVarRealmDemo->Set(Args.Num() > 1 ? FCString::Atof(*Args[1]) : 10.f, ECVF_SetByConsole); return; }
		if (Args[0].Equals(TEXT("time"), ESearchCase::IgnoreCase)) { if (Args.Num() > 1) { YggTime::Set(World, FCString::Atof(*Args[1])); } return; }
		if (Args[0].Equals(TEXT("next"), ESearchCase::IgnoreCase)) { Sky->ForecastNow(); return; }
		Sky->SetScheduled(false);   // a realm named by hand stays until auto
		Sky->bFollowRoute = Args[0].Equals(TEXT("route"), ESearchCase::IgnoreCase);
		if (Sky->bFollowRoute) { return; }
		if (Args[0].Equals(TEXT("all"), ESearchCase::IgnoreCase)) { Sky->ShowAll(); return; }
		for (int32 i = 0; i < NumRealms; ++i)
		{
			if (Args[0].Equals(RealmLabels[i], ESearchCase::IgnoreCase)) { Sky->ShowRealm(ERealm(i)); Sky->PlayWheel(ERealm(i)); return; }
		}
		Sky->ShowRealm(ERealm::None);
	}));

ARealmSky::ARealmSky()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PostUpdateWork;
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
	Grade = CreateDefaultSubobject<UPostProcessComponent>(TEXT("Grade"));
	Grade->SetupAttachment(GetRootComponent());
	Grade->bUnbound = true;
	Grade->Priority = 10.f;
	Grade->BlendWeight = 0.f;
}

ARealmSky* ARealmSky::Get(UWorld* World)
{
	if (!World)
	{
		return nullptr;
	}
	for (TActorIterator<ARealmSky> It(World); It; ++It)
	{
		return *It;
	}
	// none placed: make one at runtime, it finds the planets by label
	return World->SpawnActor<ARealmSky>();
}

void ARealmSky::BeginPlay()
{
	Super::BeginPlay();
	Planets.SetNum(NumSlots);
	LoadLookOverrides();
	// the planets were placed around the summit view point (Tools/Realms/place_realms.py)
	PlacedAround = FVector(1240508.0, 1412617.0, 382663.0);
	for (TActorIterator<AStaticMeshActor> It(GetWorld()); It; ++It)
	{
		const FString Label = It->GetActorNameOrLabel();
		for (int32 i = 0; i < NumSlots; ++i)
		{
			if (Label == FString(TEXT("Realm_")) + RealmLabels[i])
			{
				FPlanet& P = Planets[i];
				P.Actor = *It;
				P.Offset = It->GetActorLocation() - PlacedAround;
				P.BaseScale = It->GetActorScale3D();
				UStaticMeshComponent* Mesh = It->GetStaticMeshComponent();
				for (int32 m = 0; m < Mesh->GetNumMaterials(); ++m)
				{
					if (UMaterialInstanceDynamic* D = Mesh->CreateAndSetMaterialInstanceDynamic(m))
					{
						float B = 2.f;
						D->GetScalarParameterValue(FName(TEXT("Bright")), B);
						P.BaseBright = B;
						D->SetScalarParameterValue(TEXT("Bright"), 0.f);
						D->SetScalarParameterValue(TEXT("Fade"), 0.f);
						P.Materials.Add(D);
					}
				}
				// shown through the fade, not the hidden flag
				It->SetActorHiddenInGame(false);
				Mesh->SetVisibility(false);
			}
		}
	}
	MakeAsgardRings();
	FParse::Value(FCommandLine::Get(), TEXT("YggForecastIn="), AnnounceIn);   // captures: spin the wheel at a known moment
	ShowRealm(ERealm::None);   // the world starts in ordinary clear weather
	if (FParse::Param(FCommandLine::Get(), TEXT("YggPerfSteps")))
	{
		FFileHelper::LoadFileToStringArray(PerfSteps, *(FPaths::ProjectSavedDir() / TEXT("perf_steps.txt")));
		bScheduled = false;
		ShowRealm(ERealm::None);
	}
	if (FParse::Param(FCommandLine::Get(), TEXT("YggTour"))) { TourIndex = 0; bScheduled = false; ShowRealm(ERealm::None); }
}

void ARealmSky::MakeAsgardRings()
{
	FPlanet& P = Planets[int32(ERealm::Asgard)];
	AActor* A = P.Actor.Get();
	UMaterialInterface* Mat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Environment/Realms/M_RealmRing.M_RealmRing"));
	if (!A || !Mat) { return; }
	Ring = NewObject<UStaticMeshComponent>(A);
	Ring->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Plane.Plane")));
	Ring->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Ring->SetCastShadow(false);
	Ring->SetMobility(EComponentMobility::Movable);
	Ring->SetupAttachment(A->GetRootComponent());
	Ring->SetUsingAbsoluteRotation(true);
	Ring->SetRelativeScale3D(FVector(RingScale, RingScale, 1.0));   // plane 1 m, sphere radius 1 m: 12 = rings out to 6 planet radii
	Ring->SetTranslucentSortPriority(1);
	Ring->BoundsScale = 4.f;
	Ring->RegisterComponent();
	RingMat = Ring->CreateAndSetMaterialInstanceDynamicFromMaterial(0, Mat);
	RingMat->SetScalarParameterValue(TEXT("Bright"), 2.f);
	RingMat->SetScalarParameterValue(TEXT("Fade"), 0.f);
	P.Materials.Add(RingMat);
	Ring->SetVisibility(false);
}

void ARealmSky::ShowRealm(ERealm Realm)
{
	Shown = Realm;
	ShownAtHours = SkyHours;
	bAll = false;
	SetTargets();
}

void ARealmSky::ShowAll()
{
	Shown = ERealm::None;
	bAll = true;
	SetTargets();
}

namespace
{
	// Ultra Dynamic Sky is a Blueprint: its variables are set by name through reflection (as ExpeditionComponent reads UDW)
	AActor* FindSky(UWorld* World)
	{
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			if (It->GetClass()->GetName().Contains(TEXT("Ultra_Dynamic_Sky"))) { return *It; }
		}
		return nullptr;
	}
	void SetSkyBool(AActor* Sky, const TCHAR* Name, bool bValue)
	{
		if (FBoolProperty* P = Sky ? CastField<FBoolProperty>(Sky->GetClass()->FindPropertyByName(Name)) : nullptr) { P->SetPropertyValue_InContainer(Sky, bValue); }
	}
	void SetSkyFloat(AActor* Sky, const TCHAR* Name, double Value)
	{
		FProperty* P = Sky ? Sky->GetClass()->FindPropertyByName(Name) : nullptr;
		if (FDoubleProperty* D = CastField<FDoubleProperty>(P)) { D->SetPropertyValue_InContainer(Sky, Value); }
		else if (FFloatProperty* F = CastField<FFloatProperty>(P)) { F->SetPropertyValue_InContainer(Sky, float(Value)); }
	}
	void SetSkyColor(AActor* Sky, const TCHAR* Name, const FLinearColor& C)
	{
		FStructProperty* P = Sky ? CastField<FStructProperty>(Sky->GetClass()->FindPropertyByName(Name)) : nullptr;
		if (P && P->Struct == TBaseStructure<FLinearColor>::Get()) { *P->ContainerPtrToValuePtr<FLinearColor>(Sky) = C; }
	}
}

void ARealmSky::SetTargets()
{
	// Vanaheim's Aurora Tide: green curtains with violet upper edges (UDS aurora), off for every other realm
	if (AActor* Sky = FindSky(GetWorld()))
	{
		// remember the sky's own values once, so clearing a realm puts them back
		if (!bSkySaved)
		{
			FProperty* P = Sky->GetClass()->FindPropertyByName(TEXT("Cloud Coverage"));
			if (FDoubleProperty* D = CastField<FDoubleProperty>(P)) { BaseCloud = D->GetPropertyValue_InContainer(Sky); }
			bSkySaved = true;
		}
		// Niflheim: white fog filling the valleys; Jotunheim: a heavy storm sky
		SetSkyFloat(Sky, TEXT("Cloud Coverage"), Shown == ERealm::Jotunheim ? 3.0 : (Shown == ERealm::Niflheim ? 1.6 : BaseCloud));
		const bool bAurora = Shown == ERealm::Vanaheim || bAll;
		SetSkyBool(Sky, TEXT("Use Auroras"), bAurora);
		if (bAurora)
		{
			SetSkyFloat(Sky, TEXT("Aurora Intensity"), 8.0);
			SetSkyColor(Sky, TEXT("Aurora Color 1"), FLinearColor(0.45f, 0.10f, 0.85f));
			SetSkyColor(Sky, TEXT("Aurora Color 2"), FLinearColor(0.10f, 1.0f, 0.35f));
			SetSkyColor(Sky, TEXT("Aurora Color 3"), FLinearColor(0.05f, 0.6f, 0.30f));
		}
	}
	for (int32 i = 0; i < Planets.Num(); ++i)
	{
		const bool bOn = (bAll && i != AsgardPillar && i != AlfheimBow) || i == int32(Shown) || (i == HelTrail && bAll);
		Planets[i].Target = bOn ? 1.f : 0.f;
	}
	ChangeWeather();
	// Each realm's air: what hangs, falls or streams around the player. Svartalfheim and Midgard have none.
	static const TCHAR* const Air[NumRealms] = { nullptr, TEXT("NS_RealmCrystals"), TEXT("NS_RealmEmbers"), TEXT("NS_RealmBlizzard"), TEXT("NS_RealmPrism"),
		TEXT("NS_RealmSpores"), nullptr, TEXT("NS_RealmMotes"), TEXT("NS_RealmGold") };
	AirFx.SetNum(NumRealms);
	for (int32 i = 0; i < NumRealms; ++i)
	{
		if (Air[i])
		{
			const FString Path = FString::Printf(TEXT("/Game/Environment/Realms/%s.%s"), Air[i], Air[i]);
			SetParticles(AirFx[i], *Path, Shown == ERealm(i));
		}
	}
}

namespace
{
	// One row per realm: this is where each realm's mood lives. Fields left out stay neutral. Midgard is the real world.
	FRealmLook* LookTable()
	{
		static FRealmLook Looks[] = {
			/* Midgard */ {},
			/* Niflheim: a pale ivory white-out, no shadows, no horizon */
			{ .Active = 1.f, .SunScale = 0.55f, .SkyScale = 1.3f, .FogDensity = 0.09f, .FogFalloff = 0.012f, .FogBelow = 2500.f, .FogCutoff = 0.f, .FogColor = FLinearColor(0.82f, 0.86f, 0.88f),
				.Saturation = 0.55f, .Contrast = 0.85f, .PlanetAlpha = 0.3f, .Temp = 6800.f, .Cold = 0.01f },
			/* Muspelheim: copper light on exposed faces, shadows stay cold, a burnt sky */
			{ .Active = 1.f, .SunScale = 1.1f, .SunColor = FLinearColor(1.f, 0.42f, 0.18f), .SkyScale = 0.6f, .SkyColor = FLinearColor(0.62f, 0.74f, 1.f), .FogDensity = 0.02f, .FogFalloff = 0.08f, .FogColor = FLinearColor(0.45f, 0.16f, 0.07f),
				.Saturation = 1.15f, .Contrast = 1.12f, .Gain = FLinearColor(1.f, 0.9f, 0.82f), .Temp = 5200.f, .Exposure = -0.4f, .Bloom = 1.4f, .Vignette = 0.5f, .Cold = -0.02f },
			/* Jotunheim: a slate storm, ice-blue, almost colourless, heavy */
			{ .Active = 1.f, .SunScale = 0.12f, .SunColor = FLinearColor(0.7f, 0.82f, 1.f), .SkyScale = 0.7f, .SkyColor = FLinearColor(0.7f, 0.82f, 1.f), .FogDensity = 0.018f, .FogFalloff = 0.03f, .FogBelow = 1000.f, .FogColor = FLinearColor(0.09f, 0.12f, 0.16f),
				.Saturation = 0.45f, .Contrast = 1.2f, .Gain = FLinearColor(0.88f, 0.95f, 1.05f), .Temp = 8200.f, .Exposure = -0.8f, .Vignette = 0.7f, .Cold = 0.015f, .StaminaDrain = 0.6f },
			/* Alfheim: soft pearl and lilac, weightless, thin bright mist */
			{ .Active = 1.f, .SunScale = 0.7f, .SunColor = FLinearColor(1.f, 0.88f, 0.95f), .SkyScale = 1.2f, .SkyColor = FLinearColor(0.95f, 0.85f, 1.f), .FogDensity = 0.007f, .FogFalloff = 0.05f, .FogColor = FLinearColor(0.7f, 0.62f, 0.8f),
				.Saturation = 1.0f, .Contrast = 0.95f, .Gain = FLinearColor(1.f, 0.96f, 1.04f), .Bloom = 2.f, .SanityGain = 0.5f },
			/* Vanaheim: a jade dusk for the aurora */
			{ .Active = 1.f, .SunScale = 0.08f, .SunColor = FLinearColor(0.6f, 1.f, 0.8f), .SkyScale = 0.6f, .SkyColor = FLinearColor(0.5f, 1.f, 0.78f), .FogDensity = 0.004f, .FogFalloff = 0.06f, .FogColor = FLinearColor(0.03f, 0.12f, 0.09f),
				.Saturation = 0.85f, .Contrast = 1.1f, .Gain = FLinearColor(0.86f, 1.f, 0.95f), .Exposure = -1.2f, .Vignette = 0.5f, .SanityGain = 0.8f },
			/* Svartalfheim: eclipse, silver on black, hard and airless */
			{ .Active = 1.f, .SunScale = 0.06f, .SunColor = FLinearColor(0.8f, 0.88f, 1.f), .SkyScale = 0.35f, .SkyColor = FLinearColor(0.8f, 0.87f, 1.f), .Saturation = 0.25f, .Contrast = 1.3f, .Temp = 7800.f, .Exposure = -0.6f, .Vignette = 0.9f, .Cold = 0.02f, .SanityGain = -0.3f },
			/* Hel: indigo stillness, drained of warmth */
			{ .Active = 1.f, .SunScale = 0.1f, .SunColor = FLinearColor(0.55f, 0.55f, 1.f), .SkyScale = 0.8f, .SkyColor = FLinearColor(0.6f, 0.55f, 1.f), .FogDensity = 0.04f, .FogFalloff = 0.07f, .FogBelow = 500.f, .FogColor = FLinearColor(0.09f, 0.08f, 0.22f),
				.Saturation = 0.6f, .Contrast = 1.1f, .Gain = FLinearColor(0.8f, 0.78f, 1.1f), .Temp = 9000.f, .Exposure = -1.0f, .Vignette = 0.8f, .Cold = 0.01f, .SanityGain = -0.2f },
			/* Asgard: gold, huge low light, glowing air */
			{ .Active = 1.f, .SunScale = 1.5f, .SunColor = FLinearColor(1.f, 0.72f, 0.36f), .SkyScale = 0.8f, .SkyColor = FLinearColor(1.f, 0.82f, 0.58f), .FogDensity = 0.003f, .FogFalloff = 0.05f, .FogColor = FLinearColor(0.6f, 0.42f, 0.18f),
				.Saturation = 1.15f, .Contrast = 1.12f, .Gain = FLinearColor(1.05f, 0.97f, 0.82f), .Temp = 5000.f, .Bloom = 2.6f, .Cold = -0.01f, .StaminaDrain = -1.f, .SanityGain = 0.4f },
		};
		return Looks;
	}

	const FRealmLook& LookOf(ERealm Realm, bool bAll)
	{
		static const FRealmLook Neutral;
		return bAll || Realm == ERealm::None ? Neutral : LookTable()[int32(Realm)];
	}

	// Ultra Dynamic Weather preset per realm: clouds, wind, fog, particles and weather sound change together
	FString GWeather[NumRealms] = { TEXT("Clear_Skies"), TEXT("Foggy"), TEXT("Sand_Dust_Calm"), TEXT("Snow_Blizzard"), TEXT("Partly_Cloudy"),
		TEXT("Clear_Skies"), TEXT("Clear_Skies"), TEXT("Snow_Light"), TEXT("Cloudy") };

	// The sound when the weather wheel lands on this realm (/Game/Audio/UI/<name>): the more dangerous the weather, the
	// more intense the sound -- harmless: C rune bell / L choir; risky: E bell of the Norns / H bell + horns; deadly:
	// K brass + drum / M bowed drone + drum (user's choice, sounds review 2026-11-11). "Landing" in RealmLooks.json overrides.
	FString GLanding[NumRealms] = { TEXT("UI_Chime_C"), TEXT("UI_Chime_E"), TEXT("UI_Chime_H"), TEXT("UI_Chime_K"), TEXT("UI_Chime_L"),
		TEXT("UI_Chime_L"), TEXT("UI_Chime_M"), TEXT("UI_Chime_E"), TEXT("UI_Chime_L") };

	// Config/RealmLooks.json: { "Hel": { "SunScale": 0.2, "Gain": [1,1,1], "Weather": "Overcast" }, ... } - tune without compiling (ygg.Realm reload)
	void LoadLookOverrides()
	{
		FString Text;
		if (!FFileHelper::LoadFileToString(Text, *(FPaths::ProjectConfigDir() / TEXT("RealmLooks.json")))) { return; }
		TSharedPtr<FJsonObject> Root;
		if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Root) || !Root.IsValid()) { UE_LOG(LogTemp, Warning, TEXT("RealmSky: bad RealmLooks.json")); return; }
		for (int32 i = 0; i < NumRealms; ++i)
		{
			const TSharedPtr<FJsonObject>* Row = nullptr;
			if (!Root->TryGetObjectField(RealmLabels[i], Row)) { continue; }
			FRealmLook& L = LookTable()[i];
			const TPair<const TCHAR*, float*> Floats[] = { {TEXT("Active"), &L.Active}, {TEXT("SunScale"), &L.SunScale}, {TEXT("SkyScale"), &L.SkyScale}, {TEXT("FogDensity"), &L.FogDensity},
				{TEXT("FogFalloff"), &L.FogFalloff}, {TEXT("FogBelow"), &L.FogBelow}, {TEXT("FogCutoff"), &L.FogCutoff}, {TEXT("Saturation"), &L.Saturation}, {TEXT("Contrast"), &L.Contrast},
				{TEXT("PlanetAlpha"), &L.PlanetAlpha}, {TEXT("Temp"), &L.Temp}, {TEXT("Exposure"), &L.Exposure}, {TEXT("Bloom"), &L.Bloom}, {TEXT("Vignette"), &L.Vignette},
				{TEXT("Cold"), &L.Cold}, {TEXT("StaminaDrain"), &L.StaminaDrain}, {TEXT("SanityGain"), &L.SanityGain}, {TEXT("HealthDrain"), &L.HealthDrain}, {TEXT("Wind"), &L.Wind}, {TEXT("CloudCover"), &L.CloudCover}, {TEXT("CloudSpeed"), &L.CloudSpeed}, {TEXT("CloudScale"), &L.CloudScale} };
			for (const auto& F : Floats) { double V; if ((*Row)->TryGetNumberField(F.Key, V)) { *F.Value = float(V); } }
			const TPair<const TCHAR*, FLinearColor*> Colors[] = { {TEXT("SunColor"), &L.SunColor}, {TEXT("SkyColor"), &L.SkyColor}, {TEXT("FogColor"), &L.FogColor}, {TEXT("Gain"), &L.Gain}, {TEXT("CloudDark"), &L.CloudDark}, {TEXT("CloudLight"), &L.CloudLight} };
			for (const auto& C : Colors)
			{
				const TArray<TSharedPtr<FJsonValue>>* A = nullptr;
				if ((*Row)->TryGetArrayField(C.Key, A) && A->Num() == 3) { *C.Value = FLinearColor((*A)[0]->AsNumber(), (*A)[1]->AsNumber(), (*A)[2]->AsNumber()); }
			}
			(*Row)->TryGetStringField(TEXT("Weather"), GWeather[i]);
			(*Row)->TryGetStringField(TEXT("Landing"), GLanding[i]);
			double V;
			if ((*Row)->TryGetNumberField(TEXT("PlanetDegrees"), V)) { GPlanetSky[i].Degrees = float(V); }
			if ((*Row)->TryGetNumberField(TEXT("PlanetYaw"), V)) { GPlanetSky[i].Yaw = float(V); }
			if ((*Row)->TryGetNumberField(TEXT("PlanetPitch"), V)) { GPlanetSky[i].Pitch = float(V); }
		}
		UE_LOG(LogTemp, Display, TEXT("RealmSky: RealmLooks.json loaded"));
	}
}

FRealmLook FRealmLook::Lerp(const FRealmLook& A, const FRealmLook& B, float T)
{
	FRealmLook R;
	const auto F = [T](float X, float Y) { return FMath::Lerp(X, Y, T); };
	R.Active = F(A.Active, B.Active); R.SunScale = F(A.SunScale, B.SunScale); R.SkyScale = F(A.SkyScale, B.SkyScale);
	R.SunColor = FMath::Lerp(A.SunColor, B.SunColor, T); R.SkyColor = FMath::Lerp(A.SkyColor, B.SkyColor, T); R.FogColor = FMath::Lerp(A.FogColor, B.FogColor, T); R.Gain = FMath::Lerp(A.Gain, B.Gain, T);
	R.FogDensity = F(A.FogDensity, B.FogDensity); R.FogFalloff = F(A.FogFalloff, B.FogFalloff); R.FogBelow = F(A.FogBelow, B.FogBelow); R.FogCutoff = F(A.FogCutoff, B.FogCutoff); R.PlanetAlpha = F(A.PlanetAlpha, B.PlanetAlpha);
	R.Saturation = F(A.Saturation, B.Saturation); R.Contrast = F(A.Contrast, B.Contrast); R.Temp = F(A.Temp, B.Temp);
	R.Wind = F(A.Wind, B.Wind); R.CloudCover = F(A.CloudCover, B.CloudCover); R.CloudSpeed = F(A.CloudSpeed, B.CloudSpeed); R.CloudScale = F(A.CloudScale, B.CloudScale);
	R.CloudDark = FMath::Lerp(A.CloudDark, B.CloudDark, T); R.CloudLight = FMath::Lerp(A.CloudLight, B.CloudLight, T);
	 R.Exposure = F(A.Exposure, B.Exposure); R.Bloom = F(A.Bloom, B.Bloom); R.Vignette = F(A.Vignette, B.Vignette);
	return R;
}

FVector ARealmSky::PlanetAnchor(FVector& OutDir, float DeltaSeconds)
{
	// The sun's daily circle, computed from the clock (the sun light itself only updates in steps, which made the planet
	// jump). Measured from the sky at three times of day: axis (0.866, 0, 0.5), 15 degrees an hour, this direction at 08:01.
	static const FVector SunAxis(0.866, 0.0, 0.5);
	static const FVector SunAt0801(-0.2488, 0.8676, 0.4305);
	const float TimeOfDay = YggTime::Get(GetWorld());
	const FVector SunDir = SunAt0801.RotateAngleAxis((TimeOfDay - 801.7f) / 100.f * 15.f, SunAxis).GetSafeNormal();
	if (!MoonLight.IsValid() && SkyCached.IsValid())
	{
		TArray<UDirectionalLightComponent*> Lights; SkyCached->GetComponents(Lights);
		for (UDirectionalLightComponent* L : Lights) { if (L->GetName() == TEXT("Moon")) { MoonLight = L; } }
	}
	const FVector MoonNow = MoonLight.IsValid() ? -MoonLight->GetForwardVector() : -SunDir;
	SmoothMoon = SmoothMoon.IsNearlyZero() ? MoonNow : (SmoothMoon + (MoonNow - SmoothMoon) * FMath::Min(1.f, DeltaSeconds * 1.5f)).GetSafeNormal();
	const float SunEl = FMath::RadiansToDegrees(FMath::Asin(FMath::Clamp(SunDir.Z, -1.0, 1.0)));
	const float MoonEl = FMath::RadiansToDegrees(FMath::Asin(FMath::Clamp(SmoothMoon.Z, -1.0, 1.0)));
	// Hand over in twilight, when the planet has faded out on both: it sets with the sun and rises with the moon
	if (SunEl >= -5.f)
	{
		OutDir = SunDir;
		return FVector(FMath::SmoothStep(-5.f, 0.f, SunEl), 1.f, 0.f);
	}
	OutDir = SmoothMoon;
	return FVector((1.f - FMath::SmoothStep(-10.f, -5.f, SunEl)) * FMath::SmoothStep(-3.f, 2.f, MoonEl), 0.45f, 0.f);
}

void ARealmSky::ApplyLook(float DeltaSeconds, const FVector& Around)
{
	Look = FRealmLook::Lerp(Look, LookOf(Shown, bAll), 1.f - FMath::Exp(-DeltaSeconds / TransitionTau));
	if (Look.Active < 0.002f && LookOf(Shown, bAll).Active <= 0.f) { Look = FRealmLook(); }

	if (!Sun.IsValid() || !Fog.IsValid() || !SkyLight.IsValid())
	{
		if (AActor* Sky = FindSky(GetWorld()))
		{
			TArray<UDirectionalLightComponent*> Suns; Sky->GetComponents(Suns);
			for (UDirectionalLightComponent* L : Suns) { if (L->GetName() == TEXT("Sun")) { Sun = L; } }
			TArray<USkyLightComponent*> Lights; Sky->GetComponents(Lights);
			for (USkyLightComponent* L : Lights) { if (L->GetName() == TEXT("Captured Scene Sky Light")) { SkyLight = L; } }
			Fog = Sky->FindComponentByClass<UExponentialHeightFogComponent>();
		}
	}
	UDirectionalLightComponent* S = Sun.Get(); USkyLightComponent* L = SkyLight.Get(); UExponentialHeightFogComponent* F = Fog.Get();
	// Above 7,000 m the air is clear and the whole world can be seen: height fog fades away by 8,000 m, unless a realm brings its own.
	if (F)
	{
		const float ElevationM = 6326.3f + float(Around.Z - 129770.0) / 100.f;
		const float High = FMath::SmoothStep(7000.f, 8000.f, ElevationM);
		const float RealmFog = FMath::Clamp(LookOf(Shown, bAll).FogDensity / 0.02f, 0.f, 1.f) * Look.Active;
		// Low down too the fog stays a thin veil: full height fog buried the middle distance in a dark band while the far strip stayed bright.
		const float Opacity = FMath::Lerp(0.3f * (1.f - High), 1.f, RealmFog);
		if (!FMath::IsNearlyEqual(F->FogMaxOpacity, Opacity, 0.002f)) { F->SetFogMaxOpacity(Opacity); }
	}
	// The realm's wind: 0 is dead still, 1 a full gale; spindrift, the snow streaks and the flags all read it. Below 0 = the world's own.
	if (static IConsoleVariable* WindVar = IConsoleManager::Get().FindConsoleVariable(TEXT("ygg.Snow.Spindrift")))
	{
		WindVar->Set(Look.Active > 0.01f ? FMath::Lerp(-1.f, Look.Wind, FMath::Min(Look.Active * 4.f, 1.f)) : -1.f, ECVF_SetByCode);
	}
	const bool bFlash = FlashLeft > 0.f && FMath::Fmod(FlashLeft, 0.12f) > 0.04f;   // a short flicker, not one pulse
	const float A = Look.Active;
	// The cloud deck: the sky plugin's own clouds do not answer in this setup, so each realm's sky moves here
	if (!Deck)
	{
		if (UMaterialInterface* Mat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Environment/Realms/M_RealmClouds.M_RealmClouds")))
		{
			Deck = NewObject<UStaticMeshComponent>(this);
			Deck->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Plane.Plane")));
			Deck->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			Deck->SetCastShadow(false);
			Deck->SetMobility(EComponentMobility::Movable);
			Deck->SetAbsolute(true, true, true);
			Deck->SetWorldScale3D(FVector(400000.0, 400000.0, 1.0));   // 400 km across, the far planes must reach
			Deck->BoundsScale = 10.f;
			Deck->RegisterComponent();
			DeckMat = Deck->CreateAndSetMaterialInstanceDynamicFromMaterial(0, Mat);
		}
	}
	// How far up the sun is. The sky's own clock moves it; every value below that is scaled from a daytime base follows it.
	float DayK = 1.f, LowSun = 0.f;
	if (S)
	{
		const float Elevation = FMath::RadiansToDegrees(FMath::Asin(FMath::Clamp(-S->GetForwardVector().Z, -1.f, 1.f)));
		DayK = FMath::SmoothStep(-3.f, 6.f, Elevation);
		LowSun = 1.f - FMath::SmoothStep(3.f, 25.f, Elevation);
	}
	const FLinearColor NightDeck = FLinearColor(1.f, 1.f, 1.f) * FMath::Lerp(0.08f, 1.f, DayK);
	if (Deck && DeckMat)
	{
		Deck->SetVisibility(Look.CloudCover > 0.01f);
		Deck->SetWorldLocation(FVector(Around.X, Around.Y, Around.Z + 250000.0));   // 2.5 km over the head, always
		DeckMat->SetScalarParameterValue(TEXT("Cover"), Look.CloudCover);
		DeckMat->SetScalarParameterValue(TEXT("Speed"), Look.CloudSpeed);
		DeckMat->SetScalarParameterValue(TEXT("Scale"), Look.CloudScale);
		DeckMat->SetVectorParameterValue(TEXT("Dark"), bFlash ? FLinearColor(0.75f, 0.9f, 1.2f) : Look.CloudDark * NightDeck);   // lightning lights the deck from inside, same frames as the ground
		DeckMat->SetVectorParameterValue(TEXT("Light"), bFlash ? FLinearColor(1.5f, 1.8f, 2.2f) : Look.CloudLight * NightDeck);
	}
	// UDS only rewrites its lights when they change enough, and this ticks after it: remember its values while no realm
	// runs and set ours from those - never compound a scale on top of an earlier one
	if (!bBasesKnown || (A <= 0.f && !bFlash && !bLookOn))
	{
		// only a daytime reading is a base: a save loaded at night must not leave every later day dark
		if (S && DayK >= 1.f) { SunBase = S->Intensity; SunColor = S->GetLightColor(); }
		if (L && DayK >= 1.f) { SkyBase = L->Intensity; }
		if (F) { FogBase = F->FogDensity; FalloffBase = F->FogHeightFalloff; FogColorBase = F->FogInscatteringLuminance; FogLocation = F->GetComponentLocation(); CutoffBase = F->FogCutoffDistance; }
		bBasesKnown = S && L && F;
	}
	if (A <= 0.f && !bFlash)
	{
		if (bLookOn)
		{
			if (S) { S->SetIntensity(SunBase); S->SetLightColor(SunColor); }
			if (L) { L->SetIntensity(SkyBase); L->SetLightColor(FLinearColor::White); }
			if (F) { F->SetFogDensity(FogBase); F->SetFogHeightFalloff(FalloffBase); F->SetFogInscatteringColor(FogColorBase); F->SetWorldLocation(FogLocation); F->SetFogCutoffDistance(CutoffBase); }
			bLookOn = false;
		}
		Grade->BlendWeight = 0.f;
		return;
	}
	bLookOn = true;
	if (S)
	{
		S->SetIntensity(SunBase * Look.SunScale * DayK);
		S->SetLightColor(FMath::Lerp(SunColor, Look.SunColor, A) * FMath::Lerp(FLinearColor::White, FLinearColor(1.f, 0.66f, 0.42f), LowSun * 0.7f));
	}
	if (L)
	{
		L->SetLightColor(bFlash ? FLinearColor(0.6f, 0.8f, 1.f) : Look.SkyColor);   // the colour of the shadows
		L->SetIntensity(SkyBase * Look.SkyScale * FMath::Lerp(0.2f, 1.f, DayK) * (bFlash ? 6.f : 1.f));
	}
	if (F)
	{
		F->SetFogDensity(FMath::Lerp(FogBase, Look.FogDensity, A));
		F->SetFogHeightFalloff(FMath::Lerp(FalloffBase, Look.FogFalloff, A));
		F->SetFogCutoffDistance(Look.FogCutoff < 1.f ? 0.f : CutoffBase);   // 0: the fog takes the sky too, a real white-out
		F->SetFogInscatteringColor(FMath::Lerp(FogColorBase, Look.FogColor, A) * FMath::Lerp(0.03f, 1.f, DayK));
		// height fog is measured from its own height: keep the layer around the player, wherever on the mountain
		F->SetWorldLocation(FVector(FogLocation.X, FogLocation.Y, FMath::Lerp(FogLocation.Z, Around.Z - Look.FogBelow, A)));
	}
	// What the realm does to the body and mind, once it has really arrived; a shelter keeps it out
	const FRealmLook& Row = LookOf(Shown, bAll);
	const APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
	USurvivalComponent* Body = Player ? Player->FindComponentByClass<USurvivalComponent>() : nullptr;
	// A presentation must not kill the presenter
	const bool bFelt = A > 0.5f && DemoIndex < 0;
	if (Body) { Body->SetWeatherCold(bFelt ? Row.Cold : 0.f); }
	if (Body && DemoIndex >= 0 && !Body->IsDead()) { Body->AddToStat(ESurvivalStat::Health, 100.f); }
	if (Body && bFelt && !Body->IsSheltered() && !Body->IsDead())
	{
		Body->AddToStat(ESurvivalStat::Stamina, -Row.StaminaDrain * DeltaSeconds);
		Body->AddToStat(ESurvivalStat::Health, -Row.HealthDrain * DeltaSeconds);
		if (USanityComponent* Mind = Player->FindComponentByClass<USanityComponent>())
		{
			Mind->SetSanity(Mind->GetSanity() + Row.SanityGain * DeltaSeconds);
		}
	}
	FPostProcessSettings& P = Grade->Settings;
	P.bOverride_ColorSaturation = P.bOverride_ColorContrast = P.bOverride_ColorGain = P.bOverride_WhiteTemp = P.bOverride_AutoExposureBias = P.bOverride_BloomIntensity = P.bOverride_VignetteIntensity = true;
	P.bOverride_DepthOfFieldFocalDistance = true; P.DepthOfFieldFocalDistance = 0.f;   // near snow must stay readable
	P.ColorSaturation = FVector4(Look.Saturation, Look.Saturation, Look.Saturation, 1.f);
	P.ColorContrast = FVector4(Look.Contrast, Look.Contrast, Look.Contrast, 1.f);
	P.ColorGain = FVector4(Look.Gain.R, Look.Gain.G, Look.Gain.B, 1.f);
	P.WhiteTemp = Look.Temp;
	P.AutoExposureBias = Look.Exposure;
	P.BloomIntensity = Look.Bloom;
	P.VignetteIntensity = Look.Vignette;
	Grade->BlendWeight = A;
}

namespace
{
	// What the player reads: never the realm's own name. Danger: 0 harmless, 1 risky, 2 deadly without shelter.
	const FRealmWeatherInfo GInfo[NumRealms] = {
		{ TEXT("QUIET SKY"), TEXT("Clear skies ahead"), 0 },
		{ TEXT("VEIL OF MIST"), TEXT("Whiteout approaching"), 1 },
		{ TEXT("EMBERFALL"), TEXT("Burning ash approaching"), 1 },
		{ TEXT("WINTER\u2019S VEIL"), TEXT("Snowstorm approaching"), 2 },
		{ TEXT("PEARL LIGHT"), TEXT("Soft light ahead"), 0 },
		{ TEXT("JADE TIDE"), TEXT("Aurora rising"), 0 },
		{ TEXT("SILVER ECLIPSE"), TEXT("Killing cold approaching"), 2 },
		{ TEXT("STARFALL FROST"), TEXT("The mind grows cold"), 1 },
		{ TEXT("GOLDEN HUSH"), TEXT("Warm light ahead"), 0 },
	};
}

const FRealmWeatherInfo& ARealmSky::GetWeatherInfo(int32 Index) { return GInfo[FMath::Clamp(Index, 0, NumRealms - 1)]; }
int32 ARealmSky::NumWeathers() { return NumRealms; }
const FString& ARealmSky::GetLandingSound(int32 Index) { return GLanding[FMath::Clamp(Index, 0, NumRealms - 1)]; }

ERealm ARealmSky::DrawFromBag()
{
	if (Bag.Num() == 0)
	{
		for (int32 i = 0; i < NumRealms; ++i) { Bag.Add(i); }
		for (int32 i = Bag.Num() - 1; i > 0; --i) { Bag.Swap(i, FMath::RandRange(0, i)); }
		if (Bag.Last() == int32(Shown) && Bag.Num() > 1) { Bag.Swap(0, Bag.Num() - 1); }   // never the same one twice running
	}
	return ERealm(Bag.Pop());
}

void ARealmSky::SetScheduled(bool bOn)
{
	bScheduled = bOn;
	bForecast = false;
	if (bOn) { WeatherLeft = FMath::Min(WeatherLeft, 30.f); }
}

void ARealmSky::ForecastNow()
{
	bScheduled = true;
	bForecast = false;
	WeatherLeft = 0.f;
}

void ARealmSky::TickWeather(float DeltaSeconds, const FVector& Around)
{
	if (DemoIndex >= 0) { return; }   // the demo decides, wherever it is shown from
	// All nine together belong to the summit alone: nothing else happens up there
	const bool bSummit = FVector::DistXY(Around, PlacedAround) < 60000.0;
	if (bSummit != bAtSummit)
	{
		bAtSummit = bSummit;
		bForecast = false;
		if (bSummit) { ShowAll(); } else { ShowRealm(ERealm::None); WeatherLeft = FMath::FRandRange(360.f, 600.f); }
	}
	if (!bScheduled || bAtSummit) { return; }
	WeatherLeft -= DeltaSeconds;
	// A realm passes like a storm, then ordinary weather returns: normal sun, no planet.
	if (Shown != ERealm::None)
	{
		if (WeatherLeft <= 0.f)
		{
			GOrdinaryWeather = FMath::RandBool() ? TEXT("Clear_Skies") : TEXT("Snow_Light");
			ShowRealm(ERealm::None);
			bForecast = false;
			WeatherLeft = FMath::FRandRange(360.f, 600.f);
			UE_LOG(LogTemp, Display, TEXT("RealmSky: ordinary weather %s for %.0f s"), *GOrdinaryWeather, WeatherLeft);
		}
		return;
	}
	if (!bForecast)
	{
		NextRealm = DrawFromBag();
		bForecast = true;
		// dangerous weather gives time to reach shelter; harmless weather arrives with less notice
		ForecastLeft = GInfo[int32(NextRealm)].Danger > 0 ? 150.f : 45.f;
		WeatherLeft = FMath::Max(WeatherLeft, ForecastLeft) ;
		if (AnnounceIn >= 0.f) { WeatherLeft = AnnounceIn + ForecastLeft; AnnounceIn = -1.f; }
		ForecastSerial += (WeatherLeft <= ForecastLeft + 0.5f) ? 1 : 0;
		bAnnounced = WeatherLeft <= ForecastLeft + 0.5f;
	}
	if (!bAnnounced && WeatherLeft <= ForecastLeft)
	{
		bAnnounced = true;
		++ForecastSerial;   // the wheel spins once for this
	}
	if (WeatherLeft <= 0.f)
	{
		ShowRealm(NextRealm);
		bForecast = false;
		WeatherLeft = FMath::FRandRange(600.f, 720.f);
		UE_LOG(LogTemp, Display, TEXT("RealmSky: weather %s for %.0f s"), GInfo[int32(Shown)].Name, WeatherLeft);
	}
}

float ARealmSky::GetSkyHoursSinceShown() const
{
	return SkyHours;
}

// -YggTour: one launch visits every realm and logs its frame times, then quits (no realm first, as the baseline)
void ARealmSky::TickTour(float DeltaSeconds)
{
	if (PerfSteps.Num())
	{
		// -YggPerfSteps: runs Saved/perf_steps.txt, one console command per line, logging the frame cost after each
		PerfTime += DeltaSeconds;
		if (PerfTime > 10.f) { ++PerfFrames; PerfFrame += DeltaSeconds * 1000.f; PerfGpu += FPlatformTime::ToMilliseconds(RHIGetGPUFrameCycles()); PerfRender += FPlatformTime::ToMilliseconds(GRenderThreadTime); }
		if (PerfTime > 20.f)
		{
			const int32 N = FMath::Max(PerfFrames, 1);
			UE_LOG(LogTemp, Display, TEXT("STEP [%s] fps %.0f frame %.1f render %.1f gpu %.1f"), *PerfSteps[0], N / (PerfFrame / 1000.f), PerfFrame / N, PerfRender / N, PerfGpu / N);
			PerfSteps.RemoveAt(0);
			PerfTime = 0.f; PerfFrames = 0; PerfFrame = PerfGpu = PerfRender = 0.f;
			if (PerfSteps.Num() == 0) { FPlatformMisc::RequestExit(false); return; }
			if (APlayerController* PC = GetWorld()->GetFirstPlayerController()) { PC->ConsoleCommand(PerfSteps[0]); }
		}
		return;
	}
	if (TourIndex < 0) { return; }
	constexpr float Settle = 14.f, Measure = 15.f;
	TourTime += DeltaSeconds;
	if (TourTime > Settle)
	{
		++TourFrames; TourFrame += DeltaSeconds * 1000.f;
		TourRender += FPlatformTime::ToMilliseconds(GRenderThreadTime); TourGame += FPlatformTime::ToMilliseconds(GGameThreadTime); TourGpu += FPlatformTime::ToMilliseconds(RHIGetGPUFrameCycles());
	}
	if (TourTime < Settle + Measure) { return; }
	const int32 F = FMath::Max(TourFrames, 1);
	UE_LOG(LogTemp, Display, TEXT("TOUR %s fps %.0f frame %.1f game %.1f render %.1f gpu %.1f"), TourIndex == 0 ? TEXT("none") : RealmLabels[TourIndex - 1], F / (TourFrame / 1000.f), TourFrame / F, TourGame / F, TourRender / F, TourGpu / F);
	TourTime = 0.f; TourFrames = 0; TourFrame = TourGame = TourRender = TourGpu = 0.f;
	if (++TourIndex > NumRealms) { FPlatformMisc::RequestExit(false); return; }
	ShowRealm(ERealm(TourIndex - 1));
}

// ygg.Realm.Demo <seconds>: the nine realms in order. Each turn ends the way real weather comes: the wheel spins, lands on
// the next realm, counts down, and that realm arrives. The fades are shortened so each one arrives within its turn.
void ARealmSky::TickDemo(float DeltaSeconds)
{
	const float Seconds = CVarRealmDemo.GetValueOnGameThread();
	if (Seconds <= 0.f)
	{
		if (DemoIndex >= 0)
		{
			DemoIndex = -1;
			bDemoAnnounced = false;
			TransitionTau = SavedTau;
			FadeSeconds = SavedFade;
			SetScheduled(true);
		}
		return;
	}
	const float Lead = FMath::Min(4.2f, Seconds * 0.6f);   // the wheel's own length
	if (DemoIndex < 0)
	{
		SavedTau = TransitionTau;
		SavedFade = FadeSeconds;
		TransitionTau = FMath::Min(TransitionTau, Seconds / 6.f);
		FadeSeconds = FMath::Min(FadeSeconds, Seconds / 3.f);
		SetScheduled(false);
		bAtSummit = false;
		DemoIndex = NumRealms - 1;   // the next is the first
		DemoLeft = Lead;
		bDemoAnnounced = false;
	}
	DemoLeft -= DeltaSeconds;
	if (!bDemoAnnounced && DemoLeft <= Lead)
	{
		bDemoAnnounced = true;
		NextRealm = ERealm((DemoIndex + 1) % NumRealms);
		PlayWheel(NextRealm);
	}
	if (DemoLeft > 0.f) { return; }
	DemoLeft = Seconds;
	bDemoAnnounced = false;
	DemoIndex = (DemoIndex + 1) % NumRealms;
	ShowRealm(ERealm(DemoIndex));
	UE_LOG(LogTemp, Display, TEXT("RealmSky: demo %d/%d %s"), DemoIndex + 1, NumRealms, RealmLabels[DemoIndex]);
	LogStatus();
}

// ygg.Realm status: what the plugins really hold now, for checking by log
void ARealmSky::LogStatus() const
{
	const auto Read = [](const UObject* O, const TCHAR* Name) -> double
	{
		const FProperty* P = O ? O->GetClass()->FindPropertyByName(Name) : nullptr;
		if (const FDoubleProperty* D = CastField<FDoubleProperty>(P)) { return D->GetPropertyValue_InContainer(O); }
		if (const FFloatProperty* F = CastField<FFloatProperty>(P)) { return F->GetPropertyValue_InContainer(O); }
		if (const FBoolProperty* B = CastField<FBoolProperty>(P)) { return B->GetPropertyValue_InContainer(O) ? 1.0 : 0.0; }
		return -999.0;
	};
	const AActor* Sky = SkyCached.Get();
	UE_LOG(LogTemp, Display, TEXT("RealmSky status: shown %d tod %.1f animate %.0f skyhours %.2f sun %.2f sky %.2f fog %.4f"), int32(Shown), Read(Sky, TEXT("Time of Day")), Read(Sky, TEXT("Animate Time of Day")), SkyHours,
		Sun.IsValid() ? Sun->Intensity : -1.f, SkyLight.IsValid() ? SkyLight->Intensity : -1.f, Fog.IsValid() ? Fog->FogDensity : -1.f);
	if (Sky)
	{
		TArray<UActorComponent*> Comps;
		Sky->GetComponents(Comps);
		FString Names;
		for (const UActorComponent* C : Comps) { if (C->GetName().Contains(TEXT("Moon")) || C->GetName().Contains(TEXT("Sun"))) { Names += C->GetName() + TEXT("(") + C->GetClass()->GetName() + TEXT(") "); } }
		UE_LOG(LogTemp, Display, TEXT("RealmSky status: bodies %s"), *Names);
	}
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		if (!It->GetClass()->GetName().Contains(TEXT("Ultra_Dynamic_Weather"))) { continue; }
		const FObjectPropertyBase* P = CastField<FObjectPropertyBase>(It->GetClass()->FindPropertyByName(TEXT("Local Weather State")));
		const UObject* State = P ? P->GetObjectPropertyValue_InContainer(*It) : nullptr;
		UE_LOG(LogTemp, Display, TEXT("RealmSky status: local cloud %.2f snow %.2f fog %.2f wind %.2f (state %d)"), Read(State, TEXT("Cloud Coverage")), Read(State, TEXT("Snow")), Read(State, TEXT("Fog")), Read(State, TEXT("Wind Intensity")), State != nullptr);
		break;
	}
}

void ARealmSky::ReloadLooks()
{
	LoadLookOverrides();
	LastWeather.Reset();
	SetTargets();
}

void ARealmSky::ChangeWeather()
{
	AActor* Udw = nullptr;
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		if (It->GetClass()->GetName().Contains(TEXT("Ultra_Dynamic_Weather"))) { Udw = *It; break; }
	}
	if (!Udw) { return; }
	const FString Name = bAll ? FString(TEXT("Partly_Cloudy")) : Shown != ERealm::None ? GWeather[int32(Shown)] : GOrdinaryWeather;
	if (Name == LastWeather) { return; }
	LastWeather = Name;
	const FString Call = FString::Printf(TEXT("\"Change Weather\" /Game/UltraDynamicSky/Blueprints/Weather_Effects/Weather_Presets/%s.%s %f"), *Name, *Name, TransitionTau * 3.f);
	FOutputDeviceNull Out;
	const bool bOk = Udw->CallFunctionByNameWithArguments(*Call, Out, nullptr, true);
	UE_LOG(LogTemp, Display, TEXT("RealmSky: weather -> %s (%d)"), *Name, bOk);
}

void ARealmSky::UpdateRouteRealm(const FVector& Where)
{
	// prototype order (not a design decision): RouteLandmark_N -> realm, held until the next landmark; the last one is the summit
	static const ERealm ByLandmark[] = { ERealm::Midgard, ERealm::Niflheim, ERealm::Vanaheim, ERealm::Muspelheim, ERealm::Alfheim,
		ERealm::Asgard, ERealm::Jotunheim, ERealm::Svartalfheim };
	constexpr double Radius = 30000.0;
	// Hel is not a place on the route: it comes when the mind goes, where the parents' ghosts appear, and leaves when it recovers
	if (const APawn* Pawn = UGameplayStatics::GetPlayerPawn(this, 0))
	{
		if (const USanityComponent* Mind = Pawn->FindComponentByClass<USanityComponent>())
		{
			const float Sanity = Mind->GetSanity();
			if (!bInHel && Sanity < 30.f) { bInHel = true; ShowRealm(ERealm::Hel); }
			else if (bInHel && Sanity > 40.f) { bInHel = false; RouteIndex = -1; }
		}
	}
	if (bInHel) { return; }
	for (TActorIterator<ARouteLandmark> It(GetWorld()); It; ++It)
	{
		FString Left, Right;
		if (FVector::DistXY(It->GetActorLocation(), Where) > Radius || !It->GetActorNameOrLabel().Split(TEXT("_"), &Left, &Right, ESearchCase::IgnoreCase, ESearchDir::FromEnd)) { continue; }
		const int32 Index = FCString::Atoi(*Right);
		if (Index == RouteIndex) { return; }
		RouteIndex = Index;
		UE_LOG(LogTemp, Display, TEXT("RealmSky: route landmark %d"), Index);
		if (Index >= UE_ARRAY_COUNT(ByLandmark)) { ShowAll(); } else { ShowRealm(ByLandmark[Index]); }
		return;
	}
}

void ARealmSky::SetParticles(TObjectPtr<UNiagaraComponent>& Comp, const TCHAR* Path, bool bOn)
{
	if (Comp)
	{
		if (bOn) { Comp->Activate(true); } else { Comp->Deactivate(); }   // Deactivate lets the falling ones finish
		return;
	}
	APawn* Pawn = UGameplayStatics::GetPlayerPawn(this, 0);
	UNiagaraSystem* System = bOn && Pawn ? LoadObject<UNiagaraSystem>(nullptr, Path) : nullptr;
	if (System)
	{
		Comp = UNiagaraFunctionLibrary::SpawnSystemAttached(System, Pawn->GetRootComponent(), NAME_None, FVector::ZeroVector, FRotator::ZeroRotator, EAttachLocation::KeepRelativeOffset, false);
		if (Comp) { Comp->SetUsingAbsoluteRotation(true); }
	}
}

void ARealmSky::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	// ygg.Realm.TimeLapse: the sky's lights normally update only in steps (cheap); while watching a fast day they must follow every frame.
	const float Lapse = CVarTimeLapse.GetValueOnGameThread();
	if (AActor* LapseSky = (Lapse > 0.f || bTimeLapseOn) ? FindSky(GetWorld()) : nullptr)
	{
		if (Lapse > 0.f)
		{
			YggTime::Set(GetWorld(), YggTime::Get(GetWorld()) + 2400.f * DeltaSeconds / Lapse);
		}
		if ((Lapse > 0.f) != bTimeLapseOn)
		{
			bTimeLapseOn = Lapse > 0.f;
			SetSkyFloat(LapseSky, TEXT("Lights Update Period"), bTimeLapseOn ? 0.0 : 4.0);
			SetSkyFloat(LapseSky, TEXT("Lights Update Degree Threshold"), bTimeLapseOn ? 0.0 : 0.4);
		}
	}
	// Jotunheim's Icevein storm: an ice-blue flash in the storm bank, thunder 3-5 s later from that direction
	if (Shown == ERealm::Jotunheim && Planets[int32(ERealm::Jotunheim)].Fade > 0.5f)
	{
		NextFlash -= DeltaSeconds;
		if (NextFlash <= 0.f)
		{
			NextFlash = FMath::FRandRange(14.f, 26.f);
			FlashLeft = 0.35f;
			ThunderIn = FMath::FRandRange(3.f, 5.f);
			UE_LOG(LogTemp, Display, TEXT("RealmSky: Jotunheim flash, thunder in %.1f s"), ThunderIn);
		}
	}
	FlashLeft = FMath::Max(0.f, FlashLeft - DeltaSeconds);
	if (ThunderIn > 0.f)
	{
		ThunderIn -= DeltaSeconds;
		if (ThunderIn <= 0.f)
		{
			if (Thunders.Num() == 0)
			{
				// real thunder recordings, each starting at its strike (Tools/Audio/build_game_sounds.py); else the old boom
				for (int32 i = 1; i <= 8; ++i)
				{
					const FString N = FString::Printf(TEXT("Thunder_%02d"), i);
					if (USoundBase* S = LoadObject<USoundBase>(nullptr, *(TEXT("/Game/Audio/Weather/") + N + TEXT(".") + N), nullptr, LOAD_NoWarn | LOAD_Quiet)) { Thunders.Add(S); }
				}
				if (Thunders.Num() == 0)
				{
					if (USoundBase* S = LoadObject<USoundBase>(nullptr, TEXT("/Game/Audio/Ice/Mountain_Boom_02.Mountain_Boom_02"))) { Thunders.Add(S); }
				}
			}
			const APawn* P = UGameplayStatics::GetPlayerPawn(this, 0);
			if (Thunders.Num() > 0 && P && Planets[int32(ERealm::Jotunheim)].Actor.IsValid())
			{
				int32 Pick = FMath::RandRange(0, Thunders.Num() - 1);
				if (Thunders.Num() > 1 && Pick == LastThunder) { Pick = (Pick + 1) % Thunders.Num(); }
				LastThunder = Pick;
				const FVector Dir = (Planets[int32(ERealm::Jotunheim)].Actor->GetActorLocation() - P->GetActorLocation()).GetSafeNormal();
				UE_LOG(LogTemp, Display, TEXT("RealmSky: thunder (%s)"), *Thunders[Pick]->GetName());
				UGameplayStatics::PlaySoundAtLocation(this, Thunders[Pick], P->GetActorLocation() + Dir * 3000.f, 1.0f, FMath::FRandRange(0.9f, 1.03f));
			}
		}
	}
	// keep the sky centred on the player: 5000 km away, a planet would otherwise drift as you walk 30 km
	const APawn* Pawn = UGameplayStatics::GetPlayerPawn(this, 0);
	const FVector Around = Pawn ? Pawn->GetActorLocation() : PlacedAround;
	TickTour(DeltaSeconds);
	TickDemo(DeltaSeconds);
	if (const int32 LookAt = CVarRealmLook.GetValueOnGameThread())
	{
		if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
		{
			UDirectionalLightComponent* S = Sun.Get();
			TArray<UDirectionalLightComponent*> Lights;
			if (SkyCached.IsValid()) { SkyCached->GetComponents(Lights); }
			for (UDirectionalLightComponent* L : Lights) { if (LookAt == 2 && L->GetName() == TEXT("Moon")) { S = L; } }
			const AActor* Planet = Shown != ERealm::None ? Planets[int32(Shown)].Actor.Get() : nullptr;
			if (LookAt == 3 && Planet) { PC->SetControlRotation((Planet->GetActorLocation() - Around).Rotation()); }
			else if (S) { PC->SetControlRotation((-S->GetForwardVector()).Rotation()); }
		}
	}
	TickWeather(DeltaSeconds, Around);
	ApplyLook(DeltaSeconds, Around);
	// The planets cross the sky with the sky's own clock, a little slower than the sun
	if (!SkyCached.IsValid()) { SkyCached = FindSky(GetWorld()); }
	if (!bStartTimeSet)
	{
		bStartTimeSet = true;
		if (CVarRealmStart.GetValueOnGameThread() >= 0.f) { YggTime::Set(GetWorld(), CVarRealmStart.GetValueOnGameThread()); }
	}
	if (AActor* SkyActor = SkyCached.Get())
	{
		if (FDoubleProperty* Tod = CastField<FDoubleProperty>(SkyActor->GetClass()->FindPropertyByName(TEXT("Time of Day"))))
		{
			const double Now = Tod->GetPropertyValue_InContainer(SkyActor);
			if (LastTod >= 0.0)
			{
				double Step = Now - LastTod;
				if (Step < -1200.0) { Step += 2400.0; }
				if (FMath::Abs(Step) < 300.0) { SkyHours += float(Step / 100.0); }
			}
			LastTod = Now;
		}
	}
	if (bFollowRoute) { UpdateRouteRealm(Around); }
	// Alfheim's prism bow: only while Alfheim is up, only from the one spot on the route
	if (const AActor* Bow = Planets[AlfheimBow].Actor.Get())
	{
		const FVector Spot = Bow->GetActorLocation() + Bow->GetActorUpVector() * BowDistance;
		Planets[AlfheimBow].Target = (Shown == ERealm::Alfheim && FVector::DistXY(Around, Spot) < BowSpotRadius) ? 1.f : 0.f;
	}
	FVector AnchorDir;
	const FVector Anchor = PlanetAnchor(AnchorDir, DeltaSeconds);
	for (FPlanet& P : Planets)
	{
		AActor* A = P.Actor.Get();
		if (!A)
		{
			continue;
		}
		P.Fade = FMath::FInterpConstantTo(P.Fade, P.Target, DeltaSeconds, 1.f / FadeSeconds);
		const bool bLone = !bAll && int32(&P - Planets.GetData()) < NumRealms;
		// a lone planet sets with the sun and rises with the moon; by night it gives a dimmer light
		const float Eased = P.Fade * P.Fade * (3.f - 2.f * P.Fade) * (bLone ? Anchor.X * Anchor.Y : 1.f);
		for (const TWeakObjectPtr<UMaterialInstanceDynamic>& D : P.Materials)
		{
			if (D.IsValid())
			{
				D->SetScalarParameterValue(TEXT("Bright"), P.BaseBright * Eased);
				D->SetScalarParameterValue(TEXT("Fade"), Eased * Look.PlanetAlpha);      // opacity too: a dim planet must not block the sky
			}
		}
		UStaticMeshComponent* Mesh = Cast<AStaticMeshActor>(A)->GetStaticMeshComponent();
		Mesh->SetVisibility(P.Fade > 0.001f);
		if (Ring && &P == &Planets[int32(ERealm::Asgard)]) { Ring->SetVisibility(P.Fade > 0.001f); }
		if (P.Fade > 0.001f && &P != &Planets[AsgardPillar] && &P != &Planets[AlfheimBow])
		{
			// Alone in the sky a planet has its own apparent size and place (seen from the ground, sizes must differ hugely);
			// at the summit all nine keep the arrangement they were placed in.
			const int32 Index = int32(&P - Planets.GetData());
			if (!bAll && Index < NumRealms)
			{
				const FPlanetSky& Sky = PlanetSky(Index);
				const double Dist = P.Offset.Size();
				// The planet takes the sun's place by day and the moon's by night, so the light and the shadows come from it
				// (it used to trail the sun by 40 degrees: below the horizon all morning, and its night side faced the player)
				const FVector Dir = AnchorDir;
				A->SetActorLocation(Around + Dir * Dist);
				A->SetActorScale3D(FVector(Dist * FMath::Tan(FMath::DegreesToRadians(Sky.Degrees * 0.5f)) / 100.0));
				if (Index == int32(ERealm::Asgard) && Ring)
				{
					// Seen from below at a slant, running diagonally across the sky; the rings are not part of the disk's size target
					const FVector Side = FRotationMatrix(FRotator(Sky.Pitch, Sky.Yaw, RingRoll)).GetUnitAxis(EAxis::Z);
					const FVector Normal = (-Dir * FMath::Cos(FMath::DegreesToRadians(RingTilt)) + Side * FMath::Sin(FMath::DegreesToRadians(RingTilt))).GetSafeNormal();
					Ring->SetWorldRotation(FRotationMatrix::MakeFromZ(Normal).Rotator());
					RingMat->SetScalarParameterValue(TEXT("PlanetRadius"), float(A->GetActorScale3D().X * 100.0));
				}
			}
			else
			{
				if (Index < NumRealms) { A->SetActorScale3D(P.BaseScale); }
				A->SetActorLocation(Around + P.Offset);
			}
		}
	}
}
