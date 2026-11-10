import unreal,json
world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
eas=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
al=eas.get_all_level_actors()
ign=[a for a in al if a.get_actor_label() in ('Rocks_Route','Rocks_Baked')]
pts=json.load(open('C:/Users/danji/AppData/Local/Temp/ygg/trpts.json')); out=[]
for n,x,y in pts:
    h=unreal.SystemLibrary.line_trace_single_for_objects(world,unreal.Vector(x,y,900000),unreal.Vector(x,y,-400000),[unreal.ObjectTypeQuery.OBJECT_TYPE_QUERY1],True,ign,unreal.DrawDebugTrace.NONE,True)
    if h:
        t=h.to_tuple(); out.append([n,x,y,t[4].z,t[9].get_actor_label() if t[9] else None])
    else: out.append([n,x,y,None,None])
json.dump(out,open('C:/Users/danji/AppData/Local/Temp/ygg/trout.json','w')); print('traced',len(out))
