# The family's Sherpa house for Chomolungma chapter 1, after the user's reference photos (2026-11-14): a long dry-stone
# house - grey stones with lichen, laid in courses with interlocking corners - blue-painted door and window frames, a low
# roof of weathered boards with a few tin sheets and stones on it; inside, a smoke-darkened mud-plastered kitchen with a
# mud stove (chulo) under a hood, wooden butter-tea churns (dongmo), a firewood stack and woven Tibetan rugs.
#   blender -b --factory-startup --python Tools/Blender/sherpa_house.py
# House frame = the world builder's room frame (Tools/Story/build_chapter1_world): origin at the floor centre, +X = the
# door wall, +Y = the window wall, metres. Output: Art/Blender/Export/House/SM_House.fbx (one mesh per part, absolute
# positions), SM_HouseProps.fbx (props at their own origin), T_TibetanRug.png, slots.json; previews.
import sys, os, math, random, json
sys.path.append(os.path.dirname(os.path.abspath(__file__)))
from ygg_blend import *
import bmesh
from mathutils import Vector, Matrix, Euler

random.seed(1978)
L, W, H, T = 6.0, 4.6, 2.4, 0.4               # outer length / width, room height, wall thickness (= builder's L_, W_, H_, T_)
PITCH = math.radians(18.0); OV = 0.45; OVG = 0.35
EAVE_TOP = H + 0.12                          # top of the long walls (outer face)
ZE = EAVE_TOP - OV * math.tan(PITCH)         # roof underside height at the eave edge
DOOR = (-0.5, 0.5, 0.0, 1.9)                 # front wall (x=+L/2): y0, y1, z0, z1
WIN = (0.35, 1.25, 1.0, 1.7)                 # window wall (y=+W/2): x0, x1, z0, z1
FOUND = -1.6                                 # stones go this far below the floor (the plinth is buried in snow)
DEST = os.path.join(OUT, "Export", "House")
os.makedirs(DEST, exist_ok=True)
SLOTS = {}


def slot(name, fabric, tint, tile, rough=0.8, metal=0.0, emissive=0.0):
    if name not in SLOTS:
        SLOTS[name] = dict(fabric=fabric, tint=list(tint), tile_m=tile, rough=rough, metal=metal, emissive=emissive)   # tint = LINEAR albedo
        mat(name, tint, metal=metal, rough=rough)
    return bpy.data.materials[name]


M = dict(stone=slot("house_stone", "lichen_rock", (0.15, 0.145, 0.135), 0.5, 1.0),
         core=slot("house_core", "clay_plaster", (0.07, 0.06, 0.05), 1.2, 1.0),
         roof=slot("house_roof", "weathered_planks", (0.22, 0.21, 0.2), 1.6, 0.9),
         deck=slot("house_deck", "rough_wood", (0.06, 0.05, 0.045), 1.2, 0.9),
         iron=slot("house_iron", "worn_corrugated_iron", (0.4, 0.4, 0.42), 1.2, 0.55, 0.6),
         blue=slot("house_blue", "blue_painted_planks", (0.03, 0.11, 0.30), 0.9, 0.8),       # weathered blue (0.42 glowed)
         glass=slot("house_glass", "", (0.02, 0.025, 0.03), 1.0, 0.08),
         beam=slot("house_beam", "rough_wood", (0.06, 0.045, 0.035), 1.0, 0.85),
         plaster=slot("house_plaster", "clay_plaster", (0.42, 0.29, 0.17), 1.4, 0.92),
         floor=slot("house_floor", "old_wooden_floor_02", (0.12, 0.09, 0.065), 2.0, 0.75),
         clay=slot("house_clay", "clay_plaster", (0.15, 0.12, 0.095), 0.9, 0.95),     # mud plaster (0.42 read white in game)
         soot=slot("house_soot", "rusty_metal_02", (0.03, 0.028, 0.026), 0.8, 0.6, 0.4),
         bark=slot("house_bark", "bark_brown_02", (0.1, 0.08, 0.06), 0.5, 0.95),
         wood=slot("house_wood", "rough_wood", (0.18, 0.12, 0.07), 0.6, 0.8),
         brass=slot("house_brass", "", (0.75, 0.52, 0.22), 1.0, 0.35, 1.0),
         copper=slot("house_copper", "", (0.75, 0.33, 0.18), 1.0, 0.4, 1.0),
         ember=slot("house_ember", "", (1.0, 0.35, 0.08), 1.0, 0.9, 0.0, 12.0),
         rug=slot("house_rug", "@rug", (1, 1, 1), 1.0, 0.95))


# ---------------------------------------------------------------- bmesh building blocks
class Part:
    """One mesh being built: bmesh with UV (metres) + colour (R = brightness/AO) + material slots."""
    def __init__(self, name):
        self.name = name; self.bm = bmesh.new(); self.mats = []
        self.uv = self.bm.loops.layers.uv.verify(); self.col = self.bm.loops.layers.color.new("Col")

    def mi(self, m):
        if m not in self.mats: self.mats.append(m)
        return self.mats.index(m)

    def box(self, centre, size, m, rot=None, bevel=0.0, jitter=0.0, uvoff=(0, 0), shade=1.0, uvscale=1.0):
        g = bmesh.ops.create_cube(self.bm, size=1.0)
        vs = g["verts"]
        for v in vs: v.co = Vector((v.co.x * size[0], v.co.y * size[1], v.co.z * size[2]))
        seed = next(iter(vs[0].link_faces))
        if bevel > 0:
            edges = list({e for v in vs for e in v.link_edges})
            r = bmesh.ops.bevel(self.bm, geom=edges + vs, offset=min(bevel, min(size) * 0.3), segments=1, profile=0.5,
                                affect='EDGES', clamp_overlap=True)
            seed = next((f for f in r.get("faces", []) if f.is_valid), seed)
        # the bevel REPLACES the six main faces with new ones, and BMesh reuses freed slots, so neither the old face list
        # nor creation order finds them (flat stones, plaster beams, floating boards - 2026-11-14): the box is a closed
        # island, so walk it from one of its faces
        faces, stack, seen = [], [seed], set()
        while stack:
            f = stack.pop()
            if f in seen: continue
            seen.add(f); faces.append(f)
            for e in f.edges:
                for h in e.link_faces:
                    if h not in seen: stack.append(h)
        vs = list({v for f in faces for v in f.verts})
        if jitter > 0:
            for v in vs: v.co += Vector((random.uniform(-1, 1), random.uniform(-1, 1), random.uniform(-1, 1))) * jitter
        R = (rot or Matrix.Identity(3)).to_4x4()
        bmesh.ops.transform(self.bm, matrix=Matrix.Translation(Vector(centre)) @ R, verts=vs)
        self._finish(faces, m, uvoff, shade, uvscale)
        return faces

    def _finish(self, faces, m, uvoff, shade, uvscale):
        idx = self.mi(m)
        for f in faces:
            f.material_index = idx; f.smooth = False
            f.normal_update(); n = f.normal; ax = max(range(3), key=lambda i: abs(n[i]))
            for lp in f.loops:
                c = lp.vert.co
                u, v = ((c.y, c.z), (c.x, c.z), (c.x, c.y))[ax]
                lp[self.uv].uv = (u * uvscale + uvoff[0], v * uvscale + uvoff[1])
                lp[self.col] = (shade, shade, shade, 1.0)

    def quad(self, pts, m, shade=1.0, uvoff=(0, 0), flip=False):
        vs = [self.bm.verts.new(p) for p in pts]
        f = self.bm.faces.new(vs[::-1] if flip else vs)
        self._finish([f], m, uvoff, shade, 1.0)
        return f

    def cylinder(self, centre, r, depth, m, axis='Z', seg=12, r2=None, shade=1.0, uvoff=(0, 0), caps=True):
        g = bmesh.ops.create_cone(self.bm, cap_ends=caps, segments=seg, radius1=r, radius2=r if r2 is None else r2, depth=depth)
        vs = g["verts"]; faces = list({f for v in vs for f in v.link_faces})
        R = {'Z': Matrix.Identity(4), 'X': Matrix.Rotation(math.pi / 2, 4, 'Y'), 'Y': Matrix.Rotation(math.pi / 2, 4, 'X')}[axis]
        bmesh.ops.transform(self.bm, matrix=Matrix.Translation(Vector(centre)) @ R, verts=vs)
        self._finish(faces, m, uvoff, shade, 1.0)
        for f in faces: f.smooth = len(f.verts) == 4
        return faces

    def build(self):
        me = bpy.data.meshes.new(self.name); self.bm.normal_update(); self.bm.to_mesh(me); self.bm.free()
        for m in self.mats: me.materials.append(m)
        o = bpy.data.objects.new(self.name, me); bpy.context.collection.objects.link(o)
        for p in me.polygons:
            if p.use_smooth: pass
        return o


# ---------------------------------------------------------------- dry-stone walls
def course_heights(z0, z1):
    zs, z = [z0], z0
    while z < z1:
        z += random.choice((0.11, 0.13, 0.15, 0.17, 0.2, 0.23, 0.14, 0.16)); zs.append(min(z, z1 + 0.25))
    return zs


COURSES = course_heights(FOUND, H + 1.1)


def gable_top(y):
    """Roof underside above |y| (the gable walls rise to it)."""
    return ZE + (W / 2 + OV - abs(y)) * math.tan(PITCH)


# walls: origin of u, u direction, outward normal, u length, openings (u0, u1, z0, z1), gable?
WALLS = [
    ("front", Vector((L / 2, -W / 2, 0)), Vector((0, 1, 0)), Vector((1, 0, 0)), W, [(DOOR[0] + W / 2, DOOR[1] + W / 2, DOOR[2] - 0.5, DOOR[3])], True),
    ("back", Vector((-L / 2, W / 2, 0)), Vector((0, -1, 0)), Vector((-1, 0, 0)), W, [], True),
    ("left", Vector((-L / 2, -W / 2, 0)), Vector((1, 0, 0)), Vector((0, -1, 0)), L, [], False),
    ("right", Vector((L / 2, W / 2, 0)), Vector((-1, 0, 0)), Vector((0, 1, 0)), L, [(L / 2 - WIN[1], L / 2 - WIN[0], WIN[2], WIN[3])], False),
]
LINTEL = 0.16; LINTEL_EXT = 0.25
stones = Part("SM_House_Stones"); core = Part("SM_House_Core"); timber = Part("SM_House_Timber")
nstones = 0
for k in range(len(COURSES) - 1):
    z0, z1 = COURSES[k], COURSES[k + 1]
    for name, o, u, n, length, openings, gable in WALLS:
        long_wall = length > 5
        ext = (k % 2 == 0) == long_wall           # alternate which wall covers the corner: the corners interlock
        D = 0.2
        ua, ub = (0.0, length) if ext else (D, length - D)     # the covering wall ends flush with the corner
        top_cap = None if gable else EAVE_TOP
        if top_cap is not None and z0 >= top_cap: continue
        if gable and z0 >= gable_top(0) - 0.04: continue
        # blocked intervals in this course: openings and their lintels
        blocked = []
        for (a, b, oz0, oz1) in openings:
            if z1 > oz0 and z0 < oz1 + LINTEL: blocked.append((a - (LINTEL_EXT if z1 > oz1 else 0.0), b + (LINTEL_EXT if z1 > oz1 else 0.0)))
        uu = ua + random.uniform(0, 0.25) * (k % 3 == 1)
        while uu < ub - 0.05:
            ln = random.choice((0.16, 0.2, 0.26, 0.32, 0.38, 0.45, 0.52, 0.6, 0.7, 0.3, 0.35))
            a, b = uu, min(uu + ln, ub)
            for (ba, bb) in blocked:                 # clip against openings
                if a < bb and b > ba:
                    if a >= ba: a = bb
                    else: b = ba
            if b - a < 0.08: uu = max(uu + ln, a + 0.02); continue
            gap = random.uniform(0.008, 0.022)
            hz0, hz1 = z0 + random.uniform(0.004, 0.012), z1 - random.uniform(0.004, 0.012)
            if top_cap is not None: hz1 = min(hz1, top_cap)
            if gable:
                yl = (o + u * a).y; yr = (o + u * b).y
                cap = min(gable_top(yl), gable_top(yr)) - 0.015
                if z0 > EAVE_TOP - 0.05:
                    hz1 = min(hz1, cap)
            if hz1 - hz0 < 0.05: uu = b + gap; continue
            mid = (a + b) / 2; out = random.uniform(-0.01, 0.045)
            dz = random.uniform(-0.015, 0.015); hs = random.uniform(0.82, 1.12)
            zc = (hz0 + hz1) / 2 + dz; hh = (hz1 - hz0) * hs
            if top_cap is not None: hh = min(hh, 2 * (top_cap - zc))
            if gable and z0 > EAVE_TOP - 0.05: hh = min(hh, 2 * (cap - zc))
            if hh < 0.04: uu = b + gap; continue
            c = o + u * mid + n * (out - D / 2) + Vector((0, 0, zc))
            sx = b - a - gap; sz = hh
            # stone box axes: u (length), n (depth), z
            Rm = Matrix((n, u, (0, 0, 1))).transposed()        # right-handed: (u, n, z) mirrored every stone inside out
            tilt = Euler((random.uniform(-0.08, 0.08), random.uniform(-0.04, 0.04), random.uniform(-0.05, 0.05))).to_matrix()
            shade = random.choice((0.62, 0.7, 0.78, 0.85, 0.92, 1.0, 0.88, 0.95))
            stones.box(c, (D, sx, sz), M["stone"], Rm @ tilt, bevel=random.uniform(0.018, 0.04), jitter=0.02,
                       uvoff=(random.random() * 4, random.random() * 4), shade=shade)
            nstones += 1
            uu = b + gap
print("STONES", nstones)

# dark mud core behind the stones (gaps read as depth, never as light)
for name, o, u, n, length, openings, gable in WALLS:
    zt = EAVE_TOP if not gable else gable_top(0)
    segs = [(0.0, length)]
    for (a, b, oz0, oz1) in openings:
        segs = [s for s in segs]
    pts = []
    inset = -0.07
    def P(uv, z): return o + u * uv + n * inset + Vector((0, 0, z))
    if not gable:
        cuts = sorted(openings)
        u_prev = 0.0
        for (a, b, oz0, oz1) in cuts:
            core.quad([P(u_prev, FOUND), P(a, FOUND), P(a, zt), P(u_prev, zt)], M["core"], 0.5)
            core.quad([P(a, FOUND), P(b, FOUND), P(b, max(oz0, FOUND)), P(a, max(oz0, FOUND))], M["core"], 0.5)
            core.quad([P(a, oz1), P(b, oz1), P(b, zt), P(a, zt)], M["core"], 0.5)
            u_prev = b
        core.quad([P(u_prev, FOUND), P(length, FOUND), P(length, zt), P(u_prev, zt)], M["core"], 0.5)
    else:
        a, b = (openings[0][0], openings[0][1]) if openings else (length / 2, length / 2)
        for (s0, s1) in ((0.0, a), (b, length)):
            if s1 - s0 < 0.01: continue
            core.quad([P(s0, FOUND), P(s1, FOUND), P(s1, EAVE_TOP), P(s0, EAVE_TOP)], M["core"], 0.5)
        if openings:
            core.quad([P(a, openings[0][3]), P(b, openings[0][3]), P(b, EAVE_TOP), P(a, EAVE_TOP)], M["core"], 0.5)
        # gable triangle up to the roof
        core.quad([P(0, EAVE_TOP), P(length, EAVE_TOP), P(length / 2, gable_top(0))], M["core"], 0.5)

# lintels over the door and the window (rough timber, set into the stones)
timber.box((L / 2 - T / 2, 0, DOOR[3] + LINTEL / 2), (T + 0.04, (DOOR[1] - DOOR[0]) + 2 * LINTEL_EXT, LINTEL), M["beam"], bevel=0.01, jitter=0.006)
timber.box(((WIN[0] + WIN[1]) / 2, W / 2 - T / 2, WIN[3] + LINTEL / 2), ((WIN[1] - WIN[0]) + 2 * LINTEL_EXT, T + 0.04, LINTEL), M["beam"], bevel=0.01, jitter=0.006)
# blue frames: door (full-depth jambs + head), window (frame, cross bars, glass), plank door leaf open inwards
frames = Part("SM_House_Frames")
fw = 0.09
for y in (DOOR[0] + fw / 2, DOOR[1] - fw / 2):
    frames.box((L / 2 - T / 2, y, DOOR[3] / 2), (T + 0.02, fw, DOOR[3]), M["blue"], bevel=0.008)
frames.box((L / 2 - T / 2, 0, DOOR[3] - fw / 2), (T + 0.02, DOOR[1] - DOOR[0], fw), M["blue"], bevel=0.008)
frames.box((L / 2 - T / 2, 0, 0.02), (T + 0.06, DOOR[1] - DOOR[0] + 0.1, 0.05), M["beam"], bevel=0.01)          # threshold
hinge = Vector((L / 2 - T + 0.04, DOOR[0] + fw, 0))
ang = math.radians(105)
for i in range(5):                                   # door leaf: five planks, opened inwards
    w_ = (DOOR[1] - DOOR[0] - 2 * fw) / 5
    local = Vector((-0.02, w_ * (i + 0.5), (DOOR[3] - fw) / 2 + 0.01))
    Rz = Matrix.Rotation(ang, 3, 'Z')
    frames.box(hinge + Rz @ local, (0.04, w_ - 0.006, DOOR[3] - fw - 0.03), M["blue"], Rz, bevel=0.004, uvoff=(i * 0.3, 0))
for zz in (0.35, 1.45):
    Rz = Matrix.Rotation(ang, 3, 'Z')
    frames.box(hinge + Rz @ Vector((-0.05, (DOOR[1] - DOOR[0] - 2 * fw) / 2, zz)), (0.03, DOOR[1] - DOOR[0] - 2 * fw - 0.06, 0.1), M["blue"], Rz, bevel=0.004)
x0, x1, wz0, wz1 = WIN; yy = W / 2 - T / 2
for xx in (x0 + fw / 2, x1 - fw / 2):
    frames.box((xx, yy, (wz0 + wz1) / 2), (fw, T + 0.02, wz1 - wz0), M["blue"], bevel=0.008)
for zz in (wz0 + fw / 2, wz1 - fw / 2):
    frames.box(((x0 + x1) / 2, yy, zz), (x1 - x0, T + 0.02, fw), M["blue"], bevel=0.008)
frames.box(((x0 + x1) / 2, W / 2 - 0.06, (wz0 + wz1) / 2), (0.035, 0.05, wz1 - wz0 - 2 * fw), M["blue"])
frames.box(((x0 + x1) / 2, W / 2 - 0.06, (wz0 + wz1) / 2), (x1 - x0 - 2 * fw, 0.05, 0.035), M["blue"])
frames.box(((x0 + x1) / 2, W / 2 - 0.08, (wz0 + wz1) / 2), (x1 - x0 - 2 * fw, 0.01, wz1 - wz0 - 2 * fw), M["glass"])
frames.box(((x0 + x1) / 2, W / 2 + 0.03, wz0 - 0.03), (x1 - x0 + 0.16, 0.12, 0.05), M["beam"], bevel=0.01)          # sill

# ---------------------------------------------------------------- roof: deck, weathered boards, tin sheets, stones
roof = Part("SM_House_Roof")
xa, xb = -L / 2 - OVG, L / 2 + OVG
def roof_z(y): return ZE + (W / 2 + OV - abs(y)) * math.tan(PITCH)
for sgn in (-1, 1):
    slope_len = (W / 2 + OV) / math.cos(PITCH)
    Rx = Matrix.Rotation(-sgn * PITCH, 3, 'X')
    mid_y = sgn * (W / 2 + OV) / 2
    roof.box((0, mid_y, roof_z(mid_y) + 0.0), (xb - xa, slope_len + 0.02, 0.03), M["deck"], Rx)            # deck
    x = xa
    while x < xb - 0.05:
        bw = random.uniform(0.16, 0.27); w_ = min(bw, xb - x)
        lift = random.uniform(0.0, 0.012); extra = random.uniform(-0.04, 0.06)
        blen = slope_len + 0.06 + extra
        cy = sgn * ((W / 2 + OV) / 2 + extra / 2 * math.cos(PITCH))
        tw = Euler((random.uniform(-0.01, 0.01), random.uniform(-0.012, 0.012), random.uniform(-0.015, 0.015))).to_matrix()
        roof.box((x + w_ / 2, cy, roof_z(cy) + 0.03 + lift), (w_ - 0.008, blen, 0.024), M["roof"], Rx @ tw, bevel=0.004,
                 uvoff=(random.random() * 3, random.random() * 3), shade=random.uniform(0.72, 1.0))
        x += w_
    roof.box((0, sgn * 0.09, roof_z(0) + 0.07), (xb - xa + 0.02, 0.2, 0.025), M["roof"], Matrix.Rotation(-sgn * PITCH * 0.5, 3, 'X'), bevel=0.004, shade=0.8)
    # fascia board along the eave
    roof.box((0, sgn * (W / 2 + OV - 0.01), ZE - 0.05), (xb - xa, 0.03, 0.14), M["deck"], bevel=0.005)
# two worn tin sheets on the front half of one slope (like the reference), corrugated
for (sx0, sgn) in ((1.2, -1), (-2.3, 1)):
    sheet = bmesh.new()
for (cx_, sgn, wsh) in ((1.6, -1, 0.95), (2.4, -1, 0.85), (-1.9, 1, 0.95)):
    Rx = Matrix.Rotation(-sgn * PITCH, 3, 'X')
    nx, ny = 16, 6; slope_len = 1.9
    vs = []
    for j in range(ny + 1):
        row = []
        for i in range(nx + 1):
            lx = -wsh / 2 + wsh * i / nx; ly = -slope_len / 2 + slope_len * j / ny
            lz = 0.012 * math.sin(i * math.pi)          # corrugations
            row.append(roof.bm.verts.new(Rx @ Vector((lx, ly, lz)) + Vector((cx_, sgn * (W / 2 + OV) * 0.52, roof_z(sgn * (W / 2 + OV) * 0.52) + 0.065))))
        vs.append(row)
    fs = []
    for j in range(ny):
        for i in range(nx):
            fs.append(roof.bm.faces.new((vs[j][i], vs[j][i + 1], vs[j + 1][i + 1], vs[j + 1][i])))
    roof._finish(fs, M["iron"], (random.random(), 0), 0.9, 1.0)
# stones holding the roof down
rocks = Part("SM_House_RoofStones")
for k in range(16):
    sgn = random.choice((-1, 1)); x = random.uniform(xa + 0.3, xb - 0.3)
    yy = sgn * random.choice((0.25, W / 2 + OV - 0.25, 1.2))
    s = random.uniform(0.14, 0.26)
    rocks.box((x, yy, roof_z(yy) + 0.06 + s * 0.2), (s, s * random.uniform(0.7, 1.1), s * 0.45), M["stone"],
              Euler((random.uniform(-0.2, 0.2), random.uniform(-0.2, 0.2), random.uniform(0, 3))).to_matrix(), bevel=0.04, jitter=0.02,
              uvoff=(random.random() * 4, random.random() * 4), shade=random.uniform(0.6, 0.9))

# ---------------------------------------------------------------- inside: mud plaster, plank ceiling + beams, board floor
inner = Part("SM_House_Interior")
iL, iW = L - 2 * T, W - 2 * T
SX, SY = -0.95, 0.95                                   # the stove (soot above it)
def soot(p):
    """Smoke: walls and ceiling darken upwards and above the stove."""
    up = smoothstep(0.7, H, p.z) * 0.6
    d = math.hypot(p.x - SX, p.y - SY)
    plume = math.exp(-(d / 1.1) ** 2) * smoothstep(1.0, H, p.z) * 0.7
    return max(0.12, 1.0 - up - plume)
def smoothstep(a, b, x):
    t = max(0.0, min(1.0, (x - a) / (b - a))); return t * t * (3 - 2 * t)

def plaster_wall(p0, p1, holes, nseg_u=12, nseg_z=8):
    """Inner wall face from p0 to p1 (floor level), split around holes (u0, u1, z0, z1), vertex soot."""
    u = (p1 - p0); length = u.length; u.normalize()
    cuts_u = sorted({0.0, length} | {h[0] for h in holes} | {h[1] for h in holes})
    cuts_z = sorted({0.0, H} | {h[2] for h in holes} | {h[3] for h in holes})
    def fine(vals, n):
        out = []
        for a, b in zip(vals, vals[1:]):
            k = max(1, int(round((b - a) / (H / n))))
            out += [a + (b - a) * i / k for i in range(k)]
        return out + [vals[-1]]
    us, zs = fine(cuts_u, nseg_u), fine(cuts_z, nseg_z)
    for i in range(len(us) - 1):
        for j in range(len(zs) - 1):
            cu, cz = (us[i] + us[i + 1]) / 2, (zs[j] + zs[j + 1]) / 2
            if any(h[0] < cu < h[1] and h[2] < cz < h[3] for h in holes): continue
            pts = [p0 + u * us[i] + Vector((0, 0, zs[j])), p0 + u * us[i + 1] + Vector((0, 0, zs[j])),
                   p0 + u * us[i + 1] + Vector((0, 0, zs[j + 1])), p0 + u * us[i] + Vector((0, 0, zs[j + 1]))]
            f = inner.quad(pts, M["plaster"], flip=True)       # faces the room
            for lp in f.loops:
                s = soot(lp.vert.co); lp[inner.col] = (s, s, s, 1)
xi, yi = iL / 2, iW / 2
plaster_wall(Vector((xi, -yi, 0)), Vector((xi, yi, 0)), [(DOOR[0] + yi, DOOR[1] + yi, DOOR[2], DOOR[3])])            # front (door)
plaster_wall(Vector((-xi, yi, 0)), Vector((-xi, -yi, 0)), [])                                                      # back (altar)
plaster_wall(Vector((-xi, -yi, 0)), Vector((xi, -yi, 0)), [])                                                      # -y (benches)
plaster_wall(Vector((xi, yi, 0)), Vector((-xi, yi, 0)), [(xi - WIN[1], xi - WIN[0], WIN[2], WIN[3])])              # +y (window)
# reveals of the openings (plaster) - the blue frames sit in them
# ceiling planks + squared beams, board floor
x = -xi
while x < xi - 0.01:
    bw = min(random.uniform(0.17, 0.24), xi - x)
    f = inner.box((x + bw / 2, 0, H + 0.015), (bw - 0.004, iW, 0.03), M["beam"], uvoff=(random.random() * 2, 0), shade=0.7)
    for ff in f:
        for lp in ff.loops:
            s = soot(lp.vert.co) * 0.85; lp[inner.col] = (s, s, s, 1)
    x += bw
for bx in (-1.8, -0.6, 0.6, 1.8):
    f = inner.box((bx, 0, H - 0.09), (0.16, iW + 0.3, 0.18), M["beam"], bevel=0.012, jitter=0.004, uvoff=(bx, 0))
    for ff in f:
        for lp in ff.loops:
            s = soot(lp.vert.co) * 0.8; lp[inner.col] = (s, s, s, 1)
inner.box((0, 0, 0.005), (iL, iW, 0.01), M["floor"])
inner = inner

# ---------------------------------------------------------------- outside: door steps, firewood against the -y wall
yard = Part("SM_House_Yard")
yard.box((L / 2 + 0.32, 0, -0.12), (0.55, 1.3, 0.2), M["stone"], bevel=0.04, jitter=0.012, uvoff=(0.3, 0.7), shade=0.8)
yard.box((L / 2 + 0.78, 0.05, -0.36), (0.6, 1.5, 0.22), M["stone"], bevel=0.04, jitter=0.015, uvoff=(1.3, 0.2), shade=0.75)
def logs(part, centre, length, rows, cols, r=0.055):
    for i in range(rows):
        for j in range(cols - (i % 2)):
            rr = r * random.uniform(0.75, 1.15)
            c = Vector(centre) + Vector((0, (j + 0.5 * (i % 2)) * 2 * r - cols * r, i * 1.75 * r + rr))
            c.x += random.uniform(-0.05, 0.05)
            part.cylinder(c, rr, length * random.uniform(0.9, 1.05), M["bark"], 'X', seg=8, uvoff=(random.random() * 3, random.random() * 3))
logs(yard, (L / 2 - 1.1, -W / 2 - 0.2, -0.6), 0.6, 9, 12)

# ---------------------------------------------------------------- props (own origins): mud stove, dongmo churn, rug
stove = Part("SM_MudStove")
# body: plastered clay block with a front firebox and two pot holes; hood + pipe above
stove.box((0, 0, 0.29), (0.9, 0.72, 0.58), M["clay"], bevel=0.09, jitter=0.014, shade=0.95)
stove.box((0.5, 0, 0.26), (0.26, 0.5, 0.52), M["clay"], bevel=0.07, jitter=0.012, shade=0.85)
stove.box((0.64, 0, 0.21), (0.02, 0.24, 0.26), M["soot"])                       # the dark fire mouth
stove.box((0.6, 0, 0.13), (0.06, 0.2, 0.05), M["ember"])                        # embers in it
for k in range(3):
    stove.cylinder((0.72 + k * 0.03, -0.06 + k * 0.06, 0.17 + k * 0.025), 0.035, 0.42, M["wood"], 'X', seg=7,
                   uvoff=(k, 0))                                                 # firewood sticking out
for py in (-0.17, 0.18):                                                          # two pot rings (the pots are Poly Haven models)
    stove.cylinder((0.0, py, 0.585), 0.15, 0.012, M["soot"], seg=18)
# hood (blackened sheet metal) hanging from the beams, pipe up through the roof
hood_z0, hood_z1 = 1.5, 1.95
g = bmesh.ops.create_cone(stove.bm, cap_ends=False, segments=4, radius1=0.62, radius2=0.16, depth=hood_z1 - hood_z0)
vs = g["verts"]; bmesh.ops.rotate(stove.bm, verts=vs, cent=Vector(), matrix=Matrix.Rotation(math.pi / 4, 3, 'Z'))
bmesh.ops.transform(stove.bm, matrix=Matrix.Translation(Vector((0, 0, (hood_z0 + hood_z1) / 2))), verts=vs)
stove._finish(list({f for v in vs for f in v.link_faces}), M["soot"], (0, 0), 0.8, 1.0)
stove.cylinder((0, 0, (hood_z1 + 3.9) / 2), 0.08, 3.9 - hood_z1, M["soot"], seg=12, caps=False)
stove.box((0, 0, hood_z0 - 0.02), (0.92, 0.92, 0.04), M["soot"])                # hood rim (a shelf for drying cheese)

churn = Part("SM_Dongmo")
seg = 16
churn.cylinder((0, 0, 0.42), 0.085, 0.84, M["wood"], seg=seg, uvoff=(0.2, 0))
for z in (0.12, 0.42, 0.72):
    churn.cylinder((0, 0, z), 0.092, 0.035, M["brass"], seg=seg)
churn.cylinder((0, 0, 0.84 + 0.18), 0.012, 0.42, M["wood"], seg=6)             # plunger rod
churn.cylinder((0, 0, 0.84 + 0.4), 0.03, 0.03, M["wood"], seg=8)
churn.cylinder((0, 0, 0.84 + 0.01), 0.09, 0.02, M["brass"], seg=seg)

rug = Part("SM_TibetanRug")
f = rug.quad([Vector((-0.5, -0.5, 0)), Vector((0.5, -0.5, 0)), Vector((0.5, 0.5, 0)), Vector((-0.5, 0.5, 0))], M["rug"])
for lp, uvv in zip(f.loops, ((0, 0), (1, 0), (1, 1), (0, 1))): lp[rug.uv].uv = uvv
rug.box((0, 0, -0.004), (1.0, 1.0, 0.008), M["rug"])


def make_rug_texture(path, W_=512, H_=1024):
    """A Tibetan rug: dark blue field, two lotus/medallion motifs, red and saffron border with a key (swastika-free) meander."""
    img = bpy.data.images.new("T_TibetanRug", W_, H_, alpha=False)
    px = [0.0] * (W_ * H_ * 4)
    navy, red, saffron, cream, black, teal = (0.03, 0.05, 0.16), (0.45, 0.04, 0.03), (0.75, 0.38, 0.05), (0.78, 0.7, 0.55), (0.02, 0.02, 0.02), (0.05, 0.22, 0.22)
    for y in range(H_):
        for x in range(W_):
            u, v = x / W_, y / H_
            dx, dy = min(u, 1 - u), min(v, 1 - v) * (H_ / W_)
            e = min(dx, dy)
            if e < 0.018: c = black
            elif e < 0.07:
                # meander border: alternating saffron/red blocks with cream steps
                t = (x // 18 + y // 18) % 4 if dx < dy else (x // 18 + y // 18) % 4
                c = (saffron, red, cream, red)[t]
            elif e < 0.085: c = cream
            else:
                c = navy
                for cy in (0.3, 0.7):
                    rr = math.hypot(u - 0.5, (v - cy) * H_ / W_)
                    if rr < 0.2:
                        ang = math.atan2((v - cy) * H_ / W_, u - 0.5)
                        petal = 0.13 + 0.05 * math.cos(ang * 8)
                        c = red if rr < petal else (saffron if rr < petal + 0.025 else teal if rr < 0.19 else cream)
                        if rr < 0.05: c = cream if rr > 0.03 else saffron
                if 0.47 < u < 0.53 and 0.45 < v < 0.55: c = saffron
            n = 0.88 + 0.12 * (((x * 7) ^ (y * 13)) % 9) / 8.0
            i = (y * W_ + x) * 4; px[i:i + 4] = [c[0] * n, c[1] * n, c[2] * n, 1.0]
    img.pixels = px; img.filepath_raw = path; img.file_format = 'PNG'; img.save()


# ---------------------------------------------------------------- build + export
house = [stones.build(), core.build(), timber.build(), frames.build(), roof.build(), rocks.build(), inner.build(), yard.build()]
props = [stove.build(), churn.build(), rug.build()]
for o in house + props:
    for p in o.data.polygons: p.use_smooth = False
make_rug_texture(os.path.join(DEST, "T_TibetanRug.png"))
tris = {o.name: sum(len(p.vertices) - 2 for p in o.data.polygons) for o in house + props}
print("TRIS", tris)
def fbx(objs, path):
    bpy.ops.object.select_all(action='DESELECT')
    for o in objs: o.select_set(True)
    bpy.context.view_layer.objects.active = objs[0]
    bpy.ops.export_scene.fbx(filepath=path, use_selection=True, apply_scale_options='FBX_SCALE_UNITS', mesh_smooth_type='FACE',
                             add_leaf_bones=False, bake_anim=False, colors_type='SRGB')
fbx(house, os.path.join(DEST, "SM_House.fbx"))
fbx(props, os.path.join(DEST, "SM_HouseProps.fbx"))
json.dump(SLOTS, open(os.path.join(DEST, "slots.json"), "w"), indent=1)

# previews: outside three-quarter, the door side, and a cutaway into the kitchen
s = bpy.context.scene
s.render.engine = 'BLENDER_WORKBENCH'; s.display.shading.color_type = 'MATERIAL'; s.display.shading.light = 'STUDIO'
s.display.shading.show_cavity = True; s.render.resolution_x = 1280; s.render.resolution_y = 800
for o in props: o.hide_render = True
cam = bpy.data.objects.new("Cam", bpy.data.cameras.new("Cam")); s.collection.objects.link(cam); s.camera = cam; cam.data.lens = 28
os.makedirs(os.path.join(OUT, "Preview"), exist_ok=True)
for nm, loc, tgt in (("house_threeq", (9.5, 8.0, 3.2), (0, 0, 1.0)), ("house_door", (7.5, -2.5, 1.3), (2.5, 0, 1.2)),
                     ("house_back", (-8.0, -7.5, 2.5), (0, 0, 1.0))):
    cam.location = Vector(loc); cam.rotation_euler = (Vector(tgt) - Vector(loc)).to_track_quat('-Z', 'Y').to_euler()
    s.render.filepath = os.path.join(OUT, "Preview", f"{nm}.png"); bpy.ops.render.render(write_still=True)
for o in props: o.hide_render = False
props[0].location = (SX, SY, 0); props[1].location = (-1.5, 1.6, 0); props[2].location = (0.2, -0.2, 0.012); props[2].scale = (2.0, 1.3, 1)
for o in house:
    if o.name in ("SM_House_Roof", "SM_House_RoofStones"): o.hide_render = True
cam.location = Vector((2.1, -1.6, 1.9)); cam.rotation_euler = (Vector((-1.5, 1.0, 0.8)) - cam.location).to_track_quat('-Z', 'Y').to_euler()
s.render.filepath = os.path.join(OUT, "Preview", "house_kitchen.png"); bpy.ops.render.render(write_still=True)
save_blend("sherpa_house")
print("DONE house")
