// The thesis data logger (Development_Plan.md block A3): one CSV row per second of play with the climber's body and
// surroundings, so every session can be charted (Tools/Thesis/plot_session.py).

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Tasks/Task.h"
#include "YggTelemetryComponent.generated.h"

/**
 *  Writes Saved/Telemetry/session_<date>_<time>.csv while ygg.Log is 1 (default in development builds, 0 in shipping):
 *  position, altitude, speed, what the climber is doing, SpO2, heart rate, core temperature, stamina, food, water,
 *  health, sanity, bottled oxygen, shelter, realm, air temperature, the body effects on the view, and events (every
 *  expedition message, deaths). Rows are buffered and appended every few seconds; nothing is written while off.
 *  The file is written by background tasks, one after another (2026-11-14: appending on the game thread froze the game
 *  for as long as the drive stalled). Added to the local player by UExpeditionComponent.
 */
UCLASS(ClassGroup=(Survival))
class YGGDRASILSPEAK_API UYggTelemetryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UYggTelemetryComponent();

	/** Adds a note to the events column of the next row. */
	void Note(const FString& Event);
	const FString& GetFilePath() const { return FilePath; }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	UFUNCTION()
	void HandleMessage(const FText& Message, float Seconds);
	void WriteRow();
	void Flush();
	/** Hands Text to a background task that writes it to FilePath (bReplace: the file starts over, else it is appended),
	 *  after the write handed over before it, so the lines keep their order. */
	void Write(FString Text, bool bReplace);

	FString FilePath;
	FString Buffer;
	UE::Tasks::FTask LastWrite;         // the newest write; the next one waits for it, EndPlay waits for the last
	TArray<FString> Pending;
	float SinceRow = 0.f;
	float SinceFlush = 0.f;
	double StartSeconds = 0.0;
	FVector LastPosition = FVector::ZeroVector;
	bool bHavePosition = false;
	bool bOn = false;
	bool bWasDead = false;
	int32 Rows = 0;
};
