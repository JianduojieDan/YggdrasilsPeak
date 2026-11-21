import unreal, json, os
# Runs INSIDE the editor via Tools/RoutePlanning/uepy (no dot-p-y text in this file). #82 / #84 root cause (2026-11-21):
# every Meshy NPC retargeter (RTG_Manny_to_<id>, made by Tools/Story/ai_retarget) ran UE's default "Root Motion" op.
# That op writes the source's root motion (Manny's floor-level "root" bone, sliding forward) onto the TARGET's root
# bone - and a Meshy skeleton has no root bone, its root IS the pelvis. So every retargeted clip pinned the pelvis to
# the floor (z = 0, moving forward) while the op kept the children where they were: every vertex skinned to the pelvis
# (hips, crotch, coat bottom) was dragged toward the ground -> Elena's coat stretched to the knees and torn open, the
# triangle flap between Nima's legs, the body "lifting" with speed. Standard setup: a skeleton without a root bone gets
# no root-motion op (our NPCs are moved by code, not root motion). This turns the op off and re-bakes the walk clips
# and blend spaces in place (same names, overwrite), leaving the per-NPC ABP alone (the game plays the blend space).
# Then it checks the pelvis height in the re-baked walk against the reference pose.
# IDS: set by the line below (the runner rewrites it per batch).
IDS = ["sangye_climb", "nima_climb", "climber_icefall", "hale", "lama", "pasang_cook", "father"]   # batch 2 (batch 1 2026-11-21: elena_ai nima_ai marek_ai pema_body dolma sangye)
ROOT = "F:/UE_project/YggdrasilsPeak"
EAL = unreal.EditorAssetLibrary
ar = unreal.AssetRegistryHelpers.get_asset_registry()
APE = unreal.AnimPoseExtensions
info = json.load(open(f"{ROOT}/Saved/AIAssets.json"))
src_mesh = unreal.load_asset("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple")
abp_path = "/Game/Characters/Mannequins/Anims/Unarmed/ABP_Unarmed"

deps, todo = set(), [abp_path]
while todo:
    p = todo.pop()
    for d in ar.get_dependencies(p, unreal.AssetRegistryDependencyOptions(True, True, False, False, False)) or []:
        d = str(d)
        if d in deps or not d.startswith("/Game/Characters/Mannequins/Anims"): continue
        cls = str(EAL.find_asset_data(d).asset_class_path.asset_name)
        if cls in ("AnimSequence", "BlendSpace", "BlendSpace1D", "AimOffsetBlendSpace", "AnimMontage"):
            deps.add(d); todo.append(d)
ctx = [EAL.find_asset_data(d) for d in sorted(deps)]
print("FIX clips to re-bake", len(ctx))


def pelvis_z(gid):
    mesh = unreal.load_asset(info[gid]["mesh"])
    a = unreal.load_asset(f"/Game/Story/AI/{gid}/Anims/MF_Unarmed_Walk_Fwd_{gid}")
    ref = APE.get_reference_pose(mesh.get_editor_property("skeleton"))
    opt = unreal.AnimPoseEvaluationOptions(); opt.set_editor_property("extract_root_motion", False)
    opt.set_editor_property("optional_skeletal_mesh", mesh)
    p = APE.get_anim_pose_at_time(a, a.get_play_length() * 0.25, opt)
    w = lambda pose, b: APE.get_bone_pose(pose, b, unreal.AnimPoseSpaces.WORLD).translation
    return round(w(ref, "pelvis").z, 1), round(w(p, "pelvis").z, 1), round(w(p, "pelvis").y, 1), round(w(p, "thigh_l").z - w(p, "pelvis").z, 1)


for gid in IDS:
    D = f"/Game/Story/AI/{gid}/Anims"
    rtg = unreal.load_asset(f"{D}/RTG_Manny_to_{gid}")
    if not rtg or gid not in info:
        print("FIX", gid, "no retargeter, skipped"); continue
    before = pelvis_z(gid)
    rc = unreal.IKRetargeterController.get_controller(rtg)
    i = rc.get_index_of_op_by_name("Root Motion")
    if i >= 0 and rc.get_retarget_op_enabled(i):
        rc.set_retarget_op_enabled(i, False)
    EAL.save_asset(f"{D}/RTG_Manny_to_{gid}", False)
    mesh = unreal.load_asset(info[gid]["mesh"])
    out = unreal.IKRetargetBatchOperation.duplicate_and_retarget(ctx, src_mesh, mesh, rtg, "", "", "", f"_{gid}", D, False, False, True)
    for a in out or []:
        EAL.save_asset(str(a.package_name), False)
    after = pelvis_z(gid)
    print("FIX", gid, "root-motion op off; re-baked", len(out or []), "| pelvis z ref/walk/fwd/thigh-pelvis before", before, "after", after)
print("done")
