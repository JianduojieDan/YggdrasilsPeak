# Builds 1 m detail terrain tiles along the climbing route (seamless: all noise is a function of world position).
# Input:  Saved/DetailTiles/corridor_sample.npz from sample_corridor.py (landscape heights at its vertices + props),
#         Tools/RoutePlanning/route.json, Saved/Compare/ygg_info.json (ropes).
# Output: Saved/DetailTiles/T_<i>_<j>.obj (render, 1 m) + T_<i>_<j>_col.obj (collision, 4 m) + tiles.json (pivots).
# Usage:  python build_corridor.py [min_elev_m] [max_elev_m] [only_tile i,j]
import sys, os, json, numpy as np
from scipy.interpolate import RectBivariateSpline
from scipy.ndimage import gaussian_filter, maximum_filter
from scipy.spatial import cKDTree

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
OUT = os.path.join(ROOT, 'Saved/DetailTiles')
LX = LY = -5040.0; S = 16.8712                 # landscape origin / spacing, metres
TILE = 500; MARGIN = 40; CORE_M = 250.0; EDGE_M = 350.0; COL_STEP = 4
REF_Z = None                                     # set from sample

smp = np.load(os.path.join(OUT, 'corridor_sample.npz'), allow_pickle=True)
G = smp['heights'] / 100.0; I0, J0 = int(smp['i0']), int(smp['j0'])
props = json.loads(str(smp['props']))
if np.isnan(G).any():                            # fill missed samples from the nearest good one, then soften
    from scipy.ndimage import distance_transform_edt
    m = np.isnan(G); ii = distance_transform_edt(m, return_distances=False, return_indices=True)
    F = G[tuple(ii)]; G = np.where(m, gaussian_filter(F, 1.5), G)
GX = LX + (I0 + np.arange(G.shape[1])) * S; GY = LY + (J0 + np.arange(G.shape[0])) * S
spline = RectBivariateSpline(GY, GX, G, kx=3, ky=3)
elev_off = 8849.0 - G.max()                      # world metres -> real elevation

route = np.array([p[:2] for l in json.load(open(os.path.join(ROOT, 'Tools/RoutePlanning/route.json'))) for p in l['pts']]) / 100.0
dense = np.concatenate([np.linspace(a, b, max(2, int(np.hypot(*(b - a)) / 4)), endpoint=False) for a, b in zip(route[:-1], route[1:])] + [route[-1:]])
rtree = cKDTree(dense)
ropes = [np.array(r) / 100.0 for r in json.load(open(os.path.join(ROOT, 'Saved/Compare/ygg_info.json')))['ropes']]
rope_pts = np.concatenate([np.concatenate([np.linspace(a, b, max(2, int(np.hypot(*(b - a)) / 2)), endpoint=False) for a, b in zip(r[:-1], r[1:])]) for r in ropes if len(r) > 1])
ptree = cKDTree(rope_pts)
keep_props = [(px / 100, py / 100, max(np.hypot(ex, ey) / 100 + 3.0, 4.0)) for name, px, py, pz, ex, ey, ez in props if max(ex, ey) < 3000]
ktree = cKDTree(np.array([(a, b) for a, b, _ in keep_props])) if keep_props else None

# ---------- world-coherent noise
def _hash(ix, iy, seed):
    h = (ix.astype(np.int64) * 374761393 + iy.astype(np.int64) * 668265263 + seed * 2147483647) & 0xFFFFFFFF
    h = ((h ^ (h >> 13)) * 1274126177) & 0xFFFFFFFF
    return ((h ^ (h >> 16)) & 0xFFFFFF) / float(0xFFFFFF)
def vnoise(X, Y, wl, seed):
    x, y = X / wl, Y / wl; ix, iy = np.floor(x), np.floor(y); fx, fy = x - ix, y - iy
    ux, uy = fx * fx * fx * (fx * (fx * 6 - 15) + 10), fy * fy * fy * (fy * (fy * 6 - 15) + 10)
    a, b = _hash(ix, iy, seed), _hash(ix + 1, iy, seed); c, d = _hash(ix, iy + 1, seed), _hash(ix + 1, iy + 1, seed)
    v = a + (b - a) * ux + (c - a) * uy + (a - b - c + d) * ux * uy
    return (v - 0.5) / 0.2                       # ~unit std
_seed = [0]
def octave(X, Y, wl, amp, seed):                 # two rotated value-noise layers for less grid look
    c, s = 0.8, 0.6
    return (vnoise(X, Y, wl, seed) + vnoise(X * c - Y * s, X * s + Y * c, wl * 1.07, seed + 999)) * 0.7071 * amp
def sstep(a, b, x): t = np.clip((x - a) / (b - a), 0, 1); return t * t * (3 - 2 * t)

def boulders(X, Y, cell, prob, hmax, rad, seed):
    out = np.full(X.shape, -1e3)
    cx0, cy0 = np.floor(X / cell), np.floor(Y / cell)
    for ox in (-1, 0, 1):
        for oy in (-1, 0, 1):
            ix, iy = cx0 + ox, cy0 + oy
            px = (ix + _hash(ix, iy, seed)) * cell; py = (iy + _hash(ix, iy, seed + 1)) * cell
            on = _hash(ix, iy, seed + 2) < prob; h = (0.4 + 0.6 * _hash(ix, iy, seed + 3)) * hmax
            r = rad * (0.6 + 0.8 * _hash(ix, iy, seed + 4))
            dd = np.hypot(X - px, Y - py) / r
            out = np.maximum(out, np.where(on & (dd < 1), h * (1 - dd * dd) ** 0.7, -1e3))
    return out

def detail_field(X, Y, smooth, elev):
    gy, gx = np.gradient(smooth, 1.0); steep = np.clip((np.hypot(gx, gy) - 0.3) / 0.5, 0, 1)
    low = 1 - sstep(4900, 5500, elev)            # trek / moraine country: rock and rubble, softer
    zone = np.maximum.reduce([sstep(-0.2, 0.5, octave(X, Y, 90, 1, 11) + 0.35 * octave(X, Y, 25, 1, 12)), steep, low])
    drift = octave(X, Y, 35, 1.4, 21) + octave(X, Y, 12, 0.5, 22)
    warp = octave(X, Y, 25, 8.0, 23) + octave(X, Y, 70, 20.0, 24)
    ph = (X + warp) / 4.5 + octave(X, Y, 40, 1.5, 25)
    saw = ph % 1.0; saw = np.where(saw < 0.8, saw / 0.8, (1 - saw) / 0.2)
    snow = drift + (saw ** 2 - 0.35) * 0.55 * sstep(-0.3, 0.6, octave(X, Y, 30, 1, 26))
    def ridged(wl, amp, pw, sd): f = octave(X, Y, wl, 1, sd); return (np.clip(1 - np.abs(f) * 0.6, 0, 1) ** pw - 0.4) * amp
    rock = ridged(45, 3.2, 2.0, 31) + ridged(14, 1.3, 2.5, 32) + ridged(4.5, 0.45, 3.0, 33) + octave(X, Y, 1.6, 0.08, 34)
    st = 2.2; t = (smooth + rock) / st
    rock += (np.floor(t) * st + st * sstep(0.7, 1.0, t % 1) - (smooth + rock)) * 0.7 * sstep(-0.2, 0.4, octave(X, Y, 35, 1, 35))
    for cell, prob, hmax, rad, sd in [(14, 0.5, 2.6, 8, 41), (7, 0.5, 1.3, 4.5, 42), (3.5, 0.45, 0.5, 2.2, 43)]:
        rock = np.maximum(rock, boulders(X, Y, cell, prob, hmax, rad, sd) - 0.3 + octave(X, Y, 2, 0.12, sd + 50))
    return (snow * (1 - zone) + rock * zone) * (1 - 0.35 * low)

def lin_surface(X, Y):                           # exact piecewise-linear landscape, max of both splits
    fx = (X - GX[0]) / S; fy = (Y - GY[0]) / S
    i = np.clip(np.floor(fx).astype(int), 0, len(GX) - 2); j = np.clip(np.floor(fy).astype(int), 0, len(GY) - 2)
    u, v = fx - i, fy - j; h00, h10, h01, h11 = G[j, i], G[j, i + 1], G[j + 1, i], G[j + 1, i + 1]
    ta = np.where(u + v <= 1, h00 + (h10 - h00) * u + (h01 - h00) * v, h11 + (h01 - h11) * (1 - u) + (h10 - h11) * (1 - v))
    tb = np.where(u >= v, h00 + (h10 - h00) * u + (h11 - h10) * v, h00 + (h01 - h00) * v + (h11 - h01) * u)
    return np.maximum(ta, tb)

def build_tile(ti, tj):
    x0, y0 = ti * TILE, tj * TILE
    xs = np.arange(x0 - MARGIN, x0 + TILE + MARGIN + 1, 1.0); ys = np.arange(y0 - MARGIN, y0 + TILE + MARGIN + 1, 1.0)
    X, Y = np.meshgrid(xs, ys)
    dist = rtree.query(np.c_[X.ravel(), Y.ravel()])[0].reshape(X.shape)
    if dist[MARGIN:-MARGIN, MARGIN:-MARGIN].min() > EDGE_M: return None
    smooth = spline(ys, xs); lin = lin_surface(X, Y); elev = smooth + elev_off
    det = detail_field(X, Y, smooth, elev)
    keep = np.ones_like(X)
    rd = ptree.query(np.c_[X.ravel(), Y.ravel()])[0].reshape(X.shape); keep = np.minimum(keep, np.clip((rd - 2.0) / 8.0, 0, 1))
    if ktree is not None:
        for k in ktree.query_ball_point([x0 + TILE / 2, y0 + TILE / 2], TILE):
            a, b, r = keep_props[k]; keep = np.minimum(keep, np.clip((np.hypot(X - a, Y - b) - r) / 12.0, 0, 1))
    corridor = 1 - sstep(CORE_M, EDGE_M, dist)
    lift = np.log1p(np.exp(3.0 * (det + 2.5))) / 3.0
    under = gaussian_filter(maximum_filter(np.maximum(lin - smooth, 0), 25), 8)
    Z = np.maximum(smooth + under * corridor + 0.05 + lift * keep * corridor, lin + 0.03)
    sl = np.s_[MARGIN:-MARGIN, MARGIN:-MARGIN]
    X, Y, Z, dist = X[sl], Y[sl], Z[sl], dist[sl]
    piv = [x0 + TILE / 2, y0 + TILE / 2, float(Z.min())]
    write_obj(os.path.join(OUT, f'T_{ti}_{tj}.obj'), X, Y, Z, dist, piv, 1)
    write_obj(os.path.join(OUT, f'T_{ti}_{tj}_col.obj'), X, Y, Z, dist, piv, COL_STEP)
    return piv

def write_obj(path, X, Y, Z, dist, piv, step):
    X, Y, Z, dist = X[::step, ::step], Y[::step, ::step], Z[::step, ::step], dist[::step, ::step]
    n0, n1 = X.shape
    inside = dist < EDGE_M + 15
    quad = inside[:-1, :-1] | inside[1:, :-1] | inside[:-1, 1:] | inside[1:, 1:]
    used = np.zeros_like(inside); used[:-1, :-1] |= quad; used[1:, :-1] |= quad; used[:-1, 1:] |= quad; used[1:, 1:] |= quad
    idx = np.full(X.shape, -1, np.int64); idx[used] = np.arange(used.sum()) + 1
    lx = (X - piv[0]) * 100; ly = -(Y - piv[1]) * 100; lz = (Z - piv[2]) * 100     # UE's OBJ import mirrors Y
    with open(path, 'w') as f:
        f.write(''.join('v %.1f %.1f %.1f\n' % t for t in zip(lx[used], ly[used], lz[used])))
        f.write(''.join('vt %.4f %.4f\n' % t for t in zip((X[used] / 4) % 1000, (Y[used] / 4) % 1000)))
        qy, qx = np.nonzero(quad)
        p, q, s_, t = idx[qy, qx], idx[qy, qx + 1], idx[qy + 1, qx], idx[qy + 1, qx + 1]
        f.write(''.join('f %d/%d %d/%d %d/%d\nf %d/%d %d/%d %d/%d\n' % (a, a, c, c, b, b, c, c, e, e, b, b) for a, b, c, e in zip(p, q, s_, t)))

if __name__ == '__main__':
    lo = float(sys.argv[1]) if len(sys.argv) > 1 else 0; hi = float(sys.argv[2]) if len(sys.argv) > 2 else 1e9
    only = tuple(int(v) for v in sys.argv[3].split(',')) if len(sys.argv) > 3 else None
    tiles = set()
    for p in dense[::5]:
        e = float(spline(p[1], p[0])[0, 0]) + elev_off
        if lo <= e <= hi:
            for dx in (-EDGE_M, 0, EDGE_M):
                for dy in (-EDGE_M, 0, EDGE_M):
                    tiles.add((int((p[0] + dx) // TILE), int((p[1] + dy) // TILE)))
    if only: tiles = {only}
    index_path = os.path.join(OUT, 'tiles.json')
    index = json.load(open(index_path)) if os.path.exists(index_path) else {}
    for k, (ti, tj) in enumerate(sorted(tiles)):
        piv = build_tile(ti, tj)
        if piv: index[f'T_{ti}_{tj}'] = piv
        print(f'{k + 1}/{len(tiles)} T_{ti}_{tj}', 'skipped' if piv is None else 'ok', flush=True)
    json.dump(index, open(index_path, 'w'), indent=0)
