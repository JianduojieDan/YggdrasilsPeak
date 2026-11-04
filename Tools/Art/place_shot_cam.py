# Runs INSIDE the editor: puts the (hidden, harmless) camera used by game_shot.ps1 at SHOT = (x, y, z, pitch, yaw) and saves.
import unreal
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
x, y, z, p, w = SHOT
cam = [a for a in eas.get_all_level_actors() if a.get_actor_label() == "GameShotCam"]
cam = cam[0] if cam else eas.spawn_actor_from_class(unreal.CameraActor, unreal.Vector(x, y, z))
cam.set_actor_label("GameShotCam"); cam.set_folder_path("Tools")
cam.set_actor_location_and_rotation(unreal.Vector(x, y, z), unreal.Rotator(roll=0, pitch=p, yaw=w), False, True)
cc = cam.get_component_by_class(unreal.CameraComponent)
cc.set_editor_property("field_of_view", 90.0); cc.set_editor_property("constrain_aspect_ratio", False)
unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
print("CAMNAME", cam.get_name())
