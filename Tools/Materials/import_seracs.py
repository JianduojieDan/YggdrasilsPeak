# Runs INSIDE the editor (uepy.py). Imports the Blender seracs (Tools/Blender/serac.py -> Art/Blender/Export/Seracs)
# as Nanite static meshes in /Game/Environment/Glacier/Seracs, the ambientCG Ice004 textures (CC0), and builds
# M_SeracIce: glacier ice coloured from the crack texture's brightness (deep blue cracks, white-blue faces), bluer
# and darker towards the base, snow on the upward faces, Ice004's crack normals for the fractured surface.
import unreal, os, glob
MEL = unreal.MaterialEditingLibrary
at = unreal.AssetToolsHelpers.get_asset_tools()
ROOT = "F:/UE_project/YggdrasilsPeak"
D = "/Game/Environment/Glacier/Seracs"

def task(src, name, dest, options=None):
    t = unreal.AssetImportTask()
    t.filename = src; t.destination_path = dest; t.destination_name = name
    t.automated = True; t.save = True; t.replace_existing = True
    if options: t.options = options
    at.import_asset_tasks([t])
    return unreal.load_asset(f"{dest}/{name}")

# textures
I = f"{ROOT}/Imports/ambientCG/Ice004/Ice004_2K-JPG_"
tc = task(I + "Color.jpg", "T_Ice004_D", D)
tn = task(I + "NormalDX.jpg", "T_Ice004_N", D)
tn.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_NORMALMAP); tn.set_editor_property("srgb", False)
unreal.EditorAssetLibrary.save_loaded_asset(tn, False)

# meshes
unreal.SystemLibrary.execute_console_command(None, "Interchange.FeatureFlags.Import.FBX 0")
meshes = []
for f in ([] if globals().get("MATERIAL_ONLY") else sorted(glob.glob(f"{ROOT}/Art/Blender/Export/Seracs/SM_Serac_*.fbx"))):
    name = os.path.splitext(os.path.basename(f))[0]
    o = unreal.FbxImportUI()
    o.import_mesh = True; o.import_materials = False; o.import_textures = False
    o.mesh_type_to_import = unreal.FBXImportType.FBXIT_STATIC_MESH
    o.static_mesh_import_data.combine_meshes = True
    o.static_mesh_import_data.build_nanite = True
    m = task(f.replace("\\", "/"), name, D, o)
    ns = m.get_editor_property("nanite_settings"); ns.enabled = True; m.set_editor_property("nanite_settings", ns)
    meshes.append(m)
unreal.SystemLibrary.execute_console_command(None, "Interchange.FeatureFlags.Import.FBX 1")

# material
P = f"{D}/M_SeracIce"
mat = unreal.load_asset(P)
if mat:
    MEL.delete_all_material_expressions(mat)
else:
    mat = at.create_asset("M_SeracIce", D, unreal.Material, unreal.MaterialFactoryNew())
uv = MEL.create_material_expression(mat, unreal.MaterialExpressionTextureCoordinate, -1300, 100)
uv.set_editor_property("u_tiling", 0.55); uv.set_editor_property("v_tiling", 0.55)  # ~11 m: large-scale variation, no polka spots
def sample(tex, y, st):
    e = MEL.create_material_expression(mat, unreal.MaterialExpressionTextureSample, -1000, y)
    e.set_editor_property("texture", tex); e.set_editor_property("sampler_type", st)
    MEL.connect_material_expressions(uv, "", e, "UVs")
    return e
sc = sample(tc, 0, unreal.MaterialSamplerType.SAMPLERTYPE_COLOR)
sn = sample(tn, 300, unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)
vn = MEL.create_material_expression(mat, unreal.MaterialExpressionVertexNormalWS, -1000, 600)
wp = MEL.create_material_expression(mat, unreal.MaterialExpressionWorldPosition, -1000, 700)
op = MEL.create_material_expression(mat, unreal.MaterialExpressionObjectPositionWS, -1000, 800)
cu = MEL.create_material_expression(mat, unreal.MaterialExpressionCustom, -500, 200)
cu.set_editor_property("output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT3)
ins = []
for n in ("Crack", "VN", "WP", "Obj"):
    ci = unreal.CustomInput(); ci.set_editor_property("input_name", n); ins.append(ci)
cu.set_editor_property("inputs", ins)
co = unreal.CustomOutput(); co.set_editor_property("output_name", "Rough"); co.set_editor_property("output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT1)
cg = unreal.CustomOutput(); cg.set_editor_property("output_name", "Glow"); cg.set_editor_property("output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT3)
cu.set_editor_property("additional_outputs", [co, cg])
cu.set_editor_property("code", open(f"{ROOT}/Tools/Materials/M_SeracIce.hlsl", encoding="utf8").read())
C = MEL.connect_material_expressions
C(sc, "RGB", cu, "Crack"); C(vn, "", cu, "VN"); C(wp, "", cu, "WP"); C(op, "", cu, "Obj")
MEL.connect_material_property(cu, "", unreal.MaterialProperty.MP_BASE_COLOR)
MEL.connect_material_property(cu, "Rough", unreal.MaterialProperty.MP_ROUGHNESS)
MEL.connect_material_property(cu, "Glow", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
MEL.connect_material_property(sn, "RGB", unreal.MaterialProperty.MP_NORMAL)
MEL.recompile_material(mat)
unreal.EditorAssetLibrary.save_loaded_asset(mat, False)
for m in meshes:
    m.set_material(0, mat)
    unreal.EditorAssetLibrary.save_loaded_asset(m, False)
    b = m.get_bounds()
    print(m.get_name(), "size m", [round(v / 50.0, 1) for v in (b.box_extent.x, b.box_extent.y, b.box_extent.z)])
