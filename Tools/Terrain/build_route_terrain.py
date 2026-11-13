# Route terrain (v6): 1 m detail tiles along the whole climbing route, replacing the landscape inside the corridor.
# Look rules come from the reference photos (Docs/ArtDirection/terrain_refs + the Commons set): snow covers most of the
# ground and lies flat and calm; rock and rubble show only where the mountain puts them (crests, steep ground, the South
# Col saddle, the debris-covered glacier tongue); stones of every size sit in and around the rubble.
#
# Per 500 m tile:  <out>/<name>.obj (render, 1 m, UV0.x = rubble 0..1), <name>_col.obj (collision, 2 m),
#                  scatter_<name>.json (rocks), and one landscape_patch.bin for LandscapeRebuilder.ApplyHeightPatch.
# Corridor: full detail to CORE_M from the route, fades to the landscape's exact triangles by FADE_M, then a short skirt
# drops just under the landscape so the two surfaces never fight (no edge shadow, no seam line).
# Noise is a function of world position only, so tiles join without seams.
# Usage: python build_route_terrain.py [only_tiles "tx,ty;tx,ty"]
import sys, os, json, math, numpy as np
from scipy.interpolate import RectBivariateSpline
from scipy.ndimage import gaussian_filter, distance_transform_edt, minimum_filter, laplace, map_coordinates
from scipy.spatial import cKDTree
from PIL import Image

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
SRC = os.path.join(ROOT, 'Saved/DetailTiles'); OUT = os.path.join(SRC, 'route'); os.makedirs(OUT, exist_ok=True)
LX = LY = -5040.0; S = 16.8712; LOCZ = 1389.5695625; CM_PER_UNIT = 951.4 / 128.0
TILE = 500; MARGIN = 64
CORE_M, FADE_M, SKIRT_M, EMIT_M = 400.0, 545.0, 560.0, 575.0   # detail -> landscape shape -> under it -> end of mesh
SINK_VERTEX_M = 520.0                                            # landscape vertices nearer than this go under the tiles
CLEAR_M = 0.6
CURV_STD = 0.008             # typical std of the curvature field along the route (0.004 EBC .. 0.017 summit)
ROCK_THR = 1.7               # score above this = rock/rubble (snow covers most of the ground)

# ---------------- inputs
smp = np.load(os.path.join(SRC, 'corridor_sample.npz'), allow_pickle=True)
G = smp['heights'] / 100.0; I0, J0 = int(smp['i0']), int(smp['j0'])
props = json.loads(str(smp['props']))
if np.isnan(G).any():
    m = np.isnan(G); ii = distance_transform_edt(m, return_distances=False, return_indices=True)
    G = np.where(m, gaussian_filter(G[tuple(ii)], 1.5), G)
GX = LX + (I0 + np.arange(G.shape[1])) * S; GY = LY + (J0 + np.arange(G.shape[0])) * S
spline = RectBivariateSpline(GY, GX, G, kx=3, ky=3)
elev_off = 8849.0 - G.max()
info = json.load(open(os.path.join(ROOT, 'Saved/Compare/ygg_info.json')))
route = np.array([p[:2] for l in json.load(open(os.path.join(ROOT, 'Tools/RoutePlanning/route.json'))) for p in l['pts']]) / 100.0
dense = np.concatenate([np.linspace(a, b, max(2, int(np.hypot(*(b - a)) / 3)), endpoint=False) for a, b in zip(route[:-1], route[1:])] + [route[-1:]])
rtree = cKDTree(dense)
ropes = [np.array(r) / 100.0 for r in info['ropes'] if len(r) > 1]
rope_pts = np.concatenate([np.linspace(a, b, max(2, int(np.hypot(*(b - a)))), endpoint=False) for r in ropes for a, b in zip(r[:-1], r[1:])])
ptree = cKDTree(rope_pts)
camps = [(x / 100, y / 100) for _, x, y in info['camps']]
tents = [(px / 100, py / 100, np.hypot(ex, ey) / 100) for n, px, py, pz, ex, ey, ez in props
         if max(ex, ey) < 3000 and any(k in n for k in ('Tent', 'Depot', 'Stores', 'Landmark'))]
avoid = [(px / 100, py / 100, np.hypot(ex, ey) / 100 + 2.5) for n, px, py, pz, ex, ey, ez in props if max(ex, ey) < 3000]
atree = cKDTree(np.array([(a, b) for a, b, _ in avoid])) if avoid else None
gmask = np.asarray(Image.open(os.path.join(ROOT, 'Imports/Glacier/T_GlacierMask.png')).convert('L'), np.float32) / 255.0
gmask_soft = gaussian_filter(gmask, 1.2)

def glacier_at(X, Y):          # same mapping as the landscape material: uv = (world cm + 504000) / (3374.24 * 1009)
    u = (X * 100 + 504000.0) / (3374.24 * 1009.0) * gmask.shape[1]; v = (Y * 100 + 504000.0) / (3374.24 * 1009.0) * gmask.shape[0]
    # cubic on a softened mask (2026-11-11): linear interpolation of the 34 m mask grid creased the ground along its grid lines
    return map_coordinates(gmask_soft, [v.ravel() - 0.5, u.ravel() - 0.5], order=3, mode='nearest').reshape(X.shape)

# ---------------- world-coherent noise
def _hash(ix, iy, seed):
    h = (ix.astype(np.int64) * 374761393 + iy.astype(np.int64) * 668265263 + seed * 2147483647) & 0xFFFFFFFF
    h = ((h ^ (h >> 13)) * 1274126177) & 0xFFFFFFFF
    return ((h ^ (h >> 16)) & 0xFFFFFF) / float(0xFFFFFF)
def vnoise(X, Y, wl, seed):
    x, y = X / wl, Y / wl; ix, iy = np.floor(x), np.floor(y); fx, fy = x - ix, y - iy
    ux, uy = fx * fx * fx * (fx * (fx * 6 - 15) + 10), fy * fy * fy * (fy * (fy * 6 - 15) + 10)
    a, b = _hash(ix, iy, seed), _hash(ix + 1, iy, seed); c, d = _hash(ix, iy + 1, seed), _hash(ix + 1, iy + 1, seed)
    return (a + (b - a) * ux + (c - a) * uy + (a - b - c + d) * ux * uy - 0.5) / 0.2
def octave(X, Y, wl, amp, seed):
    return (vnoise(X, Y, wl, seed) + vnoise(X * 0.8 - Y * 0.6, X * 0.6 + Y * 0.8, wl * 1.07, seed + 999)) * 0.7071 * amp
def sstep(a, b, x): t = np.clip((x - a) / (b - a), 0, 1); return t * t * (3 - 2 * t)

def land_surface(X, Y):
    """The landscape exactly as Unreal draws it: each 17 m quad split along its (0,0)-(1,1) diagonal."""
    fx = (X - GX[0]) / S; fy = (Y - GY[0]) / S
    i = np.clip(np.floor(fx).astype(int), 0, len(GX) - 2); j = np.clip(np.floor(fy).astype(int), 0, len(GY) - 2)
    u, v = fx - i, fy - j; h00, h10, h01, h11 = G[j, i], G[j, i + 1], G[j + 1, i], G[j + 1, i + 1]
    return np.where(u >= v, h00 + (h10 - h00) * u + (h11 - h10) * v, h00 + (h01 - h00) * v + (h11 - h01) * u)

def build(X, Y):
    """Heights (m), rubble 0..1 and helpers for a block of 1 m samples (with margin)."""
    smooth = spline(Y[:, 0], X[0]); land = land_surface(X, Y)
    dist = rtree.query(np.c_[X.ravel(), Y.ravel()])[0].reshape(X.shape)
    elev = smooth + elev_off
    gl = glacier_at(X, Y)
    debris = (1 - sstep(5050, 5230, elev)) * sstep(0.3, 0.6, gl)                 # rubble-covered glacier tongue
    ice = sstep(0.3, 0.6, gl) * (1 - debris)                                      # clean glacier: calm, no rubble
    gy, gx = np.gradient(smooth, 1.0); slope = np.hypot(gx, gy)
    steep = sstep(0.6, 1.0, slope)
    calm = 0.3 + 0.7 * sstep(-0.5, 0.8, octave(X, Y, 500, 1, 101))
    big = (octave(X, Y, 260, 4.0, 102) + octave(X, Y, 110, 1.4, 103)) * calm * (1 - 0.6 * ice)
    big += (octave(X, Y, 45, 1.6, 106) + np.abs(octave(X, Y, 18, 0.8, 107))) * debris   # moraine hummocks
    base = smooth + big
    curv = -laplace(gaussian_filter(base, 20.0)) / CURV_STD
    ax, ay = camps[7]
    saddle = 1 - sstep(120, 320, np.hypot(X - ax, Y - ay))
    score = 0.9 * curv + 0.55 * octave(X, Y, 120, 1, 105) + 0.9 * steep + 2.2 * saddle
    score -= 1.2 * sstep(8250, 8550, elev)                                       # the summit ridge is snow (Hillary Step photo)
    score = score * (1 - ice) + 3.0 * debris
    soft = gaussian_filter((score > ROCK_THR).astype(float), 8.0)
    rock = sstep(0.2, 0.8, soft)
    # snow: flat and calm (photos), a faint wind texture only in rare patches
    snow = octave(X, Y, 40, 0.18, 111) + octave(X, Y, 14, 0.03, 112)
    warp = octave(X, Y, 30, 6.0, 113) + octave(X, Y, 80, 15.0, 114)
    ph = (X + warp) / 5.0 + octave(X, Y, 50, 1.2, 115)
    saw = ph % 1.0; saw = np.where(saw < 0.82, saw / 0.82, (1 - saw) / 0.18)
    snow += (saw ** 1.6) * 0.06 * sstep(0.6, 0.95, octave(X, Y, 45, 1, 116)) * (1 - ice)
    # rubble: knobbly ground at 2-6 m (the 1 m mesh), finer stones come from displacement and scanned rocks
    f = octave(X, Y, 22, 1, 121)
    rockrel = (np.clip(1 - np.abs(f) * 0.7, 0, 1) ** 2 - 0.4) * 1.0 + octave(X, Y, 6, 0.35, 122) + np.abs(octave(X, Y, 3, 0.25, 123)) - 0.12
    detail = snow * (1 - rock) + rockrel * rock
    hollow = gaussian_filter(detail, 4.0) - detail
    breakup = octave(X, Y, 9, 0.25, 141) + octave(X, Y, 3.5, 0.12, 142) + octave(X, Y, 25, 0.15, 143)
    near_band = 0.3 + 0.7 * sstep(0.02, 0.3, soft)                            # breakup only shapes real bands
    rockw = sstep(0.3, 0.7, soft + (breakup - hollow * 0.6) * near_band + 0.15 * saddle * soft)
    rockw = rockw * (1 - ice)
    # keep camps, ropes and the walking line usable
    k_big = np.ones_like(X); k_small = np.ones_like(X)
    rd = ptree.query(np.c_[X.ravel(), Y.ravel()])[0].reshape(X.shape)
    # gentle and wide (2026-11-11): the old 1.5-6 m flattening along every rope left straight parallel creases
    k_big = np.minimum(k_big, 0.35 + 0.65 * sstep(10, 90, rd)); k_small = np.minimum(k_small, 0.7 + 0.3 * sstep(2, 16, rd))
    trench = 0.0   # 2026-11-11: the 35 cm trench under every fixed rope drew straight light/dark lines across the snow (user); removed
    cx0, cy0 = X.mean(), Y.mean()
    for (tx, ty, r) in tents:
        if abs(tx - cx0) > 900 or abs(ty - cy0) > 900: continue
        d = np.hypot(X - tx, Y - ty)
        k_big = np.minimum(k_big, sstep(r + 20, r + 70, d)); k_small = np.minimum(k_small, 0.2 + 0.8 * sstep(r + 1, r + 8, d))
    full = smooth + big * k_big + detail * k_small - trench
    fade = 1 - sstep(CORE_M, FADE_M, dist)
    Z = land + (full - land) * fade - 0.3 * sstep(FADE_M, SKIRT_M, dist)
    rockw = rockw * (1 - sstep(CORE_M, FADE_M, dist))
    near = sstep(0.02, 0.35, gaussian_filter(rockw, 10.0))                    # within ~20-30 m of rubble
    gy2, gx2 = np.gradient(Z, 1.0)
    return dict(Z=Z, near=near, rock=rockw, dist=dist, land=land, gx=gx2, gy=gy2, ice=ice, debris=debris, k_small=k_small, rd=rd)

# ---------------- rocks (scanned meshes from the project library; closed scans only - the cliff scans are hollow)
def h1(ix, iy, s):
    v = (np.int64(ix) * 374761393 + np.int64(iy) * 668265263 + s * 2147483647) & 0xFFFFFFFF
    v = ((v ^ (v >> 13)) * 1274126177) & 0xFFFFFFFF
    return ((v ^ (v >> 16)) & 0xFFFFFF) / float(0xFFFFFF)
HEIGHT_M = {'stonephoto': 2.9, 'alaskan': 1.4, 'stone': 0.2, 'rockysnowpile': 0.8}
FOOT_M = {'stonephoto': 3.5, 'alaskan': 0.5, 'stone': 0.35, 'rockysnowpile': 2.0}
def scatter(T, X0, Y0, n0, n1, out):
    """T: build() dict cropped to the tile (rows = y, cols = x, 1 m). Appends [comp, x, y, z, yaw, upx, upy, scale] (cm)."""
    Z, R, gx, gy = T['Z'], T['rock'], T['gx'], T['gy']
    def at(a, x, y):
        # bilinear (2026-11-13: nearest-grid heights left stones up to ~35 cm in the air on slopes; Tools/Placement/ground_fix
        # seats anything placed afterwards)
        fy, fx = y - Y0, x - X0
        i, j = int(math.floor(fy)), int(math.floor(fx))
        if not (0 <= i < n0 - 1 and 0 <= j < n1 - 1):
            i, j = int(round(fy)), int(round(fx))
            return a[i, j] if 0 <= i < n0 and 0 <= j < n1 else None
        u, v = fx - j, fy - i
        return (a[i, j] * (1 - u) + a[i, j + 1] * u) * (1 - v) + (a[i + 1, j] * (1 - u) + a[i + 1, j + 1] * u) * v
    def ok(x, y, pad):
        if ptree.query([x, y])[0] < 3.0 + pad: return False
        if atree is not None:
            for k in atree.query_ball_point([x, y], 40):
                ax, ay, r = avoid[k]
                if math.hypot(x - ax, y - ay) < r + pad: return False
        return True
    def put(comp, key, x, y, sc, sink, s, check=True):
        z = at(Z, x, y)
        if z is None: return
        ga, gb = at(gx, x, y), at(gy, x, y); tilt = 0.6; hm = HEIGHT_M[key] * sc
        if check:
            rf = FOOT_M[key] * sc; worst = 0.0
            for k in range(12):
                a = k * math.pi / 6
                for fr in (0.5, 1.0):
                    px, py = x + math.cos(a) * rf * fr, y + math.sin(a) * rf * fr
                    zz = at(Z, px, py)
                    if zz is None: return
                    worst = max(worst, z + (ga * (px - x) + gb * (py - y)) * tilt - zz)
            if worst > max(hm * sink * 0.7, 0.12): return
        out.append([comp, x * 100, y * 100, (z - hm * sink) * 100, h1(int(x * 7), int(y * 7), s) * 360.0, -ga * tilt, -gb * tilt, sc])
    def cl(x, y, wl, s):
        fx, fy = x / wl, y / wl; ix, iy = math.floor(fx), math.floor(fy); u, v = fx - ix, fy - iy
        u, v = u * u * (3 - 2 * u), v * v * (3 - 2 * v)
        return h1(ix, iy, s) * (1 - u) * (1 - v) + h1(ix + 1, iy, s) * u * (1 - v) + h1(ix, iy + 1, s) * (1 - u) * v + h1(ix + 1, iy + 1, s) * u * v
    # giants: 20-30 m boulders, rare, on or beside rock bands and slopes, never on the walking line
    for gy_ in np.arange(Y0 + 20, Y0 + n0 - 20, 70.0):
        for gx_ in np.arange(X0 + 20, X0 + n1 - 20, 70.0):
            ix, iy = int(gx_), int(gy_)
            x = gx_ + (h1(ix, iy, 201) - 0.5) * 60; y = gy_ + (h1(ix, iy, 202) - 0.5) * 60
            r = at(R, x, y); d = at(T['dist'], x, y)
            if r is None or d is None or d > CORE_M or at(T['ice'], x, y) > 0.5: continue
            if h1(ix, iy, 203) > 0.06 + 0.25 * r * cl(x, y, 400, 204) or not ok(x, y, 25): continue
            put('giant', 'stonephoto', x, y, (20 + 10 * h1(ix, iy, 205)) / 7.0, 0.38, 206)
    # big 4-12 m boulders, clumped in rubble
    for gy_ in np.arange(Y0 + 8, Y0 + n0 - 8, 24.0):
        for gx_ in np.arange(X0 + 8, X0 + n1 - 8, 24.0):
            ix, iy = int(gx_), int(gy_)
            x = gx_ + (h1(ix, iy, 211) - 0.5) * 22; y = gy_ + (h1(ix, iy, 212) - 0.5) * 22
            r = at(R, x, y)
            if r is None or r < 0.5 or h1(ix, iy, 213) > 0.4 * cl(x, y, 90, 214) ** 2 + 0.04 or not ok(x, y, 6): continue
            put('boulder', 'stonephoto', x, y, (0.6 + 1.1 * h1(ix, iy, 215) ** 1.5), 0.38, 216)
    # medium 1-4 m, along rubble edges and in clumps
    for gy_ in np.arange(Y0 + 4, Y0 + n0 - 4, 5.0):
        for gx_ in np.arange(X0 + 4, X0 + n1 - 4, 5.0):
            ix, iy = int(gx_), int(gy_)
            x = gx_ + (h1(ix, iy, 221) - 0.5) * 4.8; y = gy_ + (h1(ix, iy, 222) - 0.5) * 4.8
            r = at(R, x, y)
            if r is None: continue
            edge = 1 - abs(r - 0.5) * 2
            want = 0.35 * edge * cl(x, y, 35, 223) + 0.22 * r * cl(x, y, 60, 224) ** 2
            if h1(ix, iy, 225) > want or not ok(x, y, 1.5): continue
            key = ('alaskan', 'stonephoto', 'rockysnowpile')[int(h1(ix, iy, 226) * 3) % 3]
            sc = {'alaskan': 0.8 + 2.2 * h1(ix, iy, 227) ** 1.8, 'stonephoto': 0.15 + 0.35 * h1(ix, iy, 227) ** 1.8, 'rockysnowpile': 0.6 + 0.8 * h1(ix, iy, 227)}[key]
            put('medium_' + key, key, x, y, sc, 0.32, 228)
    # small 0.15-1.2 m, in large numbers: dense in rubble, sparse stones poking through the snow near it
    for gy_ in np.arange(Y0 + 2, Y0 + n0 - 2, 2.6):
        for gx_ in np.arange(X0 + 2, X0 + n1 - 2, 2.6):
            ix, iy = int(gx_ * 3), int(gy_ * 3)
            x = gx_ + (h1(ix, iy, 231) - 0.5) * 2.5; y = gy_ + (h1(ix, iy, 232) - 0.5) * 2.5
            r = at(R, x, y); d = at(T['dist'], x, y)
            if r is None or d is None or d > FADE_M: continue
            near = gaussian_near = r
            nr = at(T['near'], x, y)
            want = 0.55 * r * (0.4 + 0.6 * cl(x, y, 18, 233)) + (1 - r) * (0.4 * nr * cl(x, y, 25, 239) + 0.015 * cl(x, y, 50, 234))
            if h1(ix, iy, 235) > want or not ok(x, y, 0.6): continue
            if at(T['ice'], x, y) > 0.5: continue
            key = 'stone' if h1(ix, iy, 236) < 0.6 else 'alaskan'
            sc = (0.4 + 2.6 * h1(ix, iy, 237) ** 2.5) if key == 'stone' else (0.1 + 0.55 * h1(ix, iy, 237) ** 2.2)
            put('small_' + key, key, x, y, sc, 0.3, 238, check=False)

# ---------------- tiles
def tile_list():
    tiles = set()
    for p in dense[::4]:
        for dx in np.arange(-EMIT_M - 10, EMIT_M + 11, 125):
            for dy in np.arange(-EMIT_M - 10, EMIT_M + 11, 125):
                if math.hypot(dx, dy) <= EMIT_M + 10: tiles.add((int((p[0] + dx) // TILE), int((p[1] + dy) // TILE)))
    return sorted(tiles)

def write_obj(path, X, Y, Z, U, keep, piv):
    used = keep.copy()
    quad = keep[:-1, :-1] | keep[1:, :-1] | keep[:-1, 1:] | keep[1:, 1:]
    used[:] = False; used[:-1, :-1] |= quad; used[1:, :-1] |= quad; used[:-1, 1:] |= quad; used[1:, 1:] |= quad
    idx = np.full(X.shape, -1, np.int64); idx[used] = np.arange(used.sum()) + 1
    with open(path, 'w') as f:
        f.write(''.join('v %.1f %.1f %.1f\n' % t for t in zip(((X - piv[0]) * 100)[used], (-(Y - piv[1]) * 100)[used], ((Z - piv[2]) * 100)[used])))
        f.write(''.join('vt %.4f 0\n' % u for u in U[used]))
        qy, qx = np.nonzero(quad)
        p, q, s_, t = idx[qy, qx], idx[qy, qx + 1], idx[qy + 1, qx], idx[qy + 1, qx + 1]
        f.write(''.join('f %d/%d %d/%d %d/%d\nf %d/%d %d/%d %d/%d\n' % (a, a, c, c, b, b, c, c, e, e, b, b) for a, b, c, e in zip(p, q, s_, t)))
    return int(quad.sum()) * 2

if __name__ == '__main__':
    only = None
    if len(sys.argv) > 1: only = {tuple(int(v) for v in s.split(',')) for s in sys.argv[1].split(';')}
    tiles = [t for t in tile_list() if only is None or t in only]
    index = {}; patch = {}
    for k, (tx, ty) in enumerate(tiles):
        x0, y0 = tx * TILE, ty * TILE
        xs = np.arange(x0 - MARGIN, x0 + TILE + MARGIN + 1, 1.0); ys = np.arange(y0 - MARGIN, y0 + TILE + MARGIN + 1, 1.0)
        X, Y = np.meshgrid(xs, ys)
        if rtree.query(np.c_[X[::25, ::25].ravel(), Y[::25, ::25].ravel()])[0].min() > EMIT_M + 30: continue
        B = build(X, Y)
        c = np.s_[MARGIN:-MARGIN, MARGIN:-MARGIN]
        T = {key: (v[c] if isinstance(v, np.ndarray) else v) for key, v in B.items()}
        TX, TY = X[c], Y[c]
        keep = T['dist'] < EMIT_M
        if not keep.any(): continue
        name = f'R_{tx}_{ty}'; piv = [x0 + TILE / 2, y0 + TILE / 2, float(T['Z'][keep].min())]
        tris = write_obj(os.path.join(OUT, name + '.obj'), TX, TY, T['Z'], T['rock'], keep, piv)
        write_obj(os.path.join(OUT, name + '_col.obj'), TX[::2, ::2], TY[::2, ::2], T['Z'][::2, ::2], T['rock'][::2, ::2], keep[::2, ::2], piv)
        sc = []; scatter(T, xs[MARGIN], ys[MARGIN], TX.shape[0], TX.shape[1], sc)
        json.dump(sc, open(os.path.join(OUT, f'scatter_{name}.json'), 'w'))
        # landscape vertices in this tile that go under the detail surface
        zmin = minimum_filter(B['Z'], size=int(2 * S) + 3)
        for jj in range(int(math.ceil((y0 - LY) / S)), int(math.floor((y0 + TILE - LY) / S)) + 1):
            for ii in range(int(math.ceil((x0 - LX) / S)), int(math.floor((x0 + TILE - LX) / S)) + 1):
                vx, vy = LX + ii * S, LY + jj * S
                pj, pi = int(round(vy - ys[0])), int(round(vx - xs[0]))
                if not (0 <= pj < B['Z'].shape[0] and 0 <= pi < B['Z'].shape[1]): continue
                if B['dist'][pj, pi] >= SINK_VERTEX_M: continue
                zc = (zmin[pj, pi] - CLEAR_M) * 100.0
                patch[(ii, jj)] = 32768.0 + (zc - LOCZ * 100.0) / CM_PER_UNIT
        index[name] = piv
        print(f'{k + 1}/{len(tiles)} {name} tris {tris} rocks {len(sc)} rubble {100 * (T["rock"][keep] > 0.5).mean():.0f}%', flush=True)
    json.dump(index, open(os.path.join(OUT, 'tiles.json'), 'w'), indent=0)
    if patch:
        iis = [p[0] for p in patch]; jjs = [p[1] for p in patch]
        i0, i1, j0, j1 = min(iis), max(iis), min(jjs), max(jjs)
        arr = np.full((j1 - j0 + 1, i1 - i0 + 1), -1.0, np.float32)
        for (ii, jj), v in patch.items(): arr[jj - j0, ii - i0] = v
        with open(os.path.join(OUT, 'landscape_patch.bin'), 'wb') as f:
            np.array([i0, j0, arr.shape[1], arr.shape[0]], np.int32).tofile(f); arr.tofile(f)
        print('patch', arr.shape, len(patch))
