# Placement tools: is everything standing on the ground?

All scripts run **inside the open editor** through `Tools/RoutePlanning/uepy.py` (base_map must be the open level;
`open_base_map.py` opens it, the editor starts on the start-menu map).

```
python -I Tools/RoutePlanning/uepy.py Tools/Placement/ground_pass.py          # measure only (~30 s for ~330k things)
```
To fix, set `APPLY = True` in a small wrapper that execs ground_pass (it is popped each run, so a plain rerun only measures):
```
APPLY = True
exec(open(r'F:/UE_project/YggdrasilsPeak/Tools/Placement/ground_pass.py').read())
```

**Run `ground_pass` (with APPLY) after any terrain edit or any script that places things** (camps, rocks, pickups,
icefall, route terrain). It is the safety net that keeps things from floating.

| file | what |
|---|---|
| `ground_lib.py` | ground lookup (detail tiles, else landscape; each component traced alone), exact + support measures |
| `ground_pass.py` | one pass: report per kind, optional fix by rules, `Saved/Placement/ground_audit.csv`, moves appended to `ground_moves.csv` |
| `revert_moves.py` | put named actors back to their first logged position |
| `revert_last_run.py` | undo the actor moves of the last logged run |
| `inventory.py` | one line per group of everything placed in the level |
| `viewport_shot.py` | point the editor viewport at a spot and take a high-res shot (one per run) |
| `check_support.py` | sanity check: fast support measure vs exact vertex measure |

## How it measures (fast)
- Ground = route detail tiles' complex collision where tiles exist, else `Landscape_Smooth` heightfield. On snow and ice
  that is the visible surface (snow has no material displacement). In rock/rubble the material lifts the visible surface
  a little (rock relief biased upward), so things there end up slightly deeper - intended.
- **exact** (props, landmarks, rocks in components under 20k instances): mesh vertices in world space, binned in a 4x4
  (6x6 for > 8 m) grid; lowest vertex per cell vs the ground under it. `pen` = depth of the deepest part.
- **support** (the 217k small route stones, ice blocks): ground plane from 5 traces + the mesh's support function on a
  cube map -> depth into the plane in O(1). Agrees with exact on 99% of stones (check_support).
- ~7 us per trace, ~1.8 M traces per full pass.

## Rules (see the top of ground_pass.py)
small stones 20% of thickness sunk; ice 30%; rocks 12% of height at the deepest point; poles/pickets 15% of height
(max 25 cm); tents, bottles, boxes tilted to the slope once (tag `GroundTilted`) then lowered until 3/4 of the underside
touches. Nothing is ever raised (a deep part can be a pole or a rock meant to stand buried). Route landmarks are only
reported (their signs and flag lines hang in the air by design). Ropes and trees are only reported: they are rebuilt by
their own tools.

## 2026-11-13 first run
102,800 objects moved: 59.5k + 40.6k small stones (median 31 cm), 1.4k rocks (median 43 cm, worst 3 m: boulders over
the glacier), 1,080 ice blocks, ~250 camp props, pickups, pickets. Cause: the route rock generator read the ground at
the nearest metre point (fixed: bilinear, Tools/Terrain/build_route_terrain.py), and terrain edits after placement.

## Gotcha: screenshots right after a map load
Nanite streams the detail tiles in for a few seconds after the level opens; a shot taken then shows the coarse root
level, where the ground can sit tens of cm off and objects look like they float. Wait ~30 s (or take the shot twice).

## Ropes
Ropes are laid by Tools/RoutePlanning/lay_fixed_ropes.py (lying on the snow, anchors, old ropes) - run it after
terrain edits or after place_ropes.
