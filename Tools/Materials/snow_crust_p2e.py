# Runs INSIDE the editor (Tools/RoutePlanning/uepy). 2026-11-14, snow step P2e: the crust's near-feet boost (P2b,
# 1.6-1.9x the map's slopes) drew the plate edges as dark lines in shade, like dried mud. Eased to 1.25-1.45x.
import unreal
A = "lerp(lerp(0.09,0.30,hard),lerp(1.6,1.9,hard),nearK)"
B = "lerp(lerp(0.09,0.30,hard),lerp(1.25,1.45,hard),nearK)"
for path in ["/Game/Materials/M_EverestDetailTile", "/Game/Materials/M_EverestLandscape"]:
    m = unreal.load_asset(path)
    c = [e for e in unreal.ObjectIterator(unreal.MaterialExpressionCustom) if e.get_outer() == m and e.get_name() == "MaterialExpressionCustom_0"][0]
    code = c.get_editor_property("code")
    print(path, "count", code.count(A), "already", code.count(B))
    if code.count(A) == 1:
        c.set_editor_property("code", code.replace(A, B))
    unreal.MaterialEditingLibrary.recompile_material(m)
    unreal.EditorAssetLibrary.save_loaded_asset(m, False)
    print(path, "recompiled + saved")
