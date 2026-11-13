# Runs INSIDE the editor (Tools/RoutePlanning/uepy): builds /Game/UI/Materials/M_PP_Body from Tools/UI/shaders/pp_body
# (the body on the view: thin air, heartbeat, frost, exhaustion, near collapse), replacing an earlier build in place.
import unreal

MEL = unreal.MaterialEditingLibrary
ROOT = "F:/UE_project/YggdrasilsPeak/Tools/UI/shaders/"
DIR = "/Game/UI/Materials"
NAME = "M_PP_Body"
path = f"{DIR}/{NAME}"
if unreal.EditorAssetLibrary.does_asset_exist(path):
    m = unreal.load_asset(path)
    MEL.delete_all_material_expressions(m)
else:
    m = unreal.AssetToolsHelpers.get_asset_tools().create_asset(NAME, DIR, unreal.Material, unreal.MaterialFactoryNew())
m.set_editor_property("material_domain", unreal.MaterialDomain.MD_POST_PROCESS)
m.set_editor_property("blendable_location", unreal.BlendableLocation.BL_SCENE_COLOR_AFTER_TONEMAPPING)


def scalar(name, value, y):
    e = MEL.create_material_expression(m, unreal.MaterialExpressionScalarParameter, -900, y)
    e.set_editor_property("parameter_name", name)
    e.set_editor_property("default_value", value)
    return e


sp = MEL.create_material_expression(m, unreal.MaterialExpressionScreenPosition, -900, 0)
st = MEL.create_material_expression(m, unreal.MaterialExpressionSceneTexture, -900, 120)
st.set_editor_property("scene_texture_id", unreal.SceneTextureId.PPI_POST_PROCESS_INPUT0)
ins = {"UV": (sp, "ViewportUV"), "SceneTex": (st, "Color")}
for i, (n, v) in enumerate([("T", 0.0), ("Aspect", 1.7778), ("Hyp", 0.0), ("Beat", 0.0), ("Frost", 0.0), ("Exhaust", 0.0), ("Faint", 0.0)]):
    ins[n] = (scalar(n, v, 240 + i * 100), "")
c = MEL.create_material_expression(m, unreal.MaterialExpressionCustom, -450, 200)
c.set_editor_property("code", open(ROOT + "pp_body.hlsl", encoding="utf-8").read())
c.set_editor_property("output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT3)
c.set_editor_property("description", "pp_body.hlsl")
cins = []
for n in ins:
    ci = unreal.CustomInput()
    ci.set_editor_property("input_name", n)
    cins.append(ci)
c.set_editor_property("inputs", cins)
for k, (e, out) in ins.items():
    MEL.connect_material_expressions(e, out, c, k)
MEL.connect_material_property(c, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
MEL.recompile_material(m)
unreal.EditorAssetLibrary.save_loaded_asset(m)
print("M_PP_Body built")
