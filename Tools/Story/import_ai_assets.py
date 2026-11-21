import unreal, json, os
# Runs INSIDE the editor via Tools/RoutePlanning/uepy (keep the dot-p-y text out of this file). Imports the Meshy models:
#  props      Art/Meshy/<id>.glb (real size, bottom pivot, from Tools/Blender/prep_meshy)  -> /Game/Story/AI/<id>/
#  characters Downloads/ai_assets/<id>/<id>_rigged.glb (UE5-mannequin bone names)       -> /Game/Story/AI/<id>/
# then gives the tents a two-sided material (seen from inside) and collision, and writes Saved/AIAssets.json
# (id -> mesh path) for the world builder.
ROOT = "F:/UE_project/YggdrasilsPeak"
EAL = unreal.EditorAssetLibrary
AR = unreal.AssetRegistryHelpers.get_asset_registry()
at = unreal.AssetToolsHelpers.get_asset_tools()
PROPS = ["tent_dome", "tent_mess", "tent_kitchen", "tent_toilet", "duffel", "barrel_blue", "bedding", "porter_load", "doko",
         "stove_set", "cook_set", "chorten_a", "chorten_b", "chorten_c", "father_stone", "khata_folded", "khata_tied",
         "copper_drum", "offering_bowl", "butter_lamp", "choktse", "meat_slats", "dung_heap", "tarchen", "glacier_glasses",
         "headlamp", "water_bottle", "daypack", "stream_hole",
         # round 2 (2026-11-21)
         "lodge_main", "lhapso_altar", "sang_burner", "crampons", "ice_anchor", "carabiner_sling", "climbing_helmet",
         "load_pack", "puja_offerings", "gamow_bag", "hanging_stove", "double_boots", "knit_beanie"]
CHARS = ["dolma", "father", "sangye", "nima_ai", "elena_ai", "marek_ai",
         "pema_body", "sangye_climb", "nima_climb", "climber_icefall", "hale", "lama", "pasang_cook",
         "marek_hl", "nima_hl", "elena_hl", "pema_body_hl"]
COLLIDE = {"tent_dome", "tent_mess", "tent_kitchen", "tent_toilet", "barrel_blue", "chorten_a", "chorten_b", "chorten_c",
           "father_stone", "copper_drum", "choktse", "doko", "porter_load", "duffel", "tarchen", "dung_heap", "cook_set",
           "lodge_main", "lhapso_altar", "sang_burner", "gamow_bag", "load_pack"}
TWO_SIDED = {"tent_dome", "tent_mess", "tent_kitchen", "tent_toilet", "khata_folded", "khata_tied", "tarchen", "meat_slats"}
ONLY = ["marek_hl", "nima_hl", "elena_hl", "pema_body_hl"]   # 2026-11-21 headless bodies for MetaHuman heads
ONLY_R2 = ["lodge_main", "lhapso_altar", "sang_burner", "crampons", "ice_anchor", "carabiner_sling", "climbing_helmet", "load_pack",
        "puja_offerings", "gamow_bag", "hanging_stove", "double_boots", "knit_beanie", "pema_body", "sangye_climb", "nima_climb",
        "climber_icefall", "hale", "lama", "pasang_cook"]   # 2026-11-21 round 2 (empty = all)


def import_file(src, dest):
    t = unreal.AssetImportTask()
    t.set_editor_property("filename", src)
    t.set_editor_property("destination_path", dest)
    t.set_editor_property("automated", True)
    t.set_editor_property("replace_existing", True)
    t.set_editor_property("save", True)
    at.import_asset_tasks([t])
    return list(t.get_editor_property("imported_object_paths") or [])


def find(dest, cls):
    out = []
    for a in AR.get_assets_by_path(dest, True):
        if str(a.asset_class_path.asset_name) == cls:
            out.append(str(a.package_name))
    return out


result = {}
try:
    result = json.load(open(f"{ROOT}/Saved/AIAssets.json"))
except Exception:
    pass

for i in PROPS:
    if ONLY and i not in ONLY:
        continue
    dest = f"/Game/Story/AI/{i}"
    import_file(f"{ROOT}/Art/Meshy/{i}.glb", dest)
    meshes = find(dest, "StaticMesh")
    if not meshes:
        print("IMPORT FAILED", i); continue
    sm = unreal.load_asset(meshes[0])
    if i in COLLIDE:   # the mesh itself is the collision (AI meshes have no simple collision)
        bs = sm.get_editor_property("body_setup")
        if bs:
            bs.set_editor_property("collision_trace_flag", unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE)
    if i in TWO_SIDED:
        for k in range(len(sm.get_editor_property("static_materials"))):
            m = sm.get_material(k)
            if isinstance(m, unreal.MaterialInstanceConstant):
                ov = m.get_editor_property("base_property_overrides")
                ov.set_editor_property("override_two_sided", True); ov.set_editor_property("two_sided", True)
                m.set_editor_property("base_property_overrides", ov)
                unreal.MaterialEditingLibrary.update_material_instance(m)
                EAL.save_loaded_asset(m, False)
    EAL.save_loaded_asset(sm, False)
    b = sm.get_bounding_box()
    result[i] = {"mesh": meshes[0], "size_cm": [round(b.max.x - b.min.x), round(b.max.y - b.min.y), round(b.max.z - b.min.z)],
                 "min_z": round(b.min.z, 1)}
    print("PROP", i, result[i])

for i in CHARS:
    if ONLY and i not in ONLY:
        continue
    dest = f"/Game/Story/AI/{i}"
    import_file(f"{ROOT}/Downloads/ai_assets/{i}/{i}_rigged.glb", dest)
    sk = find(dest, "SkeletalMesh")
    if not sk:
        print("IMPORT FAILED", i); continue
    m = unreal.load_asset(sk[0])
    b = m.get_bounds().box_extent
    skel = m.get_editor_property("skeleton")
    result[i] = {"mesh": sk[0], "skeleton": skel.get_path_name() if skel else None,
                 "extent_cm": [round(b.x), round(b.y), round(b.z)]}
    print("CHAR", i, result[i])

json.dump(result, open(f"{ROOT}/Saved/AIAssets.json", "w"), indent=1)
print("done", len(result))
