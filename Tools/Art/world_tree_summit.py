# Runs INSIDE the editor (uepy.py). The World Tree (Yggdrasil, /Game/world_tree/SM_Yggdrasil_LightWithin) on the summit
# of Everest, made big and beautiful for a fantasy world (user, 2026-11-11):
#   - rooted on the real peak (traced), roots sinking into the summit snow, ~500 m tall: the landmark of the mountain;
#   - its bare spiky branch tips crowned with golden ginkgo foliage (Megaplant Library branches, static meshes) - leaves
#     clustered on every branch end, gold like the tree's own resin;
#   - a warm light from the heart of the trunk; complex collision so the climber can reach and touch it.
import unreal, math, random
SCALE_XY = 3.0   # narrower than tall: its roots reach down the peak instead of hanging out over the drop
SCALE = 4.0   # ~700 m tall over the summit: a world tree, seen from the whole valley
rng = random.Random(8848)
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
for a in eas.get_all_level_actors():
    if a.get_actor_label().startswith("WorldTree_"):
        eas.destroy_actor(a)
ignore = [a for a in eas.get_all_level_actors() if a.get_actor_label().startswith(("Rocks_", "Icefall_", "Summit_", "Glacier_", "WorldTree", "Forest_"))]
def ground(x, y):
    h = unreal.SystemLibrary.line_trace_single_for_objects(world, unreal.Vector(x, y, 900000), unreal.Vector(x, y, 0),
        [unreal.ObjectTypeQuery.OBJECT_TYPE_QUERY1], True, ignore, unreal.DrawDebugTrace.NONE, True)
    return h.to_tuple()[4].z if h else None
cx, cy = 1240430.0, 1412410.0
best = None
for i in range(-12, 13):
    for j in range(-12, 13):
        z = ground(cx + i * 400, cy + j * 400)
        if z and (best is None or z > best[2]): best = (cx + i * 400, cy + j * 400, z)
print("peak", best)

mesh = unreal.load_asset("/Game/world_tree/SM_Yggdrasil_LightWithin")
bs = mesh.get_editor_property("body_setup")
if bs and bs.get_editor_property("collision_trace_flag") != unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE:
    bs.set_editor_property("collision_trace_flag", unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE)
    unreal.EditorAssetLibrary.save_loaded_asset(mesh, False)
YAW = 35.0
# Seat the roots on the mountain (2026-11-11 user: roots floated in the air). Every root vertex is put at its place
# for a trial height, the ground under it is traced, and the tree is lowered until most of the root ends touch or
# bite into the slopes (the trunk flare may sink into the summit snow; that reads as gripping the peak).
md0 = mesh.get_static_mesh_description(0)
yr = math.radians(YAW)
roots = []
for i in range(0, md0.get_vertex_count(), 3):
    p = md0.get_vertex_position(unreal.VertexID(i))
    if p.z < 1500 and math.hypot(p.x, p.y) > 1200:
        roots.append(p)
gaps = []
for p in roots[::max(1, len(roots) // 400)]:
    wx = best[0] + (p.x * math.cos(yr) - p.y * math.sin(yr)) * SCALE_XY
    wy = best[1] + (p.x * math.sin(yr) + p.y * math.cos(yr)) * SCALE_XY
    gz = ground(wx, wy)
    if gz is not None:
        gaps.append(best[2] + p.z * SCALE - gz)       # > 0: this root point would hang in the air
gaps.sort()
sink = max(600 * SCALE, gaps[int(len(gaps) * 0.75)] if gaps else 0.0)   # most roots on the slopes, some out in the air, as roots do
sink = min(sink, 9000 * SCALE * 0.5)
print("root points", len(gaps), "air gap 50/80/95%%: %.0f / %.0f / %.0f cm -> sink %.0f cm" % (gaps[len(gaps)//2], gaps[int(len(gaps)*0.8)], gaps[int(len(gaps)*0.95)], sink) if gaps else "no gaps")
origin = unreal.Vector(best[0], best[1], best[2] - sink)
tree = eas.spawn_actor_from_object(mesh, origin, unreal.Rotator(roll=0, pitch=0, yaw=YAW))
tree.set_actor_label("WorldTree_Yggdrasil"); tree.set_folder_path("Environment/WorldTree")
tree.set_actor_scale3d(unreal.Vector(SCALE_XY, SCALE_XY, SCALE))
tree.static_mesh_component.set_mobility(unreal.ComponentMobility.STATIC)
tree.static_mesh_component.set_collision_profile_name("BlockAll")

# ---- branch tips from the mesh itself: vertices high and far out, grouped in 10 m cells
md = mesh.get_static_mesh_description(0)
pts = []
for i in range(md.get_vertex_count()):
    p = md.get_vertex_position(unreal.VertexID(i))
    pts.append((p.x, p.y, p.z))
zmax = max(p[2] for p in pts)
cells = {}
for x, y, z in pts:
    r = math.hypot(x, y)
    if z < 0.35 * zmax or (r < 2500 and z < 0.8 * zmax):
        continue      # roots and the bare trunk stay bare
    k = (int(x // 1000), int(y // 1000), int(z // 1000))
    c = cells.setdefault(k, [0, 0, 0, 0])
    c[0] += x; c[1] += y; c[2] += z; c[3] += 1
cent = [(c[0] / c[3], c[1] / c[3], c[2] / c[3]) for c in cells.values()]
# spread the crown points: farthest-point sampling, favouring the outer and higher ends
cent.sort(key=lambda p: -(math.hypot(p[0], p[1]) + p[2] * 0.6))
chosen = [cent[0]]
while len(chosen) < min(110, len(cent)):
    nxt = max(cent, key=lambda p: min((p[0] - q[0]) ** 2 + (p[1] - q[1]) ** 2 + (p[2] - q[2]) ** 2 for q in chosen))
    chosen.append(nxt)
print("crown points", len(chosen), "of", len(cent), "cells")

# ---- golden ginkgo foliage
D = "/Game/Megaplant_Library/Tree_Ginkgo/Instances/"
branches = [unreal.load_asset(D + n) for n in ["Ginkgo_Branch_01", "Ginkgo_Branch_02", "Ginkgo_Branch_03", "Ginkgo_Branch_04", "Ginkgo_Branch_05", "Ginkgo_Spur_01", "Ginkgo_Spur_02", "Ginkgo_Spur_03"]]
branches = [b for b in branches if b]
at = unreal.AssetToolsHelpers.get_asset_tools()
gold = unreal.load_asset("/Game/world_tree/MI_WorldTree_GoldLeaves")
if not gold:
    gold = at.create_asset("MI_WorldTree_GoldLeaves", "/Game/world_tree", unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    gold.set_editor_property("parent", unreal.load_asset("/Game/Megaplant_Library/Tree_Ginkgo/Materials/MI_Ginkgo_Foliage_01"))
ML = unreal.MaterialEditingLibrary
for name, col in [("Season Color 1", (1.0, 0.72, 0.18)), ("Season Color 2", (1.0, 0.55, 0.12)), ("BaseColor Tint Leaves", (1.0, 0.80, 0.35)),
                  ("Translucency Tint", (1.0, 0.70, 0.25))]:
    ML.set_material_instance_vector_parameter_value(gold, name, unreal.LinearColor(*col, 1.0))
for name, v in [("Health Offset", -1.0), ("Noise Tint Tiling", 0.3)]:
    ML.set_material_instance_scalar_parameter_value(gold, name, v)
for name, col in [("BaseColor Tint", (1.0, 0.78, 0.30)), ("BaseColor Noise Tint", (1.0, 0.62, 0.15))]:
    ML.set_material_instance_vector_parameter_value(gold, name, unreal.LinearColor(*col, 1.0))
unreal.EditorAssetLibrary.save_loaded_asset(gold, False)
foliage_mi = unreal.load_asset("/Game/Megaplant_Library/Tree_Ginkgo/Materials/MI_Ginkgo_Foliage_01")

crown = eas.spawn_actor_from_class(unreal.Actor, origin)
crown.set_actor_label("WorldTree_Crown"); crown.set_folder_path("Environment/WorldTree")
sub = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem); root = sub.k2_gather_subobject_data_for_instance(crown)[0]
comps = {}
def comp(m):
    if m.get_name() not in comps:
        h, _ = sub.add_new_subobject(unreal.AddNewSubobjectParams(parent_handle=root, new_class=unreal.InstancedStaticMeshComponent))
        c = unreal.SubobjectDataBlueprintFunctionLibrary.get_object(unreal.SubobjectDataBlueprintFunctionLibrary.get_data(h))
        c.set_static_mesh(m); c.set_cast_shadow(True)
        c.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
        c.set_editor_property("visible_in_ray_tracing", False)
        for si, s in enumerate(m.static_materials):
            if s.material_interface and s.material_interface.get_name() == foliage_mi.get_name():
                c.set_material(si, gold)
        comps[m.get_name()] = (c, [])
    return comps[m.get_name()][1]
yawr = math.radians(YAW)
def to_world(p):
    x, y, z = p[0] * SCALE_XY, p[1] * SCALE_XY, p[2] * SCALE
    return unreal.Vector(origin.x + x * math.cos(yawr) - y * math.sin(yawr), origin.y + x * math.sin(yawr) + y * math.cos(yawr), origin.z + z)
n = 0
for p in chosen:
    c = to_world(p)
    out = math.degrees(math.atan2(p[1], p[0])) + YAW
    for _ in range(rng.randint(7, 12)):
        m = rng.choice(branches)
        s = rng.uniform(9.0, 16.0) * SCALE        # a 1 m branch -> 18-32 m bunches of leaves
        off = unreal.Vector(rng.uniform(-900, 900), rng.uniform(-900, 900), rng.uniform(-700, 500)) * SCALE * 0.6
        rot = unreal.Rotator(roll=rng.uniform(-60, 60), pitch=rng.uniform(-75, 25), yaw=out + rng.uniform(-80, 80))
        comp(m).append(unreal.Transform(c + off, rot, unreal.Vector(s, s, s)))
        n += 1
for name, (cc, xs) in comps.items():
    cc.add_instances(xs, False, True)
print("crown branches", n)

glow = eas.spawn_actor_from_class(unreal.PointLight, unreal.Vector(best[0], best[1], best[2] + 6000 * SCALE))
glow.set_actor_label("WorldTree_Glow"); glow.set_folder_path("Environment/WorldTree")
pl = glow.point_light_component
pl.set_editor_property("intensity_units", unreal.LightUnits.CANDELAS)
pl.set_intensity(400.0); pl.set_light_color(unreal.LinearColor(1.0, 0.78, 0.45, 1.0))
pl.set_attenuation_radius(80000.0); pl.set_cast_shadows(False)
print("saved", unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level())
