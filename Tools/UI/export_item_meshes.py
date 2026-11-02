# Runs INSIDE the editor (uepy.py). Exports the pack items' game meshes (with their textures) as .glb into
# Saved/ItemIcons/src for Tools/UI/render_item_icons.py (Blender) - the icons are drawn from the real game assets.
import unreal, os
OUT = "F:/UE_project/YggdrasilsPeak/Saved/ItemIcons/src"
os.makedirs(OUT, exist_ok=True)
ITEMS = {
    "Ration": "/Game/Props/Supplies/SM_Ration",
    "Water": "/Game/Props/Supplies/SM_Thermos",
    "Oxygen": "/Game/Fab/Oxygen_Tank/oxygen_tank/StaticMeshes/oxygen_tank",
    "IceAxe": "/Game/Fab/Climbing_Axe_for_Blender/climbing_axe_422_packed/StaticMeshes/climbing_axe_422_packed",
    "Rope": "/Game/Fab/Megascans/3D/Bundled_Rope_Coil_vhnmbdcs/Raw/vhnmbdcs_tier_0/StaticMeshes/vhnmbdcs_tier_0",
    "Stove": "/Game/Props/Blender/SM_Food_Stove",
    "Gas": "/Game/Props/Blender/SM_Food_GasCanister",
    "Tent": "/Game/Fab/Camping_Tent/camping_tent/StaticMeshes/camping_tent",
}
for name, path in ITEMS.items():
    t = unreal.AssetExportTask()
    t.object = unreal.load_asset(path)
    t.filename = f"{OUT}/{name}.glb"
    t.automated = True; t.prompt = False; t.replace_identical = True
    ok = unreal.Exporter.run_asset_export_task(t)
    print(name, ok, os.path.exists(t.filename) and os.path.getsize(t.filename))
