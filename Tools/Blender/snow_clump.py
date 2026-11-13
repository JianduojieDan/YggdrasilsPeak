# A small lumpy snow clump (2026-11-13: the step "puff" of big translucent sprites was costly and read as paper; now each
# step flicks a few solid clumps off the boot). 100 cm across like the engine shapes (scaled to 0.5-3 cm per instance),
# centred, ~120 triangles, smooth shaded. Run: blender.exe --background --factory-startup --python Tools/Blender/snow_clump.py
import sys, os, math, random
sys.path.append(os.path.dirname(__file__))
from ygg_blend import *
import bpy, bmesh
from mathutils import Vector, noise

reset()
rng = random.Random(7)
me = bpy.data.meshes.new("SnowClump")
bm = bmesh.new()
bmesh.ops.create_icosphere(bm, subdivisions=2, radius=0.5)
for v in bm.verts:
    d = v.co.normalized()
    n = noise.noise(d * 2.2 + Vector((3.1, 1.7, 0.4)))            # broken, not round
    v.co = d * 0.5 * (1.0 + 0.28 * n + 0.08 * rng.uniform(-1, 1))
    v.co.z *= 0.72                                                 # a little flattened
bm.to_mesh(me); bm.free()
for p in me.polygons: p.use_smooth = True
o = bpy.data.objects.new("SnowClump", me); bpy.context.collection.objects.link(o)
me.materials.append(mat("M_SnowClump", (0.9, 0.92, 0.95), rough=0.65))
path = os.path.join(OUT, "Export", "Snow", "SM_SnowClump.fbx")
os.makedirs(os.path.dirname(path), exist_ok=True)
bpy.ops.object.select_all(action='DESELECT'); o.select_set(True); bpy.context.view_layer.objects.active = o
bpy.ops.object.mode_set(mode='EDIT'); bpy.ops.mesh.select_all(action='SELECT'); bpy.ops.uv.smart_project(); bpy.ops.object.mode_set(mode='OBJECT')
bpy.ops.export_scene.fbx(filepath=path, use_selection=True, apply_scale_options='FBX_SCALE_UNITS', mesh_smooth_type='FACE', add_leaf_bones=False, bake_anim=False)
print("EXPORTED", path, "tris", sum(len(p.vertices) - 2 for p in me.polygons))
