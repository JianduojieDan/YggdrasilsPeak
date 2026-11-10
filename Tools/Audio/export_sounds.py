# Runs INSIDE the editor (Tools/Terrain/editor/run.py). Exports every SoundWave under /Game/Game_Design/sound_effect
# (the user's recordings) to Saved/AudioWork/raw/<subfolder>/<name>.wav for Tools/Audio/build_game_sounds.py.
import unreal, os
OUT = "F:/UE_project/YggdrasilsPeak/Saved/AudioWork/raw"
SRC = "/Game/Game_Design/sound_effect"
reg = unreal.AssetRegistryHelpers.get_asset_registry()
for a in reg.get_assets_by_path(SRC, recursive=True):
    obj = a.get_asset()
    if not isinstance(obj, unreal.SoundWave):
        print("SKIP", a.asset_name, obj.get_class().get_name()); continue
    sub = str(a.package_path)[len(SRC):].strip("/")
    os.makedirs(f"{OUT}/{sub}", exist_ok=True)
    t = unreal.AssetExportTask()
    t.object = obj; t.filename = f"{OUT}/{sub}/{a.asset_name}.wav"
    t.automated = True; t.prompt = False; t.replace_identical = True
    t.exporter = unreal.SoundExporterWAV()
    ok = unreal.Exporter.run_asset_export_task(t)
    print("OK" if ok else "FAIL", sub, a.asset_name, round(obj.get_editor_property("duration"), 1))
