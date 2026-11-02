# Blender (headless): ice lips for the crevasse crossings (UE ACrevasseCrossing, placed by place_crevasses.py).
#   blender -b -P Tools/Blender/crevasse_lip.py -- <out_dir> [count]
# One lip = a snow ramp rising along +X to a crest at x = 0, then a sheer (slightly overhanging) ice wall dropping
# into the crevasse and on below the terrain. Two lips facing each other make the slot; the ladder rests on the
# crests. Y = along the crevasse; the crest falls away near both ends so the lip melts into the glacier.
# Origin = middle of the crest edge at crest height minus H (so it sits on the ground for z-scale 1).
# The last P metres before the edge are a flat landing so the ladder end lies on snow, not over the ramp.
# Metres in Blender, plain-metre FBX export (no bake_space_transform), like serac.py.
import bpy, bmesh, sys, os, random, math
from mathutils import Vector, noise

args = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
OUT = args[0] if args else os.path.abspath("Art/Blender/Export/Crevasse")
COUNT = int(args[1]) if len(args) > 1 else 2
os.makedirs(OUT, exist_ok=True)

H, L, W, D = 6.0, 15.0, 36.0, 9.0    # crest height, lip length (ramp + landing), length along the crevasse, wall depth below ground
P = 3.2                                  # flat landing at the crest, packed by climbers, where the ladder end rests


def lip(seed):
    rng = random.Random(seed)
    off = Vector((seed * 17.3, seed * 5.1, seed * 2.9))
    NX, NY = 40, 72
    def edge_x(y):    # the crevasse is not ruler-straight
        return 0.5 * noise.noise(Vector((0.0, y * 0.09, 0.0)) + off) + 0.2 * noise.noise(Vector((0.0, y * 0.4, 1.0)) + off)
    def crest(y):     # crest height along the crevasse, falling away at the ends
        e = min(abs(y) / (W / 2), 1.0)
        return H * (1.0 - e ** 4) * (1.0 + 0.12 * noise.noise(Vector((y * 0.15, 3.0, 0.0)) + off)) + 0.3
    bm = bmesh.new()
    top, bot = [], []
    for j in range(NY + 1):
        y = -W / 2 + W * j / NY
        ex, ch = edge_x(y), crest(y)
        rt, rb = [], []
        for i in range(NX + 1):
            u = i / NX                                  # 0 = ramp foot, 1 = crest edge
            x = -L + (L + ex) * u
            r = min(max((x + L) / (L - P), 0.0), 1.0)   # 0 = ramp foot, 1 = start of the landing
            z = ch * (r ** 1.25) - 0.6 * (1 - r)        # ramp foot buried 0.6 m, flat landing after it
            if i == NX:
                x += 0.35                               # overhanging cornice at the crest
            rt.append(bm.verts.new((x, y, z)))
            rb.append(bm.verts.new((x - (0.0 if i < NX else 0.6), y, -D)))
        top.append(rt); bot.append(rb)
    for j in range(NY):
        for i in range(NX):
            bm.faces.new((top[j][i], top[j][i + 1], top[j + 1][i + 1], top[j + 1][i]))
            bm.faces.new((bot[j][i], bot[j + 1][i], bot[j + 1][i + 1], bot[j][i + 1]))
        # crest wall and ramp-foot wall
        bm.faces.new((top[j][NX], bot[j][NX], bot[j + 1][NX], top[j + 1][NX]))
        bm.faces.new((top[j][0], top[j + 1][0], bot[j + 1][0], bot[j][0]))
    for i in range(NX):                                   # end caps
        bm.faces.new((top[0][i], bot[0][i], bot[0][i + 1], top[0][i + 1]))
        bm.faces.new((top[NY][i], top[NY][i + 1], bot[NY][i + 1], bot[NY][i]))
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    me = bpy.data.meshes.new(f"lip_{seed}")
    bm.to_mesh(me); bm.free()
    ob = bpy.data.objects.new(f"SM_CrevasseLip_{seed:02d}", me)
    bpy.context.scene.collection.objects.link(ob)
    bpy.context.view_layer.objects.active = ob
    ob.select_set(True)

    rm = ob.modifiers.new("remesh", "REMESH"); rm.mode = "VOXEL"; rm.voxel_size = 0.14
    bpy.ops.object.modifier_apply(modifier="remesh")
    for v in me.vertices:
        p, n = v.co, v.normal
        if n.x > 0.4 and p.x > -1.5:
            # crevasse wall: long vertical melt grooves, horizontal annual layers, broken crust at the top
            groove = noise.noise(Vector((p.x * 0.8, p.y * 0.8, p.z * 0.1)) + off) * 0.25
            layer = math.sin(p.z * 2.3 + noise.noise(p * 0.3 + off) * 2.0) * 0.04
            crumble = noise.fractal(p * 0.9 + off, 0.55, 2.2, 4) * (0.08 + 0.18 * max(0.0, min(1.0, p.z / H)))
            v.co = p + n * (groove + layer + crumble)
        elif n.z > 0.3:
            # snow ramp: soft wind-packed undulation, a few sastrugi; the landing stays nearly flat (trampled)
            flat = 0.15 if p.x > -P else 1.0
            v.co = p + n * flat * (noise.noise(p * 0.25 + off) * 0.25 + noise.noise(Vector((p.x * 1.5, p.y * 0.4, 0)) + off) * 0.04)
        else:
            v.co = p + n * noise.fractal(p * 0.6 + off, 0.5, 2.0, 3) * 0.15
    me.update()
    sm = ob.modifiers.new("smooth", "SMOOTH"); sm.factor = 0.3; sm.iterations = 1
    bpy.ops.object.modifier_apply(modifier="smooth")
    dec = ob.modifiers.new("dec", "DECIMATE"); dec.ratio = min(1.0, 24000 / max(len(me.polygons), 1))
    bpy.ops.object.modifier_apply(modifier="dec")
    bpy.ops.object.shade_smooth()
    bpy.ops.object.mode_set(mode="EDIT")
    bpy.ops.mesh.select_all(action="SELECT")
    bpy.ops.uv.cube_project(cube_size=6.0)
    bpy.ops.object.mode_set(mode="OBJECT")
    return ob, len(me.polygons)


for i in range(COUNT):
    bpy.ops.wm.read_factory_settings(use_empty=True)
    ob, tris = lip(i + 1)
    path = os.path.join(OUT, ob.name + ".fbx")
    bpy.ops.export_scene.fbx(filepath=path, use_selection=True, apply_unit_scale=True, mesh_smooth_type="FACE", add_leaf_bones=False)
    print("exported", path, "faces", tris)
