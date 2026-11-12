// Copyright Epic Games, Inc. All Rights Reserved.

#include "Environment/LightningBolt.h"
#include "ProceduralMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"

namespace
{
	// how thick a channel is drawn: never thinner than a couple of pixels, however far away
	constexpr float MinWidthCm = 90.f;
	constexpr float AngularWidth = 0.0011f;   // radians of view per unit of relative width
	constexpr float HaloScale = 7.f;          // the soft glow around the core
	constexpr float GlowEmissive = 90.f;      // emissive of a full stroke at Brightness 1
	constexpr float LeaderSeconds = 0.06f;    // the leader takes this long from the cloud to the ground
	constexpr float StrokeTau = 0.055f;       // how fast one stroke dies
}

ALightningBolt::ALightningBolt()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PostUpdateWork;
	Mesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("Bolt"));
	SetRootComponent(Mesh);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetCastShadow(false);
	Mesh->bAffectDistanceFieldLighting = false;
	Mesh->bAffectDynamicIndirectLighting = false;
	Mesh->bVisibleInRayTracing = false;
	Mesh->bUseAsyncCooking = true;
	Mesh->TranslucencySortPriority = 10;
	Mesh->SetMobility(EComponentMobility::Movable);
}

void ALightningBolt::Fire(FVector From, FVector To, float Branchiness, float InLife, float InBrightness, int32 InSeed)
{
	SetActorLocationAndRotation(From, FRotator::ZeroRotator);
	Seed = InSeed;
	Life = FMath::Max(InLife, 0.5f);
	Brightness = InBrightness;
	Branchiness = FMath::Clamp(Branchiness, 0.f, 1.f);
	FRandomStream R(Seed);
	Channels.Reset();
	Strokes.Reset();

	// the first stroke, then 1-4 return strokes down the same channel within about a second
	Strokes.Add(0.f);
	float T = 0.f;
	for (int32 i = R.RandRange(1, 4); i > 0; --i)
	{
		T += R.FRandRange(0.06f, 0.28f);
		Strokes.Add(T);
	}

	const FVector B = To - From;
	Grow(R, FVector::ZeroVector, B, 1.f, 0.7f, 1.f, 1.f, 0, Branchiness, 0.f, 1.f, B.Z);
	// sometimes the strike forks to the ground in two channels
	if (Channels.Num() > 0 && R.FRand() < 0.05f + 0.25f * Branchiness)
	{
		const FChannel& Main = Channels[0];
		const int32 k = FMath::Clamp(int32(Main.P.Num() * R.FRandRange(0.3f, 0.6f)), 1, Main.P.Num() - 2);
		const FVector Start = Main.P[k];
		const float Drop = Start.Z - B.Z;
		const FVector Side = FRotator(0.f, R.FRandRange(0.f, 360.f), 0.f).Vector() * Drop * R.FRandRange(0.2f, 0.45f);
		Grow(R, Start, FVector(Start.X + Side.X, Start.Y + Side.Y, B.Z), 0.75f, 0.6f, 0.85f, 0.9f, 0, Branchiness * 0.6f,
			Main.Reach[k], 1.f, B.Z);
	}

	if (!Mat)
	{
		if (UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Environment/Realms/M_LightningBolt.M_LightningBolt")))
		{
			Mat = UMaterialInstanceDynamic::Create(Base, this);
		}
	}
	Mesh->ClearAllMeshSections();
	bFired = true;
	Age = 0.f;
	FVector Eye = From - FVector(0.f, 0.f, 100000.f);
	if (const APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr)
	{
		if (PC->PlayerCameraManager) { Eye = PC->PlayerCameraManager->GetCameraLocation(); }
	}
	Draw(Eye);
}

void ALightningBolt::Grow(FRandomStream& R, const FVector& A, const FVector& B, float W0, float W1, float Bright, float FadeAt,
	int32 Level, float Br, float ReachA, float ReachB, float GroundZ)
{
	const float Len = FVector::Dist(A, B);
	if (Len < 200.f || Channels.Num() >= 160)
	{
		return;
	}
	// midpoint displacement: jagged at every scale, like a real channel
	TArray<FVector> Pts = { A, B };
	const float Rough = Level == 0 ? 0.19f : 0.25f;
	const float SegTarget = Level == 0 ? 2400.f : 1600.f;
	const int32 Steps = FMath::Clamp(FMath::CeilToInt(FMath::Log2(Len / SegTarget)), 1, 8);
	for (int32 d = 0; d < Steps; ++d)
	{
		TArray<FVector> Next;
		Next.Reserve(Pts.Num() * 2);
		for (int32 i = 0; i + 1 < Pts.Num(); ++i)
		{
			const FVector P = Pts[i], Q = Pts[i + 1];
			const FVector Dir = Q - P;
			const float L = Dir.Size();
			FVector Perp = R.GetUnitVector();
			Perp = (Perp - Dir.GetSafeNormal() * FVector::DotProduct(Perp, Dir.GetSafeNormal())).GetSafeNormal();
			const float Push = (R.FRand() + R.FRand() - 1.f) * L * Rough;
			Next.Add(P);
			Next.Add((P + Q) * 0.5f + Perp * Push);
		}
		Next.Add(Pts.Last());
		Pts = MoveTemp(Next);
	}
	for (FVector& P : Pts) { P.Z = FMath::Max(P.Z, GroundZ); }

	const int32 Index = Channels.AddDefaulted();
	{
		FChannel& C = Channels[Index];
		C.P = Pts;
		C.Reach.SetNum(Pts.Num());
		for (int32 i = 0; i < Pts.Num(); ++i) { C.Reach[i] = FMath::Lerp(ReachA, ReachB, float(i) / float(Pts.Num() - 1)); }
		C.Width0 = W0; C.Width1 = W1; C.Bright = Bright; C.FadeAt = FadeAt; C.Level = Level;
	}

	// side branches: more near the top, fewer and finer the deeper the tree goes
	if (Level >= 3 || Br <= 0.f)
	{
		return;
	}
	const float PerKm = (Level == 0 ? 9.f : Level == 1 ? 6.f : 4.f) * Br;
	const float SegKm = Len / float(Pts.Num() - 1) / 100000.f;
	const float Height = FMath::Max(Channels[0].P.Num() > 0 ? Channels[0].P[0].Z - GroundZ : Len, 1000.f);
	for (int32 i = 2; i + 3 < Pts.Num(); ++i)
	{
		const float U = float(i) / float(Pts.Num() - 1);
		if (R.FRand() >= PerKm * SegKm * (1.25f - 0.9f * U))
		{
			continue;
		}
		const FVector Tangent = (Pts[i + 1] - Pts[i - 1]).GetSafeNormal();
		const FVector Out = FRotator(0.f, R.FRandRange(0.f, 360.f), 0.f).Vector();
		const float Angle = FMath::DegreesToRadians(R.FRandRange(25.f, 60.f));
		FVector Dir = Tangent * FMath::Cos(Angle) + Out * FMath::Sin(Angle);
		Dir.Z -= 0.25f;
		Dir = Dir.GetSafeNormal();
		const float Remaining = Len * (1.f - U);
		float BranchLen = Remaining * R.FRandRange(0.15f, 0.5f) * (Level == 0 ? 1.f : 0.8f);
		if (Dir.Z < -0.05f) { BranchLen = FMath::Min(BranchLen, (Pts[i].Z - GroundZ) / -Dir.Z); }
		const float W = FMath::Lerp(W0, W1, U);
		const float ReachHere = FMath::Lerp(ReachA, ReachB, U);
		Grow(R, Pts[i], Pts[i] + Dir * BranchLen, W * R.FRandRange(0.35f, 0.55f), 0.12f, Bright * R.FRandRange(0.45f, 0.7f),
			FadeAt * R.FRandRange(0.35f, 0.6f), Level + 1, Br, ReachHere, FMath::Min(1.f, ReachHere + BranchLen / Height), GroundZ);
	}
}

void ALightningBolt::Envelope(float T, float& OutStroke, float& OutGlow) const
{
	OutStroke = 0.f;
	for (int32 i = 0; i < Strokes.Num(); ++i)
	{
		if (T >= Strokes[i])
		{
			// later strokes a little weaker, each its own
			const float Amp = i == 0 ? 1.f : 0.6f + 0.4f * FMath::Frac(FMath::Sin(float(Seed * 7 + i * 13)) * 43758.5f);
			OutStroke = FMath::Max(OutStroke, Amp * FMath::Exp(-(T - Strokes[i]) / StrokeTau));
		}
	}
	// the channel cools slowly: a fading afterglow with a faint shimmer, gone at Life
	const float K = FMath::Clamp(T / Life, 0.f, 1.f);
	const float Shimmer = 0.88f + 0.12f * FMath::Sin(T * 31.f + float(Seed)) * FMath::Sin(T * 17.f);
	// holds a faint glow most of its life (the thunder of a near strike comes while it is still seen), then goes
	OutGlow = T < 0.f ? 0.f : 0.12f * (0.6f + 0.4f * FMath::Exp(-T / 0.8f)) * (1.f - FMath::SmoothStep(0.6f, 1.f, K)) * Shimmer;
}

void ALightningBolt::Draw(const FVector& Eye)
{
	if (Channels.Num() == 0)
	{
		return;
	}
	float StrokeAll, Glow;
	Envelope(Age, StrokeAll, Glow);
	const float StrokeFirst = FMath::Exp(-FMath::Max(Age, 0.f) / StrokeTau);
	const float StrokeLater = Strokes.Num() > 1 && Age >= Strokes[1] ? StrokeAll : 0.f;
	LightNow = FMath::Clamp(FMath::Max(StrokeFirst, StrokeLater) * Brightness, 0.f, 1.f);

	// each channel's own brightness now: return strokes run down the main channel; branches mostly light only once
	TArray<float> Lit;
	Lit.SetNum(Channels.Num());
	float Peak = 0.001f;
	for (int32 c = 0; c < Channels.Num(); ++c)
	{
		const FChannel& C = Channels[c];
		const float Stroke = C.Level == 0 ? FMath::Max(StrokeFirst, StrokeLater) : FMath::Max(StrokeFirst, 0.15f * StrokeLater);
		const float Cool = C.Level == 0 ? 1.f : 1.f - FMath::SmoothStep(C.FadeAt * Life * 0.45f, C.FadeAt * Life, Age);
		Lit[c] = C.Bright * (Stroke + Glow * Cool * (C.Level == 0 ? 1.f : 0.6f));
		Peak = FMath::Max(Peak, Lit[c]);
	}

	const float Hot = FMath::Clamp(FMath::Max(StrokeFirst, StrokeLater) * 3.f, 0.f, 1.f);
	const FVector Origin = GetActorLocation();
	TArray<FVector> V[2];
	TArray<FVector2D> UV;
	TArray<FLinearColor> Col[2];
	TArray<int32> Tris;
	const bool bFirst = Mesh->GetNumSections() == 0;
	for (const FChannel& C : Channels)
	{
		const int32 N = C.P.Num();
		const int32 Base = V[0].Num();
		const float L = Lit[&C - Channels.GetData()] / Peak;
		for (int32 i = 0; i < N; ++i)
		{
			const float U = float(i) / float(N - 1);
			const FVector Tangent = (C.P[FMath::Min(i + 1, N - 1)] - C.P[FMath::Max(i - 1, 0)]).GetSafeNormal();
			const FVector World = Origin + C.P[i];
			const FVector ToEye = Eye - World;
			const float Dist = ToEye.Size();
			FVector Side = FVector::CrossProduct(Tangent, ToEye / FMath::Max(Dist, 1.f)).GetSafeNormal();
			if (Side.IsNearlyZero()) { Side = FVector::RightVector; }
			// a cooling channel narrows to a thread
			const float W = FMath::Lerp(C.Width0, C.Width1, U) * FMath::Max(MinWidthCm, Dist * AngularWidth) * FMath::Lerp(0.55f, 1.f, Hot);
			// the leader comes down first; a branch's tip dims away
			const float Shown = FMath::SmoothStep(C.Reach[i] * LeaderSeconds - 0.012f, C.Reach[i] * LeaderSeconds, Age);
			const float Tip = C.Level == 0 ? 1.f : 1.f - FMath::SmoothStep(0.8f, 1.f, U);
			const float I = L * Shown * Tip;
			for (int32 s = 0; s < 2; ++s)
			{
				const float Half = 0.5f * W * (s == 0 ? 1.f : HaloScale);
				V[s].Add(C.P[i] - Side * Half);
				V[s].Add(C.P[i] + Side * Half);
				const FLinearColor Cl(I * (s == 0 ? 1.f : 0.1f), 0.f, 0.f, 1.f);
				Col[s].Add(Cl);
				Col[s].Add(Cl);
			}
			if (bFirst)
			{
				UV.Add(FVector2D(U, 0.0));
				UV.Add(FVector2D(U, 1.0));
			}
		}
		if (bFirst)
		{
			for (int32 i = 0; i + 1 < N; ++i)
			{
				const int32 a = Base + i * 2;
				Tris.Append({ a, a + 2, a + 1, a + 1, a + 2, a + 3 });
			}
		}
	}

	const TArray<FVector> NoNormals;
	const TArray<FProcMeshTangent> NoTangents;
	for (int32 s = 0; s < 2; ++s)
	{
		if (bFirst)
		{
			Mesh->CreateMeshSection_LinearColor(s, V[s], Tris, NoNormals, UV, Col[s], NoTangents, false);
			if (Mat) { Mesh->SetMaterial(s, Mat); }
		}
		else
		{
			Mesh->UpdateMeshSection_LinearColor(s, V[s], NoNormals, TArray<FVector2D>(), Col[s], NoTangents);
		}
	}
	if (Mat)
	{
		// white-blue in the strokes, cooling to violet in the afterglow
		const FLinearColor Cool = FMath::Lerp(FLinearColor(0.7f, 0.72f, 1.f), FLinearColor(0.55f, 0.42f, 1.f), FMath::Clamp(Age / Life, 0.f, 1.f));
		Mat->SetVectorParameterValue(TEXT("Color"), FMath::Lerp(Cool, FLinearColor(0.85f, 0.92f, 1.f), Hot));
		Mat->SetScalarParameterValue(TEXT("Glow"), GlowEmissive * Brightness * Peak);
	}
}

void ALightningBolt::Preview(FVector Eye, float InAge)
{
	if (!bFired)
	{
		return;
	}
	Age = InAge;
	Draw(Eye);
}

void ALightningBolt::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!bFired)
	{
		return;
	}
	Age += DeltaSeconds;
	if (Age > Life + 0.1f)
	{
		Destroy();
		return;
	}
	if (const APlayerController* PC = GetWorld()->GetFirstPlayerController())
	{
		if (PC->PlayerCameraManager) { Draw(PC->PlayerCameraManager->GetCameraLocation()); }
	}
}
