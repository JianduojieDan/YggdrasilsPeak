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
 *  Game: field of view, head motion, story camera, heartbeat zoom, units, HUD, hints.
 *  Graphics: preset, upscaling, frame generation, ray-traced light, footprints, motion blur.
 *  Display: window, resolution, vsync, frame limit, brightness.
 *  Sound: master, effects, ambience, voices, interface (sound classes in /Game/Audio/Classes), sound in the background.
 *  Controls: mouse sensitivity, invert look, key bindings of our own actions (RegisterKey / Rebind).
 *  Accessibility: interface size, comfort dot, softer lightning flashes, colour vision correction.
 *  The camera rows follow Docs/Development/Chapter1_Camera_Spec.md §5.7, §5.10, §5.11 (2026-11-19).
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
	float FieldOfView();                     // degrees, 0 = the camera's own (saved values are clamped to FovMin..FovMax when applied)
	float Brightness();                      // -1..1 (0 = neutral)

	// ---- view: how the camera feels (camera spec §5.3, §5.7, §5.11). The view rig and the story read these; the
	// settings menu only stores them (and sets ygg.View.HeadMotion), so a change applies at once without a restart.
	constexpr float FovMin = 80.f;           // the player's field of view range (was 90-110 until 2026-11-19)
	constexpr float FovMax = 110.f;
	int32 HeadMotionChoice();                // 0 full, 1 reduced, 2 off
	float HeadMotion();                      // 1 / 0.4 / 0: the step bob, breathing and landing scale (ygg.View.HeadMotion)
	bool StoryCameraGentle();                // Gentle: the story never moves the view (no nudges, no reflexes; the slip is a plain eyelid close)
	bool HeartbeatZoom();                    // the plume's heartbeat FOV push and the altitude heart FOV term; false = the FOV stays still
	bool ComfortDot();                       // a small steady dot in the centre of the screen (drawn by UYggComfortDot)

	// ---- frame generation (camera spec §5.10)
	int32 FrameGenChoice();                  // -1 auto, 0 off, 1..3 = 2x..4x (saved "FrameGen")
	int32 FrameGenInUse();                   // what runs right now: 0 off, 1..3 = 2x..4x (Auto may step it while playing)

	// ---- sound (0..1)
	enum class ESound : uint8 { Master, Effects, Ambience, Voices, Interface, Num };
	float Volume(ESound Which);
	const TCHAR* SoundKey(ESound Which);
	bool SoundInBackground();

	// ---- controls
	/** The mouse speed when the player never chose one: 0.7 of the controller's scale (0.175 -> about 0.12 degrees per
	 *  count; camera spec §5.7 - flicks of 600 deg/s on a 30 fps base fed the turning streaks). */
	constexpr float DefaultSensitivity = 0.7f;
	float Sensitivity();                     // multiplier on the controller's input scale (DefaultSensitivity when unset)
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
	void ApplyView(UWorld* World);           // field of view on the player's camera, head motion, the comfort dot
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
