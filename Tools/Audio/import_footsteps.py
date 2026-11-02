# Runs INSIDE the editor (uepy.py). Imports the synthesized footsteps (Tools/Audio/synth_footsteps.py ->
# Art/Audio/Footsteps) into /Game/Audio/Footsteps. Snow steps use Ultra Dynamic Sky's Snow_Compress recordings.
import unreal, glob, os
at = unreal.AssetToolsHelpers.get_asset_tools()
D = "/Game/Audio/Footsteps"
tasks = []
for f in sorted(glob.glob("F:/UE_project/YggdrasilsPeak/Art/Audio/Footsteps/Step_*.wav")) + sorted(glob.glob("F:/UE_project/YggdrasilsPeak/Art/Audio/Ice/*.wav")) + sorted(glob.glob("F:/UE_project/YggdrasilsPeak/Art/Audio/Mind/*.wav")):
    t = unreal.AssetImportTask(); t.filename = f.replace("\\", "/"); t.destination_path = D if "Footsteps" in f else ("/Game/Audio/Mind" if "Mind" in f else "/Game/Audio/Ice")
    t.destination_name = os.path.splitext(os.path.basename(f))[0]
    t.automated = True; t.save = True; t.replace_existing = True
    tasks.append(t)
at.import_asset_tasks(tasks)
print("imported", [unreal.load_asset(f"{t.destination_path}/{t.destination_name}").get_class().get_name() for t in tasks][:3], len(tasks))
