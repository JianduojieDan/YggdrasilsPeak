// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "EngineUtils.h"
#include "UObject/UnrealType.h"

/** Time of day as Ultra Dynamic Sky keeps it ("Time of Day", 0..2400 = hours * 100), read and set by reflection. */
namespace YggTime
{
	inline AActor* FindSky(UWorld* World)
	{
		// walking every actor by name is too dear to do each frame
		static TWeakObjectPtr<AActor> Cached;
		if (Cached.IsValid() && Cached->GetWorld() == World) { return Cached.Get(); }
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			if (It->GetClass()->GetName().Contains(TEXT("Ultra_Dynamic_Sky")))
			{
				Cached = *It;
				return *It;
			}
		}
		return nullptr;
	}

	/** 0..2400, or 1200 when there is no sky. */
	inline float Get(UWorld* World)
	{
		AActor* Sky = World ? FindSky(World) : nullptr;
		const FProperty* P = Sky ? Sky->GetClass()->FindPropertyByName(TEXT("Time of Day")) : nullptr;
		if (const FDoubleProperty* D = CastField<FDoubleProperty>(P)) return float(D->GetPropertyValue_InContainer(Sky));
		if (const FFloatProperty* F = CastField<FFloatProperty>(P)) return F->GetPropertyValue_InContainer(Sky);
		return 1200.f;
	}

	inline void Set(UWorld* World, float Value)
	{
		AActor* Sky = World ? FindSky(World) : nullptr;
		const FProperty* P = Sky ? Sky->GetClass()->FindPropertyByName(TEXT("Time of Day")) : nullptr;
		Value = FMath::Fmod(FMath::Fmod(Value, 2400.f) + 2400.f, 2400.f);
		if (const FDoubleProperty* D = CastField<FDoubleProperty>(P)) D->SetPropertyValue_InContainer(Sky, Value);
		else if (const FFloatProperty* F = CastField<FFloatProperty>(P)) F->SetPropertyValue_InContainer(Sky, Value);
	}

	/** "04:30" from 450 (hours * 100, minutes as a fraction of the hour). */
	inline FString Clock(float Value)
	{
		const int32 H = FMath::FloorToInt(Value / 100.f) % 24;
		const int32 M = FMath::Clamp(FMath::RoundToInt(FMath::Fmod(Value, 100.f) * 0.6f), 0, 59);
		return FString::Printf(TEXT("%02d:%02d"), H, M);
	}
}
