# Runs INSIDE the editor (Tools/RoutePlanning/uepy). 2026-11-14, snow step P2 + P3 (Docs/Development/Snow_Diagnosis_2026-11-14.md):
#   - imports our own snow detail maps (Tools/Materials/make_snow_detail.py, no downloads): wind-packed crust grain
#     (2 m tile: 1-3 mm grain, 1-3 cm crust, 2-10 cm plates with crisp broken edges) and wind ripples (3 m tile)
#   - the snow's texture slots get the crust instead of ClumpSnow (its round lumps read as a grid of bumps, #1)
#   - SnowTileCm 600 -> 200 (the crust's real size); the 3.7x layer B then carries 7-37 cm plates
#   - route tiles: the snow gets real height like the rock (SnowDisplacementCm 0 -> 3 = -0.8..+1.5 cm), read at mip 3
#     (8 mm, about the size of the micro-triangles near the eye) on the same cell layout as the colour node
#   - glints: about 6x fewer and only within ~2-3 m (the first version sprinkled bright dots over every surface)
import unreal

MEL = unreal.MaterialEditingLibrary
EAL = unreal.EditorAssetLibrary
SRC = "F:/UE_project/YggdrasilsPeak/Saved/SnowTex/"
DST = "/Game/Environment/Snow/Textures"

# ---- 1. import
tasks = []
for name in ["T_SnowFine_N", "T_SnowFine_HRA", "T_SnowRipple_N", "T_SnowRipple_H"]:
    t = unreal.AssetImportTask()
    t.set_editor_property("filename", SRC + name + ".png")
    t.set_editor_property("destination_path", DST)
    t.set_editor_property("destination_name", name)
    t.set_editor_property("replace_existing", True)
    t.set_editor_property("automated", True)
    t.set_editor_property("save", False)
    tasks.append(t)
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)
tex = {}
for name in ["T_SnowFine_N", "T_SnowFine_HRA", "T_SnowRipple_N", "T_SnowRipple_H"]:
    a = unreal.load_asset(DST + "/" + name)
    if not a:
        print("IMPORT FAILED", name); continue
    normal = name.endswith("_N")
    a.set_editor_property("srgb", False)
    a.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_NORMALMAP if normal else
                          (unreal.TextureCompressionSettings.TC_MASKS if name.endswith("HRA") else unreal.TextureCompressionSettings.TC_GRAYSCALE))
    a.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_WORLD_NORMAL_MAP if normal else unreal.TextureGroup.TEXTUREGROUP_WORLD)
    a.set_editor_property("flip_green_channel", False)    # the maps are DirectX convention, as UE expects
    a.set_editor_property("max_texture_size", 2048)
    EAL.save_loaded_asset(a, False)
    tex[name] = a
    print("imported", name, a.blueprint_get_size_x(), a.blueprint_get_size_y())

SWAP = {"ClumpSnow_Height": tex.get("T_SnowFine_HRA"), "ClumpSnow_Normal": tex.get("T_SnowFine_N")}

def fix_crlf(code, a, b):
    if "\r\n" in code:
        a = a.replace("\r\n", "\n").replace("\n", "\r\n"); b = b.replace("\r\n", "\n").replace("\n", "\r\n")
    return a, b

def patch(node, repl):
    code = node.get_editor_property("code"); res = []
    for a, b in repl:
        a, b = fix_crlf(code, a, b)
        if a in code: code = code.replace(a, b); res.append("ok")
        elif b in code: res.append("already")
        else: res.append("MISSING")
    node.set_editor_property("code", code)
    return res

GLINT = [
    ("float spark=step(0.985,float(gh&0xffffu)/65536.0)*step(0.55,dot(Hs,n))*(1.0-smoothstep(0.6,1.6,fp))*(1.0-ice);",
     "float spark=step(0.9975,float(gh&0xffffu)/65536.0)*step(0.75,dot(Hs,n))*(1.0-smoothstep(0.35,0.9,fp))*(1.0-ice); // rare, near only (v1 sprinkled dots everywhere)"),
    ("sRough=lerp(sRough,0.06,spark);", "sRough=lerp(sRough,0.12,spark);"),
]
DISP = [
    ("float2 Xf(float2 uv, float2 q, float2 o){ return Rot(uv*exp2(lerp(-1.5,1.5,q.y)), q.x*6.2831853)+o; }",
     "float2 Xf(float2 uv, float2 q, float2 o){ return Rot(uv*exp2(lerp(-0.5,0.5,q.y)), q.x*6.2831853)+o; }"),
    ("hx.Cell(uvS+53.1,sw,p1,p2,p3,r1,r2,r3);", "hx.Cell(uvS*1.8+53.1,sw,p1,p2,p3,r1,r2,r3); // same cells as the colour node (2026-11-14)"),
    ("float3 sh3=float3(SnowH.SampleLevel(SnowHSampler,hx.Xf(uvS,r1,p1),0).r,SnowH.SampleLevel(SnowHSampler,hx.Xf(uvS,r2,p2),0).r,SnowH.SampleLevel(SnowHSampler,hx.Xf(uvS,r3,p3),0).r);",
     "float3 sh3=float3(SnowH.SampleLevel(SnowHSampler,hx.Xf(uvS,r1,p1),3).r,SnowH.SampleLevel(SnowHSampler,hx.Xf(uvS,r2,p2),3).r,SnowH.SampleLevel(SnowHSampler,hx.Xf(uvS,r3,p3),3).r); // mip 3 = 8 mm"),
]

for path, disp_cm in [("/Game/Materials/M_EverestDetailTile", 3.0), ("/Game/Materials/M_EverestLandscape", None)]:
    m = unreal.load_asset(path)
    exprs = [e for e in unreal.ObjectIterator(unreal.MaterialExpression) if e.get_outer() == m]
    swapped = 0
    for e in exprs:
        if isinstance(e, (unreal.MaterialExpressionTextureBase,)):
            t = e.get_editor_property("texture")
            if t and t.get_name() in SWAP and SWAP[t.get_name()]:
                new = SWAP[t.get_name()]
                e.set_editor_property("texture", new); swapped += 1
                # the node's sampler type must match the new texture's compression or the material fails to compile
                e.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL if new.get_name().endswith("_N")
                                      else unreal.MaterialSamplerType.SAMPLERTYPE_MASKS)
        if isinstance(e, unreal.MaterialExpressionScalarParameter):
            pn = str(e.get_editor_property("parameter_name"))
            if pn == "SnowTileCm":
                e.set_editor_property("default_value", 200.0); print(path, "SnowTileCm -> 200")
            if pn == "SnowDisplacementCm" and disp_cm is not None:
                e.set_editor_property("default_value", disp_cm); print(path, "SnowDisplacementCm ->", disp_cm)
    print(path, "texture slots swapped", swapped)
    customs = [e for e in exprs if isinstance(e, unreal.MaterialExpressionCustom)]
    for c in customs:
        code = c.get_editor_property("code")
        if "---- Snow (2026-10-22)" in code:
            print(path, "glints", patch(c, GLINT))
        if disp_cm is not None and "snow: none (SnowDisplacementCm 0" in code:
            print(path, "displacement", patch(c, DISP))
    MEL.recompile_material(m)
    EAL.save_loaded_asset(m, False)
    print(path, "recompiled + saved")
