# Runs inside the editor. Landscape Nanite was simplified into huge flat triangles (edge factor 16) with cracks (no skirt):
# the summit looked like hard sheets with gaps. Shorter edges and a skirt, then rebuild by toggling Nanite and saving.
import unreal
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
ls = [a for a in eas.get_all_level_actors() if a.get_actor_label() == "Landscape_Smooth"][0]
for p in ("nanite_max_edge_length_factor", "nanite_skirt_enabled", "nanite_skirt_depth"):
    print("before", p, ls.get_editor_property(p))
ls.set_editor_property("nanite_max_edge_length_factor", 2.0)
ls.set_editor_property("nanite_skirt_enabled", True)
ls.set_editor_property("nanite_skirt_depth", 0.5)
ls.set_editor_property("enable_nanite", False)
ls.set_editor_property("enable_nanite", True)
for p in ("nanite_max_edge_length_factor", "nanite_skirt_enabled", "nanite_skirt_depth"):
    print("after", p, ls.get_editor_property(p))
unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
print("saved")
