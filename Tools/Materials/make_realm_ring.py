# Runs INSIDE the editor: M_RealmRing (translucent unlit, no fog) -> /Game/Environment/Realms
import unreal
mel = unreal.MaterialEditingLibrary; at = unreal.AssetToolsHelpers.get_asset_tools(); D = "/Game/Environment/Realms"
m = unreal.load_asset(f"{D}/M_RealmRing") or at.create_asset("M_RealmRing", D, unreal.Material, unreal.MaterialFactoryNew())
mel.delete_all_material_expressions(m)
m.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
m.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT); m.set_editor_property("two_sided", True)
# clouds over the rings too (2026-11-13: the planets showed in front of the clouds): per-pixel fog + cloud fogging
m.set_editor_property("use_translucency_vertex_fog", True); m.set_editor_property("compute_fog_per_pixel", True); m.set_editor_property("apply_cloud_fogging", True)
c = mel.create_material_expression(m, unreal.MaterialExpressionCustom, -500, 0)
c.set_editor_property("code", open("F:/UE_project/YggdrasilsPeak/Tools/Materials/M_RealmRing.hlsl").read())
c.set_editor_property("output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT4)
ins = []
for n in ["UV", "WP", "Ctr", "Cam", "R", "Bright", "Fade"]:
    i = unreal.CustomInput(); i.set_editor_property("input_name", n); ins.append(i)
c.set_editor_property("inputs", ins)
def s(n, v):
    e = mel.create_material_expression(m, unreal.MaterialExpressionScalarParameter, -900, 0); e.set_editor_property("parameter_name", n); e.set_editor_property("default_value", v); return e
for e, n in [(mel.create_material_expression(m, unreal.MaterialExpressionTextureCoordinate, -900, 0), "UV"), (mel.create_material_expression(m, unreal.MaterialExpressionWorldPosition, -900, 0), "WP"),
             (mel.create_material_expression(m, unreal.MaterialExpressionObjectPositionWS, -900, 0), "Ctr"), (mel.create_material_expression(m, unreal.MaterialExpressionCameraPositionWS, -900, 0), "Cam"),
             (s("PlanetRadius", 1.0), "R"), (s("Bright", 2.0), "Bright"), (s("Fade", 1.0), "Fade")]:
    mel.connect_material_expressions(e, "", c, n)
rgb = mel.create_material_expression(m, unreal.MaterialExpressionComponentMask, -200, 0)
for k in "rgb": rgb.set_editor_property(k, True)
a = mel.create_material_expression(m, unreal.MaterialExpressionComponentMask, -200, 100); a.set_editor_property("a", True)
mel.connect_material_expressions(c, "", rgb, ""); mel.connect_material_expressions(c, "", a, "")
mel.connect_material_property(rgb, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR); mel.connect_material_property(a, "", unreal.MaterialProperty.MP_OPACITY)
mel.recompile_material(m); unreal.EditorAssetLibrary.save_loaded_asset(m); print("ring ok")
