# Runs inside the editor. Base Camp stands on rubble over ice, not clean snow: flat stones scattered around the camp (actor Camp_Rubble,
# no collision, no shadows from the small ones), and a bigger puja altar around the flag pole. Run once.
import unreal, math, random
STONE = "/Game/Fab/Stone_-_photogrammetry/stone_photogrammetry/StaticMeshes/stone_photogrammetry"
CAMPS = {"Landmark_2": (6500.0, 1600), "Landmark_1": (3000.0, 350)}
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
w = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
acts = eas.get_all_level_actors()
for a in acts:
    if a.get_actor_label() == "Camp_Rubble":
        a.destroy_actor()
L = {a.get_actor_label(): a for a in acts}
base = L["Landmark_2"].get_actor_location()
scaled = 0
for a in acts:
    lab = a.get_actor_label()
    if "Altar" in lab and a.get_class().get_name() == "StaticMeshActor":
        l = a.get_actor_location()
        if math.hypot(l.x - base.x, l.y - base.y) < 2000:
            a.set_actor_location(unreal.Vector(base.x + (l.x - base.x) * 1.9, base.y + (l.y - base.y) * 1.9, l.z), False, True)
            a.set_actor_scale3d(a.get_actor_scale3d() * 2.0)
            scaled += 1
print("altar stones", scaled)
mesh = unreal.load_asset(STONE)
b = mesh.get_bounding_box()
size = b.max.x - b.min.x
rng = random.Random(21)
out = []
for name, (radius, count) in CAMPS.items():
    c = L[name].get_actor_location()
    for _ in range(count):
        d = radius * math.sqrt(rng.random())
        a = rng.uniform(0, 2 * math.pi)
        x, y = c.x + math.cos(a) * d, c.y + math.sin(a) * d
        h = unreal.SystemLibrary.line_trace_single(w, unreal.Vector(x, y, c.z + 30000), unreal.Vector(x, y, c.z - 30000), unreal.TraceTypeQuery.ECC_VISIBILITY, False, [], unreal.DrawDebugTrace.NONE, True)
        if not h: continue
        t = h.to_tuple()
        if not t[9] or t[9].get_actor_label() != "Landscape_Smooth": continue   # not under tents or props
        width = rng.choice((rng.uniform(15, 40), rng.uniform(15, 40), rng.uniform(40, 90), rng.uniform(90, 180)))
        k = width / size
        up = t[7]
        rot = unreal.MathLibrary.make_rot_from_zx(up, unreal.Vector(math.cos(a * 7), math.sin(a * 7), 0.0))
        out.append(unreal.Transform(unreal.Vector(t[4].x, t[4].y, t[4].z - b.min.z * k * 0.5 - (b.max.z - b.min.z) * k * 0.2), rot, unreal.Vector(k, k * rng.uniform(0.7, 1.1), k * rng.uniform(0.35, 0.7))))
act = eas.spawn_actor_from_class(unreal.Actor, unreal.Vector(0, 0, 0))
act.set_actor_label("Camp_Rubble")
sub = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
root = sub.k2_gather_subobject_data_for_instance(act)[0]
h, reason = sub.add_new_subobject(unreal.AddNewSubobjectParams(parent_handle=root, new_class=unreal.HierarchicalInstancedStaticMeshComponent))
comp = unreal.SubobjectDataBlueprintFunctionLibrary.get_associated_object(unreal.SubobjectDataBlueprintFunctionLibrary.get_data(h))
comp.set_static_mesh(mesh)
comp.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
comp.set_cast_shadow(False)
comp.add_instances(out, False, True)
print("rubble stones", comp.get_instance_count())
unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
print("saved")
