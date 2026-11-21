# blender -b --factory-startup -P Tools/Blender/fire_card.py
# 2026-11-21 (#86 the camp fire pit had no fire): one upright card for the flame / smoke materials. 1 m x 1 m, pivot at the
# BOTTOM CENTRE, in the XZ plane facing +-Y, 4 x 6 quads, UV v = 0 at the bottom (UE flips it: V = 1 at the bottom).
import bpy, bmesh, os
OUT = "F:/UE_project/YggdrasilsPeak/Art/Fire/SM_FireCard.fbx"
bpy.ops.wm.read_factory_settings(use_empty=True)
sc = bpy.context.scene; sc.unit_settings.system = 'METRIC'; sc.unit_settings.scale_length = 0.01
me = bpy.data.meshes.new("SM_FireCard"); ob = bpy.data.objects.new("SM_FireCard", me); sc.collection.objects.link(ob)
bm = bmesh.new(); uvl = bm.loops.layers.uv.new("UVMap")
NX, NZ = 4, 6
V = {(i, j): bm.verts.new(((i / NX - 0.5) * 100.0, 0.0, j / NZ * 100.0)) for j in range(NZ + 1) for i in range(NX + 1)}
for j in range(NZ):
    for i in range(NX):
        f = bm.faces.new((V[(i, j)], V[(i + 1, j)], V[(i + 1, j + 1)], V[(i, j + 1)]))
        for l, (a, b) in zip(f.loops, ((i, j), (i + 1, j), (i + 1, j + 1), (i, j + 1))):
            l[uvl].uv = (a / NX, b / NZ)
bm.to_mesh(me); bm.free()
os.makedirs(os.path.dirname(OUT), exist_ok=True)
bpy.ops.export_scene.fbx(filepath=OUT, apply_unit_scale=True, global_scale=1.0, axis_forward='-Y', axis_up='Z', mesh_smooth_type='FACE')
print("FIRECARD", OUT, len(me.vertices))
