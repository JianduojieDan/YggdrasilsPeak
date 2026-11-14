# Runs INSIDE the editor: the two public-domain paintings (Downloads/wikimedia_thangka_2026-11-14) as textures + picture
# instances of M_StoryProp (Kind 8) for the chapter 1 room.
import unreal, glob, os
at = unreal.AssetToolsHelpers.get_asset_tools(); EAL = unreal.EditorAssetLibrary; mel = unreal.MaterialEditingLibrary
SRC = "F:/UE_project/YggdrasilsPeak/Downloads/wikimedia_thangka_2026-11-14"; TD = "/Game/Story/Photos"; MD = "/Game/Story/Materials"
tasks = []
for f in glob.glob(f"{SRC}/T_*.jpg"):
    t = unreal.AssetImportTask(); t.filename = f; t.destination_path = TD; t.automated = True; t.replace_existing = True; t.save = True; tasks.append(t)
at.import_asset_tasks(tasks)
parent = unreal.load_asset(f"{MD}/M_StoryProp")
for f in glob.glob(f"{SRC}/T_*.jpg"):
    n = os.path.splitext(os.path.basename(f))[0]; tex = unreal.load_asset(f"{TD}/{n}")
    mi_name = "MI_Story_" + n.replace("T_", ""); path = f"{MD}/{mi_name}"
    mi = unreal.load_asset(path) or at.create_asset(mi_name, MD, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    mel.set_material_instance_parent(mi, parent); mel.set_material_instance_scalar_parameter_value(mi, "Kind", 8.0)
    mel.set_material_instance_texture_parameter_value(mi, "Photo", tex); EAL.save_loaded_asset(mi, False); print(mi_name)
