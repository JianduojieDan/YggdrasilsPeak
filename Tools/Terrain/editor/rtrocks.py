import unreal
eas=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
for a in eas.get_all_level_actors():
    if a.get_actor_label()=='Rocks_Route':
        for c in a.get_components_by_class(unreal.HierarchicalInstancedStaticMeshComponent):
            m=c.static_mesh.get_name(); n=c.get_instance_count()
            nan=c.static_mesh.get_editor_property('nanite_settings').enabled
            if n>50000: c.set_editor_property('visible_in_ray_tracing',False)
            print(c.get_name(),m,n,'nanite',nan,'rt',c.get_editor_property('visible_in_ray_tracing'))
