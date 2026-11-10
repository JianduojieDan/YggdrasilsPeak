# Route detail terrain: editor side

Run after `python Tools/Terrain/build_route_terrain.py` has written `Saved/DetailTiles/route/` (OBJs, scatter, tiles.json, landscape_patch.bin).
The scripts run inside the open editor through Python remote execution (`python run.py <script>`); most read their input from
`%TEMP%/ygg/*.txt`, so keep a copy of this folder at `%TEMP%/ygg` (they were developed there).

1. `import_all.sh`: reads `batches.txt` (one line of tile names per batch). Each batch runs `routeBatch.py` (imports render + 2 m
   collision OBJ, Nanite on the render mesh, Nanite OFF on the collision mesh, complex-as-simple, tile material), waits for mesh
   builds, then runs `routeBatchB.py` (attaches the collision mesh, spawns `DetailTile_R_*`, no ray tracing, saves). Restarts the
   editor (`restart.sh`) when it passes 10.5 GB.
2. `patchset.py` with `patch_args.txt` = `<landscape_patch.bin>|1`: sinks the landscape under the tiles (LandscapeRebuilder.ApplyHeightPatch,
   writes `<file>.undo.bin`; back that up first, a second apply overwrites it).
3. `propapply.py`: re-seats camp props and fixed ropes on the tiles (`props_offsets.json`, made by `propdump.py` before any change).
4. `bakeddump.py`, then `python Tools/Terrain/baked_in_corridor.py`, then `bakedremove.py`: removes old `Rocks_Baked` instances in the corridor.
5. `rocksroute.py`: rebuilds `Rocks_Route` (one HISM per rock kind); `rtrocks.py` keeps the two small-stone sets (>50k each) out of ray tracing.
6. Check: `trace.py` + `cmp.py` (collision vs visible surface), `snapg.sh "x y dz pitch yaw"` (screenshot dz above the ground).
   `fixcol.py` / `rtoff.py` repair tiles imported before the collision / ray-tracing fixes.

Restore the level's Time of Day (785.363531) with `tod2.py` before `sv.py` saves the level.
