# Runs INSIDE the editor (uepy.py): Saved/Backdrops/T_Backdrop_NN.png (from capture_backdrops.py) -> /Game/UI/StartMenu/Backdrops
import unreal, os
at = unreal.AssetToolsHelpers.get_asset_tools()
SRC = "F:/UE_project/YggdrasilsPeak/Saved/Backdrops"
D = "/Game/UI/StartMenu/Backdrops"
for i in range(1, 9):
    n = "T_Backdrop_%02d" % i
    f = f"{SRC}/{n}.png"
    if not os.path.exists(f): continue
    t = unreal.AssetImportTask(); t.filename = f; t.destination_path = D; t.destination_name = n
    t.automated = True; t.save = True; t.replace_existing = True
    at.import_asset_tasks([t])
    a = unreal.load_asset(f"{D}/{n}")
    a.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_UI)
    a.set_editor_property("mip_gen_settings", unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
    a.set_editor_property("never_stream", True)
    unreal.EditorAssetLibrary.save_loaded_asset(a, False)
    print("imported", n)
