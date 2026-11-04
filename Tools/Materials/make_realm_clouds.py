# Runs INSIDE the editor: M_RealmClouds (translucent unlit, two-sided, no fog) -> /Game/Environment/Realms
import unreal
mel = unreal.MaterialEditingLibrary; at = unreal.AssetToolsHelpers.get_asset_tools(); D = "/Game/Environment/Realms"
m = unreal.load_asset(f"{D}/M_RealmClouds") or at.create_asset("M_RealmClouds", D, unreal.Material, unreal.MaterialFactoryNew())
mel.delete_all_material_expressions(m)
m.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
m.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT); m.set_editor_property("two_sided", True)
c = mel.create_material_expression(m, unreal.MaterialExpressionCustom, -500, 0)
c.set_editor_property("code", open("F:/UE_project/YggdrasilsPeak/Tools/Materials/M_RealmClouds.hlsl").read())
c.set_editor_property("output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT4)
names = ["WP", "Cam", "T", "Cover", "Speed", "Scale", "Dark", "Light"]; ins = []
for n in names:
    i = unreal.CustomInput(); i.set_editor_property("input_name", n); ins.append(i)
c.set_editor_property("inputs", ins)
def s(n, v):
    e = mel.create_material_expression(m, unreal.MaterialExpressionScalarParameter, -900, 0); e.set_editor_property("parameter_name", n); e.set_editor_property("default_value", v); return e
def v(n, x):
    e = mel.create_material_expression(m, unreal.MaterialExpressionVectorParameter, -900, 0); e.set_editor_property("parameter_name", n); e.set_editor_property("default_value", unreal.LinearColor(x, x, x, 1)); return e
for e, n in [(mel.create_material_expression(m, unreal.MaterialExpressionWorldPosition, -900, 0), "WP"), (mel.create_material_expression(m, unreal.MaterialExpressionCameraPositionWS, -900, 0), "Cam"),
             (mel.create_material_expression(m, unreal.MaterialExpressionTime, -900, 0), "T"), (s("Cover", 0.0), "Cover"), (s("Speed", 5.0), "Speed"), (s("Scale", 6.0), "Scale"), (v("Dark", 0.3), "Dark"), (v("Light", 1.0), "Light")]:
    mel.connect_material_expressions(e, "", c, n)
rgb = mel.create_material_expression(m, unreal.MaterialExpressionComponentMask, -200, 0)
for k in "rgb": rgb.set_editor_property(k, True)
a = mel.create_material_expression(m, unreal.MaterialExpressionComponentMask, -200, 100); a.set_editor_property("a", True)
mel.connect_material_expressions(c, "", rgb, ""); mel.connect_material_expressions(c, "", a, "")
mel.connect_material_property(rgb, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR); mel.connect_material_property(a, "", unreal.MaterialProperty.MP_OPACITY)
mel.recompile_material(m); unreal.EditorAssetLibrary.save_loaded_asset(m); print("clouds ok")
