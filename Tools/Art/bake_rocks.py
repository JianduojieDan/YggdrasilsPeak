# Runs INSIDE the editor. PCG_Rocks regenerated itself after landscape edits, bringing back cliff meshes and old sizes. This takes
# its true rocks, sizes them (small to huge), seats them on the ground clear of route and camps, and stores them in a plain actor
# "Rocks_Baked". PCG_Rocks is switched off. Rerun after terrain edits only with RESEAT_ONLY = True, which keeps the sizes.
import unreal, math, json, random
RESEAT_ONLY = False
GROW = 2.5
ROOT = "F:/UE_project/YggdrasilsPeak"
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
w = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
acts = eas.get_all_level_actors()
pcg = [a for a in acts if a.get_actor_label() == "PCG_Rocks"][0]
baked = ([a for a in acts if a.get_actor_label() == "Rocks_Baked"] or [None])[0]

keep = []
for leg in json.load(open(f"{ROOT}/Tools/RoutePlanning/route.json")):
    keep += [(p[0], p[1], 400.0) for p in leg["pts"]]
for a in acts:
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
    return any(math.hypot(px - x, py - y) < r + c for i in range(cx - reach, cx + reach + 1) for j in range(cy - reach, cy + reach + 1) for px, py, c in grid.get((i, j), ()))
ignore = [pcg] + ([baked] if baked else [])
def ground(x, y, z):
    h = unreal.SystemLibrary.line_trace_single(w, unreal.Vector(x, y, z + 80000), unreal.Vector(x, y, z - 150000), unreal.TraceTypeQuery.ECC_VISIBILITY, True, ignore, unreal.DrawDebugTrace.NONE, True)
    return h.to_tuple()[4].z if h else None
rng = random.Random(4242)
def factor():
    if RESEAT_ONLY: return 1.0
    r = rng.random()
    f = rng.uniform(0.25, 0.6) if r < 0.55 else rng.uniform(0.6, 1.3) if r < 0.90 else rng.uniform(1.3, 2.5) if r < 0.985 else rng.uniform(3.0, 5.0)
    return f * GROW

source = baked if RESEAT_ONLY else pcg
out = {}
widths = []
dropped = 0
for c in source.get_components_by_class(unreal.InstancedStaticMeshComponent):
    n = c.get_instance_count()
    mesh = c.static_mesh
    if not n or "cliff" in mesh.get_path_name().lower():
        continue
    b = mesh.get_bounding_box()
    rx, ry, hz = (b.max.x - b.min.x) / 2, (b.max.y - b.min.y) / 2, b.max.z - b.min.z
    lst = out.setdefault(mesh.get_path_name(), [])
    for i in range(n):
        t = c.get_instance_transform(i, True)
        sc = t.scale3d * factor()
        s = sc.x; P = t.translation; yaw = math.radians(t.rotation.rotator().yaw)
        r = 0.8 * max(rx, ry) * s
        if blocked(P.x, P.y, r):
            dropped += 1; continue
        g0 = ground(P.x, P.y, P.z)
        if g0 is None:
            dropped += 1; continue
        pts = [(0.0, 0.0, g0)]
        for k in range(8):
            a = k * math.pi / 4; x, y = r * math.cos(a), r * math.sin(a)
            g = ground(P.x + x, P.y + y, P.z)
            if g is not None: pts.append((x, y, g))
        sxx = sum(p[0] ** 2 for p in pts) or 1.0; syy = sum(p[1] ** 2 for p in pts) or 1.0
        ga = sum(p[0] * (p[2] - g0) for p in pts) / sxx; gb = sum(p[1] * (p[2] - g0) for p in pts) / syy
        if max(abs(p[2] - (g0 + ga * p[0] + gb * p[1])) for p in pts) > 0.45 * hz * s:
            dropped += 1; continue
        up = unreal.Vector(-ga, -gb, 1.0); up.normalize()
        rot = unreal.MathLibrary.make_rot_from_zx(up, unreal.Vector(math.cos(yaw), math.sin(yaw), 0.0))
        lst.append(unreal.Transform(unreal.Vector(P.x, P.y, g0 - b.min.z * s * up.z - 0.15 * hz * s), rot, sc))
        widths.append(2 * max(rx, ry) * s / 100.0)

if baked is None:
    baked = eas.spawn_actor_from_class(unreal.Actor, unreal.Vector(0, 0, 0))
    baked.set_actor_label("Rocks_Baked")
    sub = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
    root = sub.k2_gather_subobject_data_for_instance(baked)[0]
    for path in out:
        h, reason = sub.add_new_subobject(unreal.AddNewSubobjectParams(parent_handle=root, new_class=unreal.HierarchicalInstancedStaticMeshComponent))
        comp = unreal.SubobjectDataBlueprintFunctionLibrary.get_object(unreal.SubobjectDataBlueprintFunctionLibrary.get_data(h))
        comp.set_static_mesh(unreal.load_asset(path))
for c in baked.get_components_by_class(unreal.InstancedStaticMeshComponent):
    c.clear_instances()
    c.set_cast_shadow(True)
    c.add_instances(out[c.static_mesh.get_path_name()], False, True)
    print("baked", c.static_mesh.get_name(), c.get_instance_count())
if not RESEAT_ONLY:
    pc = pcg.get_components_by_class(unreal.PCGComponent)[0]
    pc.set_editor_property("activated", False)
    for c in pcg.get_components_by_class(unreal.InstancedStaticMeshComponent):
        c.clear_instances()
widths.sort()
print("rocks", len(widths), "dropped", dropped, "width m min/median/p95/max", round(widths[0], 1), round(widths[len(widths) // 2], 1), round(widths[int(len(widths) * .95)], 1), round(widths[-1], 1))
unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
print("saved")
