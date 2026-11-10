# Builds a 1 m resolution detail terrain tile (OBJ, cm, Z up) over the coarse 17 m landscape.
# Input: Saved/Compare/<name>_sample.json from the editor (landscape heights at its own vertices + nearby props).
# The tile follows the landscape smoothly (cubic), adds player-scale relief (lumps, rock steps, wind ridges),
# never dips below the landscape, keeps tents/ropes flat, and fades to the landscape at its edges.
# Usage: python build_detail_tile.py <sample.json> <out.obj> <centerX_cm> <centerY_cm> [half_m=300] [preview.png]
import sys, json, numpy as np
from scipy.interpolate import RectBivariateSpline
from scipy.ndimage import gaussian_filter

LX = LY = -504000.0; S = 1687.12        # landscape origin / vertex spacing, cm
d = json.load(open(sys.argv[1])); cx, cy = float(sys.argv[3]), float(sys.argv[4])
half = float(sys.argv[5]) if len(sys.argv) > 5 else 300.0
g = np.array([[np.nan if v is None else v for v in r] for r in d['grid']]) / 100.0   # metres
# fill missing samples from neighbours
while np.isnan(g).any():
    m = np.isnan(g); p = np.pad(g, 1, mode='edge')
    nb = np.stack([p[:-2, 1:-1], p[2:, 1:-1], p[1:-1, :-2], p[1:-1, 2:]])
    g[m] = np.nanmean(nb, 0)[m]
gx = (LX + (d['i0'] + np.arange(g.shape[1])) * S) / 100.0
gy = (LY + (d['j0'] + np.arange(g.shape[0])) * S) / 100.0
spline = RectBivariateSpline(gy, gx, g, kx=3, ky=3)

n = int(2 * half) + 1
xs = cx / 100.0 + np.linspace(-half, half, n); ys = cy / 100.0 + np.linspace(-half, half, n)
X, Y = np.meshgrid(xs, ys)
smooth = spline(ys, xs)
# exact landscape surface is piecewise linear; take the higher of both triangle splits so we never go under it
fx = (X - gx[0]) / (S / 100); fy = (Y - gy[0]) / (S / 100)
i = np.clip(np.floor(fx).astype(int), 0, len(gx) - 2); j = np.clip(np.floor(fy).astype(int), 0, len(gy) - 2)
u = fx - i; v = fy - j
h00, h10, h01, h11 = g[j, i], g[j, i + 1], g[j + 1, i], g[j + 1, i + 1]
ta = np.where(u + v <= 1, h00 + (h10 - h00) * u + (h01 - h00) * v, h11 + (h01 - h11) * (1 - u) + (h10 - h11) * (1 - v))
tb = np.where(u >= v, h00 + (h10 - h00) * u + (h11 - h10) * v, h00 + (h01 - h00) * v + (h11 - h01) * u)
lin = np.maximum(ta, tb)

rng = np.random.default_rng(7)
def octave(wl, amp, ridged=False):
    k = max(1, int(round(wl / 2)))
    f = gaussian_filter(rng.normal(0, 1, (n + 4 * k, n + 4 * k)), wl / 2.5, mode='wrap')[2 * k:2 * k + n, 2 * k:2 * k + n]
    f /= f.std() + 1e-9
    if ridged: f = 1.0 - np.abs(f) * 1.4
    return f * amp
gyy, gxx = np.gradient(smooth, 1.0)
slope = np.hypot(gxx, gyy)                              # rise/run
steep = np.clip((slope - 0.3) / 0.5, 0, 1)              # ~17 deg -> ~39 deg
def sstep(a, b, x): t = np.clip((x - a) / (b - a), 0, 1); return t * t * (3 - 2 * t)
# zones: 0 = wind-packed snow, 1 = broken rock and rubble (patches, plus everything steep)
zone = np.maximum(sstep(-0.2, 0.5, octave(90, 1.0) + 0.35 * octave(25, 1.0)), steep)
# --- snow: big drifts + knife-edged wind ridges (sastrugi), wind from the west with wandering direction
drift = octave(35, 1.4) + octave(12, 0.5)
warp = octave(25, 8.0) + octave(70, 20.0)
ph = (X + warp) / 4.5 + octave(40, 1.5)
saw = (ph % 1.0); saw = np.where(saw < 0.8, saw / 0.8, (1 - saw) / 0.2)          # gentle windward, steep lee face
sastrugi = (saw ** 2 - 0.35) * 0.55 * sstep(-0.3, 0.6, octave(30, 1.0))
snow = drift + sastrugi
# --- rock: sharp ridged relief + ledges + scattered boulders
def ridged(wl, amp, power=2.0):
    f = octave(wl, 1.0); return (np.clip(1.0 - np.abs(f) * 0.6, 0, 1) ** power - 0.4) * amp
rock = ridged(45, 3.2) + ridged(14, 1.3, 2.5) + ridged(4.5, 0.45, 3.0) + octave(1.6, 0.08)
step_h = 2.2
terr = np.floor((smooth + rock) / step_h) * step_h + step_h * sstep(0.7, 1.0, ((smooth + rock) / step_h) % 1)
rock += (terr - (smooth + rock)) * 0.7 * sstep(-0.2, 0.4, octave(35, 1.0))
from scipy.ndimage import maximum_filter, grey_dilation
for size, dens, hmax in [(9, 0.0012, 2.6), (5, 0.004, 1.3), (3, 0.012, 0.5)]:   # boulders: cell size m, density, height
    pts = np.where(rng.random((n, n)) < dens, rng.random((n, n)) * hmax, 0.0)
    yy_, xx_ = np.mgrid[-size:size + 1, -size:size + 1]
    dome = -(np.hypot(xx_, yy_) / size) ** 2 * hmax; dome[np.hypot(xx_, yy_) > size] = -1e3
    rock = np.maximum(rock, grey_dilation(pts, structure=dome) - 0.3 + octave(2, 0.12))
detail = snow * (1 - zone) + rock * zone

# keep tents, camp items and fixed ropes walkable; fade at the tile edge
keep = np.ones_like(X)
for name, px, py, pz, ex, ey, ez in d['props']:
    if max(ex, ey) > 3000: continue      # long ropes: their bounds cover the whole tile
    if 'Tent' in name or 'Depot' in name or 'Rope' in name or 'Landmark' == name.split('_')[0]:
        r = np.hypot(ex, ey) / 100.0 + 3.0
        dist = np.hypot(X - px / 100.0, Y - py / 100.0)
        keep = np.minimum(keep, np.clip((dist - r) / 12.0, 0, 1))
if len(sys.argv) > 7:                    # info json with fixed rope points
    for rope in json.load(open(sys.argv[7]))['ropes']:
        for rx, ry in rope:
            rx, ry = rx / 100.0, ry / 100.0
            if abs(rx - cx / 100.0) < half + 20 and abs(ry - cy / 100.0) < half + 20:
                r0 = max(0, int((rx - xs[0])) - 12); r1 = int(rx - xs[0]) + 13; c0 = max(0, int(ry - ys[0]) - 12); c1 = int(ry - ys[0]) + 13
                sl_ = np.s_[c0:c1, r0:r1]
                keep[sl_] = np.minimum(keep[sl_], np.clip((np.hypot(X[sl_] - rx, Y[sl_] - ry) - 2.0) / 8.0, 0, 1))
edge = np.minimum.reduce([X - xs[0], xs[-1] - X, Y - ys[0], ys[-1] - Y])
fade = np.clip(edge / 40.0, 0, 1); fade = fade * fade * (3 - 2 * fade)
# sit on top of the landscape: shift so almost all of the relief is above it, soft floor for the rest (no flat cut-offs)
d0 = detail - np.percentile(detail, 2)
lift = np.log1p(np.exp(3.0 * d0)) / 3.0
under = gaussian_filter(maximum_filter(np.maximum(lin - smooth, 0), 25), 8)   # smooth cover over the landscape facets
z = np.maximum(smooth + under + 0.05 + lift * keep * fade, lin + 0.03)
print('detail range m: %.2f..%.2f, lift over landscape mean %.2f max %.2f' % (detail.min(), detail.max(), (z - lin).mean(), (z - lin).max()))

# OBJ in cm, relative to the tile centre and its lowest point (pivot); UV = 1 per 4 m
zc = z.min()
lx = (X - cx / 100.0) * 100; ly = (Y - cy / 100.0) * 100; lz = (z - zc) * 100
with open(sys.argv[2], 'w') as f:
    f.write('# detail tile\n')
    f.write(''.join('v %.1f %.1f %.1f\n' % t for t in zip(lx.ravel(), -ly.ravel(), lz.ravel())))
    f.write(''.join('vt %.4f %.4f\n' % t for t in zip((X.ravel() / 4) % 1000, (Y.ravel() / 4) % 1000)))
    idx = np.arange(n * n).reshape(n, n) + 1
    a, b, c, e = idx[:-1, :-1].ravel(), idx[:-1, 1:].ravel(), idx[1:, :-1].ravel(), idx[1:, 1:].ravel()
    f.write(''.join('f %d/%d %d/%d %d/%d\nf %d/%d %d/%d %d/%d\n' % (p, p, q, q, s_, s_, q, q, t, t, s_, s_) for p, q, s_, t in zip(a, c, b, e)))
print('pivot cm', cx, cy, zc * 100)
json.dump(dict(pivot=[cx, cy, zc * 100]), open(sys.argv[2] + '.json', 'w'))
if len(sys.argv) > 6:
    import matplotlib; matplotlib.use('Agg'); import matplotlib.pyplot as plt
    def shade(a):
        gy_, gx_ = np.gradient(a); sl = np.arctan(np.hypot(gx_, gy_)); asp = np.arctan2(-gx_, gy_)
        return np.sin(0.6) * np.cos(sl) + np.cos(0.6) * np.sin(sl) * np.cos(np.radians(315) - asp)
    fig, ax = plt.subplots(1, 2, figsize=(16, 8))
    for a_, arr, t_ in [(ax[0], lin, 'landscape now (17 m)'), (ax[1], z, 'detail tile (1 m)')]:
        a_.imshow(shade(arr[::-1]), cmap='gray', vmin=0, vmax=1); a_.set_title(t_); a_.axis('off')
    plt.tight_layout(); plt.savefig(sys.argv[6], dpi=80)
