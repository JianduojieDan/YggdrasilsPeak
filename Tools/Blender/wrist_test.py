# Close-up render of both wrists with the hands rolled and bent, to check for twisting at the wrist.
#   blender --background --factory-startup --python wrist_test.py -- <Name>
import sys, os, math, bpy
from mathutils import Vector
sys.path.append(os.path.dirname(__file__))
from ygg_blend import OUT

NAME = sys.argv[sys.argv.index("--") + 1]
bpy.ops.wm.open_mainfile(filepath=os.path.join(OUT, f"{NAME}.blend"))
arm = bpy.data.objects["root"]
P = arm.pose.bones
for pb in P:
    pb.rotation_mode = 'XYZ'; pb.rotation_euler = (0, 0, 0)
# forearms raised in front, hands rolled and bent - typical first-person poses
for s, sg in (("l", 1), ("r", -1)):
    P[f"upperarm_{s}"].rotation_euler = (0, 0, math.radians(40))
    P[f"lowerarm_{s}"].rotation_euler = (0, 0, math.radians(-70))
    P[f"hand_{s}"].rotation_euler = (math.radians(45), 0, math.radians(25))
bpy.context.view_layer.update()
s = bpy.context.scene; s.render.engine = 'BLENDER_EEVEE'; s.render.resolution_x, s.render.resolution_y = 900, 500
w = bpy.data.worlds.new("W"); s.world = w; w.color = (0.6, 0.6, 0.65)
sun = bpy.data.objects.new("Sun", bpy.data.lights.new("Sun", 'SUN')); s.collection.objects.link(sun)
sun.rotation_euler = (0.8, 0.2, 0.6); sun.data.energy = 3
cam = bpy.data.objects.new("Cam", bpy.data.cameras.new("Cam")); s.collection.objects.link(cam); s.camera = cam
for s_ in ("l", "r"):
    hand = arm.matrix_world @ P[f"hand_{s_}"].head
    cam.location = hand + Vector((0, -0.45, 0.12))
    cam.rotation_euler = (hand - cam.location).to_track_quat('-Z', 'Y').to_euler()
    s.render.filepath = os.path.join(OUT, "Preview", f"{NAME}_wrist_{s_}.png"); bpy.ops.render.render(write_still=True)
