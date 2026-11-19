# Runs INSIDE the editor: M_PrayerFlag (masked, two-sided, worn cloth, free edge swings in the wind) + one muted instance per colour
import unreal
mel = unreal.MaterialEditingLibrary; at = unreal.AssetToolsHelpers.get_asset_tools()
D = "/Game/Environment/Landmarks"
m = unreal.load_asset(f"{D}/M_PrayerFlag") or at.create_asset("M_PrayerFlag", D, unreal.Material, unreal.MaterialFactoryNew())
mel.delete_all_material_expressions(m)
m.set_editor_property("blend_mode", unreal.BlendMode.BLEND_MASKED); m.set_editor_property("two_sided", True)
m.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_TWO_SIDED_FOLIAGE)   # sunlight glows through the cloth
# 2026-11-20: the cloth is a real printed lungta texture (Art/PrayerFlags/T_PrayerFlag_<colour>.png, from one Meshy image of five
# flags: wind horse, four animals, mantra rows, frayed bottom threads in the alpha) instead of the old procedural print and
# spiky edge (user: "the flags look fake"). Color stays as a tint (white = as printed).
TD = f"{D}/Flags"
TEX = {}
for k in ("Blue", "White", "Red", "Green", "Yellow"):
    tk = unreal.AssetImportTask(); tk.filename = f"F:/UE_project/YggdrasilsPeak/Art/PrayerFlags/T_PrayerFlag_{k}.png"
    tk.destination_path = TD; tk.automated = True; tk.replace_existing = True; tk.save = True
    at.import_asset_tasks([tk])
    tx = unreal.load_asset(f"{TD}/T_PrayerFlag_{k}")
    tx.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_DEFAULT)
    tx.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_WORLD)
    unreal.EditorAssetLibrary.save_loaded_asset(tx); TEX[k] = tx
uv = mel.create_material_expression(m, unreal.MaterialExpressionTextureCoordinate, -900, -100)
pos = mel.create_material_expression(m, unreal.MaterialExpressionObjectPositionWS, -900, 150)
tex = mel.create_material_expression(m, unreal.MaterialExpressionTextureSampleParameter2D, -650, -50)
tex.set_editor_property("parameter_name", "FlagTex"); tex.set_editor_property("texture", TEX["Blue"])
mel.connect_material_expressions(uv, "", tex, "UVs")
col = mel.create_material_expression(m, unreal.MaterialExpressionVectorParameter, -650, 150); col.set_editor_property("parameter_name", "Color")
col.set_editor_property("default_value", unreal.LinearColor(1, 1, 1, 1))
rgb = mel.create_material_expression(m, unreal.MaterialExpressionMultiply, -350, 0)
mel.connect_material_expressions(tex, "RGB", rgb, "A"); mel.connect_material_expressions(col, "", rgb, "B")
mel.connect_material_property(rgb, "", unreal.MaterialProperty.MP_BASE_COLOR)
mel.connect_material_property(tex, "A", unreal.MaterialProperty.MP_OPACITY_MASK)
r = mel.create_material_expression(m, unreal.MaterialExpressionConstant, -200, 200); r.set_editor_property("r", 0.85)
mel.connect_material_property(r, "", unreal.MaterialProperty.MP_ROUGHNESS)
# light through the cloth: its own colour
mel.connect_material_property(rgb, "", unreal.MaterialProperty.MP_SUBSURFACE_COLOR)
# WPO: tied at the cord, the rest streams and ripples with the live wind (MPC_Wind WindDir: xy direction, z strength)
w = mel.create_material_expression(m, unreal.MaterialExpressionCustom, -500, 300)
w.set_editor_property("code", """float s = saturate(Wind.z);
float v = UV.y;
float t = T * lerp(2.2, 13.0, s) + P.x * 0.013 + P.y * 0.017;
float wave = sin(UV.x * 7.0 - t + v * 4.0) + 0.5 * sin(UV.x * 17.0 - t * 1.7 + v * 7.0);
float gust = 0.6 + 0.4 * sin(T * 0.7 + P.x * 0.002);
float3 dir = float3(Wind.xy, 0.0);
return (dir * lerp(1.0, 26.0, s * s) * gust + N * wave * lerp(0.9, 7.0, s)) * v * (0.35 + 0.65 * v);""")
w.set_editor_property("output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT3)
ins = []
for n in ["UV", "T", "P", "N", "Wind"]:
    i = unreal.CustomInput(); i.set_editor_property("input_name", n); ins.append(i)
w.set_editor_property("inputs", ins)
t = mel.create_material_expression(m, unreal.MaterialExpressionTime, -900, 300)
vn = mel.create_material_expression(m, unreal.MaterialExpressionVertexNormalWS, -900, 400)
cp = mel.create_material_expression(m, unreal.MaterialExpressionCollectionParameter, -900, 500)
cp.set_editor_property("collection", unreal.load_asset("/Game/Environment/Snow/MPC_Wind")); cp.set_editor_property("parameter_name", "WindDir")
for e, n in [(uv, "UV"), (t, "T"), (pos, "P"), (vn, "N"), (cp, "Wind")]:
    mel.connect_material_expressions(e, "", w, n)
mel.connect_material_property(w, "", unreal.MaterialProperty.MP_WORLD_POSITION_OFFSET)
mel.recompile_material(m); unreal.EditorAssetLibrary.save_loaded_asset(m)
tk = unreal.AssetImportTask(); tk.filename = "F:/UE_project/YggdrasilsPeak/Saved/Realms/SM_PrayerFlag.glb"; tk.destination_path = D; tk.destination_name = "SM_PrayerFlag"; tk.automated = True; tk.replace_existing = True; tk.save = True
at.import_asset_tasks([tk]); print(list(tk.imported_object_paths)[:4])
COL = {"Blue": (1, 1, 1), "White": (0.95, 0.95, 0.93), "Red": (1, 1, 1), "Green": (1, 1, 1), "Yellow": (1, 1, 1)}   # tints over the printed texture
for k, v in COL.items():
    n = f"MI_PrayerFlag_{k}"
    mi = unreal.load_asset(f"{D}/{n}") or at.create_asset(n, D, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    mi.set_editor_property("parent", m)
    unreal.MaterialEditingLibrary.set_material_instance_vector_parameter_value(mi, "Color", unreal.LinearColor(v[0], v[1], v[2], 1))
    unreal.MaterialEditingLibrary.set_material_instance_texture_parameter_value(mi, "FlagTex", TEX[k])
    unreal.EditorAssetLibrary.save_loaded_asset(mi)
print("flags ok")
