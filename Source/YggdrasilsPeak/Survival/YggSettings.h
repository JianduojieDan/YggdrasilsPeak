// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "InputCoreTypes.h"

class UInputAction;
class UInputMappingContext;
class APlayerController;

/**
 *  Every player setting in one place (2026-11-12 menu redesign). Saved in GameUserSettings.ini [YggdrasilsPeak.Display]
 *  (window mode, resolution and vsync in the engine's own GameUserSettings), applied at every start (the start menu and
 *  each time the climber spawns) and live when changed in the settings menu (USettingsWidget).
 *  Game: units, HUD, hints.  Graphics: preset, upscaling, ray-traced light, footprints, motion blur.
 *  Display: window, resolution, vsync, frame limit, field of view, brightness.
 *  Sound: master, effects, ambience, voices, interface (sound classes in /Game/Audio/Classes), sound in the background.
 *  Controls: mouse sensitivity, invert look, key bindings of our own actions (RegisterKey / Rebind).
 *  Accessibility: interface size, softer lightning flashes, colour vision correction.
 */
namespace YggSettings
{
	int32 GetInt(const TCHAR* Key, int32 Default);
	float GetFloat(const TCHAR* Key, float Default);
	void SetInt(const TCHAR* Key, int32 Value);
	void SetFloat(const TCHAR* Key, float Value);

	// ---- game
	bool Imperial();                         // feet instead of metres
	bool HudHidden();
	bool Hints();
	/** "8,849" / "29,032" for the HUD and the menus, with its unit "m" / "ft". */
	FString AltitudeNumber(float Meters);
	const TCHAR* AltitudeUnit();

	// ---- display
	int32 FrameCap();                        // 0 = 30, 1 = 60, 2 = 120, 3 = off
	float FieldOfView();                     // degrees, 0 = the camera's own
	float Brightness();                      // -1..1 (0 = neutral)

	// ---- sound (0..1)
	enum class ESound : uint8 { Master, Effects, Ambience, Voices, Interface, Num };
	float Volume(ESound Which);
	const TCHAR* SoundKey(ESound Which);
	bool SoundInBackground();

	// ---- controls
	float Sensitivity();                     // 1 = the game's default speed
	bool InvertY();

	// ---- accessibility
	float InterfaceScale();                  // 0.8 .. 1.3
	bool SoftFlashes();                      // lightning lights the world less and the bolts glow less
	float FlashScale();                      // 1, or 0.35 with soft flashes
	int32 ColourVision();                    // 0 off, 1 protan, 2 deutan, 3 tritan (corrected)

	// ---- applying
	void ApplyAll(UWorld* World);            // everything (at a start)
	void ApplyGraphics(UWorld* World);
	void ApplyDisplay(UWorld* World);        // window, resolution, vsync, frame limit, brightness
	void ApplyView(UWorld* World);           // field of view on the player's camera
	void ApplySound(UWorld* World);
	void ApplyControls(UWorld* World);       // mouse speed and invert on the player controller
	void ApplyAccessibility(UWorld* World);
	void ResetTab(UWorld* World, int32 Tab); // back to the defaults of one settings tab (0 game .. 5 accessibility)

	/** What the engine is really doing right now (scalability, resolution, upscaler, frame cap, gamma, FOV, mouse scale,
	 *  volumes, interface scale...), name -> value: the settings self-test (ygg.Settings.Test) diffs it per row. */
	TMap<FString, FString> EngineState(UWorld* World);
	bool HasDLSS();                          // NVIDIA's plugin is installed and running
	bool HasFrameGeneration();

	// ---- key bindings of our own actions
	struct FBinding
	{
		FName Action;
		FText Label;
		FKey Default;
		FKey Key;
		TWeakObjectPtr<UInputMappingContext> Context;
		TWeakObjectPtr<UInputAction> InputAction;
	};
	/** The key for an action: the player's choice, or Default. Use it where the action is mapped. */
	FKey KeyFor(FName Action, const FKey& Default);
	/** Called where a component maps one of its keyboard keys, so the settings can list and rebind it live. */
	void RegisterKey(FName Action, const FText& Label, const FKey& Default, UInputMappingContext* Context, UInputAction* InputAction);
	/** Saves and applies a new key now (the old one is unmapped). Returns false for keys that can't be bound. */
	bool Rebind(UWorld* World, FName Action, const FKey& NewKey);
	const TArray<FBinding>& Bindings();
	/** A key's name in English, whatever language the engine runs in ("LEFT SHIFT", "SPACE", "1"). */
	FString KeyName(const FKey& Key);
	/** "[1]"-style label of an action's key for the HUD. */
	FString KeyLabel(FName Action, const FKey& Default);
}
