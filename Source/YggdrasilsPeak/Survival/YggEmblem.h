// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/Widget.h"
#include "YggEmblem.generated.h"

class FSlateWindowElementList;

/**
 *  The game's emblem, drawn in hairlines (2026-11-13): a ring with nine beads for the nine realms, the mountain, and
 *  the World Tree rising from its summit, rooted inside it. It draws itself on when it appears (the ring first, then
 *  the mountain, then the tree), and a gold light wanders along the nine beads. Vector lines, any size, no texture.
 *  YggEmblem::Paint is shared with the loading screen (plain Slate).
 */
namespace YggEmblem
{
	/** Centre and diameter in the geometry's local space; Time in seconds since it appeared; Alpha 0..1;
	 *  Spin 0 = the realms' light wanders slowly, 1 = it runs around the ring (loading). */
	void Paint(FSlateWindowElementList& Out, int32 Layer, const FGeometry& G, const FVector2f& Centre, float Diameter, float Time, float Alpha, float Spin = 0.f);
}

UCLASS()
class YGGDRASILSPEAK_API UYggEmblem : public UWidget
{
	GENERATED_BODY()

public:
	float Diameter = 120.f;
	float Spin = 0.f;

	virtual void ReleaseSlateResources(bool bReleaseChildren) override;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	TSharedPtr<SWidget> Slate;
};
