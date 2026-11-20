# Physiology model in the game: integration spec (Development Plan A2, game side)

Version 0.1 · 2026-11-20 · status: **proposal, nothing built yet**.
Code read at commit `fbcec150`. Bot data: tonight's runs in `Saved/Bot/summit_20261120_*`.
This spec does **not** change the model itself (the oxygen chain in `Tools/Thesis/physio_model.py`). It covers what sits
between the model and the player: the game layer, saving, timings, cold, tests and your decisions.

---

## 0. Summary

1. **The model is already in the game.** Commit `1a2bc993` (2026-11-14) ported the model's equations to C++.
   Commit `7efa99f2` retuned it. It is on by default. Tonight's A5 timings were measured **with** it.
2. **The "100 % at Base Camp" finding is about the old straight-line rule.** That rule is still in the code. The console
   command `ygg.Physio 0` brings it back. `Docs/Thesis/Physiology_Model.md` section 1 still describes the old rule as
   "the game today" (section 2 is the 2026-11-13 log, from before the port). That write-up needs an update (not done
   here; listed in section 11).
3. Between the model and the player sits a **game layer** of about ten design numbers: a breathing floor of 0.75, no
   adaptation above 7,000 m, a bottle worth 2,500 m, +4 points in camp, and others (section 2). Most of what the player
   feels in the death zone comes from these numbers, not from the model. The thesis must name them as **[game]**.
4. **Four gaps:** sleeping at a camp gives no acclimatisation. Every climber's core settles at about 33 °C. The heart
   rate hits its 188 bpm cap in the death zone. The bottle's 2,500 m has no source.
5. **Demo timings:** the 20-minute oxygen summit is robust. The no-oxygen death at 8,552 m is fragile: small changes
   in the A2 calibration move it by 300–700 m (section 7).

## 1. Words used here

| Word | Meaning |
|---|---|
| SpO₂ | Blood oxygen saturation, in %. The HUD's oxygen number. |
| PaCO₂ | CO₂ pressure in the lungs and blood (Torr). Breathing harder lowers it, which lets more oxygen in. |
| Effort gap | The model's alveolar–arterial gap (AaD): oxygen lost between the lungs and the blood. Work makes it bigger. |
| Adapted altitude | The altitude the body is used to (metres). The game's saved acclimatisation state. |
| Acclimatisation factor | 0 = just arrived, 1 = fully adapted. The model's input. The game derives it from the adapted altitude. |
| Felt altitude | The altitude the lungs "feel". A bottle lowers it. |
| Breathing floor | The lowest acclimatisation factor the game allows (0.75). |
| Damage line | The altitude where SpO₂ first reaches the damage threshold (40 %). |

## 2. How the game computes it today

```
altitude ─► felt altitude (bottle on: altitude − 2,500 m)
          ─► acclimatisation factor = 1 − (felt altitude − adapted altitude) / 3,000 m, kept between 0.75 and 1
          ─► MODEL: air pressure ─► inspired O₂ ─► alveolar O₂ ─► minus effort gap ─► blood pH ─► Severinghaus SpO₂
          ─► +4 in a camp shelter ─► moves towards that target at 1.5 points per second ─► SpO₂ stat
          ─► thresholds: fatigue 72 · auto bottle 58 · warning 52 (off at 57) · damage 40
          ─► effects: speed down to ×0.7, stamina recovery down to ×0.5, damage 0.6 health/s after 10 s, screen tunnel, heart rate
```

Code: `SurvivalComponent.cpp` (`PhysioSpO2`, `StepSimulation`), numbers in `SurvivalConfig.h` (category Physiology),
heart rate in `ExpeditionComponent.cpp` (`UpdateScreenEffect`). The model step runs 10 times per second.

**The game layer** (all **[game]**: design choices, not physiology):

| Number | Value | What it does |
|---|---|---|
| `PhysioMinAcclimatised` (breathing floor) | 0.75 | The factor never drops below 0.75. Above 7,750 m without a bottle it is always 0.75. |
| `AdaptMaxMeters` | 7,000 m | Nobody adapts higher than this. |
| `AdaptSpanMeters` | 3,000 m | 3,000 m above your adapted altitude you breathe like someone just arrived (before the floor). |
| `StartAdaptedMeters` | 4,400 m | A new game starts adapted to Dingboche. |
| `AdaptRestMetersPerSecond` / `AdaptMovingMetersPerSecond` | 5 / 0.4 m/s | Adaptation speed: resting in a camp / anywhere else. |
| `AdaptLossMetersPerSecond` | 0.1 m/s | Adaptation lost while more than 300 m below it. |
| `BottleAltitudeDropMeters` | 2,500 m | A bottle lowers the felt altitude by this. No flow setting. |
| `PhysioShelterBonus` | +4 points | Resting in a camp shelter. |
| Effort gap rest / walk / sprint / per grade | 5 / +2 / +5 / +3 Torr | Work lowers SpO₂. On a fixed rope the grade counts half. |
| Thresholds | 72 / 58 / 52–57 / 40 | Fatigue, auto bottle, warning, damage. |
| `OxygenBottleSeconds` | 1,800 s | One bottle lasts 30 minutes (time is compressed; a real summit day is many hours). |

## 3. Side by side: old rule, model, game today

"Game today" = model + game layer, for a climber adapted as far as play allows (adapted to the altitude, at most
7,000 m). Walking = flat ground (effort gap 7 Torr). Computed with `physio_model.py` and a copy of the game's rules;
bot values are measured (walking on the fixed rope, effort gap about 7.7 Torr).

### 3a. Resting, no bottle (SpO₂ %)

| Altitude | Old rule | Old rule, in camp | Model, adapted | Model, just arrived | Game today | Game today, in camp | Air pressure (Torr) |
|---|---|---|---|---|---|---|---|
| 4,400 m (Dingboche) | 100.0 | 100.0 | 83.9 | 76.9 | 83.9 | 87.9 | 452 |
| 5,300 m (Base Camp) | 92.3 | 100.0 | 79.7 | 67.8 | 79.7 | 83.7 | 404 |
| 6,400 m (Camp 2) | 81.7 | 89.7 | 72.6 | 52.7 | 72.6 | 76.6 | 351 |
| 7,900 m (South Col) | 67.2 | 75.2 | 61.4 | 29.4 | 54.5 | 58.5 | 287 |
| 8,849 m (summit) | 58.0 | 66.0 | 52.2 | 17.4 | 43.6 | 47.6 | 253 |

Up to 7,000 m "game today" equals "model, adapted". Above it the game sits between the two model curves, because
adaptation stops at 7,000 m and the floor holds the factor at 0.75. Check against a measurement: at 8,400 m the model,
adapted, gives 57.0 %; Grocott et al. 2009 measured 54.0 % **[cited, see Physiology_Model.md]**.

### 3b. Walking, no bottle (SpO₂ %)

| Altitude | Old rule | Model, adapted | Game today | Bot tonight |
|---|---|---|---|---|
| 4,400 m | 100.0 | 82.1 | 82.1 | — |
| 5,300 m | 91.2 | 77.2 | 77.2 | — |
| 6,400 m | 79.0 | 69.0 | 69.0 | — |
| 7,900 m | 62.5 | 56.3 | 48.7 | 47.5 |
| 8,849 m | 52.0 | 46.1 | 37.1 | died at 8,552 m (38.7 at 8,501 m) |

### 3c. With bottled oxygen (SpO₂ %)

The model has no bottle term yet (**[to model]** in the write-up). The game uses the felt-altitude stand-in.

| Altitude | Old rule, rest | Old rule, walk | Game today, rest | Game today, walk | Bot tonight (walk) |
|---|---|---|---|---|---|
| 4,400 m | 100.0 | 100.0 | 93.9 | 93.4 | — |
| 5,300 m | 100.0 | 100.0 | 90.9 | 90.0 | — |
| 6,400 m | 100.0 | 100.0 | 86.4 | 84.9 | — |
| 7,900 m | 99.2 | 94.5 | 79.1 | 76.5 | 76.0 |
| 8,849 m | 90.0 | 84.0 | 72.9 | 69.4 | 67.7 (at 8,800 m) |

Check: standing in the summit shelter with a bottle on, the bot read 77.0 %. The game's rule gives 72.9 + 4 = 76.9 %.

### 3d. Where each threshold starts (game today)

| Threshold | Resting | Walking, flat | Bot on the rope | Bot on the rope, bottle on |
|---|---|---|---|---|
| Fatigue (72) | 6,487 m | 6,015 m | 5,863 m | 8,363 m |
| Auto bottle (58) | 7,651 m | 7,384 m | 7,291 m | never |
| Warning (52) | 8,134 m | 7,679 m | 7,585 m | never |
| Damage (40) | never | 8,625 m | 8,454 m | never |

The bot took its first damage at about 8,435 m. The copy of the game's rules predicts 8,454 m with one fitted number
(effort gap 7.7 Torr on the rope). So the Python copy is good enough to start B1's fast simulation from.

## 4. What changes for the player

Already true today (since 2026-11-14), compared with the old rule:
- The HUD shows lower numbers from the start: about 84 % in Dingboche, not 100 %. Thresholds were moved down with the
  scale, so there are no warnings below about 7,600 m.
- Fatigue (slower stamina recovery, a small speed loss) starts when walking near Camp 1, not near Camp 3.
- **Without a bottle:** the warning is on from about 7,600 m. Walking on the rope hurts from about 8,450 m.
  **Resting never hurts** (43.6 % at the summit, above 40). So a "walk, stop, breathe" rhythm may get a careful player
  to the summit without a bottle. The 10 s grace time and its 2× faster reset make this easier (rough estimate: walk
  about 12 s, stand about 7 s near the summit). Real climbers without oxygen do climb like this. Not tested yet; the
  cold (section 8) may kill this tactic first.
- **With a bottle:** SpO₂ never goes below 66 %. No warning, only a speed loss under 5 % above about 8,360 m.
- In camp: +4 points and fast adaptation.

What the player also sees in the death zone, from rules outside the oxygen model:
- Frost on the lens and shivering at full strength for the whole push, because the core sits at about 33 °C (section 8).
- Heart rate at its 188 bpm cap for over 10 minutes without a bottle.
- The "spent" sway at full strength: stamina is 0 from about 8,200 m, because the uphill drain is larger than walking
  recovery. (Stamina rule; outside this spec, but it shapes the same moment.)

## 5. The C++ port

### 5.1 Closed form, not a data table
Keep the closed form that is in the game now (`USurvivalComponent::PhysioSpO2`). It is the same equations and the same
two CO₂ tables as `physio_model.py`, so results match the thesis code exactly.
- **Cost:** one call = one `exp`, one `pow`, one `log10` and two lookups in tables of at most 9 entries. It runs 10
  times per second, for one character. That is far below 0.01 ms per second of play. Nothing to optimise.
- A precomputed table (altitude × factor × gap) would add rounding error and a build step, and save nothing.
- For B1's virtual climbers (many at once, or sped-up time) the cost is still tiny: 1,000 climbers at 10 steps per
  second is about 10,000 calls per second.

### 5.2 One source of truth
The CO₂ tables live twice: in `physio_model.py` and in `SurvivalComponent.cpp`. **Rule: the Python file is the
reference.** When A2 recalibrates, change both, then run a parity check:
- `ygg.Physio.Sweep csv` (new option): write `Saved/Review/physio_sweep_game.csv`, the pure model (no game layer) on a
  grid: altitude 0–8,900 m every 100 m × factor 0, 0.25, 0.5, 0.75, 1 × gap 5, 7, 10, 13 Torr.
- `physio_model.py --check <csv>` (new option): compute the same grid and print the largest difference.
  Pass: 0.05 points or less.
- The current `ygg.Physio.Sweep` only prints the camp table to the log. Keep that, too.

### 5.3 Keep the blood temperature at 37 °C inside the model
The Severinghaus curve has a temperature term. Today both the Python and the C++ use 37 °C. Do **not** feed the game's
core stat into it: at 33 °C the curve shifts and SpO₂ would jump by 13–15 points (walking, no bottle: 44.5 → 59.1 at
7,900 m, 32.6 → 45.9 at the summit). The cold would then "help" the oxygen, which hides the thin air.

### 5.4 Update rate and smoothing
- Model: in the fixed 0.1 s survival step (exists).
- SpO₂ moves towards its target at 1.5 points per second **[game]**. A 30-point change (bottle on or off) takes about
  20 s. That feels right; a source for how fast real readings change would make it **[cited]** (to find).

### 5.5 Telemetry columns to add
Today `ygg.Log` and the bot's `samples.csv` record SpO₂ but not the model's inner state. Add:
`adapted_m`, `acclim` (factor), `felt_alt_m`, `gap_torr`, `spo2_target`, `core_target_c`.
Without these, B1 and B2 cannot show *why* SpO₂ was what it was.

### 5.6 Fast simulation for B1
B1 needs the whole survival step in Python, not only SpO₂: oxygen, stamina, cold, damage and speed. Start from the
copy used for this spec (it matched the bot's damage line within about 20 m). Check it against every bot run:
same route profile in, same SpO₂, core and death time out, within a stated tolerance.

## 6. Acclimatisation: how it is stored and saved

Today:
- **State:** `AdaptedAltitude` (metres, float) in `USurvivalComponent`. The 0..1 factor is computed from it every
  step and never saved, so it cannot drift.
- **Saved** in `UYggSaveGame::AdaptedAltitude` at every checkpoint and after a rest; loaded with the save.
- **New game:** 4,400 m. **Old saves** (before 2026-11-14): adapted to the camp they continue from (at most 7,000 m).
- **Camp jump:** adapted to the target camp (at most 7,000 m), so a jump stands for the climb up to it.
- **Death:** adaptation is not rolled back to the checkpoint. That is fine; write it down.

Speed: resting in a camp, Dingboche → fully adapted to 7,000 m takes about **9 minutes**. Real acclimatisation for
Everest takes weeks. The game compresses time; the thesis must state the compression it uses.

**Gap: sleeping gives no acclimatisation.** The [R] rest skips 5 minutes to about 14 hours of game clock and refills
stamina, warmth and health. But adaptation only grows during the 2-second fade (about 10 m). The most natural unit
of acclimatisation, a night at a camp, does nothing for it.

Proposal (for B4 "camps, rest and acclimatisation loop", after your decision D3):
- A rest adds adaptation per slept hour, for example one night = +300 m towards the 7,000 m cap (placeholder number).
- The per-second rule stays for awake time, but slower.
- This gives "climb high, sleep low" a real reason, and B1 can count rotations in nights.
- No save change is needed: the state stays `AdaptedAltitude`.

## 7. Tonight's demo timings: will they hold?

Tonight's runs already used the integrated model. The question is what the next changes would do. Rule of thumb for
the no-oxygen death: **death ≈ the moment walking SpO₂ first reaches 40 + 10 s grace + 167 s** (100 health ÷ 0.6 per
second), so about 3 minutes after the damage line.

| Change | Oxygen summit (20:39 walking) | No-oxygen death (21:09 at 8,552 m) |
|---|---|---|
| Nothing | holds | holds |
| A2 lowers the CO₂ tables by 2 Torr | holds | **no death**: walking SpO₂ at the summit 44 %; a no-oxygen summit becomes possible |
| A2 raises the CO₂ tables by 2 Torr | holds | **damage line 7,736 m**: death about 3 minutes after leaving the South Col |
| Remove the breathing floor (0.75 → none) | holds (with a bottle the factor is 1) | damage line 8,110 m: death after about 12 minutes, near 8,230 m |
| Floor 0.9 instead of 0.75 | holds | no damage while walking: summit possible |
| Bottle worth 2,000 m | about +0.5 min | holds |
| Bottle worth 1,500 m | about +2–3 min (about 23 min) | holds |
| Bottle worth 1,000 m | about +5 min; warning on for the whole push | holds |
| Cold re-tune (section 8, option B) | holds | holds; a long stop on the ridge no longer kills by cold |
| Heart-rate cap by altitude | holds (heart rate is display only) | holds |

**Why the oxygen summit is robust:** with a bottle, SpO₂ stays between 66 and 76 %. That is far above every threshold
except fatigue, and fatigue only costs a few percent of speed. The time is set by the route, the grade and walking
speed. (The A5 target of "20 minutes or less" is met only when sprinting: 18:46.)

**Why the no-oxygen death is fragile:** the damage line sits only about 400 m below the summit. Small calibration
changes move it 300–700 m. After A2 recalibrates, re-run the bots. If the death moves, either re-tune only the game
layer (the damage threshold or the floor) to keep the demo, or report the new result as it is (decision D6).

Also note: the first run tonight (`summit_20261120_061556_o2`) **froze to death at 8,284 m with a bottle on**. Its core
fell 0.0085 °C/s instead of the normal 0.005 °C/s. The storm column was 0; the extra cooling source was not recorded.
The cold is the closer killer (section 8).

## 8. The cold model: why the core sits at 33 °C

The rule (**[game]**, `SurvivalComponent.cpp` step 2):
- Target core = 36.6 − 5.6 × exposure.
- Exposure = 0.75 × (how far you are from 4,000 m to the summit, 0..1), plus weather; × 0.85 while moving; 0 in camp.
- The core falls 0.005 °C per second towards the target (36.6 → 33.4 in about 11 minutes), then stays **on** it.
- Hypothermia damage starts at 33.0 °C.

| Altitude | Target, moving | Target, resting outside |
|---|---|---|
| 6,400 m | 34.83 | 34.52 |
| 7,900 m | 33.73 | 33.22 |
| 8,400 m | 33.36 | 32.79 |
| 8,849 m | 33.03 | 32.40 |

Bot data match these targets exactly (8,400 m: 33.36; 8,600 m: 33.21; 8,800 m: 33.07). So 33 °C is not a bug. It is
the rule's balance point, and the numbers put it 0.03 °C above the damage line at the summit.

Consequences:
- **Resting outside a shelter above about 8,160 m kills by cold** (target below 33), with or without a bottle. Near
  the summit: damage after about 25 s, death about 2 minutes later. The summit itself counts as a camp shelter (the
  bot's core went back to 36.6 °C in about 20 s there), so a summit stay is safe; a long stop on the ridge is not.
- Any extra cold (storm, realm, night) kills before thin air does. Tonight's 06:15 run shows it.
- Frost and shiver are at full strength for the whole death zone, so they stop meaning anything.
- It is not physiology. Medicine calls a core below 35 °C hypothermia; at 33 °C people are confused and clumsy
  **[to cite: a clinical staging source]**. Well-dressed climbers who keep moving usually keep a near-normal core
  **[to cite]**.
- The thesis topic names cold as one of the three parts (thin air, cold, exhaustion). This rule cannot be defended
  as a model yet.

Options:
- **A. Keep it.** Document it as **[game]**. Cheapest; the problems above stay.
- **B. Re-tune now (recommended for the interim).** `ColdExposureAtSummit` 0.75 → 0.45. Summit target: 34.5 °C
  moving, 34.1 °C resting. The cold warning (35 °C) stays on high up, there is no damage in clear weather, and storms
  and realm cold still kill. Frost drops to about half. One number; then re-run the bots.
- **C. Heat-balance sub-model (Phase B).** Heat made by effort (less when hypoxic or exhausted), lost to the air
  temperature (`RealmSky` already has it) and wind through the clothing's insulation; the core changes slowly with
  the difference. Every parameter needs a source. This is the "cold" part of RQ1. About 2 days, together with B4.

## 9. Test plan

All bot runs go through `Tools/Bot/run_bot.ps1`, one at a time (VRAM limit), when no one else uses the editor.

1. **Parity (no editor):** `physio_model.py --check` on the sweep CSV (section 5.2). Pass: 0.05 points or less.
2. **Calibration sweep:** the camp table from `ygg.Physio.Sweep` against the targets in Physiology_Model.md section 4.
   The tolerance is set in the write-up (for example ±5 points at 8,400 m, where the model gives 57.0 and Grocott
   measured 54.0).
3. **Bot sweep (one night, about 14 runs of 20–25 minutes):**

   | Axis | Values |
   |---|---|
   | Bottle | on, off |
   | Pace | walk, sprint when possible, "walk–stop–breathe" (rest when SpO₂ < 42 until it is > 46; new bot mode) |
   | Adapted altitude at start | 7,000 m (camp jump), 6,400 m, 5,364 m |
   | Weather | clear, storm |
   | Stop on the ridge (outside a shelter) | none, 3 minutes at 8,600 m |

   Not the full grid: bottle × pace at 7,000 m and clear weather (6 runs), then adapted altitude and storm on the
   walking bot (8 runs).
4. **Pass criteria:** with a bottle, walking summit in 22 minutes or less and sprinting in 20 or less; without a
   bottle, walking dies between 8,400 and 8,700 m; no cold death with a bottle in clear weather; no death below 8,000 m
   in clear weather; "walk–stop–breathe" result reported either way (it answers D1).
5. **Charts** (`plot_session.py`, extended; output `Saved/Review/physio_integration/`):
   SpO₂ against altitude with the model curves and threshold bands; core against time with its target line; health
   and damage over time; adapted altitude over time; one summary table per run (summit time, time above 8,000 m,
   lowest SpO₂, cause of death).
6. **Before / after:** run the same commands before and after each change in section 11; one table side by side.
7. **Normal play (A2 "done when"):** the A4 route walker from Dingboche to Camp 1: no oxygen warning, no damage.

## 10. Decisions for you

| # | Question | My recommendation |
|---|---|---|
| D1 | Without a bottle: always fatal, or possible for a careful player (stop and breathe)? | Possible but hard, like real life. Let the bot and B1 find out how hard. |
| D2 | Cold: keep (A), quick re-tune (B) or full model later (C)? | B now, C in Phase B if the supervisor keeps cold in the thesis. |
| D3 | Should a night at a camp build acclimatisation? How much is one night worth? | Yes. Start with one night = +300 m, then tune with the bot. |
| D4 | Bottle strength: when a source gives a smaller value than 2,500 m, use it even if the summit gets 2–5 minutes slower? | Yes, use the cited value; shorten the route time elsewhere if needed. |
| D5 | Heart rate: let the maximum fall with altitude (more real, smaller number up high) or keep the 188 cap? | Let it fall, once a source is found **[to cite]**. |
| D6 | If calibration moves the no-oxygen death: re-tune the game layer to keep the demo, or show the new result? | Keep the demo by tuning the game layer; report the pure model numbers separately in the thesis. |

No decision needed (my call unless you object): keep the blood temperature at 37 °C inside the model (section 5.3).

## 11. Work list (after your decisions)

| Step | Work | Size | Needs |
|---|---|---|---|
| 1 | Update Physiology_Model.md: section 1 = the physiology path as it is now; add a "game layer" section (section 2 here) | 0.25 day | — |
| 2 | `ygg.Physio.Sweep csv` + `physio_model.py --check` | 0.25 day | editor free for a build |
| 3 | Telemetry columns (section 5.5) in `ygg.Log` and bot samples | 0.25 day | build |
| 4 | Cold option B | 0.1 day + bot runs | D2 |
| 5 | Sleep builds acclimatisation | 0.5 day (overlaps B4) | D3 |
| 6 | Heart-rate maximum by altitude | 0.1 day | D5, a source |
| 7 | Bot sweep night + charts (section 9) | 0.5 day | steps 2–4 |
| 8 | After A2 calibration: change both copies, parity check, bot re-run, re-tune if D6 says so | 0.25 day | A2 calibration |

## 12. Sources and status of the numbers

- Game rules and parameters: read from `Source/YggdrasilsPeak/Survival/SurvivalComponent.cpp`, `SurvivalConfig.h`,
  `ExpeditionComponent.cpp`, `YggTelemetryComponent.cpp` (read only).
- Model values: `Tools/Thesis/physio_model.py`, imported unchanged.
- Bot values: `Saved/Bot/summit_20261120_*/report.md` and `samples.csv`.
- Values cited in this spec: only those already in Physiology_Model.md (West 1996, Severinghaus 1979, Grocott 2009).
- Still **[to cite]**: hypothermia staging, core temperature of moving climbers, how fast SpO₂ readings change,
  maximum heart rate at extreme altitude, the effect of 2–4 l/min of bottled oxygen. For the bottle, a simple
  gas-mixing estimate gives very different answers depending on breathing rate and how the flow is measured, so the
  value must come from published measurements, not from an estimate.
