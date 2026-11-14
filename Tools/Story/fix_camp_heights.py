import unreal, json, math
# Runs INSIDE the editor on base_map via Tools/RoutePlanning/uepy (no dot-p-y text in this file).
# The chapter 1 world builder traced the ground while the detail tiles' collision was still cooking (it starts after
# the map loads), so story places and camp props landed on the landscape ~1.07 m UNDER the drawn tile at the camps
# (NPCs waist-deep, tents half buried). This re-traces them now and lifts what sits on the landscape under a tile.
# Things already above the landscape (the room on its plinth) are left alone. Re-runnable.
W = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
if W.get_name() != "base_map":
    unreal.EditorLoadingAndSavingUtils.load_map("/Game/LevelPrototyping/base_map")
    W = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
V = unreal.Vector
PLACES = "F:/UE_project/YggdrasilsPeak/Content/Story/Chapter1_places.ygs"
IGNORE = [a for a in unreal.GameplayStatics.get_all_actors_of_class(W, unreal.Actor) if a.actor_has_tag("Ch1")]   # props never count as ground


def heights(x, y, zh):
    """(landscape z, tile z) under x,y - either may be None."""
    hits = unreal.SystemLibrary.line_trace_multi(W, V(x, y, zh + 3000), V(x, y, zh - 3000), unreal.TraceTypeQuery.ECC_VISIBILITY,
                                                 True, IGNORE, unreal.DrawDebugTrace.NONE, True) or []
    land = tile = None
    for h in hits:
        t = h.to_tuple(); a, c = t[9], t[10]
        if a is None: continue
        if "Landscape" in a.get_class().get_name() and land is None: land = t[4].z
        elif isinstance(c, unreal.StaticMeshComponent) and c.static_mesh and "/DetailTiles/" in c.static_mesh.get_path_name() and tile is None: tile = t[4].z
    if land is None:   # the landscape lies under the tile: trace again from just below the tile
        start = (tile - 5) if tile is not None else zh + 3000
        h = unreal.SystemLibrary.line_trace_single(W, V(x, y, start), V(x, y, zh - 3000), unreal.TraceTypeQuery.ECC_VISIBILITY, True, IGNORE, unreal.DrawDebugTrace.NONE, True)
        if h and h.to_tuple()[9] and "Landscape" in h.to_tuple()[9].get_class().get_name(): land = h.to_tuple()[4].z
    return land, tile


P = json.load(open(PLACES))
probe = P.get("nima_spot")
l0, t0 = heights(probe[0], probe[1], probe[2])
if t0 is None:
    raise SystemExit("detail tile collision not ready yet (no tile under nima_spot) - run again in a few seconds")
print("probe nima_spot landscape", round(l0 or 0), "tile", round(t0))

moved = []
for k, v in P.items():
    if not isinstance(v, list) or len(v) < 3: continue
    land, tile = heights(v[0], v[1], v[2])
    if land is not None and tile is not None and abs(v[2] - land) < 30 and 15 < tile - land < 250:
        moved.append((k, round(tile - land))); v[2] = round(tile, 1)
json.dump(P, open(PLACES, "w"), indent=1)
print("places lifted", moved)

lifted = []
for a in unreal.GameplayStatics.get_all_actors_of_class(W, unreal.Actor):
    if not a.actor_has_tag("Ch1") or "Room" in str(a.get_folder_path()): continue   # the room sits on its own plinth
    p = a.get_actor_location()
    land, tile = heights(p.x, p.y, p.z)
    if land is None or tile is None: continue
    d = tile - land
    if abs(p.z - land) < 45 and 15 < d < 250:
        a.set_actor_location(V(p.x, p.y, p.z + d), False, False); lifted.append((a.get_actor_label(), round(d)))
print("props lifted", len(lifted), lifted[:40])
unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
