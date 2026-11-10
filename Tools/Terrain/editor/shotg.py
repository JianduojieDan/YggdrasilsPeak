import unreal
world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
ues=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
eas=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
ign=[a for a in eas.get_all_level_actors() if a.get_actor_label() in ('Rocks_Route','Rocks_Baked')]
x,y,dz,p,yw=[float(v) for v in open('C:/Users/danji/AppData/Local/Temp/ygg/view.txt').read().split()]
h=unreal.SystemLibrary.line_trace_single_for_objects(world,unreal.Vector(x,y,900000),unreal.Vector(x,y,-400000),[unreal.ObjectTypeQuery.OBJECT_TYPE_QUERY1],True,ign,unreal.DrawDebugTrace.NONE,True)
z=h.to_tuple()[4].z if h else 0
ues.set_level_viewport_camera_info(unreal.Vector(x,y,z+dz),unreal.Rotator(pitch=p,yaw=yw,roll=0))
print('cam',x,y,z+dz,'hit',h.to_tuple()[9].get_actor_label() if h else None)
