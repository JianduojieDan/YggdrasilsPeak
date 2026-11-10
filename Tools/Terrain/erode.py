# Offline alpine erosion on the clean landscape heights (Saved/TerrainMathBase_clean.bin, 2017x2017 uint16).
# Stream-power incision (water/ice cut gullies, ridges sharpen between them) + thermal slumping (scree below steep faces).
# Route, ropes and camps are protected exactly like LandscapeRebuilder::ApplyTerrainMath.
# Usage: python erode.py <info.json> <out.bin> [preview_dir]
import sys, json, os, numpy as np
from scipy.ndimage import gaussian_filter

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
N = 2017
P = dict(iters=24, K=0.0016, m=0.5, max_cut_m=1.0, talus_deg=40.0, thermal_rate=0.15, jitter_m=4.0, cut_blur=1.2,
         above_m=4300.0, full_above_m=5200.0, route_clear_m=30.0, camp_clear_m=120.0, keep_summit_m=25.0)

info = json.load(open(sys.argv[1]))
locx, locy, locz = info['loc']; sx, sy, sz = info['scale']
cm_per_unit = sz / 128.0; sp = sx / 100.0   # metres between vertices
base = np.fromfile(os.path.join(ROOT, 'Saved/TerrainMathBase_clean.bin'), np.uint16).reshape(N, N).astype(np.float64)
h = (base - 32768.0) * cm_per_unit / 100.0          # metres, relative
elev = h - h.max() + 8849.0                           # true elevation (summit = 8849 m)

# --- protection map (1 = erode fully, 0 = untouched)
protect = np.ones((N, N))
yy, xx = np.mgrid[0:N, 0:N]
def stamp(wx, wy, clear, full):
    gx, gy = (wx - locx) / sx, (wy - locy) / sy
    r = int(np.ceil(full / sp)) + 1
    x0, x1 = max(0, int(gx) - r), min(N, int(gx) + r + 1); y0, y1 = max(0, int(gy) - r), min(N, int(gy) + r + 1)
    if x0 >= x1 or y0 >= y1: return
    d = np.hypot(xx[y0:y1, x0:x1] - gx, yy[y0:y1, x0:x1] - gy) * sp
    t = np.clip((d - clear) / (full - clear), 0, 1); s = t * t * (3 - 2 * t)
    protect[y0:y1, x0:x1] = np.minimum(protect[y0:y1, x0:x1], s)
for leg in json.load(open(os.path.join(ROOT, 'Tools/RoutePlanning/route.json'))):
    for p in leg['pts']: stamp(p[0], p[1], P['route_clear_m'], P['route_clear_m'] * 3)
for rope in info['ropes']:
    for p in rope: stamp(p[0], p[1], P['route_clear_m'], P['route_clear_m'] * 3)
for _, x, y in info['camps']: stamp(x, y, P['camp_clear_m'], P['camp_clear_m'] * 2.5)
py, px = np.unravel_index(np.argmax(h), h.shape)
d_peak = np.hypot(xx - px, yy - py) * sp
t = np.clip((d_peak - P['keep_summit_m']) / (P['keep_summit_m'] * 2), 0, 1); protect *= t * t * (3 - 2 * t)
t = np.clip((elev - P['above_m']) / (P['full_above_m'] - P['above_m']), 0, 1); protect *= t * t * (3 - 2 * t)

# --- erosion
OFF = [(-1, -1), (-1, 0), (-1, 1), (0, -1), (0, 1), (1, -1), (1, 0), (1, 1)]
DIST = np.array([np.hypot(a, b) for a, b in OFF]) * sp
def neighbours(z):
    pad = np.pad(z, 1, mode='edge')
    return np.stack([pad[1 + a:N + 1 + a, 1 + b:N + 1 + b] for a, b in OFF])
def flow_acc(z):
    nb = neighbours(z)
    drop = (z[None] - nb) / DIST[:, None, None]
    k = np.argmax(drop, 0); s = np.take_along_axis(drop, k[None], 0)[0]
    dy = np.array([o[0] for o in OFF])[k]; dx = np.array([o[1] for o in OFF])[k]
    rcv = (np.clip(yy + dy, 0, N - 1) * N + np.clip(xx + dx, 0, N - 1)).ravel()
    rcv[(s <= 0).ravel()] = -1
    order = np.argsort(-z.ravel(), kind='stable')
    acc = np.ones(N * N)
    # high to low: push each cell's water to its receiver (vectorised in height bands that cannot feed each other in-band)
    ro = rcv[order]
    for i, c in zip(order.tolist(), ro.tolist()):
        if c >= 0: acc[c] += acc[i]
    return acc.reshape(N, N), np.maximum(s, 0)

z = h.copy()
talus = np.tan(np.radians(P['talus_deg']))
for it in range(P['iters']):
    rng = np.random.default_rng(it)
    acc, s = flow_acc(z + gaussian_filter(rng.normal(0, 1, z.shape), 2.0) * P['jitter_m'])
    acc = gaussian_filter(acc, P['cut_blur'])
    area = acc * sp * sp
    cut = np.minimum(P['K'] * area ** P['m'] * s, P['max_cut_m']) * protect
    z -= gaussian_filter(cut, P['cut_blur'])
    # thermal: material above talus angle slides to the lowest neighbour
    nb = neighbours(z)
    drop = (z[None] - nb) / DIST[:, None, None]
    k = np.argmax(drop, 0); dmax = np.take_along_axis(drop, k[None], 0)[0]
    excess = np.where(dmax > talus, (dmax - talus) * DIST[k] * 0.5 * P['thermal_rate'], 0) * protect
    z -= excess
    for j, (a, b) in enumerate(OFF):
        m = (k == j) & (excess > 0)
        ys, xs = np.nonzero(m)
        np.add.at(z, (np.clip(ys + a, 0, N - 1), np.clip(xs + b, 0, N - 1)), excess[ys, xs])
    print(f'iter {it + 1}/{P["iters"]}: mean cut {cut[protect > 0.5].mean():.3f} m, max change so far {np.abs(z - h).max():.1f} m', flush=True)

delta = z - h
print('change m: min %.1f max %.1f, p99 abs %.1f' % (delta.min(), delta.max(), np.percentile(np.abs(delta), 99)))
out = np.clip(np.round(base + delta * 100.0 / cm_per_unit), 0, 65535).astype(np.uint16)
out.tofile(sys.argv[2])

if len(sys.argv) > 3:   # hillshade previews: whole map and summit 6 km crop, before/after
    import matplotlib; matplotlib.use('Agg'); import matplotlib.pyplot as plt
    def shade(a):
        gy_, gx_ = np.gradient(a, sp); az, alt = np.radians(315), np.radians(35)
        sl = np.arctan(np.hypot(gx_, gy_)); asp = np.arctan2(-gx_, gy_)
        return np.sin(alt) * np.cos(sl) + np.cos(alt) * np.sin(sl) * np.cos(az - asp)
    r = int(3000 / sp)
    for name, sl_ in [('summit', np.s_[py - r:py + r, px - r:px + r]), ('map', np.s_[:, :])]:
        fig, ax = plt.subplots(1, 2, figsize=(16, 8))
        for a_, arr, t_ in [(ax[0], h, 'before'), (ax[1], z, 'after')]:
            a_.imshow(shade(arr[sl_]), cmap='gray', vmin=0, vmax=1); a_.set_title(t_); a_.axis('off')
        plt.tight_layout(); plt.savefig(os.path.join(sys.argv[3], f'erode_{name}.png'), dpi=90); plt.close()
