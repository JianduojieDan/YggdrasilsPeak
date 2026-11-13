# Runs INSIDE the editor (Tools/RoutePlanning/uepy). Imports SM_SnowClump (Tools/Blender/snow_clump.py) into
# /Game/Environment/Snow and gives it MI_SnowClump (M_RopeAnchor's plain PBR: snow white, rough, not metal).
# USnowTrailComponent::KickClumps flicks instances of it off the boot at each step on snow.
import unreal
tools = unreal.AssetToolsHelpers.get_asset_tools()
MEL = unreal.MaterialEditingLibrary
t = unreal.AssetImportTask()
t.filename = "F:/UE_project/YggdrasilsPeak/Art/Blender/Export/Snow/SM_SnowClump.fbx"
t.destination_path = "/Game/Environment/Snow"
t.replace_existing = True; t.automated = True; t.save = True
ui = unreal.FbxImportUI()
ui.set_editor_property("import_materials", False); ui.set_editor_property("import_textures", False)
ui.static_mesh_import_data.set_editor_property("auto_generate_collision", False)
t.options = ui
tools.import_asset_tasks([t])
sm = unreal.load_asset("/Game/Environment/Snow/SM_SnowClump")
path = "/Game/Environment/Snow/MI_SnowClump"
mi = unreal.load_asset(path) if unreal.EditorAssetLibrary.does_asset_exist(path) else \
    tools.create_asset("MI_SnowClump", "/Game/Environment/Snow", unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
MEL.set_material_instance_parent(mi, unreal.load_asset("/Game/Environment/FixedRope/M_RopeAnchor"))
MEL.set_material_instance_vector_parameter_value(mi, "Color", unreal.LinearColor(0.86, 0.89, 0.93, 1))
MEL.set_material_instance_scalar_parameter_value(mi, "Metallic", 0.0)
MEL.set_material_instance_scalar_parameter_value(mi, "Roughness", 0.6)
unreal.EditorAssetLibrary.save_loaded_asset(mi)
sm.set_material(0, mi)
unreal.EditorAssetLibrary.save_loaded_asset(sm)
print("snow clump ok", sm.get_bounding_box())
