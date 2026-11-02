# Runs INSIDE the editor (uepy.py). M_Ghost: the parents' ghosts (AParentsGhosts) - unlit, translucent, a cold pale
# rim (fresnel) with a faint body, all scaled by the "Fade" parameter.
import unreal
MEL = unreal.MaterialEditingLibrary
at = unreal.AssetToolsHelpers.get_asset_tools()
D = "/Game/Environment/Story"
P = f"{D}/M_Ghost"
mat = unreal.load_asset(P) or at.create_asset("M_Ghost", D, unreal.Material, unreal.MaterialFactoryNew())
MEL.delete_all_material_expressions(mat)
mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
mat.set_editor_property("two_sided", False)
mat.set_editor_property("used_with_skeletal_mesh", True)   # it is put on the climber model
fr = MEL.create_material_expression(mat, unreal.MaterialExpressionFresnel, -700, 0)
fr.set_editor_property("exponent", 2.5)
fade = MEL.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -700, 250)
fade.set_editor_property("parameter_name", "Fade"); fade.set_editor_property("default_value", 1.0)
cu = MEL.create_material_expression(mat, unreal.MaterialExpressionCustom, -400, 100)
cu.set_editor_property("output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT3)
ins = []
for n in ("Rim", "Fade"):
    ci = unreal.CustomInput(); ci.set_editor_property("input_name", n); ins.append(ci)
cu.set_editor_property("inputs", ins)
co = unreal.CustomOutput(); co.set_editor_property("output_name", "Alpha"); co.set_editor_property("output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT1)
cu.set_editor_property("additional_outputs", [co])
cu.set_editor_property("code", """
Alpha = saturate(0.1 + Rim * 0.6) * Fade;
return float3(0.55, 0.7, 0.9) * (0.35 + Rim * 1.6) * Fade;
""")
MEL.connect_material_expressions(fr, "", cu, "Rim"); MEL.connect_material_expressions(fade, "", cu, "Fade")
MEL.connect_material_property(cu, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
MEL.connect_material_property(cu, "Alpha", unreal.MaterialProperty.MP_OPACITY)
MEL.recompile_material(mat); unreal.EditorAssetLibrary.save_loaded_asset(mat, False)
print("ghost material built")
