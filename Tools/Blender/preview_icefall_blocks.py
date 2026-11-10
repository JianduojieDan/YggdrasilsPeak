# Blender (headless): one preview image of every icefall block FBX in a folder (workbench, studio light, white snow-like).
#   blender -b -P Tools/Blender/preview_icefall_blocks.py -- <fbx_dir> <out.png>
import bpy, sys, os, glob, math
from mathutils import Vector

args = sys.argv[sys.argv.index("--") + 1:]
SRC, OUT = args[0], args[1]
bpy.ops.wm.read_factory_settings(use_empty=True)
files = sorted(glob.glob(os.path.join(SRC, "SM_Ice*.fbx")))
x = 0.0; row = {}; objs = []
for f in files:
    bpy.ops.import_scene.fbx(filepath=f)
    ob = bpy.context.selected_objects[0]
    cls = os.path.basename(f)[6]
    bb = [ob.matrix_world @ Vector(c) for c in ob.bound_box]
    w = max(v.x for v in bb) - min(v.x for v in bb)
    ry = {"R": 0, "B": 12, "S": 40, "W": 80}[cls]
    rx = row.get(cls, 0.0)
    ob.location = (rx + w / 2 - (min(v.x for v in bb) + max(v.x for v in bb)) / 2 + ob.location.x, ry, 0)
    row[cls] = rx + w + 2.0
    objs.append(ob)
sc = bpy.context.scene
sc.render.engine = "BLENDER_WORKBENCH"
sc.display.shading.light = "STUDIO"
sc.display.shading.color_type = "SINGLE"
sc.display.shading.single_color = (0.85, 0.9, 0.95)
sc.display.shading.show_shadows = True
sc.display.shading.show_cavity = True
sc.render.resolution_x, sc.render.resolution_y = 2000, 1100
cam = bpy.data.objects.new("cam", bpy.data.cameras.new("cam"))
sc.collection.objects.link(cam); sc.camera = cam
span = max(row.values())
cam.location = (span * 0.42, -45, 14)
cam.rotation_euler = (math.radians(78), 0, 0)
cam.data.lens = 22
sc.render.filepath = OUT
bpy.ops.render.render(write_still=True)
print("preview", OUT, len(objs))
