# Runs INSIDE the editor (Tools/RoutePlanning/uepy): imports the pack v4 flat-lay renders and their chalk outlines
# (Saved/ItemIcons/render_flat/<Name>.png from Tools/UI/render_item_icons --flat, render_flat_line/ from
# Tools/UI/kit_outlines) as UI textures /Game/UI/Backpack/Kit/T_KitFlat_<Name> and T_KitFlat_<Name>_Line.
import unreal
ROOT = "F:/UE_project/YggdrasilsPeak/Saved/ItemIcons/"
DST = "/Game/UI/Backpack/Kit"
names = ["Ration", "Water", "Oxygen", "Stove", "Gas", "Compass", "Journal", "IceAxe", "Rope", "Tent"]
tasks, want = [], []
for n in names:
    for sub, suffix in (("render_flat/", ""), ("render_flat_line/", "_Line")):
        t = unreal.AssetImportTask()
        t.filename = ROOT + sub + n + ".png"
        t.destination_path = DST
        t.destination_name = "T_KitFlat_" + n + suffix
        t.replace_existing = True; t.automated = True; t.save = False
        tasks.append(t); want.append(t.destination_name)
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
