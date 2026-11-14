# Runs INSIDE the editor (Tools/RoutePlanning/uepy). 2026-11-14: imports the two candidate faces for the pack screen's
# lettering (user's request "more artistic"), from Google Fonts (SIL Open Font License, licences kept next to the
# downloads in Downloads/fonts_2026-11-14): Cinzel Regular and Cormorant Garamond Medium, as static weights made from
# the variable fonts (Saved/Fonts; UE's FreeType draws only a variable font's default instance, Light for Cormorant).
# Settings copied from FF_Alegreya so they load the same way in a packaged game.
import unreal
EAL = unreal.EditorAssetLibrary
SRC = "F:/UE_project/YggdrasilsPeak/Saved/Fonts/"
DST = "/Game/UI/Fonts"
ref = unreal.load_asset(DST + "/FF_Alegreya")
props = ["loading_policy", "hinting", "sub_face_index"]
refv = {}
for p in props:
    try:
        refv[p] = ref.get_editor_property(p)
    except Exception as ex:
        print("ref has no", p, str(ex)[:60])
print("FF_Alegreya:", type(ref).__name__, refv)
tasks = []
for fn, name in [("Cinzel-Regular.ttf", "FF_Cinzel"), ("CormorantGaramond-Medium.ttf", "FF_CormorantGaramond")]:
    t = unreal.AssetImportTask()
    t.set_editor_property("filename", SRC + fn)
    t.set_editor_property("destination_path", DST)
    t.set_editor_property("destination_name", name)
    t.set_editor_property("replace_existing", True)
    t.set_editor_property("automated", True)
    t.set_editor_property("save", False)
    tasks.append(t)
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)
for t in tasks:
    print("imported objects:", list(t.get_editor_property("imported_object_paths")))
for name in ["FF_Cinzel", "FF_CormorantGaramond"]:
    a = unreal.load_asset(DST + "/" + name)
    if not a:
        print("MISSING", name); continue
    for p, v in refv.items():
        try:
            a.set_editor_property(p, v)
        except Exception as ex:
            print("could not set", p, str(ex)[:60])
    EAL.save_loaded_asset(a, False)
    print(name, type(a).__name__, {p: a.get_editor_property(p) for p in refv})
