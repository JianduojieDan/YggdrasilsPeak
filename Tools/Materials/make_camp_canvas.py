# Runs INSIDE the editor. M_CampCanvas + one instance per colour; the camps pick from these (Tools/RoutePlanning/place_camps.py).
import unreal
MEL = unreal.MaterialEditingLibrary
at = unreal.AssetToolsHelpers.get_asset_tools()
D = "/Game/Environment/Camps"
TEX = "/Game/Fab/Modern_Camping_Tent/modern_camping_tent__GLTF_/Textures/modern_campingtent_BaseColor"
m = unreal.load_asset(f"{D}/M_CampCanvas") or at.create_asset("M_CampCanvas", D, unreal.Material, unreal.MaterialFactoryNew())
MEL.delete_all_material_expressions(m)
m.set_editor_property("two_sided", True)
bc = MEL.create_material_expression(m, unreal.MaterialExpressionTextureSampleParameter2D, -900, 0)
bc.set_editor_property("parameter_name", "Base")
bc.set_editor_property("texture", unreal.load_asset(TEX))
tint = MEL.create_material_expression(m, unreal.MaterialExpressionVectorParameter, -900, 250)
tint.set_editor_property("parameter_name", "Tint")
wp = MEL.create_material_expression(m, unreal.MaterialExpressionWorldPosition, -900, 450)
vn = MEL.create_material_expression(m, unreal.MaterialExpressionVertexNormalWS, -900, 550)
cu = MEL.create_material_expression(m, unreal.MaterialExpressionCustom, -450, 200)
cu.set_editor_property("output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT3)
ins = []
for n in ("Col", "Tint", "WP", "VN"):
    ci = unreal.CustomInput(); ci.set_editor_property("input_name", n); ins.append(ci)
cu.set_editor_property("inputs", ins)
co = unreal.CustomOutput(); co.set_editor_property("output_name", "Rough"); co.set_editor_property("output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT1)
cu.set_editor_property("additional_outputs", [co])
cu.set_editor_property("code", open("F:/UE_project/YggdrasilsPeak/Tools/Materials/M_CampCanvas.hlsl", encoding="utf8").read())
C = MEL.connect_material_expressions
C(bc, "RGB", cu, "Col"); C(tint, "", cu, "Tint"); C(wp, "", cu, "WP"); C(vn, "", cu, "VN")
MEL.connect_material_property(cu, "", unreal.MaterialProperty.MP_BASE_COLOR)
MEL.connect_material_property(cu, "Rough", unreal.MaterialProperty.MP_ROUGHNESS)
MEL.recompile_material(m)
unreal.EditorAssetLibrary.save_loaded_asset(m, False)
COLOURS = {"Rust": (0.30, 0.085, 0.04), "Ochre": (0.42, 0.26, 0.09), "Slate": (0.17, 0.20, 0.24), "Canvas": (0.30, 0.26, 0.19)}
for name, c in COLOURS.items():
    p = f"{D}/MI_Camp{name}"
    mi = unreal.load_asset(p) or at.create_asset(f"MI_Camp{name}", D, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    mi.set_editor_property("parent", m)
    MEL.set_material_instance_vector_parameter_value(mi, "Tint", unreal.LinearColor(c[0], c[1], c[2], 1))
    unreal.EditorAssetLibrary.save_loaded_asset(mi, False)
print("camp canvas built")
