import unreal
w=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world(); print('WORLD', w.get_name() if w else None)
