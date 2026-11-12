// Copyright Epic Games, Inc. All Rights Reserved.

#include "Survival/SnowTrailComponent.h"

#include "Survival/ExpeditionComponent.h"
#include "Survival/FirstPersonBodyComponent.h"
#include "Survival/SurvivalComponent.h"
#include "Survival/SurvivalConfig.h"
#include "Components/DecalComponent.h"
#include "Environment/CrevasseCrossing.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/Canvas.h"
#include "Engine/Texture2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/KismetMaterialLibrary.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "LandscapeProxy.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialParameterCollection.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "PhysicalMaterials/PhysicalMaterial.h"

namespace
{
	TAutoConsoleVariable<int32> CVarSnowTrails(
		TEXT("ygg.Snow.Trails"),
		0,
		TEXT("1 = feet also stamp boot-print decals (the older look), 0 = off."));

	TAutoConsoleVariable<int32> CVarSnowDeform(
		TEXT("ygg.Snow.Deform"),
		1,
		TEXT("1 = legs push a real trench into the snow (trail map + landscape displacement), 0 = off."));

	// The in-game sound tour (snd) sets this so every step sounds like one surface wherever you walk.
	TAutoConsoleVariable<int32> CVarStepSurface(
		TEXT("ygg.Snow.StepSurface"),
		0,
		TEXT("Footstep sound surface: 0 = the real ground, 1 = snow, 2 = ice, 3 = rubble, 4 = ladder."));
}

USnowTrailComponent::USnowTrailComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	// After animation, so the foot bones are this frame's pose.
	PrimaryComponentTick.TickGroup = TG_PostUpdateWork;
}

void USnowTrailComponent::BeginPlay()
{
	Super::BeginPlay();

	const ACharacter* Character = Cast<ACharacter>(GetOwner());
	Body = Character ? Character->GetMesh() : nullptr;
	LoadDefaultFootsteps();
	if (!FootprintMaterial)
	{
		FootprintMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Environment/Snow/M_Footprint.M_Footprint"));
	}
	if (!StepPuff)
	{
		StepPuff = LoadObject<UNiagaraSystem>(nullptr, TEXT("/Game/Environment/Snow/NS_SnowPuff.NS_SnowPuff"));
	}
	// Characters on other skeletons (e.g. a Mixamo-style rig retargeted from the mannequin) name their feet differently.
	auto FindFoot = [this](FName& Bone, std::initializer_list<const TCHAR*> Alternatives)
	{
		for (const TCHAR* Name : Alternatives)
		{
			if (Body->GetBoneIndex(Bone) != INDEX_NONE)
			{
				return;
			}
			Bone = Name;
		}
	};
	if (Body)
	{
		FindFoot(LeftFootBone, { TEXT("LeftFoot"), TEXT("mixamorig:LeftFoot"), TEXT("foot_l") });
		FindFoot(RightFootBone, { TEXT("RightFoot"), TEXT("mixamorig:RightFoot"), TEXT("foot_r") });
	}
	if (!Body || Body->GetBoneIndex(LeftFootBone) == INDEX_NONE || Body->GetBoneIndex(RightFootBone) == INDEX_NONE)
	{
		UE_LOG(LogTemp, Warning, TEXT("SnowTrail: no foot bones found on %s, footprints are off"), Body ? *GetNameSafe(Body->GetSkeletalMeshAsset()) : TEXT("(no mesh)"));
		SetComponentTickEnabled(false);
		return;
	}
	Feet[0].Bone = LeftFootBone;
	Feet[0].bLeft = true;
	Feet[1].Bone = RightFootBone;

	if (!TrailMap)
	{
		TrailMap = LoadObject<UTextureRenderTarget2D>(nullptr, TEXT("/Game/Environment/Snow/RT_SnowTrail.RT_SnowTrail"));
	}
	if (!TrailParameters)
	{
		TrailParameters = LoadObject<UMaterialParameterCollection>(nullptr, TEXT("/Game/Environment/Snow/MPC_SnowTrail.MPC_SnowTrail"));
	}
	if (!TrailBrush)
	{
		TrailBrush = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Environment/Snow/M_TrailBrush.M_TrailBrush"));
	}
	if (!BootBrush)
	{
		BootBrush = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Environment/Snow/M_TrailBoot.M_TrailBoot"));
	}
	if (!TrailSoftMap)
	{
		TrailSoftMap = LoadObject<UTextureRenderTarget2D>(nullptr, TEXT("/Game/Environment/Snow/RT_SnowTrailSoft.RT_SnowTrailSoft"));
	}
	if (!TrailBlur)
	{
		TrailBlur = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Environment/Snow/M_TrailBlur.M_TrailBlur"));
	}
	if (!TrailFill)
	{
		TrailFill = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Environment/Snow/M_TrailFill.M_TrailFill"));
	}
	if (TrailMap && TrailParameters && TrailBrush && BootBrush && TrailSoftMap && TrailBlur)
	{
		FillInstance = TrailFill ? UMaterialInstanceDynamic::Create(TrailFill, this) : nullptr;
		BlurInstance = UMaterialInstanceDynamic::Create(TrailBlur, this);
		BlurInstance->SetTextureParameterValue(TEXT("Trail"), TrailMap);
		BlurInstance->SetScalarParameterValue(TEXT("Texel"), 1.f / float(TrailMap->SizeX));
		UKismetRenderingLibrary::ClearRenderTarget2D(this, TrailSoftMap, FLinearColor::Black);
		BrushInstance = UMaterialInstanceDynamic::Create(TrailBrush, this);
		for (int32 Side = 0; Side < 2; ++Side)
		{
			BootInstances[Side] = UMaterialInstanceDynamic::Create(BootBrush, this);
			BootInstances[Side]->SetScalarParameterValue(TEXT("Mirror"), float(Side));
		}
		TrailScratch = UKismetRenderingLibrary::CreateRenderTarget2D(this, TrailMap->SizeX, TrailMap->SizeY, TrailMap->RenderTargetFormat, FLinearColor::Black);
		UKismetRenderingLibrary::ClearRenderTarget2D(this, TrailMap, FLinearColor::Black);
		TrailCentre = FVector2D(GetOwner()->GetActorLocation());
		bTrailReady = true;
		PublishTrailArea(CVarSnowDeform.GetValueOnGameThread() != 0);
	}
}

void USnowTrailComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// The trail area lives in a shared parameter collection: switch it off so the editor shows untouched snow.
	if (bTrailReady)
	{
		PublishTrailArea(false);
	}
	Super::EndPlay(EndPlayReason);
}

void USnowTrailComponent::PublishTrailArea(bool bEnabled) const
{
	UKismetMaterialLibrary::SetVectorParameterValue(GetWorld(), TrailParameters, TEXT("TrailArea"),
		FLinearColor(float(TrailCentre.X), float(TrailCentre.Y), TrailAreaCm, bEnabled ? 1.f : 0.f));
}

void USnowTrailComponent::RecentreTrailMap(const FVector2D& NewCentre)
{
	// Snap to whole texels so what is already drawn moves by an exact pixel offset and does not blur.
	const float Texel = TrailAreaCm / float(TrailMap->SizeX);
	const FVector2D Snapped(FMath::RoundToDouble(NewCentre.X / Texel) * Texel, FMath::RoundToDouble(NewCentre.Y / Texel) * Texel);
	const FVector2D ShiftPixels = (TrailCentre - Snapped) / Texel;
	const FVector2D Size(TrailMap->SizeX, TrailMap->SizeY);

	// old map -> scratch (shifted), scratch -> map
	UKismetRenderingLibrary::ClearRenderTarget2D(this, TrailScratch, FLinearColor::Black);
	UCanvas* Canvas = nullptr;
	FVector2D CanvasSize;
	FDrawToRenderTargetContext Context;
	UKismetRenderingLibrary::BeginDrawCanvasToRenderTarget(this, TrailScratch, Canvas, CanvasSize, Context);
	Canvas->K2_DrawTexture(TrailMap, ShiftPixels, Size, FVector2D::ZeroVector, FVector2D::UnitVector, FLinearColor::White, BLEND_Opaque);
	UKismetRenderingLibrary::EndDrawCanvasToRenderTarget(this, Context);

	UKismetRenderingLibrary::BeginDrawCanvasToRenderTarget(this, TrailMap, Canvas, CanvasSize, Context);
	Canvas->K2_DrawTexture(TrailScratch, FVector2D::ZeroVector, Size, FVector2D::ZeroVector, FVector2D::UnitVector, FLinearColor::White, BLEND_Opaque);
	UKismetRenderingLibrary::EndDrawCanvasToRenderTarget(this, Context);

	TrailCentre = Snapped;
}

void USnowTrailComponent::FillTrailMap(float DeltaTime)
{
	// Exponential refill: each step blends the whole map a little towards 0. Done every half second rather than every
	// frame: at 120 fps the per-frame step (~0.0001) is below what a 16-bit float near 1.0 can store, so it never moved.
	constexpr float FillInterval = 0.5f;
	FillTimer += DeltaTime;
	if (FillTimer < FillInterval || !FillInstance)
	{
		return;
	}
	const UExpeditionComponent* Expedition = GetOwner()->FindComponentByClass<UExpeditionComponent>();
	const bool bStorm = Expedition && Expedition->GetStormStrength() > StormFillAbove;
	const float HalfLife = bStorm ? StormTrailHalfLifeSeconds : TrailHalfLifeSeconds;
	FillInstance->SetScalarParameterValue(TEXT("Amount"), 1.f - FMath::Pow(0.5f, FillTimer / HalfLife));
	FillTimer = 0.f;

	UCanvas* Canvas = nullptr;
	FVector2D CanvasSize;
	FDrawToRenderTargetContext Context;
	UKismetRenderingLibrary::BeginDrawCanvasToRenderTarget(this, TrailMap, Canvas, CanvasSize, Context);
	Canvas->K2_DrawMaterial(FillInstance, FVector2D::ZeroVector, CanvasSize, FVector2D::ZeroVector, FVector2D::UnitVector);
	UKismetRenderingLibrary::EndDrawCanvasToRenderTarget(this, Context);
}

int32 USnowTrailComponent::FootprintQuality = 1;

void USnowTrailComponent::UpdateTrailMap(float DeltaTime)
{
	// footprint quality (display settings): High halves the area the same map covers -> twice the detail
	if (AppliedQuality != FootprintQuality)
	{
		if (BaseTrailAreaCm < 0.f) { BaseTrailAreaCm = TrailAreaCm; BaseRecentreCm = RecentreDistanceCm; }
		const float Area = FootprintQuality >= 2 ? BaseTrailAreaCm * 0.5f : BaseTrailAreaCm;
		if (!FMath::IsNearlyEqual(Area, TrailAreaCm))
		{
			TrailAreaCm = Area;
			RecentreDistanceCm = BaseRecentreCm * (Area / BaseTrailAreaCm);
			UKismetRenderingLibrary::ClearRenderTarget2D(this, TrailMap, FLinearColor::Black);   // old prints were drawn at the old scale
			UKismetRenderingLibrary::ClearRenderTarget2D(this, TrailSoftMap, FLinearColor::Black);
		}
		AppliedQuality = FootprintQuality;
	}
	const bool bEnabled = CVarSnowDeform.GetValueOnGameThread() != 0 && FootprintQuality > 0;
	PublishTrailArea(bEnabled);
	if (!bEnabled)
	{
		return;
	}

	const FVector2D Player(GetOwner()->GetActorLocation());
	if (FVector2D::Distance(Player, TrailCentre) > RecentreDistanceCm)
	{
		RecentreTrailMap(Player);
		PublishTrailArea(true);
	}
	FillTrailMap(DeltaTime);

	// Only feet in snow press into it; deep powder all the way, hard crust just a little.
	if (GroundSoftness <= 0.f && CurrentSink <= 0.5f)
	{
		SoftenTrailMap();
		return;
	}
	const float Depth = FMath::Max(GroundSoftness, 0.15f);
	BootInstances[0]->SetScalarParameterValue(TEXT("Depth"), Depth);
	BootInstances[1]->SetScalarParameterValue(TEXT("Depth"), Depth);
	BrushInstance->SetScalarParameterValue(TEXT("Depth"), Depth * 0.75f);

	const float PixelsPerCm = float(TrailMap->SizeX) / TrailAreaCm;
	const FVector Forward = GetOwner()->GetActorForwardVector().GetSafeNormal2D();
	// The boot texture's U runs heel -> toe; turn it to where the body faces (pixel axes = world X/Y).
	const float Yaw = FMath::RadiansToDegrees(FMath::Atan2(Forward.Y, Forward.X));
	const FVector2D BootSize(BootLengthCm * PixelsPerCm, BootWidthCm * PixelsPerCm);
	const FVector2D DragSize(LegRadiusCm * 2.f * PixelsPerCm);
	UCanvas* Canvas = nullptr;
	FVector2D CanvasSize;
	FDrawToRenderTargetContext Context;
	UKismetRenderingLibrary::BeginDrawCanvasToRenderTarget(this, TrailMap, Canvas, CanvasSize, Context);
	for (const FFootState& Foot : Feet)
	{
		const FVector FootLocation = Body->GetSocketLocation(Foot.Bone);
		if (Foot.bPlanted)
		{
			// A planted foot leaves a boot print...
			const FVector2D Sole = FVector2D(FootLocation + Forward * SoleForwardCm);
			const FVector2D Pixel = (Sole - TrailCentre) * PixelsPerCm + CanvasSize * 0.5f;
			Canvas->K2_DrawMaterial(BootInstances[Foot.bLeft ? 1 : 0], Pixel - BootSize * 0.5f, BootSize, FVector2D::ZeroVector, FVector2D::UnitVector, Yaw, FVector2D(0.5f, 0.5f));
		}
		else if (Foot.Height - Foot.Lowest < CurrentSink)
		{
			// ...and a lifted one still under the surface of deep powder ploughs a channel to the next print.
			const FVector2D Pixel = (FVector2D(FootLocation) - TrailCentre) * PixelsPerCm + CanvasSize * 0.5f;
			Canvas->K2_DrawMaterial(BrushInstance, Pixel - DragSize * 0.5f, DragSize, FVector2D::ZeroVector, FVector2D::UnitVector);
		}
	}
	UKismetRenderingLibrary::EndDrawCanvasToRenderTarget(this, Context);
	SoftenTrailMap();
}

void USnowTrailComponent::SoftenTrailMap()
{
	// Sharp boot stamps on 2.5 cm texels turned into stair-steps when displaced directly; the landscape reads this
	// blurred copy instead (round walls, plus a wide spread for the banks). One full-screen pass, ~0.05 ms.
	UKismetRenderingLibrary::DrawMaterialToRenderTarget(this, TrailSoftMap, BlurInstance);
}

float USnowTrailComponent::SnowSoftness(const FVector& Location, const FVector& GroundNormal)
{
	const USurvivalConfig* C = GetDefault<USurvivalConfig>();
	const float Elevation = C->ReferenceElevationMeters + float(Location.Z - C->ReferenceWorldZ) / 100.f;
	// Same shape as the landscape shader's "hard" mask, without its noise.
	const float High = FMath::SmoothStep(6600.f, 7600.f, Elevation) * 0.9f;
	const float Steep = (1.f - FMath::SmoothStep(0.72f, 0.92f, float(GroundNormal.Z))) * 0.55f;
	return 1.f - FMath::Clamp(High + Steep, 0.f, 1.f);
}

void USnowTrailComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	const ACharacter* Character = Cast<ACharacter>(GetOwner());
	if (!Character || !Body)
	{
		return;
	}

	// A blizzard fills the prints in.
	const UExpeditionComponent* Expedition = Character->FindComponentByClass<UExpeditionComponent>();
	const bool bStorm = Expedition && Expedition->GetStormStrength() > StormFillAbove;
	if (bStorm && !bStormFilling)
	{
		FillInPrints(StormFillSeconds);
	}
	bStormFilling = bStorm;

	UpdateSinking(DeltaTime);
	if (bTrailReady)
	{
		UpdateTrailMap(DeltaTime);
	}

	const UCharacterMovementComponent* Movement = Character->GetCharacterMovement();
	const FVector Velocity = Character->GetVelocity();
	const bool bWalking = Movement && Movement->IsMovingOnGround() && Velocity.Size2D() > 40.f;
	if (!bWalking)
	{
		// Standing still or in the air: feet count as planted, so the next real step is not missed or doubled.
		Feet[0].bPlanted = Feet[1].bPlanted = true;
		return;
	}

	const FVector WalkDirection = Velocity.GetSafeNormal2D();
	UpdateFoot(Feet[0], WalkDirection, DeltaTime);
	UpdateFoot(Feet[1], WalkDirection, DeltaTime);

	// Safety net: walking steps went silent after a weather change while running steps stayed (play-test 2026-11-11,
	// cause not found yet). If no foot has planted for over two step lengths (~80 cm a step), play the step anyway and
	// say so in the log with the foot heights, so the real cause can be found.
	TimeSinceStep += DeltaTime;
	const float StepEvery = FMath::Clamp(80.f / FMath::Max(float(Velocity.Size2D()), 1.f), 0.25f, 0.9f);
	if (TimeSinceStep > StepEvery * 2.2f)
	{
		const FVector Centre = Character->GetActorLocation();
		FCollisionQueryParams Params(SCENE_QUERY_STAT(SnowTrailCadence), false, Character);
		Params.bReturnPhysicalMaterial = true;
		Params.bReturnFaceIndex = true;
		FHitResult Hit;
		if (GetWorld()->LineTraceSingleByChannel(Hit, Centre, Centre - FVector(0.f, 0.f, Character->GetSimpleCollisionHalfHeight() + 60.f), ECC_Visibility, Params))
		{
			if ((++CadenceSteps % 20) == 1)
			{
				UE_LOG(LogTemp, Warning, TEXT("SnowTrail: a step from the cadence net (#%d), plant detection missed it: speed %.0f, left foot %.1f cm (low %.1f high %.1f), right %.1f (low %.1f high %.1f), sink %.1f"),
					CadenceSteps, Velocity.Size2D(), Feet[0].Height, Feet[0].Lowest, Feet[0].Highest, Feet[1].Height, Feet[1].Lowest, Feet[1].Highest, CurrentSink);
			}
			PlayFootstep(Hit);
		}
		TimeSinceStep = 0.f;
	}
}

void USnowTrailComponent::UpdateFoot(FFootState& Foot, const FVector& WalkDirection, float DeltaTime)
{
	// Height of the foot bone above the ground right under it (the skeleton root shifts on slopes).
	const FVector FootLocation = Body->GetSocketLocation(Foot.Bone);
	FCollisionQueryParams Params(SCENE_QUERY_STAT(SnowTrail), false, GetOwner());
	Params.bReturnPhysicalMaterial = true;
	Params.bReturnFaceIndex = true;   // rock faces of the detail tiles are known by their index (USnowTrailComponent::IsRock)
	FHitResult Hit;
	if (!GetWorld()->LineTraceSingleByChannel(Hit, FootLocation + FVector(0.f, 0.f, 60.f), FootLocation - FVector(0.f, 0.f, 120.f), ECC_Visibility, Params))
	{
		return;
	}
	// The animated body's foot IK keeps its feet on the snow surface even while sunk, so no correction is needed.
	const float Height = float(FootLocation.Z - Hit.ImpactPoint.Z);
	Foot.Height = Height;

	// Each foot's own lowest and highest height over the last steps: the planted height depends on the skeleton
	// (Mixamo ankles sit higher than the mannequin's) and the lift on the clip, so thresholds sit inside that range.
	const float Drift = SwingDriftCmPerSecond * DeltaTime;
	Foot.Lowest = FMath::Min(Height, Foot.Lowest + Drift);
	Foot.Highest = FMath::Max(Height, Foot.Highest - Drift);
	const float Swing = FMath::Max(Foot.Highest - Foot.Lowest, MinSwingCm);
	const float PlantBelow = Foot.Lowest + Swing * PlantFraction;
	if (Foot.bPlanted)
	{
		Foot.bPlanted = Height < PlantBelow + Swing * LiftHysteresis;
		return;
	}
	if (Height < PlantBelow)
	{
		Foot.bPlanted = true;
		PlayFootstep(Hit);
		StampPrint(Hit, WalkDirection, Foot.bLeft);
	}
}

namespace
{
	/** T_GlacierMask (real glacier outlines on the heightmap grid), read once into memory for the footstep checks. */
	struct FGlacierMask
	{
		TArray<uint8> Pixels;
		int32 Size = 0;
		bool bLoaded = false;

		void Load()
		{
			bLoaded = true;
			const UTexture2D* Tex = LoadObject<UTexture2D>(nullptr, TEXT("/Game/Environment/Glacier/T_GlacierMask.T_GlacierMask"));
#if WITH_EDITORONLY_DATA
			if (Tex && Tex->Source.IsValid() && Tex->Source.GetFormat() == TSF_G8)
			{
				TArray64<uint8> Data;
				const_cast<UTexture2D*>(Tex)->Source.GetMipData(Data, 0);
				Size = Tex->Source.GetSizeX();
				Pixels.Append(Data.GetData(), int32(Data.Num()));
			}
#endif
		}

		float Sample(const FVector& Location)
		{
			if (!bLoaded)
			{
				Load();
			}
			if (Size <= 0)
			{
				return 0.f;
			}
			const double Extent = 3374.24 * 1009.0;
			const int32 X = FMath::FloorToInt32((Location.X + 504000.0) / Extent * Size);
			const int32 Y = FMath::FloorToInt32((Location.Y + 504000.0) / Extent * Size);
			return (X >= 0 && Y >= 0 && X < Size && Y < Size) ? Pixels[Y * Size + X] / 255.f : 0.f;
		}
	};
	FGlacierMask GlacierMask;
}

bool USnowTrailComponent::IsGlacierHard(const FVector& Location)
{
	// Same zones as the landscape's Glacier node (debris tongue and bare ice up to ~6000 m); its noisy snow patches
	// can't be matched on the CPU (GPU sin differs), so the whole zone counts as rubble / ice here.
	const USurvivalConfig* C = GetDefault<USurvivalConfig>();
	const float Elevation = C->ReferenceElevationMeters + float(Location.Z - C->ReferenceWorldZ) / 100.f;
	return Elevation < 5975.f && GlacierMask.Sample(Location) > 0.5f;
}

void USnowTrailComponent::LoadDefaultFootsteps()
{
	// Dir/NameNN.NameNN (or NameN when Digits is 1)
	auto Fill = [](TArray<TObjectPtr<USoundBase>>& Sounds, const FString& Dir, const FString& Name, int32 Count, int32 Digits)
	{
		if (Sounds.Num() > 0)
		{
			return;
		}
		for (int32 i = 1; i <= Count; ++i)
		{
			const FString Asset = Name + (Digits == 2 ? FString::Printf(TEXT("%02d"), i) : FString::FromInt(i));
			if (USoundBase* Sound = LoadObject<USoundBase>(nullptr, *(Dir / Asset + TEXT(".") + Asset), nullptr, LOAD_NoWarn | LOAD_Quiet))
			{
				Sounds.Add(Sound);
			}
		}
	};
	// boots in snow and crampons on ice: single steps cut from recordings (Tools/Audio/build_game_sounds.py)
	Fill(SnowSteps, TEXT("/Game/Audio/Footsteps"), TEXT("Step_Snow_"), 16, 2);
	Fill(SnowSteps, TEXT("/Game/Content_From_Internet/UltraDynamicSky/Sound/Snow"), TEXT("Snow_Compress_"), 6, 1);   // only if none above
	Fill(IceSteps, TEXT("/Game/Audio/Footsteps"), TEXT("Step_Ice_"), 16, 2);
	Fill(RubbleSteps, TEXT("/Game/Audio/Footsteps"), TEXT("Step_Rubble_"), 10, 2);
	Fill(LadderSteps, TEXT("/Game/Audio/Footsteps"), TEXT("Step_Ladder_"), 10, 2);
}

void USnowTrailComponent::PlayFootstep(const FHitResult& Hit)
{
	TimeSinceStep = 0.f;
	// Which surface: the ladder over a crevasse, the landscape (rock, glacier rubble / bare ice, hard crust or
	// snow, the same zones the landscape material and the footprints use), or ice meshes (seracs, crevasse lips).
	const TArray<TObjectPtr<USoundBase>>* Set = &SnowSteps;
	const ACrevasseCrossing* Crevasse = Cast<ACrevasseCrossing>(Hit.GetActor());
	if (Crevasse && Hit.GetComponent() == Crevasse->Ladder)
	{
		Set = &LadderSteps;
	}
	else if (IsTerrain(Hit))
	{
		const USurvivalConfig* C = GetDefault<USurvivalConfig>();
		const float ElevationM = C->ReferenceElevationMeters + float(Hit.ImpactPoint.Z - C->ReferenceWorldZ) / 100.f;
		if (IsRock(Hit) || (IsGlacierHard(Hit.ImpactPoint) && ElevationM < 5250.f))
		{
			Set = &RubbleSteps;
		}
		else if (IsGlacierHard(Hit.ImpactPoint) || SnowSoftness(Hit.ImpactPoint, Hit.ImpactNormal) < 0.15f)
		{
			Set = &IceSteps;
		}
	}
	else if (Hit.ImpactNormal.Z < 0.75f)
	{
		Set = &IceSteps;     // the steep side of a serac or crevasse lip; their tops carry snow
	}
	switch (CVarStepSurface.GetValueOnGameThread())
	{
	case 1: Set = &SnowSteps; break;
	case 2: Set = &IceSteps; break;
	case 3: Set = &RubbleSteps; break;
	case 4: Set = &LadderSteps; break;
	default: break;
	}
	if (Set->Num() == 0)
	{
		return;
	}
	// never the same variation twice in a row
	int32 Pick = FMath::RandRange(0, Set->Num() - 1);
	if (Set->Num() > 1 && Pick == LastStepVariation)
	{
		Pick = (Pick + 1) % Set->Num();
	}
	LastStepVariation = Pick;
	const float Speed = GetOwner()->GetVelocity().Size2D();
	const float Volume = FootstepVolume * FMath::GetMappedRangeValueClamped(FVector2f(0.f, 450.f), FVector2f(0.6f, 1.2f), Speed);
	UGameplayStatics::PlaySoundAtLocation(this, (*Set)[Pick], Hit.ImpactPoint, Volume, FMath::FRandRange(0.92f, 1.08f));
}

bool USnowTrailComponent::IsTerrain(const FHitResult& Hit)
{
	if (Cast<ALandscapeProxy>(Hit.GetActor()))
	{
		return true;
	}
	// Route detail tiles (Tools/Terrain/build_route_terrain.py): plain static mesh actors, known by their mesh folder
	const UStaticMeshComponent* Mesh = Cast<UStaticMeshComponent>(Hit.GetComponent());
	const UStaticMesh* Asset = Mesh ? Mesh->GetStaticMesh() : nullptr;
	return Asset && Asset->GetPackage()->GetFName().ToString().StartsWith(TEXT("/Game/Environment/DetailTiles/"));
}

bool USnowTrailComponent::IsRock(const FHitResult& Hit)
{
	const UPhysicalMaterial* Surface = Hit.PhysMaterial.Get();
	if (Surface && Surface->GetName().Contains(TEXT("Rock")))
	{
		return true;   // the landscape's rock layer
	}
	// Route detail tiles: their collision mesh (Tools/Terrain/build_route_collision.py) holds the snow faces first
	// (slot 0) and the rock faces after them (slot 1). Mesh traces in UE 5.8 only report the first material, so the
	// face index decides: past the snow section's triangle count, it is rock.
	const UStaticMeshComponent* Mesh = Cast<UStaticMeshComponent>(Hit.GetComponent());
	const UStaticMesh* Asset = Mesh ? Mesh->GetStaticMesh() : nullptr;
#if WITH_EDITORONLY_DATA
	const UStaticMesh* Col = Asset ? Asset->ComplexCollisionMesh.Get() : nullptr;
#else
	const UStaticMesh* Col = nullptr;
#endif
	if (!Col && Asset)
	{
		// a packaged game has no editor link to the collision mesh: it sits next to the tile as <name>_col
		static TMap<TWeakObjectPtr<const UStaticMesh>, TWeakObjectPtr<const UStaticMesh>> ColOf;
		if (const TWeakObjectPtr<const UStaticMesh>* Known = ColOf.Find(Asset)) { Col = Known->Get(); }
		else
		{
			const FString Pkg = Asset->GetOutermost()->GetName() + TEXT("_col");
			const FString Name = Asset->GetName() + TEXT("_col");
			Col = LoadObject<UStaticMesh>(nullptr, *(Pkg + TEXT(".") + Name), nullptr, LOAD_NoWarn | LOAD_Quiet);
			ColOf.Add(Asset, Col);
		}
	}
	if (!Col || Hit.FaceIndex < 0 || !Col->GetRenderData() || Col->GetRenderData()->LODResources.Num() == 0)
	{
		return false;
	}
	int32 Faces = 0;
	for (const FStaticMeshSection& Section : Col->GetRenderData()->LODResources[0].Sections)
	{
		if (!Section.bEnableCollision) { continue; }
		Faces += Section.NumTriangles;
		if (Hit.FaceIndex < Faces) { return Section.MaterialIndex == 1; }
	}
	return false;
}

bool USnowTrailComponent::IsSnow(const FHitResult& Hit)
{
	// The mountain's own snow: not tents, props, rock or glacier rubble / bare ice.
	return IsTerrain(Hit) && !IsRock(Hit) && !IsGlacierHard(Hit.ImpactPoint);
}

void USnowTrailComponent::UpdateSinking(float DeltaTime)
{
	ACharacter* Character = Cast<ACharacter>(GetOwner());
	SinkCheckTimer -= DeltaTime;
	if (SinkCheckTimer <= 0.f)
	{
		SinkCheckTimer = 0.1f;
		const FVector Centre = Character->GetActorLocation();
		const float HalfHeight = Character->GetSimpleCollisionHalfHeight();
		FCollisionQueryParams Params(SCENE_QUERY_STAT(SnowSink), false, Character);
		Params.bReturnPhysicalMaterial = true;
		Params.bReturnFaceIndex = true;   // rock faces of the detail tiles are known by their index (USnowTrailComponent::IsRock)
		FHitResult Hit;
		GroundSoftness = 0.f;
		if (Character->GetCharacterMovement()->IsMovingOnGround()
			&& GetWorld()->LineTraceSingleByChannel(Hit, Centre, Centre - FVector(0.f, 0.f, HalfHeight + 60.f), ECC_Visibility, Params)
			&& IsSnow(Hit))
		{
			GroundSoftness = SnowSoftness(Hit.ImpactPoint, Hit.ImpactNormal);
		}
		USurvivalComponent* Survival = Character->FindComponentByClass<USurvivalComponent>();
		const bool bInCamp = Survival && Survival->GetSnapshot().bSheltered;
		const float Depth = GroundSoftness * (bInCamp ? CampSinkMultiplier : 1.f);
		TargetSink = MaxSinkCm * Depth;
		// The same depth slows walking and costs stamina (deep powder up to 20% slower).
		if (Survival)
		{
			Survival->SetSnowDepth(Depth);
		}
	}
	CurrentSink = FMath::FInterpTo(CurrentSink, TargetSink, DeltaTime, SinkSpeed);
	if (UFirstPersonBodyComponent* FirstPerson = Character->FindComponentByClass<UFirstPersonBodyComponent>())
	{
		FirstPerson->SetSinkDepth(CurrentSink);
	}
}

void USnowTrailComponent::StampPrint(const FHitResult& Hit, const FVector& WalkDirection, bool bLeft)
{
	// rubble and bare glacier ice take no print (the trench shade drew a blue channel across the moraine, 2026-11-11)
	if (!IsSnow(Hit) || (IsTerrain(Hit) && IsGlacierHard(Hit.ImpactPoint)))
	{
		return;
	}

	const FVector Normal = Hit.ImpactNormal;
	const float Softness = SnowSoftness(Hit.ImpactPoint, Normal);
	if (StepPuff)
	{
		// Powder kicks up more in soft snow; hard crust only gives off a little dust.
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), StepPuff, Hit.ImpactPoint + Normal * 3.f, FRotator::ZeroRotator,
			FVector(FMath::Lerp(0.6f, 1.3f, Softness)), true, true, ENCPoolMethod::AutoRelease);
	}
	if (CVarSnowTrails.GetValueOnGameThread() == 0 || !FootprintMaterial)
	{
		return;
	}

	UDecalComponent* Print = nullptr;
	if (Pool.Num() < MaxPrints)
	{
		Print = NewObject<UDecalComponent>(GetOwner());
		// World-fixed: the print stays where it was made while the owner walks on.
		Print->SetUsingAbsoluteLocation(true);
		Print->SetUsingAbsoluteRotation(true);
		Print->SetUsingAbsoluteScale(true);
		Print->SetDecalMaterial(FootprintMaterial);
		Print->SetFadeScreenSize(0.002f);
		Print->RegisterComponent();
		Pool.Add(Print);
	}
	else
	{
		Print = Pool[NextPrint];
	}
	NextPrint = (NextPrint + 1) % MaxPrints;

	// The decal projects along its X axis (into the ground); Z runs along the walking direction.
	Print->DecalSize = FVector(PrintHalfSize.Z, PrintHalfSize.Y, PrintHalfSize.X);
	Print->SetWorldLocationAndRotation(Hit.ImpactPoint, FRotationMatrix::MakeFromXZ(-Normal, WalkDirection).Rotator());
	Print->SetDecalColor(FLinearColor(bLeft ? 1.f : 0.f, 0.f, 0.f, FMath::Lerp(0.35f, 1.f, Softness)));
	// Restarts the lifetime: the material multiplies opacity by DecalLifetimeOpacity.
	Print->SetFadeOut(PrintLifeSeconds, PrintFadeSeconds, false);
	Print->SetVisibility(true);
}

void USnowTrailComponent::FillInPrints(float Seconds)
{
	for (UDecalComponent* Print : Pool)
	{
		if (Print)
		{
			Print->SetFadeOut(0.f, Seconds, false);
		}
	}
}
