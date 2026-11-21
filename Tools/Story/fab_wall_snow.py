import unreal, json
# Runs INSIDE the editor via Tools/RoutePlanning/uepy. The scanned field walls get the project's snow-capped rock material
# (M_RockSnow: snow on the up-facing faces, like every other rock at the camp) with their own textures; the colour is the
# lichen-toned bake T_<name>_Lichen (PIL, saturation 0.32 / 0.6). The new MI_<name>_Snow replaces MI_<name> in
# Saved/Fab_2026_11_21.json, so the world builder places the walls with it.
ROOT = "F:/UE_project/YggdrasilsPeak"
SRC = f"{ROOT}/Downloads/fab_2026-11-21"
DST = "/Game/Fab/Free_2026_11_21"
EAL = unreal.EditorAssetLibrary; MEL = unreal.MaterialEditingLibrary
at = unreal.AssetToolsHelpers.get_asset_tools()
info = json.load(open(f"{ROOT}/Saved/Fab_2026_11_21.json"))
dusted = unreal.load_asset("/Game/Environment/RockSnow/MI_RockSnow_Stone_Dusted")
parent = unreal.load_asset("/Game/Environment/RockSnow/M_RockSnow")
cap = MEL.get_material_instance_scalar_parameter_value(dusted, "CapStart")
cov = MEL.get_material_instance_scalar_parameter_value(dusted, "Coverage")
print("dusted CapStart", cap, "Coverage", cov)


def imp(src, dest):
    t = unreal.AssetImportTask()
    for k, v in (("filename", src), ("destination_path", dest), ("automated", True), ("replace_existing", True), ("save", True)):
        t.set_editor_property(k, v)
    at.import_asset_tasks([t])
    p = list(t.get_editor_property("imported_object_paths") or [])
    return unreal.load_asset(str(p[0])) if p else None


for name in ("wall_mossy1", "wall_mossy2", "wall_jp1", "wall_jp2"):
    dest = f"{DST}/{name}"
    old = unreal.load_asset(info[name]["mi"])
    col = imp(f"{SRC}/{name}/x/T_{name}_Lichen.png", dest)
    col.set_editor_property("max_texture_size", 2048); EAL.save_loaded_asset(col, False)
    path = f"{dest}/MI_{name}_Snow"
    mi = unreal.load_asset(path) or at.create_asset(f"MI_{name}_Snow", dest, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    MEL.set_material_instance_parent(mi, parent)
    MEL.set_material_instance_texture_parameter_value(mi, "BaseColorTexture", col)
    MEL.set_material_instance_texture_parameter_value(mi, "NormalTexture", MEL.get_material_instance_texture_parameter_value(old, "NormalTexture"))
    MEL.set_material_instance_texture_parameter_value(mi, "MetallicRoughnessTexture", MEL.get_material_instance_texture_parameter_value(old, "MetallicRoughnessTexture"))
    MEL.set_material_instance_scalar_parameter_value(mi, "CapStart", cap)
    MEL.set_material_instance_scalar_parameter_value(mi, "Coverage", cov)
    MEL.update_material_instance(mi)
    EAL.save_loaded_asset(mi, False)
    sm = unreal.load_asset(info[name]["mesh"])
    for k in range(len(sm.get_editor_property("static_materials"))):
        sm.set_material(k, mi)
    EAL.save_loaded_asset(sm, False)
    info[name]["mi_plain"] = info[name]["mi"]; info[name]["mi"] = mi.get_path_name()
    print("SNOW", name, mi.get_path_name())
json.dump(info, open(f"{ROOT}/Saved/Fab_2026_11_21.json", "w"), indent=1)
