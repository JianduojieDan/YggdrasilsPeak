import unreal, json, sys
# Runs INSIDE the editor via Tools/RoutePlanning/uepy (no dot-p-y text in this file). #82 (2026-11-21): re-imports the
# Meshy characters whose stray-bone skin weights were moved onto the standard UE5-mannequin bones by
# Tools/Blender/meshy_clean_rig (Downloads/ai_assets/<id>/clean/<id>_rigged.glb, same file name) over the same UE
# assets. The skeleton is unchanged, so the IK rig, retargeter, ABP and the retargeted clips stay valid. Prints the
# skeleton path, material slots and vertex count before/after so a changed skeleton or lost material shows at once.
ROOT = "F:/UE_project/YggdrasilsPeak"
IDS = ["nima_climb", "hale"]   # 2026-11-21 batch 2 (batch 1: elena_ai, pema_body; sangye_climb had no weighted strays)
at = unreal.AssetToolsHelpers.get_asset_tools()
EAL = unreal.EditorAssetLibrary
SMS = unreal.get_editor_subsystem(unreal.SkeletalMeshEditorSubsystem)
info = json.load(open(f"{ROOT}/Saved/AIAssets.json"))


def state(path):
    m = unreal.load_asset(path)
    sk = m.get_editor_property("skeleton")
    mats = [(str(s.get_editor_property("material_slot_name")),
             s.get_editor_property("material_interface").get_path_name() if s.get_editor_property("material_interface") else None)
            for s in m.get_editor_property("materials")]
    return dict(skeleton=sk.get_path_name() if sk else None, mats=mats, verts=SMS.get_num_verts(m, 0), lods=SMS.get_lod_count(m))


for gid in IDS:
    path = info[gid]["mesh"]
    before = state(path)
    t = unreal.AssetImportTask()
    for k, v in (("filename", f"{ROOT}/Downloads/ai_assets/{gid}/clean/{gid}_rigged.glb"), ("destination_path", f"/Game/Story/AI/{gid}"),
                 ("automated", True), ("replace_existing", True), ("save", True)):
        t.set_editor_property(k, v)
    at.import_asset_tasks([t])
    after = state(path)
    print("CLEAN", gid, "before", before)
    print("CLEAN", gid, "after ", after)
    print("CLEAN", gid, "same skeleton", before["skeleton"] == after["skeleton"], "same materials", before["mats"] == after["mats"])
print("done")
