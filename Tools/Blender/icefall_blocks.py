# Blender (headless): SOLID ice blocks for the Khumbu Icefall, v2 (2026-11-10).
#   blender -b -P Tools/Blender/icefall_blocks.py -- <out_dir> [per_class]
# Built from the reference photos in Downloads/icefall_refs (walking height: "Atravesando el Khumbu Icefall";
# overview: KhumbuIcefall.jpg / the user's Alan Arnette photo):
#   - the ice breaks along flat fracture planes -> convex hull of FEW random points = big flat faces;
#   - many blocks are not convex: a second hull is cut out of some (a notch / a broken-off corner);
#   - fresh snow lies on every upward face as a soft rounded pillow that slightly overhangs the edges;
#   - side faces are crisp but crumbly; tall seracs have vertical melt grooves; everything is buried at the base.
# Every mesh is CLOSED (watertight): no hollow backs, no open bottoms (the old Gaussian-splat scans were facades).
# Classes: R rubble 0.6-2 m, B blocks 2-6 m, S serac towers 8-22 m tall, W transverse walls 12-25 m long.
# Metres in Blender; the FBX exporter's unit scale gives UE centimetres (no bake_space_transform).
import bpy, bmesh, sys, os, random, math
from mathutils import Vector, noise

args = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
OUT = args[0] if args else os.path.abspath("Art/Blender/Export/Icefall")
PER = int(args[1]) if len(args) > 1 else 6
os.makedirs(OUT, exist_ok=True)

CLASSES = {
    #      size ranges (w, d, h) m          hull pts  taper       lean  snow m       tris   voxel
    "R": dict(w=(0.6, 2.0), d=(0.5, 1.6), h=(0.4, 1.2), pts=(8, 12), taper=(0.7, 1.0), lean=0.35, snow=(0.1, 0.25), tris=4000, vox=0.025),
    "B": dict(w=(2.0, 6.0), d=(1.6, 4.5), h=(1.2, 3.5), pts=(9, 14), taper=(0.7, 1.0), lean=0.25, snow=(0.25, 0.5), tris=12000, vox=0.05),
    "S": dict(w=(7.0, 14.0), d=(5.0, 10.0), h=(6.0, 15.0), pts=(10, 16), taper=(0.75, 1.0), lean=0.12, snow=(0.5, 1.0), tris=26000, vox=0.16),
    "W": dict(w=(12.0, 25.0), d=(4.0, 8.0), h=(5.0, 11.0), pts=(14, 22), taper=(0.7, 0.95), lean=0.08, snow=(0.5, 0.9), tris=30000, vox=0.18),
}

def clear():
    bpy.ops.wm.read_factory_settings(use_empty=True)

def hull_object(name, pts):
    me = bpy.data.meshes.new(name)
    bm = bmesh.new()
    for p in pts:
        bm.verts.new(p)
    bmesh.ops.convex_hull(bm, input=bm.verts)
    bm.to_mesh(me); bm.free()
    ob = bpy.data.objects.new(name, me)
    bpy.context.scene.collection.objects.link(ob)
    return ob

def block_points(rng, c, w, d, h):
    taper = rng.uniform(*c["taper"])
    lean = Vector((rng.uniform(-1, 1), rng.uniform(-1, 1), 0)) * c["lean"]
    tilt = (rng.uniform(-0.35, 0.35), rng.uniform(-0.35, 0.35))
    pts = []
    for _ in range(rng.randint(*c["pts"])):
        z = rng.random() ** 0.6 * h
        s = 1.0 - (1.0 - taper) * (z / h)
        x, y = rng.uniform(-w, w) / 2 * s, rng.uniform(-d, d) / 2 * s
        z = min(z, h + x * tilt[0] + y * tilt[1])        # a sloping top plane: blocks, not spires
        pts.append(Vector((x, y, z)) + lean * z)
    for _ in range(6):   # buried base: no clean bottom face shows
        pts.append(Vector((rng.uniform(-w, w) / 2, rng.uniform(-d, d) / 2, -rng.uniform(0.1, 0.25) * h)))
    return pts

def select_only(ob):
    for o in bpy.context.scene.objects:
        o.select_set(False)
    ob.select_set(True)
    bpy.context.view_layer.objects.active = ob

def make(cls, seed):
    c = CLASSES[cls]
    rng = random.Random(hash((cls, seed)) & 0xFFFFFFFF)
    w, d, h = rng.uniform(*c["w"]), rng.uniform(*c["d"]), rng.uniform(*c["h"])
    ob = hull_object(f"SM_Ice{cls}_{seed:02d}", block_points(rng, c, w, d, h))
    select_only(ob)

    # a notch / broken-off corner on most blocks: boolean-subtract a second, smaller hull from an edge
    for _cut in range(2 if cls in ("S", "W") else 1):
      if rng.random() < (0.5 if cls == "R" else 0.8):
          cw, cd, ch = w * rng.uniform(0.3, 0.55), d * rng.uniform(0.4, 0.8), h * rng.uniform(0.35, 0.7)
          corner = Vector((rng.choice((-1, 1)) * w * 0.45, rng.choice((-1, 1)) * d * 0.4, h * rng.uniform(0.45, 0.9)))
          cut = hull_object("cut", [corner + Vector((rng.uniform(-cw, cw), rng.uniform(-cd, cd), rng.uniform(-ch, ch))) for _ in range(10)])
          b = ob.modifiers.new("cut", "BOOLEAN"); b.operation = "DIFFERENCE"; b.object = cut; b.solver = "EXACT"
          select_only(ob)
          bpy.ops.object.modifier_apply(modifier="cut")
          bpy.data.objects.remove(cut, do_unlink=True)

    # even triangles to shape
    rm = ob.modifiers.new("remesh", "REMESH"); rm.mode = "VOXEL"; rm.voxel_size = c["vox"]
    bpy.ops.object.modifier_apply(modifier="remesh")
    # voxel stair-steps on slanted planes -> smooth them away before shaping (big planes stay planes)
    pre = ob.modifiers.new("pre", "SMOOTH"); pre.factor = 1.0; pre.iterations = 2
    bpy.ops.object.modifier_apply(modifier="pre")
    me = ob.data
    me.update()

    # surface: crumbly fracture faces, vertical melt grooves on the tall ones, small at the base
    off = Vector((seed * 13.1, seed * 7.7, hash(cls) % 97 * 0.31))
    size = max(w, d, h)
    snow_t = rng.uniform(*c["snow"])
    top = me.vertex_groups if False else None
    vg = ob.vertex_groups.new(name="snowtop")
    # how much snow each vertex carries: from the normal, then averaged over its neighbours so the pillow is soft
    # and even (raw voxel normals are noisy and dimpled the snow like a golf ball)
    bm = bmesh.new(); bm.from_mesh(me); bm.verts.ensure_lookup_table()
    upw = [min(max((v.normal.z - 0.25) / 0.55, 0.0), 1.0) for v in bm.verts]
    nbr = [[e.other_vert(v).index for e in v.link_edges] for v in bm.verts]
    bm.free()
    for _ in range(10):
        upw = [(upw[i] + sum(upw[j] for j in nb)) / (1 + len(nb)) for i, nb in enumerate(nbr)]
    for v in me.vertices:
        p, n = v.co.copy(), v.normal.copy()
        crumble = noise.fractal(p * (0.9 / size ** 0.5) + off, 0.5, 2.0, 3)
        grooves = noise.noise(Vector((p.x * 1.4, p.y * 1.4, p.z * 0.1)) + off) if cls in ("S", "W") else 0.0
        side = 1.0 - min(max(n.z, 0.0), 1.0)
        disp = (crumble * 0.012 * size ** 0.6 + grooves * 0.06) * side
        # snow pillow: upward faces swell up and out, rounded by the smoothing below
        up = upw[v.index]
        up = up * up * (3 - 2 * up)
        v.co = p + n * disp + Vector((0, 0, 1)) * snow_t * up
        vg.add([v.index], up, "REPLACE")
    me.update()
    sm = ob.modifiers.new("pillow", "SMOOTH"); sm.factor = 0.9; sm.iterations = 20; sm.vertex_group = "snowtop"
    bpy.ops.object.modifier_apply(modifier="pillow")
    sm2 = ob.modifiers.new("soft", "LAPLACIANSMOOTH"); sm2.iterations = 1; sm2.lambda_factor = 0.4; sm2.lambda_border = 0.0; sm2.use_volume_preserve = True; sm2.use_normalized = True
    bpy.ops.object.modifier_apply(modifier="soft")
    dec = ob.modifiers.new("dec", "DECIMATE"); dec.ratio = min(1.0, c["tris"] / max(len(me.polygons), 1))
    bpy.ops.object.modifier_apply(modifier="dec")
    bpy.ops.object.shade_smooth()
    bpy.ops.object.mode_set(mode="EDIT")
    bpy.ops.mesh.select_all(action="SELECT")
    bpy.ops.mesh.normals_make_consistent(inside=False)
    bpy.ops.uv.cube_project(cube_size=4.0)
    bpy.ops.object.mode_set(mode="OBJECT")
    # watertight check (every edge shared by exactly two faces)
    bm = bmesh.new(); bm.from_mesh(me)
    open_edges = sum(1 for e in bm.edges if not e.is_manifold)
    bm.free()
    return ob, len(me.polygons), (w, d, h), open_edges

report = []
for cls in CLASSES:
    for i in range(PER):
        clear()
        ob, tris, dims, open_edges = make(cls, i + 1)
        path = os.path.join(OUT, ob.name + ".fbx")
        select_only(ob)
        bpy.ops.export_scene.fbx(filepath=path, use_selection=True, apply_unit_scale=True, mesh_smooth_type="FACE", add_leaf_bones=False)
        report.append(f"{ob.name} tris {tris} size {dims[0]:.1f}x{dims[1]:.1f}x{dims[2]:.1f} m open_edges {open_edges}")
        print(report[-1], flush=True)
open(os.path.join(OUT, "report.txt"), "w").write("\n".join(report))
