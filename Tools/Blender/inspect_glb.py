# Prints what is inside a GLB and renders a textured front + side preview.
#   blender --background --factory-startup --python inspect_glb.py -- <file.glb> <out_prefix>
import sys, os, bpy
from mathutils import Vector
sys.path.append(os.path.dirname(__file__))
from ygg_blend import reset, OUT

src, name = sys.argv[sys.argv.index("--") + 1:][:2]
reset()
bpy.ops.import_scene.gltf(filepath=src)
lo = Vector((1e9,) * 3); hi = Vector((-1e9,) * 3)
for o in bpy.data.objects:
    info = f"OBJ {o.name} {o.type}"
    if o.type == 'MESH':
        tris = sum(len(p.vertices) - 2 for p in o.data.polygons)
        info += f" tris={tris} mats={[m.name for m in o.data.materials]} vgroups={len(o.vertex_groups)} uv={len(o.data.uv_layers)}"
        for c in o.bound_box:
            w = o.matrix_world @ Vector(c); lo = Vector(map(min, lo, w)); hi = Vector(map(max, hi, w))
    print(info)
for i in bpy.data.images:
    print("IMG", i.name, tuple(i.size))
print("BOUNDS", tuple(round(x, 3) for x in lo), tuple(round(x, 3) for x in hi))

s = bpy.context.scene
s.render.engine = 'BLENDER_EEVEE'
s.render.resolution_x, s.render.resolution_y = 700, 900
w = bpy.data.worlds.new("W"); s.world = w; w.color = (0.6, 0.6, 0.65)
sun = bpy.data.objects.new("Sun", bpy.data.lights.new("Sun", 'SUN')); s.collection.objects.link(sun)
sun.rotation_euler = (0.8, 0.2, 0.6); sun.data.energy = 3
ctr = (lo + hi) / 2; h = (hi - lo).length
cam = bpy.data.objects.new("Cam", bpy.data.cameras.new("Cam")); s.collection.objects.link(cam); s.camera = cam
for tag, d in (("front", Vector((0, -1, 0.1))), ("side", Vector((1, 0, 0.1)))):
    cam.location = ctr + d.normalized() * h * 1.6
    cam.rotation_euler = (ctr - cam.location).to_track_quat('-Z', 'Y').to_euler()
    s.render.filepath = os.path.join(OUT, "Preview", f"{name}_{tag}.png")
    bpy.ops.render.render(write_still=True)
