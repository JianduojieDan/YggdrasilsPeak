# Runs INSIDE the editor. Imports the realm sphere + Midgard daymap, builds M_RealmPlanet (unlit sky material from
# Tools/Materials/M_RealmPlanet.hlsl) and one instance per realm: /Game/Environment/Realms/MI_Realm_<Name>.
import unreal
mel = unreal.MaterialEditingLibrary; at = unreal.AssetToolsHelpers.get_asset_tools()
D = "/Game/Environment/Realms"
ROOT = "F:/UE_project/YggdrasilsPeak"
def imp(src, name):
    t = unreal.AssetImportTask(); t.filename = src; t.destination_path = D; t.destination_name = name
    t.automated = True; t.replace_existing = True; t.save = True
    at.import_asset_tasks([t]); return unreal.load_asset(f"{D}/{name}")
sphere = unreal.load_asset(f"{D}/SM_RealmSphere/StaticMeshes/SM_RealmSphere") or imp(f"{ROOT}/Saved/Realms/SM_RealmSphere.glb", "SM_RealmSphere")
earth = unreal.load_asset(f"{D}/T_Midgard_Day") or imp(f"{ROOT}/Downloads/nasa_bluemarble/earth_daymap_2k_solarsystemscope_CC-BY-4.0.jpg", "T_Midgard_Day")
if sphere:
    unreal.EditorStaticMeshLibrary.remove_collisions(sphere)
    unreal.EditorAssetLibrary.save_loaded_asset(sphere)
code = open(f"{ROOT}/Tools/Materials/M_RealmPlanet.hlsl", encoding="utf-8").read()
m = unreal.load_asset(f"{D}/M_RealmPlanet") or at.create_asset("M_RealmPlanet", D, unreal.Material, unreal.MaterialFactoryNew())
mel.delete_all_material_expressions(m)
m.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
try:
    m.set_editor_property("is_sky", False)         # bIsSky made the spheres vanish entirely (editor + game) - plain unlit
except Exception as e:
    print("is_sky:", e)
# translucent (opacity 1) with no vertex/pixel fog: opaque surfaces always get the atmosphere's inscatter added, which
# lifts a black planet (Svartalfheim, Hel) to sky colour. Translucent still depth-tests, so ridges hide planets.
m.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
m.set_editor_property("two_sided", False)   # two-sided translucent drew the far half over the near half (bright blob at the pole)
# fog + cloud fogging ON (2026-11-13, user: the planets showed in front of the clouds): the clouds now cover the planets;
# the aerial perspective the fog adds is cancelled in M_RealmPlanet.hlsl (AP input), so dark planets stay dark
m.set_editor_property("use_translucency_vertex_fog", True)
m.set_editor_property("compute_fog_per_pixel", True)
m.set_editor_property("apply_cloud_fogging", True)
c = mel.create_material_expression(m, unreal.MaterialExpressionCustom, -400, 0)
c.set_editor_property("code", code)
c.set_editor_property("output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT3)
op = unreal.CustomOutput(); op.set_editor_property("output_name", "Op"); op.set_editor_property("output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT1)
c.set_editor_property("additional_outputs", [op])
names = ["WP", "Ctr", "Cam", "SunDir", "Sky", "Tex", "Realm", "Spin", "Bright", "WashLow", "AP", "Tilt", "Relief"]
ins = []
for n in names:
    i = unreal.CustomInput(); i.set_editor_property("input_name", n); ins.append(i)
c.set_editor_property("inputs", ins)
def node(cls, y, **props):
    e = mel.create_material_expression(m, cls, -900, y)
    for k, v in props.items(): e.set_editor_property(k, v)
    return e
wp = node(unreal.MaterialExpressionWorldPosition, -400)
ctr = node(unreal.MaterialExpressionObjectPositionWS, -300)
cam = node(unreal.MaterialExpressionCameraPositionWS, -200)
sun = node(unreal.MaterialExpressionSkyAtmosphereLightDirection, -100)
sky = node(unreal.MaterialExpressionSkyAtmosphereViewLuminance, 0)
ap = node(unreal.MaterialExpressionSkyAtmosphereAerialPerspective, 600)
tex = node(unreal.MaterialExpressionTextureObjectParameter, 100, parameter_name="DayMap", texture=earth)
def sp(name, v, y):
    return node(unreal.MaterialExpressionScalarParameter, y, parameter_name=name, default_value=v)
tl = sp("Tilt", 0.0, 700); rf = sp("Relief", 1.0, 800); rl = sp("Realm", 0.0, 200); spin = sp("Spin", 0.0, 300); br = sp("Bright", 4.0, 400); wl = sp("WashLow", 0.0, 500)
for e, n in [(wp, "WP"), (ctr, "Ctr"), (cam, "Cam"), (sun, "SunDir"), (sky, "Sky"), (tex, "Tex"), (rl, "Realm"), (spin, "Spin"), (br, "Bright"), (wl, "WashLow"), (ap, "AP"), (tl, "Tilt"), (rf, "Relief")]:
    mel.connect_material_expressions(e, "", c, n)
mel.connect_material_property(c, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
fade = sp("Fade", 1.0, 900)
mo = mel.create_material_expression(m, unreal.MaterialExpressionMultiply, -150, 300)
mel.connect_material_expressions(c, "Op", mo, "A"); mel.connect_material_expressions(fade, "", mo, "B")
mel.connect_material_property(mo, "", unreal.MaterialProperty.MP_OPACITY)
mel.recompile_material(m)
unreal.EditorAssetLibrary.save_loaded_asset(m)
REALMS = [("Midgard", 0, -1.25, 0.85), ("Niflheim", 1, 1.2, 0.4), ("Muspelheim", 2, 0.0, 0.0), ("Jotunheim", 3, 0.0, 0.2),
          ("Alfheim", 4, 0.0, 0.0), ("Vanaheim", 5, 0.8, 0.0), ("Svartalfheim", 6, 0.0, 0.0), ("Hel", 7, 1.1, 0.0),
          ("Asgard", 8, 2.6, 0.2), ("HelTrail", 9, 0.0, 0.0)]
for name, idx, spin_v, wash in REALMS:
    p = f"{D}/MI_Realm_{name}"
    mi = unreal.load_asset(p) or at.create_asset(f"MI_Realm_{name}", D, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    mi.set_editor_property("parent", m)
    mel.set_material_instance_scalar_parameter_value(mi, "Realm", float(idx))
    mel.set_material_instance_scalar_parameter_value(mi, "Spin", spin_v)
    mel.set_material_instance_scalar_parameter_value(mi, "WashLow", wash)
    BR = {"Midgard": 2.2, "Niflheim": 1.5, "Muspelheim": 2.0, "Jotunheim": 1.1, "Alfheim": 2.2, "Vanaheim": 1.2, "Svartalfheim": 2.2, "Hel": 2.0, "Asgard": 1.6, "HelTrail": 2.2}
    mel.set_material_instance_scalar_parameter_value(mi, "Bright", BR[name])
    RF = {"Midgard": 0.4, "Niflheim": 2.2, "Muspelheim": 2.6, "Jotunheim": 3.0, "Vanaheim": 0.5, "Hel": 1.2, "Asgard": 2.4}
    mel.set_material_instance_scalar_parameter_value(mi, "Relief", RF.get(name, 0.0))
    mel.set_material_instance_scalar_parameter_value(mi, "Tilt", 0.22 if name == "Midgard" else 0.0)
    unreal.EditorAssetLibrary.save_loaded_asset(mi)
print("realm materials ok", m.get_editor_property("is_sky") if hasattr(m, "is_sky") else "")
