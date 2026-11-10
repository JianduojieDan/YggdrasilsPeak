# Runs inside the editor, after a terrain change. Removes the rejected route cliffs, stands tents upright on their levelled platforms, lets the
# player walk into tents (collision follows the tent's real shape), and re-seats the rubble.
import unreal, math
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
w = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
acts = eas.get_all_level_actors()
gone = 0
for a in acts:
    if a.actor_has_tag("RouteCliff"):
        a.destroy_actor(); gone += 1
print("route cliffs removed", gone)
done = set()
tents = 0
for a in eas.get_all_level_actors():
    lab = a.get_actor_label()
    if not a.actor_has_tag("CampProp") or a.get_class().get_name() != "StaticMeshActor": continue
    if "_Tent" in lab or "DiningTent" in lab or "Stores" in lab:
        r = a.get_actor_rotation()
        a.set_actor_rotation(unreal.Rotator(roll=0, pitch=0, yaw=r.yaw), False)
        tents += 1
        mesh = a.static_mesh_component.static_mesh
        if mesh.get_path_name() not in done:
            done.add(mesh.get_path_name())
            body = mesh.get_editor_property("body_setup")
            body.set_editor_property("collision_trace_flag", unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE)
            mesh.set_editor_property("body_setup", body)
            unreal.EditorAssetLibrary.save_loaded_asset(mesh, False)
            print("walk-in collision", mesh.get_name())
print("tents upright", tents)
rub = [a for a in eas.get_all_level_actors() if a.get_actor_label() == "Camp_Rubble"]
if rub:
    ls = [a for a in eas.get_all_level_actors() if a.get_actor_label() == "Landscape_Smooth"][0]
    ign = [a for a in eas.get_all_level_actors() if a is not ls]
    moved = 0
    for c in rub[0].get_components_by_class(unreal.InstancedStaticMeshComponent):
        b = c.static_mesh.get_bounding_box()
        for i in range(c.get_instance_count()):
            t = c.get_instance_transform(i, True)
            P = t.translation
            h = unreal.SystemLibrary.line_trace_single(w, unreal.Vector(P.x, P.y, P.z + 30000), unreal.Vector(P.x, P.y, P.z - 30000), unreal.TraceTypeQuery.ECC_VISIBILITY, False, ign, unreal.DrawDebugTrace.NONE, True)
            if not h: continue
            g = h.to_tuple()[4].z
            k = t.scale3d.z
            c.update_instance_transform(i, unreal.Transform(unreal.Vector(P.x, P.y, g - b.min.z * k - (b.max.z - b.min.z) * k * 0.45), t.rotation.rotator(), t.scale3d), True, True, True)
            moved += 1
    print("rubble seated", moved)
unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
print("saved")
