import unreal, json, os, sys
# Runs INSIDE the editor via Tools/RoutePlanning/uepy. 2026-11-21: dress MetaHumans in Epic's free wardrobe items (Fab
# .mhpkg, Tools/Story/import_mhpkg) instead of my Blender garments, and rebuild (UE-optimized, High) into the same
# folders, so the Cast.ygs paths stay. ONE person per run (VRAM: 1-2 builds per editor session). Prints the built
# blueprint's meshes and grooms for Cast.ygs.
# the remote-exec script runs in the EDITOR process (its environment, not the caller's): the person comes from a file
WHO = open("F:/UE_project/YggdrasilsPeak/Saved/mh_who.txt").read().strip() if os.path.exists("F:/UE_project/YggdrasilsPeak/Saved/mh_who.txt") else "jiwoo"
PLAN = {
    # id: (character asset, build folder name, {slot: [wardrobe items]})
    "jiwoo": ("/Game/Story/MetaHumans/MH_Jiwoo_Aera", "Jiwoo", {"Outfits": ["/Game/MetaHumans/Wardrobe/WI_OA_techwearOutfit"]}),
    "pema": ("/Game/Story/MetaHumans/MH_Pema_Lani", "Pema", {"Hair": ["/Game/MetaHumans/Wardrobe/WI_Hair_M_UpdoDutchBraid"]}),
}
ROOT = "/Game/Story/MetaHumans"; COMMON = ROOT + "/Common"
EAL = unreal.EditorAssetLibrary
s = unreal.get_editor_subsystem(unreal.MetaHumanCharacterEditorSubsystem)
path, nm, slots = PLAN[WHO]
ch = unreal.load_asset(path)
if not s.is_object_added_for_editing(ch): s.try_add_object_to_edit(ch)
col = ch.get_editor_property("internal_collection")
inst = col.get_editor_property("default_instance")
print("SLOTS", [str(x) for x in col.get_slot_names()])
for slot, items in slots.items():
    keys = []
    for wp in items:
        wi = unreal.load_asset(wp)
        ks = list(col.get_item_keys_for_wardrobe_item(wi) or [])
        if not ks:
            r = col.try_add_item_from_wardrobe_item(slot, wi)
            print("ADD", slot, wp, r)
            ks = list(col.get_item_keys_for_wardrobe_item(wi) or [])
        keys += ks
    print("KEYS", slot, len(keys))
    if keys:
        inst.set_single_slot_selection(slot, keys[0])
        for k in keys[1:]:
            inst.try_add_slot_selection(slot, k)
print("CAN BUILD", s.can_build_meta_human(ch))
if s.can_build_meta_human(ch):
    p = unreal.MetaHumanCharacterEditorBuildParameters()
    p.set_editor_property("pipeline_type", unreal.MetaHumanDefaultPipelineType.OPTIMIZED)
    p.set_editor_property("pipeline_quality", unreal.MetaHumanQualityLevel.HIGH)
    p.set_editor_property("absolute_build_path", f"{ROOT}/{nm}")
    p.set_editor_property("common_folder_path", COMMON)
    p.set_editor_property("name_override", nm)
    s.build_meta_human(ch, p)
    unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
EAL.save_loaded_asset(ch, False)
bp = unreal.load_asset(f"{ROOT}/{nm}/{nm}/BP_{nm}")
subs = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
SDBL = unreal.SubobjectDataBlueprintFunctionLibrary
for h in subs.k2_gather_subobject_data_for_blueprint(bp):
    o = SDBL.get_associated_object(SDBL.get_data(h))
    if isinstance(o, unreal.SkeletalMeshComponent):
        m = o.get_skeletal_mesh_asset()
        print("COMP", type(o).__name__, o.get_name(), m.get_path_name() if m else None)
    elif isinstance(o, unreal.GroomComponent):
        print("GROOM", o.get_name(), o.groom_asset.get_path_name() if o.groom_asset else None, o.binding_asset.get_path_name() if o.binding_asset else None)
    elif o is not None and "Component" in type(o).__name__:
        print("OTHER", type(o).__name__, o.get_name())
