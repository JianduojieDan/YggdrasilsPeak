# Runs INSIDE the editor (Tools/Terrain/editor/run.py). Places the solid icefall blocks (/Game/Environment/Glacier/Icefall,
# from Tools/Blender/icefall_blocks.py) along one stretch of the route by glacier logic, as ONE actor (HISM per mesh):
#   - the glacier flows downhill (fitted from the ground) and breaks in ROWS across the flow every 14-20 m (crevasses
#     between them); each row carries serac towers / long walls on its crest, blocks along it, rubble spilling downhill;
#   - next to the trodden path the blocks are denser (you walk BETWEEN them, as in the walking-height reference), but
#     the path itself stays clear; blocks are buried at the base, tilted like toppled pieces, never floating.
# Settings in %TEMP%/ygg/icepatch.json: {"label", "route_from", "route_to", "half_width_m", "seed"} (route.json indices).
import unreal, json, math, random
ROOT = "F:/UE_project/YggdrasilsPeak"
cfg = json.load(open("C:/Users/danji/AppData/Local/Temp/ygg/icepatch.json"))
LABEL = cfg.get("label", "Icefall_TestPatch")
rng = random.Random(cfg.get("seed", 11))
D = "/Game/Environment/Glacier/Icefall"
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()

route = [p[:2] for l in json.load(open(f"{ROOT}/Tools/RoutePlanning/route.json")) for p in l["pts"]]
path = route[cfg["route_from"]:cfg["route_to"] + 1]
dense = []
for a, b in zip(path[:-1], path[1:]):
    n = max(2, int(math.hypot(b[0] - a[0], b[1] - a[1]) / 50))
    dense += [(a[0] + (b[0] - a[0]) * t / n, a[1] + (b[1] - a[1]) * t / n) for t in range(n)]
dense.append(tuple(path[-1]))
def path_dist(x, y):
    return min(math.hypot(x - px, y - py) for px, py in dense)

for a in eas.get_all_level_actors():
    if a.get_actor_label() == LABEL:
        eas.destroy_actor(a)
ignore = [a for a in eas.get_all_level_actors() if a.get_actor_label().startswith(("Rocks_", "Icefall_"))]
def ground(x, y):
    h = unreal.SystemLibrary.line_trace_single_for_objects(world, unreal.Vector(x, y, 900000), unreal.Vector(x, y, -400000),
        [unreal.ObjectTypeQuery.OBJECT_TYPE_QUERY1], True, ignore, unreal.DrawDebugTrace.NONE, True)
    if not h: return None
    t = h.to_tuple()
    lab = t[9].get_actor_label() if t[9] else ""
    return (t[4].z, t[7]) if lab.startswith(("DetailTile", "Landscape")) else None

# flow direction: downhill of a plane fitted to the ground around the stretch
cx = sum(p[0] for p in dense) / len(dense); cy = sum(p[1] for p in dense) / len(dense)
S = []
for i in range(-3, 4):
    for j in range(-3, 4):
        g = ground(cx + i * 2000, cy + j * 2000)
        if g: S.append((i * 2000, j * 2000, g[0]))
n = len(S); sx = sum(s[0] for s in S); sy = sum(s[1] for s in S); sz = sum(s[2] for s in S)
sxx = sum(s[0] ** 2 for s in S); syy = sum(s[1] ** 2 for s in S); sxz = sum(s[0] * s[2] for s in S); syz = sum(s[1] * s[2] for s in S)
gx = (sxz - sx * sz / n) / max(sxx - sx * sx / n, 1); gy = (syz - sy * sz / n) / max(syy - sy * sy / n, 1)
fl = math.hypot(gx, gy) or 1.0
F = (-gx / fl, -gy / fl)             # downhill = the way the ice flows
T = (-F[1], F[0])                    # across the flow
print("slope %.1f deg, flow (%.2f, %.2f)" % (math.degrees(math.atan(fl)), F[0], F[1]))

meshes = {}
for cls, k in (("R", 6), ("B", 6), ("S", 6), ("W", 6)):
    meshes[cls] = [unreal.load_asset(f"{D}/SM_Ice{cls}_{i:02d}") for i in range(1, k + 1)]
ext = {m.get_name(): m.get_bounds().box_extent for ms in meshes.values() for m in ms}

act = eas.spawn_actor_from_class(unreal.Actor, unreal.Vector(cx, cy, 0)); act.set_actor_label(LABEL)
act.set_folder_path("Environment/Icefall")
sub = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem); root = sub.k2_gather_subobject_data_for_instance(act)[0]
comps = {}
def comp(m):
    if m.get_name() not in comps:
        h, _ = sub.add_new_subobject(unreal.AddNewSubobjectParams(parent_handle=root, new_class=unreal.HierarchicalInstancedStaticMeshComponent))
        c = unreal.SubobjectDataBlueprintFunctionLibrary.get_object(unreal.SubobjectDataBlueprintFunctionLibrary.get_data(h))
        c.set_static_mesh(m); c.set_cast_shadow(True)
        c.set_editor_property("can_ever_affect_navigation", False)
        comps[m.get_name()] = (c, [])
    return comps[m.get_name()][1]

placed = []   # (x, y, radius) footprints already used
OVERLAP = {"W": 0.7, "S": 0.6, "B": 0.42, "R": 0.3}   # small pieces may lean into / stack against big ones: a jumble
HW = cfg.get("half_width_m", 60) * 100.0
def put(cls, x, y, yaw_deg, scale, clear_cm, tilt_deg, sink):
    m = rng.choice(meshes[cls]); e = ext[m.get_name()]
    r = max(e.x, e.y) * scale * 0.85
    if path_dist(x, y) - r < clear_cm: return False
    if any(math.hypot(x - px, y - py) < (r + pr) * OVERLAP.get(cls, 0.5) for px, py, pr in placed): return False
    g = ground(x, y)
    if not g: return False
    z, nrm = g
    rot = unreal.Rotator(roll=rng.uniform(-tilt_deg, tilt_deg), pitch=rng.uniform(-tilt_deg, tilt_deg), yaw=yaw_deg)
    sc = unreal.Vector(scale * rng.uniform(0.9, 1.1), scale * rng.uniform(0.9, 1.1), scale * rng.uniform(0.85, 1.15))
    comp(m).append(unreal.Transform(unreal.Vector(x, y, z - e.z * 2 * scale * sink), rot, sc))
    placed.append((x, y, r))
    return True

yaw_T = math.degrees(math.atan2(T[1], T[0]))
counts = {"R": 0, "B": 0, "S": 0, "W": 0}
rows = []
along = -HW
while along < HW:
    rows.append(along); along += rng.uniform(1000, 1500)
# pass 1: the big pieces on every row crest first (towers and walls are what the eye reads from afar; small pieces
# placed first used to crowd them out)
for row_f in rows:
    t = -HW
    while t < HW:
        x = cx + F[0] * row_f + T[0] * t; y = cy + F[1] * row_f + T[1] * t
        r = rng.random()
        if r < 0.3 and put("W", x, y, yaw_T + rng.uniform(-14, 14), rng.uniform(0.8, 1.15), 450, 5, 0.18): counts["W"] += 1; t += 2000; continue
        if r < 0.6 and put("S", x + F[0] * rng.uniform(-300, 300), y + F[1] * rng.uniform(-300, 300), rng.uniform(0, 360), rng.uniform(0.75, 1.1), 450, 8, 0.15): counts["S"] += 1; t += 1200; continue
        t += rng.uniform(400, 800)
# pass 2: blocks and rubble
for row_f in rows:
    # blocks along and just below the crest, denser near the path
    for _ in range(int(2 * HW / 160)):
        t = rng.uniform(-HW, HW); off = rng.uniform(-300, 900)
        x = cx + F[0] * (row_f + off) + T[0] * t; y = cy + F[1] * (row_f + off) + T[1] * t
        if put("B", x, y, rng.uniform(0, 360), rng.uniform(0.6, 1.6), 130, 16, 0.22): counts["B"] += 1
    # rubble spilling downhill from the row
    for _ in range(int(2 * HW / 45)):
        t = rng.uniform(-HW, HW); off = rng.uniform(100, 1200)
        x = cx + F[0] * (row_f + off) + T[0] * t; y = cy + F[1] * (row_f + off) + T[1] * t
        if put("R", x, y, rng.uniform(0, 360), rng.uniform(0.6, 1.8), 90, 32, 0.25): counts["R"] += 1
# the trench of the path: blocks and rubble lining it on both sides (the walking-height view)
for px, py in dense:
    for side in (-1, 1):
        # small sharp pieces right at the edge of the track (you step past them), then blocks crowding in behind
        for _ in range(3):
            d = rng.uniform(55, 140) * side
            x = px + T[0] * d + rng.uniform(-120, 120); y = py + T[1] * d + rng.uniform(-120, 120)
            if put("R", x, y, rng.uniform(0, 360), rng.uniform(0.35, 0.8), 45, 35, 0.35): counts["R"] += 1
        for _ in range(12):
            d = rng.uniform(90, 600) * side
            x = px + T[0] * d + rng.uniform(-150, 150); y = py + T[1] * d + rng.uniform(-150, 150)
            if rng.random() < 0.45:
                if put("B", x, y, rng.uniform(0, 360), rng.uniform(0.5, 1.1), 85, 18, 0.25): counts["B"] += 1
            elif put("R", x, y, rng.uniform(0, 360), rng.uniform(0.7, 1.8), 60, 32, 0.28): counts["R"] += 1

for name, (c, xs) in comps.items():
    for i in range(0, len(xs), 2000):
        c.add_instances(xs[i:i + 2000], False, True)
print("placed", counts, "actors comps", len(comps))
