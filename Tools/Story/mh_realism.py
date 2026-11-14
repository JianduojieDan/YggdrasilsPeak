import unreal, json, os
# Runs INSIDE the editor via Tools/RoutePlanning/uepy (no dot-p-y text in this file). Realism pass for the chapter 1
# MetaHumans, from research_notes/Chapter 1 realism/faces_realism (2026-11-14): sun- and wind-darkened skin, brick-red
# "plateau" cheeks, matte skin, sun spots, no makeup, believable grooms (short hair kept under hats, sparse facial hair
# on Sherpa men, natural lashes). Rebuilds at most two people per run (each build ~2 min); run again until "all done".
# Then run Tools/Story/mh_cast once so Content/Story/Cast.ygs picks up the new grooms.
ROOT = "/Game/Story/MetaHumans"
STATE = "F:/UE_project/YggdrasilsPeak/Saved/MetaHumanRealism.json"
# accents: region -> (redness, saturation, lightness); du = change of skin tone U (0 light .. 1 dark)
RECIPES = {
    "nima": dict(asset="MH_Nima_Aoi", du=0.12, rough=1.12, freckles=(0.35, 0.2, 0.35),
                 accents=dict(cheeks=(0.60, 0.48, 0.43), nose=(0.58, 0.50, 0.45), ears=(0.58, 0.5, 0.47), forehead=(0.55, 0.5, 0.46)),
                 grooms=dict(Hair="Hair_S_Clean", Beard="Beard_S_Stubble", Mustache="Mustache_S_Uneven", Eyelashes="Eyelashes_S_Sparse", Peachfuzz="Peachfuzz_M_Thin"),
                 iris_sat=1.3),
    "pema": dict(asset="MH_Pema_Lani", du=0.03, rough=1.12, freckles=(0.25, 0.15, 0.3),
                 accents=dict(cheeks=(0.66, 0.55, 0.45), nose=(0.58, 0.5, 0.47), under_eye=(0.5, 0.5, 0.45)),
                 grooms=dict(Hair="Hair_S_PulledBack", Beard=None, Mustache=None, Peachfuzz="Peachfuzz_M_Thin"),
                 iris_sat=1.4),
    "porter_a": dict(asset="MH_Portera", du=0.3, rough=1.10, freckles=(0.0, 0.0, 0.3),
                     accents=dict(cheeks=(0.72, 0.60, 0.45), nose=(0.62, 0.55, 0.46), ears=(0.6, 0.55, 0.47)),
                     grooms=dict(Hair="Hair_S_Messy", Beard=None, Mustache=None, Peachfuzz="Peachfuzz_M_Thin"),
                     iris_sat=1.3),
    "marek": dict(asset="MH_Marek_Bruce", du=0.04, rough=1.06, freckles=(0.3, 0.2, 0.5),
                  accents=dict(nose=(0.75, 0.6, 0.47), cheeks=(0.68, 0.55, 0.46), ears=(0.66, 0.55, 0.47), chin=(0.60, 0.5, 0.47), forehead=(0.56, 0.5, 0.48)),
                  grooms=dict(Hair="Hair_S_BrushCut", Beard="Beard_S_Full", Mustache="Mustache_S_Full", Peachfuzz="Peachfuzz_M_Thin")),
    "elena": dict(asset="MH_Elena_Vivian", du=0.0, rough=1.04, freckles=(0.15, 0.1, 0.4),
                  accents=dict(nose=(0.70, 0.58, 0.47), cheeks=(0.60, 0.52, 0.46), chin=(0.56, 0.5, 0.48), lips=(0.5, 0.42, 0.54)),
                  grooms=dict(Hair="Hair_S_LowPonytail", Eyelashes="Eyelashes_S_Fine", Peachfuzz="Peachfuzz_M_Thin")),
    "jiwoo": dict(asset="MH_Jiwoo", du=0.03, rough=1.04, freckles=(0.0, 0.0, 0.3),
                  accents=dict(nose=(0.66, 0.55, 0.47), cheeks=(0.58, 0.5, 0.47), chin=(0.55, 0.5, 0.48)),
                  grooms=dict(Hair="Hair_S_LowPonytail", Peachfuzz="Peachfuzz_M_Thin"), iris_sat=1.5),
}
EAL = unreal.EditorAssetLibrary
s = unreal.get_editor_subsystem(unreal.MetaHumanCharacterEditorSubsystem)
state = json.load(open(STATE)) if os.path.exists(STATE) else {}
ar = unreal.AssetRegistryHelpers.get_asset_registry()
WI = {str(a.asset_name)[3:]: str(a.package_name) for a in ar.get_assets_by_path("/MetaHumanCharacter/Optional/Grooms", recursive=True) if str(a.asset_name).startswith("WI_")}
builds = 0
for pid, r in RECIPES.items():
    st = state.setdefault(pid, {})
    if st.get("built"): print("REAL", pid, "done"); continue
    if builds >= 2: print("REAL", pid, "next run"); continue
    path = f"{ROOT}/{r['asset']}"
    if not EAL.does_asset_exist(path) and r.get("alt_asset"): path = f"{ROOT}/{r['alt_asset']}"
    ch = unreal.load_asset(path)
    if not ch: print("REAL", pid, "missing", path); continue
    if not s.is_object_added_for_editing(ch): s.try_add_object_to_edit(ch)
    if not st.get("applied"):
        ss = ch.get_editor_property("skin_settings"); sk = ss.get_editor_property("skin")
        base_u = st.setdefault("base_u", sk.get_editor_property("u"))
        sk.set_editor_property("u", min(1.0, base_u + r["du"])); sk.set_editor_property("roughness", r["rough"])
        ss.set_editor_property("skin", sk)
        fr = ss.get_editor_property("freckles"); d, stg, sat = r["freckles"]
        fr.set_editor_property("density", d); fr.set_editor_property("strength", stg); fr.set_editor_property("saturation", sat)
        ss.set_editor_property("freckles", fr)
        acc = ss.get_editor_property("accents")
        for region, (rd, sa, li) in r["accents"].items():
            a = acc.get_editor_property(region)
            a.set_editor_property("redness", rd); a.set_editor_property("saturation", sa); a.set_editor_property("lightness", li)
            acc.set_editor_property(region, a)
        ss.set_editor_property("accents", acc)
        s.commit_skin_settings(ch, ss)
        mk = ch.get_editor_property("makeup_settings")       # no makeup on anyone: they live and work at 4,300 m
        for part in ("eyes", "blush", "lips"):
            m = mk.get_editor_property(part)
            try: m.set_editor_property("type", type(m.get_editor_property("type")).NONE)
            except Exception as e: print("makeup", part, e)
            mk.set_editor_property(part, m)
        fd = mk.get_editor_property("foundation"); fd.set_editor_property("apply_foundation", False); mk.set_editor_property("foundation", fd)
        s.commit_makeup_settings(ch, mk)
        if r.get("iris_sat"):
            ey = ch.get_editor_property("eyes_settings")
            for side in ("eye_left", "eye_right"):
                e = ey.get_editor_property(side); iris = e.get_editor_property("iris")
                iris.set_editor_property("global_saturation", r["iris_sat"]); e.set_editor_property("iris", iris); ey.set_editor_property(side, e)
            s.commit_eyes_settings(ch, ey)
        coll = ch.get_editor_property("internal_collection"); inst = coll.get_editor_property("default_instance")
        for slot_name, item in r["grooms"].items():
            if item is None:
                inst.set_single_slot_selection(slot_name, unreal.MetaHumanPaletteItemKey()); continue
            wi = unreal.load_asset(WI.get(item, ""))
            if not wi: print("no wardrobe item", item); continue
            key = coll.try_add_item_from_wardrobe_item(slot_name, wi)
            if key is None: print("could not add", item, "to", slot_name); continue
            inst.set_single_slot_selection(slot_name, key)
        st["applied"] = True
        EAL.save_loaded_asset(ch, False)
    if not s.can_build_meta_human(ch):        # a skin change can ask for the texture sources again (quick)
        tp = unreal.MetaHumanCharacterTextureRequestParams(); tp.set_editor_property("blocking", True)
        s.request_texture_sources(ch, tp)
    if s.can_build_meta_human(ch):
        nm = pid.replace("_", "").capitalize()
        p = unreal.MetaHumanCharacterEditorBuildParameters()
        p.set_editor_property("pipeline_type", unreal.MetaHumanDefaultPipelineType.OPTIMIZED)
        p.set_editor_property("pipeline_quality", unreal.MetaHumanQualityLevel.HIGH)
        p.set_editor_property("absolute_build_path", f"{ROOT}/{nm}")
        p.set_editor_property("common_folder_path", f"{ROOT}/Common")
        p.set_editor_property("name_override", nm)
        s.build_meta_human(ch, p)
        unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
        st["built"] = True; builds += 1
        print("REAL", pid, "built")
    else:
        print("REAL", pid, "cannot build yet")
    json.dump(state, open(STATE, "w"), indent=1)
print("all done" if all(v.get("built") for v in state.values()) and len(state) == len(RECIPES) else "run again")
