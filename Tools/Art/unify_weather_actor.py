# Runs INSIDE the editor: the level's sky came from /Game/UltraDynamicSky but its weather from the second copy under Content_From_Internet, so weather
# could not drive the sky. Replaces the weather actor with the same class from the sky's copy, carrying over the scalar settings that differ from defaults.
import unreal
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem); les.load_level("/Game/LevelPrototyping/base_map")
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
acts = eas.get_all_level_actors()
old = [a for a in acts if "Ultra_Dynamic_Weather" in a.get_class().get_name()][0]
print("old", old.get_class().get_path_name())
cls = unreal.load_object(None, "/Game/UltraDynamicSky/Blueprints/Ultra_Dynamic_Weather.Ultra_Dynamic_Weather_C")
if "Content_From_Internet" in old.get_class().get_path_name():
    new = eas.spawn_actor_from_class(cls, old.get_actor_location(), old.get_actor_rotation())
    label = old.get_actor_label(); folder = old.get_folder_path()
    names = ["Wind Direction", "Random Weather Variation", "Use Random Weather Variation", "Enable Sound Effects", "Sound Effects Volume", "Rain Sound Volume", "Snow Particle Spawn Rate", "Weather", "Enable Snow Trails", "Snow Coverage", "Material Snow Coverage", "Use Sound Occlusion", "Simulate Real Weather", "Enable Lightning Flashes", "Lightning Flash Intensity"]
    for n in names:
        try:
            v = old.get_editor_property(n)
            if n == "Weather":
                v = unreal.load_asset("/Game/UltraDynamicSky/Blueprints/Weather_Effects/Weather_Presets/" + v.get_name()) if v else v
            new.set_editor_property(n, v); print("copied", n, str(v)[:40])
        except Exception as e:
            pass
    eas.destroy_actor(old)
    new.set_actor_label(label); new.set_folder_path(folder)
    les.save_current_level()
    print("swapped")
