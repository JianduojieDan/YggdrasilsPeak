# Runs INSIDE the editor (Tools/Terrain/editor/run.py). Imports Saved/AudioWork/out (from build_game_sounds.py and
# synth_game_sounds.py) as SoundWaves into /Game/Audio/<Folder>/<Name>, replacing same-named assets; loops are marked
# looping. The game code loads them by these paths (SnowTrail, Sanity, SeracCollapseZone, RealmSky, MountainAudio, HUD).
import unreal, json, os
OUT = "F:/UE_project/YggdrasilsPeak/Saved/AudioWork/out"
at = unreal.AssetToolsHelpers.get_asset_tools()
items = []
for mf in ("manifest.json", "manifest_synth.json"):
    p = f"{OUT}/{mf}"
    if os.path.exists(p):
        items += json.load(open(p))
tasks = []
for m in items:
    t = unreal.AssetImportTask()
    t.filename = f"{OUT}/{m['folder']}/{m['name']}.wav"
    t.destination_path = f"/Game/Audio/{m['folder']}"
    t.destination_name = m["name"]
    t.automated = True; t.replace_existing = True; t.save = False
    tasks.append(t)
at.import_asset_tasks(tasks)
done = 0
for m in items:
    s = unreal.load_asset(f"/Game/Audio/{m['folder']}/{m['name']}")
    if not s:
        print("MISSING", m["folder"], m["name"]); continue
    s.set_editor_property("looping", bool(m["loop"]))
    if m["loop"]:
        # beds fade to near-silence and back: keep them running (no restart from the top when they come back)
        s.set_editor_property("virtualization_mode", unreal.VirtualizationMode.PLAY_WHEN_SILENT)
    unreal.EditorAssetLibrary.save_loaded_asset(s, False)
    done += 1
print("imported", done, "of", len(items))
