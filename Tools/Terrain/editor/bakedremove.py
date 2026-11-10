import unreal,json
rm=json.load(open('F:/UE_project/YggdrasilsPeak/Saved/DetailTiles/route/baked_remove.json'))
eas=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
for a in eas.get_all_level_actors():
    if a.get_actor_label()=='Rocks_Baked':
        for c in a.get_components_by_class(unreal.InstancedStaticMeshComponent):
            idx=rm.get(c.static_mesh.get_name(),[])
            if idx: c.remove_instances(sorted(idx))
            print(c.static_mesh.get_name(),'removed',len(idx),'left',c.get_instance_count())
