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
FAB = {  # real Poly Haven tile sizes (~0.27 m; they were tiled 2-3x too small and the weave vanished, 2026-11-15)
       "nylon": ("@nylon", 0.265, 0.45), "softshell": ("bi_stretch", 0.265, 0.62), "fleece": ("polar_fleece", 0.273, 0.9),
       "knit": ("knitted_fleece", 0.266, 0.9), "wool": ("caban", 0.274, 0.85), "tweed": ("poly_wool_herringbone", 0.27, 0.85),
       "leather": ("brown_leather", 0.40, 0.6), "rubber": ("", 1.0, 0.85), "brocade": ("quatrefoil_jacquard_fabric", 0.28, 0.45),
       "floral": ("floral_jacquard", 0.25, 0.7), "linen": ("rough_linen", 0.27, 0.85), "pangden": ("@pangden", 1.0, 0.75),
       "scarfprint": ("@scarfprint", 0.12, 0.75), "twill": ("denim_fabric_05", 0.27, 0.75),
       "webbing": ("hessian_230", 0.08, 0.8), "metal": ("", 1.0, 0.3)}

OUTFITS = {
    # Nima, the old sirdar: mustard down jacket with black yoke, maroon knit hat, dark softshell trousers, leather boots
    "nima": dict(boots=("scan", "garmont_hiking_boot"), pants=dict(style="softshell", main=(0.07, 0.07, 0.08)),
                 jacket=dict(style="down", main=(0.80, 0.42, 0.05), panel=(0.025, 0.025, 0.03), trim=(0.02, 0.02, 0.025)),
                 hat=dict(style="beanie", main=(0.36, 0.04, 0.05), pom=True, glasses=True)),
    # Aunt Pema: maroon wool chuba, brocade edge, red sash, striped pangden apron, dark red head scarf
    # Aunt Pema (married, ~50, at home): plain dark sleeveless angi, red silk-brocade blouse (collar shows), dark knit
    # cardigan over it, striped front pangden + back apron (married women), sash + silver buckle, bright head scarf,
    # white sneakers (Pangboche 2009 photo in the notes)
    "pema": dict(boots=("scan", "striper_sneaker_white"), chuba=dict(main=(0.022, 0.02, 0.02), top=(0.05, 0.034, 0.04),
                 blouse=(0.30, 0.03, 0.045), sash=(0.30, 0.05, 0.04)), apron=True, back_apron=True,
                 hat=dict(style="scarf", main=(0.45, 0.06, 0.2), print=True)),
    # clients
    "marek": dict(boots=("scan", "salomon_x_raise"), pants=dict(style="softshell", main=(0.10, 0.10, 0.11)),
                  jacket=dict(style="shell", main=(0.16, 0.18, 0.07), trim=(0.02, 0.02, 0.025)), hat=dict(style="beanie", main=(0.12, 0.12, 0.13), pom=False)),
    "elena": dict(boots=("scan", "salomon_outline_mid"), pants=dict(style="softshell", main=(0.13, 0.12, 0.12)),
                  jacket=dict(style="shell", main=(0.62, 0.16, 0.04), trim=(0.02, 0.02, 0.025))),
    "jiwoo": dict(boots=("scan", "adidas_terrex_fast_r"), pants=dict(style="softshell", main=(0.08, 0.08, 0.09)),
                  jacket=dict(style="down", main=(0.05, 0.11, 0.28), panel=(0.02, 0.02, 0.03), trim=(0.02, 0.02, 0.025)),
                  hat=dict(style="beanie", main=(0.75, 0.74, 0.70), pom=True)),
    # porters
    # trekking porter (Pheriche, Nov 2022 photo): cheap black puffer, olive cargo trousers rolled to mid-calf, dark socks,
    # black low trainers, dark beanie
    "porter_a": dict(boots=("scan", "reebok_dmx_rainwalker"), pants=dict(style="softshell", main=(0.09, 0.085, 0.05), rolled=True, fabric="twill"),
                     jacket=dict(style="down", main=(0.02, 0.02, 0.022), panel=(0.02, 0.02, 0.022), trim=(0.01, 0.01, 0.012)),
                     hat=dict(style="beanie", main=(0.03, 0.03, 0.035), pom=False)),
    "porter_b": dict(boots=("trainer", (0.30, 0.24, 0.18)), pants=dict(style="softshell", main=(0.20, 0.20, 0.21)),
                     jacket=dict(style="fleece", main=(0.22, 0.24, 0.10), trim=(0.04, 0.04, 0.045)), hat=dict(style="beanie", main=(0.30, 0.30, 0.31), pom=True)),
    # Mingma Sherpa, the father: red 8000 m down suit
    "father": dict(boots=("scan", "garmont_hiking_boot"), pants=dict(style="down", main=(0.62, 0.04, 0.03)),
                   jacket=dict(style="down", main=(0.62, 0.04, 0.03), panel=(0.62, 0.04, 0.03), trim=(0.02, 0.02, 0.025)),
                   hat=dict(style="beanie", main=(0.05, 0.05, 0.06), pom=False)),
}
SPEC = OUTFITS[OUTFIT]
SLOTS = {}          # material name -> dict(fabric, tint, ...)


def slot(piece, role, fabric, tint):
    name = f"{OUTFIT}_{piece}_{role}"
    if name not in SLOTS:
        tex, tile, rough = (fabric, 1.0, 0.8) if fabric.startswith("@scan:") else FAB[fabric]
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


def wrap_over(o, objs, gap, cy, only=None):
    """Lay o OVER objs radially (axis = vertical line x=0, y=cy): a ray from outside in finds the outermost surface at
    each vertex's height and angle; vertices inside it + gap move out. Robust for rings (sash, apron) where nearest-
    surface push_out grabs the inner face of a two-sided robe."""
    tree = bvh_of(objs); me = o.data; n = 0
    for v in me.vertices:
        if only and not only(v.index, v.co): continue
        d = Vector((v.co.x, v.co.y - cy, 0.0)); L = d.length
        if L < 1e-4: continue
        d /= L
        R0 = L + 0.08                       # start just outside: from 1 m the A-pose arms (at waist height) were hit
        hit = tree.ray_cast(Vector((d.x * R0, cy + d.y * R0, v.co.z)), -d, R0)
        if hit[0] is None: continue
        R = Vector((hit[0].x, hit[0].y - cy, 0.0)).length
        if L < R + gap: v.co.x = d.x * (R + gap); v.co.y = cy + d.y * (R + gap); n += 1
    me.update(); return n


def neck_profile(z0, z1, NT=48):
    """Neck radius around the neck axis per angle (body + face meshes between z0 and z1): collars are built on it."""
    ax = Vector((B["neck_01"].x, B["neck_01"].y))
    rad = [0.0] * NT
    for src in (body, face):
        for v in src.data.vertices:
            c = v.co
            if z0 <= c.z <= z1:
                d = Vector((c.x - ax.x, c.y - ax.y))
                if d.length < 0.11:
                    j = int((math.atan2(d.y, d.x) % (2 * math.pi)) / (2 * math.pi) * NT) % NT
                    rad[j] = max(rad[j], d.length)
    good = [r for r in rad if r > 0]; fill = sum(good) / len(good) if good else 0.06
    rad = [r if r > 0 else fill for r in rad]
    for _ in range(3):
        rad = [(rad[(j - 1) % NT] + 2 * rad[j] + rad[(j + 1) % NT]) / 4 for j in range(NT)]
    def r_at(x, y):
        th = math.atan2(y - ax.y, x - ax.x) % (2 * math.pi); jf = th / (2 * math.pi) * NT; j0 = int(jf) % NT; u = jf - int(jf)
        return rad[j0] * (1 - u) + rad[(j0 + 1) % NT] * u
    return ax, r_at


def close_neckline(o, loop_vs, gap0, band=0.10, slope=0.55):
    """A body-copy garment opens at the collarbones (39 cm wide): pull the opening and a band below it in towards the
    neck so the yoke slopes up to a collar base gap0 off the neck - the collar then rises from THERE (it rose from the
    wide opening and read as a ragged funnel round a bare neck, user 2026-11-15)."""
    me = o.data
    zs = [me.vertices[i].co.z for i in loop_vs]
    ax, r_at = neck_profile(min(zs) - 0.02, BODY_TOP)
    kd = KDTree(len(loop_vs))
    for k, i in enumerate(loop_vs): kd.insert(me.vertices[i].co, k)
    kd.balance()
    for v in me.vertices:
        d = kd.find(v.co)[2]
        if d > band: continue
        dx, dy = v.co.x - ax.x, v.co.y - ax.y; L = math.hypot(dx, dy)
        if L < 1e-4: continue
        target = r_at(v.co.x, v.co.y) + gap0 + slope * d          # the garment widens going down from the collar
        if L > target:
            w = 1.0 - smoothstep(0.0, band, d) * 0.85 if d > 0 else 1.0
            nl = L + (target - L) * w
            v.co.x = ax.x + dx / L * nl; v.co.y = ax.y + dy / L * nl
    # the opening's height: a smooth tilted ring (low at the throat, higher at the nape), no peaks over the shoulders
    order = loop_order(me, loop_vs)
    for _ in range(12):
        zs = [me.vertices[i].co.z for i in order]; n = len(zs)
        for k, i in enumerate(order):
            me.vertices[i].co.z = 0.25 * zs[k - 1] + 0.5 * zs[k] + 0.25 * zs[(k + 1) % n]
    me.update()
    return ax, r_at


def loop_order(me, loop_vs):
    """Vertices of a closed boundary loop in walking order."""
    vs = set(loop_vs); adj = {i: [] for i in vs}
    for e in me.edges:
        a, b = e.vertices
        if a in vs and b in vs: adj[a].append(b); adj[b].append(a)
    start = next(iter(vs)); order = [start]; prev = None; cur = start
    while True:
        nxt = [j for j in adj[cur] if j != prev and (j not in order[-2:] or len(order) < 2)]
        nxt = [j for j in nxt if j not in order] or []
        if not nxt: break
        prev, cur = cur, nxt[0]; order.append(cur)
    return order if len(order) >= len(vs) * 0.8 else list(loop_vs)


def stand_collar(o, first_new, ax, r_at, gap_base, gap_top, z_base, h):
    """Collar vertices (index >= first_new) at the neck radius + a gap that narrows towards the top: a stand collar."""
    me = o.data
    for v in me.vertices[first_new:]:
        dx, dy = v.co.x - ax.x, v.co.y - ax.y; L = math.hypot(dx, dy)
        if L < 1e-4: continue
        t = max(0.0, min(1.0, (v.co.z - z_base) / max(h, 1e-3)))
        R = r_at(v.co.x, v.co.y) + gap_base + (gap_top - gap_base) * t
        v.co.x = ax.x + dx / L * R; v.co.y = ax.y + dy / L * R
    me.update()


SEAM_FAR = 0.03


def seam_uv(o, seams, edge_inset=None):
    """Second UV channel "UVSeam" = (metres along the nearest seam, signed metres across it) - the garment material
    draws stitch dashes and a seam shadow from it (research: at 1-3 m stitches/seams sell cloth, the weave does not).
    seams: callables co -> (signed distance, along) or None; edge_inset: stitching that far in from every open edge."""
    me = o.data
    best = []
    for v in me.vertices:
        bd, ba = SEAM_FAR, 0.0
        for f in seams:
            r = f(v.co)
            if r is not None and abs(r[0]) < abs(bd): bd, ba = r
        best.append([bd, ba])
    if edge_inset is not None:
        pts = []
        for edges, vs in boundary_loops(o):
            acc, prev = 0.0, None
            for i in loop_order(me, vs):
                if prev is not None: acc += (me.vertices[i].co - me.vertices[prev].co).length
                pts.append((me.vertices[i].co.copy(), acc)); prev = i
        if pts:
            kd = KDTree(len(pts))
            for k, (c, a) in enumerate(pts): kd.insert(c, k)
            kd.balance()
            for i, v in enumerate(me.vertices):
                c, k, dist = kd.find(v.co); d = dist - edge_inset
                if abs(d) < abs(best[i][0]): best[i] = [d, pts[k][1]]
    uv = me.uv_layers.get("UVSeam") or me.uv_layers.new(name="UVSeam")
    for l in me.loops:
        d, a = best[l.vertex_index]
        uv.data[l.index].uv = (a, max(-SEAM_FAR, min(SEAM_FAR, d)))


def around(c, cy, r=0.16):
    """Arc-length-like coordinate round the body's vertical axis (for dashes along horizontal seams)."""
    return math.atan2(c.y - cy, c.x) * r


def push_out(o, tree, gap, only=None, tangent=0.5):
    """Keep every vertex at least `gap` outside the surfaces in tree (no clipping in the rest pose)."""
    me = o.data; n = 0
    for v in me.vertices:
        if only and not only(v.index, v.co): continue
        hit = tree.find_nearest(v.co)
        if hit[0] is None: continue
        loc, nrm = hit[0], hit[1]
        d = (v.co - loc).dot(nrm)
        if d < gap: v.co = loc + nrm * gap + (v.co - loc - nrm * (v.co - loc).dot(nrm)) * tangent; n += 1
    me.update(); return n


def despike(o, lim=0.0025, it=4, only=None):
    """Vertices standing more than lim out of (or into) their 1-ring ALONG THE NORMAL go back towards the ring
    average (score-line slivers are in-plane and stay). Returns how many moved in the first pass."""
    me = o.data; first = None
    for _ in range(it):
        me.update(); nb, bnd = neighbours(me); n = 0
        P = [v.co.copy() for v in me.vertices]
        for v in me.vertices:
            i = v.index
            if bnd[i] or not nb[i] or (only and not only(i, P[i])): continue
            avg = sum((P[j] for j in nb[i]), Vector()) / len(nb[i])
            d = (P[i] - avg).dot(v.normal)
            if abs(d) > lim: v.co = P[i] - v.normal * (d - math.copysign(lim, d) * 0.5); n += 1
        if first is None: first = n
    me.update(); return first


def push_relax(o, tree, gap, it=4, only=None):
    """push_out, then relax the vertices it moved (+ their 1-ring) and push again normal-only: a plain push leaves
    single vertices standing proud where the nearest point jumps (shoulder tops, armpits) - spikes."""
    me = o.data; me.update(); old = [v.co.copy() for v in me.vertices]
    push_out(o, tree, gap, only=only, tangent=1.0)
    moved = {v.index for v in me.vertices if (v.co - old[v.index]).length > 1e-5}
    nb, bnd = neighbours(me)
    zone = set(moved)
    for i in moved: zone.update(nb[i])
    for _ in range(it):
        P = {i: sum((me.vertices[j].co for j in nb[i]), Vector()) / len(nb[i]) for i in zone if nb[i] and not bnd[i]}
        for i, a in P.items(): me.vertices[i].co = me.vertices[i].co.lerp(a, 0.5)
    me.update()
    push_out(o, tree, gap * 0.8, only=only, tangent=1.0)
    return len(moved)


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


def smooth_normals(me, it=8):
    """Vertex normals averaged over the 1-ring `it` times: an offset along these never folds over at creases."""
    nb, _ = neighbours(me)
    N = [v.normal.copy() for v in me.vertices]
    for _ in range(it):
        M = []
        for i, n in enumerate(N):
            s = n + sum((N[j] for j in nb[i]), Vector())
            M.append(s.normalized() if s.length > 1e-8 else n)
        N = M
    return N


def thicken(o, t, hem_w=0.011, roll=4, hem_bulge=0.0015, t_fn=None):
    """Real cloth thickness (director 2026-11-20: paper-thin single sheets, no hem, no lining):
    - an inner shell (the lining) offset t inward along SMOOTHED normals, same UVs / weights / colours, faces reversed;
    - every open edge closed by a rolled hem: a half-round of `roll` segments from the outer edge to the inner one,
      bulging out of the cloth, so a cuff/hem/collar edge reads as a padded, turned edge;
    - the outer band within hem_w of an edge sits hem_bulge proud (the turned-up hem allowance, stitched at edge_inset).
    No Solidify: with even thickness it blew the garments into km-long spikes at creases. t_fn(i, co) overrides t."""
    me = o.data; me.update()
    N = smooth_normals(me)
    nb, bnd = neighbours(me)
    bv = [i for i, b in enumerate(bnd) if b]
    if not bv: return
    kd = KDTree(len(bv))
    for k, i in enumerate(bv): kd.insert(me.vertices[i].co, k)
    kd.balance()
    for v in me.vertices:
        d = kd.find(v.co)[2]
        if d < hem_w: v.co = v.co + N[v.index] * hem_bulge * (1.0 - smoothstep(hem_w * 0.65, hem_w, d))
    me.update()
    T = [t_fn(v.index, v.co) if t_fn else t for v in me.vertices]
    bm = bmesh.new(); bm.from_mesh(me); bm.verts.ensure_lookup_table(); bm.faces.ensure_lookup_table()
    orig_v = bm.verts[:]; orig_f = bm.faces[:]
    bedges = [e for e in bm.edges if e.is_boundary and e.link_faces]
    uvls = list(bm.loops.layers.uv.values()); cols = list(bm.loops.layers.color.values())
    dup = bmesh.ops.duplicate(bm, geom=bm.verts[:] + bm.edges[:] + bm.faces[:])
    vmap = dup["vert_map"]
    new_f = [g for g in dup["geom"] if isinstance(g, bmesh.types.BMFace)]
    for v in orig_v:
        vmap[v].co = v.co - N[v.index] * T[v.index]
    bmesh.ops.reverse_faces(bm, faces=new_f)
    # out-of-cloth direction at each boundary vertex (in the surface plane, away from its face)
    E = {}
    for e in bedges:
        a, b = e.verts; f = e.link_faces[0]
        p = (b.co - a.co).cross(f.normal)
        if p.dot(f.calc_center_median() - (a.co + b.co) * 0.5) > 0: p = -p
        if p.length > 1e-9:
            p.normalize(); E[a] = E.get(a, Vector()) + p; E[b] = E.get(b, Vector()) + p
    ring = {}
    for v, e_dir in E.items():
        inner = vmap[v]; ax = v.co - inner.co; h = ax.length
        if h < 1e-6: ring[v] = [v] * roll + [inner]; continue
        ax /= h; e_dir = e_dir - ax * e_dir.dot(ax)
        e_dir = e_dir.normalized() if e_dir.length > 1e-6 else Vector((0, 0, 0))
        mid = (v.co + inner.co) * 0.5; pts = [v]
        for k in range(1, roll):
            ang = math.pi * k / roll
            pts.append(bm.verts.new(mid + ax * (h * 0.5) * math.cos(ang) + e_dir * (h * 0.55) * math.sin(ang), v))
        ring[v] = pts + [inner]
    for e in bedges:
        a, b = e.verts; f = e.link_faces[0]
        if a not in ring or b not in ring: continue
        la = next(l for l in f.loops if l.vert is a); lb = next(l for l in f.loops if l.vert is b)
        ma = (a.co + vmap[a].co) * 0.5; mb = (b.co + vmap[b].co) * 0.5
        for k in range(roll):
            q = (ring[a][k], ring[b][k], ring[b][k + 1], ring[a][k + 1])
            if len(set(q)) < 4: continue
            try: nf = bm.faces.new(q, f)
            except ValueError: continue
            nf.normal_update()
            if nf.normal.dot(nf.calc_center_median() - (ma + mb) * 0.5) < 0: nf.normal_flip()
            for l in nf.loops:
                src = la if l.vert in ring[a] else lb
                for lay in uvls: l[lay].uv = src[lay].uv
                for lay in cols: l[lay] = tuple(c * (0.82 if lay.name == "Col" else 1.0) for c in src[lay][:3]) + (1.0,)
    bm.to_mesh(me); bm.free(); me.update()


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
THICK = {"down": 0.011, "shell": 0.007, "fleece": 0.008}     # cloth thickness per jacket style (outer -> lining), m


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


BOOTS_SRC = os.path.join(ROOT, "Downloads", "boots_2026-11-14")


def make_scan_boots(name):
    """Real footwear: a downloaded scan (Downloads/boots_2026-11-14/<name>, CC BY - see CREDITS there) fitted to THIS
    body's feet: largest piece kept, decimated, its own up/toe found from the shape, scaled to the foot length plus toe
    room, checked so no foot vertex pokes through, skinned to foot/ball (bends at the ball), mirrored to the right foot.
    The user saw the earlier procedural boots as round flower pots (2026-11-14): no heel, no toe spring, no laces."""
    folder = os.path.join(BOOTS_SRC, name)
    before = set(bpy.data.objects)
    glb = os.path.join(folder, "model.glb"); obj = os.path.join(folder, "meshes", "model.obj")
    if os.path.exists(glb): bpy.ops.import_scene.gltf(filepath=glb)
    else: bpy.ops.wm.obj_import(filepath=obj)
    new = [o for o in bpy.data.objects if o not in before]
    meshes = [o for o in new if o.type == 'MESH']
    for o in new:
        if o.type != 'MESH': o.parent = None
    for o in meshes:
        mw = o.matrix_world.copy(); o.parent = None; o.matrix_world = mw
    activate(meshes[0])
    for o in meshes: o.select_set(True)
    bpy.ops.object.join(); src = bpy.context.active_object
    bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
    for o in new:                                          # empties/joined leftovers (joined meshes are already gone)
        try:
            if o != src: bpy.data.objects.remove(o)
        except ReferenceError: pass
    # texture: the scan's own base colour (embedded in a glb, a png next to an obj)
    tex = None
    for m in src.data.materials:
        if m and m.use_nodes:
            for n in m.node_tree.nodes:
                if n.type == 'TEX_IMAGE' and n.image and (tex is None or n.image.size[0] > tex.size[0]):
                    lk = [l for l in m.node_tree.links if l.from_node == n and l.to_socket.name == "Base Color"]
                    if lk or tex is None: tex = n.image
    tex_path = os.path.join(DEST, f"T_scan_{name}.png")
    gso_tex = os.path.join(folder, "materials", "textures", "texture.png")
    if os.path.exists(gso_tex):                          # Google Scanned Objects: the texture is a file next to the OBJ
        import shutil; tex = None
        if not (os.path.exists(tex_path) and os.path.getsize(tex_path) == os.path.getsize(gso_tex)):   # same file: skip
            shutil.copyfile(gso_tex, tex_path)                                                           # (a locked copy aborted the run)
    if tex is not None:
        if tex.packed_file or not os.path.exists(bpy.path.abspath(tex.filepath)):
            tex.filepath_raw = tex_path; tex.file_format = 'PNG'; tex.save()
        else:
            import shutil; shutil.copyfile(bpy.path.abspath(tex.filepath), tex_path)
    # weld the scan's seams, then keep ONE shoe: the largest welded piece, plus small parts (laces, eyelets, the sole
    # if separate) that lie inside its bounds. Pairs, ground patches and stray fragments are dropped.
    bm = bmesh.new(); bm.from_mesh(src.data)
    lo = Vector((min(v.co.x for v in bm.verts), min(v.co.y for v in bm.verts), min(v.co.z for v in bm.verts)))
    hi = Vector((max(v.co.x for v in bm.verts), max(v.co.y for v in bm.verts), max(v.co.z for v in bm.verts)))
    bmesh.ops.remove_doubles(bm, verts=bm.verts[:], dist=(hi - lo).length * 0.0005)
    bm.faces.ensure_lookup_table()
    seen, islands = set(), []
    for f in bm.faces:
        if f in seen: continue
        st, isl = [f], []
        while st:
            g = st.pop()
            if g in seen: continue
            seen.add(g); isl.append(g)
            for e in g.edges:
                for h in e.link_faces:
                    if h not in seen: st.append(h)
        islands.append(isl)
    islands.sort(key=lambda isl: -len(isl))
    main = islands[0]; keep = set(main)
    mv = {v for g in main for v in g.verts}
    blo = Vector((min(v.co.x for v in mv), min(v.co.y for v in mv), min(v.co.z for v in mv)))
    bhi = Vector((max(v.co.x for v in mv), max(v.co.y for v in mv), max(v.co.z for v in mv)))
    pad = (bhi - blo) * 0.03
    for isl in islands[1:]:
        c = sum((g.calc_center_median() for g in isl), Vector()) / len(isl)
        if all(blo[i] - pad[i] <= c[i] <= bhi[i] + pad[i] for i in range(3)): keep |= set(isl)
    bmesh.ops.delete(bm, geom=[f for f in bm.faces if f not in keep], context='FACES')
    bmesh.ops.delete(bm, geom=[v for v in bm.verts if not v.link_faces], context='VERTS')
    bm.to_mesh(src.data); bm.free(); src.data.update()
    n_tris = sum(len(p.vertices) - 2 for p in src.data.polygons)
    if n_tris > 9000:
        md = src.modifiers.new("dec", 'DECIMATE'); md.ratio = 9000.0 / n_tris; activate(src); bpy.ops.object.modifier_apply(modifier="dec")
    # the shoe's own frame: PCA -> length (largest), height, width (smallest); up = the side whose extreme points
    # are bunched (the ankle opening), not spread along the whole length (the sole); toe = away from the opening
    import numpy as np
    P = np.array([v.co[:] for v in src.data.vertices]); C = P.mean(0)
    w_, V_ = np.linalg.eigh(np.cov((P - C).T)); order = np.argsort(w_)[::-1]
    ax_len, ax_up, ax_w = V_[:, order[0]], V_[:, order[1]], V_[:, order[2]]
    def spread(sign):
        h = (P - C) @ (ax_up * sign); idx = h > np.quantile(h, 0.95)
        return np.ptp((P[idx] - C) @ ax_len), ((P[idx] - C) @ ax_len).mean()
    sp_pos, m_pos = spread(1); sp_neg, m_neg = spread(-1)
    up = ax_up if sp_pos < sp_neg else -ax_up
    top_along = m_pos if sp_pos < sp_neg else m_neg
    fwd_s = -ax_len if top_along > 0 else ax_len          # toe is away from the ankle opening
    side_s = np.cross(up, fwd_s)
    R_src = np.array([side_s, fwd_s, up])                  # rows: world x(side), y(fwd), z(up) in shoe coordinates
    Q = (P - C) @ R_src.T                                  # shoe points in its own frame: x side, y toe-ward, z up
    for i, v in enumerate(src.data.vertices): v.co = Vector(Q[i].tolist())
    src.data.update()
    Q = np.array([v.co[:] for v in src.data.vertices])
    # level the sole: the main axis tilts with a tall shaft, so pitch (about x) until the heel area and the ball area
    # touch the ground at the same height, then roll (about y) until the inner and outer edges do
    def level(Qa, axis_rot, a_idx, b_idx, lim):
        best = (1e9, 0.0, np.eye(3))
        for deg in np.arange(-lim, lim + 0.25, 0.5):
            t = np.radians(deg); c, s_ = np.cos(t), np.sin(t)
            if axis_rot == "x": R_ = np.array([[1, 0, 0], [0, c, -s_], [0, s_, c]])
            else: R_ = np.array([[c, 0, s_], [0, 1, 0], [-s_, 0, c]])
            Qr = Qa @ R_.T
            coord = Qr[:, 1] if axis_rot == "x" else Qr[:, 0]
            lo_, hi_ = coord.min(), coord.max(); u = (coord - lo_) / (hi_ - lo_)
            za = np.quantile(Qr[(u > a_idx[0]) & (u < a_idx[1]), 2], 0.03); zb = np.quantile(Qr[(u > b_idx[0]) & (u < b_idx[1]), 2], 0.03)
            score = abs(za - zb)
            if score < best[0]: best = (score, deg, R_)
        return Qa @ best[2].T, best[1]
    Q, pitch_deg = level(Q, "x", (0.04, 0.22), (0.55, 0.78), 25)  # heel strip vs ball strip (toe spring stays free)
    Q, roll_deg = level(Q, "y", (0.05, 0.3), (0.7, 0.95), 12)     # inner edge vs outer edge (a real sole is near level)
    for i, v in enumerate(src.data.vertices): v.co = Vector(Q[i].tolist())
    src.data.update()
    print("LEVEL", name, "pitch", pitch_deg, "roll", roll_deg)
    shoe_len = np.ptp(Q[:, 1]); shoe_w = np.ptp(Q[:, 0]); shoe_h = np.ptp(Q[:, 2])
    pieces = []
    for sd in ("l", "r"):
        fv = np.array([v.co[:] for v in body.data.vertices if BPART[v.index] == "foot" and v.co.x * SIDE[sd] > 0])
        fwd = Vector((B[f"ball_{sd}"].x - B[f"foot_{sd}"].x, B[f"ball_{sd}"].y - B[f"foot_{sd}"].y, 0)).normalized()
        side = Vector((fwd.y, -fwd.x, 0)) * (1 if sd == "l" else -1)     # outward
        along = fv[:, :2] @ np.array([fwd.x, fwd.y]); across = fv[:, :2] @ np.array([side.x, side.y])
        foot_len = np.ptp(along); foot_w = np.ptp(across); heel = along.min(); mid_across = (across.max() + across.min()) / 2
        scale = (foot_len + 0.05) / shoe_len                             # toe room + heel cup (a boot is ~2.5 cm longer than the foot)
        sx = max(scale, (foot_w + 0.018) / shoe_w)                       # never narrower than the foot + padding
        o = src.copy(); o.data = src.data.copy(); bpy.context.collection.objects.link(o); o.name = f"boot_{sd}"
        lo_y = Q[:, 1].min(); lo_z = Q[:, 2].min(); cx = (Q[:, 0].max() + Q[:, 0].min()) / 2
        for v in o.data.vertices:
            x = (v.co.x - cx) * sx * (1 if sd == "l" else -1); y = (v.co.y - lo_y) * scale; z = (v.co.z - lo_z) * scale
            base = fwd * (heel - 0.02) + side * mid_across      # the heel sits ~2 cm inside the heel cup
            v.co = Vector((base.x, base.y, 0)) + fwd * y + side * x * (1 if sd == "l" else -1) + Vector((0, 0, z - 0.022))   # the foot stands on the insole, ~2 cm up: the sole sinks into the snow
        if sd == "r":                                                    # mirrored: flip winding so normals face out
            bm = bmesh.new(); bm.from_mesh(o.data); bmesh.ops.reverse_faces(bm, faces=bm.faces[:]); bm.to_mesh(o.data); bm.free()
        o.data.update()
        # no foot vertex may poke through: grow the shoe about its centre until the foot is inside
        bpy.context.view_layer.update()
        tree = BVHTree.FromObject(o, bpy.context.evaluated_depsgraph_get())
        top = max(v.co.z for v in o.data.vertices)
        test = [Vector(c) for c in fv if c[2] < top - 0.02]
        # the scanned collar rim is ragged (spikes would pierce the trouser hem): trim at the typical collar height
        zs_ = np.array([v.co.z for v in o.data.vertices]); collar = float(np.percentile(zs_, 96))
        cut(o, (0, 0, collar), (0, 0, 1))
        # the instep (laces/tongue) is where a foot of another shape meets the scan: lift only the boot surface there
        # (ahead of the ball the whole shoe may move: the sole's toe spring would otherwise lift off the flat MetaHuman toes)
        ball_ = B[f"ball_{sd}"]
        push_out(o, bvh_of([body]), 0.005, only=lambda i, c: c.z > 0.02 or (c - ball_).dot(fwd) > -0.01)
        bpy.context.view_layer.update(); tree = BVHTree.FromObject(o, bpy.context.evaluated_depsgraph_get())
        worst, wpos = 0.0, None
        for c in test:
            hit = tree.find_nearest(c)
            if hit[0] is not None:
                d = (c - hit[0]).dot(hit[1])
                if d > worst: worst, wpos = d, c
        npoke = sum(1 for c in test if (lambda h: h[0] is not None and (c - h[0]).dot(h[1]) > 0.003)(tree.find_nearest(c)))
        if wpos is not None:
            rel = wpos - B[f"foot_{sd}"]
            print("POKE", sd, "count", npoke, "of", len(test), "worst at along", round(rel.dot(fwd), 3), "side", round(rel.dot(side), 3), "z", round(wpos.z, 3))
        print("BOOT", name, sd, "foot", round(foot_len, 3), "x", round(foot_w, 3), "shoe scale", round(scale, 3), "worst poke", round(worst, 4))
        # skin: rigid to the foot, the toe box blends to the ball joint (the shoe bends where a real one creases)
        ball = B[f"ball_{sd}"]
        for g in GROUPS: o.vertex_groups.new(name=g)
        for v in o.data.vertices:
            t = smoothstep(-0.035, 0.03, (v.co - ball).dot(fwd))
            vg(o, f"foot_{sd}").add([v.index], 1.0 - t, 'REPLACE')
            if t > 0: vg(o, f"ball_{sd}").add([v.index], t, 'REPLACE')
        # trail dust: scans are showroom-clean (white trainers glowed on the snow)
        dirt = {"striper_sneaker_white": (0.6, 0.57, 0.52), "reebok_dmx_rainwalker": (0.55, 0.52, 0.47)}.get(name, (0.88, 0.86, 0.82))
        o.data.materials.clear(); o.data.materials.append(slot("boots", "scan", "@scan:" + name, dirt))
        for p_ in o.data.polygons: p_.material_index = 0; p_.use_smooth = True
        set_vcol(o, [1.0] * len(o.data.vertices))
        PIECES.append(o); LAYERS.append(o); pieces.append(o)
    bpy.data.objects.remove(src)
    return float(np.percentile([v.co.z for o in pieces for v in o.data.vertices], 97))


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


def fold_noise(th, z):
    """Cheap smooth irregularity so folds are not machine-perfect rings."""
    return 0.55 * math.sin(th * 3 + z * 21) + 0.3 * math.sin(th * 7.3 - z * 13.7) + 0.15 * math.sin(th * 13.1 + z * 37)


def leg_fold(co, hem):
    """Displacement (m, along the normal) of trouser folds: compression rings behind the knee (deeper at the back),
    a stack of irregular rings above the hem, horizontal drag folds under the seat."""
    s = 1.0 if co.x > 0 else -1.0; ax = Vector((0.11 * s, B["pelvis"].y))
    th = math.atan2(co.y - ax.y, co.x - ax.x)
    back = 1.0 if (co.y - B["pelvis"].y) * FRONT < 0 else 0.35
    ph = fold_noise(th, co.z) * 1.4
    knee = 0.006 * back * gaussian(co.z, Z_KNEE + 0.01, 0.055) * math.sin(co.z * 2 * math.pi / 0.034 + ph)
    stack = 0.006 * smoothstep(hem + 0.13, hem + 0.02, co.z) * math.sin(co.z * 2 * math.pi / 0.03 + ph * 1.6)
    seat = 0.003 * back * gaussian(co.z, Z_CROTCH - 0.05, 0.035) * math.sin(co.z * 2 * math.pi / 0.03 + ph * 0.6)
    return knee + stack + seat


def leg_hull(side, z0, z1, step=0.02, NT=48, fall=0.98):
    """Per-leg outline in horizontal slices (convex support radius around each slice's centre), smoothed, and from
    the seat down never shrinking faster than `fall` per slice: a trouser leg hangs from the seat and the knee
    instead of shrink-wrapping the calf and hamstrings (body-suit look, director 2026-11-20)."""
    pts = [v.co for v in body.data.vertices if BPART[v.index] in ("thigh", "calf", "pelvis", "other") and v.co.x * side > 0.004]
    zs, cen, rows = [], [], []
    z = z0
    while z <= z1 + 1e-6:
        band = [c for c in pts if abs(c.z - z) < step * 0.75]
        if len(band) > 10:
            cx = sum(c.x for c in band) / len(band); cy = sum(c.y for c in band) / len(band)
            rad = [max((c.x - cx) * math.cos(2 * math.pi * j / NT) + (c.y - cy) * math.sin(2 * math.pi * j / NT) for c in band) for j in range(NT)]
            zs.append(z); cen.append(Vector((cx, cy))); rows.append(rad)
        z += step
    for _ in range(2):
        rows = [[(r[(j - 1) % NT] + 2 * r[j] + r[(j + 1) % NT]) / 4 for j in range(NT)] for r in rows]
    K = len(rows)
    for _ in range(3):
        cen = [(cen[max(k - 1, 0)] + cen[k] * 2 + cen[min(k + 1, K - 1)]) / 4 for k in range(K)]
        rows = [[(rows[max(k - 1, 0)][j] + 2 * rows[k][j] + rows[min(k + 1, K - 1)][j]) / 4 for j in range(NT)] for k in range(K)]
    for k in range(K - 2, -1, -1):
        rows[k] = [max(rows[k][j], rows[k + 1][j] * fall) for j in range(NT)]
    def f(co):
        if not zs: return None
        kf = max(0.0, min(K - 1.001, (co.z - zs[0]) / step)); k = int(kf); t = kf - k
        c = cen[k].lerp(cen[min(k + 1, K - 1)], t)
        th = math.atan2(co.y - c.y, co.x - c.x) % (2 * math.pi); jf = th / (2 * math.pi) * NT; j0 = int(jf) % NT; j1 = (j0 + 1) % NT; u = jf - int(jf)
        r0 = rows[k][j0] * (1 - u) + rows[k][j1] * u; k1 = min(k + 1, K - 1)
        r1 = rows[k1][j0] * (1 - u) + rows[k1][j1] * u
        return c, r0 * (1 - t) + r1 * t
    return f


def drape_legs(o, style, spec, hem):
    """Trousers on their hanging outline (leg_hull + ease) below the crotch, on the torso outline over the seat: the
    crotch, hamstrings and calves stop showing through. The ease grows towards the hem (cloth falls past the ankle)."""
    me = o.data; me.update()
    fall = 0.985 if spec.get("rolled") or style == "down" else 0.975
    hulls = {s: leg_hull(SIDE[s], hem - 0.04, Z_CROTCH + 0.04, fall=fall) for s in ("l", "r")}
    seat, scy = torso_hull(Z_CROTCH - 0.02, Z_PELVIS + 0.14)
    base = 0.026 if style == "down" else (0.016 if spec.get("rolled") else 0.010)
    for v in me.vertices:
        c = v.co; s = "l" if c.x > 0 else "r"
        wn = 1.0 - smoothstep(0.55, 0.9, abs(v.normal.z))         # crotch/seat undersides: leave to the smoothing
        ease = base + 0.010 * smoothstep(Z_KNEE, hem, c.z)
        wl = smoothstep(Z_CROTCH + 0.01, Z_CROTCH - 0.05, c.z) * wn
        if wl > 0:
            h = hulls[s](c)
            if h:
                cc, r = h; d = Vector((c.x - cc.x, c.y - cc.y)); L = d.length
                if L > 1e-4:
                    tgt = cc + d * ((r + ease) / L)
                    if tgt.x * SIDE[s] < 0.006: tgt.x = 0.006 * SIDE[s]          # never past the centre line
                    v.co.x += (tgt.x - c.x) * wl * 0.9; v.co.y += (tgt.y - c.y) * wl * 0.9
        ws = smoothstep(Z_CROTCH - 0.03, Z_CROTCH + 0.04, c.z) * wn * smoothstep(Z_PELVIS + 0.12, Z_PELVIS + 0.04, c.z)
        if ws > 0:
            r = seat(c)
            if r:
                d = Vector((c.x, c.y - scy)); L = d.length
                if L > 1e-4 and L < r + ease:
                    k = (r + ease * 0.8) / L; v.co.x += (d.x * k - d.x) * ws; v.co.y += (d.y * k - d.y) * ws
    me.update()
    laplace(o, 6, 0.5)


def make_pants(spec, boot_top):
    style = spec["style"]
    waist = Z_PELVIS + 0.115
    hem = boot_top - 0.03
    if spec.get("rolled"):                   # rolled to mid-calf: dark socks show between the cuff and the trainers
        hem = Z_KNEE - 0.16
        so = shell("socks", lambda i, c, p: p in ("calf", "foot") and Z_ANKLE - 0.02 < c.z < hem + 0.06)
        cut(so, (0, 0, hem + 0.04), (0, 0, 1)); cut(so, (0, 0, Z_ANKLE - 0.01), (0, 0, -1))
        decimate_to(so, 0.02); subdivide(so); inflate(so, lambda i, c, n: 0.004); laplace(so, 3, 0.4)
        push_out(so, bvh_of([body]), 0.003)
        assign_mats(so, [slot("pants", "socks", "knit", (0.03, 0.03, 0.035))], lambda i, c: 0)
        set_vcol(so, [0.85] * len(so.data.vertices)); uv_metres(so); PIECES.append(so); LAYERS.append(so)
    o = shell("pants", lambda i, c, p: (p in ("pelvis", "thigh", "calf") or (p == "other" and abs(c.x) < 0.22)) and hem - 0.04 < c.z < waist + 0.04)
    cut(o, (0, 0, waist), (0, 0, 1)); cut(o, (0, 0, hem), (0, 0, -1))
    decimate_to(o, 0.03); subdivide(o)
    def off(i, c, n):
        base = 0.026 if style == "down" else (0.019 if spec.get("rolled") else 0.010)   # porters' cargo trousers hang loose
        flare = 0.022 * smoothstep(Z_KNEE - 0.1, hem, c.z)
        return base + flare + 0.012 * smoothstep(Z_PELVIS - 0.08, Z_PELVIS - 0.25, c.z) * (style != "down")
    inflate(o, off); laplace(o, 10, 0.5)
    drape_legs(o, style, spec, hem)
    ao = [1.0] * len(o.data.vertices)
    if style == "down":
        inflate(o, lambda i, c, n: 0.013 * pillow(c.z, 0.10) ** 0.6)
        ao = [0.6 + 0.4 * pillow(v.co.z, 0.10) ** 0.5 for v in o.data.vertices]
    else:   # soft folds where the fabric bunches: behind the knee, stacked over the shoes, under the seat
        fl = [leg_fold(v.co, hem) for v in o.data.vertices]
        inflate(o, lambda i, c, n: fl[i])
        ao = [1.0 - 0.22 * max(0.0, -fl[v.index]) / 0.004 - 0.05 for v in o.data.vertices]
    push_out(o, bvh_of(LAYERS), 0.006)
    laplace(o, 3, 0.45)                      # the scanned shoe collars are ragged: settle the hem over them
    push_out(o, bvh_of(LAYERS), 0.004)
    main = slot("pants", "main", spec.get("fabric", "nylon" if style == "down" else "softshell"), spec["main"])
    belt = slot("pants", "belt", "webbing", (0.03, 0.03, 0.03))
    assign_mats(o, [main, belt], lambda i, c: 1 if c.z > waist - 0.035 else 0)
    # hem: turn the cuff inward so the opening has a thickness; waist band tucks in
    for edges, vs in boundary_loops(o):
        cz = loop_centre(o, vs).z
        if cz < Z_KNEE:
            c = loop_centre(o, vs)
            if spec.get("rolled"): extrude_loop(o, edges, [(-0.003, 0.007), (0.05, 0.002), (0.003, -0.007)], c, (0, 0, 1))   # the rolled cuff: a flat folded band (1.2 cm read as a rubber ring)
            # plain hem: no inward lip any more - thicken() gives it a rolled, lined edge
    ao += [0.7] * (len(o.data.vertices) - len(ao))
    set_vcol(o, ao); uv_metres(o)
    py_ = B["pelvis"].y
    seam_uv(o, [lambda c: (c.z - (waist - 0.035), around(c, py_)),                        # waistband
                lambda c: (c.y - py_, c.z) if c.z < Z_CROTCH + 0.02 else None,            # side seams + inseams
                lambda c: (c.x, c.z) if (c.y - py_) * FRONT > 0 and c.z > Z_CROTCH else None], edge_inset=0.012)   # fly
    finish(o, 0.004); o["thick"] = 0.007 if spec.get("rolled") else 0.006
    PIECES.append(o); LAYERS.append(o)
    return waist


def dbg_loops(o, tag):
    for edges, vs in boundary_loops(o):
        X = [o.data.vertices[i].co.x for i in vs]; Z = [o.data.vertices[i].co.z for i in vs]
        print("DBG", tag, len(vs), "x", round(min(X), 3), round(max(X), 3), "z", round(min(Z), 3), round(max(Z), 3))


def torso_hull(z0, z1, step=0.02, NT=64, fall=None, z_fall=None):
    """Convex outline of the torso (no arms) in horizontal slices: cloth hangs straight over hollows (under the chest,
    the spine groove, the waist) the way a real jacket does. Returns f(co) -> hull radius at co's height/angle, centre y.
    fall: below z_fall the outline may shrink at most by this factor per slice going down - cloth FALLS from the bust,
    belly and shoulder blades instead of tucking in under them (gravity drape, 2026-11-20)."""
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
    if fall:
        for k in range(len(rows) - 2, -1, -1):
            if zs[k] < (z_fall if z_fall is not None else z1):
                rows[k] = [max(rows[k][j], rows[k + 1][j] * fall) for j in range(NT)]
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


def neck_axis(z):
    """Centre of the neck at height z: the neck leans forward, so the axis runs neck_01 -> head, not straight up."""
    a, b = B["neck_01"], B["head"]
    t = max(0.0, min(1.0, (z - a.z) / max(b.z - a.z, 1e-3)))
    return Vector((a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t))


_NECK_TREE = []


def neck_radius(z, th, fallback=0.06):
    """Distance from the neck axis to the OUTERMOST skin (body + face) at height z, angle th: rays shot inwards from
    15 cm out, max over a few heights/angles so the body/face seam cannot leak a ray through. (Outward rays leaked
    through the seam, vertex bins were too sparse on the face mesh and put the collar inside the neck.) The old collars
    used the widest radius of the whole neck base at every height, so their top stood 2-3 cm off the neck: the dark
    gap the director saw (2026-11-20)."""
    if not _NECK_TREE:
        bm = bmesh.new()
        for src in (body, face):
            t = bmesh.new(); t.from_mesh(src.data)
            bmesh.ops.delete(t, geom=[f for f in t.faces if all(v.co.z > BODY_TOP + 0.14 or v.co.z < BODY_TOP - 0.16 for v in f.verts)], context='FACES')
            me = bpy.data.meshes.new("_nk"); t.to_mesh(me); t.free(); bm.from_mesh(me); bpy.data.meshes.remove(me)
        _NECK_TREE.append(BVHTree.FromBMesh(bm)); bm.free()
    tree = _NECK_TREE[0]; best = 0.0
    for dz in (-0.003, 0.0, 0.003):
        c = neck_axis(z + dz)
        for da in (-0.035, 0.0, 0.035):
            d = Vector((math.cos(th + da), math.sin(th + da), 0.0)); o = Vector((c.x, c.y, z + dz)) + d * 0.15
            hit = tree.ray_cast(o, -d, 0.149)
            if hit[0] is not None: best = max(best, 0.15 - (hit[0] - o).length)
    return best if best > 0.0 else fallback


def collar_rings(o, edges, vs, h_front, h_back, clr_top, t, mat_index, ease=0.010, rings=6, n_ext=3, pad=0.016, ext_mat=None):
    """Stand collar on the neckline loop, built like a real one: a near-vertical tube whose LINING touches the neck
    at the top edge (clearance clr_top, after thicken() offsets the lining t inwards) and stands `ease` wider at its
    seam. The seam sits where that tube meets the body (high on the trapezius at the sides, at the jugular notch in
    front); the jacket's opening is lower and wider (it is a copy of the body's neck seam), so n_ext new rows first
    carry the yoke over the trapezius to the seam (pad above the skin), then `rings` rows rise as the stand. New rows
    only: moving the jacket's own neckline instead crumpled the yoke and its baffle lines. Top edge: h_front above the
    seam at the throat, h_back at the nape, a smooth line between."""
    me = o.data
    th_front = -math.pi / 2 if FRONT < 0 else math.pi / 2
    def ang(co):
        c = neck_axis(co.z); return math.atan2(co.y - c.y, co.x - c.x)
    def backness(th): return 0.5 - 0.5 * math.cos(th - th_front)
    def seam_for(th, z0, h):
        """lowest z >= z0 where the tube (top edge h higher) clears the body: the seam; R at seam and top"""
        z = z0
        while True:
            R_top = neck_radius(z + h, th) + clr_top + t; R_base = R_top + ease
            if z >= z0 + 0.15 or neck_radius(z, th) + 0.004 + t <= R_base: return z, R_base, R_top
            z += 0.002
    loop = loop_order(me, vs)
    zf = min(me.vertices[i].co.z for i in loop); zbk = max(me.vertices[i].co.z for i in loop)
    sf = seam_for(th_front, zf, h_front)[0]; sb = seam_for(th_front + math.pi, zbk, h_back)[0]
    zt_f, zt_b = sf + h_front, sb + h_back
    def z_top(th): return zt_f + (zt_b - zt_f) * backness(th)
    plan = {}
    for i in loop:
        co = me.vertices[i].co; th = ang(co); c = neck_axis(co.z)
        zs, _, _ = seam_for(th, co.z, max(z_top(th) - co.z, 0.03))
        zs = min(zs, z_top(th) - 0.03)                 # at least 3 cm of stand everywhere
        Rt = neck_radius(z_top(th), th) + clr_top + t; Rb = max(Rt + ease, neck_radius(zs, th) + 0.004 + t)
        plan[i] = (co.z, (Vector((co.x, co.y)) - c).length, th, zs, Rb, Rt)
    nc = len(me.vertices); total = n_ext + rings; H = 1.0; dz = H / total
    extrude_loop(o, edges, [(dz, 0.0)] * total, Vector((0, 0, 0)), (0, 0, 1), mat_index=mat_index)
    key = lambda co: (round(co.x, 5), round(co.y, 5))
    by_xy = {key(me.vertices[i].co): plan[i] for i in loop}
    stand = set()
    for v in me.vertices[nc:]:
        p = by_xy.get(key(v.co))
        if p is None: continue
        z0, R0, th, zs, Rb, Rt = p
        k = int(round((v.co.z - z0) / dz))
        if k > n_ext: stand.add(v.index)
        if k <= n_ext:                       # over the trapezius to the seam
            u = smoothstep(0.0, 1.0, k / n_ext)
            z = z0 + (zs - z0) * u; R = R0 + (Rb - R0) * u
            clr0 = max(R0 - neck_radius(z0, th), pad)      # the yoke's own clearance at the old opening: no dip below it
            R = max(R, neck_radius(z, th) + clr0 * (1.0 - u) + (0.004 + t) * u)
            R = min(R, R0 + 0.002)   # the inward ray also meets the deltoid at these heights: never wider than the opening
        else:                                # the stand
            f = (k - n_ext) / rings
            zt = z_top(th); z = zs + f * (zt - zs)
            R = Rb + (Rt - Rb) * smoothstep(0.0, 1.0, f)
            R = min(max(R, neck_radius(z, th) + clr_top + t), Rb + 0.015)
        c = neck_axis(z)
        v.co = Vector((c.x + math.cos(th) * R, c.y + math.sin(th) * R, z))
    me.update()
    if ext_mat is not None:     # the rows over the trapezius are the garment's own cloth, only the stand is collar
        for pl in me.polygons:
            if any(i >= nc for i in pl.vertices) and not any(i in stand for i in pl.vertices): pl.material_index = ext_mat
    cz = [v.co.z for v in me.vertices[nc:]]
    print("COLLAR verts", len(me.vertices) - nc, "z", round(min(cz), 3), round(max(cz), 3), "top front/back", round(zt_f, 3), round(zt_b, 3),
          "seam z range", round(min(p[3] for p in plan.values()), 3), round(max(p[3] for p in plan.values()), 3))
    return nc


def blend_weights_up(o, first, z0, z1):
    """New collar vertices take the nearest body vertex's weights more and more towards the top (neck bones), so a
    turning head carries the collar's top edge with the neck instead of cutting through it."""
    bw = [{GROUPS[g.group]: g.weight for g in v.groups} for v in body.data.vertices]
    for v in o.data.vertices[first:]:
        k = smoothstep(z0, z1, v.co.z)
        if k <= 0: continue
        own = {o.vertex_groups[g.group].name: g.weight for g in v.groups}
        _, bi, _ = KD.find(v.co); tgt = bw[bi]
        for name in set(own) | set(tgt):
            w = own.get(name, 0.0) * (1 - k) + tgt.get(name, 0.0) * k
            if w > 0.001: vg(o, name).add([v.index], w, 'REPLACE')
            elif name in own: o.vertex_groups[name].remove([v.index])


def drape_sleeves(o, sleeve, sag, amp, hem=None):
    """Gravity + wear on a sleeve and body: in the A-pose the underside of a loose sleeve hangs (moved straight down
    by `sag` where the cloth faces down), compression folds ring the inside of the elbow and bunch above the cuff,
    and (hem given) a shell blouses in soft horizontal folds just above its hem. Displacements along the normal."""
    me = o.data; me.update()
    nb, _ = neighbours(me)
    sw = smooth_field([sleeve[v.index] * max(0.0, -v.normal.z) ** 1.5 * smoothstep(0.06, 0.18, arm_t(v.co, "l" if v.co.x > 0 else "r"))
                       for v in me.vertices], nb, 6)   # smooth (a per-vertex normal test spiked the bumpy shoulder); none in the armpit
    for v in me.vertices: v.co.z -= sag * sw[v.index]
    me.update(); spikes(o, "sag")
    def fold(i, c, n):
        sl = sleeve[i]; d = 0.0
        if sl > 0.01:
            s = "l" if c.x > 0 else "r"; ch = arm_chain(s)
            L1 = (ch[1] - ch[0]).length; L2 = (ch[2] - ch[1]).length; t = arm_t(c, s)
            ph = fold_noise(math.atan2(c.z - ch[1].z, c.y - ch[1].y), t) * 1.3
            inner = 1.0 if n.z > -0.2 and (c - ch[1]).dot(Vector((0, FRONT, 0))) > -0.01 else 0.45   # crook of the elbow
            d += amp * inner * gaussian(t, L1 + 0.01, 0.05) * math.sin(t * 2 * math.pi / 0.032 + ph)
            d += amp * smoothstep(L1 + L2 - 0.16, L1 + L2 - 0.05, t) * math.sin(t * 2 * math.pi / 0.028 + ph * 1.5)
            d *= sl
        if hem is not None:
            th = math.atan2(c.y - B["spine_03"].y, c.x); body_w = (1 - sl)
            d += amp * 0.8 * body_w * smoothstep(hem + 0.16, hem + 0.04, c.z) * math.sin(c.z * 2 * math.pi / 0.038 + fold_noise(th, c.z) * 1.5)
            # gravity folds: soft vertical waves hanging from the chest/shoulder blades, fading out at the hem band
            d += amp * 0.7 * body_w * smoothstep(Z_CHEST - 0.04, Z_CHEST - 0.18, c.z) * smoothstep(hem + 0.02, hem + 0.10, c.z) * math.sin(th * 0.16 * 2 * math.pi / 0.085 + 0.8 * math.sin(c.z * 9 + th * 2))
        s_ = "l" if c.x > 0 else "r"           # nothing in the armpit: the push-out then flips verts between arm and side
        return d * (1.0 - gaussian((c - (B[f"upperarm_{s_}"] - Vector((0, 0, 0.07)))).length, 0, 0.08))
    inflate(o, fold)


def spikes(o, tag, lim=0.003):
    """Debug (YGG_DBG=1): vertices standing out of their 1-ring by more than lim, with where they are."""
    if not os.environ.get("YGG_DBG"): return
    me = o.data; me.update(); nb, bnd = neighbours(me); out = []
    bm = bmesh.new(); bm.from_mesh(me); fl = []
    for f in bm.faces:
        ns = [g.normal for e in f.edges for g in e.link_faces if g is not f]
        if ns and f.normal.dot(sum(ns, Vector())) < 0: fl.append(tuple(round(x, 3) for x in f.calc_center_median()))
    bm.free(); print("FLIPPED", tag, len(fl), fl[:6])
    for v in me.vertices:
        if bnd[v.index] or not nb[v.index]: continue
        avg = sum((me.vertices[j].co for j in nb[v.index]), Vector()) / len(nb[v.index])
        d = abs((v.co - avg).dot(v.normal))
        if d > lim and v.co.z > Z_CHEST - 0.15 and abs(v.co.x) < 0.25: out.append((round(d, 4), tuple(round(x, 3) for x in v.co)))
    out.sort(reverse=True); print("SPIKES", tag, len(out), out[:4])


def yoke_off_shoulders(o):
    """The pulled-in yoke stays off the shoulders: push faded in over 10 cm (a hard z cut-off creased the shoulder
    slope), then relax the yoke and push again normal-only (the nearest-point push left thin spikes at the clavicle)."""
    me = o.data; me.update(); n0 = len(me.vertices)
    old = [v.co.copy() for v in me.vertices]
    push_out(o, bvh_of([body]), 0.010, only=lambda i, c: c.z > Z_CHEST - 0.10)
    for v in me.vertices: v.co = old[v.index].lerp(v.co, smoothstep(Z_CHEST - 0.10, Z_CHEST, old[v.index].z))
    me.update()
    nb, bnd = neighbours(me)
    for _ in range(3):
        P = [v.co.copy() for v in me.vertices]
        for v in me.vertices:
            i = v.index
            if v.co.z > Z_CHEST - 0.03 and not bnd[i] and nb[i]:
                v.co = P[i].lerp(sum((P[j] for j in nb[i]), Vector()) / len(nb[i]), 0.5)
    me.update()
    push_out(o, bvh_of([body]), 0.008, only=lambda i, c: c.z > Z_CHEST, tangent=1.0)


def hug(o, first, gap):
    """Vertices from `first` on that are farther than gap from the body move in to gap (cuffs onto the wrists)."""
    me = o.data; wt = bvh_of([body])
    for v in me.vertices[first:]:
        h = wt.find_nearest(v.co)
        if h[0] is None: continue
        off = v.co - h[0]; L = off.length
        if L > gap: v.co = h[0] + off * (gap / L)
    me.update()


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
    me = o.data; spikes(o, "subdiv")
    sleeve = sleeve_field(o)
    base_t, base_s, amp_t, amp_s, per_t, per_s = {"down": (0.032, 0.024, 0.016, 0.013, 0.115, 0.085),
                                                  "shell": (0.018, 0.016, 0, 0, 1, 1), "fleece": (0.014, 0.012, 0, 0, 1, 1)}[style]
    def side_of(c): return "l" if c.x > 0 else "r"
    inflate(o, lambda i, c, n: base_t * (1 - sleeve[i]) + base_s * sleeve[i] + 0.010 * smoothstep(Z_PELVIS + 0.05, hem, c.z))
    laplace(o, 28 if style == "down" else 20, 0.6)       # lose the body's muscles/breasts: cloth bridges hollows
    spikes(o, "laplace")
    # torso: put every vertex on the convex outline of its slice + the padding, so it hangs straight and clean
    hull, hcy = torso_hull(cut_z - 0.06, BODY_TOP - 0.06, fall={"down": 0.995, "shell": 0.99, "fleece": 0.985}[style], z_fall=Z_CHEST)
    pad = {"down": 0.040, "shell": 0.022, "fleece": 0.017}[style]
    # only where the cloth faces sideways: on the shoulder tops slices are ill-defined (keep the smoothed shape);
    # below the chest everything goes to the outline (under-bust/belly faces point down and kept the body's shape).
    # The weight field is smoothed: per-vertex normal tests left creases on the shoulder slope
    W = smooth_field([(1.0 - min(1.0, sleeve[v.index] * 1.6)) * smoothstep(BODY_TOP - 0.02, BODY_TOP - 0.10, v.co.z) * max(smoothstep(0.75, 0.35, abs(v.normal.z)), smoothstep(Z_CHEST - 0.02, Z_CHEST - 0.10, v.co.z))
                      for v in me.vertices], neighbours(me)[0], 5)
    for v in me.vertices:
        w = W[v.index]
        if w <= 0.0: continue
        r = hull(v.co)
        if r is None: continue
        d = Vector((v.co.x, v.co.y - hcy, 0.0)); L = d.length
        if L < 1e-4: continue
        target = d * ((r + pad + 0.012 * smoothstep(Z_PELVIS + 0.05, hem, v.co.z)) / L)
        v.co.x = v.co.x + (target.x - d.x) * w; v.co.y = v.co.y + (target.y - d.y) * w
    me.update()
    laplace(o, 6, 0.5)
    push_relax(o, bvh_of(LAYERS), base_s * 0.75)
    laplace(o, 3, 0.5)
    hf, hb = {"down": (0.058, 0.072), "shell": (0.058, 0.072), "fleece": (0.042, 0.054)}[style]
    ease = {"down": 0.012, "shell": 0.009, "fleece": 0.007}[style]
    spikes(o, "hull+push")
    drape_sleeves(o, sleeve_field(o), {"down": 0.006, "shell": 0.011, "fleece": 0.009}[style],
                  {"down": 0.003, "shell": 0.0065, "fleece": 0.005}[style], hem if style != "down" else None)
    spikes(o, "drape")
    sleeve = sleeve_field(o)
    def puff(c, sl):   # full rounded tubes with tight valleys (^0.35): sewn-through baffles, not soft lumps
        tor = pillow(c.z, per_t) ** 0.35; slv = pillow(arm_t(c, side_of(c)), per_s) ** 0.35
        return tor * (1 - sl) + slv * sl
    def is_front(c): return (c.y - B["spine_03"].y) * FRONT > 0
    if style == "down":
        # stitch lines first: an edge row exactly in every baffle valley, so each seam is a crisp crease after the puff
        k0 = int(hem / per_t) - 1
        for k in range(k0, int(BODY_TOP / per_t) + 2):
            for dz in (-0.006, 0.0, 0.006):      # the seam + a row on each tube wall: clean walls, no pinched zigzags
                score(o, (0, 0, k * per_t + dz), (0, 0, 1), only=lambda c: abs(c.x) < 0.2)
        for sd in ("l", "r"):
            ch = arm_chain(sd); seg = [(a, b, (b - a).length) for a, b in zip(ch, ch[1:])]
            total = sum(L for _, _, L in seg); t = per_s
            while t < total - 0.02:
                acc = 0.0
                for a, b, L in seg:
                    if acc + L >= t:
                        pnt = a + (b - a) * ((t - acc) / L); dn = (b - a).normalized()
                        for dt in (-0.005, 0.0, 0.005):
                            score(o, pnt + dn * dt, (b - a), only=lambda c, sd=sd, a=a, b=b: c.x * SIDE[sd] > 0.2 and (c - a).dot((b - a).normalized()) > -0.03 and (c - b).dot((a - b).normalized()) > -0.03)
                        break
                    acc += L
                t += per_s
        sleeve = sleeve_field(o)
        inflate(o, lambda i, c, n: (amp_t * (1 - sleeve[i]) + amp_s * sleeve[i]) * puff(c, sleeve[i]))
    else:
        yoke = Z_CHEST - 0.015
        pass   # (the 2 mm yoke dent creased the sloped shoulders into gashes; the seam reads from AO + stitches)
    push_relax(o, bvh_of(LAYERS), 0.008 if style == "down" else 0.006); spikes(o, "push2")
    print("DESPIKE jacket", despike(o, 0.003 if style == "down" else 0.0025, only=lambda i, c: c.z > Z_CHEST - 0.2)); spikes(o, "despiked")
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
    mats_by_face(o, [main, panel, trim], tag); spikes(o, "scores")
    n0 = len(me.vertices); collar_first = None
    for edges, vs in boundary_loops(o):
        c = loop_centre(o, vs)
        print("LOOP", round(c.x, 3), round(c.z, 3), len(vs), "x", round(min(o.data.vertices[i].co.x for i in vs), 3), round(max(o.data.vertices[i].co.x for i in vs), 3), "z", round(min(o.data.vertices[i].co.z for i in vs), 3), round(max(o.data.vertices[i].co.z for i in vs), 3))
        if c.z > Z_CHEST:                                                 # collar up the neck
            # close the wide collarbone opening round the neck first, then a stand collar whose LINING lies on the
            # neck skin (clearance 3-6 mm) all round: no dark gap into the jacket from any angle (2026-11-20)
            close_neckline(o, vs, {"down": 0.016, "shell": 0.010, "fleece": 0.008}[style])
            yoke_off_shoulders(o)              # BEFORE the collar rows: pushed later, the yoke rose over them (cracks)
            nc = collar_rings(o, edges, vs, hf, hb, 0.0035, THICK[style], 1 if style == "down" else 0, ease=ease,
                              pad=THICK[style] + (0.010 if style == "down" else 0.005))
            collar_first = nc
        elif abs(c.x) > 0.2:                                              # cuffs (elastic, trim colour)
            sd = side_of(c); d = (B[f"hand_{sd}"] - B[f"lowerarm_{sd}"]).normalized()
            nq = len(me.vertices)
            extrude_loop(o, edges, [(0.008, -0.009), (0.012, -0.001), (0.012, 0.0)], B[f"lowerarm_{sd}"], d, mat_index=2)
            hug(o, nq, 0.005 + THICK[style])   # elastic cuff on the wrist: the hand stood in a 2 cm open tube
        else:                                                             # hem falls straight, then the cinched band
            steps = ([(drop * 0.5, 0.006), (drop * 0.5, 0.002)] if drop > 0.01 else []) + [(0.016, -0.006)]
            extrude_loop(o, edges, steps, Vector((0, B["pelvis"].y, 0)), (0, 0, -1), mat_index=2 if style == "fleece" else None)
    push_out(o, bvh_of(LAYERS), 0.006, only=lambda i, c: i >= n0)
    if collar_first is not None: blend_weights_up(o, collar_first, BODY_TOP - 0.03, BODY_TOP + 0.03)
    spikes(o, "final")
    sleeve = sleeve_field(o)
    if style == "down":
        ao = [0.5 + 0.5 * puff(v.co, sleeve[v.index]) ** 0.7 if v.index < n0 else 0.8 for v in me.vertices]
    else:
        ao = [1.0 - 0.35 * gaussian(v.co.z, Z_CHEST - 0.015, 0.006) * (1 - sleeve[v.index]) if v.index < n0 else 0.85 for v in me.vertices]
    set_vcol(o, ao); uv_metres(o)
    cyb = B["spine_03"].y; seams = []
    if style == "down":
        for k in range(int(hem / per_t) - 1, int(BODY_TOP / per_t) + 2):      # quilting through every baffle seam
            seams.append(lambda c, z0=k * per_t: (c.z - z0, around(c, cyb)) if abs(c.x) < 0.2 else None)
        for sd in ("l", "r"):
            ch = arm_chain(sd); t = per_s; segs = [(a, b, (b - a).length) for a, b in zip(ch, ch[1:])]
            total = sum(L for _, _, L in segs)
            while t < total - 0.02:
                acc = 0.0
                for a, b, L in segs:
                    if acc + L >= t:
                        pnt = a + (b - a) * ((t - acc) / L); dn = (b - a).normalized()
                        seams.append(lambda c, pnt=pnt, dn=dn, sd=sd: ((c - pnt).dot(dn), math.atan2((c - pnt).dot(Vector((0, 0, 1))), (c - pnt).dot(dn.cross(Vector((0, 0, 1))).normalized())) * 0.05) if c.x * SIDE[sd] > 0.2 else None)
                        break
                    acc += L
                t += per_s
        seams.append(lambda c: (c.z - yz, around(c, cyb)) if abs(c.x) < 0.2 else None)          # yoke
    else:
        seams.append(lambda c: (c.z - (Z_CHEST - 0.015), around(c, cyb)) if abs(c.x) < 0.2 else None)   # yoke seam
        seams.append(lambda c: (c.y - cyb, c.z) if 0.10 < abs(c.x) < 0.2 and c.z < Z_CHEST else None)   # side seams
    seams.append(lambda c: (abs(c.x) - 0.014, c.z) if abs(c.x) < 0.03 and is_front(c) and c.z > hem else None)  # beside the zip
    seam_uv(o, seams, edge_inset=0.008)
    finish(o); o["thick"] = THICK[style]
    # (the rolled-hood tube behind the collar is gone: a pipe round the new neck-hugging collar read fake, 2026-11-20)
    PIECES.append(o); LAYERS.append(o)


def smooth_edge(o, it=10):
    """Even out every open edge along itself (the push over brows/ears left the brim ragged, and with a rolled,
    lined edge instead of the old inward lip the raggedness showed)."""
    me = o.data
    for edges, vs in boundary_loops(o):
        order = loop_order(me, vs); n = len(order)
        if n < 6: continue
        for _ in range(it):
            P = [me.vertices[i].co.copy() for i in order]
            for k, i in enumerate(order): me.vertices[i].co = P[k - 1] * 0.25 + P[k] * 0.5 + P[(k + 1) % n] * 0.25
    me.update(); laplace(o, 2, 0.4)


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
        o, p1, nrm = head_cap("hat", 0.088, 0.14, 0.009)       # knit hugs the head (a stiffer, wider cap read as a helmet)
        def rib(i, c, n):
            th = math.atan2(c.y - hc.y, c.x - hc.x)
            band = smoothstep(0.05, 0.035, (c - p1).dot(nrm))        # folded brim near the edge
            return 0.0018 * math.sin(th * 56) ** 2 + 0.0045 * band
        inflate(o, rib)
        push_out(o, bvh_of([face]), 0.007)
        smooth_edge(o, 12); push_out(o, bvh_of([face]), 0.006)
        main = slot("hat", "main", "knit", spec["main"])
        assign_mats(o, [main], lambda i, c: 0)
        ao = [0.85 + 0.15 * math.sin(math.atan2(v.co.y - hc.y, v.co.x - hc.x) * 56) ** 2 for v in o.data.vertices]
        set_vcol(o, ao); uv_metres(o); finish(o, 0.004); o["thick"] = 0.006; PIECES.append(o)
        if spec.get("glasses"):
            hb = bvh_of([o]); zc = HEAD_TOP - 0.062; gl = bmesh.new(); rows = []
            fang = -math.pi / 2 if FRONT < 0 else math.pi / 2
            for zz in (zc - 0.021, zc + 0.021):
                row = []
                for k in range(15):
                    a = fang + math.radians(-72 + 144 * k / 14)
                    d = Vector((math.cos(a), math.sin(a), 0.0)); orig = Vector((hc.x, hc.y, zz))
                    hit = hb.ray_cast(orig + d * 0.3, -d, 0.3)
                    pt = (hit[0] if hit[0] is not None else orig + d * 0.11) + d * 0.006
                    row.append(gl.verts.new(pt))
                rows.append(row)
            for k in range(14):
                gl.faces.new((rows[0][k], rows[0][k + 1], rows[1][k + 1], rows[1][k]))
            bmesh.ops.recalc_face_normals(gl, faces=gl.faces[:])
            frame_pts = [tuple(v.co) for v in rows[1]]          # before new_obj frees the bmesh
            lens = new_obj("sunglasses", gl)
            lens.data.materials.append(slot("hat", "lens", "metal", (0.01, 0.012, 0.016)))
            vg(lens, "head").add(range(len(lens.data.vertices)), 1.0, 'REPLACE')
            set_vcol(lens, [1.0] * len(lens.data.vertices)); uv_metres(lens); finish(lens, 0.003); PIECES.append(lens)
            frame = tube_along("glasses_frame", frame_pts, 0.0035, seg=6)
            frame.data.materials.append(slot("hat", "frame", "rubber", (0.015, 0.015, 0.017)))
            for g in GROUPS: frame.vertex_groups.new(name=g)
            vg(frame, "head").add(range(len(frame.data.vertices)), 1.0, 'REPLACE')
            set_vcol(frame, [1.0] * len(frame.data.vertices)); uv_metres(frame); PIECES.append(frame)
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
        smooth_edge(o, 12); push_out(o, bvh_of([face]), 0.010)
        main = slot("scarf", "main", "scarfprint", (1, 1, 1)) if spec.get("print") else slot("scarf", "main", "floral", spec["main"])
        assign_mats(o, [main], lambda i, c: 0)
        set_vcol(o, [1.0 - 0.25 * gaussian((v.co - back).length, 0, 0.05) for v in o.data.vertices]); uv_metres(o); finish(o, 0.003); o["thick"] = 0.0035
        PIECES.append(o)
        knot = sphere("knot", 0.03, loc=back - Vector((0, FRONT * 0.035, 0.02)), seg=12, rings=8, scale=(1.3, 0.8, 1))
        activate(knot); bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
        tails = []
        for k, dx in enumerate((-0.025, 0.03)):
            k0 = back - Vector((0, FRONT * 0.04, 0.03))
            pts = [(k0.x + dx, k0.y - FRONT * 0.004 * j, k0.z - 0.04 * j) for j in range(4)]   # short tails: laid over the collar they read as straps
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
    # torso on the convex outline of its slice + padding (as the jackets): wool hangs straight over the breasts/waist
    # instead of sculpting them (Pema's cardigan read as a skin-tight bodysuit in game, 2026-11-14)
    hull, hcy = torso_hull(Z_PELVIS - 0.08, BODY_TOP - 0.06, fall=0.99, z_fall=Z_CHEST)
    W = smooth_field([(1.0 - min(1.0, sleeve[v.index] * 1.6)) * smoothstep(BODY_TOP - 0.02, BODY_TOP - 0.10, v.co.z) * max(smoothstep(0.75, 0.35, abs(v.normal.z)), smoothstep(Z_CHEST - 0.02, Z_CHEST - 0.10, v.co.z))
                      for v in me.vertices], neighbours(me)[0], 5)
    for v in me.vertices:
        w = W[v.index] * smoothstep(0.29, 0.21, abs(v.co.x))     # A-pose wrists hang at hip height: never pull hands/cuffs to the hull
        if w <= 0.0: continue
        r = hull(v.co)
        if r is None: continue
        d = Vector((v.co.x, v.co.y - hcy, 0.0)); L = d.length
        if L < 1e-4: continue
        target = d * ((r + 0.024 + 0.008 * smoothstep(Z_CHEST - 0.1, Z_PELVIS, v.co.z)) / L)
        v.co.x = v.co.x + (target.x - d.x) * w; v.co.y = v.co.y + (target.y - d.y) * w
    me.update()
    laplace(o, 6, 0.5)
    push_out(o, bvh_of(LAYERS), 0.014)
    laplace(o, 3, 0.5)
    sleeve = sleeve_field(o)
    inflate(o, lambda i, c, n: 0.0035 * math.sin(arm_t(c, side_of(c)) * 2 * math.pi / 0.05) * sleeve[i] * smoothstep(0.25, 0.45, arm_t(c, side_of(c))))
    push_out(o, bvh_of(LAYERS), 0.010)
    main = slot("chuba", "main", "wool", spec["main"])
    if spec.get("top"):                      # modern mix: a dark knit cardigan over the angi; the blouse shows at the collar
        top = slot("chuba", "cardigan", "knit", spec["top"]); blouse = slot("chuba", "blouse", "brocade", spec["blouse"])
        assign_mats(o, [top, blouse], lambda i, c: 0)
        n0 = len(me.vertices)
        for edges, vs in boundary_loops(o):
            c = loop_centre(o, vs)
            if c.z > Z_CHEST:
                # slim standing blouse collar (hongju) whose lining lies on the neck: the old one only hugged above a
                # fixed height, so it lay open on the collarbones as a wide square ring (director 2026-11-20)
                close_neckline(o, vs, 0.008)
                yoke_off_shoulders(o)
                nc = collar_rings(o, edges, vs, 0.034, 0.044, 0.003, 0.008, 1, ease=0.006, rings=5, pad=0.014, ext_mat=0)
                blend_weights_up(o, nc, BODY_TOP - 0.03, BODY_TOP + 0.03)
            elif abs(c.x) > 0.2:             # ribbed knit cuff gathered at the wrist
                sd = side_of(c); d = (B[f"hand_{sd}"] - B[f"lowerarm_{sd}"]).normalized()
                nq = len(me.vertices)
                extrude_loop(o, edges, [(0.008, -0.010), (0.015, -0.001), (0.015, 0.0)], B[f"lowerarm_{sd}"], d)
                hug(o, nq, 0.013)                # ribbed cuff gathered onto the wrist
        push_out(o, bvh_of(LAYERS), 0.006, only=lambda i, c: i >= n0 and c.z < BODY_TOP)
        ao = [1.0 - 0.15 * gaussian(v.co.z, Z_PELVIS - 0.02, 0.02) if v.index < n0 else 0.9 for v in me.vertices]
        set_vcol(o, ao); uv_metres(o); seam_uv(o, [], edge_inset=0.007); finish(o, 0.005); o["thick"] = 0.008; PIECES.append(o); LAYERS.append(o)
        return chuba_skirt(spec, main, top_skirt, hem_z)
    trim = slot("chuba", "trim", "brocade", spec["trim"])
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
    set_vcol(o, ao); uv_metres(o); seam_uv(o, [], edge_inset=0.007); finish(o, 0.005); o["thick"] = 0.008; PIECES.append(o); LAYERS.append(o)
    return chuba_skirt(spec, main, top_skirt, hem_z)


def chuba_skirt(spec, main, top_skirt, hem_z):
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
            # gravity folds on the sides (the aprons cover front and back): soft flutes growing towards the hem
            fth = -math.pi / 2 if FRONT < 0 else math.pi / 2
            af = abs((th - fth + math.pi) % (2 * math.pi) - math.pi); ab = math.pi - af
            ws = smoothstep(math.radians(36), math.radians(50), af) * smoothstep(math.radians(48), math.radians(62), ab)
            r += 0.009 * f ** 0.8 * ws * math.sin(th * 11 + 0.7 * math.sin(z * 7 + th * 3))
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
    uv_metres(sk, cyl_uv, 2 * math.pi * 0.2); finish(sk, 0.006); sk["thick"] = 0.007; PIECES.append(sk)

    # ---- sash (kara) around the waist
    bm = bmesh.new(); grid = []
    for k, z in enumerate((top_skirt - 0.01, top_skirt + 0.03, top_skirt + 0.07)):
        k2 = min(range(NZ), key=lambda q: abs(rings[q][0] - top_skirt)); row = []
        for j in range(NT):
            th = 2 * math.pi * j / NT; r = rs[k2][j] + 0.022 + 0.004 * (k == 1)
            row.append(bm.verts.new((r * math.cos(th), cy + r * math.sin(th), z)))
        grid.append(row)
    for k in range(2):
        for j in range(NT):
            bm.faces.new((grid[k][j], grid[k][(j + 1) % NT], grid[k + 1][(j + 1) % NT], grid[k + 1][j]))
    outward(bm, cy)
    sa = new_obj("sash", bm); sa.data.materials.append(slot("chuba", "sash", "linen", spec["sash"]))
    for p in sa.data.polygons: p.use_smooth = True
    vg(sa, "pelvis").add(range(len(sa.data.vertices)), 1.0, 'REPLACE')
    # the sash wraps OVER the robe/cardigan: never inside it (it showed only as two pads at the sides, 2026-11-14)
    robe = [l for l in LAYERS if l.name.startswith("chuba")]
    if robe:
        wrap_over(sa, robe, 0.007, cy); laplace(sa, 2, 0.5); wrap_over(sa, robe, 0.007, cy)
    set_vcol(sa, [0.9] * len(sa.data.vertices)); uv_metres(sa, cyl_uv, 2 * math.pi * 0.2); finish(sa, 0.004); sa["thick"] = 0.005; PIECES.append(sa)
    sash_tree = BVHTree.FromObject(sa, bpy.context.evaluated_depsgraph_get())
    if spec.get("top"):                      # kyetig: the embossed silver buckle holding the aprons, front of the sash
        fth = -math.pi / 2 if FRONT < 0 else math.pi / 2
        k2 = min(range(NZ), key=lambda q: abs(rings[q][0] - top_skirt)); jj = int(round((fth % (2 * math.pi)) / (2 * math.pi) * NT)) % NT
        hit = sash_tree.ray_cast(Vector((0.0, cy + FRONT * 0.6, top_skirt + 0.03)), Vector((0.0, -FRONT, 0.0)))
        r = (abs(hit[0].y - cy) + 0.012) if hit[0] is not None else rs[k2][jj] + 0.05     # on the sash's front face
        bk = bmesh.new(); bmesh.ops.create_cube(bk, size=1.0)
        bmesh.ops.scale(bk, vec=Vector((0.065, 0.012, 0.045)), verts=bk.verts)
        bmesh.ops.translate(bk, vec=Vector((r * math.cos(fth), cy + r * math.sin(fth), top_skirt + 0.03)), verts=bk.verts)
        bu = new_obj("buckle", bk); bu.data.materials.append(slot("chuba", "buckle", "metal", (0.62, 0.62, 0.64)))
        vg(bu, "pelvis").add(range(len(bu.data.vertices)), 1.0, 'REPLACE')
        set_vcol(bu, [1.0] * len(bu.data.vertices)); uv_metres(bu); PIECES.append(bu)
    return sk, rings, rs, cy, NT


def make_apron(sk_info, back=False):
    """Pangden: the striped apron of married women, over the front of the skirt (texture made by make_pangden).
    back=True: the gyabtil, the striped back apron worn by married and unmarried women alike."""
    sk, rings, rs, cy, NT = sk_info
    NZ = len(rings); half = math.radians(50 if back else 38)
    front_th = (-math.pi / 2 if FRONT < 0 else math.pi / 2) + (math.pi if back else 0.0)
    k0 = 1; k1 = NZ - 3
    bm = bmesh.new(); grid = []
    for k in range(k0, k1 + 1):
        z = rings[k][0]; f = k / (NZ - 1); row = []
        for j in range(15):
            th = front_th - half + 2 * half * j / 14
            jj = int(round((th % (2 * math.pi)) / (2 * math.pi) * NT)) % NT
            r = rs[k][jj] + 0.022 + 0.05 * f ** 1.3 + 0.014      # clear of the back pleats (9 mm) + its own lining
            row.append(bm.verts.new((r * math.cos(th), cy + r * math.sin(th), z)))
        grid.append(row)
    for k in range(len(grid) - 1):
        for j in range(14):
            bm.faces.new((grid[k][j], grid[k][j + 1], grid[k + 1][j + 1], grid[k + 1][j]))
    outward(bm, cy)
    ap = new_obj("apron_back" if back else "apron", bm)
    ap.data.materials.append(slot("apron", "back" if back else "main", "pangden", (0.8, 0.8, 0.8) if back else (1, 1, 1)))
    for p in ap.data.polygons: p.use_smooth = True
    z_top, z_bot = rings[k0][0], rings[k1][0]
    for v in ap.data.vertices:
        s = smoothstep(z_top, z_top - 0.3, v.co.z); side = max(0.0, min(1.0, 0.5 + v.co.x / 0.16))
        vg(ap, "pelvis").add([v.index], 1 - 0.7 * s, 'REPLACE')
        if s > 0:
            vg(ap, "thigh_l").add([v.index], 0.7 * s * side, 'REPLACE')
            vg(ap, "thigh_r").add([v.index], 0.7 * s * (1 - side), 'REPLACE')
    xs = [v.co.x for v in ap.data.vertices]; x0, x1 = min(xs), max(xs)
    uv_metres(ap, lambda c: ((c.x - x0) / (x1 - x0), (c.z - z_bot) / (z_top - z_bot) * 0.8 + (0.37 if back else 0.0)))
    seam_uv(ap, [lambda c, j=j: (c.x - (x0 + (x1 - x0) * j / 3.0), c.z) for j in (1, 2)], edge_inset=0.006)   # 3 woven panels
    robe = [l for l in LAYERS if l.name.startswith("chuba")]      # over the cardigan's hem (it poked through as notches)
    if robe: wrap_over(ap, robe, 0.006, cy)
    set_vcol(ap, [1.0] * len(ap.data.vertices)); finish(ap, 0.003); ap["thick"] = 0.004; PIECES.append(ap)


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


def make_scarf_print(path):
    """Printed cotton head scarf (Sherpa/Tibetan women): small cream five-petal flowers with yellow centres and green
    leaves scattered on deep red; one 256 px tile = 12 cm of cloth. A plain floral jacquard read as a knit cap in game."""
    import random
    random.seed(11)
    W = 256; C = 64
    base = (0.50, 0.07, 0.14); petal = (0.93, 0.88, 0.76); mid = (0.95, 0.72, 0.12); leaf = (0.16, 0.40, 0.18)
    cells = {(i, j): (random.uniform(-10, 10), random.uniform(-10, 10), random.uniform(0, 1.2), random.random() < 0.5)
             for i in range(W // C) for j in range(W // C)}
    img = bpy.data.images.new("T_ScarfPrint", W, W, alpha=False)
    px = [0.0] * (W * W * 4)
    for y in range(W):
        for x in range(W):
            i, j = x // C, y // C; ox, oy, rot, small = cells[(i, j)]
            cx = i * C + C / 2 + ox; cy = j * C + C / 2 + oy
            dx = x - cx; dy = y - cy; r = math.hypot(dx, dy); a = math.atan2(dy, dx) - rot
            k = 0.7 if small else 1.0
            col = base
            for sgn in (1, -1):                      # two leaves
                lx = dx * math.cos(rot + sgn * 2.4) + dy * math.sin(rot + sgn * 2.4); ly = -dx * math.sin(rot + sgn * 2.4) + dy * math.cos(rot + sgn * 2.4)
                if ((lx - 15 * k) / (9 * k)) ** 2 + (ly / (4 * k)) ** 2 < 1: col = leaf
            pr = 9 * k * (0.65 + 0.35 * abs(math.cos(2.5 * a)))      # five petals
            if r < pr + 2.5 * k: col = petal
            if r < 3.5 * k: col = mid
            n = 0.92 + 0.08 * (((x * 7 + y * 13) % 5) / 4)          # weave
            q = (y * W + x) * 4; px[q:q + 4] = [col[0] * n, col[1] * n, col[2] * n, 1.0]
    img.pixels = px
    img.filepath_raw = path; img.file_format = 'PNG'; img.save()


# ---------------------------------------------------------------- build
bt = make_scan_boots(SPEC["boots"][1]) if SPEC["boots"][0] == "scan" else make_boots(*SPEC["boots"])
waist = make_pants(SPEC["pants"], bt) if "pants" in SPEC else Z_PELVIS + 0.115
if "jacket" in SPEC: make_jacket(SPEC["jacket"], waist)
if "chuba" in SPEC:
    info = make_chuba(SPEC["chuba"])
    if SPEC.get("apron"): make_apron(info)
    if SPEC.get("back_apron"): make_apron(info, back=True)
if "hat" in SPEC: make_hat(SPEC["hat"])

os.makedirs(DEST, exist_ok=True)
if any(s["fabric"] == "@pangden" for s in SLOTS.values()):
    try: make_pangden(os.path.join(DEST, "T_Pangden.png"))      # same seed = same image; the PNG save fails now and
    except RuntimeError as e: print("PANGDEN keep existing:", e)  # then ("internal error", a file lock) -> keep the old one
if any(s["fabric"] == "@scarfprint" for s in SLOTS.values()):
    for _try in range(3):
        try: make_scarf_print(os.path.join(DEST, "T_ScarfPrint.png")); break
        except RuntimeError as e: print("SCARFPRINT retry:", e)

for o in PIECES:
    zs = [v.co.z for v in o.data.vertices]; xs = [v.co.x for v in o.data.vertices]
    print("PIECE", o.name, "tris", sum(len(p.vertices) - 2 for p in o.data.polygons), "z", round(min(zs), 3), round(max(zs), 3), "x", round(min(xs), 3), round(max(xs), 3))
# ONE uv channel per piece, all named alike: join merges layers by NAME, and UE reads only channel 0 - pieces whose
# UVs sat in "UV0" (planar: apron, skirt) or "DiffuseUV" (body copies) got all-zero UVs there and showed a single
# texel (the pangden read as a flat orange slab, knit/wool looked untextured; found in game 2026-11-14)
for o in PIECES:
    me = o.data; L = me.uv_layers
    main_l = (L.get("UV0") or (L.active if L.active and L.active.name != "UVSeam" else None) or next((l for l in L if l.name != "UVSeam"), None))
    uv0 = [tuple(d.uv) for d in main_l.data] if main_l else [(0.0, 0.0)] * len(me.loops)
    uv1 = [tuple(d.uv) for d in L["UVSeam"].data] if L.get("UVSeam") else [(0.0, SEAM_FAR)] * len(me.loops)
    for nm in [l.name for l in L]: L.remove(L[nm])
    a = L.new(name="UVMap"); b = L.new(name="UVSeam")
    for k in range(len(me.loops)): a.data[k].uv = uv0[k]; b.data[k].uv = uv1[k]
    L.active = a; a.active_render = True
for o in PIECES:
    if "thick" in o and not os.environ.get("YGG_NOTHICK"):
        nv = len(o.data.vertices); thicken(o, o["thick"])
        print("THICK", o.name, o["thick"], nv, "->", len(o.data.vertices))
g = join(f"SKM_G_{OUTFIT}{TAG}", PIECES)
activate(g)
print("UVLAYERS", [l.name for l in g.data.uv_layers])
bpy.ops.object.vertex_group_limit_total(group_select_mode='ALL', limit=6)
bpy.ops.object.vertex_group_normalize_all(lock_active=False)
_unw = sum(1 for v in g.data.vertices if sum(x.weight for x in v.groups) < 0.5)
print("WEIGHTS verts", len(g.data.vertices), "unweighted", _unw)
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
NECK_T = Vector((B["neck_01"].x, B["neck_01"].y, BODY_TOP + 0.02))
VIEWS = (("front", Vector((0, FRONT, 0.05))), ("threeq", Vector((0.75, FRONT, 0.15))), ("back", Vector((-0.4, -FRONT, 0.1))),
         ("feet", Vector((0.8, FRONT, 0.35))),
         # collar close-ups (director: no gap at the neck from any angle): front-high, side, back-high
         ("neck_front", Vector((0.15, FRONT, 0.45))), ("neck_side", Vector((1.0, 0.15 * FRONT, 0.25))), ("neck_back", Vector((-0.3, -FRONT, 0.55))),
         # cloth edges (director: paper-thin sheets): the left cuff from below-front, so the opening's lining shows
         ("edge_cuff", Vector((0.5, FRONT, -0.55))))
CUFF_T = B["hand_l"] + (B["lowerarm_l"] - B["hand_l"]).normalized() * 0.03
for nm, d in VIEWS:
    tgt = NECK_T if nm.startswith("neck") else CUFF_T if nm == "edge_cuff" else (Vector((0, 0, HEAD_TOP * 0.52)) if nm != "feet" else Vector((0, B["ball_l"].y, 0.1)))
    cam.location = tgt + d.normalized() * (0.75 if nm.startswith("neck") else 0.45 if nm == "edge_cuff" else (4.4 if nm != "feet" else 1.3))
    cam.rotation_euler = (tgt - cam.location).to_track_quat('-Z', 'Y').to_euler()
    s.render.filepath = os.path.join(OUT, "Preview", f"garment_{OUTFIT}{TAG}_{nm}.png")
    bpy.ops.render.render(write_still=True)
save_blend(f"garment_{OUTFIT}{TAG}")
print("DONE", OUTFIT)
