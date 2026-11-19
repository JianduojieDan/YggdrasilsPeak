// Copyright Epic Games, Inc. All Rights Reserved.

#include "Survival/YggSettings.h"

#include "Survival/SettingsWidget.h"
#include "Survival/SnowTrailComponent.h"
#include "Environment/YggQualitySubsystem.h"
#include "AudioDevice.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Camera/CameraComponent.h"
#include "Containers/Ticker.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/UserInterfaceSettings.h"
#include "Engine/World.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/GameUserSettings.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/App.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/ConfigCacheIni.h"
#include "Sound/SoundClass.h"
#include "Sound/SoundMix.h"

namespace
{
	const TCHAR* const Sec = TEXT("YggdrasilsPeak.Display");

	void CVar(const TCHAR* Name, float V)
	{
		if (IConsoleVariable* C = IConsoleManager::Get().FindConsoleVariable(Name)) { C->Set(V, ECVF_SetByConsole); }
	}

	TArray<YggSettings::FBinding>& Registry()
	{
		// every rebindable action of our own, in the order the settings list them; the components fill in their
		// mapping contexts when they map the keys (RegisterKey), so a change applies at once in the game
		static TArray<YggSettings::FBinding> R;
		if (R.Num() == 0)
		{
			const struct { const TCHAR* Action; const TCHAR* Label; FKey Key; } Table[] = {
				{ TEXT("Sprint"), TEXT("Sprint"), EKeys::LeftShift },
				{ TEXT("Climb"), TEXT("Climb / grab"), EKeys::SpaceBar },
				{ TEXT("LetGo"), TEXT("Let go"), EKeys::C },
				{ TEXT("Interact"), TEXT("Read / use"), EKeys::E },
				{ TEXT("Compass"), TEXT("Compass"), EKeys::Q },
				{ TEXT("Backpack"), TEXT("Backpack"), EKeys::Tab },
				{ TEXT("Rest"), TEXT("Rest at camp"), EKeys::R },
				{ TEXT("Eat"), TEXT("Eat"), EKeys::One },
				{ TEXT("Drink"), TEXT("Drink"), EKeys::Two },
				{ TEXT("Pause"), TEXT("Pause"), EKeys::P },
			};
			for (const auto& T : Table)
			{
				YggSettings::FBinding& B = R.AddDefaulted_GetRef();
				B.Action = T.Action;
				B.Label = FText::FromString(T.Label);
				B.Default = T.Key;
				B.Key = YggSettings::KeyFor(T.Action, T.Key);
			}
		}
		return R;
	}

	USoundMix* SettingsMix()
	{
		static TWeakObjectPtr<USoundMix> Mix;
		if (!Mix.IsValid())
		{
			USoundMix* M = NewObject<USoundMix>(GetTransientPackage(), TEXT("SM_YggSettings"));
			M->AddToRoot();
			Mix = M;
		}
		return Mix.Get();
	}

	// test switch (2026-11-14): the audio device logs "RecursiveApplyAdjuster failed" ~180 times a second; this takes every
	// pushed sound mix away to see whether one of them is the source
	FAutoConsoleCommandWithWorld GClearMixes(TEXT("ygg.Sound.ClearMixes"), TEXT("Test: remove every pushed sound mix (the settings volumes too) until the next settings apply."),
		FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World)
		{
			UGameplayStatics::ClearSoundMixModifiers(World);
			UE_LOG(LogTemp, Display, TEXT("Sound: all sound mix modifiers cleared (test)"));
		}));

	float BaseYaw = 0.f, BasePitch = 0.f;

	/** Frame generation "Auto" (camera spec §5.10): 2x; 3x or 4x only after the rendered (base) rate has held 55 fps for
	 *  10 s, never more generated frames than the frame limit or the display can show (a 120 limit or a 144 Hz screen:
	 *  2x; a 60 Hz screen: off). A step down (base under 45 fps for 3 s) also lowers the ceiling for the session, so it
	 *  cannot flap; at most two steps up per session. */
	struct FSettingsFgAuto
	{
		bool bOn = false;            // Auto chosen, DLSS-G running, and a higher step is possible: the ticker watches
		int32 InUse = 0;             // frames generated now (0 off, 1..3 = 2x..4x)
		int32 Max = 1;               // the ceiling now: the frame limit, the display and FailMax
		int32 FailMax = 3;           // lowered for the session by each step down (a step that could not hold its base rate)
		float Fps = 0.f, Good = 0.f, Bad = 0.f, Hold = 0.f;
		FTSTicker::FDelegateHandle Handle;
	};
	FSettingsFgAuto& SettingsFgAuto() { static FSettingsFgAuto S; return S; }

	bool SettingsFgAutoTick(float Dt)
	{
		FSettingsFgAuto& A = SettingsFgAuto();
		if (!A.bOn) { A.Handle.Reset(); return false; }
		// the core ticker runs once per rendered frame: generated frames never reach the game thread, so this is the base rate
		if (Dt <= 0.f || Dt > 0.5f) { A.Good = 0.f; return true; }      // a load or a big hitch: start counting again
		A.Fps += (1.f / Dt - A.Fps) * FMath::Min(1.f, Dt * 2.f);        // about half a second of smoothing
		A.Hold = FMath::Max(0.f, A.Hold - Dt);
		A.Good = A.Fps >= 55.f ? A.Good + Dt : 0.f;
		A.Bad = A.Fps < 45.f ? A.Bad + Dt : 0.f;
		int32 Next = A.InUse;
		if (A.Hold <= 0.f && A.Good >= 10.f && A.InUse >= 1 && A.InUse < A.Max) { Next = A.InUse + 1; A.Hold = 10.f; }
		else if (A.Bad >= 3.f && A.InUse > 1) { Next = A.InUse - 1; A.FailMax = Next; A.Max = FMath::Min(A.Max, A.FailMax); A.Hold = 60.f; }
		if (Next != A.InUse)
		{
			UE_LOG(LogTemp, Display, TEXT("Settings: frame generation auto %dx -> %dx (base %.0f fps, ceiling %dx)"), A.InUse + 1, Next + 1, A.Fps, A.Max + 1);
			A.InUse = Next;
			A.Good = A.Bad = 0.f;
			CVar(TEXT("r.Streamline.DLSSG.FramesToGenerate"), float(Next));
		}
		return true;
	}
}

namespace YggSettings
{
	// The HUD and the storm ask every frame: values are cached here and the ini is only read once per key
	// (and written when a setting changes).
	static TMap<FString, float>& Cache() { static TMap<FString, float> C; return C; }
	int32 GetInt(const TCHAR* Key, int32 Default)
	{
		if (const float* C = Cache().Find(Key)) { return FMath::RoundToInt(*C); }
		int32 V = Default; const bool bHas = GConfig->GetInt(Sec, Key, V, GGameUserSettingsIni);
		if (bHas) { Cache().Add(Key, float(V)); }
		return V;
	}
	float GetFloat(const TCHAR* Key, float Default)
	{
		if (const float* C = Cache().Find(Key)) { return *C; }
		float V = Default; const bool bHas = GConfig->GetFloat(Sec, Key, V, GGameUserSettingsIni);
		if (bHas) { Cache().Add(Key, V); }
		return V;
	}
	void SetInt(const TCHAR* Key, int32 Value) { Cache().Add(Key, float(Value)); GConfig->SetInt(Sec, Key, Value, GGameUserSettingsIni); GConfig->Flush(false, GGameUserSettingsIni); }
	void SetFloat(const TCHAR* Key, float Value) { Cache().Add(Key, Value); GConfig->SetFloat(Sec, Key, Value, GGameUserSettingsIni); GConfig->Flush(false, GGameUserSettingsIni); }

	bool Imperial() { return GetInt(TEXT("Units"), 0) == 1; }
	bool HudHidden() { return GetInt(TEXT("Hud"), 0) == 1; }
	bool Hints() { return GetInt(TEXT("Hints"), 1) == 1; }

	FString AltitudeNumber(float Meters)
	{
		const float V = Imperial() ? Meters * 3.28084f : Meters;
		return FText::AsNumber(FMath::RoundToInt(V)).ToString();
	}
	const TCHAR* AltitudeUnit() { return Imperial() ? TEXT("ft") : TEXT("m"); }

	int32 FrameCap()
	{
		// the older setting (0 = 60, 1 = 120, 2 = off) carries over the first time
		const int32 Old = GetInt(TEXT("FrameLimit"), 2);
		return FMath::Clamp(GetInt(TEXT("FrameCap"), Old + 1), 0, 3);
	}
	float FieldOfView() { return GetFloat(TEXT("FieldOfView"), 0.f); }
	float Brightness() { return FMath::Clamp(GetFloat(TEXT("Brightness"), 0.f), -1.f, 1.f); }

	int32 HeadMotionChoice() { return FMath::Clamp(GetInt(TEXT("HeadMotion"), 0), 0, 2); }
	float HeadMotion() { static const float Scale[] = { 1.f, 0.4f, 0.f }; return Scale[HeadMotionChoice()]; }
	bool StoryCameraGentle() { return GetInt(TEXT("StoryCamera"), 0) == 1; }
	bool HeartbeatZoom() { return GetInt(TEXT("HeartbeatZoom"), 1) == 1; }
	bool ComfortDot() { return GetInt(TEXT("ComfortDot"), 0) == 1; }

	// Auto is the default from 2026-11-19; a saved 0..3 (off, 2x..4x) keeps its meaning
	int32 FrameGenChoice() { return FMath::Clamp(GetInt(TEXT("FrameGen"), -1), -1, 3); }
	int32 FrameGenInUse()
	{
		const IConsoleVariable* On = IConsoleManager::Get().FindConsoleVariable(TEXT("r.Streamline.DLSSG.Enable"));
		const IConsoleVariable* N = IConsoleManager::Get().FindConsoleVariable(TEXT("r.Streamline.DLSSG.FramesToGenerate"));
		return On && N && On->GetInt() != 0 ? FMath::Clamp(N->GetInt(), 0, 3) : 0;
	}

	const TCHAR* SoundKey(ESound Which)
	{
		static const TCHAR* const Keys[] = { TEXT("VolMaster"), TEXT("VolEffects"), TEXT("VolAmbience"), TEXT("VolVoices"), TEXT("VolInterface") };
		return Keys[FMath::Clamp(int32(Which), 0, 4)];
	}
	float Volume(ESound Which) { return FMath::Clamp(GetFloat(SoundKey(Which), 1.f), 0.f, 1.f); }
	bool SoundInBackground() { return GetInt(TEXT("SoundInBackground"), 1) == 1; }

	float Sensitivity() { return FMath::Clamp(GetFloat(TEXT("Sensitivity"), DefaultSensitivity), 0.2f, 3.f); }
	bool InvertY() { return GetInt(TEXT("InvertY"), 0) == 1; }

	float InterfaceScale() { return FMath::Clamp(GetFloat(TEXT("InterfaceScale"), 1.f), 0.8f, 1.3f); }
	bool SoftFlashes() { return GetInt(TEXT("SoftFlashes"), 0) == 1; }
	float FlashScale() { return SoftFlashes() ? 0.35f : 1.f; }
	int32 ColourVision() { return FMath::Clamp(GetInt(TEXT("ColourVision"), 0), 0, 3); }

	bool HasDLSS() { return IConsoleManager::Get().FindConsoleVariable(TEXT("r.NGX.DLSS.Enable")) != nullptr; }
	bool HasFrameGeneration() { return IConsoleManager::Get().FindConsoleVariable(TEXT("r.Streamline.DLSSG.Enable")) != nullptr; }

	void ApplyGraphics(UWorld* World)
	{
		USnowTrailComponent::SetFootprintQuality(FMath::Clamp(GetInt(TEXT("Footprints"), 1), 0, 2));
		const int32 Tier = FMath::Clamp(GetInt(TEXT("Graphics"), -1), -1, 3);
		const int32 Up = FMath::Clamp(GetInt(TEXT("Upscaling"), 0), 0, 4);
		// 2026-11-13 rework: the quality tier and the render resolution come from the player's choices every time
		// (YggQualitySubsystem::Configure); the automatic quality only ever lowers them for the session and never saves
		UYggQualitySubsystem* Q = UYggQualitySubsystem::Get(World);
		if (Q) { Q->Configure(Tier, Up); }
		// the usual detail options: "Preset" (0) follows the tier in use, 1..4 = low..epic for that group alone
		if (UGameUserSettings* U = GEngine ? GEngine->GetGameUserSettings() : nullptr)
		{
			const int32 Base = Q ? Q->GetTierInUse() : (Tier >= 0 ? Tier : 2);
			if (!Q) { U->SetOverallScalabilityLevel(Base); }
			auto Pick = [Base](const TCHAR* Key) { const int32 V = FMath::Clamp(GetInt(Key, 0), 0, 4); return V == 0 ? Base : V - 1; };
			U->SetShadowQuality(Pick(TEXT("GfxShadows")));
			U->SetViewDistanceQuality(Pick(TEXT("GfxViewDistance")));
			U->SetVisualEffectQuality(Pick(TEXT("GfxEffects")));
			U->SetTextureQuality(Pick(TEXT("GfxTextures")));
			U->SetResolutionScaleValueEx(100.f);
			// the engine re-applies its own frame-rate limit with the scalability (found by ygg.Settings.Test: Quality
			// High -> Epic set t.MaxFPS 120 -> 0); keep it the player's Frame limit
			static const float Fps[] = { 30.f, 60.f, 120.f, 0.f };
			U->SetFrameRateLimit(Fps[FrameCap()]);
			U->ApplyNonResolutionSettings();
			U->SaveSettings();                 // the player's choices only: Configure just reset the tier to the base
		}
		// the upscaler: NVIDIA DLSS when the plugin runs (Native = DLAA, DLSS at full resolution), else TSR
		const bool bDLSS = HasDLSS() && GetInt(TEXT("Upscaler"), 0) == 0;
		CVar(TEXT("r.AntiAliasingMethod"), 4.f);
		if (HasDLSS()) { CVar(TEXT("r.NGX.DLSS.Enable"), bDLSS ? 1.f : 0.f); }
		// DLSS frame generation (RTX 40/50): saved -1 auto, 0 off, 1..3 = 2x..4x frames; Reflex keeps the latency down
		// (always with frame generation). Auto (the default since 2026-11-19, camera spec §5.10) starts at 2x.
		const bool bCanFrameGen = bDLSS && HasFrameGeneration();
		const int32 FgChoice = FrameGenChoice();
		int32 FrameGen = !bCanFrameGen ? 0 : (FgChoice < 0 ? 1 : FgChoice);
		// under a frame cap, generated frames come out of the base rate: 4x under 120 fps locked the real rate at 30, and every
		// camera turn smeared (prayer-flag trails, Lumen seams at the screen edges, 2026-11-19 play-test). A cap allows at
		// most as many generated frames as keep the real rate near 55+: 120 -> 2x, 60 -> off; no cap = the player's choice
		static const float CapFps[] = { 30.f, 60.f, 120.f, 0.f };
		const float Cap = CapFps[FrameCap()];
		if (Cap > 0.f) { FrameGen = FMath::Min(FrameGen, FMath::Max(0, FMath::FloorToInt(Cap / 55.f) - 1)); }
		{
			// Auto: the display is a cap too (frames it cannot show only add delay): 60 Hz -> off, 120-144 Hz -> 2x,
			// 165 Hz -> up to 3x, 240 Hz -> up to 4x. Above 2x only after the base rate proved it (SettingsFgAutoTick).
			FSettingsFgAuto& A = SettingsFgAuto();
			A.bOn = false;
			if (bCanFrameGen && FgChoice < 0)
			{
				const int32 DisplayMax = FMath::Clamp(FPlatformMisc::GetMaxRefreshRate() / 55 - 1, 0, 3);
				const int32 Ceiling = Cap > 0.f ? FMath::Min(FrameGen, DisplayMax) : DisplayMax;
				FrameGen = FMath::Min(FrameGen, DisplayMax);
				A.Max = FMath::Min(Ceiling, A.FailMax);
				// another setting applied while Auto had stepped up keeps that step instead of starting over at 2x
				if (A.InUse >= 1 && A.InUse <= A.Max) { FrameGen = A.InUse; }
				A.InUse = FrameGen;
				A.bOn = FrameGen >= 1 && A.Max > 1;
				if (A.bOn && !A.Handle.IsValid())
				{
					A.Fps = 0.f; A.Good = A.Bad = 0.f; A.Hold = 5.f;
					A.Handle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateStatic(&SettingsFgAutoTick));
				}
			}
			else
			{
				A.InUse = 0;
				A.Max = 1;
			}
		}
		if (HasFrameGeneration())
		{
			CVar(TEXT("r.Streamline.DLSSG.Enable"), FrameGen > 0 ? 1.f : 0.f);
			if (FrameGen > 0) { CVar(TEXT("r.Streamline.DLSSG.FramesToGenerate"), float(FrameGen)); }
		}
		if (IConsoleManager::Get().FindConsoleVariable(TEXT("t.Streamline.Reflex.Enable")))
		{
			const bool bReflex = FrameGen > 0 || GetInt(TEXT("Reflex"), 1) == 1;
			CVar(TEXT("t.Streamline.Reflex.Enable"), bReflex ? 1.f : 0.f);
			CVar(TEXT("t.Streamline.Reflex.Mode"), bReflex ? 1.f : 0.f);
		}
		// a light sharpen after the upscaler (2026-11-13, user: the snow looked blurry): fine snow grain survives the rebuild
		CVar(TEXT("r.Tonemapper.Sharpen"), Up == 1 ? 0.25f : 0.5f);
		CVar(TEXT("r.Lumen.HardwareRayTracing"), float(FMath::Clamp(GetInt(TEXT("RayTracing"), 1), 0, 1)));
		// motion blur (camera spec §5.10): Off by default since 2026-11-19 - the full post-process blur smeared every turn on a
		// 30 fps base. Low = a short, faint trail: amount 0.15, at most 2% of the screen width (the engine's override cvars,
		// set at console priority so the scalability groups cannot put the full blur back). A saved 1 ("On") is now Low.
		const int32 MotionBlur = FMath::Clamp(GetInt(TEXT("MotionBlur"), 0), 0, 1);
		if (MotionBlur == 0)
		{
			CVar(TEXT("r.MotionBlurQuality"), 0.f);
			CVar(TEXT("r.MotionBlur.Amount"), -1.f);
			CVar(TEXT("r.MotionBlur.Max"), -1.f);
		}
		else
		{
			if (IConsoleVariable* C = IConsoleManager::Get().FindConsoleVariable(TEXT("r.MotionBlurQuality")); C && C->GetInt() == 0) { C->Set(4, ECVF_SetByConsole); }
			CVar(TEXT("r.MotionBlur.Amount"), 0.15f);
			CVar(TEXT("r.MotionBlur.Max"), 2.f);
		}
		if (const UGameUserSettings* U = GEngine ? GEngine->GetGameUserSettings() : nullptr)
		{
			UE_LOG(LogTemp, Display, TEXT("Settings: tier %d (%s), shadows %d, view distance %d, effects %d, textures %d, upscaling %d via %s, frame generation %d%s, ray tracing %d, footprints %d, motion blur %s"),
				Q ? Q->GetTierInUse() : -1, Tier < 0 ? TEXT("auto") : TEXT("fixed"), U->GetShadowQuality(), U->GetViewDistanceQuality(), U->GetVisualEffectQuality(), U->GetTextureQuality(),
				Up, bDLSS ? TEXT("DLSS") : TEXT("TSR"), FrameGen, FgChoice < 0 ? TEXT(" (auto)") : TEXT(""), GetInt(TEXT("RayTracing"), 1), GetInt(TEXT("Footprints"), 1),
				MotionBlur ? TEXT("low") : TEXT("off"));
		}
	}

	TMap<FString, FString> EngineState(UWorld* World)
	{
		TMap<FString, FString> S;
		for (const TCHAR* Name : { TEXT("sg.ShadowQuality"), TEXT("sg.ViewDistanceQuality"), TEXT("sg.EffectsQuality"), TEXT("sg.TextureQuality"),
			TEXT("sg.PostProcessQuality"), TEXT("sg.GlobalIlluminationQuality"), TEXT("r.ScreenPercentage"), TEXT("r.NGX.DLSS.Enable"),
			TEXT("r.Streamline.DLSSG.Enable"), TEXT("r.Streamline.DLSSG.FramesToGenerate"), TEXT("t.Streamline.Reflex.Enable"),
			TEXT("r.Lumen.HardwareRayTracing"), TEXT("r.MotionBlurQuality"), TEXT("r.MotionBlur.Amount"), TEXT("r.MotionBlur.Max"), TEXT("r.Nanite.MaxPixelsPerEdge"),
			TEXT("t.MaxFPS"), TEXT("r.VSync"), TEXT("ygg.View.HeadMotion") })
		{
			if (IConsoleVariable* C = IConsoleManager::Get().FindConsoleVariable(Name)) { S.Add(Name, C->GetString()); }
		}
		S.Add(TEXT("footprints"), FString::FromInt(USnowTrailComponent::FootprintQuality));
		if (GEngine) { S.Add(TEXT("gamma"), FString::SanitizeFloat(GEngine->DisplayGamma)); }
		if (const UGameUserSettings* U = GEngine ? GEngine->GetGameUserSettings() : nullptr)
		{
			S.Add(TEXT("window"), FString::FromInt(int32(U->GetFullscreenMode())));
			S.Add(TEXT("resolution"), U->GetScreenResolution().ToString());
		}
		if (APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr)
		{
			S.Add(TEXT("mouse yaw scale"), FString::SanitizeFloat(PC->InputYawScale_DEPRECATED));
			S.Add(TEXT("mouse pitch scale"), FString::SanitizeFloat(PC->InputPitchScale_DEPRECATED));
			if (const APawn* P = PC->GetPawn())
			{
				if (const UCameraComponent* Cam = P->FindComponentByClass<UCameraComponent>()) { S.Add(TEXT("fov"), FString::SanitizeFloat(Cam->FieldOfView)); }
			}
		}
		if (World)
		{
			if (FAudioDeviceHandle Dev = World->GetAudioDevice())
			{
				// the audio device's live values may only be read on the audio thread (reading them here crashed the
				// session monitor, 2026-11-13; the self-test never saw it because it runs with -NoSound): the master
				// volume when that is safe; for the kinds of sound, the value the settings mix applies, or that the class
				// is gone (it was garbage collected before 2026-11-14 and the setting silently stopped working)
				if (IsInAudioThread()) { S.Add(TEXT("volume master"), FString::SanitizeFloat(Dev->GetTransientPrimaryVolume())); }
				static const TCHAR* const Kinds[] = { TEXT("SC_Effects"), TEXT("SC_Ambience"), TEXT("SC_Voices"), TEXT("SC_Interface") };
				for (int32 i = 0; i < 4; ++i)
				{
					const bool bAlive = FindObject<USoundClass>(nullptr, *FString::Printf(TEXT("/Game/Audio/Classes/%s.%s"), Kinds[i], Kinds[i])) != nullptr;
					S.Add(FString(TEXT("volume ")) + Kinds[i], bAlive ? FString::SanitizeFloat(Volume(ESound(i + 1))) : FString(TEXT("class not loaded")));
				}
			}
		}
		S.Add(TEXT("unfocused volume"), FString::SanitizeFloat(FApp::GetUnfocusedVolumeMultiplier()));
		S.Add(TEXT("interface scale"), FString::SanitizeFloat(GetDefault<UUserInterfaceSettings>()->ApplicationScale));
		// read live by the game's own code (the HUD, the storm): their stored values are what matters
		S.Add(TEXT("[game] units feet"), Imperial() ? TEXT("1") : TEXT("0"));
		S.Add(TEXT("[game] hud quiet"), HudHidden() ? TEXT("1") : TEXT("0"));
		S.Add(TEXT("[game] body effects"), FString::FromInt(GetInt(TEXT("BodyFx"), 0)));
		S.Add(TEXT("[game] hud size"), FString::FromInt(GetInt(TEXT("HudSize"), 1)));
		S.Add(TEXT("[game] fps counter"), FString::FromInt(GetInt(TEXT("FpsCounter"), 0)));
		S.Add(TEXT("[game] soft flashes"), SoftFlashes() ? TEXT("1") : TEXT("0"));
		S.Add(TEXT("[game] colour vision"), FString::FromInt(ColourVision()));
		// the camera rows (2026-11-19): read live by the view rig and the story
		S.Add(TEXT("[game] head motion"), FString::SanitizeFloat(HeadMotion()));
		S.Add(TEXT("[game] story camera"), StoryCameraGentle() ? TEXT("gentle") : TEXT("guided"));
		S.Add(TEXT("[game] heartbeat zoom"), HeartbeatZoom() ? TEXT("1") : TEXT("0"));
		S.Add(TEXT("[game] comfort dot"), ComfortDot() ? TEXT("1") : TEXT("0"));
		S.Add(TEXT("[game] comfort dot shown"), UYggComfortDot::IsShown() ? TEXT("1") : TEXT("0"));
		{
			const int32 InUse = FrameGenInUse();
			const FString Now = InUse > 0 ? FString::Printf(TEXT("%dx"), InUse + 1) : FString(TEXT("off"));
			S.Add(TEXT("[graphics] frame generation"), FrameGenChoice() < 0 ? FString::Printf(TEXT("auto (%s now)"), *Now) : FString::FromInt(FrameGenChoice()));
		}
		return S;
	}

	void ApplyDisplay(UWorld* World)
	{
		static const float Fps[] = { 30.f, 60.f, 120.f, 0.f };
		CVar(TEXT("t.MaxFPS"), Fps[FrameCap()]);
		if (UGameUserSettings* U = GEngine ? GEngine->GetGameUserSettings() : nullptr) { U->SetFrameRateLimit(Fps[FrameCap()]); }
		if (GEngine)
		{
			// brightness as display gamma: 2.2 is neutral, higher lifts the shadows
			GEngine->DisplayGamma = 2.2f + Brightness() * 0.6f;
		}
		// window mode, resolution and vsync live in the engine's own settings (the editor's window is left alone)
		if (UGameUserSettings* U = GEngine ? GEngine->GetGameUserSettings() : nullptr)
		{
			// a window asked for on the command line (-windowed -ResX=..., the perf runs) wins over the saved one
			int32 ResX = 0;
			const bool bCmdWindow = FParse::Param(FCommandLine::Get(), TEXT("windowed")) || FParse::Value(FCommandLine::Get(), TEXT("ResX="), ResX);
			if (!GIsEditor && !bCmdWindow)
			{
				U->ApplyResolutionSettings(false);
			}
			CVar(TEXT("r.VSync"), U->IsVSyncEnabled() ? 1.f : 0.f);
		}
	}

	void ApplyView(UWorld* World)
	{
		// head motion: the view rig scales its step bob, breathing and landing by this cvar (camera spec §5.3); a console
		// value set for tuning lasts until the next apply (a spawn or a settings change)
		CVar(TEXT("ygg.View.HeadMotion"), HeadMotion());
		UYggComfortDot::Apply(World, ComfortDot());
		// the player's range is 80-110 degrees (camera spec §5.7; 90-110 before 2026-11-19, to-do #65); older saves may hold 70-79
		const float Fov = FieldOfView() > 1.f ? FMath::Clamp(FieldOfView(), FovMin, FovMax) : 0.f;
		APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
		APawn* P = PC ? PC->GetPawn() : nullptr;
		if (!P || Fov <= 1.f)
		{
			return;
		}
		// the camera components hold the settings FOV; story FOV terms are added on top by the rig's camera modifier and never
		// written here (the hands keep the BP camera's first-person FOV 70)
		TArray<UCameraComponent*> Cams;
		P->GetComponents<UCameraComponent>(Cams);
		for (UCameraComponent* C : Cams) { C->SetFieldOfView(Fov); }
	}

	void ApplySound(UWorld* World)
	{
		FApp::SetUnfocusedVolumeMultiplier(SoundInBackground() ? 1.f : 0.f);
		if (!World)
		{
			return;
		}
		if (FAudioDeviceHandle Dev = World->GetAudioDevice())
		{
			Dev->SetTransientPrimaryVolume(Volume(ESound::Master));
		}
		// the game's own sounds by kind (Tools/Audio/make_sound_classes.py gives every sound its class)
		static const TCHAR* const Classes[] = { nullptr, TEXT("SC_Effects"), TEXT("SC_Ambience"), TEXT("SC_Voices"), TEXT("SC_Interface") };
		USoundMix* Mix = SettingsMix();
		for (int32 i = 1; i < int32(ESound::Num); ++i)
		{
			const FString Path = FString::Printf(TEXT("/Game/Audio/Classes/%s.%s"), Classes[i], Classes[i]);
			if (USoundClass* C = LoadObject<USoundClass>(nullptr, *Path, nullptr, LOAD_NoWarn | LOAD_Quiet))
			{
				// kept for good: the audio device holds the override with a plain pointer, so nothing kept the class
				// alive; the first garbage collection (the level change, or 61 s in) unregistered it, the volume setting
				// stopped working and the device logged "RecursiveApplyAdjuster failed" ~100-180 times a second
				// (found by the session monitor, 2026-11-14)
				C->AddToRoot();
				UGameplayStatics::SetSoundMixClassOverride(World, Mix, C, FMath::Max(Volume(ESound(i)), 0.0001f), 1.f, 0.05f, true);
			}
		}
		UGameplayStatics::PushSoundMixModifier(World, Mix);
	}

	void ApplyControls(UWorld* World)
	{
		APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
		if (!PC)
		{
			return;
		}
		// the template looks with AddControllerYaw/PitchInput; with legacy input scales on (DefaultInput.ini) the
		// controller multiplies them by these
		const APlayerController* Def = PC->GetClass()->GetDefaultObject<APlayerController>();
		BaseYaw = Def->InputYawScale_DEPRECATED;
		BasePitch = Def->InputPitchScale_DEPRECATED;
		PC->InputYawScale_DEPRECATED = BaseYaw * Sensitivity();
		PC->InputPitchScale_DEPRECATED = BasePitch * Sensitivity() * (InvertY() ? -1.f : 1.f);
	}

	void ApplyAccessibility(UWorld* World)
	{
		if (UUserInterfaceSettings* UI = GetMutableDefault<UUserInterfaceSettings>())
		{
			UI->ApplicationScale = InterfaceScale();
		}
		const int32 Cv = ColourVision();
		UWidgetBlueprintLibrary::SetColorVisionDeficiencyType(Cv == 1 ? EColorVisionDeficiency::Protanope : Cv == 2 ? EColorVisionDeficiency::Deuteranope
			: Cv == 3 ? EColorVisionDeficiency::Tritanope : EColorVisionDeficiency::NormalVision, Cv > 0 ? 1.f : 0.f, Cv > 0, false);
	}

	void ApplyAll(UWorld* World)
	{
		ApplyGraphics(World);
		ApplyDisplay(World);
		ApplyView(World);
		ApplySound(World);
		ApplyControls(World);
		ApplyAccessibility(World);
	}

	void ResetTab(UWorld* World, int32 Tab)
	{
		auto Remove = [](std::initializer_list<const TCHAR*> Keys) { for (const TCHAR* K : Keys) { GConfig->RemoveKey(Sec, K, GGameUserSettingsIni); Cache().Remove(K); } };
		switch (Tab)
		{
		case 0: Remove({ TEXT("Units"), TEXT("Hud"), TEXT("HudSize"), TEXT("FpsCounter"), TEXT("BodyFx"), TEXT("Hints"), TEXT("FieldOfView"),
			TEXT("HeadMotion"), TEXT("StoryCamera"), TEXT("HeartbeatZoom") }); break;
		case 1: Remove({ TEXT("Graphics"), TEXT("Upscaling"), TEXT("Upscaler"), TEXT("FrameGen"), TEXT("Reflex"), TEXT("RayTracing"), TEXT("Footprints"), TEXT("MotionBlur"), TEXT("GfxShadows"), TEXT("GfxViewDistance"), TEXT("GfxEffects"), TEXT("GfxTextures") }); break;
		case 2:
			Remove({ TEXT("FrameCap"), TEXT("FrameLimit"), TEXT("Brightness") });
			if (UGameUserSettings* U = GEngine ? GEngine->GetGameUserSettings() : nullptr) { U->SetVSyncEnabled(false); U->SaveSettings(); }
			break;
		case 3: Remove({ TEXT("VolMaster"), TEXT("VolEffects"), TEXT("VolAmbience"), TEXT("VolVoices"), TEXT("VolInterface"), TEXT("SoundInBackground") }); break;
		case 4:
			Remove({ TEXT("Sensitivity"), TEXT("InvertY") });
			for (const FBinding& B : Registry()) { Rebind(World, B.Action, B.Default); }
			break;
		case 5: Remove({ TEXT("InterfaceScale"), TEXT("SoftFlashes"), TEXT("ColourVision"), TEXT("ComfortDot") }); break;
		default: break;
		}
		GConfig->Flush(false, GGameUserSettingsIni);
		ApplyAll(World);
	}

	FKey KeyFor(FName Action, const FKey& Default)
	{
		FString Name;
		if (GConfig->GetString(Sec, *(TEXT("Key.") + Action.ToString()), Name, GGameUserSettingsIni))
		{
			const FKey K(*Name);
			if (K.IsValid()) { return K; }
		}
		return Default;
	}

	void RegisterKey(FName Action, const FText& Label, const FKey& Default, UInputMappingContext* Context, UInputAction* InputAction)
	{
		TArray<FBinding>& R = Registry();
		FBinding* B = R.FindByPredicate([Action](const FBinding& X) { return X.Action == Action; });
		if (!B) { B = &R.AddDefaulted_GetRef(); B->Label = Label; }
		B->Action = Action;
		B->Default = Default;
		B->Key = KeyFor(Action, Default);
		B->Context = Context;
		B->InputAction = InputAction;
	}

	bool Rebind(UWorld* World, FName Action, const FKey& NewKey)
	{
		if (!NewKey.IsValid() || NewKey.IsMouseButton() || NewKey.IsGamepadKey() || NewKey == EKeys::Escape || NewKey == EKeys::F2)
		{
			return false;
		}
		FBinding* B = Registry().FindByPredicate([Action](const FBinding& X) { return X.Action == Action; });
		if (!B)
		{
			return false;
		}
		if (B->Key == NewKey)
		{
			return true;
		}
		// a key already used by another of our actions: the two swap
		if (FBinding* Other = Registry().FindByPredicate([&](const FBinding& X) { return X.Action != Action && X.Key == NewKey; }))
		{
			const FName OtherAction = Other->Action;
			const FKey Old = B->Key;
			Other->Key = FKey();   // free it first so the swap does not loop
			if (UInputMappingContext* OC = Other->Context.Get()) { if (UInputAction* OA = Other->InputAction.Get()) { OC->UnmapKey(OA, NewKey); OC->MapKey(OA, Old); } }
			Other->Key = Old;
			if (Old == Other->Default) { GConfig->RemoveKey(Sec, *(TEXT("Key.") + OtherAction.ToString()), GGameUserSettingsIni); }
			else { GConfig->SetString(Sec, *(TEXT("Key.") + OtherAction.ToString()), *Old.GetFName().ToString(), GGameUserSettingsIni); }
		}
		if (UInputMappingContext* Ctx = B->Context.Get())
		{
			if (UInputAction* IA = B->InputAction.Get())
			{
				Ctx->UnmapKey(IA, B->Key);
				Ctx->MapKey(IA, NewKey);
				if (APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr)
				{
					if (UEnhancedInputLocalPlayerSubsystem* Sub = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
					{
						Sub->RequestRebuildControlMappings();
					}
				}
			}
		}
		B->Key = NewKey;
		if (NewKey == B->Default) { GConfig->RemoveKey(Sec, *(TEXT("Key.") + Action.ToString()), GGameUserSettingsIni); }
		else { GConfig->SetString(Sec, *(TEXT("Key.") + Action.ToString()), *NewKey.GetFName().ToString(), GGameUserSettingsIni); }
		GConfig->Flush(false, GGameUserSettingsIni);
		return true;
	}

	const TArray<FBinding>& Bindings() { return Registry(); }

	FString KeyName(const FKey& Key)
	{
		static const TMap<FName, const TCHAR*> Names = {
			{ TEXT("LeftShift"), TEXT("LEFT SHIFT") }, { TEXT("RightShift"), TEXT("RIGHT SHIFT") }, { TEXT("SpaceBar"), TEXT("SPACE") },
			{ TEXT("LeftControl"), TEXT("LEFT CTRL") }, { TEXT("RightControl"), TEXT("RIGHT CTRL") }, { TEXT("LeftAlt"), TEXT("LEFT ALT") },
			{ TEXT("RightAlt"), TEXT("RIGHT ALT") }, { TEXT("Tab"), TEXT("TAB") }, { TEXT("CapsLock"), TEXT("CAPS LOCK") }, { TEXT("Enter"), TEXT("ENTER") },
			{ TEXT("BackSpace"), TEXT("BACKSPACE") }, { TEXT("Zero"), TEXT("0") }, { TEXT("One"), TEXT("1") }, { TEXT("Two"), TEXT("2") },
			{ TEXT("Three"), TEXT("3") }, { TEXT("Four"), TEXT("4") }, { TEXT("Five"), TEXT("5") }, { TEXT("Six"), TEXT("6") }, { TEXT("Seven"), TEXT("7") },
			{ TEXT("Eight"), TEXT("8") }, { TEXT("Nine"), TEXT("9") }, { TEXT("Up"), TEXT("UP") }, { TEXT("Down"), TEXT("DOWN") }, { TEXT("Left"), TEXT("LEFT") },
			{ TEXT("Right"), TEXT("RIGHT") }, { TEXT("Tilde"), TEXT("~") }, { TEXT("Comma"), TEXT(",") }, { TEXT("Period"), TEXT(".") }, { TEXT("Slash"), TEXT("/") },
			{ TEXT("Semicolon"), TEXT(";") }, { TEXT("Apostrophe"), TEXT("'") }, { TEXT("LeftBracket"), TEXT("[") }, { TEXT("RightBracket"), TEXT("]") },
			{ TEXT("Backslash"), TEXT("\\") }, { TEXT("Hyphen"), TEXT("-") }, { TEXT("Equals"), TEXT("=") },
		};
		if (const TCHAR* const* N = Names.Find(Key.GetFName())) { return *N; }
		// "NumPadOne" -> "NUM PAD ONE": spaces before capitals
		const FString S = Key.GetFName().ToString();
		FString Out;
		for (int32 i = 0; i < S.Len(); ++i)
		{
			if (i > 0 && FChar::IsUpper(S[i]) && !FChar::IsUpper(S[i - 1])) { Out.AppendChar(' '); }
			Out.AppendChar(FChar::ToUpper(S[i]));
		}
		return Out;
	}

	FString KeyLabel(FName Action, const FKey& Default)
	{
		return FString::Printf(TEXT("[%s]"), *KeyName(KeyFor(Action, Default)));
	}
}
