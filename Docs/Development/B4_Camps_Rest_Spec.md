# B4 — Camps, rest and acclimatisation; night and headlamp (implementation spec)

Version 0.1 · written 2026-11-20 · block **B4** of `Docs/Development/Development_Plan.md` (4 days, depends on A2).
Status of every proposal here: **[Proposed]** until the user says yes. Numbers marked **[estimate]** are placeholders
to tune; **[cited]** numbers have a source in section 20; **[measured]** numbers were read from the code on 2026-11-20.

Goal from the plan: *the physiology creates decisions (RQ3).* Done when: *a climber who acclimatises does measurably
better (logger plus virtual climber); you understand why without reading the code.*

Words used in this spec:
- **SpO₂**: blood oxygen saturation in percent (the "lungs %" on the HUD).
- **Adapted altitude**: the height the body is used to. It already exists in the game and in the save.
- **Altitude gap**: the altitude you breathe minus your adapted altitude. One number, explained in 4.1.
- **AMS**: acute mountain sickness (headache, sick stomach, bad sleep). **HAPE**: fluid in the lungs.
  **HACE**: swelling of the brain. HAPE and HACE kill if you do not go down.
- **UDS**: the Ultra Dynamic Sky plugin, which owns the sun, moon, stars and the clock.
- **Lids**: the view rig's eyelids (Chapter1_Camera_Spec 6.6). Teleports and time jumps happen behind closed lids.

---

## 0. The block on one page

**What the player lives.** Reach a camp. Crawl into the tent. Melt snow, eat, drink. Choose when to wake. Sleep: the
eyes close, hours pass. Wake to a short morning line that says how the night went and what the body learned. Crawl out.
Over days this becomes the real Everest pattern: **climb high, sleep low**, then the summit push that starts at night
by headlamp.

**The decisions it creates.**
1. Where to sleep tonight: higher (faster, but a bad night and sickness) or lower (slower, but the body adapts well).
2. When to wake: 02:00 for a cold, safe Icefall; 21:00 for summit night; or a full rest day at Base Camp.
3. Whether to go on with a headache, or turn back (AMS → HAPE / HACE).
4. Water: melting snow costs time and gas. No gas means no water, and that is a reason to go down.
5. Light: the headlamp is soft and the night is dark. Battery (if the user wants it) adds the 3 a.m. worry.

**What changes in code.** How the game **uses** the thesis model: when the adapted altitude grows (mostly in sleep),
how good a night is (from the model's own SpO₂), and a new altitude illness that grows from the altitude gap.
A tent interaction built only from the existing view rig postures. A player headlamp component. UDS settings for a
real sun, moon and night. HUD: a body line on the altimeter, a sleep panel, a morning line.

**What does not change.** The thesis equations (`USurvivalComponent::PhysioSpO2`, the CO₂ tables, the acclimatisation
mapping, the thresholds, the bottle drop). `ygg.Physio.Sweep` must print the same table after B4 as before it.
The old per-second acclimatisation rule stays behind a switch so A2 results stay reproducible.

---

## 1. What exists today [measured]

| Thing | Today | Where |
|---|---|---|
| Rest key | R (gamepad D-pad down), only when sheltered (inside 16 m of a camp) | `ClimberControlsComponent.cpp:83-86`, `ExpeditionComponent::Rest` |
| Sleep | Fade to black 1.5 s. Afternoon or night: wake 04:30. Day: +3 h. Stamina full, core 36.6 °C, food −2.5/h, water −3/h, health +5/h, sanity +6/h | `ExpeditionComponent::FinishRest` |
| Acclimatisation | Adapted altitude rises **5 m per real second** resting in a camp, 0.4 m/s otherwise, never above 7,000 m; falls 0.1 m/s when more than 300 m below | `SurvivalComponent.cpp:190-202`, `SurvivalConfig.h:271-295` |
| Sleep and acclimatisation | Sleeping gives **no** acclimatisation | — |
| Camp jump | Sets the adapted altitude to the camp's (max 7,000 m): a jump stands for the climb | `ExpeditionComponent::TeleportToCamp` |
| Food and water in camp | Refilled **for free** at 1 % per second while sheltered | `SurvivalComponent.cpp:232-238` |
| Stove, gas | Do not exist as items (only Ration 0.6 kg, Water 1 kg) | `InventoryComponent.cpp` |
| Headlamp | Story-only: spot on the camera, 35 lm, outer cone 34°, inner 4°, 25 m, no shadows, warm white | `YggStory.cpp:359, 3416-3437` (`ygg.Story.HeadlampLm`) |
| Night | Sanity −0.8/min before 06:00 and after 19:00; shelter restores 8/min | `SanityComponent.cpp:157-160` |
| Clock | UDS "Time of Day" 0..2400, read and set by `YggTime`; about **24 game minutes per real minute** (session monitor, 2026-11-14) | `YggTime.h` |
| Postures | Walk, Lie, Sit, Kneel1, Kneel2, Crawl; Sit with an exit place → Crawl → stand at the exit | `YggViewRig.h`, Chapter1_Camera_Spec 4.2-4.3 |
| Save | Adapted altitude is saved; time of day is **not** saved | `YggSaveGame.h` |
| Data log | No adapted-altitude column | `YggTelemetryComponent.cpp:27` |

### Two findings that shape this spec

**Finding 1: today acclimatisation is almost instant.** Camp 2 takes about 220 real seconds of resting. Rotations
are pointless, and sleeping does nothing for the body. (Also noted in Chapter_Design_v1 4.3.)

**Finding 2: in the model, acclimatisation changes SpO₂ only between Base Camp and Camp 3.** The game maps the gap to
the model's breathing adaptation as `1 − gap / 3000`, never below 0.75 (`AdaptSpanMeters`, `PhysioMinAcclimatised`).
So any altitude 750 m or more above your adapted altitude gives the same SpO₂. Computed from
`Tools/Thesis/physio_model.py` with the game's parameters (resting / walking up a 25° slope, no bottle):

| Adapted to | Camp 1 (6,065) | Camp 2 (6,400) | Camp 3 (7,160) | South Col (7,906) | Summit (8,849) |
|---|---|---|---|---|---|
| 5,364 m (Base Camp) | 71.7 / 65.2 | 68.7 / 61.5 | 62.1 / 53.5 | 54.4 / 44.4 | 43.6 / 32.6 |
| 6,400 m (Camp 2) | 74.8 / 69.1 | 72.6 / 66.3 | 62.1 / 53.5 | 54.4 / 44.4 | 43.6 / 32.6 |
| 7,000 m (the cap) | 74.8 / 69.1 | 72.6 / 66.3 | 66.5 / 58.7 | 54.4 / 44.4 | 43.6 / 32.6 |

So an acclimatised climber is about 5 SpO₂ points better in the Western Cwm and on the Lhotse Face, and **no better
at the South Col or above** (on a bottle: 74.3 vs 74.5). On its own that is too small to drive decisions.
**B4 therefore makes the decisions from two things the model already gives: night SpO₂ (sleep quality) and the
altitude gap (illness).** Whether the floor and span should be recalibrated is a thesis question for A2 and the
supervisor, not for B4 (open question 10).

---

## 2. The real-world basis (short)

- **Rotations.** South-side teams usually make two or three rotations before the summit push: first to Camp 1 or
  Camp 2, then several nights at Camp 2, then a touch or a night at Camp 3 (about 7,100-7,300 m), each time back to Base
  Camp to recover. Camp 3 is the last place where real acclimatisation happens. [cited: Arnette; Alpine Ascents]
- **Speed of adaptation.** Above 3,000 m, medical guidance is to raise the *sleeping* altitude by no more than 500 m a
  night, with a rest day every 3-4 days. [cited: WMS 2019 via CDC Yellow Book] Higher up, adaptation is slower.
- **Sleep at altitude.** Saturation falls in sleep, breathing comes in waves with pauses (periodic breathing), you wake
  often, gasping. Night saturation improves over the first nights at a camp. Studies at Everest Base Camp (5,300-5,364 m)
  measured this with small groups. [cited: Dubowitz 1998; the 5,364 m oximetry study and the 4,559 m study in section
  20 — found by web search, read the papers before citing them in the thesis]
- **AMS, HAPE, HACE.** HAPE: less power, breathless on effort, cough, then breathless at rest, blue lips, pink froth.
  HACE: unsteady walk (ataxia), confusion; often after AMS. Cure: stop going up, go down, oxygen, medicine.
  [cited: Luks 2019 via AAFP]
- **Summit night.** Climbers leave the South Col in darkness, usually in the evening or near midnight, to reach the top
  in the morning and come down in daylight. [cited: Arnette; general knowledge, check exact times]
- **Headlamps.** Real lamps give 200-400 lm at most; climbers run them low to save batteries, carry lithium spares, and
  keep spares warm inside the suit. Cold drains batteries. [general knowledge, check]
- **Camp life.** Hours each evening melting snow on a gas stove; 4-5 litres a day needed; no appetite high up; frost
  from the tent wall falls on the face. Sleeping on 0.5-1 l/min oxygen at Camp 3 and the South Col is common.
  [general knowledge, check]

---

## 3. The loop the player lives

```
 arrive at camp ──> tent door [E] ──> crawl in (kneel, eyes close, inside, sitting)
        │                                   │
        │                     inside: [1] eat  [2] drink  [F] melt snow  [R] sleep...
        │                                   │
        │                     sleep panel: wake 02:00 · 04:30 · 21:00 · rest a full day (Base Camp only)
        │                     + one line: "A good night: 900 m below where you climbed today."
        │                                   │
        │                     lie down, eyes close ── time skips ── body updated (section 4)
        │                                   │
        │                     wake: eyes open, morning line (section 10), still lying, first input sits up
        │                                   │
        └──────────────<─── crawl out [W] to the door, stand up (dawn: +0.4 EV reveal, as in chapter 1)
```

Over days (the rotation loop): **go high → come back down → sleep low → recover → go higher.** A rest day at Base Camp
skips 24 hours. The summit push: sleep at Camp 2/3, then at the South Col wake at 21:00, headlamp on, and climb
through the night into the dawn.

Story first: none of this runs while a story chapter runs (`UExpeditionComponent::IsStoryRunning`). The Dingboche tent
belongs to chapter 1; B4 tents are at Base Camp, Camp 1, Camp 2, Camp 3 and the South Col.

---

## 4. The body: what changes (a game layer on the thesis model)

### 4.1 One number: the altitude gap

`Gap = BreathedAltitude − AdaptedAltitude`. *Breathed altitude* is already in the code: the real altitude, or 2,500 m
lower on a bottle (`SurvivalComponent.cpp:204`). The model already uses this gap for SpO₂. B4 uses the **same gap** for
illness and sleep, so oxygen, going down and acclimatisation all work through one rule the player can learn:
**"stay within about 500 m of what your body knows."** The HUD shows it as a line on the altimeter (section 10).

### 4.2 A game clock for the body

The body counts **game hours**, not real seconds, so it does not depend on the clock speed.
- New `UYggClockSubsystem` keeps `ExpeditionHours` (double, saved): it adds each UDS clock step (wrap at 2400) and
  ignores steps over 0.5 h (sleep and debug jumps add their own hours).
- It also keeps the **high point**: the highest altitude reached in each of the last 48 game hours (a ring of 48
  floats, saved). `HighPoint36` = the highest of the last 36 hours.
- `Skip(Hours)` = the only way to jump the clock: sets UDS time, calls UDS's own **"Instant Time of Day Change"** (found
  in the UDS asset; check its inputs) so the sun, sky light and shadows update at once behind the lids, and tells
  `ARealmSky` to move its weather schedule on (open item: it counts real seconds today).

### 4.3 Acclimatisation comes from nights (`ygg.Accl.Mode 1`, the new default)

Per sleep of `H` game hours at sleeping altitude `S` (breathed: lower on oxygen):

```
Target  = min( max(S, HighPoint36 − StimulusMargin), AdaptMaxMeters )        StimulusMargin 200 m, cap 7,000 m (existing)
FullGain(Adapted) = 400 m at ≤5,000 m · 250 m at 6,000 m · 150 m at ≥7,000 m (straight lines between)   [estimate]
Gain    = FullGain × Q × min(H / 8, 1)                                        Q = sleep quality, 4.4
if Target > Adapted:  Adapted += min(Target − Adapted, Gain)
```

- **Climb high, sleep low** works because the target follows the high point, while sleeping lower gives a better Q.
  Example: climb to Camp 2, sleep at Base Camp: target 6,200 m, Q 0.97. Sleep at Camp 2: target 6,400 m, Q 0.56.
- **Awake**: a small creep of 10 m per game hour while breathed altitude is above adapted (no camp bonus).
- **Loss**: −2 m per game hour while more than 1,000 m below adapted (about −50 m a game day). [estimate]
- The 7,000 m cap stays: nobody adapts to the death zone.
- `ygg.Accl.Mode 0` keeps today's per-second rule exactly (for A2 and `ygg.Physio.Sweep` comparisons).

### 4.4 Sleep quality from the model

```
NightSpO2 = PhysioSpO2(S breathed, Acclim, GapRestTorr) − NightSpO2Drop      NightSpO2Drop 4 points [estimate, from 2]
Q         = clamp( (NightSpO2 − 50) / (76 − 50), 0.2, 1 )
```

Computed now (no oxygen unless marked):

| Adapted to | Base Camp | Camp 1 | Camp 2 | Camp 3 | South Col | South Col on 1 l/min* |
|---|---|---|---|---|---|---|
| 5,364 m | 0.97 | 0.68 | 0.56 | 0.31 | 0.20 | 0.40 |
| 6,400 m | 0.97 | 0.80 | 0.71 | 0.31 | 0.20 | 0.46 |
| 7,000 m | 0.97 | 0.80 | 0.71 | 0.48 | 0.20 | 0.59 |

\* "Sleeping oxygen" = breathed altitude 1,000 m lower. Today the only flow is the 2,500 m bottle; a sleep flow
uses a bottle at a quarter of the rate (open question 11). Chapter_Design_v1 4.5 proposes a flow dial later.

### 4.5 Recovery while sleeping (scaled by Q)

Replaces the fixed numbers in `FinishRest` (B4 mode):

| Stat | Per game hour slept |
|---|---|
| Stamina | full only if Q ≥ 0.6; else wake at `100 × (0.4 + Q)` capped 100 |
| Core temperature | to 36.6 °C (tent and sleeping bag), as today |
| Health | +5 × Q (today +5) |
| Sanity | +6 × Q (today +6) |
| Food / water | −2.5 / −3 (as today); drinking before sleep is the player's job (section 6) |
| Illness load | section 5 |

### 4.6 The three plans, simulated

Same rules, start adapted to 5,000 m at Base Camp, no oxygen except where said. Day = 6 h at the high point.
(The scratch script that made this table becomes `Tools/Thesis/accl_sim.py`, section 12.)

| Plan | Nights | Adapted at South Col arrival | Illness load on the way |
|---|---|---|---|
| A · straight up | BC, C1, C2, C2, C3, SC | 6,345 m | 52 after Camp 2 (AMS), **95 at Camp 3 (HAPE/HACE, near death)** |
| B · one rotation | BC, C1, C2, back to BC + rest day, C2, C2, C3, SC | 6,625 m | 52 after the first Camp 2 night, **32 at Camp 3 (AMS)** |
| C · two rotations, climb high sleep low | B + touch Camp 3 and sleep Camp 2, back to BC + 2 rest days, then C2, C3, SC | 6,831 m | 52 after the first Camp 2 night, **0 at Camp 3** |

Sleeping at the South Col on air adds 90-250 load for everyone: the game says what reality says, sleep there on oxygen
or not at all. With the 2,500 m bottle the South Col night adds nothing.

---

## 5. Altitude illness: AMS, HAPE, HACE (`ygg.Illness 1`)

A hidden load 0..100 in a new `UAltitudeIllnessComponent`. Per game hour:

```
over = BreathedAltitude − (Adapted + 500)
if over > 0:  Load += over / 1000 × (12 asleep | 8 awake)                     [estimate]
else:         Load −= 4    (12 when breathed altitude ≤ Adapted − 300: you went down)
```

Oxygen counts by itself (it lowers the breathed altitude). Going down 500 m clears a bad night in a few hours.

| Load | Stage | What the player notices (in the world, not as a bar) | Game effect |
|---|---|---|---|
| < 30 | well | — | — |
| 30-55 | **AMS** | morning line "Your head pounds."; a slow ache pulse in the lens at wake; rations taste of nothing | eating gives half; Q × 0.8 |
| 55-80 | **HAPE** (65 %) or **HACE** (35 %), chosen once per illness from the run seed | HAPE: breath short at rest, cough (text until a sound is approved), SpO₂ on the HUD lower than the air explains. HACE: the walk sways, the view drifts a little to one side, the compass needle lags, the whispers come early | HAPE: SpO₂ target −6, stamina recovery × 0.5, speed × 0.85. HACE: small random yaw drift of input (≤ 4°/s), sanity drain × 2, rope clip-in distance halved |
| 80-100 | **severe** | the same, stronger; vision greys (existing faint layer) | HAPE: SpO₂ −12; HACE: drift 8°/s; health −0.15/s |
| 100 | collapse | — | death, with a cause that teaches (section 11) |

The SpO₂ change is a game term added **after** the model, like the existing shelter bonus (`PhysioShelterBonus`): a
new `USurvivalComponent::SetOxygenOffset(float)`. The model is not touched. The visuals reuse the body layers that
exist (`M_PP_Body`, the view rig's shiver / spent / hypoxia layers, Sanity's swim and whispers); no new material.
Medicine (dexamethasone for HACE, nifedipine for HAPE, from the Chapter_Design 0.3 kg kit) is a hook: −30 load and
stage effects halved for 6 game hours. In B4 only if time allows.

---

## 6. Water, food and the stove

- **Melt snow** inside the tent: key F (or E at the stove place). F is also the story's hold / flip key
  (`YggStory.cpp:3073, 3343`); no clash, because camp life never runs while a story runs. Each litre skips **15 game minutes** behind a short
  lids-half-closed fade and uses **1/8 of a gas cartridge**. [estimate, Chapter_Design 4.4] Hydration +40 per litre
  (one water item). Up to 4 litres in one go (a choice in the panel: 1 / 2 / 4).
- **Eat**: the existing ration item (key 1). AMS halves it.
- **Gas**: new item `Gas` (0.45 kg cartridge). Without gas: "No gas. No water tonight." The stove glow is the tent's
  light (section 9).
- **Camp stock** (B4 minimum): each B4 camp has a fixed stock of rations, gas and bottles, listed in a new
  `Content/Data/CampStocks.json` (or a data table), shown in the tent panel ("Camp 2 stock: 3 rations · 2 gas · 1
  bottle"). Taking moves items to the pack, saved with the game. Load carries that refill stocks are **later**
  (Chapter_Design 4.4), not B4.
- **Free refill** off when the stove is on: `ygg.Camp.FreeRefill 0` (1 = today's free 1 %/s).

---

## 7. Time of day and night (UDS settings first, not code)

Rule from the user (memory `uds-settings-first`): change the UDS actor in `base_map` before writing any code. Set the
values with a small editor script (new `Tools/Art/apply_night_sky.py`, same style as `apply_sky_art.py`). Property
names below were found in the UDS asset with a string search; confirm each in the editor first.

| UDS setting | Value to try | Why |
|---|---|---|
| Simulate Real Sun / Simulate Real Moon / Simulate Real Stars | on | sun, moon and stars where they really are over Everest |
| Latitude / Longitude / Time Zone | 27.988 / 86.925 / +5.75 | the summit; Nepal time |
| Year / Month / Day | a May date in the pre-monsoon window (open question 9: moon phase) | sunrise ~05:10, sunset ~18:40 |
| Day Length / Night Length | keep today's speed (about 24 game min per real min); see below | clock speed |
| Night Brightness | low; tune so ridges read as dark shapes against the stars | night stays night |
| Exposure Bias Night | negative; tune so auto-exposure does not lift the night to grey | the headlamp must matter |
| Moon Light Intensity / Render Moon Directional Light | keep **off** (the 2026-11-06 perf setting) unless a measured test passes (section 17) | a shadowed moon light costs shadow-map time |
| Stars Intensity | tune high at altitude | the summit-night sky is a reward |
| Sky Light Intensity Multiplier In Interiors (+ a UDS Occlusion Volume in each tent) | dim inside | the tent is dark until the stove or lamp is on |
| Lights Update Period / Degree Threshold | keep 4 / 0.4 | the perf fix of 2026-11-06 |

**Clock speed.** At today's ~24 game minutes per real minute, the A5 robot's 20.6-minute summit climb from the South Col
lasts about **8 game hours**: leave at 22:00, summit near 06:00, which is what real climbers do. So B4 keeps today's
speed. Chapter_Design_v1 proposed 6 game min per real min for the content-heavy chapters; that is open question 3.
Night Length can make nights pass slower than days if the summit night needs more real minutes.

**Sleeping jumps the clock** only through `UYggClockSubsystem::Skip` behind closed lids, then UDS's "Instant Time of
Day Change", then the lids stay closed until the sky light has updated (at least one Lights Update Period, 4 s, or one
frame after the instant change if it really forces it; measure). The hitch is never seen.

What night already does: sanity drains (Sanity), seracs are calmer before 08:00 (Chapter_Design 4.6, measured), cold
grows with the realm and the air. B4 adds no new night penalty: darkness itself is the cost.

---

## 8. The headlamp

**One light, one owner.** New `UYggHeadlampComponent` on the player (created at runtime, like the view rig). The story's
`{"light": "headlamp"}` calls it instead of making its own spot, so chapter 1 and the climb share the same lamp.
`ygg.Story.HeadlampLm` stays as an alias of `ygg.Headlamp.Lm` so the accepted chapter-1 tuning does not move.

| Property | Value | Note |
|---|---|---|
| Low (the baseline) | **35 lm**, outer 34°, inner 4°, source radius 3 cm, 25 m, warm white (1.0, 0.94, 0.86) | today's accepted chapter-1 look; 600 lm burned a white disc (2026-11-20) |
| High (optional, open question 2) | 90 lm, same cone | finding the route in a storm; drains 3× faster |
| Red tent mode | 5 lm, red, 2 m | inside the tent; does not blind Nima |
| Shadows | **off** | a shadowed light costs 0.5-1 ms of virtual shadow maps |
| Volumetric scattering | 0 (test 0.3 behind `ygg.Headlamp.Scatter`) | spindrift in the beam is beautiful but costs fog time |
| Attach | the camera (rides the true eye, Chapter1_Camera_Spec "anything attached to the camera") | already so in the story code |
| Key | **H** (free today), rebindable in Settings; a free gamepad button | new row in `YggSettings` Registry |

- **No auto-on.** At dusk (sun below the horizon, outside a tent) one hint: "It is getting dark. [H] headlamp."
- **Battery** (open question 1; default on, gentle): charge 0-100 %. Low drains 100 % in 16 game hours (two nights);
  High in 5. Below −20 °C air: × 1.5. Spare `Battery` item (0.1 kg, 2 at the start); the empty lamp swaps one in by
  itself, like the oxygen bottle: "Changed the headlamp battery (1 left)." Under 10 % the beam dims to 60 % and flickers
  for a moment every 20-40 s. With no light at night you can still see the stars, the snow and the ridge shapes.
- **Other climbers' lamps** on the Lhotse Face and the summit ridge (the line of lights): small emissive points, no
  light components, later (B7). Mentioned so nobody adds 30 real lights.

---

## 9. The tent (reuse the chapter-1 view rig postures)

**New actor `AYggCampTent`** (`Source/YggdrasilsPeak/Survival/YggCampTent.h/.cpp`): a tent mesh (the Meshy
`/Game/Story/AI/tent_dome` or the camp's existing tent) and five scene points: `Door` (outside, facing in), `Inside`
(where the eye sits), `Out` (the crawl-out end, the chapter-1 `tent_out`), `Stove` (with the Meshy `stove_set` mesh)
and `Lamp`. One per B4 camp, placed by a new `Tools/RoutePlanning/place_player_tents.py` (run with the editor, by the
agent that owns it; traces the ground, checks the capsule fits, as chapter 1 did).

**Crawl in** (owner id **−3** = camp system; −1 is the posture system, −2 the fallbacks):
1. Within 1.5 m of `Door` and looking at it: prompt "[E] Crawl in". E is shared: a clue in range wins (as today).
2. `RequestPose(Kneel1, face = door)` 0.9 s → `SetLids(Closed, 0.35)`.
3. On the closed frame: the tent's mesh ignores the Pawn channel; teleport the capsule to `Inside`;
   `HardReset(facing the door, Sit)`; `RequestPose(Sit, snap, Exit = Out, Prop = sleeping_bag)`; take a `Move` lease
   with `keep`, so moving does not stand you up inside.
4. `SetLids(Open, 0.6)`. The stove glow and the UDS interior settings make the inside warm and dim.

**Inside** (sitting): eat, drink, melt snow, look at the stock, sleep. Release the Move lease when the player chooses
"go out" or presses W for 0.3 s with no panel open: then the existing Sit → Crawl → stand at `Out` runs, with
`exit_ev 0.4` when the sun is up (the dawn reveal). The 90 s blink fallback stays.

**Sleep**: `RequestPose(Lie)` 2.5 s → `SetLids(Sleep, 2.5)` → on closed: apply the night (sections 4-5), `Skip(H)`,
write the save → hold the lids (section 7) → `SetLids(Wake, …)` → still lying; the first input sits up (existing
Lie → Sit 2.2 s) → the morning line.

**Cancel**: death, respawn or a story start ends tent mode: `HardReset` (already in `ReleaseStoryAndView`), the tent's
Pawn collision comes back, stove light off. Everything the camp system holds is released with `ReleaseOwner(−3)`.

**Camps without an `AYggCampTent`**: R keeps today's `Rest()` behaviour (fade, time skip), now with the B4 body rules.

**Lights inside**: one point light at `Stove`, 40 lm, warm, flicker 0.9-1.0, radius 3 m, no shadows, only while
inside; plus the red headlamp mode. Outside nothing extra.

---

## 10. HUD and feedback

The look is the user's call; this is what each piece must say. Everything sits in the existing HUD v8 language.

1. **Body line on the altimeter.** A thin tick at the adapted altitude ("your body knows this"). The 500 m above it is
   a soft band. Above the band the altitude digits warm in colour. One glance tells the gap. Drawn in
   `USurvivalHUDWidget::NativePaint` next to the altimeter.
2. **Sleep panel** (in the tent, R): 3-4 rows, keys 1-4 or the mouse.
   ```
   SLEEP UNTIL    02:00  Icefall start (cold, quiet ice)
                  04:30  dawn
                  21:00  summit night
                  rest a full day          (Base Camp and below)
   Camp 2 · 6,400 m — a poor night: you are 1,000 m above what your body knows.
   ```
   The last line is the teaching line, from the same numbers as the night itself (Q and the gap):
   good night / poor night / dangerous night, and why.
3. **Morning line** (the existing message area, 6 s), for example:
   - "Camp 2 · 6,400 m. You woke nine times, gasping. Your body now knows 5,900 m (+210). Your head pounds."
   - "Base Camp. Thick air after the Cwm. A deep sleep. Your body now knows 6,200 m (+310)."
4. **Clock.** A wrist-watch glance (open question 7): time, altitude, the day number ("Day 12 · 04:31").
5. **Headlamp battery**: only when below 25 %, a small lamp mark beside the oxygen reading.
6. **Illness** is shown in the world (section 5) and in the morning line, never as a bar.
7. All lines are short and plain (the user reads English as a second language); all go through `SayAmbient`, so a
   story chapter can hold them.

---

## 11. Failure

| How it goes wrong | What happens | The death line (it must teach) |
|---|---|---|
| Sleeping too high (plan A) | bad nights (Q ≤ 0.6), AMS in the morning, HAPE or HACE on the next night high | "HAPE — fluid in your lungs. Going down 500 m would have saved you." |
| | | "HACE — your brain swelled. Going down, or oxygen, would have saved you." |
| Pushing on with AMS | load keeps growing while you stay high | (as above) |
| South Col night on air | load +5-10 per hour; Q 0.2 | "You slept in the death zone without oxygen." |
| No water | dehydration damage (existing) | existing line, plus the morning hint "No gas. No water tonight." the day before |
| Lamp dead at night | no damage; only darkness, slower, sanity | — |
| Turnaround (optional) | after 12:00 on summit day below the summit: "It is past noon. Every climber who stays up here this late gambles." once | — |

Respawn after an illness death: at the last camp (existing rule), with the load reset to 30 and the adapted altitude
as it was, so the player can choose to go down. Open question 8 asks whether illness may kill at all.

---

## 12. File-by-file implementation plan

Read Chapter1_Camera_Spec before touching the view rig. Build rules: editor closed for C++ builds (Development_Plan 9).

| File | Change |
|---|---|
| `Survival/SurvivalConfig.h` | New category "Acclimatisation (B4)": `StimulusMarginMeters 200`, `NightGain5000 400`, `NightGain6000 250`, `NightGain7000 150`, `AwakeGainPerGameHour 10`, `LossPerGameHourFarBelow 2`, `LossFarBelowMeters 1000`, `NightSpO2Drop 4`, `SleepQualityLow 50`, `SleepQualityGood 76`, `SleepQualityMin 0.2`. Category "Illness": `IllnessToleranceMeters 500`, `IllnessAsleepPerKmHour 12`, `IllnessAwakePerKmHour 8`, `IllnessRecoverPerHour 4`, `IllnessRecoverLowPerHour 12`, stage thresholds 30/55/80, `HapeShare 0.65`, `HapeSpO2Drop 6/12`. **Do not change** any existing Physiology value. |
| `Survival/SurvivalComponent.h/.cpp` | `ApplySleep(Hours, SleepAltitude, bOnOxygen) → FYggSleepReport` (4.3-4.5); `SetOxygenOffset(float)` (illness); awake creep and loss in game hours when `ygg.Accl.Mode 1`; today's block kept intact under `Mode 0`; `GetAltitudeGap()`, `GetBreathedAltitude()`. |
| `Survival/YggClock.h/.cpp` (new) | `UYggClockSubsystem` (world subsystem): `ExpeditionHours`, `Day`, the 48-hour high-point ring, `Skip(Hours)` with UDS "Instant Time of Day Change" by reflection, an event for skips; `ygg.Time <hhmm>`, `ygg.Time.Skip <h>`. Uses `YggTime` (keeps its cached sky lookup). |
| `Survival/AltitudeIllnessComponent.h/.cpp` (new) | Load, stage, HAPE/HACE pick (seeded), effects through existing hooks (SurvivalComponent offset and speed, view rig body layers, Sanity drain, ClimberControls yaw drift), death cause text; `ygg.Illness`, `ygg.Illness.Set <load>`, `ygg.Illness.Show <0-3>`. |
| `Survival/YggCampTent.h/.cpp` (new) | `AYggCampTent` with the five points, the stove mesh and light, the interior volume reference, `SetPawnBlocking(bool)`. |
| `Survival/YggCampLife.h/.cpp` (new) | `UYggCampLifeComponent` (added at runtime by `UExpeditionComponent`, like Sanity): tent state machine (Outside → Entering → Inside → Sleeping → Waking → Leaving), the panel actions, stove and stock, the sleep report and the morning line; runs at 2 Hz outside tents. |
| `Survival/ExpeditionComponent.cpp` | `Rest()`: in a tent → the sleep panel; sheltered but no tent → today's rest with the B4 body rules; E routes to the tent door when no clue is in range; `TeleportToCamp`: obey `ygg.Accl.JumpGrants` and `ygg.Accl.JumpPlan`; save and load the new fields; release camp owner −3 in `ReleaseStoryAndView`. |
| `Survival/YggHeadlamp.h/.cpp` (new) | `UYggHeadlampComponent`: modes, battery, cold, flicker, dusk hint; `ygg.Headlamp`, `.Lm`, `.HighLm`, `.Battery`, `.Scatter`. |
| `Story/YggStory.cpp` | `SetLight("headlamp")` uses `UYggHeadlampComponent` (on/off, fade); `ygg.Story.HeadlampLm` forwards to `ygg.Headlamp.Lm`. |
| `Survival/InventoryComponent.cpp` | Items `Gas` (0.45 kg), `Battery` (0.1 kg); pack screen lines for them. |
| `Survival/ClimberControlsComponent.cpp`, `YggSettings.cpp` | Keys: Headlamp H, Melt snow F (inside only), panel 1-4 while the panel is open; HACE yaw drift input hook. |
| `Survival/SanityComponent.cpp` | Read the illness drain multiplier; nothing else. |
| `Survival/SurvivalHUDWidget.cpp` | Body line and band on the altimeter; sleep panel; battery mark; watch glance. |
| `Survival/YggSaveGame.h` | New fields (section 14). **Do not bump `Version`**: older saves would be ignored (the user's save is lost). New fields load as defaults. |
| `Survival/YggTelemetryComponent.cpp` | New columns (section 15), appended after `events`. |
| `Survival/YggRotationBot.h/.cpp` (new) | `ygg.Bot.Rotation` (section 16). `YggSummitBot`: add `time <hhmm>`. |
| `Tools/Art/apply_night_sky.py` (new) | UDS night and real-sun settings (section 7), run by the agent with the editor. |
| `Tools/RoutePlanning/place_player_tents.py` (new) | Places `AYggCampTent` at the five B4 camps, ground-traced, capsule fit checked. |
| `Tools/Thesis/accl_sim.py` (new) | Python twin of 4.3-4.5 and 5 (imports `physio_model.model`); the three plans; CSV + chart `Saved/Review/b4_rotations.png`; seed of B1's fast mode. |
| `Tools/Thesis/plot_session.py` | Plot `adapt_m` and `high_m` against `alt_m` (the "climb high, sleep low" staircase), illness load, sleep events. |
| `Content/Data/CampStocks.json` (new) | Stock per camp. |

**Order of work (4 days).**
- Day 1: clock subsystem, `ApplySleep`, the night rule, sleep quality, recovery; `accl_sim.py`; T1 passes.
- Day 2: illness component and effects; telemetry and save fields; T2 (rotation robot) passes.
- Day 3: tent actor, camp life, stove and stock, HUD pieces; T3, T5, T6 pass.
- Day 4: headlamp component and story migration; UDS night settings; night summit run T4; perf; T7; the user's
  test play.

---

## 13. Console variables (new)

| Cvar | Default | Meaning |
|---|---|---|
| `ygg.Accl.Mode` | 1 | 1 = acclimatisation from nights (B4); 0 = today's per-second rule |
| `ygg.Accl.JumpGrants` | 1 | a camp jump sets the adapted altitude like today; 0 = no free adaptation (tests) |
| `ygg.Accl.JumpPlan` | 0 | what a camp jump stands for: 0 today's rule, 1 straight up, 2 one rotation, 3 two rotations (uses the 4.6 rules) |
| `ygg.Accl.Report` | — | prints adapted, HighPoint36, gap, load, the last five nights |
| `ygg.Sleep <hhmm\|hours> [force]` | — | sleep now (force = outside a camp, tests only) |
| `ygg.Sleep.Sim <plan\|all>` | — | prints the 4.6 table from the engine's own functions (compared with `accl_sim.py`) |
| `ygg.Illness` / `.Set <load>` / `.Show <0-3>` | 1 / — / −1 | illness on/off; set the load; force a stage's look (like `ygg.Body.*`) |
| `ygg.Camp.FreeRefill` | 0 | 1 = today's free food and water in camps |
| `ygg.Stove.MinutesPerLitre` / `.LitresPerCartridge` | 15 / 8 | stove cost |
| `ygg.Headlamp` | −1 | −1 player's choice, 0 off, 1 on |
| `ygg.Headlamp.Lm` / `.HighLm` / `.Battery` / `.Scatter` | 35 / 90 / 1 / 0 | baseline, high, battery on/off, volumetric scattering |
| `ygg.Time <hhmm>` / `ygg.Time.Skip <h>` | — | set or jump the clock through the clock subsystem |
| `ygg.Tent.Test <camp>` | — | the tent robot (T3) |
| `ygg.Bot.Rotation <A\|B\|C> [fast] [walk]` | — | the rotation robot (T2) |

Existing ones kept: `ygg.Physio`, `ygg.Physio.Sweep`, `ygg.Physio.Adapt`, `ygg.Log`, `ygg.Story.HeadlampLm` (alias).

## 14. Save game (new fields, same `Version`)

`TimeOfDay`, `ExpeditionHours`, `HighPointRing[48]`, `IllnessLoad`, `IllnessKind`, `HeadlampCharge`, `HeadlampOn`,
`CampStocks` (camp name → item counts), `LastNights` (five short sleep reports, for the journal later).

## 15. Data log (new columns, appended)

`adapt_m, high_m, gap_m, illness, illness_kind, asleep, sleep_q, headlamp, battery, exp_hours`. Each sleep also writes
an event `sleep:<camp>:<hours>:<Q>:<gain>`.

---

## 16. Robot-player tests

| # | Test | How | Pass |
|---|---|---|---|
| T1 | The rules are deterministic | `ygg.Sleep.Sim all` vs `python -I Tools/Thesis/accl_sim.py` | every night's adapted altitude within 1 m, load within 0.5 |
| T2 | **Acclimatising pays** (the "Done when") | `ygg.Bot.Rotation A`, `B`, `C` with `JumpGrants 0`: teleports camp to camp, advances the clock by the measured leg times, sleeps as planned, then **walks the Lhotse Face (route leg 5, Camp 2 → Camp 3)** with `ygg.Bot.Route from 5 to 5`, then sleeps at Camp 3 and the South Col (on a bottle) | C reaches the South Col with load < 30; A has HAPE/HACE by Camp 3; on leg 5, C's mean SpO₂ ≥ A's + 4 and its time ≤ A's; no death in C. Report `Saved/Bot/rotation_<time>/report.md` + the staircase chart |
| T3 | The tent works | `ygg.Tent.Test <camp>` at each B4 camp: door → crawl in → melt 2 l → sleep to 04:30 → crawl out | posture order Walk → Kneel1 → Sit → Lie → Sit → Crawl → Walk; eye inside each band (`ygg.View.Audit`); 0 flight-recorder incidents; no blink fallback; tent collision back on; panel shown and hidden |
| T4 | Summit night by headlamp | `ygg.Bot.Summit time 2200` with the lamp on | summit before 08:00 game time; battery never empty with one spare; screenshots at 22:30, 01:00, 04:00, 05:30, 07:00 for **the user's eye**; FPS in section 17 |
| T5 | Save and continue | sleep, quit, continue | clock, day, adapted, high points, load, battery, stock identical |
| T6 | Stove | melt 2 l, then try with no gas | 30 game min skipped, gas −0.25, hydration +80; then the "No gas" line |
| T7 | Nothing else broke | `ygg.Physio.Sweep` (same output), `ygg.Bot.Run chapter1 fast`, `ygg.Bot.Summit` in daylight | sweep unchanged; chapter 1 at least as good as now (24/25); summit time within 5 % of 20:39 |

The rotation robot reuses the virtual-climber idea (B1): its plans are the first three hand-made strategies, and its
CSV rows have the same shape B1 will use (success, time, time above 8,000 m, lowest SpO₂, cause).

---

## 17. Performance budget

Target (Development_Plan 9): 60 fps or more at 1080p High at the worst spot of the demo route, laptop RTX 5070 Ti.
Measure before and after with `Tools/Art/perf_run.ps1`, standalone, at the worst spot by day and at the South Col at
night with the lamp on.

| Item | Budget | How it stays cheap |
|---|---|---|
| Headlamp (spot, no shadows) | ≤ 0.10 ms GPU | no shadows, scattering 0, one light |
| Stove light | ≤ 0.05 ms, only inside | radius 3 m, no shadows, off outside |
| Night sky settings | ≤ +0.3 ms vs day | moon directional light stays off unless measured; stars are sprites |
| Moon light, if the user wants it | ≤ 0.3 ms, shadows off | test once, keep only if it fits |
| UDS occlusion volumes in tents | ≤ 0.05 ms CPU | volume mode, not trace mode |
| Camp life, illness, clock | ≤ 0.05 ms CPU total | 2 Hz checks; the body maths runs in the existing fixed step; no actor iteration per frame (`YggTime` caches the sky) |
| Time skip | hitch hidden | done behind closed lids; lids open only after the sky light update |
| Memory | no new texture over 1k | reuse Meshy tent and stove; no new materials (illness uses `M_PP_Body`) |

---

## 18. Risks and gotchas

- **Save version**: never bump `UYggSaveGame::Version` for new fields (older saves are ignored).
- **Story ownership**: camp life must check `IsStoryRunning()`; the Dingboche dawn tent is a story tent.
- **Lights update period 4 s**: after a time jump the sun can step visibly; keep the lids closed or use the UDS instant
  change.
- **Weather schedule in real seconds**: an 8-hour sleep must also move the realm weather on, or the same storm waits
  outside after a night.
- **Camp jumps** stand for rotations in the thesis build; with `JumpGrants 1` they still give adaptation for free.
  Tests set it to 0.
- **The tent and the capsule**: a 1.9 m capsule does not fit a dome tent; the mesh ignores the Pawn channel only while
  the camp system holds the player, and walking is impossible while posed.
- **Editor scripts**: never put the file extension text ".py" in a uepy script's first comment lines.
- **Sounds** (stove hiss, cough, battery click): none go in before the user has listened (`feedback-sound-by-ear`).

---

## 19. Open questions for the user

1. **Headlamp battery**: gentle battery with spares (default), or no battery at all?
2. **High beam**: only the soft 35 lm light, or also a 90 lm high beam that drains faster?
3. **Clock speed**: keep today's (summit night already lasts about 8 game hours), or slow the day for the story chapters?
4. **Free food and water in camps**: switch off (stove, gas and stock decide), or keep free?
5. **Camp jump in the thesis build**: let the player choose how they "acclimatised" (straight up / one / two
   rotations; a nice study variable), or always arrive well acclimatised?
6. **Body line on the altimeter**: yes, and how it should look (your call by eye).
7. **Clock on screen**: a wrist-watch glance key, always on, or only in the tent?
8. **May illness kill**, or only force you down (collapse, then wake lower with help)?
9. **Moonlight**: a real moon by date (a full moon on summit night is bright and beautiful), or a moonless dark night?
10. **For A2 and the supervisor, not B4**: the model's floor (0.75) and span (3,000 m) make acclimatisation irrelevant at
    the South Col and above. Recalibrate in A2, or keep and explain?
11. **Sleeping oxygen**: allow a low flow (a bottle at a quarter rate, about 1,000 m lower) at Camp 3 and the South Col
    before the flow dial exists?

---

## 20. Sources

- Arnette A. Everest climbing guides and rotation descriptions: https://www.alanarnette.com/?p=25602
- Alpine Ascents, Everest itinerary: https://www.alpineascents.com/climbs/mount-everest/itinerary/
- CDC Yellow Book, High-altitude travel and altitude illness (WMS 2019 sleeping-altitude rule):
  https://www.cdc.gov/yellow-book/hcp/environmental-hazards-risks/high-altitude-travel-and-altitude-illness.html
- Luks AM et al. WMS guidelines, summarised in AAFP 2020 (HAPE / HACE signs, treatment):
  https://www.aafp.org/afp/2020/0415/p505.html
- Dubowitz G. Temazepam and sleep at 5,300 m, Everest Base Camp (night saturation, 11 people), BMJ 1998:
  https://pmc.ncbi.nlm.nih.gov/articles/PMC28461/
- Overnight oximetry at South Base Camp, 5,364 m (five people, 10-day stay), Sleep and Breathing 2015:
  https://link.springer.com/article/10.1007/s11325-015-1205-z
- Night breathing and saturation over the first nights at 4,559 m (2012): https://pmc.ncbi.nlm.nih.gov/articles/PMC3274343
- These three were found by a web search on 2026-11-20; the author names and details still need checking against the
  papers.
- The model itself: `Docs/Thesis/Physiology_Model.md`, `Tools/Thesis/physio_model.py` (West 1996, Severinghaus 1979,
  Grocott 2009).
- Game design background: `Docs/Development/Chapter_Design_v1.md` sections 2.3, 4.1-4.6 (proposals this spec builds on).
