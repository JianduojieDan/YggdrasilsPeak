# Runs INSIDE the editor (uepy.py). Puts M_EverestLandscape_TrailDisp.hlsl into the TrailDisp Custom node
# (MaterialExpressionCustom_2) with inputs Base, WP, Rock, Area, Trail, Mask, VN and reconnects all of them
# (rewriting a Custom node's input list drops its wires).
import unreal
MEL = unreal.MaterialEditingLibrary
ROOT = "F:/UE_project/YggdrasilsPeak"
m = unreal.load_asset("/Game/Materials/M_EverestLandscape")
ex = {e.get_name(): e for e in unreal.ObjectIterator(unreal.MaterialExpression) if e.get_outer() == m}
node = ex["MaterialExpressionCustom_2"]
mask = next(e for e in ex.values() if isinstance(e, unreal.MaterialExpressionTextureObject)
            and e.get_editor_property("texture") and e.get_editor_property("texture").get_name() == "T_GlacierMask")
names = ["Base", "WP", "Rock", "Area", "Trail", "Mask", "VN"]
ins = []
for n in names:
    ci = unreal.CustomInput(); ci.set_editor_property("input_name", n); ins.append(ci)
node.set_editor_property("inputs", ins)
node.set_editor_property("code", open(f"{ROOT}/Tools/Materials/M_EverestLandscape_TrailDisp.hlsl", encoding="utf8").read())
C = MEL.connect_material_expressions
ok = [C(ex["MaterialExpressionCustom_1"], "", node, "Base"),
      C(ex["MaterialExpressionWorldPosition_0"], "", node, "WP"),
      C(ex["MaterialExpressionCustom_1"], "RockMask", node, "Rock"),
      C(ex["MaterialExpressionCollectionParameter_0"], "", node, "Area"),
      C(ex["MaterialExpressionTextureObjectParameter_7"], "", node, "Trail"),
      C(mask, "", node, "Mask"),
      C(ex["MaterialExpressionVertexNormalWS_0"], "", node, "VN")]
print("connections", ok)
print("trail disp inputs now", [x.get_name() if x else None for x in MEL.get_inputs_for_material_expression(m, node)])
MEL.recompile_material(m)
unreal.EditorAssetLibrary.save_loaded_asset(m, False)
