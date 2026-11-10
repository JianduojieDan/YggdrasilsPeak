# Runs INSIDE the editor (Tools/Terrain/editor/run.py). Imports the solid icefall blocks (Tools/Blender/icefall_blocks.py ->
# Art/Blender/Export/Icefall) as Nanite meshes into /Game/Environment/Glacier/Icefall and builds M_IcefallIce
# (Tools/Materials/M_IcefallIce.hlsl). Collision: a few convex hulls per block (the player bumps into them; not walked on).
# Set MATERIAL_ONLY = True in globals to rebuild only the material.
import unreal, os, glob
MEL = unreal.MaterialEditingLibrary
at = unreal.AssetToolsHelpers.get_asset_tools()
SML = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
ROOT = "F:/UE_project/YggdrasilsPeak"
D = "/Game/Environment/Glacier/Icefall"

def task(src, name, dest, options=None):
    t = unreal.AssetImportTask()
    t.filename = src; t.destination_path = dest; t.destination_name = name
    t.automated = True; t.save = True; t.replace_existing = True
    if options: t.options = options
    at.import_asset_tasks([t])
    return unreal.load_asset(f"{dest}/{name}")

# ---- material
P = f"{D}/M_IcefallIce"
mat = unreal.load_asset(P) or at.create_asset("M_IcefallIce", D, unreal.Material, unreal.MaterialFactoryNew())
MEL.delete_all_material_expressions(mat)
mat.set_editor_property("tangent_space_normal", False)
def tex(path, y, name):
    e = MEL.create_material_expression(mat, unreal.MaterialExpressionTextureObject, -1100, y)
    e.set_editor_property("texture", unreal.load_asset(path)); return e
iceC = tex("/Game/Environment/Glacier/Seracs/T_Ice004_D", 0, "IceC")
iceN = tex("/Game/Environment/Glacier/Seracs/T_Ice004_N", 150, "IceN")
snC = tex("/Game/AzielArts_Material/Textures/ClumpSnow/ClumpSnow_Color", 300, "SnowC")
snN = tex("/Game/AzielArts_Material/Textures/ClumpSnow/ClumpSnow_Normal", 450, "SnowN")
vn = MEL.create_material_expression(mat, unreal.MaterialExpressionVertexNormalWS, -1100, 600)
wp = MEL.create_material_expression(mat, unreal.MaterialExpressionWorldPosition, -1100, 700)
op = MEL.create_material_expression(mat, unreal.MaterialExpressionObjectPositionWS, -1100, 800)
cu = MEL.create_material_expression(mat, unreal.MaterialExpressionCustom, -500, 300)
cu.set_editor_property("description", "IcefallIce")
cu.set_editor_property("output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT3)
ins = []
for n in ("WP", "VN", "Obj", "IceC", "IceN", "SnowC", "SnowN"):
    ci = unreal.CustomInput(); ci.set_editor_property("input_name", n); ins.append(ci)
cu.set_editor_property("inputs", ins)
outs = []
for n, t in (("Rough", unreal.CustomMaterialOutputType.CMOT_FLOAT1), ("Nrm", unreal.CustomMaterialOutputType.CMOT_FLOAT3), ("Glow", unreal.CustomMaterialOutputType.CMOT_FLOAT3)):
    o = unreal.CustomOutput(); o.set_editor_property("output_name", n); o.set_editor_property("output_type", t); outs.append(o)
cu.set_editor_property("additional_outputs", outs)
cu.set_editor_property("code", open(f"{ROOT}/Tools/Materials/M_IcefallIce.hlsl", encoding="utf8").read())
C = MEL.connect_material_expressions
C(wp, "", cu, "WP"); C(vn, "", cu, "VN"); C(op, "", cu, "Obj")
C(iceC, "", cu, "IceC"); C(iceN, "", cu, "IceN"); C(snC, "", cu, "SnowC"); C(snN, "", cu, "SnowN")
MEL.connect_material_property(cu, "", unreal.MaterialProperty.MP_BASE_COLOR)
MEL.connect_material_property(cu, "Rough", unreal.MaterialProperty.MP_ROUGHNESS)
MEL.connect_material_property(cu, "Nrm", unreal.MaterialProperty.MP_NORMAL)
MEL.connect_material_property(cu, "Glow", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
sp = MEL.create_material_expression(mat, unreal.MaterialExpressionConstant, -300, 700); sp.set_editor_property("r", 0.45)
MEL.connect_material_property(sp, "", unreal.MaterialProperty.MP_SPECULAR)
MEL.recompile_material(mat)
unreal.EditorAssetLibrary.save_loaded_asset(mat, False)
print("material", P)

# ---- meshes
if not globals().get("MATERIAL_ONLY"):
    unreal.SystemLibrary.execute_console_command(None, "Interchange.FeatureFlags.Import.FBX 0")
    for f in sorted(glob.glob(f"{ROOT}/Art/Blender/Export/Icefall/SM_Ice*.fbx")):
        name = os.path.splitext(os.path.basename(f))[0]
        o = unreal.FbxImportUI()
        o.import_mesh = True; o.import_materials = False; o.import_textures = False
        o.mesh_type_to_import = unreal.FBXImportType.FBXIT_STATIC_MESH
        o.static_mesh_import_data.combine_meshes = True
        o.static_mesh_import_data.build_nanite = True
        o.static_mesh_import_data.auto_generate_collision = False
        m = task(f.replace("\\", "/"), name, D, o)
        ns = m.get_editor_property("nanite_settings"); ns.enabled = True; SML.set_nanite_settings(m, ns, True)
        m.set_material(0, mat)
        m.set_editor_property("has_navigation_data", False)
        SML.remove_collisions(m)
        hulls = 1 if name.startswith("SM_IceR") else 4
        SML.set_convex_decomposition_collisions(m, hulls, 16, 100000)
        unreal.EditorAssetLibrary.save_loaded_asset(m, False)
        b = m.get_bounds()
        print(name, "size m", [round(v / 50.0, 1) for v in (b.box_extent.x, b.box_extent.y, b.box_extent.z)])
    unreal.SystemLibrary.execute_console_command(None, "Interchange.FeatureFlags.Import.FBX 1")
