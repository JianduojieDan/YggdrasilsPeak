# Runs INSIDE the editor (uepy.py). Puts M_EverestLandscape_TrailShade.hlsl into the TrailShade Custom node
# (MaterialExpressionCustom_3) of the landscape AND the detail tile material, with inputs Col, N, WP, Rock, Area,
# Trail, Mask, VN, and reconnects them all (rewriting a Custom node's input list drops its wires).
import unreal
MEL = unreal.MaterialEditingLibrary
ROOT = "F:/UE_project/YggdrasilsPeak"
code = open(f"{ROOT}/Tools/Materials/M_EverestLandscape_TrailShade.hlsl", encoding="utf8").read()
for path in ["/Game/Materials/M_EverestLandscape", "/Game/Materials/M_EverestDetailTile"]:
    m = unreal.load_asset(path)
    ex = {e.get_name(): e for e in unreal.ObjectIterator(unreal.MaterialExpression) if e.get_outer() == m}
    node = ex["MaterialExpressionCustom_3"]
    assert "Lighting and shade for the trail" in node.get_editor_property("code")
    mask = next(e for e in ex.values() if isinstance(e, unreal.MaterialExpressionTextureObject)
                and e.get_editor_property("texture") and e.get_editor_property("texture").get_name() == "T_GlacierMask")
    vn = next(e for n, e in ex.items() if n.startswith("MaterialExpressionVertexNormalWS"))
    ins = []
    for n in ["Col", "N", "WP", "Rock", "Area", "Trail", "Mask", "VN"]:
        ci = unreal.CustomInput(); ci.set_editor_property("input_name", n); ins.append(ci)
    node.set_editor_property("inputs", ins)
    node.set_editor_property("code", code)
    C = MEL.connect_material_expressions
    s0 = ex["MaterialExpressionCustom_0"]
    ok = [C(s0, "", node, "Col"), C(s0, "Normal", node, "N"), C(ex["MaterialExpressionWorldPosition_0"], "", node, "WP"),
          C(s0, "RockMask", node, "Rock"), C(ex["MaterialExpressionCollectionParameter_0"], "", node, "Area"),
          C(ex["MaterialExpressionTextureObjectParameter_7"], "", node, "Trail"), C(mask, "", node, "Mask"), C(vn, "", node, "VN")]
    print(path, "connections", ok)
    MEL.recompile_material(m)
    unreal.EditorAssetLibrary.save_loaded_asset(m, False)
