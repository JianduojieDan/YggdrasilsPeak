# Runs INSIDE the editor: builds M_FarTerrain (+ MI_FarRing, MI_FarBackground) from Tools/Materials/M_FarTerrain.hlsl
import unreal
mel = unreal.MaterialEditingLibrary; at = unreal.AssetToolsHelpers.get_asset_tools()
P = "/Game/Environment/Distant/Far"
code = open("F:/UE_project/YggdrasilsPeak/Tools/Materials/M_FarTerrain.hlsl").read()
m = unreal.load_asset(f"{P}/M_FarTerrain") or at.create_asset("M_FarTerrain", P, unreal.Material, unreal.MaterialFactoryNew())
mel.delete_all_material_expressions(m)
m.set_editor_property("tangent_space_normal", False); m.set_editor_property("two_sided", True)
c = mel.create_material_expression(m, unreal.MaterialExpressionCustom, -400, 0)
c.set_editor_property("code", code)
c.set_editor_property("output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT3)
def ci(n):
    i = unreal.CustomInput(); i.set_editor_property("input_name", n); return i
names = ["WP", "Tex", "Origin", "Size", "Centre", "RockCut"]
c.set_editor_property("inputs", [ci(n) for n in names])
def co(n, t):
    o = unreal.CustomOutput(); o.set_editor_property("output_name", n); o.set_editor_property("output_type", t); return o
c.set_editor_property("additional_outputs", [co("Rough", unreal.CustomMaterialOutputType.CMOT_FLOAT1), co("Nrm", unreal.CustomMaterialOutputType.CMOT_FLOAT3)])
wp = mel.create_material_expression(m, unreal.MaterialExpressionWorldPosition, -800, -200)
tx = mel.create_material_expression(m, unreal.MaterialExpressionTextureObjectParameter, -800, -100)
tx.set_editor_property("parameter_name", "NormalMap")
tx.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)
tx.set_editor_property("texture", unreal.load_asset(f"{P}/T_FarRing_N"))
def vec(name, v, y):
    e = mel.create_material_expression(m, unreal.MaterialExpressionVectorParameter, -800, y)
    e.set_editor_property("parameter_name", name); e.set_editor_property("default_value", unreal.LinearColor(*v)); return e
def sc(name, v, y):
    e = mel.create_material_expression(m, unreal.MaterialExpressionScalarParameter, -800, y)
    e.set_editor_property("parameter_name", name); e.set_editor_property("default_value", v); return e
org = vec("Origin", (1196616.96 - 7.5e6, 1196616.96 - 7.5e6, 0, 0), 0)
siz = sc("Size", 1.5e7, 100)
cen = vec("Centre", (1196616.96, 1196616.96, 0, 0), 200)
rc = sc("RockCut", 0.90, 300)
# masks: Origin/Centre as float2
def mask2(e, y):
    k = mel.create_material_expression(m, unreal.MaterialExpressionComponentMask, -600, y)
    k.set_editor_property("r", True); k.set_editor_property("g", True); k.set_editor_property("b", False); k.set_editor_property("a", False)
    mel.connect_material_expressions(e, "", k, ""); return k
mel.connect_material_expressions(wp, "", c, "WP")
mel.connect_material_expressions(tx, "", c, "Tex")
mel.connect_material_expressions(mask2(org, 0), "", c, "Origin")
mel.connect_material_expressions(siz, "", c, "Size")
mel.connect_material_expressions(mask2(cen, 200), "", c, "Centre")
mel.connect_material_expressions(rc, "", c, "RockCut")
mel.connect_material_property(c, "", unreal.MaterialProperty.MP_BASE_COLOR)
mel.connect_material_property(c, "Rough", unreal.MaterialProperty.MP_ROUGHNESS)
mel.connect_material_property(c, "Nrm", unreal.MaterialProperty.MP_NORMAL)
mel.recompile_material(m)
unreal.EditorAssetLibrary.save_loaded_asset(m)
for n, half, tex in [("FarRing", 7.5e6, "T_FarRing_N"), ("FarBackground", 3.3e7, "T_FarBackground_N")]:
    mi = unreal.load_asset(f"{P}/MI_{n}") or at.create_asset(f"MI_{n}", P, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    mi.set_editor_property("parent", m)
    mel.set_material_instance_vector_parameter_value(mi, "Origin", unreal.LinearColor(1196616.96 - half, 1196616.96 - half, 0, 0))
    mel.set_material_instance_scalar_parameter_value(mi, "Size", 2 * half)
    mel.set_material_instance_texture_parameter_value(mi, "NormalMap", unreal.load_asset(f"{P}/{tex}"))
    unreal.EditorAssetLibrary.save_loaded_asset(mi)
    sm = unreal.load_asset(f"{P}/{n}"); sm.set_material(0, mi); unreal.EditorAssetLibrary.save_loaded_asset(sm)
print("ok")
