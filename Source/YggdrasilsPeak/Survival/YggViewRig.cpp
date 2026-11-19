#include "Survival/YggViewRig.h"

#include "Survival/ClimbingComponent.h"
#include "Survival/FirstPersonBodyComponent.h"
#include "Survival/MountainAudioComponent.h"
#include "Survival/SurvivalComponent.h"
#include "Survival/YggSettings.h"
#include "AudioDevice.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Camera/CameraTypes.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/CapsuleComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WorldPartitionStreamingSourceComponent.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/WorldSettings.h"
#include "HAL/IConsoleManager.h"
#include "InputCoreTypes.h"
#include "Kismet/GameplayStatics.h"
#include "Stats/Stats.h"

static TAutoConsoleVariable<float> CVarPlayerHeightCm(TEXT("ygg.Player.HeightCm"), 190.f, TEXT("Sangye's height in cm (170 = the model as built; the pawn scales, the eye follows)."));

DEFINE_LOG_CATEGORY_STATIC(LogYggView, Log, All);
DECLARE_STATS_GROUP(TEXT("YggView"), STATGROUP_YggView, STATCAT_Advanced);
DECLARE_CYCLE_STAT(TEXT("View rig"), STAT_YggView, STATGROUP_YggView);

namespace
{
	TAutoConsoleVariable<int32> CVarViewRig(TEXT("ygg.View.Rig"), 1,
		TEXT("1 = the view rig owns the first-person eye (Chapter1_Camera_Spec). 0 = the old camera on the head socket (A/B tests): the rig stops writing the view, leases and walk shaping stay."));
	TAutoConsoleVariable<float> CVarViewHeadMotion(TEXT("ygg.View.HeadMotion"), 1.f,
		TEXT("Head motion (Settings > Head motion): 1 full, 0.4 reduced (no roll, no breathing motion), 0 off (no bob, no breathing, landing x0.3). The footstep sound always stays."));
	TAutoConsoleVariable<float> CVarViewBob(TEXT("ygg.View.Bob"), 1.f, TEXT("Step bob scale (test switch; 0 = off)."));
	TAutoConsoleVariable<float> CVarViewBreath(TEXT("ygg.View.Breath"), 1.f, TEXT("Breathing motion scale (test switch; 0 = off)."));
	TAutoConsoleVariable<float> CVarViewEyeFwd(TEXT("ygg.View.EyeFwd"), 16.f, TEXT("The eye sits this far (cm) in front of the body axis (out of the jacket collar)."));
	TAutoConsoleVariable<float> CVarViewNeck(TEXT("ygg.View.Neck"), 1.f, TEXT("Neck and waist model scale (test switch; 0 = the eye turns on the spot)."));
	TAutoConsoleVariable<int32> CVarViewOneBody(TEXT("ygg.View.OneBody"), 1,
		TEXT("1 = walking shows one world-space body (FeetBody with torso and arms, no seam at the hips); 0 = the older two-projection body."));
	TAutoConsoleVariable<int32> CVarViewLog(TEXT("ygg.View.Log"), 0, TEXT("1 = log every lease taken / released and the posture steps."));
	TAutoConsoleVariable<int32> CVarViewDebug(TEXT("ygg.View.Debug"), 0, TEXT("1 = draw the rig's state and leases on the screen."));
	TAutoConsoleVariable<float> CVarViewSlipSlowMo(TEXT("ygg.View.SlipSlowMo"), 0.35f,
		TEXT("Time dilation in the middle of the prologue slip (spec 6.8); 1 = no slow motion."));
	TAutoConsoleVariable<int32> CVarViewSteady(TEXT("ygg.View.SteadyExposure"), 1,
		TEXT("1 = from 20 to 40 degrees below level the auto exposure all but stops adapting (snow filling the view no longer dims the screen). 0 = engine default."));
	TAutoConsoleVariable<int32> CVarViewDeadZone(TEXT("ygg.View.TurnDeadZone"), 1,
		TEXT("1 = standing still, the body only turns after the view when it is more than 45 degrees off (the legs no longer spin while looking around)."));
	TAutoConsoleVariable<int32> CVarViewGentle(TEXT("ygg.View.Gentle"), -1,
		TEXT("Story camera: -1 = the setting (Story camera Guided / Gentle), 0 = guided, 1 = gentle (no nudges or reflexes; the slip is a plain eyelid close)."));
	TAutoConsoleVariable<int32> CVarViewHeartZoom(TEXT("ygg.View.HeartZoom"), -1,
		TEXT("Heartbeat zoom: -1 = the setting, 0 = off (no heart FOV, no FOV beat), 1 = on."));

	bool RigLog() { return CVarViewLog.GetValueOnGameThread() != 0; }

	float Ease(float T) { T = FMath::Clamp(T, 0.f, 1.f); return T * T * (3.f - 2.f * T); }
	float EaseOut(float T) { T = FMath::Clamp(T, 0.f, 1.f); return 1.f - (1.f - T) * (1.f - T); }
	float Smoother(float T) { T = FMath::Clamp(T, 0.f, 1.f); return T * T * T * (T * (T * 6.f - 15.f) + 10.f); }
	float ExpTo(float Cur, float Target, float Tau, float Dt) { return Tau <= 0.f ? Target : Cur + (Target - Cur) * (1.f - FMath::Exp(-Dt / Tau)); }
	float Bump(float X, float At, float W) { const float D = (X - At) / W; return FMath::Exp(-D * D); }
	float Norm(double Angle) { return float(FRotator::NormalizeAxis(Angle)); }
	/** 0 at or past the limit, 1 from SoftEdge away: the look slows into a posture's limit instead of hitting a wall. */
	float Soft(float Dist, float Edge) { return Dist <= 0.f ? 0.f : FMath::SmoothStep(0.f, FMath::Max(Edge, 0.01f), Dist); }

	/** Exact step of a critically damped spring (stable at any frame time). */
	void CriticalSpring(float& X, float& V, float Goal, float Omega, float Dt)
	{
		const float X0 = X - Goal;
		const float E = FMath::Exp(-Omega * Dt);
		const float Tmp = (V + Omega * X0) * Dt;
		X = Goal + (X0 + Tmp) * E;
		V = (V - Tmp * Omega) * E;
	}
	/** A damped spring back to 0, sub-stepped so a long frame can't blow it up. */
	void DampedSpring(float& X, float& V, float Omega, float Zeta, float Dt)
	{
		const int32 N = FMath::Clamp(FMath::CeilToInt(Dt * 240.f), 1, 16);
		const float H = Dt / float(N);
		for (int32 i = 0; i < N; ++i)
		{
			V += (-2.f * Zeta * Omega * V - Omega * Omega * X) * H;
			X += V * H;
		}
	}
	/** The start speed that makes a damped spring at rest peak at Deg. */
	float NodImpulse(float Deg, float Omega, float Zeta)
	{
		const float Z = FMath::Clamp(Zeta, 0.05f, 0.95f);
		const float Root = FMath::Sqrt(1.f - Z * Z);
		const float Peak = FMath::Exp(-Z / Root * FMath::Atan2(Root, Z));
		return Deg * Omega / FMath::Max(Peak, 0.05f);
	}
	float TableLerp(const TArray<float>& Xs, const TArray<float>& Ys, float X)
	{
		const int32 N = FMath::Min(Xs.Num(), Ys.Num());
		if (N == 0) { return 0.f; }
		if (X <= Xs[0]) { return Ys[0]; }
		for (int32 i = 1; i < N; ++i)
		{
			if (X <= Xs[i]) { return FMath::Lerp(Ys[i - 1], Ys[i], (X - Xs[i - 1]) / FMath::Max(Xs[i] - Xs[i - 1], 0.001f)); }
		}
		return Ys[N - 1];
	}

	const TCHAR* LeaseName(EYggLease K)
	{
		static const TCHAR* const Names[] = { TEXT("Move"), TEXT("Look"), TEXT("Pace"), TEXT("Fall"), TEXT("Knock"), TEXT("Fly"), TEXT("Inspect"), TEXT("Hands"),
			TEXT("Attention"), TEXT("FovBeat"), TEXT("StoryFov"), TEXT("Nudge"), TEXT("Lids"), TEXT("Breath"), TEXT("Exposure"), TEXT("Lens") };
		return Names[FMath::Clamp(int32(K), 0, 15)];
	}
	const TCHAR* ViewName(EYggView V)
	{
		static const TCHAR* const Names[] = { TEXT("stand"), TEXT("lie"), TEXT("sit"), TEXT("kneel1"), TEXT("kneel2"), TEXT("crawl") };
		return Names[FMath::Clamp(int32(V), 0, 5)];
	}
	const TCHAR* ScriptedName(EYggScripted S)
	{
		static const TCHAR* const Names[] = { TEXT("none"), TEXT("fall"), TEXT("knock"), TEXT("fly") };
		return Names[FMath::Clamp(int32(S), 0, 3)];
	}
	constexpr int32 LeaseKinds = 16;

	/** The prologue slip at 1.5 s (spec 6.8): time, eye drop, forward, pitch, yaw and roll toward the side. */
	struct FFallKey { float T, Z, Fwd, Pitch, Yaw, Roll; };
	const FFallKey FallKeys[] = {
		{ 0.00f,   0.f,  0.f,   0.f,  0.f,  0.f },
		{ 0.12f,  -5.f,  0.f,  -4.f,  0.f,  0.f },
		{ 0.40f, -35.f, 10.f, -22.f,  8.f, 12.f },
		{ 1.10f, -70.f, 10.f, -48.f, 12.f, 26.f },
		{ 1.50f, -85.f,  5.f, -62.f, 15.f, 30.f } };
	constexpr int32 NumFallKeys = UE_ARRAY_COUNT(FallKeys);

	/** One channel of the fall curve: a cubic through the keys (Catmull-Rom tangents, flat at the ends), so no corner jerks. */
	float FallChannel(float T, float FFallKey::* Ch)
	{
		if (T <= FallKeys[0].T) { return FallKeys[0].*Ch; }
		if (T >= FallKeys[NumFallKeys - 1].T) { return FallKeys[NumFallKeys - 1].*Ch; }
		int32 i = 0;
		while (i < NumFallKeys - 2 && T > FallKeys[i + 1].T) { ++i; }
		const auto Tangent = [Ch](int32 k)
		{
			if (k <= 0 || k >= NumFallKeys - 1) { return 0.f; }
			return (FallKeys[k + 1].*Ch - FallKeys[k - 1].*Ch) / (FallKeys[k + 1].T - FallKeys[k - 1].T);
		};
		const float T0 = FallKeys[i].T, T1 = FallKeys[i + 1].T, H = T1 - T0;
		const float S = (T - T0) / H, S2 = S * S, S3 = S2 * S;
		return (2.f * S3 - 3.f * S2 + 1.f) * (FallKeys[i].*Ch) + (S3 - 2.f * S2 + S) * H * Tangent(i)
			+ (-2.f * S3 + 3.f * S2) * (FallKeys[i + 1].*Ch) + (S3 - S2) * H * Tangent(i + 1);
	}

	/** Uniform Catmull-Rom through points, S in 0..Num-1. */
	FVector CatmullRom(const TArray<FVector>& P, float S)
	{
		const int32 N = P.Num();
		if (N == 0) { return FVector::ZeroVector; }
		if (N == 1) { return P[0]; }
		S = FMath::Clamp(S, 0.f, float(N - 1));
		const int32 i = FMath::Min(FMath::FloorToInt(S), N - 2);
		const float T = S - float(i);
		const FVector& P0 = P[FMath::Max(i - 1, 0)];
		const FVector& P1 = P[i];
		const FVector& P2 = P[i + 1];
		const FVector& P3 = P[FMath::Min(i + 2, N - 1)];
		const float T2 = T * T, T3 = T2 * T;
		return 0.5f * ((2.f * P1) + (-P0 + P2) * T + (2.f * P0 - 5.f * P1 + 4.f * P2 - P3) * T2 + (-P0 + 3.f * P1 - 3.f * P2 + P3) * T3);
	}
	/** Cumulative arc length (normalised 0..1) at even curve-parameter samples. */
	void BuildArc(const TArray<FVector>& P, TArray<float>& Out)
	{
		Out.Reset();
		const int32 Segs = FMath::Max(P.Num() - 1, 1);
		const int32 M = 32 * Segs;
		Out.Reserve(M + 1);
		Out.Add(0.f);
		FVector Last = CatmullRom(P, 0.f);
		float Total = 0.f;
		for (int32 j = 1; j <= M; ++j)
		{
			const FVector Now = CatmullRom(P, float(j) / float(M) * float(Segs));
			Total += float(FVector::Dist(Now, Last));
			Out.Add(Total);
			Last = Now;
		}
		for (float& A : Out) { A = Total > 1.f ? A / Total : 0.f; }
		if (Total <= 1.f) { for (int32 j = 0; j <= M; ++j) { Out[j] = float(j) / float(M); } }
	}
	/** The point at arc fraction U along the curve. */
	FVector ArcPoint(const TArray<FVector>& P, const TArray<float>& Arc, float U)
	{
		if (P.Num() <= 1 || Arc.Num() < 2) { return P.Num() ? P[0] : FVector::ZeroVector; }
		U = FMath::Clamp(U, 0.f, 1.f);
		int32 Lo = 0, Hi = Arc.Num() - 1;
		while (Hi - Lo > 1) { const int32 Mid = (Lo + Hi) / 2; if (Arc[Mid] <= U) { Lo = Mid; } else { Hi = Mid; } }
		const float Span = FMath::Max(Arc[Hi] - Arc[Lo], 1e-6f);
		const float J = float(Lo) + (U - Arc[Lo]) / Span;
		const int32 Segs = FMath::Max(P.Num() - 1, 1);
		return CatmullRom(P, J / float(Arc.Num() - 1) * float(Segs));
	}

	FAutoConsoleCommandWithWorld GViewAuditCommand(TEXT("ygg.View.Audit"),
		TEXT("Print the view rig: posture, eye height, lids, takeover, every lease (kind, beat, age / max), view target, story lens, recent repairs."),
		FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World)
		{
			UYggViewRigComponent* Rig = World ? UYggViewRigComponent::Find(World) : nullptr;
			if (!Rig)
			{
				UE_LOG(LogYggView, Display, TEXT("Rig: no view rig on the local player"));
				return;
			}
			TArray<FString> Lines;
			Rig->Audit().ParseIntoArrayLines(Lines);
			for (const FString& L : Lines) { UE_LOG(LogYggView, Display, TEXT("%s"), *L); }
		}));
}

// ------------------------------------------------------------------------------------------------ set-up

UYggViewRigComponent::UYggViewRigComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
	// after character movement, before the camera manager updates (the eye is placed for this frame's view)
	PrimaryComponentTick.TickGroup = TG_PostPhysics;

	// defaults of the posture table (DefaultGame.ini +Poses replaces them)
	const auto AddPose = [this](const TCHAR* Name, float Eye, float Band, float Down, float Up, float Yaw, float PMin, float PMax, bool bHide)
	{
		FYggPoseDef D;
		D.Name = Name; D.EyeCm = Eye; D.BandCm = Band; D.DownS = Down; D.UpS = Up; D.YawHalf = Yaw; D.PitchMin = PMin; D.PitchMax = PMax; D.bHideBody = bHide;
		Poses.Add(D);
	};
	AddPose(TEXT("stand"), 156.f, 6.f, 0.f, 0.f, 180.f, -75.f, 80.f, false);
	AddPose(TEXT("lie"), 32.f, 6.f, 2.5f, 2.2f, 70.f, -20.f, 70.f, true);
	AddPose(TEXT("sit"), 78.f, 6.f, 2.0f, 1.6f, 110.f, -60.f, 55.f, true);
	AddPose(TEXT("kneel1"), 108.f, 6.f, 0.9f, 1.0f, 85.f, -50.f, 50.f, true);
	AddPose(TEXT("kneel2"), 110.f, 6.f, 1.0f, 1.2f, 80.f, -45.f, 65.f, true);
	AddPose(TEXT("crawl"), 68.f, 8.f, 0.8f, 1.4f, 45.f, -40.f, 25.f, true);

	BobSpeedCmS = { 45.f, 60.f, 110.f, 160.f, 230.f, 300.f, 500.f };
	BobVerticalCm = { 1.2f, 1.5f, 2.0f, 2.6f, 3.0f, 3.6f, 4.5f };
	BobLateralCm = { 0.5f, 0.8f, 1.0f, 1.3f, 1.5f, 1.8f, 2.2f };
	BobRollDeg = { 0.f, 0.f, 0.15f, 0.2f, 0.2f, 0.25f, 0.3f };

	const auto AddHands = [this](const TCHAR* Name, const FVector& L, const FVector& R)
	{
		FYggHandsDef H;
		H.Name = Name; H.Left = L; H.Right = R;
		Hands.Add(H);
	};
	AddHands(TEXT("compass"), FVector::ZeroVector, FVector(40.f, 8.f, -22.f));
	AddHands(TEXT("photo"), FVector(38.f, -10.f, -16.f), FVector(38.f, 10.f, -16.f));
	AddHands(TEXT("bottle"), FVector::ZeroVector, FVector(38.f, 6.f, -24.f));
	AddHands(TEXT("khata"), FVector(42.f, -12.f, -30.f), FVector(42.f, 12.f, -30.f));
	AddHands(TEXT("dolma"), FVector(30.f, -25.f, -55.f), FVector::ZeroVector);

	for (int32& O : LastOwner) { O = -1; }
}

UYggViewRigComponent* UYggViewRigComponent::Find(const UObject* Ctx)
{
	if (!Ctx)
	{
		return nullptr;
	}
	const AActor* Actor = Cast<AActor>(Ctx);
	if (!Actor)
	{
		if (const UActorComponent* Comp = Cast<UActorComponent>(Ctx)) { Actor = Comp->GetOwner(); }
	}
	if (Actor)
	{
		if (UYggViewRigComponent* R = Actor->FindComponentByClass<UYggViewRigComponent>()) { return R; }
	}
	UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(Ctx, EGetWorldErrorMode::ReturnNull) : nullptr;
	APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
	const APawn* Pawn = PC ? PC->GetPawn() : nullptr;
	return Pawn ? Pawn->FindComponentByClass<UYggViewRigComponent>() : nullptr;
}

UYggViewRigComponent* UYggViewRigComponent::EnsureFor(ACharacter* Character)
{
	if (!Character)
	{
		return nullptr;
	}
	if (UYggViewRigComponent* Existing = Character->FindComponentByClass<UYggViewRigComponent>())
	{
		Existing->EnsureModifier();
		return Existing;
	}
	// Sangye's height (user 2026-11-20: 1.7 m -> 1.9 m): the whole pawn scales (capsule, body, feet), the eye follows in
	// the rig (StandEyeCm is for a 1.7 m body and is multiplied by the same factor in BeginPlay)
	if (const IConsoleVariable* H = IConsoleManager::Get().FindConsoleVariable(TEXT("ygg.Player.HeightCm")))
	{
		const float K = FMath::Clamp(H->GetFloat() / 170.f, 0.8f, 1.25f);
		if (!FMath::IsNearlyEqual(float(Character->GetActorScale3D().Z), K, 0.01f)) { Character->SetActorScale3D(FVector(K)); }
	}
	UYggViewRigComponent* Rig = NewObject<UYggViewRigComponent>(Character, MakeUniqueObjectName(Character, UYggViewRigComponent::StaticClass(), TEXT("YggViewRig")));
	Character->AddInstanceComponent(Rig);
	Rig->RegisterComponent();   // the pawn already plays: BeginPlay runs here
	Rig->EnsureModifier();
	return Rig;
}

void UYggViewRigComponent::BeginPlay()
{
	Super::BeginPlay();

	ACharacter* C = Cast<ACharacter>(GetOwner());
	Char = C;
	if (!C)
	{
		return;
	}
	// the S-curve: smoothstep(u^p) with p chosen so that 70% of the change is done at 60% of the time
	{
		const float Target = FMath::Clamp(PoseCurveAt60, 0.55f, 0.95f);
		float Lo = 0.f, Hi = 1.f;
		for (int32 i = 0; i < 30; ++i) { const float Mid = 0.5f * (Lo + Hi); if (Mid * Mid * (3.f - 2.f * Mid) < Target) { Lo = Mid; } else { Hi = Mid; } }
		PoseCurveExp = FMath::Loge(FMath::Max(0.5f * (Lo + Hi), 0.01f)) / FMath::Loge(0.6f);
	}
	// the exposure channel's fixed parts (config is loaded by now)
	PPSteady.bOverride_AutoExposureSpeedUp = true;
	PPSteady.bOverride_AutoExposureSpeedDown = true;
	PPSteady.AutoExposureSpeedUp = SteadyExposureSpeed;
	PPSteady.AutoExposureSpeedDown = SteadyExposureSpeed;
	PPBias.bOverride_AutoExposureBias = true;
	PPInspect.bOverride_ColorSaturation = true;
	PPInspect.bOverride_ColorGain = true;
	const float Sat = 1.f - InspectDesaturate, Gain = 1.f - InspectDarken;
	PPInspect.ColorSaturation = FVector4(Sat, Sat, Sat, 1.f);
	PPInspect.ColorGain = FVector4(Gain, Gain, Gain, 1.f);

	// the first-person camera (the climbing camera is another camera component on the same pawn)
	TArray<UCameraComponent*> Cams;
	C->GetComponents(Cams);
	for (UCameraComponent* Cam : Cams) { if (Cam && Cam->GetName() == TEXT("FirstPersonCamera")) { Camera = Cam; break; } }
	if (!Camera)
	{
		for (UCameraComponent* Cam : Cams) { if (Cam && !Cam->GetName().Contains(TEXT("Climb"))) { Camera = Cam; break; } }
	}
	bSavedCtrlYaw = C->bUseControllerRotationYaw;
	AnchorYaw = Norm(C->GetControlRotation().Yaw);
	StandEyeCm = 156.f * FMath::Clamp(CVarPlayerHeightCm.GetValueOnGameThread() / 170.f, 0.8f, 1.25f);   // eye of the scaled body
	PoseH = PoseFrom = PoseTo = StandEyeCm;
	C->LandedDelegate.AddDynamic(this, &UYggViewRigComponent::OnLanded);
	NextLookup = 0.0;
	LookupSources();
	ApplyRigToggle();
	UE_LOG(LogYggView, Display, TEXT("Rig: on %s (camera %s%s)"), *GetNameSafe(C), *GetNameSafe(Camera),
		bCamReparented ? TEXT(" re-parented to the capsule") : TEXT(", not moved"));
}

void UYggViewRigComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	SetDilation(1.f, 1.f);
	if (ACharacter* C = Char.Get())
	{
		C->LandedDelegate.RemoveDynamic(this, &UYggViewRigComponent::OnLanded);
		SetCtrlYawOwned(false);
		if (UCharacterMovementComponent* Move = C->GetCharacterMovement())
		{
			if (Move->bConstrainToPlane) { Move->SetPlaneConstraintEnabled(false); }
			if (bJumpBlocked) { Move->SetJumpAllowed(bJumpWasAllowed); bJumpBlocked = false; }
		}
	}
	if (APlayerCameraManager* PCM = ModPCM.Get())
	{
		if (bBackstopSet) { PCM->ViewPitchMin = SavedPitchMin; PCM->ViewPitchMax = SavedPitchMax; bBackstopSet = false; }
		if (Modifier) { PCM->RemoveCameraModifier(Modifier); }
	}
	Modifier = nullptr;
	if (FlyCam)
	{
		FlyCam->Destroy();
		FlyCam = nullptr;
	}
	Super::EndPlay(EndPlayReason);
}

APlayerController* UYggViewRigComponent::GetPC() const
{
	const ACharacter* C = Char.Get();
	return C ? Cast<APlayerController>(C->GetController()) : nullptr;
}

APlayerCameraManager* UYggViewRigComponent::GetPCM() const
{
	const APlayerController* PC = GetPC();
	return PC ? PC->PlayerCameraManager.Get() : nullptr;
}

void UYggViewRigComponent::EnsureModifier()
{
	APlayerCameraManager* PCM = GetPCM();
	if (!PCM)
	{
		return;
	}
	if (ModPCM.Get() != PCM || !Modifier)
	{
		// a new camera manager (first possession, a respawn): the modifier goes there
		UYggViewModifier* M = Cast<UYggViewModifier>(PCM->FindCameraModifierByClass(UYggViewModifier::StaticClass()));
		if (!M) { M = Cast<UYggViewModifier>(PCM->AddNewCameraModifier(UYggViewModifier::StaticClass())); }
		if (M) { M->Rig = this; }
		Modifier = M;
		ModPCM = PCM;
		bBackstopSet = false;
	}
	if (bRigOn && !bBackstopSet)
	{
		// hard pitch backstops for the soft limits (BP_FirstPersonCameraManager had -70 / 80)
		SavedPitchMin = PCM->ViewPitchMin;
		SavedPitchMax = PCM->ViewPitchMax;
		PCM->ViewPitchMin = BackstopPitchMin;
		PCM->ViewPitchMax = BackstopPitchMax;
		bBackstopSet = true;
	}
}

void UYggViewRigComponent::LookupSources()
{
	if (Clock < NextLookup)
	{
		return;
	}
	NextLookup = Clock + 0.5;
	ACharacter* C = Char.Get();
	if (!C)
	{
		return;
	}
	// ExpeditionComponent adds the audio and the snow trail at run time, maybe after this BeginPlay: looked up until found
	if (!Body.IsValid()) { Body = C->FindComponentByClass<UFirstPersonBodyComponent>(); }
	if (!Audio.IsValid()) { Audio = C->FindComponentByClass<UMountainAudioComponent>(); }
	if (!Survival.IsValid()) { Survival = C->FindComponentByClass<USurvivalComponent>(); }
	if (!Climbing.IsValid()) { Climbing = C->FindComponentByClass<UClimbingComponent>(); }
	// steps: USnowTrailComponent feeds OnStep directly (it must not also be bound to its OnStep delegate: double steps)
	EnsureModifier();
}

void UYggViewRigComponent::ApplyRigToggle()
{
	ACharacter* C = Char.Get();
	const bool bWant = CVarViewRig.GetValueOnGameThread() != 0 && C && Camera;
	if (bWant == bRigOn)
	{
		return;
	}
	bRigOn = bWant;
	APlayerCameraManager* PCM = GetPCM();
	if (bRigOn)
	{
		// The eye leaves the head socket: the rig places it every tick on the capsule, and anything attached to the camera
		// (compass, inspect card, headlamp) rides the true eye. The old socket spot is kept for ygg.View.Rig 0.
		if (!bCamReparented && C->GetRootComponent())
		{
			OldCamParent = Camera->GetAttachParent();
			OldCamSocket = Camera->GetAttachSocketName();
			OldCamRelative = Camera->GetRelativeTransform();
			Camera->AttachToComponent(C->GetRootComponent(), FAttachmentTransformRules::KeepWorldTransform);
			bCamReparented = true;
		}
		bTerrainInit = false;
		bBackstopSet = false;
		EnsureModifier();
	}
	else
	{
		if (bCamReparented && OldCamParent.IsValid())
		{
			Camera->AttachToComponent(OldCamParent.Get(), FAttachmentTransformRules::KeepRelativeTransform, OldCamSocket);
			Camera->SetRelativeTransform(OldCamRelative);
		}
		bCamReparented = false;
		if (PCM && bBackstopSet) { PCM->ViewPitchMin = SavedPitchMin; PCM->ViewPitchMax = SavedPitchMax; }
		bBackstopSet = false;
		SetCtrlYawOwned(false);
		SetDilation(1.f, 1.f);
		if (UCharacterMovementComponent* Move = C ? C->GetCharacterMovement() : nullptr)
		{
			if (Move->bConstrainToPlane) { Move->SetPlaneConstraintEnabled(false); }
		}
		UE_LOG(LogYggView, Display, TEXT("Rig: off (ygg.View.Rig 0): the camera is back on its socket"));
	}
}

// ------------------------------------------------------------------------------------------------ tick

void UYggViewRigComponent::TickComponent(float Dt, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	SCOPE_CYCLE_COUNTER(STAT_YggView);
	Super::TickComponent(Dt, TickType, ThisTickFunction);

	ACharacter* C = Char.Get();
	UWorld* World = GetWorld();
	if (!C || !World)
	{
		return;
	}
	// story things (lids, takeovers, the fly-out, leases) run on real time, so the slip's slow motion doesn't stretch them
	RealDt = FMath::Clamp(World->DeltaRealTimeSeconds, 0.f, 0.25f);
	const float GameDt = FMath::Clamp(Dt, 0.f, 0.1f);
	Clock += RealDt;

	LookupSources();
	ApplyRigToggle();
	TickLeases();
	UpdateFeeds(GameDt);
	TickLids(RealDt);
	TickPosture(GameDt);
	TickBodyYaw(GameDt);
	TickLimits(RealDt);
	TickBob(GameDt);
	TickBreath(GameDt);
	TickLandingAndNods(GameDt);
	TickScripted(RealDt);
	TickFly(RealDt);
	TickInspect(RealDt);
	TickLens(RealDt);
	Watchdog(GameDt);
	ComposeEye(GameDt);
	ComposeViewOffsets(GameDt);
	PushBody();
	if (CVarViewDebug.GetValueOnGameThread() != 0)
	{
		DrawDebug();
	}
}

void UYggViewRigComponent::UpdateFeeds(float Dt)
{
	ACharacter* C = Char.Get();
	const UCharacterMovementComponent* Move = C->GetCharacterMovement();
	const FVector V = C->GetVelocity();
	Speed2D = float(V.Size2D());
	bOnGround = Move && (Move->IsMovingOnGround() || Move->MovementMode == MOVE_None);
	if (Move && Move->IsFalling())
	{
		LastFallVz = FMath::Min(LastFallVz, float(V.Z));
	}
	else if (Clock - LastLandTime > 0.2)
	{
		LastFallVz = 0.f;
	}
	if (const USurvivalComponent* S = Survival.Get())
	{
		AltM = S->GetElevationMeters();
	}
	// the grade along the walking direction, from the movement's own floor (no trace)
	if (Move && bOnGround && Speed2D > 30.f && Move->CurrentFloor.bBlockingHit)
	{
		const FVector N = Move->CurrentFloor.HitResult.ImpactNormal;
		const FVector D = V.GetSafeNormal2D();
		GradeDeg = FMath::RadiansToDegrees(FMath::Atan2(-float(N | D), FMath::Max(float(N.Z), 0.05f)));
	}
	else
	{
		GradeDeg = FMath::FInterpTo(GradeDeg, 0.f, Dt, 2.f);
	}
	HeadMotion = FMath::Clamp(CVarViewHeadMotion.GetValueOnGameThread(), 0.f, 1.f);
	if (Clock >= NextSettingsRead)
	{
		NextSettingsRead = Clock + 0.5;
		const int32 G = CVarViewGentle.GetValueOnGameThread();
		bGentle = G >= 0 ? G != 0 : YggSettings::StoryCameraGentle();
		const int32 H = CVarViewHeartZoom.GetValueOnGameThread();
		bHeartZoom = H >= 0 ? H != 0 : YggSettings::HeartbeatZoom();
	}
	// how fast the view turns (attention waits for stillness, the optional turn lean)
	const float Yaw = Norm(C->GetControlRotation().Yaw);
	if (bHasLastCtrlYaw && Dt > KINDA_SMALL_NUMBER)
	{
		TurnRate = FMath::FInterpTo(TurnRate, Norm(Yaw - LastCtrlYaw) / Dt, Dt, 1.f / FMath::Max(TurnLeanTau, 0.02f));
	}
	LastCtrlYaw = Yaw;
	bHasLastCtrlYaw = true;
}

// ------------------------------------------------------------------------------------------------ leases

float UYggViewRigComponent::LeaseCap(EYggLease Kind, bool bKeep) const
{
	switch (Kind)
	{
	case EYggLease::Move: return bKeep ? 3600.f : 120.f;   // "hold": "end" lasts to the chapter end (night beat only)
	case EYggLease::Look: return 20.f;
	case EYggLease::Pace: return 600.f;
	case EYggLease::Fall: return 3.f;
	case EYggLease::Knock: return 2.f;
	case EYggLease::Fly: return 600.f;                       // the shot's own length + 3 s is passed in
	case EYggLease::Inspect: return 300.f;
	case EYggLease::Hands: return 60.f;
	case EYggLease::Attention: return 15.f;
	case EYggLease::FovBeat: return 12.f;
	case EYggLease::StoryFov: return bKeep ? 1200.f : 300.f;   // a kept story lens spans a long walk (alone beat ~950 m)
	case EYggLease::Nudge: return 3.f;
	case EYggLease::Lids: return 8.f;
	case EYggLease::Breath: return 10.f;
	case EYggLease::Exposure: return 10.f;
	case EYggLease::Lens: return 20.f;
	default: return 10.f;
	}
}

int32 UYggViewRigComponent::AcquireLease(int32 Owner, EYggLease Kind, float MaxSec, bool bKeep)
{
	const float Cap = LeaseCap(Kind, bKeep);
	const float Max = MaxSec > 0.f ? FMath::Min(MaxSec, Cap) : Cap;
	LastOwner[FMath::Clamp(int32(Kind), 0, LeaseKinds - 1)] = Owner;
	for (FLease& L : Leases)
	{
		if (L.Owner == Owner && L.Kind == Kind)
		{
			// a refresh never shortens what is left of a live lease (the rig's retimed fly-out vs the story's own length)
			L.bKeep |= bKeep;
			const float NewMax = MaxSec > 0.f ? FMath::Min(MaxSec, LeaseCap(Kind, L.bKeep)) : LeaseCap(Kind, L.bKeep);
			L.MaxSec = FMath::Max(NewMax, L.MaxSec - float(Clock - L.Start));
			L.Start = Clock;
			return L.Id;
		}
	}
	FLease L;
	L.Id = NextLeaseId++;
	L.Owner = Owner;
	L.Kind = Kind;
	L.Start = Clock;
	L.MaxSec = Max;
	L.bKeep = bKeep;
	Leases.Add(L);
	LeaseLog(TEXT("takes"), L);
	return L.Id;
}

void UYggViewRigComponent::ReleaseLease(int32 Owner, EYggLease Kind)
{
	for (int32 i = Leases.Num() - 1; i >= 0; --i)
	{
		if (Leases[i].Owner == Owner && Leases[i].Kind == Kind)
		{
			LeaseLog(TEXT("releases"), Leases[i]);
			Leases.RemoveAt(i);
		}
	}
}

void UYggViewRigComponent::QuietRelease(int32 Owner, EYggLease Kind)
{
	ReleaseLease(Owner, Kind);
}

void UYggViewRigComponent::ReleaseOwner(int32 Owner)
{
	for (int32 i = Leases.Num() - 1; i >= 0; --i)
	{
		if (Leases[i].Owner == Owner && !Leases[i].bKeep)
		{
			LeaseLog(TEXT("releases (beat ended)"), Leases[i]);
			Leases.RemoveAt(i);
		}
	}
}

void UYggViewRigComponent::ReleaseAll(float Blend)
{
	UE_LOG(LogYggView, Display, TEXT("Rig: release all (%d leases, blend %.1f s)"), Leases.Num(), Blend);
	Leases.Reset();
	BlendOutSec = FMath::Max(Blend, 0.f);
	// what the story lens held fades over the blend (0 = at once); the watchdog winds down the rest this tick
	if (FMath::Abs(StoryFov) > 0.01f)
	{
		if (BlendOutSec <= 0.01f) { StoryFov = StoryFovTo = 0.f; StoryFovLen = 0.f; }
		else { StoryFovFrom = StoryFov; StoryFovTo = 0.f; StoryFovT = 0.f; StoryFovLen = BlendOutSec; }
	}
	bAttn = false;
	if (BlendOutSec <= 0.01f) { AttnValue = 0.f; FovBeatValue = 0.f; bFovBeat = false; }
	for (FExposurePulse& P : Pulses)
	{
		if (P.bStory && P.T < P.Rise + P.Hold) { P.Hold = FMath::Max(0.f, P.T - P.Rise); P.Tau = FMath::Max(BlendOutSec, 0.05f) / 3.f; }
	}
	StoryB = -1.f;
	PaceCmS = -1.f;
	if (bNudge) { EndNudge(TEXT("released")); }
	bReflex = false;
	PullActor = nullptr;
	PullRate = 0.f;
	bPending = false;
	// the rig's own pending closures must not run after a reset (crawl fallback teleport, view-target repair)
	LidOnClosed = nullptr;
	bRepairingViewTarget = false;
	bCrawlFallback = false;
}

bool UYggViewRigComponent::HasLease(EYggLease Kind) const
{
	for (const FLease& L : Leases)
	{
		if (L.Kind == Kind) { return true; }
	}
	return false;
}

uint32 UYggViewRigComponent::GetLeaseMask() const
{
	uint32 Mask = 0;
	for (const FLease& L : Leases) { Mask |= 1u << uint32(L.Kind); }
	return Mask;
}

void UYggViewRigComponent::TickLeases()
{
	for (int32 i = Leases.Num() - 1; i >= 0; --i)
	{
		const FLease& L = Leases[i];
		const float Age = float(Clock - L.Start);
		if (Age >= L.MaxSec)
		{
			if (L.bQuiet) { LeaseLog(TEXT("ends (timed)"), L); }
			else { UE_LOG(LogYggView, Warning, TEXT("Rig: lease %s of beat %d expired after %.1f s"), LeaseName(L.Kind), L.Owner, Age); }
			Leases.RemoveAt(i);
		}
	}
}

void UYggViewRigComponent::LeaseLog(const TCHAR* What, const FLease& L) const
{
	if (RigLog())
	{
		UE_LOG(LogYggView, Display, TEXT("Rig: beat %d %s lease %s (max %.0f s%s)"), L.Owner, What, LeaseName(L.Kind), L.MaxSec, L.bKeep ? TEXT(", keep") : TEXT(""));
	}
}

void UYggViewRigComponent::Repair(const FString& What, EYggLease Kind)
{
	const int32 Beat = LastOwner[FMath::Clamp(int32(Kind), 0, LeaseKinds - 1)];
	UE_LOG(LogYggView, Warning, TEXT("Rig: repaired %s (beat %d)"), *What, Beat);
	RecentRepairs.Add(FString::Printf(TEXT("%s (beat %d)"), *What, Beat));
	if (RecentRepairs.Num() > 4) { RecentRepairs.RemoveAt(0); }
}

// ------------------------------------------------------------------------------------------------ gameplay feeds

void UYggViewRigComponent::SetBody(float Shiver, float Exhaust, float Beat, float Hypoxia, float PackKg)
{
	FeedShiver = FMath::Clamp(Shiver, 0.f, 1.f);
	FeedExhaust = FMath::Clamp(Exhaust, 0.f, 1.f);
	FeedBeat = FMath::Clamp(Beat, 0.f, 1.f);
	FeedHyp = FMath::Clamp(Hypoxia, 0.f, 1.f);
	FeedPackKg = FMath::Max(PackKg, 0.f);
}

void UYggViewRigComponent::SetExposureBias(FName Source, float Ev)
{
	if (FMath::Abs(Ev) < 0.001f) { ExposureSources.Remove(Source); }
	else { ExposureSources.Add(Source, FMath::Clamp(Ev, -3.f, 3.f)); }
}

void UYggViewRigComponent::AddShake(float Deg, float Seconds)
{
	if (Seconds <= 0.f || Deg <= 0.f)
	{
		return;
	}
	// a new rumble takes over when it is stronger than what is left of the old one
	const float Left = ShakeLen > 0.f ? ShakeDeg * FMath::Square(FMath::Max(0.f, 1.f - ShakeT / ShakeLen)) : 0.f;
	if (Deg >= Left)
	{
		ShakeDeg = FMath::Min(Deg, 2.f);
		ShakeLen = Seconds;
		ShakeT = 0.f;
	}
}

float UYggViewRigComponent::ShapeWalkSpeed(float Target) const
{
	// a takeover, a story hold or an inspect: standing still; postures: crawl speed or nothing (walking while posed is
	// impossible by construction, spec 4.2)
	if (bFlyActive || bFlyWaiting || Scripted == EYggScripted::Fall || bInspect)
	{
		return 0.f;
	}
	if (HasLease(EYggLease::Move) || HasLease(EYggLease::Fall) || HasLease(EYggLease::Fly) || HasLease(EYggLease::Inspect))
	{
		return 0.f;
	}
	if (bPending && !Pending.bAfterInput)
	{
		return 0.f;   // a pose waits for the climber to stop
	}
	if (View == EYggView::Crawl)
	{
		return Phase == EPhase::Leaving ? 0.f : FMath::Min(Target, CrawlSpeedCmS);
	}
	if (View != EYggView::Walk)
	{
		return 0.f;
	}
	float S = Target;
	if (PaceCmS > 0.f && HasLease(EYggLease::Pace))
	{
		S = FMath::Min(S, PaceCmS);
	}
	return S;
}

void UYggViewRigComponent::OnLanded(const FHitResult& Hit)
{
	const ACharacter* C = Char.Get();
	if (!C)
	{
		return;
	}
	// the vertical speed before impact: the velocity may already be cut, the last falling frame has it
	const float Vz = FMath::Min(float(C->GetVelocity().Z), LastFallVz);
	LastLandTime = Clock;
	LastFallVz = 0.f;
	const float Down = -Vz;
	if (Down <= LandThresholdCmS)
	{
		return;
	}
	float Dip = FMath::Min((Down - LandThresholdCmS) / 100.f * LandDipPer100, LandDipMaxCm);
	Dip *= 1.f + PackLandPerKg * FMath::Max(0.f, FeedPackKg - PackLandFromKg);
	Dip *= HeadMotion <= 0.01f ? 0.3f : 1.f;
	LandStart = LandZ;
	LandTarget = -Dip;
	LandDownT = 0.f;
}

void UYggViewRigComponent::OnStep(bool bLeft, float SpeedCmS, uint8 Surface)
{
	DoStep(bLeft, SpeedCmS, Surface, false);
}

void UYggViewRigComponent::DoStep(bool bLeft, float SpeedCmS, uint8 Surface, bool bSynth)
{
	const double Gap = Clock - LastStepTime;
	if (Gap < BobMinStepGap)
	{
		return;   // a notify and a fallback for the same boot
	}
	const float S = SpeedCmS > 1.f ? SpeedCmS : Speed2D;
	StepPeriod = Gap < 1.5 ? FMath::Clamp(float(Gap), BobPeriodMin, BobPeriodMax) : FMath::Clamp(90.f / FMath::Max(S, 1.f), BobPeriodMin, BobPeriodMax);
	LastStepTime = Clock;
	if (!bSynth)
	{
		LastRealStepTime = Clock;
		FallbackDist = 0.f;
	}
	// lock the phase: the nearest whole step of the running phase whose parity is this foot (even = left plant), so a
	// missed or doubled step never unwinds the bob
	double N = FMath::FloorToDouble(BobPsi + 0.5);
	const bool bEven = FMath::Fmod(N, 2.0) < 0.5;
	if (bEven != bLeft) { N += BobPsi > N ? 1.0 : -1.0; }
	StepBase = FMath::Max(N, 0.0);
	// this step's size: by ground speed, a little different each time (more when spent)
	float A = TableLerp(BobSpeedCmS, BobVerticalCm, S);
	float L = TableLerp(BobSpeedCmS, BobLateralCm, S);
	const float R = TableLerp(BobSpeedCmS, BobRollDeg, S);
	const float Jit = 1.f + FMath::FRandRange(-BobJitter, BobJitter) * (1.f + BobJitterFatigue * FeedExhaust);
	A *= Jit;
	L *= Jit;
	if (GradeDeg < -DownhillFromDeg)
	{
		// downhill each step lands harder and nods
		A *= DownhillDipScale;
		NodV -= NodImpulse(DownhillNodDeg, StartStopOmega, StartStopZeta) * HeadMotion;
	}
	StepA = A;
	StepL = L;
	StepR = R;
	if (int32(Surface) == RockSurfaceId && !bSynth)
	{
		RockDipCm = FMath::FRandRange(RockDipMinCm, RockDipMaxCm);
		RockDipAt = Clock;
	}
}

// ------------------------------------------------------------------------------------------------ eyelids

void UYggViewRigComponent::SetLids(int32 Owner, EYggLidMode Mode, float T, float Hold, TFunction<void()> OnClosed)
{
	const bool bClosing = Mode == EYggLidMode::Blink || Mode == EYggLidMode::Closed || Mode == EYggLidMode::Sleep;
	// a "during" that has not run yet is never lost: kept for this close, or run now before an opening
	if (LidOnClosed)
	{
		if (bClosing)
		{
			if (OnClosed)
			{
				TFunction<void()> Old = MoveTemp(LidOnClosed);
				TFunction<void()> New = MoveTemp(OnClosed);
				LidOnClosed = [Old, New]() { Old(); New(); };
			}
		}
		else
		{
			TFunction<void()> Old = MoveTemp(LidOnClosed);
			LidOnClosed = nullptr;
			if (Lid < 0.99f) { UE_LOG(LogYggView, Warning, TEXT("Rig: eyelids opened before they closed: their pending actions run now, eyes open")); }
			Old();
		}
	}
	if (bClosing && !LidOnClosed && OnClosed)
	{
		LidOnClosed = MoveTemp(OnClosed);
	}
	LidMode = Mode;
	LidOwner = Owner;
	LidT = 0.f;
	bLidMoving = true;
	bLidRepair = Owner == -2 && Mode == EYggLidMode::Open;
	LidKeys.Reset();
	const float L0 = Lid;
	switch (Mode)
	{
	case EYggLidMode::Blink:
	{
		const float Close = T > 0.f ? T : BlinkCloseSec;
		LidKeys.Add(FVector2f(0.f, L0));
		LidKeys.Add(FVector2f(Close, 1.f));
		LidKeys.Add(FVector2f(Close + FMath::Max(Hold, 0.f), 1.f));
		LidKeys.Add(FVector2f(Close + FMath::Max(Hold, 0.f) + BlinkOpenSec, 0.f));
		break;
	}
	case EYggLidMode::Closed:
		LidKeys.Add(FVector2f(0.f, L0));
		LidKeys.Add(FVector2f(FMath::Max(T, 0.f), 1.f));
		break;
	case EYggLidMode::Open:
		LidKeys.Add(FVector2f(0.f, L0));
		LidKeys.Add(FVector2f(FMath::Max(T, 0.f), 0.f));
		break;
	case EYggLidMode::Wake:
	{
		// 35% open at 0.6 s, shut again at 1.1 s, fully open at T (2.8 s): waking is not one smooth fade
		const float Len = T > 0.f ? T : 2.8f;
		const float K = Len < 1.4f ? Len / 2.8f : 1.f;
		LidKeys.Add(FVector2f(0.f, L0));
		LidKeys.Add(FVector2f(0.6f * K, 0.65f));
		LidKeys.Add(FVector2f(1.1f * K, 1.f));
		LidKeys.Add(FVector2f(Len, 0.f));
		// the scene comes in bright and settles (+0.5 EV, tau 1.5 s), peaking as the eye opens
		FExposurePulse P;
		P.Ev = WakeEv;
		P.Rise = 1.1f * K + 0.5f * (Len - 1.1f * K);
		P.Hold = 0.f;
		P.Tau = WakeEvTau;
		Pulses.Add(P);
		break;
	}
	case EYggLidMode::Sleep:
	{
		// one flutter on the way down: to 60% at 40% of the time, back to 30%, then closed
		const float Len = T > 0.f ? T : 2.5f;
		LidKeys.Add(FVector2f(0.f, L0));
		LidKeys.Add(FVector2f(0.4f * Len, FMath::Max(L0, 0.6f)));
		LidKeys.Add(FVector2f(0.6f * Len, FMath::Max(L0 * 0.5f, 0.3f)));
		LidKeys.Add(FVector2f(Len, 1.f));
		break;
	}
	default:
		break;
	}
	// closed holds and opening moves are leases, so the watchdog leaves them alone
	AcquireLease(Owner, EYggLease::Lids, 0.f);
	LidNoLeaseTime = 0.f;
	if (RigLog()) { UE_LOG(LogYggView, Display, TEXT("Rig: eyelids %d over %.2f s (beat %d)"), int32(Mode), T, Owner); }
	// t 0: at once, so the same story step can already teleport behind them
	if (LidKeys.Num() == 2 && LidKeys[1].X <= 0.f)
	{
		Lid = LidKeys[1].Y;
		TickLids(0.f);
	}
}

void UYggViewRigComponent::TickLids(float Dt)
{
	if (bLidMoving && LidKeys.Num() >= 2)
	{
		const float PrevT = LidT;
		LidT += Dt;
		// a long frame (hitch while spawning in the dark) must never step over a fully closed key:
		// hold on it for this frame so LidOnClosed runs behind shut lids
		for (const FVector2f& K : LidKeys)
		{
			if (K.Y >= 0.99f && K.X > PrevT && K.X < LidT) { LidT = K.X; break; }
		}
		const float End = LidKeys.Last().X;
		if (LidT >= End)
		{
			Lid = LidKeys.Last().Y;
			bLidMoving = false;
			// a blink or an opening is over: its lease goes; a closed hold keeps its lease until opened or expired
			if (LidMode == EYggLidMode::Blink)
			{
				QuietRelease(LidOwner, EYggLease::Lids);
			}
			else if (LidMode == EYggLidMode::Open || LidMode == EYggLidMode::Wake)
			{
				for (int32 i = Leases.Num() - 1; i >= 0; --i) { if (Leases[i].Kind == EYggLease::Lids) { Leases.RemoveAt(i); } }
				bLidRepair = false;
			}
		}
		else
		{
			int32 i = 0;
			while (i < LidKeys.Num() - 2 && LidT > LidKeys[i + 1].X) { ++i; }
			const float Span = LidKeys[i + 1].X - LidKeys[i].X;
			const float K = Span > 0.f ? (LidT - LidKeys[i].X) / Span : 1.f;
			Lid = FMath::Lerp(LidKeys[i].Y, LidKeys[i + 1].Y, Ease(K));
		}
	}
	Lid = FMath::Clamp(Lid, 0.f, 1.f);
	// the first fully closed frame: teleports, pose snaps and spawns happen behind the lids
	if (Lid >= 0.99f && LidOnClosed)
	{
		TFunction<void()> Run = MoveTemp(LidOnClosed);
		LidOnClosed = nullptr;
		Run();
	}
}

// ------------------------------------------------------------------------------------------------ postures

const FYggPoseDef& UYggViewRigComponent::PoseDef(EYggView V) const
{
	static const TCHAR* const Names[] = { TEXT("stand"), TEXT("lie"), TEXT("sit"), TEXT("kneel1"), TEXT("kneel2"), TEXT("crawl") };
	const FName Want(Names[FMath::Clamp(int32(V), 0, 5)]);
	for (const FYggPoseDef& D : Poses)
	{
		if (D.Name == Want) { return D; }
	}
	static const FYggPoseDef Fallback;
	return Fallback;
}

float UYggViewRigComponent::GetPostureDrop() const
{
	return FMath::Max(0.f, StandEyeCm - PoseH);
}

void UYggViewRigComponent::RequestPose(int32 Owner, const FYggPoseReq& R)
{
	// the fall and the fly-out own the eye; a knock does not: its offsets are additive and spring back on their own, and
	// the risky porter pass asks for kneel1 right after it (spec 6.9)
	if (Scripted == EYggScripted::Fall || Scripted == EYggScripted::Fly || bFlyActive)
	{
		UE_LOG(LogYggView, Warning, TEXT("Rig: pose %s (beat %d) ignored during the %s takeover"), ViewName(R.Pose), Owner, ScriptedName(bFlyActive ? EYggScripted::Fly : Scripted));
		return;
	}
	const float CurYaw = Char.IsValid() ? Norm(Char->GetControlRotation().Yaw) : AnchorYaw;
	const bool bBehindLids = Lid >= 0.99f || Clock - LastHardReset < 0.5 || Clock - LastCutTime < 0.5;
	UE_LOG(LogYggView, Display, TEXT("Rig: pose %s (beat %d)%s%s"), ViewName(R.Pose), Owner, R.bSnap ? TEXT(" snap") : TEXT(""), R.bAfterInput ? TEXT(" after input") : TEXT(""));
	if (R.Pose == EYggView::Walk)
	{
		bPending = false;
		if (R.bSnap && bBehindLids)
		{
			SnapPose(EYggView::Walk, CurYaw);
		}
		else if (View != EYggView::Walk || PoseH < StandEyeCm - 1.f)
		{
			StandUp(R.Seconds > 0.f ? R.Seconds : FMath::Max(PoseDef(View).UpS, 0.3f));
		}
		return;
	}
	if (R.bAfterInput)
	{
		bPending = true;
		Pending = R;
		PendingOwner = Owner;
		PendingSince = Clock;
		return;
	}
	if (R.bSnap)
	{
		if (bBehindLids)
		{
			// extras first (exit, prop), then the posture at once
			bHasExit = R.Exit.IsSet();
			if (bHasExit) { ExitPos = R.Exit.GetValue(); }
			ExitEv = R.ExitEv;
			PoseProp = !R.Prop.IsNone() ? R.Prop : PoseDef(R.Pose).Prop;
			ShowProp(PoseProp, true);
			SnapPose(R.Pose, R.FaceYaw.IsSet() ? Norm(R.FaceYaw.GetValue()) : CurYaw);
			if (R.Pose == EYggView::Crawl && bHasExit) { StartCrawl(); PoseH = PoseTo; Phase = EPhase::Holding; }
			return;
		}
		UE_LOG(LogYggView, Warning, TEXT("Rig: pose snap with the eyes open (beat %d) - played over its time instead"), Owner);
	}
	if (View == EYggView::Walk && Speed2D >= PoseEnterMaxSpeedCmS)
	{
		// the climber stops first (walk speed is shaped to 0 meanwhile), then the posture starts
		bPending = true;
		Pending = R;
		Pending.bSnap = false;
		PendingOwner = Owner;
		PendingSince = Clock;
		return;
	}
	EnterPose(Owner, R);
}

void UYggViewRigComponent::EnterPose(int32 Owner, const FYggPoseReq& R)
{
	ACharacter* C = Char.Get();
	if (!C)
	{
		return;
	}
	const FYggPoseDef& D = PoseDef(R.Pose);
	const float CurYaw = Norm(C->GetControlRotation().Yaw);
	const float Anchor = R.FaceYaw.IsSet() ? Norm(R.FaceYaw.GetValue()) : CurYaw;
	bHasExit = R.Exit.IsSet();
	if (bHasExit) { ExitPos = R.Exit.GetValue(); }
	ExitEv = R.ExitEv;
	PoseProp = !R.Prop.IsNone() ? R.Prop : D.Prop;
	if (R.Pose == EYggView::Crawl && bHasExit)
	{
		ShowProp(PoseProp, true);
		StartCrawl();
		return;
	}
	// the height: down at the new posture's pace, up at the old posture's (lie -> sit takes lie's 2.2 s)
	PoseFrom = PoseH;
	PoseTo = D.EyeCm;
	PoseT = 0.f;
	const bool bDown = PoseTo < PoseFrom;
	const float Own = bDown ? D.DownS : (View == EYggView::Walk ? D.DownS : PoseDef(View).UpS);
	PoseLen = FMath::Max(R.Seconds > 0.f ? R.Seconds : Own, 0.05f);
	View = R.Pose;
	Phase = EPhase::Entering;
	ShowProp(PoseProp, true);
	// the body turns to the face over 0.6 s (capsule yaw, the controller no longer turns it); the look limits come in
	// over 1 s from wherever the view is, so it never snaps
	SetCtrlYawOwned(true);
	bBodyTurning = true;
	BodyTurnFrom = Norm(C->GetActorRotation().Yaw);
	BodyTurnTo = Anchor;
	BodyTurnT = 0.f;
	bCatchUp = false;
	bDeadZoneTurning = false;
	SetLimitsTo(D.PitchMin, D.PitchMax, D.YawHalf, Anchor, false);
	MoveHeld = 0.f;
	ShuffleTime = 0.f;
	if (RigLog()) { UE_LOG(LogYggView, Display, TEXT("Rig: entering %s over %.1f s (beat %d, eye %.0f -> %.0f cm)"), ViewName(View), PoseLen, Owner, PoseFrom, PoseTo); }
}

void UYggViewRigComponent::SnapPose(EYggView V, float Yaw)
{
	const FYggPoseDef& D = PoseDef(V);
	View = V;
	Phase = EPhase::Holding;
	PoseH = PoseFrom = PoseTo = V == EYggView::Walk ? StandEyeCm : D.EyeCm;
	PoseT = PoseLen = 0.f;
	GazeBend = 0.f;
	bBodyTurning = false;
	bCatchUp = false;
	bDeadZoneTurning = false;
	MoveHeld = 0.f;
	SetLimitsTo(D.PitchMin, D.PitchMax, D.YawHalf, Norm(Yaw), true);
	if (V != EYggView::Walk)
	{
		SetCtrlYawOwned(true);
		SetBodyYaw(AnchorYaw);
	}
	else
	{
		SetCtrlYawOwned(false);
		ShowProp(ShownProp, false);
		bHasExit = false;
		if (UCharacterMovementComponent* Move = Char.IsValid() ? Char->GetCharacterMovement() : nullptr)
		{
			if (Move->bConstrainToPlane) { Move->SetPlaneConstraintEnabled(false); }
		}
	}
}

void UYggViewRigComponent::StandUp(float Seconds)
{
	if (View == EYggView::Walk && Phase != EPhase::Entering)
	{
		return;
	}
	bPending = false;
	Phase = EPhase::Leaving;
	PoseFrom = PoseH;
	PoseTo = StandEyeCm;
	PoseT = 0.f;
	PoseLen = FMath::Max(Seconds, 0.1f);
	GazeBend = 0.f;
	if (View == EYggView::Walk)
	{
		Phase = EPhase::Entering;   // already standing, only the last part of the rise is left
	}
	if (RigLog()) { UE_LOG(LogYggView, Display, TEXT("Rig: standing up from %s over %.1f s"), ViewName(View), PoseLen); }
}

void UYggViewRigComponent::StartCrawl()
{
	ACharacter* C = Char.Get();
	UCharacterMovementComponent* Move = C ? C->GetCharacterMovement() : nullptr;
	if (!C || !Move)
	{
		return;
	}
	const FYggPoseDef& D = PoseDef(EYggView::Crawl);
	View = EYggView::Crawl;
	Phase = EPhase::Entering;
	PoseFrom = PoseH;
	PoseTo = D.EyeCm;
	PoseT = 0.f;
	PoseLen = FMath::Max(D.DownS, 0.05f);
	// along the line from here to the exit (through the tent door): a plane constraint, so the crawl can't slide off it
	CrawlDir = (ExitPos - C->GetActorLocation()).GetSafeNormal2D();
	if (CrawlDir.IsNearlyZero()) { CrawlDir = FRotator(0.f, AnchorYaw, 0.f).Vector(); }
	const float Yaw = Norm(CrawlDir.Rotation().Yaw);
	Move->SetPlaneConstraintNormal(FVector::CrossProduct(CrawlDir, FVector::UpVector).GetSafeNormal());
	Move->SetPlaneConstraintOrigin(C->GetActorLocation());
	Move->SetPlaneConstraintEnabled(true);
	SetCtrlYawOwned(true);
	bBodyTurning = true;
	BodyTurnFrom = Norm(C->GetActorRotation().Yaw);
	BodyTurnTo = Yaw;
	BodyTurnT = 0.f;
	SetLimitsTo(D.PitchMin, D.PitchMax, D.YawHalf, Yaw, false);
	CrawlSince = Clock;
	CrawlTravel = 0.f;
	bCrawlFallback = false;
	MoveHeld = 0.f;
	UE_LOG(LogYggView, Display, TEXT("Rig: crawling toward the exit (%.0f cm away)"), float(FVector::Dist2D(ExitPos, C->GetActorLocation())));
}

void UYggViewRigComponent::BecomeWalk()
{
	View = EYggView::Walk;
	Phase = PoseH < PoseTo - 0.5f ? EPhase::Entering : EPhase::Holding;   // the last part of the rise goes on
	ShowProp(ShownProp, false);
	bHasExit = false;
	if (UCharacterMovementComponent* Move = Char.IsValid() ? Char->GetCharacterMovement() : nullptr)
	{
		if (Move->bConstrainToPlane) { Move->SetPlaneConstraintEnabled(false); }
	}
	const FYggPoseDef& D = PoseDef(EYggView::Walk);
	SetLimitsTo(D.PitchMin, D.PitchMax, 180.f, AnchorYaw, false);
	// the body turns to the view (360 deg/s, at most 0.5 s) before the controller drives it again
	bBodyTurning = false;
	bCatchUp = true;
	CatchUpT = 0.f;
	CatchUpRate = StandBodyTurnDegS;
	if (RigLog()) { UE_LOG(LogYggView, Display, TEXT("Rig: walking again (eye %.0f cm)"), PoseH); }
}

void UYggViewRigComponent::SetLimitsTo(float MinP, float MaxP, float YawHalf, float NewAnchor, bool bSnap)
{
	const ACharacter* C = Char.Get();
	const FRotator Ctrl = C ? C->GetControlRotation() : FRotator::ZeroRotator;
	const float CurPitch = Norm(Ctrl.Pitch);
	const float CurOff = FMath::Abs(Norm(Ctrl.Yaw - NewAnchor));
	LimToMin = MinP;
	LimToMax = MaxP;
	LimToYaw = FMath::Clamp(YawHalf, 1.f, 180.f);
	if (bSnap)
	{
		LimPitchMin = LimFromMin = MinP;
		LimPitchMax = LimFromMax = MaxP;
		LimYawHalf = LimFromYaw = LimToYaw;
		AnchorYaw = NewAnchor;
		LimLen = 0.f;
		return;
	}
	// from the current limits, widened to hold the current view: the limits close in, the view is never snapped
	const float OldAround = LimYawHalf >= 179.f ? 180.f : FMath::Min(180.f, LimYawHalf + FMath::Abs(Norm(NewAnchor - AnchorYaw)));
	LimFromMin = FMath::Min(LimPitchMin, CurPitch);
	LimFromMax = FMath::Max(LimPitchMax, CurPitch);
	LimFromYaw = FMath::Max(OldAround, CurOff);
	AnchorYaw = NewAnchor;
	// limits that open up open at once (standing up and turning round must not hit an invisible wall); only closing ones tween
	if (MinP <= LimFromMin) { LimFromMin = LimPitchMin = MinP; }
	if (MaxP >= LimFromMax) { LimFromMax = LimPitchMax = MaxP; }
	if (LimToYaw >= LimFromYaw) { LimFromYaw = LimYawHalf = LimToYaw; }
	// a bigger gap gets a longer tween: the view is never dragged faster than LimitDragMaxDegS
	const float Drag = FMath::Max3(FMath::Max(0.f, MinP - CurPitch), FMath::Max(0.f, CurPitch - MaxP), LimToYaw < 179.f ? FMath::Max(0.f, CurOff - LimToYaw) : 0.f);
	LimLen = FMath::Max(LimitTweenSec, 1.5f * Drag / FMath::Max(LimitDragMaxDegS, 1.f));
	LimT = 0.f;
}

void UYggViewRigComponent::TickLimits(float Dt)
{
	if (LimLen <= 0.f)
	{
		return;
	}
	LimT += Dt;
	const float A = Ease(LimT / LimLen);
	LimPitchMin = FMath::Lerp(LimFromMin, LimToMin, A);
	LimPitchMax = FMath::Lerp(LimFromMax, LimToMax, A);
	LimYawHalf = FMath::Lerp(LimFromYaw, LimToYaw, A);
	if (LimT >= LimLen)
	{
		LimPitchMin = LimToMin;
		LimPitchMax = LimToMax;
		LimYawHalf = LimToYaw;
		LimLen = 0.f;
	}
}

void UYggViewRigComponent::ShowProp(FName Tag, bool bShow)
{
	UWorld* World = GetWorld();
	if (Tag.IsNone() || !World)
	{
		return;
	}
	if (bShow && !ShownProp.IsNone() && ShownProp != Tag)
	{
		ShowProp(ShownProp, false);
	}
	if (bShow == (ShownProp == Tag))
	{
		return;
	}
	// once per posture change, not per frame
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		if (!It->ActorHasTag(Tag))
		{
			continue;
		}
		It->SetActorHiddenInGame(!bShow);
		if (bShow)
		{
			// the world builder also hides the components themselves: show them too
			TInlineComponentArray<UPrimitiveComponent*> Prims(*It);
			for (UPrimitiveComponent* P : Prims) { P->SetVisibility(true); P->SetHiddenInGame(false); }
		}
	}
	ShownProp = bShow ? Tag : NAME_None;
}

void UYggViewRigComponent::SetCtrlYawOwned(bool bOwned)
{
	ACharacter* C = Char.Get();
	if (!C || bOwned == bCtrlYawOwned)
	{
		return;
	}
	if (bOwned && !bSavedCtrlYaw)
	{
		return;   // the body never followed the controller: nothing to take over
	}
	C->bUseControllerRotationYaw = bOwned ? false : bSavedCtrlYaw;
	bCtrlYawOwned = bOwned;
}

void UYggViewRigComponent::SetBodyYaw(float Yaw)
{
	ACharacter* C = Char.Get();
	if (C && FMath::Abs(Norm(Yaw - C->GetActorRotation().Yaw)) > 0.01f)
	{
		C->SetActorRotation(FRotator(0.f, Norm(Yaw), 0.f));
	}
}

void UYggViewRigComponent::TickPosture(float Dt)
{
	ACharacter* C = Char.Get();
	// move input this frame (consumed by the movement already); a Move lease means it must not stand you up
	const bool bMoveLease = HasLease(EYggLease::Move);
	// an inspect card or a takeover owns the moment: a held move key must not stand you up or crawl you off under it
	const bool bMoveIn = !bMoveLease && !bInspect && !HasLease(EYggLease::Inspect) && Scripted == EYggScripted::None
		&& !bFlyActive && !bFlyWaiting && C->GetLastMovementInputVector().SizeSquared() > 0.04f;
	MoveHeld = bMoveIn ? MoveHeld + Dt : 0.f;

	// a request waiting for input (lie -> sit), or for the climber to stop
	if (bPending)
	{
		bool bGo;
		if (Pending.bAfterInput)
		{
			const bool bInput = bMoveIn || LastLookInput > PendingSince + 0.05;
			bGo = bInput || (Pending.Auto > 0.f && Clock - PendingSince >= Pending.Auto);
		}
		else
		{
			bGo = Speed2D < PoseEnterMaxSpeedCmS || Clock - PendingSince > 3.0;
		}
		if (bGo)
		{
			bPending = false;
			FYggPoseReq R = Pending;
			R.bAfterInput = false;
			EnterPose(PendingOwner, R);
			MoveHeld = 0.f;
		}
	}

	// the height curve: an S that does 70% of the change in the first 60% of the time, then a small settle
	if (Phase != EPhase::Holding)
	{
		PoseT += Dt;
		const float U = PoseLen > 0.f ? FMath::Clamp(PoseT / PoseLen, 0.f, 1.f) : 1.f;
		const float Delta = PoseTo - PoseFrom;
		float H = PoseFrom + Delta * Ease(FMath::Pow(U, PoseCurveExp));
		const bool bSettle = FMath::Abs(Delta) > 10.f && PoseSettleCm > 0.f;
		const float SettleLen = 0.2f * PoseLen + 0.3f;
		if (bSettle)
		{
			const float S = (PoseT - 0.8f * PoseLen) / SettleLen;
			if (S > 0.f && S < 1.f) { H += FMath::Sign(Delta) * PoseSettleCm * FMath::Sin(PI * S); }
		}
		PoseH = H;
		// on the way down the gaze bends a little toward where you go down (view only)
		GazeBend = Delta < -10.f && Phase == EPhase::Entering ? GazeBendDeg * FMath::Sin(PI * U) : 0.f;
		if (Phase == EPhase::Leaving && View != EYggView::Walk && PoseH >= WalkResumeFrac * StandEyeCm)
		{
			BecomeWalk();
		}
		if (PoseT >= (bSettle ? 0.8f * PoseLen + SettleLen : PoseLen))
		{
			PoseH = PoseTo;
			GazeBend = 0.f;
			Phase = View == EYggView::Walk || Phase == EPhase::Entering ? EPhase::Holding : Phase;
			if (Phase == EPhase::Leaving)
			{
				BecomeWalk();
				Phase = EPhase::Holding;
			}
		}
	}

	// what the player's moves do while posed
	switch (View)
	{
	case EYggView::Lie:
		if (Phase == EPhase::Holding && !bPending && MoveHeld >= StandOnMoveHoldSec)
		{
			FYggPoseReq R;
			R.Pose = EYggView::Sit;
			R.FaceYaw = AnchorYaw;
			R.Prop = ShownProp;
			EnterPose(-1, R);
			MoveHeld = 0.f;
		}
		break;
	case EYggView::Sit:
	case EYggView::Kneel1:
	case EYggView::Kneel2:
		if (MoveHeld >= StandOnMoveHoldSec && !bPending && Phase != EPhase::Leaving)
		{
			// a sit with an exit (the tent) never stands straight up through the roof: it crawls once the sit-up is done;
			// while still sitting up MoveHeld is kept, so the crawl starts the moment the pose holds
			if (View == EYggView::Sit && bHasExit)
			{
				if (Phase == EPhase::Holding) { StartCrawl(); MoveHeld = 0.f; }
			}
			else
			{
				StandUp(FMath::Max(PoseDef(View).UpS, 0.3f));
				MoveHeld = 0.f;
			}
		}
		break;
	case EYggView::Crawl:
		if (Phase != EPhase::Leaving && bHasExit)
		{
			CrawlTravel += Speed2D * Dt;
			if (FVector::Dist2D(C->GetActorLocation(), ExitPos) <= CrawlExitRadiusCm)
			{
				// out of the tent: standing up is the reveal of the camp (with the dawn's exposure overshoot)
				StandUp(CrawlStandSec);
				if (FMath::Abs(ExitEv) > 0.001f)
				{
					FExposurePulse P;
					P.Ev = ExitEv;
					P.Rise = 0.4f;
					P.Hold = 0.f;
					P.Tau = CrawlExitEvTau;
					Pulses.Add(P);
				}
				UE_LOG(LogYggView, Display, TEXT("Rig: at the exit, standing up"));
			}
			else if (!bCrawlFallback && Clock - CrawlSince > CrawlFallbackSec)
			{
				// 90 s without reaching the exit: a blink, and the climber stands at the exit behind the lids
				bCrawlFallback = true;
				const FVector Dest = ExitPos;
				const float Yaw = Norm(CrawlDir.Rotation().Yaw);
				SetLids(-2, EYggLidMode::Blink, BlinkCloseSec, 0.f, [this, Dest, Yaw]()
				{
					ACharacter* Ch = Char.Get();
					if (!Ch) { return; }
					const float Half = Ch->GetCapsuleComponent() ? Ch->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() : 96.f;
					Ch->TeleportTo(Dest + FVector(0.f, 0.f, Half + 5.f), FRotator(0.f, Yaw, 0.f), false, false);
					Cut();
					HardReset(FRotator(0.f, Yaw, 0.f), EYggView::Walk);
					Repair(TEXT("crawl: the exit was not reached in 90 s, teleported there behind a blink"), EYggLease::Move);
				});
			}
		}
		break;
	default:
		break;
	}
}

void UYggViewRigComponent::TickBodyYaw(float Dt)
{
	ACharacter* C = Char.Get();
	const UClimbingComponent* Climb = Cast<UClimbingComponent>(Climbing.Get());
	if (!C || (Climb && Climb->IsClimbing()))
	{
		return;   // climbing holds the body to the face itself
	}
	// the rig is the one owner of the flag (spec 5.5): letting go of a climb hands back "true" even while the rig holds
	// the body (posed, dead zone), so it is put back here
	if (bCtrlYawOwned && C->bUseControllerRotationYaw)
	{
		C->bUseControllerRotationYaw = false;
	}
	const float CtrlYaw = Norm(C->GetControlRotation().Yaw);
	const float BodyYaw = Norm(C->GetActorRotation().Yaw);
	if (View != EYggView::Walk)
	{
		// posed: the body turns to the face, then holds the anchor (the sit shuffle moves it)
		SetCtrlYawOwned(true);
		if (bBodyTurning)
		{
			BodyTurnT += Dt;
			const float A = Ease(BodyTurnT / FMath::Max(BodyTurnToFaceSec, 0.01f));
			SetBodyYaw(BodyTurnFrom + Norm(BodyTurnTo - BodyTurnFrom) * A);
			if (A >= 1.f) { bBodyTurning = false; }
		}
		else if (View != EYggView::Crawl)
		{
			SetBodyYaw(AnchorYaw);
		}
		return;
	}
	if (bCatchUp)
	{
		CatchUpT += Dt;
		const float Err = Norm(CtrlYaw - BodyYaw);
		const float Step = CatchUpRate * Dt;
		if (FMath::Abs(Err) <= Step + 0.5f || CatchUpT >= StandBodyTurnMaxSec)
		{
			bCatchUp = false;
			SetCtrlYawOwned(false);   // the controller drives the body again (the rest of the turn, if any, lands now)
		}
		else
		{
			SetBodyYaw(BodyYaw + FMath::Sign(Err) * Step);
		}
		return;
	}
	// turn dead zone (P1): standing still, the legs stay put until the view is more than 45 deg off, then step round
	const bool bDeadZone = CVarViewDeadZone.GetValueOnGameThread() != 0 && bSavedCtrlYaw && bOnGround && Speed2D < DeadZoneSpeedCmS
		&& HandsItem.IsNone() && Phase == EPhase::Holding && Scripted == EYggScripted::None && bRigOn;
	if (bDeadZone)
	{
		SetCtrlYawOwned(true);
		const float Err = Norm(CtrlYaw - BodyYaw);
		if (FMath::Abs(Err) > DeadZoneDeg) { bDeadZoneTurning = true; }
		if (bDeadZoneTurning)
		{
			const float Step = DeadZoneCatchUpDegS * Dt;
			if (FMath::Abs(Err) <= Step) { SetBodyYaw(CtrlYaw); bDeadZoneTurning = false; }
			else { SetBodyYaw(BodyYaw + FMath::Sign(Err) * Step); }
		}
	}
	else if (bCtrlYawOwned)
	{
		// walking again: the body catches up quickly, then the controller has it again
		bDeadZoneTurning = false;
		bCatchUp = true;
		CatchUpT = 0.f;
		CatchUpRate = 720.f;
	}
}

// ------------------------------------------------------------------------------------------------ bob, breath, landing

void UYggViewRigComponent::TickBob(float Dt)
{
	const float Scale = HeadMotion * FMath::Clamp(CVarViewBob.GetValueOnGameThread(), 0.f, 2.f);
	const float RollOn = HeadMotion >= 0.99f ? 1.f : 0.f;   // Reduced head motion: no roll
	const UClimbingComponent* Climb = Cast<UClimbingComponent>(Climbing.Get());
	const bool bClimbing = Climb && Climb->IsClimbing();
	if (View == EYggView::Crawl && Phase != EPhase::Leaving)
	{
		// a small hand-plant dip every 40 cm
		const float Z = -CrawlDipCm * 0.5f * (1.f - FMath::Cos(2.f * PI * CrawlTravel / FMath::Max(CrawlDipEveryCm, 1.f)));
		BobZ = FMath::FInterpTo(BobZ, Z * Scale, Dt, 40.f);
		BobY = FMath::FInterpTo(BobY, 0.f, Dt, 10.f);
		BobRoll = FMath::FInterpTo(BobRoll, 0.f, Dt, 10.f);
	}
	else if (View == EYggView::Walk && bOnGround && !bClimbing)
	{
		// no step heard for a while but walking (clips without notifies, no snow trail): steps every half stride
		if (Clock - LastRealStepTime > BobFallbackAfterSec && Speed2D > BobFallbackSpeedCmS)
		{
			FallbackDist += Speed2D * Dt;
			if (FallbackDist >= 0.5f * BobFallbackStrideCm)
			{
				FallbackDist = 0.f;
				const bool bNextLeft = FMath::Fmod(StepBase + 1.0, 2.0) < 0.5;
				DoStep(bNextLeft, Speed2D, 0, true);
			}
		}
		const float Since = float(Clock - LastStepTime);
		// the running phase chases "last step + time since / period": locked to the crunch, free-running between steps
		const double Target = StepBase + double(Since / StepPeriod);
		BobPsi += double(Dt / StepPeriod);
		BobPsi += (Target - BobPsi) * double(FMath::Min(1.f, 8.f * Dt));
		const bool bIdle = Since > BobDecayAfterPeriods * StepPeriod;
		BobAmp = bIdle ? BobAmp * FMath::Pow(0.5f, Dt / FMath::Max(BobDecayHalfLife, 0.01f)) : FMath::FInterpTo(BobAmp, 1.f, Dt, 8.f);
		RestW = FMath::FInterpTo(RestW, Speed2D < RestStepBelowCmS ? 1.f : 0.f, Dt, 4.f);
		const float Frac2 = float(FMath::Fmod(BobPsi, 2.0));
		// low point just after the crunch, high point at mid-stance
		const float Cosine = -0.5f * StepA * FMath::Cos(2.f * PI * (Frac2 - BobTroughPhase));
		// rest-step: a dip and a hold instead of a wave (slowing down is visibly rewarded)
		const float DipT = RestStepDipSec, HoldT = RestStepHoldSec, RiseT = RestStepRiseSec;
		const float Env = Since < DipT ? Ease(Since / DipT) : Since < DipT + HoldT ? 1.f : Since < DipT + HoldT + RiseT ? 1.f - Ease((Since - DipT - HoldT) / RiseT) : 0.f;
		float Z = FMath::Lerp(Cosine, -RestStepDipCm * Env, RestW) * BobAmp;
		const float RT = float(Clock - RockDipAt);
		if (RT < 0.28f) { Z -= RockDipCm * (RT < 0.08f ? Ease(RT / 0.08f) : 1.f - Ease((RT - 0.08f) / 0.2f)); }
		// over the planted foot: even steps are left plants (the eye shifts left, a hair of left roll)
		const float Side = FMath::Sin(PI * Frac2);
		const float Lat = -0.5f * StepL * Side * BobAmp;
		const float Rol = -StepR * Side * BobAmp * RollOn;
		BobZ = FMath::FInterpTo(BobZ, Z * Scale, Dt, 40.f);
		BobY = FMath::FInterpTo(BobY, Lat * Scale, Dt, 40.f);
		BobRoll = FMath::FInterpTo(BobRoll, Rol * Scale, Dt, 40.f);
	}
	else
	{
		BobAmp *= FMath::Pow(0.5f, Dt / FMath::Max(BobDecayHalfLife, 0.01f));
		BobZ = FMath::FInterpTo(BobZ, 0.f, Dt, 6.f);
		BobY = FMath::FInterpTo(BobY, 0.f, Dt, 6.f);
		BobRoll = FMath::FInterpTo(BobRoll, 0.f, Dt, 6.f);
	}
	// the eyes hold a point 10 m ahead steady
	BobPitch = -FMath::RadiansToDegrees(FMath::Atan(BobZ / FMath::Max(FixationCm, 100.f)));
}

void UYggViewRigComponent::TickBreath(float Dt)
{
	// the breathing value B: effort (the breath recordings follow the same number, so the sound leads), grown with
	// altitude, with a floor in the death zone; the plume's Breath lease forces it
	const float Effort = Audio.IsValid() ? Audio->GetEffort() : 0.f;
	const float AltGain = 0.6f + 0.4f * FMath::Clamp((AltM - AltGainFromM) / FMath::Max(AltGainRangeM, 1.f), 0.f, 1.f);
	const float Floor = BreathFloorMax * FMath::Clamp((AltM - BreathFloorFromM) / FMath::Max(BreathFloorRangeM, 1.f), 0.f, 1.f);
	const float Want = Effort * AltGain;
	BSmooth = ExpTo(BSmooth, Want, Want > BSmooth ? BreathRiseTau : BreathFallTau * (0.5f + AltGain), Dt);
	float B = FMath::Max(Floor, BSmooth);
	if (StoryB >= 0.f) { B = StoryB; }
	// below the threshold the head is effectively still (the 0.2-0.4 Hz sickness band stays nearly empty); above it
	// panting at 0.5-0.75 Hz, never any roll
	const bool bLow = B < BreathThreshold;
	const float FWant = bLow ? BreathIdleHz : BreathHzBase + BreathHzPerB * B;
	const float PWant = bLow ? BreathIdlePitchDeg : BreathPitchPerB * B;
	const float ZWant = bLow ? BreathIdleZCm : BreathZPerB * B;
	BFreq = FMath::FInterpTo(BFreq, FWant, Dt, 1.f);
	BAmpPitch = FMath::FInterpTo(BAmpPitch, PWant, Dt, 1.5f);
	BAmpZ = FMath::FInterpTo(BAmpZ, ZWant, Dt, 1.5f);
	BPhase = FMath::Fmod(BPhase + BFreq * Dt, 1.f);
	const float BreathOn = HeadMotion >= 0.99f ? FMath::Clamp(CVarViewBreath.GetValueOnGameThread(), 0.f, 2.f) : 0.f;   // Reduced / Off: no breathing motion
	const float W = FMath::Sin(2.f * PI * BPhase);
	BreathPitch = BAmpPitch * W * BreathOn;
	BreathZ = BAmpZ * W * BreathOn;

	// spent: the head hangs (eased over 4 s) with an irregular slow sway, never a continuous roll
	ExhaustHang = ExpTo(ExhaustHang, FMath::Clamp(FeedExhaust * 2.f, 0.f, 1.f), ExhaustTau, Dt);
	SwayClock += Dt;
	const float S = ExhaustSwayDeg * ExhaustHang * HeadMotion;
	SwayP = S * (0.6f * FMath::Sin(2.f * PI * 0.63f * SwayClock + 0.4f) + 0.4f * FMath::Sin(2.f * PI * 0.87f * SwayClock + 2.1f));
	SwayY = 0.7f * S * (0.6f * FMath::Sin(2.f * PI * 0.71f * SwayClock + 1.3f) + 0.4f * FMath::Sin(2.f * PI * 0.9f * SwayClock + 4.f));

	// shivering and rumbles: band-limited 5-8 Hz, capped at a quarter of the frame rate so it can't alias into jitter
	const float FpsCap = Dt > KINDA_SMALL_NUMBER ? ShiverFpsFrac / Dt : ShiverHzMax;
	const float Hi = FMath::Max(FMath::Min(ShiverHzMax, FpsCap), 0.5f);
	const float Lo = FMath::Min(ShiverHzMin, Hi);
	for (int32 i = 0; i < 2; ++i) { ShivFreq[i] = FMath::Clamp(ShivFreq[i] + FMath::FRandRange(-1.f, 1.f) * 4.f * Dt, Lo, Hi); }
	ShivPhase[0] = FMath::Fmod(ShivPhase[0] + ShivFreq[0] * Dt, 1.f);
	ShivPhase[1] = FMath::Fmod(ShivPhase[1] + ShivFreq[1] * Dt, 1.f);
	ShivPhase[2] = FMath::Fmod(ShivPhase[2] + ShivFreq[1] * 0.93f * Dt, 1.f);
	ShivPhase[3] = FMath::Fmod(ShivPhase[3] + ShivFreq[0] * 1.07f * Dt, 1.f);
	const float NP = 0.6f * FMath::Sin(2.f * PI * ShivPhase[0]) + 0.4f * FMath::Sin(2.f * PI * ShivPhase[1]);
	const float NY = 0.6f * FMath::Sin(2.f * PI * ShivPhase[2]) + 0.4f * FMath::Sin(2.f * PI * ShivPhase[3]);
	const float Sh = ShiverDeg * FeedShiver;
	ShivP = Sh * NP;
	ShivY = 0.7f * Sh * NY;
	ShakeP = ShakeY = 0.f;
	if (ShakeLen > 0.f)
	{
		ShakeT += Dt;
		const float Left = FMath::Max(0.f, 1.f - ShakeT / ShakeLen);
		const float A = ShakeDeg * Left * Left;
		ShakeP = A * NY;
		ShakeY = 0.8f * A * NP;
		if (Left <= 0.f) { ShakeLen = 0.f; }
	}
}

void UYggViewRigComponent::TickLandingAndNods(float Dt)
{
	// landing: down in 0.1 s, then a damped spring back (slower above 5,000 m)
	if (LandDownT >= 0.f)
	{
		LandDownT += Dt;
		const float K = FMath::Clamp(LandDownT / FMath::Max(LandDownSec, 0.01f), 0.f, 1.f);
		LandZ = FMath::Lerp(LandStart, LandTarget, Ease(K));
		if (K >= 1.f) { LandDownT = -1.f; LandV = 0.f; }
	}
	else
	{
		const float W = LandOmega / (AltM > LandSlowAboveM ? FMath::Max(LandSlowFactor, 1.f) : 1.f);
		DampedSpring(LandZ, LandV, W, LandZeta, Dt);
	}
	// a small nod down on the first step, up on the last
	const bool bMoving = bWasMoving ? Speed2D > StartStopSpeedCmS - 5.f : Speed2D > StartStopSpeedCmS + 5.f;
	if (bMoving != bWasMoving && View == EYggView::Walk && bOnGround && Scripted == EYggScripted::None)
	{
		NodV += (bMoving ? -1.f : 1.f) * NodImpulse(StartStopNodDeg, StartStopOmega, StartStopZeta) * HeadMotion;
	}
	bWasMoving = bMoving;
	DampedSpring(NodX, NodV, StartStopOmega, StartStopZeta, Dt);
}

// ------------------------------------------------------------------------------------------------ eye

void UYggViewRigComponent::ComposeEye(float Dt)
{
	ACharacter* C = Char.Get();
	UCapsuleComponent* Cap = C ? C->GetCapsuleComponent() : nullptr;
	const UCharacterMovementComponent* Move = C ? C->GetCharacterMovement() : nullptr;
	if (!Cap || !Move)
	{
		return;
	}
	const FVector CapLoc = Cap->GetComponentLocation();
	const float Half = Cap->GetScaledCapsuleHalfHeight();
	const float CapBottom = float(CapLoc.Z) - Half;
	const float FloorZ = CapBottom - (Move->CurrentFloor.bBlockingHit ? Move->CurrentFloor.FloorDist : 0.f);

	// terrain: the powder sink once, half the slope's hips drop (capped), through a critically damped spring that smooths
	// step-ups and rock edges (the posture curves bypass it; HardReset snaps it)
	const UFirstPersonBodyComponent* B = Body.Get();
	const float Sink = B ? B->GetSinkDepth() : 0.f;
	const float Drop = B ? B->GetFootDrop() : 0.f;
	FootDropSm = ExpTo(FootDropSm, Drop, Drop > FootDropSm ? FootDropTauDown : FootDropTauUp, Dt);
	// the powder sink reaches the eye only up to SinkEyeMaxCm: the full 30-35 cm sink at the camp held the walking eye at
	// 120 cm, 'the camera is at my feet' (flight recorder, 2026-11-20). The legs still sink; the head does not follow all the way.
	const float Goal = CapBottom - FMath::Min(Sink * SinkToEye, SinkEyeMaxCm) - FMath::Min(FootDropSm * FootDropToEye, FootDropMaxCm);
	if (!bTerrainInit)
	{
		TerrainZ = Goal;
		TerrainV = 0.f;
		bTerrainInit = true;
	}
	CriticalSpring(TerrainZ, TerrainV, Goal, 2.f * PI * FMath::Max(TerrainSpringHz, 0.1f), Dt);
	TerrainZ = FMath::Clamp(TerrainZ, Goal - TerrainMaxLagCm, Goal + TerrainMaxLagCm);

	// the stoops: slope and pack only when walking
	const bool bWalk = View == EYggView::Walk;
	const float Up = FMath::Clamp((GradeDeg - SlopeFromDeg) / FMath::Max(SlopeFullDeg - SlopeFromDeg, 1.f), 0.f, (SlopeCapDeg - SlopeFromDeg) / FMath::Max(SlopeFullDeg - SlopeFromDeg, 1.f));
	const float Dn = FMath::Clamp((-GradeDeg - SlopeFromDeg) / FMath::Max(SlopeFullDeg - SlopeFromDeg, 1.f), 0.f, 1.f);
	SlopeFwdSm = ExpTo(SlopeFwdSm, bWalk ? SlopeUpFwdCm * Up - SlopeDownBackCm * Dn : 0.f, SlopeTau, Dt);
	SlopeDownSm = ExpTo(SlopeDownSm, bWalk ? SlopeUpDownCm * Up : 0.f, SlopeTau, Dt);
	PackStoop = ExpTo(PackStoop, bWalk ? FMath::Clamp((FeedPackKg - PackFromKg) * PackCmPerKg, 0.f, PackMaxCm) : 0.f, PackTau, Dt);

	const float HRaw = PoseH - PackStoop - SlopeDownSm - ExhaustHangCm * ExhaustHang;
	EyeAboveCapRaw = TerrainZ + HRaw - CapBottom;
	const float H = HRaw + WatchdogLift;

	const FRotator Ctrl = C->GetControlRotation();
	const float Pitch = Norm(Ctrl.Pitch);
	const float Yaw = Norm(Ctrl.Yaw);
	const FRotator YawRot(0.f, Yaw, 0.f);
	const FVector Fwd = YawRot.Vector();
	const FVector Right = FRotationMatrix(YawRot).GetUnitAxis(EAxis::Y);
	FVector E(CapLoc.X, CapLoc.Y, TerrainZ + H);
	E += Fwd * SlopeFwdSm;
	// neck: the eye sits in front of and above the neck pivot, so looking down brings it forward and down
	const float NeckK = FMath::Clamp(CVarViewNeck.GetValueOnGameThread(), 0.f, 2.f);
	const FVector N = NeckScale * NeckK * FVector(NeckFwdCm, 0.f, NeckUpCm);
	E += FRotator(Pitch, Yaw, 0.f).RotateVector(N) - YawRot.RotateVector(N);
	// the eyes sit in front of the neck: on the capsule axis the camera was inside the jacket collar ring whenever the
	// player looked down (2026-11-20 check). Walking and kneeling only; takeovers place the eye themselves.
	if (!InTakeover()) { E += Fwd * CVarViewEyeFwd.GetValueOnGameThread() * FMath::Clamp(H / StandEyeCm, 0.5f, 1.2f); }
	// waist: a steep look down leans over the chest to the boots, not into the collar
	const float Wl = FMath::SmoothStep(WaistLeanFromDeg, FMath::Max(WaistLeanFullDeg, WaistLeanFromDeg + 1.f), -Pitch) * NeckK;
	E += Fwd * (WaistLeanFwdCm * Wl) - FVector::UpVector * (WaistLeanDownCm * Wl);
	E.Z += BobZ + BreathZ + LandZ;
	E += Right * BobY;
	E += Fwd * TakeLoc.X + Right * TakeLoc.Y + FVector::UpVector * TakeLoc.Z;

	// invariants (spec 4.6), every build
	const bool bTake = InTakeover();
	FVector Off2D(E.X - CapLoc.X, E.Y - CapLoc.Y, 0.f);
	if (Off2D.SizeSquared() > FMath::Square(EyeHorizMaxCm))
	{
		Off2D = Off2D.GetSafeNormal() * EyeHorizMaxCm;
		E.X = CapLoc.X + Off2D.X;
		E.Y = CapLoc.Y + Off2D.Y;
	}
	if (!bTake && Phase == EPhase::Holding && !bFlyActive)
	{
		const FYggPoseDef& D = PoseDef(View);
		const float Eye0 = View == EYggView::Walk ? StandEyeCm : D.EyeCm;
		const float Rel = float(E.Z) - TerrainZ;
		const float Lo = Eye0 - D.BandCm - 15.f - PackMaxCm - SlopeUpDownCm - ExhaustHangCm, Hi = Eye0 + D.BandCm + 15.f;
		E.Z = TerrainZ + FMath::Clamp(Rel, Lo, Hi);
	}
	const float MinZ = FloorZ + (bTake ? EyeFloorMinTakeoverCm : EyeFloorMinCm);
	// the capsule top keeps a low eye under a tent roof, but the climber's capsule is shorter than a standing eye: the
	// robot player's flight recorder caught the eye held at 108-127 cm instead of 156 while walking (2026-11-20). Standing
	// (Walk, no takeover) may reach its own eye band above the capsule.
	float MaxZ = float(CapLoc.Z) + Half - EyeTopMarginCm;
	if (!bTake && View == EYggView::Walk)
	{
		MaxZ = FMath::Max(MaxZ, FloorZ + StandEyeCm + PoseDef(View).BandCm);
	}
	E.Z = FMath::Clamp(float(E.Z), FMath::Min(MinZ, MaxZ), MaxZ);

	Eye = E;
	bEyeValid = true;
	EyeAboveFloor = float(E.Z) - FloorZ;
	if (bRigOn && Camera)
	{
		Camera->SetWorldLocation(E);
	}
}

void UYggViewRigComponent::ComposeViewOffsets(float Dt)
{
	const float LandNod = FMath::Clamp(LandNodPerCm * LandZ, -LandNodMaxDeg, LandNodMaxDeg);
	const float HangP = ExhaustHangPitchDeg * ExhaustHang;
	// optional turn lean (off by default, TurnLeanK 0)
	const float LeanWant = TurnLeanK > 0.f && View == EYggView::Walk
		? FMath::Clamp(TurnLeanK * TurnRate * FMath::Clamp(Speed2D / 230.f, 0.f, 1.f), -TurnLeanMaxDeg, TurnLeanMaxDeg) : 0.f;
	TurnLean = ExpTo(TurnLean, LeanWant, TurnLeanTau, Dt);
	ViewPitchOff = BobPitch + BreathPitch + LandNod + NodX + HangP + SwayP + ShivP + ShakeP + GazeBend + float(TakeRot.Pitch);
	ViewYawOff = SwayY + ShivY + ShakeY + float(TakeRot.Yaw);
	ViewRollOff = BobRoll + TurnLean + float(TakeRot.Roll);
}

void UYggViewRigComponent::PushBody()
{
	UFirstPersonBodyComponent* B = Body.Get();
	if (!B)
	{
		return;
	}
	const int32 One = CVarViewOneBody.GetValueOnGameThread() != 0 ? 1 : 0;
	if (One != PushedOneBody)
	{
		PushedOneBody = One;
		B->SetOneBody(One != 0);
	}
	// the posture for the body: standing legs hidden while posed, the first-person mesh lowered under the low eye
	const float Drop = GetPostureDrop();
	if (View != PushedBodyView || FMath::Abs(Drop - PushedBodyDrop) > 0.25f)
	{
		PushedBodyView = View;
		PushedBodyDrop = Drop;
		B->SetPostureView(View, Drop);
	}
}

// ------------------------------------------------------------------------------------------------ the modifier's sides

bool UYggViewRigComponent::IsLookLocked() const
{
	// look locks only in the fall, the fly-out and closed-eyelid holds (spec R8)
	return HasLease(EYggLease::Look) || Scripted == EYggScripted::Fall || bFlyActive || bFlyWaiting || Lid >= 0.98f;
}

bool UYggViewRigComponent::CanGuide() const
{
	return bRigOn && Scripted == EYggScripted::None && !bInspect && !IsLookLocked()
		&& (View == EYggView::Walk || View == EYggView::Sit || View == EYggView::Kneel1 || View == EYggView::Kneel2);
}

void UYggViewRigComponent::ProcessRotation(AActor* ViewTarget, float Dt, FRotator& ViewRot, FRotator& Delta)
{
	SCOPE_CYCLE_COUNTER(STAT_YggView);
	// the control rotation never rolls
	ViewRot.Roll = 0.f;
	Delta.Roll = 0.f;
	const float RawYaw = float(Delta.Yaw), RawPitch = float(Delta.Pitch);
	const float InMag = FMath::Sqrt(RawYaw * RawYaw + RawPitch * RawPitch);
	LastInputRate = Dt > KINDA_SMALL_NUMBER ? InMag / Dt : 0.f;
	if (InMag > 0.05f) { LastLookInput = Clock; }
	if (!bRigOn)
	{
		return;
	}
	const float Pitch0 = Norm(ViewRot.Pitch);
	const float Yaw0 = Norm(ViewRot.Yaw);
	const bool bPawnView = ViewTarget == GetOwner();

	if (bInspect && InspPhase != EInspPhase::Pocket)
	{
		// 1. inspect: the mouse turns the object; the world view stays
		InspYaw = Norm(InspYaw + RawYaw * InspectMouse);
		InspPitch = FMath::Clamp(InspPitch + RawPitch * InspectMouse, -InspectPitchMax, InspectPitchMax);
		Delta.Yaw = 0.f;
		Delta.Pitch = 0.f;
	}
	else if (IsLookLocked() || !bPawnView)
	{
		// 2. look lock
		Delta.Yaw = 0.f;
		Delta.Pitch = 0.f;
	}
	else
	{
		// 3. soft limits: the look slows into a limit (12 deg soft edge) instead of hitting a wall
		float Dp = RawPitch, Dy = RawYaw;
		if (Dp < 0.f) { Dp *= Soft(Pitch0 - LimPitchMin, SoftEdgeDeg); }
		else if (Dp > 0.f) { Dp *= Soft(LimPitchMax - Pitch0, SoftEdgeDeg); }
		if (LimYawHalf < 179.f && Dy != 0.f)
		{
			const float Off = Norm(Yaw0 - AnchorYaw);
			const float Dist = Dy > 0.f ? LimYawHalf - Off : LimYawHalf + Off;
			if (View == EYggView::Sit && Phase == EPhase::Holding && Dist < 2.f)
			{
				// sitting, holding against the edge for 0.6 s shuffles the body round (the anchor follows at 60 deg/s)
				ShuffleTime += Dt;
				if (ShuffleTime >= SitShuffleAfterSec)
				{
					const float Step = FMath::Sign(Dy) * FMath::Min(FMath::Abs(Dy), SitShuffleDegS * Dt);
					AnchorYaw = Norm(AnchorYaw + Step);
					Dy = Step;
				}
				else
				{
					Dy *= Soft(Dist, SoftEdgeDeg);
				}
			}
			else
			{
				ShuffleTime = 0.f;
				Dy *= Soft(Dist, SoftEdgeDeg);
			}
		}
		Delta.Pitch = Dp;
		Delta.Yaw = Dy;
	}

	// 4. nudge, reflex and hand pull: small deltas through the same input path, yielding to the mouse
	if (bPawnView && CanGuide())
	{
		const float Yield = FMath::Clamp(1.f - LastInputRate / FMath::Max(NudgeYieldDegS, 1.f), 0.f, 1.f);
		if (bNudge)
		{
			NudgeT += Dt;
			NudgeFastT = LastInputRate > NudgeCancelDegS ? NudgeFastT + Dt : 0.f;
			if (NudgeFastT >= NudgeCancelSec)
			{
				EndNudge(TEXT("the player turned away himself"));
			}
			else if (NudgeT > NudgeMaxSec)
			{
				EndNudge(TEXT("time"));
			}
			else
			{
				const FRotator Want = (NudgeTarget - Eye).Rotation();
				const float Ey = Norm(Want.Yaw - (Yaw0 + Delta.Yaw));
				const float Ep = Norm(Want.Pitch - (Pitch0 + Delta.Pitch));
				if (FMath::Abs(Ey) < NudgeDoneYaw && FMath::Abs(Ep) < NudgeDonePitch)
				{
					EndNudge(TEXT("the target is in view"));
				}
				else
				{
					const float W = NudgeRamp > 0.f ? FMath::Clamp(NudgeT / NudgeRamp, 0.f, 1.f) : 1.f;
					float Dy = FMath::Clamp(NudgeK * Ey, -NudgeRate, NudgeRate) * Dt * W * Yield;
					float Dp = FMath::Clamp(NudgeK * Ep, -NudgePitchRate, NudgePitchRate) * Dt * W * Yield;
					Dy = FMath::Clamp(NudgeYawDone + Dy, -NudgeMaxYaw, NudgeMaxYaw) - NudgeYawDone;
					Dp = FMath::Clamp(NudgePitchDone + Dp, -NudgePitchMax, NudgePitchMax) - NudgePitchDone;
					NudgeYawDone += Dy;
					NudgePitchDone += Dp;
					Delta.Yaw += Dy;
					Delta.Pitch += Dp;
				}
			}
		}
		if (bReflex)
		{
			ReflexT += Dt;
			const float E = EaseOut(ReflexT / FMath::Max(ReflexSec, 0.01f));
			const float D = E - ReflexDone;
			ReflexDone = E;
			Delta.Yaw += float(ReflexTotal.X) * D;
			Delta.Pitch += float(ReflexTotal.Y) * D;
			if (ReflexT >= ReflexSec) { bReflex = false; }
		}
		if (const AActor* Toward = PullActor.Get(); Toward && PullRate > 0.f)
		{
			const FVector To = Toward->GetActorLocation() + Toward->GetActorForwardVector() * 150.f;
			const float E = Norm((To - Eye).Rotation().Yaw - (Yaw0 + Delta.Yaw));
			Delta.Yaw += FMath::Clamp(HandPullK * E, -PullRate, PullRate) * Dt * Yield;
		}
	}
	else
	{
		bReflex = false;
	}

	// 5. the normalised pitch (and the posture's yaw) stay inside the limits; a limit tween drags the view in smoothly
	const float MinP = FMath::Max(LimPitchMin, BackstopPitchMin);
	const float MaxP = FMath::Min(LimPitchMax, BackstopPitchMax);
	const float NewPitch = FMath::Clamp(Pitch0 + float(Delta.Pitch), MinP, FMath::Max(MinP, MaxP));
	Delta.Pitch = NewPitch - Pitch0;
	if (LimYawHalf < 179.f && bPawnView)
	{
		const float Off = Norm(Yaw0 - AnchorYaw);
		const float NewOff = FMath::Clamp(Off + float(Delta.Yaw), -LimYawHalf, LimYawHalf);
		Delta.Yaw = NewOff - Off;
	}
	ViewRot.Pitch = Pitch0;
	ViewRot.Yaw = Yaw0;
}

void UYggViewRigComponent::ApplyView(APlayerCameraManager* PCM, float Dt, FMinimalViewInfo& POV)
{
	SCOPE_CYCLE_COUNTER(STAT_YggView);
	if (!PCM)
	{
		return;
	}
	const AActor* VT = PCM->GetViewTarget();
	if (FlyCam && VT == FlyCam)
	{
		// the fly camera: its FOV, and the exposure channel; nothing else is touched
		if (bFlyActive) { POV.FOV = FlyFovNow; }
		if (FMath::Abs(ExposureSum) > 0.005f) { PPBias.AutoExposureBias = ExposureSum; PCM->AddCachedPPBlend(PPBias, 1.f); }
		return;
	}
	if (VT != GetOwner() || !bRigOn)
	{
		return;
	}
	// view-only rotation: bob, breathing, landing, nods, hang, sway, shiver, gaze bend and the takeovers
	FRotator R = POV.Rotation;
	float Pitch = Norm(R.Pitch) + ViewPitchOff;
	const float Yaw = Norm(R.Yaw + ViewYawOff);
	float Roll = ViewRollOff;
	float MinP, MaxP, RollMax;
	if (Scripted == EYggScripted::Fall)
	{
		MinP = -72.f; MaxP = 80.f; RollMax = RollMaxFallDeg;
	}
	else
	{
		MinP = FMath::Max(FMath::Max(LimPitchMin, BackstopPitchMin) - 2.f, AbsPitchMin);
		MaxP = FMath::Min(FMath::Min(LimPitchMax, BackstopPitchMax) + 2.f, AbsPitchMax);
		RollMax = TakeFade > 0.f ? RollMaxFallDeg : Scripted == EYggScripted::Knock ? RollMaxKnockDeg : RollMaxDeg;
	}
	Pitch = FMath::Clamp(Pitch, MinP, FMath::Max(MinP, MaxP));
	Roll = FMath::Clamp(Roll, -RollMax, RollMax);
	POV.Rotation = FRotator(Pitch, Yaw, Roll);
	// FOV terms together within the settings FOV +- 5 deg
	const float Terms = HeartFov + StoryFov + AttnValue + FovBeatValue + InspectFovDeg * InspAlpha;
	POV.FOV += FMath::Clamp(Terms, -ViewFovMaxDeg, ViewFovMaxDeg);
	// the one exposure channel: steady while looking down, the story's overshoots and the named sources, inspect's darkening
	if (CVarViewSteady.GetValueOnGameThread() != 0)
	{
		const float K = FMath::SmoothStep(SteadyFromDeg, FMath::Max(SteadyFullDeg, SteadyFromDeg + 1.f), -Pitch);
		if (K > 0.001f) { PCM->AddCachedPPBlend(PPSteady, K); }
	}
	if (FMath::Abs(ExposureSum) > 0.005f)
	{
		PPBias.AutoExposureBias = ExposureSum;
		PCM->AddCachedPPBlend(PPBias, 1.f);
	}
	if (InspAlpha > 0.001f)
	{
		PCM->AddCachedPPBlend(PPInspect, InspAlpha);
	}
}

// ------------------------------------------------------------------------------------------------ story guidance

bool UYggViewRigComponent::Nudge(int32 Owner, const FVector& Target, float MaxYaw, float Rate, float PitchMax)
{
	const TCHAR* Why = nullptr;
	if (bGentle) { Why = TEXT("Story camera Gentle"); }
	else if (NudgesUsed >= NudgeBudget) { Why = TEXT("the chapter's budget is spent"); }
	else if (Clock - LastNudgeTime < NudgeGapSec) { Why = TEXT("less than 60 s since the last"); }
	else if (Speed2D > NudgeMaxSpeedCmS) { Why = TEXT("walking too fast"); }
	else if (!CanGuide()) { Why = TEXT("not now (posture, takeover, inspect or closed eyes)"); }
	if (Why)
	{
		UE_LOG(LogYggView, Display, TEXT("Rig: nudge refused (beat %d): %s - lure instead"), Owner, Why);
		return false;
	}
	++NudgesUsed;
	LastNudgeTime = Clock;
	bNudge = true;
	NudgeTarget = Target;
	NudgeMaxYaw = MaxYaw > 0.f ? FMath::Min(MaxYaw, 45.f) : 45.f;
	NudgeRate = Rate > 0.f ? FMath::Min(Rate, 20.f) : 20.f;
	NudgePitchMax = PitchMax > 0.f ? FMath::Min(PitchMax, 12.f) : 8.f;
	NudgeT = NudgeYawDone = NudgePitchDone = NudgeFastT = 0.f;
	NudgeOwner = Owner;
	const int32 Id = AcquireLease(Owner, EYggLease::Nudge, 0.f);
	for (FLease& L : Leases) { if (L.Id == Id) { L.bQuiet = true; } }
	UE_LOG(LogYggView, Display, TEXT("Rig: nudge %d of %d (beat %d)"), NudgesUsed, NudgeBudget, Owner);
	return true;
}

void UYggViewRigComponent::EndNudge(const TCHAR* Why)
{
	if (!bNudge)
	{
		return;
	}
	bNudge = false;
	QuietRelease(NudgeOwner, EYggLease::Nudge);
	UE_LOG(LogYggView, Display, TEXT("Rig: nudge ended: %s (%.0f deg yaw, %.0f deg pitch)"), Why, FMath::Abs(NudgeYawDone), FMath::Abs(NudgePitchDone));
}

void UYggViewRigComponent::ResetChapterBudget()
{
	NudgesUsed = 0;
	LastNudgeTime = -1000.0;
	LastReflexTime = -1000.0;
}

void UYggViewRigComponent::Reflex(const FVector& Source, float Deg)
{
	const ACharacter* C = Char.Get();
	if (!C || bGentle || Clock - LastReflexTime < ReflexCooldownSec || !CanGuide())
	{
		UE_LOG(LogYggView, Display, TEXT("Rig: reflex skipped (%s)"), bGentle ? TEXT("Gentle") : !CanGuide() ? TEXT("not now") : TEXT("cooldown"));
		return;
	}
	const FRotator Want = (Source - Eye).Rotation();
	const FRotator Ctrl = C->GetControlRotation();
	const float Ey = Norm(Want.Yaw - Ctrl.Yaw);
	const float Ep = Norm(Want.Pitch - Norm(Ctrl.Pitch));
	const float Off = FMath::Sqrt(Ey * Ey + Ep * Ep);
	// only for a sound well off-centre: ahead you already see it, behind you the player turns
	if (Off < ReflexMinOffDeg || Off > ReflexMaxOffDeg)
	{
		UE_LOG(LogYggView, Display, TEXT("Rig: reflex skipped (source %.0f deg off-centre)"), Off);
		return;
	}
	const float D = FMath::Min(Deg > 0.f ? Deg : 10.f, Off);
	const FVector2D Dir = FVector2D(Ey, Ep).GetSafeNormal();
	ReflexTotal = FVector2D(Dir.X * D, FMath::Clamp(float(Dir.Y) * D, -3.f, 3.f));
	ReflexT = 0.f;
	ReflexDone = 0.f;
	bReflex = true;
	LastReflexTime = Clock;
	UE_LOG(LogYggView, Display, TEXT("Rig: reflex %.0f deg toward a sound %.0f deg off-centre"), D, Off);
}

void UYggViewRigComponent::SetHandPull(TWeakObjectPtr<AActor> Toward, float Rate)
{
	PullActor = Toward;
	PullRate = Toward.IsValid() ? FMath::Clamp(Rate, 0.f, 8.f) : 0.f;
}

// ------------------------------------------------------------------------------------------------ takeovers

void UYggViewRigComponent::SetDilation(float D, float AudioPitch)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	if (!FMath::IsNearlyEqual(D, Dilation, 0.001f))
	{
		UGameplayStatics::SetGlobalTimeDilation(this, D);
		Dilation = D;
	}
	const bool bPitch = !FMath::IsNearlyEqual(AudioPitch, 1.f, 0.001f);
	if ((bPitch || bAudioPitched) && !FMath::IsNearlyEqual(AudioPitch, LastAudioPitch, 0.01f))
	{
		if (FAudioDeviceHandle Device = World->GetAudioDevice())
		{
			Device->SetGlobalPitchModulation(AudioPitch, 0.1f);
		}
		LastAudioPitch = AudioPitch;
		bAudioPitched = bPitch;
	}
}

void UYggViewRigComponent::Fall(int32 Owner, float Seconds, float Side)
{
	ACharacter* C = Char.Get();
	if (!C)
	{
		return;
	}
	const float S = Seconds > 0.2f ? Seconds : 1.5f;
	AcquireLease(Owner, EYggLease::Look, 0.f);
	AcquireLease(Owner, EYggLease::Move, 0.f);
	if (bGentle)
	{
		// Gentle: the slip is a plain eyelid close
		const int32 Id = AcquireLease(Owner, EYggLease::Fall, 0.8f);
		for (FLease& L : Leases) { if (L.Id == Id) { L.bQuiet = true; } }
		SetLids(Owner, EYggLidMode::Closed, 0.6f, 0.f, nullptr);
		UE_LOG(LogYggView, Display, TEXT("Rig: fall as an eyelid close (Gentle, beat %d)"), Owner);
		return;
	}
	const int32 Id = AcquireLease(Owner, EYggLease::Fall, FMath::Min(S + 0.5f, 3.f));
	for (FLease& L : Leases) { if (L.Id == Id) { L.bQuiet = true; } }
	Scripted = EYggScripted::Fall;
	ScriptOwner = Owner;
	ScriptT = 0.f;
	ScriptLen = S;
	ScriptSide = Side < 0.f ? -1.f : 1.f;
	bFallLidsSlammed = false;
	TakeFade = 0.f;
	bFallSnapPending = false;
	// one sphere trace at the start: the drop is scaled so the eye stays 30 cm over the floor
	FallZScale = 1.f;
	if (UWorld* World = GetWorld())
	{
		FHitResult Hit;
		FCollisionQueryParams Q(SCENE_QUERY_STAT(YggViewFall), false, C);
		if (World->SweepSingleByChannel(Hit, Eye, Eye - FVector(0.f, 0.f, 250.f), FQuat::Identity, ECC_Visibility, FCollisionShape::MakeSphere(10.f), Q))
		{
			const float Room = float(Eye.Z - Hit.ImpactPoint.Z) - EyeFloorMinTakeoverCm;
			FallZScale = FMath::Clamp(Room / 85.f, 0.1f, 1.f);
		}
	}
	UE_LOG(LogYggView, Display, TEXT("Rig: fall %.1f s to the %s (beat %d)"), S, ScriptSide < 0.f ? TEXT("left") : TEXT("right"), Owner);
}

void UYggViewRigComponent::Knock(int32 Owner, float Side)
{
	if (Scripted != EYggScripted::None || bFlyActive)
	{
		UE_LOG(LogYggView, Warning, TEXT("Rig: knock (beat %d) ignored during another takeover"), Owner);
		return;
	}
	const int32 Id = AcquireLease(Owner, EYggLease::Knock, KnockSec + KnockBackSec + 0.3f);
	for (FLease& L : Leases) { if (L.Id == Id) { L.bQuiet = true; } }
	Scripted = EYggScripted::Knock;
	ScriptOwner = Owner;
	ScriptT = 0.f;
	ScriptSide = Side < 0.f ? -1.f : 1.f;
	TakeFade = 0.f;
	bFallSnapPending = false;
	UE_LOG(LogYggView, Display, TEXT("Rig: knock to the %s (beat %d)"), ScriptSide < 0.f ? TEXT("left") : TEXT("right"), Owner);
}

void UYggViewRigComponent::TickScripted(float Dt)
{
	if (Scripted == EYggScripted::Fall)
	{
		ScriptT += Dt;
		const float K = 1.5f / FMath::Max(ScriptLen, 0.2f);   // the curve is written for 1.5 s
		const float T = ScriptT * K;
		const float Z = FallChannel(T, &FFallKey::Z) * FallZScale;
		TakeLoc = FVector(FallChannel(T, &FFallKey::Fwd), 0.f, Z);
		TakeRot = FRotator(FallChannel(T, &FFallKey::Pitch), FallChannel(T, &FFallKey::Yaw) * ScriptSide, FallChannel(T, &FFallKey::Roll) * ScriptSide);
		// slow motion in the slide, eased in and out over 0.1 s (real time), with the sound pitched down
		const float Slow = FMath::Clamp(CVarViewSlipSlowMo.GetValueOnGameThread(), 0.05f, 1.f);
		const float RealPer = 1.f / K;
		float Ramp = 0.f;
		if (T >= 0.40f && T < 1.10f) { Ramp = Ease((T - 0.40f) * RealPer / 0.1f); }
		else if (T >= 1.10f) { Ramp = 1.f - Ease((T - 1.10f) * RealPer / 0.1f); }
		SetDilation(FMath::Lerp(1.f, Slow, Ramp), FMath::Lerp(1.f, FallAudioPitch, Ramp));
		// the eyelids slam shut at the end of the drop
		if (!bFallLidsSlammed && T >= 1.30f)
		{
			bFallLidsSlammed = true;
			SetLids(ScriptOwner, EYggLidMode::Closed, 0.15f / K, 0.f, nullptr);
		}
		if (ScriptT >= ScriptLen)
		{
			// the Fall lease ends (Look and Move stay with the beat). The offsets (-66 deg pitch, -30 roll at the end) go home in
			// ONE frame as a camera cut while the eyelids are fully shut: easing them home over 0.3 s turned the camera 20 deg
			// in 0.15 s (a spin_pitch incident) and would show if the lids were still closing. Not shut yet: hold the fallen
			// view until they are (at most 1 s), then cut.
			SetDilation(1.f, 1.f);
			QuietRelease(ScriptOwner, EYggLease::Fall);
			Scripted = EYggScripted::None;
			TakeFade = 0.f;
			bFallSnapPending = true;
			FallSnapWait = 0.f;
			UE_LOG(LogYggView, Display, TEXT("Rig: fall over (eyes %s)"), Lid >= 0.99f ? TEXT("closed") : TEXT("still closing - holding the view"));
		}
	}
	else if (Scripted == EYggScripted::Knock)
	{
		ScriptT += Dt;
		const float T = ScriptT;
		const float K = T < KnockSec ? FMath::Sin(0.5f * PI * T / FMath::Max(KnockSec, 0.01f))
			: 1.f - Ease((T - KnockSec) / FMath::Max(KnockBackSec, 0.01f));
		TakeLoc = FVector(0.f, 0.f, -KnockDownCm * K);
		TakeRot = FRotator(KnockPitchDeg * K, KnockYawDeg * ScriptSide * K, KnockRollDeg * ScriptSide * K);
		if (T >= KnockSec + KnockBackSec)
		{
			TakeLoc = FVector::ZeroVector;
			TakeRot = FRotator::ZeroRotator;
			Scripted = EYggScripted::None;
			QuietRelease(ScriptOwner, EYggLease::Knock);
		}
	}
	else if (TakeFade > 0.f)
	{
		TakeFade = FMath::Max(0.f, TakeFade - Dt);
		const float A = Ease(TakeFade / FMath::Max(TakeFadeLen, 0.01f));
		TakeLoc = TakeFadeLoc * A;
		TakeRot = TakeFadeRot * A;
		if (TakeFade <= 0.f)
		{
			TakeLoc = FVector::ZeroVector;
			TakeRot = FRotator::ZeroRotator;
		}
	}
	// after the chain: runs in the same frame the fall ends, so with the lids already shut nothing is drawn with the offsets
	if (bFallSnapPending && Scripted == EYggScripted::None)
	{
		FallSnapWait += Dt;
		if (Lid >= 0.99f || FallSnapWait >= 1.f)
		{
			bFallSnapPending = false;
			if (Lid >= 0.99f)
			{
				TakeLoc = FVector::ZeroVector;
				TakeRot = FRotator::ZeroRotator;
				TakeFade = 0.f;
				Cut();
				UE_LOG(LogYggView, Display, TEXT("Rig: fall offsets reset behind closed eyes (cut)"));
			}
			else
			{
				// the lids never shut (a beat opened them): ease home instead of cutting in front of the player
				TakeFadeLoc = TakeLoc;
				TakeFadeRot = TakeRot;
				TakeFadeLen = 0.6f;
				TakeFade = TakeFadeLen;
				UE_LOG(LogYggView, Warning, TEXT("Rig: fall over with the eyes open - easing the offsets home over 0.6 s"));
			}
		}
	}
}

void UYggViewRigComponent::FlyOut(int32 Owner, const TArray<FVector>& Path, const TArray<FVector>& Look, float T, FVector2D Fov, float Open)
{
	ACharacter* C = Char.Get();
	UWorld* World = GetWorld();
	if (!C || !World || Path.Num() == 0 || Look.Num() == 0)
	{
		UE_LOG(LogYggView, Warning, TEXT("Rig: fly-out (beat %d) needs a path and look points"), Owner);
		return;
	}
	FlyPath = Path;
	if (FlyPath.Num() == 1) { FlyPath.Add(FlyPath[0] + FVector(0.f, 0.f, 1.f)); }
	FlyLook = Look;
	BuildArc(FlyPath, FlyPathArc);
	BuildArc(FlyLook, FlyLookArc);
	BuildFlyTables(T > 0.5f ? T : 13.f);
	FlyFov = FVector2D(Fov.X > 1.f ? Fov.X : 70.f, Fov.Y > 1.f ? Fov.Y : (Fov.X > 1.f ? Fov.X : 50.f));
	FlyFovNow = float(FlyFov.X);
	FlyOpen = FMath::Max(Open, 0.f);
	FlyOwner = Owner;
	// one pooled camera, hidden, never destroyed between uses
	if (!FlyCam)
	{
		FActorSpawnParameters P;
		P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		P.ObjectFlags |= RF_Transient;
		P.Owner = C;
		FlyCam = World->SpawnActor<ACameraActor>(ACameraActor::StaticClass(), FlyPath[0], FRotator::ZeroRotator, P);
		if (FlyCam)
		{
			FlyCam->SetActorHiddenInGame(true);
			if (UCameraComponent* Cam = FlyCam->GetCameraComponent()) { Cam->SetConstraintAspectRatio(false); }
			// World Partition: the shot streams the land it flies over (a no-op on a map without it)
			FlyStreaming = NewObject<UWorldPartitionStreamingSourceComponent>(FlyCam, MakeUniqueObjectName(FlyCam, UWorldPartitionStreamingSourceComponent::StaticClass(), TEXT("FlyStreaming")));
			FlyStreaming->RegisterComponent();
			FlyStreaming->DisableStreamingSource();
		}
	}
	if (!FlyCam)
	{
		UE_LOG(LogYggView, Warning, TEXT("Rig: fly-out camera could not be spawned"));
		return;
	}
	AcquireLease(Owner, EYggLease::Fly, FlyLen + 3.f);
	bFlyWaiting = true;
	bFlyActive = false;
	bFlyEnding = false;
	// it starts behind closed eyes: now if they are, else after a quick close
	if (Lid >= 0.99f)
	{
		StartFly();
	}
	else
	{
		SetLids(Owner, EYggLidMode::Closed, BlinkCloseSec, 0.f, [this]() { if (bFlyWaiting) { StartFly(); } });
	}
}

// the fly camera's look elevation is held inside +-80 deg: near the nadir/zenith the yaw of a look vector flips
// within a few frames (a spin with roll 0) and the angle between look vectors would not show it
static FRotator FlyLookRot(const FVector& Dir)
{
	FRotator R = Dir.Rotation();
	R.Pitch = FMath::Clamp(FRotator::NormalizeAxis(R.Pitch), -80.f, 80.f);
	R.Roll = 0.f;
	return R;
}

void UYggViewRigComponent::BuildFlyTables(float T)
{
	// 64 samples of the eased path: where the camera looks at each; a stretch turning faster than 12 deg/s gets more time
	constexpr int32 K = 64;
	FlySampleU.SetNum(K);
	FlySampleT.SetNum(K);
	TArray<FVector> Dirs;
	Dirs.SetNum(K);
	bool bSteep = false;
	for (int32 i = 0; i < K; ++i)
	{
		const float U = Smoother(float(i) / float(K - 1));
		FlySampleU[i] = U;
		const FVector D = FlyLookAt(U) - FlyPathAt(U);
		if (!D.IsNearlyZero() && FMath::Abs(FRotator::NormalizeAxis(D.Rotation().Pitch)) > 80.f)
		{
			bSteep = true;
		}
		// the retimer measures what is shown (clamped elevation), so a yaw swing near the nadir is slowed too
		Dirs[i] = D.IsNearlyZero() ? (i > 0 ? Dirs[i - 1] : FVector::ForwardVector) : FlyLookRot(D).Vector();
	}
	if (bSteep)
	{
		UE_LOG(LogYggView, Warning, TEXT("Rig: fly-out looks steeper than 80 deg at some point: elevation clamped (check the fly_* places)"));
	}
	const float Planned = T / float(K - 1);
	float Total = 0.f, MaxRate = 0.f;
	FlySampleT[0] = 0.f;
	for (int32 i = 1; i < K; ++i)
	{
		const float Deg = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(float(Dirs[i - 1] | Dirs[i]), -1.f, 1.f)));
		MaxRate = FMath::Max(MaxRate, Deg / Planned);
		Total += FMath::Max(Planned, Deg / FMath::Max(FlyMaxTurnDegS, 1.f));
		FlySampleT[i] = Total;
	}
	FlyLen = FMath::Max(Total, 0.1f);
	if (FlyLen > T + 0.05f)
	{
		UE_LOG(LogYggView, Warning, TEXT("Rig: fly-out turns up to %.1f deg/s (cap %.0f): retimed %.1f s -> %.1f s"), MaxRate, FlyMaxTurnDegS, T, FlyLen);
	}
}

FVector UYggViewRigComponent::FlyPathAt(float ArcU) const
{
	return ArcPoint(FlyPath, FlyPathArc, ArcU);
}

FVector UYggViewRigComponent::FlyLookAt(float ArcU) const
{
	return ArcPoint(FlyLook, FlyLookArc, ArcU);
}

void UYggViewRigComponent::StartFly()
{
	APlayerController* PC = GetPC();
	ACharacter* C = Char.Get();
	if (!PC || !C || !FlyCam)
	{
		bFlyWaiting = false;
		QuietRelease(FlyOwner, EYggLease::Fly);
		return;
	}
	bFlyWaiting = false;
	bFlyActive = true;
	bFlyEnding = false;
	FlyT = 0.f;
	Scripted = EYggScripted::Fly;
	TickFly(0.f);
	if (FlyStreaming) { FlyStreaming->EnableStreamingSource(); }
	PC->SetViewTargetWithBlend(FlyCam, 0.f);
	Cut();
	C->SetActorHiddenInGame(true);
	bPawnHiddenByRig = true;
	SetLids(FlyOwner, EYggLidMode::Open, FlyOpen, 0.f, nullptr);
	UE_LOG(LogYggView, Display, TEXT("Rig: camera cut (fly-out start, %.1f s, beat %d)"), FlyLen, FlyOwner);
}

void UYggViewRigComponent::EndFly(bool bCut)
{
	const bool bWas = bFlyActive || bFlyWaiting;
	bFlyActive = false;
	bFlyWaiting = false;
	bFlyEnding = false;
	if (Scripted == EYggScripted::Fly) { Scripted = EYggScripted::None; }
	if (FlyStreaming) { FlyStreaming->DisableStreamingSource(); }
	for (int32 i = Leases.Num() - 1; i >= 0; --i) { if (Leases[i].Kind == EYggLease::Fly) { Leases.RemoveAt(i); } }
	ACharacter* C = Char.Get();
	APlayerController* PC = GetPC();
	if (PC && C && PC->GetViewTarget() != C)
	{
		PC->SetViewTarget(C);
		if (bCut) { Cut(); }
	}
	if (C && C->IsHidden()) { C->SetActorHiddenInGame(false); }
	bPawnHiddenByRig = false;
	if (bWas)
	{
		UE_LOG(LogYggView, Display, TEXT("Rig: camera cut (fly-out end)"));
		HardReset(C && C->GetController() ? C->GetController()->GetControlRotation() : FRotator::ZeroRotator, EYggView::Walk);
	}
}

void UYggViewRigComponent::TickFly(float Dt)
{
	if (!bFlyActive || !FlyCam || FlySampleT.Num() < 2)
	{
		return;
	}
	// camera auto-management hands the view back to the pawn now and then: the shot keeps it
	if (APlayerController* PC = GetPC(); PC && PC->GetViewTarget() != FlyCam)
	{
		PC->SetViewTarget(FlyCam);
	}
	FlyT += Dt;
	const float T = FMath::Clamp(FlyT, 0.f, FlyLen);
	int32 i = 0;
	while (i < FlySampleT.Num() - 2 && T > FlySampleT[i + 1]) { ++i; }
	const float Span = FMath::Max(FlySampleT[i + 1] - FlySampleT[i], 1e-4f);
	const float U = FMath::Lerp(FlySampleU[i], FlySampleU[i + 1], FMath::Clamp((T - FlySampleT[i]) / Span, 0.f, 1.f));
	const FVector Pos = FlyPathAt(U);
	const FVector Dir = FlyLookAt(U) - Pos;
	// rebuilt from a look vector every frame, roll 0: no angle is ever interpolated, so it can't wrap or roll
	FRotator Rot = Dir.IsNearlyZero() ? FlyCam->GetActorRotation() : FlyLookRot(Dir);
	Rot.Roll = 0.f;
	FlyCam->SetActorLocationAndRotation(Pos, Rot);
	FlyFovNow = FMath::Lerp(float(FlyFov.X), float(FlyFov.Y), Ease(T / FlyLen));
	if (FlyT >= FlyLen && !bFlyEnding)
	{
		bFlyEnding = true;
		if (Lid >= 0.99f) { EndFly(true); }
		else { SetLids(FlyOwner, EYggLidMode::Closed, FlyCloseSec, 0.f, [this]() { if (bFlyActive) { EndFly(true); } }); }
	}
}

// ------------------------------------------------------------------------------------------------ story lens

void UYggViewRigComponent::SetStoryFov(int32 Owner, float Deg, float T)
{
	SetStoryFov(Owner, Deg, T, false);
}

void UYggViewRigComponent::SetStoryFov(int32 Owner, float Deg, float T, bool bKeep)
{
	const float D = FMath::Clamp(Deg, -StoryFovMaxDeg, StoryFovMaxDeg);
	if (!FMath::IsNearlyEqual(D, Deg))
	{
		UE_LOG(LogYggView, Display, TEXT("Rig: story FOV %.1f clamped to %.1f (beat %d)"), Deg, D, Owner);
	}
	if (FMath::Abs(D) > 0.01f)
	{
		AcquireLease(Owner, EYggLease::StoryFov, 0.f, bKeep);
	}
	StoryFovOwner = Owner;
	StoryFovFrom = StoryFov;
	StoryFovTo = D;
	StoryFovT = 0.f;
	StoryFovLen = FMath::Max(T, StoryFovMinEase);   // never a snap, never oscillating
}

void UYggViewRigComponent::Attention(int32 Owner, float Deg, float T)
{
	AttnTarget = FMath::Clamp(Deg, -StoryFovMaxDeg, StoryFovMaxDeg);
	AttnIn = FMath::Max(T, 0.5f);
	AttnOwner = Owner;
	AttnLeft = LeaseCap(EYggLease::Attention, false) - AttentionReleaseSec - 0.2f;
	bAttn = true;
	const int32 Id = AcquireLease(Owner, EYggLease::Attention, 0.f);
	for (FLease& L : Leases) { if (L.Id == Id) { L.bQuiet = true; } }
}

void UYggViewRigComponent::FovBeat(int32 Owner, float Deg, float Bpm, float T)
{
	if (!bHeartZoom)
	{
		UE_LOG(LogYggView, Display, TEXT("Rig: FOV beat skipped (Heartbeat zoom off)"));
		return;
	}
	FovBeatDeg = FMath::Clamp(FMath::Abs(Deg), 0.f, FovBeatCap);
	FovBeatBpm = Bpm > 1.f ? Bpm : 96.f;
	FovBeatHold = FMath::Max(T, 0.f);
	FovBeatT = 0.f;
	FovBeatOutFrom = -1.f;
	FovBeatOwner = Owner;
	bFovBeat = true;
	const int32 Id = AcquireLease(Owner, EYggLease::FovBeat, FovBeatIn + FovBeatHold + FovBeatOut + 0.3f);
	for (FLease& L : Leases) { if (L.Id == Id) { L.bQuiet = true; } }
}

void UYggViewRigComponent::StoryExposure(int32 Owner, float Ev, float T, float Hold)
{
	FExposurePulse P;
	P.Ev = FMath::Clamp(Ev, -2.f, 2.f);
	P.Rise = FMath::Max(T, 0.05f);
	P.Hold = FMath::Max(Hold, 0.f);
	P.Tau = FMath::Max(T, 0.5f);
	P.bStory = true;
	Pulses.Add(P);
	const int32 Id = AcquireLease(Owner, EYggLease::Exposure, P.Rise + P.Hold + 0.25f);
	for (FLease& L : Leases) { if (L.Id == Id) { L.bQuiet = true; } }
}

void UYggViewRigComponent::StoryBreath(int32 Owner, float B, float T)
{
	StoryB = FMath::Clamp(B, 0.f, 1.f);
	const int32 Id = AcquireLease(Owner, EYggLease::Breath, T);
	if (T > 0.f) { for (FLease& L : Leases) { if (L.Id == Id) { L.bQuiet = true; } } }
}

void UYggViewRigComponent::SetPace(int32 Owner, float Ms, float T, bool bKeep)
{
	if (Ms <= 0.f)
	{
		// "pace": false lets go of every pace (the prologue's keep pace ends in the slip beat)
		PaceCmS = -1.f;
		for (int32 i = Leases.Num() - 1; i >= 0; --i) { if (Leases[i].Kind == EYggLease::Pace) { Leases.RemoveAt(i); } }
		return;
	}
	PaceCmS = Ms * 100.f;
	const int32 Id = AcquireLease(Owner, EYggLease::Pace, T, bKeep);
	if (T > 0.f) { for (FLease& L : Leases) { if (L.Id == Id) { L.bQuiet = true; } } }
}

void UYggViewRigComponent::TickLens(float Dt)
{
	// story FOV: eased, and its lease goes once it is back at 0
	if (StoryFovLen > 0.f)
	{
		StoryFovT += Dt;
		StoryFov = FMath::Lerp(StoryFovFrom, StoryFovTo, Ease(StoryFovT / StoryFovLen));
		if (StoryFovT >= StoryFovLen)
		{
			StoryFov = StoryFovTo;
			StoryFovLen = 0.f;
			if (FMath::Abs(StoryFovTo) < 0.01f)
			{
				for (int32 i = Leases.Num() - 1; i >= 0; --i) { if (Leases[i].Kind == EYggLease::StoryFov) { Leases.RemoveAt(i); } }
			}
		}
	}
	// attention: only while the player is still (the player causes it); any look or move once it shows lets it go
	const bool bStill = Speed2D < AttentionStillCmS && FMath::Abs(TurnRate) < AttentionStillDegS && LastInputRate < AttentionStillDegS;
	StillTime = bStill ? StillTime + Dt : 0.f;
	float AttnWant = 0.f;
	if (bAttn)
	{
		AttnLeft -= Dt;
		const bool bShowing = FMath::Abs(AttnValue) > 0.05f;
		if ((bShowing && !bStill) || AttnLeft <= 0.f || !HasLease(EYggLease::Attention))
		{
			bAttn = false;
			QuietRelease(AttnOwner, EYggLease::Attention);
		}
		else if (StillTime >= AttentionStillSec)
		{
			AttnWant = AttnTarget;
		}
	}
	const float AttnRate = AttnWant != 0.f ? FMath::Abs(AttnTarget) / FMath::Max(AttnIn, 0.1f)
		: FMath::Max(FMath::Abs(AttnTarget), 0.5f) / FMath::Max(BlendOutSec > 0.01f ? FMath::Min(BlendOutSec, AttentionReleaseSec) : AttentionReleaseSec, 0.1f);
	AttnValue = FMath::FInterpConstantTo(AttnValue, AttnWant, Dt, AttnRate);
	// the heartbeat push: 1.5 s in, held, 2 s out, a lub-dub on top
	if (bFovBeat || FMath::Abs(FovBeatValue) > 0.001f)
	{
		FovBeatT += Dt;
		if (bFovBeat && !HasLease(EYggLease::FovBeat) && FovBeatOutFrom < 0.f && FovBeatT < FovBeatIn + FovBeatHold)
		{
			// the beat ended early: out from where it is
			FovBeatOutFrom = FovBeatT < FovBeatIn ? Ease(FovBeatT / FMath::Max(FovBeatIn, 0.01f)) : 1.f;
			FovBeatT = FovBeatIn + FovBeatHold;
		}
		const float In = FMath::Max(FovBeatIn, 0.01f), Out = FMath::Max(FovBeatOut, 0.01f);
		const float From = FovBeatOutFrom >= 0.f ? FovBeatOutFrom : 1.f;
		float Env;
		if (FovBeatT < In) { Env = Ease(FovBeatT / In); }
		else if (FovBeatT < In + FovBeatHold) { Env = 1.f; }
		else { Env = From * (1.f - Ease((FovBeatT - In - FovBeatHold) / Out)); }
		const float Phase01 = FMath::Fmod(FovBeatT * FovBeatBpm / 60.f, 1.f);
		const float Beat = FMath::Max(Bump(Phase01, 0.06f, 0.05f), 0.67f * Bump(Phase01, 0.26f, 0.05f));   // lub-dub 60/40
		FovBeatValue = bHeartZoom ? -FMath::Min(Env * (FovBeatDeg + FovBeatPerBeat * Beat), FovBeatCap) : 0.f;
		if (FovBeatT >= In + FovBeatHold + Out)
		{
			FovBeatValue = 0.f;
			if (bFovBeat) { bFovBeat = false; QuietRelease(FovBeatOwner, EYggLease::FovBeat); }
		}
	}
	// heart FOV: nothing below 7,000 m; above, a fraction of a degree per beat with thin air or exhaustion
	const float AltK = FMath::SmoothStep(HeartFovFromM - 100.f, HeartFovFromM + 100.f, AltM);
	HeartFov = bHeartZoom ? -HeartFovDeg * FeedBeat * FMath::Max(FeedHyp, FeedExhaust * 0.7f) * AltK : 0.f;
	// exposure: the overshoots (a story one holds only while its lease lives) and the named sources, summed
	const bool bStoryExposure = HasLease(EYggLease::Exposure);
	float Sum = 0.f;
	for (int32 i = Pulses.Num() - 1; i >= 0; --i)
	{
		FExposurePulse& P = Pulses[i];
		P.T += Dt;
		if (P.bStory && !bStoryExposure && P.T < P.Rise + P.Hold) { P.Hold = FMath::Max(0.f, P.T - P.Rise); }
		float V;
		if (P.T < P.Rise) { V = P.Ev * Ease(P.T / FMath::Max(P.Rise, 0.01f)); }
		else if (P.T < P.Rise + P.Hold) { V = P.Ev; }
		else { V = P.Ev * FMath::Exp(-(P.T - P.Rise - P.Hold) / FMath::Max(P.Tau, 0.01f)); }
		if (P.T > P.Rise + P.Hold + 5.f * P.Tau) { Pulses.RemoveAt(i); continue; }
		Sum += V;
	}
	for (const TPair<FName, float>& S : ExposureSources) { Sum += S.Value; }
	ExposureSum = FMath::Clamp(Sum, -3.f, 3.f);
}

// ------------------------------------------------------------------------------------------------ inspect, hands

void UYggViewRigComponent::BeginInspect(int32 Owner, const FYggInspect& I)
{
	ACharacter* C = Char.Get();
	Insp = I;
	bInspect = true;
	InspPhase = EInspPhase::Rising;
	InspT = 0.f;
	InspYaw = 0.f;
	InspPitch = 0.f;
	BackT = 0.f;
	bBackSeen = false;
	InspOwner = Owner;
	AcquireLease(Owner, EYggLease::Inspect, 0.f);
	if (I.Mesh && C && Camera)
	{
		// one reused owner-only mesh held up in front of the eye (a card is drawn by the story overlay instead)
		if (!InspMesh)
		{
			InspMesh = NewObject<UStaticMeshComponent>(C, MakeUniqueObjectName(C, UStaticMeshComponent::StaticClass(), TEXT("YggInspectMesh")));
			InspMesh->SetupAttachment(Camera);
			InspMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			InspMesh->SetCastShadow(false);
			InspMesh->SetOnlyOwnerSee(true);
			InspMesh->SetFirstPersonPrimitiveType(EFirstPersonPrimitiveType::FirstPerson);
			InspMesh->RegisterComponent();
		}
		InspMesh->SetStaticMesh(I.Mesh);
		InspMesh->SetVisibility(true);
	}
	else if (InspMesh)
	{
		InspMesh->SetVisibility(false);
	}
	UE_LOG(LogYggView, Display, TEXT("Rig: inspect %s (beat %d)%s"), *I.Id.ToString(), Owner, I.bNeedBack ? TEXT(", the back must be seen") : TEXT(""));
}

bool UYggViewRigComponent::IsInspectBackFacing() const
{
	return FMath::Cos(FMath::DegreesToRadians(InspYaw)) * FMath::Cos(FMath::DegreesToRadians(InspPitch)) < 0.f;
}

FText UYggViewRigComponent::GetInspectCaption() const
{
	return IsInspectBackFacing() && !Insp.BackText.IsEmpty() ? Insp.BackText : Insp.Text;
}

void UYggViewRigComponent::TickInspect(float Dt)
{
	if (!bInspect)
	{
		InspAlpha = 0.f;
		return;
	}
	InspT += Dt;
	switch (InspPhase)
	{
	case EInspPhase::Rising:
		InspAlpha = Ease(InspT / FMath::Max(InspectRiseSec, 0.01f));
		if (InspT >= InspectRiseSec) { InspPhase = EInspPhase::Open; InspT = 0.f; }
		break;
	case EInspPhase::Open:
	{
		InspAlpha = 1.f;
		const float Dot = FMath::Cos(FMath::DegreesToRadians(InspYaw)) * FMath::Cos(FMath::DegreesToRadians(InspPitch));
		BackT = Dot < InspectBackDot ? BackT + Dt : 0.f;
		if (BackT >= InspectBackSec && !bBackSeen) { bBackSeen = true; }
		bool bClose = !HasLease(EYggLease::Inspect);
		// E, right mouse or Esc put it away (left mouse never does); not in the frame that opened it
		const APlayerController* PC = GetPC();
		if (!bClose && PC && InspT > 0.3f
			&& (PC->WasInputKeyJustPressed(EKeys::E) || PC->WasInputKeyJustPressed(EKeys::RightMouseButton)
				|| PC->WasInputKeyJustPressed(EKeys::Escape) || PC->WasInputKeyJustPressed(EKeys::Gamepad_FaceButton_Right)))
		{
			bClose = CanCloseInspect();
		}
		if (bClose) { InspPhase = EInspPhase::Pocket; InspT = 0.f; }
		break;
	}
	case EInspPhase::Pocket:
		InspAlpha = 1.f - Ease(InspT / FMath::Max(InspectPocketSec, 0.01f));
		if (InspT >= InspectPocketSec)
		{
			bInspect = false;
			InspAlpha = 0.f;
			QuietRelease(InspOwner, EYggLease::Inspect);
			if (InspMesh) { InspMesh->SetVisibility(false); }
			UE_LOG(LogYggView, Display, TEXT("Rig: inspect %s put away"), *Insp.Id.ToString());
		}
		break;
	}
	if (InspMesh && InspMesh->IsVisible())
	{
		// 40 cm out, 18 deg below the eye line, rising from lower down
		const float Below = FMath::DegreesToRadians(InspectBelowDeg);
		const FVector Rel(InspectDistCm * FMath::Cos(Below), 0.f, -InspectDistCm * FMath::Sin(Below) - (1.f - InspAlpha) * 15.f);
		InspMesh->SetRelativeLocationAndRotation(Rel, FRotator(InspPitch, 180.f + InspYaw, 0.f));
	}
}

void UYggViewRigComponent::SetHands(int32 Owner, FName Item)
{
	UFirstPersonBodyComponent* B = Body.Get();
	if (Item.IsNone())
	{
		if (!HandsItem.IsNone()) { QuietRelease(HandsOwner, EYggLease::Hands); }
		HandsItem = NAME_None;
		HandsOwner = -1;
		if (B) { B->SetHandsInUse(NAME_None, FVector::ZeroVector, FVector::ZeroVector); }
		return;
	}
	FVector L = FVector::ZeroVector, R = FVector::ZeroVector;
	for (const FYggHandsDef& H : Hands)
	{
		if (H.Name == Item) { L = H.Left; R = H.Right; break; }
	}
	AcquireLease(Owner, EYggLease::Hands, 0.f);
	HandsItem = Item;
	HandsOwner = Owner;
	if (B) { B->SetHandsInUse(Item, L, R); }   // zero spots: the body's own defaults for that item
	else { UE_LOG(LogYggView, Warning, TEXT("Rig: hands %s without a first-person body"), *Item.ToString()); }
}

// ------------------------------------------------------------------------------------------------ reset, watchdog

void UYggViewRigComponent::Cut()
{
	if (APlayerCameraManager* PCM = GetPCM())
	{
		PCM->SetGameCameraCutThisFrame();
	}
	LastCutTime = Clock;
}

bool UYggViewRigComponent::WasCutRecently() const
{
	return Clock - LastCutTime < 0.05;
}

void UYggViewRigComponent::HardReset(const FRotator& Facing, EYggView Pose)
{
	ACharacter* C = Char.Get();
	if (!C)
	{
		return;
	}
	LastHardReset = Clock;
	// a view-target switch or a big height snap is a camera cut (no temporal AA / motion blur smear across it)
	APlayerController* PCut = GetPC();
	const bool bVTChanged = PCut && PCut->GetViewTarget() != C;
	const float OldPoseH = PoseH;
	// a fly-out still running ends here (a jump or a stop in the middle of the shot)
	if (bFlyActive || bFlyWaiting)
	{
		bFlyActive = bFlyWaiting = bFlyEnding = false;
		if (FlyStreaming) { FlyStreaming->DisableStreamingSource(); }
		for (int32 i = Leases.Num() - 1; i >= 0; --i) { if (Leases[i].Kind == EYggLease::Fly) { Leases.RemoveAt(i); } }
	}
	// takeovers off, time and sound back to normal
	Scripted = EYggScripted::None;
	ScriptT = 0.f;
	TakeLoc = FVector::ZeroVector;
	TakeRot = FRotator::ZeroRotator;
	TakeFade = 0.f;
	bFallSnapPending = false;
	if (UWorld* World = GetWorld())
	{
		if (World->GetWorldSettings() && !FMath::IsNearlyEqual(World->GetWorldSettings()->TimeDilation, 1.f))
		{
			UGameplayStatics::SetGlobalTimeDilation(this, 1.f);
		}
		Dilation = 1.f;
		if (bAudioPitched || !FMath::IsNearlyEqual(LastAudioPitch, 1.f))
		{
			if (FAudioDeviceHandle Device = World->GetAudioDevice()) { Device->SetGlobalPitchModulation(1.f, 0.1f); }
			bAudioPitched = false;
			LastAudioPitch = 1.f;
		}
	}
	// the view on the pawn, the pawn seen, no plane constraint
	APlayerController* PC = GetPC();
	if (PC && PC->GetViewTarget() != C) { PC->SetViewTarget(C); }
	if (C->IsHidden()) { C->SetActorHiddenInGame(false); }
	bPawnHiddenByRig = false;
	if (UCharacterMovementComponent* Move = C->GetCharacterMovement())
	{
		if (Move->bConstrainToPlane) { Move->SetPlaneConstraintEnabled(false); }
	}
	// the facing (pitch normalised: control pitch is stored 0..360, never pass that on), the posture at once
	bPending = false;
	ShowProp(ShownProp, false);
	const float Yaw = Norm(Facing.Yaw);
	SnapPose(Pose, Yaw);
	if (bVTChanged || FMath::Abs(OldPoseH - PoseH) > 5.f) { Cut(); }
	const float Pitch = FMath::Clamp(Norm(Facing.Pitch), FMath::Max(LimPitchMin, BackstopPitchMin), FMath::Min(LimPitchMax, BackstopPitchMax));
	if (PC) { PC->SetControlRotation(FRotator(Pitch, Yaw, 0.f)); }
	if (Pose == EYggView::Walk)
	{
		bCatchUp = false;
		SetCtrlYawOwned(false);
		SetBodyYaw(Yaw);
	}
	// every spring at its target: the eye never glides after a teleport
	bTerrainInit = false;
	FootDropSm = Body.IsValid() ? Body->GetFootDrop() : 0.f;
	LandZ = LandV = 0.f;
	LandDownT = -1.f;
	NodX = NodV = 0.f;
	BobAmp = 0.f;
	BobZ = BobY = BobRoll = BobPitch = 0.f;
	LastStepTime = LastRealStepTime = Clock;
	SlopeFwdSm = SlopeDownSm = 0.f;
	PackStoop = View == EYggView::Walk ? FMath::Clamp((FeedPackKg - PackFromKg) * PackCmPerKg, 0.f, PackMaxCm) : 0.f;
	ExhaustHang = FMath::Clamp(FeedExhaust * 2.f, 0.f, 1.f);
	{
		const float Effort = Audio.IsValid() ? Audio->GetEffort() : 0.f;
		BSmooth = Effort * (0.6f + 0.4f * FMath::Clamp((AltM - AltGainFromM) / FMath::Max(AltGainRangeM, 1.f), 0.f, 1.f));
	}
	WatchdogLift = 0.f;
	LowTime = 0.f;
	bWatchdogTripped = false;
	GazeBend = 0.f;
	// the story lens at 0; a teleport also drops the nudge, attention and knock
	StoryFov = StoryFovFrom = StoryFovTo = 0.f;
	StoryFovLen = 0.f;
	AttnValue = 0.f;
	bAttn = false;
	FovBeatValue = 0.f;
	bFovBeat = false;
	if (bNudge) { EndNudge(TEXT("hard reset")); }
	bReflex = false;
	for (int32 i = Leases.Num() - 1; i >= 0; --i)
	{
		const EYggLease K = Leases[i].Kind;
		if (K == EYggLease::Nudge || K == EYggLease::Attention || K == EYggLease::Knock || K == EYggLease::StoryFov || K == EYggLease::FovBeat
			|| K == EYggLease::Fall)
		{
			Leases.RemoveAt(i);
		}
	}
	// the camera manager's backstops
	if (APlayerCameraManager* PCM = GetPCM(); PCM && bRigOn)
	{
		if (!bBackstopSet) { SavedPitchMin = PCM->ViewPitchMin; SavedPitchMax = PCM->ViewPitchMax; }
		PCM->ViewPitchMin = BackstopPitchMin;
		PCM->ViewPitchMax = BackstopPitchMax;
		bBackstopSet = true;
	}
	UE_LOG(LogYggView, Display, TEXT("Rig: hard reset (facing %.0f, %s)"), Yaw, ViewName(Pose));
}

void UYggViewRigComponent::Watchdog(float Dt)
{
	ACharacter* C = Char.Get();
	APlayerController* PC = GetPC();
	UCharacterMovementComponent* Move = C->GetCharacterMovement();

	// the view target is the pawn unless a fly-out holds it
	const bool bFly = HasLease(EYggLease::Fly) || bFlyActive || bFlyWaiting || bFlyEnding;
	if ((bFlyActive || bFlyWaiting) && !HasLease(EYggLease::Fly) && !bFlyEnding)
	{
		// the shot never cuts back to the eye with the eyes open: close the lids first (bFlyEnding keeps bFly true meanwhile)
		Repair(TEXT("fly-out ended (its lease is gone)"), EYggLease::Fly);
		bFlyEnding = true;
		if (Lid >= 0.99f || bFlyWaiting) { EndFly(true); }
		else { SetLids(-2, EYggLidMode::Closed, BlinkCloseSec, 0.f, [this]() { if (bFlyActive) { EndFly(true); } }); }
	}
	else if (bFlyActive && bFlyEnding && !LidOnClosed && Lid >= 0.99f)
	{
		// the closing's callback was dropped (a release all in the middle): end the shot behind the shut lids
		EndFly(true);
	}
	// (only the fly camera or no target at all is repaired: the story never sets a view target (R1), so any other one is
	// someone else's on purpose, e.g. a menu backdrop camera set by a level Blueprint, and must not be fought every blink)
	else if (PC && !bFly && PC->GetViewTarget() != C && !bRepairingViewTarget
		&& (PC->GetViewTarget() == nullptr || (FlyCam && PC->GetViewTarget() == FlyCam)))
	{
		bRepairingViewTarget = true;
		Repair(FString::Printf(TEXT("view target %s back to the pawn"), *GetNameSafe(PC->GetViewTarget())), EYggLease::Fly);
		SetLids(-2, EYggLidMode::Blink, BlinkCloseSec, 0.f, [this]()
		{
			APlayerController* P = GetPC();
			ACharacter* Ch = Char.Get();
			if (P && Ch && P->GetViewTarget() != Ch) { P->SetViewTarget(Ch); Cut(); }
			bRepairingViewTarget = false;
		});
	}
	// takeover offsets only with their lease; time and sound back to normal
	const bool bTakeLease = HasLease(EYggLease::Fall) || HasLease(EYggLease::Knock);
	if (!bTakeLease && (Scripted == EYggScripted::Fall || Scripted == EYggScripted::Knock))
	{
		Repair(FString::Printf(TEXT("%s takeover stopped (its lease is gone)"), ScriptedName(Scripted)), Scripted == EYggScripted::Fall ? EYggLease::Fall : EYggLease::Knock);
		Scripted = EYggScripted::None;
		TakeFadeLoc = TakeLoc;
		TakeFadeRot = TakeRot;
		TakeFadeLen = 0.3f;
		TakeFade = TakeFadeLen;
	}
	else if (!bTakeLease && TakeFade <= 0.f && !bFallSnapPending && Scripted == EYggScripted::None && (!TakeLoc.IsNearlyZero(0.01) || !TakeRot.IsNearlyZero(0.01f)))
	{
		Repair(TEXT("takeover offsets back to 0"), EYggLease::Fall);
		TakeFadeLoc = TakeLoc;
		TakeFadeRot = TakeRot;
		TakeFadeLen = 0.3f;
		TakeFade = TakeFadeLen;
	}
	if (!bTakeLease && Scripted != EYggScripted::Fall && (!FMath::IsNearlyEqual(Dilation, 1.f) || bAudioPitched))
	{
		Repair(TEXT("time dilation and audio pitch back to 1"), EYggLease::Fall);
		SetDilation(1.f, 1.f);
	}
	// walk shaping and jumping follow the leases and the posture
	if (!HasLease(EYggLease::Pace)) { PaceCmS = -1.f; }
	if (Move)
	{
		const bool bBlock = View != EYggView::Walk || HasLease(EYggLease::Move) || bInspect || Scripted != EYggScripted::None || bFlyActive;
		if (bBlock != bJumpBlocked)
		{
			if (bBlock) { bJumpWasAllowed = Move->IsJumpAllowed(); Move->SetJumpAllowed(false); }
			else { Move->SetJumpAllowed(bJumpWasAllowed); }
			bJumpBlocked = bBlock;
		}
	}
	// eyelids open again without a lease (a closed lid waits a little for the next beat to take over: slip -> dawn)
	if (!HasLease(EYggLease::Lids) && Lid > 0.001f && !(bLidMoving && (LidMode == EYggLidMode::Open || LidMode == EYggLidMode::Wake)))
	{
		LidNoLeaseTime += FMath::Min(Dt, 0.1f);   // a hitch (a spawn in the black) does not count
		if (Lid < 0.99f || LidNoLeaseTime >= LidGraceSec)
		{
			Repair(FString::Printf(TEXT("eyelids opened (%.0f%% closed, no Lids lease)"), Lid * 100.f), EYggLease::Lids);
			SetLids(-2, EYggLidMode::Open, BlinkOpenSec, 0.f, nullptr);
		}
	}
	else
	{
		LidNoLeaseTime = 0.f;
	}
	// story FOV without its lease: back to 0 over 1 s
	if (!HasLease(EYggLease::StoryFov) && FMath::Abs(StoryFov) > 0.01f && !(StoryFovLen > 0.f && FMath::Abs(StoryFovTo) < 0.01f))
	{
		Repair(TEXT("story FOV back to 0 (no lease)"), EYggLease::StoryFov);
		StoryFovFrom = StoryFov;
		StoryFovTo = 0.f;
		StoryFovT = 0.f;
		StoryFovLen = FMath::Max(BlendOutSec, 1.f);
	}
	// what ends with its lease, quietly (a beat that finished, a release)
	if (StoryB >= 0.f && !HasLease(EYggLease::Breath)) { StoryB = -1.f; }
	if (bNudge && !HasLease(EYggLease::Nudge)) { EndNudge(TEXT("its lease is gone")); }
	if (!HandsItem.IsNone() && !HasLease(EYggLease::Hands)) { SetHands(-2, NAME_None); }
	// the pawn is seen unless the fly-out hides it (only a hide of the rig's own is undone: a level Blueprint may hide the
	// pawn on purpose, e.g. behind a menu)
	if (bPawnHiddenByRig && !bFly)
	{
		bPawnHiddenByRig = false;
		if (C->IsHidden())
		{
			Repair(TEXT("pawn shown"), EYggLease::Fly);
			C->SetActorHiddenInGame(false);
		}
	}
	// the crawl's plane constraint only in the crawl
	if (Move && Move->bConstrainToPlane && View != EYggView::Crawl)
	{
		Repair(TEXT("plane constraint off (not crawling)"), EYggLease::Move);
		Move->SetPlaneConstraintEnabled(false);
	}
	// no control roll (ProcessViewRotation already zeroes it each frame)
	if (PC && FMath::Abs(Norm(PC->GetControlRotation().Roll)) > 0.01f)
	{
		Repair(TEXT("control roll set to 0"), EYggLease::Look);
		FRotator R = PC->GetControlRotation();
		R.Roll = 0.f;
		PC->SetControlRotation(R);
	}
	// the standing watchdog (the knee-height detector): walking fast with the eye 36 cm low for 0.5 s
	const bool bCheck = View == EYggView::Walk && Phase == EPhase::Holding && Speed2D > StandWatchdogSpeedCmS && bOnGround
		&& Clock - LastLandTime > 0.5 && !InTakeover() && !bFlyActive;
	const float Floor = StandEyeCm - StandWatchdogDropCm;
	if (bCheck && EyeAboveCapRaw + WatchdogLift < Floor)
	{
		LowTime += Dt;
		if (LowTime >= StandWatchdogSec && !bWatchdogTripped)
		{
			bWatchdogTripped = true;
			UE_LOG(LogYggView, Error, TEXT("Rig: repaired eye height: standing watchdog, eye %.0f cm over the capsule bottom while walking %.0f cm/s (beat %d)"),
				EyeAboveCapRaw, Speed2D, LastOwner[int32(EYggLease::Move)]);
			RecentRepairs.Add(TEXT("standing watchdog"));
			if (RecentRepairs.Num() > 4) { RecentRepairs.RemoveAt(0); }
		}
	}
	else
	{
		LowTime = 0.f;
		if (bWatchdogTripped && EyeAboveCapRaw >= Floor) { bWatchdogTripped = false; }
	}
	WatchdogLift = ExpTo(WatchdogLift, bWatchdogTripped ? FMath::Max(0.f, Floor - EyeAboveCapRaw) : 0.f, 0.15f, Dt);
}

// ------------------------------------------------------------------------------------------------ audit

FString UYggViewRigComponent::Audit() const
{
	// no signed live numbers here: the session monitor logs a "rig" event whenever the text changes beyond its digits
	static const TCHAR* const PhaseNames[] = { TEXT("holding"), TEXT("entering"), TEXT("leaving") };
	FString S = FString::Printf(TEXT("Rig %s | %s %s | eye %.0f cm | lid %.2f | takeover %s%s\n"), bRigOn ? TEXT("on") : TEXT("off"),
		ViewName(View), PhaseNames[FMath::Clamp(int32(Phase), 0, 2)], FMath::Max(EyeAboveFloor, 0.f), Lid, ScriptedName(Scripted),
		bPending ? *FString::Printf(TEXT(" | pose %s waiting"), ViewName(Pending.Pose)) : TEXT(""));
	S += TEXT("leases:");
	if (Leases.Num() == 0) { S += TEXT(" none"); }
	for (const FLease& L : Leases)
	{
		S += FString::Printf(TEXT(" %s(beat %d %.0f/%.0f s%s)"), LeaseName(L.Kind), L.Owner, float(Clock - L.Start), L.MaxSec, L.bKeep ? TEXT(" keep") : TEXT(""));
	}
	S += TEXT("\n");
	const APlayerController* PC = GetPC();
	const ACharacter* C = Char.Get();
	const UCharacterMovementComponent* Move = C ? C->GetCharacterMovement() : nullptr;
	S += FString::Printf(TEXT("view target %s | pawn %s | plane %s | dilation %.2f | jump %s\n"),
		PC ? *GetNameSafe(PC->GetViewTarget()) : TEXT("-"), C && C->IsHidden() ? TEXT("hidden") : TEXT("shown"),
		Move && Move->bConstrainToPlane ? TEXT("on") : TEXT("off"), Dilation, bJumpBlocked ? TEXT("blocked") : TEXT("free"));
	S += FString::Printf(TEXT("lens: story %.1f attention %.1f beat %.1f | exposure %d source(s) %d pulse(s) | nudges %d/%d%s | inspect %s | hands %s | pace %s\n"),
		FMath::Abs(StoryFov), FMath::Abs(AttnValue), FMath::Abs(FovBeatValue), ExposureSources.Num(), Pulses.Num(), NudgesUsed, NudgeBudget,
		bNudge ? TEXT(" (nudging)") : TEXT(""), bInspect ? *Insp.Id.ToString() : TEXT("-"), HandsItem.IsNone() ? TEXT("-") : *HandsItem.ToString(),
		PaceCmS > 0.f ? *FString::Printf(TEXT("%.1f m/s"), PaceCmS / 100.f) : TEXT("-"));
	S += TEXT("repairs:");
	if (RecentRepairs.Num() == 0) { S += TEXT(" none"); }
	for (const FString& R : RecentRepairs) { S += TEXT(" [") + R + TEXT("]"); }
	return S;
}

void UYggViewRigComponent::DrawDebug() const
{
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(uint64(0x59676756), 0.f, FColor(150, 220, 255), Audit());
	}
}

// ------------------------------------------------------------------------------------------------ modifier

UYggViewModifier::UYggViewModifier()
{
	Priority = 0;
}

bool UYggViewModifier::ProcessViewRotation(AActor* ViewTarget, float Dt, FRotator& OutViewRotation, FRotator& OutDeltaRot)
{
	if (UYggViewRigComponent* R = Rig.Get())
	{
		R->ProcessRotation(ViewTarget, Dt, OutViewRotation, OutDeltaRot);
	}
	return false;
}

bool UYggViewModifier::ModifyCamera(float Dt, FMinimalViewInfo& InOutPOV)
{
	if (UYggViewRigComponent* R = Rig.Get())
	{
		R->ApplyView(CameraOwner.Get(), Dt, InOutPOV);
	}
	return false;
}
