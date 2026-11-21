import unreal
# Runs INSIDE the editor via Tools/RoutePlanning/uepy. 2026-11-21 (user: "the tent interior is very shiny - make a light
# inside the tent and leave some shadows"): swaps the chapter-1 tent's centred 7.0 point light (tag "tentlight": evenly
# bright like daylight, one hard shadow) for a small lamp hung from the apex loop and aimed down - wide soft cone onto the
# bags and gear, upper walls darker, soft shadows, low shine. Keeps the tags/folder/visibility so the story still shows/hides
# it. The world builder (build_chapter1_world) now places the same lamp.
W = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
if W.get_name() != "base_map":
    unreal.EditorLoadingAndSavingUtils.load_map("/Game/LevelPrototyping/base_map")
EAS = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
acts = EAS.get_all_level_actors()
old = [a for a in acts if "tentlight" in [str(t) for t in a.tags]]
tents = [a for a in acts if isinstance(a, unreal.StaticMeshActor) and a.static_mesh_component.static_mesh
         and a.static_mesh_component.static_mesh.get_name() == "tent_dome"]
for o in old:
    loc = o.get_actor_location()
    tent = min(tents, key=lambda t: (t.get_actor_location() - loc).length())
    to, te = tent.get_actor_bounds(False)
    vis = o.light_component.get_editor_property("visible")
    tags, folder = list(o.tags), o.get_folder_path()
    at = unreal.Vector(loc.x, loc.y, to.z + te.z - 16.0)
    s = EAS.spawn_actor_from_class(unreal.SpotLight, at, unreal.Rotator(0, -90, 0))
    c = s.light_component
    for k, v in (("intensity", 4.0), ("attenuation_radius", 260.0), ("outer_cone_angle", 68.0), ("inner_cone_angle", 22.0),
                 ("source_radius", 2.5), ("specular_scale", 0.35), ("cast_shadows", True), ("visible", vis)):
        c.set_editor_property(k, v)
    c.set_editor_property("light_color", unreal.Color(r=255, g=200, b=158, a=255))
    s.tags = tags; s.set_folder_path(folder); s.set_actor_label("TentLamp")
    print("LAMP", o.get_actor_label(), "->", round(at.z - (to.z - te.z)), "cm over the tent base, under the apex", round(to.z + te.z - at.z), "vis", vis,
          "dir", s.get_actor_forward_vector())
    EAS.destroy_actor(o)
unreal.EditorLevelLibrary.save_current_level()
print("done")
