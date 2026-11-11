# Runs INSIDE the editor (uepy.py). The route detail tiles were imported from OBJ files without normals, so every
# 1 m triangle got its own flat normal: wherever two triangles meet at a slight angle the snow shows a crisp light/dark
# line (the "straight lines" on the glacier, 2026-11-11). Gives every tile one smooth normal per vertex (Geometry
# Script set_per_vertex_normals), keeping materials and Nanite. Tiles listed in %TEMP%/ygg/smooth_tiles.txt
# (one asset name per line, e.g. SM_R_10_25); done names are appended to smooth_done.txt so a batch can resume.
import unreal, os
D = "/Game/Environment/DetailTiles/Route"
T = os.path.join(os.environ.get("TEMP", "C:/Temp"), "ygg")
todo = [l.strip() for l in open(os.path.join(T, "smooth_tiles.txt")) if l.strip()]
done_path = os.path.join(T, "smooth_done.txt")
done = set(l.strip() for l in open(done_path)) if os.path.exists(done_path) else set()
AU, N = unreal.GeometryScript_AssetUtils, unreal.GeometryScript_Normals
n = 0
for name in todo:
    if name in done:
        continue
    sm = unreal.load_asset(f"{D}/{name}")
    if not sm:
        continue
    dm = unreal.DynamicMesh()
    ro = unreal.GeometryScriptCopyMeshFromAssetOptions()
    ro.set_editor_property("apply_build_settings", False)
    ro.set_editor_property("request_tangents", False)
    dm, outcome = AU.copy_mesh_from_static_mesh(sm, dm, ro, unreal.GeometryScriptMeshReadLOD())
    if outcome != unreal.GeometryScriptOutcomePins.SUCCESS:
        print("read failed", name); continue
    N.set_per_vertex_normals(dm)
    wo = unreal.GeometryScriptCopyMeshToAssetOptions()
    wo.set_editor_property("enable_recompute_normals", False)
    wo.set_editor_property("enable_recompute_tangents", True)
    wo.set_editor_property("replace_materials", False)
    wo.set_editor_property("apply_nanite_settings", False)
    wo.set_editor_property("defer_mesh_post_edit_change", False)
    dm, outcome = AU.copy_mesh_to_static_mesh(dm, sm, wo, unreal.GeometryScriptMeshWriteLOD(), True)
    if outcome != unreal.GeometryScriptOutcomePins.SUCCESS:
        print("write failed", name); continue
    unreal.EditorAssetLibrary.save_loaded_asset(sm, False)
    open(done_path, "a").write(name + "\n")
    n += 1
    dm = None
print("smoothed", n)
