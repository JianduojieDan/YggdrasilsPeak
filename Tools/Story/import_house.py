import unreal, glob, json, os
# Runs INSIDE the editor via Tools/RoutePlanning/uepy (no dot-p-y text in this file). Imports the Blender Sherpa house
# (Tools/Blender/sherpa_house -> Art/Blender/Export/House/SM_House + SM_HouseProps FBX, slots json, rug texture) into
# /Game/Story/House: textures from Downloads/polyhaven_house_2026-11-14 (CC0), M_StorySurface (UV in metres, colour =
# tint x texture luminance detail, vertex R = shade/soot, metal, emissive), one MI per slot, Nanite meshes.
at = unreal.AssetToolsHelpers.get_asset_tools(); EAL = unreal.EditorAssetLibrary; mel = unreal.MaterialEditingLibrary
SRC = "F:/UE_project/YggdrasilsPeak/Art/Blender/Export/House"
TSRC = "F:/UE_project/YggdrasilsPeak/Downloads/polyhaven_house_2026-11-14"
DST = "/Game/Story/House"; TEX = DST + "/Tex"; MAT = DST + "/Materials"


def tex(name, kind):
    path = f"{TEX}/{name}"
    if not EAL.does_asset_exist(path):
        src = next((p for p in (f"{SRC}/{name}.png", f"{TSRC}/{name}.jpg", f"{TSRC}/{name}.png",
                                f"F:/UE_project/YggdrasilsPeak/Art/Blender/Export/Garments/{name}.png") if os.path.exists(p)), None)
        if not src: print("missing texture", name); return None
        t = unreal.AssetImportTask(); t.filename = src; t.destination_path = TEX; t.automated = True; t.replace_existing = True; t.save = True
        at.import_asset_tasks([t])
        tx = unreal.load_asset(path)
        if kind == "nor":
            tx.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_NORMALMAP); tx.set_editor_property("srgb", False)
        elif kind in ("rough", "lin"):
            tx.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_MASKS); tx.set_editor_property("srgb", False)
        tx.set_editor_property("max_texture_size", 2048)
        EAL.save_asset(path, False)
    return unreal.load_asset(path)


def master():
    path = f"{DST}/M_StorySurface"
    m = unreal.load_asset(path) or at.create_asset("M_StorySurface", DST, unreal.Material, unreal.MaterialFactoryNew())
    mel.delete_all_material_expressions(m)
    m.set_editor_property("used_with_nanite", True); m.set_editor_property("used_with_static_lighting", True)
    E = lambda cls, x, y: mel.create_material_expression(m, cls, x, y)
    def scalar(n, v, x, y):
        e = E(unreal.MaterialExpressionScalarParameter, x, y); e.set_editor_property("parameter_name", n); e.set_editor_property("default_value", v); return e
    def vec(n, v, x, y):
        e = E(unreal.MaterialExpressionVectorParameter, x, y); e.set_editor_property("parameter_name", n); e.set_editor_property("default_value", unreal.LinearColor(*v)); return e
    def tsp(n, t, st, uv, x, y, mip=None):
        e = E(unreal.MaterialExpressionTextureSampleParameter2D, x, y); e.set_editor_property("parameter_name", n)
        e.set_editor_property("texture", t); e.set_editor_property("sampler_type", st); mel.connect_material_expressions(uv, "", e, "UVs")
        if mip is not None:
            e.set_editor_property("mip_value_mode", unreal.TextureMipValueMode.TMVM_MIP_LEVEL)
            mc = E(unreal.MaterialExpressionConstant, x - 150, y + 60); mc.set_editor_property("r", mip); mel.connect_material_expressions(mc, "", e, "MipValue")
        return e
    tc = E(unreal.MaterialExpressionTextureCoordinate, -1500, 0)
    tile = scalar("TileM", 1.0, -1500, 100)
    uv = E(unreal.MaterialExpressionDivide, -1300, 0); mel.connect_material_expressions(tc, "", uv, "A"); mel.connect_material_expressions(tile, "", uv, "B")
    white = unreal.load_asset("/Engine/EngineResources/WhiteSquareTexture"); flat = unreal.load_asset("/Engine/EngineMaterials/FlatNormal")
    linw = tex("T_White_Linear", "lin")
    col = tsp("Color", white, unreal.MaterialSamplerType.SAMPLERTYPE_COLOR, uv, -1050, -300)
    avg = tsp("Color", white, unreal.MaterialSamplerType.SAMPLERTYPE_COLOR, uv, -1050, -450, 14.0)
    nor = tsp("Normal", flat, unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL, uv, -1050, 150)
    rgh = tsp("Rough", linw, unreal.MaterialSamplerType.SAMPLERTYPE_MASKS, uv, -1050, 450)
    ds1 = E(unreal.MaterialExpressionDesaturation, -850, -300); mel.connect_material_expressions(col, "RGB", ds1, "")
    ds2 = E(unreal.MaterialExpressionDesaturation, -850, -450); mel.connect_material_expressions(avg, "RGB", ds2, "")
    mx = E(unreal.MaterialExpressionMax, -750, -450); lo = E(unreal.MaterialExpressionConstant, -850, -520); lo.set_editor_property("r", 0.02)
    mel.connect_material_expressions(ds2, "", mx, "A"); mel.connect_material_expressions(lo, "", mx, "B")
    ratio = E(unreal.MaterialExpressionDivide, -650, -350); mel.connect_material_expressions(ds1, "", ratio, "A"); mel.connect_material_expressions(mx, "", ratio, "B")
    # keep some of the photo's own hue (stone lichen, wood grain colour): lerp(luma detail, real colour / its average, Hue)
    ratio_c = E(unreal.MaterialExpressionDivide, -650, -250); mel.connect_material_expressions(col, "RGB", ratio_c, "A"); mel.connect_material_expressions(mx, "", ratio_c, "B")
    hue = scalar("Hue", 0.5, -650, -150)
    mixc = E(unreal.MaterialExpressionLinearInterpolate, -500, -300)
    mel.connect_material_expressions(ratio, "", mixc, "A"); mel.connect_material_expressions(ratio_c, "", mixc, "B"); mel.connect_material_expressions(hue, "", mixc, "Alpha")
    usec = scalar("UseTexColor", 0.0, -650, -600)
    pick = E(unreal.MaterialExpressionLinearInterpolate, -400, -350)
    mel.connect_material_expressions(mixc, "", pick, "A"); mel.connect_material_expressions(col, "RGB", pick, "B"); mel.connect_material_expressions(usec, "", pick, "Alpha")
    tint = vec("Tint", (1, 1, 1, 1), -650, -700)
    m1 = E(unreal.MaterialExpressionMultiply, -300, -400); mel.connect_material_expressions(pick, "", m1, "A"); mel.connect_material_expressions(tint, "", m1, "B")
    vc = E(unreal.MaterialExpressionVertexColor, -650, -800)
    shade = E(unreal.MaterialExpressionMultiply, -200, -400); mel.connect_material_expressions(m1, "", shade, "A"); mel.connect_material_expressions(vc, "R", shade, "B")
    mel.connect_material_property(shade, "", unreal.MaterialProperty.MP_BASE_COLOR)
    fl = E(unreal.MaterialExpressionConstant3Vector, -650, 100); fl.set_editor_property("constant", unreal.LinearColor(0, 0, 1, 1))
    ns = scalar("NormalStrength", 1.0, -650, 250)
    nl = E(unreal.MaterialExpressionLinearInterpolate, -450, 150)
    mel.connect_material_expressions(fl, "", nl, "A"); mel.connect_material_expressions(nor, "RGB", nl, "B"); mel.connect_material_expressions(ns, "", nl, "Alpha")
    mel.connect_material_property(nl, "", unreal.MaterialProperty.MP_NORMAL)
    rs = scalar("RoughScale", 1.0, -650, 500)
    rm = E(unreal.MaterialExpressionMultiply, -450, 450); mel.connect_material_expressions(rgh, "R", rm, "A"); mel.connect_material_expressions(rs, "", rm, "B")
    mel.connect_material_property(rm, "", unreal.MaterialProperty.MP_ROUGHNESS)
    met = scalar("Metallic", 0.0, -450, 560); mel.connect_material_property(met, "", unreal.MaterialProperty.MP_METALLIC)
    emi = scalar("Emissive", 0.0, -450, 640)
    em = E(unreal.MaterialExpressionMultiply, -250, 600); mel.connect_material_expressions(m1, "", em, "A"); mel.connect_material_expressions(emi, "", em, "B")
    mel.connect_material_property(em, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    mel.recompile_material(m); EAL.save_loaded_asset(m, False)
    return m


M = master()
slots = json.load(open(f"{SRC}/slots.json"))
mis = {}
for name, d in slots.items():
    mp = f"{MAT}/MI_H_{name}"
    mi = unreal.load_asset(mp) or at.create_asset(f"MI_H_{name}", MAT, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    mel.set_material_instance_parent(mi, M)
    fab = d["fabric"]
    if fab == "@rug":
        mel.set_material_instance_texture_parameter_value(mi, "Color", tex("T_TibetanRug", "diff"))
        mel.set_material_instance_scalar_parameter_value(mi, "UseTexColor", 1.0)
        mel.set_material_instance_texture_parameter_value(mi, "Normal", tex("T_rough_linen_nor", "nor") or unreal.load_asset("/Engine/EngineMaterials/FlatNormal"))
    elif fab:
        mel.set_material_instance_texture_parameter_value(mi, "Color", tex(f"T_{fab}_diff", "diff"))
        mel.set_material_instance_texture_parameter_value(mi, "Normal", tex(f"T_{fab}_nor", "nor"))
        mel.set_material_instance_texture_parameter_value(mi, "Rough", tex(f"T_{fab}_rough", "rough"))
    mel.set_material_instance_scalar_parameter_value(mi, "TileM", d["tile_m"])
    t = d["tint"]; mel.set_material_instance_vector_parameter_value(mi, "Tint", unreal.LinearColor(t[0], t[1], t[2], 1))
    mel.set_material_instance_scalar_parameter_value(mi, "RoughScale", d["rough"] / (0.7 if fab and fab != "@rug" else 1.0))
    mel.set_material_instance_scalar_parameter_value(mi, "Metallic", d.get("metal", 0.0))
    mel.set_material_instance_scalar_parameter_value(mi, "Emissive", d.get("emissive", 0.0))
    # how much of the photo's own hue survives: a little lichen on grey stone, more grain colour in wood
    hue = {"lichen_rock": 0.3, "gray_rocks": 0.3, "rough_wood": 0.6, "bark_brown_02": 0.6, "old_wooden_floor_02": 0.55, "weathered_planks": 0.45}.get(fab, 0.2)
    mel.set_material_instance_scalar_parameter_value(mi, "Hue", hue)
    EAL.save_loaded_asset(mi, False); mis[name] = mi

done = []
for fbx in ("SM_House", "SM_HouseProps"):
    t = unreal.AssetImportTask(); t.filename = f"{SRC}/{fbx}.fbx"; t.destination_path = DST
    t.replace_existing = True; t.automated = True; t.save = True
    o = unreal.FbxImportUI(); o.import_mesh = True; o.import_as_skeletal = False; o.import_animations = False
    o.mesh_type_to_import = unreal.FBXImportType.FBXIT_STATIC_MESH; o.import_materials = False; o.import_textures = False
    sd = o.static_mesh_import_data
    sd.set_editor_property("combine_meshes", False); sd.set_editor_property("auto_generate_collision", False)
    sd.set_editor_property("vertex_color_import_option", unreal.VertexColorImportOption.REPLACE)
    sd.set_editor_property("generate_lightmap_u_vs", False)
    t.options = o
    at.import_asset_tasks([t])
    for p in t.get_editor_property("imported_object_paths") or []:
        a = unreal.load_asset(p)
        if not isinstance(a, unreal.StaticMesh): continue
        ns = a.get_editor_property("nanite_settings"); ns.set_editor_property("enabled", True); a.set_editor_property("nanite_settings", ns)
        mats = list(a.get_editor_property("static_materials"))
        for sm in mats:
            n = str(sm.material_slot_name)
            if n in mis: sm.material_interface = mis[n]
        a.set_editor_property("static_materials", mats)
        EAL.save_asset(p.split(".")[0], False)
        b = a.get_bounds().box_extent
        done.append((p.split(".")[0].split("/")[-1], round(b.x * 2), round(b.y * 2), round(b.z * 2)))
print("house meshes", done)
