#include "Survival/YggTelemetryComponent.h"
#include "Survival/ExpeditionComponent.h"
#include "Survival/SurvivalComponent.h"
#include "Survival/SanityComponent.h"
#include "Survival/ClimbingComponent.h"
#include "Survival/YggTime.h"
#include "Environment/RealmSky.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "HAL/FileManager.h"
#include "HAL/IConsoleManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

static TAutoConsoleVariable<int32> CVarTelemetry(TEXT("ygg.Log"),
#if UE_BUILD_SHIPPING
	0,
#else
	1,
#endif
	TEXT("1 = write the thesis data log (Saved/Telemetry/session_*.csv, one row per ygg.Log.Interval seconds), 0 = off."));
static TAutoConsoleVariable<float> CVarTelemetryInterval(TEXT("ygg.Log.Interval"), 1.f, TEXT("Seconds between rows of the thesis data log."));

namespace
{
	// prefixed: this file and YggSessionMonitor.cpp both had a Csv(), and a unity build puts both in one translation unit
	const TCHAR* TelemetryCsvHeader = TEXT("t_s,uds_time,x_m,y_m,z_m,alt_m,speed_ms,climb_ms,state,spo2,hr_bpm,core_c,stamina,satiety,hydration,health,sanity,"
		"bottle_s,bottles,sheltered,on_rope,realm,air_c,fx_hypoxia,fx_frost,fx_exhaust,fx_faint,events\n");

	FString TelemetryCsv(FString S)
	{
		S.ReplaceInline(TEXT("\""), TEXT("'"));
		S.ReplaceInline(TEXT("\n"), TEXT(" "));
		return TEXT("\"") + S + TEXT("\"");
	}
}

UYggTelemetryComponent::UYggTelemetryComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
}

void UYggTelemetryComponent::BeginPlay()
{
	Super::BeginPlay();
	bOn = CVarTelemetry.GetValueOnGameThread() != 0;
	if (!bOn)
	{
		return;
	}
	FilePath = FPaths::ProjectSavedDir() / TEXT("Telemetry") / FString::Printf(TEXT("session_%s.csv"), *FDateTime::Now().ToString(TEXT("%Y%m%d_%H%M%S")));
	Write(TelemetryCsvHeader, true);                    // the folder is made and the file started by the first write
	StartSeconds = GetWorld()->GetTimeSeconds();
	if (UExpeditionComponent* Exp = GetOwner()->FindComponentByClass<UExpeditionComponent>())
	{
		Exp->OnMessage.AddDynamic(this, &UYggTelemetryComponent::HandleMessage);
	}
	Note(TEXT("session start"));
	UE_LOG(LogTemp, Display, TEXT("Telemetry: logging to %s"), *FilePath);
}

void UYggTelemetryComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (bOn)
	{
		Note(TEXT("session end"));
		WriteRow();
		Flush();
		// the last rows must reach the file before the game closes (the task system may drop work still queued at exit);
		// a stalled drive holds the end of the level up to 10 s, never the play
		if (!LastWrite.Wait(FTimespan::FromSeconds(10.0)))
		{
			UE_LOG(LogTemp, Warning, TEXT("Telemetry: the last rows were still being written after 10 s (slow drive)"));
		}
		UE_LOG(LogTemp, Display, TEXT("Telemetry: %d rows in %s"), Rows, *FilePath);
	}
	Super::EndPlay(EndPlayReason);
}

void UYggTelemetryComponent::Note(const FString& Event)
{
	Pending.Add(Event);
}

void UYggTelemetryComponent::HandleMessage(const FText& Message, float Seconds)
{
	Note(Message.ToString());
}

void UYggTelemetryComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (!bOn)
	{
		return;
	}
	SinceRow += DeltaTime;
	SinceFlush += DeltaTime;
	if (SinceRow >= FMath::Max(0.1f, CVarTelemetryInterval.GetValueOnGameThread()))
	{
		WriteRow();
		SinceRow = 0.f;
	}
	if (SinceFlush >= 5.f)
	{
		Flush();
		SinceFlush = 0.f;
	}
}

void UYggTelemetryComponent::WriteRow()
{
	const AActor* Owner = GetOwner();
	const USurvivalComponent* Body = Owner->FindComponentByClass<USurvivalComponent>();
	if (!Body)
	{
		return;
	}
	const FSurvivalSnapshot Snap = Body->GetSnapshot();
	const UExpeditionComponent* Exp = Owner->FindComponentByClass<UExpeditionComponent>();
	const USanityComponent* Mind = Owner->FindComponentByClass<USanityComponent>();
	const UClimbingComponent* Climb = Owner->FindComponentByClass<UClimbingComponent>();
	const ACharacter* Character = Cast<ACharacter>(Owner);

	// speed over the last row (horizontal and vertical), from positions so teleports show as jumps, not speed spikes
	const FVector Pos = Owner->GetActorLocation();
	const float Dt = FMath::Max(SinceRow, 0.01f);
	float Speed = 0.f, Climbing = 0.f;
	if (bHavePosition)
	{
		const FVector D = Pos - LastPosition;
		if (D.Size() < 5000.f * Dt)           // faster than 50 m/s is a teleport (camp jump, respawn)
		{
			Speed = float(D.Size2D()) / 100.f / Dt;
			Climbing = float(D.Z) / 100.f / Dt;
		}
		else
		{
			Pending.Add(TEXT("teleport"));
		}
	}
	LastPosition = Pos;
	bHavePosition = true;

	const TCHAR* State = TEXT("rest");
	if (Snap.bDead) { State = TEXT("dead"); }
	else if (Climb && Climb->IsClimbing()) { State = TEXT("climb"); }
	else if (Character && Character->GetCharacterMovement() && Character->GetCharacterMovement()->IsFalling()) { State = TEXT("air"); }
	else if (Snap.bSprinting) { State = TEXT("sprint"); }
	else if (Speed > 0.3f) { State = Snap.bOnRope ? TEXT("rope") : TEXT("walk"); }

	if (Snap.bDead && !bWasDead) { Pending.Add(TEXT("death")); }
	bWasDead = Snap.bDead;

	float Hyp = 0.f, Frost = 0.f, Ex = 0.f, Faint = 0.f;
	if (Exp) { Exp->GetBodyEffects(Hyp, Frost, Ex, Faint); }
	const ARealmSky* Sky = ARealmSky::Get(GetWorld());
	const FString Realm = Sky ? (Sky->GetShownRealm() == ERealm::None ? FString(TEXT("none")) : FString(ARealmSky::GetWeatherInfo(Sky->GetShownIndex()).Name)) : FString(TEXT("-"));
	const float Air = Sky ? Sky->GetAirTemperature(Snap.ElevationMeters) : 0.f;

	Buffer += FString::Printf(TEXT("%.1f,%.0f,%.1f,%.1f,%.1f,%.1f,%.2f,%.3f,%s,%.1f,%.0f,%.2f,%.1f,%.1f,%.1f,%.1f,%.1f,%.0f,%d,%d,%d,%s,%.1f,%.3f,%.3f,%.3f,%.3f,%s\n"),
		GetWorld()->GetTimeSeconds() - StartSeconds, YggTime::Get(GetWorld()),
		Pos.X / 100.f, Pos.Y / 100.f, Pos.Z / 100.f, Snap.ElevationMeters, Speed, Climbing, State,
		Snap.Oxygen, Exp ? Exp->GetHeartRate() : 0.f, Snap.BodyTemperature, Snap.Stamina, Snap.Satiety, Snap.Hydration, Snap.Health,
		Mind ? Mind->GetSanity() : 100.f, Snap.OxygenBottleSeconds, Snap.CarriedBottles, Snap.bSheltered ? 1 : 0, Snap.bOnRope ? 1 : 0,
		*TelemetryCsv(Realm), Air, Hyp, Frost, Ex, Faint, *TelemetryCsv(FString::Join(Pending, TEXT(" | "))));
	Pending.Reset();
	++Rows;
}

void UYggTelemetryComponent::Flush()
{
	if (Buffer.IsEmpty() || FilePath.IsEmpty())
	{
		return;
	}
	const int32 Len = Buffer.Len();
	Write(MoveTemp(Buffer), false);
	Buffer.Reset(Len + Len / 4);                // the next five seconds' rows fit without growing the string row by row
}

void UYggTelemetryComponent::Write(FString Text, bool bReplace)
{
	if (FilePath.IsEmpty() || Text.IsEmpty())
	{
		return;
	}
	// A background task, after the previous write (its prerequisite), so the rows reach the file in order. Background
	// priority: a write stuck on a stalled drive holds one background worker, not the game thread nor the workers the
	// frame waits on.
	LastWrite = UE::Tasks::Launch(TEXT("YggTelemetryWrite"),
		[Path = FilePath, Text = MoveTemp(Text), bReplace]()
		{
			if (bReplace)
			{
				IFileManager::Get().MakeDirectory(*FPaths::GetPath(Path), true);
			}
			FFileHelper::SaveStringToFile(Text, *Path, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM, &IFileManager::Get(), bReplace ? 0 : FILEWRITE_Append);
		},
		UE::Tasks::Prerequisites(LastWrite), UE::Tasks::ETaskPriority::BackgroundNormal);
}
