# Runs INSIDE the editor (uepy.py). 2026-11-11 user: the summit camp is swallowed by the World Tree -> a new summit camp
# next to the tree, the end point of the climb (Landmark_8, Kind Summit, ends the expedition: end screen), and the tree
# decorated: long lines of prayer flags strung from the trunk down to the snow all round it, golden motes drifting in
# the crown, warm light at the roots.
import unreal, math, random, json
rng = random.Random(8849)
ROOT = "F:/UE_project/YggdrasilsPeak"
ea = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
PEAK = (1239630.0, 1413610.0, 381988.0)
for a in ea.get_all_level_actors():
    if a.get_actor_label().startswith(("SummitCamp_", "WorldTree_Flags", "WorldTree_Motes", "WorldTree_RootLight")):
        ea.destroy_actor(a)
acts = ea.get_all_level_actors()
tree = [a for a in acts if a.get_actor_label() == "WorldTree_Yggdrasil"][0]
skip = [a for a in acts if a.get_actor_label().startswith(("Rocks_", "Icefall_", "Glacier_", "Summit_", "Landmark_", "WorldTree_Crown", "WorldTree_Glow"))]
def hit(x, y, with_tree):
    ig = skip + ([] if with_tree else [tree])
    h = unreal.SystemLibrary.line_trace_single_for_objects(world, unreal.Vector(x, y, 900000), unreal.Vector(x, y, 0),
        [unreal.ObjectTypeQuery.OBJECT_TYPE_QUERY1], True, ig, unreal.DrawDebugTrace.NONE, True)
    if not h: return None
    t = h.to_tuple()
    return t[4].z, t[7].z, (t[9].get_actor_label() if t[9] else "")

# ---- the camp: the flattest open ground 35-130 m from the trunk, preferring the side the route arrives from
route = [p for l in json.load(open(f"{ROOT}/Tools/RoutePlanning/route.json")) for p in l["pts"]]
ax, ay = route[-12][0] - PEAK[0], route[-12][1] - PEAK[1]
arrive = math.atan2(ay, ax)
best = None
for r in range(3500, 13001, 700):
    for k in range(48):
        a = k / 48 * 2 * math.pi
        x, y = PEAK[0] + math.cos(a) * r, PEAK[1] + math.sin(a) * r
        g = hit(x, y, True)
        if not g or g[2].startswith("WorldTree"): continue            # roots or trunk there
        z, nz, _ = g
        hs = [hit(x + dx, y + dy, False) for dx, dy in ((600, 0), (-600, 0), (0, 600), (0, -600))]
        if any(h is None for h in hs): continue
        rough = max(abs(h[0] - z) for h in hs)
        score = rough / 100.0 + (1.0 - nz) * 40.0 + abs(math.atan2(math.sin(a - arrive), math.cos(a - arrive))) * 1.5 + r / 10000.0
        if best is None or score < best[0]: best = (score, x, y, z, nz, rough, a)
print("camp spot", [round(v, 2) for v in best])
_, cx, cy, cz, _, _, ca = best
face = math.degrees(ca) + 180.0     # tents face the tree
def seat(x, y):
    g = hit(x, y, False)
    return g[0] if g else cz

# the summit landmark (prayer-flag cairn; reaching it ends the climb) and the two oxygen bottles move here
for a in acts:
    lab = a.get_actor_label()
    if lab == "Landmark_8":
        a.set_actor_location(unreal.Vector(cx, cy, seat(cx, cy)), False, True)
    elif lab.startswith("Summit_O2Pickup_"):
        i = int(lab[-1]); x, y = cx + 350 + i * 120, cy - 250
        a.set_actor_location(unreal.Vector(x, y, seat(x, y) + 15), False, True)

TENT = unreal.load_asset("/Game/Fab/Modern_Camping_Tent/modern_camping_tent__GLTF_/StaticMeshes/modern_camping_tent__GLTF_")
O2 = unreal.load_asset("/Game/Fab/Oxygen_Tank/oxygen_tank/StaticMeshes/oxygen_tank")
STONE = unreal.load_asset("/Game/Fab/Stone_-_photogrammetry/stone_photogrammetry/StaticMeshes/stone_photogrammetry")
AXE = unreal.load_asset("/Game/Fab/Climbing_Axe_for_Blender/climbing_axe_422_packed/StaticMeshes/climbing_axe_422_packed")
ROPE = unreal.load_asset("/Game/Fab/Megascans/3D/Bundled_Rope_Coil_vhnmbdcs/Raw/vhnmbdcs_tier_0/StaticMeshes/vhnmbdcs_tier_0")
def prop(label, mesh, x, y, yaw, s, dz=0.0, pitch=0.0):
    a = ea.spawn_actor_from_object(mesh, unreal.Vector(x, y, seat(x, y) + dz), unreal.Rotator(roll=0, pitch=pitch, yaw=yaw))
    a.set_actor_label("SummitCamp_" + label); a.set_folder_path("Camps/SummitCamp"); a.tags = ["CampProp"]
    a.set_actor_scale3d(unreal.Vector(s, s, s))
    return a
for i, (dx, dy) in enumerate(((-900, 600), (-500, -900), (700, 900))):
    rx = cx + dx * math.cos(ca) - dy * math.sin(ca); ry = cy + dx * math.sin(ca) + dy * math.cos(ca)
    prop(f"Tent{i}", TENT, rx, ry, face + rng.uniform(-25, 25), 2.0, -8)
for i in range(4):
    prop(f"O2_{i}", O2, cx + 200 + i * 45, cy + 120 + rng.uniform(-30, 30), rng.uniform(0, 360), 1.3, 10, rng.choice((0, 80)))
prop("IceAxe", AXE, cx - 150, cy + 220, rng.uniform(0, 360), 1.5, 30, 70)
prop("Rope", ROPE, cx + 80, cy - 200, rng.uniform(0, 360), 1.8, 5)
for i in range(14):   # a low windbreak of stones round the cairn
    a = i / 14 * 2 * math.pi
    prop(f"Stone{i}", STONE, cx + math.cos(a) * 420, cy + math.sin(a) * 420, rng.uniform(0, 360), rng.uniform(0.05, 0.08), -10)

# ---- prayer flags strung from the trunk down to the snow all round the tree
FLAG = unreal.load_asset("/Game/Environment/Landmarks/SM_PrayerFlag")
CORD = unreal.load_asset("/Engine/BasicShapes/Cylinder")
cols = [unreal.load_asset(f"/Game/Environment/Landmarks/MI_PrayerFlag_{c}") for c in ("Blue", "White", "Red", "Green", "Yellow")]
white = unreal.load_asset("/Game/Environment/Landmarks/MI_Landmark_White")
flags_actor = ea.spawn_actor_from_class(unreal.Actor, unreal.Vector(*PEAK))
flags_actor.set_actor_label("WorldTree_Flags"); flags_actor.set_folder_path("Environment/WorldTree")
sub = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem); root = sub.k2_gather_subobject_data_for_instance(flags_actor)[0]
def ism(mesh, mat):
    h, _ = sub.add_new_subobject(unreal.AddNewSubobjectParams(parent_handle=root, new_class=unreal.InstancedStaticMeshComponent))
    c = unreal.SubobjectDataBlueprintFunctionLibrary.get_object(unreal.SubobjectDataBlueprintFunctionLibrary.get_data(h))
    c.set_static_mesh(mesh); c.set_material(0, mat); c.set_cast_shadow(True)
    c.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION); c.set_editor_property("visible_in_ray_tracing", False)
    return c
flag_x = [[] for _ in cols]; cord_x = []
STRINGS = 20
for s in range(STRINGS):
    a = s / STRINGS * 2 * math.pi + rng.uniform(-0.12, 0.12)
    r0, r1 = rng.uniform(1800, 2600), rng.uniform(9000, 15000)
    x0, y0 = PEAK[0] + math.cos(a) * r0, PEAK[1] + math.sin(a) * r0
    z0 = PEAK[2] + rng.uniform(3500, 7000)                       # tied high in the trunk and lower branches
    x1, y1 = PEAK[0] + math.cos(a) * r1, PEAK[1] + math.sin(a) * r1
    g1 = hit(x1, y1, False)
    if not g1: continue
    z1 = g1[0] + 150
    length = math.dist((x0, y0, z0), (x1, y1, z1))
    n = int(length / 560)   # big flags for a giant tree: 4 x 5.5 m, every 5.6 m
    pts = []
    for i in range(n + 1):
        t = i / n
        pts.append(unreal.Vector(x0 + (x1 - x0) * t, y0 + (y1 - y0) * t, z0 + (z1 - z0) * t - math.sin(t * math.pi) * length * 0.06))
    yaw = math.degrees(a) + 90.0
    for i in range(n):
        p0, p1 = pts[i], pts[i + 1]
        d = p1 - p0
        cord_x.append(unreal.Transform((p0 + p1) * 0.5, unreal.MathLibrary.make_rot_from_z(d), unreal.Vector(0.08, 0.08, d.length() / 100.0)))
        if i > 0:
            flag_x[i % len(cols)].append(unreal.Transform(p0 - unreal.Vector(0, 0, 280), unreal.Rotator(roll=90, pitch=0, yaw=yaw), unreal.Vector(4.0, 5.5, 1.0)))
for c, xs in zip(cols, flag_x):
    ism(FLAG, c).add_instances(xs, False, True)
ism(CORD, white).add_instances(cord_x, False, True)
print("flags", sum(len(x) for x in flag_x), "cord segments", len(cord_x))

# ---- golden motes drifting in the crown and round the trunk, warm light at the roots
ns = unreal.load_asset("/Game/Environment/Realms/NS_RealmGold")
for i, (dz, s) in enumerate(((20000, 30.0), (45000, 40.0), (8000, 15.0))):
    m = ea.spawn_actor_from_class(unreal.NiagaraActor, unreal.Vector(PEAK[0], PEAK[1], PEAK[2] + dz))
    m.set_actor_label(f"WorldTree_Motes{i}"); m.set_folder_path("Environment/WorldTree")
    m.niagara_component.set_asset(ns); m.set_actor_scale3d(unreal.Vector(s, s, s))
for i in range(4):
    a = i / 4 * 2 * math.pi + 0.4
    x, y = PEAK[0] + math.cos(a) * 2500, PEAK[1] + math.sin(a) * 2500
    l = ea.spawn_actor_from_class(unreal.PointLight, unreal.Vector(x, y, seat(x, y) + 600))
    l.set_actor_label(f"WorldTree_RootLight{i}"); l.set_folder_path("Environment/WorldTree")
    pl = l.point_light_component
    pl.set_editor_property("intensity_units", unreal.LightUnits.CANDELAS)
    pl.set_intensity(60.0); pl.set_light_color(unreal.LinearColor(1.0, 0.72, 0.4, 1.0)); pl.set_attenuation_radius(6000.0); pl.set_cast_shadows(False)
print("saved", unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level())
