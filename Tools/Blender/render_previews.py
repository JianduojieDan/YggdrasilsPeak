# Renders one framed preview per model from the saved .blend files into Art/Blender/Preview/<name>.png
import sys, os
sys.path.append(os.path.dirname(__file__))
from ygg_blend import *

for blend in ("ladder", "oxygen", "food", "suit_temp"):
    bpy.ops.wm.open_mainfile(filepath=os.path.join(OUT, f"{blend}.blend"))
    meshes = [o for o in bpy.data.objects if o.type == 'MESH']
    for o in meshes:
        for other in meshes:
            other.hide_render = other is not o
        o.location = (0, 0, 0) if o.parent is None else o.location
        s = bpy.context.scene
        s.world = s.world or bpy.data.worlds.new("W")
        s.display.shading.background_type = 'VIEWPORT'
        preview(o.name, [o], res=900)
