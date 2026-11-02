# Runs INSIDE the editor (uepy.py). Serac collapse zones (ASeracCollapseZone) on the Icefall route, each end marked
# with red pickets either side of the trail. Kept clear of crevasse crossings, checkpoints, clues and camps.
# Re-running replaces them (tag Hazard).
import unreal, json, math
R = json.load(open("F:/UE_project/YggdrasilsPeak/Tools/RoutePlanning/route.json"))
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
for a in eas.get_all_level_actors():
    if a.actor_has_tag("Hazard"):
        a.destroy_actor()
ZONES = [(2, 0.56), (3, 0.47)]          # (leg, fraction) - shifted along the leg until clear
HALF_LEN = 2200.0
avoid = [a.get_actor_location() for a in eas.get_all_level_actors()
         if a.get_class().get_name() in ("TrailCheckpoint", "StoryClue", "RouteLandmark", "CrevasseCrossing")]
IGN = [a for a in eas.get_all_level_actors() if a.get_class().get_name() in ("ConstrainedScatter", "CrevasseCrossing")]
def ground(x, y, z):
    h = unreal.SystemLibrary.line_trace_single(world, unreal.Vector(x, y, z + 5000), unreal.Vector(x, y, z - 8000),
        unreal.TraceTypeQuery.ECC_VISIBILITY, True, IGN, unreal.DrawDebugTrace.NONE, True).to_tuple()
    return h[4].z if h[0] else z
ZC = unreal.load_class(None, "/Script/YggdrasilsPeak.SeracCollapseZone")
picket = unreal.load_asset("/Game/Props/Blender/SM_SnowPicket")
for n, (leg, f) in enumerate(ZONES):
    pts = R[leg]["pts"]
    for step in range(40):
        ff = f + (step // 2 + 1) * 0.01 * (1 if step % 2 == 0 else -1) if step else f
        i = min(int(ff * (len(pts) - 1)), len(pts) - 2)
        c = pts[i]
        if min((math.hypot(c[0] - v.x, c[1] - v.y) for v in avoid), default=1e9) > HALF_LEN + 3000:
            break
    a, b = pts[max(i - 3, 0)], pts[min(i + 3, len(pts) - 1)]
    yaw = math.degrees(math.atan2(b[1] - a[1], b[0] - a[0]))
    fwd = unreal.Vector(math.cos(math.radians(yaw)), math.sin(math.radians(yaw)), 0); side = unreal.Vector(-fwd.y, fwd.x, 0)
    gz = ground(c[0], c[1], c[2])
    z = unreal.load_class(None, "/Script/YggdrasilsPeak.SeracCollapseZone")
    zone = eas.spawn_actor_from_class(ZC, unreal.Vector(c[0], c[1], gz + 1000), unreal.Rotator(roll=0, pitch=0, yaw=yaw))
    zone.set_actor_label(f"SeracZone_{n}_leg{leg}"); zone.set_folder_path("Route/Hazards"); zone.tags = ["Hazard"]
    for e in (-1, 1):
        for s in (-1, 1):
            x = c[0] + fwd.x * e * HALF_LEN + side.x * s * 260; y = c[1] + fwd.y * e * HALF_LEN + side.y * s * 260
            pk = eas.spawn_actor_from_object(picket, unreal.Vector(x, y, ground(x, y, gz) - 20), unreal.Rotator(roll=0, pitch=0, yaw=yaw + 20 * s))
            pk.set_actor_scale3d(unreal.Vector(1.6, 1.6, 1.6))
            pk.tags = ["Hazard"]; pk.set_folder_path("Route/Hazards")
            pk.get_component_by_class(unreal.StaticMeshComponent).set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
    print("zone", n, "leg", leg, "at f", round(ff if step else f, 2), (round(c[0]), round(c[1]), round(gz)))

# hidden crevasses: off the roped route on the upper Icefall and the Western Cwm, running across the valley
HC = unreal.load_class(None, "/Script/YggdrasilsPeak.HiddenCrevasse")
import random
rnd = random.Random(11)
placed = 0
for leg, count in ((3, 6), (4, 8)):
    pts = R[leg]["pts"]
    for k in range(count * 3):
        if sum(1 for a in eas.get_all_level_actors() if a.actor_has_tag("Hazard") and a.get_actor_label().startswith(f"HiddenCrevasse_leg{leg}")) >= count:
            break
        i = rnd.randint(int(len(pts) * 0.15), int(len(pts) * 0.9))
        a_, b_ = pts[max(i - 2, 0)], pts[min(i + 2, len(pts) - 1)]
        yaw = math.degrees(math.atan2(b_[1] - a_[1], b_[0] - a_[0]))
        side = rnd.choice((-1, 1)); off = rnd.uniform(1800, 3500)
        x = pts[i][0] + math.cos(math.radians(yaw + 90 * side)) * off; y = pts[i][1] + math.sin(math.radians(yaw + 90 * side)) * off
        if min((math.hypot(x - v.x, y - v.y) for v in avoid), default=1e9) < 3000:
            continue
        length = rnd.uniform(1800, 3200)
        gz = ground(x, y, pts[i][2])
        hc = eas.spawn_actor_from_class(HC, unreal.Vector(x, y, gz), unreal.Rotator(roll=0, pitch=0, yaw=yaw + rnd.uniform(-20, 20)))
        hc.call_method("SetSize", (length, rnd.uniform(200, 320)))
        hc.get_editor_property("sag").set_decal_material(unreal.load_asset("/Game/Environment/Glacier/Crevasse/M_HiddenCrevasseSag"))
        hc.set_actor_label(f"HiddenCrevasse_leg{leg}_{k}"); hc.set_folder_path("Route/Hazards"); hc.tags = ["Hazard"]
        placed += 1
print("hidden crevasses", placed)
