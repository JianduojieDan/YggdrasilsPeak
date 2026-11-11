# Runs INSIDE the editor (uepy.py): puts the Glacier node sources into M_EverestLandscape and M_EverestDetailTile
# (MaterialExpressionCustom_5; inputs unchanged) and recompiles. Sources: Tools/Materials/M_EverestLandscape_Glacier.hlsl
# and M_EverestDetailTile_Glacier.hlsl (see glacier_rewrite_2026_11_11.py).
import unreal
MEL = unreal.MaterialEditingLibrary
R = "F:/UE_project/YggdrasilsPeak/Tools/Materials"
for path, src in [("/Game/Materials/M_EverestLandscape", "M_EverestLandscape_Glacier.hlsl"), ("/Game/Materials/M_EverestDetailTile", "M_EverestDetailTile_Glacier.hlsl")]:
    m = unreal.load_asset(path)
    ex = {e.get_name(): e for e in unreal.ObjectIterator(unreal.MaterialExpression) if e.get_outer() == m}
    node = ex["MaterialExpressionCustom_5"]
    assert node.get_editor_property("description") == "Glacier"
    node.set_editor_property("code", open(f"{R}/{src}", encoding="utf8").read())
    unreal.log_warning("GLACIER_PUSH " + path)
    MEL.recompile_material(m)
    unreal.EditorAssetLibrary.save_loaded_asset(m, False)
    print("pushed", path)
