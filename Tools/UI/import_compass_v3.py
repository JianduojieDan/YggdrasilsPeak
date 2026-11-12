# Runs INSIDE the editor (uepy.py): re-imports the v3 dial (Tools/UI/compass_rose_v3.py) over T_CompassDial and its rune
# glow mask T_CompassDialRunes (same assets, so M_CompassDial and the compass pick them up unchanged).
import unreal
at = unreal.AssetToolsHelpers.get_asset_tools()
D = "/Game/Props/Compass"
ART = "F:/UE_project/YggdrasilsPeak/Art/UI/Compass"
for name, srgb, mask in (("T_CompassDial", True, False), ("T_CompassDialRunes", False, True)):
    t = unreal.AssetImportTask(); t.filename = f"{ART}/{name}.png"; t.destination_path = D; t.destination_name = name
    t.automated = True; t.save = True; t.replace_existing = True
    at.import_asset_tasks([t])
    a = unreal.load_asset(f"{D}/{name}")
    a.set_editor_property("srgb", srgb)
    if mask:
        a.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_GRAYSCALE)
    unreal.EditorAssetLibrary.save_loaded_asset(a, False)
    print("imported", name, a.blueprint_get_size_x(), a.blueprint_get_size_y())
