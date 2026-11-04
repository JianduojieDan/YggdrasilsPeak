# Runs INSIDE the editor: M_LightShaft (additive unlit) from Tools/Materials/M_LightShaft.hlsl -> /Game/Environment/Realms
import unreal
mel = unreal.MaterialEditingLibrary; at = unreal.AssetToolsHelpers.get_asset_tools()
D = "/Game/Environment/Realms"
m = unreal.load_asset(f"{D}/M_LightShaft") or at.create_asset("M_LightShaft", D, unreal.Material, unreal.MaterialFactoryNew())
mel.delete_all_material_expressions(m)
m.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
m.set_editor_property("blend_mode", unreal.BlendMode.BLEND_ADDITIVE)
m.set_editor_property("two_sided", True)
c = mel.create_material_expression(m, unreal.MaterialExpressionCustom, -400, 0)
c.set_editor_property("code", open("F:/UE_project/YggdrasilsPeak/Tools/Materials/M_LightShaft.hlsl").read())
c.set_editor_property("output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT3)
ins = []
for n in ["UV", "V", "T", "Bright"]:
    i = unreal.CustomInput(); i.set_editor_property("input_name", n); ins.append(i)
c.set_editor_property("inputs", ins)
uv = mel.create_material_expression(m, unreal.MaterialExpressionTextureCoordinate, -800, -100)
# facing = |dot(view, normal)| computed in HLSL from the camera vector + vertex normal (Fresnel had a hard rim)
cv = mel.create_material_expression(m, unreal.MaterialExpressionCameraVectorWS, -800, 0)
vn = mel.create_material_expression(m, unreal.MaterialExpressionVertexNormalWS, -800, 50)
dp = mel.create_material_expression(m, unreal.MaterialExpressionDotProduct, -650, 0)
mel.connect_material_expressions(cv, "", dp, "A"); mel.connect_material_expressions(vn, "", dp, "B")
inv = mel.create_material_expression(m, unreal.MaterialExpressionAbs, -550, 0)
mel.connect_material_expressions(dp, "", inv, "")
t = mel.create_material_expression(m, unreal.MaterialExpressionTime, -800, 100)
b = mel.create_material_expression(m, unreal.MaterialExpressionScalarParameter, -800, 200)
b.set_editor_property("parameter_name", "Bright"); b.set_editor_property("default_value", 4.5)
for e, n in [(uv, "UV"), (inv, "V"), (t, "T"), (b, "Bright")]:
    mel.connect_material_expressions(e, "", c, n)
mel.connect_material_property(c, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
mel.recompile_material(m); unreal.EditorAssetLibrary.save_loaded_asset(m)
print("shaft material ok")
