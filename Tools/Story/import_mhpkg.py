import unreal
# Runs INSIDE the editor via Tools/RoutePlanning/uepy. Imports the free Epic MetaHuman wardrobe packages (.mhpkg from
# Fab, Downloads/fab_2026-11-21/mh_*) with the engine's MetaHumanPackageFactory and lists what each one created.
ROOT = "F:/UE_project/YggdrasilsPeak"
at = unreal.AssetToolsHelpers.get_asset_tools()
AR = unreal.AssetRegistryHelpers.get_asset_registry()
PKGS = {"mh_techwear": "oa_techwearoutfit.mhpkg", "mh_sweater": "oa_sweater.mhpkg", "mh_cargo": "oa_cargopants.mhpkg",
        "mh_boots": "oa_boots.mhpkg", "mh_braid": "hair_m_updodutchbraid.mhpkg"}
before = set(str(a.package_name) for a in AR.get_assets_by_path("/Game", True))
for k, f in PKGS.items():
    t = unreal.AssetImportTask()
    for p, v in (("filename", f"{ROOT}/Downloads/fab_2026-11-21/{k}/{f}"), ("destination_path", "/Game/MetaHumans/Wardrobe"),
                 ("automated", True), ("replace_existing", True), ("save", True)):
        t.set_editor_property(p, v)
    at.import_asset_tasks([t])
    print("MHPKG", k, [str(x) for x in (t.get_editor_property("imported_object_paths") or [])][:6])
after = set(str(a.package_name) for a in AR.get_assets_by_path("/Game", True))
new = sorted(after - before)
print("NEW", len(new))
for n in new:
    if "/WI_" in n or "Outfit" in n or "Groom" in n or "Hair" in n:
        print("  ", n)
