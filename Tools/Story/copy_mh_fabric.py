import unreal
# Runs INSIDE the editor (Tools/RoutePlanning/uepy). Copies Epic's MetaHuman clothing fabric textures (micro weave/knit
# normals, macro heather/pilling, crease + memory wrinkle normals, default clothing AO/N) from the MetaHumanCharacter
# plugin into /Game/Story/Garments/Tex/MH so the garment material can use them in a packaged game, and prints how
# Epic's fabric material is parameterised (to copy its approach). Never write the dot-p-y text in this file.
EAL = unreal.EditorAssetLibrary
SRC = "/MetaHumanCharacter/Optional/Clothing/Common/Materials"
DST = "/Game/Story/Garments/Tex/MH"
done = []
for p in EAL.list_assets(SRC, recursive=True, include_folder=False):
    path = p.split(".")[0]
    a = unreal.load_asset(path)
    if isinstance(a, unreal.Texture2D):
        dst = f"{DST}/{a.get_name()}"
        if not EAL.does_asset_exist(dst):
            EAL.duplicate_asset(path, dst)
        t = unreal.load_asset(dst)
        done.append((a.get_name(), t.blueprint_get_size_x(), t.blueprint_get_size_y(), str(t.get_editor_property("compression_settings")), t.get_editor_property("srgb"), path.replace(SRC, "")))
for d in sorted(done): print("TEX", d)
mel = unreal.MaterialEditingLibrary
for path in [SRC + "/M_fabric_simpler", SRC + "/Crewneckt/Male/M_top_crewneckt"]:
    m = unreal.load_asset(path)
    if not m:
        for p in EAL.list_assets(SRC, recursive=True):
            if p.split(".")[0].endswith(path.split("/")[-1]): m = unreal.load_asset(p.split(".")[0]); break
    if not m: print("MISSING", path); continue
    print("MAT", m.get_path_name(), type(m).__name__)
    try:
        print("  scalars", [str(n) for n in mel.get_scalar_parameter_names(m)])
        print("  vectors", [str(n) for n in mel.get_vector_parameter_names(m)])
        print("  textures", [str(n) for n in mel.get_texture_parameter_names(m)])
        for n in mel.get_scalar_parameter_names(m):
            print("   s", str(n), mel.get_material_default_scalar_parameter_value(m, n))
        for n in mel.get_texture_parameter_names(m):
            t = mel.get_material_default_texture_parameter_value(m, n); print("   t", str(n), t.get_path_name() if t else None)
    except Exception as e:
        print("  params error", e)
unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
print("copied", len(done))
