# Runs INSIDE the editor (uepy.py). Imports the backpack screen's art (Art/UI/Items, Art/UI/Backpack) into
# /Game/UI/Backpack as UI textures (no mips, BC7) and the two OFL fonts (Imports/Fonts) as /Game/UI/Fonts/F_*.
import unreal, glob, os
at = unreal.AssetToolsHelpers.get_asset_tools()
ROOT = "F:/UE_project/YggdrasilsPeak"
D = "/Game/UI/Backpack"
files = sorted(glob.glob(f"{ROOT}/Art/UI/Items/*.png") + glob.glob(f"{ROOT}/Art/UI/Backpack/*.png"))
tasks = []
for f in files:
    t = unreal.AssetImportTask(); t.filename = f.replace("\\", "/"); t.destination_path = D
    t.destination_name = os.path.splitext(os.path.basename(f))[0]; t.automated = True; t.save = False; t.replace_existing = True
    tasks.append(t)
at.import_asset_tasks(tasks)
for t in tasks:
    tex = unreal.load_asset(f"{D}/{t.destination_name}")
    tex.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_UI)
    tex.set_editor_property("mip_gen_settings", unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
    tex.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_BC7)
    tex.set_editor_property("never_stream", True)
    unreal.EditorAssetLibrary.save_loaded_asset(tex, False)
print("textures", len(tasks))
for name in ("Caveat", "Alegreya"):
    t = unreal.AssetImportTask(); t.filename = f"{ROOT}/Imports/Fonts/{name}.ttf"; t.destination_path = "/Game/UI/Fonts"
    t.destination_name = f"FF_{name}"; t.automated = True; t.save = True; t.replace_existing = True
    at.import_asset_tasks([t])
    face = unreal.load_asset(f"/Game/UI/Fonts/FF_{name}")
    print(name, face.get_class().get_name() if face else None, t.imported_object_paths)
