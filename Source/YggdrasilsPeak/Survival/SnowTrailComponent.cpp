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

	TAutoConsoleVariable<float> CVarAutoWalk(
		TEXT("ygg.Snow.AutoWalk"),
		0.f,
		TEXT("Testing: >0 = the player walks straight ahead by itself at this input strength (0.5 walk, 1 jog)."));

	TAutoConsoleVariable<float> CVarAutoYaw(
		TEXT("ygg.Snow.AutoYaw"),
		-999.f,
		TEXT("Testing, with ygg.Snow.AutoWalk: the heading (world yaw, degrees) to walk; -999 = keep the current one."));

	TAutoConsoleVariable<int32> CVarStepLog(
		TEXT("ygg.Snow.StepLog"),
		0,
		TEXT("1 = log every footstep: which foot, its height and how fast it still moved (share of the walking speed)."));
}

// Testing the footsteps on rock: finds the biggest walkable patch of terrain rock near the player and puts them on it.
static FAutoConsoleCommandWithWorldAndArgs CmdGoRock(TEXT("ygg.Snow.GoRock"), TEXT("ygg.Snow.GoRock [radius m = 150] - stand on the nearest big walkable patch of rock (testing footsteps)."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
	{
		APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
		APawn* Pawn = PC ? PC->GetPawn() : nullptr;
		if (!Pawn) { return; }
		const float Radius = (Args.Num() > 0 ? FCString::Atof(*Args[0]) : 150.f) * 100.f;
		const FVector Me = Pawn->GetActorLocation();
		FCollisionQueryParams Params(SCENE_QUERY_STAT(GoRock), false, Pawn);
		Params.bReturnFaceIndex = true;
		const float Step = 300.f;
		const int32 N = FMath::CeilToInt(Radius / Step);
		TMap<FIntPoint, FVector> Rock;
		for (int32 y = -N; y <= N; ++y)
		{
			for (int32 x = -N; x <= N; ++x)
			{
				const FVector P = Me + FVector(x * Step, y * Step, 0.f);
				FHitResult Hit;
				if (World->LineTraceSingleByChannel(Hit, P + FVector(0.f, 0.f, 50000.f), P - FVector(0.f, 0.f, 50000.f), ECC_Visibility, Params)
					&& Hit.ImpactNormal.Z > 0.82f && USnowTrailComponent::IsTerrain(Hit) && USnowTrailComponent::IsRock(Hit))
				{
					Rock.Add(FIntPoint(x, y), Hit.ImpactPoint);
				}
			}
		}
		// the patch with the most rock around it (a 5 x 5 neighbourhood), nearest first on a tie
		FIntPoint Best(INT32_MAX, 0);
		int32 BestScore = 0;
		for (const TPair<FIntPoint, FVector>& R : Rock)
		{
			int32 Score = 0;
			for (int32 dy = -2; dy <= 2; ++dy) { for (int32 dx = -2; dx <= 2; ++dx) { Score += Rock.Contains(R.Key + FIntPoint(dx, dy)) ? 1 : 0; } }
			if (Score > BestScore || (Score == BestScore && FVector::DistSquared(R.Value, Me) < FVector::DistSquared(Rock.FindRef(Best), Me)))
			{
				BestScore = Score;
				Best = R.Key;
			}
		}
		if (BestScore == 0) { UE_LOG(LogTemp, Warning, TEXT("GoRock: no walkable rock within %.0f m (%d probes)"), Radius / 100.f, (2 * N + 1) * (2 * N + 1)); return; }
		// walk along the patch: towards the neighbour direction with the most rock
		const FVector At = Rock[Best];
		int32 BestDir = 0, DirScore = -1;
		for (int32 d = 0; d < 8; ++d)
		{
			const FIntPoint Dir(FMath::RoundToInt(FMath::Cos(d * PI / 4.f)), FMath::RoundToInt(FMath::Sin(d * PI / 4.f)));
			int32 S = 0;
			for (int32 k = 1; k <= 4; ++k) { S += Rock.Contains(Best + Dir * k) ? 1 : 0; }
			if (S > DirScore) { DirScore = S; BestDir = d; }
		}
		Pawn->TeleportTo(At + FVector(0.f, 0.f, Pawn->GetSimpleCollisionHalfHeight() + 20.f), FRotator(0.f, BestDir * 45.f, 0.f));
		PC->SetControlRotation(FRotator(0.f, BestDir * 45.f, 0.f));
		UE_LOG(LogTemp, Display, TEXT("GoRock: %d rock probes, standing on %s (patch %d/25), heading %d deg (rock ahead %d/4) - set ygg.Snow.AutoYaw %d"),
			Rock.Num(), *At.ToCompactString(), BestScore, BestDir * 45, DirScore, BestDir * 45);
	}));

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

	// steps waiting for the boot to land (hard ground, see PickStepSet)
	if (PendingSteps.Num() > 0)
	{
		const double Now = GetWorld()->GetTimeSeconds();
		for (int32 i = PendingSteps.Num() - 1; i >= 0; --i)
		{
			if (PendingSteps[i].At <= Now)
			{
				if (PendingSteps[i].Set) { PlayStepFrom(*PendingSteps[i].Set, PendingSteps[i].Location); }
				PendingSteps.RemoveAtSwap(i);
			}
		}
	}

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

	if (const float Auto = CVarAutoWalk.GetValueOnGameThread(); Auto > 0.f)
	{
		if (APawn* Pawn = Cast<APawn>(GetOwner()))
		{
			FVector Dir = Pawn->GetActorForwardVector();
			if (const float Yaw = CVarAutoYaw.GetValueOnGameThread(); Yaw > -900.f)
			{
				Dir = FRotator(0.f, Yaw, 0.f).Vector();
				if (AController* C = Pawn->GetController()) { C->SetControlRotation(FRotator(0.f, Yaw, 0.f)); }
			}
			Pawn->AddMovementInput(Dir, FMath::Min(Auto, 1.f));
		}
	}

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
		Feet[0].bHasLast = Feet[1].bHasLast = false;   // no stale foot position when walking starts again
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

bool USnowTrailComponent::NotifiesActive() const
{
	const UWorld* World = GetWorld();
	return World && World->GetTimeSeconds() - LastNotifyTime < 1.5;
}

void USnowTrailComponent::OnFootContact(bool bLeft, USkeletalMeshComponent* Mesh)
{
	UWorld* World = GetWorld();
	const ACharacter* Character = Cast<ACharacter>(GetOwner());
	if (!World || !Body || Mesh != Body || !Character) { return; }
	const UCharacterMovementComponent* Movement = Character->GetCharacterMovement();
	if (!Movement || !Movement->IsMovingOnGround() || Character->GetVelocity().Size2D() < 40.f) { return; }
	const double Now = World->GetTimeSeconds();
	double& Last = LastContactTime[bLeft ? 1 : 0];
	if (Now - Last < 0.18) { return; }   // the same contact again, from another clip of the blend
	Last = Now;
	LastNotifyTime = Now;
	const FFootState& Foot = Feet[0].bLeft == bLeft ? Feet[0] : Feet[1];
	// the foot is still in the air: look where it is about to land
	FVector Ahead = FVector(Foot.Velocity.X, Foot.Velocity.Y, 0.0) * StepNotifyLead;
	Ahead = Ahead.GetClampedToMaxSize(60.0);
	const FVector Land = Body->GetSocketLocation(Foot.Bone) + Ahead;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(SnowTrailContact), false, GetOwner());
	Params.bReturnPhysicalMaterial = true;
	Params.bReturnFaceIndex = true;
	FHitResult Hit;
	if (!World->LineTraceSingleByChannel(Hit, Land + FVector(0.f, 0.f, 60.f), Land - FVector(0.f, 0.f, 150.f), ECC_Visibility, Params))
	{
		return;
	}
	TimeSinceStep = 0.f;
	float Delay = 0.f;
	const TArray<TObjectPtr<USoundBase>>* Set = PickStepSet(Hit, Delay);
	if (CVarStepLog.GetValueOnGameThread() != 0)
	{
		UE_LOG(LogTemp, Display, TEXT("Contact %s: lands in %.0f ms, %s, sound after %.0f ms, walk %.0f cm/s"),
			bLeft ? TEXT("L") : TEXT("R"), StepNotifyLead * 1000.f, Set == &RubbleSteps ? TEXT("rock") : Set == &IceSteps ? TEXT("ice") : Set == &LadderSteps ? TEXT("ladder") : TEXT("snow"),
			Delay * 1000.f, Character->GetVelocity().Size2D());
	}
	if (!Set) { return; }
	if (Delay <= 0.005f) { PlayStepFrom(*Set, Hit.ImpactPoint); }
	else { PendingSteps.Add({ Now + Delay, Set, Hit.ImpactPoint }); }
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

	// How fast the foot itself moves, as a share of the walking speed: a planted foot stands still on the ground,
	// a swinging one moves at about twice the body's speed. (Play-test 2026-11-13: on rock the steps came at the wrong
	// time. The height above the ground right under a swinging foot jumps when it passes over a rock edge, which read
	// as a step; snow is smooth, so only rock did it. The speed does not care what the ground looks like.)
	const float WalkSpeed = FMath::Max(float(GetOwner()->GetVelocity().Size2D()), 60.f);
	if (Foot.bHasLast && DeltaTime > KINDA_SMALL_NUMBER)
	{
		const float Raw = float(FVector::Dist(FootLocation, Foot.LastLocation)) / DeltaTime / WalkSpeed;
		const float K = 1.f - FMath::Exp(-DeltaTime * 40.f);
		Foot.SpeedRatio = FMath::Lerp(Foot.SpeedRatio, Raw, K);
		Foot.Velocity = FMath::Lerp(Foot.Velocity, (FootLocation - Foot.LastLocation) / DeltaTime, double(K));
	}
	Foot.LastLocation = FootLocation;
	Foot.bHasLast = true;

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
		Foot.LowSeconds = 0.f;
		return;
	}
	if (Height < PlantBelow)
	{
		// low enough: it lands once it has (nearly) stopped. A clip whose feet slide a little still lands a moment
		// later, but a foot sweeping over a rock edge at full swing speed never counts.
		if (Foot.LowSeconds <= 0.f && CVarStepLog.GetValueOnGameThread() != 0)
		{
			UE_LOG(LogTemp, Display, TEXT("Low  %s: height %.1f (low %.1f high %.1f), foot speed %.2f x walk, %s"),
				Foot.bLeft ? TEXT("L") : TEXT("R"), Height, Foot.Lowest, Foot.Highest, Foot.SpeedRatio, IsRock(Hit) ? TEXT("rock") : TEXT("other"));
		}
		Foot.LowSeconds += DeltaTime;
		const bool bStill = Foot.SpeedRatio < 0.75f || (Foot.LowSeconds > 0.12f && Foot.SpeedRatio < 1.3f);
		if (!bStill)
		{
			return;
		}
		if (CVarStepLog.GetValueOnGameThread() != 0)
		{
			UE_LOG(LogTemp, Display, TEXT("Step %s: height %.1f (low %.1f high %.1f), foot speed %.2f x walk, %.0f ms after it got low, %s, walk %.0f cm/s at %s"),
				Foot.bLeft ? TEXT("L") : TEXT("R"), Height, Foot.Lowest, Foot.Highest, Foot.SpeedRatio, Foot.LowSeconds * 1000.f, IsRock(Hit) ? TEXT("rock") : TEXT("other"),
				GetOwner()->GetVelocity().Size2D(), *Hit.ImpactPoint.ToCompactString());
		}
		Foot.bPlanted = true;
		Foot.LowSeconds = 0.f;
		if (!NotifiesActive())
		{
			PlayFootstep(Hit);   // fallback for clips without foot-contact notifies
		}
		StampPrint(Hit, WalkDirection, Foot.bLeft);
	}
	else
	{
		Foot.LowSeconds = 0.f;
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

const TArray<TObjectPtr<USoundBase>>* USnowTrailComponent::PickStepSet(const FHitResult& Hit, float& OutDelay) const
{
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
	// A notify comes StepNotifyLead before the boot lands. Snow's crunch builds up and peaks as it lands, so it starts
	// at once (as approved); the hard ground's clips hit within their first few tens of ms, so they wait for the boot
	// (measured onsets: rubble ~30 ms, ice ~10 ms, ladder ~5 ms into the clip).
	OutDelay = Set == &RubbleSteps ? StepNotifyLead - 0.03f
		: Set == &IceSteps ? StepNotifyLead - 0.01f
		: Set == &LadderSteps ? StepNotifyLead - 0.005f
		: 0.f;
	return Set;
}

void USnowTrailComponent::PlayStepFrom(const TArray<TObjectPtr<USoundBase>>& Set, const FVector& Location)
{
	if (Set.Num() == 0)
	{
		return;
	}
	// never the same variation twice in a row
	int32 Pick = FMath::RandRange(0, Set.Num() - 1);
	if (Set.Num() > 1 && Pick == LastStepVariation)
	{
		Pick = (Pick + 1) % Set.Num();
	}
	LastStepVariation = Pick;
	const float Speed = GetOwner()->GetVelocity().Size2D();
	const float Volume = FootstepVolume * FMath::GetMappedRangeValueClamped(FVector2f(0.f, 450.f), FVector2f(0.6f, 1.2f), Speed);
	UGameplayStatics::PlaySoundAtLocation(this, Set[Pick], Location, Volume, FMath::FRandRange(0.92f, 1.08f));
}

void USnowTrailComponent::PlayFootstep(const FHitResult& Hit)
{
	// the fallback paths (foot height, cadence net): no notify lead to make up for, so everything plays at once
	TimeSinceStep = 0.f;
	float Delay = 0.f;
	if (const TArray<TObjectPtr<USoundBase>>* Set = PickStepSet(Hit, Delay))
	{
		PlayStepFrom(*Set, Hit.ImpactPoint);
	}
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
