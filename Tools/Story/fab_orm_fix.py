import unreal, json
# Runs INSIDE the editor via Tools/RoutePlanning/uepy. Follow-up to import_fab_2026_11_21: M_MS_Base reads a glTF-style
# packed texture (R = ambient occlusion, G = roughness, B = metallic); the first import gave it plain roughness, which
# made the stones metallic. Imports the packed T_<name>_ORM.png (made with PIL from the Megascans AO + roughness) and
# the juniper's colour+opacity, and prints the parent's scalar / vector parameters (for the lichen tint).
ROOT = "F:/UE_project/YggdrasilsPeak"
SRC = f"{ROOT}/Downloads/fab_2026-11-21"
DST = "/Game/Fab/Free_2026_11_21"
EAL = unreal.EditorAssetLibrary; MEL = unreal.MaterialEditingLibrary
at = unreal.AssetToolsHelpers.get_asset_tools()
info = json.load(open(f"{ROOT}/Saved/Fab_2026_11_21.json"))


def imp(src, dest):
    t = unreal.AssetImportTask()
    for k, v in (("filename", src), ("destination_path", dest), ("automated", True), ("replace_existing", True), ("save", True)):
        t.set_editor_property(k, v)
    at.import_asset_tasks([t])
    p = list(t.get_editor_property("imported_object_paths") or [])
    return unreal.load_asset(str(p[0])) if p else None


for name in ("wall_mossy1", "wall_mossy2", "wall_jp1", "wall_jp2", "bamboo", "rope_coil", "stupa_wall", "juniper"):
    dest = f"{DST}/{name}"
    tex = imp(f"{SRC}/{name}/x/T_{name}_ORM.png", dest)
    tex.set_editor_property("srgb", False)
    tex.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_MASKS)
    tex.set_editor_property("max_texture_size", 2048)
    EAL.save_loaded_asset(tex, False)
    mi = unreal.load_asset(info[name]["mi"])
    MEL.set_material_instance_texture_parameter_value(mi, "MetallicRoughnessTexture", tex)
    if name == "juniper":
        col = imp(f"{SRC}/{name}/x/T_{name}_BaseColorAlpha.png", dest)
        col.set_editor_property("max_texture_size", 2048); EAL.save_loaded_asset(col, False)
        MEL.set_material_instance_texture_parameter_value(mi, "BaseColorTexture", col)
    EAL.save_loaded_asset(mi, False)
    print("ORM", name, tex.get_path_name())

parent = unreal.load_asset("/Game/Fab/Materials/Standard/M_MS_Base")
print("scalars", [str(p) for p in MEL.get_scalar_parameter_names(parent)])
print("vectors", [str(p) for p in MEL.get_vector_parameter_names(parent)])
print("statics", [str(p) for p in MEL.get_static_switch_parameter_names(parent)])
