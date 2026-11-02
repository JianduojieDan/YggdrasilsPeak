# Runs INSIDE the editor (uepy.py). M_WeatheredTent: the parents' tent (Fab modern_camping_tent) as it would look
# after twenty years on the glacier - the dye bleached out, grit and snow dust. Applied as an override on the clue
# actor only; the Fab tent asset and its material are not changed.
import unreal
MEL = unreal.MaterialEditingLibrary
at = unreal.AssetToolsHelpers.get_asset_tools()
ROOT = "F:/UE_project/YggdrasilsPeak"
D = "/Game/Environment/Story"
TX = "/Game/Fab/Modern_Camping_Tent/modern_camping_tent__GLTF_/Textures/"
P = f"{D}/M_WeatheredTent"
mat = unreal.load_asset(P)
if mat:
    MEL.delete_all_material_expressions(mat)
else:
    mat = at.create_asset("M_WeatheredTent", D, unreal.Material, unreal.MaterialFactoryNew())
mat.set_editor_property("two_sided", True)
def ts(name, y, normal=False):
    e = MEL.create_material_expression(mat, unreal.MaterialExpressionTextureSample, -900, y)
    e.set_editor_property("texture", unreal.load_asset(TX + name))
    if normal: e.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)
    return e
bc, nm = ts("modern_campingtent_BaseColor", 0), ts("modern_campingtent_Normal_GL", 300, True)
wp = MEL.create_material_expression(mat, unreal.MaterialExpressionWorldPosition, -900, 600)
vn = MEL.create_material_expression(mat, unreal.MaterialExpressionVertexNormalWS, -900, 700)
cu = MEL.create_material_expression(mat, unreal.MaterialExpressionCustom, -450, 200)
cu.set_editor_property("output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT3)
ins = []
for n in ("Col", "WP", "VN"):
    ci = unreal.CustomInput(); ci.set_editor_property("input_name", n); ins.append(ci)
cu.set_editor_property("inputs", ins)
co = unreal.CustomOutput(); co.set_editor_property("output_name", "Rough"); co.set_editor_property("output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT1)
cu.set_editor_property("additional_outputs", [co])
cu.set_editor_property("code", open(f"{ROOT}/Tools/Materials/M_WeatheredFabric.hlsl", encoding="utf8").read())
C = MEL.connect_material_expressions
C(bc, "RGB", cu, "Col"); C(wp, "", cu, "WP"); C(vn, "", cu, "VN")
MEL.connect_material_property(cu, "", unreal.MaterialProperty.MP_BASE_COLOR)
MEL.connect_material_property(cu, "Rough", unreal.MaterialProperty.MP_ROUGHNESS)
MEL.connect_material_property(nm, "RGB", unreal.MaterialProperty.MP_NORMAL)
MEL.recompile_material(mat)
unreal.EditorAssetLibrary.save_loaded_asset(mat, False)
for a in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors():
    if a.get_actor_label() == "Clue_ParentsCamp":
        a.get_editor_property("mesh").set_material(0, mat); print("applied to", a.get_actor_label())
print("M_WeatheredTent built")
