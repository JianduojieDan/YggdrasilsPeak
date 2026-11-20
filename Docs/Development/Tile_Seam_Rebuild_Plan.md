# Tile seam rebuild plan (route detail tiles)

Written 2026-11-20 as a read-only investigation. Nothing was built, imported, edited or committed.
How the numbers were made: the on-disk OBJs (`Saved/DetailTiles/route/*_col.obj` = the 2 m grid that the collision is cut
from, `route_col/*`, `backup_2026_11_11/*`) read back with `tiles.json` pivots, then every pair of neighbouring tiles
compared along their shared edge, keeping only edge samples within 25 m of the route (`Tools/RoutePlanning/route.json`).
Scratch scripts: `C:/Users/danji/AppData/Local/Temp/claude/5382e85ac046e29c/26076da0-ca10-4575-93bc-6f309440ee57/scratchpad/seam*.py`.
Not seen: the live level (the .umap is binary). The numbers agree with what the walker measured, so the level is
assumed to match the files (see 1.2).

## 1. What is wrong

### 1.1 The seams (route crossing a tile border with a step)

Route metre = distance along route.json from Dingboche (the walker's "m along" agrees within a metre: 2160 vs 2159,
11591 vs 11592). Leg m = metres into that leg. Step = height of the second tile minus the first at the seam, where the
route comes within 25 m of the seam. Render and collision give the same number: the collision grid is the render
grid sampled every 2 m and the RTIN keeps every border sample (`build_route_collision.py:127,133`). Boots collision
vs visible on the route never differed by more than 5 cm in any of the 7 walks (no collision-vs-surface problem inside tiles).

79 tile borders come within 25 m of the route. As imported now: 42 have a step over 5 cm, 26 over 30 cm, 12 at or over 1 m.
The "10 seams, 1-1.7 m" of the morning report are the 12 at or over 1 m (rows marked **).

| leg | leg m | route m | tiles (A -> B) | step | walker saw |
|---|---|---|---|---|---|
| 0 | 2160 | 2160 | R_4_46 -> R_4_47 ** | -1.14 | blocked 117 cm, 2159 m |
| 0 | 2212 | 2212 | R_3_46 -> R_4_46 ** | +1.14 | |
| 0 | 4260 | 4260 | R_1_43 -> R_2_43 ** | +2.13 | |
| 0 | 4452 | 4452 | R_2_42 -> R_2_43 ** | +1.73 | |
| 0 | 4474 | 4474 | R_1_42 -> R_2_42 ** | -1.15 | |
| 0 | 4974 | 4974 | R_2_41 -> R_2_42 ** | -1.73 | "under" 671 cm and a 7.3 m fall at 4991 m (more than the step, see 1.3) |
| 0 | 5601 | 5601 | R_2_41 -> R_3_41 | +0.51 | |
| 0 | 5651 | 5651 | R_2_40 -> R_3_40 | +0.50 | |
| 0 | 6183 | 6183 | R_3_39 -> R_3_40 | +0.44 | |
| 0 | 6620 | 6620 | R_3_39 -> R_4_39 ** | -1.22 | |
| 0 | 6740 | 6740 | R_3_38 -> R_4_38, R_3_38 -> R_3_39 | +0.57, +0.60 | |
| 0 | 6744 | 6744 | R_4_38 -> R_4_39 ** | -1.19 | |
| 0 | 10447 | 10447 | R_5_31 -> R_5_32 | +0.84 | |
| 0 | 10969 | 10969 | R_5_30 -> R_5_31 ** | -1.41 | "under" 139 cm and blocked 130 cm, 10977 m |
| 0 | 11128 | 11128 | R_5_30 -> R_6_30 ** | -1.19 | |
| 1 | 33 | 11524 | R_5_29 -> R_5_30 ** | +1.57 | |
| 1 | 88 | 11579 | R_6_29 -> R_6_30 | +0.31 | |
| 1 | 100 | 11591 | R_5_29 -> R_6_29 ** | +1.01 | blocked 91 and 94 cm at 95 m (leg 1 run) and 11592 m (leg 0 run), twice |
| 1 | 1055 | 12546 | R_7_28 -> R_7_29 | +0.48 | |
| 1 | 1290 | 12780 | R_7_28 -> R_8_28 | +0.48 | |
| 3 | 1192 | 18022 | R_15_26 -> R_15_27 | +0.51 | blocked 52 cm at 3512 m (leg 3 run) |
| 3 | 1727 | 18557 | R_15_27 -> R_15_28 | -0.53 | |
| 5 | 684 | 22131 | R_21_30 -> R_21_31 | +0.54 | not walked yet |
| 5 | 1262 | 22709 | R_21_31 -> R_21_32 | -0.54 | not walked yet |
| 7 | 773 | 26365 | R_25_30 -> R_26_30 | -0.92 | not walked yet |

Beyond the walked part (legs 4-7) there are more steps of 5-20 cm (R_18_29/R_18_30, R_19_29, R_19_30/R_20_30, R_20_30/R_21_30,
R_24_31/R_25_31, R_25_30/R_25_31); the full list is what `seam3.py` prints. These will all disappear with a consistent build.

### 1.2 The steps are mostly whole-tile height offsets

Along most bad borders the step is the same number on all 251 samples (R_2_42 -> R_3_42: min 1.73, max 1.73). R_4_46 sits
exactly 1.14 m higher than all four of its neighbours. A shape change cannot do that, a tile that is offset in Z can.

- Of the 65 tiles rebuilt on 2026-11-12 (exactly `Tools/Terrain/editor/rope_tiles.txt`, files dated 11-12 00:05-00:15),
  19 have their whole mesh offset against the same tile's pre-rebuild mesh in `backup_2026_11_11/`:
  R_13_26 -0.16, R_15_27 +0.53, R_19_29 -0.09, R_20_30 -0.21, R_21_31 +0.54, R_25_31 -0.41, R_2_42 -1.73, R_3_40 +0.48,
  R_4_39 -1.19, R_4_46 +1.14, R_5_29 -1.01, R_5_30 +0.57, R_5_31 -0.84, R_5_33 -0.22, R_7_28 -0.48 (and four under 5 cm).
  Using the backup meshes with the same pivots, those borders are continuous (0.00-0.01 m).
- Taking that offset out of the current meshes: borders over 1 m on the route go 12 -> 2, over 30 cm 26 -> 11, over 5 cm 42 -> 14.
  The walker's 91-139 cm hits match the offsets (R_5_29/R_6_29: 1.01 vs 91-94 cm; R_4_46/R_4_47: 1.14 vs 117 cm; R_5_30/R_5_31: 1.41 vs 130-139 cm).
- Tiles never rebuilt (older than 11-12) are continuous with each other: 106 old-old border pairs, median largest
  difference 1 mm, none over 50 cm.

### 1.3 The rest is a half-finished rebuild

`glacier_tiles.txt` lists 130 tiles whose height changes with the 2026-11-11 builder edits (cubic glacier mask, gentle rope flattening,
see 2.2). `rope_tiles.txt` is a subset of 65. Only the 65 were rebuilt; the other 65 (R_1_40, R_2_38/39, R_3_35-38, R_4_29-33,
R_5_35, R_6_30-33, R_7_26/27/30/31, R_8_26/29/30, ... all in `glacier_tiles.txt` and not in `rope_tiles.txt`) still hold the old
shape. Where a rebuilt tile touches one of them the ground differs in shape, not only in height: R_1_43 -> R_2_43 2.13 m and
R_1_42 -> R_2_42 1.73 m remain after the offset is removed (R_1_42 and R_1_43 were never rebuilt). Both rebuilt-vs-old pairs
and rebuilt-vs-rebuilt pairs show it (new-new 37 of 65 borders over 5 cm, new-old 31 of 65, old-new 37 of 65).

The 7.3 m fall at 4991 m (R_2_41 / R_2_42 seam, 1.73 m step): the step does not explain 7 m. It sits in the stretch where
R_2_42 is 1.73 m low and the landscape below it was sunk by the 11-12 patch; likely the walker dropped off the step onto the
sunk landscape or between the tile's low edge and its collision. Re-test after the fix; not proven.

### 1.4 Other tile problems seen (not seams)

| where | what | action |
|---|---|---|
| R_4_36, 8004 m (219386 1829438 -8402), normal z 0.99, 126 cm | a flat surface 1.26 m above the boots in the middle of tile R_4_36 (x 2193 m is not a border; y 18294 is 206 m from one) | check with `Tools/Placement/probe_point.py`; if it persists after the rebuild it is a rock/prop with complex-as-simple, not the tile |
| R_13_26, 2219-2297 m, normal z 0.70, 11 cm | steep ice slope, 11 cm over the boots | not a seam, ignore (leg 2 problems are the icefall ice, `Icefall_S04/S05/S06`) |
| auto convex hull | Tile collision is `CTF_USE_COMPLEX_AS_SIMPLE` with `SM_<tile>_col` (`routeRenderBatch.py:23-24`), all route blocks name "1 convex, 0 box" for the *render* mesh only; no convex-hull blocker on a tile was found. The convex-hull blockers of the morning were the icefall seracs (fixed in 8f3f075f) | none |
| collision vs visible on the route | max difference 0 cases over 5 cm in all `points.csv` (boots_collision_cm vs boots_visible_cm) | none |

## 2. Cause

### 2.1 Why a whole tile ends up 1-1.7 m off

- `Tools/Terrain/build_route_terrain.py:265` takes the tile pivot as `piv = [x0 + TILE/2, y0 + TILE/2, float(T['Z'][keep].min())]`.
  The OBJ vertices are written relative to it (`write_obj` line 242). Any change of the ground shape inside a tile moves its
  minimum, so its pivot Z changes by up to a couple of metres. After the 11-11 edits this happened for 19 of the 65 rebuilt tiles.
- `build_route_terrain.py:282` writes `tiles.json` from only the tiles in that run (`index` starts empty at :253). A run with an
  `only_tiles` argument therefore needs its index merged by hand; the file now has all 185 entries in sorted order and, for the
  rebuilt tiles, an entry whose Z does not belong to the new OBJ (the backup meshes line up with it, the new ones do not).
- `Tools/Terrain/editor/reimport_tiles.sh` runs `routeRenderBatch.py`, which only re-imports `SM_<tile>` and `SM_<tile>_col`
  with `replace_existing` (`routeRenderBatch.py:10-12`). It never moves or re-spawns the `DetailTile_R_*` actors; only
  `routeBatchB.py` does that (destroy + `spawn_actor_from_object` at the `tiles.json` pivot, `routeBatchB.py:14-15`).
  So the new meshes (relative to the new pivot) hang on actors still at the old pivot. This is the same class of mistake as
  the old "render and collision from different builds, the player floated" gotcha, one level up: mesh and actor position
  must also come from the same build.

I could not read the actors' Z in the level; the diagnosis is the only reading that fits all three facts (constant offsets,
backup meshes continuous with tiles.json, walker steps equal to the offsets), and the first step of the procedure checks it.

### 2.2 Why the shape also differs at borders

Edits made on 2026-11-11 (diff of `2b8883b2..1e3da0ba` on the builder) changed ground height near ropes and on glacier ground:
- `build_route_terrain.py:53-56` glacier mask now cubic on a softened mask (`gmask_soft`, line 51). It decides `debris`/`ice` (lines 85-86) and so the hummocks (line 91) and rubble.
- `build_route_terrain.py:120` rope flattening changed from `sstep(15,45,rd)` to `0.35+0.65*sstep(10,90,rd)` and the trench (line 121) removed. The 4 m `big` terrain is now reduced to 35% within 90 m of any rope instead of 45 m, so heights shift by up to ~1 m over a 90 m band.
Only half of the affected tiles were rebuilt (1.3), so borders between rebuilt and old tiles are not the same function.
The generator itself is stitch-safe: noise is a function of world position only (header comment :10, `vnoise` :63), the 64 m margin
(`MARGIN`, :21) covers every filter except the sigma-20 curvature filter (line 93, reach 80 m, so up to 16 m of the margin is
reflected; a millimetre-level effect, 106 untouched old-old borders differ by a median of 1 mm). Collision decimation is not a cause: the RTIN keeps every sample on the tile border (`build_route_collision.py:127,133`), so neighbours join exactly (verified: `route_col` border heights equal the 2 m grid on borders). LOD is not a cause either: collision is the separate non-Nanite `_col` mesh.

## 3. Procedure

Do it in one pass, with the editor closed for the builders (the builders are plain Python and do not need Unreal; the editor is
needed only for steps 5-9). Time/RAM below are from file dates of the 11-12 run unless marked estimate.

### 3.0 Scope

- Render: rebuild all 185 tiles in one run (no `only_tiles`), so `tiles.json`, the pivots, the scatter and `landscape_patch.bin` come
  from one build. About 9.5 s per tile (65 tiles took 10 minutes) -> about 30 minutes, one Python process, RAM in the low GB
  (arrays of 629 x 629 per tile; estimate, not measured). The 55 tiles outside `glacier_tiles.txt` should come out
  identical to their old files (check by hash in step 4 and skip nothing blindly).
- Collision: rebuild the `_col` of every one of the 185 tiles (1.5 minutes: `route_col` files were written 00:15:40-00:17:14) with the `ROCK_U` change in 3.1.
- Import: render + collision together for every tile that changed in either; because `ROCK_U` changes all collision, that is all 185.

### 3.1 Add ROCK_U = 0.15 to the collision build (second tile job, same pass)

`build_route_collision.py:19` has `ROCK_U = 0.5` and line 104 decides `rock = mean(UV0.x of the 3 vertices) > ROCK_U`.
Measured on R_6_49 near Dingboche tent_out: 12% of visible rubble reads as snow; with `ROCK_U = 0.15` that drops to 0%
and no clean snow becomes rock. Change line 19 to `ROCK_U = 0.15` (before the collision build, so every `route_col` file uses it).
- Runtime/RAM: none. The value only decides which of the two material groups (`a_snow`, `b_rock`) a face goes into; the
  RTIN triangle choice (heights only) and the triangle counts stay the same, so the mesh sizes (the 123 MB `route_col`,
  the ~2.3 M collision triangles from `stats.json`) do not change. The snow section just gets a few percent smaller and the
  rock section larger, and `USnowTrailComponent::IsRock` (`SnowTrailComponent.cpp:1100-1130`) reads those section sizes from the
  imported `_col` asset, so it follows automatically after import. Only behaviour changes: more rubble faces read as rock for footsteps, footprints (none on rock)
  and `ClimbingComponent`'s surface; that is the intended fix. The render material (UV0.x) is untouched.
- One check to add: the build prints `ALL ROCK <tile>` (line 143) if a tile becomes all rock; expect none beyond what it printed before.

### 3.2 Steps

All paths relative to `F:/UE_project/YggdrasilsPeak`, Git Bash.

1. Backups (about 7 GB of OBJ + 0.3 GB of assets; F: has 536 GB free).
   ```
   B=Saved/DetailTiles/backup_2026_11_20_seams; mkdir -p $B/route $B/route_col
   cp Saved/DetailTiles/route/*.obj Saved/DetailTiles/route/tiles.json $B/route/
   cp Saved/DetailTiles/route/landscape_patch.bin Saved/DetailTiles/route/landscape_patch.bin.undo.bin $B/route/
   cp Saved/DetailTiles/route_col/* $B/route_col/
   cp Saved/Placement/ground_moves.csv $B/
   ```
   plus a git commit of the editor assets (`Content/Environment/DetailTiles/Route`, `Content/LevelPrototyping/base_map.umap`)
   and a tag `backup-before-seam-rebuild-2026-11-20` made by the user's usual routine (not done now). Note `landscape_patch.bin.undo.bin`
   is overwritten by a second apply (editor README step 2), so the copy above is the only way back.
2. Confirm the diagnosis (editor, 1 minute, read only): `Tools/Placement/probe_point.py` at two seam points, for example (3000, 14972)
   (R_5_29/R_6_29: expect a 1.0 m step on the ground trace) and (2000, 23500) (R_4_46/R_4_47: 1.1 m); and read the Z of actors
   `DetailTile_R_4_46`, `DetailTile_R_5_29` (expect the old pivot, 114 cm / 101 cm away from where the new mesh needs it).
3. Render build, whole route (closing the editor first is not needed for this; keep it idle, the builder is CPU only):
   ```
   python -I Tools/Terrain/build_route_terrain.py
   ```
4. Collision build with `ROCK_U = 0.15`:
   ```
   python -I Tools/Terrain/build_route_collision.py
   ```
   Check right after, before touching the editor: re-run the scratch seam scripts (`seam3.py`, `seam5.py`): every border within 25 m of the
   route must be under 5 cm (expect about 1-2 mm); `ALL ROCK` lines (none new); `stats.json` triangle total about 2.3 M as before.
   Hash the 55 tiles outside `glacier_tiles.txt` against the backup: identical files can be left out of the render import in step 6 (saves time) but their `_col` is still imported.
5. Close the editor if open (user's rule: save via MCP and close without asking), then relaunch, or let `reimport_tiles.sh` start it.
6. Re-import render + collision together, every tile in one list (the script restarts the editor above 13 GB; each batch of 8 adds about 3 GB, so about 24 batches and a restart every 4 batches; estimate 1.5-2.5 h total, I have no timing of past runs):
   ```
   ls Saved/DetailTiles/route | grep '^R_[0-9_]*\.obj$' | sed 's/\.obj//' | tr '\n' ' ' > $TEMP/ygg/all_tiles.txt
   bash Tools/Terrain/editor/reimport_tiles.sh $TEMP/ygg/all_tiles.txt
   ```
   This imports `SM_<tile>` and `SM_<tile>_col` as a pair for every tile (routeRenderBatch.py:10-25) and re-links `complex_collision_mesh`.
   Do not import only one of the two for a tile.
7. Put every actor on its new pivot (the step that was missing). Preferred: a three-line editor script (to write at execution time) that sets, for each `DetailTile_R_*` actor, `set_actor_location(piv * 100)` from the new `route/tiles.json`. That keeps all per-actor settings (ray tracing off, shadow caster flags, tags, folder). Fallback: `routeBatchB.py` (destroy and spawn), which resets those to what the script sets (ray tracing off only); use it only if the move script fails, then re-run `Tools/Terrain/editor/rtoff.py`, `Tools/Terrain/set_shadow_caster.py` and compare with a before/after `Tools/Terrain/editor/propdump.py`.
   Then `tod2.py` (restore Time of Day 785.363531) and `sv.py` (save level), as in the editor README.
8. Landscape under the tiles: apply the new `landscape_patch.bin` (it is sized to the new tile Z) with `patchset.py` and `patch_args.txt` = `<abs path of new landscape_patch.bin>|1`. Needed because the tiles move up to 1.7 m and the half-rebuilt glacier ground changes by up to ~1 m; otherwise the old landscape (sunk to the old minimum - 0.6 m) can poke through or leave a gap. Back-up (step 1) comes first.
9. Re-seat everything that stands on the ground, measure first:
   ```
   python -I Tools/RoutePlanning/uepy.py Tools/Placement/ground_pass.py
   ```
   Read `Saved/Placement/ground_audit.csv`. The tiles moved up to 1.7 m after the 11-13 audit seated props, ropes, camp items, Rocks_Route, ice blocks on the shifted ground, so expect many moves near the 19 tiles in 1.2. Apply with the `APPLY = True` wrapper only after the numbers look sane (memory note 2026-11-14: an APPLY once sank 83k rocks and had to be reverted with `revert_last_run.py`; check the per-kind counts before and after, rocks above all). Do not re-run `rocksroute.py`, `rtrocks.py`, `bakedremove.py` or the scatter import: the scatter JSONs are only needed for the rock instances, which already exist and are re-seated by `ground_pass`.
10. Save the level, close the editor via the user's routine.

### 3.3 Check afterwards (the walker and probes, in this order)

1. Offline, already in step 4: seam scripts under 5 cm on every route border.
2. In the editor, `Tools/Placement/tile_render_vs_col.py` on the 19 offset tiles (R_4_46, R_5_29, R_5_30, R_5_31, R_2_42, R_4_39, R_3_40, R_7_28, R_15_27, R_21_31, R_25_31, R_13_26, R_20_30, R_19_29, R_5_33 and the rest of 1.2): render minus collision under 5 cm. `probe_point.py` at the seam points of 1.1: no step on the ground trace.
3. Route walker, the legs that failed:
   ```
   powershell -File Tools/Bot/run_bot.ps1 -Chapter route -RouteArgs "from 0 to 1 speed 4" -Extra "ygg.Realm.Force none" -TimeoutMin 100
   ```
   (leg 0: R_4_46 at 2160 m, R_2_41/42 at 4974-4991 m, R_5_30/31 at 10969-10977 m, R_5_29/R_6_29 at 11591 m; leg 1: the same seam from the other side),
   then `from 3 to 3 speed 4` for R_15_26/27 at leg-3 metre 1192. Expected: no `blocked ... DetailTile` and no `under` row in `report.md`; stuck teleports from tile hits 0; `largest floor gap` stays about 2.4 cm.
4. Ground probe spots: one `ground_probe.csv` row (`Tools/Placement/probe_point.py`) at each of R_5_29/R_6_29, R_4_46/R_4_47, R_2_41/R_2_42 seams, checking `vis_minus_col` under 5 cm.
5. Footprints and rock reading: walk a rubble patch (R_6_49 near the Dingboche tent_out, the measured spot): no prints on the rubble, prints in the snow beside it, `ygg.Feet.GoSteep` surface 2 (rock) on the slopes. The R_4_36 flat surface at 8004 m should be re-walked; if it is still there it is a placed object.

## 4. Risks and what must not change

- Footprints on tiles (`SnowTrailComponent`): tile actors must keep their names (`DetailTile_R_*`), `SM_<tile>` must keep the linked complex collision `SM_<tile>_col`, collision stays non-Nanite, complex-as-simple, `has_navigation_data` False (all done by `routeRenderBatch.py`), and the snow section must stay first (slot 0, `a_snow`) and rock second (`b_rock`); the rock decision is by face index past the snow count. Keep the 2 m collision grid and the face winding of the builders as they are.
- DisplacementScaling note: `M_EverestDetailTile` DisplacementScaling magnitude 0.4204 x 951.4 = 400 cm (centre 0.5), see `Docs/Development/Snow_Diagnosis_2026-11-14.md:63`. The re-import only sets `m.set_material(0, mat)` to `M_EverestDetailTile`; do not edit the material, and do not rescale the tile meshes (they are in cm already). Because the material displaces by up to +-2 m, the visible surface is up to a couple of metres around the render mesh: this is the reason collision must come from the same build as the render mesh.
- Rock/snow look: UV0.x of the render mesh is the rubble weight written by the same function; because the builder's look code (`ROCK_THR`, `rockw`, scatter `put`) is unchanged and the 55 non-glacier tiles are expected identical, the look should be as before, except on the glacier and rope tiles where it becomes the look the user approved on 11-11/11-12 (the 65 rebuilt tiles are already that look; the other 65 will newly change, so the user should look at them: Khumbu glacier, Western Cwm). The scatter JSONs are a by-product; the existing Rocks_Route instances stay (ground_pass only seats them).
- Do not run: `build_route_terrain.py` with an `only_tiles` list (it overwrites `tiles.json` with only those tiles), `ground_pass` with APPLY before reading its measure run, anything that also re-places `Rocks_Route`.
- Memory/performance: the editor grows by about 3 GB per batch of 8 tiles; stay under the 13 GB restart threshold in `reimport_tiles.sh`; VRAM is not involved. Game runtime/RAM unchanged (same tile count, same triangle counts; the 12 GB game budget is untouched). One heavy process at a time (the walker, the editor and the builders do not run together).
- The icefall slides recorded in `Saved/Placement/ground_moves.csv` (runs 10:39, 11:14, 11:15) were made against ground near R_10_25-R_15_27; those tiles are in the rebuild. Rebuilding should not move the ground there more than the offsets above (R_13_26 -0.16, R_15_27 +0.53), but run the leg 2/3 walker again (3.3 item 3) since the ice corridor was cleared at 2.3 m.

### Revert path

1. Editor assets: `git checkout -- Content/Environment/DetailTiles/Route Content/LevelPrototyping/base_map.umap` (or reset to the backup tag; the 370 tile assets and the level are tracked).
2. OBJ sources: copy `Saved/DetailTiles/backup_2026_11_20_seams/route/*` and `route_col/*` back over `Saved/DetailTiles/route` and `route_col`.
3. Landscape: apply the saved undo: `patchset.py` with `patch_args.txt` = `<backup>/route/landscape_patch.bin.undo.bin|1`, then the old patch if needed.
4. Placement: `Tools/Placement/revert_last_run.py` (or `revert_moves.py`) undoes the `ground_pass` moves; `ground_moves.csv` backup in step 1.
5. Re-run the seam scripts and the walker (3.3) to confirm the state is the one from before (the 12 seams).

## 5. Summary for the decision

- 12 route tile borders with a step of 1.0-2.1 m (26 over 30 cm, 42 over 5 cm, out of 79 on the route); 5 of them already blocked the walker (R_5_29/R_6_29, R_4_46/R_4_47, R_5_30/R_5_31, R_5_29/R_5_30, R_2_41/R_2_42 at 4991 m).
- Cause: tile pivot Z changed for 19 of the 65 tiles rebuilt on 2026-11-12 (`build_route_terrain.py:265`), re-import (`routeRenderBatch.py`) replaces meshes but does not move the actors, and `tiles.json` (`:282`) only holds the run's tiles; plus 65 of the 130 changed tiles (glacier and rope code of 11-11, :53-56, :120) were never rebuilt.
- Commands: backup, `python -I Tools/Terrain/build_route_terrain.py`, set `ROCK_U = 0.15` (`build_route_collision.py:19`) then `python -I Tools/Terrain/build_route_collision.py`, `bash Tools/Terrain/editor/reimport_tiles.sh <all 185 tiles>`, move actors to the new pivots, `patchset.py`, `ground_pass.py` (measure, then APPLY), walker legs 0-1 and 3.
