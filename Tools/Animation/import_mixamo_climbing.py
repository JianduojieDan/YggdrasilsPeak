# Runs INSIDE the editor (uepy.py). Imports the Mixamo climbing clips (Imports/Mixamo/Climbing_*.fbx, X Bot "With
# Skin", most "In Place") onto /Game/Characters/Mixamo/XBot_Skeleton and retargets them to the climber with
# RTG_XBot_to_Climber_Rigged -> /Game/Characters/Climber_Rigged/Anims/Climb_<name>_Climber. Root motion is locked
# (the climbing component moves the climber; the slope / top-out clips are not in place on Mixamo).
import unreal, glob, os
at = unreal.AssetToolsHelpers.get_asset_tools()
ROOT = "F:/UE_project/YggdrasilsPeak"
SRC_DIR = "/Game/Characters/Mixamo/Climbing"
OUT_DIR = "/Game/Characters/Climber_Rigged/Anims"
skel = unreal.load_asset("/Game/Characters/Mixamo/XBot_Skeleton")
unreal.SystemLibrary.execute_console_command(None, "Interchange.FeatureFlags.Import.FBX 0")
imported = []
for f in sorted(glob.glob(f"{ROOT}/Imports/Mixamo/Climbing_*.fbx")):
    name = "Climb_" + os.path.splitext(os.path.basename(f))[0].replace("Climbing_", "")
    o = unreal.FbxImportUI()
    o.import_mesh = False; o.import_animations = True; o.import_materials = False; o.import_textures = False
    o.skeleton = skel
    o.mesh_type_to_import = unreal.FBXImportType.FBXIT_ANIMATION
    o.original_import_type = unreal.FBXImportType.FBXIT_ANIMATION
    o.automated_import_should_detect_type = False
    t = unreal.AssetImportTask(); t.filename = f.replace("\\", "/"); t.destination_path = SRC_DIR; t.destination_name = name
    t.automated = True; t.save = True; t.replace_existing = True; t.options = o
    at.import_asset_tasks([t])
    paths = [p for p in t.imported_object_paths]
    anim = next((unreal.load_asset(p) for p in paths if isinstance(unreal.load_asset(p), unreal.AnimSequence)), None)
    if not anim:
        print("no anim from", f, paths); continue
    imported.append(anim)
    print("imported", anim.get_path_name(), round(anim.get_play_length(), 2), "s")
unreal.SystemLibrary.execute_console_command(None, "Interchange.FeatureFlags.Import.FBX 1")

rtg = unreal.load_asset("/Game/Characters/Mixamo/RTG_XBot_to_Climber_Rigged")
src_mesh = unreal.load_asset("/Game/Characters/Mixamo/XBot")
dst_mesh = unreal.load_asset("/Game/Characters/Climber_Rigged/SKM_Climber_Rigged")
ctx = [unreal.EditorAssetLibrary.find_asset_data(a.get_path_name()) for a in imported]
out = unreal.IKRetargetBatchOperation.duplicate_and_retarget(ctx, src_mesh, dst_mesh, rtg, "", "", "", "_Climber", OUT_DIR, False, False, True)
for a in imported:   # no root motion: the climbing component moves the climber
    pass
for a in out:
    p = a.package_name if hasattr(a, "package_name") else a
    asset = unreal.load_asset(str(p))
    print("retargeted", asset.get_path_name() if asset else p)
