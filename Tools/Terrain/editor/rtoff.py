import unreal
v=open('C:/Users/danji/AppData/Local/Temp/ygg/rt.txt').read().strip()=='1'
eas=unreal.get_editor_subsystem(unreal.EditorActorSubsystem); n=0
for a in eas.get_all_level_actors():
    if a.get_actor_label().startswith('DetailTile_'):
        c=a.static_mesh_component; c.set_editor_property('visible_in_ray_tracing',v); n+=1
print('rt',v,n)
