# Runs inside the editor: opens the game level (the editor starts on the start-menu map).
import unreal
ok = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).load_level('/Game/LevelPrototyping/base_map')
print('loaded base_map', ok)
