# Runs inside the editor. The landscape's flat 17 m triangles shadowing themselves at a low sun made hard polygon shapes
# (fine shading looks smooth, the shadow does not). So the landscape casts no shadow; FarRing, about 30 m below it in the
# playable area, casts the mountain shadows. Objects keep their own. Proven by captures 2026-11-06 (Saved/Compare/cast_sheet).
import unreal
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
acts = {a.get_actor_label(): a for a in eas.get_all_level_actors()}
land = acts["Landscape_Smooth"]
ring = acts["FarRing"].static_mesh_component
land.set_editor_property("cast_shadow", False)
land.set_editor_property("cast_dynamic_shadow", False)
for c in land.get_components_by_class(unreal.LandscapeComponent):
    c.set_cast_shadow(False)
ring.set_editor_property("cast_shadow", True)
ring.set_cast_shadow(True)
print("landscape cast", land.get_editor_property("cast_shadow"), "ring cast", ring.get_editor_property("cast_shadow"))
unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
print("saved")
