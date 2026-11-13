# Route check: the whole route without walking it

Walking the 28 km route takes hours. `ygg.RouteCheck` in the game jumps along it instead: every 200 m it puts the
climber on the route, waits for streaming to settle and checks the ground, every object within 40 m and the guidance
ahead. It also takes a picture looking along the route. The whole route takes about 15-20 minutes.

| file | what |
|---|---|
| `Source/YggdrasilsPeak/Survival/YggRouteCheck.*` | the console command (a game-instance subsystem, one step per frame) |
| `route_check.ps1` | starts the standalone game, waits for `done.txt`, then runs the analyser |
| `analyze_route_check.py` | `report.md` and the contact sheets `sheet_NN.jpg` in the run folder |

## Run it

Close the editor first (one heavy process at a time), then from the project folder:

```
powershell -File Tools/Route/route_check.ps1                         # whole route, every 200 m, one picture per point
powershell -File Tools/Route/route_check.ps1 -Spacing:100 -FirstLeg:2 -LastLeg:3 -Shots:2   # the Icefall closely, looking back too
python -I Tools/Route/analyze_route_check.py latest                  # the report again (or give a run folder)
```

Parameters: `-Spacing` metres (horizontal, along the polyline), `-FirstLeg`/`-LastLeg` (route.json legs 0 Dingboche ..
7 summit), `-Shots` 0 none / 1 along the route / 2 also back, `-Settle` seconds per point, `-Time` time of day held at
every point (0..2400; `-Time:-1` lets the clock run), `-Radius` metres for objects, `-Extra "cmd, cmd"` more console
commands (for example `ygg.Quality 3`), `-ResX/-ResY`, `-TimeoutMin`, `-NoAnalyse`, `-Sound`. Negative numbers need the
colon form, because `powershell -File` reads `-1` as a parameter name.

The script launches the game like `Tools/Art/shot.ps1` does: `UnrealEditor.exe <uproject> /Game/LevelPrototyping/base_map
-game -windowed -ResX=1280 -ResY=720 -nosplash -NoSound -unattended -YggNoSave -ExecCmds="..."`. It adds
`ygg.Realm.Force none` (ordinary weather) and `ygg.RouteCheck.QuitWhenDone 1`. The game log is
`%LOCALAPPDATA%\UnrealEngine\5.8\Saved\Logs\RouteCheck.log`.

In a game you are playing yourself (Play in the editor, or the game), use the console:

```
ygg.RouteCheck [SpacingM=200] [FirstLeg=0] [LastLeg=all] [Shots=1]      (or Spacing= First= Last= Shots= in any order)
ygg.RouteCheck stop
```

Switches: `ygg.RouteCheck.QuitWhenDone` (0), `.Settle` (3 s; it waits up to 3x longer while textures still stream),
`.FirstWait` (15 s of play before the first point), `.Radius` (40 m), `.MaxObjects` (400 per point, nearest first),
`.Time` (1100).

While it runs, the climber flies with its collision off and the body simulation is paused. No fall, crevasse, serac or
snow-bridge trigger can touch it. Frost, the thin-air tunnel and blowing snow are held off so the pictures stay clean.
When the game is not quit afterwards, all of this is put back and the climber stands at the last point.

## What it writes: `Saved/RouteCheck/<yyyyMMdd_HHmmss>/`

- `points.csv`: one row per point.
  - `ground` (tile / landscape / object / none) and `surface` (snow / rock / glacier / object), using the same rules as the
    footsteps (`USnowTrailComponent::IsRock`, face index on the detail tiles).
  - `stand_z`; `dz_route_cm` (against the planned route height); `slope_deg` (ground plane over 2 m); `grade_deg` (slope
    along the route); `walkable` (plane slope against the climber's walkable floor angle).
  - `capsule_blocked` (the standing climber does not fit).
  - `pawn_dz_cm`: where the climber's own collision (Pawn channel, simple collision) finds the ground, minus where the eye
    sees it (Visibility channel, complex). A large value means the seen surface and the walked surface differ.
  - `tile_z` / `land_z`: the detail tile and the landscape under it.
  - `issues`: see the report. `seg_*` describes the stretch since the previous point, sampled every 5 m: no ground, holes
    (a tile covers the spot but the ray found only the landscape), no tile, steep samples, the biggest step.
  - Guidance: `rope/flag/clue/checkpoint_ahead_m`, the nearest one within 100 m in front, and `*_los` (whether the eye can
    see it).
  - Object counts, `frame_ms` (settled frame time there), `streaming_left` (textures still streaming when measured), the
    picture names and a `bugitgo`.
- `objects.csv`: one row per object within the radius. This covers every visible static or instanced mesh except
  terrain, sky, far terrain, World Tree, realms, ropes and crevasse ladders (the skip lists of
  `Tools/Placement/ground_lib.py`), plus the trees from `AForestStreamer`.
  - Measured on a 3x3 grid (4x4 over 4 m). In each cell it finds the object's underside (a ray up from below that keeps
    only its own hit) and what is under it (terrain, or another object it rests on).
  - `gap_cm` is the closest contact. Over 10 cm it counts as `floating`.
  - `frac_under` is the share of its height below the terrain. Over 0.5 it counts as `buried`. `pen_cm` is the deepest
    point.
  - `method`: `trace`; `bounds` when it has no collision (the box bottom: floating is still certain, buried is rough);
    `root` for trees.
  - `blocks_player` (collision that stops the climber) and `walk_into` (no such collision, standing on the ground,
    over 50 cm tall and 30 cm wide).
  - Kind `hanging` (flags, cords, poles) and the camp markers' pieces without collision (kind `landmark`: their cords
    and flags) hang in the air by design, so they never count as floating. A camp marker's tents, pole and cairn do.
    `drift` (snow drifts) is soft by design, so it never counts as walked through.
  - Contact counts when another object is under the underside or inside the object's own column (a stone sunk into
    a boulder, an ice block set into a glacier sheet), not only the terrain.
- `shots/NNN_leg<L>_<dist>m.jpg` (1280x720, the scene only with no HUD), plus `_back.jpg` with Shots=2.
- `log.txt`, and `done.txt` (status and totals) as the last file.

## Reading the report

Read `report.md` from the top down: the summary, then floating, buried, walked through, ground problems, holes between
points, steep ground, no guidance, frame time, and the contact sheets. On the sheets, a red frame marks a ground
problem and an orange frame marks no guidance ahead.

To look at a spot in the game, open the console, type `Ghost`, then paste the `BugItGo` line (without Ghost the climber
falls from there).

Ground problem names: `no_ground`, `pawn_falls_through`, `pawn_sinks` (a seen surface that is not solid),
`pawn_floats` (invisible collision), `steep`, `capsule_blocked`, `tile_hole`, `no_tile`, `land_above_tile`. Two more
names are only facts: `on_object` (standing on a rock or prop) and `far_from_route_z` (more than 3 m from the planned
route height).

## Limits

- The visible Nanite mesh of a detail tile is not readable in the game. "Seen" here means the Visibility trace on the
  tile's complex collision (`_col`). To compare the render mesh with the collision mesh, use
  `Tools/Placement/tile_render_vs_col.py` in the editor. The pictures are there for the eye.
- Streaming: each point waits `Settle` seconds, then until no texture is still wanted, for at most 3x as long. Nanite
  streaming has no cheap query, so the fixed wait covers it. Pictures from points with `streaming_left > 0` can be soft.
- Objects that were already measured at an earlier point are not measured again. With `-Spacing` under 80 m, the
  objects between points are counted once.
- Trees have no collision at all (`AForestStreamer` turns it off), so every tree has `walk_into` = 1. The report leaves
  them out of the walked-through list.
