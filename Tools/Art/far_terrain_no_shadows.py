# Runs INSIDE the editor: the far meshes (200 m+ triangles) must not cast: their coarse silhouettes threw straight-edged blocks of shadow over the near landscape.
import unreal
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
for a in eas.get_all_level_actors():
    if a.get_actor_label() in ("DistantEverest", "FarHazeFloor", "HorizonSkirt", "FarRing", "FarBackground"):
        a.modify(); a.static_mesh_component.modify(); a.static_mesh_component.set_editor_property("cast_shadow", False); print("no shadow", a.get_actor_label())
unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, False)
