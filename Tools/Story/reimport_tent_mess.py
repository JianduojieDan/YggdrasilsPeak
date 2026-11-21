import unreal, json, struct, random
# Runs INSIDE the editor via Tools/RoutePlanning/uepy. 2026-11-21 (#75 mess dome "scrambled"): the dome's colours were
# patchy even unlit, while the same Art/Meshy/tent_mess.glb renders clean in Blender -> the UE asset's UVs did not match
# the current glb (likely imported from an older prep pass). Measures how many UE vertex UVs match the glb at the same
# position, re-imports the current glb in place (same paths, so the actor and MI_tent_mess_warm keep working), measures
# again, restores the import_ai_assets settings (complex collision, two-sided) and the MI's normal map.
ROOT = "F:/UE_project/YggdrasilsPeak"
EAL = unreal.EditorAssetLibrary
at = unreal.AssetToolsHelpers.get_asset_tools()
MESH = "/Game/Story/AI/tent_mess/tent_mess/StaticMeshes/tent_mess"

b = open(f"{ROOT}/Art/Meshy/tent_mess.glb", "rb").read()
jl = struct.unpack("<I", b[12:16])[0]; J = json.loads(b[20:20 + jl]); off = 20 + jl
bl = struct.unpack("<I", b[off:off + 4])[0]; BIN = b[off + 8: off + 8 + bl]
pr = J["meshes"][0]["primitives"][0]


def read(ai, n):
    a = J["accessors"][ai]; bv = J["bufferViews"][a["bufferView"]]; o = bv.get("byteOffset", 0) + a.get("byteOffset", 0)
    st = bv.get("byteStride", 4 * n)
    return [struct.unpack_from("<" + "f" * n, BIN, o + i * st) for i in range(a["count"])]


GP = read(pr["attributes"]["POSITION"], 3); GT = read(pr["attributes"]["TEXCOORD_0"], 2)
ref = {}
for p, t in zip(GP, GT):
    ref.setdefault((round(p[0] * 100), round(p[2] * 100), round(p[1] * 100)), []).append(t)   # UE (x,y,z) = 100 (gx, gz, gy)


def match_pct():
    sm = unreal.load_asset(MESH); d = sm.get_static_mesh_description(0)
    n = d.get_vertex_instance_count(); random.seed(5); ok = tot = 0
    for i in random.sample(range(n), min(3000, n)):
        vi = unreal.VertexInstanceID(i); p = d.get_vertex_position(d.get_vertex_instance_vertex(vi)); uv = d.get_vertex_instance_uv(vi, 0)
        c = ref.get((round(p.x), round(p.y), round(p.z)))
        if c is None: continue
        tot += 1
        if any(abs(uv.x - t[0]) < 0.003 and abs(uv.y - t[1]) < 0.003 for t in c): ok += 1
    return ok, tot


print("BEFORE uv match", match_pct())
t = unreal.AssetImportTask()
for k, v in (("filename", f"{ROOT}/Art/Meshy/tent_mess.glb"), ("destination_path", "/Game/Story/AI/tent_mess"), ("automated", True),
             ("replace_existing", True), ("save", True)):
    t.set_editor_property(k, v)
at.import_asset_tasks([t])
print("imported", list(t.get_editor_property("imported_object_paths") or [])[:6])
sm = unreal.load_asset(MESH)
bs = sm.get_editor_property("body_setup")
if bs: bs.set_editor_property("collision_trace_flag", unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE)
for k in range(len(sm.get_editor_property("static_materials"))):
    m = sm.get_material(k)
    if isinstance(m, unreal.MaterialInstanceConstant):
        ov = m.get_editor_property("base_property_overrides")
        ov.set_editor_property("override_two_sided", True); ov.set_editor_property("two_sided", True)
        m.set_editor_property("base_property_overrides", ov)
        unreal.MaterialEditingLibrary.update_material_instance(m); EAL.save_loaded_asset(m, False)
EAL.save_loaded_asset(sm, False)
bb = sm.get_bounding_box()
print("size", round(bb.max.x - bb.min.x), round(bb.max.y - bb.min.y), round(bb.max.z - bb.min.z), "min_z", round(bb.min.z, 1),
      "nanite", sm.get_editor_property("nanite_settings").enabled)
print("AFTER uv match", match_pct())
mi = unreal.load_asset("/Game/Story/AI/tent_mess/tent_mess/MI_tent_mess_warm")
print("MI parent", mi.get_editor_property("parent").get_path_name())
nt = unreal.load_asset("/Game/Story/AI/tent_mess/tent_mess/Textures/normal")
if nt:
    unreal.MaterialEditingLibrary.set_material_instance_texture_parameter_value(mi, "NormalTexture", nt)
    unreal.MaterialEditingLibrary.update_material_instance(mi); EAL.save_loaded_asset(mi, False)
print("done")
