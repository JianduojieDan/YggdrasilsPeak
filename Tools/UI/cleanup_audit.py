# Clean-up audit (2026-11-14), run in the editor through Tools/RoutePlanning/uepy (remote Python).
# Lists every asset under /Game/UI/StartMenu, /Game/UI/HUD/Icons6 and /Game/UI/HUD/Icons with who references it,
# and finds the user's pink spawn cube in the open level. Read-only.
import unreal

reg = unreal.AssetRegistryHelpers.get_asset_registry()
opts = unreal.AssetRegistryDependencyOptions(include_soft_package_references=True, include_hard_package_references=True,
                                             include_searchable_names=False, include_soft_management_references=False,
                                             include_hard_management_references=False)
for folder in ['/Game/UI/StartMenu', '/Game/UI/HUD/Icons6', '/Game/UI/HUD/Icons']:
    print('=== ' + folder)
    assets = reg.get_assets_by_path(folder, recursive=True)
    for a in sorted(assets, key=lambda x: str(x.package_name)):
        pkg = str(a.package_name)
        refs = reg.get_referencers(pkg, opts) or []
        refs = [str(r) for r in refs if str(r) != pkg]
        print('%s [%s] <- %s' % (pkg, a.asset_class_path.asset_name, ', '.join(refs) if refs else '(nothing)'))

print('=== cube candidates in the open level')
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
for a in eas.get_all_level_actors():
    lab = a.get_actor_label()
    if 'cube' in lab.lower() or 'spawn' in lab.lower() or 'marker' in lab.lower():
        comps = a.get_components_by_class(unreal.StaticMeshComponent)
        mesh = comps[0].static_mesh.get_path_name() if comps and comps[0].static_mesh else '-'
        mats = [m.get_path_name() if m else '-' for m in (comps[0].get_materials() if comps else [])]
        print(lab, a.get_class().get_name(), a.get_actor_location(), mesh, mats, a.tags)
