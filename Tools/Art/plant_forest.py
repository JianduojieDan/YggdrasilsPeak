# Runs INSIDE the editor (Tools/RoutePlanning/uepy). Plants the trees (Megaplant Library: Nanite skeletal foliage, needs
# r.Nanite.Foliage=True) into the AForestStreamer (C++ Environment/ForestStreamer streams them in 3 km chunks).
#
# 2026-11-13 (user: "more variant - height, direction, density of one region compared to another; it is a fantasy world,
# place them literally everywhere near or on the climbing road, not only below 4,400 m"):
#   - where: the valley SW of Dingboche (woods, as before) AND a corridor 6-420 m either side of the whole route, any
#     altitude; never on the rope line (6 m), at camps (45 m), in the icefall, on slopes over ~38 deg, or into rocks/ice;
#   - regions (noise fields, km scale) decide: forest-ness (bare stretches / scattered lone trees / groves / woods),
#     grove patches (~350 m), regional tree height (x0.55-1.5), regional wind -> lean direction and amount (2-14 deg);
#   - species by altitude, one species per grove: spruce woods low; burnt dead-pine stands from the treeline up;
#     golden ginkgo groves (gold like the World Tree) high on the mountain; sparser the higher it goes;
#   - seated on the LOWEST ground under the trunk (5 traces) and sunk by its root depth, so none float.
import json, math, random
import unreal
exec(open('F:/UE_project/YggdrasilsPeak/Tools/Placement/ground_lib.py').read())

ROOT = "F:/UE_project/YggdrasilsPeak"
rng = random.Random(1113)
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()

def elev(z): return 6326.3 + (z - 129770.0) / 100.0

def H(x, y): return (math.sin(x * 12.9898 + y * 78.233) * 43758.5453) % 1.0
def vn(x, y):
    i, j = math.floor(x), math.floor(y); fx, fy = x - i, y - j
    fx, fy = fx * fx * (3 - 2 * fx), fy * fy * (3 - 2 * fy)
    a, b, c, d = H(i, j), H(i + 1, j), H(i, j + 1), H(i + 1, j + 1)
    return a + (b - a) * fx + (c - a) * fy + (a - b - c + d) * fx * fy
def fbm(x, y): return 0.5 * vn(x, y) + 0.3 * vn(x * 2.1 + 5.2, y * 2.1 + 1.3) + 0.2 * vn(x * 4.3 + 9.1, y * 4.3 + 7.7)
def sstep(a, b, v):
    t = max(0.0, min(1.0, (v - a) / (b - a))); return t * t * (3 - 2 * t)

# ---------------------------------------------------------------- keep-outs (hashed for speed)
class Near:
    def __init__(self, pts, cell):
        self.c = cell; self.g = {}
        for p in pts: self.g.setdefault((int(p[0] // cell), int(p[1] // cell)), []).append(p)
    def within(self, x, y, r):
        cx, cy = int(x // self.c), int(y // self.c); n = int(math.ceil(r / self.c))
        for i in range(cx - n, cx + n + 1):
            for j in range(cy - n, cy + n + 1):
                for p in self.g.get((i, j), ()):
                    if (p[0] - x) ** 2 + (p[1] - y) ** 2 < r * r: return True
        return False
route = []
for leg in json.load(open(f"{ROOT}/Tools/RoutePlanning/route.json")):
    pts = leg["pts"]
    for a, b in zip(pts, pts[1:]):
        n = max(1, int(math.dist(a[:2], b[:2]) / 1500))
        route += [(a[0] + (b[0] - a[0]) * t / n, a[1] + (b[1] - a[1]) * t / n) for t in range(n)]
corridor = Near(route, 20000.0)
rope_pts = []
for a in acts:
    if a.get_class().get_name() == 'FixedRope':
        rope_pts += [(p.x, p.y) for p in list(a.get_editor_property('rope_points'))[::4]]
rope = Near(rope_pts, 2000.0)
camps = [(a.get_actor_location().x, a.get_actor_location().y) for a in acts if a.get_class().get_name() == 'RouteLandmark']
ice_boxes = []
for a in acts:
    if str(a.get_folder_path()).startswith('Environment/Icefall'):
        for c in a.get_components_by_class(unreal.PrimitiveComponent):
            o, e, _ = unreal.SystemLibrary.get_component_bounds(c)
            ice_boxes.append((o.x - e.x - 1500, o.y - e.y - 1500, o.x + e.x + 1500, o.y + e.y + 1500))
def keep_out(x, y):
    if rope.within(x, y, 600): return True
    if any((x - cx) ** 2 + (y - cy) ** 2 < 4500 ** 2 for cx, cy in camps): return True
    return any(b[0] < x < b[2] and b[1] < y < b[3] for b in ice_boxes)
VALLEY = (-500000.0, 2250000.0, 400000.0, 2900000.0)
def in_valley(x, y): return VALLEY[0] < x < VALLEY[2] and VALLEY[1] < y < VALLEY[3]
def on_ground(x, y):
    """True if nothing (rock, ice, prop) stands here: the first thing a trace from above hits is the ground."""
    h = unreal.SystemLibrary.line_trace_single(world, unreal.Vector(x, y, 900000), unreal.Vector(x, y, -500000),
        unreal.TraceTypeQuery.ECC_VISIBILITY, True, [], unreal.DrawDebugTrace.NONE, True)
    if not h: return False
    t = h.to_tuple()
    return bool(t[9]) and t[9].get_actor_label().startswith(("DetailTile", "Landscape"))

# ---------------------------------------------------------------- species
L = "/Game/Megaplant_Library/"
SPECIES = {
    "Spruce": [f"{L}Tree_Norway_Spruce/Tree_Norway_Spruce_01/Tree_Norway_Spruce_01_{v}" for v in "CD"],
    "Ginkgo": [f"{L}Tree_Ginkgo/Tree_Ginkgo_01/Tree_Ginkgo_01_{v}" for v in "CD"],
    "DeadPine": [f"{L}Tree_Dead_Pine/Tree_Dead_Pine_01/Tree_Dead_Pine_{v}" for v in "ABCDE"],
}
BASE = {"Spruce": (0.6, 1.2), "Ginkgo": (1.3, 2.3), "DeadPine": (0.5, 0.95)}
meshes = {k: [m for m in (unreal.load_asset(p) for p in v) if m] for k, v in SPECIES.items()}
inst = {k: [[] for _ in v] for k, v in meshes.items()}
gold = unreal.load_asset("/Game/world_tree/MI_WorldTree_GoldLeaves")

def species_for(e, gx, gy):
    """One species per grove (~350 m cell), weighted by altitude."""
    if e < 4300: w = (("Spruce", 0.62), ("Ginkgo", 0.14), ("DeadPine", 0.24 if e > 4050 else 0.0))
    elif e < 5000: w = (("DeadPine", 0.55), ("Spruce", 0.30), ("Ginkgo", 0.15))
    elif e < 6500: w = (("DeadPine", 0.55), ("Ginkgo", 0.35), ("Spruce", 0.10))
    else: w = (("Ginkgo", 0.5), ("DeadPine", 0.5))
    r = H(gx * 1.7 + 3.1, gy * 2.3 + 7.9) * sum(v for _, v in w)
    for k, v in w:
        r -= v
        if r <= 0: return k
    return w[0][0]

# ---------------------------------------------------------------- plant
STEP = 3000.0
count = {k: 0 for k in meshes}
rejected = {'keepout': 0, 'slope': 0, 'blocked': 0, 'air': 0}
xs = [p[0] for p in route]; ys = [p[1] for p in route]
box = (min(min(xs) - 45000, VALLEY[0]), min(min(ys) - 45000, VALLEY[1]), max(max(xs) + 45000, VALLEY[2]), max(max(ys) + 45000, VALLEY[3]))
x = box[0]
while x < box[2]:
    y = box[1]
    while y < box[3]:
        px, py = x + rng.uniform(-1200, 1200), y + rng.uniform(-1200, 1200)
        y += STEP
        valley = in_valley(px, py)
        if not valley and not corridor.within(px, py, 42000):
            continue
        g = ground(px, py)
        if not g: continue
        z, nz, _ = g
        e = elev(z)
        if valley and e > 4450 and not corridor.within(px, py, 42000):
            continue
        m = px / 100.0, py / 100.0                                     # metres
        region = fbm(m[0] / 2500.0, m[1] / 2500.0)                     # forest-ness, km scale
        grove = fbm(m[0] / 350.0 + 31.0, m[1] / 350.0 + 7.0)           # grove patches
        tall = 0.55 + 0.95 * fbm(m[0] / 1800.0 + 11.0, m[1] / 1800.0 + 3.0)
        wind = fbm(m[0] / 4000.0 + 61.0, m[1] / 4000.0 + 17.0) * 6.2831853
        lean_amt = 2.0 + 12.0 * sstep(0.35, 0.8, fbm(m[0] / 2000.0 + 5.0, m[1] / 2000.0 + 41.0))
        alt = 1.0 if e < 4400 else (0.8 if e < 5500 else (0.55 if e < 7000 else 0.35))
        dens = sstep(0.40, 0.72, region) * (0.30 + 0.70 * sstep(0.45, 0.70, grove)) * alt
        if valley and e < 4250:
            dens = max(dens, 0.55 * sstep(0.40, 0.65, grove))         # the valley keeps real woods
        n = int(dens * 7 + rng.random())
        if n == 0 and rng.random() < 0.03 * alt:                       # lone sentinels on bare slopes
            n = 1
        if n == 0: continue
        if nz < 0.78:
            rejected['slope'] += 1; continue
        k = species_for(e, int(m[0] // 350), int(m[1] // 350))
        edge = 0.6 + 0.4 * sstep(0.45, 0.75, grove)                  # smaller at a grove's edge
        for _ in range(n):
            tx, ty = px + rng.uniform(-1500, 1500), py + rng.uniform(-1500, 1500)
            if keep_out(tx, ty):
                rejected['keepout'] += 1; continue
            lo, hi = BASE[k]
            s = rng.uniform(lo, hi) * tall * edge * rng.uniform(0.75, 1.25)
            if k == "Spruce" and e > 4400: s *= 0.6 + 0.3 * rng.random()            # dwarfed spruce up high
            rr = 45.0 * s
            gs = [ground(tx + dx, ty + dy) for dx, dy in ((0, 0), (rr, 0), (-rr, 0), (0, rr), (0, -rr))]
            if not all(gs):
                rejected['air'] += 1; continue
            zs = [q[0] for q in gs]
            if max(zs) - min(zs) > 90.0 * s or gs[0][1] < 0.76:
                rejected['slope'] += 1; continue
            if not on_ground(tx, ty):
                rejected['blocked'] += 1; continue
            lean = lean_amt * rng.uniform(0.5, 1.2) * (1.3 if k == "DeadPine" else 1.0)
            pitch = lean * math.cos(wind) + rng.uniform(-2, 2)
            roll = lean * math.sin(wind) + rng.uniform(-2, 2)
            vi = rng.randrange(len(meshes[k]))
            inst[k][vi].append(unreal.Transform(unreal.Vector(tx, ty, min(zs) - 30.0 * s),
                unreal.Rotator(roll=roll, pitch=pitch, yaw=rng.uniform(0, 360)), unreal.Vector(s, s, s)))
            count[k] += 1
    x += STEP
print("trees", count, "total", sum(count.values()), "rejected", rejected)

streamer = [a for a in acts if a.get_class().get_name() == 'ForestStreamer']
streamer = streamer[0] if streamer else eas.spawn_actor_from_class(unreal.ForestStreamer, unreal.Vector(-50000, 2575000, -100000))
streamer.set_actor_label("ForestStreamer"); streamer.set_folder_path("Environment/Forest")
sets = []
for k, lists in inst.items():
    for vi, xs_ in enumerate(lists):
        if not xs_: continue
        fs = unreal.ForestSet()
        fs.set_editor_property("mesh", meshes[k][vi])
        if k == "Ginkgo" and gold: fs.set_editor_property("foliage_material", gold)
        fs.set_editor_property("instances", xs_)
        sets.append(fs)
streamer.set_editor_property("sets", sets)
hi_trees = sum(1 for k in inst for l in inst[k] for t in l if elev(t.translation.z) > 4400)
print("streamer sets", len(sets), "trees", sum(len(f.get_editor_property("instances")) for f in sets), "above 4,400 m:", hi_trees)
print("saved", unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level())
