// Copyright Epic Games, Inc. All Rights Reserved.

#include "Survival/YggSettings.h"

#include "Survival/SnowTrailComponent.h"
#include "Environment/YggQualitySubsystem.h"
#include "AudioDevice.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Camera/CameraComponent.h"
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

	float BaseYaw = 0.f, BasePitch = 0.f;
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

	const TCHAR* SoundKey(ESound Which)
	{
		static const TCHAR* const Keys[] = { TEXT("VolMaster"), TEXT("VolEffects"), TEXT("VolAmbience"), TEXT("VolVoices"), TEXT("VolInterface") };
		return Keys[FMath::Clamp(int32(Which), 0, 4)];
	}
	float Volume(ESound Which) { return FMath::Clamp(GetFloat(SoundKey(Which), 1.f), 0.f, 1.f); }
	bool SoundInBackground() { return GetInt(TEXT("SoundInBackground"), 1) == 1; }

	float Sensitivity() { return FMath::Clamp(GetFloat(TEXT("Sensitivity"), 1.f), 0.2f, 3.f); }
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
		// DLSS frame generation (RTX 40/50): 0 off, 1..3 = 2x..4x frames; Reflex keeps the latency down (always with frame generation)
		const int32 FrameGen = bDLSS && HasFrameGeneration() ? FMath::Clamp(GetInt(TEXT("FrameGen"), 0), 0, 3) : 0;
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
		if (GetInt(TEXT("MotionBlur"), 1) == 0) { CVar(TEXT("r.MotionBlurQuality"), 0.f); }
		else if (IConsoleVariable* C = IConsoleManager::Get().FindConsoleVariable(TEXT("r.MotionBlurQuality")); C && C->GetInt() == 0) { C->Set(4, ECVF_SetByConsole); }
		if (const UGameUserSettings* U = GEngine ? GEngine->GetGameUserSettings() : nullptr)
		{
			UE_LOG(LogTemp, Display, TEXT("Settings: tier %d (%s), shadows %d, view distance %d, effects %d, textures %d, upscaling %d via %s, frame generation %d, ray tracing %d, footprints %d, motion blur %d"),
				Q ? Q->GetTierInUse() : -1, Tier < 0 ? TEXT("auto") : TEXT("fixed"), U->GetShadowQuality(), U->GetViewDistanceQuality(), U->GetVisualEffectQuality(), U->GetTextureQuality(),
				Up, bDLSS ? TEXT("DLSS") : TEXT("TSR"), FrameGen, GetInt(TEXT("RayTracing"), 1), GetInt(TEXT("Footprints"), 1), GetInt(TEXT("MotionBlur"), 1));
		}
	}

	TMap<FString, FString> EngineState(UWorld* World)
	{
		TMap<FString, FString> S;
		for (const TCHAR* Name : { TEXT("sg.ShadowQuality"), TEXT("sg.ViewDistanceQuality"), TEXT("sg.EffectsQuality"), TEXT("sg.TextureQuality"),
			TEXT("sg.PostProcessQuality"), TEXT("sg.GlobalIlluminationQuality"), TEXT("r.ScreenPercentage"), TEXT("r.NGX.DLSS.Enable"),
			TEXT("r.Streamline.DLSSG.Enable"), TEXT("r.Streamline.DLSSG.FramesToGenerate"), TEXT("t.Streamline.Reflex.Enable"),
			TEXT("r.Lumen.HardwareRayTracing"), TEXT("r.MotionBlurQuality"), TEXT("r.Nanite.MaxPixelsPerEdge"), TEXT("t.MaxFPS"), TEXT("r.VSync") })
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
				S.Add(TEXT("volume master"), FString::SanitizeFloat(Dev->GetTransientPrimaryVolume()));
				for (const TCHAR* Cls : { TEXT("SC_Effects"), TEXT("SC_Ambience"), TEXT("SC_Voices"), TEXT("SC_Interface") })
				{
					if (USoundClass* C = LoadObject<USoundClass>(nullptr, *FString::Printf(TEXT("/Game/Audio/Classes/%s.%s"), Cls, Cls), nullptr, LOAD_NoWarn | LOAD_Quiet))
					{
						if (const FSoundClassProperties* Props = Dev->GetSoundClassCurrentProperties(C)) { S.Add(FString(TEXT("volume ")) + Cls, FString::SanitizeFloat(Props->Volume)); }
					}
				}
			}
		}
		S.Add(TEXT("unfocused volume"), FString::SanitizeFloat(FApp::GetUnfocusedVolumeMultiplier()));
		S.Add(TEXT("interface scale"), FString::SanitizeFloat(GetDefault<UUserInterfaceSettings>()->ApplicationScale));
		// read live by the game's own code (the HUD, the storm): their stored values are what matters
		S.Add(TEXT("[game] units feet"), Imperial() ? TEXT("1") : TEXT("0"));
		S.Add(TEXT("[game] hud quiet"), HudHidden() ? TEXT("1") : TEXT("0"));
		S.Add(TEXT("[game] body effects"), FString::FromInt(GetInt(TEXT("BodyFx"), 0)));
		S.Add(TEXT("[game] soft flashes"), SoftFlashes() ? TEXT("1") : TEXT("0"));
		S.Add(TEXT("[game] colour vision"), FString::FromInt(ColourVision()));
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
		// the player's range is 90-110 degrees (to-do #65); older saves may hold 70-89
		const float Fov = FieldOfView() > 1.f ? FMath::Clamp(FieldOfView(), 90.f, 110.f) : 0.f;
		APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
		APawn* P = PC ? PC->GetPawn() : nullptr;
		if (!P || Fov <= 1.f)
		{
			return;
		}
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
		case 0: Remove({ TEXT("Units"), TEXT("Hud"), TEXT("BodyFx"), TEXT("Hints"), TEXT("FieldOfView") }); break;
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
		case 5: Remove({ TEXT("InterfaceScale"), TEXT("SoftFlashes"), TEXT("ColourVision") }); break;
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
