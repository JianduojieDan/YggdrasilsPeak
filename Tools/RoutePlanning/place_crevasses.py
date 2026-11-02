# Runs INSIDE the editor (uepy.py). Crevasse crossings with ladders (ACrevasseCrossing) on the Icefall and the
# Western Cwm. Order: place_ropes.py -> place_crevasses.py (lifts the fixed rope over each crossing so it becomes the
# hand line along the ladder) -> place_slice_content.py (the serac scatter keeps clear of actors tagged
# ScatterProtectBox). Re-running replaces what it placed (tag Crevasse) and re-lifts the ropes from the landscape.
#
# Each crossing (local X = walking direction, origin = middle of the gap at the ladder):
#   near lip: snow ramp up from the trail to a crest, then a sheer wall into the slot; far lip mirrored.
#   Lip X/Z scale is set per side so its ramp foot meets the ground (the ramp angle stays ~25 deg either way).
#   The ladder rests on both crests (pitched if they differ); snow pickets anchor it.
import unreal, json, math
ROOT = "F:/UE_project/YggdrasilsPeak"
R = json.load(open(f"{ROOT}/Tools/RoutePlanning/route.json"))
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
if not unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world().get_name() == "base_map":
    les.load_level("/Game/LevelPrototyping/base_map")
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()

# (leg, fraction, gap m, lip variant) - leg 2 EBC -> Icefall, 3 Icefall -> C1, 4 C1 -> C2 (Western Cwm)
CROSSINGS = [(2, 0.45, 3.0, 1), (2, 0.66, 3.6, 2), (2, 0.84, 2.6, 1),
             (3, 0.22, 3.2, 2), (3, 0.58, 4.0, 1), (3, 0.76, 2.8, 2),
             (4, 0.30, 4.4, 1)]
VISIBLE_M = 4.5          # crest height above the higher gap edge's ground
RAMP_DEG = 30.0          # ramp angle (walkable while clipped to the fixed rope, which runs over every crossing)
LIP_H, LIP_FOOT, LIP_L = 630.0, 60.0, 1500.0   # mesh: crest height, foot depth, ramp length (cm)
LIP_W = 3600.0
LIP_LANDING = 320.0      # flat landing at the crest (mesh cm); never scaled shorter than the ladder overhang
LADDER_HALF = 400.0
SLOT_LEN = 2800.0        # dark floor / fall zone length: where both walls are still tall (the lip ends taper)
KEEP_CLEAR_M = 40.0      # from checkpoints / clues / camps

CC = unreal.load_class(None, "/Script/YggdrasilsPeak.CrevasseCrossing")
ROPE = unreal.load_class(None, "/Script/YggdrasilsPeak.FixedRope")
ladder = unreal.load_asset("/Game/Props/Blender/SM_IcefallLadder_Span3")
SMS = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
if SMS.get_simple_collision_count(ladder) == 0:          # one box over the whole span: something to stand on
    SMS.add_simple_collisions(ladder, unreal.ScriptCollisionShapeType.BOX)
    unreal.EditorAssetLibrary.save_loaded_asset(ladder, False)
picket = unreal.load_asset("/Game/Props/Blender/SM_SnowPicket")
cube = unreal.load_asset("/Engine/BasicShapes/Cube")
abyss_mat = unreal.load_asset("/Game/Environment/Glacier/Crevasse/M_CrevasseAbyss")
lips = {i: unreal.load_asset(f"/Game/Environment/Glacier/Crevasse/SM_CrevasseLip_{i:02d}") for i in (1, 2)}

for a in eas.get_all_level_actors():
    if a.actor_has_tag("Crevasse"):
        a.destroy_actor()

# a multi trace stops at the first blocking hit: ignore our lips and the seracs to reach the landscape
IGNORE = [a for a in eas.get_all_level_actors() if a.get_class().get_name() == "ConstrainedScatter"]
def landscape_z(x, y):
    hits = unreal.SystemLibrary.line_trace_multi(world, unreal.Vector(x, y, 600000), unreal.Vector(x, y, -200000),
        unreal.TraceTypeQuery.ECC_VISIBILITY, True, IGNORE, unreal.DrawDebugTrace.NONE, True)
    for h in hits or []:
        t = h.to_tuple()
        if t[9] and t[9].get_class().get_name().startswith("Landscape"):
            return t[4].z
    return None

def top_z(x, y, comps):
    """Highest surface among the given components (and the landscape) at x, y."""
    best = landscape_z(x, y)
    for c in comps:
        # returns (location, normal, bone, hit result); a miss leaves location and normal zero
        r = c.line_trace_component(unreal.Vector(x, y, 600000), unreal.Vector(x, y, -200000), True, False, False)
        if r and r[1].length() > 0.5:
            z = r[0].z
            best = z if best is None else max(best, z)
    return best

# things the crossings must keep away from
avoid = [a.get_actor_location() for a in eas.get_all_level_actors()
         if a.get_class().get_name() in ("TrailCheckpoint", "StoryClue", "RouteLandmark")]

# ropes back to the landscape first (re-runs must not stack lifts)
ropes = {a.get_actor_label(): a for a in eas.get_all_level_actors() if a.get_class() == ROPE}
for r in ropes.values():
    pts = list(r.get_editor_property("rope_points"))
    out = []
    for p in pts:
        z = landscape_z(p.x, p.y)
        out.append(unreal.Vector(p.x, p.y, (z + 30.0) if z is not None else p.z))
    r.set_editor_property("rope_points", out)

report = []
for n, (leg, f, gap_m, variant) in enumerate(CROSSINGS):
    pts = R[leg]["pts"]
    i = min(int(f * (len(pts) - 1)), len(pts) - 2)
    a, b = pts[max(i - 2, 0)], pts[min(i + 3, len(pts) - 1)]
    fwd = unreal.Vector(b[0] - a[0], b[1] - a[1], 0).normal()
    side = unreal.Vector(-fwd.y, fwd.x, 0)
    yaw = math.degrees(math.atan2(fwd.y, fwd.x))
    gx, gy = pts[i][0], pts[i][1]
    near_d = min((math.hypot(gx - v.x, gy - v.y) for v in avoid), default=1e9) / 100.0
    if near_d < KEEP_CLEAR_M:
        print(f"crossing {n}: skipped, {near_d:.0f} m from a checkpoint / clue / camp"); continue
    gap = gap_m * 100.0
    at = lambda ax, ay=0.0: (gx + fwd.x * ax + side.x * ay, gy + fwd.y * ax + side.y * ay)
    g_edge_n = landscape_z(*at(-gap / 2)); g_edge_f = landscape_z(*at(gap / 2))
    ZL = max(g_edge_n, g_edge_f) + VISIBLE_M * 100.0
    # lip scale per side: X so the ramp (RAMP_DEG) meets the terrain, Z so the foot is just buried
    def lip_scale(sign):
        g_edge = g_edge_n if sign < 0 else g_edge_f
        g_out = landscape_z(*at(sign * (gap / 2 + 1000.0)))
        slope_out = (g_out - g_edge) / 1000.0              # terrain rise per cm walking away from the gap
        ramp_len = (ZL - g_edge) / max(math.tan(math.radians(RAMP_DEG)) + slope_out, 0.12)
        sx = min(max(ramp_len / LIP_L, (LADDER_HALF - gap / 2 + 60.0) / LIP_LANDING, 0.3), 2.6)
        gf = landscape_z(*at(sign * (gap / 2 + LIP_L * sx)))
        sz = min(max((ZL - gf + 40.0) / (LIP_H + LIP_FOOT), 0.35), 2.6)
        return sx, sz
    (sxn, s_n), (sxf, s_f) = lip_scale(-1), lip_scale(1)

    cc = eas.spawn_actor_from_class(CC, unreal.Vector(gx, gy, ZL), unreal.Rotator(roll=0, pitch=0, yaw=yaw))
    cc.set_actor_label(f"Crevasse_{n}_leg{leg}")
    cc.set_folder_path("Route/Crevasses")
    cc.tags = ["Crevasse", "ScatterProtectBox"]
    IGNORE.append(cc)
    ln, lf = cc.get_editor_property("lip_near"), cc.get_editor_property("lip_far")
    for c in (ln, lf, cc.get_editor_property("ladder")):
        c.set_collision_profile_name("BlockAll")          # static scenery (C++ default too since 2026-11-02)
    ln.set_static_mesh(lips[variant]); lf.set_static_mesh(lips[3 - variant])
    ln.set_relative_location(unreal.Vector(-gap / 2 - 60.0, 0, -LIP_H * s_n), False, True)
    ln.set_relative_scale3d(unreal.Vector(sxn, 1, s_n))
    lf.set_relative_location(unreal.Vector(gap / 2 + 60.0, 0, -LIP_H * s_f), False, True)
    lf.set_relative_rotation(unreal.Rotator(roll=0, pitch=0, yaw=180), False, True)
    lf.set_relative_scale3d(unreal.Vector(sxf, 1, s_f))

    # ladder on both crests, 50 cm back from each edge
    # (the crest edge wanders ~0.7 m along the crevasse: take the highest of a few samples behind it)
    zn = max(top_z(*at(-gap / 2 - d, y), [ln]) for d in (80.0, 140.0, 200.0) for y in (-30.0, 30.0))
    zf = max(top_z(*at(gap / 2 + d, y), [lf]) for d in (80.0, 140.0, 200.0) for y in (-30.0, 30.0))
    pitch = math.degrees(math.atan2(zf - zn, gap + 100.0))
    zl = (zn + zf) / 2 - ZL
    bb = ladder.get_bounding_box(); lc = (bb.min + bb.max) * 0.5
    lad = cc.get_editor_property("ladder")
    lad.set_static_mesh(ladder)
    lad.set_relative_rotation(unreal.Rotator(roll=0, pitch=pitch, yaw=0), False, True)
    lad.set_relative_location(unreal.Vector(-lc.x, -lc.y, zl + 2.0), False, True)
    lz = cc.get_editor_property("ladder_zone")
    lz.set_box_extent(unreal.Vector(gap / 2 + 150.0, 70.0, 110.0), True)
    lz.set_relative_location(unreal.Vector(0, 0, zl + 100.0), False, True)
    lz.set_relative_rotation(unreal.Rotator(roll=0, pitch=pitch, yaw=0), False, True)

    # slot: dark floor hugging the ground along the crevasse (tilted to the cross-slope, 20 cm above the highest
    # bump), the fall zone from there up to 1.5 m under the ladder
    ys = [-SLOT_LEN / 2 + SLOT_LEN * k / 12 for k in range(13)]
    gs = [max(landscape_z(*at(x, y)) for x in (-gap / 2, 0, gap / 2)) - ZL for y in ys]
    slope = (gs[-1] - gs[0]) / SLOT_LEN
    lift = max(g - (gs[0] + slope * (y - ys[0])) for g, y in zip(gs, ys)) + 20.0
    mid = (gs[0] + gs[-1]) / 2 + lift                   # floor top at the slot's middle (relative to the ladder level)
    floor = max(gs[5:8]) + 20.0                         # around the ladder, for the fall zone
    ab = cc.get_editor_property("abyss")
    ab.set_static_mesh(cube); ab.set_material(0, abyss_mat)
    ab.set_relative_scale3d(unreal.Vector(gap / 100.0 + 1.6, SLOT_LEN / 100.0, 3.0))
    tilt = math.degrees(math.atan(slope))
    ab.set_relative_rotation(unreal.Rotator(roll=-tilt, pitch=0, yaw=0), False, True)
    ab.set_relative_location(unreal.Vector(0, 0, mid - 150.0), False, True)
    # (check the tilt direction: the floor's +Y end must sit at the +Y ground)
    t = ab.get_world_transform()
    top_end = t.transform_location(unreal.Vector(0, 50, 50)).z - ZL
    if abs(top_end - (gs[-1] + lift)) > abs(t.transform_location(unreal.Vector(0, -50, 50)).z - ZL - (gs[-1] + lift)):
        ab.set_relative_rotation(unreal.Rotator(roll=tilt, pitch=0, yaw=0), False, True)
    fz = cc.get_editor_property("fall_zone")
    fz_top = zl - 150.0
    fz.set_box_extent(unreal.Vector(gap / 2 - 30.0, SLOT_LEN / 2 - 100.0, max((fz_top - floor) / 2, 50.0)), True)
    fz.set_relative_location(unreal.Vector(0, 0, (fz_top + floor) / 2), False, True)

    # snow pickets anchoring the ladder ends and the hand line
    for sx, zz in ((-1, zn), (1, zf)):
        for sy in (-1, 1):
            x, y = at(sx * (gap / 2 + 330.0), sy * 45.0)
            z = top_z(x, y, [ln, lf])
            pk = eas.spawn_actor_from_object(picket, unreal.Vector(x, y, z - 25.0), unreal.Rotator(roll=0, pitch=0, yaw=yaw + 30 * sy))
            pk.tags = ["Crevasse"]; pk.set_folder_path("Route/Crevasses")
            pk.get_component_by_class(unreal.StaticMeshComponent).set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)

    # the fixed rope over the crossing: follows the ramps (30 cm up), hand line 1 m over the ladder
    rope = next((r for k, r in ropes.items() if k == f"FixedRope_{leg}"), None)
    if rope:
        rp = list(rope.get_editor_property("rope_points"))
        def local(p):
            dx, dy = p.x - gx, p.y - gy
            return dx * fwd.x + dy * fwd.y, dx * side.x + dy * side.y
        x0, x1 = -(gap / 2 + LIP_L * sxn + 200.0), gap / 2 + LIP_L * sxf + 200.0
        inside = [k for k, p in enumerate(rp) if x0 <= local(p)[0] <= x1 and abs(local(p)[1]) < LIP_W / 2]
        if inside:
            k0, k1 = min(inside), max(inside)
            new = []
            x = x0
            while x <= x1:
                wx, wy = at(x)
                if abs(x) < 420.0:                              # along the whole 8 m ladder
                    z = ZL + zl + 100.0 + (x / (gap + 100.0)) * (zf - zn)
                else:
                    z = top_z(wx, wy, [ln, lf]) + 30.0
                new.append(unreal.Vector(wx, wy, z))
                x += 200.0
            rp = rp[:k0] + new + rp[k1 + 1:]
            rope.set_editor_property("rope_points", rp)
    ramp_n = math.degrees(math.atan2((LIP_H + LIP_FOOT) * s_n, (LIP_L - LIP_LANDING) * sxn)); ramp_f = math.degrees(math.atan2((LIP_H + LIP_FOOT) * s_f, (LIP_L - LIP_LANDING) * sxf))
    report.append(dict(n=n, leg=leg, f=f, gap_m=gap_m, crest_m=round(VISIBLE_M, 1), scale_near=(round(sxn, 2), round(s_n, 2)),
                       scale_far=(round(sxf, 2), round(s_f, 2)), ladder_pitch=round(pitch, 1), slot_depth_m=round((zl - floor) / 100.0, 1),
                       ramp_deg=(round(ramp_n, 1), round(ramp_f, 1)), x=round(gx), y=round(gy), z=round(ZL)))
    print("crossing", report[-1])
json.dump(report, open(f"{ROOT}/Saved/ScatterReports/crevasses.json", "w"), indent=1)
print("crossings placed:", len(report))
