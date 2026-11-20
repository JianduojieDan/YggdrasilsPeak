# B3 Climbing Spec: Rope, Ladder, Axe

Development Plan block **B3 · Climbing feel on the demo route** (7 days, depends on A4).
Written 2026-11-20. Status: spec, nothing built. Read with `Chapter1_Camera_Spec.md` (the view rig) and
`Chapter_Design_v1.md` §4.8 (the mechanics table this spec makes real).

**Goal (from the plan):** the route feels like climbing, not walking.
- Rope ascending on steep fixed ropes with an ascender (jumar) rhythm.
- Ladder crossings with balance and wobble.
- A slip that you can stop with the ice axe.
- Every action feeds effort into the physiology model.

**Done when:** each action works from start to end, can fail with a clear cause, and recovers. The robot tests in §11
pass. The user has played it.

Words used in this spec:
- **Jumar** (ascender): a handle that clamps on the rope. It slides up, and its teeth stop it from sliding down.
- **Anchor**: where the fixed rope is tied to the mountain (snow stake, ice screw, rock peg), every 10-25 m.
- **Hand line**: the rope along a ladder that you clip into and hold.
- **Self-arrest**: stopping a slide on snow by rolling onto the ice axe and driving its pick into the snow.
- **Lease**: a time-limited hold on the camera (see the camera spec §4.4). It always ends by itself.
- **Owner -3**: the new lease owner id for climbing gameplay (the story uses beat numbers, -1 and -2 are taken).

---

## 1. What we learned (short)

### 1.1 Real technique

| Topic | What really happens | What we keep |
|---|---|---|
| Jumar on fixed lines | Climbers push the jumar up the rope, step up into it, rest on it. The teeth catch a fall. You are never unattached: a safety carabiner stays on the rope too. Descending: rope wrapped round the arm on easy ground, figure-8 rappel on steep ground. | push → step → step → breathe; hanging on the jumar is a rest; you cannot fall while on it. |
| Passing an anchor | The jumar cannot slide past the knot. You move the safety carabiner above the anchor first, then the jumar. One point is always attached. | A two-part "clip past" at each anchor, about 1.2 s, slower with cold or hypoxic hands. Skipping it is impossible: the jumar stops. |
| Icefall ladders | Aluminium ladders lashed together over crevasses. You clip both hand lines first. Crampon points catch on the rungs, so every step is placed with care. Long ladders sag and bounce. Teammates pull the lines tight to make a rail. Advice: look at the rungs, not into the hole. | Clip the hand line (optional, but it saves you); deliberate steps; a crampon can catch; bounce grows with length; looking at the rungs helps. |
| Self-arrest | Roll toward the pick. Pick into the snow by the shoulder, body weight on the shaft. With crampons, feet UP (they catch and flip you). Act fast: once you tumble, arrest is nearly impossible. Once stopped, kick steps before you take the pick out. | One "axe" input; the sooner, the better; too fast or on ice it fails; a tumble ends the chance; kick steps before letting go. |

Sources: Alan Arnette, [Khumbu Icefall ladders](https://www.alanarnette.com/2013/04/12/everest-2013-crossing-the-ladders-of-everest/);
RMI, [training for the Khumbu Icefall](https://www.rmiguides.com/blog/2009/04/11/training_for_the_khumbu_icefall);
ExplorersWeb, [fixed ropes in Himalayan climbing](https://explorersweb.com/newcomers-guide-fixed-ropes-in-himalayan-climbing/);
Wikipedia, [self-arrest](https://en.wikipedia.org/wiki/Self-arrest);
The Mountaineers, [steep snow tutorial](https://www.mountaineers.org/locations-lodges/seattle-branch/committees/seattle-climbing-committee/course-templates/alpine-climbing-courses/intense-basic-alpine-climbing-course/course-materials/steep-snow-tutorial).

### 1.2 Games (the supervisor asked us to study Steep and PEAK; Cairn added because it is the closest)

| Game | What makes it feel physical | What we take | What we avoid |
|---|---|---|---|
| **PEAK** (2025) | One stamina bar is everything. Hold a button to climb; the bar drains; empty = you fall. Carried weight and injuries take visible pieces off the bar, so you always see WHY you are weak. Lunges cost stamina. | Stamina is the climbing budget; the cause of weakness is visible (pack weight, cold, thin air). Every B3 failure names its cause. | Floppy, comic bodies. Our tone is fate, not slapstick: hands are precise IK, never rubber. |
| **Steep** (Ubisoft) | Not a climbing game. Its feel comes from momentum on snow: speed builds visibly, sound and spray sell it, you read the line ahead before you commit. Falls restart fast. | The slide: speed you can read, wind roar and spray instead of FOV tricks; see where the slope ends before you commit; restart near where you died (our checkpoints). | Its arcade physics and instant no-cost restarts. |
| **Cairn** (2026) | Deliberate, limb-by-limb climbing. Stamina has no bar: you feel it in shaking limbs, breathing and a greying screen. Pitons stop falls and are finite. | The body is the main meter: breath, heart, shiver and tunnel (built 2026-11-13) tell you about effort. The HUD bar is secondary. | Limb-by-limb control: too slow for 14 h of route. |

Sources: [PEAK wiki, how to play](https://peak.wiki.gg/wiki/How_to_play); [PEAK stamina guide](https://deltiasgaming.com/how-does-the-stamina-work-in-peak);
[PC Gamer Cairn review](https://www.pcgamer.com/games/action/cairn-review/); [Quarter to Three on Cairn](https://www.quartertothree.com/fp/?p=53693).

---

## 2. Feel goals (what the player should feel)

1. **Weight.** Going up a steep rope is work you do with your arms and legs, step by step. Walking was free; this is not.
2. **Rhythm, not mashing.** Push, step, step, breathe. A steady rhythm is as fast as rushing over a long climb and keeps you
   strong. Rushing wins 30 m and loses 120 m. Higher up, the breath you need between steps gets longer.
3. **Safe, but slow.** On the jumar you cannot fall. The danger on the rope is time, cold and oxygen, not a sudden death.
4. **The gap.** A ladder is short (3-4 m here) and terrifying. Slow steps, metal under crampons, the slot below, wind on
   the side. Clipping costs a second and saves your life.
5. **The slip is the one fast moment.** Off the rope on steep snow, one careless sprint and you are sliding. You have about
   a second to act. Done right, you stop and lie there breathing. Done late, you go over the edge.
6. **Your body tells you.** Breath, heartbeat, shaking hands and the edge of the view say how you are doing before the HUD.
7. **Every failure has one clear cause** on screen: "Your grip gave out: exhausted at 8,400 m." "You fell into a
   crevasse. You were not clipped to the hand line." "The axe would not bite: blue ice."
8. **The camera is your eyes.** No third-person, no cut, no FOV kick. The view moves only because the body moved.

---

## 3. What exists today (read before building)

| Thing | Where | Today |
|---|---|---|
| Clip-in to fixed ropes | `UExpeditionComponent::UpdateRope` (ExpeditionComponent.cpp:360), 5 Hz | Within 4 m (6 m to let go) you are "on rope": walkable floor 58°, uphill costs ×0.5 (`RopeGradeMultiplier`), half the storm slowdown, no deadly falls. Automatic, no input. |
| Rope data | `AFixedRope` (FixedRope.h) | `RopePoints` (clip-in path), `LayPoints` (visual), anchors as three transform arrays: `PicketAnchors`, `ScrewAnchors`, `PitonAnchors`. `DistanceToRope()`, `IsNear()`. No distance-along-rope, no anchor order. |
| Ladders | `ACrevasseCrossing` (CrevasseCrossing.h/.cpp), 7 placed by `place_crevasses.py` | `LadderZone` overlap sets `SetFootingSpeedMultiplier(0.45)`, no sprint. The fixed rope is lifted over each ladder and is the (one) hand line. `FallZone` kills (`KillBy`). The view sway is OFF when the view rig exists (it wrote the control rotation's roll; the rig forbids that). Camera spec §10.3 P2 lists "CrevasseCrossing sway as a rig layer". |
| Face climbing | `UClimbingComponent` (Space grab, C let go) | Flying mode, stamina 0.8/s, slips when tired, falls: 7 m safe, 26 m deadly. `ygg.Climb.Camera 1` pulls a third-person arm (against camera rule R1; chapter 1 sets 0). Reads WASD by `ConsumeMovementInputVector()`. |
| Survival model | `USurvivalComponent::StepSimulation` (SurvivalComponent.cpp:160-270) | Stamina: walk recovers 8/s, idle 15/s, sprint −12/s, uphill −20/s per grade unit. SpO₂ from `PhysioSpO2(alt, acclim, gapTorr)`; the gap is 2 Torr walking, 5 sprinting, +3 per grade unit. `SetClimbing(true)` stops recovery. `SetFootingSpeedMultiplier` (clamped 0.1..1). |
| Heart rate | ExpeditionComponent.cpp:766 | 64 + 1.5 per SpO₂ point under 96 + effort (sprint 55, else speed/230 × 24) + 18 exhausted + cold. Feeds the rig (`SetBody`) and the HUD. |
| View rig | `UYggViewRigComponent` (YggViewRig.h) | The ONE writer of the view. Postures (Walk, Lie, Sit, Kneel1, Kneel2, Crawl), leases, `Knock`, `Fall`, `AddShake`, `SetHands`, `OnStep`, `ShapeWalkSpeed`, `HardReset`, watchdog, roll ≤ 2° outside takeovers. |
| Hands and axe | `UFirstPersonBodyComponent` | Axe = `HeldToolMesh` on the right arm by two-bone IK (only if the Blueprint sets it). `SetHandsInUse(Item, Left, Right)` raises both arms to view-space spots — but it is refused while a tool is held (FirstPersonBodyComponent.cpp:649). |
| Steps | `USnowTrailComponent::OnFootContact` (from `UYggFootstepNotify`) | Plays the surface sound (0 snow, 1 ice, 2 rock, 3 ladder), stamps prints, calls `Rig->OnStep`. Ignores contacts under 40 cm/s or off the ground. |
| Telemetry | `YggTelemetryComponent` | `state` column: climb / rope / walk / … |
| Robots | `YggPlayBot` (story), `YggRouteBot` (A4 walker), `YggSummitBot` (A5) | Console-driven, write `Saved/Bot/<run>/report.md`. |
| Inputs taken | | Space climb/jump, C let go, Shift sprint, 1/2 eat/drink, E read, R rest, Tab pack, Q compass, P pause. **Free: left and right mouse buttons, gamepad triggers.** |

Steep rope on the demo legs (route.json 20 m grid, so the real rope is steeper in places): leg 2 (Base Camp → Icefall)
has 9 segments of 25-34° (the steepest 34°, about 2.1 km along); leg 7 (South Col → summit) has about 24 segments of
25-42°, the steepest (40-42°) about 1.0 km and 1.75 km along the leg. Leg 3 (Icefall → Camp 1) is mostly gentle. Ladders: six on legs 2-3 (gaps 2.6-4.0 m) plus one more
crossing actor (seven `ACrevasseCrossing` in total).

---

## 4. Shared design

### 4.1 One input idea: the mouse buttons are your hands

| Input | Keyboard / mouse | Gamepad | Meaning |
|---|---|---|---|
| **Left hand** | Left mouse button | Left trigger | Gear: push the jumar, clip past an anchor, clip / unclip the hand line |
| **Right hand** | Right mouse button | Right trigger | The ice axe: hold to self-arrest |
| Step / walk | W (A/D sideways) | Left stick | Step up into the jumar; step along the ladder; kick steps after an arrest |
| Balance | A / D | Left stick X | Lean against a wobble on a ladder |
| Pull up | Space | A (bottom face) | Climb back onto a ladder from the hand line |

- Both hand inputs are registered with `YggSettings::RegisterKey` so they can be rebound (`HandLeft`, `HandRight`).
- They live in `UClimberControlsComponent` (it already owns runtime actions), with getters and a robot override.
- Prompts appear only the first two times each, then only as a small key glyph next to the HUD state line
  (`[jumar]`, `[ladder · clipped]`). Same quiet style as the existing messages. No QTE circles.

### 4.2 Settings rows (Settings > Game)

| Row | Values (default first) | Effect |
|---|---|---|
| Climbing input | Rhythm / Hold | Hold: holding W runs the jumar cycle by itself at a steady rhythm. Same speed and cost as a good steady rhythm, never better. For comfort and accessibility. |
| Clip at anchors | Steep only / Everywhere | Everywhere = the Chapter_Design §4.8 rule on gentle ropes too (P2, see §5.9). |
| Balance help | Off / On | On: half of every ladder lean is countered for you. |
| Slips | Full / Gentle | Gentle: footing risk ×0.5, arrest window ×1.5, ice arrests like hard snow. |

### 4.3 The activity model (one way every action reaches the body)

Every B3 action tells `USurvivalComponent` what the body is doing: `SetActivity(EYggActivity, Intensity01, SpeedCapCmS)`.
While an activity is set, its cost **replaces** the walking terms (walk / sprint / grade) in stamina, the SpO₂ gap and the
heart rate, so nothing is counted twice. `None` gives the walking model back.

```
UENUM() enum class EYggActivity : uint8 { None, Ascend, AscendRest, Clip, Ladder, LadderHang, PullUp, Slide, Arrest, Held };
```

Starting values (all in config; tuned from the robot tables in §11):

| Activity | Stamina (of 100) | SpO₂ gap (Torr) | Heart (+bpm, replaces the speed term) | MET (logged for the thesis) |
|---|---|---|---|---|
| Walking (today, for comparison) | +8/s recover − 20/s × grade | 2 + 3 × grade | 0-24 by speed; sprint 55 | 3-6 |
| **Ascend**, per step | −0.9 per step (rushed ×1.6) | 4 (rushed +3) | 34 (rushed +15) | 8 |
| Jumar push | −0.3 per push | — | — | — |
| **AscendRest** (hanging on the jumar, the breath) | +5/s recover × the usual penalties (hungry/cold ×0.5, oxygen fatigue up to ×0.5) | 1 | 10 | 2.5 |
| **Clip** (past an anchor, a hand line) | −0.5 per clip | 1 | 6 | 2.5 |
| **Ladder** | −1.5/s (no recovery while balancing) | 1 | 22 (fear) | 3 |
| Ladder wobble saved | −3 per save | — | +15 spike | — |
| **LadderHang** (on the hand line) | −3/s | 3 | 35 | 5 |
| **PullUp** | −6 per pull | 5 | 45 | 9 |
| **Slide** | 0 | 0 | adrenaline spike +40 | — |
| **Arrest** | −10 at the bite, then −6/s | 4 | adrenaline spike +50 | 9 |
| **Held** (stopped on the axe) | −2/s | 2 | — | 3 |

- **Adrenaline** is a new decaying heart term (τ 20 s) so a slip or a ladder wobble shows on the heart rate and the beat
  for a while after, even standing still.
- MET values are rough starting points from general activity tables; check them in B2 against the Compendium of Physical
  Activities before the thesis uses them.
- Exhaustion (stamina 0) keeps its meaning: `bExhausted` until stamina is back at 20 (`SprintResumeStamina`).
- The telemetry `state` column gets `ascend`, `ascend_rest`, `clip`, `ladder`, `ladder_hang`, `slide`, `arrest`,
  `held`. Three new columns are appended at the END (so `plot_session.py` keeps working): `met`, `footing`, `balance`.

### 4.4 Rules every B3 state follows

- **Camera:** only through the rig (§9). No direct camera, control rotation or view-target writes. No nudges (they belong
  to the story's budget). No look locks. Every view change has a body reason: a step, a breath, a slip, a jolt.
- **Leases** are taken by owner **-3** and are released by the state that took them. All B3 states listen to the rig's new
  `OnHardReset` (teleport, death, story `ReleaseAll`) and drop to Idle at once.
- **Story first:** no B3 state starts while the body is not free (the same test as `UClimbingComponent::IsBodyFree`: posed,
  held by a Move lease, inspecting, or in a takeover).
- **Ticks:** each B3 component ticks only while active. Watching for a start is piggy-backed on things that already run
  (the 5 Hz rope check, the overlap events, the footstep system). See §12.
- **Messages:** one line, what happened and why, through `UExpeditionComponent::SayAmbient` (held back during story beats).

---

## 5. Mechanic A: rope ascending (the jumar)

### 5.1 When it starts and stops

- **Starts** (automatic) when all hold: on a fixed rope (`IsOnRope`), the rope rises at least **AscendFromDeg = 30°**
  over the next 6 m, and the player moves up the rope (move input · rope direction > 0.3). First 0.6 s = **Attach**: the
  left hand takes the jumar off the harness and clamps it on the rope (clip sound). First time: "Steep rope. Push the
  jumar up [LMB], then step [W]."
- **Ends** when the rope rises less than **AscendToDeg = 24°** over the next 6 m for 2 m of travel, at the end of the rope,
  or when the player turns and goes down the rope (→ Descend, §5.8). The jumar goes back on the harness (0.5 s, click).
- The 30° start is a decision for you (open question 1): 30° gives more ascending (and a slower summit), 35° keeps it to
  the steepest bits. The robot reports how many metres of each leg become ascending.

### 5.2 States

| State | What the body does | Moves? | Exits |
|---|---|---|---|
| Attach | Left hand clamps the jumar on the rope | no | 0.6 s → Ready |
| Ready | Jumar at chest height. You can push. | no | LMB → Push |
| Push | Left arm slides the jumar up the rope, 110 cm, 0.35 s | no | done → Loaded |
| Loaded | Jumar above you. You can step. | no (resting on the rope = AscendRest) | W → Step |
| Step | One step up the rope, 55 cm along the slope, 0.45 s (speed pulse) | yes | done → Loaded (1st step) or Ready (2nd step: jumar back at the chest) |
| ClipPast | At an anchor: safety carabiner over (0.55 s), then the jumar over (0.65 s) | no | done → Ready; LMB released early pauses between the two parts (still on the safety) |
| Spent | Stamina 0. You hang on the jumar. Push is refused. | no | stamina ≥ 20 → Ready |
| Detach | Jumar back on the harness | no | 0.5 s → Idle (walking on the rope again) |

Between steps nothing moves: you are hanging on the jumar (AscendRest), which is how real climbers rest.

### 5.3 The rhythm (the heart of the feel)

One cycle: **Push → Step → Step → Breathe**. Breathe = the pause between the second step and the next push.

- **Breath target** (seconds), rising with altitude, shorter on bottled oxygen, longer when tired:
  `BreathTarget = lerp(0.3, 1.6, (alt − 5500) / (8849 − 5500)) × (bottle ? 0.6 : 1) × (1 + 0.5 × exhaust)`.
- A push before **70%** of the target = a **rushed** cycle: steps cost ×1.6 stamina, SpO₂ gap +3 Torr, heart +15.
- A push between 70% and 200% = **steady**: normal cost. The pause itself recovered stamina (AscendRest).
- Longer than 200% = resting. Fine.
- The cue is the breathing sound: `MountainAudioComponent`'s breath phrase ends about when the target is met, and the
  rig's breathing (camera spec §5.4) settles. No meter on screen.

What the numbers give (6,000 m, fresh; checked by robot test C1/C2):

| Style | Cycle time | Along the rope | One full bar lasts | Over 60 m | Over 120 m |
|---|---|---|---|---|---|
| Steady | 1.25 s + 0.4 s breath | 0.67 m/s | practically sustainable (the breath pays back each cycle) | ≈ 90 s | ≈ 180 s, still strong |
| Rushed | 1.25 s | 0.88 m/s, then forced rests | ≈ 32 m | ≈ 85 s, exhausted | ≈ 190 s, exhausted, higher heart |

So rushing wins a short dash and loses the climb, and leaves you exhausted with a lower SpO₂. At 8,600 m the breath target
is ≈ 1.5 s and recovery is halved by thin air: steady ≈ 0.4 m/s along the rope.

Compared with today's clipped walking on a 35° rope (≈ 1.6 m/s at 6,000 m), ascending is about half the speed. That is
the point (it is climbing now), but it lengthens A5 (open question 1).

**Hold mode** (setting): holding W runs Push → Step → Step → Breathe by itself with the breath target met exactly.
Never faster than steady.

### 5.4 Anchors

- At load, `AFixedRope` sorts its anchors by distance along `RopePoints` (`AnchorS`).
- The jumar stops 15 cm under the next anchor. Push and Step do nothing more there. Prompt (first two times):
  "Anchor. Clip past it [LMB]."
- **ClipPast** time 1.2 s at a normal body; × 1.4 with cold hands (core under 35.0 °C), × 1.3 when hypoxic (SpO₂ under
  60), × 1.2 exhausted. These stack (cold, hypoxic and tired at 8,500 m ≈ 2.6 s, which is the honest feel).
- You are never unattached on a steep rope. A missed clip only blocks you. (Gentle ropes: §5.9.)

### 5.5 Moving while on the rope

- Movement stays in walking mode (collision, foot IK on the slope, footprints and step sounds keep working). Walkable
  floor stays 58° while clipped (the rope's surface is at most 48°).
- `UYggRopeAscendComponent` ticks before character movement (tick prerequisite), takes the move input
  (`ConsumeMovementInputVector`, like `UClimbingComponent`) and puts back its own input along the rope during a Step.
- Speed: `SetActivity(Ascend, …, SpeedCapCmS)` makes `ApplyMovementSpeed` use a half-sine pulse (0 → 160 cm/s → 0 over
  0.45 s, ≈ 55 cm per step). Between steps the cap is 0.
- Leash: A/D move you sideways at most 1.5 m from the rope line. Beyond that, the sideways part of the input is dropped.
- Each Step calls the new `USnowTrailComponent::TriggerStep(bLeft)` at mid-step: the surface sound, the print, and
  `Rig->OnStep` (the bob locks to it). Animation notifies are muted while the ascend drives the steps
  (`SetExternalSteps(true)`), so steps are never doubled.

### 5.6 Failure and recovery

| What goes wrong | What happens | Clear cause shown | Recovery |
|---|---|---|---|
| Stamina reaches 0 | Spent: you hang on the jumar; push refused | "Spent. Hang on the jumar and breathe." | Stamina ≥ 20 → Ready. Cold and oxygen keep running: resting too long at 8,500 m without a bottle still kills (existing hypoxia death, with its own cause). |
| Rushing | Exhausted sooner, lower SpO₂, heart up | (the body shows it; the message only when Spent) | Steady rhythm |
| Trying to pass an anchor | Jumar stops | "Anchor. Clip past it [LMB]." | ClipPast |
| Too far sideways | Input dropped at 1.5 m | — | Move back |
| Slip on the rope | Not possible while on the jumar | — | — |

### 5.7 Camera, hands, body

- **Camera (rig):** `SetMotion(Ascend)` every frame. The eye leans into the slope (forward 10 cm, down 5 cm, the slope
  stoop cap raised from 25° to 40°). The step bob locks to the ascend steps. The breathing layer shows the breath. Free
  look (you look up the line, down the drop, at the anchor). Nothing else.
- **Hands:** left hand on the jumar (`jumar_low` at the chest, `jumar_high` at the end of a push; the climbing code moves
  the target between them each frame through `SetHandsInUse(Item, Left, Right)`; the hands trail by ≈ 70 ms as specified).
  Right hand: holds the axe like a cane if the axe is out (§8.2), else rests on the rope below.
- **Clip past:** both hands go to the anchor (`clip` item), close to the chest, at the anchor's real position.
- **Props:** a jumar mesh on the left hand and a short sling from the harness to it (one stretched rope piece). P1.
- **World body (mirror, shadow, later Nima):** P0 = the walk blend space at the pulse speed with foot IK. P1 = an
  upper-body "push" clip (Meshy library or Mixamo; §8).

### 5.8 Descend (minimal in B3)

- Going down a steep rope: **arm wrap** (rope round the arm): walking down while clipped at speed ×0.8, stamina −0.5/s,
  anchors pass by themselves. This is today's clipped walking plus a hand pose (`armwrap`) and a rope-slide sound.
- Figure-8 rappel on steep ice (Chapter_Design §4.8: ×2 speed with a brake input; too fast is a slip) is **P2**.

### 5.9 Gentle ropes and "Clip at anchors: Everywhere" (P2)

- Default: on ropes under 30° the clip stays automatic (today's behaviour), anchors pass by themselves.
- Everywhere: at each anchor on a gentle rope you get a 3 m window to press LMB (0.8 s clip). Miss it and you are
  unroped until the next anchor: walkable floor back to normal, deadly falls back on, slips possible. "Unclipped: you
  missed the anchor." This is the Chapter_Design rule; it is a lot of clicks over 27 km of rope, so it is a setting.

---

## 6. Mechanic B: ladder crossing

### 6.1 The moment (what the player sees)

You reach the lip. The ladder is 3-4 m of aluminium over a black slot. The hand line runs beside it. "Clip the hand line
[LMB]." Click, the rope tightens. You step on: crampons ring on the rungs, the view drops a little as the ladder sags. A
gust from the left: the horizon tilts a degree, your hand grips the line, the ladder creaks. You press D, it settles. You
look down at the rungs; the slot goes on below them. A crampon point catches: clank, a jolt, you stop. Three more steps.
Snow under the boots. Unclip.

### 6.2 States

| State | What happens | Exits |
|---|---|---|
| Approach | Within the LadderZone at the near end. Prompt: clip the hand line. Walking on unclipped is allowed. | LMB → Clipping; W onto the rungs → Cross (unclipped) |
| Clipping | Carabiner onto the hand line (0.6 s) and the rope tightened (0.4 s) | done → Cross (clipped) |
| Cross | Deliberate steps rung to rung. Balance runs. | far end → Unclip; balance ≥ 0.6 → Wobble; ≥ 1.0 → Slip |
| Wobble | You stop. Hands grip the line (if clipped). Steps blocked. | balance < 0.4 → Cross; ≥ 1.0 → Slip |
| Slip | Boots off the rung: a Knock jolt toward the lean side | clipped → Hang; unclipped → Fall |
| Hang | Hanging from the hand line in the slot, under the ladder's edge | Space ×3 → PullUp |
| PullUp | Three pulls (0.6 s each) back onto the rungs | done → Cross (standing) |
| Fall | Into the slot. The existing `FallZone` kills. | death → last checkpoint |
| Unclip | At the far end (0.5 s). Or walk off: auto-unclip after 2 m, with the click. | → Idle |

### 6.3 Steps and balance

- **Steps:** each step goes two rungs (≈ 60 cm). Holding W steps every 0.7 s. Tapping W faster (down to 0.45 s) is allowed
  but kicks the balance harder. No sprint. Crossing 3-4 m ≈ 5-7 steps ≈ 4-6 s. Each step calls `TriggerStep` with
  surface 3 (ladder sound, rig dip).
- **Balance** `L` in −1..1 (0 = upright). Updated each frame:
  `dL/dt = 0.6·L (a lean grows by itself) + StepKick + Wind + BodyNoise − Counter − Damping·L`
  - StepKick per step: ±(0.08 + 0.10 × step-speed factor) × bounce (1 + 0.35 × (sections − 1)); sign leans the way you
    already lean 60% of the time.
  - Crampon catch: chance 6% per step × speed factor × (1 + exhaust) × look factor. A catch = 0.4 s stop, kick 0.25,
    clank. Look factor: view pitch under −35° (looking at the rungs) ×0.5; above −10° (looking ahead) ×1.5.
  - Wind: `UExpeditionComponent::GetWindIntensity()` × 0.35 × a slow gust curve × the cross-wind share. Test cvar
    `ygg.Climb.LadderWind <0..1>` (−1 = live).
  - Body noise: 0.15 × max(exhaust, hypoxia, shiver), filtered (shaky legs).
  - Counter: A/D, 0.9 per second at full input. Balance help On counters half of `L` for you.
  - Damping: clipped and a hand on the line 1.2; unclipped 0.3. Clipping is the big help; this is the real reason people clip.
- **Thresholds:** 0.6 = Wobble (stop, grip, creak, the heart jumps +15); 1.0 = Slip.
- Sections = ceil(length / 3 m). Here 1-2 sections. Longer, tied double ladders later (Chapter_Design) make bounce grow.

### 6.4 Failure and recovery

| What goes wrong | What happens | Clear cause | Recovery |
|---|---|---|---|
| Lean past 1.0, clipped | Slip → Hang in the slot | "Your boots slid off the rung. The hand line held." | Space ×3. If stamina runs out while hanging you stay hanging (the line holds); you lose time and warmth; at 20 stamina you can pull. |
| Lean past 1.0, NOT clipped | Fall → death in `FallZone` | "You fell into a crevasse. You were not clipped to the hand line." | Last checkpoint (existing) |
| Crampon catch | Short stop and jolt | (sound only) | Keep going |
| Crossing in a gust | Wobbles | (body + sound) | Wait for the gust to pass, or counter it |

`ACrevasseCrossing::OnFall` must ignore a climber whose ladder state is Hang or PullUp (the hanging capsule is inside the
FallZone).

### 6.5 Camera, hands, body

- **Camera (rig):** `SetMotion(Ladder, L)`: roll = 2° × L (the rig's roll limit stays 2°), eye sideways 5 cm × L, a sag
  of up to 3 cm at mid-span (× sections). This is the camera spec's P2 item "CrevasseCrossing sway as a rig layer"; the
  old control-rotation sway code is deleted. Slip = `Knock(-3, side)` (0.14 s, roll 8°, allowed: it is a body reason).
  Free look: looking down at the rungs is encouraged, and the slot shows below them.
- **Hang:** the capsule hangs in flying mode 110 cm under the clip point on the hand line, beside the ladder. The eye stays
  at standing height on the capsule (no new posture). Pitch soft limit +60 (you look up at the ladder and the line).
- **Hands:** `handline`: left hand on the hand line (world point → view space each frame), right hand free or axe.
  Wobble: both hands grip. Hang: both hands up on the line. PullUp: hands move from the line to the rungs.
- **World body:** P0 = walk + foot IK onto the ladder box; Hang = the existing `Climb_HangIdle` clip. P1 = a careful
  "ladder walk" clip.
- **Ladder visual (P1):** a material parameter makes the ladder sag a few cm under the player (vertex offset by distance
  along the ladder). A second hand line on the other side (real crossings have two) is a placement change, P2.

---

## 7. Mechanic C: slip and self-arrest

### 7.1 When you can slip

Only **unroped**, on steep snow or ice. On a rope, a stumble is caught by the rope (§7.5). On the jumar, you cannot slip.

- **Footing** `F` 0..1 (hidden). It rises while you move on ground steeper than the surface's slip angle, and falls back
  (−0.25/s) when you stop, slow down or get on a rope.
- Slip angle: ice 25°, hard snow 32°, soft snow 40°, rock — (no slide; rock falls are the face-climbing falls).
- Rise per second = 0.12 × e × speed factor × body × down × wind, where:
  - e = (slope − slip angle) / 10°, clamped 0..2
  - speed factor: standing 0, walking 1, sprinting 3
  - body: 1 + 1.5 × exhaust + 1.0 × oxygen fatigue
  - down: ×1.5 when going downhill
  - wind: + 0.5 × wind intensity
- **Warning** at F = 0.6: a boot scrape sound, a small stumble (`AddShake(0.6°, 0.3 s)` and a rock-dip on the eye). Slow
  down and it fades.
- **Slip** at F = 1.0.
- No new traces: the slope comes from `CharacterMovement->CurrentFloor`; the surface from the footstep system's last
  step (the new `USnowTrailComponent::GetLastGround()`: surface id + softness). Checked at 10 Hz.
- Also: a face-climbing fall (`UClimbingComponent` grip gives out) that lands on a slope steeper than its slip angle starts
  a slide instead of only fall damage.

### 7.2 States

| State | What happens | Exits |
|---|---|---|
| Watch | Footing runs at 10 Hz (cheap) | F ≥ 1 → Slip |
| Slip | 0.35 s lurch (rig `Slip` takeover: drop 30 cm, pitch −18°, roll 10° toward the fall line side). The body goes down (posture `slide`). | → Slide |
| Slide | Feet first down the fall line. Speed builds. You see where you are going. | RMB → Arrest; no RMB for 1.2 s and speed > 6 m/s → Tumble; slope < 15° and speed < 0.5 → Stopped (runout); no ground within 1.5 m → Airborne |
| Arrest | Roll onto the axe (0.3 s jolt), the pick bites; braking builds over 0.3 s | speed < 0.3 → Held; speed > 11 m/s at the bite → torn → Tumble; RMB released → Slide |
| Held | Lying on the axe, stopped. Prompt: "Kick steps [W]." | hold W 0.8 s (two kicks) → Secure; RMB released before → Slide again (0.3 s grace) |
| Secure | Feet are in. You can let go. | RMB released → StandUp |
| StandUp | Posture `slide` → standing, 1.2 s | → Watch |
| Tumble | Rolling. Arrest impossible until speed < 3 m/s. −2 health/s. | speed < 3 → Slide (arrest possible again); runout → Stopped; edge → Airborne |
| Stopped (runout) | Came to rest on flatter ground | damage applied → StandUp |
| Airborne | Over an edge: falling mode; fall height tracked by the climbing component (7 m safe, 26 m deadly) | landing → existing fall damage / death |

### 7.3 Slide physics (simple and readable)

- Along the fall line: `a = g · (sin θ − μ · cos θ)`, θ from the ground normal. μ: soft snow 0.30, hard snow 0.12, ice 0.05.
  Speed cap 25 m/s. (Hard snow at 38°: a ≈ 5.1 m/s², 10 m/s in 2 s.)
- Movement: flying mode with the capsule's own swept move (collision), plus one downward line trace per frame for the ground
  normal and the edge. That is the only trace B3 adds while active.
- **Arrest braking** `B = Bite(surface) × Grip × ramp(0.3 s)`; Bite: soft snow 8 m/s², hard snow 6.5, ice 2.0, rock 0
  (sparks, a screech). Grip = 0.5 + 0.5 × stamina (0.3 at stamina 0). Net slowing = B − a.
  - If net slowing is 0 or less, you cannot stop there (too steep for that snow). You slow only when the slope eases.
  - This makes the outcome physical and readable: soft snow, early, slow = stop; ice, late, fast = no stop.
- **No axe** (not carried): hands and boots only, Bite × 0.25.
- **Runout damage:** 1.5 health per m/s of the top speed over 6 m/s; +10 after a tumble. (Sprains, Chapter_Design §4.9,
  are P2.)
- Hidden crevasses, serac zones and crevasse slots keep their own rules during a slide.

### 7.4 Failure and recovery

| What goes wrong | Clear cause | Recovery |
|---|---|---|
| Arrest too late, tumble, over an edge | "You slipped on steep hard snow and went over the edge. Arrest sooner: you have about a second." | Fall damage or death (existing); checkpoint |
| Arrest on blue ice | "The axe would not bite: blue ice." | Slows only where the slope eases; avoid ice unroped |
| Too fast at the bite | "Too fast. The axe was torn from your grip." | Tumble until slow, then arrest again |
| Let go before kicking steps | "Kick steps before you let go." | Arrest again |
| Exhausted, weak grip | "Too weak to hold the axe." | Rest before steep ground; take the rope |
| What caused the slip (shown with any of the above) | "Too fast on steep ice while exhausted." (built from the largest footing terms) | — |

### 7.5 Roped stumble

On a rope (not on the jumar), footing still rises on steep ground, but at F = 1 you get a **rope catch** instead of a
slide: ≤ 2 m slide, `Knock(-3, side)`, rope-tighten sound, −10 stamina, "Caught by the rope." This keeps the rope's
value visible.

### 7.6 Camera, hands, body

- **New rig takeover `Slip`** (0.35 s, lease kind `Slip`, max 1 s, peak rate under 120°/s, roll up to 12°): the lurch.
  It reuses the `Fall` takeover's code path, shorter, with no slow motion and no eyelid slam.
- **New posture `slide`** (eye 45 cm, band 35-55, down 0.35 s, up 1.2 s, yaw soft limit ±70° around the slide direction,
  pitch −55..+35, body hidden). Entered by owner -3 at any speed (the normal 20 cm/s posture entry limit does not apply
  to it). Rule R4 holds: the eye is low only in a posture.
- **Arrest view:** you keep looking down the fall line. The bite is a 0.3 s roll of up to 10° toward the pick side
  (`AddShake` and a view offset inside the posture), the eye drops to 35 cm, snow spray, and the axe in both hands
  appears at the bottom of the view. This is a deliberate simplification: a real arrest faces the slope, but turning the
  view 180° in 0.3 s would break the comfort caps. The world body (P2 clip) can show the true position for the mirror
  and for NPCs.
- **Tumble:** no spinning camera. Eyelid flickers (`SetLids Blink`), shake bursts (`AddShake(3°, 0.5 s)`), the roar of
  snow. Comfort first.
- **Hands:** slide: the axe in the right hand. Arrest: `arrest` grip (left hand on the head by the shoulder, right hand on
  the shaft by the hip). This needs the tool-mode change in §8.2.
- **After:** `StoryBreath`-style breath catch (owner -3 Breath lease, 6 s) and the adrenaline heart: you lie there and
  breathe. That is the reward.

---

## 8. Animation, hands and assets

### 8.1 Where each motion comes from

| Need | P0 (no new clips) | P1 / P2 source |
|---|---|---|
| Ascend legs | Walk blend space at the pulse speed + existing foot IK (FeetBody `PlaceFeet`) | P1: Meshy animation library or Mixamo "climbing a slope / step up" on the world body |
| Jumar arm | First-person IK (`SetHandsInUse`, left arm) | P1: upper-body push clip for the world body |
| Clip past anchor | First-person IK (`clip`) | — |
| Ladder walk | Walk + foot IK on the ladder box | P1: careful ladder-walk clip |
| Hang on hand line | `Climb_HangIdle` (already imported, Mixamo) | — |
| Pull up | First-person IK; world body `Climb_TopOut` (imported) | — |
| Slide / arrest | Posture `slide` hides the body (like Lie); first-person arms + axe by IK | P2: prone arrest clip (Mixamo or Meshy) for mirror / NPCs |
| Descend arm wrap | Walk + IK (`armwrap`) | P2: rappel clip |

- Mixamo needs your account (camera spec §12.6). Meshy animations need credits (7 left on 2026-11-20) and your OK.
  Clips to search for: "climbing up a slope", "ladder walk / balance", "hanging idle", "standing up from lying",
  "falling and sliding". You pick by eye; I import with the existing retarget tools (`Tools/Story/ai_retarget`,
  `Tools/Animation/import_mixamo_climbing.py`).

### 8.2 Hands and the axe (FirstPersonBodyComponent changes)

- **Tool modes:** `SetToolMode(Hidden | Cane | Arrest)`. Hidden = the axe on the pack (story hands allowed). Cane = the
  axe in the right hand as today. Arrest = both hands on the axe across the chest (a second view-space grip preset).
- **Left-hand items while a tool is held:** today every hands request is refused while the axe is out
  (FirstPersonBodyComponent.cpp:649). Change: refuse only items that need the right arm. Jumar, clip and hand line use
  the left arm only, so the axe can stay in the right hand (real Sherpas often climb like that).
- **World-space targets:** `SetHandsWorld(Item, LeftWorld, RightWorld)` turns world points (rope, anchor, hand line) into
  view space each frame. Targets are clamped to arm reach: **never stretch** the arm (your rule from 2026-10-26).
- **New hand rows** in the rig's Hands table (DefaultGame.ini): `jumar_low`, `jumar_high`, `clip`, `handline`,
  `handline_grip`, `hang`, `armwrap`, `arrest`. Starting view-space spots (cm, X forward, Y right, Z up):
  jumar_low L(32,−14,−22); jumar_high L(46,−10,6); clip L(30,−6,−18) R(30,6,−18); handline L(28,−30,−30);
  hang L(14,−16,32) R(14,16,32); arrest L(30,−18,−20) R(22,14,−38). Tuned on screen by you.
- **Props (P1):** jumar (ascender) and carabiner meshes, a sling from the harness to the jumar (one stretched
  `SM_FixedRope_Piece`). Source: a free download (Fab / Sketchfab CC0) or Meshy (your credits). Not hand-modelled
  (your rule from 2026-11-15). Open question 4.

---

## 9. Camera: what the view rig needs (all through the rig, camera spec rules R1-R10)

New in `UYggViewRigComponent` (small, additive; the camera spec's owner reviews it):

| Addition | What | Limits |
|---|---|---|
| `SetMotion(EYggMotion Mode, float Balance = 0)` | Gameplay feed each frame, like `SetBody`. Modes: None, Ascend, Ladder, Hang, Slide. Ascend: lean 10/5 cm and slope-stoop cap 40°. Ladder: roll 2° × Balance, sideways 5 cm × Balance, sag ≤ 3 cm. Hang: no terrain spring (no floor), eye on the capsule. | Roll stays ≤ 2° (invariant). A mode not refreshed for 0.5 s fades out over 0.3 s (cannot stick). |
| `Slip(int32 Owner, float Side)` + `EYggScripted::Slip` + lease kind `Slip` (max 1 s) | The slip lurch (§7.6) | ≤ 120°/s, roll ≤ 12° (`RollMaxSlipDeg`) |
| Posture `slide` (enum `EYggView::Slide` + a `+Poses` row) | Low body for slide, arrest, held | Band 35-55 cm; entered by owner -3 at any speed |
| `FSimpleMulticastDelegate OnHardReset` | B3 states go Idle on every HardReset | — |
| Owner id -3 documented | Climbing gameplay leases | Released by `ReleaseAll` like all others |
| Watchdog row | Posture `slide` while no B3 slide state is active for 1 s → stand up, log `Rig: repaired slide` | Guarantees no stuck-low eye (V2) |
| Monitor / recorder columns | `motion`, `balance` | — |

Already there and reused: `Knock` (ladder slip, rope catch), `AddShake` (stumble warning, pick bite, tumble), `OnStep`
(ascend and ladder steps), `SetLids` (tumble flicker), `StoryBreath` (breath catch after an arrest, owner -3),
`SetHands` / FirstPersonBody hands.

Not used: nudges (story budget), look locks (R8), FOV terms (no sprint or speed FOV kick, §5.7 of the camera spec), the
old third-person climb arm (`ygg.Climb.Camera` stays 0 for everything in B3).

---

## 10. File-by-file implementation plan

### 10.1 New files (`Source/YggdrasilsPeak/Survival/`)

| File | Contents |
|---|---|
| `YggClimbTypes.h` | `EYggActivity`, `FYggActivityCost` (Stamina, PerAction, GapTorr, HeartBpm, Met), `namespace YggClimb { constexpr int32 Owner = -3; bool IsBusy(const AActor*); }` (any B3 state active), `FYggClimbBotInput` (left / right hand held, move, balance), `DECLARE_STATS_GROUP("YggClimb")` and cycle stats. Unique names (unity build clash gotcha). |
| `YggRopeAscendComponent.h/.cpp` | `UYggRopeAscendComponent` (Config=Game). States of §5.2. `OnRopeUpdate(AFixedRope*, bool bOnRope)` from the 5 Hz rope check (start test), `TickComponent` only while active (TG_PrePhysics, prerequisite of CharacterMovement), `PushJumar()`, `TryStep()`, `ClipPast()`, rhythm and breath target, speed pulse, leash, `TriggerStep` calls, hands, `SetActivity`, `SetMotion(Ascend)`, messages. Config: `AscendFromDeg 30`, `AscendToDeg 24`, `LookAheadCm 600`, `JumarTravelCm 110`, `StepCm 55`, `StepSec 0.45`, `PushSec 0.35`, `PulsePeakCmS 160`, `BreathMinSec 0.3`, `BreathMaxSec 1.6`, `RushedBelow 0.7`, `ClipPastSec 1.2`, cold / hypoxia / exhaust multipliers, `LeashCm 150`, `SpentResume 20`. |
| `YggLadderComponent.h/.cpp` | `UYggLadderComponent` (player side). `Enter(ACrevasseCrossing*)`, `Leave()`, states of §6.2, balance ODE, steps, crampon catch, wobble, slip → hang / fall, pull-up, hand line IK, `IsHeldByLine()` (for `OnFall`), `SetActivity`, `SetMotion(Ladder, L)`. Config: the §6.3 numbers, `RungCm 30`, `StepSec 0.7`, `MinStepSec 0.45`, `HangDropCm 110`, `PullSec 0.6`, `Pulls 3`. |
| `YggSlipComponent.h/.cpp` | `UYggSlipComponent`. Footing watch at 10 Hz (timer, not tick), states of §7.2, slide physics, arrest, held, secure, stand-up, tumble, runout, airborne hand-off, rope catch, cause text builder, `ForceSlip()` (test). Config: the §7.1 and §7.3 numbers. |
| `YggClimbTest.h/.cpp` | `UYggClimbTest` world subsystem: `ygg.Bot.Climb <case\|all> [seed n] [noshots]`, finds the spots itself at runtime (§11), drives the bot input, writes `Saved/Bot/climb_<time>/report.md`, `samples.csv`, `arrest_table.csv`; quits when done (`ygg.Bot.Climb.Quit`). |

### 10.2 Changed files

| File | Change |
|---|---|
| `Environment/FixedRope.h/.cpp` | At BeginPlay: cumulative length of `RopePoints` and `AnchorS` (all three anchor arrays projected onto the path, sorted; one pass, anchors are already in order). New: `ProjectToRope(Loc, int32& InOutSeg, float& OutS, FVector& OutDir)` (searches ±3 segments from the last one), `PointAt(S)`, `SlopeAhead(S, Cm, bUp)`, `NextAnchorAfter(S)`. |
| `Survival/ExpeditionComponent.cpp/.h` | Adds the three components at runtime (like `UClimbingComponent`, line 120). `UpdateRope` also calls `Ascend->OnRopeUpdate`. Heart rate: when an activity is set, use its heart term instead of the speed term (line 766), plus the adrenaline term. |
| `Survival/SurvivalComponent.h/.cpp` | `SetActivity(EYggActivity, float Intensity01, float SpeedCapCmS = -1)`, `GetActivity()`, `AddAdrenaline(float Bpm)`; in `StepSimulation` the activity cost replaces walk / sprint / grade terms in stamina, the SpO₂ gap and needs; `ApplyMovementSpeed` applies the cap; snapshot gains `Activity`, `ActivityHeartBpm`, `Adrenaline`. |
| `Survival/SurvivalConfig.h` | `TMap<EYggActivity, FYggActivityCost> ActivityCosts` with the §4.3 defaults filled in the constructor. |
| `Environment/CrevasseCrossing.h/.cpp` | Delete the control-rotation sway and the `SetFootingSpeedMultiplier` calls; on overlap call `UYggLadderComponent::Enter/Leave`; expose `GetLadderFrame()`, `GetLengthCm()`, `GetSections()`, `GetHandLine(A, B)` (the lifted fixed rope points inside the LadderZone, found at BeginPlay); `OnFall` ignores a climber held by the line; P1: a ladder MID with `LoadAlong` / `SagCm`. |
| `Survival/ClimbingComponent.h/.cpp` | `Classify` becomes a shared static (`YggSurface::Classify` in the same file or `YggClimbTypes`); `StartFallTracking(float ApexZ)` public; landing on a slide-able slope hands over to `UYggSlipComponent`; `IsBodyFree()` also false while `YggClimb::IsBusy`. |
| `Survival/FirstPersonBodyComponent.h/.cpp` | `SetToolMode`, left-arm-only items allowed with a tool, `SetHandsWorld`, reach clamp, jumar prop + sling (P1). |
| `Survival/YggViewRig.h/.cpp` | §9 additions. |
| `Survival/SnowTrailComponent.h/.cpp` | `TriggerStep(bool bLeft, uint8 SurfaceHint = 255)` (no 40 cm/s gate; the driver knows it is a step), `SetExternalSteps(bool)` (mutes notify steps), `GetLastGround(uint8& Surface, float& Softness)`. |
| `Survival/ClimberControlsComponent.h/.cpp` | Actions `HandLeft` (LMB / LT) and `HandRight` (RMB / RT) registered with `YggSettings::RegisterKey`; `IsLeftHandHeld()`, `WasLeftHandPressed()`, same for right; `SetBotInput(const FYggClimbBotInput&)`. |
| `Survival/YggSettings.cpp`, `Survival/SettingsWidget.cpp` | The four rows of §4.2. |
| `Survival/YggTelemetryComponent.cpp` | New state values; `met`, `footing`, `balance` appended at the end of the header. |
| `Survival/YggSessionMonitor.cpp`, `Survival/YggCameraRecorder.cpp` | `motion` and `balance` columns; Slip takeover allowed like Fall (no false incident). |
| `Survival/SurvivalHUDWidget.cpp` | State line `[jumar]`, `[ladder · clipped]`, `[ladder · NOT clipped]` (red), key glyph prompts. |
| `Survival/MountainAudioComponent.cpp` | Plays the §10.4 cues (existing gear sounds first, placeholders for the rest). |
| `Config/DefaultGame.ini` | Sections for the three new components; `+Poses=(Name="slide",EyeCm=45,BandCm=10,DownS=0.35,UpS=1.2,YawHalf=70,PitchMin=-55,PitchMax=35,bHideBody=True)`; the new `+Hands` rows; rig: `RollMaxSlipDeg=12`, `AscendLeanFwdCm=10`, `AscendLeanDownCm=5`, `AscendSlopeCapDeg=40`, `BalanceLateralCm=5`, `LadderSagCm=3`. |
| `Tools/Bot/run_bot.ps1`, `Tools/Bot/README.md` | `-Chapter climb -RouteArgs "all"` runs `ygg.Bot.Climb`. |
| `Tools/Audio/synth_*.py` | Placeholder sounds for the missing cues (you judge real ones by ear). |

### 10.3 Console variables and commands

| cvar / command | Default | |
|---|---|---|
| `ygg.Climb.Ascend` | 1 | rope ascending on / off |
| `ygg.Climb.AscendFromDeg` | 30 | start angle (live tuning) |
| `ygg.Climb.Input` | −1 | −1 = the setting; 0 rhythm; 1 hold |
| `ygg.Climb.AutoClip` | 0 | 1 = anchors and hand lines clip by themselves (tests, A4/A5 bots) |
| `ygg.Climb.Ladder` | 1 | ladder mechanic on / off (off = today's slow walk) |
| `ygg.Climb.LadderWind` | −1 | force the ladder wind 0..1 |
| `ygg.Climb.Slips` | 1 | systemic slips on / off |
| `ygg.Climb.ForceSlip` | — | command: slip now (test) |
| `ygg.Climb.Debug` | 0 | on-screen state, footing, balance, breath target, costs |
| `ygg.Climb.Log` | 0 | one log line per state change |
| `stat YggClimb` | — | cost per component |

### 10.4 Sound cues (list only; existing ones first)

Existing: `Gear/Carabiner_ClipIn_*` (clip), `Gear/Carabiner_Click_*` (unclip), `Gear/Rope_Tighten_*`,
`Gear/Rope_Slide_Loop` (arm wrap), `Footsteps/Step_Ice_*` (crampons on hard snow), `Footsteps/Step_Ladder_*` (D5,
placeholder), `Body/Breath_Phrase_*` (the rhythm cue), Sound_Map E3 axe strike, D9 body into snow.

New (placeholders first, real ones need a download and your ear):
1. Jumar sliding up the rope (a ratchet zip).
2. Jumar cam biting under load (a clack) and the rope creaking under weight.
3. The jumar touching an anchor (metal clink on the screw or stake).
4. Crampon catch on a rung (a sharp clank).
5. Ladder flex: a slow aluminium groan; lashings creaking; a rattle in gusts.
6. Hand line twang when it takes your weight.
7. The slot: deep air and drips from below while on a ladder or hanging.
8. Boot scrape (the footing warning).
9. Slide hiss on snow; ice chatter on ice.
10. Pick biting into snow (a crunching drag); screech on rock.
11. Kicked steps (two snow kicks).
12. The heartbeat (Sound_Map F3, still missing; plan block A7) — matters most after a slip.
13. Sangye's grunt or gasp on a slip (your voice, per the Chomolungma direction; optional).

No music cue here. Nothing cute: the slip and the ladder are about fate.

### 10.5 Order of work (7 days)

| Day | Work | Proves |
|---|---|---|
| 1 | `YggClimbTypes`, `SetActivity` and costs, adrenaline, hand inputs + settings rows, rig additions (§9), FixedRope anchors / projection | Builds; `ygg.View.Audit` clean |
| 2 | Rope ascend P0 (states, rhythm, anchors, spent, leash, steps, hands without prop) | C1-C5 |
| 3 | Ladder P0 (clip, balance, wobble, slip → hang / fall, pull-up, unclip; CrevasseCrossing changes) | C6-C8 |
| 4 | Slip and arrest P0 (footing, slip, slide physics, arrest, held, secure, stand-up, tumble, runout, airborne; rope catch; climbing hand-off) | C9-C11 |
| 5 | `UYggClimbTest`; run all; fix; A4 / A5 regression bots; perf | C12-C14 |
| 6 | P1 feel: jumar prop and sling, placeholder sounds, hand spot tuning, messages, HUD line, telemetry columns | screenshots for you |
| 7 | Tuning from the robot tables; your test play; your feel decisions | the plan's "you have played it" |

**P2 (after you play):** figure-8 rappel; "Clip at anchors: Everywhere"; a second hand line; ladder sag material;
world-body clips; Nima's help (holds the ladder = more damping, catches a slip) as hooks for the people system; vertical
ladders on serac walls; frostbitten fingers clip slower (Chapter_Design §4.9).

---

## 11. Robot tests (prove each mechanic)

`ygg.Bot.Climb <case|all>` in the standalone game (`run_bot.ps1 -Chapter climb`). The subsystem finds its own spots:
- **Steep rope:** walks each `AFixedRope` with `SlopeAhead` and takes the first 120 m stretch above `AscendFromDeg` on
  leg 7 (and on leg 2 if one exists).
- **Ladders:** every `ACrevasseCrossing` in the level.
- **Slip slopes:** samples the ground within 200 m of legs 3 and 7 for unroped slopes of 36-42° on soft snow, hard snow and
  ice, each with 40 m of slope and a known runout. Logged with coordinates so a run can be repeated.

Every teleport is followed by the rig's `HardReset` (like `YggPlayBot`). The bot drives the same input path as a player
(`FYggClimbBotInput` + `AddMovementInput`). The ladder bot balances with a simple controller (press against the lean).

| Case | Does | Pass |
|---|---|---|
| C1 ascend_steady | 120 m up the steep rope, rhythm met exactly | No error states; anchors passed = anchors in the stretch; each ClipPast 1.2 ± 0.2 s (normal body); never more than 1.5 m off the rope; stamina at the end ≥ 60; heart mean between walking and sprinting; m/min along and vertical reported |
| C2 ascend_rushed | Same, no breath | Stamina per 10 m ≥ 1.5 × C1; ≥ 1 forced Spent over 120 m; time over 120 m ≥ C1's; SpO₂ min lower than C1's |
| C3 ascend_hold | Same, Hold mode | Finishes; time and cost within ±10% of C1 |
| C4 anchor_block | Holds W and never presses LMB at an anchor for 5 s, then presses it | Before: never past the anchor −10 cm; after: past it within 1.5 s |
| C5 ascend_spent | Sets stamina 0 mid-stretch | Slides ≤ 5 cm in 5 s; push refused with the message; resumes at ≥ 20 |
| C6 ladder_clipped | Every crossing: clip, cross with balance, wind 0.5 | Reaches the far end every time; 0 slips; unclips; crossing time logged; rig roll never over 2° |
| C7 ladder_unclipped_storm | Wind 1.0, no clip, no counter | Slips within 15 s; death cause contains "not clipped" |
| C8 ladder_clipped_storm | Wind 1.0, clipped, no counter | Slip → Hang (no death inside the FallZone); 3 pulls; back on the rungs within 10 s; no lease of owner −3 left |
| C9 arrest_matrix | Each surface × reaction 0.2 / 0.5 / 0.9 / 1.4 s after `ForceSlip` | Soft and hard snow at ≤ 0.5 s stop within 15 m on ≤ 40°; ice at ≥ 0.9 s does not stop before the runout; 1.4 s at > 6 m/s → Tumble. Writes `arrest_table.csv` (surface, angle, reaction, top speed, stop distance, outcome): a thesis-ready figure |
| C10 arrest_release | Releases RMB before kicking; then a run with kicks | Without: slides again within 0.5 s. With: moves ≤ 10 cm in 5 s after release |
| C11 rope_catch | `ForceSlip` while clipped (not on the jumar) | Stops within 2 m; Knock logged; −10 stamina |
| C12 hardreset | Teleport and death in the middle of every state | All B3 states Idle within 0.2 s; no lease of owner −3 (`ygg.View.Audit`); speed shaping normal; tool mode back; telemetry state back to walk |
| C13 regression | `ygg.Bot.Route from 2 to 3` and `ygg.Bot.Summit` with Hold mode, `ygg.Climb.AutoClip 1`, Balance help on | Both finish; times reported against the baselines (A4 walker, A5 20:39 with oxygen); ascend metres per leg reported |
| C14 perf | Each state for 30 s at a fixed spot, B3 on and off (`ygg.Climb.* 0`) | `stat YggClimb` within §12; frame median difference ≤ 0.3 ms |

Checked during every case (from the session monitor and the camera recorder):
- CamRec: no new incident types. Roll ≤ 2° outside the Slip and Knock takeovers.
- Eye ≥ 120 cm over the floor whenever speed > 0.6 m/s and the posture is stand (camera V2); low only in `slide`.
- Pitch within the posture limits.
- Telemetry CSV shows the right `state` values, and `met` / `footing` / `balance` move as expected.
- Heart rate: ascending > walking on the same slope; adrenaline visible for ≥ 10 s after a slip.

---

## 12. Performance budget (game thread, laptop RTX 5070 Ti; total under 0.3 ms in the worst case)

| Part | When | Budget | How |
|---|---|---|---|
| Ascend start check | 5 Hz, inside the existing rope check | < 0.005 ms | `SlopeAhead` over ≤ 6 rope points |
| Footing watch | 10 Hz timer | < 0.005 ms | `CurrentFloor` + the footstep system's cached ground; no trace |
| Ladder idle | — | 0 | overlap events only |
| Ascend active | per frame | ≤ 0.03 ms | projection on ±3 segments, binary search of `AnchorS`, pulse maths, 2 hand targets |
| Ladder active | per frame | ≤ 0.03 ms | balance update, closest point on the hand line, rig feed |
| Slide / arrest active | per frame | ≤ 0.08 ms | 1 line trace (detail tiles use complex collision) + maths; the swept move is CharacterMovement's own cost |
| Rig additions | per frame | ≤ 0.01 ms | inside the rig's 0.05 ms budget (camera V12) |
| Hands IK | per frame while hands are up | existing FirstPersonBody cost | no new solver |

- Only one B3 state is active at a time, so the worst case is the slide (≈ 0.1 ms with the rig). Idle cost ≈ 0.01 ms.
- Components tick only while active (`SetComponentTickEnabled`).
- GPU: jumar + sling = 2 draws; the ladder sag is vertex maths in one material; slide snow spray (P1 Niagara) ≤ 0.1 ms.
- Memory: a few sounds (a few hundred KB) and two small meshes.
- Measured with `stat YggClimb` and the session monitor (C14), per your rule: measure after every change.

---

## 13. Risks and open questions

### Decisions for you

1. **How much slower may the summit get?** Ascending is about half the speed of today's clipped walking on the same
   slope. A5 measured 20:39 to the summit with oxygen. Start at 30° (more climbing, maybe 25-30 min) or 35° (only the
   steepest bits, maybe 22-24 min)? The robot (C13) will give the real numbers for both before you choose.
2. **Clip at anchors default:** "Steep only" (my proposal) or "Everywhere" (Chapter_Design §4.8, many clicks)?
3. **The axe in hand:** always visible as a cane above Base Camp, or only when you slip (it appears on the arrest)?
4. **Jumar and carabiner props:** free download, or Meshy (needs your credits)?
5. **Unclipped ladder = death.** Is that the right tension for the demo, or should an unclipped slip give one last grab?
6. **Sounds:** the new cues need downloads; you judge them by ear (`Tools/Audio/listen_sounds.bat`). Placeholders until then.
7. **Animation clips** for the world body: your Mixamo account, or Meshy credits? P0 works without any.
8. **Rhythm vs Hold default:** is the push-step-step-breathe rhythm fun for you, or should Hold be the default?

### Risks

| Risk | Mitigation |
|---|---|
| The rhythm feels like a quick-time event | No on-screen meter or circles; the cue is the breath; Hold mode; tune after your play |
| Walking mode with a speed pulse jitters on 40-48° rope | The capsule stays on the ground with walkable 58°; fallback = flying mode with a ground-follow (like face climbing) behind a cvar |
| The fixed rope data does not fit (anchors far off the clip-in path) | `AnchorS` projection logs the worst distance; anchors farther than 1 m from the path are ignored and reported |
| Slips surprise players on the trail | Slips only unroped on ≥ 25° ice / 32° hard snow; the demo route is roped; warnings at F 0.6; Gentle setting |
| Robots on A4 / A5 break | `ygg.Climb.AutoClip 1` + Hold mode + Balance help for route bots; C13 runs both every time |
| Hanging capsule inside the crevasse FallZone dies | `OnFall` checks `IsHeldByLine()` (C8 proves it) |
| Hand targets out of reach stretch the arm | Reach clamp; your "never stretch" rule; checked in screenshots |
| A B3 state left on after a teleport or death | `OnHardReset` → Idle; rig watchdog for the slide posture; C12 |
| Arrest view is not the true prone position | Stated simplification for comfort; world-body clip in P2 for the mirror and NPCs |
| Story chapters collide with B3 | B3 never starts while the body is not free; chapter 1 keeps climbing off |
| Unity build name clashes | Unique names in anonymous namespaces (known gotcha) |
| The 7 days slip | P0 per mechanic is shippable on its own (ascend, then ladder, then slip). P1 / P2 wait. |
