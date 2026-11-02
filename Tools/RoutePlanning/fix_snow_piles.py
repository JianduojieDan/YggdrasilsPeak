# Runs INSIDE the editor (uepy.py). Re-seats every camp snow pile / snow wall (CampProp actors labelled
# *SnowPile* / *SnowWall*) on the landscape as it is now (it was rebuilt after place_camps.py ran):
#   1. clear of tents: if its footprint overlaps a tent / mess tent (+1 m), push it outwards from the camp centre
#   2. tilt to the plane fitted through the ground under its whole footprint (not just the centre point)
#   3. height: lower it until every bottom-edge vertex of the mesh is 5 cm into the snow, so no skirt floats
#      (the scans are open shells, so the rim vertices are measured, not traced)
# Prints, per pile, the worst floating gap before and after. Set DRY = True to only measure.
import unreal, math
DRY = globals().get("DRY", False)
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
actors = eas.get_all_level_actors()
piles = sorted([a for a in actors if a.actor_has_tag("CampProp") and ("SnowPile" in a.get_actor_label() or "SnowWall" in a.get_actor_label())],
               key=lambda a: a.get_actor_label())
tents = [a for a in actors if a.actor_has_tag("CampProp") and any(k in a.get_actor_label() for k in ("Tent", "Mess"))]
camps = [a for a in actors if a.get_class().get_name() == "RouteLandmark"]

def land(x, y, z):
    hits = unreal.SystemLibrary.line_trace_multi(world, unreal.Vector(x, y, z + 20000), unreal.Vector(x, y, z - 20000),
        unreal.TraceTypeQuery.ECC_VISIBILITY, True, [], unreal.DrawDebugTrace.NONE, True)
    for h in hits or []:
        t = h.to_tuple()
        if t[9] and t[9].get_class().get_name().startswith("Landscape"):
            return t[5].z
    return None

def underside(comp, x, y, zg):
    """Lowest point of the mesh above (x, y), traced upwards from under the ground; None if the mesh is not there."""
    r = comp.line_trace_component(unreal.Vector(x, y, zg - 400), unreal.Vector(x, y, zg + 4000), True, False, False)
    return r[0].z if r and r[1].length() > 0.5 else None

def footprint(a, frac):
    o, e = a.get_actor_bounds(False)
    return [(o.x + e.x * frac * u, o.y + e.y * frac * v) for u in (-1, -0.5, 0, 0.5, 1) for v in (-1, -0.5, 0, 0.5, 1)]

RIMS = {}
def rim(mesh):
    """The pile's bottom edge in its own space: vertices in the lowest 10 % of its height (sampled)."""
    key = mesh.get_path_name()
    if key not in RIMS:
        verts = list(unreal.ProceduralMeshLibrary.get_section_from_static_mesh(mesh, 0, 0)[0])
        zs = [v.z for v in verts]; lo, hi = min(zs), max(zs)
        low = [v for v in verts if v.z < lo + (hi - lo) * 0.10]
        RIMS[key] = low[:: max(1, len(low) // 80)]
    return RIMS[key]

def worst_gap(a, comp):
    """Scanned piles are open shells: every bottom-edge vertex should be at or under the ground. Returns the highest
    gap (cm) of the rim above the ground under it."""
    t = comp.get_world_transform()
    worst = -1e9
    for v in rim(comp.static_mesh):
        w = t.transform_location(v)
        zg = land(w.x, w.y, w.z)
        if zg is not None:
            worst = max(worst, w.z - zg)
    return worst

def overlaps_tent(a):
    o, e = a.get_actor_bounds(False)
    for t in tents:
        to, te = t.get_actor_bounds(False)
        if abs(o.x - to.x) < e.x + te.x + 100 and abs(o.y - to.y) < e.y + te.y + 100:
            return t
    return None

report = []
for a in piles:
    comp = a.static_mesh_component
    prof = comp.get_collision_profile_name()
    comp.set_collision_profile_name("BlockAll")        # the measuring traces need collision on
    before = worst_gap(a, comp)
    moved = 0
    if not DRY:
        # 1. out from under tents
        camp = min(camps, key=lambda c: (c.get_actor_location() - a.get_actor_location()).length())
        cl = camp.get_actor_location()
        for _ in range(12):
            t = overlaps_tent(a)
            if not t:
                break
            l = a.get_actor_location()
            d = unreal.Vector(l.x - cl.x, l.y - cl.y, 0).normal()
            a.set_actor_location(unreal.Vector(l.x + d.x * 200, l.y + d.y * 200, l.z), False, True)
            moved += 2
        # 2. tilt to the ground plane under the footprint (least squares z = ax + by + c)
        pts = [(x, y, land(x, y, a.get_actor_location().z)) for x, y in footprint(a, 0.8)]
        pts = [p for p in pts if p[2] is not None]
        n = len(pts); mx = sum(p[0] for p in pts) / n; my = sum(p[1] for p in pts) / n; mz = sum(p[2] for p in pts) / n
        sxx = sum((p[0] - mx) ** 2 for p in pts); syy = sum((p[1] - my) ** 2 for p in pts); sxy = sum((p[0] - mx) * (p[1] - my) for p in pts)
        sxz = sum((p[0] - mx) * (p[2] - mz) for p in pts); syz = sum((p[1] - my) * (p[2] - mz) for p in pts)
        det = sxx * syy - sxy * sxy
        ga = (sxz * syy - syz * sxy) / det if det else 0.0
        gb = (syz * sxx - sxz * sxy) / det if det else 0.0
        up = unreal.Vector(-ga, -gb, 1.0).normal()
        if up.z < math.cos(math.radians(28)):          # never lean more than 28 degrees
            k = math.tan(math.radians(28)) / max(math.hypot(up.x, up.y) / up.z, 1e-6)
            up = unreal.Vector(up.x * k, up.y * k, up.z).normal()
        yaw = a.get_actor_rotation().yaw
        fwd = unreal.Vector(math.cos(math.radians(yaw)), math.sin(math.radians(yaw)), 0)
        a.set_actor_rotation(unreal.MathLibrary.make_rot_from_zx(up, fwd), False)
        # 3. down until nothing floats (5 cm into the snow); twice, as the footprint shifts a little
        for _ in range(2):
            g = worst_gap(a, comp)
            if g > -1e8:
                l = a.get_actor_location()
                a.set_actor_location(unreal.Vector(l.x, l.y, l.z - (g + 5.0)), False, True)
    after = worst_gap(a, comp)
    comp.set_collision_profile_name(prof)
    o, e = a.get_actor_bounds(False)
    zc = land(o.x, o.y, o.z) or o.z
    showing = (o.z + e.z) - zc                         # how much stands above the ground at its centre
    report.append((a.get_actor_label(), round(before), round(after), moved, round(showing)))
for r in report:
    print("pile %-28s float before %6s cm  after %6s cm  moved out %2s m  stands %5s cm" % r)
print("piles", len(report), "floating after (>2 cm):", sum(1 for r in report if r[2] > 2))
