# Runs INSIDE the editor (uepy.py). M_CrevasseIce for the crevasse lips (see M_CrevasseIce.hlsl): CrestZ comes from
# the component's pivot plus its scaled local crest height (630 cm, Tools/Blender/crevasse_lip.py), so it follows
# each lip's per-crossing scale.
import unreal
MEL = unreal.MaterialEditingLibrary
at = unreal.AssetToolsHelpers.get_asset_tools()
ROOT = "F:/UE_project/YggdrasilsPeak"
D = "/Game/Environment/Glacier/Crevasse"
S = "/Game/Environment/Glacier/Seracs"
P = f"{D}/M_CrevasseIce"
mat = unreal.load_asset(P) or at.create_asset("M_CrevasseIce", D, unreal.Material, unreal.MaterialFactoryNew())
MEL.delete_all_material_expressions(mat)
uv = MEL.create_material_expression(mat, unreal.MaterialExpressionTextureCoordinate, -1300, 100)
uv.set_editor_property("u_tiling", 0.55); uv.set_editor_property("v_tiling", 0.55)
def sample(tex, y, st):
    e = MEL.create_material_expression(mat, unreal.MaterialExpressionTextureSample, -1000, y)
    e.set_editor_property("texture", unreal.load_asset(tex)); e.set_editor_property("sampler_type", st)
    MEL.connect_material_expressions(uv, "", e, "UVs")
    return e
sc = sample(f"{S}/T_Ice004_D", 0, unreal.MaterialSamplerType.SAMPLERTYPE_COLOR)
sn = sample(f"{S}/T_Ice004_N", 300, unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)
vn = MEL.create_material_expression(mat, unreal.MaterialExpressionVertexNormalWS, -1000, 600)
wp = MEL.create_material_expression(mat, unreal.MaterialExpressionWorldPosition, -1000, 700)
op = MEL.create_material_expression(mat, unreal.MaterialExpressionObjectPositionWS, -1300, 800)
cv = MEL.create_material_expression(mat, unreal.MaterialExpressionConstant3Vector, -1500, 900)
cv.set_editor_property("constant", unreal.LinearColor(0, 0, 630, 0))
tv = MEL.create_material_expression(mat, unreal.MaterialExpressionTransform, -1300, 900)
tv.set_editor_property("transform_source_type", unreal.MaterialVectorCoordTransformSource.TRANSFORMSOURCE_LOCAL)
tv.set_editor_property("transform_type", unreal.MaterialVectorCoordTransform.TRANSFORM_WORLD)
MEL.connect_material_expressions(cv, "", tv, "")
add = MEL.create_material_expression(mat, unreal.MaterialExpressionAdd, -1100, 850)
MEL.connect_material_expressions(op, "", add, "A"); MEL.connect_material_expressions(tv, "", add, "B")
mz = MEL.create_material_expression(mat, unreal.MaterialExpressionComponentMask, -1000, 850)
mz.set_editor_property("r", False); mz.set_editor_property("g", False); mz.set_editor_property("b", True)
MEL.connect_material_expressions(add, "", mz, "")
cu = MEL.create_material_expression(mat, unreal.MaterialExpressionCustom, -500, 200)
cu.set_editor_property("output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT3)
ins = []
for n in ("Crack", "VN", "WP", "CrestZ"):
    ci = unreal.CustomInput(); ci.set_editor_property("input_name", n); ins.append(ci)
cu.set_editor_property("inputs", ins)
outs = []
for n, t in (("Rough", unreal.CustomMaterialOutputType.CMOT_FLOAT1), ("Glow", unreal.CustomMaterialOutputType.CMOT_FLOAT3)):
    co = unreal.CustomOutput(); co.set_editor_property("output_name", n); co.set_editor_property("output_type", t); outs.append(co)
cu.set_editor_property("additional_outputs", outs)
cu.set_editor_property("code", open(f"{ROOT}/Tools/Materials/M_CrevasseIce.hlsl", encoding="utf8").read())
C = MEL.connect_material_expressions
C(sc, "RGB", cu, "Crack"); C(vn, "", cu, "VN"); C(wp, "", cu, "WP"); C(mz, "", cu, "CrestZ")
MEL.connect_material_property(cu, "", unreal.MaterialProperty.MP_BASE_COLOR)
MEL.connect_material_property(cu, "Rough", unreal.MaterialProperty.MP_ROUGHNESS)
MEL.connect_material_property(cu, "Glow", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
MEL.connect_material_property(sn, "RGB", unreal.MaterialProperty.MP_NORMAL)
MEL.recompile_material(mat)
unreal.EditorAssetLibrary.save_loaded_asset(mat, False)
for i in (1, 2):
    m = unreal.load_asset(f"{D}/SM_CrevasseLip_{i:02d}")
    m.set_material(0, mat); unreal.EditorAssetLibrary.save_loaded_asset(m, False)
# abyss: same near-black blue as the bottom of the walls
ab = unreal.load_asset(f"{D}/M_CrevasseAbyss")
print("M_CrevasseIce built")
