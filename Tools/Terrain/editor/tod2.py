import unreal
eas=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
u=[a for a in eas.get_all_level_actors() if a.get_class().get_name()=='Ultra_Dynamic_Sky_C'][0]
t=float(open('C:/Users/danji/AppData/Local/Temp/ygg/tod.txt').read().strip())
u.set_editor_property('Time of Day', t)
l=u.get_actor_location(); u.set_actor_location(l+unreal.Vector(0,0,1),False,False); u.set_actor_location(l,False,False)
print('time now', u.get_editor_property('Time of Day'))
