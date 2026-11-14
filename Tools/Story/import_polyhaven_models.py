# Runs INSIDE the editor. Imports the downloaded Poly Haven glTF models into /Game/Story/PH/<id>/ and prints each static
# mesh path and size (cm), for the room / camp builders. Re-runnable (skips folders that already hold a mesh).
import unreal, glob, os
at = unreal.AssetToolsHelpers.get_asset_tools(); EAL = unreal.EditorAssetLibrary
SRC = "F:/UE_project/YggdrasilsPeak/Downloads/polyhaven_models_2026-11-14"
out = {}
for g in sorted(glob.glob(f"{SRC}/*/*.gltf")):
    i = os.path.basename(os.path.dirname(g)); dest = f"/Game/Story/PH/{i}"
    have = [p for p in EAL.list_assets(dest, recursive=True) if isinstance(unreal.load_asset(p.split(".")[0]), unreal.StaticMesh)] if EAL.does_directory_exist(dest) else []
    if not have:
        t = unreal.AssetImportTask(); t.filename = g; t.destination_path = dest; t.automated = True; t.replace_existing = True; t.save = True
        at.import_asset_tasks([t])
        have = [p for p in EAL.list_assets(dest, recursive=True) if isinstance(unreal.load_asset(p.split(".")[0]), unreal.StaticMesh)]
    for p in have:
        m = unreal.load_asset(p.split(".")[0]); b = m.get_bounds()
        out[i] = out.get(i, []) + [(p.split(".")[0], round(b.box_extent.x * 2), round(b.box_extent.y * 2), round(b.box_extent.z * 2))]
for i, v in out.items():
    for p, x, y, z in v: print(i, p, x, y, z)
print("models", len(out))
