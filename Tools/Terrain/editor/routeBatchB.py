import unreal,json
D='F:/UE_project/YggdrasilsPeak/Saved/DetailTiles/route'
idx=json.load(open(D+'/tiles.json'))
names=[n for n in open('C:/Users/danji/AppData/Local/Temp/ygg/batch.txt').read().split() if n in idx]
eas=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
have={a.get_actor_label():a for a in eas.get_all_level_actors() if a.get_actor_label().startswith('DetailTile_R_')}
for name in names:
    piv=idx[name]
    m=unreal.load_asset(f'/Game/Environment/DetailTiles/Route/SM_{name}'); c=unreal.load_asset(f'/Game/Environment/DetailTiles/Route/SM_{name}_col')
    m.set_editor_property('complex_collision_mesh',c)
    unreal.EditorAssetLibrary.save_loaded_asset(c); unreal.EditorAssetLibrary.save_loaded_asset(m)
    lab='DetailTile_'+name
    if lab in have: eas.destroy_actor(have[lab])
    a=eas.spawn_actor_from_object(m,unreal.Vector(piv[0]*100,piv[1]*100,piv[2]*100)); a.set_actor_label(lab); a.set_folder_path('Environment/DetailTiles/Route')
    # keep tiles out of hardware ray tracing: Lumen would trace their coarse Nanite fallback and the snow turns
    # beige in straight-edged patches where that proxy sits above the real surface
    a.static_mesh_component.set_editor_property('visible_in_ray_tracing',False)
unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
unreal.SystemLibrary.collect_garbage()
print('placed',len(names))
