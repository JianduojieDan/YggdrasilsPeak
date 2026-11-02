# Runs INSIDE the editor (uepy.py). M_CarbonisedWood: the giant carbonised wood fibre exposed in the Icefall
# (story, Ch. 2). Poly Haven metasequoia_bark (CC0, long stringy fibres) projected on the world's vertical so the
# grain always runs up the formation, darkened to charcoal with a faint sheen on the fibres; world-space normals.
import unreal
MEL = unreal.MaterialEditingLibrary
at = unreal.AssetToolsHelpers.get_asset_tools()
ROOT = "F:/UE_project/YggdrasilsPeak"
D = "/Game/Environment/Story"

def tex(src, name, normal=False):
    p = f"{D}/{name}"
    if not unreal.EditorAssetLibrary.does_asset_exist(p):
        t = unreal.AssetImportTask(); t.filename = src; t.destination_path = D; t.destination_name = name
        t.automated = True; t.save = True; t.replace_existing = True
        at.import_asset_tasks([t])
    a = unreal.load_asset(p)
    if normal:
        a.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_NORMALMAP); a.set_editor_property("srgb", False)
        unreal.EditorAssetLibrary.save_loaded_asset(a, False)
    return a
B = f"{ROOT}/Imports/PolyHaven/metasequoia_bark/metasequoia_bark_"
td = tex(B + "diff_2k.png", "T_FibreBark_D")
tn = tex(B + "nor_dx_2k.png", "T_FibreBark_N", True)

P = f"{D}/M_CarbonisedWood"
mat = unreal.load_asset(P)
if mat:
    MEL.delete_all_material_expressions(mat)
else:
    mat = at.create_asset("M_CarbonisedWood", D, unreal.Material, unreal.MaterialFactoryNew())
mat.set_editor_property("tangent_space_normal", False)
def tobj(t, y):
    e = MEL.create_material_expression(mat, unreal.MaterialExpressionTextureObject, -900, y); e.set_editor_property("texture", t); return e
od, on = tobj(td, 0), tobj(tn, 200)
wp = MEL.create_material_expression(mat, unreal.MaterialExpressionWorldPosition, -900, 400)
vn = MEL.create_material_expression(mat, unreal.MaterialExpressionVertexNormalWS, -900, 500)
cu = MEL.create_material_expression(mat, unreal.MaterialExpressionCustom, -450, 200)
cu.set_editor_property("output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT3)
ins = []
for n in ("Diff", "Nrm", "WP", "VN"):
    ci = unreal.CustomInput(); ci.set_editor_property("input_name", n); ins.append(ci)
cu.set_editor_property("inputs", ins)
outs = []
for n, t in (("Rough", unreal.CustomMaterialOutputType.CMOT_FLOAT1), ("NrmOut", unreal.CustomMaterialOutputType.CMOT_FLOAT3)):
    co = unreal.CustomOutput(); co.set_editor_property("output_name", n); co.set_editor_property("output_type", t); outs.append(co)
cu.set_editor_property("additional_outputs", outs)
cu.set_editor_property("code", open(f"{ROOT}/Tools/Materials/M_CarbonisedWood.hlsl", encoding="utf8").read())
C = MEL.connect_material_expressions
C(od, "", cu, "Diff"); C(on, "", cu, "Nrm"); C(wp, "", cu, "WP"); C(vn, "", cu, "VN")
MEL.connect_material_property(cu, "", unreal.MaterialProperty.MP_BASE_COLOR)
MEL.connect_material_property(cu, "Rough", unreal.MaterialProperty.MP_ROUGHNESS)
MEL.connect_material_property(cu, "NrmOut", unreal.MaterialProperty.MP_NORMAL)
MEL.recompile_material(mat)
unreal.EditorAssetLibrary.save_loaded_asset(mat, False)
print("M_CarbonisedWood rebuilt")
