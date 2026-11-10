import unreal
names=open('C:/Users/danji/AppData/Local/Temp/ygg/batch.txt').read().split()
ss=unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem); n=0
for name in names:
    m=unreal.load_asset(f'/Game/Environment/DetailTiles/Route/SM_{name}'); c=unreal.load_asset(f'/Game/Environment/DetailTiles/Route/SM_{name}_col')
    if not m or not c: continue
    ns=c.get_editor_property('nanite_settings')
    if ns.enabled:
        ns.enabled=False; ss.set_nanite_settings(c,ns,True)
    m.set_editor_property('complex_collision_mesh',None); m.set_editor_property('complex_collision_mesh',c)
    unreal.EditorAssetLibrary.save_loaded_asset(c); unreal.EditorAssetLibrary.save_loaded_asset(m); n+=1
print('fixed',n,'colverts',ss.get_number_verts(c,0))
