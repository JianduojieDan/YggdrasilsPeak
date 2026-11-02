# Blender (headless): procedural serac / ice-block meshes for the Khumbu Icefall.
#   blender -b -P Tools/Blender/serac.py -- <out_dir> [count]
# Each serac: a faceted block from the convex hull of random points in a tapered tall box (sharp fracture planes),
# voxel-remeshed, then displaced gently (vertical melt grooves, a little crumbling at the top) so the flat fracture
# faces stay but it does not read as a clean primitive, decimated to ~16k tris, box-projected UVs (the UE material projects in world space anyway).
# Metres in Blender; the FBX exporter's unit scale gives UE centimetres.
import bpy, bmesh, sys, os, random, math
from mathutils import Vector, noise

args = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
OUT = args[0] if args else os.path.abspath("Art/Blender/Export/Seracs")
COUNT = int(args[1]) if len(args) > 1 else 6
os.makedirs(OUT, exist_ok=True)

def clear():
    bpy.ops.wm.read_factory_settings(use_empty=True)

def serac(seed):
    rng = random.Random(seed)
    w, d, h = rng.uniform(8, 16), rng.uniform(6, 12), rng.uniform(7, 16)
    taper = rng.uniform(0.55, 0.9)           # narrower at the top
    lean = Vector((rng.uniform(-0.3, 0.3), rng.uniform(-0.3, 0.3), 0))  # jumbled, toppling blocks
    pts = []
    for _ in range(rng.randint(12, 20)):     # few points = big flat fracture faces
        z = rng.random() ** 0.8 * h
        s = 1.0 - (1.0 - taper) * (z / h)
        pts.append(Vector((rng.uniform(-w, w) / 2 * s, rng.uniform(-d, d) / 2 * s, z)) + lean * z)
    # a few points below ground so the base is buried, not a clean bottom face
    for _ in range(6):
        pts.append(Vector((rng.uniform(-w, w) / 2, rng.uniform(-d, d) / 2, -rng.uniform(0.5, 1.5))))
    me = bpy.data.meshes.new(f"serac_{seed}")
    bm = bmesh.new()
    for p in pts:
        bm.verts.new(p)
    bmesh.ops.convex_hull(bm, input=bm.verts)
    bm.to_mesh(me); bm.free()
    ob = bpy.data.objects.new(f"SM_Serac_{seed:02d}", me)
    bpy.context.scene.collection.objects.link(ob)
    bpy.context.view_layer.objects.active = ob
    ob.select_set(True)

    # even triangles to displace
    rm = ob.modifiers.new("remesh", "REMESH"); rm.mode = "VOXEL"; rm.voxel_size = 0.12
    bpy.ops.object.modifier_apply(modifier="remesh")

    # displacement: vertical melt ridges (stretched noise), crumbly fracture detail, stronger on the upper edges
    off = Vector((seed * 13.1, seed * 7.7, seed * 3.3))
    for v in me.vertices:
        p = v.co
        n = v.normal
        ridge = noise.noise(Vector((p.x * 0.9, p.y * 0.9, p.z * 0.12)) + off)           # long vertical grooves
        crumble = noise.fractal(p * 0.7 + off, 0.55, 2.2, 4)                            # broken surface
        fine = noise.noise(p * 3.5 + off) * 0.25
        top = min(max(p.z / h, 0.0), 1.0)
        amp = 0.05 + 0.2 * top ** 2                                                     # crumbling only near the top
        v.co = p + n * (ridge * 0.22 + crumble * amp + fine * 0.05)
    me.update()

    sm = ob.modifiers.new("smooth", "SMOOTH"); sm.factor = 0.3; sm.iterations = 1
    bpy.ops.object.modifier_apply(modifier="smooth")
    dec = ob.modifiers.new("dec", "DECIMATE"); dec.ratio = min(1.0, 16000 / max(len(me.polygons), 1))
    bpy.ops.object.modifier_apply(modifier="dec")
    bpy.ops.object.shade_smooth()

    bpy.ops.object.mode_set(mode="EDIT")
    bpy.ops.mesh.select_all(action="SELECT")
    bpy.ops.uv.cube_project(cube_size=6.0)
    bpy.ops.object.mode_set(mode="OBJECT")
    return ob, len(me.polygons)

for i in range(COUNT):
    clear()
    ob, tris = serac(i + 1)
    path = os.path.join(OUT, ob.name + ".fbx")
    # plain metres; bake_space_transform must stay off (it shrank the mesh 100x and laid it on its side in UE)
    bpy.ops.export_scene.fbx(filepath=path, use_selection=True, apply_unit_scale=True, mesh_smooth_type="FACE", add_leaf_bones=False)
    print("exported", path, "faces", tris)
