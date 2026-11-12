# Runs INSIDE the editor (uepy.py): sound classes for the settings' volume sliders (YggSettings::ApplySound) and gives
# every game sound in /Game/Audio its class by folder. Only sounds without a class get one; loudness is unchanged
# (a class at volume 1 changes nothing until the player moves a slider).
import unreal
AT = unreal.AssetToolsHelpers.get_asset_tools()
EAL = unreal.EditorAssetLibrary
D = "/Game/Audio/Classes"
classes = {}
for n in ("SC_Effects", "SC_Ambience", "SC_Voices", "SC_Interface"):
    c = unreal.load_asset(f"{D}/{n}")
    if c is None:
        c = AT.create_asset(n, D, unreal.SoundClass, unreal.SoundClassFactory())
        EAL.save_loaded_asset(c, False)
    classes[n] = c
by_folder = {"Body": "SC_Effects", "Footsteps": "SC_Effects", "Gear": "SC_Effects", "Ice": "SC_Effects",
             "Weather": "SC_Ambience", "Mind": "SC_Voices", "UI": "SC_Interface"}
done, kept = 0, 0
for folder, cname in by_folder.items():
    for p in EAL.list_assets(f"/Game/Audio/{folder}", recursive=True, include_folder=False):
        a = EAL.load_asset(p)
        if not isinstance(a, unreal.SoundBase):
            continue
        cur = a.get_editor_property("sound_class_object")
        if cur is not None and cur.get_name() not in classes:
            kept += 1; print("keeps its class", p, cur.get_name()); continue
        a.set_editor_property("sound_class_object", classes[cname])
        EAL.save_loaded_asset(a, False)
        done += 1
print("classed", done, "kept", kept)
