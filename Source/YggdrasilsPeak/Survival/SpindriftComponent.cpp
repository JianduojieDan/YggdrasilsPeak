// Copyright Epic Games, Inc. All Rights Reserved.

#include "Survival/SpindriftComponent.h"

#include "Survival/ExpeditionComponent.h"
#include "Survival/SurvivalComponent.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/KismetMaterialLibrary.h"
#include "Materials/MaterialParameterCollection.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "UObject/ConstructorHelpers.h"

static TAutoConsoleVariable<float> CVarSpindrift(
	TEXT("ygg.Snow.Spindrift"), -1.f,
	TEXT("Blowing snow: -1 = follow the weather, 0..1 = force this strength (0 turns it off)."));

USpindriftComponent::USpindriftComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;

	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> Effect(TEXT("/Game/Environment/Snow/NS_Spindrift.NS_Spindrift"));
	GustEffect = Effect.Object;
	static ConstructorHelpers::FObjectFinder<UMaterialParameterCollection> Collection(TEXT("/Game/Environment/Snow/MPC_Wind.MPC_Wind"));
	WindCollection = Collection.Object;
}

void USpindriftComponent::BeginPlay()
{
	Super::BeginPlay();

	Expedition = GetOwner()->FindComponentByClass<UExpeditionComponent>();
	if (GustEffect)
	{
		// Follows the player but not their turning; the particles themselves live in world space.
		Gusts = UNiagaraFunctionLibrary::SpawnSystemAttached(GustEffect, GetOwner()->GetRootComponent(), NAME_None,
			FVector::ZeroVector, FRotator::ZeroRotator, EAttachLocation::KeepRelativeOffset, false);
		if (Gusts)
		{
			Gusts->SetUsingAbsoluteRotation(true);
		}
	}
}

void USpindriftComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (WindCollection && GetWorld())
	{
		UKismetMaterialLibrary::SetVectorParameterValue(this, WindCollection, TEXT("WindDir"), FLinearColor(1.f, 0.f, 0.f, 0.f));
	}
	if (Gusts)
	{
		Gusts->DestroyComponent();
	}
	Super::EndPlay(EndPlayReason);
}

void USpindriftComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	const AActor* Owner = GetOwner();
	const float Intensity = Expedition.IsValid() ? Expedition->GetWindIntensity() : 4.f;
	const float Yaw = Expedition.IsValid() ? Expedition->GetWindDirection() : 15.f;

	// Target strength: the wind, plus a bit more the higher you are.
	float Target = FMath::GetMappedRangeValueClamped(IntensityRange, FVector2D(0.f, 1.f), Intensity);
	if (const USurvivalComponent* Survival = Owner->FindComponentByClass<USurvivalComponent>())
	{
		Target += AltitudeBoost * FMath::GetMappedRangeValueClamped(AltitudeRange, FVector2D(0.f, 1.f), Survival->GetElevationMeters()) * FMath::Max(Target, 0.3f);
	}
	const float Forced = CVarSpindrift.GetValueOnGameThread();
	Target = Forced >= 0.f ? Forced : FMath::Clamp(Target, 0.f, 1.f);

	const float Ease = 1.f - FMath::Exp(-DeltaTime / Smoothing);
	Strength = FMath::Lerp(Strength, Target, Ease);
	// Turn by angle (a vector lerp towards the opposite direction would never get past the halfway point).
	const float WantedAngle = FMath::DegreesToRadians(Yaw);
	// The weather is first read a moment after spawning: take its direction as is until then.
	Age += DeltaTime;
	DirectionAngle = Age > 1.f ? DirectionAngle + FMath::FindDeltaAngleRadians(DirectionAngle, WantedAngle) * Ease : WantedAngle;
	Direction = FVector2D(FMath::Cos(DirectionAngle), FMath::Sin(DirectionAngle));
	// Strong blowing snow needs strong wind: high up (or when forced) the snow moves faster than the valley wind.
	SpeedCm = 100.f * FMath::Lerp(WindSpeedRange.X, WindSpeedRange.Y, FMath::Clamp(FMath::Max(Intensity / 10.f, Strength * 0.8f), 0.f, 1.f));

	// Streak frame: p = R(dir) * (xy - Anchor) - Scroll. Re-anchor at the player when the wind turns or they walk far,
	// keeping p unchanged at the player so nothing jumps there.
	const FVector2D Player(Owner->GetActorLocation());
	auto Rotate = [](const FVector2D& Dir, const FVector2D& V) { return FVector2D(V | Dir, V.X * -Dir.Y + V.Y * Dir.X); };
	if (!bHasFrame)
	{
		FrameDirection = Direction;
		Anchor = Player;
		bHasFrame = true;
	}
	else if ((Direction | FrameDirection) < FMath::Cos(FMath::DegreesToRadians(0.25f)) || FVector2D::DistSquared(Player, Anchor) > FMath::Square(50000.f))
	{
		Scroll -= Rotate(FrameDirection, Player - Anchor);
		Anchor = Player;
		FrameDirection = Direction;
	}
	Scroll.X += SpeedCm * SurfaceSpeedFraction * DeltaTime;
	// The streak texture tiles, so the offset can wrap by whole tiles.
	Scroll.X = FMath::Fmod(Scroll.X, StreakTileCm.X);
	Scroll.Y = FMath::Fmod(Scroll.Y, StreakTileCm.Y);

	if (WindCollection)
	{
		UKismetMaterialLibrary::SetVectorParameterValue(this, WindCollection, TEXT("WindDir"),
			FLinearColor(FrameDirection.X, FrameDirection.Y, Strength, SpeedCm / 100.f));
		UKismetMaterialLibrary::SetVectorParameterValue(this, WindCollection, TEXT("WindAnchor"),
			FLinearColor(Anchor.X, Anchor.Y, Scroll.X, Scroll.Y));
	}

	if (Gusts)
	{
		Gusts->SetVariableFloat(TEXT("Intensity"), Strength);
		Gusts->SetVariableVec3(TEXT("WindVelocity"), FVector(Direction * SpeedCm, 0.f));
		const bool bWanted = Strength > 0.02f;
		if (bWanted != Gusts->IsActive())
		{
			bWanted ? Gusts->Activate() : Gusts->Deactivate();
		}
	}
}
