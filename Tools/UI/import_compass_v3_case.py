# Runs INSIDE the editor (uepy.py): the v3 case (smooth reeded bezel, Tools/Blender/compass_v2.py) over SM_Compass2_Case
# and the v3 lid engraving (Tools/UI/compass_lid_v3.py) over T_CompassLidIn / T_CompassLidInMask; slots keep their materials.
import unreal
at = unreal.AssetToolsHelpers.get_asset_tools()
D = "/Game/Props/Compass"
ART = "F:/UE_project/YggdrasilsPeak/Art/UI/Compass"
for name, srgb, mask in (("T_CompassLidIn", True, False), ("T_CompassLidInMask", False, True)):
    t = unreal.AssetImportTask(); t.filename = f"{ART}/{name}.png"; t.destination_path = D; t.destination_name = name
    t.automated = True; t.save = True; t.replace_existing = True
    at.import_asset_tasks([t])
    a = unreal.load_asset(f"{D}/{name}"); a.set_editor_property("srgb", srgb)
    if mask: a.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_GRAYSCALE)
    unreal.EditorAssetLibrary.save_loaded_asset(a, False)
m = unreal.load_asset(f"{D}/SM_Compass2_Case")
keep = {str(s.material_slot_name).split(".")[0]: s.material_interface for s in m.static_materials}
unreal.SystemLibrary.execute_console_command(None, "Interchange.FeatureFlags.Import.FBX 0")
o = unreal.FbxImportUI()
o.import_mesh = True; o.import_materials = False; o.import_textures = False
o.mesh_type_to_import = unreal.FBXImportType.FBXIT_STATIC_MESH
o.static_mesh_import_data.combine_meshes = True; o.static_mesh_import_data.auto_generate_collision = False
t = unreal.AssetImportTask(); t.filename = "F:/UE_project/YggdrasilsPeak/Art/Blender/Export/Compass/SM_Compass_Case.fbx"
t.destination_path = D; t.destination_name = "SM_Compass2_Case"; t.automated = True; t.save = True; t.replace_existing = True; t.options = o
at.import_asset_tasks([t])
unreal.SystemLibrary.execute_console_command(None, "Interchange.FeatureFlags.Import.FBX 1")
m = unreal.load_asset(f"{D}/SM_Compass2_Case")
for i, sm in enumerate(m.static_materials):
    s = str(sm.material_slot_name).split(".")[0]
    if s in keep and keep[s]: m.set_material(i, keep[s])
unreal.EditorAssetLibrary.save_loaded_asset(m, False)
print([(str(x.material_slot_name), x.material_interface.get_name() if x.material_interface else None) for x in m.static_materials])
