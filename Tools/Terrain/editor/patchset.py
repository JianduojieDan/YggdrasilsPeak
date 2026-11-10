import unreal
eas=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
r=[a for a in eas.get_all_level_actors() if a.get_class().get_name()=='LandscapeRebuilder'][0]
args=open('C:/Users/danji/AppData/Local/Temp/ygg/patch_args.txt').read().split('|')
print(r.apply_height_patch(args[0], args[1].strip()=='1'))
