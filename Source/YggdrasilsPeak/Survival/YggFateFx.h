// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 *  How a death is shown (2026-11-13, user: "if you've frozen to death, the font should be frozen ... make it artistic";
 *  later the same day: the effects were "not good enough" - redone). Two parts:
 *  - The world behind the death screen turns into the cause: M_PP_Fate, a post-process material on an unbound volume
 *    (Tools/UI/shaders/pp_fate.hlsl): Frozen = frost grows over the lens from the edges, crystals catch the light, the
 *    world under it blurs and goes cold; Breathless = the view tunnels in with each gasp, the edges smear and drain;
 *    Fallen = a jolt, then walls of blue glacier ice close over the last sliver of sky; Buried = the avalanche's snow
 *    rises over the view, powder hangs above it, the light goes; Parched = heat shimmer, bleached dust colour, a dry
 *    crust cracking in from the edges; Starved = the colour drains, the sight fades and comes back weaker; Lost = a
 *    whiteout swallows everything, spindrift streams through it.
 *  - The title's letters act it out (UYggMenuBase::AnimateFate): shiver and freeze, gasp, fall and land crooked, sink,
 *    shimmer, fade unevenly, or (Lost) are written by hand and drift apart.
 *  (The v2 vector art, UYggFateFx, is gone.)
 */
namespace YggFate
{
	enum : uint8 { None, Frozen, Breathless, Fallen, Buried, Parched, Starved, Lost };
	/** 0..1, a short in-breath and a long letting go (3.4 s). */
	inline float Breath(float T)
	{
		const float Ph = FMath::Fmod(FMath::Max(T, 0.f), 3.4f) / 3.4f;
		auto E = [](float X) { X = FMath::Clamp(X, 0.f, 1.f); return X * X * (3.f - 2.f * X); };
		return Ph < 0.25f ? E(Ph / 0.25f) : 1.f - E((Ph - 0.25f) / 0.75f);
	}
}
