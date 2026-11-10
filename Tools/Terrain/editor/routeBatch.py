# Imports one batch of route tiles (names in batch.txt): render + 2 m collision, Nanite, material, actor placement.
import unreal,json
D='F:/UE_project/YggdrasilsPeak/Saved/DetailTiles/route'
idx=json.load(open(D+'/tiles.json'))
names=[n for n in open('C:/Users/danji/AppData/Local/Temp/ygg/batch.txt').read().split() if n in idx]
at=unreal.AssetToolsHelpers.get_asset_tools(); ss=unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
mat=unreal.load_asset('/Game/Materials/M_EverestDetailTile')
for name in names:
    ts=[]
    for suf in ('','_col'):
        t=unreal.AssetImportTask(); t.filename=f'{D}/{name}{suf}.obj'; t.destination_path='/Game/Environment/DetailTiles/Route'
        t.destination_name=f'SM_{name}{suf}'; t.automated=True; t.save=False; t.replace_existing=True; ts.append(t)
    at.import_asset_tasks(ts)
    m=unreal.load_asset(f'/Game/Environment/DetailTiles/Route/SM_{name}')
    ns=m.get_editor_property('nanite_settings'); ns.enabled=True; ss.set_nanite_settings(m,ns,True)
    # the collision mesh must NOT be Nanite: complex collision is cooked from its LOD0, which for a Nanite mesh is the
    # coarse fallback (150 verts per tile -> 0.3 m median / 1.5 m max error). Non-Nanite: 1 mm median.
    c=unreal.load_asset(f'/Game/Environment/DetailTiles/Route/SM_{name}_col')
    cs=c.get_editor_property('nanite_settings'); cs.enabled=False; ss.set_nanite_settings(c,cs,True)
    m.get_editor_property('body_setup').set_editor_property('collision_trace_flag',unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE)
    m.set_material(0,mat)
print('imported',len(names))
