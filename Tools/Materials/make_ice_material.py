# Runs INSIDE the editor. M_IceScan plus one instance per ice cliff scan, applied to the meshes; collision hugs the shape better than a box.
import unreal
MEL = unreal.MaterialEditingLibrary
at = unreal.AssetToolsHelpers.get_asset_tools()
D = "/Game/Environment/Glacier"
SCANS = {"uhpocesfa": "Raw/uhpocesfa_tier_0", "uhupbgxfa": "Raw/uhupbgxfa_tier_0", "veipbcmcb": "Raw/veipbcmcb_tier_0"}
ar = unreal.AssetRegistryHelpers.get_asset_registry()
for d in ar.get_assets(unreal.ARFilter(package_paths=["/Game/Fab/Megascans/3D/Ice_Cliff_vd5leh0ga"], recursive_paths=True, class_paths=[unreal.TopLevelAssetPath("/Script/Engine", "StaticMesh")])):
    p = str(d.package_name); SCANS["vd5leh0ga"] = "/".join(p.split("/")[-4:-2]); print("found", p)
m = unreal.load_asset(f"{D}/M_IceScan") or at.create_asset("M_IceScan", D, unreal.Material, unreal.MaterialFactoryNew())
MEL.delete_all_material_expressions(m)
m.set_editor_property("two_sided", True)   # the scans are open shells: the inside of a tilted one must not vanish
bc = MEL.create_material_expression(m, unreal.MaterialExpressionTextureSampleParameter2D, -900, 0)
bc.set_editor_property("parameter_name", "Base")
bc.set_editor_property("texture", unreal.load_asset("/Game/Fab/Megascans/3D/Ice_Cliff_veipbcmcb/Raw/veipbcmcb_tier_0/Textures/T_veipbcmcb_8K_B"))
nm =MEL.create_material_expression(m, unreal.MaterialExpressionTextureSampleParameter2D, -900, 300)
nm.set_editor_property("parameter_name", "Nrm")
nm.set_editor_property("texture", unreal.load_asset("/Game/Fab/Megascans/3D/Ice_Cliff_veipbcmcb/Raw/veipbcmcb_tier_0/Textures/T_veipbcmcb_8K_N"))
nm.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)
vn = MEL.create_material_expression(m, unreal.MaterialExpressionVertexNormalWS, -900, 600)
cu = MEL.create_material_expression(m, unreal.MaterialExpressionCustom, -450, 100)
cu.set_editor_property("output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT3)
ins = []
for n in ("Col", "VN"):
    ci = unreal.CustomInput(); ci.set_editor_property("input_name", n); ins.append(ci)
cu.set_editor_property("inputs", ins)
co = unreal.CustomOutput(); co.set_editor_property("output_name", "Rough"); co.set_editor_property("output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT1)
cu.set_editor_property("additional_outputs", [co])
cu.set_editor_property("code", open("F:/UE_project/YggdrasilsPeak/Tools/Materials/M_IceScan.hlsl", encoding="utf8").read())
C = MEL.connect_material_expressions
C(bc, "RGB", cu, "Col"); C(vn, "", cu, "VN")
MEL.connect_material_property(cu, "", unreal.MaterialProperty.MP_BASE_COLOR)
MEL.connect_material_property(cu, "Rough", unreal.MaterialProperty.MP_ROUGHNESS)
MEL.connect_material_property(nm, "RGB", unreal.MaterialProperty.MP_NORMAL)
MEL.recompile_material(m)
unreal.EditorAssetLibrary.save_loaded_asset(m, False)
sub = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
for name, rel in SCANS.items():
    base = f"/Game/Fab/Megascans/3D/Ice_Cliff_{name}/{rel}"
    tex = unreal.EditorAssetLibrary.list_assets(f"{base}/Textures", False, False)
    b = [t for t in tex if "_B." in t or t.endswith("_B")]; n = [t for t in tex if "_N." in t or t.endswith("_N")]
    mi_p = f"{D}/MI_IceScan_{name}"
    mi = unreal.load_asset(mi_p) or at.create_asset(f"MI_IceScan_{name}", D, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    mi.set_editor_property("parent", m)
    MEL.set_material_instance_texture_parameter_value(mi, "Base", unreal.load_asset(b[0].split(".")[0]))
    MEL.set_material_instance_texture_parameter_value(mi, "Nrm", unreal.load_asset(n[0].split(".")[0]))
    MEL.update_material_instance(mi)
    unreal.EditorAssetLibrary.save_loaded_asset(mi, False)
    mesh = unreal.load_asset(f"{base}/StaticMeshes/{rel.split('/')[-1]}")
    mesh.set_material(0, mi)
    if sub.get_simple_collision_count(mesh) == 0:
        sub.add_simple_collisions(mesh, unreal.ScriptCollisionShapeType.BOX)
    if sub.get_simple_collision_count(mesh) == 0:
        unreal.EditorStaticMeshLibrary.add_simple_collisions(mesh, unreal.ScriptingCollisionShapeType.BOX)
    print(name, "collision shapes", sub.get_simple_collision_count(mesh))
    unreal.EditorAssetLibrary.save_loaded_asset(mesh, False)
print("ice material done", list(SCANS))
