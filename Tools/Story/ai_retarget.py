import unreal, json
# Runs INSIDE the editor via Tools/RoutePlanning/uepy (no dot-p-y text here). The Meshy characters (UE5-mannequin bone
# names, own skeletons) get the mannequin's locomotion: IK rig (auto retarget definition) per character, RTG from the
# Manny rig, ABP_Unarmed + its clips duplicated and retargeted with suffix _<id>. Then the Cast.ygs looks
# ("dolma_ai", "father_ai", "sangye_ai") with their own "abp", and a facing check (spawned temp actor, head vs headfront).
ROOT = "F:/UE_project/YggdrasilsPeak"
at = unreal.AssetToolsHelpers.get_asset_tools(); EAL = unreal.EditorAssetLibrary
ar = unreal.AssetRegistryHelpers.get_asset_registry()
EAS = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
src_mesh = unreal.load_asset("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple")
src_rig = unreal.load_asset("/Game/Characters/Climber_Rigged/IK_MannyClimber")
info = json.load(open(f"{ROOT}/Saved/AIAssets.json"))
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

cast_file = f"{ROOT}/Content/Story/Cast.ygs"
cast = json.load(open(cast_file, encoding="utf-8"))
for who in [w for w in ("marek_hl", "nima_hl", "elena_hl", "pema_body_hl") if w in info]:   # 2026-11-21 headless bodies (round 2 chars done earlier)   # round 2 (2026-11-21); nima_ai/elena_ai/marek_ai done 2026-11-20   # dolma/father/sangye done 2026-11-19 (father_ai keeps its sitting "anim")
    mesh = unreal.load_asset(info[who]["mesh"])
    D = f"/Game/Story/AI/{who}/Anims"
    rig_path = f"{D}/IK_{who}"
    rig = unreal.load_asset(rig_path) or at.create_asset(f"IK_{who}", D, unreal.IKRigDefinition, unreal.IKRigDefinitionFactory())
    c = unreal.IKRigController.get_controller(rig)
    c.set_skeletal_mesh(mesh); c.apply_auto_generated_retarget_definition(); c.apply_auto_fbik()
    EAL.save_asset(rig_path, False)
    print(who, "rig root", c.get_retarget_root(), "chains", len(c.get_retarget_chains()))
    rtg_path = f"{D}/RTG_Manny_to_{who}"
    rtg = unreal.load_asset(rtg_path) or at.create_asset(f"RTG_Manny_to_{who}", D, unreal.IKRetargeter, unreal.IKRetargetFactory())
    rc = unreal.IKRetargeterController.get_controller(rtg)
    rc.set_ik_rig(unreal.RetargetSourceOrTarget.SOURCE, src_rig); rc.set_ik_rig(unreal.RetargetSourceOrTarget.TARGET, rig)
    rc.set_preview_mesh(unreal.RetargetSourceOrTarget.SOURCE, src_mesh); rc.set_preview_mesh(unreal.RetargetSourceOrTarget.TARGET, mesh)
    rc.remove_all_ops(); rc.add_default_ops()
    rc.assign_ik_rig_to_all_ops(unreal.RetargetSourceOrTarget.SOURCE, src_rig)
    rc.assign_ik_rig_to_all_ops(unreal.RetargetSourceOrTarget.TARGET, rig)
    rc.auto_map_chains(unreal.AutoMapChainType.EXACT, True)
    # a Meshy skeleton has no root bone (its root IS the pelvis): the default "Root Motion" op would write Manny's
    # floor-level root motion onto the pelvis and drag the hips/coat to the ground (#82, Tools/Story/npc_retarget_fix)
    _rm = rc.get_index_of_op_by_name("Root Motion")
    if _rm >= 0: rc.set_retarget_op_enabled(_rm, False)
    EAL.save_asset(rtg_path, False)
    ctx = [EAL.find_asset_data(abp_path)] + [EAL.find_asset_data(d) for d in sorted(deps)]
    unreal.IKRetargetBatchOperation.duplicate_and_retarget(ctx, src_mesh, mesh, rtg, "", "", "", f"_{who}", D, False, False, True)
    abp = unreal.load_asset(f"{D}/ABP_Unarmed_{who}")
    print(who, "ABP", abp.get_path_name() if abp else "MISSING")
    # facing: Manny faces +Y in mesh space; read where the head points on a temporary actor
    a = EAS.spawn_actor_from_class(unreal.SkeletalMeshActor, unreal.Vector(0, 0, -500000), unreal.Rotator(0, 0, 0))
    smc = a.skeletal_mesh_component; smc.set_skinned_asset_and_update(mesh)
    h, f = smc.get_socket_location("head"), smc.get_socket_location("headfront")
    fwd = f - h
    print(who, "head->headfront (cm)", round(fwd.x, 1), round(fwd.y, 1), round(fwd.z, 1))
    EAS.destroy_actor(a)
    cast[who if who.endswith("_ai") else f"{who}_ai"] = {"body": info[who]["mesh"], "abp": f"{D}/ABP_Unarmed_{who}", "face": "", "garment": "", "hat": False, "grooms": []}

json.dump(cast, open(cast_file, "w", encoding="utf-8"), indent=1, ensure_ascii=False)
print("cast looks", [k for k in cast if k.endswith("_ai")])
