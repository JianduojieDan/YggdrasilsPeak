# Runs INSIDE the editor (uepy.py). Father's rune compass v2 (Tools/Blender/compass_v2.py + Tools/UI/compass_dial.py)
# into /Game/Props/Compass as SM_Compass2_Case / SM_Compass2_Needle. Materials per slot:
#   brass, needle_n, needle_s, jewel - instances of M_CompassPart (colour / metal / rough / emissive*Glow)
#   dial  - M_CompassDial: the enamel dial texture, runes glowing with "Glow" (UCompassComponent drives it)
#   lidin - M_CompassLidIn: the World Tree engraving, glowing a little with "Glow"
#   glass - M_CompassGlass: thin translucent glass with a bright reflection
import unreal
MEL = unreal.MaterialEditingLibrary
at = unreal.AssetToolsHelpers.get_asset_tools()
D = "/Game/Props/Compass"
ART = "F:/UE_project/YggdrasilsPeak/Art/UI/Compass"

def tex(name, srgb=True, mask=False):
    t = unreal.AssetImportTask(); t.filename = f"{ART}/{name}.png"; t.destination_path = D; t.destination_name = name
    t.automated = True; t.save = True; t.replace_existing = True
    at.import_asset_tasks([t])
    a = unreal.load_asset(f"{D}/{name}")
    a.set_editor_property("srgb", srgb)
    if mask:
        a.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_GRAYSCALE)
    unreal.EditorAssetLibrary.save_loaded_asset(a, False)
    return a

def textured(name, colour, glow_mask, glow_rgb, glow_scale, metallic, rough):
    P = f"{D}/{name}"
    m = unreal.load_asset(P) or at.create_asset(name, D, unreal.Material, unreal.MaterialFactoryNew())
    MEL.delete_all_material_expressions(m)
    c = MEL.create_material_expression(m, unreal.MaterialExpressionTextureSample, -700, 0); c.set_editor_property("texture", colour)
    k = MEL.create_material_expression(m, unreal.MaterialExpressionTextureSample, -700, 300); k.set_editor_property("texture", glow_mask)
    k.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_GRAYSCALE)   # masks are linear (sRGB off)
    g = MEL.create_material_expression(m, unreal.MaterialExpressionScalarParameter, -700, 550); g.set_editor_property("parameter_name", "Glow"); g.set_editor_property("default_value", 0.0)
    col = MEL.create_material_expression(m, unreal.MaterialExpressionConstant3Vector, -700, 650); col.set_editor_property("constant", unreal.LinearColor(*glow_rgb, 1))
    sc = MEL.create_material_expression(m, unreal.MaterialExpressionConstant, -700, 750); sc.set_editor_property("r", glow_scale)
    m1 = MEL.create_material_expression(m, unreal.MaterialExpressionMultiply, -450, 500)
    m2 = MEL.create_material_expression(m, unreal.MaterialExpressionMultiply, -300, 500)
    m3 = MEL.create_material_expression(m, unreal.MaterialExpressionMultiply, -150, 500)
    MEL.connect_material_expressions(k, "R", m1, "A"); MEL.connect_material_expressions(g, "", m1, "B")
    MEL.connect_material_expressions(m1, "", m2, "A"); MEL.connect_material_expressions(col, "", m2, "B")
    MEL.connect_material_expressions(m2, "", m3, "A"); MEL.connect_material_expressions(sc, "", m3, "B")
    me = MEL.create_material_expression(m, unreal.MaterialExpressionConstant, -300, 200); me.set_editor_property("r", metallic)
    ro = MEL.create_material_expression(m, unreal.MaterialExpressionConstant, -300, 280); ro.set_editor_property("r", rough)
    MEL.connect_material_property(c, "RGB", unreal.MaterialProperty.MP_BASE_COLOR)
    MEL.connect_material_property(me, "", unreal.MaterialProperty.MP_METALLIC)
    MEL.connect_material_property(ro, "", unreal.MaterialProperty.MP_ROUGHNESS)
    MEL.connect_material_property(m3, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    MEL.recompile_material(m); unreal.EditorAssetLibrary.save_loaded_asset(m, False)
    return m

dial = textured("M_CompassDial", tex("T_CompassDial"), tex("T_CompassDialRunes", False, True), (2.2, 1.2, 0.3), 1.0, 0.0, 0.55)
lidin = textured("M_CompassLidIn", tex("T_CompassLidIn"), tex("T_CompassLidInMask", False, True), (1.2, 0.65, 0.16), 0.6, 0.85, 0.35)

# glass
P = f"{D}/M_CompassGlass"
glass = unreal.load_asset(P) or at.create_asset("M_CompassGlass", D, unreal.Material, unreal.MaterialFactoryNew())
MEL.delete_all_material_expressions(glass)
glass.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
glass.set_editor_property("translucency_lighting_mode", unreal.TranslucencyLightingMode.TLM_SURFACE)
fr = MEL.create_material_expression(glass, unreal.MaterialExpressionFresnel, -500, 0); fr.set_editor_property("exponent", 4.0)
lerp = MEL.create_material_expression(glass, unreal.MaterialExpressionLinearInterpolate, -250, 0)
a0 = MEL.create_material_expression(glass, unreal.MaterialExpressionConstant, -500, 150); a0.set_editor_property("r", 0.06)
a1 = MEL.create_material_expression(glass, unreal.MaterialExpressionConstant, -500, 220); a1.set_editor_property("r", 0.45)
MEL.connect_material_expressions(a0, "", lerp, "A"); MEL.connect_material_expressions(a1, "", lerp, "B"); MEL.connect_material_expressions(fr, "", lerp, "Alpha")
bc = MEL.create_material_expression(glass, unreal.MaterialExpressionConstant3Vector, -250, 200); bc.set_editor_property("constant", unreal.LinearColor(0.9, 0.95, 1.0, 1))
ro = MEL.create_material_expression(glass, unreal.MaterialExpressionConstant, -250, 300); ro.set_editor_property("r", 0.04)
MEL.connect_material_property(bc, "", unreal.MaterialProperty.MP_BASE_COLOR)
MEL.connect_material_property(ro, "", unreal.MaterialProperty.MP_ROUGHNESS)
MEL.connect_material_property(lerp, "", unreal.MaterialProperty.MP_OPACITY)
MEL.recompile_material(glass); unreal.EditorAssetLibrary.save_loaded_asset(glass, False)

# metal / painted parts as instances of M_CompassPart
part = unreal.load_asset(f"{D}/M_CompassPart")
PARTS = {"brass": ((0.66, 0.47, 0.20), 1.0, 0.33), "needle_n": ((0.62, 0.07, 0.05), 0.2, 0.35),
         "needle_s": ((0.10, 0.13, 0.22), 1.0, 0.25), "jewel": ((0.6, 0.04, 0.08), 0.0, 0.08)}
slots = {"dial": dial, "lidin": lidin, "glass": glass}
for slot, (c, mtl, r) in PARTS.items():
    n = f"MI_Compass2_{slot}"
    mi = unreal.load_asset(f"{D}/{n}") or at.create_asset(n, D, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    mi.set_editor_property("parent", part)
    MEL.set_material_instance_vector_parameter_value(mi, "BaseColor", unreal.LinearColor(*c, 1))
    MEL.set_material_instance_scalar_parameter_value(mi, "Metallic", mtl)
    MEL.set_material_instance_scalar_parameter_value(mi, "Roughness", r)
    MEL.set_material_instance_vector_parameter_value(mi, "EmissiveColor", unreal.LinearColor(0, 0, 0, 1))
    unreal.EditorAssetLibrary.save_loaded_asset(mi, False)
    slots[slot] = mi

unreal.SystemLibrary.execute_console_command(None, "Interchange.FeatureFlags.Import.FBX 0")
for src, name in (("SM_Compass_Case", "SM_Compass2_Case"), ("SM_Compass_Needle", "SM_Compass2_Needle")):   # fresh assets: re-importing over v1 kept its old slots
    o = unreal.FbxImportUI()
    o.import_mesh = True; o.import_materials = False; o.import_textures = False
    o.mesh_type_to_import = unreal.FBXImportType.FBXIT_STATIC_MESH
    o.static_mesh_import_data.combine_meshes = True; o.static_mesh_import_data.auto_generate_collision = False
    t = unreal.AssetImportTask(); t.filename = f"F:/UE_project/YggdrasilsPeak/Art/Blender/Export/Compass/{src}.fbx"
    t.destination_path = D; t.destination_name = name; t.automated = True; t.save = True; t.replace_existing = True; t.options = o
    at.import_asset_tasks([t])
    m = unreal.load_asset(f"{D}/{name}")
    for i, sm in enumerate(m.static_materials):
        s = str(sm.material_slot_name).split(".")[0]
        if s in slots:
            m.set_material(i, slots[s])
    unreal.EditorAssetLibrary.save_loaded_asset(m, False)
    print(name, [(str(x.material_slot_name), x.material_interface.get_name() if x.material_interface else None) for x in m.static_materials], m.get_bounding_box())
unreal.SystemLibrary.execute_console_command(None, "Interchange.FeatureFlags.Import.FBX 1")
