import unreal, json, os, glob
# Runs INSIDE the editor via Tools/RoutePlanning/uepy (keep the dot-p-y text out of this file).
# Imports the free Fab downloads of 2026-11-21 (Downloads/fab_2026-11-21, see SOURCES.md there) into
# /Game/Fab/Free_2026_11_21/<name>/. Megascans FBX folders get their textures imported at max 2048 px (laptop VRAM) and a
# material instance of /Game/Fab/Materials/Standard/M_MS_Base (masked for the juniper atlas); GLB/FBX props keep the
# materials Interchange makes. Walls are Nanite. Writes Saved/Fab_2026_11_21.json (name -> mesh, material, size).
ROOT = "F:/UE_project/YggdrasilsPeak"
SRC = f"{ROOT}/Downloads/fab_2026-11-21"
DST = "/Game/Fab/Free_2026_11_21"
EAL = unreal.EditorAssetLibrary
MEL = unreal.MaterialEditingLibrary
at = unreal.AssetToolsHelpers.get_asset_tools()
AR = unreal.AssetRegistryHelpers.get_asset_registry()

MEGASCANS = ["wall_mossy1", "wall_mossy2", "wall_jp1", "wall_jp2", "bamboo", "rope_coil"]
NANITE = {"wall_mossy1", "wall_mossy2", "wall_jp1", "wall_jp2", "snow_fence"}
MODELS = {"walkie": "walkie/walkie_talkie_raw_scan.glb", "whiteboard": "whiteboard/whiteboard.glb",
          "ifak": "ifak/individual_first_aid_kit_ifakblack_color.glb", "chullo": "chullo/chullo_hat.glb",
          "beanie": "beanie/better_beanie.glb", "smartphone": None, "solar": None, "snow_fence": None}
TEXSETS = {"juniper": "masked", "stupa_wall": "opaque"}
ONLY = []   # empty = all


def task(src, dest, opts=None):
    t = unreal.AssetImportTask()
    t.set_editor_property("filename", src)
    t.set_editor_property("destination_path", dest)
    t.set_editor_property("automated", True)
    t.set_editor_property("replace_existing", True)
    t.set_editor_property("save", True)
    if opts:
        t.set_editor_property("options", opts)
    at.import_asset_tasks([t])
    return [str(p) for p in (t.get_editor_property("imported_object_paths") or [])]


def assets_of(dest, cls):
    return [str(a.package_name) for a in AR.get_assets_by_path(dest, True) if str(a.asset_class_path.asset_name) == cls]


def first(pattern, folder):
    hits = sorted(glob.glob(os.path.join(folder, "**", pattern), recursive=True))
    return hits[0].replace("\\", "/") if hits else None


def import_textures(folder, dest):
    """BaseColor / Normal / Roughness / AO (+ Opacity / Displacement) at max 2048, right compression per kind"""
    out = {}
    for key, pats in (("albedo", ["*BaseColor*", "*Albedo*"]), ("normal", ["*Normal*"]), ("roughness", ["*Roughness*"]),
                      ("ao", ["*_AO*", "*AO.*"]), ("opacity", ["*Opacity*"]), ("displacement", ["*Displacement*"])):
        f = None
        for p in pats:
            f = first(p + ".jpg", folder) or first(p + ".png", folder) or first(p + ".jpeg", folder)
            if f:
                break
        if not f:
            continue
        paths = task(f, dest)
        tex = unreal.load_asset(paths[0]) if paths else None
        if not tex:
            continue
        tex.set_editor_property("max_texture_size", 2048)
        if key == "normal":
            tex.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_NORMALMAP)
            tex.set_editor_property("srgb", False)
        elif key in ("roughness", "ao", "opacity", "displacement"):
            tex.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_MASKS)
            tex.set_editor_property("srgb", False)
        EAL.save_loaded_asset(tex, False)
        out[key] = tex
    return out


def make_mi(name, dest, texs, masked=False):
    parent = unreal.load_asset("/Game/Fab/Materials/Standard/M_MS_Base_Masked" if masked else "/Game/Fab/Materials/Standard/M_MS_Base")
    params = [str(p) for p in MEL.get_texture_parameter_names(parent)]
    mi_path = f"{dest}/MI_{name}"
    mi = unreal.load_asset(mi_path) or at.create_asset(f"MI_{name}", dest, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    MEL.set_material_instance_parent(mi, parent)
    want = {"albedo": ["albedo", "basecolor", "base color", "diffuse"], "normal": ["normal"], "roughness": ["roughness"],
            "ao": ["ao", "occlusion"], "opacity": ["opacity", "mask"], "displacement": ["displacement", "height"]}
    used = {}
    for key, tex in texs.items():
        for p in params:
            if any(w in p.lower() for w in want.get(key, [])) and p not in used:
                MEL.set_material_instance_texture_parameter_value(mi, p, tex)
                used[p] = key
                break
    EAL.save_loaded_asset(mi, False)
    return mi, used, params


result = {}
try:
    result = json.load(open(f"{ROOT}/Saved/Fab_2026_11_21.json"))
except Exception:
    pass

for name in MEGASCANS:
    if ONLY and name not in ONLY:
        continue
    folder = f"{SRC}/{name}/x"
    fbx = first("*.fbx", folder)
    dest = f"{DST}/{name}"
    paths = task(fbx, dest)
    meshes = assets_of(dest, "StaticMesh")
    if not meshes:
        print("IMPORT FAILED", name, fbx); continue
    sm = unreal.load_asset(meshes[0])
    texs = import_textures(folder, dest)
    mi, used, params = make_mi(name, dest, texs)
    for k in range(len(sm.get_editor_property("static_materials"))):
        sm.set_material(k, mi)
    if name in NANITE:
        ns = sm.get_editor_property("nanite_settings"); ns.set_editor_property("enabled", True); sm.set_editor_property("nanite_settings", ns)
    EAL.save_loaded_asset(sm, False)
    b = sm.get_bounding_box()
    result[name] = {"mesh": meshes[0], "mi": mi.get_path_name(), "params_used": used,
                    "size_cm": [round(b.max.x - b.min.x), round(b.max.y - b.min.y), round(b.max.z - b.min.z)]}
    print("MEGASCANS", name, result[name])
print("M_MS_Base texture params:", params if MEGASCANS else "-")

for name, rel in MODELS.items():
    if ONLY and name not in ONLY:
        continue
    folder = f"{SRC}/{name}"
    src = f"{SRC}/{rel}" if rel else (first("*.fbx", folder) or first("*.glb", folder))
    dest = f"{DST}/{name}"
    task(src, dest)
    meshes = assets_of(dest, "StaticMesh") or assets_of(dest, "SkeletalMesh")
    if not meshes:
        print("IMPORT FAILED", name, src); continue
    sm = unreal.load_asset(meshes[0])
    if name in NANITE and isinstance(sm, unreal.StaticMesh):
        ns = sm.get_editor_property("nanite_settings"); ns.set_editor_property("enabled", True); sm.set_editor_property("nanite_settings", ns)
        EAL.save_loaded_asset(sm, False)
    for t in assets_of(dest, "Texture2D"):
        tex = unreal.load_asset(t)
        if tex and tex.get_editor_property("max_texture_size") == 0:
            tex.set_editor_property("max_texture_size", 2048); EAL.save_loaded_asset(tex, False)
    b = sm.get_bounding_box() if isinstance(sm, unreal.StaticMesh) else sm.get_bounds().box_extent
    size = [round(b.max.x - b.min.x), round(b.max.y - b.min.y), round(b.max.z - b.min.z)] if isinstance(sm, unreal.StaticMesh) else [round(b.x * 2), round(b.y * 2), round(b.z * 2)]
    result[name] = {"mesh": meshes[0], "count": len(meshes), "size_cm": size, "src": src}
    print("MODEL", name, result[name])

for name, kind in TEXSETS.items():
    if ONLY and name not in ONLY:
        continue
    dest = f"{DST}/{name}"
    texs = import_textures(f"{SRC}/{name}/x", dest)
    mi, used, params = make_mi(name, dest, texs, masked=(kind == "masked"))
    result[name] = {"mi": mi.get_path_name(), "params_used": used, "textures": sorted(texs)}
    print("TEXSET", name, result[name])

json.dump(result, open(f"{ROOT}/Saved/Fab_2026_11_21.json", "w"), indent=1)
print("done", len(result))
