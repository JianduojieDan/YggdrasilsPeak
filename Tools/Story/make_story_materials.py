# Runs INSIDE the editor (Tools/RoutePlanning/uepy.py). Chomolungma chapter 1 props: M_StoryProp (one material, Kind picks
# stone / wood / tin / rug / copper / iron / silk / flame / photo / nylon / water / painted cloth / plain) + instances,
# and M_StoryPlume (the golden plume over the summit, additive, Glow 0..1). Re-runnable.
import unreal
mel = unreal.MaterialEditingLibrary; at = unreal.AssetToolsHelpers.get_asset_tools(); EAL = unreal.EditorAssetLibrary
D = "/Game/Story/Materials"
ROOT = "F:/UE_project/YggdrasilsPeak"

def material(name):
    m = unreal.load_asset(f"{D}/{name}") or at.create_asset(name, D, unreal.Material, unreal.MaterialFactoryNew())
    mel.delete_all_material_expressions(m)
    return m

def custom(m, code, ins, x, y, outs=(), otype=unreal.CustomMaterialOutputType.CMOT_FLOAT3):
    c = mel.create_material_expression(m, unreal.MaterialExpressionCustom, x, y)
    c.set_editor_property("code", code); c.set_editor_property("output_type", otype)
    li = []
    for n in ins:
        i = unreal.CustomInput(); i.set_editor_property("input_name", n); li.append(i)
    c.set_editor_property("inputs", li)
    lo = []
    for n, t in outs:
        o = unreal.CustomOutput(); o.set_editor_property("output_name", n); o.set_editor_property("output_type", t); lo.append(o)
    if lo: c.set_editor_property("additional_outputs", lo)
    return c

# rock colour scan used by the terrain (stone walls borrow it)
tile = unreal.load_asset("/Game/Materials/M_EverestDetailTile")
rock_tex = None
for e in unreal.ObjectIterator(unreal.MaterialExpressionTextureObjectParameter):
    if e.get_outer() == tile and str(e.get_editor_property("parameter_name")).lower().startswith("rockcol"):
        rock_tex = e.get_editor_property("texture")
if not rock_tex:
    for e in unreal.ObjectIterator(unreal.MaterialExpressionTextureObjectParameter):
        if e.get_outer() == tile: rock_tex = e.get_editor_property("texture"); break
white = unreal.load_asset("/Engine/EngineResources/WhiteSquareTexture")
print("rock texture", rock_tex.get_name() if rock_tex else None)

# ---------------- M_StoryProp
m = material("M_StoryProp")
m.set_editor_property("two_sided", True)
m.set_editor_property("tangent_space_normal", False)
m.set_editor_property("used_with_nanite", True)   # glb meshes import as Nanite: without the flag the game draws the default material
F1 = unreal.CustomMaterialOutputType.CMOT_FLOAT1; F3 = unreal.CustomMaterialOutputType.CMOT_FLOAT3
c = custom(m, open(f"{ROOT}/Tools/Story/M_StoryProp.hlsl", encoding="utf8").read(),
           ["UV", "WP", "N", "Kind", "Col", "T", "Rock", "Photo"], -500, 0, [("Rough", F1), ("Metal", F1), ("Emis", F3), ("Nrm", F3)])
uv = mel.create_material_expression(m, unreal.MaterialExpressionTextureCoordinate, -900, -200)
wp = mel.create_material_expression(m, unreal.MaterialExpressionWorldPosition, -900, -120)
vn = mel.create_material_expression(m, unreal.MaterialExpressionVertexNormalWS, -900, -40)
kind = mel.create_material_expression(m, unreal.MaterialExpressionScalarParameter, -900, 40); kind.set_editor_property("parameter_name", "Kind")
col = mel.create_material_expression(m, unreal.MaterialExpressionVectorParameter, -900, 120); col.set_editor_property("parameter_name", "Color")
col.set_editor_property("default_value", unreal.LinearColor(0.5, 0.5, 0.5, 1))
tm = mel.create_material_expression(m, unreal.MaterialExpressionTime, -900, 200)
rk = mel.create_material_expression(m, unreal.MaterialExpressionTextureObject, -900, 280)
if rock_tex: rk.set_editor_property("texture", rock_tex)
ph = mel.create_material_expression(m, unreal.MaterialExpressionTextureObjectParameter, -900, 380); ph.set_editor_property("parameter_name", "Photo")
ph.set_editor_property("texture", white)
for e, n in [(uv, "UV"), (wp, "WP"), (vn, "N"), (kind, "Kind"), (col, "Col"), (tm, "T"), (rk, "Rock"), (ph, "Photo")]:
    mel.connect_material_expressions(e, "", c, n)
mel.connect_material_property(c, "", unreal.MaterialProperty.MP_BASE_COLOR)
mel.connect_material_property(c, "Rough", unreal.MaterialProperty.MP_ROUGHNESS)
mel.connect_material_property(c, "Metal", unreal.MaterialProperty.MP_METALLIC)
mel.connect_material_property(c, "Emis", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
mel.connect_material_property(c, "Nrm", unreal.MaterialProperty.MP_NORMAL)
mel.recompile_material(m); EAL.save_loaded_asset(m, False)

MIS = {  # name: (kind, colour)
    "Stone": (0, (1, 1, 1)), "Wood": (1, (0.23, 0.14, 0.08)), "WoodDark": (1, (0.1, 0.06, 0.035)),
    "TinBlue": (2, (0.07, 0.2, 0.42)), "TinGreen": (2, (0.08, 0.26, 0.16)), "Rug": (3, (1, 1, 1)),
    "Copper": (4, (1, 1, 1)), "Iron": (5, (1, 1, 1)), "Silk": (6, (1, 1, 1)), "Flame": (7, (1.0, 0.52, 0.18)),
    "Ember": (7, (1.0, 0.25, 0.05)), "TentYellow": (9, (0.62, 0.42, 0.05)), "TentOrange": (9, (0.62, 0.2, 0.03)),
    "TentRed": (9, (0.42, 0.05, 0.04)), "TentBlue": (9, (0.05, 0.14, 0.34)), "TentGrey": (9, (0.24, 0.25, 0.26)),
    "Water": (10, (1, 1, 1)), "Thangka": (12, (1, 1, 1)), "Cord": (11, (0.5, 0.48, 0.42)),
    "Pole": (11, (0.08, 0.08, 0.09)), "Plaster": (11, (0.62, 0.58, 0.5)), "SnowWhite": (11, (0.8, 0.82, 0.86)),
}
for name, (k, rgb) in MIS.items():
    path = f"{D}/MI_Story_{name}"
    mi = unreal.load_asset(path) or at.create_asset(f"MI_Story_{name}", D, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    mel.set_material_instance_parent(mi, m)
    mel.set_material_instance_scalar_parameter_value(mi, "Kind", float(k))
    mel.set_material_instance_vector_parameter_value(mi, "Color", unreal.LinearColor(rgb[0], rgb[1], rgb[2], 1))
    EAL.save_loaded_asset(mi, False)

# ---------------- M_StoryPlume: a golden snow plume streaming off the summit, only seen when Glow > 0
p = material("M_StoryPlume")
p.set_editor_property("blend_mode", unreal.BlendMode.BLEND_ADDITIVE)
p.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
p.set_editor_property("two_sided", True)
pc = custom(p, """float2 q = UV; float t = T * 0.03;
float2 w = float2(q.x * 3.0 - t * 2.0, q.y * 1.3);
float n = 0.0, a = 0.5;
for (int i = 0; i < 5; i++) { float2 f = frac(w), g = floor(w); f = f*f*(3.0-2.0*f);
  float h00 = frac(sin(dot(g, float2(127.1,311.7)))*43758.5), h10 = frac(sin(dot(g+float2(1,0), float2(127.1,311.7)))*43758.5);
  float h01 = frac(sin(dot(g+float2(0,1), float2(127.1,311.7)))*43758.5), h11 = frac(sin(dot(g+float2(1,1), float2(127.1,311.7)))*43758.5);
  n += a * lerp(lerp(h00,h10,f.x), lerp(h01,h11,f.x), f.y); w = w * 2.03 + 7.1; a *= 0.5; }
float body = smoothstep(0.0, 0.25, q.x) * (1.0 - smoothstep(0.55, 1.0, q.x));           // starts at the summit, thins downwind
float band = exp(-pow((q.y - 0.5 - (q.x - 0.2) * 0.15) * (2.2 + q.x * 2.5), 2.0) * 3.0);  // a ribbon that spreads
float edge = smoothstep(0.0, 0.12, q.y) * smoothstep(1.0, 0.88, q.y) * smoothstep(0.0, 0.06, q.x) * smoothstep(1.0, 0.8, q.x);
float d = saturate(n * 1.3 - 0.5) * body * band * edge;
return float3(1.0, 0.66, 0.3) * d * Glow * 14.0;""", ["UV", "T", "Glow"], -500, 0)
puv = mel.create_material_expression(p, unreal.MaterialExpressionTextureCoordinate, -900, -100)
pt = mel.create_material_expression(p, unreal.MaterialExpressionTime, -900, 0)
pg = mel.create_material_expression(p, unreal.MaterialExpressionScalarParameter, -900, 100); pg.set_editor_property("parameter_name", "Glow")
for e, n in [(puv, "UV"), (pt, "T"), (pg, "Glow")]:
    mel.connect_material_expressions(e, "", pc, n)
mel.connect_material_property(pc, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
mel.recompile_material(p); EAL.save_loaded_asset(p, False)
flag = unreal.load_asset("/Game/Environment/Landmarks/M_PrayerFlag")
flag.set_editor_property("used_with_nanite", True); mel.recompile_material(flag); EAL.save_loaded_asset(flag, False)
npc = unreal.load_asset(f"{D}/M_StoryNPC")
if npc: npc.set_editor_property("used_with_skeletal_mesh", True); mel.recompile_material(npc); EAL.save_loaded_asset(npc, False)
print("story materials done", len(MIS))
