# Runs INSIDE the editor. Imports the downloaded Poly Haven texture sets (Downloads/polyhaven_room_2026-11-14, CC0) and
# makes M_StoryPBR: world-space triplanar PBR (tiles at real scale on any box) + MI_Story_PBR_* for the chapter 1 room.
import unreal, glob, os
at = unreal.AssetToolsHelpers.get_asset_tools(); EAL = unreal.EditorAssetLibrary; mel = unreal.MaterialEditingLibrary
SRC = "F:/UE_project/YggdrasilsPeak/Downloads/polyhaven_room_2026-11-14"
TD = "/Game/Story/PBR"; MD = "/Game/Story/Materials"
tasks = []
for f in sorted(glob.glob(f"{SRC}/T_*.jpg")):
    t = unreal.AssetImportTask(); t.filename = f; t.destination_path = TD; t.automated = True; t.replace_existing = True; t.save = True
    tasks.append(t)
at.import_asset_tasks(tasks)
for f in sorted(glob.glob(f"{SRC}/T_*.jpg")):
    n = os.path.splitext(os.path.basename(f))[0]
    tex = unreal.load_asset(f"{TD}/{n}")
    if n.endswith("_nor"):
        tex.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_NORMALMAP); tex.set_editor_property("srgb", False)
    elif n.endswith("_rough"):
        tex.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_MASKS); tex.set_editor_property("srgb", False)
    EAL.save_asset(f"{TD}/{n}", False)

m = unreal.load_asset(f"{MD}/M_StoryPBR") or at.create_asset("M_StoryPBR", MD, unreal.Material, unreal.MaterialFactoryNew())
mel.delete_all_material_expressions(m)
m.set_editor_property("tangent_space_normal", False); m.set_editor_property("used_with_nanite", True)
c = mel.create_material_expression(m, unreal.MaterialExpressionCustom, -400, 0)
c.set_editor_property("code", """#if RAYHITGROUPSHADER
#define SMP(T, U) T.SampleLevel(T##Sampler, U, 0)
#else
#define SMP(T, U) T.Sample(T##Sampler, U)
#endif
float3 n = normalize(N);
float3 w = pow(abs(n), 6.0); w /= (w.x + w.y + w.z);
float2 ux = WP.yz / Scale, uy = WP.xz / Scale, uz = WP.xy / Scale;
float3 cx = SMP(C, ux).rgb, cy = SMP(C, uy).rgb, cz = SMP(C, uz).rgb;
Rough = dot(float3(SMP(R, ux).r, SMP(R, uy).r, SMP(R, uz).r), w);
Nrm = n;   // TODO: proper triplanar normal blend (the first try drew black); colour + roughness carry the look
return (cx * w.x + cy * w.y + cz * w.z) * Tint;""")
c.set_editor_property("output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT3)
ins = []
for nm in ["WP", "N", "C", "Nm", "R", "Scale", "Tint", "Str"]:
    i = unreal.CustomInput(); i.set_editor_property("input_name", nm); ins.append(i)
c.set_editor_property("inputs", ins)
outs = []
for nm, tp in (("Rough", unreal.CustomMaterialOutputType.CMOT_FLOAT1), ("Nrm", unreal.CustomMaterialOutputType.CMOT_FLOAT3)):
    o = unreal.CustomOutput(); o.set_editor_property("output_name", nm); o.set_editor_property("output_type", tp); outs.append(o)
c.set_editor_property("additional_outputs", outs)
wp = mel.create_material_expression(m, unreal.MaterialExpressionWorldPosition, -800, -200)
vn = mel.create_material_expression(m, unreal.MaterialExpressionVertexNormalWS, -800, -120)
def tparam(name, y, default):
    p = mel.create_material_expression(m, unreal.MaterialExpressionTextureObjectParameter, -800, y); p.set_editor_property("parameter_name", name)
    p.set_editor_property("texture", unreal.load_asset(default)); return p
tc = tparam("Color", -40, f"{TD}/T_plastered_stone_wall_diff"); tn = tparam("Normal", 60, f"{TD}/T_plastered_stone_wall_nor"); tr = tparam("Rough", 160, f"{TD}/T_plastered_stone_wall_rough")
tn.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL); tr.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_MASKS)
sc = mel.create_material_expression(m, unreal.MaterialExpressionScalarParameter, -800, 260); sc.set_editor_property("parameter_name", "TileCm"); sc.set_editor_property("default_value", 200.0)
ti = mel.create_material_expression(m, unreal.MaterialExpressionVectorParameter, -800, 340); ti.set_editor_property("parameter_name", "Tint"); ti.set_editor_property("default_value", unreal.LinearColor(1, 1, 1, 1))
st = mel.create_material_expression(m, unreal.MaterialExpressionScalarParameter, -800, 420); st.set_editor_property("parameter_name", "NormalStrength"); st.set_editor_property("default_value", 1.0)
for e, nm in [(wp, "WP"), (vn, "N"), (tc, "C"), (tn, "Nm"), (tr, "R"), (sc, "Scale"), (ti, "Tint"), (st, "Str")]:
    mel.connect_material_expressions(e, "", c, nm)
mel.connect_material_property(c, "", unreal.MaterialProperty.MP_BASE_COLOR)
mel.connect_material_property(c, "Rough", unreal.MaterialProperty.MP_ROUGHNESS)
mel.connect_material_property(c, "Nrm", unreal.MaterialProperty.MP_NORMAL)
mel.recompile_material(m); EAL.save_loaded_asset(m, False)

SETS = {"PBR_Wall": ("plastered_stone_wall", 220.0, (0.92, 0.88, 0.82)), "PBR_Floor": ("old_wood_floor", 180.0, (0.8, 0.72, 0.62)),
        "PBR_Wood": ("dark_planks", 140.0, (0.85, 0.78, 0.7)), "PBR_Roof": ("corrugated_iron", 160.0, (0.55, 0.75, 1.15)),
        "PBR_StoneOut": ("plastered_stone_wall", 260.0, (0.78, 0.76, 0.72))}
for name, (tex, tile, tint) in SETS.items():
    path = f"{MD}/MI_Story_{name}"
    mi = unreal.load_asset(path) or at.create_asset(f"MI_Story_{name}", MD, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    mel.set_material_instance_parent(mi, m)
    mel.set_material_instance_texture_parameter_value(mi, "Color", unreal.load_asset(f"{TD}/T_{tex}_diff"))
    mel.set_material_instance_texture_parameter_value(mi, "Normal", unreal.load_asset(f"{TD}/T_{tex}_nor"))
    mel.set_material_instance_texture_parameter_value(mi, "Rough", unreal.load_asset(f"{TD}/T_{tex}_rough"))
    mel.set_material_instance_scalar_parameter_value(mi, "TileCm", tile)
    mel.set_material_instance_vector_parameter_value(mi, "Tint", unreal.LinearColor(*tint, 1))
    EAL.save_loaded_asset(mi, False)
print("pbr done", len(SETS))
