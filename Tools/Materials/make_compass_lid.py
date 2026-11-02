# Runs INSIDE the editor (uepy.py). M_CompassLid for the rune compass's lid (slot "darkbrass"): the World Tree
# engraving (Tools/UI/compass_emblem.py) in gold on dark brass, glowing faintly with the runes (parameter Glow,
# driven by UCompassComponent like the runes').
import unreal
MEL = unreal.MaterialEditingLibrary
at = unreal.AssetToolsHelpers.get_asset_tools()
D = "/Game/Props/Compass"
t = unreal.AssetImportTask(); t.filename = "F:/UE_project/YggdrasilsPeak/Art/UI/Compass/T_CompassEmblem.png"
t.destination_path = D; t.destination_name = "T_CompassEmblem"; t.automated = True; t.save = True; t.replace_existing = True
at.import_asset_tasks([t])
tex = unreal.load_asset(f"{D}/T_CompassEmblem")
tex.set_editor_property("srgb", False); tex.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_GRAYSCALE)
unreal.EditorAssetLibrary.save_loaded_asset(tex, False)
P = f"{D}/M_CompassLid"
mat = unreal.load_asset(P) or at.create_asset("M_CompassLid", D, unreal.Material, unreal.MaterialFactoryNew())
MEL.delete_all_material_expressions(mat)
to = MEL.create_material_expression(mat, unreal.MaterialExpressionTextureObject, -900, 0); to.set_editor_property("texture", tex)
lp = MEL.create_material_expression(mat, unreal.MaterialExpressionLocalPosition, -900, 200)
cu = MEL.create_material_expression(mat, unreal.MaterialExpressionCustom, -500, 100)
cu.set_editor_property("output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT3)
ins = []
for n in ("Emblem", "LP"):
    ci = unreal.CustomInput(); ci.set_editor_property("input_name", n); ins.append(ci)
cu.set_editor_property("inputs", ins)
outs = []
for n in ("Mask", "Rough"):
    co = unreal.CustomOutput(); co.set_editor_property("output_name", n); co.set_editor_property("output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT1); outs.append(co)
cu.set_editor_property("additional_outputs", outs)
cu.set_editor_property("code", open("F:/UE_project/YggdrasilsPeak/Tools/Materials/M_CompassLid.hlsl", encoding="utf8").read())
MEL.connect_material_expressions(to, "", cu, "Emblem"); MEL.connect_material_expressions(lp, "", cu, "LP")
glow = MEL.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -500, 400)
glow.set_editor_property("parameter_name", "Glow"); glow.set_editor_property("default_value", 0.0)
gold = MEL.create_material_expression(mat, unreal.MaterialExpressionConstant3Vector, -500, 500)
gold.set_editor_property("constant", unreal.LinearColor(1.2, 0.65, 0.16, 1))
m1 = MEL.create_material_expression(mat, unreal.MaterialExpressionMultiply, -250, 420)
m2 = MEL.create_material_expression(mat, unreal.MaterialExpressionMultiply, -100, 420)
MEL.connect_material_expressions(gold, "", m1, "A"); MEL.connect_material_expressions(glow, "", m1, "B")
MEL.connect_material_expressions(m1, "", m2, "A"); MEL.connect_material_expressions(cu, "Mask", m2, "B")
one = MEL.create_material_expression(mat, unreal.MaterialExpressionConstant, -250, 300); one.set_editor_property("r", 0.35)   # mostly non-metal: an engraving must read in flat light
MEL.connect_material_property(cu, "", unreal.MaterialProperty.MP_BASE_COLOR)
MEL.connect_material_property(cu, "Rough", unreal.MaterialProperty.MP_ROUGHNESS)
MEL.connect_material_property(one, "", unreal.MaterialProperty.MP_METALLIC)
MEL.connect_material_property(m2, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
MEL.recompile_material(mat); unreal.EditorAssetLibrary.save_loaded_asset(mat, False)
m = unreal.load_asset(f"{D}/SM_Compass_Case")
for i, sm in enumerate(m.static_materials):
    if str(sm.material_slot_name) == "darkbrass":
        m.set_material(i, mat)
unreal.EditorAssetLibrary.save_loaded_asset(m, False)
print("lid material on case")
