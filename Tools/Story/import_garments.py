import unreal, glob, json, os, sys
# Runs INSIDE the editor via Tools/RoutePlanning/uepy (the script must not contain the text dot-p-y anywhere:
# the editor's file-exec mode then takes everything up to it for a file name).
# Imports the Blender garments (Tools/Blender/garments -> Art/Blender/Export/Garments/SKM_G_<outfit>.fbx + .json)
# as skeletal meshes on the MetaHuman body skeleton, makes M_Garment (two-sided cloth shading on the downloaded
# Poly Haven fabrics, CC0) and one MI per material slot from the json (fabric, tint, tiling, roughness).
at = unreal.AssetToolsHelpers.get_asset_tools(); EAL = unreal.EditorAssetLibrary; mel = unreal.MaterialEditingLibrary
GSRC = "F:/UE_project/YggdrasilsPeak/Art/Blender/Export/Garments"
FSRC = "F:/UE_project/YggdrasilsPeak/Downloads/polyhaven_fabric_2026-11-14"
DST = "/Game/Story/Garments"; TEX = DST + "/Tex"
ONLY = [a for a in sys.argv[1:] if not a.startswith("-")] if hasattr(sys, "argv") else []
SKEL = os.environ.get("YGG_GARMENT_SKEL", "")


def skeleton():
    for p in [SKEL, "/Game/Story/MetaHumans/Common/Female/Medium/NormalWeight/Body/metahuman_base_skel",
              "/MetaHumanCharacter/Female/Medium/NormalWeight/Body/metahuman_base_skel"]:
        if p and EAL.does_asset_exist(p): return unreal.load_asset(p)


def tex(name, kind):
    path = f"{TEX}/{name}"
    if not EAL.does_asset_exist(path):
        src = f"{GSRC}/{name}.png" if name in ("T_Pangden", "T_White_Linear") else f"{FSRC}/{name}.jpg"
        t = unreal.AssetImportTask(); t.filename = src; t.destination_path = TEX; t.automated = True; t.replace_existing = True; t.save = True
        at.import_asset_tasks([t])
        tx = unreal.load_asset(path)
        if kind == "nor":
            tx.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_NORMALMAP); tx.set_editor_property("srgb", False)
        elif kind == "rough":
            tx.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_MASKS); tx.set_editor_property("srgb", False)
        tx.set_editor_property("max_texture_size", 1024)     # cloth detail at NPC distance; keeps the laptop's VRAM free
        EAL.save_asset(path, False)
    return unreal.load_asset(path)


def master():   # (rough maps are linear masks: the default must be a linear white too, or the material fails to compile)
    path = f"{DST}/M_Garment"
    m = unreal.load_asset(path) or at.create_asset("M_Garment", DST, unreal.Material, unreal.MaterialFactoryNew())
    mel.delete_all_material_expressions(m)
    m.set_editor_property("two_sided", True); m.set_editor_property("used_with_skeletal_mesh", True)
    m.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_DEFAULT_LIT)
    E = lambda cls, x, y: mel.create_material_expression(m, cls, x, y)
    def scalar(n, v, x, y):
        e = E(unreal.MaterialExpressionScalarParameter, x, y); e.set_editor_property("parameter_name", n); e.set_editor_property("default_value", v); return e
    def vec(n, v, x, y):
        e = E(unreal.MaterialExpressionVectorParameter, x, y); e.set_editor_property("parameter_name", n); e.set_editor_property("default_value", unreal.LinearColor(*v)); return e
    def tsp(n, t, st, uv, x, y):
        e = E(unreal.MaterialExpressionTextureSampleParameter2D, x, y); e.set_editor_property("parameter_name", n)
        e.set_editor_property("texture", t); e.set_editor_property("sampler_type", st); mel.connect_material_expressions(uv, "", e, "UVs"); return e
    tc = E(unreal.MaterialExpressionTextureCoordinate, -1400, 0)
    tile = scalar("TileM", 0.25, -1400, 100); dtile = scalar("DetailTileM", 0.25, -1400, 200)
    uvc = E(unreal.MaterialExpressionDivide, -1200, 0); mel.connect_material_expressions(tc, "", uvc, "A"); mel.connect_material_expressions(tile, "", uvc, "B")
    uvd = E(unreal.MaterialExpressionDivide, -1200, 200); mel.connect_material_expressions(tc, "", uvd, "A"); mel.connect_material_expressions(dtile, "", uvd, "B")
    white = unreal.load_asset("/Engine/EngineResources/WhiteSquareTexture"); flat = unreal.load_asset("/Engine/EngineMaterials/FlatNormal")
    col = tsp("Color", white, unreal.MaterialSamplerType.SAMPLERTYPE_COLOR, uvc, -950, -200)
    nor = tsp("Normal", flat, unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL, uvd, -950, 150)
    rgh = tsp("Rough", tex("T_White_Linear", "rough"), unreal.MaterialSamplerType.SAMPLERTYPE_MASKS, uvd, -950, 450)
    tint = vec("Tint", (1, 1, 1, 1), -950, -420)
    vc = E(unreal.MaterialExpressionVertexColor, -950, -560)
    aos = scalar("AOStrength", 0.85, -950, -650)
    one = E(unreal.MaterialExpressionConstant, -800, -700); one.set_editor_property("r", 1.0)
    ao = E(unreal.MaterialExpressionLinearInterpolate, -650, -600)
    mel.connect_material_expressions(one, "", ao, "A"); mel.connect_material_expressions(vc, "R", ao, "B"); mel.connect_material_expressions(aos, "", ao, "Alpha")
    # colour = the designed tint x the fabric's light/dark detail only: the photo's own hue would shift the tint
    # (navy read green, olive read lime). Detail = luma(texel) / luma(texture average = its last mip).
    avg = E(unreal.MaterialExpressionTextureSampleParameter2D, -950, -330); avg.set_editor_property("parameter_name", "Color")
    avg.set_editor_property("texture", white); avg.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_COLOR)
    avg.set_editor_property("mip_value_mode", unreal.TextureMipValueMode.TMVM_MIP_LEVEL)
    mipc = E(unreal.MaterialExpressionConstant, -1150, -330); mipc.set_editor_property("r", 14.0)
    mel.connect_material_expressions(uvc, "", avg, "UVs"); mel.connect_material_expressions(mipc, "", avg, "MipValue")
    ds1 = E(unreal.MaterialExpressionDesaturation, -800, -260); mel.connect_material_expressions(col, "RGB", ds1, "")
    ds2 = E(unreal.MaterialExpressionDesaturation, -800, -360); mel.connect_material_expressions(avg, "RGB", ds2, "")
    mx = E(unreal.MaterialExpressionMax, -700, -360); lo = E(unreal.MaterialExpressionConstant, -800, -420); lo.set_editor_property("r", 0.02)
    mel.connect_material_expressions(ds2, "", mx, "A"); mel.connect_material_expressions(lo, "", mx, "B")
    ratio = E(unreal.MaterialExpressionDivide, -650, -300); mel.connect_material_expressions(ds1, "", ratio, "A"); mel.connect_material_expressions(mx, "", ratio, "B")
    detail = scalar("Detail", 1.0, -800, -480)
    one1 = E(unreal.MaterialExpressionConstant, -800, -520); one1.set_editor_property("r", 1.0)
    dl = E(unreal.MaterialExpressionLinearInterpolate, -560, -340)
    mel.connect_material_expressions(one1, "", dl, "A"); mel.connect_material_expressions(ratio, "", dl, "B"); mel.connect_material_expressions(detail, "", dl, "Alpha")
    usec = scalar("UseTexColor", 0.0, -800, -560)    # 1 = keep the texture's colours (the striped pangden)
    pick = E(unreal.MaterialExpressionLinearInterpolate, -480, -320)
    mel.connect_material_expressions(dl, "", pick, "A"); mel.connect_material_expressions(col, "RGB", pick, "B"); mel.connect_material_expressions(usec, "", pick, "Alpha")
    m1 = E(unreal.MaterialExpressionMultiply, -400, -300); mel.connect_material_expressions(pick, "", m1, "A"); mel.connect_material_expressions(tint, "", m1, "B")
    bc0 = E(unreal.MaterialExpressionMultiply, -450, -300); mel.connect_material_expressions(m1, "", bc0, "A"); mel.connect_material_expressions(ao, "", bc0, "B")
    # fuzz (wool, fleece, knit): fibres catch light at grazing angles -> lighten towards the rim
    fr = E(unreal.MaterialExpressionFresnel, -650, -150); fr.set_editor_property("exponent", 2.5)
    fz = scalar("Fuzz", 0.0, -650, -80)
    fa = E(unreal.MaterialExpressionMultiply, -450, -150); mel.connect_material_expressions(fr, "", fa, "A"); mel.connect_material_expressions(fz, "", fa, "B")
    lift = E(unreal.MaterialExpressionMultiply, -450, -60); k = E(unreal.MaterialExpressionConstant, -650, -20); k.set_editor_property("r", 1.8)
    mel.connect_material_expressions(bc0, "", lift, "A"); mel.connect_material_expressions(k, "", lift, "B")
    bc = E(unreal.MaterialExpressionLinearInterpolate, -250, -250)
    mel.connect_material_expressions(bc0, "", bc, "A"); mel.connect_material_expressions(lift, "", bc, "B"); mel.connect_material_expressions(fa, "", bc, "Alpha")
    mel.connect_material_property(bc, "", unreal.MaterialProperty.MP_BASE_COLOR)
    sp = scalar("Specular", 0.5, -450, 40); mel.connect_material_property(sp, "", unreal.MaterialProperty.MP_SPECULAR)
    # normal strength: lerp flat -> map
    fl = E(unreal.MaterialExpressionConstant3Vector, -650, 100); fl.set_editor_property("constant", unreal.LinearColor(0, 0, 1, 1))
    ns = scalar("NormalStrength", 1.0, -650, 250)
    nl = E(unreal.MaterialExpressionLinearInterpolate, -450, 150)
    mel.connect_material_expressions(fl, "", nl, "A"); mel.connect_material_expressions(nor, "RGB", nl, "B"); mel.connect_material_expressions(ns, "", nl, "Alpha")
    mel.connect_material_property(nl, "", unreal.MaterialProperty.MP_NORMAL)
    rs = scalar("RoughScale", 1.0, -650, 500)
    rm = E(unreal.MaterialExpressionMultiply, -450, 450); mel.connect_material_expressions(rgh, "R", rm, "A"); mel.connect_material_expressions(rs, "", rm, "B")
    mel.connect_material_property(rm, "", unreal.MaterialProperty.MP_ROUGHNESS)
    mel.recompile_material(m); EAL.save_loaded_asset(m, False)
    return m


FUZZ = {"caban": 0.7, "polar_fleece": 0.9, "knitted_fleece": 0.85, "poly_wool_herringbone": 0.6, "jersey_melange": 0.35,
        "rough_linen": 0.4, "@pangden": 0.5, "hessian_230": 0.3, "floral_jacquard": 0.3, "quatrefoil_jacquard_fabric": 0.2}
M = master()
sk_asset = skeleton()
print("skeleton", sk_asset.get_path_name() if sk_asset else None)
done = []
for js in sorted(glob.glob(f"{GSRC}/SKM_G_*.json")):
    meta = json.load(open(js)); name = os.path.splitext(os.path.basename(js))[0]
    if ONLY and meta["outfit"] not in ONLY and name not in ONLY: continue
    # material instances per slot
    mis = {}
    for slot, d in meta["slots"].items():
        mp = f"{DST}/Materials/MI_G_{slot}"
        mi = unreal.load_asset(mp) or at.create_asset(f"MI_G_{slot}", f"{DST}/Materials", unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
        mel.set_material_instance_parent(mi, M)
        fab = d["fabric"]
        if fab == "@pangden":
            mel.set_material_instance_texture_parameter_value(mi, "Color", tex("T_Pangden", "diff"))
            mel.set_material_instance_texture_parameter_value(mi, "Normal", tex("T_rough_linen_nor", "nor"))
            mel.set_material_instance_texture_parameter_value(mi, "Rough", tex("T_rough_linen_rough", "rough"))
            mel.set_material_instance_scalar_parameter_value(mi, "TileM", 1.0); mel.set_material_instance_scalar_parameter_value(mi, "DetailTileM", 0.04)
            mel.set_material_instance_scalar_parameter_value(mi, "UseTexColor", 1.0)
        elif fab:
            for k in ("diff", "nor", "rough"):
                mel.set_material_instance_texture_parameter_value(mi, {"diff": "Color", "nor": "Normal", "rough": "Rough"}[k], tex(f"T_{fab}_{k}", k))
            mel.set_material_instance_scalar_parameter_value(mi, "TileM", d["tile_m"]); mel.set_material_instance_scalar_parameter_value(mi, "DetailTileM", d["tile_m"])
        # Poly Haven colour maps carry their own hue: desaturate-by-tint keeps the designed colours (tint x ~1.6 since maps are mid-grey)
        tint = d["tint"]; gain = 1.0          # the material normalises the fabric to its average: tint = the real colour
        mel.set_material_instance_vector_parameter_value(mi, "Tint", unreal.LinearColor(tint[0] * gain, tint[1] * gain, tint[2] * gain, 1))
        mel.set_material_instance_scalar_parameter_value(mi, "RoughScale", d["rough"] / (0.7 if fab else 1.0))
        mel.set_material_instance_scalar_parameter_value(mi, "Fuzz", FUZZ.get(fab, 0.0))
        mel.set_material_instance_scalar_parameter_value(mi, "Specular", 0.25 if FUZZ.get(fab, 0) > 0.5 else 0.5)
        EAL.save_loaded_asset(mi, False); mis[slot] = mi
    # skeletal mesh
    t = unreal.AssetImportTask(); t.filename = f"{GSRC}/{name}.fbx"; t.destination_path = DST
    t.replace_existing = True; t.automated = True; t.save = True
    o = unreal.FbxImportUI(); o.import_mesh = True; o.import_as_skeletal = True; o.import_animations = False
    o.mesh_type_to_import = unreal.FBXImportType.FBXIT_SKELETAL_MESH
    o.skeleton = sk_asset; o.create_physics_asset = False; o.import_materials = False; o.import_textures = False
    o.skeletal_mesh_import_data.set_editor_property("vertex_color_import_option", unreal.VertexColorImportOption.REPLACE)
    o.skeletal_mesh_import_data.set_editor_property("import_morph_targets", False)
    t.options = o
    at.import_asset_tasks([t])
    sk = unreal.load_asset(f"{DST}/{name}")
    if not sk:
        print("IMPORT FAILED", name); continue
    mats = list(sk.get_editor_property("materials"))
    for sm in mats:
        n = str(sm.material_slot_name)
        if n in mis: sm.material_interface = mis[n]
    sk.set_editor_property("materials", mats)
    EAL.save_asset(f"{DST}/{name}", False)
    b = sk.get_bounds().box_extent
    print("GARMENT", name, "skel", sk.skeleton.get_name(), "size cm", round(b.x * 2), round(b.y * 2), round(b.z * 2),
          "slots", [(str(s.material_slot_name), s.material_interface.get_name() if s.material_interface else None) for s in mats])
    done.append(name)
print("garments imported", done)
