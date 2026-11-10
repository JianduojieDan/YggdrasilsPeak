# Runs inside the editor. Hand-style placement of cliff meshes beside the route, for climbing complexity. Each stands to one side
# of the line, never on it, clear of camps, at varied sizes. Rerun replaces them (tag RouteCliff). Screenshot views are printed.
import unreal, json, math, random
ROOT = "F:/UE_project/YggdrasilsPeak"
CLIFFS = ["/Game/Fab/Alaskan_Cliff_Rock_18_Free/alaskan_cliff_rock_18_free/StaticMeshes/alaskan_cliff_rock_18_free",
          "/Game/Fab/Megascans/3D/Huge_Nordic_Coastal_Cliff_venrdcgga/Medium/venrdcgga_tier_2/StaticMeshes/venrdcgga_tier_2",
          "/Game/Fab/Megascans/3D/Huge_Icelandic_Lava_Cliff_siEoZ/Medium/siEoZ_tier_2/StaticMeshes/siEoZ_tier_2"]
LEGS = {3: 5, 4: 6}         # route leg -> cliffs on it
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
w = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
acts = eas.get_all_level_actors()
for a in acts:
    if a.actor_has_tag("RouteCliff"):
        a.destroy_actor()
acts = eas.get_all_level_actors()
camps = [a.get_actor_location() for a in acts if a.get_class().get_name() == "RouteLandmark"]
R = json.load(open(f"{ROOT}/Tools/RoutePlanning/route.json"))
all_pts = [p for leg in R for p in leg["pts"]]
for a in acts:
    if a.get_class().get_name() == "FixedRope":
        all_pts += [[p.x, p.y, p.z] for p in a.get_editor_property("rope_points")]
meshes = [unreal.load_asset(p) for p in CLIFFS]
rng = random.Random(77)
def ground(x, y, z):
    h = unreal.SystemLibrary.line_trace_single(w, unreal.Vector(x, y, z + 100000), unreal.Vector(x, y, z - 150000), unreal.TraceTypeQuery.ECC_VISIBILITY, False, [], unreal.DrawDebugTrace.NONE, True)
    return h.to_tuple()[4] if h else None
placed = 0
for leg, count in LEGS.items():
    pts = R[leg]["pts"]
    for i in range(count):
        k = int((i + 0.5) / count * (len(pts) - 2))
        a, b = pts[k], pts[k + 1]
        yaw = math.atan2(b[1] - a[1], b[0] - a[0])
        mesh = meshes[i % len(meshes)]
        bb = mesh.get_bounding_box()
        width = max(bb.max.x - bb.min.x, bb.max.y - bb.min.y)
        size = rng.uniform(5000.0, 11000.0)
        s = size / width
        for side in (1, -1):
            off = size * 0.6 + 900.0
            x, y = a[0] + math.cos(yaw + side * math.pi / 2) * off, a[1] + math.sin(yaw + side * math.pi / 2) * off
            if min(math.hypot(p[0] - x, p[1] - y) for p in all_pts) < size * 0.55 + 500.0: continue
            if min(math.hypot(c.x - x, c.y - y) for c in camps) < size * 0.6 + 4000.0: continue
            g = ground(x, y, a[2])
            if not g: continue
            act = eas.spawn_actor_from_object(mesh, unreal.Vector(x, y, g.z - bb.min.z * s - (bb.max.z - bb.min.z) * s * 0.4),
                                              unreal.Rotator(roll=0, pitch=0, yaw=math.degrees(yaw) + (90 if side > 0 else -90)))
            act.set_actor_scale3d(unreal.Vector(s, s, s))
            act.set_actor_label("RouteCliff_%d_%d" % (leg, i))
            act.tags = [unreal.Name("RouteCliff")]
            placed += 1
            print("VIEW", leg, i, round(x), round(y), round(g.z), round(size / 100), mesh.get_name())
            break
print("cliffs placed", placed)
unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
print("saved")
