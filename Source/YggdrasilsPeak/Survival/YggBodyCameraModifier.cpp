#include "Survival/YggBodyCameraModifier.h"

bool UYggBodyCameraModifier::ModifyCamera(float DeltaTime, FMinimalViewInfo& InOutPOV)
{
	Super::ModifyCamera(DeltaTime, InOutPOV);
	Clock += DeltaTime;
	if (Shiver > 0.001f)
	{
		// shivering: a fast uneven tremor, 8-13 Hz, a quarter of a degree at most
		const float A = 0.25f * Shiver;
		InOutPOV.Rotation.Pitch += A * (FMath::Sin(Clock * 60.f) * 0.6f + FMath::Sin(Clock * 83.f + 1.7f) * 0.4f);
		InOutPOV.Rotation.Yaw += A * 0.7f * (FMath::Sin(Clock * 53.f + 0.6f) * 0.6f + FMath::Sin(Clock * 77.f + 2.9f) * 0.4f);
		InOutPOV.Rotation.Roll += A * 0.5f * FMath::Sin(Clock * 67.f + 4.1f);
	}
	if (Sway > 0.001f)
	{
		// spent: the head hangs a little and sways with the breath
		const float Breath = 0.5f + 0.5f * FMath::Sin(Clock * 1.4f);
		InOutPOV.Rotation.Pitch += Sway * (-1.0f + 0.9f * FMath::Sin(Clock * 1.4f));
		InOutPOV.Rotation.Roll += Sway * 0.8f * FMath::Sin(Clock * 0.7f + 0.5f);
		InOutPOV.Location.Z -= Sway * 2.5f * Breath;
	}
	if (Pulse > 0.001f)
	{
		// the heartbeat pulls the view in, a fraction of a degree
		InOutPOV.FOV -= 0.9f * Pulse;
	}
	return false;
}
