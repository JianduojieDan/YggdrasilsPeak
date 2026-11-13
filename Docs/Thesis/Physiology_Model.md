# Physiology model — write-up (draft 0.1, 2026-11-13)

Development_Plan.md block **A2**. This is the thesis core: what the climber's body does, why, and how well it matches
published data. Every number carries a status: **[cited]** (from a named source; still to be checked against the paper
before the thesis), **[estimate]** (my placeholder, to calibrate), or **[game]** (a design choice, not physiology).

Tools: `Tools/Thesis/physio_model.py` (model curves and the comparison chart `Saved/Review/physio_spo2_altitude.png`),
`Tools/Thesis/telemetry_test.ps1` + `plot_session.py` (play sessions to data and charts, block A3).

---

## 1. The model as it is in the game today

Code: `Source/YggdrasilsPeak/Survival/SurvivalComponent.cpp` (TickSurvival), parameters in `SurvivalConfig.h`,
heart rate and body effects in `ExpeditionComponent.cpp` (UpdateScreenEffect), air temperature in `RealmSky.cpp`.
One simulation step per tick: environment and activity first, then the stats, then their effects, then the HUD.

| State | Range | How it changes now |
|---|---|---|
| SpO₂ (blood oxygen saturation) | 0–100 % | Moves at 1.5 %/s towards a target: `100 − T·(42 + effort + 10·grade)` with T = (altitude − 4,500) / (8,849 − 4,500) clamped 0..1; effort 6 walking, 16 sprinting; +32 on bottled oxygen, +8 inside a camp **[game]** |
| Core temperature | 33–36.6 °C | Towards a target set by exposure (altitude from 4,000 m, weather, moving ×0.85), shelter warms; realms add a direct cooling rate **[game]** |
| Stamina | 0–100 | Sprinting and uphill grade drain it; resting recovers it, slower with low SpO₂ (fully penalised at 52 %), faster in camp **[game]** |
| Food, water | 0–100 | Fall with time × activity; refilled in camps and by eating/drinking **[game]** |
| Health | 0–100 | Damage after a grace period from starvation, dehydration, hypothermia (core ≤ 33 °C) and hypoxia (SpO₂ ≤ 52 %) **[game]** |
| Speed | × | Uphill, low SpO₂ (down to ×0.7 at 52 %), weather, deep snow, load, footing **[game]** |
| Heart rate | 48–188 bpm | Eases towards `64 + 1.5·max(0, 96 − SpO₂) + effort (24 walking, 55 sprinting) + 18 exhausted + cold (up to 12)` **[estimate]** |
| Air temperature | −100…+60 °C | `16 − 6·altitude(km)` pulled towards the realm's air (Emberfall +60, Winter's Veil −100, …) **[game]** |

## 2. What the first data log showed (2026-11-13)

Session `Saved/Telemetry/session_20261113_211541.csv` (camp jumps Dingboche → Base Camp → Camp 1 → Camp 3 → summit),
chart `Saved/Review/telemetry_20261113_211541_timeline.png`:
- SpO₂ reads **100 % at Base Camp (5,364 m)**: the straight-line rule gives 91.7 % at rest outside a camp, and the
  camp bonus (+8) fills it to 100 %. Real acclimatised climbers there are far lower (section 4).
- The rule is a straight line from 4,500 m; real saturation falls along a curve set by air pressure and breathing.
- Heart rate rose with altitude as designed (64 → 109 bpm at rest on the summit).

## 3. Proposed oxygen model (physiology-based)

Standard equations, few parameters, each with a meaning a reader can check:

1. **Air pressure** from altitude h (km), Everest-specific model atmosphere:
   `PB = exp(6.63268 − 0.1112·h − 0.00149·h²)` Torr. Gives 253 Torr on the summit. **[cited: West 1996]**
2. **Inspired oxygen**: `PIO₂ = 0.2093·(PB − 47)` (47 Torr = water vapour at 37 °C). **[standard]**
3. **Alveolar gas equation**: `PAO₂ = PIO₂ − PACO₂ / R`, R = 0.8. **[standard]**
4. **Arterial oxygen**: `PaO₂ = PAO₂ − AaD`; AaD (alveolar–arterial gap) 5 Torr at rest; exercise widens it.
   **[estimate, calibrate]**
5. **Blood pH** from CO₂ (respiratory alkalosis): `pH = 7.40 + k·(40 − PaCO₂)`, k = 0.008 just arrived,
   0.0049 acclimatised (kidney compensation). **[estimate, calibrate]**
6. **Shift of the dissociation curve** (temperature, pH, CO₂): `PO₂v = PaO₂·10^(0.024·(37 − T) + 0.40·(pH − 7.40) +
   0.06·log₁₀(40 / PaCO₂))`. **[cited: Severinghaus 1979]**
7. **Saturation**: `SaO₂ = 100 / (23400 / (PO₂v³ + 150·PO₂v) + 1)`. **[cited: Severinghaus 1979]**

**Acclimatisation** is the breathing's adaptation: PACO₂ moves from the "just arrived" curve to the "acclimatised"
curve (acclimatisation 0..1, a slow state to add in the game: grows with time spent high, faster resting in camp, decays
lower down, saved with the game). **[design, from standard physiology]**

**Effort**: walking and climbing widen the AaD (exercise desaturation) and raise CO₂ production. **[to model]**
**Bottled oxygen**: raises the inspired O₂ fraction (flow-dependent) — equivalently lowers the "felt" altitude. **[to model,
calibrate against published values]**

### Fit so far (`physio_model.py`, resting)

| Place | Altitude | Game now | Game in camp | Model, acclimatised | Model, just arrived | PB (Torr) |
|---|---|---|---|---|---|---|
| Dingboche | 4,410 m | 100.0 | 100.0 | 83.9 | 76.8 | 451.8 |
| Base Camp | 5,364 m | 91.7 | 99.7 | 79.3 | 67.0 | 400.7 |
| Camp 1 | 6,065 m | 84.9 | 92.9 | 74.8 | 57.6 | 366.3 |
| Camp 2 | 6,400 m | 81.7 | 89.7 | 72.6 | 52.7 | 350.7 |
| Camp 3 | 7,160 m | 74.3 | 82.3 | 67.6 | 40.6 | 317.4 |
| South Col | 7,906 m | 67.1 | 75.1 | 61.4 | 29.3 | 287.3 |
| Summit | 8,849 m | 58.0 | 66.0 | 52.2 | 17.4 | 252.6 |

**Check against a measurement**: at 8,400 m, acclimatised, breathing air, the model gives PaO₂ 24.8 Torr and SaO₂
57.0 % (pH 7.53). Grocott et al. measured PaO₂ 24.6 Torr and SaO₂ 54.0 % in climbers there. **[cited: Grocott 2009]**

## 4. Calibration targets (to fill from the literature and the user's own papers)

| Target | Value | Status |
|---|---|---|
| Summit barometric pressure | 253 Torr | [cited: West 1983 / 1996] |
| Arterial PaO₂ at 8,400 m, acclimatised | 24.6 Torr | [cited: Grocott 2009] |
| Arterial PaCO₂ at 8,400 m | 13.3 Torr | [cited: Grocott 2009] |
| Arterial SaO₂ at 8,400 m | 54.0 % | [cited: Grocott 2009] |
| Alveolar PCO₂ on the summit | 7.5 Torr | [cited: West 1983; one climber, extreme] — the model uses 12.2 [estimate] so saturation keeps falling |
| Resting SpO₂ at Base Camp, acclimatised | ~80 % | [estimate — find a measured value] |
| Resting SpO₂ on arrival at ~4,400–5,400 m | ~70–80 % | [estimate — find measured values] |
| PACO₂ curves between sea level and 8.4 km | table in physio_model.py | [estimate — calibrate] |
| Heart rate at rest and walking by altitude | — | [to find] |
| Effect of bottled oxygen (2–4 L/min) on SpO₂ | — | [to find] |
| Cooling rates of the core in cold wind | — | [to find] |

## 5. Next steps in A2

1. Find measured values for every [estimate] / [to find] row (published studies; the user's own papers where they fit).
2. Fit the PACO₂ curves, AaD and the pH constants to them; show the fit with residuals.
3. Put the model into the game (`SurvivalComponent`): SpO₂ from the equations, acclimatisation as a saved state, effort
   and bottled oxygen as above; re-map the game thresholds (fatigue, warning, damage) to the new scale so the climb stays
   possible and the death zone stays deadly.
4. `ygg.Physio.Sweep`: the same table from inside the game, compared with `physio_model.py`.
5. Re-run the logged session; compare before and after.

## References (verify every value against the paper before the thesis)

- West JB. Prediction of barometric pressures at high altitudes with the use of model atmospheres. *J Appl Physiol*
  1996; 81(4): 1850–1854.
- West JB, Hackett PH, Maret KH, Milledge JS, Peters RM, Pizzo CJ, Winslow RM. Pulmonary gas exchange on the summit of
  Mount Everest. *J Appl Physiol* 1983; 55(3): 678–687.
- Severinghaus JW. Simple, accurate equations for human blood O₂ dissociation computations. *J Appl Physiol* 1979;
  46(3): 599–602.
- Grocott MPW, Martin DS, Levett DZH, McMorrow R, Windsor J, Montgomery HE, for the Caudwell Xtreme Everest Research
  Group. Arterial blood gases and oxygen content in climbers on Mount Everest. *N Engl J Med* 2009; 360(2): 140–149.
- The user's own published work: to add (titles, and which part of the model each supports).
