# Terrain v3: 1 m detail tiles that REPLACE the landscape in a region (the landscape is sunk under them),
# shaped from the reference photos in Docs/ArtDirection/terrain_refs:
#   snow   = smooth large curves + crisp, lopsided wind ridges in patches (never dents everywhere)
#   rubble = dark angular stones and hummocks with snow filling the gaps (South Col, crests, exposed ground)
#   big    = uneven rises and hollows 90-220 m across, strength varying from calm to rough
#   route  = a trodden boot trench along fixed ropes; tents stand on levelled stony ground
# Rock exposure is written to UV0.x (1 = bare rock, 0 = snow) for the tile material.
# Input:  Saved/DetailTiles/corridor_sample.npz (landscape heights + props), route.json, Saved/Compare/ygg_info.json
# Output: Saved/DetailTiles/v3/<name>.obj, tiles.json (pivots), landscape_patch.bin (sink heights for ApplyHeightPatch)
# Usage:  python build_terrain_v3.py <centerX_m> <centerY_m> <half_size_m> [name_prefix]
import sys, os, json, numpy as np
from scipy.interpolate import RectBivariateSpline
from scipy.ndimage import gaussian_filter, distance_transform_edt, minimum_filter, laplace
from scipy.spatial import cKDTree

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
SRC = os.path.join(ROOT, 'Saved/DetailTiles'); OUT = os.path.join(SRC, 'v3'); os.makedirs(OUT, exist_ok=True)
LX = LY = -5040.0; S = 16.8712; LOCZ = 1389.5695625; CM_PER_UNIT = 951.4 / 128.0      # landscape transform (m, cm)
TILE = 500; FADE_M = 100.0; CLEAR_M = 0.6

cx, cy, half = float(sys.argv[1]), float(sys.argv[2]), float(sys.argv[3])
prefix = sys.argv[4] if len(sys.argv) > 4 else 'V3'

smp = np.load(os.path.join(SRC, 'corridor_sample.npz'), allow_pickle=True)
G = smp['heights'] / 100.0; I0, J0 = int(smp['i0']), int(smp['j0'])
props = json.loads(str(smp['props']))
if np.isnan(G).any():
    m = np.isnan(G); ii = distance_transform_edt(m, return_distances=False, return_indices=True)
    G = np.where(m, gaussian_filter(G[tuple(ii)], 1.5), G)
GX = LX + (I0 + np.arange(G.shape[1])) * S; GY = LY + (J0 + np.arange(G.shape[0])) * S
spline = RectBivariateSpline(GY, GX, G, kx=3, ky=3)
ropes = [np.array(r) / 100.0 for r in json.load(open(os.path.join(ROOT, 'Saved/Compare/ygg_info.json')))['ropes']]
rope_pts = np.concatenate([np.concatenate([np.linspace(a, b, max(2, int(np.hypot(*(b - a)) / 1.0)), endpoint=False) for a, b in zip(r[:-1], r[1:])]) for r in ropes if len(r) > 1])
rtree = cKDTree(rope_pts)
camps = [(x / 100, y / 100) for _, x, y in json.load(open(os.path.join(ROOT, 'Saved/Compare/ygg_info.json')))['camps']]
tents = [(px / 100, py / 100, np.hypot(ex, ey) / 100) for name, px, py, pz, ex, ey, ez in props
         if max(ex, ey) < 3000 and any(k in name for k in ('Tent', 'Depot', 'Stores', 'Landmark'))]
items = [(px / 100, py / 100, max(np.hypot(ex, ey) / 100, 0.5)) for name, px, py, pz, ex, ey, ez in props if max(ex, ey) < 3000]

# ---------- world-coherent noise (same everywhere, so tiles join without seams)
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

def stones(X, Y, cell, prob, hmax, rad, seed):
    """Angular stones: each a rotated, slightly squashed block with a flat-ish top and broken edges."""
    out = np.full(X.shape, -1e3)
    cx0, cy0 = np.floor(X / cell), np.floor(Y / cell)
    for ox in (-1, 0, 1):
        for oy in (-1, 0, 1):
            ix, iy = cx0 + ox, cy0 + oy
            px = (ix + _hash(ix, iy, seed)) * cell; py = (iy + _hash(ix, iy, seed + 1)) * cell
            on = _hash(ix, iy, seed + 2) < prob
            h = (0.25 + 0.75 * _hash(ix, iy, seed + 3) ** 1.5) * hmax
            r = rad * (0.5 + 0.9 * _hash(ix, iy, seed + 4)); ang = _hash(ix, iy, seed + 5) * np.pi
            sq = 0.6 + 0.4 * _hash(ix, iy, seed + 6)
            dx, dy = X - px, Y - py
            u = (dx * np.cos(ang) + dy * np.sin(ang)) / r; v = (-dx * np.sin(ang) + dy * np.cos(ang)) / (r * sq)
            dd = 0.65 * np.maximum(np.abs(u), np.abs(v)) + 0.35 * np.hypot(u, v)          # blocky, not round
            prof = np.clip(1 - dd, 0, 1) ** 0.45                                           # steep sides, flattish top
            out = np.maximum(out, np.where(on & (dd < 1), h * prof, -1e3))
    return out

def surface(X, Y):
    """v4, calm: real snow is mostly smooth; rock only where the mountain puts it (ribs, crests, steep ground, the saddle),
    in a few connected bands. Fine rock detail comes from scanned rock meshes placed on top, not from noise."""
    smooth = spline(Y[:, 0], X[0])
    gy, gx = np.gradient(smooth, 1.0); slope = np.hypot(gx, gy)
    steep = sstep(0.6, 1.0, slope)                                     # ~31..45 deg
    calm = 0.3 + 0.7 * sstep(-0.5, 0.8, octave(X, Y, 500, 1, 101))
    big = (octave(X, Y, 260, 5.0, 102) + octave(X, Y, 110, 1.8, 103)) * calm
    base = smooth + big
    curv = -laplace(gaussian_filter(base, 20.0))
    curv = curv / (np.std(curv) + 1e-9)
    ax, ay = camps[7]
    saddle = 1 - sstep(120, 320, np.hypot(X - ax, Y - ay))
    score = 0.9 * curv + 0.55 * octave(X, Y, 120, 1, 105) + 1.6 * steep + 3.0 * saddle
    thr = np.percentile(score, 80)                                     # about a fifth of the ground is rock
    soft = gaussian_filter((score > thr).astype(float), 10.0)          # wide, gradual band edges
    rock = sstep(0.2, 0.8, soft)
    snow = octave(X, Y, 40, 0.35, 111) + octave(X, Y, 14, 0.07, 112)
    warp = octave(X, Y, 30, 6.0, 113) + octave(X, Y, 80, 15.0, 114)
    ph = (X + warp) / 5.0 + octave(X, Y, 50, 1.2, 115)
    saw = ph % 1.0; saw = np.where(saw < 0.82, saw / 0.82, (1 - saw) / 0.18)
    snow += (saw ** 1.6) * 0.10 * sstep(0.5, 0.9, octave(X, Y, 45, 1, 116))   # wind ridges: faint, rare patches
    f = octave(X, Y, 22, 1, 121)
    rockrel = (np.clip(1 - np.abs(f) * 0.7, 0, 1) ** 2 - 0.4) * 1.1 + octave(X, Y, 6, 0.2, 122)
    detail = snow * (1 - rock) + rockrel * rock
    # what the eye sees as rock: ragged edges, snow lying in every dip, stone poking through thin snow
    hollow = gaussian_filter(detail, 4.0) - detail                      # >0 in dips
    breakup = octave(X, Y, 9, 0.25, 141) + octave(X, Y, 3.5, 0.12, 142) + octave(X, Y, 25, 0.15, 143)
    rockw = sstep(0.3, 0.7, soft + breakup - hollow * 0.6 + 0.25 * saddle)   # the South Col stays stony right up to the tents
    surface.score = score; surface.thr = thr
    return smooth, big, detail, rockw

def keep_maps(X, Y):
    k_big = np.ones_like(X); k_small = np.ones_like(X)
    rd = rtree.query(np.c_[X.ravel(), Y.ravel()])[0].reshape(X.shape)
    k_big = np.minimum(k_big, sstep(15, 45, rd)); k_small = np.minimum(k_small, 0.25 + 0.75 * sstep(1.5, 6, rd))
    trench = (1 - sstep(0.6, 1.6, rd)) * 0.35
    for (ax, ay, r) in tents:
        d = np.hypot(X - ax, Y - ay)
        k_big = np.minimum(k_big, sstep(r + 20, r + 70, d)); k_small = np.minimum(k_small, 0.2 + 0.8 * sstep(r + 1, r + 8, d))
    return k_big, k_small, trench

def lin_surface(X, Y):
    fx = (X - GX[0]) / S; fy = (Y - GY[0]) / S
    i = np.clip(np.floor(fx).astype(int), 0, len(GX) - 2); j = np.clip(np.floor(fy).astype(int), 0, len(GY) - 2)
    u, v = fx - i, fy - j; h00, h10, h01, h11 = G[j, i], G[j, i + 1], G[j + 1, i], G[j + 1, i + 1]
    ta = np.where(u + v <= 1, h00 + (h10 - h00) * u + (h01 - h00) * v, h11 + (h01 - h11) * (1 - u) + (h10 - h11) * (1 - v))
    tb = np.where(u >= v, h00 + (h10 - h00) * u + (h11 - h10) * v, h00 + (h01 - h00) * v + (h11 - h01) * u)
    return np.maximum(ta, tb)

# ---------- build the whole region at 1 m, then cut tiles
x0, x1, y0, y1 = cx - half, cx + half, cy - half, cy + half
xs = np.arange(np.floor(x0), np.ceil(x1) + 1); ys = np.arange(np.floor(y0), np.ceil(y1) + 1)
X, Y = np.meshgrid(xs, ys)
smooth, big, detail, rockw = surface(X, Y)
k_big, k_small, trench = keep_maps(X, Y)
edge = np.minimum.reduce([X - xs[0], xs[-1] - X, Y - ys[0], ys[-1] - Y])
fade = sstep(0, FADE_M, edge)
lin = lin_surface(X, Y)
Z = smooth + (big * k_big + detail * k_small - trench) * fade
Z = lin + (Z - lin) * fade                                              # edge meets the landscape exactly
rockw = rockw * fade
print('Z - landscape: min %.1f max %.1f m; rock share %.0f%%' % ((Z - lin).min(), (Z - lin).max(), 100 * (rockw > 0.5).mean()))

# ---------- landscape sink patch: every vertex whose surrounding cells are covered goes CLEAR_M under the tile minimum
li0 = int(np.ceil((x0 - LX) / S)) + 1; li1 = int(np.floor((x1 - LX) / S)) - 1
lj0 = int(np.ceil((y0 - LY) / S)) + 1; lj1 = int(np.floor((y1 - LY) / S)) - 1
zmin = minimum_filter(Z, size=int(2 * S) + 3)
tgt = np.full((lj1 - lj0 + 1, li1 - li0 + 1), -1.0, np.float32)
for jj in range(lj0, lj1 + 1):
    for ii in range(li0, li1 + 1):
        vx, vy = LX + ii * S, LY + jj * S
        px, py = int(round(vx - xs[0])), int(round(vy - ys[0]))
        zc = (zmin[py, px] - CLEAR_M) * 100.0
        tgt[jj - lj0, ii - li0] = 32768.0 + (zc - LOCZ * 100.0) / CM_PER_UNIT
with open(os.path.join(OUT, 'landscape_patch.bin'), 'wb') as f:
    np.array([li0, lj0, tgt.shape[1], tgt.shape[0]], np.int32).tofile(f); tgt.tofile(f)

# ---------- tiles (500 m), UV0 = (rock, 0)
index = {}
for ty in range(int(np.floor(y0 / TILE)), int(np.ceil(y1 / TILE))):
    for tx in range(int(np.floor(x0 / TILE)), int(np.ceil(x1 / TILE))):
        a0, a1 = max(tx * TILE, xs[0]), min((tx + 1) * TILE, xs[-1]); b0, b1 = max(ty * TILE, ys[0]), min((ty + 1) * TILE, ys[-1])
        if a1 <= a0 or b1 <= b0: continue
        sl = np.s_[int(b0 - ys[0]):int(b1 - ys[0]) + 1, int(a0 - xs[0]):int(a1 - xs[0]) + 1]
        TX, TY, TZ, TR = X[sl], Y[sl], Z[sl], rockw[sl]
        name = f'{prefix}_{tx}_{ty}'; piv = [(a0 + a1) / 2, (b0 + b1) / 2, float(TZ.min())]
        n0, n1 = TX.shape; idx = np.arange(n0 * n1).reshape(n0, n1) + 1
        with open(os.path.join(OUT, name + '.obj'), 'w') as f:
            f.write(''.join('v %.1f %.1f %.1f\n' % t for t in zip(((TX - piv[0]) * 100).ravel(), (-(TY - piv[1]) * 100).ravel(), ((TZ - piv[2]) * 100).ravel())))
            f.write(''.join('vt %.4f 0\n' % r for r in TR.ravel()))
            p, q, s_, t = idx[:-1, :-1].ravel(), idx[:-1, 1:].ravel(), idx[1:, :-1].ravel(), idx[1:, 1:].ravel()
            f.write(''.join('f %d/%d %d/%d %d/%d\nf %d/%d %d/%d %d/%d\n' % (a, a, c, c, b, b, c, c, e, e, b, b) for a, b, c, e in zip(p, q, s_, t)))
        index[name] = piv
        print(name, TX.shape, flush=True)
json.dump(index, open(os.path.join(OUT, 'tiles.json'), 'w'), indent=0)
np.savez_compressed(os.path.join(OUT, 'region.npz'), xs=xs, ys=ys, Z=Z.astype(np.float32), rock=rockw.astype(np.float16), score=(surface.score - surface.thr).astype(np.float16))
