# Runs INSIDE the editor. PCG_Rocks holds true rocks only; cliff meshes are for hand placement on the route, never scatter.
# Each rock's up axis is aligned to the ground plane under it, sunk 15% at the centre of its footprint. Sizes are kept.
# GROW multiplies existing sizes, so set it back to 1.0 after a run. Rocks too near the route, ropes or camps are removed.
import unreal, math, json
CLIFFS = ("cliff",)
GROW = 1.0   # 2.5 was run once on 2026-11-06
keep = []
for leg in json.load(open("F:/UE_project/YggdrasilsPeak/Tools/RoutePlanning/route.json")):
    keep += [(p[0], p[1], 400.0) for p in leg["pts"]]
for a in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors():
    n = a.get_class().get_name()
    if n == "FixedRope":
        keep += [(p.x, p.y, 400.0) for p in a.get_editor_property("rope_points")]
    elif n == "RouteLandmark":
        l = a.get_actor_location(); keep.append((l.x, l.y, 5000.0))
CELL = 20000.0
grid = {}
for x, y, r in keep:
    grid.setdefault((int(x // CELL), int(y // CELL)), []).append((x, y, r))
def blocked(x, y, r):
    reach = int((r + 5000.0) // CELL) + 1
    cx, cy = int(x // CELL), int(y // CELL)
    for i in range(cx - reach, cx + reach + 1):
        for j in range(cy - reach, cy + reach + 1):
            for px, py, c in grid.get((i, j), ()):
                if math.hypot(px - x, py - y) < r + c:
                    return True
    return False
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
w = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
v = [a for a in eas.get_all_level_actors() if a.get_actor_label() == "PCG_Rocks"][0]
def ground(x, y, z):
    h = unreal.SystemLibrary.line_trace_single(w, unreal.Vector(x, y, z + 80000), unreal.Vector(x, y, z - 150000),
        unreal.TraceTypeQuery.ECC_VISIBILITY, True, [v], unreal.DrawDebugTrace.NONE, True)
    return h.to_tuple()[4].z if h else None
placed = dropped = cleared = 0
for c in v.get_components_by_class(unreal.InstancedStaticMeshComponent):
    n = c.get_instance_count()
    if not n:
        continue
    if any(k in c.static_mesh.get_path_name().lower() for k in CLIFFS):
        c.clear_instances(); cleared += n
        continue
    b = c.static_mesh.get_bounding_box()
    rx, ry, hz = (b.max.x - b.min.x) / 2, (b.max.y - b.min.y) / 2, (b.max.z - b.min.z)
    drop = []
    for i in range(n):
        t = c.get_instance_transform(i, True)
        sc = t.scale3d * GROW
        s = sc.x; P = t.translation
        yaw = math.radians(t.rotation.rotator().yaw)
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
        sxx = sum(p[0] ** 2 for p in pts) or 1.0; syy = sum(p[1] ** 2 for p in pts) or 1.0
        ga = sum(p[0] * (p[2] - g0) for p in pts) / sxx; gb = sum(p[1] * (p[2] - g0) for p in pts) / syy
        if max(abs(p[2] - (g0 + ga * p[0] + gb * p[1])) for p in pts) > 0.45 * hz * s:
            drop.append(i); continue
        up = unreal.Vector(-ga, -gb, 1.0); up.normalize()
        rot = unreal.MathLibrary.make_rot_from_zx(up, unreal.Vector(math.cos(yaw), math.sin(yaw), 0.0))
        z = g0 - b.min.z * s * up.z - 0.15 * hz * s
        c.update_instance_transform(i, unreal.Transform(unreal.Vector(P.x, P.y, z), rot, sc), True, True, True)
        placed += 1
    if drop:
        c.remove_instances(drop); dropped += len(drop)
print("cliffs cleared", cleared, "rocks placed", placed, "dropped", dropped)
unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
print("saved")
