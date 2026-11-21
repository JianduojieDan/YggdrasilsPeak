// Copyright Epic Games, Inc. All Rights Reserved.

#include "Survival/SnowTrailComponent.h"

#include "Survival/ExpeditionComponent.h"
#include "Survival/FirstPersonBodyComponent.h"
#include "Survival/SurvivalComponent.h"
#include "Survival/SurvivalConfig.h"
#include "Survival/YggViewRig.h"
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
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialParameterCollection.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "UObject/ConstructorHelpers.h"
#include "RHIGPUReadback.h"
#include "RenderGraphBuilder.h"
#include "RenderGraphUtils.h"
#include "RenderingThread.h"
#include "TextureResource.h"
#include "Math/Float16.h"
#include <atomic>

/** A patch of TrailSoftMap read back to the CPU (#84). The game thread sends a copy and owns everything but the three
 *  render-thread fields; the render thread copies, polls the fence, decodes into Pending and then sets bFresh, after which
 *  the game thread takes Pending over (it never sends the next copy before that, so the two never touch Pending at once). */
struct USnowTrailComponent::FTrailPatch
{
	TUniquePtr<FRHIGPUTextureReadback> Readback;
	/** Texels per side. Fixed for the patch's life: the readback keeps the staging texture of its first copy. */
	int32 Size = 0;
	// the copy in flight (game thread)
	FIntPoint Origin = FIntPoint::ZeroValue;
	FVector2D Centre = FVector2D::ZeroVector;   // the trail map's centre and side when the copy was sent
	float Area = 0.f;
	int32 MapSize = 0;
	int32 Epoch = 0;
	double SentAt = -100.0;
	bool bInFlight = false;
	// render thread
	std::atomic<bool> bPollQueued{ false };
	std::atomic<bool> bFresh{ false };
	EPixelFormat Format = PF_Unknown;
	TArray<FVector2f> Pending;   // R, G per texel, row by row
	// the last finished read (game thread)
	TArray<FVector2f> Values;
	FIntPoint ValuesOrigin = FIntPoint::ZeroValue;
	FVector2D ValuesCentre = FVector2D::ZeroVector;
	float ValuesArea = 0.f;
	int32 ValuesMapSize = 0;
	int32 ValuesEpoch = -1;
	double ValuesAt = -100.0;
};

namespace
{
	void SnowTrailWarmGlacierMask();   // reads T_GlacierMask now instead of at the first footstep check (below)

	TAutoConsoleVariable<int32> CVarSnowTrails(
		TEXT("ygg.Snow.Trails"),
		0,
		TEXT("1 = feet also stamp boot-print decals (the older look), 0 = off."));

	TAutoConsoleVariable<int32> CVarSnowDeform(
		TEXT("ygg.Snow.Deform"),
		1,
		TEXT("1 = legs push a real trench into the snow (trail map + landscape displacement), 0 = off."));

	TAutoConsoleVariable<int32> CVarShadowRefresh(
		TEXT("ygg.Snow.ShadowRefresh"),
		0,
		TEXT("Off by default (2026-11-14: the blocky shadows the author saw were broken prayer flags, not the cache). 1 = keep the sun's cached shadow up to date over the trail (an invisible box steps across the trail area, one of 8x8 cells a frame); 0 = off (old pits' shadows linger as dark blocks)."));

	// The in-game sound tour (snd) sets this so every step sounds like one surface wherever you walk.
	TAutoConsoleVariable<int32> CVarStepSurface(
		TEXT("ygg.Snow.StepSurface"),
		0,
		TEXT("Footstep sound surface: 0 = the real ground, 1 = snow, 2 = ice, 3 = rubble, 4 = ladder."));

	TAutoConsoleVariable<int32> CVarSnowKick(
		TEXT("ygg.Snow.Kick"),
		1,
		TEXT("Snow kicked up by each step: 0 = none, 1 = solid clumps off the boot (2026-11-13), 2 = the old powder puff (Niagara sprites)."));

	TAutoConsoleVariable<float> CVarAutoWalk(
		TEXT("ygg.Snow.AutoWalk"),
		0.f,
		TEXT("Testing: >0 = the player walks straight ahead by itself at this input strength (0.5 walk, 1 jog)."));

	TAutoConsoleVariable<float> CVarAutoYaw(
		TEXT("ygg.Snow.AutoYaw"),
		-999.f,
		TEXT("Testing, with ygg.Snow.AutoWalk: the heading (world yaw, degrees) to walk; -999 = keep the current one."));

	TAutoConsoleVariable<float> CVarAutoPitch(
		TEXT("ygg.Snow.AutoPitch"),
		-999.f,
		TEXT("Testing: holds the view pitch (degrees, e.g. -60 to look at your feet), walking or not; -999 = off. With ygg.Snow.AutoYaw the heading is held too."));

	TAutoConsoleVariable<int32> CVarStepLog(
		TEXT("ygg.Snow.StepLog"),
		0,
		TEXT("1 = log every footstep: which foot, its height and how fast it still moved (share of the walking speed)."));

	/** A finished trail patch readback (Lock()ed staging texture) -> R, G per texel. False for a format it does not know. */
	bool SnowTrailDecodePatch(const uint8* Data, int32 PitchPixels, int32 Rows, int32 Size, EPixelFormat Format, TArray<FVector2f>& Out)
	{
		int32 Channels = 0;
		bool bHalf = true;
		switch (Format)
		{
		case PF_R16F: Channels = 1; break;
		case PF_G16R16F: Channels = 2; break;
		case PF_FloatRGBA: Channels = 4; break;
		case PF_R32_FLOAT: Channels = 1; bHalf = false; break;
		case PF_G32R32F: Channels = 2; bHalf = false; break;
		case PF_A32B32G32R32F: Channels = 4; bHalf = false; break;
		default: return false;
		}
		const int32 Bytes = GPixelFormats[Format].BlockBytes;
		if (!Data || Size <= 0 || PitchPixels < Size || (Rows > 0 && Rows < Size) || Bytes <= 0) { return false; }
		Out.SetNumUninitialized(Size * Size);
		for (int32 y = 0; y < Size; ++y)
		{
			const uint8* Row = Data + SIZE_T(y) * SIZE_T(PitchPixels) * SIZE_T(Bytes);
			for (int32 x = 0; x < Size; ++x)
			{
				const uint8* Px = Row + SIZE_T(x) * SIZE_T(Bytes);
				float R, G;
				if (bHalf)
				{
					const FFloat16* H = reinterpret_cast<const FFloat16*>(Px);
					R = H[0].GetFloat();
					G = Channels > 1 ? H[1].GetFloat() : R;
				}
				else
				{
					const float* F = reinterpret_cast<const float*>(Px);
					R = F[0];
					G = Channels > 1 ? F[1] : R;
				}
				Out[y * Size + x] = FVector2f(R, G);
			}
		}
		return true;
	}
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

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> ClearFinder(TEXT("/Engine/EngineMaterials/Widget3DPassThrough_Translucent.Widget3DPassThrough_Translucent"));
	RefresherMesh = CubeFinder.Object;
	RefresherMaterial = ClearFinder.Object;
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
	// the first step's snow clumps and the glacier outlines are loaded now, with the level (2026-11-14: loading them at
	// the first step - KickClumps, IsGlacierHard - made the game thread wait for the disk)
	if (!ClumpMesh)
	{
		ClumpMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Environment/Snow/SM_SnowClump.SM_SnowClump"));
	}
	SnowTrailWarmGlacierMask();
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

		if (RefresherMesh && RefresherMaterial)
		{
			// Never seen: no main pass, no depth, see-through material (draws nothing into the shadow map either), but it
			// counts as a shadow caster, which is what makes the cache redraw the pages it moves over.
			ShadowRefresher = NewObject<UStaticMeshComponent>(GetOwner(), TEXT("TrailShadowRefresher"));
			ShadowRefresher->SetUsingAbsoluteLocation(true);
			ShadowRefresher->SetUsingAbsoluteRotation(true);
			ShadowRefresher->SetUsingAbsoluteScale(true);
			ShadowRefresher->SetMobility(EComponentMobility::Movable);
			ShadowRefresher->SetStaticMesh(RefresherMesh);
			ShadowRefresher->SetMaterial(0, RefresherMaterial);
			ShadowRefresher->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			ShadowRefresher->SetGenerateOverlapEvents(false);
			ShadowRefresher->SetCanEverAffectNavigation(false);
			ShadowRefresher->CastShadow = true;
			ShadowRefresher->bCastDynamicShadow = true;
			ShadowRefresher->bRenderInMainPass = false;
			ShadowRefresher->bRenderInDepthPass = false;
			ShadowRefresher->bReceivesDecals = false;
			ShadowRefresher->bVisibleInReflectionCaptures = false;
			ShadowRefresher->bVisibleInRealTimeSkyCaptures = false;
			ShadowRefresher->bVisibleInRayTracing = false;
			ShadowRefresher->bAffectDistanceFieldLighting = false;
			ShadowRefresher->bAffectDynamicIndirectLighting = false;
			ShadowRefresher->SetWorldLocation(GetOwner()->GetActorLocation() - FVector(0.f, 0.f, 100000.f));
			ShadowRefresher->SetMobility(EComponentMobility::Static);
			ShadowRefresher->RegisterComponent();
		}
	}
}

void USnowTrailComponent::RefreshTrailShadows()
{
	if (!ShadowRefresher)
	{
		return;
	}
	const bool bOn = CVarShadowRefresh.GetValueOnGameThread() != 0;
	if (ShadowRefresher->IsVisible() != bOn)
	{
		ShadowRefresher->SetVisibility(bOn);
	}
	if (!bOn)
	{
		return;
	}
	// 8 x 8 cells over the trail area (6.4 m each, 3.2 m on High footprints), one a frame: all of it every 64 frames.
	constexpr int32 Cells = 8;
	const int32 Cell = RefreshCell;
	RefreshCell = (RefreshCell + 1) % (Cells * Cells);
	const float Side = TrailAreaCm / float(Cells);
	const FVector2D Centre = TrailCentre + FVector2D((float(Cell % Cells) + 0.5f) * Side, (float(Cell / Cells) + 0.5f) * Side) - FVector2D(TrailAreaCm * 0.5f);

	// The box only needs to span the snow surface in that cell (pits go 18 cm down, banks 7 cm up; the drawn surface
	// can sit a little off the collision): the ground's height range from five traces, plus a margin.
	const double Z = GetOwner()->GetActorLocation().Z;
	double Lo = TNumericLimits<double>::Max();
	double Hi = TNumericLimits<double>::Lowest();
	FCollisionQueryParams Params(SCENE_QUERY_STAT(TrailShadowRefresh), false, GetOwner());
	static const FVector2D Probes[5] = { FVector2D(0.0, 0.0), FVector2D(-0.5, -0.5), FVector2D(0.5, -0.5), FVector2D(-0.5, 0.5), FVector2D(0.5, 0.5) };
	for (const FVector2D& Probe : Probes)
	{
		const FVector At(Centre + Probe * Side, Z);
		FHitResult Hit;
		if (GetWorld()->LineTraceSingleByChannel(Hit, At + FVector(0.f, 0.f, 5000.f), At - FVector(0.f, 0.f, 5000.f), ECC_Visibility, Params))
		{
			Lo = FMath::Min(Lo, Hit.ImpactPoint.Z);
			Hi = FMath::Max(Hi, Hit.ImpactPoint.Z);
		}
	}
	if (Lo > Hi)
	{
		return;   // no ground under this cell within 50 m: nothing to refresh
	}
	Lo -= 60.0;
	Hi += 40.0;
	// The terrain is static, so its shadow sits in the cache's STATIC pages, and an invalidation only reaches the pages
	// of the kind its primitive is cached as (VirtualShadowMapCacheInvalidation.ush: static -> INVALIDATE_STATIC, moving ->
	// INVALIDATE_DYNAMIC). A moving box therefore never touched the pits' shadows (the first fix, play-test 2026-11-14).
	// So the box hops as a STATIC primitive: removed and added again at the next cell (both invalidate their bounds).
	// The engine cube is 100 cm, centred.
	ShadowRefresher->UnregisterComponent();
	ShadowRefresher->SetMobility(EComponentMobility::Movable);
	ShadowRefresher->SetWorldTransform(FTransform(FQuat::Identity, FVector(Centre, 0.5 * (Lo + Hi)),
		FVector(Side * 1.1f / 100.f, Side * 1.1f / 100.f, float(Hi - Lo) / 100.f)));
	ShadowRefresher->SetMobility(EComponentMobility::Static);
	ShadowRefresher->RegisterComponent();
}

void USnowTrailComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// The trail area lives in a shared parameter collection: switch it off so the editor shows untouched snow.
	if (bTrailReady)
	{
		PublishTrailArea(false);
	}
	for (FTrailPatchSlot& S : TrailPatches) { ReleaseTrailPatch(S.Patch); }
	TrailPatches.Reset();
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
			++TrailMapEpoch;   // read-back patches of the old map no longer describe it
		}
		AppliedQuality = FootprintQuality;
	}
	const bool bEnabled = CVarSnowDeform.GetValueOnGameThread() != 0 && FootprintQuality > 0;
	PublishTrailArea(bEnabled);
	// keeps going a few seconds after the trail is switched off, so the pits' shadows go with the pits
	ShadowRefreshLinger = bEnabled ? 3.f : ShadowRefreshLinger - DeltaTime;
	if (ShadowRefreshLinger > 0.f)
	{
		RefreshTrailShadows();
	}
	if (!bEnabled)
	{
		OtherPrints.Reset();
		return;
	}

	const FVector2D Player(GetOwner()->GetActorLocation());
	if (FVector2D::Distance(Player, TrailCentre) > RecentreDistanceCm)
	{
		RecentreTrailMap(Player);
		PublishTrailArea(true);
	}
	FillTrailMap(DeltaTime);

	// Only feet in snow press into it; deep powder all the way, hard crust just a little. Other people's prints
	// (StampOtherFoot) are drawn whether or not the player stands in snow.
	// Each foot is asked about its own ground (2026-11-20, #69): the body's ground alone let a foot on rock or rubble
	// beside snow stamp, and the easing sink kept both feet stamping ~1 s after stepping onto moraine.
	const bool bOwnFeet = (GroundSoftness > 0.f || CurrentSink > 0.5f) && (Feet[0].bOnSnow || Feet[1].bOnSnow);
	if (!bOwnFeet && OtherPrints.Num() == 0)
	{
		SoftenTrailMap();
		return;
	}
	float Depth = FMath::Max(GroundSoftness, 0.15f);
	if (!bOwnFeet)
	{
		// the boot instances carry one depth per frame (canvas batching): the deepest waiting print sets it
		Depth = 0.15f;
		for (const FOtherPrint& O : OtherPrints) { Depth = FMath::Max(Depth, O.Depth); }
	}
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
	for (int32 i = OtherPrints.Num() - 1; i >= 0; --i)
	{
		FOtherPrint& O = OtherPrints[i];
		const FVector2D Pixel = (O.At - TrailCentre) * PixelsPerCm + CanvasSize * 0.5f;
		Canvas->K2_DrawMaterial(BootInstances[O.bLeft ? 1 : 0], Pixel - BootSize * 0.5f, BootSize, FVector2D::ZeroVector, FVector2D::UnitVector, O.Yaw, FVector2D(0.5f, 0.5f));
		if (--O.Frames <= 0) { OtherPrints.RemoveAtSwap(i); }
	}
	for (const FFootState& Foot : Feet)
	{
		if (!bOwnFeet) { break; }
		const FVector FootLocation = Body->GetSocketLocation(Foot.Bone);
		// not snow under this boot, or not measured here (standing still after a teleport): no print, no channel (2026-11-20)
		if (!Foot.bOnSnow || FVector::DistSquared2D(FootLocation, Foot.SnowCheckedAt) > FMath::Square(50.0))
		{
			continue;
		}
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

bool USnowTrailComponent::StampOtherFoot(const FVector& FootLocation, const FVector& Forward, bool bLeft, const AActor* Who)
{
	if (!bTrailReady || CVarSnowDeform.GetValueOnGameThread() == 0 || FootprintQuality <= 0 || OtherPrints.Num() >= 48)
	{
		return false;
	}
	const FVector2D Sole = FVector2D(FootLocation + Forward * SoleForwardCm);
	if (FMath::Abs(Sole.X - TrailCentre.X) > TrailAreaCm * 0.5f || FMath::Abs(Sole.Y - TrailCentre.Y) > TrailAreaCm * 0.5f)
	{
		return false;
	}
	FCollisionQueryParams Params(SCENE_QUERY_STAT(SnowOtherFoot), false, Who);
	Params.AddIgnoredActor(GetOwner());
	Params.bReturnPhysicalMaterial = true;
	Params.bReturnFaceIndex = true;   // rock faces of the detail tiles are known by their index (IsRock)
	FHitResult Hit;
	if (!GetWorld()->LineTraceSingleByChannel(Hit, FootLocation + FVector(0.f, 0.f, 60.f), FootLocation - FVector(0.f, 0.f, 80.f), ECC_Visibility, Params)
		|| !IsSnow(Hit))
	{
		return false;
	}
	FOtherPrint O;
	O.At = Sole;
	O.Yaw = FMath::RadiansToDegrees(FMath::Atan2(Forward.Y, Forward.X));
	O.Depth = FMath::Max(SnowSoftness(Hit.ImpactPoint, Hit.ImpactNormal), 0.15f);
	O.Frames = 3;
	O.bLeft = bLeft;
	OtherPrints.Add(O);
	return true;
}

void USnowTrailComponent::SoftenTrailMap()
{
	// Sharp boot stamps on 2.5 cm texels turned into stair-steps when displaced directly; the landscape reads this
	// blurred copy instead (round walls, plus a wide spread for the banks). One full-screen pass, ~0.05 ms.
	UKismetRenderingLibrary::DrawMaterialToRenderTarget(this, TrailSoftMap, BlurInstance);
}

// ---- the drawn trench on the CPU (#84): small patches of TrailSoftMap read back from the GPU

float USnowTrailComponent::SinkDepthFor(const FHitResult& Ground) const
{
	// the rule of UpdateSinking, for any ground (story people sink like the player)
	if (!IsSnow(Ground))
	{
		return 0.f;
	}
	const AActor* Owner = GetOwner();
	const USurvivalComponent* Survival = Owner ? Owner->FindComponentByClass<USurvivalComponent>() : nullptr;
	const bool bInCamp = Survival && Survival->GetSnapshot().bSheltered;
	return MaxSinkCm * SnowSoftness(Ground.ImpactPoint, Ground.ImpactNormal) * (bInCamp ? CampSinkMultiplier : 1.f);
}

void USnowTrailComponent::RequestTrailPatch(const UObject* Who, const FVector& Location)
{
	if (!Who || !bTrailReady || !GetWorld())
	{
		return;
	}
	const double Now = GetWorld()->GetRealTimeSeconds();
	FTrailPatchSlot* Slot = TrailPatches.FindByPredicate([Who](const FTrailPatchSlot& S) { return S.Who.Get() == Who; });
	if (!Slot)
	{
		if (TrailPatches.Num() >= MaxTrailPatches)
		{
			// full: the asker forgotten longest ago gives its place up (TickTrailPatches drops those after 2 s anyway)
			int32 Old = INDEX_NONE;
			for (int32 i = 0; i < TrailPatches.Num(); ++i)
			{
				if (Now - TrailPatches[i].AskedAt > 1.0 && (Old == INDEX_NONE || TrailPatches[i].AskedAt < TrailPatches[Old].AskedAt)) { Old = i; }
			}
			if (Old == INDEX_NONE) { return; }
			ReleaseTrailPatch(TrailPatches[Old].Patch);
			TrailPatches.RemoveAtSwap(Old);
		}
		Slot = &TrailPatches.AddDefaulted_GetRef();
		Slot->Who = Who;
	}
	Slot->AskedFor = FVector2D(Location);
	Slot->AskedAt = Now;
}

void USnowTrailComponent::ReleaseTrailPatch(TSharedPtr<FTrailPatch, ESPMode::ThreadSafe>& Patch)
{
	if (!Patch.IsValid())
	{
		return;
	}
	// the readback (an RHI staging texture and fence) goes on the render thread, after every copy and poll queued for it
	TSharedPtr<FTrailPatch, ESPMode::ThreadSafe> Keep = MoveTemp(Patch);
	Patch.Reset();
	ENQUEUE_RENDER_COMMAND(YggTrailPatchRelease)([Keep](FRHICommandListImmediate& RHICmdList)
	{
		Keep->Readback.Reset();
	});
}

void USnowTrailComponent::TickTrailPatches()
{
	UWorld* World = GetWorld();
	if (TrailPatches.Num() == 0 || !World || !TrailSoftMap)
	{
		return;
	}
	const double Now = World->GetRealTimeSeconds();
	const bool bOn = CVarSnowDeform.GetValueOnGameThread() != 0 && FootprintQuality > 0;
	const int32 MapSize = TrailSoftMap->SizeX;
	FTextureRenderTargetResource* Resource = TrailSoftMap->GameThread_GetRenderTargetResource();
	if (MapSize < 32 || TrailSoftMap->SizeY != MapSize || !Resource)
	{
		return;
	}
	const float Texel = TrailAreaCm / float(MapSize);
	// ~2.4 m square: both boots of a walking person, with room to walk on for a few reads (96 texels at 2.5 cm, 192 at 1.25)
	const int32 Want = FMath::Clamp(FMath::DivideAndRoundUp(FMath::CeilToInt(240.f / Texel), 16) * 16, 32, FMath::Min(256, MapSize));
	int32 Sends = 0;
	for (int32 i = TrailPatches.Num() - 1; i >= 0; --i)
	{
		FTrailPatchSlot& S = TrailPatches[i];
		if (!S.Who.IsValid() || Now - S.AskedAt > 2.0)
		{
			ReleaseTrailPatch(S.Patch);
			TrailPatches.RemoveAtSwap(i);
			continue;
		}
		if (!S.Patch.IsValid() || S.Patch->Size != Want)
		{
			ReleaseTrailPatch(S.Patch);
			S.Patch = MakeShared<FTrailPatch, ESPMode::ThreadSafe>();
			S.Patch->Readback = MakeUnique<FRHIGPUTextureReadback>(TEXT("YggTrailPatch"));
			S.Patch->Size = Want;
		}
		TSharedPtr<FTrailPatch, ESPMode::ThreadSafe> Keep = S.Patch;
		FTrailPatch& P = *Keep;
		if (P.bInFlight)
		{
			if (P.bFresh.load())
			{
				// the render thread is done with Pending: take it over with the mapping the copy was sent with
				P.Values = MoveTemp(P.Pending);
				P.Pending.Reset();
				P.ValuesOrigin = P.Origin;
				P.ValuesCentre = P.Centre;
				P.ValuesArea = P.Area;
				P.ValuesMapSize = P.MapSize;
				P.ValuesEpoch = P.Epoch;
				P.ValuesAt = Now;
				P.bFresh = false;
				P.bInFlight = false;
			}
			else if (Now - P.SentAt > 1.0)
			{
				// never came back (no fence on this RHI, a format it cannot decode): a new readback next frame
				static bool bWarned = false;
				if (!bWarned) { bWarned = true; UE_LOG(LogTemp, Warning, TEXT("SnowTrail: a trail patch readback did not come back within 1 s - retrying (story people's trench height unknown meanwhile)")); }
				ReleaseTrailPatch(S.Patch);
				continue;
			}
			else if (!P.bPollQueued.load())
			{
				P.bPollQueued = true;
				ENQUEUE_RENDER_COMMAND(YggTrailPatchPoll)([Keep](FRHICommandListImmediate& RHICmdList)
				{
					FRHIGPUTextureReadback* R = Keep->Readback.Get();
					if (R && R->IsReady())
					{
						int32 Pitch = 0, Rows = 0;
						const uint8* Data = static_cast<const uint8*>(R->Lock(Pitch, &Rows));
						if (Data)
						{
							const bool bOk = SnowTrailDecodePatch(Data, Pitch, Rows, Keep->Size, Keep->Format, Keep->Pending);
							R->Unlock();
							if (bOk) { Keep->bFresh = true; }
						}
					}
					Keep->bPollQueued = false;
				});
			}
		}
		if (P.bInFlight || !bOn || Sends >= 3)
		{
			continue;
		}
		// a place outside the drawn area (the material fades the trench out by 48% of the side) needs no patch
		const FVector2D UV = (S.AskedFor - TrailCentre) / double(TrailAreaCm) + FVector2D(0.5, 0.5);
		if (FMath::Max(FMath::Abs(UV.X - 0.5), FMath::Abs(UV.Y - 0.5)) > 0.48)
		{
			continue;
		}
		// due every TrailPatchEverySec, or at once when the asker nears the edge of the last read (70 cm margin)
		bool bCovered = false;
		if (P.Values.Num() == P.Size * P.Size && P.ValuesEpoch == TrailMapEpoch && P.ValuesMapSize > 0)
		{
			const double T = double(P.ValuesArea) / double(P.ValuesMapSize);
			const double Px = (S.AskedFor.X - P.ValuesCentre.X) / T + P.ValuesMapSize * 0.5 - 0.5 - P.ValuesOrigin.X;
			const double Py = (S.AskedFor.Y - P.ValuesCentre.Y) / T + P.ValuesMapSize * 0.5 - 0.5 - P.ValuesOrigin.Y;
			const double M = 70.0 / T;
			bCovered = Px >= M && Py >= M && Px <= P.Size - 1 - M && Py <= P.Size - 1 - M;
		}
		if (bCovered && Now - P.SentAt < TrailPatchEverySec)
		{
			continue;
		}
		// texel centres sit at i + 0.5 (the material's uv = (xy - centre) / side + 0.5)
		const double Cx = (S.AskedFor.X - TrailCentre.X) / Texel + MapSize * 0.5;
		const double Cy = (S.AskedFor.Y - TrailCentre.Y) / Texel + MapSize * 0.5;
		P.Origin = FIntPoint(FMath::Clamp(FMath::RoundToInt32(Cx) - Want / 2, 0, MapSize - Want), FMath::Clamp(FMath::RoundToInt32(Cy) - Want / 2, 0, MapSize - Want));
		P.Centre = TrailCentre;
		P.Area = TrailAreaCm;
		P.MapSize = MapSize;
		P.Epoch = TrailMapEpoch;
		P.SentAt = Now;
		P.bInFlight = true;
		P.bFresh = false;
		const FIntPoint O = P.Origin;
		const int32 N = P.Size;
		ENQUEUE_RENDER_COMMAND(YggTrailPatchCopy)([Keep, Resource, O, N](FRHICommandListImmediate& RHICmdList)
		{
			FRHITexture* Tex = Resource->GetRenderTargetTexture().GetReference();
			if (!Tex || !Keep->Readback.IsValid())
			{
				return;   // never ready: the game thread retries after 1 s
			}
			Keep->Format = Tex->GetFormat();
			// RDG knows the map's current state (written by the canvas, read by the landscape) and moves it to a copy source
			FRDGBuilder GraphBuilder(RHICmdList);
			const FRDGTextureRef Source = RegisterExternalTexture(GraphBuilder, Tex, TEXT("YggTrailPatchSource"));
			AddEnqueueCopyPass(GraphBuilder, Keep->Readback.Get(), Source, FResolveRect(O.X, O.Y, O.X + N, O.Y + N));
			GraphBuilder.Execute();
		});
		++Sends;
	}
}

float USnowTrailComponent::TrailOffsetCm(const UObject* Who, const FVector& Location, bool* bOutKnown) const
{
	if (bOutKnown) { *bOutKnown = false; }
	const UWorld* World = GetWorld();
	if (!bTrailReady || !World)
	{
		return 0.f;
	}
	// the trail off: the material draws no trench at all
	if (CVarSnowDeform.GetValueOnGameThread() == 0 || FootprintQuality <= 0)
	{
		if (bOutKnown) { *bOutKnown = true; }
		return 0.f;
	}
	// the material's fade towards the edge of the area it reads now (M_EverestLandscape_TrailDisp)
	const FVector2D UV = (FVector2D(Location) - TrailCentre) / double(TrailAreaCm) + FVector2D(0.5, 0.5);
	const float Edge = float(FMath::Max(FMath::Abs(UV.X - 0.5), FMath::Abs(UV.Y - 0.5)));
	const float Fade = 1.f - FMath::SmoothStep(0.40f, 0.48f, Edge);
	if (Fade <= 0.f)
	{
		if (bOutKnown) { *bOutKnown = true; }
		return 0.f;
	}
	const double Now = World->GetRealTimeSeconds();
	// bilinear like the material's sampler; the patch's own mapping (a recentre since moved the map's content with the world)
	auto Sample = [&](const FTrailPatch& P, FVector2f& Out) -> bool
	{
		if (P.Values.Num() != P.Size * P.Size || P.Size < 2 || P.ValuesEpoch != TrailMapEpoch || Now - P.ValuesAt > 1.0 || P.ValuesMapSize <= 0)
		{
			return false;
		}
		const double T = double(P.ValuesArea) / double(P.ValuesMapSize);
		const double Px = (Location.X - P.ValuesCentre.X) / T + P.ValuesMapSize * 0.5 - 0.5 - P.ValuesOrigin.X;
		const double Py = (Location.Y - P.ValuesCentre.Y) / T + P.ValuesMapSize * 0.5 - 0.5 - P.ValuesOrigin.Y;
		if (Px < 0.0 || Py < 0.0 || Px > P.Size - 1.0 || Py > P.Size - 1.0)
		{
			return false;
		}
		const int32 X0 = FMath::Min(FMath::FloorToInt32(Px), P.Size - 2);
		const int32 Y0 = FMath::Min(FMath::FloorToInt32(Py), P.Size - 2);
		const float Fx = float(Px - X0), Fy = float(Py - Y0);
		const FVector2f A = FMath::Lerp(P.Values[Y0 * P.Size + X0], P.Values[Y0 * P.Size + X0 + 1], Fx);
		const FVector2f B = FMath::Lerp(P.Values[(Y0 + 1) * P.Size + X0], P.Values[(Y0 + 1) * P.Size + X0 + 1], Fx);
		Out = FMath::Lerp(A, B, Fy);
		return true;
	};
	FVector2f RG;
	bool bFound = false;
	if (const FTrailPatchSlot* Own = TrailPatches.FindByPredicate([Who](const FTrailPatchSlot& S) { return Who && S.Who.Get() == Who; }))
	{
		bFound = Own->Patch.IsValid() && Sample(*Own->Patch, RG);
	}
	for (int32 i = 0; !bFound && i < TrailPatches.Num(); ++i)
	{
		bFound = TrailPatches[i].Patch.IsValid() && Sample(*TrailPatches[i].Patch, RG);
	}
	if (!bFound)
	{
		return 0.f;
	}
	if (bOutKnown) { *bOutKnown = true; }
	// TrailHelpers::Height: a scooped hole up to 18 cm (x 0.93 + 0.07 noise) and soft banks up to 7 cm (x 0.75 + 0.5 noise);
	// the value noise is taken at its mean (0.5): +-0.6 cm in the hole, +-1.8 cm on a bank
	const float R = FMath::Clamp(RG.X, 0.f, 1.f);
	const float D = R * R * (3.f - 2.f * R);
	float Bank = FMath::Clamp(RG.Y * 1.5f - R * 1.3f, 0.f, 1.f);
	Bank = Bank * Bank * (3.f - 2.f * Bank);
	return (-18.f * D * 0.965f + Bank * 7.f) * Fade;
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

	TickClumps(DeltaTime);

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
	// notify steps whose boot has landed now (the crunch is heard): the step bob listens
	if (PendingStepEvents.Num() > 0)
	{
		const double Now = GetWorld()->GetTimeSeconds();
		for (int32 i = 0; i < PendingStepEvents.Num(); )
		{
			if (PendingStepEvents[i].At <= Now)
			{
				const FPendingStepEvent Event = PendingStepEvents[i];
				PendingStepEvents.RemoveAt(i);   // in order: left and right never swap
				BroadcastStep(Event.bLeft, Event.Surface);
			}
			else
			{
				++i;
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
	if (const float Pitch = CVarAutoPitch.GetValueOnGameThread(); Pitch > -900.f)
	{
		if (const APawn* Pawn = Cast<APawn>(GetOwner()); Pawn && Pawn->GetController())
		{
			const float Yaw = CVarAutoYaw.GetValueOnGameThread();
			const FRotator Now = Pawn->GetController()->GetControlRotation();
			Pawn->GetController()->SetControlRotation(FRotator(Pitch, Yaw > -900.f ? Yaw : Now.Yaw, 0.f));
		}
	}

	UpdateSinking(DeltaTime);
	if (bTrailReady)
	{
		UpdateTrailMap(DeltaTime);
		// after this frame's trail drawing is queued, so a copy sent now reads it
		TickTrailPatches();
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
	// cause not found yet). If no foot has planted for over a step length and a third (~80 cm a step), play the step anyway
	// and say so in the log with the foot heights. (2026-11-20, #46: it waited 2.2 step lengths, so a walk whose plant
	// detection was stuck got one step in every 2.2 - sparse. The cause, a stuck lowest foot height, is fixed in UpdateFoot;
	// the third of slack keeps the net from doubling real steps that are a little longer than 80 cm.)
	TimeSinceStep += DeltaTime;
	const float StepEvery = FMath::Clamp(80.f / FMath::Max(float(Velocity.Size2D()), 1.f), 0.25f, 0.9f);
	if (TimeSinceStep > StepEvery * 1.35f)
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
			PlayFootstep(Hit, !bLastStepLeft);   // no foot known here: the other one from the last step
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
		// nothing under the landing spot (a gap in the collision, a ray past a ledge): the ground under the body instead,
		// so the contact still sounds (2026-11-20, #46: a missed ray dropped the step and left it to the cadence net)
		const FVector Centre = Character->GetActorLocation();
		if (!World->LineTraceSingleByChannel(Hit, Centre, Centre - FVector(0.f, 0.f, Character->GetSimpleCollisionHalfHeight() + 60.f), ECC_Visibility, Params))
		{
			return;
		}
		if (CVarStepLog.GetValueOnGameThread() != 0)
		{
			UE_LOG(LogTemp, Display, TEXT("Contact %s: no ground under the landing spot, used the ground under the body"), bLeft ? TEXT("L") : TEXT("R"));
		}
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
	// The step itself is when the boot lands, StepNotifyLead from now, on every surface: snow's crunch peaks there and the
	// hard ground's clips hit there (their delay above is that lead minus each clip's onset).
	PendingStepEvents.Add({ Now + StepNotifyLead, bLeft, SurfaceOf(Set) });
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
		Foot.bOnSnow = false;
		return;
	}
	// this foot's own ground decides its print and trench (UpdateTrailMap), from the same trace (2026-11-20, #69)
	Foot.bOnSnow = IsSnow(Hit);
	Foot.SnowCheckedAt = FootLocation;
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
	// Kept over two windows of about one foot cycle each, so an odd height is forgotten within two windows (2026-11-20,
	// #46: the old lowest crept back up at only 3 cm/s, so one odd trace of -45 cm - the ray hit something above the
	// boot - kept the plant line out of reach for ~10 s and walking steps went silent; logs showed low -45 at height -10..+3).
	const float Window = FMath::Clamp(120.f / WalkSpeed, 0.5f, 2.f);   // 0.8 s at a 150 cm/s walk (a foot cycle ~1.1 s)
	Foot.WindowLow = FMath::Min(Foot.WindowLow, Height);
	Foot.WindowHigh = FMath::Max(Foot.WindowHigh, Height);
	Foot.WindowAge += DeltaTime;
	if (Foot.WindowAge > Window)
	{
		Foot.PrevLow = Foot.WindowLow;
		Foot.PrevHigh = Foot.WindowHigh;
		Foot.WindowLow = Foot.WindowHigh = Height;
		Foot.WindowAge = 0.f;
	}
	Foot.Lowest = FMath::Min(Foot.WindowLow, Foot.PrevLow);
	Foot.Highest = FMath::Max(Foot.WindowHigh, Foot.PrevHigh);
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
				Foot.bLeft ? TEXT("L") : TEXT("R"), Height, Foot.Lowest, Foot.Highest, Foot.SpeedRatio, Foot.LowSeconds * 1000.f,
				IsRock(Hit) ? TEXT("rock") : Foot.bOnSnow ? TEXT("snow") : TEXT("other (no print)"),
				GetOwner()->GetVelocity().Size2D(), *Hit.ImpactPoint.ToCompactString());
		}
		Foot.bPlanted = true;
		Foot.LowSeconds = 0.f;
		if (!NotifiesActive())
		{
			PlayFootstep(Hit, Foot.bLeft);   // fallback for clips without foot-contact notifies
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

	void SnowTrailWarmGlacierMask()
	{
		if (!GlacierMask.bLoaded)
		{
			GlacierMask.Load();
		}
	}
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

void USnowTrailComponent::PlayFootstep(const FHitResult& Hit, bool bLeft)
{
	// the fallback paths (foot height, cadence net): no notify lead to make up for, so everything plays at once
	TimeSinceStep = 0.f;
	float Delay = 0.f;
	if (const TArray<TObjectPtr<USoundBase>>* Set = PickStepSet(Hit, Delay))
	{
		PlayStepFrom(*Set, Hit.ImpactPoint);
		BroadcastStep(bLeft, SurfaceOf(Set));
	}
}

uint8 USnowTrailComponent::SurfaceOf(const TArray<TObjectPtr<USoundBase>>* Set) const
{
	return Set == &RubbleSteps ? EYggStepSurface::Rock
		: Set == &IceSteps ? EYggStepSurface::Ice
		: Set == &LadderSteps ? EYggStepSurface::Ladder
		: EYggStepSurface::Snow;
}

void USnowTrailComponent::BroadcastStep(bool bLeft, uint8 Surface)
{
	bLastStepLeft = bLeft;
	const AActor* Owner = GetOwner();
	const float Speed = Owner ? float(Owner->GetVelocity().Size2D()) : 0.f;
	OnStep.Broadcast(bLeft, Speed, Surface);
	// the step bob (Chapter1_Camera_Spec.md 5.3): fed straight to the rig, which exists only on the locally controlled pawn;
	// checked to be this owner's, so another walker's steps can never move the player's eye, and skipped when the rig
	// listens to OnStep itself (one step, one bob)
	const APawn* Pawn = Cast<APawn>(Owner);
	if (Pawn && Pawn->IsLocallyControlled())
	{
		if (UYggViewRigComponent* Rig = UYggViewRigComponent::Find(this); Rig && Rig->GetOwner() == Owner && !OnStep.IsBoundToObject(Rig))
		{
			Rig->OnStep(bLeft, Speed, Surface);
		}
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

namespace
{
	/** A route tile's collision sections as IsRock reads them: X = the first face after the section, Y = its material slot
	 *  (0 snow, 1 rock). Only collision-enabled sections count, as in the face index of a trace. */
	void SnowTrailReadSections(const UStaticMesh* Col, TArray<FIntPoint>& Out)
	{
		Out.Reset();
		if (!Col || !Col->GetRenderData() || Col->GetRenderData()->LODResources.Num() == 0)
		{
			return;
		}
		int32 Faces = 0;
		for (const FStaticMeshSection& Section : Col->GetRenderData()->LODResources[0].Sections)
		{
			if (!Section.bEnableCollision) { continue; }
			Faces += Section.NumTriangles;
			Out.Emplace(Faces, Section.MaterialIndex);
		}
	}

	bool SnowTrailRockFace(const TArray<FIntPoint>& Sections, int32 FaceIndex)
	{
		for (const FIntPoint& S : Sections)
		{
			if (FaceIndex < S.X) { return S.Y == 1; }
		}
		return false;
	}

	/** Packaged game: the section table of Tile's <name>_col mesh, or null while it is still loading. The first call for a
	 *  tile starts a background load (LoadPackageAsync: the game thread never waits for it); when it is in, the sections
	 *  are read and the mesh itself is let go (garbage collection may drop it, the table stays). A missing _col gives an
	 *  empty table: no rock on that tile, as before. */
	const TArray<FIntPoint>* SnowTrailColSections(const UStaticMesh* Tile)
	{
		struct FEntry
		{
			TArray<FIntPoint> Sections;
			bool bReady = false;
		};
		static TMap<TWeakObjectPtr<const UStaticMesh>, FEntry> Known;
		const TWeakObjectPtr<const UStaticMesh> Key(Tile);
		if (const FEntry* Entry = Known.Find(Key))
		{
			return Entry->bReady ? &Entry->Sections : nullptr;
		}
		FEntry& Added = Known.Add(Key);
		// only route tiles have a _col beside them: any other mesh (tents, props, the editor binary's every non-tile mesh -
		// IsRock is asked about all of them by the feet, the climb and the body probes) is never rock here, as before,
		// without asking the loader for a package that does not exist
		if (!Tile->GetPackage()->GetFName().ToString().StartsWith(TEXT("/Game/Environment/DetailTiles/")))
		{
			Added.bReady = true;
			return &Added.Sections;
		}
		const FString ColName = Tile->GetName() + TEXT("_col");
		LoadPackageAsync(Tile->GetOutermost()->GetName() + TEXT("_col"), FLoadPackageAsyncDelegate::CreateLambda(
			[Key, ColName](const FName& PackageName, UPackage* Package, EAsyncLoadingResult::Type Result)
			{
				if (FEntry* Entry = Known.Find(Key))
				{
					const UStaticMesh* Col = (Result == EAsyncLoadingResult::Succeeded && Package) ? FindObject<UStaticMesh>(Package, *ColName) : nullptr;
					SnowTrailReadSections(Col, Entry->Sections);
					Entry->bReady = true;
				}
			}));
		return nullptr;
	}

	/** True while a route tile's rock faces are not known yet (packaged game, its _col still loading): IsRock answers
	 *  "not rock" then, which made the first steps on a rocky tile count as snow (2026-11-20). */
	bool SnowTrailTileRockPending(const FHitResult& Hit)
	{
		const UStaticMeshComponent* Mesh = Cast<UStaticMeshComponent>(Hit.GetComponent());
		const UStaticMesh* Asset = Mesh ? Mesh->GetStaticMesh() : nullptr;
		if (!Asset)
		{
			return false;
		}
#if WITH_EDITORONLY_DATA
		if (Asset->ComplexCollisionMesh.Get())
		{
			return false;   // the editor reads the collision mesh directly (IsRock)
		}
#endif
		return SnowTrailColSections(Asset) == nullptr;
	}
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
		// A packaged game has no editor link to the collision mesh: it sits next to the tile as <name>_col. It is loaded in
		// the background the first time the tile is stepped on and only its section sizes are kept (2026-11-14: the
		// LoadObject here waited for the disk on the game thread at the first step on every tile, and the weakly held mesh
		// was dropped by the next garbage collection - from then on that tile's rock read as snow). Until it is in, steps
		// on that tile count as snow.
		const TArray<FIntPoint>* Sections = SnowTrailColSections(Asset);
		return Sections && Hit.FaceIndex >= 0 && SnowTrailRockFace(*Sections, Hit.FaceIndex);
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
	if (!IsTerrain(Hit) || IsRock(Hit) || IsGlacierHard(Hit.ImpactPoint))
	{
		return false;
	}
	// a tile whose rock faces are still loading is not known to be snow: no print, trench or sinking on it for that
	// moment (2026-11-20; the landscape is never pending: it is not a static mesh)
	return Cast<ALandscapeProxy>(Hit.GetActor()) != nullptr || !SnowTrailTileRockPending(Hit);
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

void USnowTrailComponent::KickClumps(const FVector& At, const FVector& Normal, const FVector& WalkDirection, float Softness)
{
	if (MaxClumps <= 0)
	{
		return;
	}
	if (!ClumpInstances)
	{
		if (!ClumpMesh)
		{
			ClumpMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Environment/Snow/SM_SnowClump.SM_SnowClump"));
		}
		if (!ClumpMesh)
		{
			return;
		}
		// world space: the component sits at the origin and never moves; its instances are the clumps
		ClumpInstances = NewObject<UInstancedStaticMeshComponent>(GetOwner(), TEXT("SnowClumps"));
		ClumpInstances->SetUsingAbsoluteLocation(true);
		ClumpInstances->SetUsingAbsoluteRotation(true);
		ClumpInstances->SetUsingAbsoluteScale(true);
		ClumpInstances->SetMobility(EComponentMobility::Movable);
		ClumpInstances->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		ClumpInstances->SetCastShadow(false);
		ClumpInstances->bVisibleInRayTracing = false;
		ClumpInstances->SetStaticMesh(ClumpMesh);
		ClumpInstances->RegisterComponent();
		ClumpInstances->SetWorldTransform(FTransform::Identity);
		// fixed slots, parked at zero size when not in use: updated in one batch per frame
		TArray<FTransform> Parked;
		Parked.Init(FTransform(FQuat::Identity, FVector::ZeroVector, FVector::ZeroVector), MaxClumps);
		ClumpInstances->AddInstances(Parked, false, true);
		ClumpSlots = MaxClumps;
	}
	// a step on soft powder flicks a handful off the toe; hard crust only a few crumbs
	const FVector Fwd = FVector::VectorPlaneProject(WalkDirection, Normal).GetSafeNormal();
	const FVector Side = FVector::CrossProduct(Normal, Fwd);
	const int32 N = FMath::RoundToInt(FMath::Lerp(3.f, 12.f, Softness) * FMath::FRandRange(0.7f, 1.2f));
	static int32 KickCount = 0;
	if ((KickCount++ % 10) == 0) { UE_LOG(LogTemp, Display, TEXT("SnowKick: step %d, %d clumps (softness %.2f), %d in the air"), KickCount, N, Softness, Clumps.Num()); }
	for (int32 i = 0; i < N; ++i)
	{
		if (Clumps.Num() >= ClumpSlots)
		{
			Clumps.RemoveAt(0);
		}
		FClump& C = Clumps.AddDefaulted_GetRef();
		C.PlaneAt = At;
		C.PlaneN = Normal;
		C.P = At + Fwd * FMath::FRandRange(6.f, 14.f) + Side * FMath::FRandRange(-5.f, 5.f) + Normal * 2.f;
		C.V = Fwd * FMath::FRandRange(60.f, 170.f) * FMath::Lerp(0.6f, 1.f, Softness) + Normal * FMath::FRandRange(60.f, 150.f) + Side * FMath::FRandRange(-45.f, 45.f);
		C.Size = FMath::Lerp(0.5f, 2.6f, FMath::Pow(FMath::FRand(), 2.2f)) * FMath::Lerp(0.8f, 1.15f, Softness);
		C.Squash = FVector(FMath::FRandRange(0.75f, 1.2f), FMath::FRandRange(0.75f, 1.2f), FMath::FRandRange(0.6f, 1.f));
		C.R = FQuat(FVector(FMath::FRand(), FMath::FRand(), FMath::FRand()).GetSafeNormal(), FMath::FRandRange(0.f, 6.28f));
		C.Spin = FVector(FMath::FRandRange(-12.f, 12.f), FMath::FRandRange(-12.f, 12.f), FMath::FRandRange(-12.f, 12.f));
	}
}

void USnowTrailComponent::TickClumps(float DeltaTime)
{
	if (!ClumpInstances || (Clumps.Num() == 0 && !bClumpsDirty))
	{
		return;
	}
	const float Dt = FMath::Min(DeltaTime, 0.05f);
	constexpr float MeltSeconds = 0.55f;
	for (int32 i = Clumps.Num() - 1; i >= 0; --i)
	{
		FClump& C = Clumps[i];
		if (C.Rest < 0.f)
		{
			C.V += FVector(0.f, 0.f, -980.f) * Dt;
			C.V *= FMath::Max(0.f, 1.f - 1.6f * Dt);                    // air drag on a loose clump
			C.P += C.V * Dt;
			C.R = FQuat(C.Spin.GetSafeNormal(), C.Spin.Size() * Dt) * C.R;
			// landed: on (half into) the step's ground
			if (FVector::DotProduct(C.P - C.PlaneAt, C.PlaneN) < C.Size * 0.15f && FVector::DotProduct(C.V, C.PlaneN) < 0.f)
			{
				C.Rest = 0.f;
			}
		}
		else if ((C.Rest += Dt) > MeltSeconds)
		{
			Clumps.RemoveAt(i);
		}
	}
	TArray<FTransform> T;
	T.Reserve(ClumpSlots);
	for (const FClump& C : Clumps)
	{
		const float Melt = C.Rest < 0.f ? 1.f : FMath::Square(1.f - C.Rest / MeltSeconds);
		T.Emplace(C.R, C.P - C.PlaneN * (C.Rest < 0.f ? 0.f : C.Size * 0.3f * (C.Rest / MeltSeconds)), C.Squash * (C.Size * Melt / 100.f));
	}
	while (T.Num() < ClumpSlots)
	{
		T.Emplace(FQuat::Identity, FVector::ZeroVector, FVector::ZeroVector);
	}
	ClumpInstances->BatchUpdateInstancesTransforms(0, T, true, true, true);
	bClumpsDirty = Clumps.Num() > 0;                                   // one more update after the last one goes
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
	const int32 Kick = CVarSnowKick.GetValueOnGameThread();
	if (Kick == 2 && StepPuff)
	{
		// the old puff, kept to compare: powder kicks up more in soft snow; hard crust only gives off a little dust
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), StepPuff, Hit.ImpactPoint + Normal * 3.f, FRotator::ZeroRotator,
			FVector(FMath::Lerp(0.6f, 1.3f, Softness)), true, true, ENCPoolMethod::AutoRelease);
	}
	else if (Kick == 1)
	{
		KickClumps(Hit.ImpactPoint, Normal, WalkDirection, Softness);
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
