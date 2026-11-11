# Runs INSIDE the editor (uepy.py). A fantasy forest in the valley below Dingboche (the only ground under ~4,400 m on the
# map) from the user's Megaplant Library trees (Nanite skeletal foliage; needs r.Nanite.Foliage=True), placed as
# InstancedSkinnedMeshComponents on one actor per species ("Forest_<species>"):
#   - Norway spruce woods in patches on the valley sides (< 4,200 m), the backbone of the forest;
#   - black poplars on the flat valley floor (< 4,050 m);
#   - rare golden ginkgo groves (fantasy accent, gold like the World Tree on the summit);
#   - twisted dead pines thinning out at the treeline (4,150-4,450 m);
#   - dry reeds in clumps on the flats.
# Kept off the route (14 m) and away from the Dingboche camp; slope limits keep trees off cliffs.
import unreal, json, math, random
ROOT = "F:/UE_project/YggdrasilsPeak"
rng = random.Random(4410)
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
for a in eas.get_all_level_actors():
    if a.get_actor_label().startswith("Forest_"):
        eas.destroy_actor(a)
ignore = [a for a in eas.get_all_level_actors() if a.get_actor_label().startswith(("Rocks_", "Icefall_", "Glacier_", "WorldTree"))]
route = [p[:2] for l in json.load(open(f"{ROOT}/Tools/RoutePlanning/route.json")) for p in l["pts"]]
start = route[0]
near_route = [p for p in route if -520000 < p[0] < 420000 and 2230000 < p[1] < 2920000]
def off_route(x, y):
    if math.hypot(x - start[0], y - start[1]) < 9000: return False       # Dingboche camp
    return all(math.hypot(x - px, y - py) > 1400 for px, py in near_route)

def H(x, y):
    return (math.sin(x * 12.9898 + y * 78.233) * 43758.5453) % 1.0
def vn(x, y):
    i, j = math.floor(x), math.floor(y); fx, fy = x - i, y - j
    fx, fy = fx * fx * (3 - 2 * fx), fy * fy * (3 - 2 * fy)
    a, b, c, d = H(i, j), H(i + 1, j), H(i, j + 1), H(i + 1, j + 1)
    return a + (b - a) * fx + (c - a) * fy + (a - b - c + d) * fx * fy
def fbm(x, y):
    return 0.5 * vn(x, y) + 0.3 * vn(x * 2.1 + 5.2, y * 2.1 + 1.3) + 0.2 * vn(x * 4.3 + 9.1, y * 4.3 + 7.7)

def ground(x, y):
    h = unreal.SystemLibrary.line_trace_single_for_objects(world, unreal.Vector(x, y, 900000), unreal.Vector(x, y, -500000),
        [unreal.ObjectTypeQuery.OBJECT_TYPE_QUERY1], True, ignore, unreal.DrawDebugTrace.NONE, True)
    if not h: return None
    t = h.to_tuple()
    lab = t[9].get_actor_label() if t[9] else ""
    if not lab.startswith(("Landscape", "DetailTile")): return None
    return t[4].z, t[7].z

L = "/Game/Megaplant_Library/"
SPECIES = {
    "Spruce": [f"{L}Tree_Norway_Spruce/Tree_Norway_Spruce_01/Tree_Norway_Spruce_01_{v}" for v in "CD"],   # C/D: a third of the memory of A/B
    "Poplar": [f"{L}Tree_Black_Poplar/Tree_Black_Poplar_01/Tree_Black_Poplar_01_{v}" for v in "ABCD"],
    "Ginkgo": [f"{L}Tree_Ginkgo/Tree_Ginkgo_01/Tree_Ginkgo_01_{v}" for v in "CD"],
    "DeadPine": [f"{L}Tree_Dead_Pine/Tree_Dead_Pine_01/Tree_Dead_Pine_{v}" for v in "ABCDE"],
    "Reeds": [f"{L}Plant_Dry_Common_Reeds/Plant_Dry_Common_Reeds_01/Plant_Dry_Common_Reed_{v}" for v in "ABCD"],
}
meshes = {k: [m for m in (unreal.load_asset(p) for p in v) if m] for k, v in SPECIES.items()}
inst = {k: [[] for _ in v] for k, v in meshes.items()}
gold = unreal.load_asset("/Game/world_tree/MI_WorldTree_GoldLeaves")

STEP = 3000.0
count = {k: 0 for k in meshes}
x = -500000.0
while x < 400000.0:
    y = 2250000.0
    while y < 2900000.0:
        px, py = x + rng.uniform(-1200, 1200), y + rng.uniform(-1200, 1200)
        y += STEP
        g = ground(px, py)
        if not g or not off_route(px, py): continue
        z, nz = g
        e = 6326.3 + (z - 129770) / 100
        if e > 4450 or nz < 0.78: continue
        wood = fbm(px / 60000.0, py / 60000.0)            # forest patches ~ a few hundred metres
        grove = fbm(px / 25000.0 + 31.0, py / 25000.0 + 7.0)
        # how many trees stand in this 30 m cell: woods are dense (spruce ~10 m apart), groves of gold, the treeline thin
        picks = []
        if e < 4250 and grove > 0.72:
            picks += [("Ginkgo", rng.uniform(1.4, 2.4))] * rng.randint(2, 5)
        elif e < 4050 and nz > 0.95 and wood < 0.45 and rng.random() < 0.5:
            picks += [("Poplar", rng.uniform(0.6, 1.0))] * rng.randint(1, 3)
        elif e < 4250 and wood > 0.40:
            dens = min(1.0, (wood - 0.40) * 4.0) * (1.0 - max(0.0, e - 4050) / 250)
            picks += [("Spruce", rng.uniform(0.6, 1.2) * (1.0 - max(0.0, e - 4000) / 700))] * int(dens * 9 + rng.random())
        # dead pines stand together in old burnt stands, not one by one across the snow (they read as stray poles)
        if 4050 < e < 4450 and fbm(px / 20000.0 + 50.0, py / 20000.0 + 9.0) > 0.62 and rng.random() < 0.8 * (1.0 - (e - 4050) / 400):
            picks += [("DeadPine", rng.uniform(0.5, 0.9))] * rng.randint(2, 4)
        for k, s in picks:
            vi = rng.randrange(len(meshes[k]))
            tx, ty = px + rng.uniform(-1500, 1500), py + rng.uniform(-1500, 1500)
            tz = z
            if abs(tx - px) + abs(ty - py) > 600:
                g2 = ground(tx, ty)
                if not g2 or g2[1] < 0.75: continue
                tz = g2[0]
            if not off_route(tx, ty): continue
            s *= rng.uniform(0.85, 1.15)
            inst[k][vi].append(unreal.Transform(unreal.Vector(tx, ty, tz - 40 * s), unreal.Rotator(roll=rng.uniform(-2, 2), pitch=rng.uniform(-2, 2), yaw=rng.uniform(0, 360)), unreal.Vector(s, s, s)))
            count[k] += 1
        # reeds: clumps on the flats of the valley floor
        if e < 4250 and nz > 0.96 and fbm(px / 9000.0 + 3.0, py / 9000.0) > 0.62:
            for _ in range(rng.randint(3, 8)):
                vi = rng.randrange(len(meshes["Reeds"]))
                rx, ry = px + rng.uniform(-900, 900), py + rng.uniform(-900, 900)
                s = rng.uniform(0.8, 1.3)
                inst["Reeds"][vi].append(unreal.Transform(unreal.Vector(rx, ry, z - 10), unreal.Rotator(roll=0, pitch=0, yaw=rng.uniform(0, 360)), unreal.Vector(s, s, s)))
                count["Reeds"] += 1
    x += STEP
print("trees", count)

# 2026-11-11 RAM pass: no tree actors in the level any more; one AForestStreamer holds soft references and transforms
# and loads the meshes only while the climber is near the valley (C++: Environment/ForestStreamer).
for a in eas.get_all_level_actors():
    if a.get_actor_label().startswith(("Forest_", "ForestStreamer")):
        eas.destroy_actor(a)
streamer = eas.spawn_actor_from_class(unreal.ForestStreamer, unreal.Vector(-50000, 2575000, -100000))
streamer.set_actor_label("ForestStreamer"); streamer.set_folder_path("Environment/Forest")
sets = []
for k, lists in inst.items():
    if k in ("Poplar", "Reeds"): continue
    for vi, xs in enumerate(lists):
        if not xs: continue
        fs = unreal.ForestSet()
        fs.set_editor_property("mesh", meshes[k][vi])
        if k == "Ginkgo" and gold: fs.set_editor_property("foliage_material", gold)
        fs.set_editor_property("instances", xs)
        sets.append(fs)
streamer.set_editor_property("sets", sets)
print("streamer sets", len(sets), "trees", sum(len(f.get_editor_property("instances")) for f in sets))
print("saved", unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level())
