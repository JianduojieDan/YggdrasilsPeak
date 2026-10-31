# Imports a character made by rig_glb_character.py into /Game/Characters/<Name>/ on SK_Mannequin and
# builds its material from the glTF textures (BaseColor, Normal, MetalRough: G = roughness, B = metallic).
#   python Tools/Blender/import_character_ue.py <Name>
import sys, time
sys.path.append("E:/UE5.8/UE_5.8/Engine/Plugins/Experimental/PythonScriptPlugin/Content/Python")
from remote_execution import RemoteExecution, MODE_EXEC_FILE

NAME = sys.argv[1]
CODE = r'''
import unreal
NAME = "%s"
SRC = "F:/UE_project/YggdrasilsPeak/Art/Blender/Export/Skel/" + NAME
DST = "/Game/Characters/" + NAME
at = unreal.AssetToolsHelpers.get_asset_tools()
mel = unreal.MaterialEditingLibrary

tex_tasks = []
for tag in ("BaseColor", "Normal", "MetalRough"):
    t = unreal.AssetImportTask(); t.filename = f"{SRC}/T_{NAME}_{tag}.png"; t.destination_path = DST
    t.replace_existing = True; t.automated = True; t.save = True; tex_tasks.append(t)
at.import_asset_tasks(tex_tasks)
tx = {tag: unreal.load_asset(f"{DST}/T_{NAME}_{tag}") for tag in ("BaseColor", "Normal", "MetalRough")}
tx["Normal"].set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_NORMALMAP)
tx["Normal"].set_editor_property("srgb", False)
tx["MetalRough"].set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_MASKS)
tx["MetalRough"].set_editor_property("srgb", False)

import glob, os
meshes = [os.path.splitext(os.path.basename(f))[0] for f in glob.glob(SRC + "/SKM_*.fbx")]  # full body + _FPArms
for mesh in meshes:
    t = unreal.AssetImportTask(); t.filename = f"{SRC}/{mesh}.fbx"; t.destination_path = DST
    t.replace_existing = True; t.automated = True; t.save = True
    o = unreal.FbxImportUI(); o.import_mesh = True; o.import_as_skeletal = True; o.import_animations = False
    o.mesh_type_to_import = unreal.FBXImportType.FBXIT_SKELETAL_MESH
    o.skeleton = unreal.load_asset("/Game/Characters/Mannequins/Meshes/SK_Mannequin")
    o.create_physics_asset = False; o.import_materials = False; o.import_textures = False
    t.options = o
    at.import_asset_tasks([t])

mp = f"{DST}/M_{NAME}"
m = unreal.load_asset(mp) or at.create_asset(f"M_{NAME}", DST, unreal.Material, unreal.MaterialFactoryNew())
mel.delete_all_material_expressions(m)
def samp(tag, x, y, st):
    s = mel.create_material_expression(m, unreal.MaterialExpressionTextureSample, x, y)
    s.texture = tx[tag]; s.sampler_type = st; return s
bc = samp("BaseColor", -500, -200, unreal.MaterialSamplerType.SAMPLERTYPE_COLOR)
nm = samp("Normal", -500, 300, unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)
mr = samp("MetalRough", -500, 50, unreal.MaterialSamplerType.SAMPLERTYPE_MASKS)
mel.connect_material_property(bc, "RGB", unreal.MaterialProperty.MP_BASE_COLOR)
mel.connect_material_property(mr, "G", unreal.MaterialProperty.MP_ROUGHNESS)
mel.connect_material_property(mr, "B", unreal.MaterialProperty.MP_METALLIC)
mel.connect_material_property(nm, "RGB", unreal.MaterialProperty.MP_NORMAL)
m.set_editor_property("used_with_skeletal_mesh", True)
mel.recompile_material(m)
unreal.EditorAssetLibrary.save_asset(mp)

for mesh in meshes:
    sk = unreal.load_asset(f"{DST}/{mesh}")
    mats = list(sk.get_editor_property("materials"))
    gloves = unreal.load_asset("/Game/Survival_Character/Materials/MI_Survival_Character_Gloves")
    for sm in mats:   # attribute set on the copies, then write the list back
        sm.material_interface = gloves if "Glove" in str(sm.material_slot_name) else m
    sk.set_editor_property("materials", mats)
    unreal.EditorAssetLibrary.save_asset(f"{DST}/{mesh}", False)
    print(mesh, "SKEL", sk.skeleton.get_name(), "BOUNDS", sk.get_bounds().box_extent, "MATS", [(str(s.material_slot_name), s.material_interface.get_name()) for s in mats])
''' % NAME

re = RemoteExecution(); re.start()
for _ in range(50):
    if re.remote_nodes: break
    time.sleep(0.2)
if not re.remote_nodes:
    sys.exit("No editor found - is UnrealEditor running?")
re.open_command_connection(re.remote_nodes[0]["node_id"])
r = re.run_command(CODE, unattended=True, exec_mode=MODE_EXEC_FILE)
print("".join(o["output"] for o in r.get("output", [])))
print("success:", r.get("success"))
re.stop()
