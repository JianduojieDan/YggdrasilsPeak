# Runs INSIDE the editor. Re-seats and re-sizes every PCG_Rocks instance. Burying below the LOWEST ground point sank rocks on
# slopes completely into the mountain (their shadows then came up through the snow, as the landscape casts none). Now: tilted
# fully to the slope, 15% sunk at the CENTRE of the footprint, sizes from small to huge, clear of route, ropes and camps.
# Scales multiply what is there: run once per fresh placement.
import unreal, math, json, random
ROOT = "F:/UE_project/YggdrasilsPeak"
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
w = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
acts = eas.get_all_level_actors()
v = [a for a in acts if a.get_actor_label() == "PCG_Rocks"][0]
ignore = [v]
def ground(x, y, z):
    h = unreal.SystemLibrary.line_trace_single(w, unreal.Vector(x, y, z + 80000), unreal.Vector(x, y, z - 150000),
        unreal.TraceTypeQuery.ECC_VISIBILITY, True, ignore, unreal.DrawDebugTrace.NONE, True)
    return h.to_tuple()[4].z if h else None
# route, ropes and camps on a coarse grid for quick distance checks
CELL = 10000.0
keep = {}
def mark(x, y, clear):
    k = (int(x // CELL), int(y // CELL))
    keep.setdefault(k, []).append((x, y, clear))
for leg in json.load(open(f"{ROOT}/Tools/RoutePlanning/route.json")):
    for p in leg["pts"]:
        mark(p[0], p[1], 400.0)
for a in acts:
    n = a.get_class().get_name()
    if n == "FixedRope":
        for p in a.get_editor_property("rope_points"):
            mark(p.x, p.y, 400.0)
    elif n == "RouteLandmark":
        l = a.get_actor_location()
        mark(l.x, l.y, 5000.0)
def blocked(x, y, r):
    reach = int((r + 5000.0) // CELL) + 1
    cx, cy = int(x // CELL), int(y // CELL)
    for i in range(cx - reach, cx + reach + 1):
        for j in range(cy - reach, cy + reach + 1):
            for px, py, clear in keep.get((i, j), ()):
                if math.hypot(px - x, py - y) < r + clear:
                    return True
    return False
rng = random.Random(4242)
def size_factor():
    r = rng.random()
    if r < 0.55: return rng.uniform(0.25, 0.6)   # small, most of them
    if r < 0.90: return rng.uniform(0.6, 1.3)    # medium
    if r < 0.985: return rng.uniform(1.3, 2.5)   # big
    return rng.uniform(3.0, 5.0)                 # a rare giant
moved = dropped = 0
sizes = []
for c in v.get_components_by_class(unreal.InstancedStaticMeshComponent):
    n = c.get_instance_count()
    if not n:
        continue
    b = c.static_mesh.get_bounding_box()
    rx, ry, hz = (b.max.x - b.min.x) / 2, (b.max.y - b.min.y) / 2, (b.max.z - b.min.z)
    drop = []
    for i in range(n):
        t = c.get_instance_transform(i, True)
        sc = t.scale3d * size_factor()
        s = sc.x; P = t.translation; yaw = t.rotation.rotator().yaw
        r = 0.8 * max(rx, ry) * s
        if blocked(P.x, P.y, r):
            drop.append(i); continue
        g0 = ground(P.x, P.y, P.z)
        if g0 is None:
            drop.append(i); continue
        pts = [(0.0, 0.0, g0)]
        for k in range(8):
            a = k * math.pi / 4; x, y = r * math.cos(a), r * math.sin(a)
            g = ground(P.x + x, P.y + y, P.z)
            if g is not None:
                pts.append((x, y, g))
        cnt = len(pts)
        sxx = sum(p[0] ** 2 for p in pts) or 1.0; syy = sum(p[1] ** 2 for p in pts) or 1.0
        ga = sum(p[0] * (p[2] - g0) for p in pts) / sxx; gb = sum(p[1] * (p[2] - g0) for p in pts) / syy
        resid = max(abs(p[2] - (g0 + ga * p[0] + gb * p[1])) for p in pts)
        if resid > 0.45 * hz * s:      # over a drop or knife edge: it would hang in the air
            drop.append(i); continue
        pitch = -math.degrees(math.atan(ga)); roll = math.degrees(math.atan(gb))
        q = unreal.Rotator(roll=0, pitch=0, yaw=yaw).quaternion()
        rot = (unreal.Rotator(roll=roll, pitch=pitch, yaw=0).quaternion() * q).rotator()
        z = g0 - b.min.z * s - 0.15 * hz * s
        c.update_instance_transform(i, unreal.Transform(unreal.Vector(P.x, P.y, z), rot, sc), True, True, True)
        moved += 1
        sizes.append(2 * max(rx, ry) * s / 100.0)
    if drop:
        c.remove_instances(drop); dropped += len(drop)
sizes.sort()
print("placed", moved, "dropped", dropped, "size m min/median/p95/max", round(sizes[0], 1), round(sizes[len(sizes) // 2], 1), round(sizes[int(len(sizes) * .95)], 1), round(sizes[-1], 1))
unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
print("saved")
