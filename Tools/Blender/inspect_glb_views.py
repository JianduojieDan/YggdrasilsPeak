# Renders the untouched GLB from front, back, left and a close-up of the legs, and checks for loose pieces.
#   blender --background --factory-startup --python inspect_glb_views.py -- <file.glb> <out_prefix>
import sys, os, bpy, bmesh
from mathutils import Vector
sys.path.append(os.path.dirname(__file__))
from ygg_blend import reset, OUT

src, name = sys.argv[sys.argv.index("--") + 1:][:2]
reset()
bpy.ops.import_scene.gltf(filepath=src)
body = next(o for o in bpy.data.objects if o.type == 'MESH')
bm = bmesh.new(); bm.from_mesh(body.data)
islands = 0; seen = set()
for v in bm.verts:
    if v.index in seen: continue
    islands += 1; stack = [v]; seen.add(v.index)
    while stack:
        x = stack.pop()
        for e in x.link_edges:
            o = e.other_vert(x)
            if o.index not in seen: seen.add(o.index); stack.append(o)
print("LOOSE PIECES", islands, "non-manifold edges", sum(1 for e in bm.edges if not e.is_manifold))
bm.free()

lo = Vector(body.bound_box[0]); hi = Vector(body.bound_box[6])
ctr = body.matrix_world @ ((lo + hi) / 2); h = (hi - lo).length
s = bpy.context.scene; s.render.engine = 'BLENDER_EEVEE'; s.render.resolution_x, s.render.resolution_y = 600, 800
w = bpy.data.worlds.new("W"); s.world = w; w.color = (0.6, 0.6, 0.65)
sun = bpy.data.objects.new("Sun", bpy.data.lights.new("Sun", 'SUN')); s.collection.objects.link(sun)
sun.rotation_euler = (0.8, 0.2, 0.6); sun.data.energy = 3
cam = bpy.data.objects.new("Cam", bpy.data.cameras.new("Cam")); s.collection.objects.link(cam); s.camera = cam
legs = ctr - Vector((0, 0, h * 0.22))
for tag, d, c, dist in (("front", (0, -1, 0.1), ctr, 1.6), ("back", (0, 1, 0.1), ctr, 1.6), ("left", (1, 0, 0.1), ctr, 1.6),
                        ("legs_front", (0.3, -1, 0.05), legs, 0.8), ("legs_back", (-0.3, 1, 0.05), legs, 0.8)):
    cam.location = c + Vector(d).normalized() * h * dist
    cam.rotation_euler = (c - cam.location).to_track_quat('-Z', 'Y').to_euler()
    s.render.filepath = os.path.join(OUT, "Preview", f"{name}_{tag}.png"); bpy.ops.render.render(write_still=True)
