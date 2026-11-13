# Runs INSIDE the editor (Tools/RoutePlanning/uepy.py). Builds the UI redesign's materials (2026-11-13) from the HLSL
# in Tools/UI/shaders, replacing earlier builds in place (same asset paths, so code that loads them by path keeps working):
#   /Game/UI/Materials/M_UI_CloudVeil  - UI domain: drifting cloud shadow behind the menus
#   /Game/UI/Materials/M_PP_Fate       - post process: the world behind a death screen turns into what took you
#   /Game/UI/Materials/M_UI_Cloth      - UI domain: the pack screen's canvas tarp in a pool of lamplight
import unreal

MEL = unreal.MaterialEditingLibrary
ROOT = "F:/UE_project/YggdrasilsPeak/Tools/UI/shaders/"
DIR = "/Game/UI/Materials"


def fresh(name):
    path = f"{DIR}/{name}"
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        m = unreal.load_asset(path)
        MEL.delete_all_material_expressions(m)
        return m
    return unreal.AssetToolsHelpers.get_asset_tools().create_asset(name, DIR, unreal.Material, unreal.MaterialFactoryNew())


def scalar(m, name, value, x, y):
    e = MEL.create_material_expression(m, unreal.MaterialExpressionScalarParameter, x, y)
    e.set_editor_property("parameter_name", name)
    e.set_editor_property("default_value", value)
    return e


def vector(m, name, value, x, y):
    e = MEL.create_material_expression(m, unreal.MaterialExpressionVectorParameter, x, y)
    e.set_editor_property("parameter_name", name)
    e.set_editor_property("default_value", unreal.LinearColor(*value))
    return e


def custom(m, file, inputs, out_type, x, y):
    e = MEL.create_material_expression(m, unreal.MaterialExpressionCustom, x, y)
    e.set_editor_property("code", open(ROOT + file, encoding="utf-8").read())
    e.set_editor_property("output_type", out_type)
    e.set_editor_property("description", file)
    ins = []
    for n in inputs:
        ci = unreal.CustomInput()
        ci.set_editor_property("input_name", n)
        ins.append(ci)
    e.set_editor_property("inputs", ins)
    return e


def mask(m, src, r, g, b, a, x, y):
    e = MEL.create_material_expression(m, unreal.MaterialExpressionComponentMask, x, y)
    for k, v in zip("rgba", (r, g, b, a)):
        e.set_editor_property(k, v)
    MEL.connect_material_expressions(src, "", e, "")
    return e


# ---- M_UI_CloudVeil
m = fresh("M_UI_CloudVeil")
m.set_editor_property("material_domain", unreal.MaterialDomain.MD_UI)
m.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
uvn = MEL.create_material_expression(m, unreal.MaterialExpressionTextureCoordinate, -900, 0)
ins = {
    "UV": uvn,
    "T": scalar(m, "T", 0.0, -900, 100),
    "Aspect": scalar(m, "Aspect", 1.7778, -900, 200),
    "Left": scalar(m, "Left", 0.82, -900, 300),
    "Right": scalar(m, "Right", 0.22, -900, 400),
    "Drift": scalar(m, "Drift", 0.16, -900, 500),
    "Ink": vector(m, "Ink", (0.003, 0.0055, 0.0125, 1), -900, 600),
    "Light": vector(m, "Light", (0.30, 0.36, 0.45, 1), -900, 800),
}
c = custom(m, "cloud_veil.hlsl", list(ins.keys()), unreal.CustomMaterialOutputType.CMOT_FLOAT4, -500, 200)
for k, e in ins.items():
    MEL.connect_material_expressions(e, "", c, k)
MEL.connect_material_property(mask(m, c, True, True, True, False, -250, 150), "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
MEL.connect_material_property(mask(m, c, False, False, False, True, -250, 300), "", unreal.MaterialProperty.MP_OPACITY)
MEL.recompile_material(m)
unreal.EditorAssetLibrary.save_loaded_asset(m)
print("M_UI_CloudVeil built")

# ---- M_PP_Fate
m = fresh("M_PP_Fate")
m.set_editor_property("material_domain", unreal.MaterialDomain.MD_POST_PROCESS)
m.set_editor_property("blendable_location", unreal.BlendableLocation.BL_SCENE_COLOR_AFTER_TONEMAPPING)
sp = MEL.create_material_expression(m, unreal.MaterialExpressionScreenPosition, -900, 0)
st = MEL.create_material_expression(m, unreal.MaterialExpressionSceneTexture, -900, 150)
st.set_editor_property("scene_texture_id", unreal.SceneTextureId.PPI_POST_PROCESS_INPUT0)
ins = {
    "UV": (sp, "ViewportUV"),
    "SceneTex": (st, "Color"),
    "Fate": (scalar(m, "Fate", 1.0, -900, 300), ""),
    "T": (scalar(m, "T", 2.0, -900, 400), ""),
    "Amount": (scalar(m, "Amount", 1.0, -900, 500), ""),
    "Aspect": (scalar(m, "Aspect", 1.7778, -900, 600), ""),
}
c = custom(m, "pp_fate.hlsl", list(ins.keys()), unreal.CustomMaterialOutputType.CMOT_FLOAT3, -450, 200)
for k, (e, out) in ins.items():
    MEL.connect_material_expressions(e, out, c, k)
MEL.connect_material_property(c, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
MEL.recompile_material(m)
unreal.EditorAssetLibrary.save_loaded_asset(m)
print("M_PP_Fate built")

# ---- M_UI_Cloth
m = fresh("M_UI_Cloth")
m.set_editor_property("material_domain", unreal.MaterialDomain.MD_UI)
m.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
uvn = MEL.create_material_expression(m, unreal.MaterialExpressionTextureCoordinate, -900, 0)
ins = {"UV": uvn, "Aspect": scalar(m, "Aspect", 1.7778, -900, 100), "Light": scalar(m, "Light", 1.0, -900, 200)}
c = custom(m, "cloth.hlsl", list(ins.keys()), unreal.CustomMaterialOutputType.CMOT_FLOAT4, -500, 100)
for k, e in ins.items():
    MEL.connect_material_expressions(e, "", c, k)
MEL.connect_material_property(mask(m, c, True, True, True, False, -250, 50), "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
MEL.connect_material_property(mask(m, c, False, False, False, True, -250, 200), "", unreal.MaterialProperty.MP_OPACITY)
MEL.recompile_material(m)
unreal.EditorAssetLibrary.save_loaded_asset(m)
print("M_UI_Cloth built")