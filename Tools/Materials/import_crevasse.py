# Runs INSIDE the editor (uepy.py). Imports the crevasse lips (Tools/Blender/crevasse_lip.py ->
# Art/Blender/Export/Crevasse) into /Game/Environment/Glacier/Crevasse with M_SeracIce, and builds M_CrevasseAbyss
# (the dark floor of the slot). Lips are NOT Nanite: they are walked on and need complex-as-simple collision on the
# full mesh (a convex hull floats above the curved ramp); three reduced LODs keep them cheap at a distance.
import unreal, os, glob
MEL = unreal.MaterialEditingLibrary
at = unreal.AssetToolsHelpers.get_asset_tools()
SML = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
ROOT = "F:/UE_project/YggdrasilsPeak"
D = "/Game/Environment/Glacier/Crevasse"
ice = unreal.load_asset("/Game/Environment/Glacier/Crevasse/M_CrevasseIce") or unreal.load_asset("/Game/Environment/Glacier/Seracs/M_SeracIce")

def task(src, name, dest, options=None):
    t = unreal.AssetImportTask()
    t.filename = src; t.destination_path = dest; t.destination_name = name
    t.automated = True; t.save = True; t.replace_existing = True
    if options: t.options = options
    at.import_asset_tasks([t])
    return unreal.load_asset(f"{dest}/{name}")

unreal.SystemLibrary.execute_console_command(None, "Interchange.FeatureFlags.Import.FBX 0")
for f in sorted(glob.glob(f"{ROOT}/Art/Blender/Export/Crevasse/SM_CrevasseLip_*.fbx")):
    name = os.path.splitext(os.path.basename(f))[0]
    o = unreal.FbxImportUI()
    o.import_mesh = True; o.import_materials = False; o.import_textures = False
    o.mesh_type_to_import = unreal.FBXImportType.FBXIT_STATIC_MESH
    o.static_mesh_import_data.combine_meshes = True
    o.static_mesh_import_data.build_nanite = False
    o.static_mesh_import_data.auto_generate_collision = False
    m = task(f.replace("\\", "/"), name, D, o)
    m.set_material(0, ice)
    opts = unreal.StaticMeshReductionOptions()
    opts.auto_compute_lod_screen_size = False
    sets = []
    for pct, scr in ((1.0, 1.0), (0.3, 0.35), (0.08, 0.12)):
        s = unreal.StaticMeshReductionSettings(); s.percent_triangles = pct; s.screen_size = scr; sets.append(s)
    opts.reduction_settings = sets
    SML.set_lods(m, opts)
    SML.remove_collisions(m)
    bs = m.get_editor_property("body_setup")
    bs.set_editor_property("collision_trace_flag", unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE)
    unreal.EditorAssetLibrary.save_loaded_asset(m, False)
    print(name, "lods", SML.get_lod_count(m), "bounds", m.get_bounding_box())
unreal.SystemLibrary.execute_console_command(None, "Interchange.FeatureFlags.Import.FBX 1")

P = f"{D}/M_CrevasseAbyss"
mat = unreal.load_asset(P) or at.create_asset("M_CrevasseAbyss", D, unreal.Material, unreal.MaterialFactoryNew())
MEL.delete_all_material_expressions(mat)
# unlit near-black blue: the same colour the walls fade to (M_CrevasseIce), so the slot has no visible bottom
mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
em = MEL.create_material_expression(mat, unreal.MaterialExpressionConstant3Vector, -400, 200)
em.set_editor_property("constant", unreal.LinearColor(0.002, 0.006, 0.012, 1))
MEL.connect_material_property(em, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
MEL.recompile_material(mat)
unreal.EditorAssetLibrary.save_loaded_asset(mat, False)
print("crevasse assets done")
