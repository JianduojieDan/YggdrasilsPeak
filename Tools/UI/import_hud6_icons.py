# Runs INSIDE the editor (Tools/RoutePlanning/uepy): imports the HUD v6 line icons, their glows and the small mountain
# (Saved/Review/hud6_icons from Tools/UI/hud_icons) as UI textures /Game/UI/HUD/Icons6/T_HUD6_<Name>[_Halo].
import unreal, os
ROOT = "F:/UE_project/YggdrasilsPeak/Saved/Review/hud6_icons/"
DST = "/Game/UI/HUD/Icons6"
tasks, want = [], []
for f in sorted(os.listdir(ROOT)):
    if not f.endswith(".png"): continue
    t = unreal.AssetImportTask()
    t.filename = ROOT + f; t.destination_path = DST; t.destination_name = f[:-4]
    t.replace_existing = True; t.automated = True; t.save = False
    tasks.append(t); want.append(f[:-4])
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)
done = []
for name in want:
    tex = unreal.load_asset(f"{DST}/{name}")
    if not tex:
        print("MISSING", name); continue
    tex.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_EDITOR_ICON)
    tex.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_UI)
    tex.set_editor_property("mip_gen_settings", unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
    tex.set_editor_property("srgb", True)
    unreal.EditorAssetLibrary.save_loaded_asset(tex)
    done.append(name)
print("imported", len(done), "of", len(want))
