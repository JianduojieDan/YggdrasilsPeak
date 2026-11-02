# Blender (headless): the rune compass as game meshes for the held compass (UCompassComponent). Opens the icon
# model (Saved/ItemIcons/src/Compass.blend from Tools/UI/story_items.py), splits it by material into the case
# (brass, lid, dial, runes) and the needle (needle + pin, origin on the pivot), exports both as plain-metre FBX
# to Art/Blender/Export/Compass/.   blender -b -P Tools/Blender/compass_mesh.py
import bpy, os
SRC = "F:/UE_project/YggdrasilsPeak/Saved/ItemIcons/src/Compass.blend"
OUT = "F:/UE_project/YggdrasilsPeak/Art/Blender/Export/Compass"
os.makedirs(OUT, exist_ok=True)
bpy.ops.wm.open_mainfile(filepath=SRC)
o = bpy.data.objects["Compass"]
bpy.context.view_layer.objects.active = o
o.select_set(True)
bpy.ops.object.mode_set(mode="EDIT"); bpy.ops.mesh.select_all(action="SELECT")
bpy.ops.mesh.separate(type="MATERIAL")
bpy.ops.object.mode_set(mode="OBJECT")
parts = [p for p in bpy.context.scene.objects if p.type == "MESH"]
needle = [p for p in parts if p.material_slots and p.material_slots[0].material.name.split(".")[0] in ("needle", "steel")]
body = [p for p in parts if p not in needle]
def join(objs, name):
    bpy.ops.object.select_all(action="DESELECT")
    for p in objs: p.select_set(True)
    bpy.context.view_layer.objects.active = objs[0]
    bpy.ops.object.join()
    j = bpy.context.active_object; j.name = name
    return j
b = join(body, "SM_Compass_Case"); n = join(needle, "SM_Compass_Needle")
# needle pivot on the compass axis
bpy.context.scene.cursor.location = (0, 0, 0.0175)
bpy.ops.object.select_all(action="DESELECT"); n.select_set(True); bpy.context.view_layer.objects.active = n
bpy.ops.object.origin_set(type="ORIGIN_CURSOR")
for obj in (b, n):
    bpy.ops.object.select_all(action="DESELECT"); obj.select_set(True); bpy.context.view_layer.objects.active = obj
    loc = obj.location.copy(); obj.location = (0, 0, 0)
    bpy.ops.export_scene.fbx(filepath=f"{OUT}/{obj.name}.fbx", use_selection=True, apply_unit_scale=True, mesh_smooth_type="FACE", add_leaf_bones=False)
    obj.location = loc
    print("exported", obj.name, [s.material.name for s in obj.material_slots])
