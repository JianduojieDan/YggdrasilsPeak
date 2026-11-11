# Runs INSIDE the editor (uepy.py): imports Step_Rubble_* / Step_Ladder_* from Saved/AudioWork/out/Footsteps into
# /Game/Audio/Footsteps, replacing the old synthesized placeholders (built by build_rubble_ladder.py).
import unreal, glob, os
src = "F:/UE_project/YggdrasilsPeak/Saved/AudioWork/out/Footsteps"
tasks = []
for f in sorted(glob.glob(src + "/Step_Rubble_*.wav") + glob.glob(src + "/Step_Ladder_*.wav")):
    t = unreal.AssetImportTask()
    t.filename = f.replace("\\", "/"); t.destination_path = "/Game/Audio/Footsteps"
    t.destination_name = os.path.splitext(os.path.basename(f))[0]
    t.replace_existing = True; t.automated = True; t.save = True
    tasks.append(t)
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)
print("imported", [t.destination_name for t in tasks if t.imported_object_paths])
