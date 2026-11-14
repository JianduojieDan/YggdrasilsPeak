import unreal
# Runs INSIDE the editor via Tools/RoutePlanning/uepy (no dot-p-y text in this file). Makes the MetaHuman body walk
# with the UE mannequin's locomotion: IK_MetaHumanBody (auto retarget definition), RTG_Manny_to_MetaHuman, then
# ABP_Unarmed + every clip/blend space it uses duplicated and retargeted to metahuman_base_skel with suffix _MH.
# All built MetaHumans share Common/.../metahuman_base_skel, so one ABP_Unarmed_MH drives the whole cast.
at = unreal.AssetToolsHelpers.get_asset_tools(); EAL = unreal.EditorAssetLibrary
D = "/Game/Story/MetaHumans/Common/Anims"
src_mesh = unreal.load_asset("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple")
dst_mesh = unreal.load_asset("/Game/Story/MetaHumans/Nima/Nima/Body/SKM_MH_Nima_BodyMesh")
src_rig = unreal.load_asset("/Game/Characters/Climber_Rigged/IK_MannyClimber")


def ik_rig(name, mesh):
    path = f"{D}/{name}"
    rig = unreal.load_asset(path) or at.create_asset(name, D, unreal.IKRigDefinition, unreal.IKRigDefinitionFactory())
    c = unreal.IKRigController.get_controller(rig)
    c.set_skeletal_mesh(mesh)
    c.apply_auto_generated_retarget_definition()
    c.apply_auto_fbik()
    EAL.save_asset(path, False)
    print(name, "root", c.get_retarget_root(), "chains", len(c.get_retarget_chains()))
    return rig


dst_rig = ik_rig("IK_MetaHumanBody", dst_mesh)
rtg_path = f"{D}/RTG_Manny_to_MetaHuman"
rtg = unreal.load_asset(rtg_path) or at.create_asset("RTG_Manny_to_MetaHuman", D, unreal.IKRetargeter, unreal.IKRetargetFactory())
rc = unreal.IKRetargeterController.get_controller(rtg)
rc.set_ik_rig(unreal.RetargetSourceOrTarget.SOURCE, src_rig)
rc.set_ik_rig(unreal.RetargetSourceOrTarget.TARGET, dst_rig)
rc.set_preview_mesh(unreal.RetargetSourceOrTarget.SOURCE, src_mesh)
rc.set_preview_mesh(unreal.RetargetSourceOrTarget.TARGET, dst_mesh)
rc.remove_all_ops(); rc.add_default_ops()
rc.assign_ik_rig_to_all_ops(unreal.RetargetSourceOrTarget.SOURCE, src_rig)
rc.assign_ik_rig_to_all_ops(unreal.RetargetSourceOrTarget.TARGET, dst_rig)
rc.auto_map_chains(unreal.AutoMapChainType.EXACT, True)       # same bone names on both: exact chain mapping
EAL.save_asset(rtg_path, False)
print("RTG ops", rc.get_num_retarget_ops())

abp_path = "/Game/Characters/Mannequins/Anims/Unarmed/ABP_Unarmed"
ar = unreal.AssetRegistryHelpers.get_asset_registry()
deps = set()
todo = [abp_path]
while todo:     # the ABP's clips and blend spaces (and the clips inside the blend spaces)
    p = todo.pop()
    for d in ar.get_dependencies(p, unreal.AssetRegistryDependencyOptions(True, True, False, False, False)) or []:
        d = str(d)
        if d in deps or not d.startswith("/Game/Characters/Mannequins/Anims"): continue
        a = EAL.find_asset_data(d)
        cls = str(a.asset_class_path.asset_name)
        if cls in ("AnimSequence", "BlendSpace", "BlendSpace1D", "AimOffsetBlendSpace", "AnimMontage"):
            deps.add(d); todo.append(d)
print("clips", sorted(deps))
ctx = [EAL.find_asset_data(abp_path)] + [EAL.find_asset_data(d) for d in sorted(deps)]
out = unreal.IKRetargetBatchOperation.duplicate_and_retarget(ctx, src_mesh, dst_mesh, rtg, "", "", "", "_MH", D, False, False, True)
for a in out:
    p = str(a.package_name) if hasattr(a, "package_name") else str(a)
    print("retargeted", p)
abp = unreal.load_asset(f"{D}/ABP_Unarmed_MH")
print("ABP_Unarmed_MH", abp, abp.get_editor_property("target_skeleton").get_path_name() if abp else None)
