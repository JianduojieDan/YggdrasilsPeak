# Runs INSIDE the editor. The Khumbu Icefall from reference photos: a dense jumble of tilted ice blocks of mixed sizes, a trodden corridor kept
# clear along the route and ropes, clues and pickups clear too. Uses the ice scans (M_IceScan). Replaces Icefall_IceCliffs each run.
import unreal, json, math, random
ROOT = "F:/UE_project/YggdrasilsPeak"
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
w = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
acts = eas.get_all_level_actors()
scatter = [a for a in acts if a.get_actor_label() == "Scatter_IcefallSeracs"][0]
seed = []
for c in scatter.get_components_by_class(unreal.InstancedStaticMeshComponent):
    for i in range(c.get_instance_count()):
        t = c.get_instance_transform(i, True).translation
        seed.append((t.x, t.y, t.z))
for a in acts:
    if a.get_actor_label() == "Icefall_IceCliffs":
        for c in a.get_components_by_class(unreal.InstancedStaticMeshComponent):
            for i in range(c.get_instance_count()):
                t = c.get_instance_transform(i, True).translation
                seed.append((t.x, t.y, t.z))
        a.destroy_actor()
ar = unreal.AssetRegistryHelpers.get_asset_registry()
meshes = []
for n in ("uhpocesfa", "uhupbgxfa", "veipbcmcb", "vd5leh0ga"):
    ds = [d for d in ar.get_assets(unreal.ARFilter(package_paths=[f"/Game/Fab/Megascans/3D/Ice_Cliff_{n}"], recursive_paths=True, class_paths=[unreal.TopLevelAssetPath("/Script/Engine", "StaticMesh")]))]
    ds = sorted(ds, key=lambda d: "Raw" not in str(d.package_name))
    if ds:
        meshes.append(unreal.load_asset(str(ds[0].package_name)))
print("meshes", [m.get_name() for m in meshes])
keep = []
for leg in json.load(open(f"{ROOT}/Tools/RoutePlanning/route.json")):
    keep += [(p[0], p[1], 450.0) for p in leg["pts"]]
for a in eas.get_all_level_actors():
    n = a.get_class().get_name()
    l = a.get_actor_location()
    if n == "FixedRope":
        keep += [(p.x, p.y, 450.0) for p in a.get_editor_property("rope_points")]
    elif n == "RouteLandmark":
        keep.append((l.x, l.y, 5000.0))
    elif n in ("CrevasseCrossing", "StoryClue", "SupplyPickup", "OxygenPickup") or any(k in a.get_actor_label() for k in ("IceCave", "Checkpoint", "Clue_")):
        keep.append((l.x, l.y, 1200.0))
CELL = 10000.0
grid = {}
for x, y, r in keep:
    grid.setdefault((int(x // CELL), int(y // CELL)), []).append((x, y, r))
def blocked(x, y, r):
    reach = int((r + 5000.0) // CELL) + 1
    cx, cy = int(x // CELL), int(y // CELL)
    return any(math.hypot(px - x, py - y) < r + c for i in range(cx - reach, cx + reach + 1) for j in range(cy - reach, cy + reach + 1) for px, py, c in grid.get((i, j), ()))
rng = random.Random(1953)
cands = []
for x, y, z in seed:
    for _ in range(14):
        cands.append((x + rng.uniform(-2500, 2500), y + rng.uniform(-2500, 2500), z))
rng.shuffle(cands)
placed_grid = {}
PC = 3000.0
def free(x, y, r):
    cx, cy = int(x // PC), int(y // PC)
    for i in range(cx - 2, cx + 3):
        for j in range(cy - 2, cy + 3):
            for px, py, pr in placed_grid.get((i, j), ()):
                if math.hypot(px - x, py - y) < (r + pr) * 0.42:   # blocks crowd and overlap, as in the photos
                    return False
    return True
act = eas.spawn_actor_from_class(unreal.Actor, unreal.Vector(0, 0, 0))
act.set_actor_label("Icefall_IceCliffs")
ls = [a for a in eas.get_all_level_actors() if a.get_actor_label() == "Landscape_Smooth"][0]
ign = [a for a in eas.get_all_level_actors() if a is not ls]
def ground(x, y, z):
    h = unreal.SystemLibrary.line_trace_single(w, unreal.Vector(x, y, z + 50000), unreal.Vector(x, y, z - 80000), unreal.TraceTypeQuery.ECC_VISIBILITY, False, ign, unreal.DrawDebugTrace.NONE, True)
    return h.to_tuple()[4].z if h else None
out = [[] for _ in meshes]
small = [[] for _ in meshes]   # under 9 m: no shadow, gone beyond 400 m, for the frame rate
sizes = []
for x, y, z in cands:
    if len(sizes) >= 5500: break
    r = rng.random()
    width = rng.uniform(350, 900) if r < 0.6 else rng.uniform(900, 2200) if r < 0.93 else rng.uniform(2200, 5200)
    if blocked(x, y, width * 0.5) or not free(x, y, width * 0.5):
        continue
    g = ground(x, y, z)
    if g is None:
        continue
    i = rng.randrange(len(meshes))
    b = meshes[i].get_bounding_box()
    bw, bd, bh = b.max.x - b.min.x, b.max.y - b.min.y, b.max.z - b.min.z
    k = width / bw
    sc = unreal.Vector(k * rng.uniform(0.7, 1.2), k * rng.uniform(1.0, 3.2), k * rng.uniform(0.5, 1.5))   # thick blocks, not thin walls
    rot = unreal.Rotator(roll=rng.gauss(0, 26), pitch=rng.gauss(0, 26), yaw=rng.uniform(0, 360))
    (small[i] if width < 900 else out[i]).append(unreal.Transform(unreal.Vector(x, y, g - b.min.z * sc.z - bh * sc.z * rng.uniform(0.3, 0.55)), rot, sc))
    placed_grid.setdefault((int(x // PC), int(y // PC)), []).append((x, y, width * 0.5))
    sizes.append(round(width / 100, 1))
sub = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
root = sub.k2_gather_subobject_data_for_instance(act)[0]
for i, m in enumerate(meshes):
    for group, cheap in ((out[i], False), (small[i], True)):
        h, reason = sub.add_new_subobject(unreal.AddNewSubobjectParams(parent_handle=root, new_class=unreal.HierarchicalInstancedStaticMeshComponent))
        comp = unreal.SubobjectDataBlueprintFunctionLibrary.get_associated_object(unreal.SubobjectDataBlueprintFunctionLibrary.get_data(h))
        comp.set_static_mesh(m)
        if cheap:
            comp.set_cast_shadow(False)
            comp.set_editor_property("instance_start_cull_distance", 30000)
            comp.set_editor_property("instance_end_cull_distance", 40000)
        comp.add_instances(group, False, True)
sizes.sort()
print("ice blocks", len(sizes), "width m median/p95/max", sizes[len(sizes) // 2], sizes[int(len(sizes) * .95)], sizes[-1])
unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
print("saved")
