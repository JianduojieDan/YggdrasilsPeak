# Paints out the bright brand marks on the downloaded shoe scans (CC BY does not license trademarks): saturated red
# and yellow blobs (Vibram octagon, Salomon triangle, Garmont "M") are masked by colour, grown a little and filled
# from the surrounding leather/fabric by normalised blur at growing radii. Originals kept as T_scan_<name>_orig.png.
#   python -I Tools/Story/clean_scan_logos.py [names...]
import os, sys
import numpy as np
from PIL import Image
from scipy import ndimage

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
SRC = os.path.join(ROOT, "Art", "Blender", "Export", "Garments")
# per scan: which hues are logos (the shoe's own colours must not be touched: adidas blue, Sperry navy stay)
TARGETS = {"garmont_hiking_boot": ("red", "yellow", "orange"), "salomon_x_raise": ("red",), "salomon_outline_mid": ("red",)}


def hsv(a):
    r, g, b = a[..., 0], a[..., 1], a[..., 2]
    mx = a.max(-1); mn = a.min(-1); d = mx - mn + 1e-6
    h = np.where(mx == r, ((g - b) / d) % 6, np.where(mx == g, (b - r) / d + 2, (r - g) / d + 4)) * 60
    s = d / (mx + 1e-6)
    return h, s, mx


def clean(name, hues):
    path = os.path.join(SRC, f"T_scan_{name}.png"); orig = os.path.join(SRC, f"T_scan_{name}_orig.png")
    if not os.path.exists(orig):
        Image.open(path).save(orig)
    a = np.asarray(Image.open(orig).convert("RGB")).astype(np.float32) / 255.0
    h, s, v = hsv(a)
    m = np.zeros(h.shape, bool)
    if "red" in hues: m |= ((h < 18) | (h > 340)) & (s > 0.55) & (v > 0.35)
    if "orange" in hues: m |= (h >= 18) & (h < 38) & (s > 0.62) & (v > 0.45)     # vivid only: the leather is a dull brown
    if "yellow" in hues: m |= (h > 38) & (h < 62) & (s > 0.6) & (v > 0.55)
    m = ndimage.binary_opening(m, iterations=1)                       # drop single noisy pixels
    lab, n = ndimage.label(m)
    sizes = ndimage.sum(m, lab, range(1, n + 1))
    keep = np.isin(lab, [i + 1 for i, sz in enumerate(sizes) if sz >= 30])   # logo blobs, not specks of rust
    keep = ndimage.binary_dilation(keep, iterations=6)
    out = a.copy(); w = (~keep).astype(np.float32)
    filled = np.zeros_like(a); have = np.zeros(h.shape, np.float32)
    for sig in (4, 10, 24, 60):                                       # nearest material first, wider if still empty
        num = np.stack([ndimage.gaussian_filter(a[..., c] * w, sig) for c in range(3)], -1)
        den = ndimage.gaussian_filter(w, sig)[..., None]
        est = num / np.maximum(den, 1e-4)
        take = (have == 0) & (den[..., 0] > 0.15) & keep
        filled[take] = est[take]; have[take] = 1
    filled[(have == 0) & keep] = a[~keep].mean(0)
    grain = (a - np.stack([ndimage.gaussian_filter(a[..., c], 3) for c in range(3)], -1)) * 0.0   # (kept simple)
    out[keep] = filled[keep] + grain[keep]
    Image.fromarray((np.clip(out, 0, 1) * 255).astype(np.uint8)).save(path)
    print("CLEAN", name, "logo pixels", int(keep.sum()), "blobs", int(sum(1 for sz in sizes if sz >= 30)))


if __name__ == "__main__":
    names = sys.argv[1:] or list(TARGETS)
    for n in names:
        clean(n, TARGETS.get(n, ("red",)))
