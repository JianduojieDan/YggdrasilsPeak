import unreal, json, os, time
# Runs INSIDE the editor via Tools/RoutePlanning/uepy (no dot-p-y text in this file: the editor's file-exec mode
# would take it for a file name). Re-runnable, one cloud job per run, for the chapter 1 cast as MetaHumans:
#   create from preset -> clear the default outfit (the optimized build deletes the body under it, and the Blender
#   garments need the whole body) -> skin tone -> texture sources + auto-rig (Epic cloud, ONE AT A TIME: parallel
#   requests fail with "Busy"; needs the user's Epic sign-in) -> build (UE-optimized, High) -> Content/Story/Cast.ygs.
# Run it again until every person says "built".
CAST = {
    # id: (preset, skin U (0 light .. 1 dark) or None, garment outfit, hide hair under a hat)
    # chosen by eye from Saved/Compare/faces/_sheet (Tools/Story/mh_facesheet), 2026-11-14
    "nima":    ("Aoi",    None, "nima",     True),     # older, sun-weathered: the old sirdar
    "pema":    ("Lani",   None, "pema",     True),     # weathered, warm
    "marek":   ("Bruce",  None, "marek",    True),     # rugged expedition leader
    "elena":   ("Vivian", 0.36, "elena",    False),    # Spanish first-timer: warmer skin than the preset
    "jiwoo":   ("Aera",   None, "jiwoo",    True),
    "porter_a": ("Kelvin", 0.55, "porter_a", True),
}
ROOT = "/Game/Story/MetaHumans"; COMMON = ROOT + "/Common"
STATE = "F:/UE_project/YggdrasilsPeak/Saved/MetaHumanCast.json"
EAL = unreal.EditorAssetLibrary
s = unreal.get_editor_subsystem(unreal.MetaHumanCharacterEditorSubsystem)
state = json.load(open(STATE)) if os.path.exists(STATE) else {}
cast_out = {}
cloud_used = False


def save_state(): json.dump(state, open(STATE, "w"), indent=1)


def short(i): return i.replace("_", "").capitalize()


for pid, (preset, skin_u, outfit, hat) in CAST.items():
    nm = short(pid); path = f"{ROOT}/MH_{nm}_{preset.replace('-', '')}"     # one character asset per preset tried
    st = state.setdefault(pid, {})
    if st.get("preset", preset) != preset:      # recast: build again into the same folder (Cast.ygs paths stay)
        st.clear()
    st["preset"] = preset
    if st.get("built_full") and not EAL.does_asset_exist(path):
        path = f"{ROOT}/MH_{nm}"              # built before the per-preset names (porter, Ji-woo): never make an unrigged copy
    if not EAL.does_asset_exist(path):
        EAL.duplicate_asset(f"/MetaHumanCharacter/Optional/Presets/{preset}", path)
    ch = unreal.load_asset(path)
    if not s.is_object_added_for_editing(ch): s.try_add_object_to_edit(ch)
    bp_path = f"{ROOT}/{nm}/{nm}/BP_{nm}"
    msg = ""
    if not st.get("built_full"):
        inst = ch.get_editor_property("internal_collection").get_editor_property("default_instance")
        inst.set_single_slot_selection("Outfits", unreal.MetaHumanPaletteItemKey())
        if skin_u is not None:
            ss = ch.get_editor_property("skin_settings"); sk = ss.get_editor_property("skin")
            if abs(sk.get_editor_property("u") - skin_u) > 0.01:
                sk.set_editor_property("u", skin_u); ss.set_editor_property("skin", sk); s.commit_skin_settings(ch, ss)
        if not s.can_build_meta_human(ch) and not cloud_used:
            t0 = time.time()
            if not ch.get_editor_property("has_high_resolution_textures"):
                tp = unreal.MetaHumanCharacterTextureRequestParams(); tp.set_editor_property("blocking", True)
                s.request_texture_sources(ch, tp)
            if not s.can_build_meta_human(ch):
                rp = unreal.MetaHumanCharacterAutoRiggingRequestParams()
                rp.set_editor_property("rig_type", unreal.MetaHumanRigType.JOINTS_AND_BLEND_SHAPES)
                rp.set_editor_property("blocking", True)
                s.request_auto_rigging(ch, rp)
            cloud_used = True
            msg += f"cloud {round(time.time() - t0)} s; "
        if s.can_build_meta_human(ch):
            p = unreal.MetaHumanCharacterEditorBuildParameters()
            p.set_editor_property("pipeline_type", unreal.MetaHumanDefaultPipelineType.OPTIMIZED)
            p.set_editor_property("pipeline_quality", unreal.MetaHumanQualityLevel.HIGH)
            p.set_editor_property("absolute_build_path", f"{ROOT}/{nm}")
            p.set_editor_property("common_folder_path", COMMON)
            p.set_editor_property("name_override", nm)
            s.build_meta_human(ch, p)
            unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)   # builds stay in memory otherwise
            if EAL.does_asset_exist(bp_path):
                st["built_full"] = True; st["exported"] = False; msg += "built now"
            else:
                msg += "build failed"
        else:
            msg += "waiting (next run)"
        EAL.save_loaded_asset(ch, False); save_state()
    else:
        msg = "built"
    print("CAST", pid, preset, msg)
    if st.get("built_full"):
        d = f"{ROOT}/{nm}/{nm}"
        bp = unreal.load_asset(bp_path)
        subs = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
        grooms = []; meshes = {}
        for h in subs.k2_gather_subobject_data_for_blueprint(bp):
            o = unreal.SubobjectDataBlueprintFunctionLibrary.get_associated_object(unreal.SubobjectDataBlueprintFunctionLibrary.get_data(h))
            if isinstance(o, unreal.SkeletalMeshComponent) and o.get_skeletal_mesh_asset():
                # the build names meshes after the CHARACTER asset (SKM_MH_Pema_Lani_BodyMesh): read them from the BP
                k = o.get_name().replace("_GEN_VARIABLE", "")
                if k in ("Body", "Face"): meshes[k] = o.get_skeletal_mesh_asset().get_path_name().split(".")[0]
            if isinstance(o, unreal.GroomComponent) and o.groom_asset:
                g = (o.get_name().replace("_GEN_VARIABLE", ""), o.groom_asset.get_path_name(), o.binding_asset.get_path_name() if o.binding_asset else "")
                if g not in grooms: grooms.append(g)
        body = meshes.get("Body", f"{d}/Body/SKM_MH_{nm}_BodyMesh"); face = meshes.get("Face", f"{d}/Face/SKM_MH_{nm}_FaceMesh")
        cast_out[pid] = dict(body=body, face=face,
                             garment=f"/Game/Story/Garments/SKM_G_{outfit}", hat=hat,
                             grooms=[dict(slot=a, groom=b, binding=c) for a, b, c in grooms])   # short hair stays under hats (a bald nape reads as CG)
        # body + face for the Blender tailor (Tools/Blender/garments fits each outfit to its own person)
        for src, fn in ((body, f"SKM_MH_{nm}_Body"), (face, f"SKM_MH_{nm}_Face")):
            fbx = f"F:/UE_project/YggdrasilsPeak/Art/Blender/Source/MetaHuman/{fn}.fbx"
            if os.path.exists(fbx) and st.get("exported"): continue
            t = unreal.AssetExportTask(); t.object = unreal.load_asset(src); t.filename = fbx; t.automated = True; t.prompt = False; t.replace_identical = True
            t.exporter = unreal.SkeletalMeshExporterFBX()
            o = unreal.FbxExportOption(); o.set_editor_property("level_of_detail", False); o.set_editor_property("export_morph_targets", False); t.options = o
            unreal.Exporter.run_asset_export_task(t)
        st["exported"] = True; save_state()
out = "F:/UE_project/YggdrasilsPeak/Content/Story/Cast.ygs"
old = json.load(open(out)) if os.path.exists(out) else {}
old.update(cast_out)
json.dump(old, open(out, "w"), indent=1)
print("cast file", list(old))
