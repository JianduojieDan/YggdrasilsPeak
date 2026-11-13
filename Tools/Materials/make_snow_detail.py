"""make_snow_detail.py - procedural, tileable snow DETAIL texture sets (2026-11-14).

Why: the snow reads blurry next to the scanned rock (Docs/Development/Snow_Diagnosis_2026-11-14.md, P2/P3).
No downloads allowed tonight, so we make our own maps. The old ClumpSnow texture has round clumps on a grid,
the wrong character for wind-packed Himalayan snow.

Two sets, both 2048 x 2048 and seamless (everything is built on a torus: FFT noise, periodic Voronoi with a
wrapped KD-tree, wrap-mode filters, wrapped finite differences):

  SnowFine   2 m x 2 m  (0.977 mm/texel)  wind-packed crust: 1-3 mm sugar grain, 1-3 cm angular crust facets,
                                          sparse 2-10 cm crumbs and thin plates with small sharp broken edges.
  SnowRipple 3 m x 3 m  (1.465 mm/texel)  wind ripples, crests across the wind (wind = +X / +U), 8-25 cm
                                          wavelength varying smoothly, branching / merging / ending crests,
                                          gentle windward rise, short steep lee face, crisp crest, patchy amplitude.

Map conventions (also written to Saved/SnowTex/README.md):
  *_H.png    16-bit grey height, 0.5 = mean height, full range = +-HEIGHT_CM (see constants below).
  *_N.png    8-bit tangent-space normal, DirectX convention (green = +V = DOWN the image), what UE's
             TC_Normalmap expects with "flip green" OFF. Derived from the height at its real physical size.
  *_HRA.png  8-bit RGB: R = height, G = roughness (0.70-0.92, crests smoother), B = AO (0.70-1.00, cavities dark).

Run:  python -I Tools/Materials/make_snow_detail.py [fine|ripple|all]   (needs numpy, scipy, Pillow; ~30 s)
      set YGG_SNOW_DEBUG=<dir> to also dump 1:1 lit crops, a 4x4 tiling and a ripple+fine combined view
Output: F:/UE_project/YggdrasilsPeak/Saved/SnowTex/
"""
import os
import sys
import time

import numpy as np
from PIL import Image
from scipy import ndimage
from scipy.spatial import cKDTree

OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "Saved", "SnowTex")
OUT = os.path.normpath(OUT)
N = 2048

FINE_SIZE_CM = 200.0
FINE_HEIGHT_CM = 0.6        # height map 0..1 = -0.6 .. +0.6 cm
RIPPLE_SIZE_CM = 300.0
RIPPLE_HEIGHT_CM = 1.5      # height map 0..1 = -1.5 .. +1.5 cm

# SnowFine layer sizes (px ~ mm) and relief (mm, before the final scale to +-FINE_HEIGHT_CM)
GRAIN_MM = 0.14
CRUST_SPACING_PX = 15.0
CRUST_MM = 0.45
PLATE_SPACING_PX = 52.0
PLATE_FRACTION = 0.18
PLATE_WARP_PX = 2.5
PLATE_MM = 2.6
CRUMB_MM = 2.6

# SnowRipple
RIP_LAMBDA_CM = 13.5        # base wavelength
RIP_DEFECT_SIGMA = 4.0      # bandwidth (cycles/tile) of the random phase: more = more forks / ends, more wander
RIP_WARP_STD = 0.28         # std of the local wavelength scale
RIP_BEND_PX = 4.0
RIP_ENV = (0.08, 0.45)      # crest taper near a fork / end (|M| relative to its RMS)
RIP_PATCH_CYC = 6.0         # amplitude patches per tile (more, smaller patches = less visible repetition)

DEBUG_DIR =os.environ.get("YGG_SNOW_DEBUG", "")   # if set: 1:1 lit crops are dumped there for inspection

T0 = time.time()


def log(msg):
    print("[%6.1fs] %s" % (time.time() - T0, msg), flush=True)


# ----------------------------------------------------------------------------------------------------------------
# periodic helpers
# ----------------------------------------------------------------------------------------------------------------
def freq_grids_r(n):
    """rfft2 frequency grids in cycles per tile."""
    fy = (np.fft.fftfreq(n) * n).astype(np.float32)[:, None]
    fx = (np.fft.rfftfreq(n) * n).astype(np.float32)[None, :]
    return fx, fy


def band_noise(rng, n, filt):
    """Real periodic noise: white noise shaped in the Fourier domain by filt(fx, fy, fr); zero mean, unit std."""
    w = rng.standard_normal((n, n)).astype(np.float32)
    F = np.fft.rfft2(w)
    fx, fy = freq_grids_r(n)
    fr = np.sqrt(fx * fx + fy * fy)
    F *= filt(fx, fy, fr)
    F[0, 0] = 0.0
    out = np.fft.irfft2(F, s=(n, n)).astype(np.float32)
    out -= out.mean()
    out /= out.std() + 1e-12
    return out


def gauss_band(f0, sig):
    return lambda fx, fy, fr: np.exp(-0.5 * ((fr - f0) / sig) ** 2)


def blur(a, sigma):
    return ndimage.gaussian_filter(a, sigma, mode="wrap")


def smoothstep(e0, e1, x):
    t = np.clip((x - e0) / (e1 - e0), 0.0, 1.0)
    return t * t * (3.0 - 2.0 * t)


def wrapv(v, n):
    return v - n * np.round(v / n)


def grid(n):
    yy, xx = np.mgrid[0:n, 0:n].astype(np.float32)
    return xx, yy


def voronoi2(seeds, n, qx, qy):
    """Nearest two seeds on the torus. Returns d1, d2, i1, i2 and e = distance to the i1/i2 bisector (cell edge)."""
    tree = cKDTree(seeds, boxsize=n)
    px = np.mod(qx, n).ravel().astype(np.float64)
    py = np.mod(qy, n).ravel().astype(np.float64)
    px[px >= n] = 0.0
    py[py >= n] = 0.0
    d, i = tree.query(np.stack([px, py], 1), k=2, workers=-1)
    shp = qx.shape
    i1 = i[:, 0].reshape(shp)
    i2 = i[:, 1].reshape(shp)
    d1 = d[:, 0].reshape(shp).astype(np.float32)
    d2 = d[:, 1].reshape(shp).astype(np.float32)
    # vectors from the query point to both seeds (wrapped)
    ax = wrapv(seeds[i1, 0] - px.reshape(shp), n).astype(np.float32)
    ay = wrapv(seeds[i1, 1] - py.reshape(shp), n).astype(np.float32)
    bx = wrapv(seeds[i2, 0] - px.reshape(shp), n).astype(np.float32)
    by = wrapv(seeds[i2, 1] - py.reshape(shp), n).astype(np.float32)
    sep = np.sqrt((bx - ax) ** 2 + (by - ay) ** 2) + 1e-6
    e = (d2 * d2 - d1 * d1) / (2.0 * sep)
    return d1, d2, i1, i2, e, ax, ay


def sample_at(field, n, x, y):
    """Bilinear periodic lookup of a seed-position property field (for clustered selections)."""
    return ndimage.map_coordinates(field, [np.mod(y, n), np.mod(x, n)], order=1, mode="grid-wrap")


def poisson_seeds(rng, n, spacing):
    cnt = int((n / spacing) ** 2)
    return rng.uniform(0.0, n, (cnt, 2))


def tin_periodic(seeds, values, n, qx, qy, margin):
    """Piecewise-linear surface over the Delaunay triangulation of the seeds, on the torus (seeds near the border
    are copied across it). Continuous, with sharp creases: angular facets, no round bumps."""
    from scipy.interpolate import LinearNDInterpolator
    pts = [seeds]
    vals = [values]
    for ox in (-n, 0, n):
        for oy in (-n, 0, n):
            if ox == 0 and oy == 0:
                continue
            p = seeds + np.array([ox, oy])
            keep = (p[:, 0] > -margin) & (p[:, 0] < n + margin) & (p[:, 1] > -margin) & (p[:, 1] < n + margin)
            pts.append(p[keep])
            vals.append(values[keep])
    ip = LinearNDInterpolator(np.concatenate(pts), np.concatenate(vals))
    out = ip(np.mod(qx, n).ravel(), np.mod(qy, n).ravel()).reshape(qx.shape)
    return np.nan_to_num(out).astype(np.float32)


def warp_field(rng, n, cycles, amp_px):
    """Two periodic displacement fields with a band around `cycles` per tile, std = amp_px."""
    f = gauss_band(cycles, cycles * 0.5)
    return band_noise(rng, n, f) * amp_px, band_noise(rng, n, f) * amp_px


# ----------------------------------------------------------------------------------------------------------------
# maps from height
# ----------------------------------------------------------------------------------------------------------------
def slopes(h_mm, dx_mm):
    """Physical slopes dh/dU (right) and dh/dV (down the image), central differences on the torus."""
    gx = (np.roll(h_mm, -1, 1) - np.roll(h_mm, 1, 1)) / (2.0 * dx_mm)
    gy = (np.roll(h_mm, -1, 0) - np.roll(h_mm, 1, 0)) / (2.0 * dx_mm)
    return gx, gy


def normal_dx(h_mm, dx_mm):
    """Tangent-space normal, DirectX (green = +V = down the image): n = normalize(-dh/dU, -dh/dV, 1)."""
    gx, gy = slopes(h_mm, dx_mm)
    nz = 1.0 / np.sqrt(gx * gx + gy * gy + 1.0)
    return np.stack([-gx * nz, -gy * nz, nz], -1).astype(np.float32)


def encode_normal(nrm):
    return np.clip(np.round((nrm * 0.5 + 0.5) * 255.0), 0, 255).astype(np.uint8)


def height_ao(h_mm, dx_mm, radii=(2.0, 5.0, 12.0, 28.0)):
    """Cheap AO from the height: how far each texel sits below its neighbourhood, as an angle, over a few radii."""
    occ = np.zeros_like(h_mm)
    for r in radii:
        dh = blur(h_mm, r) - h_mm                         # >0 in cavities
        ang = np.arctan(np.maximum(dh, 0.0) / (r * dx_mm))  # elevation of the surrounding "rim"
        occ += np.sin(ang)
    occ /= len(radii)
    return occ


def save_png16(path, a01):
    a = np.clip(np.round(a01 * 65535.0), 0, 65535).astype(np.uint16)
    Image.fromarray(a).save(path)  # uint16 2-D -> mode I;16 -> 16-bit grey PNG
    chk = np.array(Image.open(path))
    assert chk.dtype == np.uint16 or chk.max() > 255, "16-bit PNG write failed"


def save_png8(path, rgb_u8):
    Image.fromarray(rgb_u8).save(path)


def to_unit(h_mm, half_range_mm):
    """Shift to zero mean and scale so the larger side exactly reaches +-half_range_mm. Returns (h_mm, h01)."""
    h = h_mm - h_mm.mean()
    s = half_range_mm / np.abs(h).max()
    h = (h * s).astype(np.float32)
    return h, (0.5 + 0.5 * h / half_range_mm).astype(np.float32)


def seam_report(name, h):
    inner = np.mean(np.abs(np.diff(h, axis=1)))
    seam_x = np.mean(np.abs(h[:, 0] - h[:, -1]))
    innery = np.mean(np.abs(np.diff(h, axis=0)))
    seam_y = np.mean(np.abs(h[0, :] - h[-1, :]))
    log("%s seam check: |dh| across U seam %.4f vs interior %.4f; V seam %.4f vs interior %.4f"
        % (name, seam_x, inner, seam_y, innery))


# ----------------------------------------------------------------------------------------------------------------
# preview renderer: low sun from the side, blue sky ambient, cast shadows on the torus
# ----------------------------------------------------------------------------------------------------------------
def sweep_shadow(h_mm, dx_mm, elev_deg, az_deg):
    """Hard-ish cast shadows of a periodic height field. az = direction TOWARD the sun in image space
    (0 = +U right, 90 = +V down). Sweeps away from the sun two laps so the horizon wraps."""
    el = np.radians(elev_deg)
    ux, uy = np.cos(np.radians(az_deg)), np.sin(np.radians(az_deg))
    h = h_mm
    transpose = abs(uy) > abs(ux)
    if transpose:
        h = h.T
        ux, uy = uy, ux
    flip = ux > 0  # sweep must start at the sun side: we want the sun at smaller column index
    if flip:
        h = h[:, ::-1]
        uy = uy
        ux = -ux
    # one column step toward the sun: column -1, row + uy/|ux|, distance 1/|ux| px
    rshift = uy / abs(ux)
    drop = dx_mm / abs(ux) * np.tan(el)
    i0 = int(np.floor(rshift))
    f = rshift - i0
    n_r, n_c = h.shape
    hor = h[:, -1].copy()
    lit = np.ones_like(h)
    soft = 0.15 * drop + 1e-3
    for lap in range(2):
        for c in range(n_c):
            # horizon one step toward the sun, sampled at the fractional row
            prev = (1.0 - f) * np.roll(hor, -i0) + f * np.roll(hor, -i0 - 1)
            prev = prev - drop
            col = h[:, c]
            if lap == 1:
                lit[:, c] = np.clip(1.0 - (prev - col) / soft, 0.0, 1.0)
            hor = np.maximum(col, prev)
    if flip:
        lit = lit[:, ::-1]
    if transpose:
        lit = lit.T
    return lit


def render(h_mm, dx_mm, elev_deg=17.0, az_deg=200.0):
    """Linear-light render of one tile at full resolution, returned as sRGB float 0..1 (H, W, 3)."""
    nrm = normal_dx(h_mm, dx_mm)
    # normal_dx gives (x = +U, y = +V down, z = up): the same frame as the light vector below.
    el, az = np.radians(elev_deg), np.radians(az_deg)
    L = np.array([np.cos(el) * np.cos(az), np.cos(el) * np.sin(az), np.sin(el)], np.float32)
    ndl = np.clip(nrm @ L, 0.0, 1.0)
    sh = sweep_shadow(h_mm, dx_mm, elev_deg, az_deg)
    occ = height_ao(h_mm, dx_mm)
    ao = np.clip(1.0 - 1.6 * occ, 0.3, 1.0)
    sky_vis = (0.55 + 0.45 * nrm[..., 2]) * ao
    sun = np.array([1.0, 0.96, 0.90], np.float32) * 2.7
    sky = np.array([0.42, 0.56, 0.85], np.float32) * 0.30
    alb = 0.90
    col = alb * ((ndl * sh)[..., None] * sun + sky_vis[..., None] * sky)
    col = 1.0 - np.exp(-col * 1.25)  # soft shoulder
    return np.clip(col, 0.0, 1.0) ** (1.0 / 2.2)


def downsample(img, f):
    h, w = img.shape[:2]
    return img.reshape(h // f, f, w // f, f, -1).mean(axis=(1, 3))


def to_u8(img):
    return np.clip(np.round(img * 255.0), 0, 255).astype(np.uint8)


def write_previews(tag, h_mm, dx_mm, az_deg, extra_az=None):
    """preview_<tag>.png: 2x2 tiles at 1024 px. Returns pieces for the contact sheet."""
    lit = render(h_mm, dx_mm, 17.0, az_deg)
    tile512 = downsample(lit, 4)
    prev = np.tile(tile512, (2, 2, 1))
    save_png8(os.path.join(OUT, "preview_%s.png" % tag), to_u8(prev))
    pieces = {"lit_full": lit, "tile512": tile512}
    if extra_az is not None:
        pieces["lit_full_b"] = render(h_mm, dx_mm, 17.0, extra_az)
    if DEBUG_DIR:
        os.makedirs(DEBUG_DIR, exist_ok=True)
        save_png8(os.path.join(DEBUG_DIR, "%s_crop1024_a.png" % tag), to_u8(lit[:1024, :1024]))
        save_png8(os.path.join(DEBUG_DIR, "%s_half_a.png" % tag), to_u8(downsample(lit, 2)))
        save_png8(os.path.join(DEBUG_DIR, "%s_4x4.png" % tag), to_u8(np.tile(downsample(lit, 8), (4, 4, 1))))
        if extra_az is not None:
            save_png8(os.path.join(DEBUG_DIR, "%s_crop1024_b.png" % tag), to_u8(pieces["lit_full_b"][:1024, :1024]))
    return pieces


# ----------------------------------------------------------------------------------------------------------------
# SnowFine
# ----------------------------------------------------------------------------------------------------------------
def make_fine(seed=20261114):
    rng = np.random.default_rng(seed)
    n = N
    dx_mm = FINE_SIZE_CM * 10.0 / n
    xx, yy = grid(n)
    log("fine: dx = %.3f mm/texel" % dx_mm)

    # -- 1. sugar grain, 1-3 mm: packed grains = parabolic caps around Poisson seeds, creases where they meet
    seeds = poisson_seeds(rng, n, 2.2)
    rad = rng.uniform(0.8, 1.7, len(seeds)).astype(np.float32)        # grain radius, px (~mm)
    lift = rng.normal(0.0, 0.18, len(seeds)).astype(np.float32)
    d1, d2, i1, i2, e, ax, ay = voronoi2(seeds, n, xx + 0.5, yy + 0.5)
    r = rad[i1]
    grain = r * (1.0 - (d1 / r) ** 2) * 0.5 + lift[i1] * r
    grain = (grain - grain.mean()).astype(np.float32)
    grain /= grain.std()
    del d1, d2, i1, i2, e, ax, ay
    log("fine: grain done (%d seeds)" % len(seeds))

    # -- 2. crust facets, 1-3 cm: random heights on Poisson points, linear over the Delaunay triangles ->
    # a continuous, slightly angular sugar-crust relief with sharp little creases (no steps, no round bumps)
    # jitter for every ragged edge below: ~0.8 px at a 1.8 cm period (jagged, broken) + a little finer
    # (a 0.7 px jitter at a 4 px period read as a dotted 'perforation' line along every plate edge)
    wx2, wy2 = warp_field(rng, n, n / 18.0, 0.8)
    wx3, wy3 = warp_field(rng, n, n / 5.0, 0.22)
    wx2 += wx3
    wy2 += wy3
    del wx3, wy3
    seeds = poisson_seeds(rng, n, CRUST_SPACING_PX)
    m = len(seeds)
    crust = tin_periodic(seeds, rng.normal(0.0, 1.0, m), n, xx, yy, 4 * CRUST_SPACING_PX)
    crust = blur(crust, 0.8)                          # creases a touch worn, not low-poly
    crust = (crust - crust.mean()).astype(np.float32)
    crust /= crust.std()
    log("fine: crust facets done (%d points)" % m)

    # -- 3. thin plates, 2-10 cm, clustered, with small sharp broken edges (wind crust)
    wx, wy = warp_field(rng, n, n / 70.0, PLATE_WARP_PX)
    seeds = poisson_seeds(rng, n, PLATE_SPACING_PX)
    m = len(seeds)
    clus = band_noise(rng, n, gauss_band(n / 220.0, n / 500.0))          # ~20 cm clusters
    sel = sample_at(clus, n, seeds[:, 0], seeds[:, 1]) + rng.normal(0, 0.6, m)
    raised = sel > np.quantile(sel, 1.0 - PLATE_FRACTION)
    lvl = sample_at(band_noise(rng, n, gauss_band(n / 300.0, n / 600.0)), n, seeds[:, 0], seeds[:, 1])
    step = np.where(raised, 1.0 + 0.12 * lvl + rng.normal(0, 0.08, m), 0.0).astype(np.float32)
    ptx = rng.normal(0, 0.004, m).astype(np.float32)
    pty = rng.normal(0, 0.004, m).astype(np.float32)
    d1, d2, i1, i2, e, ax, ay = voronoi2(seeds, n, xx + wx + wx2 * 1.4, yy + wy + wy2 * 1.4)
    v1 = step[i1] - (ptx[i1] * ax + pty[i1] * ay) * step[i1]
    v2 = step[i2] - (ptx[i2] * (ax) + pty[i2] * (ay)) * step[i2]
    # most edges crisp (~1 px, not aliased), some worn into short ramps; step height wanders along the edge
    wsoft = 1.0 + 4.0 * smoothstep(0.4, 1.6, band_noise(rng, n, gauss_band(n / 110.0, n / 220.0)))
    stepmod = 0.5 + 0.5 * smoothstep(-1.3, 1.0, band_noise(rng, n, gauss_band(n / 80.0, n / 160.0)))
    k = smoothstep(0.0, wsoft, e)
    plates = ((0.5 * (v1 + v2)) * (1.0 - k) + v1 * k) * stepmod
    plate_mask = np.clip(plates / 0.45, 0.0, 1.0).astype(np.float32)
    plates = plates.astype(np.float32)
    del d1, d2, i1, i2, e, ax, ay, v1, v2, k, wsoft, stepmod
    log("fine: plates done (%d cells, %d raised)" % (m, raised.sum()))

    # -- 4. crumbs, 2-6 cm, sparse: polygonal lumps with steep sides and a rounded top
    wx, wy = warp_field(rng, n, n / 18.0, 3.0)
    seeds = poisson_seeds(rng, n, 30.0)
    m = len(seeds)
    has = rng.random(m) < 0.05
    ch = np.where(has, rng.uniform(0.7, 1.3, m), 0.0).astype(np.float32)
    margin = rng.uniform(3.0, 7.0, m).astype(np.float32)
    d1, d2, i1, i2, e, ax, ay = voronoi2(seeds, n, xx + wx + wx2 * 0.8, yy + wy + wy2 * 0.8)
    inside = np.maximum(e - margin[i1], 0.0)
    crumbs = (ch[i1] * (1.0 - np.exp(-inside / 2.2))).astype(np.float32)
    del d1, d2, i1, i2, e, ax, ay, inside
    log("fine: crumbs done (%d crumbs)" % has.sum())

    # -- combine (mm). Plate tops are wind-polished: less crust relief and a more sintered grain on them.
    h = (GRAIN_MM * grain * (1.0 - 0.35 * plate_mask)
         + CRUST_MM * crust * (1.0 - 0.55 * plate_mask)
         + PLATE_MM * plates
         + CRUMB_MM * crumbs)
    h = blur(h, 0.45)
    # no blotches: remove anything slower than ~30 cm so the tile is statistically even (no repeat at 4x4)
    h = h - 0.9 * blur(h, 90.0)
    h_mm, h01 = to_unit(h.astype(np.float32), FINE_HEIGHT_CM * 10.0)
    seam_report("fine", h01)

    # -- maps
    nrm = normal_dx(h_mm, dx_mm)
    occ = height_ao(h_mm, dx_mm)
    ao = 1.0 - occ / np.quantile(occ, 0.995) * 0.30
    ao = np.clip(ao, 0.70, 1.0)
    # crests (locally high) polished by the wind = smoother; a continuous measure, not a threshold
    local = h_mm - blur(h_mm, 8.0)
    crest = smoothstep(-1.2, 1.8, local / (np.std(local) + 1e-6))
    rough = 0.92 - 0.15 * crest - 0.07 * blur(plate_mask, 1.5)
    rough = np.clip(rough, 0.70, 0.92)

    save_png16(os.path.join(OUT, "T_SnowFine_H.png"), h01)
    save_png8(os.path.join(OUT, "T_SnowFine_N.png"), encode_normal(nrm))
    hra = np.stack([h01, rough, ao], -1)
    save_png8(os.path.join(OUT, "T_SnowFine_HRA.png"), to_u8(hra))
    stats = dict(range_mm=FINE_HEIGHT_CM * 10, h01_min=float(h01.min()), h01_max=float(h01.max()),
                 h_std_mm=float(h_mm.std()), rough=(float(rough.min()), float(rough.max())),
                 ao=(float(ao.min()), float(ao.max())),
                 slope_med_deg=float(np.degrees(np.arccos(np.median(nrm[..., 2])))))
    log("fine: maps written %s" % stats)
    pieces = write_previews("fine", h_mm, dx_mm, az_deg=200.0, extra_az=250.0)
    pieces.update(h01=h01, nrm=nrm, hra=hra, stats=stats, h_mm=h_mm)
    log("fine: previews written")
    return pieces


# ----------------------------------------------------------------------------------------------------------------
# SnowRipple
# ----------------------------------------------------------------------------------------------------------------
def ripple_profile(t, a):
    """One ripple period, t in [0,1) along +U (the wind). Trough at t=0, gentle windward rise to a crisp crest at
    t=a, then a short steep lee face easing into the next trough. Returns -1..1."""
    u = np.clip(t / a, 0.0, 1.0)
    stoss = u ** 1.55 * (1.0 - 0.18 * (1.0 - u))          # smooth trough, slightly steepening to the crest
    stoss = stoss / 1.0
    v = np.clip((t - a) / (1.0 - a), 0.0, 1.0)
    lee = (1.0 - v) ** 2.2 * (1.0 + 1.2 * v)                # steep straight-ish slip face, concave at the foot
    lee = np.clip(lee, 0.0, 1.0)
    p = np.where(t < a, stoss, lee)
    return 2.0 * p - 1.0


def make_ripple(seed=20261115):
    rng = np.random.default_rng(seed)
    n = N
    dx_mm = RIPPLE_SIZE_CM * 10.0 / n
    xx, yy = grid(n)
    lam0_cm = RIP_LAMBDA_CM
    f0 = RIPPLE_SIZE_CM / lam0_cm                         # cycles per tile
    log("ripple: dx = %.3f mm/texel, base wavelength %.1f cm (%.1f cycles/tile)" % (dx_mm, lam0_cm, f0))

    # -- phase field = plane wave along +U (integer cycles per tile, so it tiles) + a smooth random phase arg(M).
    # M is a smooth, isotropic complex random field (scale > wavelength): every zero of M is a phase vortex, i.e.
    # an edge dislocation in the crest pattern - one crest ends between two, or two crests merge into one (Y-fork),
    # exactly like wind / sand ripples. Away from the zeros the crests are continuous and gently curved.
    # (An analytic band-pass field was tried first: its low-amplitude seams gave horizontal crest links.)
    f0i = int(round(f0))
    fy = (np.fft.fftfreq(n) * n).astype(np.float32)[:, None]
    fx = (np.fft.fftfreq(n) * n).astype(np.float32)[None, :]
    fr = np.sqrt(fx * fx + fy * fy)
    white = (rng.standard_normal((n, n)) + 1j * rng.standard_normal((n, n))).astype(np.complex64)
    M = np.fft.ifft2(np.fft.fft2(white) * np.exp(-0.5 * (fr / RIP_DEFECT_SIGMA) ** 2)).astype(np.complex64)
    del white, fr
    M /= np.sqrt(np.mean(np.abs(M) ** 2))
    argM = np.angle(M).astype(np.float32)
    magM = np.abs(M).astype(np.float32)
    # count the vortices (phase winding round each texel square)
    def wrap_pi(a):
        return (a + np.pi) % (2 * np.pi) - np.pi
    d1 = wrap_pi(np.roll(argM, -1, 1) - argM)
    d2 = wrap_pi(np.roll(np.roll(argM, -1, 1), -1, 0) - np.roll(argM, -1, 1))
    d3 = wrap_pi(np.roll(argM, -1, 0) - np.roll(np.roll(argM, -1, 1), -1, 0))
    d4 = wrap_pi(argM - np.roll(argM, -1, 0))
    n_vort = int(np.sum(np.abs(d1 + d2 + d3 + d4) > np.pi))
    del d1, d2, d3, d4, M
    log("ripple: %d crest forks / ends per tile (%.1f per m2)" % (n_vort, n_vort / (RIPPLE_SIZE_CM / 100.0) ** 2))

    # -- smooth wavelength change and crest bending: periodic warp of the U coordinate.
    # local wavenumber scale s = 1 + dw/dU; s in ~[0.54, 1.69] -> wavelength ~8-25 cm
    fxr, fyr = freq_grids_r(n)
    Wf = np.fft.rfft2(rng.standard_normal((n, n)).astype(np.float32))
    # zones of short / long ripples follow the fetch: the warp varies mostly along U, slowly along V
    Wf *= (np.exp(-0.5 * ((fxr - 2.0) / 0.9) ** 2) * np.exp(-0.5 * (fyr / 2.2) ** 2) * (fxr >= 1.0))
    w = np.fft.irfft2(Wf, s=(n, n)).astype(np.float32)
    w_u = np.fft.irfft2(Wf * (2j * np.pi * fxr / n), s=(n, n)).astype(np.float32)
    del Wf
    sc = RIP_WARP_STD / w_u.std()
    w *= sc
    w_u *= sc
    # bend: small extra V-dependent U shift, crests wander a few degrees
    bend = band_noise(rng, n, gauss_band(5.0, 1.5)) * RIP_BEND_PX
    s = 1.0 + w_u
    s = np.clip(s, 0.54, 1.69)                           # wavelength 8 .. 25 cm
    w_v = (np.roll(w + bend, -1, 0) - np.roll(w + bend, 1, 0)) * 0.5
    log("ripple: crest tilt from the warp, std %.1f deg" % np.degrees(np.arctan(w_v.std())))
    lam_local = lam0_cm * f0 / f0i / s
    log("ripple: local wavelength p1/p50/p99 = %.1f / %.1f / %.1f cm"
        % tuple(np.percentile(lam_local, [1, 50, 99])))

    phase = 2.0 * np.pi * f0i * (xx + w + bend) / n + argM
    t = np.mod(phase / (2 * np.pi), 1.0).astype(np.float32)
    del phase, w, bend, w_v
    # crests taper out only close to a vortex (where the phase is undefined)
    env = smoothstep(RIP_ENV[0], RIP_ENV[1], magM)
    log("ripple: crest coverage (env > 0.5) %.0f %%" % (100.0 * np.mean(env > 0.5)))
    if DEBUG_DIR:
        os.makedirs(DEBUG_DIR, exist_ok=True)
        dbg = np.concatenate([t, env], 1)
        save_png8(os.path.join(DEBUG_DIR, "ripple_fields.png"), to_u8(downsample(grey3(dbg), 4)))
    a = 0.70 + 0.07 * band_noise(rng, n, gauss_band(3.0, 1.5))           # crest position (lee share 0.23-0.37)
    a = np.clip(a, 0.6, 0.8)
    prof = ripple_profile(t, a)
    # mean of the profile over a period, so faded areas sit at the mean level
    tt = np.linspace(0, 1, 4001, endpoint=False, dtype=np.float32)
    pmean = float(ripple_profile(tt, 0.70).mean())
    prof = prof - pmean

    # -- patches: some areas almost smooth; longer ripples are taller
    patch = band_noise(rng, n, gauss_band(RIP_PATCH_CYC, RIP_PATCH_CYC * 0.4))
    patch_m = 0.05 + 0.95 * smoothstep(-0.8, 0.9, patch)
    hscale = (1.0 / s) ** 0.8
    crest_var = 0.65 + 0.35 * np.clip(magM / 1.2, 0.0, 1.0)            # crest height wanders along each crest
    h = prof * env * patch_m * hscale * crest_var
    h = blur(h, 0.6)
    h_mm, h01 = to_unit(h.astype(np.float32), RIPPLE_HEIGHT_CM * 10.0)
    seam_report("ripple", h01)

    nrm = normal_dx(h_mm, dx_mm)
    save_png16(os.path.join(OUT, "T_SnowRipple_H.png"), h01)
    save_png8(os.path.join(OUT, "T_SnowRipple_N.png"), encode_normal(nrm))
    stats = dict(range_mm=RIPPLE_HEIGHT_CM * 10, h01_min=float(h01.min()), h01_max=float(h01.max()),
                 h_std_mm=float(h_mm.std()),
                 lam_cm_p1_p99=(float(np.percentile(lam_local, 1)), float(np.percentile(lam_local, 99))),
                 slope_p99_deg=float(np.degrees(np.arccos(np.percentile(nrm[..., 2], 1)))))
    log("ripple: maps written %s" % stats)
    # sun from the windward side, low, a bit off-axis; second view with the sun along the crests
    pieces = write_previews("ripple", h_mm, dx_mm, az_deg=200.0, extra_az=250.0)
    pieces.update(h01=h01, nrm=nrm, stats=stats, h_mm=h_mm)
    log("ripple: previews written")
    return pieces


# ----------------------------------------------------------------------------------------------------------------
# contact sheet
# ----------------------------------------------------------------------------------------------------------------
def grey3(a):
    return np.repeat(a[..., None], 3, -1)


def contact_sheet(fine, ripple):
    from PIL import ImageDraw
    cell = 512
    cols = 5
    rows = []

    def strip(p, has_hra):
        a = [downsample(grey3(p["h01"]), 4)]
        a.append(downsample(p["nrm"] * 0.5 + 0.5, 4))
        if has_hra:
            a.append(downsample(p["hra"], 4))
        a.append(p["lit_full"][:cell, :cell])                   # 1:1 crop (one texel per pixel)
        a.append(downsample(np.tile(p["tile512"], (4, 4, 1)), 4))  # 4x4 tiling, repetition check
        a.append(p["lit_full_b"][:cell, :cell])
        return a

    labels_f = ["fine H", "fine N (DX)", "fine HRA", "fine lit 1:1 (sun left)", "fine 4x4 tiles (8 m)",
                "fine lit 1:1 (sun top)"]
    labels_r = ["ripple H", "ripple N (DX)", "ripple lit 1:1 (sun left)", "ripple 4x4 tiles (12 m)",
                "ripple lit 1:1 (sun top)", "ripple + fine combined 1:1 (sun left)"]
    sf = strip(fine, True)
    sr = strip(ripple, False)
    # what the material will roughly show: ripple + fine crust at their real sizes (fine resampled onto the
    # ripple texel grid, 1.465 / 0.977 mm), sun from the left; crop away from the wrap line
    dxr = RIPPLE_SIZE_CM * 10.0 / N
    f = (RIPPLE_SIZE_CM / FINE_SIZE_CM)
    xx, yy = grid(N)
    fine_on_r = ndimage.map_coordinates(fine["h_mm"], [yy * f, xx * f], order=1, mode="grid-wrap")
    comb = render(ripple["h_mm"] + fine_on_r, dxr, 17.0, 200.0)
    sr.append(comb[700:700 + cell, 700:700 + cell])
    if DEBUG_DIR:
        save_png8(os.path.join(DEBUG_DIR, "combined_crop1024.png"), to_u8(comb[600:1624, 600:1624]))
    cols = max(len(sf), len(sr))
    sheet = np.full((2 * (cell + 24), cols * cell, 3), 0.12, np.float32)
    for r, (imgs, labels) in enumerate([(sf, labels_f), (sr, labels_r)]):
        for c, im in enumerate(imgs):
            y0 = r * (cell + 24) + 24
            sheet[y0:y0 + cell, c * cell:(c + 1) * cell] = im[:cell, :cell, :3]
    img = Image.fromarray(to_u8(sheet))
    d = ImageDraw.Draw(img)
    for r, labels in enumerate([labels_f, labels_r]):
        for c, lab in enumerate(labels):
            d.text((c * cell + 6, r * (cell + 24) + 6), lab, fill=(230, 230, 230))
    img.save(os.path.join(OUT, "preview_sheet.png"))


def write_readme(fine_stats, ripple_stats):
    txt = """# Procedural snow detail textures (generated {date})

Generator: `Tools/Materials/make_snow_detail.py` (run `python -I Tools/Materials/make_snow_detail.py`).
Made procedurally (numpy/scipy), no downloaded source. All maps are 2048 x 2048 and tile seamlessly.

## Conventions

| | SnowFine | SnowRipple |
|---|---|---|
| physical tile size | **2 m x 2 m** (0.977 mm/texel) | **3 m x 3 m** (1.465 mm/texel) |
| height map | `T_SnowFine_H.png` 16-bit grey | `T_SnowRipple_H.png` 16-bit grey |
| height range | **0..1 = -0.6 cm .. +0.6 cm** (1.2 cm total) | **0..1 = -1.5 cm .. +1.5 cm** (3.0 cm total) |
| 0.5 = | mean height | mean height |
| normal map | `T_SnowFine_N.png` | `T_SnowRipple_N.png` |
| extra | `T_SnowFine_HRA.png` (R height, G roughness, B AO) | - |

Displacement in the material: `d_cm = (H - 0.5) * 2 * range_cm`, i.e. `(H - 0.5) * 1.2` for SnowFine and
`(H - 0.5) * 3.0` for SnowRipple at the tile sizes above (SnowTileCm = 200 / RipTile = 300). If a tile is drawn
at a different world size, scale the displacement by the same factor to keep the normals matching.

**Normal maps: DirectX convention, green = +V = DOWN the image** (n = normalize(-dh/dU, -dh/dV, 1), encoded
n*0.5+0.5). This is UE's native convention: import as TC_Normalmap with **"Flip Green Channel" OFF**. Red = +U
(right). The slopes are physically correct for the tile size and height range above (not exaggerated).

HRA (8-bit, import as TC_Masks, sRGB OFF): R = height (same as _H, 8-bit), G = roughness 0.70-0.92 (crests and
plate tops wind-polished = smoother), B = ambient occlusion 0.70-1.00 from the height (cavities darker).

Wind direction for SnowRipple: wind blows along **+U** (texture +X). Crests run roughly along V; the windward
(stoss) face is long and gentle and faces -U, the lee face is short and steep and faces +U, the crest is crisp.
Wavelength varies smoothly over ~8-25 cm; amplitude varies in patches (some areas almost smooth).

## Measured (this run)
- SnowFine: height std {f_std:.2f} mm; H spans {f_min:.3f} .. {f_max:.3f} (0.5 = mean; the relief is skewed upward -
  plates, crumbs and crest facets stick up - so the top reaches 1.0 and the deepest cavity only ~{f_min:.2f});
  median surface slope {f_slope:.1f} deg; roughness {r0:.2f}-{r1:.2f}; AO {a0:.2f}-{a1:.2f}.
- SnowRipple: height std {r_std:.2f} mm; H spans {r_min:.3f} .. {r_max:.3f} (crests stick up, troughs are wide);
  local wavelength p1-p99 {l0:.1f}-{l1:.1f} cm; 99th-percentile slope {r_slope:.0f} deg (lee faces).

## Previews
`preview_fine.png`, `preview_ripple.png`: 2x2 tiles, 1024 px, sun 17 deg above the horizon from the side
(azimuth 200 deg: from the left/top-left), a little blue sky light, cast shadows. `preview_sheet.png`: maps, 1:1
crops (one texel per pixel), a 4x4 tiling for repetition, a second sun direction (from the top), and a
ripple + fine combined crop at their real sizes (roughly what the material will show where both layers blend).
""".format(date=time.strftime("%Y-%m-%d %H:%M"),
           f_std=fine_stats["h_std_mm"], f_min=fine_stats["h01_min"], f_max=fine_stats["h01_max"],
           f_slope=fine_stats["slope_med_deg"], r0=fine_stats["rough"][0], r1=fine_stats["rough"][1],
           a0=fine_stats["ao"][0], a1=fine_stats["ao"][1],
           r_std=ripple_stats["h_std_mm"], r_min=ripple_stats["h01_min"], r_max=ripple_stats["h01_max"],
           l0=ripple_stats["lam_cm_p1_p99"][0], l1=ripple_stats["lam_cm_p1_p99"][1],
           r_slope=ripple_stats["slope_p99_deg"])
    with open(os.path.join(OUT, "README.md"), "w", encoding="utf-8") as f:
        f.write(txt)


def self_test_normal_convention():
    """A bump must have green > 0.5 on its LOWER half (DirectX / UE), red > 0.5 on its right half."""
    n = 64
    xx, yy = grid(n)
    h = np.exp(-(((xx - 32) ** 2 + (yy - 32) ** 2) / 50.0)).astype(np.float32)
    nrm = encode_normal(normal_dx(h, 1.0))
    assert nrm[40, 32, 1] > 128 and nrm[24, 32, 1] < 128, "green convention wrong"
    assert nrm[32, 40, 0] > 128 and nrm[32, 24, 0] < 128, "red convention wrong"


def main():
    which = sys.argv[1] if len(sys.argv) > 1 else "all"
    os.makedirs(OUT, exist_ok=True)
    self_test_normal_convention()
    fine = ripple = None
    if which in ("fine", "all"):
        t = time.time()
        fine = make_fine()
        log("fine set: %.1f s" % (time.time() - t))
    if which in ("ripple", "all"):
        t = time.time()
        ripple = make_ripple()
        log("ripple set: %.1f s" % (time.time() - t))
    if fine is not None and ripple is not None:
        contact_sheet(fine, ripple)
        write_readme(fine["stats"], ripple["stats"])
        log("contact sheet + README written to %s" % OUT)


if __name__ == "__main__":
    main()
