# Runs INSIDE the editor (Tools/RoutePlanning/uepy.py): imports the pack items' studio renders
# (Saved/ItemIcons/render/<Name>.png, Tools/UI/render_item_icons.py) as UI textures /Game/UI/Backpack/Kit/T_Kit_<Name>
# for the v3 pack screen (the kit laid out on cloth). Re-run replaces them.
import unreal
SRC = "F:/UE_project/YggdrasilsPeak/Saved/ItemIcons/render/"
DST = "/Game/UI/Backpack/Kit"
names = ["Ration", "Water", "Oxygen", "Stove", "Gas", "Compass", "Journal", "IceAxe", "Rope"]
tasks = []
for n in names:
    t = unreal.AssetImportTask()
    t.filename = SRC + n + ".png"
    t.destination_path = DST
    t.destination_name = "T_Kit_" + n
    t.replace_existing = True
    t.automated = True
    t.save = False
    tasks.append(t)
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)
done = []
for n in names:
    tex = unreal.load_asset(f"{DST}/T_Kit_{n}")
    if not tex:
        print("MISSING", n); continue
    tex.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_EDITOR_ICON)
    tex.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_UI)
    tex.set_editor_property("mip_gen_settings", unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
    tex.set_editor_property("srgb", True)
    unreal.EditorAssetLibrary.save_loaded_asset(tex)
    done.append(n)
print("imported", done)
