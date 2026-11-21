import unreal, random
# Runs INSIDE the editor via Tools/RoutePlanning/uepy. 2026-11-21: fire_setup2 spawned the stone ring with a plain yaw, but
# the Fab stone_fireplace mesh is not Z-up (the camp's own ring carries a tilt to stand it up) -> it came in on its side.
# Copy each old ring's rotation + scale, add a world-Z yaw, size it to a ~1.2 m ring, sit its bottom 4 cm into the snow
# right under the flames. Prints sizes so the result can be checked by numbers.
W = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
if W.get_name() != "base_map":
    unreal.EditorLoadingAndSavingUtils.load_map("/Game/LevelPrototyping/base_map")
    W = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
EAS = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
ML = unreal.MathLibrary
random.seed(88)
acts = EAS.get_all_level_actors()
olds = [a for a in acts if isinstance(a, unreal.StaticMeshActor) and a.static_mesh_component.static_mesh
        and a.static_mesh_component.static_mesh.get_name() == "stone_fireplace" and "CampFire" not in [str(t) for t in a.tags]]
rings = [a for a in acts if a.get_actor_label().startswith("CampFire_Ring")]
card = unreal.load_asset("/Game/Environment/Fire/SM_FireCard"); cb = card.get_bounding_box()
print("CARD", cb.min, cb.max)
sm = olds[0].static_mesh_component.static_mesh; bb = sm.get_bounding_box()
print("MESH", bb.min, bb.max)
for r in rings:
    loc = r.get_actor_location()
    old = min(olds, key=lambda o: (o.get_actor_location() - loc).length())
    orot, osc = old.get_actor_rotation(), old.get_actor_scale3d()
    print("OLD", old.get_actor_label(), orot, osc)
    rot = ML.compose_rotators(orot, unreal.Rotator(0, 0, random.uniform(0, 360)))
    r.set_actor_rotation(rot, False); r.set_actor_scale3d(osc)
    o, e = r.get_actor_bounds(False)
    k = 120.0 / max(2 * e.x, 2 * e.y, 1.0)
    r.set_actor_scale3d(osc * k)
    o, e = r.get_actor_bounds(False)
    r.set_actor_location(unreal.Vector(loc.x, loc.y, r.get_actor_location().z + (loc.z - 1.0) - (o.z - e.z)), False, False)
    o, e = r.get_actor_bounds(False)
    print("RING", r.get_actor_label(), "size", round(2 * e.x), round(2 * e.y), "h", round(2 * e.z), "bottom", round(o.z - e.z), "ground~", round(loc.z + 3))
unreal.EditorLevelLibrary.save_current_level()
print("done")
