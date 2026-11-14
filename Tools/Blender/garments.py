# Clothing for the Chomolungma cast, built on a MetaHuman body (and its face mesh, for hats/scarves).
#   blender -b --factory-startup --python Tools/Blender/garments.py -- <outfit> [body.fbx|-] [face.fbx|-] [tag]
# outfits: see OUTFITS below. Body/face FBX are exported from UE (template: Art/Blender/Source/MetaHuman/SKM_MH_*.fbx;
# per character: the built MetaHuman's body + face). Output: Art/Blender/Export/Garments/SKM_G_<outfit>[_tag].fbx
# (one skeletal mesh on metahuman_base_skel bones, material slots "<outfit>_<piece>_<role>") + .json (slot -> fabric,
# tint, tiling) for Tools/Story/import_garments.py, and a preview render in Art/Blender/Preview/.
#
# How a garment is made: faces of the body inside the garment zone are copied (so every vertex keeps the body's exact
# skin weights), cut cleanly with planes (hem, cuffs, neck), subdivided, inflated along the normal (+ down baffles,
# folds, ribs), smoothed so the body's muscles disappear, pushed out of the body and of the garments underneath
# (layering: boots < trousers < jacket), then collars / cuffs / hems are extruded from the open edges and the whole
# thing gets a thin solidify so hems have a real edge. Skirts (chuba) are lofted around both legs. UVs are in metres
# so the downloaded Poly Haven fabrics tile at real scale; vertex colour R = baked seam/fold darkness.
import sys, os, math, json
sys.path.append(os.path.dirname(os.path.abspath(__file__)))
from ygg_blend import *
import bmesh
from mathutils import Vector, Matrix
from mathutils.bvhtree import BVHTree
from mathutils.kdtree import KDTree

argv = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
OUTFIT = argv[0] if argv else "nima"
SRC = os.path.join(OUT, "Source", "MetaHuman")
BODY_FBX = argv[1] if len(argv) > 1 and argv[1] != "-" else os.path.join(SRC, "SKM_MH_Body.fbx")
FACE_FBX = argv[2] if len(argv) > 2 and argv[2] != "-" else os.path.join(SRC, "SKM_MH_Face.fbx")
TAG = ("_" + argv[3]) if len(argv) > 3 else ""
DEST = os.path.join(OUT, "Export", "Garments")

# fabric ids = Poly Haven sets in Downloads/polyhaven_fabric_2026-11-14 ("" = flat colour); tile = metres per repeat
FAB = {"nylon": ("stretch_poplin", 0.18, 0.42), "softshell": ("jersey_melange", 0.25, 0.75), "fleece": ("polar_fleece", 0.22, 0.95),
       "knit": ("knitted_fleece", 0.12, 0.95), "wool": ("caban", 0.35, 0.9), "tweed": ("poly_wool_herringbone", 0.3, 0.9),
       "leather": ("brown_leather", 0.35, 0.6), "rubber": ("", 1.0, 0.85), "brocade": ("quatrefoil_jacquard_fabric", 0.16, 0.55),
       "floral": ("floral_jacquard", 0.2, 0.7), "linen": ("rough_linen", 0.3, 0.9), "pangden": ("@pangden", 1.0, 0.85),
       "webbing": ("hessian_230", 0.08, 0.8), "metal": ("", 1.0, 0.3)}

OUTFITS = {
    # Nima, the old sirdar: mustard down jacket with black yoke, maroon knit hat, dark softshell trousers, leather boots
    "nima": dict(boots=("leather", (0.30, 0.18, 0.10)), pants=dict(style="softshell", main=(0.07, 0.07, 0.08)),
                 jacket=dict(style="down", main=(0.80, 0.42, 0.05), panel=(0.025, 0.025, 0.03), trim=(0.02, 0.02, 0.025)),
                 hat=dict(style="beanie", main=(0.36, 0.04, 0.05), pom=True)),
    # Aunt Pema: maroon wool chuba, brocade edge, red sash, striped pangden apron, dark red head scarf
    "pema": dict(boots=("leather", (0.16, 0.10, 0.07)), chuba=dict(main=(0.20, 0.07, 0.06), trim=(0.85, 0.62, 0.22), sash=(0.62, 0.10, 0.05)),
                 apron=True, hat=dict(style="scarf", main=(0.40, 0.07, 0.09))),
    # clients
    "marek": dict(boots=("mountain", (0.85, 0.55, 0.04)), pants=dict(style="softshell", main=(0.10, 0.10, 0.11)),
                  jacket=dict(style="shell", main=(0.16, 0.18, 0.07), trim=(0.02, 0.02, 0.025)), hat=dict(style="beanie", main=(0.12, 0.12, 0.13), pom=False)),
    "elena": dict(boots=("mountain", (0.20, 0.20, 0.22)), pants=dict(style="softshell", main=(0.13, 0.12, 0.12)),
                  jacket=dict(style="shell", main=(0.62, 0.16, 0.04), trim=(0.02, 0.02, 0.025))),
    "jiwoo": dict(boots=("mountain", (0.85, 0.55, 0.04)), pants=dict(style="softshell", main=(0.08, 0.08, 0.09)),
                  jacket=dict(style="down", main=(0.05, 0.11, 0.28), panel=(0.02, 0.02, 0.03), trim=(0.02, 0.02, 0.025)),
                  hat=dict(style="beanie", main=(0.75, 0.74, 0.70), pom=True)),
    # porters
    "porter_a": dict(boots=("trainer", (0.22, 0.22, 0.24)), pants=dict(style="softshell", main=(0.16, 0.15, 0.13)),
                     jacket=dict(style="fleece", main=(0.33, 0.05, 0.06), trim=(0.04, 0.04, 0.045)), hat=dict(style="beanie", main=(0.08, 0.10, 0.18), pom=False)),
    "porter_b": dict(boots=("trainer", (0.30, 0.24, 0.18)), pants=dict(style="softshell", main=(0.20, 0.20, 0.21)),
                     jacket=dict(style="fleece", main=(0.22, 0.24, 0.10), trim=(0.04, 0.04, 0.045)), hat=dict(style="beanie", main=(0.30, 0.30, 0.31), pom=True)),
    # Mingma Sherpa, the father: red 8000 m down suit
    "father": dict(boots=("mountain", (0.85, 0.55, 0.04)), pants=dict(style="down", main=(0.62, 0.04, 0.03)),
                   jacket=dict(style="down", main=(0.62, 0.04, 0.03), panel=(0.62, 0.04, 0.03), trim=(0.02, 0.02, 0.025)),
                   hat=dict(style="beanie", main=(0.05, 0.05, 0.06), pom=False)),
}
SPEC = OUTFITS[OUTFIT]
SLOTS = {}          # material name -> dict(fabric, tint, ...)


def slot(piece, role, fabric, tint):
    name = f"{OUTFIT}_{piece}_{role}"
    if name not in SLOTS:
        tex, tile, rough = FAB[fabric]
        SLOTS[name] = dict(fabric=tex, tile_m=tile, rough=rough, tint=[round(c, 4) for c in tint])
        mat(name, tint, rough=rough)
    return bpy.data.materials[name]


def smoothstep(a, b, x):
    t = max(0.0, min(1.0, (x - a) / (b - a))) if b != a else 0.0
    return t * t * (3 - 2 * t)


# ---------------------------------------------------------------- load body + face (metres, rest pose)
def load_fbx(path):
    before = set(bpy.data.objects)
    bpy.ops.import_scene.fbx(filepath=path)
    new = [o for o in bpy.data.objects if o not in before]
    arm = next(o for o in new if o.type == 'ARMATURE')
    meshes = [o for o in new if o.type == 'MESH']
    for o in [arm] + meshes:
        mw = o.matrix_world.copy(); o.parent = None; o.matrix_world = mw
    for o in new:
        if o.type == 'EMPTY': bpy.data.objects.remove(o)
    for o in [arm] + meshes:
        bpy.ops.object.select_all(action='DESELECT'); o.select_set(True); bpy.context.view_layer.objects.active = o
        bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
    for m in meshes:
        for md in list(m.modifiers): m.modifiers.remove(md)
        # UE exports the render mesh: vertices are split along every UV seam -> weld, or garments copied from it
        # come apart into islands (front/back torso, arms...) with hidden boundaries everywhere
        bm = bmesh.new(); bm.from_mesh(m.data)
        bmesh.ops.remove_doubles(bm, verts=bm.verts[:], dist=1e-5)
        bm.to_mesh(m.data); bm.free(); m.data.update()
    return arm, meshes


reset()
farm, fmeshes = load_fbx(FACE_FBX)
face = max(fmeshes, key=lambda m: len(m.data.vertices)); face.name = "MH_Face"
for m in fmeshes:
    if m is not face: bpy.data.objects.remove(m)
face.vertex_groups.clear(); bpy.data.objects.remove(farm)
arm, bmeshes = load_fbx(BODY_FBX)
body = max(bmeshes, key=lambda m: len(m.data.vertices)); body.name = "MH_Body"
arm.name = "root"
assert 1.0 < body.dimensions.z < 2.2, body.dimensions
B = {b.name: (arm.matrix_world @ b.head_local) for b in arm.data.bones}
FRONT = -1.0 if (B["ball_l"].y < B["foot_l"].y) else 1.0        # sign of y towards the face (UE export: -y)
HEAD_TOP = max(v.co.z for v in face.data.vertices)
print("BODY", tuple(round(x, 3) for x in body.dimensions), "head top", round(HEAD_TOP, 3), "front", FRONT)

GROUPS = [g.name for g in body.vertex_groups]


def part_of(b):
    b = b.lower()
    if b.startswith(("pinky", "ring", "middle", "index", "thumb", "hand")): return "hand"
    if b.startswith("upperarm"): return "uarm"
    if b.startswith("lowerarm"): return "larm"
    if b.startswith(("spine", "clavicle")): return "chest"
    if b.startswith(("neck", "head")): return "neck"
    if b.startswith("pelvis"): return "pelvis"
    if b.startswith("thigh"): return "thigh"
    if b.startswith("calf"): return "calf"
    if b.startswith(("foot", "ball")) or "toe" in b: return "foot"
    return "other"


BPART = []
for v in body.data.vertices:
    g = max(v.groups, key=lambda g: g.weight, default=None)
    BPART.append(part_of(GROUPS[g.group]) if g else "other")
print("PARTS", {p: BPART.count(p) for p in set(BPART)})
for _p in sorted(set(BPART)):
    _z = [body.data.vertices[i].co.z for i in range(len(BPART)) if BPART[i] == _p]
    _x = [body.data.vertices[i].co.x for i in range(len(BPART)) if BPART[i] == _p]
    print("PART", _p, "z", round(min(_z), 3), round(max(_z), 3), "x", round(min(_x), 3), round(max(_x), 3))
_other = {}
for v in body.data.vertices:
    g = max(v.groups, key=lambda g: g.weight, default=None)
    if g and part_of(GROUPS[g.group]) == "other": _other[GROUPS[g.group]] = _other.get(GROUPS[g.group], 0) + 1
print("OTHER BONES", sorted(_other.items(), key=lambda t: -t[1])[:15])
KD = KDTree(len(body.data.vertices))
for i, v in enumerate(body.data.vertices): KD.insert(v.co, i)
KD.balance()


def bvh_of(objs):
    bm = bmesh.new()
    for o in objs:
        t = bmesh.new(); t.from_mesh(o.data); t.transform(o.matrix_world)
        me = bpy.data.meshes.new("_tmp"); t.to_mesh(me); t.free(); bm.from_mesh(me); bpy.data.meshes.remove(me)
    tree = BVHTree.FromBMesh(bm); bm.free(); return tree


LAYERS = [body, face]          # what the next garment must stay outside of


# ---------------------------------------------------------------- mesh helpers
def vg(o, name):
    """Vertex group by bone name, created if the body had no weights on that bone (e.g. head)."""
    return o.vertex_groups.get(name) or o.vertex_groups.new(name=name)


def new_obj(name, bm):
    me = bpy.data.meshes.new(name); bm.to_mesh(me); bm.free()
    o = bpy.data.objects.new(name, me); bpy.context.collection.objects.link(o)
    for g in GROUPS: o.vertex_groups.new(name=g)
    return o


def shell(name, keep, src=None):
    """Copy the faces of src (body) whose vertices all pass keep(index, co, part) -> object with body weights."""
    src = src or body
    bm = bmesh.new(); bm.from_mesh(src.data); bm.verts.ensure_lookup_table()
    if src is not body:
        for dl in list(bm.verts.layers.deform.values()): bm.verts.layers.deform.remove(dl)
    ok = [keep(v.index, v.co, BPART[v.index] if src is body else "") for v in bm.verts]
    bmesh.ops.delete(bm, geom=[f for f in bm.faces if not all(ok[v.index] for v in f.verts)], context='FACES_ONLY')
    bmesh.ops.delete(bm, geom=[v for v in bm.verts if not v.link_faces], context='VERTS')
    o = new_obj(name, bm)
    if src is not body: o.vertex_groups.clear()
    return o


def cut(o, co, no, only=None):
    """Delete everything on the +no side of the plane through co; only(co) limits it to faces touching such verts."""
    bm = bmesh.new(); bm.from_mesh(o.data)
    if only:
        fs = [f for f in bm.faces if any(only(v.co) for v in f.verts)]
        geom = list({v for f in fs for v in f.verts}) + list({e for f in fs for e in f.edges}) + fs
    else:
        geom = bm.verts[:] + bm.edges[:] + bm.faces[:]
    bmesh.ops.bisect_plane(bm, geom=geom, plane_co=Vector(co), plane_no=Vector(no).normalized(), clear_outer=True, dist=1e-5)
    bmesh.ops.delete(bm, geom=[v for v in bm.verts if not v.link_faces], context='VERTS')
    bm.to_mesh(o.data); bm.free(); o.data.update()


def subdivide(o, levels=1):
    md = o.modifiers.new("ss", 'SUBSURF'); md.levels = levels; md.render_levels = levels
    md.boundary_smooth = 'PRESERVE_CORNERS'; md.use_limit_surface = True
    activate(o); bpy.ops.object.modifier_apply(modifier="ss")


def activate(o):
    bpy.ops.object.select_all(action='DESELECT'); o.select_set(True); bpy.context.view_layer.objects.active = o


def neighbours(me):
    nb = [[] for _ in me.vertices]
    for e in me.edges:
        a, b = e.vertices; nb[a].append(b); nb[b].append(a)
    bnd = [False] * len(me.vertices)
    cnt = {}
    for p in me.polygons:
        for k in p.edge_keys: cnt[k] = cnt.get(k, 0) + 1
    for k, c in cnt.items():
        if c == 1: bnd[k[0]] = bnd[k[1]] = True
    return nb, bnd


def smooth_field(vals, nb, it):
    for _ in range(it):
        vals = [(vals[i] + sum(vals[j] for j in nb[i])) / (1 + len(nb[i])) if nb[i] else vals[i] for i in range(len(vals))]
    return vals


def laplace(o, it, fac=0.5, pin_boundary=True):
    me = o.data; nb, bnd = neighbours(me)
    P = [v.co.copy() for v in me.vertices]
    for _ in range(it):
        Q = []
        for i, p in enumerate(P):
            if not nb[i] or (pin_boundary and bnd[i]): Q.append(p); continue
            avg = sum((P[j] for j in nb[i]), Vector()) / len(nb[i]); Q.append(p + (avg - p) * fac)
        P = Q
    for v, p in zip(me.vertices, P): v.co = p
    me.update()


def inflate(o, fn):
    """Move every vertex along its normal by fn(i, co, normal)."""
    me = o.data; me.update()
    moves = [(v.co.copy(), v.normal.copy()) for v in me.vertices]
    for i, (c, n) in enumerate(moves): me.vertices[i].co = c + n * fn(i, c, n)
    me.update()


def push_out(o, tree, gap, only=None):
    """Keep every vertex at least `gap` outside the surfaces in tree (no clipping in the rest pose)."""
    me = o.data; n = 0
    for v in me.vertices:
        if only and not only(v.index, v.co): continue
        hit = tree.find_nearest(v.co)
        if hit[0] is None: continue
        loc, nrm = hit[0], hit[1]
        d = (v.co - loc).dot(nrm)
        if d < gap: v.co = loc + nrm * gap + (v.co - loc - nrm * (v.co - loc).dot(nrm)) * 0.5; n += 1
    me.update(); return n


def boundary_loops(o):
    bm = bmesh.new(); bm.from_mesh(o.data)
    edges = [e for e in bm.edges if e.is_boundary]
    seen, loops = set(), []
    for e in edges:
        if e.index in seen: continue
        loop, stack = set(), [e]
        while stack:
            x = stack.pop()
            if x.index in seen: continue
            seen.add(x.index); loop.add(x.index)
            for v in x.verts:
                for y in v.link_edges:
                    if y.is_boundary and y.index not in seen: stack.append(y)
        vs = {v.index for x in loop for v in bm.edges[x].verts} if False else None
        loops.append(loop)
    bm.edges.ensure_lookup_table()
    out = []
    for loop in loops:
        vs = sorted({v.index for x in loop for v in bm.edges[x].verts})
        c = sum((bm.verts[i].co for i in vs), Vector()) / len(vs) if False else None
        out.append((sorted(loop), vs))
    bm.free()
    return out


def loop_centre(o, vs):
    return sum((o.data.vertices[i].co for i in vs), Vector()) / len(vs)


def extrude_loop(o, edge_ids, steps, axis_co, axis_dir, mat_index=None):
    """Extrude the boundary loop in `steps` [(along, radial)] relative to an axis (radial measured from it)."""
    bm = bmesh.new(); bm.from_mesh(o.data); bm.edges.ensure_lookup_table()
    edges = [bm.edges[i] for i in edge_ids]
    d = Vector(axis_dir).normalized(); c0 = Vector(axis_co)
    for along, radial in steps:
        r = bmesh.ops.extrude_edge_only(bm, edges=edges)
        nv = [g for g in r["geom"] if isinstance(g, bmesh.types.BMVert)]
        edges = [g for g in r["geom"] if isinstance(g, bmesh.types.BMEdge) and all(v in nv for v in g.verts)]
        for v in nv:
            rel = v.co - c0; rad = rel - d * rel.dot(d)
            rn = rad.normalized() if rad.length > 1e-6 else Vector((0, 0, 1))
            v.co = v.co + d * along + rn * radial
        if mat_index is not None:
            for f in {f for v in nv for f in v.link_faces}: f.material_index = mat_index
    bm.to_mesh(o.data); bm.free(); o.data.update()


def set_vcol(o, ao):
    me = o.data
    a = me.color_attributes.get("Col") or me.color_attributes.new("Col", 'BYTE_COLOR', 'CORNER')
    for li, l in enumerate(me.loops):
        x = max(0.0, min(1.0, ao[l.vertex_index])); a.data[li].color = (x, x, x, 1.0)


def assign_mats(o, mats, tag_fn):
    """tag_fn(i, co) -> index into mats per vertex; faces take the majority."""
    me = o.data
    for m in mats: me.materials.append(m)
    tags = [tag_fn(v.index, v.co) for v in me.vertices]
    for p in me.polygons:
        ts = [tags[i] for i in p.vertices]; p.material_index = max(set(ts), key=ts.count); p.use_smooth = True


def uv_metres(o, planar=None, period=None):
    """UVs where 1.0 = 1 metre of cloth (materials tile by metres). planar(co)->(u,v) overrides; period = u wrap."""
    me = o.data
    if planar:
        uv = me.uv_layers.new(name="UV0")
        for l in me.loops:
            u, v = planar(me.vertices[l.vertex_index].co); uv.data[l.index].uv = (u, v)
        if period:
            for p in me.polygons:
                us = [uv.data[li].uv.x for li in p.loop_indices]
                if max(us) - min(us) > period * 0.5:
                    for li in p.loop_indices:
                        if uv.data[li].uv.x < 0: uv.data[li].uv.x += period
        return
    activate(o); bpy.ops.object.mode_set(mode='EDIT'); bpy.ops.mesh.select_all(action='SELECT')
    bpy.ops.uv.smart_project(angle_limit=math.radians(60), island_margin=0.002, scale_to_bounds=False)
    bpy.ops.uv.average_islands_scale()
    bpy.ops.object.mode_set(mode='OBJECT')
    uv = me.uv_layers.active
    a3 = sum(p.area for p in me.polygons)
    auv = 0.0
    for p in me.polygons:
        pts = [uv.data[li].uv for li in p.loop_indices]
        for k in range(1, len(pts) - 1):
            auv += abs((pts[k] - pts[0]).cross(pts[k + 1] - pts[0])) * 0.5
    s = math.sqrt(a3 / max(auv, 1e-9))
    for d in uv.data: d.uv = d.uv * s


def finish(o, thickness=0.005):
    """Single-sided cloth (the UE material is two-sided; hems/cuffs get thickness from their inward lips).
    A solidify with even offset blew up into km-long spikes at the folded sole/push-out creases - don't."""
    o.data.update()


def decimate_to(o, edge):
    """Thin the copied body faces to ~edge-sized faces before shaping (hands/feet/face are very dense)."""
    me = o.data; area = sum(p.area for p in me.polygons)
    ratio = min(1.0, (area / (edge * edge)) / max(1, len(me.polygons)))
    if ratio < 0.98:
        md = o.modifiers.new("dec", 'DECIMATE'); md.ratio = ratio; md.use_collapse_triangulate = False
        activate(o); bpy.ops.object.modifier_apply(modifier="dec")


def outward(bm, axis_y):
    """Lofted tubes: make the face normals point away from the vertical axis through (0, axis_y)."""
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces[:])
    score = 0.0
    for f in bm.faces:
        c = f.calc_center_median(); r = Vector((c.x, c.y - axis_y, 0)); score += r.dot(f.normal)
    if score < 0:
        bmesh.ops.reverse_faces(bm, faces=bm.faces[:])


def gaussian(x, mu, s): return math.exp(-((x - mu) / s) ** 2)


# body landmarks
Z_PELVIS = B["pelvis"].z; Z_CHEST = B["spine_05"].z; Z_NECK = B["neck_01"].z
Z_KNEE = B["calf_l"].z; Z_ANKLE = B["foot_l"].z
BODY_TOP = max(v.co.z for v in body.data.vertices)
Z_CROTCH = min((v.co.z for v in body.data.vertices if abs(v.co.x) < 0.012 and BPART[v.index] in ("pelvis", "thigh", "other") and v.co.z > 0.4), default=Z_PELVIS - 0.08)
print("CROTCH", round(Z_CROTCH, 3), "pelvis", round(Z_PELVIS, 3))
SIDE = {"l": 1.0, "r": -1.0}


def arm_chain(s):
    return [B[f"upperarm_{s}"], B[f"lowerarm_{s}"], B[f"hand_{s}"]]


def arm_t(co, s):
    """Arc length (m) of co projected on the shoulder->elbow->wrist chain of side s."""
    ch = arm_chain(s); best, acc, bt = 1e9, 0.0, 0.0
    for a, b in zip(ch, ch[1:]):
        ab = b - a; L = ab.length; t = max(0.0, min(1.0, (co - a).dot(ab) / (L * L)))
        d = (a + ab * t - co).length
        if d < best: best, bt = d, acc + t * L
        acc += L
    return bt


def pillow(t, period):
    f = (t / period) % 1.0
    return math.sin(math.pi * f)


# ---------------------------------------------------------------- pieces
PIECES = []


def score(o, co, no, only=None):
    """Cut a clean edge line into the surface along a plane (no deletion) - for zips, yokes, soles."""
    bm = bmesh.new(); bm.from_mesh(o.data)
    if only:
        fs = [f for f in bm.faces if all(only(v.co) for v in f.verts)]
        geom = list({v for f in fs for v in f.verts}) + list({e for f in fs for e in f.edges}) + fs
    else:
        geom = bm.verts[:] + bm.edges[:] + bm.faces[:]
    bmesh.ops.bisect_plane(bm, geom=geom, plane_co=Vector(co), plane_no=Vector(no).normalized(), dist=1e-5)
    bm.to_mesh(o.data); bm.free(); o.data.update()


def mats_by_face(o, mats, fn):
    """fn(face centre) -> index into mats (clean, follows the scored lines)."""
    me = o.data
    for m in mats: me.materials.append(m)
    for p in me.polygons:
        p.material_index = fn(p.center); p.use_smooth = True


def sleeve_field(o, it=5):
    me = o.data; sl = []
    for v in me.vertices:
        _, bi, _ = KD.find(v.co); sl.append(1.0 if BPART[bi] in ("uarm", "larm", "hand") else 0.0)
    nb, _ = neighbours(me); return smooth_field(sl, nb, it)


def weights_from_body(o):
    """Copy each vertex's skin weights from the nearest body vertex (for lofted pieces that sit close to the body)."""
    bw = [[(GROUPS[g.group], g.weight) for g in v.groups] for v in body.data.vertices]
    cache = {}
    for v in o.data.vertices:
        _, bi, _ = KD.find(v.co)
        for name, w in bw[bi]:
            if w > 0.001: vg(o, name).add([v.index], w, 'REPLACE')


def make_boots(kind, tint):
    """Boots as a stack of convex cross-sections of the foot/leg (like a boot last): one rounded toe box, flat sole,
    rubber rand, lace strip on the instep, padded collar."""
    top = Z_ANKLE + (0.13 if kind != "trainer" else 0.045)
    NT = 40
    upper = slot("boots", "upper", "leather" if kind == "leather" else ("nylon" if kind == "mountain" else "softshell"), tint)
    sole = slot("boots", "sole", "rubber", (0.025, 0.022, 0.02))
    rand = slot("boots", "rand", "rubber", (0.05, 0.045, 0.04) if kind == "leather" else (0.03, 0.03, 0.03))
    lace = slot("boots", "lace", "webbing", (0.06, 0.04, 0.03) if kind == "leather" else (0.04, 0.04, 0.04))
    levels = [-0.016, -0.006, 0.004, 0.012, 0.020, 0.030, 0.042, 0.055, 0.068]
    z = 0.068
    while z < top - 0.001:
        z = min(top, z + 0.018); levels.append(z)
    for sd in ("l", "r"):
        pts = [v.co for v in body.data.vertices if BPART[v.index] in ("foot", "calf") and v.co.x * SIDE[sd] > 0 and v.co.z < top + 0.03]
        fwd = Vector((B[f"ball_{sd}"].x - B[f"foot_{sd}"].x, B[f"ball_{sd}"].y - B[f"foot_{sd}"].y, 0)).normalized()
        rings = []
        for zl in levels:
            zq = max(zl, 0.010)
            band = [c for c in pts if abs(c.z - zq) < 0.012]
            if len(band) < 8: band = [c for c in pts if abs(c.z - zq) < 0.03]
            cx = sum(c.x for c in band) / len(band); cy = sum(c.y for c in band) / len(band)
            rad = []
            for j in range(NT):
                th = 2 * math.pi * j / NT; d = Vector((math.cos(th), math.sin(th), 0))
                rad.append(max((Vector((c.x - cx, c.y - cy, 0))).dot(d) for c in band))
            rings.append([zl, cx, cy, rad])
        for _ in range(3):
            for r in rings:
                q = r[3]; r[3] = [(q[(j - 1) % NT] + 2 * q[j] + q[(j + 1) % NT]) / 4 for j in range(NT)]
        bm = bmesh.new(); grid = []
        for k, (zl, cx, cy, rad) in enumerate(rings):
            row = []
            for j in range(NT):
                th = 2 * math.pi * j / NT; d = Vector((math.cos(th), math.sin(th), 0))
                ahead = max(0.0, d.dot(fwd))
                off = (0.013 if zl < 0.012 else 0.010) + 0.005 * ahead * (zl < 0.05)
                if zl < -0.01: off -= 0.002                 # slight bevel under the sole
                row.append(bm.verts.new((cx + d.x * (rad[j] + off), cy + d.y * (rad[j] + off), zl)))
            grid.append(row)
        for k in range(len(grid) - 1):
            for j in range(NT):
                f = bm.faces.new((grid[k][j], grid[k][(j + 1) % NT], grid[k + 1][(j + 1) % NT], grid[k + 1][j]))
                zc = (rings[k][0] + rings[k + 1][0]) / 2
                th = 2 * math.pi * (j + 0.5) / NT; d = Vector((math.cos(th), math.sin(th), 0))
                if zc < -0.005: f.material_index = 1
                elif zc < 0.016 and kind != "trainer": f.material_index = 2
                elif zc < 0.008: f.material_index = 1
                elif d.dot(fwd) > 0.93 and 0.045 < zc < top - 0.015: f.material_index = 3
                else: f.material_index = 0
        bot = bm.faces.new(list(reversed(grid[0]))); bot.material_index = 1      # flat sole
        bmesh.ops.recalc_face_normals(bm, faces=bm.faces[:])
        o = new_obj(f"boot_{sd}", bm)
        for m in (upper, sole, rand, lace): o.data.materials.append(m)
        for p in o.data.polygons: p.use_smooth = p.material_index != 1
        top_edges = [e.index for e in o.data.edges if all(abs(o.data.vertices[i].co.z - levels[-1]) < 1e-5 for i in e.vertices)]
        c = loop_centre(o, [v.index for v in o.data.vertices if abs(v.co.z - levels[-1]) < 1e-5])
        extrude_loop(o, top_edges, [(0.006, 0.004), (0.004, -0.012)], c, (0, 0, 1), mat_index=0)   # padded collar
        push_out(o, bvh_of([body]), 0.006, only=lambda i, c: c.z > 0.03)
        weights_from_body(o)
        ao = [0.55 + 0.45 * smoothstep(-0.012, 0.03, v.co.z) for v in o.data.vertices]
        set_vcol(o, ao)
        uv_metres(o, lambda c, cx=rings[-1][1], cy=rings[-1][2]: (math.atan2(c.y - cy, c.x - cx) * 0.06, c.z), period=2 * math.pi * 0.06)
        PIECES.append(o); LAYERS.append(o)
    return top


def make_pants(spec, boot_top):
    style = spec["style"]
    waist = Z_PELVIS + 0.115
    hem = boot_top - 0.03
    o = shell("pants", lambda i, c, p: (p in ("pelvis", "thigh", "calf") or (p == "other" and abs(c.x) < 0.22)) and hem - 0.04 < c.z < waist + 0.04)
    cut(o, (0, 0, waist), (0, 0, 1)); cut(o, (0, 0, hem), (0, 0, -1))
    decimate_to(o, 0.03); subdivide(o)
    def off(i, c, n):
        base = 0.026 if style == "down" else 0.010
        flare = 0.022 * smoothstep(Z_KNEE - 0.1, hem, c.z)
        return base + flare + 0.012 * smoothstep(Z_PELVIS - 0.08, Z_PELVIS - 0.25, c.z) * (style != "down")
    inflate(o, off); laplace(o, 10, 0.5)
    ao = [1.0] * len(o.data.vertices)
    if style == "down":
        inflate(o, lambda i, c, n: 0.013 * pillow(c.z, 0.10) ** 0.6)
        ao = [0.6 + 0.4 * pillow(v.co.z, 0.10) ** 0.5 for v in o.data.vertices]
    else:   # soft folds where the fabric bunches: behind the knee, over the boots
        inflate(o, lambda i, c, n: 0.004 * math.sin(c.z * 2 * math.pi / 0.032) * (gaussian(c.z, Z_KNEE, 0.05) + gaussian(c.z, hem + 0.04, 0.04)))
        ao = [1.0 - 0.18 * (gaussian(v.co.z, Z_KNEE, 0.05) + gaussian(v.co.z, hem + 0.04, 0.04)) * (0.5 + 0.5 * math.sin(v.co.z * 2 * math.pi / 0.032)) for v in o.data.vertices]
    push_out(o, bvh_of(LAYERS), 0.006)
    main = slot("pants", "main", "nylon" if style == "down" else "softshell", spec["main"])
    belt = slot("pants", "belt", "webbing", (0.03, 0.03, 0.03))
    assign_mats(o, [main, belt], lambda i, c: 1 if c.z > waist - 0.035 else 0)
    # hem: turn the cuff inward so the opening has a thickness; waist band tucks in
    for edges, vs in boundary_loops(o):
        cz = loop_centre(o, vs).z
        if cz < Z_KNEE:
            c = loop_centre(o, vs); extrude_loop(o, edges, [(-0.006, -0.006)], c, (0, 0, 1))
    ao += [0.7] * (len(o.data.vertices) - len(ao))
    set_vcol(o, ao); uv_metres(o); finish(o, 0.004)
    PIECES.append(o); LAYERS.append(o)
    return waist


def dbg_loops(o, tag):
    for edges, vs in boundary_loops(o):
        X = [o.data.vertices[i].co.x for i in vs]; Z = [o.data.vertices[i].co.z for i in vs]
        print("DBG", tag, len(vs), "x", round(min(X), 3), round(max(X), 3), "z", round(min(Z), 3), round(max(Z), 3))


def torso_hull(z0, z1, step=0.02, NT=64):
    """Convex outline of the torso (no arms) in horizontal slices: cloth hangs straight over hollows (under the chest,
    the spine groove, the waist) the way a real jacket does. Returns f(co) -> hull radius at co's height/angle, centre y."""
    pts = [v.co for v in body.data.vertices if BPART[v.index] in ("chest", "pelvis", "thigh", "other", "neck") and abs(v.co.x) < 0.24]
    cy = B["spine_03"].y
    zs, rows = [], []
    z = z0
    while z <= z1 + 1e-6:
        band = [c for c in pts if abs(c.z - z) < step * 0.75]
        if len(band) > 12:
            rad = []
            for j in range(NT):
                th = 2 * math.pi * j / NT; dx, dy = math.cos(th), math.sin(th)
                rad.append(max((c.x * dx + (c.y - cy) * dy) for c in band))
            for _ in range(2):
                rad = [(rad[(j - 1) % NT] + 2 * rad[j] + rad[(j + 1) % NT]) / 4 for j in range(NT)]
            zs.append(z); rows.append(rad)
        z += step
    for _ in range(2):      # smooth between slices too
        rows = [[(rows[max(k - 1, 0)][j] + 2 * rows[k][j] + rows[min(k + 1, len(rows) - 1)][j]) / 4 for j in range(NT)] for k in range(len(rows))]
    def f(co):
        if not zs: return None
        k = max(0, min(len(zs) - 2, int((co.z - zs[0]) / step)))
        t = max(0.0, min(1.0, (co.z - zs[k]) / step)) if len(zs) > 1 else 0.0
        th = math.atan2(co.y - cy, co.x) % (2 * math.pi); jf = th / (2 * math.pi) * NT; j0 = int(jf) % NT; j1 = (j0 + 1) % NT; u = jf - int(jf)
        r0 = rows[k][j0] * (1 - u) + rows[k][j1] * u
        r1 = rows[min(k + 1, len(rows) - 1)][j0] * (1 - u) + rows[min(k + 1, len(rows) - 1)][j1] * u
        return r0 * (1 - t) + r1 * t
    return f, cy


def neck_bvh(above=0.10):
    """BVH of the face mesh's neck only (below the jaw): collars hug this, never the ears/hair cap/head interior."""
    bm = bmesh.new(); bm.from_mesh(face.data)
    bmesh.ops.delete(bm, geom=[f for f in bm.faces if any(v.co.z > BODY_TOP + above for v in f.verts)], context='FACES')
    t = BVHTree.FromBMesh(bm); bm.free(); return t


def make_jacket(spec, waist):
    style = spec["style"]
    hem = Z_PELVIS - (0.05 if style == "down" else 0.09)
    cut_z = max(hem, Z_CROTCH + 0.03)                    # below the crotch the body splits into two legs
    drop = cut_z - hem
    o = shell("jacket", lambda i, c, p: p in ("chest", "uarm", "larm", "pelvis", "neck", "other", "thigh") and c.z > cut_z - 0.04)
    cut(o, (0, 0, cut_z), (0, 0, -1))                    # body's neck seam stays as the collar line
    for s in ("l", "r"):    # sleeves end 2 cm before the wrist joint
        w = B[f"hand_{s}"]; d = (B[f"hand_{s}"] - B[f"lowerarm_{s}"]).normalized()
        cut(o, w - d * 0.02, d, only=lambda c, s=s: c.x * SIDE[s] > 0.2)
    decimate_to(o, 0.028); subdivide(o)
    me = o.data
    sleeve = sleeve_field(o)
    base_t, base_s, amp_t, amp_s, per_t, per_s = {"down": (0.032, 0.024, 0.016, 0.013, 0.115, 0.085),
                                                  "shell": (0.018, 0.016, 0, 0, 1, 1), "fleece": (0.014, 0.012, 0, 0, 1, 1)}[style]
    def side_of(c): return "l" if c.x > 0 else "r"
    inflate(o, lambda i, c, n: base_t * (1 - sleeve[i]) + base_s * sleeve[i] + 0.010 * smoothstep(Z_PELVIS + 0.05, hem, c.z))
    laplace(o, 28 if style == "down" else 20, 0.6)       # lose the body's muscles/breasts: cloth bridges hollows
    # torso: put every vertex on the convex outline of its slice + the padding, so it hangs straight and clean
    hull, hcy = torso_hull(cut_z - 0.06, BODY_TOP - 0.06)
    pad = {"down": 0.040, "shell": 0.022, "fleece": 0.017}[style]
    for v in me.vertices:
        # only where the cloth faces sideways: on the shoulder tops slices are ill-defined (keep the smoothed shape)
        w = (1.0 - min(1.0, sleeve[v.index] * 1.6)) * smoothstep(BODY_TOP - 0.02, BODY_TOP - 0.10, v.co.z) * smoothstep(0.75, 0.35, abs(v.normal.z))
        if w <= 0.0: continue
        r = hull(v.co)
        if r is None: continue
        d = Vector((v.co.x, v.co.y - hcy, 0.0)); L = d.length
        if L < 1e-4: continue
        target = d * ((r + pad + 0.012 * smoothstep(Z_PELVIS + 0.05, hem, v.co.z)) / L)
        v.co.x = v.co.x + (target.x - d.x) * w; v.co.y = v.co.y + (target.y - d.y) * w
    me.update()
    laplace(o, 6, 0.5)
    push_out(o, bvh_of(LAYERS), base_s * 0.75)
    laplace(o, 3, 0.5)
    sleeve = sleeve_field(o)
    def puff(c, sl):
        tor = pillow(c.z, per_t) ** 0.55; slv = pillow(arm_t(c, side_of(c)), per_s) ** 0.55
        return tor * (1 - sl) + slv * sl
    def is_front(c): return (c.y - B["spine_03"].y) * FRONT > 0
    if style == "down":
        inflate(o, lambda i, c, n: (amp_t * (1 - sleeve[i]) + amp_s * sleeve[i]) * puff(c, sleeve[i]))
    else:
        yoke = Z_CHEST - 0.015
        inflate(o, lambda i, c, n: -0.002 * gaussian(c.z, yoke, 0.006) * (1 - sleeve[i]))
    push_out(o, bvh_of(LAYERS), 0.008 if style == "down" else 0.006)
    # clean lines: centre-front zip, shoulder yoke (down) and its sleeve continuation
    torso = lambda c: abs(c.x) < 0.17
    score(o, (0.009, 0, 0), (1, 0, 0), only=lambda c: torso(c) and is_front(c))
    score(o, (-0.009, 0, 0), (1, 0, 0), only=lambda c: torso(c) and is_front(c))
    yz = Z_CHEST + 0.02
    if style == "down":
        score(o, (0, 0, yz), (0, 0, 1))
    main = slot("jacket", "main", "nylon" if style != "fleece" else "fleece", spec["main"])
    panel = slot("jacket", "panel", "nylon" if style != "fleece" else "fleece", spec.get("panel", spec["main"]))
    trim = slot("jacket", "trim", "webbing" if style != "fleece" else "softshell", spec["trim"])
    def tag(c):
        if torso(c) and is_front(c) and abs(c.x) < 0.009 and c.z > hem: return 2
        if style == "down" and c.z > yz: return 1
        return 0
    mats_by_face(o, [main, panel, trim], tag)
    n0 = len(me.vertices)
    for edges, vs in boundary_loops(o):
        c = loop_centre(o, vs)
        print("LOOP", round(c.x, 3), round(c.z, 3), len(vs), "x", round(min(o.data.vertices[i].co.x for i in vs), 3), round(max(o.data.vertices[i].co.x for i in vs), 3), "z", round(min(o.data.vertices[i].co.z for i in vs), 3), round(max(o.data.vertices[i].co.z for i in vs), 3))
        if c.z > Z_CHEST:                                                 # collar up the neck
            h = {"down": 0.065, "shell": 0.075, "fleece": 0.05}[style]
            nc = len(me.vertices)
            extrude_loop(o, edges, [(0.012, 0.010), (h * 0.5, -0.02), (h * 0.5 - 0.012, -0.01)],
                         Vector((B["neck_01"].x, B["neck_01"].y, 0)), (0, 0, 1), mat_index=1 if style == "down" else 0)
            # the upper collar rings hug the neck (face mesh) at a constant gap: a stand collar, not a funnel
            ft = neck_bvh(); gapc = {"down": 0.024, "shell": 0.018, "fleece": 0.014}[style]
            for v in me.vertices[nc:]:
                if v.co.z > BODY_TOP + 0.02:
                    hit = ft.find_nearest(v.co)
                    if hit[0] is not None and (hit[0] - v.co).length < 0.08:
                        z = v.co.z; v.co = hit[0] + hit[1] * gapc; v.co.z = min(z, BODY_TOP + h)   # keep the ring height
            me.update()
        elif abs(c.x) > 0.2:                                              # cuffs (elastic, trim colour)
            sd = side_of(c); d = (B[f"hand_{sd}"] - B[f"lowerarm_{sd}"]).normalized()
            extrude_loop(o, edges, [(0.006, -0.014), (0.022, -0.002)], B[f"lowerarm_{sd}"], d, mat_index=2)
        else:                                                             # hem falls straight, then the cinched lip
            steps = ([(drop * 0.5, 0.006), (drop * 0.5, 0.002)] if drop > 0.01 else []) + [(0.016, -0.012)]
            extrude_loop(o, edges, steps, Vector((0, B["pelvis"].y, 0)), (0, 0, -1), mat_index=2 if style == "fleece" else None)
    push_out(o, bvh_of(LAYERS), 0.006, only=lambda i, c: i >= n0)
    sleeve = sleeve_field(o)
    if style == "down":
        ao = [0.5 + 0.5 * puff(v.co, sleeve[v.index]) ** 0.7 if v.index < n0 else 0.8 for v in me.vertices]
    else:
        ao = [1.0 - 0.35 * gaussian(v.co.z, Z_CHEST - 0.015, 0.006) * (1 - sleeve[v.index]) if v.index < n0 else 0.85 for v in me.vertices]
    set_vcol(o, ao); uv_metres(o); finish(o)
    if style == "shell":    # rolled hood behind the collar
        a0 = B["neck_01"]; pts = []
        for k in range(13):
            th = math.radians(-100 + 200 * k / 12)
            pts.append((a0.x + 0.085 * math.sin(th), a0.y - FRONT * 0.075 * math.cos(th), BODY_TOP + 0.01 + 0.01 * math.cos(th)))
        hood = tube_along("hood", pts, 0.028, seg=10)
        hood.data.materials.append(main)
        for g in GROUPS: hood.vertex_groups.new(name=g)
        vg(hood, "neck_01").add(range(len(hood.data.vertices)), 1.0, 'REPLACE')
        set_vcol(hood, [0.85] * len(hood.data.vertices)); uv_metres(hood); PIECES.append(hood)
    PIECES.append(o); LAYERS.append(o)


def head_cap(name, front_drop, back_drop, gap):
    """Cap of the face mesh above a plane that drops front_drop below the crown at the brow and back_drop at the nape."""
    ys = [v.co.y for v in face.data.vertices if v.co.z > HEAD_TOP - 0.06]
    y_front = min(ys) if FRONT < 0 else max(ys); y_back = max(ys) if FRONT < 0 else min(ys)
    p1 = Vector((0, y_front, HEAD_TOP - front_drop)); p2 = Vector((0, y_back, HEAD_TOP - back_drop))
    nrm = (p2 - p1).cross(Vector((1, 0, 0))).normalized()
    if nrm.z < 0: nrm = -nrm
    o = shell(name, lambda i, c, p: (c - p1).dot(nrm) > -0.02, src=face)
    cut(o, p1, -nrm)
    decimate_to(o, 0.016); subdivide(o)
    for g in GROUPS: o.vertex_groups.new(name=g)
    vg(o, "head").add(range(len(o.data.vertices)), 1.0, 'REPLACE')
    inflate(o, lambda i, c, n: gap); laplace(o, 6, 0.5)
    return o, p1, nrm


def make_hat(spec):
    hc = Vector((B["head"].x, B["head"].y, 0))
    if spec["style"] == "beanie":
        o, p1, nrm = head_cap("hat", 0.088, 0.14, 0.014)
        def rib(i, c, n):
            th = math.atan2(c.y - hc.y, c.x - hc.x)
            band = smoothstep(0.05, 0.035, (c - p1).dot(nrm))        # folded brim near the edge
            return 0.0022 * math.sin(th * 56) ** 2 + 0.007 * band
        inflate(o, rib)
        push_out(o, bvh_of([face]), 0.010)
        main = slot("hat", "main", "knit", spec["main"])
        assign_mats(o, [main], lambda i, c: 0)
        ao = [0.85 + 0.15 * math.sin(math.atan2(v.co.y - hc.y, v.co.x - hc.x) * 56) ** 2 for v in o.data.vertices]
        for edges, vs in boundary_loops(o):
            extrude_loop(o, edges, [(0.0, -0.008)], hc, (0, 0, 1))
        ao += [0.7] * (len(o.data.vertices) - len(ao))
        set_vcol(o, ao); uv_metres(o); finish(o, 0.004); PIECES.append(o)
        if spec.get("pom"):
            top = max(o.data.vertices, key=lambda v: v.co.z).co
            pom = sphere("pom", 0.04, loc=(top.x, top.y, top.z + 0.025), seg=18, rings=10)
            activate(pom); bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
            for v in pom.data.vertices:   # yarn fluff
                v.co += (v.co - Vector((top.x, top.y, top.z + 0.025))).normalized() * 0.006 * math.sin(v.index * 12.9898) ** 2
            pom.data.materials.append(main)
            for g in GROUPS: pom.vertex_groups.new(name=g)
            vg(pom, "head").add(range(len(pom.data.vertices)), 1.0, 'REPLACE')
            set_vcol(pom, [0.8] * len(pom.data.vertices)); uv_metres(pom); PIECES.append(pom)
    else:   # head scarf tied at the nape, tails down the back
        o, p1, nrm = head_cap("scarf", 0.072, 0.215, 0.018)
        back = Vector((hc.x, B["head"].y - FRONT * 0.08, HEAD_TOP - 0.17))
        inflate(o, lambda i, c, n: 0.028 * gaussian((c - back).length, 0, 0.06) + 0.002 * math.sin(c.z * 90 + c.x * 40))
        push_out(o, bvh_of([face]), 0.012)
        main = slot("scarf", "main", "floral", spec["main"])
        assign_mats(o, [main], lambda i, c: 0)
        set_vcol(o, [1.0 - 0.25 * gaussian((v.co - back).length, 0, 0.05) for v in o.data.vertices]); uv_metres(o); finish(o, 0.003)
        PIECES.append(o)
        knot = sphere("knot", 0.03, loc=back - Vector((0, FRONT * 0.035, 0.02)), seg=12, rings=8, scale=(1.3, 0.8, 1))
        activate(knot); bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
        tails = []
        for k, dx in enumerate((-0.025, 0.03)):
            k0 = back - Vector((0, FRONT * 0.04, 0.03))
            pts = [(k0.x + dx, k0.y - FRONT * 0.004 * j, k0.z - 0.045 * j) for j in range(6)]
            t = tube_along(f"tail{k}", pts, 0.02, seg=6); t.scale = (1, 0.35, 1)
            activate(t); bpy.ops.object.transform_apply(location=False, rotation=True, scale=True); tails.append(t)
        for t in [knot] + tails:
            t.data.materials.clear(); t.data.materials.append(main)
            for g in GROUPS: t.vertex_groups.new(name=g)
            for v in t.data.vertices:
                w = smoothstep(HEAD_TOP - 0.2, HEAD_TOP - 0.3, v.co.z)
                vg(t, "head").add([v.index], 1 - w, 'REPLACE')
                if w > 0: vg(t, "neck_01").add([v.index], w, 'REPLACE')
            set_vcol(t, [0.8] * len(t.data.vertices)); uv_metres(t); PIECES.append(t)


def make_chuba(spec):
    """Tibetan chuba: wrap robe, long sleeves, skirt lofted around both legs, sash, brocade edge; pangden apron."""
    hem_z = Z_ANKLE + 0.02; top_skirt = Z_PELVIS + 0.10
    neck_cut = BODY_TOP - 0.03
    o = shell("chuba", lambda i, c, p: p in ("chest", "uarm", "larm", "pelvis", "neck", "other", "thigh") and c.z > Z_PELVIS - 0.05)
    cut(o, (0, 0, Z_PELVIS - 0.02), (0, 0, -1))
    for s in ("l", "r"):
        w = B[f"hand_{s}"]; d = (B[f"hand_{s}"] - B[f"lowerarm_{s}"]).normalized()
        cut(o, w + d * 0.01, d, only=lambda c, s=s: c.x * SIDE[s] > 0.2)
    decimate_to(o, 0.03); subdivide(o)
    me = o.data
    sleeve = sleeve_field(o)
    def side_of(c): return "l" if c.x > 0 else "r"
    # a loose robe: thick wool that bridges the body's hollows, sleeves widening to the cuff
    inflate(o, lambda i, c, n: 0.022 + sleeve[i] * 0.024 * smoothstep(0.1, 0.5, arm_t(c, side_of(c))))
    laplace(o, 26, 0.6)
    push_out(o, bvh_of(LAYERS), 0.014)
    laplace(o, 3, 0.5)
    sleeve = sleeve_field(o)
    inflate(o, lambda i, c, n: 0.0035 * math.sin(arm_t(c, side_of(c)) * 2 * math.pi / 0.05) * sleeve[i] * smoothstep(0.25, 0.45, arm_t(c, side_of(c))))
    push_out(o, bvh_of(LAYERS), 0.010)
    main = slot("chuba", "main", "wool", spec["main"]); trim = slot("chuba", "trim", "brocade", spec["trim"])
    # wrap edge: a clean brocade band from the throat down to the wearer's right underarm (wearer's right = -x)
    a = Vector((0.0, 0, BODY_TOP - 0.01)); b = Vector((-0.16, 0, Z_CHEST - 0.13))
    nE = (b - a).cross(Vector((0, 1, 0))).normalized()
    def front(c): return (c.y - B["spine_03"].y) * FRONT > 0
    for k in (-0.018, 0.018):
        score(o, a + nE * k, nE, only=lambda c: front(c) and c.x < 0.06)
    def on_edge(c):
        if not front(c) or c.x > 0.05: return False
        t = (Vector((c.x, 0, c.z)) - a).dot((b - a).normalized())
        return abs((Vector((c.x, 0, c.z)) - a).dot(nE)) < 0.018 and -0.02 < t < (b - a).length + 0.02
    mats_by_face(o, [main, trim], lambda c: 1 if on_edge(c) else 0)
    n0 = len(me.vertices)
    for edges, vs in boundary_loops(o):
        c = loop_centre(o, vs)
        if c.z > Z_CHEST:                       # standing brocade collar, hugging the neck
            nc = len(me.vertices)
            extrude_loop(o, edges, [(0.010, 0.006), (0.035, -0.02)], Vector((B["neck_01"].x, B["neck_01"].y, 0)), (0, 0, 1), mat_index=1)
            ft = neck_bvh()
            for v in me.vertices[nc:]:
                if v.co.z > BODY_TOP + 0.02:
                    hit = ft.find_nearest(v.co)
                    if hit[0] is not None and (hit[0] - v.co).length < 0.08:
                        z = v.co.z; v.co = hit[0] + hit[1] * 0.014; v.co.z = z
            me.update()
        elif abs(c.x) > 0.2:
            sd = side_of(c); d = (B[f"hand_{sd}"] - B[f"lowerarm_{sd}"]).normalized()
            extrude_loop(o, edges, [(0.0, 0.008), (-0.06, 0.004)], B[f"lowerarm_{sd}"], d, mat_index=1)   # turned-back cuff
    push_out(o, bvh_of(LAYERS), 0.006, only=lambda i, c: i >= n0 and c.z < BODY_TOP)
    ao = [1.0 - 0.2 * gaussian(v.co.z, Z_PELVIS - 0.02, 0.02) if v.index < n0 else 0.9 for v in me.vertices]
    set_vcol(o, ao); uv_metres(o); finish(o, 0.005); PIECES.append(o); LAYERS.append(o)

    # ---- skirt: rings around both legs (support function of the body section), flaring to the ankle, back pleats
    NT, NZ = 72, 34
    bverts = [v.co for v in body.data.vertices if BPART[v.index] in ("pelvis", "thigh", "calf", "foot", "other") and abs(v.co.x) < 0.24]
    cy = B["pelvis"].y
    rings = []
    for k in range(NZ):
        z = top_skirt + (hem_z - top_skirt) * k / (NZ - 1)
        band = [c for c in bverts if abs(c.z - z) < 0.025] or [Vector((0, cy, z))]
        rad = []
        for j in range(NT):
            th = 2 * math.pi * j / NT; d = Vector((math.cos(th), math.sin(th), 0))
            rad.append(max((c - Vector((0, cy, z))).dot(d) for c in band))
        rings.append((z, rad))
    for _ in range(4):      # smooth around + down
        rings = [(z, [(r[(j - 1) % NT] + 2 * r[j] + r[(j + 1) % NT]) / 4 for j in range(NT)]) for z, r in rings]
    rs = [r for _, r in rings]
    for _ in range(3):
        rs = [[(rs[max(k - 1, 0)][j] + 2 * rs[k][j] + rs[min(k + 1, NZ - 1)][j]) / 4 for j in range(NT)] for k in range(NZ)]
    # the hull never shrinks going down (the cloth falls from the widest point) + clearance for walking
    for k in range(1, NZ):
        rs[k] = [max(rs[k][j], rs[k - 1][j] * 0.985) for j in range(NT)]
    bm = bmesh.new(); grid = []
    for k, (z, _) in enumerate(rings):
        f = k / (NZ - 1); row = []
        for j in range(NT):
            th = 2 * math.pi * j / NT
            back = max(0.0, -math.sin(th) * FRONT)          # pleats on the back half
            r = rs[k][j] + 0.022 + 0.05 * f ** 1.3 + 0.009 * back * f * math.sin(th * 14) ** 2
            row.append(bm.verts.new((r * math.cos(th), cy + r * math.sin(th), z)))
        grid.append(row)
    for k in range(NZ - 1):
        for j in range(NT):
            bm.faces.new((grid[k][j], grid[k][(j + 1) % NT], grid[k + 1][(j + 1) % NT], grid[k + 1][j]))
    outward(bm, cy)
    sk = new_obj("skirt", bm)
    sk.data.materials.append(main)
    for p in sk.data.polygons: p.use_smooth = True
    for v in sk.data.vertices:
        s = smoothstep(top_skirt, top_skirt - 0.3, v.co.z); side = max(0.0, min(1.0, 0.5 + v.co.x / 0.16))
        vg(sk, "pelvis").add([v.index], 1 - 0.7 * s, 'REPLACE')
        if s > 0:
            vg(sk, "thigh_l").add([v.index], 0.7 * s * side, 'REPLACE')
            vg(sk, "thigh_r").add([v.index], 0.7 * s * (1 - side), 'REPLACE')
    def cyl_uv(c):
        th = math.atan2(c.y - cy, c.x); return (th * 0.2, c.z)
    set_vcol(sk, [1.0 - 0.25 * max(0.0, -math.sin(math.atan2(v.co.y - cy, v.co.x)) * FRONT) * math.sin(math.atan2(v.co.y - cy, v.co.x) * 14) ** 2 for v in sk.data.vertices])
    uv_metres(sk, cyl_uv, 2 * math.pi * 0.2); finish(sk, 0.006); PIECES.append(sk)

    # ---- sash (kara) around the waist
    bm = bmesh.new(); grid = []
    for k, z in enumerate((top_skirt - 0.01, top_skirt + 0.03, top_skirt + 0.07)):
        k2 = min(range(NZ), key=lambda q: abs(rings[q][0] - top_skirt)); row = []
        for j in range(NT):
            th = 2 * math.pi * j / NT; r = rs[k2][j] + 0.034 + 0.004 * (k == 1)
            row.append(bm.verts.new((r * math.cos(th), cy + r * math.sin(th), z)))
        grid.append(row)
    for k in range(2):
        for j in range(NT):
            bm.faces.new((grid[k][j], grid[k][(j + 1) % NT], grid[k + 1][(j + 1) % NT], grid[k + 1][j]))
    outward(bm, cy)
    sa = new_obj("sash", bm); sa.data.materials.append(slot("chuba", "sash", "linen", spec["sash"]))
    for p in sa.data.polygons: p.use_smooth = True
    vg(sa, "pelvis").add(range(len(sa.data.vertices)), 1.0, 'REPLACE')
    set_vcol(sa, [0.9] * len(sa.data.vertices)); uv_metres(sa, cyl_uv, 2 * math.pi * 0.2); finish(sa, 0.004); PIECES.append(sa)
    return sk, rings, rs, cy, NT


def make_apron(sk_info):
    """Pangden: the striped apron of married women, over the front of the skirt (texture made by make_pangden)."""
    sk, rings, rs, cy, NT = sk_info
    NZ = len(rings); half = math.radians(38)
    front_th = -math.pi / 2 if FRONT < 0 else math.pi / 2
    k0 = 1; k1 = NZ - 3
    bm = bmesh.new(); grid = []
    for k in range(k0, k1 + 1):
        z = rings[k][0]; f = k / (NZ - 1); row = []
        for j in range(15):
            th = front_th - half + 2 * half * j / 14
            jj = int(round((th % (2 * math.pi)) / (2 * math.pi) * NT)) % NT
            r = rs[k][jj] + 0.022 + 0.05 * f ** 1.3 + 0.008
            row.append(bm.verts.new((r * math.cos(th), cy + r * math.sin(th), z)))
        grid.append(row)
    for k in range(len(grid) - 1):
        for j in range(14):
            bm.faces.new((grid[k][j], grid[k][j + 1], grid[k + 1][j + 1], grid[k + 1][j]))
    outward(bm, cy)
    ap = new_obj("apron", bm)
    ap.data.materials.append(slot("apron", "main", "pangden", (1, 1, 1)))
    for p in ap.data.polygons: p.use_smooth = True
    z_top, z_bot = rings[k0][0], rings[k1][0]
    for v in ap.data.vertices:
        s = smoothstep(z_top, z_top - 0.3, v.co.z); side = max(0.0, min(1.0, 0.5 + v.co.x / 0.16))
        vg(ap, "pelvis").add([v.index], 1 - 0.7 * s, 'REPLACE')
        if s > 0:
            vg(ap, "thigh_l").add([v.index], 0.7 * s * side, 'REPLACE')
            vg(ap, "thigh_r").add([v.index], 0.7 * s * (1 - side), 'REPLACE')
    xs = [v.co.x for v in ap.data.vertices]; x0, x1 = min(xs), max(xs)
    uv_metres(ap, lambda c: ((c.x - x0) / (x1 - x0), (c.z - z_bot) / (z_top - z_bot)))
    set_vcol(ap, [1.0] * len(ap.data.vertices)); finish(ap, 0.003); PIECES.append(ap)


def make_pangden(path):
    """Pangden stripe texture: narrow bands of red, green, yellow, blue, white, black, orange, a little woven noise."""
    import random
    random.seed(7)
    cols = [(0.62, 0.05, 0.05), (0.05, 0.32, 0.12), (0.85, 0.62, 0.05), (0.05, 0.12, 0.42), (0.85, 0.82, 0.74),
            (0.62, 0.05, 0.05), (0.85, 0.35, 0.05), (0.03, 0.03, 0.03), (0.08, 0.38, 0.40), (0.55, 0.08, 0.25)]
    W, H = 256, 1024
    img = bpy.data.images.new("T_Pangden", W, H, alpha=False)
    bands, y = [], 0
    while y < H:
        h = random.choice((10, 14, 18, 22, 30)); c = random.choice(cols)
        bands.append((y, y + h, c)); y += h
        if random.random() < 0.35: bands.append((y, y + 3, (0.03, 0.03, 0.03))); y += 3
    px = [0.0] * (W * H * 4)
    for y0, y1, c in bands:
        for yy in range(y0, min(y1, H)):
            for x in range(W):
                n = 0.9 + 0.1 * (((x * 7 + yy * 13) % 5) / 4) * (1 if (x // 2 + yy // 2) % 2 else 0.6)
                i = (yy * W + x) * 4; px[i:i + 4] = [c[0] * n, c[1] * n, c[2] * n, 1.0]
    img.pixels = px
    img.filepath_raw = path; img.file_format = 'PNG'; img.save()


# ---------------------------------------------------------------- build
bt = make_boots(*SPEC["boots"])
waist = make_pants(SPEC["pants"], bt) if "pants" in SPEC else Z_PELVIS + 0.115
if "jacket" in SPEC: make_jacket(SPEC["jacket"], waist)
if "chuba" in SPEC:
    info = make_chuba(SPEC["chuba"])
    if SPEC.get("apron"): make_apron(info)
if "hat" in SPEC: make_hat(SPEC["hat"])

os.makedirs(DEST, exist_ok=True)
if any(s["fabric"] == "@pangden" for s in SLOTS.values()):
    make_pangden(os.path.join(DEST, "T_Pangden.png"))

for o in PIECES:
    zs = [v.co.z for v in o.data.vertices]; xs = [v.co.x for v in o.data.vertices]
    print("PIECE", o.name, "tris", sum(len(p.vertices) - 2 for p in o.data.polygons), "z", round(min(zs), 3), round(max(zs), 3), "x", round(min(xs), 3), round(max(xs), 3))
g = join(f"SKM_G_{OUTFIT}{TAG}", PIECES)
activate(g)
bpy.ops.object.vertex_group_limit_total(group_select_mode='ALL', limit=6)
bpy.ops.object.vertex_group_normalize_all(lock_active=False)
g.parent = arm
am = g.modifiers.new("Armature", 'ARMATURE'); am.object = arm
tris = sum(len(p.vertices) - 2 for p in g.data.polygons)
print(f"GARMENT {OUTFIT} tris={tris} slots={list(SLOTS)}")
export_skeletal(arm, g, os.path.join(DEST, f"SKM_G_{OUTFIT}{TAG}.fbx"))
json.dump(dict(outfit=OUTFIT, tag=TAG, tris=tris, slots=SLOTS), open(os.path.join(DEST, f"SKM_G_{OUTFIT}{TAG}.json"), "w"), indent=1)

# preview: front + three-quarter, garments over the grey body/head
s = bpy.context.scene
s.render.engine = 'BLENDER_WORKBENCH'; s.display.shading.color_type = 'MATERIAL'; s.display.shading.light = 'STUDIO'
s.display.shading.show_cavity = True; s.display.shading.cavity_type = 'BOTH'
s.render.resolution_x = 900; s.render.resolution_y = 1100
grey = mat("skin_preview", (0.42, 0.45, 0.5), rough=0.6)
for o in (body, face):
    o.data.materials.clear(); o.data.materials.append(grey)
cam = bpy.data.objects.new("Cam", bpy.data.cameras.new("Cam")); s.collection.objects.link(cam); s.camera = cam
cam.data.lens = 70
os.makedirs(os.path.join(OUT, "Preview"), exist_ok=True)
for nm, d in (("front", Vector((0, FRONT, 0.05))), ("threeq", Vector((0.75, FRONT, 0.15))), ("back", Vector((-0.4, -FRONT, 0.1)))):
    tgt = Vector((0, 0, HEAD_TOP * 0.52))
    cam.location = tgt + d.normalized() * 4.4
    cam.rotation_euler = (tgt - cam.location).to_track_quat('-Z', 'Y').to_euler()
    s.render.filepath = os.path.join(OUT, "Preview", f"garment_{OUTFIT}{TAG}_{nm}.png")
    bpy.ops.render.render(write_still=True)
save_blend(f"garment_{OUTFIT}{TAG}")
print("DONE", OUTFIT)
