# UE 5.x procedural camera / cinematic systems + research on automated virtual cinematography and cutscene generation

Scope: (A) which Unreal Engine 5.x systems let code and rules make cinematic camera behaviour and sequences without hand-keying; (B) what research says about automatic camera placement, shot selection and cutscene generation, and which methods fit a first-person game built by one developer with an AI coding agent (Claude Code, which can run editor Python and C++ builds). Notes written 2026-11-14. UE 5.8 shipped in June 2026 and is reported as the last 5.x release before UE6.

Source-quality note: Epic's documentation pages (dev.epicgames.com) load their content with JavaScript and came back empty when fetched. These include the 5.7 and 5.8 release notes and the Gameplay Cameras overview. Facts about those pages come from search snippets, from the Gameplay Cameras developer's own blog, from Epic API page titles, and from press coverage (CG Channel, GameFromScratch). Each source is named inline.

---

## A1. Gameplay Cameras plugin (UE 5.5+): what it does in 5.6-5.8, is it production-ready, and how it compares with PlayerCameraManager / Camera Modifiers / Camera Shakes

### Takeaway
Gameplay Cameras (GPC) is a data-driven camera system: camera assets hold camera rigs (node graphs), and camera directors choose which rig is active, with blended transitions between rigs. In 5.7 it was still **Experimental**, maintained mostly by one Epic developer, with breaking changes in every release. Its 5.8 status could not be confirmed. For a shipping first-person game, the classic PlayerCameraManager + Camera Modifiers + Camera Shakes stack is still the safer base. GPC is worth watching, and worth a prototype at most.

### Cited Findings
- **Status in 5.7:** the developer calls the plugin "experimental", so backwards compatibility is not guaranteed. He hopes it moves to "beta" in 5.8, but only if the plan holds — [Ludovic Chabant, "UE5 Gameplay Cameras: Upgrading to 5.7" (2025-11-14)](https://ludovic.chabant.com/blog/2025/11/14/ue5-gameplay-cameras-upgrading-to-5-7/)
- **Who builds it:** the same post says the system is "98% written by just one dev (myself)". The author says he is on the Sequencer team and splits his time between Sequencer and GPC — [Chabant 5.7 post](https://ludovic.chabant.com/blog/2025/11/14/ue5-gameplay-cameras-upgrading-to-5-7/)
- **Breaking changes in 5.7:**
  - The "pre-blended" option on camera rig parameters now defaults to off.
  - Camera assets loaded when PIE starts are rebuilt if outdated, much like Blueprint auto-compile. This often marks assets as modified.
  - A Property Bag bug leaves some assets always showing as modified. The fix was planned for 5.7.1.
  - Source: [Chabant 5.7 post](https://ludovic.chabant.com/blog/2025/11/14/ue5-gameplay-cameras-upgrading-to-5-7/)
- **New in 5.7:**
  - Blueprint getter nodes for camera rig parameters, and setter nodes that return the set value (so they can be chained).
  - The Camera Debugger can bind to a specific camera system instance. Its "auto-bind" option picks one through the Player Camera Manager and the current View Target.
  - Asset-family shortcuts link Camera Assets, Directors, Rigs and Rig Proxies.
  - Source: [Chabant 5.7 post](https://ludovic.chabant.com/blog/2025/11/14/ue5-gameplay-cameras-upgrading-to-5-7/)
- **Planned for 5.8:** a "centralized" camera system on the GPC Player Camera Manager, for advanced transitions between scattered GPC actors. Chabant says this ability was partly lost in 5.6 when the Camera System Actor was deprecated. He also says 5.8 will search references more deeply, for example through Chooser Tables — [Chabant 5.7 post](https://ludovic.chabant.com/blog/2025/11/14/ue5-gameplay-cameras-upgrading-to-5-7/)
- **Changes in 5.6:**
  - Camera rigs that lived inside camera assets were split out into separate assets.
  - Camera Directors can now activate any Camera Rig in the Content Browser.
  - Gameplay Camera Components now run their own camera system when they activate, and the Camera System Actor is deprecated. To keep a component from running in the background, you turn off Auto Activate.
  - Source: [Chabant, "UE5 Gameplay Cameras: Upgrading to 5.6" (2025-06-06)](https://ludovic.chabant.com/blog/2025/06/06/ue5-gameplay-cameras-upgrading-to-5-6/). The same summary appeared alongside a [forum thread on rigs broken in 5.6](https://forums.unrealengine.com/t/gameplay-camera-rig-broken-in-5-6/2550183). In that thread, a user's camera got stuck at the torso after upgrading; it was fixed by activating the component for the player controller.
- **Asset churn in 5.5 to 5.6:** there is a separate post on camera asset format changes — [Chabant, "UE5 Gameplay Cameras: Asset Changes" (2025-01-20)](https://ludovic.chabant.com/blog/2025/01/20/ue5-gameplay-cameras-asset-changes/)
- **Official API:** `UCameraDirector` is part of the GameplayCameras plugin. Its API page lists director duties such as building the director and gathering rig usage info — [UCameraDirector API](https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Plugins/GameplayCameras/UCameraDirector)
- **First-person node:** the plugin has a first-person camera node, `UFirstPersonCameraNode` (found by API page title; contents not fetched) — [UFirstPersonCameraNode API](https://dev.epicgames.com/documentation/unreal-engine/API/Plugins/GameplayCameras/UFirstPersonCameraNode)
- **Documentation gap:** a forum post said the plugin was still "Experimental" and that "there's not really any proper documentation for it" — [forum, rig broken in 5.6](https://forums.unrealengine.com/t/gameplay-camera-rig-broken-in-5-6/2550183)
- **Third-party guide, dated 2026-04-17. Treat with caution: the vendor sells camera plugins and an MCP server, and its class and node names did not match other sources.**
  - Rigs are `UCameraRigAsset` node graphs.
  - The listed nodes are BoomArm, FollowTarget, OrbitInput, LookAt, FramingSubjects, CameraShake, Offset, CollisionAvoidance, PerlinNoise and SplineFollow. The guide itself admits node names changed between 5.5 and 5.7.
  - Custom nodes subclass `UCameraNode`.
  - Transitions have a duration, an easing type and interrupt rules.
  - The guide quotes a roadmap target of Beta around 5.8. It reports roughly 0.03-0.08 ms per 10-node rig, without saying how this was measured.
  - It does not cover first-person use.
  - Source: [StraySpark, "Gameplay Cameras Plugin in UE5.7: A Production Guide"](https://www.strayspark.studio/blog/gameplay-cameras-plugin-ue5-7-production-guide)
- **Known limitation with first-person rendering:** in a 5.5 forum thread, native first-person FOV worked on a standard camera component. It did **not** work with Lyra's camera manager or with the Game Animation Sample (GASP) Gameplay Camera setup, and no confirmed fix was posted — [forum: 5.5 First Person Rendering FOV with CameraManager (Lyra or GASP)](https://forums.unrealengine.com/t/5-5-first-person-rendering-fov-with-cameramanager-lyra-or-gasp/2224698)
- **Camera shakes are a separate system:** shake patterns (Perlin, Wave, Composite, Sequence) live in the "EngineCameras" API area — [EngineCameras API](https://dev.epicgames.com/documentation/unreal-engine/API/Plugins/EngineCameras), [EOscillatorWaveform](https://dev.epicgames.com/documentation/unreal-engine/API/Plugins/EngineCameras/EOscillatorWaveform)

### Inferences
- The GPC model maps cleanly onto ideas from research. Rig = a shot or camera behaviour. Director = the logic that picks a shot, close to He et al.'s idiom state machines (see B1). Transition = blending. In principle it is the "camera director" layer the developer wants.
- Building on GPC in 5.7/5.8 now carries real risk for a 17-chapter project:
  - The format breaks every release.
  - It is maintained largely by one person.
  - It interacts badly with the first-person FOV/scale rendering feature.
  - 5.8 is the last 5.x release, so whatever GPC is in 5.8 is what this project gets unless it moves to UE6.
- Background, not re-checked this session: the classic stack is `APlayerCameraManager` → `UCameraModifier`, a priority list of additive or overriding modifiers with alpha blend in/out, plus `UCameraShakeBase` and `CameraAnimationSequence` (Sequencer-authored camera animations played as shakes). It is production-proven, C++-friendly, and already works with a custom first-person pawn. A "camera director" can be written as a few `UCameraModifier` subclasses, for example LookAtTarget, ForcedPitch, SlipTumble and FallRoll, each driven by a data asset. That is about the same idea as GPC rigs, without depending on an experimental plugin.

### Gaps
- Gameplay Cameras' real status in 5.8 (Experimental or Beta) could not be confirmed. The 5.8 release notes page did not render, and no 5.8 upgrade post from Chabant turned up in search.
- The official GPC overview page (`gameplay-cameras-in-unreal-engine`) did not render, so the full official node list and director types (single, state machine, Blueprint, priority/chooser) are not verified here.
- No measured performance numbers from Epic for GPC.

---

## A2. Sequencer driven from code: Python/Blueprint API, camera cuts, keyed transforms, spawnables, event tracks, Rail/Crane rigs, Take Recorder, constraints, prestreaming, Director Blueprints, Motion Design

### Takeaway
Level Sequences can be created and filled entirely from editor Python: create the asset, add possessable or spawnable bindings, create a spawnable camera with a camera cut track, and add transform and animation tracks and sections. Claude Code can generate cutscene assets offline from a data description, such as a shot list. Known rough edges are camera-cut-track and component-track bugs. Keying is done through the scripting channel API, which the docs barely cover, so the engine's sample scripts are the real reference.

### Cited Findings
- **Asset creation:** `unreal.AssetToolsHelpers.get_asset_tools().create_asset(...)` with `unreal.LevelSequence` and `unreal.LevelSequenceFactoryNew()` — [Epic docs: Python Scripting in Sequencer](https://dev.epicgames.com/documentation/en-us/unreal-engine/python-scripting-in-sequencer-in-unreal-engine)
- **Opening and focusing:** `unreal.LevelSequenceEditorBlueprintLibrary` (`open_level_sequence`, `get_current_level_sequence`, `get_focused_level_sequence`, `focus_level_sequence`, `refresh_current_level_sequence`) — [same doc](https://dev.epicgames.com/documentation/en-us/unreal-engine/python-scripting-in-sequencer-in-unreal-engine)
- **Frame rate and range:** `unreal.FrameRate`, `set_display_rate`, `set_playback_start/end`. The defaults are 30 fps and a range of 0-150 — [same doc](https://dev.epicgames.com/documentation/en-us/unreal-engine/python-scripting-in-sequencer-in-unreal-engine)
- **Bindings:** `LevelSequenceEditorSubsystem` has `add_actors()` (possessables), `create_camera(spawnable=True)` (adds a spawnable camera binding plus a camera cut track), and `convert_to_spawnable()` / `convert_to_possessable()`. Spawnables exist only while the sequence plays — [same doc](https://dev.epicgames.com/documentation/en-us/unreal-engine/python-scripting-in-sequencer-in-unreal-engine)
- **Tracks:** `binding.add_track(unreal.MovieScene3DTransformTrack)` and `unreal.MovieSceneSkeletalAnimationTrack`, then `track.add_section()`, `section.set_range()`, and `anim_section.params.animation` to assign an animation — [same doc](https://dev.epicgames.com/documentation/en-us/unreal-engine/python-scripting-in-sequencer-in-unreal-engine)
- **Keys and channels:** the terminology table names `MovieSceneScriptingChannel`, `MovieSceneScriptingFloatChannel` and `MovieSceneScriptingFloatKey`, but the page shows no keyframing code — [same doc](https://dev.epicgames.com/documentation/en-us/unreal-engine/python-scripting-in-sequencer-in-unreal-engine)
- **Not on the page:** event-track APIs, Movie Render Queue and `MovieSceneSequenceExtensions`. It points to the sample scripts at `Engine\Plugins\MovieScene\SequencerScripting\Content\Python` (for example `sequencer_examples.py`) — [same doc](https://dev.epicgames.com/documentation/en-us/unreal-engine/python-scripting-in-sequencer-in-unreal-engine)
- **Camera cuts from data:** community examples add a camera cut track and set each cut section's range and camera with `camera_section.set_camera_binding_id(camera_id)`. One thread drives camera cuts from a CSV — [forum: drive sequencer camera cuts from a CSV](https://forums.unrealengine.com/t/want-to-drive-sequencer-camera-cuts-from-a-csv/152121)
- **Scripting bugs (status per engine version unknown):**
  - A camera cut track added by script could not be set as the movie scene's camera cut track — [UE-87538](https://issues.unrealengine.com/issue/UE-87538)
  - A spawnable CineCameraActor added by script got no camera component track — [UE-90656](https://issues.unrealengine.com/issue/UE-90656)
  - The forum advice: create the camera, make it spawnable, then add a possessable binding for its CineCameraComponent so focal length and focus can be keyed — [forum: add CineCameraComponent to spawnable](https://forums.unrealengine.com/t/add-cinecameracomponent-to-spawnable-cinecameraactor/481841)
- **Sub-sequence problem:** converting a camera to spawnable inside a sub-sequence blocked its camera cut, while the same steps worked in a top-level sequence — [forum: cannot convert camera to spawnable in sub-sequence](https://forums.unrealengine.com/t/cannot-convert-camera-to-spawnable-state-in-sub-sequence/140347)
- **Camera cut track:** Epic documents the Camera Cut Track as the track that switches which camera is viewed — [Epic docs: Camera Cut Track](https://dev.epicgames.com/documentation/unreal-engine/cinematic-camera-cut-track-in-unreal-engine)
- **Rail and Crane rigs:**
  - Camera Rig Rail is a spline-based dolly track whose length and curve can be edited.
  - Camera Rig Crane gives boom shots with pivot and extension.
  - Both can be animated in Sequencer.
  - Source: [Epic docs: Camera Jibs and Dollies](https://dev.epicgames.com/documentation/en-us/unreal-engine/camera-jibs-and-dollies-in-unreal-engine), [UE 4.27 Camera Rig Rail how-to](https://docs.unrealengine.com/4.27/AnimatingObjects/Sequencer/Cameras/CameraRigRail)
- **Known jitter:** a Cine Camera using Look At while attached to a Rail or Crane jitters. The workaround is baking the transform — [UE-33632](https://issues.unrealengine.com/issue/UE-33632), [forum, 5.3.2 jitter](https://forums.unrealengine.com/t/jittering-when-using-the-lookat-camera-and-camera-rig-crane-in-sequencer-ue-5-3-2/1733803)
- **World Partition crash (5.7):** a forum user reported a Sequencer crash and suspected a camera cut at the start of a sequence combined with World Partition unloading during the jump — [UE 5.7 Released forum thread](https://forums.unrealengine.com/t/unreal-engine-5-7-released/2673913)
- **Motion Design (Avalanche):** introduced in UE 5.4. 5.8 adds "performance and workflow improvements, including the option to combine masks" — [CG Channel on UE 5.8 (2026-06-17)](https://www.cgchannel.com/?p=176531)

### Inferences
- An offline "cutscene compiler" is realistic for this project. Claude Code would read a JSON/YAML shot list (beat, duration, camera behaviour, target, lens, shake) and write a Level Sequence through editor Python. The project already runs editor Python remotely (see project memory "Editor Python remote exec"). This turns a cutscene into data that can be reviewed and regenerated, instead of hand-keyed work.
- For first person, most takeovers should **not** be Sequencer cutscenes with a camera cut, because a cut breaks presence. The better pattern is to play a Level Sequence or CameraAnimationSequence as an *additive* offset on the player camera, or to blend the view target to a sequence camera that starts at the exact player eye transform and returns there.
- Background, not re-checked this session:
  - Level Sequence Director Blueprints let event tracks call gameplay functions.
  - Take Recorder can record a play session (camera and actors) into a Level Sequence. That suggests "play it, record it, clean it up" authoring: the developer plays the summit reveal once with a gamepad, and the recording becomes the keyed camera.
  - Sequencer Constraints (5.2+) can parent or aim a camera at moving actors without baking.
  - Cinematic Prestreaming records which assets a camera cut needs and pre-streams them. That matters for a 1:1 terrain with World Partition.
  - These should be checked against the 5.8 docs before relying on them.

### Gaps
- No official Python example for keying transform channels or adding event-track keys was retrieved. The engine's `SequencerScripting/Content/Python` samples should be read locally (they ship with the engine).
- Whether UE-87538 and UE-90656 are fixed in 5.8 is unknown.
- Take Recorder, Sequencer Constraints, Cinematic Prestreaming and Level Sequence Director docs were not fetched this session (time and budget). The facts above are background knowledge only.

---

## A3. Look-at, focus/depth of field, auto-focus, procedural camera shakes, first-person camera layering

### Takeaway
UE has built-in procedural pieces that need no keyframes:
- Cine Camera Look At Tracking, with an interp speed.
- Cine Camera focus settings.
- Four camera shake pattern types (Perlin, Wave/sine oscillator, Composite, Sequence) with a previewer.
- Native First Person Rendering: a separate FOV and scale for first-person primitives. Experimental in 5.5; a user reports it as Beta in 5.6.

Together these cover most short first-person "cinematic" moments (slip, fall, gaze pull, breath, exhaustion) as parameterised, data-driven effects.

### Cited Findings
- **Look At Tracking:** the Cine Camera setting "overrides the camera's rotation and modifies it to aim towards a specified actor". An Interp Speed value controls how fast it follows, and the default 0 tracks instantly — [Epic docs: Cinematic Cameras](https://dev.epicgames.com/documentation/unreal-engine/cinematic-cameras-in-unreal-engine?lang=en-US) (via search snippet)
- **5.7 regression report:** cameras made in 5.6 with Look At Tracking plus a Relative Offset tilt slightly upward after upgrading to 5.7. This is a forum report, not a confirmed bug — [UE 5.7 Released forum thread](https://forums.unrealengine.com/t/unreal-engine-5-7-released/2673913)
- **Perlin noise shake:** generates noise over time by sampling random points from an amplitude and frequency. Recommended for high-intensity shakes such as rumbles or nearby explosions — [Epic docs: Camera Shakes](https://dev.epicgames.com/documentation/en-us/unreal-engine/camera-shakes-in-unreal-engine)
- **Wave oscillator shake:** a smooth sinusoidal oscillation for light effects like a boat rocking or a drifting feel — [Epic docs: Camera Shakes](https://dev.epicgames.com/documentation/en-us/unreal-engine/camera-shakes-in-unreal-engine)
- **Composite shake:** layers Perlin, Wave and Sequence shakes into one shake. A Camera Shake Previewer is available — [Epic docs: Camera Shakes](https://dev.epicgames.com/documentation/en-us/unreal-engine/camera-shakes-in-unreal-engine)
- **Python exposure:** shakes have Python-exposed classes such as `PerlinNoiseShaker` and `OscillatorWaveform` — [PerlinNoiseShaker Python API](https://docs.unrealengine.com/PythonAPI/class/PerlinNoiseShaker.html), [OscillatorWaveform Python API](https://docs.unrealengine.com/5.0/en-US/PythonAPI/class/OscillatorWaveform.html)
- **Community extensions:**
  - "Enhanced Camera Shake Patterns" adds frequency blending, so shakes speed up and slow down on start and stop, plus manual phase offset — [forum: Hubert Mika - Enhanced Camera Shake Patterns](https://forums.unrealengine.com/t/hubert-mika-enhanced-camera-shake-patterns/2491748)
  - A paid "Camera Shake Toolkit" for 5.7 exists — [StraySpark Camera Shake Toolkit](https://www.strayspark.studio/products/camera-shake-toolkit)
- **First Person Rendering:** primitives can be tagged as first person, and cameras apply a custom FOV and scale to them. This stops the view body and hands from clipping or distorting at gameplay FOV. Camera components expose "Enable First Person Field Of View" and "Enable First Person Scale" — [Epic docs: First Person Rendering](https://dev.epicgames.com/documentation/unreal-engine/first-person-rendering?lang=en-US), [80.lv on UE 5.5 native first-person rendering](https://80.lv/articles/unreal-engine-5-5-received-native-first-person-rendering-support)
- **First Person Rendering status:** Experimental in 5.5; one user reports it as Beta in 5.6 and working well. This is a single user's report — [forum: how to use First Person Rendering 5.5](https://forums.unrealengine.com/t/how-to-use-first-person-rendering-5-5-new-experimental-feature/2122644/8). Other threads report bugs and question whether it works only for skeletal meshes — [forum: first person rendering bugs](https://forums.unrealengine.com/t/first-person-rendering-bugs-5-5-experimental-feature/2388918), [forum: only skeletal meshes?](https://forums.unrealengine.com/t/does-first-person-rendering-in-5-5-only-work-for-skeletal-meshes/2566018)
- **Depth of field in 5.8:** Movie Render Graph is "now production-ready" and gets a new accumulation depth-of-field system that "plays nicer with deferred rendering". This affects offline renders such as trailers, not the real-time game — [CG Channel on UE 5.8](https://www.cgchannel.com/?p=176531)

### Inferences
- First-person "camera moments" split cleanly into three layers that one person can build:
  1. **Additive shake/sway layer:** a library of `UCameraShakeBase` assets (Perlin, Wave, Composite, Sequence) chosen by gameplay state. Breathing at altitude, wind gusts, exhaustion, ice-axe impacts, slip judder.
  2. **Gaze/aim override layer:** a Camera Modifier that blends the view rotation toward a target with eased weight, with player input still mixed in at reduced strength. For summit reveal, serac collapse, a rope partner calling.
  3. **Full takeover layer:** rare. Fall/tumble or faint, played as a short CameraAnimationSequence or Level Sequence that starts and ends at the player's eye transform.
- Auto-focus/DoF: for a real-time first-person game, depth of field during takeovers can be driven by a line trace from the camera to the look-at target, setting focus distance each frame on the post-process or Cine Camera. Background knowledge: Cine Camera focus settings include a "Tracking" focus method that follows an actor; not fetched this session.
- The project's existing view body and ice-axe IK (memory: "Fixed ropes and hands", "Feet contact + footprints") would benefit from First Person Rendering FOV and scale. The forum reports show it conflicts with Lyra and GASP camera managers, which suggests keeping the custom pawn camera rather than adopting GPC.

### Gaps
- The Cine Camera focus settings page (Manual / Tracking / Disabled, focus smoothing) returned HTTP 403 and was not verified.
- No Epic doc fetched on layering camera animation from mocap (head-bone-driven camera) for first person. Background: many UE first-person setups attach the camera to the head bone or blend head-bone motion into the camera, but this is not sourced here.

---

## A4. UE 5.6 / 5.7 / 5.8 release-note items that matter for automation

### Takeaway
5.8 (June 2026) is the most relevant release:
- An **experimental MCP plugin for the Unreal Editor** (LLM tool access).
- **Experimental animation mixing in Sequencer.**
- **Production-ready Movie Render Graph.**
- MetaHuman Animator on more platforms; experimental MetaHuman crowds.

5.7 added an Ease Curve preset tool in Sequencer. No release-note item was found that adds a dedicated "procedural camera" or "automatic cinematography" feature.

### Cited Findings
- **Release date and context:** UE 5.8 was released (CG Channel article dated 2026-06-17). Hotfix 5.8.1 followed with 260+ fixes. Several outlets describe 5.8 as the last UE 5.x release before UE6 — [CG Channel](https://www.cgchannel.com/?p=176531), [5.8.1 Hotfix forum](https://forums.unrealengine.com/t/5-8-1-hotfix-released/2738864), [GameFromScratch: UE 5.8 Released](https://gamefromscratch.com/unreal-engine-5-8-released/)
- **5.8 Sequencer and cinematics:**
  - "experimental support for animation mixing in Sequencer".
  - Direct Mesh Control (pose characters "by pulling body parts around directly"), experimental, in the viewport and Sequencer.
  - Control Rig Dynamics (particle-based secondary motion), experimental.
  - Source: [CG Channel on UE 5.8](https://www.cgchannel.com/?p=176531)
- **5.8 MetaHuman:**
  - MetaHuman Animator now runs in-editor on Linux and macOS as well as Windows.
  - MetaHuman Crowd ("use MetaHumans for crowds, not just hero characters") is experimental.
  - Source: [CG Channel on UE 5.8](https://www.cgchannel.com/?p=176531), [Epic forum: MetaHuman 5.8 released](https://forums.unrealengine.com/t/metahuman-5-8-released/2729288)
- **5.8 MCP plugin:** "an experimental Model Context Protocol (MCP) plugin for the Unreal Editor" — [CG Channel on UE 5.8](https://www.cgchannel.com/?p=176531), [PC Gamer on UE 5.8](https://pcgamer.com/software/unreal-engine-5-8-launches-with-improved-terrain-and-vegetation-tools-a-lumen-lite-option-for-faster-global-illumination-and-for-the-times-we-now-live-in-an-open-standard-plugin-for-llm-systems/)
- **5.8 rendering:** MegaLights is Production Ready; Lumen Lite and an experimental Mesh Terrain are added — [Epic 5.8 release notes (search snippet)](https://dev.epicgames.com/documentation/en-us/unreal-engine/unreal-engine-5-8-release-notes), [Wccftech](https://wccftech.com/unreal-engine-5-8-lumen-lite-60-fps-switch-2/)
- **5.7 Sequencer:**
  - An Ease Curve tool to "apply and save different Ease Curve presets in Sequencer, Curve Editor, and UMG".
  - Scale lock in the Sequencer tree.
  - Source: [Epic 5.7 release notes (search snippet)](https://dev.epicgames.com/documentation/unreal-engine/unreal-engine-5-7-release-notes)
- **5.7 Gameplay Cameras changes:** see A1 — [Chabant 5.7 post](https://ludovic.chabant.com/blog/2025/11/14/ue5-gameplay-cameras-upgrading-to-5-7/)

### Inferences
- The 5.8 MCP plugin matters for the "AI makes the cinematics" goal. It is Epic's own route for an LLM agent to drive the editor. The project currently uses a third-party unreal-mcp server plus remote Python. Being experimental, it should be judged against the existing setup rather than adopted blind.
- Animation mixing in Sequencer (experimental) could let generated sequences layer and blend animation clips for story characters (Nima, Dr. Hale, the Kovac team in the chapter design) without hand-keying. This is unverified until tested.

### Gaps
- The full camera, Gameplay Cameras and Sequencer sections of the 5.6, 5.7 and 5.8 release notes could not be read (JS-rendered pages), so camera-specific items such as GPC status in 5.8 may be missing.
- No 5.6 release-note camera items were retrieved directly. Only the GPC 5.6 changes (via Chabant) are covered.

---

## A5. Epic samples showing procedural / gameplay-driven cameras

### Takeaway
The only confirmed sample link found is the **Game Animation Sample (GASP)**, which uses a Gameplay Cameras setup. Lyra uses its own camera-manager approach. No confirmed Epic sample was found that shows procedural *cinematic* cameras for first person. Nothing was found on "Stack O Bot", "Cropout" or "Cassini" in relation to cameras.

### Cited Findings
- A 5.5 forum thread names "the GASP Gameplay Camera setup" and the Lyra camera manager. Both ignored native first-person FOV settings, and a reply notes that "the lyra camera component ignores these settings" — [forum: 5.5 First Person Rendering FOV with CameraManager (Lyra or GASP)](https://forums.unrealengine.com/t/5-5-first-person-rendering-fov-with-cameramanager-lyra-or-gasp/2224698)
- A forum thread discusses adding first person to the GASP (UE 5.5+), which ships as third person — [forum: First person in new Animation sample project](https://forums.unrealengine.com/t/first-person-in-new-animation-sample-project-ue-5-5/2360856)
- Epic's C++ tutorial track covers adding a first-person camera, mesh and animation — [Epic docs: adding a first-person camera, mesh and animation](https://dev.epicgames.com/documentation/unreal-engine/coder-04-adding-a-firstperson-camera-mesh-and-animation?lang=ja)

### Inferences
- GASP is the best Epic reference for studying GPC rigs in a real character setup. It is third person, so its camera graph would have to be rebuilt for first person.
- Background, not verified this session: Lyra's camera uses a `ULyraCameraMode` stack with weighted blending. That is a simple, readable pattern for "push a camera mode for 1.5 s, then pop it", and it fits short first-person takeovers well.

### Gaps
- Could not confirm which (if any) of Stack O Bot, Cropout or the 2025-2026 samples (including "Cassini") use Gameplay Cameras or procedural cinematic cameras.

---

## B1. Classic automated virtual cinematography (1996-2015): DCCL, Virtual Cinematographer, discourse planning, toric space, camera-on-rails, Prose Storyboard Language

### Takeaway
The foundational methods are rule-based and **cheap to run**. Cinematographic knowledge is encoded as film "idioms": small hierarchical state machines (He et al. 1996) or a declarative language (DCCL, Christianson et al. 1996). Discourse planning (Jhala & Young, Darshak) adds a narrative layer. The toric space (Lino & Christie) reduces framing one or two subjects to 3 angles. Rails (Galvane) keep camera paths smooth and realistic. PSL (Ronfard) is a text notation for shots. All of these assume an *external* camera filming characters; only parts carry over to first person.

### Cited Findings
- **DCCL (Christianson et al., AAAI 1996):** "Declarative Camera Control for Automatic Cinematography" formalizes principles of cinematography into a declarative language, the Declarative Camera Control Language (DCCL) — [ResearchGate: Declarative Camera Control for Automatic Cinematography](https://www.researchgate.net/publication/2329036_Declarative_Camera_Control_for_Automatic_Cinematography)
- **Virtual Cinematographer (He, Cohen & Salesin, SIGGRAPH 1996):**
  - Demonstrated in a virtual "party" setting.
  - Cinematographic expertise is encoded as film idioms, stored as "a set of small hierarchically organized finite state machines".
  - Source: summary of the DCCL / Virtual Cinematographer records — [ResearchGate DCCL record](https://www.researchgate.net/publication/2329036_Declarative_Camera_Control_for_Automatic_Cinematography). The SIGGRAPH paper's own page was not fetched.
- **Jhala & Young, AAAI 2005:**
  - Decides at three levels: geometric composition, camera parameters for affective information, and shot transitions for rhetorical coherence.
  - Idioms capture the causal motivation for shot selection, and designers can rank candidate shot sequences.
  - Implemented in the Mimesis system.
  - Source: [AAAI 2005 paper PDF](https://cdn.aaai.org/AAAI/2005/AAAI05-049.pdf), [ACM DL record](https://dl.acm.org/doi/10.5555/1619332.1619383)
- **Darshak:** Jhala & Young's later end-to-end system. A hierarchical partial-order planner takes a structured representation of scene actions and searches for idioms and shots to convey them — [AAAI 2005 / Darshak line](https://cdn.aaai.org/AAAI/2005/AAAI05-049.pdf), [Springer: Discourse and Camera Control in Interactive Narratives](https://link.springer.com/rwe/10.1007/978-981-4560-50-4_56)
- **Toric space (Lino & Christie, ACM TOG 34(4), 2015):** reduces the usual 7-degree-of-freedom camera problem to 3 DOF, so a viewpoint becomes "a 3d vector of angles" relative to two targets — [ResearchGate: Intuitive and efficient camera control with the toric space](https://www.researchgate.net/publication/278625180_Intuitive_and_efficient_camera_control_with_the_toric_space)
- **ToricCam:** Lino publishes a C++ library, ToricCam, as "a starting point to using the Toric Space viewpoint representation" — [Christophe Lino: ToricCam](https://sites.google.com/site/christophelino/softwares/toric-cam)
- **Camera-on-rails (Galvane thesis, 2015):** *Automatic Cinematography and Editing in Virtual Environments*, supervised by Ronfard and Christie. Camera motion is constrained to a virtual rail, and position and orientation are optimized along it for realistic paths. The work also covers narrative-driven camera control for cinematic replay of games, building on DCCL idioms — [ResearchGate: Automatic Cinematography and Editing in Virtual Environments](https://www.researchgate.net/publication/286089572_Automatic_Cinematography_and_Editing_in_Virtual_Environments)
- **Prose Storyboard Language (Ronfard, Gandhi, Boiron, Murukutla):**
  - A formal notation that describes a movie shot by shot, "each shot described with a unique sentence".
  - It separately covers the spatial layout of frames (compositions) and the temporal structure of shots.
  - It is meant to sit on top of existing camera control systems.
  - A later paper extends PSL to automatically stage actors and cameras in 3D.
  - Source: [arXiv 1508.07593](https://arxiv.org/abs/1508.07593?context=cs), [Eurographics WICED 2022 version](https://diglib.eg.org:443/handle/10.2312/wiced20221048)
- **Review article:** a 2024 review in *Artificial Intelligence Review*, "Intelligent Cinematography: a review of AI research for cinematographic production", surveys the field — [Springer AIR 2024](https://link.springer.com/article/10.1007/s10462-024-11089-3)
- **Industry voice:** John Nesky (Journey's dynamic camera designer), "50 Game Camera Mistakes", GDC 2014 — [GDC Vault](https://gdcvault.com/play/1020460/50-Camera). He says "Dynamic third-person cameras are the hardest to design" and advises not to be embarrassed to choose simpler options — [Vice](https://www.vice.com/en/article/lights-camera-distraction-the-problem-with-virtual-camera-systems). Journey turns the camera to hint at direction near level edges (second-hand, via a Russian write-up) — [Habr](https://habr.com/en/post/272401)

### Inferences
- **What carries over to first person:**
  - The *idiom state-machine* (He et al.) maps directly onto a "camera event director". States such as Free, Glance-at-landmark, Brace, Slip, Tumble, Recover and Reveal, with entry conditions and timed exits, are the same design pattern, at the scale of a few hundred lines of C++.
  - The *discourse layer* (Jhala & Young: why a shot is chosen, ranked candidates) maps onto choosing *which* landmark or beat deserves a gaze-pull right now.
  - *PSL-style text shot descriptions* are an ideal interchange format between an LLM (or the developer's design doc) and a code generator that writes Sequencer assets.
- **What does not carry over:** shot/reverse-shot, the 180° rule, framing two characters (the toric space) and rail dolly paths. These matter only in third-person cutscenes, such as a short out-of-body prologue or a death/rescue replay. The chapter design mentions a summit-night prologue and ghosts, where a third-person camera might appear.
- Nesky's advice backs the simple route: in first person the "dynamic camera" is mostly the player's own head. Automation should change the head *gently and rarely* instead of acting as a director.

### Gaps
- He, Cohen & Salesin's SIGGRAPH 1996 page and the Galvane/Ronfard/Christie/Szilas cinematic-replay paper were not fetched directly. Details come from secondary records.
- No sourced material was found on first-person-specific automated camera research (gaze guidance in games, as opposed to VR). This seems to be a real gap in the literature, or it sits under "attention guidance in VR", which was not searched.
- Other GDC camera talks (Uncharted, God of War's one-shot camera, Insomniac) were not retrieved this session.

---

## B2. Learning-based and LLM-driven camera / trajectory generation (2020-2026)

### Takeaway
Since 2020 the field has moved from rule systems to learning camera behaviour from film:
- Example-driven cinematography (2020) and keyframing with style (2021).
- Text-to-trajectory diffusion: Cinematographic Camera Diffusion (2024) and E.T./DIRECTOR (2024).
- Autoregressive / LLM methods: ChatCam (2024), GenDoP (2025), LensCraft (2025).
- 2026 LLM-agent methods: CinemaTraj composes atomic moves (dolly, orbit, crane, pan, tilt, zoom, arc) over a scene graph; Auteur has a multimodal LLM emit sparse keyframes in a DSL.

These are research code, trained on third-person film framing of characters, and mostly not integrated with any game engine. The design pattern of the 2026 agent papers is the practical lesson: an LLM picks from a small library of parametric camera primitives, and a solver makes them collision-free.

### Cited Findings
- **Example-driven Virtual Cinematography by Learning Camera Behaviors (Jiang, Wang, Christie, Chen et al., ACM TOG 39(4), 2020):** learns camera behaviour from example film clips and expresses camera pose in toric-space coordinates — [paper PDF](https://binwangbfa.github.io/publication/sig20_camerabehavior/SIG20_CameraBehavior.pdf)
- **Camera Keyframing with Style and Control (Jiang, Christie, Wang, Liu, Chen; ACM TOG 40(6), Dec 2021)** — [DOI 10.1145/3478513.3480533](https://doi.org/10.1145/3478513.3480533)
- **Cinematographic Camera Diffusion Model (Jiang, Wang, Christie, Liu, Chen; Computer Graphics Forum 43(2), Eurographics 2024):**
  - A transformer-based diffusion model that generates camera trajectories from text.
  - Supports keyframing constraints and blends motions by latent interpolation.
  - Evaluated with professional artists' feedback.
  - Code and data on GitHub (jianghd1996/Camera-control).
  - Source: [Eurographics DL](https://diglib.eg.org/handle/10.1111/cgf15055), [arXiv 2402.16143](https://web3.arxiv.org/abs/2402.16143)
- **E.T. the Exceptional Trajectories (Courant, Dufour, Wang, Christie, Kalogeiton; ECCV 2024):**
  - A dataset pairing camera trajectories with character information and text captions, described as "the first dataset of its kind".
  - DIRECTOR is a diffusion model that generates trajectories from captions describing how camera and characters relate.
  - CLaTr is a contrastive language-trajectory embedding used for evaluation.
  - Source: [arXiv 2407.01516](https://arxiv.org/abs/2407.01516), [project page](https://www.lix.polytechnique.fr/vista/projects/2024_et_courant/)
- **ChatCam / CineGPT (2024):** a GPT-style autoregressive model for text-conditioned trajectories, plus an LLM agent that uses tools and an "Anchor Determinator" for precise placement — [arXiv 2409.17331](https://arxiv.org/html/2409.17331)
- **GenDoP (April 2025):** autoregressive trajectory generation from text and geometry cues. It introduces DataDoP, a dataset taken from long shots in artistic films, and argues diffusion methods often produce discontinuous motion — [arXiv 2504.07083](https://arxiv.org/pdf/2504.07083)
- **LensCraft (June 2025):** models the filmed subject as a 3D volume rather than a point, with a masked-autoencoder transformer and CLIP conditioning. An independent review says its state-of-the-art claims rest on the authors' own simulator and flags a questionable loss term — [arXiv 2506.00988](https://arxiv.org/pdf/2506.00988), [Pith review](https://pith.science/paper/2506.00988)
- **Survey (June 2025):** "Camera Trajectory Generation: A Comprehensive Survey of Methods, Metrics, and Future Directions" — [arXiv 2506.00974](https://arxiv.org/pdf/2506.00974)
- **FilmAgent (Jan 2025):** "A Multi-Agent Framework for End-to-End Film Automation in Virtual 3D Spaces" (title-level only; details not fetched) — [arXiv 2501.12909](https://arxiv.org/pdf/2501.12909)
- **VERTIGO (2026):** post-trains camera-trajectory generators with preference optimization, using Unity-rendered previews as visual rewards (search-summary level) — [arXiv 2604.02467](https://arxiv.org/html/2604.02467)
- **Auteur (2026):** a fine-tuned multimodal LLM acts as a "virtual director" and emits sparse keyframes in a DSL, which are interpolated into 6-DoF camera paths (search-summary level) — [arXiv 2606.01900](https://arxiv.org/html/2606.01900)
- **CinemaTraj (Li et al., arXiv 2607.26910, submitted 2026-07-29):**
  - Breaks cinematography into atomic movements (dolly, orbit, crane, pan, tilt, zoom, arc), each a parametric trajectory optimized to avoid collisions.
  - An LLM agent splits a natural-language prompt into a sequence of these, grounded in a 3D scene graph built from RGB-D images.
  - Also generates synchronized voiceover and subtitles.
  - Tested on ScanNet++ and reported to beat existing methods on prompt alignment, trajectory quality and safety. The abstract gives no numbers, and a code release was not confirmed.
  - Source: [arXiv 2607.26910v1](https://arxiv.org/abs/2607.26910v1)
- **Text-to-video camera control:** CameraCtrl and DirecT2V were named in the brief, but neither paper was retrieved this session (see Gaps).

### Inferences
- **Practicality for this project: low for runtime ML, moderate for offline ideas.**
  - These models output a third-person camera path relative to characters, trained on film datasets.
  - A first-person climbing game has no external camera most of the time. Its "camera" is the climber's head, limited by body, rope and terrain.
  - Running a diffusion or transformer model at runtime in UE on a laptop target clashes with the project's "performance first" rule. Offline use (generate a path, bake it into a Level Sequence) is possible but has to be bridged by hand from Python research code. CCD (Camera-control repo) is the most accessible because code and data are public.
- **The transferable pattern is CinemaTraj/Auteur-style "LLM picks parametric primitives, solver makes them valid".** For this project:
  - Claude Code (offline) reads a chapter's beat list.
  - For each beat it picks from a small, hand-tuned library of first-person camera primitives: gaze-pull-to-landmark (duration, max angle, ease), slip-judder (Perlin shake preset), fall-tumble (CameraAnimationSequence), exhaustion-sway (Wave shake), summit-reveal (slow yaw plus FOV breath plus DoF focus pull).
  - It writes the result as data assets or Level Sequences through editor Python.
  - At runtime, cheap C++ evaluates them.
  - This matches the project's existing workflow (data, scripts, measured verification) and avoids ML dependencies.

### Gaps
- CameraCtrl and DirecT2V were not retrieved. From prior knowledge, both are text-to-*video* generation methods (camera control inside a video diffusion model) and do not output engine camera paths, so they would not apply directly. This is unverified here.
- No paper was found that applies these learned trajectory models to **first-person** game cameras or to any Unreal Engine pipeline.
- Dataset sizes, licences and code availability were not checked for E.T., GenDoP, CinemaTraj or Auteur.

---

## B3. Procedural / generative cutscene and story generation (game logs, narrative planning with camera, drama managers, storylets)

### Takeaway
The closest academic work to "automatic cutscenes for a game" is:
- **Cine-AI (CHI PLAY 2022):** an open-source Unity toolset that generates in-game cutscenes in the style of human directors, with timeline and storyboard editing plus runtime automation.
- **Narrative-driven camera planning** (Jhala & Young, Galvane's cinematic replay).

On the story side, **storylets** (Kreminski & Wardrip-Fruin 2018), small content chunks chosen by preconditions on world state, are the most practical way to make "the climb writes its own story" scale to 17 chapters.

### Cited Findings
- **Cine-AI (Evin, Hämäläinen, Guckelsberger; PACM HCI CHI PLAY, Article 223, Oct 2022):**
  - "open-source procedural cinematography toolset" that generates in-game cutscenes in the style of eminent human directors.
  - Has a timeline and storyboard interface for design-time editing, runtime cinematography automation, and a director-imitation dataset users can extend.
  - Built on Unity.
  - In two user studies, participants correctly matched generated cutscenes to the target director, and usability was above average.
  - Source: [arXiv 2208.05701](https://arxiv.org/abs/2208.05701)
- **Discourse planning with camera:** Jhala & Young's camera discourse planner (Mimesis) and Darshak plan shot sequences from structured descriptions of story actions — [AAAI 2005](https://cdn.aaai.org/AAAI/2005/AAAI05-049.pdf), [Springer chapter](https://link.springer.com/rwe/10.1007/978-981-4560-50-4_56)
- **Cinematic replay of game sessions:** narrative-driven cinematic replay of game sessions (Galvane, Ronfard, Christie, Szilas), building on DCCL idioms — [Galvane thesis](https://www.researchgate.net/publication/286089572_Automatic_Cinematography_and_Editing_in_Virtual_Environments)
- **Storylets (Kreminski & Wardrip-Fruin, ICIDS 2018):** "Sketching a Map of the Storylets Design Space" maps storylet-based narrative systems along four dimensions: how preconditions are defined, whether a storylet repeats, what content it holds (linear text, replacement grammars, branching), and how content is selected — [paper PDF](https://mkremins.github.io/publications/Storylets_SketchingAMap.pdf), [Emily Short summary](https://emshort.blog/2019/01/06/kreminski-on-storylets/)
- **Origin of the term:** Failbetter Games' term from Echo Bazaar / Fallen London, where storylets are discrete chunks of narrative — [Emily Short tag: storylets](https://emshort.blog/tag/storylets/)
- **LLM-agent film automation:** FilmAgent (2025) is a multi-agent framework for end-to-end film automation in virtual 3D spaces (title-level) — [arXiv 2501.12909](https://arxiv.org/pdf/2501.12909)

### Inferences
- A practical architecture for "the climb writes its own story", which the chapter design already calls a self-writing climb journal:
  - **Storylets** are data rows: preconditions on world state (altitude, weather realm, O2, time awake, companion present, recent slip) plus content (journal line, radio line, camera primitive, sound).
  - A **runtime selector** in C++ picks eligible storylets.
  - **Camera primitives** (B2 inferences) are attached to storylets.
  - This puts drama-manager-like behaviour into a plain table that Claude Code can generate and extend per chapter. It suits the "one chapter per week" bottleneck better than any ML camera method.
- Cine-AI shows the "design-time timeline plus runtime automation" split is viable, but it is Unity and third person. Its value here is as a reference design, not as code to reuse.

### Gaps
- Façade's beat-based drama manager (Mateas & Stern) and Versu (Evans & Short) were not retrieved this session. The search returned nothing usable on either.
- No source found on "cutscene generation from game logs" beyond the cinematic-replay line of work (Galvane et al.).

---

## B4. Which methods are realistic for a solo developer making a first-person game with an AI coding assistant

### Takeaway
Rule-based and data-driven methods win by a wide margin:
- An idiom-style **camera event director** (a few Camera Modifiers or a small state machine).
- A **camera shake library** using UE's Perlin, Wave, Composite and Sequence patterns.
- **Look-at/gaze pulls** with eased weights.
- **Offline Python generation of Level Sequences** from text shot lists.
- **Storylet tables** that trigger them.

ML trajectory models (2024-2026) are third-person, research-grade, and do not fit a laptop-targeted first-person game. Their best use is as inspiration for the "LLM picks parametric primitives" pattern. Gameplay Cameras is architecturally appealing but still experimental, as of the last confirmed status (5.7).

### Cited Findings
- **Industry advice on simplicity:** Nesky calls dynamic third-person cameras "the hardest to design" and advises choosing simpler options without embarrassment — [Vice](https://www.vice.com/en/article/lights-camera-distraction-the-problem-with-virtual-camera-systems), [GDC Vault](https://gdcvault.com/play/1020460/50-Camera)
- **Idiom state machines are small:** the 1996 systems encode cinematography as "small hierarchically organized finite state machines" — [DCCL / Virtual Cinematographer record](https://www.researchgate.net/publication/2329036_Declarative_Camera_Control_for_Automatic_Cinematography)
- **Built-in procedural shake patterns** need no keyframes: Perlin, Wave, Composite, Sequence — [Epic docs: Camera Shakes](https://dev.epicgames.com/documentation/en-us/unreal-engine/camera-shakes-in-unreal-engine)
- **Sequencer from Python:** asset creation, spawnable cameras with camera cuts, and transform/animation tracks are all scriptable — [Epic docs: Python Scripting in Sequencer](https://dev.epicgames.com/documentation/en-us/unreal-engine/python-scripting-in-sequencer-in-unreal-engine)
- **LLM agents plus primitives:** the 2026 work composes atomic parametric camera moves with an LLM over a scene representation — [CinemaTraj arXiv 2607.26910](https://arxiv.org/abs/2607.26910v1), [Auteur arXiv 2606.01900](https://arxiv.org/html/2606.01900)
- **Gameplay Cameras status:** Experimental, mostly one developer, with breaking changes each release — [Chabant 5.7 post](https://ludovic.chabant.com/blog/2025/11/14/ue5-gameplay-cameras-upgrading-to-5-7/)
- **Epic's MCP plugin:** UE 5.8 ships an experimental MCP plugin for the editor, giving an LLM agent an official route into editor automation — [CG Channel on UE 5.8](https://www.cgchannel.com/?p=176531)

### Inferences
- **Recommended stack, ranked by value for effort.** All of it is C++ plus data assets, generated or edited by Claude Code. None of it needs ML at runtime.
  1. **Camera Event Director (C++, about 300-600 lines):**
     - A `UCameraModifier`-based layer on the existing first-person camera.
     - Event types: GazePull(target, maxAngle, holdTime, ease, inputDamping), Brace(pitch, shake), Slip(judder preset, recover), Fall(CameraAnimSequence), Faint(fade, tilt), Reveal(slow yaw, FOV breath, focus pull).
     - One priority stack, so events never fight, with blend in/out on each. This is the He et al. idiom state machine applied to first person.
  2. **Shake library:** about 10-15 `UCameraShakeBase` assets (Perlin, Wave, Composite) tuned once, with intensity scaled by game state (wind, altitude, fatigue). Previewed in the Camera Shake Previewer. Very cheap at runtime.
  3. **Beat/storylet table** (DataTable or JSON): preconditions plus camera event plus journal/radio/sound. Claude Code writes the rows for each chapter from the chapter design doc. This makes the camera layer scale to 17 chapters.
  4. **Offline Sequencer generator** (editor Python): for the few real cutscenes (prologue, summit, rescue or death replay), Claude Code turns a PSL-like text shot list into a Level Sequence with spawnable Cine Cameras, camera cuts, rail/crane rigs and keyed transforms. The developer reviews visually. This fits the project's "verify by measurement, user makes the visual call" rule.
  5. **Optional:** record a gamepad pass with Take Recorder (background knowledge) for hero reveals, then clean it up in script.
  6. **Defer:** adopting Gameplay Cameras until its status is confirmed in 5.8 or UE6. Treat ML trajectory models as reading material only.
- **First-person specific rules** drawn from the research and the project's design goals:
  - Never take full control for more than a second or two except at a fall, faint or death.
  - Keep some player look input during gaze pulls.
  - Start and end every takeover at the exact eye transform.
  - Use cuts only in deliberate out-of-body moments such as the prologue or replays.
  - Never take control during dangerous traversal, so the player is not killed by a cinematic.

### Gaps
- No published comparison of rule-based vs. learned camera systems *for first-person games* was found.
- No measured cost numbers for Camera Modifiers or shakes in UE 5.8 were found. They should be measured in the project with the existing play-session monitor.
