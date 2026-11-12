# Runs INSIDE the editor (uepy.py): /Game/Environment/Realms/M_LightningBolt for ALightningBolt (hand-drawn strikes).
# Unlit, additive, two-sided. Emissive = Color * Glow * VertexColor.R * a soft core across the ribbon (UV.y).
import unreal
MEL = unreal.MaterialEditingLibrary
AT = unreal.AssetToolsHelpers.get_asset_tools()
path, name = "/Game/Environment/Realms", "M_LightningBolt"
m = unreal.load_asset(f"{path}/{name}")
if m is None:
    m = AT.create_asset(name, path, unreal.Material, unreal.MaterialFactoryNew())
MEL.delete_all_material_expressions(m)
m.set_editor_property("blend_mode", unreal.BlendMode.BLEND_ADDITIVE)
m.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
m.set_editor_property("two_sided", True)
for prop, val in (("use_translucency_vertex_fog", False), ("apply_fogging", False), ("compute_fog_per_pixel", False), ("output_translucent_velocity", False)):
    try:
        m.set_editor_property(prop, val); print("set", prop, val)
    except Exception as e:
        print("no", prop)
X = lambda cls, x, y: MEL.create_material_expression(m, cls, x, y)
uv = X(unreal.MaterialExpressionTextureCoordinate, -1200, 0)
mask = X(unreal.MaterialExpressionComponentMask, -1000, 0)
mask.set_editor_property("r", False); mask.set_editor_property("g", True)
MEL.connect_material_expressions(uv, "", mask, "")
two = X(unreal.MaterialExpressionMultiply, -850, 0); two.set_editor_property("const_b", 2.0)
MEL.connect_material_expressions(mask, "", two, "A")
sub = X(unreal.MaterialExpressionSubtract, -700, 0); sub.set_editor_property("const_b", 1.0)
MEL.connect_material_expressions(two, "", sub, "A")
ab = X(unreal.MaterialExpressionAbs, -560, 0)
MEL.connect_material_expressions(sub, "", ab, "")
om = X(unreal.MaterialExpressionOneMinus, -430, 0)
MEL.connect_material_expressions(ab, "", om, "")
pw = X(unreal.MaterialExpressionPower, -300, 0); pw.set_editor_property("const_exponent", 2.2)
MEL.connect_material_expressions(om, "", pw, "Base")
vc = X(unreal.MaterialExpressionVertexColor, -560, 220)
m1 = X(unreal.MaterialExpressionMultiply, -150, 80)
MEL.connect_material_expressions(pw, "", m1, "A")
MEL.connect_material_expressions(vc, "R", m1, "B")
glow = X(unreal.MaterialExpressionScalarParameter, -300, 320); glow.set_editor_property("parameter_name", "Glow"); glow.set_editor_property("default_value", 60.0)
col = X(unreal.MaterialExpressionVectorParameter, -300, 440); col.set_editor_property("parameter_name", "Color"); col.set_editor_property("default_value", unreal.LinearColor(0.85, 0.92, 1.0, 1.0))
m2 = X(unreal.MaterialExpressionMultiply, 0, 200)
MEL.connect_material_expressions(glow, "", m2, "A")
MEL.connect_material_expressions(col, "", m2, "B")
m3 = X(unreal.MaterialExpressionMultiply, 150, 100)
MEL.connect_material_expressions(m1, "", m3, "A")
MEL.connect_material_expressions(m2, "", m3, "B")
MEL.connect_material_property(m3, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
MEL.recompile_material(m)
unreal.EditorAssetLibrary.save_loaded_asset(m, False)
print("made", m.get_path_name())
