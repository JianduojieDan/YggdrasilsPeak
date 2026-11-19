import unreal, json
# Runs INSIDE the editor via Tools/RoutePlanning/uepy (no dot-p-y text here). The father's seated pose from Meshy
# (animation action 363 "Sit Cross-legged on Floor" on his rig): mesh + animation -> /Game/Story/AI/father_sit, and the
# Cast.ygs look "father_ai" holds that pose instead of walking.
ROOT = "F:/UE_project/YggdrasilsPeak"
at = unreal.AssetToolsHelpers.get_asset_tools(); AR = unreal.AssetRegistryHelpers.get_asset_registry()
t = unreal.AssetImportTask()
t.set_editor_property("filename", f"{ROOT}/Downloads/ai_assets/father/father_sit_crosslegged.glb")
t.set_editor_property("destination_path", "/Game/Story/AI/father_sit")
t.set_editor_property("automated", True); t.set_editor_property("replace_existing", True); t.set_editor_property("save", True)
at.import_asset_tasks([t])
found = {}
for a in AR.get_assets_by_path("/Game/Story/AI/father_sit", True):
    found.setdefault(str(a.asset_class_path.asset_name), []).append(str(a.package_name))
print("imported", {k: v for k, v in found.items() if k in ("SkeletalMesh", "AnimSequence", "Skeleton")})
mesh = found.get("SkeletalMesh", [None])[0]; anim = found.get("AnimSequence", [None])[0]
if mesh and anim:
    seq = unreal.load_asset(anim)
    print("anim length s", round(seq.get_play_length(), 2), "skeleton", seq.get_editor_property("skeleton").get_name())
    cf = f"{ROOT}/Content/Story/Cast.ygs"
    cast = json.load(open(cf, encoding="utf-8"))
    cast["father_ai"] = {"body": mesh, "anim": anim, "face": "", "garment": "", "hat": False, "grooms": []}
    json.dump(cast, open(cf, "w", encoding="utf-8"), indent=1, ensure_ascii=False)
    print("father_ai ->", cast["father_ai"])
