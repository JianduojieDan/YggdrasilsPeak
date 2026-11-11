# Runs INSIDE the editor (uepy.py). Performance pass on the Megaplant forest (2026-11-11: Play lagged, 13 GB RAM):
#   - no tree in ray tracing (20k skinned instances rebuilt the ray-tracing scene every frame);
#   - culled by distance: the trees only matter around the valley below Dingboche (10 km), reeds within 150 m;
#   - the poplars (129 trees) and reeds are removed: each species loads its whole set of Nanite branch parts
#     (108 skeletal meshes, ~2.4 GB) for very little on screen.
import unreal
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
for a in eas.get_all_level_actors():
    lab = a.get_actor_label()
    if lab in ("Forest_Poplar", "Forest_Reeds"):
        eas.destroy_actor(a); print("removed", lab); continue
    if not lab.startswith("Forest_"):
        continue
    for c in a.get_components_by_class(unreal.InstancedSkinnedMeshComponent):
        c.set_editor_property("visible_in_ray_tracing", False)
        c.set_editor_property("instance_start_cull_distance", 800000)
        c.set_editor_property("instance_end_cull_distance", 1000000)
    print("tuned", lab)
print("saved", unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level())
