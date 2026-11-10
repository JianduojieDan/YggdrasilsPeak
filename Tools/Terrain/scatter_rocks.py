# Places scanned rock meshes on the v3/v4 terrain region (Saved/DetailTiles/v3/region.npz), the way the reference
# photos show them: outcrops in the rock bands (ribs, crests, saddle), rubble along band edges and in clusters,
# a few snow drifts tucked against rock. Never uniform: density follows the rock map and large cluster noise.
# Each piece is sunk into the ground by part of its height and tilted with the slope, so nothing floats.
# Output: Saved/DetailTiles/v3/scatter.json  [mesh_key, x, y, z_base, yaw_deg, up_x, up_y, scale] (cm); z_base = where the mesh's lowest point goes
import os, json, sys, numpy as np
sys.argv = sys.argv[:1] + ['12867', '15847', '700']
import importlib.util
ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
V3 = os.path.join(ROOT, 'Saved/DetailTiles/v3')
d = np.load(os.path.join(V3, 'region.npz'))
xs, ys, Z, R = d['xs'], d['ys'], d['Z'].astype(float), d['rock'].astype(float)
gy, gx = np.gradient(Z, 1.0)
info = json.load(open(os.path.join(ROOT, 'Saved/Compare/ygg_info.json')))
smp = np.load(os.path.join(ROOT, 'Saved/DetailTiles/corridor_sample.npz'), allow_pickle=True)
props = json.loads(str(smp['props']))
avoid = [(px / 100, py / 100, np.hypot(ex, ey) / 100 + 2.5) for n, px, py, pz, ex, ey, ez in props
         if max(ex, ey) < 3000 and any(k in n for k in ('Tent', 'Depot', 'Stores', 'Landmark', 'O2', 'Clue', 'Ration', 'Water'))]
rope = np.concatenate([np.array(r) / 100 for r in info['ropes'] if len(r) > 1])
from scipy.spatial import cKDTree
rtree = cKDTree(np.concatenate([np.linspace(a, b, max(2, int(np.hypot(*(b - a)))), endpoint=False) for r in info['ropes'] if len(r) > 1 for a, b in zip(np.array(r)[:-1] / 100, np.array(r)[1:] / 100)]))

def h(ix, iy, s):
    v = (np.int64(ix) * 374761393 + np.int64(iy) * 668265263 + s * 2147483647) & 0xFFFFFFFF
    v = ((v ^ (v >> 13)) * 1274126177) & 0xFFFFFFFF
    return ((v ^ (v >> 16)) & 0xFFFFFF) / float(0xFFFFFF)
def at(a, x, y):
    i = int(round(y - ys[0])); j = int(round(x - xs[0]))
    if 0 <= i < a.shape[0] and 0 <= j < a.shape[1]: return a[i, j]
    return None
def cluster(x, y, wl, s):        # smooth 0..1 noise for clumping
    fx, fy = x / wl, y / wl; ix, iy = np.floor(fx), np.floor(fy); u, v = fx - ix, fy - iy
    u, v = u * u * (3 - 2 * u), v * v * (3 - 2 * v)
    return (h(ix, iy, s) * (1 - u) * (1 - v) + h(ix + 1, iy, s) * u * (1 - v) + h(ix, iy + 1, s) * (1 - u) * v + h(ix + 1, iy + 1, s) * u * v)

# mesh key -> (height in m at scale 1, sink fraction)
KINDS = {
    'outcrop': (['stonephoto'], 7.0, (0.6, 1.5), 0.38),   # closed boulders only: the cliff scans are open-backed (hollow from behind)
    'boulder': (['stonephoto'], 7.0, (0.12, 0.45), 0.35),
    'rubble': (['rockysnowpile', 'stonephoto', 'alaskan', 'stone'], 5.0, (0.3, 1.0), 0.30),
    'drift': (['snowpile', 'embankment'], 18.0, (0.6, 1.3), 0.5),
}
SCALE_BASE = {'cliff_a': 1.0, 'cliff_b': 1.0, 'cliff_c': 1.0, 'dolerite': 1.2, 'stonephoto': 1.0, 'rockysnowpile': 1.0,
              'alaskan': 2.2, 'stone': 2.5, 'snowpile': 1.0, 'embankment': 1.0}
FOOT_M = {'cliff_a': 9.8, 'cliff_b': 9.4, 'cliff_c': 6.9, 'dolerite': 5.5, 'stonephoto': 3.5, 'rockysnowpile': 2.0,
          'alaskan': 0.5, 'stone': 0.35, 'snowpile': 3.5, 'embankment': 3.2}   # half footprint at scale 1
HEIGHT_M = {'cliff_a': 14.9, 'cliff_b': 10.1, 'cliff_c': 8.9, 'dolerite': 3.7, 'stonephoto': 2.9, 'rockysnowpile': 0.8,
            'alaskan': 1.4, 'stone': 0.2, 'snowpile': 2.1, 'embankment': 0.6}

out = []
def ok(x, y, pad):
    if rtree.query([x, y])[0] < 3.0 + pad: return False
    return all(np.hypot(x - ax, y - ay) > r + pad for ax, ay, r in avoid)
def place(kind, key, x, y, sc, s):
    z = at(Z, x, y)
    if z is None: return
    ga, gb = at(gx, x, y), at(gy, x, y)
    tilt = 0.6                                                         # lean with the slope, not fully
    hm = HEIGHT_M[key] * sc
    sink = KINDS[kind][3] + 0.15 * h(int(x), int(y), s + 9)
    # footprint check: the ground under the whole base must stay within the sunk part, or an edge would float
    rf = FOOT_M[key] * sc
    worst = 0.0
    for k in range(12):
        a = k * np.pi / 6
        for f in (0.5, 1.0):
            px, py = x + np.cos(a) * rf * f, y + np.sin(a) * rf * f
            zz = at(Z, px, py)
            if zz is None: return
            plane = z + (ga * (px - x) + gb * (py - y)) * tilt
            worst = max(worst, plane - zz)                              # ground lower than the tilted base
    if worst > max(hm * sink * 0.7, 0.12): return
    yaw = h(int(x * 7), int(y * 7), s) * 360.0
    out.append([key, x * 100, y * 100, (z - hm * sink) * 100, yaw, -ga * tilt, -gb * tilt, sc])

for gy_ in np.arange(ys[0] + 60, ys[-1] - 60, 24.0):                    # big boulders in rock bands, clumped
    for gx_ in np.arange(xs[0] + 60, xs[-1] - 60, 24.0):
        ix, iy = int(gx_), int(gy_)
        x = gx_ + (h(ix, iy, 1) - 0.5) * 14; y = gy_ + (h(ix, iy, 2) - 0.5) * 14
        r = at(R, x, y)
        if r is None or r < 0.6 or h(ix, iy, 3) > 0.45 * cluster(x, y, 90, 4) ** 2 + 0.05 or not ok(x, y, 6): continue
        keys = KINDS['outcrop'][0]; key = keys[int(h(ix, iy, 5) * len(keys)) % len(keys)]
        lo, hi = KINDS['outcrop'][2]; sc = (lo + (hi - lo) * h(ix, iy, 6) ** 1.6) * SCALE_BASE[key]
        place('outcrop', key, x, y, sc, 7)
for gy_ in np.arange(ys[0] + 50, ys[-1] - 50, 4.0):                     # rubble: band edges and clumps
    for gx_ in np.arange(xs[0] + 50, xs[-1] - 50, 4.0):
        ix, iy = int(gx_), int(gy_)
        x = gx_ + (h(ix, iy, 11) - 0.5) * 3.8; y = gy_ + (h(ix, iy, 12) - 0.5) * 3.8
        r = at(R, x, y)
        if r is None: continue
        edge = 1 - abs(r - 0.5) * 2                                      # strongest along rock-band edges
        want = 0.45 * edge * cluster(x, y, 35, 13) + 0.25 * r * cluster(x, y, 60, 14) ** 2
        if h(ix, iy, 15) > want or not ok(x, y, 1.5): continue
        big = h(ix, iy, 16) > 0.9
        kind = 'boulder' if big else 'rubble'
        keys = KINDS[kind][0]; key = keys[int(h(ix, iy, 17) * len(keys)) % len(keys)]
        lo, hi = KINDS[kind][2]; sc = (lo + (hi - lo) * h(ix, iy, 18) ** 2.2) * SCALE_BASE[key]
        if key == 'stonephoto' and kind == 'rubble': sc *= 0.18
        place(kind, key, x, y, sc, 19)
for gy_ in np.arange(ys[0] + 60, ys[-1] - 60, 28.0):                    # snow drifts against rock, rare
    for gx_ in np.arange(xs[0] + 60, xs[-1] - 60, 28.0):
        ix, iy = int(gx_), int(gy_)
        x = gx_ + (h(ix, iy, 21) - 0.5) * 24; y = gy_ + (h(ix, iy, 22) - 0.5) * 24
        r = at(R, x, y)
        if r is None or not (0.05 < r < 0.35) or h(ix, iy, 23) > 0.18 or not ok(x, y, 4): continue
        keys = KINDS['drift'][0]; key = keys[int(h(ix, iy, 24) * 2) % 2]
        lo, hi = KINDS['drift'][2]; place('drift', key, x, y, lo + (hi - lo) * h(ix, iy, 25), 26)
json.dump(out, open(os.path.join(V3, 'scatter.json'), 'w'))
from collections import Counter
print(len(out), Counter(o[0] for o in out))
