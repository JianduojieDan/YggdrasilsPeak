# Runs INSIDE the editor (Tools/RoutePlanning/uepy): puts Tools/Materials/M_RockSnow_Custom.hlsl into the Custom node of
# /Game/Environment/RockSnow/M_RockSnow (keeps its inputs and wiring), recompiles and saves.
import unreal
m = unreal.load_asset('/Game/Environment/RockSnow/M_RockSnow')
code = open('F:/UE_project/YggdrasilsPeak/Tools/Materials/M_RockSnow_Custom.hlsl', encoding='utf-8').read()
n = 0
for obj in unreal.ObjectIterator(unreal.MaterialExpressionCustom):
    try:
        owner = obj.get_typed_outer(unreal.Material)
    except Exception:
        owner = None
    if owner == m:
        obj.set_editor_property('code', code)
        n += 1
unreal.MaterialEditingLibrary.recompile_material(m)
unreal.EditorAssetLibrary.save_loaded_asset(m)
print('custom nodes updated:', n)
