import unreal,json
eas=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
out={}
for a in eas.get_all_level_actors():
    if a.get_actor_label()=='Rocks_Baked':
        for c in a.get_components_by_class(unreal.InstancedStaticMeshComponent):
            out[c.static_mesh.get_name()]=[[c.get_instance_transform(i,True).translation.x,c.get_instance_transform(i,True).translation.y] for i in range(c.get_instance_count())]
json.dump(out,open('F:/UE_project/YggdrasilsPeak/Saved/DetailTiles/route/baked_rocks.json','w')); print({k:len(v) for k,v in out.items()})
