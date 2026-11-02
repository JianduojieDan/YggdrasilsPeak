# Runs INSIDE the editor (uepy.py). M_HiddenCrevasseSag: the only warning of a hidden crevasse - a long, soft,
# slightly blue-grey sag in the snow (deferred decal; easy to miss if you are not looking for it).
import unreal
MEL = unreal.MaterialEditingLibrary
at = unreal.AssetToolsHelpers.get_asset_tools()
D = "/Game/Environment/Glacier/Crevasse"
P = f"{D}/M_HiddenCrevasseSag"
mat = unreal.load_asset(P) or at.create_asset("M_HiddenCrevasseSag", D, unreal.Material, unreal.MaterialFactoryNew())
MEL.delete_all_material_expressions(mat)
mat.set_editor_property("material_domain", unreal.MaterialDomain.MD_DEFERRED_DECAL)
mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
uv = MEL.create_material_expression(mat, unreal.MaterialExpressionTextureCoordinate, -700, 0)
cu = MEL.create_material_expression(mat, unreal.MaterialExpressionCustom, -400, 0)
cu.set_editor_property("output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT1)
ci = unreal.CustomInput(); ci.set_editor_property("input_name", "UV"); cu.set_editor_property("inputs", [ci])
cu.set_editor_property("code", """
// across the crevasse (one UV axis) a soft trough, along it (the other) fading at both ends, gently wavering
float across = abs(UV.y - 0.5 - 0.06 * sin(UV.x * 11.0) - 0.03 * sin(UV.x * 29.0)) * 2.0;
float trough = pow(saturate(1.0 - across), 2.2);
float ends = smoothstep(0.0, 0.15, UV.x) * smoothstep(1.0, 0.85, UV.x);
return trough * ends * 0.6;
""")
MEL.connect_material_expressions(uv, "", cu, "UV")
col = MEL.create_material_expression(mat, unreal.MaterialExpressionConstant3Vector, -400, 200)
col.set_editor_property("constant", unreal.LinearColor(0.42, 0.55, 0.76, 1))
MEL.connect_material_property(col, "", unreal.MaterialProperty.MP_BASE_COLOR)
MEL.connect_material_property(cu, "", unreal.MaterialProperty.MP_OPACITY)
MEL.recompile_material(mat); unreal.EditorAssetLibrary.save_loaded_asset(mat, False)
print("sag decal material built")
