// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "YggLoadingScreen.generated.h"

class UFont;
class UTexture2D;

/**
 *  The loading screen on the way to the mountain (2026-11-12): a still of the mountain slowly drifting, the place you
 *  arrive at in the serif, one line - a thought of the story or a climber's tip - and a hairline with a gold bead
 *  that travels while the world loads. Drawn by the movie player on its own thread, so it keeps moving while the
 *  level loads (packaged game and standalone; the editor's Play loads in place and shows nothing).
 */
UCLASS()
class YGGDRASILSPEAK_API UYggLoadingScreen : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

private:
	void OnPreLoadMap(const FString& MapName);
	void OnPostLoadMap(UWorld* World);

	UPROPERTY(Transient) TObjectPtr<UTexture2D> Still;
	UPROPERTY(Transient) TObjectPtr<UFont> Serif;
	FDelegateHandle PreHandle, PostHandle;
};
