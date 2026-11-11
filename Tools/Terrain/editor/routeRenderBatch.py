# Re-imports the render meshes of a batch of route tiles (names in %TEMP%/ygg/batch.txt) and their route-aware
# collision (Saved/DetailTiles/route_col, as colswap): Nanite, tile material, complex collision from SM_<tile>_col.
import unreal
D = 'F:/UE_project/YggdrasilsPeak/Saved/DetailTiles/route'; DC = 'F:/UE_project/YggdrasilsPeak/Saved/DetailTiles/route_col'
names = open('C:/Users/danji/AppData/Local/Temp/ygg/batch.txt').read().split()
at = unreal.AssetToolsHelpers.get_asset_tools(); ss = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
mat = unreal.load_asset('/Game/Materials/M_EverestDetailTile')
ts = []
for name in names:
    for f, dest in ((f'{D}/{name}.obj', f'SM_{name}'), (f'{DC}/{name}_col.obj', f'SM_{name}_col')):
        t = unreal.AssetImportTask(); t.filename = f; t.destination_path = '/Game/Environment/DetailTiles/Route'
        t.destination_name = dest; t.automated = True; t.save = False; t.replace_existing = True; ts.append(t)
at.import_asset_tasks(ts)
for name in names:
    m = unreal.load_asset(f'/Game/Environment/DetailTiles/Route/SM_{name}'); c = unreal.load_asset(f'/Game/Environment/DetailTiles/Route/SM_{name}_col')
    ns = m.get_editor_property('nanite_settings'); ns.enabled = True; ss.set_nanite_settings(m, ns, True)
    cs = c.get_editor_property('nanite_settings')
    if cs.enabled: cs.enabled = False; ss.set_nanite_settings(c, cs, True)
    c.set_editor_property('has_navigation_data', False); m.set_editor_property('has_navigation_data', False)
    sm = list(m.static_materials)
    if len(sm) > 1: m.set_editor_property('static_materials', sm[:1])
    m.set_material(0, mat)
    m.get_editor_property('body_setup').set_editor_property('collision_trace_flag', unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE)
    m.set_editor_property('complex_collision_mesh', None); m.set_editor_property('complex_collision_mesh', c)
    unreal.EditorAssetLibrary.save_loaded_asset(m, False); unreal.EditorAssetLibrary.save_loaded_asset(c, False)
print('imported', len(names))
