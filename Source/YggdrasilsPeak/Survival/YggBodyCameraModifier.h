// What the body does to the view (2026-11-13, "a death zone you can feel"): UExpeditionComponent sets these each frame.

#pragma once

#include "CoreMinimal.h"
#include "Camera/CameraModifier.h"
#include "YggBodyCameraModifier.generated.h"

/** Shivering when the core is cold, a slow heavy sway when spent, a small pull of the view on each heartbeat when the
 *  air is thin. Added to the player's camera manager by UExpeditionComponent; all values 0..1, 0 = no effect. */
UCLASS()
class YGGDRASILSPEAK_API UYggBodyCameraModifier : public UCameraModifier
{
	GENERATED_BODY()

public:
	float Shiver = 0.f;
	float Sway = 0.f;
	float Pulse = 0.f;

	virtual bool ModifyCamera(float DeltaTime, FMinimalViewInfo& InOutPOV) override;

private:
	float Clock = 0.f;
};
