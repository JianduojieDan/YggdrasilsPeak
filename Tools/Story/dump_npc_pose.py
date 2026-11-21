import unreal, json, os
# Runs INSIDE the editor via Tools/RoutePlanning/uepy (no dot-p-y text in this file). #82 (2026-11-21): writes the
# retargeted walk/jog poses of the Meshy NPCs (component space, every bone, 8 times per clip) plus the reference pose to
# Saved/tmp_blender/pose_<id>.json, so Tools/Blender can deform the model with EXACTLY the pose the game plays
# (Blender's own posing never bent the stray chain bones, which hid the coat split).
ROOT = "F:/UE_project/YggdrasilsPeak"
IDS = ["elena_ai", "nima_ai", "pema_body", "marek_ai"]
CLIPS = ["MF_Unarmed_Walk_Fwd", "MF_Unarmed_Jog_Fwd"]
APE = unreal.AnimPoseExtensions
info = json.load(open(f"{ROOT}/Saved/AIAssets.json"))
os.makedirs(f"{ROOT}/Saved/tmp_blender", exist_ok=True)


def dump(pose, names):
    out = {}
    for n in names:
        t = APE.get_bone_pose(pose, n, unreal.AnimPoseSpaces.WORLD)
        q = t.rotation; p = t.translation
        out[str(n)] = [p.x, p.y, p.z, q.x, q.y, q.z, q.w]
    return out


for gid in IDS:
    mesh = unreal.load_asset(info[gid]["mesh"])
    skel = mesh.get_editor_property("skeleton")
    ref = APE.get_reference_pose(skel)
    names = APE.get_bone_names(ref)
    res = {"ref": dump(ref, names), "clips": {}}
    opt = unreal.AnimPoseEvaluationOptions()
    print("OPTS", [(k, opt.get_editor_property(k)) for k in ("extract_root_motion", "should_retarget", "evaluation_type")])
    opt.set_editor_property("extract_root_motion", False)
    opt.set_editor_property("optional_skeletal_mesh", mesh)
    for c in CLIPS:
        a = unreal.load_asset(f"/Game/Story/AI/{gid}/Anims/{c}_{gid}")
        if not a:
            print("missing", gid, c); continue
        L = a.get_play_length()
        res["clips"][c] = {"length": L, "frames": []}
        for k in range(8):
            t = L * k / 8.0
            res["clips"][c]["frames"].append({"t": t, "bones": dump(APE.get_anim_pose_at_time(a, t, opt), names)})
    json.dump(res, open(f"{ROOT}/Saved/tmp_blender/pose_{gid}.json", "w"))
    print("POSE", gid, len(names), "bones", {c: len(v["frames"]) for c, v in res["clips"].items()})
print("done")
