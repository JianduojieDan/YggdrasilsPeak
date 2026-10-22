// Copyright Epic Games, Inc. All Rights Reserved.

#include "Survival/SnowTrailComponent.h"

#include "Survival/ExpeditionComponent.h"
#include "Survival/FirstPersonBodyComponent.h"
#include "Survival/SurvivalComponent.h"
#include "Survival/SurvivalConfig.h"
#include "Components/DecalComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/Canvas.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/KismetMaterialLibrary.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "LandscapeProxy.h"
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
	if (!FootprintMaterial)
	{
		FootprintMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Environment/Snow/M_Footprint.M_Footprint"));
	}
	if (!StepPuff)
	{
		StepPuff = LoadObject<UNiagaraSystem>(nullptr, TEXT("/Game/Environment/Snow/NS_SnowPuff.NS_SnowPuff"));
	}
	if (!Body || Body->GetBoneIndex(LeftFootBone) == INDEX_NONE || Body->GetBoneIndex(RightFootBone) == INDEX_NONE)
	{
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

void USnowTrailComponent::UpdateTrailMap(float DeltaTime)
{
	const bool bEnabled = CVarSnowDeform.GetValueOnGameThread() != 0;
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
		else if (Foot.Height < CurrentSink)
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
	UpdateFoot(Feet[0], WalkDirection);
	UpdateFoot(Feet[1], WalkDirection);
}

void USnowTrailComponent::UpdateFoot(FFootState& Foot, const FVector& WalkDirection)
{
	// Height of the foot bone above the ground right under it (the skeleton root shifts on slopes).
	const FVector FootLocation = Body->GetSocketLocation(Foot.Bone);
	FCollisionQueryParams Params(SCENE_QUERY_STAT(SnowTrail), false, GetOwner());
	Params.bReturnPhysicalMaterial = true;
	FHitResult Hit;
	if (!GetWorld()->LineTraceSingleByChannel(Hit, FootLocation + FVector(0.f, 0.f, 60.f), FootLocation - FVector(0.f, 0.f, 120.f), ECC_Visibility, Params))
	{
		return;
	}
	// The animated body's foot IK keeps its feet on the snow surface even while sunk, so no correction is needed.
	const float Height = float(FootLocation.Z - Hit.ImpactPoint.Z);
	Foot.Height = Height;
	if (Foot.bPlanted)
	{
		Foot.bPlanted = Height < PlantHeight + 4.f;
		return;
	}
	if (Height < PlantHeight)
	{
		Foot.bPlanted = true;
		StampPrint(Hit, WalkDirection, Foot.bLeft);
	}
}

bool USnowTrailComponent::IsSnow(const FHitResult& Hit)
{
	// The landscape's own snow: not tents, props or the landscape's rock (its physical material says which).
	if (!Cast<ALandscapeProxy>(Hit.GetActor()))
	{
		return false;
	}
	const UPhysicalMaterial* Surface = Hit.PhysMaterial.Get();
	return !Surface || !Surface->GetName().Contains(TEXT("Rock"));
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
	if (!IsSnow(Hit))
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
