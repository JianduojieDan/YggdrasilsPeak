# Imports a skinned glb (UE-mannequin bone names), poses it (arms up 60, elbows 70, knees 60, head 40) and renders
# front + side.   blender -b --factory-startup -P rig_pose_test.py -- <rigged.glb> <out_prefix> [arm|full]
import bpy, sys, os, math
from mathutils import Vector, Matrix
A = sys.argv[sys.argv.index("--") + 1:]
SRC, PRE = os.path.abspath(A[0]), os.path.abspath(A[1])
MODE = A[2] if len(A) > 2 else "full"
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.gltf(filepath=SRC)
arm = next(o for o in bpy.data.objects if o.type == 'ARMATURE')
for o in list(bpy.data.objects):
    if o.type == 'MESH' and o.parent != arm: bpy.data.objects.remove(o)
bpy.context.view_layer.objects.active = arm; bpy.ops.object.mode_set(mode='POSE')
P = arm.pose.bones
def rot(n, axis, deg):
    pb = P[n]; h = pb.head.copy()
    pb.matrix = Matrix.Translation(h) @ Matrix.Rotation(math.radians(deg), 4, axis.normalized()) @ Matrix.Translation(-h) @ pb.matrix
    bpy.context.view_layer.update()
fwd = Vector((0, -1, 0))
for sfx, sgn in (("l", 1), ("r", -1)):
    rot("upperarm_" + sfx, Vector((0, 1, 0)), -60 * sgn)
    if MODE == "full":
        v = (P["hand_" + sfx].head - P["lowerarm_" + sfx].head).normalized()
        rot("lowerarm_" + sfx, v.cross(fwd), 70)
        v = (P["foot_" + sfx].head - P["calf_" + sfx].head).normalized()
        rot("calf_" + sfx, v.cross(-fwd), 60)
if MODE == "full": rot("head", Vector((0, 0, 1)), 40)
bpy.ops.object.mode_set(mode='OBJECT')
sc = bpy.context.scene
sc.render.engine = 'BLENDER_WORKBENCH'
sc.display.shading.light = 'STUDIO'; sc.display.shading.color_type = 'TEXTURE'
sc.render.resolution_x = sc.render.resolution_y = 900
w = bpy.data.worlds.new("W"); sc.world = w; w.color = (0.75, 0.77, 0.8)
cam = bpy.data.objects.new("Cam", bpy.data.cameras.new("Cam")); sc.collection.objects.link(cam); sc.camera = cam
cam.data.type = 'ORTHO'; cam.data.ortho_scale = 2.4
for tag, loc, r in (("front", (0, -6, 1.05), (90, 0, 0)), ("side", (6, 0, 1.05), (90, 0, 90))):
    cam.location = loc; cam.rotation_euler = [math.radians(a) for a in r]
    sc.render.filepath = "%s_%s.png" % (PRE, tag)
    bpy.ops.render.render(write_still=True)
    print("POSE render", sc.render.filepath)
