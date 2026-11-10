# Runs inside the editor. Rebuilds the camps around the RouteLandmark actors, laid out as real Everest camps are: at Base Camp a
# semicircle of sleeping tents on the rise above the dining tent, kitchen and stores below, the puja altar and its pole at the centre;
# Camp 2 strung along a line; Camp 3 a single row on the steep face; the South Col a wide wind-beaten cluster. Tents stand on levelled
# stones, doors look downhill, gear gathers at the doors. Muted fabric from Tools/Materials/make_camp_canvas.py.
# Few actors on purpose, all reused meshes: small gear casts no shadow and has no collision. Oxygen pickups are left where they are.
import math, random
import unreal

ROOT = "F:/UE_project/YggdrasilsPeak"
exec(open(f"{ROOT}/Tools/RoutePlanning/assets.py").read())  # defines A

eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
A["stone"] = "/Game/Fab/Stone_-_photogrammetry/stone_photogrammetry/StaticMeshes/stone_photogrammetry"
meshes = {k: unreal.load_asset(A[k]) for k in ("tent_modern", "mess", "fire", "o2", "axe", "rope", "stone")}
bounds = {k: m.get_bounding_box() for k, m in meshes.items()}
FABRIC = {n: unreal.load_asset(f"/Game/Environment/Camps/MI_Camp{n}") for n in ("Rust", "Ochre", "Slate", "Canvas")}
FIRE_UP = unreal.MathLibrary.make_rot_from_xz(unreal.Vector(0.8212, 0.5537, 0.1379), unreal.Vector(0.5106, -0.6052, -0.6108))
landmarks = sorted([a for a in eas.get_all_level_actors() if a.get_class().get_name() == "RouteLandmark"], key=lambda a: a.get_actor_label())
ignore = list(landmarks)

TENT_SCALE = 2.0
ROW_GAP = 760.0       # along the contour, centre to centre
ROW_DEPTH = 800.0     # up the slope, between terraces
FIRST_ROW = 950.0     # from the pole up to the first terrace

# layout: arc (Base Camp), rows (terraces), line (one long string). stones: per tent, on its windward edge. altar: stone puja altar at the pole.
KITS = {
    0: dict(tents=3, layout="rows", row=3, fire=True, o2=0, stones=2, altar=False, lead="Ochre", second="Rust"),      # Dingboche
    1: dict(tents=3, layout="rows", row=3, fire=True, o2=0, stones=3, altar=False, lead="Rust", second="Slate"),      # Gorak Shep
    2: dict(tents=9, layout="arc", mess=True, stores=True, o2=4, axe=True, stones=3, altar=True, lead="Ochre", second="Rust"),  # Everest Base Camp
    3: dict(tents=2, layout="rows", row=2, o2=2, axe=True, stones=0, altar=False, lead="Rust", second="Ochre"),       # Khumbu Icefall
    4: dict(tents=4, layout="rows", row=2, o2=3, axe=True, stones=0, altar=False, lead="Ochre", second="Slate"),      # Camp 1: snow, nothing else
    5: dict(tents=6, layout="line", mess=True, o2=4, axe=True, stones=2, altar=True, lead="Ochre", second="Rust"),    # Camp 2: along a gully
    6: dict(tents=3, layout="line", o2=4, axe=True, stones=0, altar=False, lead="Rust", second="Rust"),               # Camp 3: cut into the face
    7: dict(tents=6, layout="scatter", o2=7, axe=True, stones=4, altar=False, lead="Ochre", second="Slate"),          # South Col: rocks hold the tents down
    8: None,                                                                                                          # Summit: only the pole
}

import json
ROUTE = [p for leg in json.load(open(f"{ROOT}/Tools/RoutePlanning/route.json")) for p in leg["pts"]]
for rope in [a for a in eas.get_all_level_actors() if a.get_class().get_name() == "FixedRope"]:
    ROUTE += [(p.x, p.y) for p in rope.get_editor_property("rope_points")]   # the ropes run their own line

def clear_of_route(x, y, keep=650.0):
    """The climbing line stays open: a tent too close is pushed straight away from the nearest route point."""
    near = min(ROUTE, key=lambda p: (p[0] - x) ** 2 + (p[1] - y) ** 2)
    d = math.hypot(x - near[0], y - near[1])
    if d >= keep:
        return x, y
    if d < 1.0:
        return near[0] + keep, near[1]
    return near[0] + (x - near[0]) / d * keep, near[1] + (y - near[1]) / d * keep

def RV(rot, v):
    return unreal.MathLibrary.greater_greater_vector_rotator(v, rot)

def ground(x, y, z_ref):
    hits = unreal.SystemLibrary.line_trace_multi(world, unreal.Vector(x, y, z_ref + 30000), unreal.Vector(x, y, z_ref - 30000),
        unreal.TraceTypeQuery.ECC_VISIBILITY, True, ignore, unreal.DrawDebugTrace.NONE, True)
    for h in hits or []:
        t = h.to_tuple()
        if t[9] and t[9].get_class().get_name().startswith("Landscape"):
            return t[5], t[7]
    return None, None

def downhill(base):
    """Unit xy vector down the slope around the camp, from four samples 15 m out."""
    s = {}
    for k, (dx, dy) in {"e": (1500, 0), "w": (-1500, 0), "n": (0, 1500), "s": (0, -1500)}.items():
        p, _ = ground(base.x + dx, base.y + dy, base.z)
        s[k] = p.z if p else base.z
    x, y = s["w"] - s["e"], s["s"] - s["n"]
    n = math.hypot(x, y)
    return (x / n, y / n) if n > 1 else (1.0, 0.0)

def place(camp, key, x, y, yaw, scale=1.0, big=False, align=1.0, sink=10.0, pitch=0.0, label=None, pre=None, fabric=None):
    base = camp.get_actor_location()
    p, n = ground(x, y, base.z)
    if p is None:
        print("  no ground for", key); return None
    up = unreal.Vector(n.x * align, n.y * align, 1.0 - align + n.z * align).normal()
    rot = unreal.MathLibrary.make_rot_from_zx(up, unreal.Vector(math.cos(math.radians(yaw)), math.sin(math.radians(yaw)), 0.0))
    rot = unreal.MathLibrary.compose_rotators(unreal.Rotator(roll=0.0, pitch=pitch, yaw=0.0), rot)
    if pre is not None:
        rot = unreal.MathLibrary.compose_rotators(pre, rot)
    b = bounds[key]
    centre = RV(rot, unreal.Vector((b.min.x + b.max.x) * 0.5 * scale, (b.min.y + b.max.y) * 0.5 * scale, (b.min.z + b.max.z) * 0.5 * scale))
    low = min(RV(rot, unreal.Vector(cx * scale, cy * scale, cz * scale)).z
              for cx in (b.min.x, b.max.x) for cy in (b.min.y, b.max.y) for cz in (b.min.z, b.max.z))
    a = eas.spawn_actor_from_object(meshes[key], unreal.Vector(p.x - centre.x, p.y - centre.y, p.z - low - sink), rot)
    a.set_actor_scale3d(unreal.Vector(scale, scale, scale))
    c = a.static_mesh_component
    c.set_collision_profile_name("BlockAll" if big else "NoCollision")
    c.set_editor_property("mobility", unreal.ComponentMobility.STATIC)
    c.set_editor_property("cast_shadow", big)
    if fabric:
        for slot in range(c.get_num_materials()):
            c.set_material(slot, FABRIC[fabric])
    name = camp.get_editor_property("landmark_name").split(" (")[0]
    a.set_actor_label("%s_%s" % (name.replace(" ", ""), label or key))
    a.set_folder_path("Route/CampProps/" + name)
    a.tags = [unreal.Name("CampProp")]
    ignore.append(a)
    return a

STONE_SIZE = (bounds["stone"].max.x - bounds["stone"].min.x)

def stone(camp, x, y, rng, size, label, sink=None):
    a = place(camp, "stone", x, y, rng.uniform(0, 360), scale=size / STONE_SIZE, align=0.3, sink=size * 0.3 if sink is None else sink, label=label)
    if a:
        a.static_mesh_component.set_editor_property("cast_shadow", size > 60)

for idx, camp in enumerate(landmarks):
    if globals().get("ONLY") and idx not in ONLY:
        continue
    name = camp.get_editor_property("landmark_name").split(" (")[0]
    folder = "Route/CampProps/" + name
    old = [a for a in eas.get_all_level_actors() if isinstance(a, unreal.StaticMeshActor) and a.actor_has_tag("CampProp") and str(a.get_folder_path()) == folder]
    for a in old:
        eas.destroy_actor(a)
    camp.set_editor_property("num_tents", 0)
    kit = KITS[idx]
    print("camp", idx, name, "removed", len(old))
    if not kit:
        continue
    rng = random.Random(2000 + idx)
    base = camp.get_actor_location()
    dx, dy = downhill(base)
    door = math.degrees(math.atan2(dy, dx))           # +X of the dome is its door
    along = (-dy, dx)
    def at(a, u):                                     # a along the contour, u metres-in-cm up the slope from the pole
        return base.x + along[0] * a - dx * u, base.y + along[1] * a - dy * u
    lead, second = kit["lead"], kit["second"]
    n = kit["tents"]
    spots = []
    for i in range(n):
        if kit["layout"] == "arc":          # a semicircle on the rise, every door towards the centre
            ang = math.radians(-70 + 140.0 * i / max(n - 1, 1))
            a, u = math.sin(ang) * 1900.0, math.cos(ang) * 1900.0
            turn = math.degrees(math.atan2(-a * along[1] + u * dy, -a * along[0] + u * dx))
        elif kit["layout"] == "line":
            a, u = (i - (n - 1) / 2.0) * ROW_GAP, FIRST_ROW + (i % 2) * 130.0
            turn = door
        elif kit["layout"] == "scatter":    # no order: wherever a tent would stand
            a, u = rng.uniform(-1500, 1500), FIRST_ROW + rng.uniform(0, 1500)
            for _ in range(20):
                if all(math.hypot(a - p, u - q) > 700 for p, q, _t in spots):
                    break
                a, u = rng.uniform(-1500, 1500), FIRST_ROW + rng.uniform(0, 1500)
            turn = door + rng.uniform(-40, 40)
        else:
            row, col = i // kit["row"], i % kit["row"]
            count = min(kit["row"], n - row * kit["row"])
            a = (col - (count - 1) / 2.0) * ROW_GAP + (ROW_GAP * 0.5 if row % 2 else 0.0) + rng.uniform(-50, 50)
            u = FIRST_ROW + row * ROW_DEPTH + rng.uniform(-60, 60)
            turn = door + rng.uniform(-9, 9)
        spots.append((a, u, turn))
    doors = []
    for i, (a, u, turn) in enumerate(spots):
        x, y = clear_of_route(*at(a, u))
        place(camp, "tent_modern", x, y, turn, scale=TENT_SCALE, big=True, align=0.8, label="Tent%d" % i, fabric=second if i % 3 == 2 else lead)
        doors.append((a, u))
        for k in range(kit["stones"]):      # the levelled platform's edge, or the rocks that hold the fly down
            sa = rng.uniform(100, 260)
            sx, sy = x + math.cos(math.radians(turn + sa)) * 330.0, y + math.sin(math.radians(turn + sa)) * 330.0
            stone(camp, sx, sy, rng, rng.uniform(35, 70), "Stone%d_%d" % (i, k))
    if kit.get("altar"):                    # the puja altar: dry stones heaped round the foot of the flag pole
        for k in range(7):
            ang = k * 2 * math.pi / 7
            stone(camp, base.x + math.cos(ang) * 95, base.y + math.sin(ang) * 95, rng, rng.uniform(80, 115), "Altar%d" % k)
        for k in range(3):
            ang = k * 2 * math.pi / 3 + 0.5
            stone(camp, base.x + math.cos(ang) * 35, base.y + math.sin(ang) * 35, rng, 70, "AltarTop%d" % k, sink=-55)
    if kit.get("mess"):                     # the dining tent below the sleeping tents; its long side faces them
        x, y = clear_of_route(*at(1500, -900 if kit["layout"] == "arc" else FIRST_ROW + 200), keep=1300.0)
        place(camp, "mess", x, y, door + 90, scale=0.85, big=True, align=0.4, sink=20, label="DiningTent", fabric="Canvas")
    if kit.get("stores"):                   # kitchen and stores, off to the other side
        x, y = clear_of_route(*at(-1700, -700), keep=1300.0)
        place(camp, "mess", x, y, door + 75, scale=0.6, big=True, align=0.4, sink=20, label="Stores", fabric="Canvas")
    if kit.get("fire"):
        x, y = at(rng.uniform(-150, 150), -420)
        place(camp, "fire", x, y, rng.uniform(0, 360), scale=0.12, align=1.0, sink=8, label="FireRing", pre=FIRE_UP)
    # the oxygen stands in a neat stack by the lead tent's door, as climbers keep it
    a0, u0 = doors[0]
    for i in range(kit["o2"]):
        x, y = at(a0 - 230 + (i % 3) * 34, u0 - 330 + (i // 3) * 34)
        place(camp, "o2", x, y, rng.uniform(0, 360), scale=1.3, align=0.6, pitch=90.0, sink=4, label="O2_%d" % i)
    a1, u1 = doors[-1]
    if kit.get("axe"):
        x, y = at(a1 + 240, u1 - 300)
        place(camp, "axe", x, y, door, scale=1.5, align=0.0, pitch=-65.0, sink=18, label="IceAxe")
    x, y = at(a0 + 210, u0 - 340)
    place(camp, "rope", x, y, rng.uniform(0, 360), scale=1.8, sink=3, label="Rope")

print("placed", len([a for a in eas.get_all_level_actors() if isinstance(a, unreal.StaticMeshActor) and a.actor_has_tag("CampProp")]))
