# blender -b --factory-startup -P Tools/Blender/prayer_flag_cloth.py
# 2026-11-21 (#6 prayer flags looked fake / hovered): one prayer-flag cloth card for the HISM flag lines. 1 m x 1 m,
# pivot at the TOP CENTRE (the point that hangs on the cord), hanging down -Z in the XZ plane, facing +-Y; 8 x 12 quads so
# the material's wind flutter (world position offset, stronger toward the free bottom edge) can bend it; a faint baked
# crinkle (two low sine bumps + a sag between the top corners) so a still flag already reads as thin cotton, not a board;
# UV (0,0) top-left .. (1,1) bottom-right, matching the flag prints. -> Art/Flags/SM_PrayerFlagCloth.fbx (cm, root scale 1)
import bpy, bmesh, math, random, os
random.seed(7)
OUT = "F:/UE_project/YggdrasilsPeak/Art/Flags/SM_PrayerFlagCloth.fbx"
bpy.ops.wm.read_factory_settings(use_empty=True)
sc = bpy.context.scene
sc.unit_settings.system = 'METRIC'; sc.unit_settings.scale_length = 0.01   # 1 unit = 1 cm in the FBX
NX, NZ = 8, 12
me = bpy.data.meshes.new("SM_PrayerFlagCloth"); ob = bpy.data.objects.new("SM_PrayerFlagCloth", me)
sc.collection.objects.link(ob)
bm = bmesh.new()
uvl = bm.loops.layers.uv.new("UVMap")
verts = {}
for j in range(NZ + 1):
    for i in range(NX + 1):
        u, v = i / NX, j / NZ
        x = (u - 0.5) * 100.0
        z = -v * 100.0 + 1.6 * math.sin(math.pi * u) * (1.0 - v) * 0.0          # top edge straight (it is sewn to the cord)
        y = (2.2 * math.sin(math.pi * 1.5 * u + 0.6) * math.sin(math.pi * 0.9 * v)
             + 1.4 * math.sin(math.pi * 3.0 * v + 1.3 * u) * v
             + random.uniform(-0.25, 0.25) * v)
        verts[(i, j)] = bm.verts.new((x, y, z))
bm.verts.ensure_lookup_table()
for j in range(NZ):
    for i in range(NX):
        f = bm.faces.new((verts[(i, j)], verts[(i, j + 1)], verts[(i + 1, j + 1)], verts[(i + 1, j)]))
        for l in f.loops:
            ii = [k for k, vv in verts.items() if vv == l.vert][0]
            l[uvl].uv = (ii[0] / NX, 1.0 - ii[1] / NZ)
bm.normal_update()
bm.to_mesh(me); bm.free()
for p in me.polygons: p.use_smooth = True
os.makedirs(os.path.dirname(OUT), exist_ok=True)
bpy.ops.export_scene.fbx(filepath=OUT, use_selection=False, apply_unit_scale=True, global_scale=1.0, axis_forward='-Y', axis_up='Z',
                         mesh_smooth_type='FACE', add_leaf_bones=False)
print("FLAGMESH", OUT, len(me.vertices), "verts")
