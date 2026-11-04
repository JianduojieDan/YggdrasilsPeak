# Runs INSIDE the editor: imports Saved/FarTerrain meshes + normal maps to /Game/Environment/Distant/Far
import unreal
SRC = "F:/UE_project/YggdrasilsPeak/Saved/FarTerrain"
DST = "/Game/Environment/Distant/Far"
tasks = []
for f in ["FarRing.glb", "FarBackground.glb", "T_FarRing_N.png", "T_FarBackground_N.png"]:
    t = unreal.AssetImportTask()
    t.filename = f"{SRC}/{f}"; t.destination_path = DST; t.replace_existing = True; t.automated = True; t.save = False
    tasks.append(t)
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)
for t in tasks: print(t.filename, [str(p) for p in t.imported_object_paths])
for n in ["T_FarRing_N", "T_FarBackground_N"]:
    tx = unreal.load_asset(f"{DST}/{n}")
    tx.set_editor_property("srgb", False)
    tx.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_NORMALMAP)
    tx.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_WORLD)
    tx.set_editor_property("never_stream", True)
for n in ["FarRing", "FarBackground"]:
    m = unreal.load_asset(f"{DST}/{n}")
    ns = m.get_editor_property("nanite_settings"); ns.enabled = True
    ns.fallback_target = unreal.NaniteFallbackTarget.PERCENT_TRIANGLES; ns.fallback_percent_triangles = 0.5; ns.position_precision = 4
    m.set_editor_property("nanite_settings", ns)
    unreal.EditorStaticMeshLibrary.remove_collisions(m)
    bs = m.get_editor_property("body_setup")
    if bs: bs.set_editor_property("collision_trace_flag", unreal.CollisionTraceFlag.CTF_USE_SIMPLE_AS_COMPLEX)
    print(n, m.get_num_vertices(0), m.get_num_triangles(0) if hasattr(m,'get_num_triangles') else "", m.get_bounds())
unreal.EditorAssetLibrary.save_directory(DST)
