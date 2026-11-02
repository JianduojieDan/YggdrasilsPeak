# Runs INSIDE the editor (uepy.py). The held rune compass: imports Art/Blender/Export/Compass/SM_Compass_Case and
# SM_Compass_Needle (Tools/Blender/compass_mesh.py) into /Game/Props/Compass, builds M_CompassPart (colour /
# metal / rough / emissive * Glow parameters) and one instance per material slot. The runes' Glow is driven at
# runtime by UCompassComponent (they wake when the needle points up the mountain).
import unreal
MEL = unreal.MaterialEditingLibrary
at = unreal.AssetToolsHelpers.get_asset_tools()
D = "/Game/Props/Compass"
SRC = "F:/UE_project/YggdrasilsPeak/Art/Blender/Export/Compass"
P = f"{D}/M_CompassPart"
mat = unreal.load_asset(P) or at.create_asset("M_CompassPart", D, unreal.Material, unreal.MaterialFactoryNew())
MEL.delete_all_material_expressions(mat)
def vparam(name, val, y):
    e = MEL.create_material_expression(mat, unreal.MaterialExpressionVectorParameter, -600, y)
    e.set_editor_property("parameter_name", name); e.set_editor_property("default_value", unreal.LinearColor(*val, 1)); return e
def sparam(name, val, y):
    e = MEL.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -600, y)
    e.set_editor_property("parameter_name", name); e.set_editor_property("default_value", val); return e
bc = vparam("BaseColor", (0.62, 0.45, 0.2), 0)
me = sparam("Metallic", 1.0, 150); ro = sparam("Roughness", 0.35, 250)
em = vparam("EmissiveColor", (0, 0, 0), 350); gl = sparam("Glow", 0.0, 500)
mul = MEL.create_material_expression(mat, unreal.MaterialExpressionMultiply, -300, 400)
MEL.connect_material_expressions(em, "", mul, "A"); MEL.connect_material_expressions(gl, "", mul, "B")
MEL.connect_material_property(bc, "", unreal.MaterialProperty.MP_BASE_COLOR)
MEL.connect_material_property(me, "", unreal.MaterialProperty.MP_METALLIC)
MEL.connect_material_property(ro, "", unreal.MaterialProperty.MP_ROUGHNESS)
MEL.connect_material_property(mul, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
MEL.recompile_material(mat); unreal.EditorAssetLibrary.save_loaded_asset(mat, False)
PARTS = {  # slot: (colour, metallic, roughness, emissive)
    "brass": ((0.62, 0.45, 0.2), 1.0, 0.38, (0, 0, 0)), "darkbrass": ((0.3, 0.2, 0.1), 1.0, 0.5, (0, 0, 0)),
    "dial": ((0.8, 0.74, 0.6), 0.0, 0.75, (0, 0, 0)), "rune": ((0.9, 0.66, 0.25), 1.0, 0.3, (2.2, 1.2, 0.3)),
    "steel": ((0.25, 0.27, 0.3), 1.0, 0.35, (0, 0, 0)), "needle": ((0.55, 0.06, 0.04), 0.0, 0.45, (0, 0, 0)),
}
mis = {}
for slot, (c, m, r, e) in PARTS.items():
    n = f"MI_Compass_{slot}"
    mi = unreal.load_asset(f"{D}/{n}") or at.create_asset(n, D, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    mi.set_editor_property("parent", mat)
    MEL.set_material_instance_vector_parameter_value(mi, "BaseColor", unreal.LinearColor(*c, 1))
    MEL.set_material_instance_scalar_parameter_value(mi, "Metallic", m)
    MEL.set_material_instance_scalar_parameter_value(mi, "Roughness", r)
    MEL.set_material_instance_vector_parameter_value(mi, "EmissiveColor", unreal.LinearColor(*e, 1))
    unreal.EditorAssetLibrary.save_loaded_asset(mi, False)
    mis[slot] = mi
unreal.SystemLibrary.execute_console_command(None, "Interchange.FeatureFlags.Import.FBX 0")
for name in ("SM_Compass_Case", "SM_Compass_Needle"):
    o = unreal.FbxImportUI()
    o.import_mesh = True; o.import_materials = False; o.import_textures = False
    o.mesh_type_to_import = unreal.FBXImportType.FBXIT_STATIC_MESH
    o.static_mesh_import_data.combine_meshes = True
    o.static_mesh_import_data.auto_generate_collision = False
    t = unreal.AssetImportTask(); t.filename = f"{SRC}/{name}.fbx"; t.destination_path = D; t.destination_name = name
    t.automated = True; t.save = True; t.replace_existing = True; t.options = o
    at.import_asset_tasks([t])
    m = unreal.load_asset(f"{D}/{name}")
    for i, sm in enumerate(m.static_materials):
        slot = str(sm.material_slot_name).split(".")[0]
        if slot in mis:
            m.set_material(i, mis[slot])
    unreal.EditorAssetLibrary.save_loaded_asset(m, False)
    print(name, [str(s.material_slot_name) for s in m.static_materials], m.get_bounding_box())
unreal.SystemLibrary.execute_console_command(None, "Interchange.FeatureFlags.Import.FBX 1")
