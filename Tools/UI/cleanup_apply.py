# Clean-up (2026-11-14, user approved): run in the editor through Tools/RoutePlanning/uepy (remote Python).
# 1. base_map: the pink spawn cube goes; the PlayerStart (13.5 m up on top of it) is set onto the ground.
# 2. The old StartMenu folder: what the game still uses moves to /Game/UI/Menu (start map, game instance, loading
#    arrival + its widget, backdrops); the old Blueprint menu goes (WBP_StartMenu, BP_MenuBootstrap and its actor,
#    the photo and the font only it used, an unused material and game mode). Config and C++ paths are updated outside.
# 3. The unused HUD v6 icons go (halos, mountain, snow, summit, tree, wind); the six the HUD draws stay.
import unreal

EAL = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)

# ---- 1. base_map
acts = {a.get_actor_label(): a for a in eas.get_all_level_actors()}
cube, ps = acts.get('Cube'), acts.get('PlayerStart')
if cube:
    eas.destroy_actor(cube)
    print('cube removed')
if ps:
    loc = ps.get_actor_location()
    ps.set_actor_location(unreal.Vector(loc.x, loc.y, -69243.1 + 110.0), False, False)
    print('PlayerStart moved to', ps.get_actor_location())
print('base_map saved', les.save_current_level())

# ---- 2. move what is live
moves = [
    ('/Game/UI/StartMenu/L_StartMenu', '/Game/UI/Menu'),
    ('/Game/UI/StartMenu/BP_MenuInstance', '/Game/UI/Menu'),
    ('/Game/UI/StartMenu/BP_LoadingArrival', '/Game/UI/Menu'),
    ('/Game/UI/StartMenu/WBP_LoadingReveal', '/Game/UI/Menu'),
] + [('/Game/UI/StartMenu/Backdrops/T_Backdrop_%02d' % i, '/Game/UI/Menu/Backdrops') for i in range(1, 5)]
data = []
for src, dst in moves:
    obj = EAL.load_asset(src)
    if not obj:
        print('MISSING', src); continue
    data.append(unreal.AssetRenameData(obj, dst, src.split('/')[-1]))
print('renamed', tools.rename_assets(data))

# the old bootstrap actor lives in the start map: open it, remove the actor, save
les.load_level('/Game/UI/Menu/L_StartMenu')
for a in eas.get_all_level_actors():
    if 'MenuBootstrap' in a.get_class().get_name():
        print('removing', a.get_actor_label()); eas.destroy_actor(a)
print('L_StartMenu saved', les.save_current_level())

# redirectors left behind by the moves: point every referencer at the new place
reg = unreal.AssetRegistryHelpers.get_asset_registry()
reds = [EAL.load_asset(str(a.package_name)) for a in reg.get_assets_by_path('/Game/UI/StartMenu', recursive=True)
        if str(a.asset_class_path.asset_name) == 'ObjectRedirector']
reds = [r for r in reds if r]
if reds:
    tools.fixup_referencers(reds)
    print('redirectors fixed', len(reds))

# ---- the dead old menu
for p in ['/Game/UI/StartMenu/WBP_StartMenu', '/Game/UI/StartMenu/BP_MenuBootstrap', '/Game/UI/StartMenu/BP_MenuGameMode',
          '/Game/UI/StartMenu/M_FrostLandscape', '/Game/UI/StartMenu/weichao-deng-eyn0LjpNWV4-unsplash',
          '/Game/UI/StartMenu/Fonts/F_Isometra', '/Game/UI/StartMenu/Fonts/Isometra']:
    print('delete', p, EAL.delete_asset(p) if EAL.does_asset_exist(p) else 'not there')
left = [str(a.package_name) for a in reg.get_assets_by_path('/Game/UI/StartMenu', recursive=True)]
print('left in StartMenu:', left)
if not left:
    print('folder deleted', EAL.delete_directory('/Game/UI/StartMenu'))

# ---- 3. unused HUD v6 icons
keep = {'Drop', 'Food', 'Heart', 'Lungs', 'Thermo', 'Tent'}
for a in reg.get_assets_by_path('/Game/UI/HUD/Icons6', recursive=False):
    name = str(a.asset_name)
    if name.replace('T_HUD6_', '') in keep:
        continue
    print('delete', name, EAL.delete_asset(str(a.package_name)))

les.load_level('/Game/LevelPrototyping/base_map')
print('done')
