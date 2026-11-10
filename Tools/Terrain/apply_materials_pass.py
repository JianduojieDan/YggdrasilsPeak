# Runs inside the editor. Pushes the snow, glacier and tent shader files into their Custom nodes.
import unreal
L = unreal.MaterialEditingLibrary
T = "F:/UE_project/YggdrasilsPeak/Tools/Materials/"
m = unreal.load_asset("/Game/Materials/M_EverestLandscape")
for e in L.get_material_expressions(m):
    n = e.get_name()
    f = {"MaterialExpressionCustom_0": "M_EverestLandscape_Custom0.hlsl", "MaterialExpressionCustom_5": "M_EverestLandscape_Glacier.hlsl"}.get(n)
    if f:
        e.set_editor_property("code", open(T + f, encoding="utf-8", newline="").read())
        print("set", n)
L.recompile_material(m)
unreal.EditorAssetLibrary.save_loaded_asset(m, False)
ar = unreal.AssetRegistryHelpers.get_asset_registry()
c = unreal.load_asset("/Game/Environment/Camps/M_CampCanvas")
for e in L.get_material_expressions(c):
    if e.get_class().get_name() == "MaterialExpressionCustom":
        e.set_editor_property("code", open(T + "M_CampCanvas.hlsl", encoding="utf-8", newline="").read())
        print("canvas set")
L.recompile_material(c)
unreal.EditorAssetLibrary.save_loaded_asset(c, False)
print("done")
