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
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "UObject/UnrealType.h"

namespace
{
	const TCHAR* RealmLabels[] = { TEXT("Midgard"), TEXT("Niflheim"), TEXT("Muspelheim"), TEXT("Jotunheim"), TEXT("Alfheim"),
		TEXT("Vanaheim"), TEXT("Svartalfheim"), TEXT("Hel"), TEXT("Asgard"), TEXT("HelTrail"), TEXT("AsgardPillar"), TEXT("AlfheimBow") };
	constexpr int32 NumRealms = 9;
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
		if (Args[0].Equals(TEXT("next"), ESearchCase::IgnoreCase)) { Sky->ForecastNow(); return; }
		Sky->SetScheduled(false);   // a realm named by hand stays until auto
		Sky->bFollowRoute = Args[0].Equals(TEXT("route"), ESearchCase::IgnoreCase);
		if (Sky->bFollowRoute) { return; }
		if (Args[0].Equals(TEXT("all"), ESearchCase::IgnoreCase)) { Sky->ShowAll(); return; }
		for (int32 i = 0; i < NumRealms; ++i)
		{
			if (Args[0].Equals(RealmLabels[i], ESearchCase::IgnoreCase)) { Sky->ShowRealm(ERealm(i)); return; }
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
	ShowRealm(ERealm::Midgard);   // the world starts under a quiet sky
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
			SetSkyFloat(Sky, TEXT("Aurora Intensity"), 2.5);
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
	if (Deck && DeckMat)
	{
		Deck->SetVisibility(Look.CloudCover > 0.01f);
		Deck->SetWorldLocation(FVector(Around.X, Around.Y, Around.Z + 250000.0));   // 2.5 km over the head, always
		DeckMat->SetScalarParameterValue(TEXT("Cover"), Look.CloudCover);
		DeckMat->SetScalarParameterValue(TEXT("Speed"), Look.CloudSpeed);
		DeckMat->SetScalarParameterValue(TEXT("Scale"), Look.CloudScale);
		DeckMat->SetVectorParameterValue(TEXT("Dark"), bFlash ? FLinearColor(0.75f, 0.9f, 1.2f) : Look.CloudDark);   // lightning lights the deck from inside, same frames as the ground
		DeckMat->SetVectorParameterValue(TEXT("Light"), bFlash ? FLinearColor(1.5f, 1.8f, 2.2f) : Look.CloudLight);
	}
	// UDS only rewrites its lights when they change enough, and this ticks after it: remember its values while no realm
	// runs and set ours from those - never compound a scale on top of an earlier one
	if (!bBasesKnown || (A <= 0.f && !bFlash && !bLookOn))
	{
		if (S) { SunBase = S->Intensity; SunColor = S->GetLightColor(); }
		if (L) { SkyBase = L->Intensity; }
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
	if (S) { S->SetIntensity(SunBase * Look.SunScale); S->SetLightColor(FMath::Lerp(SunColor, Look.SunColor, A)); }
	if (L)
	{
		L->SetLightColor(bFlash ? FLinearColor(0.6f, 0.8f, 1.f) : Look.SkyColor);   // the colour of the shadows
		L->SetIntensity(SkyBase * Look.SkyScale * (bFlash ? 6.f : 1.f));
	}
	if (F)
	{
		F->SetFogDensity(FMath::Lerp(FogBase, Look.FogDensity, A));
		F->SetFogHeightFalloff(FMath::Lerp(FalloffBase, Look.FogFalloff, A));
		F->SetFogCutoffDistance(Look.FogCutoff < 1.f ? 0.f : CutoffBase);   // 0: the fog takes the sky too, a real white-out
		F->SetFogInscatteringColor(FMath::Lerp(FogColorBase, Look.FogColor, A));
		// height fog is measured from its own height: keep the layer around the player, wherever on the mountain
		F->SetWorldLocation(FVector(FogLocation.X, FogLocation.Y, FMath::Lerp(FogLocation.Z, Around.Z - Look.FogBelow, A)));
	}
	// What the realm does to the body and mind, once it has really arrived; a shelter keeps it out
	const FRealmLook& Row = LookOf(Shown, bAll);
	const APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
	USurvivalComponent* Body = Player ? Player->FindComponentByClass<USurvivalComponent>() : nullptr;
	if (Body && A > 0.5f && !Body->IsSheltered() && !Body->IsDead())
	{
		Body->AddToStat(ESurvivalStat::BodyTemperature, -Row.Cold * DeltaSeconds);
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
	// All nine together belong to the summit alone: nothing else happens up there
	const bool bSummit = FVector::DistXY(Around, PlacedAround) < 60000.0;
	if (bSummit != bAtSummit)
	{
		bAtSummit = bSummit;
		bForecast = false;
		if (bSummit) { ShowAll(); } else { ShowRealm(Bag.Num() ? ERealm(Bag.Last()) : ERealm::Midgard); WeatherLeft = FMath::FRandRange(600.f, 720.f); }
	}
	if (!bScheduled || bAtSummit) { return; }
	WeatherLeft -= DeltaSeconds;
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
	const FString Name = !bAll && Shown != ERealm::None ? GWeather[int32(Shown)] : TEXT("Partly_Cloudy");
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
			if (!Thunder) { Thunder = LoadObject<USoundBase>(nullptr, TEXT("/Game/Audio/Ice/Mountain_Boom_02.Mountain_Boom_02")); }
			const APawn* P = UGameplayStatics::GetPlayerPawn(this, 0);
			if (Thunder && P && Planets[int32(ERealm::Jotunheim)].Actor.IsValid())
			{
				const FVector Dir = (Planets[int32(ERealm::Jotunheim)].Actor->GetActorLocation() - P->GetActorLocation()).GetSafeNormal();
				UE_LOG(LogTemp, Display, TEXT("RealmSky: thunder"));
				UGameplayStatics::PlaySoundAtLocation(this, Thunder, P->GetActorLocation() + Dir * 3000.f, 1.4f, FMath::FRandRange(0.7f, 0.85f));
			}
		}
	}
	// keep the sky centred on the player: 5000 km away, a planet would otherwise drift as you walk 30 km
	const APawn* Pawn = UGameplayStatics::GetPlayerPawn(this, 0);
	const FVector Around = Pawn ? Pawn->GetActorLocation() : PlacedAround;
	TickTour(DeltaSeconds);
	TickWeather(DeltaSeconds, Around);
	ApplyLook(DeltaSeconds, Around);
	// The planets cross the sky with the sky's own clock, a little slower than the sun
	if (AActor* SkyActor = FindSky(GetWorld()))
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
	for (FPlanet& P : Planets)
	{
		AActor* A = P.Actor.Get();
		if (!A)
		{
			continue;
		}
		P.Fade = FMath::FInterpConstantTo(P.Fade, P.Target, DeltaSeconds, 1.f / FadeSeconds);
		const float Eased = P.Fade * P.Fade * (3.f - 2.f * P.Fade);
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
				// 12 degrees per sky hour along a slow arc, from where the reference shot has it
				const float Drift = FMath::Fmod(SkyHours - ShownAtHours, 24.f) * 12.f;
				const FVector Dir = FRotator(Sky.Pitch + 14.f * FMath::Sin(FMath::DegreesToRadians(Drift)), Sky.Yaw + Drift, 0.f).Vector();
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
