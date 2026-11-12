// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/Widget.h"
#include "YggFateFx.generated.h"

/**
 *  The death screen's art (2026-11-13, user: "if you've frozen to death, the font should be frozen ... make it artistic").
 *  Each way of dying has its own title and its own screen:
 *  - Frozen: the letters shiver, then freeze ice-blue; icicles grow under them, frost glints on top; frost creeps in from
 *    the corners of the screen along every edge.
 *  - Breathless: the letters breathe in short gasps and drift apart, some go dark; the view tunnels in and out.
 *  - Fallen: the letters fall into place and land crooked; the screen cracks from where they hit, with a jolt.
 *  - Buried: the letters sink and a drift of snow rises over their feet; heavy snow, a white haze from below.
 *  - Parched: the letters turn sand-coloured, shimmer in the heat and crack; warm dust in a burnt vignette.
 *  - Starved: the letters grow faint unevenly and sag; a grey, near-empty world.
 *  - Lost: the word is written by hand, letter by letter, then drifts apart into the fog.
 *  One UYggFateFx sits over the title (title effects, letter rectangles given each frame by the menu), one over the
 *  whole screen. Vector lines and the menus' soft textures only.
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

struct FYggFateState
{
	uint8 Fate = 0;
	bool bScreen = false;
	double Start = 0.0;
	TArray<FBox2f> Letters;                     // title layer: each letter's box in the layer's own space
	FVector2f AnchorAbs = FVector2f(-1.f, -1.f); // screen layer: where the title ends (cracks start there), absolute
};

UCLASS()
class YGGDRASILSPEAK_API UYggFateFx : public UWidget
{
	GENERATED_BODY()

public:
	TSharedPtr<FYggFateState> State = MakeShared<FYggFateState>();
	void Restart(uint8 InFate, bool bInScreen);
	virtual void ReleaseSlateResources(bool bReleaseChildren) override;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	TSharedPtr<SWidget> Slate;
};
