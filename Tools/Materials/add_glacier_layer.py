# Runs INSIDE the editor (Tools/RoutePlanning/uepy.py). Imports the glacier mask + moraine textures and inserts the
# "Glacier" Custom node (M_EverestLandscape_Glacier.hlsl) between SnowSnakes (Custom_4) and the material attributes.
# Re-running replaces the node's code only.
import unreal
MEL = unreal.MaterialEditingLibrary
ROOT = "F:/UE_project/YggdrasilsPeak"
D = "/Game/Environment/Glacier"
at = unreal.AssetToolsHelpers.get_asset_tools()

def imp(src, name):
    t = unreal.AssetImportTask()
    t.filename = src; t.destination_path = D; t.destination_name = name
    t.automated = True; t.save = True; t.replace_existing = True
    at.import_asset_tasks([t])
    return unreal.load_asset(f"{D}/{name}")

def tex(src, name, srgb, comp, clamp=False):
    a = unreal.load_asset(f"{D}/{name}") or imp(src, name)
    a.set_editor_property("srgb", srgb)
    a.set_editor_property("compression_settings", comp)
    if clamp:
        a.set_editor_property("address_x", unreal.TextureAddress.TA_CLAMP)
        a.set_editor_property("address_y", unreal.TextureAddress.TA_CLAMP)
    unreal.EditorAssetLibrary.save_loaded_asset(a, False)
    return a

PH = f"{ROOT}/Imports/PolyHaven/rocks_ground_05/rocks_ground_05"
mask = tex(f"{ROOT}/Imports/Glacier/T_GlacierMask.png", "T_GlacierMask", False, unreal.TextureCompressionSettings.TC_GRAYSCALE, True)
morc = tex(f"{PH}_diff_2k.png", "T_Moraine_D", True, unreal.TextureCompressionSettings.TC_DEFAULT)
morn = tex(f"{PH}_nor_dx_2k.png", "T_Moraine_N", False, unreal.TextureCompressionSettings.TC_NORMALMAP)
mora = tex(f"{PH}_arm_2k.png", "T_Moraine_ARM", False, unreal.TextureCompressionSettings.TC_MASKS)

m = unreal.load_asset("/Game/Materials/M_EverestLandscape")
exprs = {e.get_name(): e for e in unreal.ObjectIterator(unreal.MaterialExpression) if e.get_outer() == m}
code = open(f"{ROOT}/Tools/Materials/M_EverestLandscape_Glacier.hlsl", encoding="utf8").read()

node = next((e for e in exprs.values() if isinstance(e, unreal.MaterialExpressionCustom) and e.get_editor_property("description") == "Glacier"), None)
if node is None:
    node = MEL.create_material_expression(m, unreal.MaterialExpressionCustom, 0, 1500)
    node.set_editor_property("description", "Glacier")
    node.set_editor_property("output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT3)
    ins = []
    for n in ("Col", "N", "Rgh", "WP", "VN", "Cam", "Rock", "Mask", "MorC", "MorN", "MorA"):
        ci = unreal.CustomInput(); ci.set_editor_property("input_name", n); ins.append(ci)
    node.set_editor_property("inputs", ins)
    outs = []
    for n, ty in (("NrmOut", unreal.CustomMaterialOutputType.CMOT_FLOAT3), ("RghOut", unreal.CustomMaterialOutputType.CMOT_FLOAT1)):
        co = unreal.CustomOutput(); co.set_editor_property("output_name", n); co.set_editor_property("output_type", ty); outs.append(co)
    node.set_editor_property("additional_outputs", outs)
    def tobj(t, y):
        e = MEL.create_material_expression(m, unreal.MaterialExpressionTextureObject, -300, y)
        e.set_editor_property("texture", t)
        return e
    tm, tc, tn, ta = tobj(mask, 1700), tobj(morc, 1800), tobj(morn, 1900), tobj(mora, 2000)
    C = MEL.connect_material_expressions
    s4, s0 = exprs["MaterialExpressionCustom_4"], exprs["MaterialExpressionCustom_0"]
    ok = [C(s4, "", node, "Col"), C(s4, "NrmOut", node, "N"), C(s0, "Roughness", node, "Rgh"),
          C(exprs["MaterialExpressionWorldPosition_0"], "", node, "WP"), C(exprs["MaterialExpressionVertexNormalWS_0"], "", node, "VN"),
          C(exprs["MaterialExpressionCameraPositionWS_1"], "", node, "Cam"), C(s0, "RockMask", node, "Rock"),
          C(tm, "", node, "Mask"), C(tc, "", node, "MorC"), C(tn, "", node, "MorN"), C(ta, "", node, "MorA")]
    mk = exprs["MaterialExpressionMakeMaterialAttributes_0"]
    ok += [C(node, "", mk, "BaseColor"), C(node, "NrmOut", mk, "Normal"), C(node, "RghOut", mk, "Roughness")]
    print("connections", ok)
node.set_editor_property("code", code)
MEL.recompile_material(m)
unreal.EditorAssetLibrary.save_loaded_asset(m, False)
print("glacier node", node.get_name())
