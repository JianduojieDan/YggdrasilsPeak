# Runs inside the editor. The hidden old Landscape keeps its collision through saves, so players and props stood on an invisible surface
# above the visible terrain. Moves it 3 km down, out of the way. Undo by raising it 300000 cm. Not deleted: LandscapeRebuilder's source.
import unreal
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
old = [a for a in eas.get_all_level_actors() if a.get_actor_label() == "Landscape"][0]
print("before", old.get_actor_location())
if old.get_actor_location().z > -100000:
    old.set_actor_location(old.get_actor_location() + unreal.Vector(0, 0, -300000), False, True)
print("after", old.get_actor_location())
unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
print("saved")
