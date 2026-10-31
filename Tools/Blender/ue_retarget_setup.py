# Runs INSIDE the Unreal editor (remote Python). Sets up retargeting from the UE mannequin to a character
# that has its own skeleton (e.g. an AI-rigged Mixamo-style one), and retargets the template's locomotion.
#   IK_<Name>, IK_MannyClimber (source rig), RTG_Manny_to_<Name>, then ABP_Unarmed + its clips duplicated
#   and retargeted with suffix _<Name> into /Game/Characters/<Name>/Anims.
import unreal

NAME = "Climber_Rigged"
D = f"/Game/Characters/{NAME}"
at = unreal.AssetToolsHelpers.get_asset_tools()
eal = unreal.EditorAssetLibrary
target_mesh = unreal.load_asset(f"{D}/SKM_{NAME}")
source_mesh = unreal.load_asset("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple")

def ik_rig(asset_name, mesh):
    path = f"{D}/{asset_name}"
    rig = unreal.load_asset(path) or at.create_asset(asset_name, D, unreal.IKRigDefinition, unreal.IKRigDefinitionFactory())
    c = unreal.IKRigController.get_controller(rig)
    c.set_skeletal_mesh(mesh)
    c.apply_auto_generated_retarget_definition()
    c.apply_auto_fbik()
    eal.save_asset(path, False)
    print(asset_name, "root", c.get_retarget_root(), "chains",
          [(str(ch.get_editor_property("chain_name")), str(c.get_retarget_chain_start_bone(ch.get_editor_property("chain_name"))),
            str(c.get_retarget_chain_end_bone(ch.get_editor_property("chain_name")))) for ch in c.get_retarget_chains()])
    return rig

src_rig = ik_rig("IK_MannyClimber", source_mesh)
dst_rig = ik_rig(f"IK_{NAME}", target_mesh)

rtg_path = f"{D}/RTG_Manny_to_{NAME}"
rtg = unreal.load_asset(rtg_path) or at.create_asset(f"RTG_Manny_to_{NAME}", D, unreal.IKRetargeter, unreal.IKRetargetFactory())
rc = unreal.IKRetargeterController.get_controller(rtg)
rc.set_ik_rig(unreal.RetargetSourceOrTarget.SOURCE, src_rig)
rc.set_ik_rig(unreal.RetargetSourceOrTarget.TARGET, dst_rig)
rc.set_preview_mesh(unreal.RetargetSourceOrTarget.SOURCE, source_mesh)
rc.set_preview_mesh(unreal.RetargetSourceOrTarget.TARGET, target_mesh)
rc.remove_all_ops()
rc.add_default_ops()
rc.assign_ik_rig_to_all_ops(unreal.RetargetSourceOrTarget.SOURCE, src_rig)
rc.assign_ik_rig_to_all_ops(unreal.RetargetSourceOrTarget.TARGET, dst_rig)
rc.auto_map_chains(unreal.AutoMapChainType.FUZZY, True)
# the climber stands in a T-pose, Manny in an A-pose: pose the target's limbs to match the source before retargeting
rc.create_retarget_pose("Aligned", unreal.RetargetSourceOrTarget.TARGET)
rc.auto_align_all_bones(unreal.RetargetSourceOrTarget.TARGET)
rc.snap_bone_to_ground(unreal.Name("LeftToeBase"), unreal.RetargetSourceOrTarget.TARGET)
eal.save_asset(rtg_path, False)
print("RTG ops", rc.get_num_retarget_ops(), "pose", rc.get_current_retarget_pose_name(unreal.RetargetSourceOrTarget.TARGET))
