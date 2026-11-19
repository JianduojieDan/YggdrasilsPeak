# Chapter 1 Camera Spec: Eyes on a Body

> Status: **Proposed**, awaiting the director. Nothing is built yet. Date: 2026-11-19.
>
> This spec replaces the camera parts of `Docs/Development/Chomolungma_Chapter1.md` §2.6 and the camera actions in `Tools/Story/chapter1_story.py`.
>
> Sources:
> - the director's play session `Saved/Monitor/20261119_203955/` (frames.csv, log.txt, events.csv, report.md, shots/)
> - screenshots 3, 4, 7 and 8
> - the player-camera map and the story-camera map (read-only analyses)
> - `research_notes/AI tools for cinematic chapters/first_person_cinematic_craft.md` and `ue_camera_systems_and_research.md`
> - three competing designs, scored in Appendix A
>
> Base: "Steady Eyes" (architecture, leases, safety). Grafts: "Eyes on a Body" (invariants, self-test, breathing model, one body) and "Directed Eyes" (shot cards, staging, cinema).
>
> **Coordination:** another session is editing `Source/YggdrasilsPeak/Story/YggStory.cpp/.h` right now (changed 21:05 and 21:19). Line numbers below refer to that working copy (1692 lines) and will shift.
> - New code goes into new files first.
> - YggStory gets one merge pass after that session commits.

---

## 1. What the director saw, and why

| # | What the director saw | Root cause (measured) | Fixed by |
|---|---|---|---|
| 1 | The camera flips over when the story turns the head (gaze pulls, fly-out) | Control-rotation pitch is stored as 0..360. The target pitch is -90..90. `FMath::Lerp` takes the long way round (YggStory.cpp TickCamera L1615 gaze, L1665 fly-out). Measured at 111.4 s, 136.3 s and 451.0 s: full vertical loops at about 390°/s with 5,000°/s spikes. The fly-out ended upside down. | The story never writes control rotation (§2 R2). Nudges are deltas in `ProcessViewRotation`. Fly-out rotation is rebuilt every frame from a look vector with roll 0 (§6.10). |
| 2 | Camera stuck at knee height after the tent (shot 7) | `kneel` only lowers the camera Z (98 cm, eye about 52 cm). In the played build `{"lock","look"}` was swallowed by the gaze branch, so the player walked around camp at 2.07 m/s for 48 s at 52 cm. There is no stand-up. | Posture is a state (§4). You cannot walk while low (§4.2, §5.1 ShapeWalkSpeed). The standing watchdog (§4.6) catches it. |
| 3 | Huge boot right under the lens, screen dark (shot 8) | The standing FeetBody legs sit under a dropped eye. Vignette is forced to at least 0.4. Auto exposure adapts to a view full of snow (about 50 s at -70°). | Body visibility per posture (§5.8). Vignette 0.2, overrides only when an effect is active, steady exposure (§5.9). |
| 4 | Streaks when turning (shots 3, 4) | DLSS frame generation 4x on a 30 fps base. DLSS at 67%. Motion blur quality 5. Flicks up to 600°/s (13-20° per real frame). Story spins at 390°/s. No camera-cut flag on teleports. The diagonal light bands are still unidentified. | Render defaults (§5.10, director's call). Story speed caps (§2 R5). `SetGameCameraCutThisFrame` on every cut (§6.7). A/B tests (§11 V11). |
| 5 | Freezes in the middle of scenes | `AYggStoryNPC::DressAs` loads synchronously: 2.67 s at 69.5 s and 0.58 s at 99.0 s. | Async preload at chapter start. NPCs spawn only behind closed eyelids (§6.16). |

---

## 2. Rules (the camera language of doc §2.6, enforced in code)

- **R1. One writer.** Only `UYggViewRigComponent` and its `UYggViewModifier` move the view. The story asks the rig; it never touches the camera, the control rotation or the view target directly.
- **R2. No interpolated Euler pitch, ever.**
  - Every angle error uses `FRotator::NormalizeAxis` on pitch and yaw.
  - Scripted cameras build their rotation from a look vector (`FRotationMatrix::MakeFromX`, roll 0).
  - The only absolute `SetControlRotation` is a teleport's facing, and it happens behind closed eyelids.
- **R3. The body explains every move.** A view change the player did not make must come from one of these:
  - a step
  - a breath
  - a landing
  - a posture change
  - a reflex to a loud sound
  - a hand pulling
  - a slip
  - a slow nudge that yields to the mouse (last resort)
- **R4. The eye is never low without a low body.** An eye more than 36 cm under standing height exists only in the Lie, Sit, Kneel1, Kneel2 or Crawl posture, or during the Fall or Knock takeover. You cannot walk while posed.
- **R5. Story camera speed caps:**

| Story motion | Cap |
|---|---|
| Nudge | 20°/s yaw, 8°/s pitch (12° total at the plume) |
| Reflex | 10° in 0.3 s |
| Fly-out | 12°/s or less |
| Fall | 120°/s or less at peak |
| Knock | 15° in 0.14 s |

- **R6. Leases.** Everything the story holds is a lease, owned by a beat, with a maximum lifetime and automatic release. This covers holding still, a look lock, a takeover, the fly-out, inspect, hands, pace, the FOV beat and attention. When nothing holds the camera, the watchdog puts it back to plain walking.
- **R7. Cuts happen only with the eyes closed**, with the eyelids at 99% or more, or under the title's black veil. Every cut calls `PlayerCameraManager->SetGameCameraCutThisFrame()`.
- **R8. Look locks** exist only during the Fall, the fly-out, and closed-eyelid holds. Inspect redirects the mouse to the object instead of locking it. Postures use soft look limits, not locks.
- **R9. Pulling the gaze, in this order:**
  1. staging (people and things come into your view)
  2. sound
  3. a companion's head turn
  4. light
  5. a nudge, as the last resort: at most 3 in the chapter (hard cap 5), at least 60 s apart, each only after 12 s of not looking
- **R10. One cinematic only:** the prologue slip turns into the out-of-body fly-out. Everything else is played. There are no hard cuts in the present.

---

## 3. Architecture

### 3.1 Owners

- **`UYggViewRigComponent`** (new; `Source/YggdrasilsPeak/Survival/YggViewRig.h/.cpp`)
  - An `UActorComponent` that ticks in `TG_PostPhysics`: after character movement, before `UpdateCameraManager` (LevelTick.cpp L1847).
  - `UFirstPersonBodyComponent::BeginPlay` creates it at runtime, for the locally controlled pawn only. No Blueprint edit is needed.
  - It owns: the view state machine, postures, the eye position, bob, breathing, landing, leases, watchdog, eyelid value, takeover curves, the fly path, inspect, and the walk-speed shaping.
- **`UYggViewModifier`** (new, same files)
  - A `UCameraModifier` with priority 0, added to the `PlayerCameraManager` by the rig.
  - `ProcessViewRotation` is the input side: soft look limits, look lock, inspect capture, nudge, reflex and hand pull, as additive deltas.
  - `ModifyCamera` is the output side: view-only rotation offsets, FOV terms, and the exposure post-process blend.
  - For any view target other than the pawn, it passes everything through except FOV on the fly camera.
- **FirstPersonCamera re-parenting**
  - At rig BeginPlay, `FirstPersonCamera` is detached from FirstPersonMesh's `head` socket and attached to the capsule with `KeepWorldTransform`. `bUsePawnControlRotation` stays on.
  - The rig sets the camera's world location every tick. Anything attached to the camera rides the true eye: the compass case and needle (CompassComponent.cpp:73-93), the inspect card, the headlamp.
  - The old relative location (-9.74, -11.98, 1.04) is saved, so `ygg.View.Rig 0` restores the old socket camera for A/B tests.

### 3.2 Frame order

1. **TG_PrePhysics**
   - The controller reads input.
   - `UpdateRotation` calls `PCM->ProcessViewRotation`, which runs the modifiers' `ProcessViewRotation` (our limits, lock and nudges), then adds the delta, then `LimitViewPitch`.
   - Character movement runs, with MaxWalkSpeed already shaped by the rig through SurvivalComponent.
2. **TickObjects**
   - The story subsystem ticks: triggers and runs. It calls rig APIs only.
3. **TG_PostPhysics**
   - `UYggViewRigComponent::TickComponent` computes the eye, writes the camera location, advances postures, leases and the watchdog, and drives FlyCam.
4. **UpdateCameraManager**
   - `UYggViewModifier::ModifyCamera` adds view-only rotation and FOV.
5. **TG_PostUpdateWork**
   - FirstPersonBodyComponent places the body meshes, reading the rig's posture drop.

### 3.3 Removed

| Removed | Where |
|---|---|
| `UYggStoryCameraModifier` | YggStory.h:23-36, .cpp:170-194 (its SteadyExposure moves into the modifier) |
| Gaze lerp | GazeFrom/To/Time/Length; handler ≈.cpp:1371-1384; tick :1611-1617 |
| Tumble | Tumble* members; ≈.cpp:1403-1408 and :1626-1634 |
| Old fly-out (FlyRotFrom/To, "rise/back/face" maths) | ≈.cpp:1409-1430, :1651-1667 |
| Auto stand-up hack | :1635-1646 |
| HideOwnBody, HiddenParts, ygg.Story.KneelHideCm | :905, :1647-1650 |
| Locks and the per-tick re-assert | bMoveLocked/bLookLocked, SetLocks, LockForCard, UnlockAfterCard; :1619-1625 |
| ResetStoryCamera | replaced by Rig->ReleaseAll + HardReset |
| Card veil (72%) | |
| `Survival/YggBodyCameraModifier.h/.cpp` | its use in ExpeditionComponent.cpp:742-755 |
| FirstPersonBodyComponent head-follow | HeadFollow, EyeOffset, FollowSpeed (.h:50-60); boom capture (.cpp:170-175); boom block (.cpp:392-400). It never touched the first-person camera. |

---

## 4. The camera state machine

### 4.1 States

There is exactly one **view state**: `Walk`, `Lie`, `Sit`, `Kneel1`, `Kneel2` or `Crawl`. Each posture has the phases `Entering`, `Holding` and `Leaving`.

**Overlays** take priority in this order: `Scripted` (Fall, Knock, Fly) > `Inspect` > view state.
- Inspect returns to the view state it came from.
- Scripted always ends either in black, with a HardReset (Fall, Fly), or with a spring back to the eye (Knock).

**Additive layers** (each with a cvar and a weight):

| Layer | Active in |
|---|---|
| terrain spring | all |
| posture height | all |
| pack and slope stoop | Walk |
| neck and waist | all |
| step bob | Walk, plus Crawl's hand-plant dip |
| breathing | all except Scripted |
| landing | Walk |
| start and stop nods | Walk |
| shiver | all |
| heart FOV | all |
| story FOV, attention, FOV beat | all |
| nudge, reflex, hand pull | Walk, Sit, Kneel |
| exposure | all |
| eyelids | all |

### 4.2 Transitions

| From | To | Trigger | Notes |
|---|---|---|---|
| Walk | Posture P | story `{"pose": P}` and speed < 20 cm/s | The body turns to `face` over 0.6 s (capsule yaw; `bUseControllerRotationYaw` off and saved). The anchor yaw is the face yaw, or the current view yaw. Look limits tween in over 1.0 s from the current view, so the view never snaps. |
| any | Posture P (snap) | `{"pose": P, "snap": true}` | Only with the eyelids at 99% or more, or inside a teleport's "during". |
| Lie | Sit | first move or look-key press, or `auto` seconds | 2.2 s sit-up |
| Sit (with `exit`) | Crawl | move input held for 0.3 s or more, and no Move lease | 0.8 s; plane constraint along the exit line |
| Sit / Kneel1 / Kneel2 (no `exit`) | Walk | move input held for 0.3 s or more, and no Move lease | Walk speed stays 0 until the eye reaches 85% of standing height. Then the body turns to control yaw at 360°/s (at most 0.5 s) before `bUseControllerRotationYaw` returns. |
| Crawl | Walk | within 60 cm of the exit place | 1.4 s stand-up, exposure +`exit_ev` |
| Crawl | Walk (fallback) | 90 s in Crawl without reaching the exit | blink 0.15 / 0.5 s, teleport to the exit place behind the eyelids |
| any | Inspect | story `inspect`, or an inspectable prompt + E | Takes an Inspect lease. Speed is shaped to 0 and the mouse turns the object. |
| Inspect | previous | E, right mouse or Esc (left mouse does not close it). With `need_back`, only after the back has been seen. | 0.25 s pocket |
| any | Fall | story `fall` | Takes Fall, Look and Move leases. Ends with the eyelids closed. |
| Walk | Knock | story `knock` | 0.14 s kick, then a spring back over 0.5 s. Usually followed by a story `pose kneel1`. |
| any (eyelids closed) | Fly | story `flyout` | View target switches to FlyCam with a cut; pawn hidden |
| Fly | Walk | path finished, eyelids or veil closed | Cut, `HardReset`, pawn shown |

Moving with no Move lease always stands you up first. Walking while posed is impossible by construction: `ShapeWalkSpeed` returns 0 in every posture except Crawl.

### 4.3 Postures (Phase 1: no animation clips needed)

| Posture | Eye above floor (band) | Down / up time | Yaw limit (± around anchor) | Pitch min / max | Movement | What you see of yourself |
|---|---|---|---|---|---|---|
| Walk (stand) | 156 cm (150-160) | — | free | -75 / +80 | walk | OneBody: FeetBody legs, torso and arms |
| Lie | 32 (26-38) | 2.5 (or snap) / 2.2 to Sit | 70 | -20 / +70 | none | nothing (sleeping bag prop) |
| Sit | 78 (72-84) | 2.0 / 1.6 | 110 | -60 / +55 | none | FirstPersonMesh torso and arms lowered by PostureDrop, legs hidden, sleeping bag prop |
| Kneel1 | 108 (102-114) | 0.9 / 1.0 | 85 | -50 / +50 | none | same, legs hidden |
| Kneel2 | 110 (104-116) | 1.0 / 1.2 | 80 | -45 / +65 | none | same, legs hidden |
| Crawl | 68 (60-76) | 0.8 from Sit / 1.4 to stand | 45 around the exit direction | -40 / +25 | 40 cm/s along the exit line | nothing |

How the postures behave:
- **Height curve:** an S-ease that covers 70% of the change in the first 60% of the time, then a 1.5 cm settle overshoot. Reversing mid-way continues from the current height.
- **Gaze bend on the way down:** -6° toward the `face` target, view-only, released by the end of the transition.
- **PostureDrop** = 156 − posture eye. FirstPersonBodyComponent lowers FirstPersonMesh by it, so the shoulders stay under the eye. FirstPersonMesh is drawn with `FirstPersonPrimitiveType None` (world projection) while posed. The pitch floors keep the missing legs out of frame.
- **Phase 2:** Mixamo clips (sit, kneel, one knee, crawl) play on CharacterMesh0 through a dynamic montage. FeetBody already copies CharacterMesh0 (FirstPersonBodyComponent.cpp:772), so the legs follow automatically. PostureDrop then goes to 0 and the pitch floors relax.

### 4.4 Leases

```
struct FYggLease { int32 Owner; EYggLease Kind; double Start; float MaxSec; bool bKeep; };
EYggLease: Move, Look, Pace, Fall, Knock, Fly, Inspect, Hands, Attention, FovBeat, StoryFov,
           Nudge, Lids, Breath, Exposure, Lens
```

- **Owner** is the beat index. System owners are -1 (posture) and -2 (fallbacks).
- **Release:**

| When | What happens |
|---|---|
| A beat's run ends (`TickRuns`) | `Rig->ReleaseOwner(beat)`, except leases created with `"keep": true` |
| `JumpTo`, `Stop`, chapter end, `Deinitialize`, death, continue | `Rig->ReleaseAll(0.4)` + `HardReset` (death uses 0) |
| A teleport | releases Nudge, Attention and Knock |
| MaxSec is reached | the lease is released and logged: `Rig: lease <Kind> of beat <id> expired after <s>` |

- **Maximum lifetimes:**

| Lease | Max |
|---|---|
| Move | 120 s; `"hold": "end"` lasts to the chapter end (night beat only, checked by the validator) |
| Look | 20 s, and only valid together with Fall or Fly |
| Pace | the given `t`, or 600 s with keep |
| Fall | 3 s |
| Knock | 2 s |
| Fly | its length + 3 s |
| Inspect | until closed, hard cap 300 s |
| Hands | 60 s |
| Attention | 15 s |
| FovBeat | 12 s |
| StoryFov | 300 s with keep |
| Nudge | 3 s |
| Lids closed | 8 s |
| Breath override | 10 s |
| Exposure | 10 s |
| Lens | 20 s |

- Leases never use the controller's ignore-input counters, which menus reset (YggMenuBase.cpp:655/684, SettingsWidget.cpp:234/266, BackpackWidget.cpp:669). So menus can neither leak nor break them.
- **Move hold** is enforced in two places:
  - `ShapeWalkSpeed() = 0`, applied in SurvivalComponent
  - `CharacterMovement->SetJumpAllowed(false)`
- **Look lock** is enforced by zeroing `OutDeltaRot` in `ProcessViewRotation`.

### 4.5 HardReset

`HardReset(FRotator Facing, EYggView Posture = Walk)` runs on every teleport, fly-out end, JumpTo, Stop, death and continue. It does this:
- snaps the terrain spring, bob phase, landing spring and breathing to their targets, so the eye never glides 300 m after a teleport
- sets the posture (snap) and its limits
- sets the takeover offsets to 0
- sets the global time dilation to 1 and the audio pitch to 1
- sets the view target to the pawn and shows the pawn
- turns the plane constraint off
- snaps the story FOV, attention and FOV beat to 0
- restores the camera manager pitch backstops
- restores `bUseControllerRotationYaw`

It does not open the eyelids. The caller decides that.

### 4.6 Watchdog and invariants

The watchdog runs every rig tick, as a handful of compares. Each repair logs `Rig: repaired <what> (beat <id>)`.

| Check | Repair |
|---|---|
| No Fly lease and view target is not the pawn | back to the pawn, behind a 0.15 / 0.5 s blink |
| No Fall or Knock lease and takeover offsets are not 0 | offsets to 0 over 0.3 s; dilation 1; audio pitch 1 |
| No Look lease | look free |
| No Move or Inspect lease, state Walk | speed shaping back to pace or 1 |
| No Lids lease and eyelid > 0 | eyelids open over 0.6 s |
| Story FOV term with no lease | to 0 over 1 s |
| Pawn hidden with no Fly lease | pawn shown |
| Plane constraint on while not in Crawl | constraint off |
| Control-rotation roll is not 0 | roll set to 0 |
| **Standing watchdog**: state Walk, speed > 60 cm/s, eye above the capsule bottom < StandEye − 36 cm (120 cm) for 0.5 s, not landing | eye restored over 0.5 s; logged as an error (the knee-height detector) |

**Invariants**, clamped in every build:

| Invariant | Limit |
|---|---|
| Final pitch | within the active posture limits; never outside [-80, +82] except in Fall, where it is ≥ -72 relative to level |
| Roll | at most 2° outside a takeover; at most 30° in Fall |
| Eye horizontal distance from the capsule axis | at most 24 cm (capsule radius 34 minus the 10 cm near clip) |
| Eye height | at least floor + 20 cm (floor + 30 cm in a takeover; floor = capsule bottom − `CurrentFloor.FloorDist`); at most capsule top − 5 cm |
| Posture band | eye within its band ± 15 cm outside transitions |
| Story FOV | within settings FOV ± 5° (FOV beat included) |

---

## 5. Player camera (walking, standing, turning, looking down)

### 5.1 Eye composition (rig tick; no traces; about 0.02 ms)

```
CapBottom   = Capsule.Z - HalfHeight(96)
TerrainGoal = CapBottom - SinkDepth*SinkToEye(1.0) - min(SmoothedFootDrop*0.5, 10)
              // SinkDepth counted ONCE (today it is counted twice, up to 24 cm).
              // FootDrop (0-70 cm pelvis drop for the legs) reaches the eye at half, capped at 10.
              // FootDrop smoothing: tau 0.5 s down, 1.2 s up (no pumping).
TerrainZ    = CriticalSpring(TerrainZ -> TerrainGoal, 1.8 Hz), |TerrainZ - TerrainGoal| <= 15 cm
              // smooths capsule step-ups and rock edges; posture curves bypass it;
              // HardReset snaps it
H           = PostureHeight(t) - PackStoop - SlopeDown - ExhaustHangZ
Eye         = (Capsule.X, Capsule.Y, TerrainZ + H)
            + YawFrame * (SlopeFwd, 0, 0)
            + NeckModel(pitch, yaw) + WaistLean(pitch)
            + Up*(BobZ + BreathZ + LandZ) + Right*BobY
            + TakeoverLoc (Fall/Knock)
Eye         = ApplyInvariants(Eye)          // section 4.6
FirstPersonCamera->SetWorldLocation(Eye)
```

`YawFrame` is the view yaw (control yaw), not the capsule yaw, because the capsule yaw can be decoupled while posed or while standing still.

### 5.2 Neck and waist

- **Neck:** `n = NeckScale(0.8) * (8 cm forward, 0, 7.5 cm up)`. `Eye += R(pitch,yaw)·n − R(0,yaw)·n`.
  - Looking down -70°: the eye moves 1.4 cm forward and 10 cm down.
  - Looking up 60°: the eye moves 8 cm back and 2.5 cm up.
- **Waist lean:** `w = smoothstep(-45°, -75°, pitch)`; the eye moves `8w` cm forward and `3.5w` cm down. A steep look down sees over the chest to the boots, not into the collar.

### 5.3 Step bob, locked to the footstep sound

**Source.** `USnowTrailComponent` broadcasts `OnStep(bool bLeft, float SpeedCmS, uint8 Surface)` from inside `PlayFootstep` (SnowTrailComponent.cpp:879). That is the moment the crunch plays: after the existing per-surface notify delay (notifies arrive 155 ms early) and also from the cadence safety net. The foot side is kept from `OnFootContact` (:612).

**Phase.** On each step, φ = 0. The period T is the last step interval, clamped to 0.3-1.1 s. Steps closer than 0.2 s apart are ignored.
- Vertical: `BobZ = -(A/2)·cos(2π(φ − 0.07))`. The low point comes just after the crunch; the high point at mid-stance.
- Lateral (stride frequency): `BobY = side·(L/2)·sin(πφ)`, with side +1 for a left plant and −1 for a right plant. The view shifts over the planted foot.
- Pitch compensation: `−atan(BobZ / 1000 cm)`, holding a point 10 m ahead steady.
- Roll: by the table below.

**Fallback.** If no step arrives for 0.8 s while speed is above 30 cm/s, steps are synthesised every 130 cm of travel. After 1.2 T without a step, the bob decays with a 0.25 s half-life.

**Rest-step** (speed < 45 cm/s): a dip-and-hold envelope instead of a cosine: dip 1.2 cm over 0.12 s, hold 0.25 s, rise over 0.35 s, then flat until the next step. Slowing down is visibly rewarded.

Peak-to-peak amplitude by ground speed, interpolated linearly. These values are already about 40% of real head motion, sized for a 90° view on a laptop.

| Speed | Vertical A | Lateral L | Roll |
|---|---|---|---|
| < 0.45 m/s | 1.2 cm, dip-and-hold | 0.5 cm | 0 |
| 0.6 m/s | 1.5 | 0.8 | 0 |
| 1.1 m/s | 2.0 | 1.0 | 0.15° |
| 1.6 m/s | 2.6 | 1.3 | 0.2° |
| 2.3 m/s (today's walk) | 3.0 | 1.5 | 0.2° |
| 3.0 m/s | 3.6 | 1.8 | 0.25° |
| 5.0 m/s (sprint) | 4.5 | 2.2 | 0.3° |

Variation and extras:
- **Jitter per step:** amplitude ±15% × (1 + 0.75·fatigue). Timing jitter comes naturally from the notifies.
- **Downhill** (grade below -8°): dip ×1.3, plus a 0.4° forward nod per step.
- **Rough rock** (Surface = rock): an extra random micro-dip of 0.5-1.0 cm.
- **Head motion setting:**
  - Full: 1.0
  - Reduced: 0.4, no roll, no breathing motion
  - Off: no bob, no breathing; landing ×0.3; the footstep sound is always kept

### 5.4 Breathing, effort, cold, heart

This implements doc rule 8 and keeps the 0.2-0.4 Hz band, where visually induced sickness peaks, nearly empty.

```
EffortIn = MountainAudioComponent::GetEffort()          // 0..1, already drives the breath recordings: sound leads
AltGain  = 0.6 + 0.4*clamp((AltM - 3000)/5400, 0, 1)
BFloor   = 0.6*clamp((AltM - 7000)/1400, 0, 1)          // 0.6 at 8,400 m
B        = max(BFloor, Smooth(EffortIn*AltGain; rise tau 5 s, fall tau 25 s*(0.5+AltGain)))
B        = StoryBreath if a Breath lease is active (the plume: 0.4)
if B < 0.15: pitch ±0.06°, Z ±0.2 cm at 0.25 Hz   (effectively still; breathing is heard and seen as fog)
else:        f = 0.45 + 0.3B Hz (0.5-0.75 Hz); pitch ±0.55B°, Z ±0.5B cm; roll 0
```

- **Exhausted** (stamina below 18%): the head hangs -3° pitch and -3 cm Z, eased in over 4 s. On top, an irregular value-noise sway of 0.35° at 0.6-0.9 Hz. Never any continuous roll. This replaces today's 0.22 Hz pitch ±0.72° / 0.11 Hz roll ±0.64° sway (YggBodyCameraModifier.cpp:15-22).
- **Shiver** (core below 35.4 °C, full at 33.2): 0.12° × Shiver, band-limited noise at 5-8 Hz, capped at 0.25 × the rendered fps. It cannot alias into jitter at a 30 fps base; today's 8.4-13.2 Hz does. Hands get ×3 when they are up.
- **Heart FOV:** 0 below 7,000 m. Above: −0.4° × beat × max(hypoxia, exhaust × 0.7). The 2.5° heartbeat push belongs to the story's plume only (§6.13).

### 5.5 Landing, starts and stops, slope, pack, turning

- **Landing** (`ACharacter::LandedDelegate`, using the vertical speed before impact):

| Parameter | Value |
|---|---|
| Threshold | 150 cm/s |
| Dip | 1.5 cm per 100 cm/s above the threshold, capped at 10 cm |
| Down phase | 0.1 s |
| Spring back | ω 14 rad/s, ζ 0.65; ×1.3 slower above 5,000 m |
| Pitch nod | 0.25°/cm, capped at 2° |
| Examples | a 30 cm step-down gives about 1.5-2 cm; a 1 m drop about 5 cm |

- **Start and stop nods:** -0.4° on the first step, +0.4° on the last (speed crossing 30 cm/s). Spring ω 9, ζ 0.7.
- **Slope posture**, from the grade along the velocity and the floor normal, starting above 8°:
  - uphill: eye 6 cm forward and 4 cm down at 20° (capped at 25°)
  - downhill: eye 2.5 cm back
  - time constant 1.75 s
  - no automatic pitch to the slope and no roll on side slopes: the horizon stays level
- **Pack stoop:** 0.25 cm per kg above 3 kg (15 kg gives 3 cm), capped at 6 cm, blended over 1.5 s. The source is the pack weight shown on the HUD ("pack 14 / 26 kg"). The landing dip is ×(1 + 0.02 per kg over 15).
- **Turn-lean:** off by default (TurnLeanK 0). The director can enable it (0.004 × yaw °/s × speed factor, at most 0.8°).
- **Turn dead zone** (P1): below 10 cm/s the rig turns `bUseControllerRotationYaw` off. The body yaw follows only when it is more than 45° from the view, catching up at 240°/s. It switches back on when walking. Your legs no longer spin under you when you look around at your boots. The rig becomes the single owner of `bUseControllerRotationYaw`; ClimbingComponent (:236/:274) asks the rig.

### 5.6 Look limits (in `UYggViewModifier::ProcessViewRotation`)

- **Soft limits per state** (§4.3):
  - `OutDeltaRot` is scaled toward 0 by `smoothstep(0, 12°, distance to the limit in the direction of motion)`, so you never hit a wall.
  - Yaw limits are relative to the posture anchor yaw, using `NormalizeAxis`.
  - In Sit, holding against the yaw limit for 0.6 s makes the body shuffle round: the anchor follows at 60°/s.
- **Hard backstop:** at rig start, set `PCM->ViewPitchMin = -78` and `ViewPitchMax = 82`, replacing BP_FirstPersonCameraManager's -70 / 80. Restore them on `ygg.View.Rig 0`.
- **Order** inside `ProcessViewRotation`:
  1. inspect capture
  2. look lock (Fall, Fly, closed eyelids)
  3. soft limits and shuffle
  4. nudge, reflex and hand pull (§6.2-6.4)
  5. a final check that keeps the normalised pitch inside the limits

### 5.7 Input and FOV

- **Mouse 1:1:** no acceleration, no smoothing (`Config/DefaultInput.ini:66 bEnableMouseSmoothing=False`), no camera lag.
- **Weight through the body, not the lens:** raised hands trail the view by about 70 ms (`HandFollowSpeed` 40 → 13).
- **Default sensitivity:** **[Director decides]**. It measures 0.175°/count today; the proposal is a multiplier of 0.7 (about 0.12°/count). Flicks of 600°/s on a 30 fps base feed the streaks.
- **FOV:**
  - default 90° horizontal; the settings range widens from 90-110 to **80-110** (YggSettings.cpp:294)
  - no sprint FOV kick
  - story FOV terms are clamped to ±3° (§6.13)
  - the hands keep the BP camera's first-person FOV 70 and scale 0.6

### 5.8 What you see of yourself (one body, one projection)

**Walk (`ygg.View.OneBody 1`):**
- FeetBody, the world-space posable copy of CharacterMesh0 with IK boots, now shows spine, chest and arms. Only the neck and head bones are hidden (FeetBody setup, FirstPersonBodyComponent.cpp:290-318).
- Boots appear from about -40° pitch; the red jacket front and the khata from about -55°.
- Everything is drawn at world FOV, so there is no seam at the hips.
- FirstPersonMesh is hidden unless hands are in use. While hidden it uses `VisibilityBasedAnimTickOption = OnlyTickPoseWhenRendered`, saving one skeletal evaluation. FeetBody copies CharacterMesh0, not FirstPersonMesh (.cpp:772), so the legs are unaffected.
- `ygg.View.OneBody 0` returns to today's two-projection body.

**Posed:** FeetBody is hidden. FirstPersonMesh is shown (world projection), lowered by PostureDrop. The sleeping-bag prop is shown in Sit and Lie (§4.3).

**Hands in use** (compass, photo, Dolma's hand, khata, bottle; P1):
- FirstPersonMesh is shown with PoseArm two-bone IK targets in view space. This is the existing axe code, used without a tool.
- The hands rise in 0.3 s and lower in 0.25 s. FeetBody's upper-arm bones are hidden while they are up.
- The FeetBody foot fit (FeetAlpha) eases out while posed, as it does when climbing.

**Near clip:** engine default 10 cm. If the chest clips at -75° with OneBody, set `NearClipPlane=7` in DefaultEngine.ini (fallback, measure first).

### 5.9 The screen (fixes "dims when looking down")

- **ExpeditionComponent::UpdateScreenEffect** (:655-741):
  - vignette `Lerp(0.2, 0.6, Hypoxia)` instead of 0.4 → 0.9
  - `bOverride_ColorSaturation`, `SceneFringe`, `FilmGrain` and `AutoExposureBias` are set only while their effect is above 0.01. While the body is fine, the level's and Ultra Dynamic Sky's own grading win.
- **SanityComponent.cpp:199-207:** the same gating. Exposure bias goes to `Rig->SetExposureBias("sanity", v)`.
- **One exposure channel in `ModifyCamera`** (`CameraOwner->AddCachedPPBlend`):
  - **Steady exposure:** between -20° and -40° pitch, auto-exposure speed blends down to 0.05 EV/s. This is the working copy's `ygg.Story.SteadyExposure` (YggStory.cpp:161-194), moved here and always on.
  - **Story overshoots:**
    - wake +0.5 EV, τ 1.5 s
    - tent exit +0.4 EV, τ 2 s
    - plume +0.3 EV, held 6 s
- **No full-screen card veil.** Captions are lower-third text on a soft gradient. Inspect darkens the background by 25%.

### 5.10 Render policy (settings defaults; **[Director decides]**)

| Setting | Proposed default | Notes |
|---|---|---|
| Frame generation | "Auto" = 2x | 3x or 4x only after the measured base frame rate stays at 55 fps or more for 10 s (YggSettings.cpp:186). In the played session the GPU needed only 18-20 ms, so about 50 fps base is possible at 2x. |
| Motion blur | Off | "Low" = amount 0.15, max 2% (YggSettings.cpp:201) |
| Camera cuts | on every teleport and at fly-out start and end | §6.7 |
| Prayer-flag materials | check that they write velocity | |

A/B tests before blaming the camera for the streaks:
- frame generation off / 2x / 4x
- `r.MotionBlurQuality 0`
- `ShowFlag.LightShafts 0`
- `ygg.Snow.ShadowRefresh 0`
- lens frost 0

### 5.11 New settings rows (YggSettings + SettingsWidget)

| Row | Options | Effect |
|---|---|---|
| HEAD MOTION | Full / Reduced / Off | `ygg.View.HeadMotion 1 / 0.4 / 0` |
| STORY CAMERA | Guided / Gentle | Gentle: no nudges or reflexes, and the slip becomes a plain eyelid close |
| HEARTBEAT ZOOM | On / Off | |
| COMFORT DOT | Off / On | small centre anchor |
| FIELD OF VIEW | 80-110 | |
| MOTION BLUR | Off / Low | |
| FRAME GENERATION | Auto / Off / 2x / 3x / 4x | |

The existing "Body effects" setting keeps scaling the post-process body effects.

---

## 6. Story camera tools

### 6.1 Look triggers with escalation (`Conditions`, YggStory.cpp ≈1039)

T0 = the moment every other condition (after, delay, near, flags, posture) holds. That is today's `CondSince`.

| Time | Step |
|---|---|
| T0 + 0 | The player looks at the target: cone `cone` (default 14°) held for `dwell` (0.35 s), or `seen` (target inside the inner 60% of the frame for 1.0 s, for things below the eye line) → **fire** |
| T0 + 3 s | `companion`: that NPC turns their head toward the target (`npc_face`) |
| T0 + `lure_at` (7 s; the old `timeout` field maps here, capped at 8) | `lure`: spatial sound at the target, played from within 120° of the view. If the target is behind the player, it plays 60° off-centre on the target's side. |
| T0 + 12 s | `nudge`, only if set, the budget allows, and the player is under 1.5 m/s |
| T0 + `fire` (15 s) | the beat fires anyway |

- `press`: once the look and near conditions hold, a small prompt appears (for example "[E] Open"). The beat fires on Interact instead of on looking. The prompt grows to a hint at 20 s.

### 6.2 Nudge (last resort; inside `ProcessViewRotation`)

```
Want  = (Target + up) - Eye  as a rotator
e_y   = NormalizeAxis(Want.Yaw   - View.Yaw)
e_p   = NormalizeAxis(Want.Pitch - NormalizeAxis(View.Pitch))
done  if |e_y| < 15 and |e_p| < 10                    // centre third at 90° FOV
w     = ramp 0->1 over 0.4 s; stop after 3 s
in    = |player OutDeltaRot| / dt                     // deg/s
yield = clamp(1 - in/30, 0, 1)                        // 15 on gamepad
cancel the nudge if in > 60 deg/s for 0.1 s           // the player chose
rate  = clamp(1.4 * e, +-20 deg/s yaw, +-8 deg/s pitch)
OutDeltaRot += rate * dt * w * yield                  // total <= 45° yaw, <= 8° pitch (plume 12°)
```

- Budget: 5 per chapter (hard) and 60 s apart, enforced in the rig. A request over budget becomes a lure and is logged.
- The validator holds chapter 1 to 3.
- Never above 1.5 m/s. Disabled by Story camera "Gentle".

### 6.3 Reflex

`{"reflex": place|person, "deg": 10}` is the orienting reflex for sudden loud sounds:
- it adds `deg` toward the source over 0.3 s with ease-out, then nothing more; the player finishes the turn
- only when the source is 30-120° off-centre
- cooldown 20 s
- it is a body reaction, so it is not counted in the nudge budget
- disabled by "Gentle"

### 6.4 Hand pull

While Dolma's hand is held (F), a yaw bias of at most 8°/s toward her heading is applied through the same yield path. Walk speed is capped at 0.8 m/s by a Pace lease.

### 6.5 NPC blocking (`AYggStoryNPC`)

- **`EMode::Present`:**
  - goal = player position + view forward (2D) × `dist`, projected onto walkable ground, then face the player
  - the goal is recomputed when the view yaw moves more than 25° or the player moves more than 1 m
  - people come into your view; the view is not dragged to them
- **Follow `talk_side`:** while that NPC has a `say` running, the follow offset becomes side 110 cm and behind 30 cm, so a 30° glance finds the face. Afterwards it returns to its normal offset.
- **`npc_face`:** `{"npc_face": id, "to": place|"player"}` turns the head and torso over 0.5 s.

### 6.6 Eyelids and blinks (`UYggStoryOverlay::NativePaint`)

- **Drawing:** the rig owns `Lid` (0 = open, 1 = closed). Two soft curved lids use one generated texture `T_Eyelid` (256×64, soft edge 12% of the screen height, mirrored for the lower lid). The upper lid covers `Lid·55%` of the screen height, the lower lid `Lid·50%`. Above 0.98, a full black box is added.
- **Modes:**

| Mode | Timing |
|---|---|
| `blink` | close 0.15 s, hold `hold` s, open 0.6 s |
| `closed` | close over `t` |
| `open` | open over `t` |
| `wake` | 35% open at 0.6 s, shut again at 1.1 s, fully open at `t` (2.8 s); exposure overshoot +0.5 EV, τ 1.5 s |
| `sleep` | close over `t` (2.5 s) with one flutter: to 60% at 40% of t, back to 30%, then closed |

- `"during": [actions]` runs on the first fully closed frame (teleport, pose snap, NPC spawns, time jump).
- **`fade`** stays only for the title card and the end page; it shares the same black.

### 6.7 Teleports and cuts

- `teleport` is refused unless the eyelids are at 0.99 or more, the black veil is at 0.99 or more, or the action carries `"cut": true` while the screen is already black at chapter start. Otherwise it is wrapped in an automatic 0.15 / 0.5 s blink and logged as a warning.
- On the cut frame:
  1. `PC->PlayerCameraManager->SetGameCameraCutThisFrame()`
  2. `TeleportTo`
  3. control rotation from the place's own rotation (or `pitch`)
  4. `Rig->HardReset(Facing, pose or Walk)`
  5. log `Story: camera cut <place>`
- `UpdatePendingTeleport` keeps its ground hold (MOVE_Flying until ground is found). The 3 m yank-back applies only while a Move lease is active.

### 6.8 Fall (prologue slip): `{"fall": 1.5, "side": "left|right"}`

- Takes Fall, Look and Move leases. All offsets are additive from the eye at the start.
- Final pitch = `clamp(startPitch + curve, -72, 80)`.
- One downward sphere trace at the start scales Z so the eye stays at least 30 cm above the floor.

| Phase (real time) | Z | Forward | Pitch | Yaw (toward `side`) | Roll | Other |
|---|---|---|---|---|---|---|
| 0.00-0.12 crack | -5 | 0 | -4 | 0 | 0 | |
| 0.12-0.40 lurch | -35 | +10 | -22 | +8 | +12 | |
| 0.40-1.10 slide | -70 | +10 | -48 | +12 | +26 | global time dilation eases to `SlipSlowMo` (0.35) over 0.1 s; audio pitch modulation 0.8; spindrift hangs in the headlamp |
| 1.10-1.50 drop | -85 | +5 | -62 | +15 | +30 | dilation back to 1 over 0.1 s; eyelids slam 1.30-1.45 |

- Peak angular rate is about 110°/s (cap 120).
- Ends with the eyelids closed. The Fall lease ends; the Look and Move leases are held by the beat until the fly-out.
- All story clocks (waits, tweens, eyelids, rig takeover time) use `World->DeltaRealTimeSeconds` while dilation is not 1.
- `SlipSlowMo 1` = no slow motion (director's choice). "Gentle" makes the slip a plain 0.6 s eyelid close.

### 6.9 Knock (risky porter pass): `{"knock": "left|right"}`

- A 0.14 s kick: yaw 15° away from the load, roll 8°, Z -20 cm, pitch -10°. Then a 0.5 s spring back.
- Usually followed by `{"pose": "kneel1", "t": 0.6}`, which falls back to the fallback eye path.
- No look lock and no teleport.

### 6.10 Fly-out: the one cinematic

`{"flyout": "mother", "path": [...], "look": [...], "t": 13, "fov": [70, 50], "open": 1.5}`

- **Camera:** a pooled `ACameraActor` (FlyCam), hidden and not destroyed between uses.
- **Path:** Catmull-Rom through the path places. Position moves by smootherstep along the arc length.
- **Look target:** a second Catmull-Rom through the look places, on the same eased parameter.
- **Rotation:** rebuilt every frame as `FRotationMatrix::MakeFromX(Look(t) − Pos(t)).Rotator()` with roll forced to 0. Angles are never interpolated, so it cannot wrap or roll.
- **FOV:** 70 → 50 (smoothstep), a slow push-in while pulling away.
- **Turn rate:** 64 samples are precomputed at start. If the turn rate exceeds 12°/s, the parameter is retimed and a warning logged.
- **Start** (eyelids or veil closed):
  1. `SetViewTargetWithBlend(FlyCam, 0)`
  2. camera cut
  3. pawn `SetActorHiddenInGame(true)`
  4. Fly lease
  5. black opens over `open`
- **End:** if not already closed, the eyelids close over 2.0 s. Then: view target to the pawn, camera cut, pawn shown, `HardReset`.
- **Terrain:** checked offline in the world builder (§10, build_chapter1_world.py). There are no runtime traces.
- **Streaming:** if base_map uses World Partition, a `UWorldPartitionStreamingSourceComponent` sits on FlyCam during the shot. NPC and far-terrain loading happens in the slip's black.

### 6.11 Inspect

`{"inspect": id, "front": tex, "back": tex, "text": "...", "back_text": "...", "need_back": bool, "mesh": optional}`

- **The object:** one reused owner-only `UStaticMeshComponent`. It is a 15×10 cm card with front and back material instances, or a given mesh such as the compass.
  - attached to FirstPersonCamera as a first-person primitive
  - held 40 cm out and 18° below the eye line
  - rises in 0.3 s
- **The mouse:** `ProcessViewRotation` zeroes the view delta and turns the object with it (×0.35; pitch ±60°, yaw free). The world view does not move. The Inspect lease shapes speed to 0.
- **Captions:** front `text` in the lower third. Turning past 90° shows `back_text`.
- **Closing:** E, right mouse or Esc. Left mouse does not close it.
  - With `need_back`, it cannot close until the back has faced the camera (dot below -0.3 for 0.5 s).
- **Background:**
  - Default (cheap): the M_PP_Body weight darkens it by 25% and desaturates it by 20%.
  - At quality High or above: cinematic DOF on top, focus = measured item distance, f/2.8, blended over 0.4 s. Expect +0.3-0.8 ms GPU, during inspect only.
  - If DOF blurs the first-person item, keep only the darken (§12).
- **Attention zoom** of -3° while inspecting.

### 6.12 Hands (P1)

`{"hands": "compass|photo|bottle|khata|dolma"|false}`

- Raises FirstPersonMesh hands to view-space IK targets (§9) and holds a Hands lease.
- If the IK or the prop is missing, the prop alone is shown, or a caption. Nothing breaks.

### 6.13 Lens

| Action | Effect |
|---|---|
| `{"fov": x, "t": s, "keep": bool}` | story FOV term, clamped to ±3°, eased over at least 1.5 s; never oscillating |
| `{"attention": -3, "t": 4}` | applied only while the player is still (below 20 cm/s and turning under 5°/s for 1.2 s); released over 1.2 s on any input. The player causes it, so it is not a grab. |
| `{"fovbeat": 2.5, "bpm": 96, "t": 7}` | heartbeat push: -2.5° base plus -0.6° per beat, lub-dub 60/40; 1.5 s in, `t` hold, 2 s out; cap 3°. Validator: once per chapter (the plume). Off with "Heartbeat zoom: Off". |
| `{"exposure": ev, "t": s, "hold": s}` | exposure overshoot through the rig channel |
| `{"lens": {"dof": place, "fstop": 4}, "t": 0.4}` / `{"lens": null}` | optional DOF (P2 at the stone) |
| `{"breath": 0.4, "t": 6}` | Breath lease: forces B (the breath catches) |

### 6.14 Captions

`caption` draws lower-third text on a soft dark gradient. `"pos": "lowleft"` puts it small in the lower left, for "Day 1 / 49".

`card` maps to `caption`. A card with an `image` maps to `inspect`.

### 6.15 Lights

`{"light": "headlamp", "on": bool, "t": s}` controls the player's headlamp:
- a spot light attached to FirstPersonCamera
- outer cone 25°, attenuation 2,500 cm
- **CastShadows false** (a shadowed light would cost about 0.5-1 ms of virtual shadow maps)

`{"light": "nima_headlamp", ...}` controls a placed spot light: 15°, no shadows.

### 6.16 NPC preload

- At `Start`, `FStreamableManager::RequestAsyncLoad` loads every asset path in `Content/Story/Cast.ygs`. `DressAs` then finds the assets in memory.
- Every NPC spawn in chapter 1 happens inside closed eyelids or black (validator rule). That hides the remaining construction hitch.

---

## 7. Story data API

### 7.1 New and changed actions

| Action | Fields | Notes |
|---|---|---|
| `pose` | `lie\|sit\|kneel1\|kneel2\|crawl\|stand`; `t`; `face`; `snap`; `exit` (place; Sit → Crawl); `exit_ev`; `prop` (tag shown while posed); `after_input`; `auto` (s); `hold` (`true` \| `"keep"`) | Replaces `kneel`. `hold` takes a Move lease so move input does not stand you up. |
| `hold` | `true` \| `"keep"` \| `"end"` \| `false` | Move lease. Replaces `lock`. |
| `lookhold` | `true` \| `false` | Look lease. Only valid in beats with `fall` or `flyout` (validator). |
| `nudge` | place; `max` 45; `rate` 20; `pitchmax` 8; `up` cm | budgeted |
| `reflex` | place \| person; `deg` 10 | |
| `lure` | place; `sound`; `vol` | |
| `blink` | `blink\|closed\|open\|wake\|sleep`; `t`; `hold`; `during` [actions] | replaces `fade` in the present |
| `fall` | seconds; `side` | |
| `knock` | `side` | |
| `flyout` | id; `path` []; `look` []; `t`; `fov` [a, b]; `open` | old `rise/back/face` keys are removed |
| `fovbeat` | deg; `bpm`; `t` | once per chapter |
| `attention` | deg; `t` | |
| `fov` | deg; `t`; `keep` | clamped ±3 |
| `exposure` | EV; `t`; `hold` | |
| `breath` | B; `t` | |
| `inspect` | id; `front`; `back`; `text`; `back_text`; `need_back`; `mesh` | blocks until closed |
| `hands` | item \| `false` | P1 |
| `pace` | m/s \| `false`; `t`; `keep` | walk cap, as a lease |
| `light` | name; `on`; `t` | |
| `caption` | text; `t`; `pos` | |
| `npc_mode … "present"` | `dist` | |
| `npc_face` | id; `to` | |
| `follow … "talk_side"` | cm | |
| `holdkey` | key; `t`; `prompt` | waits until the key is held for `t` s (khata) |
| `preload` | `"cast"` | |
| `compass` | `{"tremble": 1, "toward": place, "t": s}` | CompassComponent story target (P1) |
| `teleport` | place; `pitch`; `cut` | gated (§6.7) |

### 7.2 New `when` fields

| Field | Meaning |
|---|---|
| `dwell` | seconds the look must hold (0.35) |
| `seen` | true = inner-60%-of-frame test |
| `lure` | `{sound, at, vol}` |
| `lure_at` | lure time (7) |
| `nudge` | true \| `{pitchmax}` |
| `companion` | NPC id |
| `fire` | fire-anyway time (15) |
| `press` | input name, for example `"Interact"`, plus `prompt` |
| `posture` | required posture |
| `any` | list of sub-condition objects, each evaluated by the same `Conditions` code; true if any holds |
| `idle` (on hints) | show only after N s without input |

### 7.3 Removed keys

The C++ parser logs `Story: removed key <k> in beat <id>` as an error and remaps to the safe equivalent. The Python validator refuses to write a `.ygs` that contains them.

| Old | New |
|---|---|
| `look` (action) | `lure` (never a pull) |
| `kneel` ≥ 90 | `pose sit` |
| `kneel` 50-89 | `kneel2` |
| `kneel` 1-49 | `kneel1` |
| `kneel` 0 | `stand` |
| `tumble` ≤ 0.5 s | `knock` |
| `tumble` > 0.5 s | `fall` |
| `fov` > 3 | clamped to 3 |
| `lock` | `hold` (+ `lookhold` only where legal) |
| `card` | `caption`, or `inspect` if it has an image |
| `teleport` with the eyes open | wrapped in a blink |

### 7.4 Validator (`validate(beats)` in chapter1_story.py; run before writing Chapter1.ygs)

- Nudges ≤ 3; `fovbeat` ≤ 1; no removed keys.
- Every `teleport` sits inside a `blink` `during` list, after `blink closed`, or under a `fade 1`, or carries `cut` in the start beat.
- Every `npc` spawn sits inside black or closed eyelids.
- Look `lure_at` ≤ 8, `fire` ≤ 16.
- `lookhold` only in beats with `fall` or `flyout`.
- `hold` without `"end"` must be released in the same beat or by `false` in a later beat.
- `"end"` only in the last beat.
- Prints `validate: OK` or a list of errors, and exits non-zero on error.

---

## 8. Chapter 1, moment by moment (shot cards)

Each card gives: frame, entry, posture, look, guidance, lens, body, exit, then the script.

### Budget for the whole chapter

| Item | Count |
|---|---|
| Nudges | at most 3, fallback only (father, flags, plume) |
| Reflexes | 4 (Dolma at the door, porters, Nima at the boom, Nima's return) |
| Teleports | 3, all behind eyelids or black (prologue, dawn, night) |
| Takeovers | Fall; Knock (risky porter choice only) |
| Fly-outs | 1 |
| Look locks | slip and fly-out only |
| Postures | lie → sit, crawl → stand, kneel1 (stream; risky porters), kneel2 (stone), sit (night) |
| FOV pushes | 1 heartbeat (plume) + attention zooms caused by the player |

Old-to-new in the chapter: 9 gaze grabs → at most 3 nudges; 4 teleports (one a hard cut) → 3 behind eyelids; 2 tumbles → Fall + Knock; 3 fake kneels → real postures; 6 cards → captions and 4 inspects.

### 1. Prologue: the Balcony, 8,400 m, night (0-3 min)

- **Frame:**
  - `balcony_start`, rotation staged by the world builder: father_sit within ±8° of centre, 25-35 m ahead along the rope, 4-6 m up; pitch +4
  - rope entering lower left, snow wall right, stars
  - the headlamp is the key light; the father is a dim silhouette until the beam finds him
- **Entry:** eyelids closed at t 0 → teleport with cut → 2 s of black with wind and a fast heartbeat → `wake` (2.8 s). Control at about 5 s.
- **Body:**
  - stand; breathing floor B = 0.6 → panting at about 0.63 Hz, ±0.33°
  - pace 0.5 m/s → rest-step dip-and-hold bob
  - mask frost as now
  - shiver mostly in the hands
- **Look:** free, standing limits.
- **Guidance:**
  - `father_seen`: near the father (15 m) + look, cone 12, dwell 0.35
  - lure Whisper_02 at 7 s, nudge #1 at 12 s, fires at 15 s
  - on fire: the father turns his head, attention -3° if the player holds still, the heartbeat sound continues; no FOV kick

```python
beat("prologue", {"start": True},
     {"blink": "closed", "t": 0}, {"preload": "cast"},
     {"cmd": ["ygg.Realm.Force none", "ygg.Survival.Reset", "ygg.Survival.Bottle 900"]},
     {"time": 230}, {"timehold": True, "rate": 0}, {"hide": "worldtree_flags"},
     {"teleport": "balcony_start", "cut": True},
     {"npc": "father", "look": "father_ai", "at": "father_sit", "tint": [0.32, 0.05, 0.04], "mode": "sit", "sink": 4, "face": False},
     {"light": "headlamp", "on": True}, {"pace": 0.5, "keep": True},
     {"sound": snd("Weather/Wind_Blizzard"), "vol": 0.75, "fadein": 3, "id": "wind"},
     {"sound": snd("Story/Story_Heartbeat"), "vol": 0.55, "id": "heart"},
     {"wait": 2.0}, {"blink": "wake", "t": 2.8},
     {"caption": "Hold [Clip]", "t": 5, "wait": False})
beat("father_seen", {"after": "prologue", "near": "father", "r": 1500, "look": "father", "cone": 12, "dwell": 0.35,
                     "lure": {"sound": snd("Mind/Whisper_02"), "at": "father", "vol": 0.9}, "nudge": True},
     {"npc_face": "father", "to": "player"}, {"attention": -3, "t": 4},
     {"wait": 1.5}, {"say": "You came too early.", "who": "Father", "t": 4})
```

Deleted: `{"lock","look"}` (L16), `{"fov": 4}` (L29), the hint "W climb".

### 2. The slip (1.5 s + 1 s black)

- 2.5 s after the line, the ice cracks 0.3 s before any motion (sound before sight).
- `fall` 1.5 s toward the drop, with slow motion in the middle (§6.8).
- The heartbeat cuts out. 1.0 s of black with the avalanche and the wind.

```python
beat("slip", {"after": "father_seen", "delay": 2.5},
     {"lookhold": True}, {"hold": True},
     {"sound": snd("Ice/Ice_Crack_03"), "vol": 1.0}, {"wait": 0.3},
     {"fall": 1.5, "side": "left"},                      # side = toward the drop at balcony_start (world builder writes it)
     {"sound": snd("Ice/Avalanche_01"), "vol": 0.6}, {"stopsound": "heart", "fadeout": 0.5},
     {"wait": 1.0},
     {"flyout": "mother", "path": ["fly_0", "fly_1", "fly_2", "fly_3"], "look": ["slip_spot", "ridge_mid", "summit"],
      "t": 13, "fov": [70, 50], "open": 1.5, "wait": False},
     {"wait": 6.5},
     {"title": "CHOMOLUNGMA", "sub": "Goddess Mother of the World", "t": 4.5},
     {"blink": "closed", "t": 2.0}, {"stopsound": "wind", "fadeout": 2.5},
     {"npc_remove": "father"}, {"light": "headlamp", "on": False}, {"pace": False})
```

### 3. Fly-out and title (13 s)

- Opens from black at **fly_0**: 4 m above `slip_spot` and 3 m out over the drop, looking down at the broken snow, the rope and the father still sitting at the top edge. The pawn is hidden.
- Rises and pulls away:
  - **fly_1**: 90 m out along the ridge away from the summit, +40 m
  - **fly_2**: 420 m out, +160 m, through the cloud top
  - **fly_3**: about 1.2 km out, level with the summit; the moonlit pyramid and its plume
- The look target slides from `slip_spot` to `ridge_mid` to `summit`. FOV 70 → 50 push-in. Turns at 12°/s or less.
- The title fades in over the moving image at 6.5 s and holds 4.5 s. The eyelids close over 2 s. The father is removed in the black.

### 4. Dawn: waking in the tent

- **In black:**
  - survival reset, time 620
  - **all day-one NPCs spawn now** (Nima, Marek, Elena, Ji-woo, Dolma, Aunt Pema; the old `team` beat L98-102 and the Dolma/aunt spawns L70-71 move here), so the DressAs stall lands in the black
  - teleport `tent_start` with a cut
  - `pose lie` snap, facing `tent_door`, sleeping-bag prop
- **Frame:**
  - lying, eye 32 cm, head on the stuff sack
  - the door glows 30-40° left, light through the fabric, the flap moving
  - frosted inner wall right; the duffel past your feet at lower right
- **Wake** over 2.8 s with +0.5 EV. The cook and the stove are heard from the door side. B = 0, so there is no camera motion: stillness is the scene.
- **Sit-up:** on the first input or after 4 s, 2.2 s to cross-legged (eye 78). Limits: yaw ±110 around the door, pitch -60/+55.
- "Day 1 / 49 · Dingboche, 4,410 m" small in the lower left.
- No lock, no hint, unless 20 s pass without input.

```python
beat("dawn", {"after": "slip"},
     {"cmd": ["ygg.Survival.Reset"]}, {"show": "worldtree_flags"}, {"time": 620}, {"timehold": True, "rate": 0.2},
     {"npc": "nima", "look": "nima", "at": "nima_spot", "tint": [0.12, 0.14, 0.17]},
     {"npc": "marek", "look": "marek", "at": "marek_spot", "tint": [0.2, 0.22, 0.08]},
     {"npc": "elena", "look": "elena", "at": "elena_spot", "tint": [0.55, 0.14, 0.05]},
     {"npc": "jiwoo", "look": "jiwoo", "at": "jiwoo_spot", "tint": [0.05, 0.18, 0.3]},
     {"npc": "dolma", "look": "dolma_ai", "at": "dolma_spot", "tint": [0.55, 0.12, 0.08]},
     {"npc": "aunt", "look": "pema", "at": "aunt_spot", "tint": [0.3, 0.06, 0.12]},
     {"teleport": "tent_start", "cut": True},
     {"pose": "lie", "snap": True, "face": "tent_door", "prop": "sleeping_bag"},
     {"sound": snd("Weather/Wind_Calm"), "vol": 0.35, "fadein": 5, "id": "wind"},
     {"sound": snd("Story/Story_Stove_Loop"), "at": "camp_kitchen", "vol": 0.45, "fadein": 3, "id": "stove"},
     {"caption": "Day 1 / 49 · Dingboche, 4,410 m", "pos": "lowleft", "t": 6, "wait": False},
     {"wait": 2.0}, {"blink": "wake", "t": 2.8},
     {"pose": "sit", "t": 2.2, "after_input": True, "auto": 4, "face": "tent_door", "exit": "tent_out", "exit_ev": 0.4, "prop": "sleeping_bag"},
     {"hint": "Look around.", "idle": 20})
```

### 5. The duffel (inspect)

- The duffel is already in the sitting frame, lower right at about -30° and 50-70 cm away.
- When looking at it within 1.6 m, the prompt "[E] Open" appears.
- At 7 s the lure: the door flap and wind (a sound picked by ear) at the duffel, and the strip of sun across it.
- No nudge, and no fire-anyway: the duffel is optional (the `read_photo` flag feeds the page).

```python
beat("duffel", {"after": "dawn", "posture": "sit", "near": "tent_duffel", "r": 160, "look": "tent_duffel", "cone": 18,
                "press": "Interact", "prompt": "[E]  Open", "lure": {"sound": "<tent flap, by ear>", "at": "tent_duffel"}},
     {"caption": "His red down jacket, patched at the elbow. His prayer beads. His summit certificate, 2019.", "t": 6},
     {"inspect": "photo_basecamp", "front": "/Game/Story/Photos/T_Photo_BaseCamp.T_Photo_BaseCamp",
      "text": "Father and Nima at Base Camp, 2019. The mountain behind them, the whole Khumbu below."},
     {"inspect": "photo_icefall", "front": "/Game/Story/Photos/T_Photo_Icefall.T_Photo_Icefall",
      "back_text": "Next time with Sangye. Not before he is ready.", "need_back": True},
     {"flag": "read_photo"})
```

### 6. Dolma calls: crawling out (played; no fade, no teleport)

- Dolma's voice comes from outside at `dolma_spot`. Reflex 10° toward the door if it is 30-120° off.
- W held for 0.3 s → Crawl (0.8 s, eye 68), at 40 cm/s along the door line.
  - The light from the door grows. A 0.8 cm hand-plant dip every 40 cm.
- At `tent_out` (60 cm radius) → stand up over 1.4 s to 156 cm, with a +0.4 EV dawn overshoot over 2 s. First frozen crunch, breath fog. **Standing up is the reveal of the camp.**
- Hint "[W] crawl out" only after 15 s idle. Blink fallback at 90 s.

```python
beat("dolma_calls", {"after": "dawn", "any": [{"after": "duffel", "delay": 3}, {"delay": 45}, {"posture": "crawl"}]},
     {"say": "Sangye-dai! Sangye-dai, they are waiting for you!", "who": "Dolma", "at": "dolma_spot", "t": 4},
     {"reflex": "tent_door", "deg": 10},
     {"hint": "[W]  crawl out", "idle": 15, "t": 6})
```

Deleted: fade, `kneel 0`, teleport `tent_out`, unlock (L73-76).

### 7. Farewell: Dolma's hand, the aunt's khata

- Dolma runs from `dolma_spot`, which is 6-8 m out within ±25° of the door's outward normal, so she is in front of you as you stand. Then `present` at 2 m.
- **Hold F:**
  - hands `dolma` (P1): your left arm reaches down-left to her hand at about 70-80 cm
  - hand pull ≤ 8°/s
  - pace 0.8 m/s
  - at `nima_spot` she stops and will not let go
- Aunt Pema `present` at 0.9 m: she arrives centred. The choice uses keys 1-3, and eat/drink are suppressed while it is open.
- **Khata:** `wear khata` attaches the khata to the chest socket of FeetBody. You look down yourself (about -55°) and see white on the red jacket. If the mesh is missing, a caption stands in.
- Dolma's drawing is an `inspect` (caption fallback if there is no texture).

```python
beat("farewell", {"after": "dolma_calls", "posture": "stand", "near": "dolma", "r": 600},
     {"stopsound": "stove", "fadeout": 3},
     {"npc_mode": "dolma", "mode": "present", "dist": 200},
     {"say": "Hold tight, Sangye-dai.", "who": "Dolma", "t": 3},
     {"hand": "dolma", "to": "nima_spot", "hint": "Hold  [F]  to hold her hand", "pull": 8, "pace": 0.8},
     {"npc_mode": "aunt", "mode": "present", "dist": 90}, {"wait": 1.5},
     {"say": "Promise me you will come down.", "who": "Aunt Pema", "t": 3.5},
     {"choice": [...unchanged...]},  {"say": ...unchanged...},
     {"wear": "khata", "on": True},
     {"inspect": "dolma_drawing", "front": "<drawing texture or omit>", "text": "A folded drawing: the mountain, a big figure, and a small one holding its hand."},
     {"say": "Dolma! Come, let him go.", "who": "Aunt Pema", "t": 3},
     {"npc_mode": "dolma", "mode": "goto", "to": "village_way"}, {"npc_mode": "aunt", "mode": "goto", "to": "village_way"})
```

Deleted: `look aunt` (L85). The `card` (L92) becomes an inspect.

### 8. The job: team and compass

- Nima `present` at 1.5 m. Marek, Elena and Ji-woo stand in a loose arc 4-6 m ahead (world-builder staging).
- After the load choice: pack stoop 3 cm, landing dip ×1.0.
- **Compass:** `inspect` of the brass compass from Nima's hands ("He never went anywhere without it."), then the hint "[Q] hold up the compass". Q holds it in the lower third at 40 cm while you walk at 75% speed.
- Nima follows with `talk_side`, so his face is a 30° glance away while he speaks.

```python
beat("job", {"after": "farewell", "delay": 2},
     {"npc_mode": "nima", "mode": "present", "dist": 150},
     ...unchanged lines and choice...,
     {"say": "Here. He never went anywhere without it.", "who": "Nima"},
     {"inspect": "compass", "mesh": "<father compass mesh>", "text": "Father's brass compass. The needle still points north."},
     {"hint": "[Q]  hold up the compass", "t": 5},
     {"npc_mode": "nima", "mode": "follow", "behind": 260, "side": 150, "talk_side": 110}, ...)
```

Deleted: `look nima` (L105); the compass `card` (L115).

### 9. The trail

- **Climb:** the breathing leads. On the steep step B rises and the panting starts at about 0.55 Hz. Nima's line comes at the effort peak (optional `when` `effort ≥ 0.6`, P1). Slowing into the rest-step changes the bob to dip-and-hold.
- **Stream and fill:**
  - the stream is heard first
  - at the hole, "[E] Fill your bottle" → `kneel1` (0.9 s, eye 108, yaw ±85 around the hole, pitch -50/+50) with `hold`
  - hands `bottle` (P1); the player looks down at the water themselves
  - after 2.2 s the hold is released; W stands you up (1.0 s)
- **Elena:** she stops at a bend 10 m ahead, within ±20° of the trail direction (staging). Look trigger cone 14, lure = her laboured breathing at 7 s, fires at 15 s. No nudge. The choice appears while you walk.
- **Why:** walk-and-talk, Nima at your right shoulder. No camera change.
- **Porters:**
  - bells and whistles from `porter_start`, which has no line of sight from the trail until 25 m (staging assert)
  - reflex 10° at the first close whistle; the choice times out after 7 s
  - safe: you step aside yourself
  - risky: `knock` toward the valley → `kneel1` for 1.5 s with heavy breathing → stand (1.0 s); health 78; pace 0.8 m/s for 60 s (hurt knee)
  - porter-player collision off during the pass
  - **the teleport to porter_end is deleted** (a 326 m hard cut)
- **Alone in the cloud:**
  - Q raises the compass
  - story FOV -2° over 20 s (`keep`): a quiet closing-in
  - the exposure channel keeps the whiteout from going grey
  - no nudge; breath and steps louder
- **Together:** Nima's voice from behind first, then reflex 10° toward him. FOV back to 0 over 4 s.

```python
beat("fill", {"after": "stream", "near": "stream_hole", "r": 200, "look": "stream_hole", "cone": 25,
              "press": "Interact", "prompt": "[E]  Fill your bottle"},
     {"pose": "kneel1", "t": 0.9, "face": "stream_hole", "hold": True}, {"hands": "bottle"},
     {"cmd": "ygg.Survival.Set Hydration 100"},
     {"caption": "The water is so cold it hurts your teeth. Bottle full.", "t": 4},
     {"wait": 2.2}, {"hands": False}, {"hold": False}, {"hint": "[W]  stand up", "idle": 6},
     {"stopsound": "stream", "fadeout": 6})
beat("elena", {"after": "job", "near": "ev_elena", "r": 1300},
     {"npc_mode": "elena", "mode": "goto", "to": "ev_elena", "speed": 0.6},
     {"lure": "elena", "sound": "<laboured breath, by ear>", "vol": 0.8}, ...)   # look elena (L141) deleted
beat("porters_pass", {"after": "porters_come", "near": "porter1", "r": 700},
     {"reflex": "porter1", "deg": 10},
     {"choice": [...], "timeout": 7, "default": 0},
     {"cmd": "ygg.Survival.Set Health 78", "if": "risky"},
     {"sound": snd("Footsteps/Step_Rubble_04"), "vol": 1.0, "if": "risky"},
     {"knock": "valley", "if": "risky"}, {"pose": "kneel1", "t": 0.6, "hold": True, "if": "risky"},
     {"wait": 1.5, "if": "risky"}, {"hold": False, "if": "risky"}, {"pose": "stand", "t": 1.0, "if": "risky"},
     {"pace": 0.8, "t": 60, "keep": True, "if": "risky"}, ...unchanged Nima lines...)
beat("alone", {...unchanged...}, ..., {"fov": -2, "t": 20, "keep": True})
beat("together", {...unchanged...}, {"reflex": "nima", "deg": 10}, {"fov": 0, "t": 4}, ...)
```

### 10. Dusk at the memorials (the peak, part 1)

- The flags loop starts at radius 2,600: heard before seen.
- `ev_flags_seen` sits at a trail turn where the backlit flags lie within ±15° straight ahead (staging).
- Look trigger `memorial`, cone 14: lure = a gust snapping the flags at 7 s, **nudge #2** at 12 s (≤ 15°/s, ≤ 45°, pitch ≤ 8°). Once seen, nothing more: the image does the work.
- Memorial: Nima walks ahead to `kneel_spot` and turns toward the stone (`npc_face`). "This way."

```python
beat("dusk_comes", {"after": "job", "near": "ev_flags_seen", "r": 2600},
     {"timeto": 1735, "t": 35},
     {"sound": snd("Story/Story_Flags_Loop"), "at": "memorial", "vol": 1.0, "fadein": 6, "id": "flags"})
beat("flags_seen", {"after": "dusk_comes", "look": "memorial", "cone": 14, "dwell": 0.35,
                    "lure": {"sound": "<flag snap gust, by ear>", "at": "memorial"}, "nudge": True},
     {"attention": -3, "t": 5})
beat("memorial", {"after": "job", "near": "memorial", "r": 1500},
     {"npc_mode": "nima", "mode": "goto", "to": "kneel_spot", "speed": 0.8},
     {"say": "This way.", "who": "Nima"}, {"npc_face": "nima", "to": "father_stone"})
```

Deleted: `look memorial` (L189) and `look father_stone` (L195).

### 11. The father's stone (played kneel)

- Near the stone (2 m) and looking at it: "[E] Kneel".
- The body turns to the stone over 0.6 s and kneels over 1.0 s (eye 110). Yaw ±80 around the stone, pitch -45/+65. **Look stays free.** A move lease is kept until the plume ends.
- **Khata:** hands `khata`; "Hold [F] to tie the khata" (2 s). `khata_on_stone` appears and the khata leaves your chest.
- Then the ridge wind rises. 2.5 s later hands `compass`: the needle trembles and points up the ridge, a lure held in your own hand.
- **Staging assert:** from the kneeling eye facing the stone, `plume_view` sits at yaw +25..35° and pitch +12..18°. The stone is in the lower-left third, the plume in the upper-right third.

```python
beat("stone", {"after": "memorial", "near": "father_stone", "r": 200, "look": "father_stone", "cone": 20,
               "press": "Interact", "prompt": "[E]  Kneel"},
     {"pose": "kneel2", "t": 1.0, "face": "father_stone", "hold": "keep"},
     {"hands": "khata"}, {"holdkey": "F", "t": 2, "prompt": "Hold  [F]  to tie the khata"},
     {"show": "khata_on_stone"}, {"wear": "khata", "on": False}, {"hands": False},
     {"say": "You tie your khata to his name.", "who": ""},
     {"sound": "<ridge wind rise, by ear>", "id": "ridge", "fadein": 3}, {"wait": 2.5},
     {"hands": "compass"}, {"compass": {"tremble": 1, "toward": "summit", "t": 20}})
```

### 12. The gold plume (the peak, part 2)

- Look trigger `plume_view`, cone 16: lure = the drone swelling at low volume at 7 s; **nudge #3** at 12 s (pitch up to 12° total at 8°/s, yaw ≤ 20°/s); fires at 15 s anyway. The 11 s glow waits for the player.
- **On seen:**
  - Glow pulse 11 s, drone, heartbeat sound
  - `fovbeat` 2.5° at 96 bpm for 7 s (the only one in the chapter)
  - attention -3° if still
  - exposure +0.3 EV
  - breath forced to 0.4 (the breath catches)
- Boom → reflex 10° to Nima on your left → "Did you hear that?" / "No. Only the wind."
- Hold released; W stands you up (1.2 s).

```python
beat("plume", {"after": "stone", "look": "plume_view", "cone": 16, "dwell": 0.35,
               "lure": {"sound": snd("Story/Story_Plume_Drone"), "vol": 0.35}, "nudge": {"pitchmax": 12}},
     {"param": "Glow", "tag": "plume", "to": 1.0, "t": 11, "pulse": True},
     {"sound": snd("Story/Story_Plume_Drone"), "vol": 0.85},
     {"sound": snd("Story/Story_Heartbeat"), "vol": 0.6, "id": "heart"},
     {"fovbeat": 2.5, "bpm": 96, "t": 7, "wait": False}, {"attention": -3, "t": 4},
     {"exposure": 0.3, "t": 1.5, "hold": 6}, {"breath": 0.4, "t": 6},
     {"wait": 5.5}, {"sound": snd("Ice/Mountain_Boom_02"), "vol": 0.7}, {"wait": 2.0},
     {"reflex": "nima", "deg": 10}, {"say": "Did you hear that?", "who": "Nima"},
     {"stopsound": "heart", "fadeout": 2}, {"stopsound": "ridge", "fadeout": 4},
     {"say": "No. Only the wind. Come, it is getting dark.", "who": "Nima"},
     {"hands": False}, {"compass": {"tremble": 0}}, {"hold": False}, {"hint": "[W]  stand", "idle": 8},
     {"npc_mode": "nima", "mode": "goto", "to": "night_nima", "speed": 0.9},
     {"hint": "Follow Nima down to the tents.", "t": 6})
```

Deleted: the 35 s full lock, `look father_stone` at strength 1.0, `look plume_view` (3.2 s wait), `fov +4` for 7 s, `look night_nima`, `kneel 70` / `kneel 0`.

### 13. Night with Nima (the end)

- At `night_tent` (r 450): a blink (close 0.15 s, hold 1.2 s, open 0.6 s). During the blink:
  - time 2040
  - teleport `night_tent_sit`
  - `pose sit` snap, facing Nima, sleeping-bag prop
  - Nima sat at `night_nima`, 1.0 m opposite, slightly left of centre
- **Light:** only Nima's headlamp (spot 15°, no shadows) hanging from a tent loop, aimed down between you. Stove glow lower right. Your own headlamp is off.
- **Look:** sit limits (yaw ±110, pitch -60/+55). `hold: "end"`. No look lock. His lines come with no pull: he is in front of you. Attention -3° if still.
- After "Your father did not fall." comes 1 s of silence, then Nima clicks his headlamp off (0.1 s): darkness, not a fade. Then 2 s of dark with the stove outside, then `sleep` 2.5 s (one flutter) → page → title → end.

```python
beat("night", {"after": "plume", "near": "night_tent", "r": 450},
     {"blink": "blink", "t": 0.15, "hold": 1.2, "during": [
         {"time": 2040}, {"teleport": "night_tent_sit"},
         {"pose": "sit", "snap": True, "face": "night_nima", "prop": "sleeping_bag"},
         {"npc_mode": "nima", "mode": "sit", "at": "night_nima", "face": True},
         {"light": "nima_headlamp", "on": True}]},
     {"hold": "end"},
     {"stopsound": "flags", "fadeout": 4}, {"stopsound": "wind2", "fadeout": 6},
     {"sound": snd("Story/Story_Stove_Loop"), "at": "night_stove", "vol": 0.55, "fadein": 3, "id": "stove"},
     {"wait": 3}, ...radio lines unchanged..., {"wait": 4},
     {"attention": -3, "t": 4, "wait": False}, {"wait": 2},
     {"say": "People say he fell.", "who": "Nima", "t": 3.5}, {"wait": 2.5},
     {"say": "Your father did not fall.", "who": "Nima", "t": 4}, {"wait": 1.0},
     {"light": "nima_headlamp", "on": False, "t": 0.1}, {"wait": 2.0},
     {"blink": "sleep", "t": 2.5}, {"stopsound": "stove", "fadeout": 3}, {"wait": 1.5},
     {"day": 1}, {"page": [...unchanged...], "head": "Day {day} / 49"},
     {"title": "CHOMOLUNGMA", "sub": "Chapter 1  ·  Day One", "t": 7}, ... {"end": True})
```

Deleted: `{"lock": True, "look": False}`, `look nima` (L236), `fade 3.5` before the page (now the sleep blink).

---

## 9. Tunables (starting values)

The rig values are `UPROPERTY(Config)` in `DefaultGame.ini` under `[/Script/YggdrasilsPeak.YggViewRigComponent]`, so they can be tuned without a rebuild. The live cvars are `ygg.View.*`. Story defaults live in the action JSON; story cvars are `ygg.Story.*`.

### Eye, terrain, neck

| Name | Value |
|---|---|
| StandEyeCm | 156 |
| SinkToEye | 1.0 (counted once) |
| FootDropToEye | 0.5, max 10 cm; smoothing τ down 0.5 s / up 1.2 s |
| TerrainSpringHz | 1.8 (critically damped); TerrainMaxLagCm 15 |
| NeckFwdCm / NeckUpCm / NeckScale | 8 / 7.5 / 0.8 |
| WaistLeanFromDeg / FullDeg | -45 / -75 |
| WaistLeanFwdCm / DownCm | 8 / 3.5 |
| EyeHorizMaxCm | 24 |
| EyeFloorMinCm | 20 (takeover 30) |
| EyeTopMarginCm | 5 |
| StandWatchdog | eye < StandEye − 36 at speed > 60 cm/s for 0.5 s |
| PCM pitch backstop | -78 / +82 |
| AbsolutePitchClamp | -80 / +82 |
| RollMaxDeg | 2 (Fall 30) |

### Postures

| Posture | Eye (band) | Down / up s | Yaw ± | Pitch |
|---|---|---|---|---|
| stand | 156 (150-160) | — | free | -75/+80 |
| lie | 32 (26-38) | 2.5 / 2.2 | 70 | -20/+70 |
| sit | 78 (72-84) | 2.0 / 1.6 | 110 | -60/+55 |
| kneel1 | 108 (102-114) | 0.9 / 1.0 | 85 | -50/+50 |
| kneel2 | 110 (104-116) | 1.0 / 1.2 | 80 | -45/+65 |
| crawl | 68 (60-76) | 0.8 / 1.4 | 45 | -40/+25 |

| Name | Value |
|---|---|
| S-curve | 70% of the change at 60% of the time; settle overshoot 1.5 cm |
| Gaze bend | -6°, view-only |
| PoseEnterMaxSpeed | 20 cm/s |
| BodyTurnToFace | 0.6 s |
| StandOnMoveHold | 0.3 s |
| WalkResumeAt | 85% of stand eye |
| StandBodyTurn | 360°/s, ≤ 0.5 s |
| Limit tween | 1.0 s |
| Soft edge | 12° |
| Sit shuffle | after 0.6 s, 60°/s |
| CrawlSpeed | 40 cm/s |
| CrawlDip | 0.8 cm per 40 cm |
| CrawlExitRadius | 60 cm |
| CrawlFallback | 90 s |
| Crawl hint | idle 15 s |

### Bob

| Name | Value |
|---|---|
| Vertical p-p by speed | <0.45 m/s 1.2 (dip-and-hold), 0.6 1.5, 1.1 2.0, 1.6 2.6, 2.3 3.0, 3.0 3.6, 5.0 4.5 cm |
| Lateral p-p | 0.5, 0.8, 1.0, 1.3, 1.5, 1.8, 2.2 cm |
| Roll | 0, 0, 0.15, 0.2, 0.2, 0.25, 0.3° |
| Trough phase | 0.07 |
| Period clamp | 0.3-1.1 s |
| Min step gap | 0.2 s |
| Fallback | stride 130 cm after 0.8 s without a step at > 30 cm/s; decay half-life 0.25 s after 1.2 T |
| Rest-step | below 45 cm/s: dip 1.2 cm / 0.12 s, hold 0.25 s, rise 0.35 s |
| Amplitude jitter | ±15% × (1 + 0.75·fatigue) |
| Downhill | dip ×1.3, nod 0.4°/step |
| Rock micro-dip | 0.5-1.0 cm |
| Fixation | 1000 cm |
| Start / stop nod | 0.4°, ω 9, ζ 0.7 |
| HeadMotion | 1 / 0.4 (no roll, no breathing) / 0 (landing ×0.3) |

### Breathing and body

| Name | Value |
|---|---|
| AltGain | 0.6 + 0.4·clamp((alt − 3000) / 5400) |
| BFloor | 0.6·clamp((alt − 7000) / 1400) |
| B rise τ | 5 s |
| B fall τ | 25 s × (0.5 + gain) |
| Threshold | 0.15 (below: ±0.06°, ±0.2 cm at 0.25 Hz) |
| Pant frequency | 0.45 + 0.3B Hz |
| Pant amplitude | ±0.55B° pitch, ±0.5B cm Z, roll 0 |
| Exhausted hang | -3°, -3 cm, τ 4 s; noise 0.35° at 0.6-0.9 Hz |
| Shiver | 0.12° at 5-8 Hz, cap 0.25 × fps; hands ×3 |
| Heart FOV | 0 below 7,000 m; above −0.4° × beat × max(hyp, ex × 0.7) |

### Landing, slope, pack, turning

| Name | Value |
|---|---|
| Landing | threshold 150 cm/s; 1.5 cm per 100 cm/s; cap 10; down 0.1 s; ω 14, ζ 0.65; ×1.3 above 5,000 m; nod 0.25°/cm, cap 2° |
| Slope uphill | from 8°; 6 cm forward, 4 cm down at 20°; cap 25°; τ 1.75 s |
| Slope downhill | 2.5 cm back |
| Pack | 0.25 cm/kg over 3 kg, cap 6 cm, blend 1.5 s; landing +2%/kg over 15 kg |
| Turn-lean | K 0 (off); 0.004 / max 0.8° / τ 0.15 s if enabled |
| Turn dead zone | below 10 cm/s, 45°, catch-up 240°/s (P1) |

### Input, FOV, screen, render

| Name | Value |
|---|---|
| Mouse | 1:1, smoothing off |
| Sensitivity default | 0.7× [Director] |
| HandFollowSpeed (hands up) | 13 |
| Hands rise / lower | 0.3 / 0.25 s |
| FOV | default 90, settings 80-110 |
| Story FOV clamp | ±3° (ease ≥ 1.5 s) |
| Attention | -3° over 4 s after 1.2 s still (< 20 cm/s, < 5°/s); release 1.2 s |
| FovBeat | 2.5° + 0.6°/beat, 96 bpm, lub-dub 60/40, in 1.5 s / hold 7 s / out 2 s, cap 3°, once per chapter |
| First-person FOV / scale | 70 / 0.6 |
| Vignette | 0.2 → 0.6 (hypoxia) |
| PP overrides | only above 0.01 |
| Steady exposure | -20° → -40° pitch → 0.05 EV/s |
| Overshoots | wake +0.5 EV τ 1.5; tent exit +0.4 τ 2; plume +0.3 hold 6 |
| Inspect background | darken 25%, desaturate 20%; DOF f/2.8 at item distance at quality ≥ High, blend 0.4 s |
| Frame generation | Auto = 2x; 3x/4x after base ≥ 55 fps for 10 s [Director] |
| Motion blur | Off; Low = 0.15 [Director] |
| Near clip | 10 (fallback 7) |

### Story guidance and takeovers

| Name | Value |
|---|---|
| Look trigger | cone 14 (12-20), dwell 0.35; seen = inner 60% for 1.0 s |
| Escalation | companion 3 s, lure 7 s, nudge 12 s, fire 15 s |
| Nudge | k 1.4/s; max 20°/s yaw, 8°/s pitch; total 45° yaw, 8° pitch (plume 12°); ramp 0.4 s; max 3 s; yield 30°/s (gamepad 15); cancel > 60°/s for 0.1 s; done 15° / 10°; budget 5 hard / 3 chapter 1; gap 60 s; only < 150 cm/s |
| Reflex | 10° over 0.3 s; only 30-120° off-centre; cooldown 20 s |
| Hand pull | ≤ 8°/s; pace 0.8 m/s |
| Present | default 200 cm (aunt 90, Nima 150) |
| talk_side | 110 cm, behind 30 |
| Eyelids | close 0.15 / open 0.6; wake 35% @0.6, shut @1.1, open @2.8; sleep 2.5 s with flutter 60% / 30%; soft edge 12%; auto-blink for teleports 0.15 / 0.5 |
| Fall | 1.5 s; phases 0.12 / 0.28 / 0.70 / 0.40; peaks Z -5/-35/-70/-85, pitch -4/-22/-48/-62, roll 0/12/26/30, yaw 0/8/12/15; SlipSlowMo 0.35 (1 = off); audio pitch 0.8; eye ≥ floor + 30; pitch ≥ -72; peak ≤ 120°/s |
| Knock | 0.14 s, yaw 15, roll 8, Z -20, pitch -10; spring back 0.5 s |
| Fly-out | 13 s; FOV 70 → 50; ≤ 12°/s; open 1.5 s; close 2.0 s; clearance ≥ 120 m from fly_1 on (fly_0 ≥ 3 m) |
| Inspect | 40 cm, 18° below eye line, rise 0.3, pocket 0.25, mouse ×0.35, pitch ±60, need_back dot < -0.3 for 0.5 s |
| Lease maximums | as in §4.4 |

### Hand rest positions (view space, cm: X forward, Y right, Z up)

| Hands | Position |
|---|---|
| compass (right) | (40, 8, -22) |
| photo (both) | (38, ±10, -16) |
| bottle (right) | (38, 6, -24) |
| khata (both) | (42, ±12, -30) |
| dolma (left) | (30, -25, -55), reach clamped, no stretching |

### Lights

| Light | Values |
|---|---|
| Player headlamp | spot 25°, 2,500 cm, no shadows |
| Nima's headlamp | spot 15°, no shadows |

---

## 10. Code change list

Build with the editor closed (UBT; see the memory note "C++ build needs editor closed").

### 10.0 Optional Phase 0 hotfix (about 1 hour; only if the director wants to replay the old chapter first; coordinate with the other session)

1. Where `GazeFrom` and `FlyRotFrom` are captured (≈YggStory.cpp:1375, :1414):
   ```cpp
   GazeFrom.Pitch = FRotator::NormalizeAxis(GazeFrom.Pitch);
   FlyRotFrom.Pitch = FRotator::NormalizeAxis(FlyRotFrom.Pitch);
   ```
   This kills the vertical loop.
2. Teleport handler: `C->PlayerCameraManager->SetGameCameraCutThisFrame();`
3. ExpeditionComponent.cpp:670: vignette `Lerp(0.2f, 0.6f, Hyp)`.
4. In the settings menu (no code): frame generation 2x, motion blur off.

### 10.1 New files

**`Source/YggdrasilsPeak/Survival/YggViewRig.h/.cpp`** (about 1,000 lines)

```cpp
UENUM() enum class EYggView : uint8 { Walk, Lie, Sit, Kneel1, Kneel2, Crawl };
UENUM() enum class EYggScripted : uint8 { None, Fall, Knock, Fly };
UENUM() enum class EYggLease : uint8 { Move, Look, Pace, Fall, Knock, Fly, Inspect, Hands, Attention, FovBeat, StoryFov, Nudge, Lids, Breath, Exposure, Lens };
UENUM() enum class EYggLidMode : uint8 { Blink, Closed, Open, Wake, Sleep };
USTRUCT() struct FYggPoseDef { FName Name; float EyeCm, BandCm, DownS, UpS, YawHalf, PitchMin, PitchMax; bool bHideBody; FName Prop; }; // Config array
USTRUCT() struct FYggPoseReq { EYggView Pose; float Seconds; TOptional<float> FaceYaw; bool bSnap, bAfterInput; float Auto; TOptional<FVector> Exit; float ExitEv; FName Prop; };
USTRUCT() struct FYggInspect { FName Id; UTexture2D* Front; UTexture2D* Back; FText Text, BackText; bool bNeedBack; UStaticMesh* Mesh; };

UCLASS(Config=Game) class UYggViewRigComponent : public UActorComponent {
public:
  static UYggViewRigComponent* Find(const UObject* Ctx);
  // gameplay feeds
  void SetBody(float Shiver, float Exhaust, float Beat, float Hypoxia, float PackKg);
  void OnStep(bool bLeft, float SpeedCmS, uint8 Surface);
  UFUNCTION() void OnLanded(const FHitResult& Hit);
  void SetExposureBias(FName Source, float Ev);
  void AddShake(float Deg, float Seconds);                    // MountainRumble (P1)
  float ShapeWalkSpeed(float Target) const;                   // 0 when posed/held/inspecting, crawl speed, pace cap
  // story (Owner = beat index)
  void RequestPose(int32 Owner, const FYggPoseReq& R);
  EYggView GetView() const; float GetEyeAboveFloor() const; float GetLid() const;
  int32 AcquireLease(int32 Owner, EYggLease Kind, float MaxSec, bool bKeep = false);
  void ReleaseLease(int32 Owner, EYggLease Kind); void ReleaseOwner(int32 Owner); void ReleaseAll(float Blend);
  bool Nudge(int32 Owner, const FVector& Target, float MaxYaw, float Rate, float PitchMax);  // false = budget -> caller lures
  void Reflex(const FVector& Source, float Deg);
  void SetHandPull(TWeakObjectPtr<AActor> Toward, float Rate);
  void SetLids(int32 Owner, EYggLidMode Mode, float T, float Hold, TFunction<void()> OnClosed);
  void Fall(int32 Owner, float Seconds, float Side); void Knock(int32 Owner, float Side);
  void FlyOut(int32 Owner, const TArray<FVector>& Path, const TArray<FVector>& Look, float T, FVector2D Fov, float Open);
  void SetStoryFov(int32 Owner, float Deg, float T); void Attention(int32 Owner, float Deg, float T);
  void FovBeat(int32 Owner, float Deg, float Bpm, float T); void StoryExposure(int32 Owner, float Ev, float T, float Hold);
  void StoryBreath(int32 Owner, float B, float T);
  void BeginInspect(int32 Owner, const FYggInspect& I); bool IsInspecting() const;
  void SetHands(int32 Owner, FName Item); void SetPace(int32 Owner, float Ms, float T, bool bKeep);
  void HardReset(const FRotator& Facing, EYggView Pose = EYggView::Walk);
  FString Audit() const;
  virtual void TickComponent(float Dt, ELevelTick, FActorComponentTickFunction*) override; // TG_PostPhysics
};

UCLASS() class UYggViewModifier : public UCameraModifier {
  virtual bool ProcessViewRotation(AActor* ViewTarget, float Dt, FRotator& OutViewRotation, FRotator& OutDeltaRot) override;
  virtual bool ModifyCamera(float Dt, FMinimalViewInfo& InOutPOV) override;
};
```

- Cvars:
  - `ygg.View.Rig` (0 = old socket camera)
  - `ygg.View.HeadMotion`, `.Bob`, `.Breath`, `.Neck`, `.OneBody`, `.Log`, `.Debug` (draws leases on the HUD)
  - `ygg.View.SlipSlowMo`
  - `ygg.View.Audit` (console command)
- `STAT_YggView` cycle counter.

**`Tools/UI/make_eyelid.py`** → `/Game/UI/T_Eyelid` (256×64, soft curved edge).

### 10.2 Changed files

| File | Change |
|---|---|
| `Survival/FirstPersonBodyComponent.h/.cpp` | BeginPlay: create the rig for the locally controlled pawn. Delete the head-follow code (.h:50-60, .cpp:170-175, :392-400). Add `SetOneBody(bool)` (FeetBody shows spine, arms and torso; hides neck and head; FeetBody setup .cpp:290-318). Add `SetPostureView(EYggView, float DropCm)`: hide FeetBody when posed; show FirstPersonMesh lowered by DropCm in `ApplyWorldBodyOffset` (.cpp:608-632) with FirstPersonPrimitiveType None; remove the second SinkDepth on FirstPersonMesh. FirstPersonMesh hidden unless hands or posed, with OnlyTickPoseWhenRendered while hidden. Add `SetHandsInUse(FName, FVector L, FVector R)` (P1; ViewBody without a tool, branch at .cpp:178; PoseArm; HandFollowSpeed 13; FeetBody upper arms hidden). FeetAlpha eases out while posed. Getters `GetSinkDepth()`, `GetFootDrop()`. |
| `Survival/SnowTrailComponent.h/.cpp` | `DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnYggStep, bool, float, uint8)`. Remember the foot side in `OnFootContact` (:612) and in the cadence net. Broadcast in `PlayFootstep` (:879). |
| `Survival/SurvivalComponent.cpp` | `ApplyMovementSpeed` (:486-501): `Target = Rig ? Rig->ShapeWalkSpeed(Target) : Target;`. The rig toggles `SetJumpAllowed`. |
| `Survival/ExpeditionComponent.cpp` | `UpdateScreenEffect` (:655-741): vignette 0.2 → 0.6; override flags only above 0.01. Replace the BodyCam block (:742-755) with `Rig->SetBody(...)`. Death and continue (:818, :851): `Story->ReleaseAll()` + `Rig->ReleaseAll(0)` + `HardReset`. Remove the YggBodyCameraModifier include. |
| `Survival/SanityComponent.cpp` | :199-207 gating; exposure → `Rig->SetExposureBias("sanity", v)`. |
| `Survival/YggBodyCameraModifier.h/.cpp` | **Delete.** |
| `Survival/YggSettings.cpp`, `SettingsWidget.cpp` | FOV 80-110 (:294); MotionBlur default 0 (:201); FrameGen Auto (:186); rows Head motion, Story camera, Heartbeat zoom, Comfort dot; sensitivity default [Director]. |
| `Config/DefaultInput.ini` | :66 `bEnableMouseSmoothing=False`. |
| `Config/DefaultGame.ini` | `[/Script/YggdrasilsPeak.YggViewRigComponent]` tunables and the `+Poses=(...)` table. |
| `Survival/ClimbingComponent.cpp` | cvar `ygg.Climb.Camera` (chapter 1 sets 0 at Start and restores at Stop): a grab never swaps to ClimbArm. `bUseControllerRotationYaw` save/restore (:236/:274) goes through the rig. |
| `Survival/MountainRumble.cpp` | :92-104 → `Rig->AddShake` (P1). CrevasseCrossing.cpp:120-141 is P2. |
| `Survival/CompassComponent.*` | `SetStoryTarget(FVector, float Tremble)` (P1, plume). |
| `ClimberControlsComponent.cpp` | :75-77: while a story choice is open, keys 1-3 answer it and do not eat or drink. |
| `Survival/YggSessionMonitor.cpp` | frames.csv adds `posture, eye_floor_cm, lid, story_fov, nudge, leases, cut, scripted`. report.md adds the §11 automatic checks. |
| `Story/YggStory.h/.cpp` | **Delete** everything in §3.3. **Add:** all actions of §7.1 and `when` fields of §7.2 in `DoAction` and `Conditions`/`TickTriggers`; legacy remaps with error logs; `TickRuns` → `Rig->ReleaseOwner(beat)` when a run ends; `JumpTo`/`Stop`/chapter end/`Deinitialize` → `ReleaseAll` + `HardReset`; the teleport gate and camera cut (§6.7); `AYggStoryNPC::EMode::Present`, `talk_side`, `npc_face`; `preload` with `FStreamableManager`; `UYggStoryOverlay::NativePaint` for eyelids and lower-third captions, with the card veil removed; story clocks on `DeltaRealTimeSeconds` while dilation ≠ 1; console `ygg.Story.CamTest` and `ygg.Cam.Frame <place> [pose]`. Keep the working copy's "option keys first" dispatch order. |
| `Tools/Story/chapter1_story.py` | Rewrite per §8; add `validate()`. |
| `Tools/Story/build_chapter1_world.py` | New places and asserts (below). |

**New places and asserts in `build_chapter1_world.py`** (each assert prints its measured value):
- `balcony_start` rotation faces `father_sit` within ±8°, pitch +4.
- `slip_spot` and the slip `side` (toward the drop).
- `fly_0..fly_3` and `ridge_mid`:
  - clearance ≥ 120 m against the **visual** far terrain (glb heightfield) every 50 m from fly_1 on
  - turn rate ≤ 12°/s at 13 s
- `tent_start` re-aimed: door at yaw -30..-40°; duffel at yaw +20..35°, pitch -25..-35 from the 32 cm and 78 cm eyes.
- `tent_door`; `tent_out` 60 cm out on the door axis. The line from tent_start to tent_out passes through the door gap.
- `dolma_spot` 6-8 m out within ±25° of the door normal.
- `ev_elena` at a bend (stop within ±20° of the trail, 10 m ahead).
- `porter_start` with no line of sight from the trail until 25 m.
- `ev_flags_seen` at a turn with the flags within ±15°.
- `kneel_spot`/`father_stone`: plume_view at yaw +25..35° and pitch +12..18° from the 110 cm kneeling eye.
- `night_tent_sit` 100 cm from `night_nima`, plus `nima_headlamp`.
- Sangye's tent:
  - an inner shell or two-sided material
  - 8 traces from the tent_start eye hit it within 1.5 m (assert)
  - pawn collision stays off (it already has `collide=not sangye`, L271-272)
- Sleeping-bag prop tag `sleeping_bag` at tent_start and night_tent_sit, hidden by default.

### 10.3 Order of work

**P0, day 1 to 1.5: every reported bug fixed, and the camera cannot break.**
1. Rig and modifier skeleton: camera re-parent; eye composition (terrain spring, sink, foot drop, neck and waist); footstep bob; breathing, exhaustion and shiver; landing; soft limits; invariants and watchdog.
2. ShapeWalkSpeed in SurvivalComponent; leases; HardReset; ReleaseAll.
3. Postures without clips (lie, sit, kneel1, kneel2, crawl with plane constraint) and body visibility (OneBody, posed view).
4. Eyelids; the teleport gate with the cut flag.
5. Post-process cleanup; mouse smoothing off; delete the old camera code; legacy remaps.
6. Look-trigger escalation with lure, reflex and nudge.
7. Minimal chapter1_story.py camera rewrite plus the validator.
8. Build. Run `ygg.Story.CamTest`, then one `ygg.Story.Auto 1` run under the session monitor (§11).

**P1, days 2-3: the cinema.**
1. Fall with slow motion; fly-out with the path and the world-builder fly points.
2. Inspect: photos, compass, drawing.
3. Hands: compass, photo, Dolma, khata, bottle.
4. Present mode, talk_side, npc_face; NPC preload; staging asserts.
5. Tent interior and sleeping bag (needs the director's eye); headlamps.
6. Compass tremble; attention, FOV beat, exposure, breath override.
7. Settings rows; monitor columns; `ygg.Cam.Frame` review shots; turn dead zone; MountainRumble as a rig shake.

**P2, after the director plays:**
- Mixamo pose clips through a montage on CharacterMesh0 (check that ABP_Unarmed_Climber's single slot is a full-body DefaultSlot)
- crawl clip
- DOF at the stone
- systemic stumbles
- climbing inside the rig
- CrevasseCrossing sway as a rig layer
- gamepad curves

---

## 11. Verification checklist (measure first; the director judges the look)

| ID | Proves | Test | Pass |
|---|---|---|---|
| V1 | No backflip, no grab | `ygg.Story.Auto 1` full run under the session monitor; frames.csv | In frames with mouse_dx = mouse_dy = 0 and no `cut`: per-frame view change ≤ story cap × dt + 1°. Pitch within [-80, 82] outside `scripted=Fall`. Story-driven yaw ≤ 20°/s, pitch ≤ 8°/s (12 at the plume). |
| V2 | Never stuck low | frames.csv | `eye_floor_cm ≥ 120` in 100% of rows with speed > 0.6 m/s; ≥ 135 in 99%. Zero rows with speed > 0.6 m/s and posture ∉ {stand, crawl}. Zero "standing watchdog" errors in log.txt. |
| V3 | Always restores | `ygg.Story.CamTest` | PASS for: lie→sit→stand; sit→crawl→stand; kneel1→stand; kneel2→stand; nudge 40° away; nudge cancelled by mouse; reflex; knock; fall + HardReset; blink + self-teleport; 3 s fly-out; inspect open/close; JumpTo mid-pose; Stop mid-fly-out; menu open/close mid-pose (the lease survives); death mid-pose. Each checks within 2 s: eye height, pitch, roll, FOV, view target, pawn visible, limits, dilation, plane constraint, leases. |
| V4 | Leases release | `ygg.View.Audit` after every beat in the Auto run | No lease owned by a finished beat, except those declared `keep` or `end`. No unplanned "expired" lines. |
| V5 | Cuts only in black | log.txt | Every `Story: camera cut` line has lid ≥ 0.99 or veil ≥ 0.99. Count = teleports + 2 (fly-out start and end). Zero automatic-blink warnings in chapter 1. |
| V6 | Nudge discipline | log.txt + frames.csv | ≤ 3 nudges per run, ≥ 60 s apart. A run where the tester looks at every target in time logs 0 nudges. |
| V7 | Bob is right | Walk 30 s on flat snow at walk speed and at 0.4 m/s | High-passed eye Z peak-to-peak within ±30% of the table. Trough within 60 ms after the footstep sound log. Rest-step shows dip-and-hold. Standing still: Z range ≤ 0.4 cm. |
| V8 | Sickness band empty | Stand still 60 s at 4,400 m at rest; then after a sprint | Pitch residual at 0.2-0.4 Hz ≤ 0.1° (FFT of frames.csv). After the sprint the oscillation is ≥ 0.45 Hz. At 8,400 m (prologue) B ≥ 0.6. |
| V9 | No dimming | Screenshots at camp: pitch 0, then after 30 s at -70° (`ygg.Cam.Frame` or monitor shots); `ygg.Body.Hypoxia 0` | Mean snow luminance differs < 15%. Vignette logged at 0.2. Saturation, fringe and grain overrides off. |
| V10 | Body reads right | Screenshots: standing at -40°, -60°, -75°; sit, kneel1, kneel2 at their pitch floors; crawl | Boots and jacket visible, no hip seam, no collar or near-clip. No visible missing leg at the posture floors. **Director judges.** |
| V11 | Streaks | Same 180°/s turn by the stove and prayer flags, recorded at frame generation 4x / 2x / off and motion blur 5 / off; also LightShafts 0 and ShadowRefresh 0 | Report base fps for each (2x target: median ≥ 45). **Director judges.** |
| V12 | Cost | `stat YggView`; session monitor at three fixed spots (tent, camp, memorial) before and after | Rig ≤ 0.05 ms game thread. Frame median unchanged within ±0.3 ms. GPU +0 outside inspect; inspect ≤ +0.8 ms. Shadowless headlamps confirmed. |
| V13 | Compositions | `ygg.Cam.Frame` for balcony_start, fly_0..fly_3, tent_start (lie, sit), tent_out after stand-up, the stream kneel, kneel_spot (plume in frame), night_tent_sit → `Saved/Review/frames/*.png` | **Director approves** before the full play. |
| V14 | Fly-out | frames.csv during the shot + world-builder report | Roll exactly 0. Turn ≤ 12°/s. Clearance ≥ 120 m. No hitch > 50 ms. Pawn visible again afterwards. |
| V15 | Script is clean | `python -I Tools/Story/chapter1_story.py` | Prints `validate: OK`. |
| V16 | No freezes in view | monitor stall list | No stall > 100 ms while the eyelids are open in chapter 1. DressAs stalls occur only in black. |
| V17 | Input safety | events.csv + survival log | Choice keys never eat or drink. Left mouse never closes an inspect. Moving while sitting never slides the climber. |

---

## 12. Risks and open decisions

### Decisions for the director

1. Frame generation default "Auto" = 2x, and motion blur off (§5.10).
2. Mouse sensitivity default: today 0.175°/count; proposal ×0.7.
3. Walk speed and acceleration (2.3 m/s today; starts and stops take about 0.1 s). Proposal: MaxAcceleration 600 and BrakingDecelerationWalking 800, so starts take about 0.4 s. Walk speed unchanged. The camera works at any speed.
4. Slow motion in the slip (`SlipSlowMo` 0.35, or 1 = off).
5. The plume's stronger nudge (12° pitch), or lure only.
6. Assets that need the director's eye or a download:
   - the tent interior (an inner shell)
   - a sleeping-bag prop
   - the khata, bottle and compass meshes
   - Dolma's drawing texture
   - the lure sounds (chosen by ear: tent flap, laboured breath, flag gust, ridge wind)
   - later, Mixamo sit, kneel and crawl clips with the director's account
7. Turn-lean off by default.

### Risks

| Risk | Mitigation |
|---|---|
| Postures without clips are a visual compromise (legs hidden; a standing torso lowered) | Pitch floors, the sleeping bag and the hands cover it. P2 clips fix it; FeetBody follows CharacterMesh0 automatically. |
| Re-parenting the camera off the head socket (FP arms, climbing view, compass) | `ygg.View.Rig 0` for A/B. Check at -60° with the compass up. Climbing is disabled in chapter 1. |
| OneBody torso clips the near plane at -75° | Hidden neck, waist lean, near clip 7 as a fallback, `ygg.View.OneBody 0`. |
| Some clips lack footstep notifies, or notifies fire twice | Distance fallback and the 0.2 s minimum gap; measured in V7. |
| The rig feels too still or too floaty for the director | Every amplitude is a config value. Head motion defaults to Full. Tune after the director plays. |
| Time dilation side effects | 0.7 s only. Real-time story clocks. The watchdog restores it. Switch available. |
| Fly-out streaming hitches | Preload in the slip's black. Streaming source on FlyCam. Fallback path ending at fly_2. |
| DOF blurs the first-person inspect item | Keep only the darken (the default anyway). |
| A wrong crawl line goes through a tent wall | The world-builder door-axis assert. |
| Players do not know W stands them up | Idle hints at 6-20 s; blink fallback at 90 s (crawl). |
| Merge conflict in YggStory | Rig in new files first; one story pass after the other session commits. |
| Old `.ygs` data with removed keys | Remapped safely with an error log; the validator refuses to write it. |
| Schedule against the 17/18 December interim | P0 (1-1.5 days) alone fixes all reported bugs. P1 items ship one by one. |

---

## Appendix A: scoring of the three designs

| Design | Fixes the complaints | Cinematic | Comfort and robustness | Fast to build | Performance | Total |
|---|---|---|---|---|---|---|
| 1. Eyes on a Body | 9 | 8 | 9 | 6 | 9 | 41 |
| 2. Directed Eyes | 8 | 9 | 7 | 5 | 8 | 37 |
| 3. Steady Eyes | 9 | 6 | 10 | 8 | 9 | **42** |

**Base: 3.**
- No clip dependency.
- Component + modifier.
- Leases.
- ShapeWalkSpeed, the only design that noticed that a lock fails silently unless SurvivalComponent's per-tick speed write is shaped.
- Validator.

**From 1:**
- invariants, CamTest, standing watchdog
- breathing grows with altitude (rule 8)
- bob table
- one body on FeetBody
- crawl-out reveal, headlamps, compass lure at the plume

**From 2:**
- shot cards and `ygg.Cam.Frame` review shots
- staging asserts
- present mode, talk_side, async preload
- lie-to-sit wake
- slow-motion slip
- fly-out from the slip spot with an FOV push and roll 0
- attention zoom, the heartbeat at the plume, lower-third captions

**Rejected:**
- Character crouch with a tent ceiling (mesh-shift and trap risk)
- IK kneeling legs
- Design 3's 0.08°/count sensitivity cut and its collision-free crawl
- Design 1's hard yaw limits through the camera manager
