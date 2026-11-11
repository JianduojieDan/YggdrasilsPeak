# Runs INSIDE the editor (uepy.py), after world_tree_summit.py and summit_camp_and_tree_decor.py.
# 2026-11-11 user: do not hide every root inside the mountain; cut root pieces may break out of the slopes at random so
# the World Tree reads as gripping the whole summit. Giant root sections (Megascans Mediterranean branch, scaled up)
# arch out of the snow around the peak, running downhill away from the trunk, half buried, ends inside the slope.
import unreal, math, random
rng = random.Random(4242)
ea = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
PEAK = (1239630.0, 1413610.0, 381988.0)
CAMP = (1236165.0, 1417075.0)
for a in ea.get_all_level_actors():
    if a.get_actor_label().startswith("WorldTree_Root"):
        ea.destroy_actor(a)
acts = ea.get_all_level_actors()
skip = [a for a in acts if a.get_actor_label().startswith(("Rocks_", "Icefall_", "Glacier_", "Summit", "Landmark_", "WorldTree", "Forest"))]
def ground(x, y):
    h = unreal.SystemLibrary.line_trace_single_for_objects(world, unreal.Vector(x, y, 900000), unreal.Vector(x, y, 0),
        [unreal.ObjectTypeQuery.OBJECT_TYPE_QUERY1], True, skip, unreal.DrawDebugTrace.NONE, True)
    if not h: return None
    t = h.to_tuple()
    return t[4].z, t[7]
BRANCH = unreal.load_asset("/Game/Fab/Megascans/3D/Forest_Mediterranean_Tree_Branch_Wood_Large_04_wjvpcilfa/Raw/SM_wjvpcilfa_tier_0/StaticMeshes/SM_wjvpcilfa_tier_0")
STUMP = unreal.load_asset("/Game/Fab/Megascans/3D/Nordic_Forest_Tree_Uprooted_Medium_xiciaat/High/xiciaat_tier_1/StaticMeshes/xiciaat_tier_1")
holder = ea.spawn_actor_from_class(unreal.Actor, unreal.Vector(*PEAK))
holder.set_actor_label("WorldTree_Roots"); holder.set_folder_path("Environment/WorldTree")
sub = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem); root = sub.k2_gather_subobject_data_for_instance(holder)[0]
def ism(mesh):
    h, _ = sub.add_new_subobject(unreal.AddNewSubobjectParams(parent_handle=root, new_class=unreal.InstancedStaticMeshComponent))
    c = unreal.SubobjectDataBlueprintFunctionLibrary.get_object(unreal.SubobjectDataBlueprintFunctionLibrary.get_data(h))
    c.set_static_mesh(mesh); c.set_cast_shadow(True); c.set_editor_property("visible_in_ray_tracing", False)
    return c
roots, stumps = [], []
placed = 0
tries = 0
while placed < 22 and tries < 400:
    tries += 1
    a = rng.uniform(0, 2 * math.pi)
    r = rng.uniform(4500, 21000)
    x, y = PEAK[0] + math.cos(a) * r, PEAK[1] + math.sin(a) * r
    if math.hypot(x - CAMP[0], y - CAMP[1]) < 2500: continue          # keep the camp clear
    g = ground(x, y)
    if not g: continue
    z, n = g
    # follow the slope downhill (away from the trunk), arching out of it: one end buried uphill, one end downhill
    down = math.atan2(y - PEAK[1], x - PEAK[0])
    yaw = math.degrees(down) + rng.uniform(-25, 25)
    slope = math.degrees(math.acos(max(-1.0, min(1.0, n.z))))
    s = rng.uniform(22.0, 42.0)                                         # 1.5 m branch -> 33-63 m of root
    thick = 36.0 * s
    pitch = -slope * rng.uniform(0.6, 1.0) + rng.uniform(-6, 6)          # roughly along the slope
    xf = unreal.Transform(unreal.Vector(x, y, z - thick * rng.uniform(0.35, 0.55)),
                          unreal.Rotator(roll=rng.uniform(-25, 25), pitch=pitch, yaw=yaw), unreal.Vector(s, s * rng.uniform(0.8, 1.2), s))
    roots.append(xf); placed += 1
    if rng.random() < 0.25 and STUMP:                                    # now and then a knot of roots breaking out
        ss = rng.uniform(6.0, 10.0)
        stumps.append(unreal.Transform(unreal.Vector(x + rng.uniform(-800, 800), y + rng.uniform(-800, 800), z - 200 * ss * 0.55),
                                       unreal.Rotator(roll=rng.uniform(-20, 20), pitch=rng.uniform(-20, 20), yaw=rng.uniform(0, 360)), unreal.Vector(ss, ss, ss)))
ism(BRANCH).add_instances(roots, False, True)
if stumps: ism(STUMP).add_instances(stumps, False, True)
print("roots", len(roots), "knots", len(stumps))
print("saved", unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level())
