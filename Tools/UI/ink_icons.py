# Turns the item renders (Tools/UI/render_item_icons.py -> Saved/ItemIcons/render/*.png) into the backpack
# screen's look: an expedition field sketch - flat watercolour washes in the
# item's own colours (muted, slightly granulated, bleeding a little past the shape), a loose dark ink outline that
# wobbles and breaks, hatching in the shadows. Output: Art/UI/Items/T_Item_<name>.png (RGBA, 512 px, transparent).
#   python Tools/UI/ink_icons.py [name ...]
import numpy as np, struct, os, sys, glob
from PIL import Image, ImageFilter
from scipy import ndimage

ROOT = os.path.join(os.path.dirname(__file__), "..", "..")
RAW = os.path.join(ROOT, "Saved", "ItemIcons", "render")
PAPER = np.array([0.93, 0.905, 0.84])        # bone-white journal paper the sketch is cut out of
CROP = {"Ration": (0.52, 0.0, 1.0, 1.0)}      # part of the render to keep (x0, y0, x1, y1 of 1): two tins, not four
NO_HATCH = {"Compass"}       # its dark lid carries the World Tree engraving: hatching would hide it
TURN = {"IceAxe": 32, "Oxygen": -18, "Ration": 8}   # tilt in the pack (degrees)
OUT = os.path.join(ROOT, "Art", "UI", "Items")
os.makedirs(OUT, exist_ok=True)
INK = np.array([0.10, 0.09, 0.085])          # warm black ink


def load(name):
    im = Image.open(os.path.join(RAW, name + ".png")).convert("RGBA")
    if name in CROP:
        c = CROP[name]
        box = (int(c[0] * im.width), int(c[1] * im.height), int(c[2] * im.width), int(c[3] * im.height))
        part = im.crop(box)
        im = Image.new("RGBA", im.size, (0, 0, 0, 0))
        im.paste(part, ((im.width - part.width) // 2, (im.height - part.height) // 2))
    if name in TURN:
        im = im.rotate(TURN[name], resample=Image.BICUBIC, expand=False)
    a = np.asarray(im, np.float32) / 255.0
    return a[..., :3], a[..., 3]


def tonemap(rgb, mask):
    # exposure from the object's own brightness, then a soft filmic curve
    lum = rgb @ np.array([0.2126, 0.7152, 0.0722])
    key = np.percentile(lum[mask > 0.5], 70) if (mask > 0.5).any() else 1.0
    x = rgb / max(key, 1e-6) * 0.55
    x = x * (1 + x / 4.0) / (1 + x)
    return np.clip(x, 0, 1) ** (1 / 2.2)


def noise(shape, scale, seed):
    rng = np.random.default_rng(seed)
    n = rng.standard_normal((shape[0] // scale + 2, shape[1] // scale + 2))
    n = ndimage.zoom(n, scale, order=3)[: shape[0], : shape[1]]
    return n / (np.abs(n).max() + 1e-6)


def ink(name, seed=0, crop=True, size=512):
    rgb, mask = load(name)
    h, w = mask.shape
    col = rgb
    lum = col @ np.array([0.3, 0.59, 0.11])
    # watercolour wash: smooth the colour, quantise the value into 4 washes, keep the hue, mute the saturation
    soft = ndimage.gaussian_filter(col, (2.2, 2.2, 0))
    v = soft.max(axis=2, keepdims=True) + 1e-4
    hue = soft / v
    grey = hue.mean(axis=2, keepdims=True)
    hue = grey + (hue - grey) * 0.8
    steps = np.array([0.28, 0.5, 0.72, 0.9])
    vq = steps[np.clip(np.digitize(ndimage.gaussian_filter(lum, 1.5), [0.18, 0.38, 0.6]), 0, 3)][..., None]
    wash = np.clip(hue * vq, 0, 1)
    # pigment granulation and paper-like unevenness
    gran = noise((h, w), 3, seed) * 0.05 + noise((h, w), 24, seed + 1) * 0.06
    wash = np.clip(wash * (1 + gran[..., None]), 0, 1)
    # the wash bleeds slightly past the shape and its edge is darker (pigment pools at the edge of a wash)
    bleed = ndimage.gaussian_filter(mask, 3.0)
    wmask = np.clip((bleed + noise((h, w), 12, seed + 2) * 0.25) * 1.6 - 0.3, 0, 1)
    edge_pool = np.clip(wmask * (1 - wmask) * 4, 0, 1) * 0.25
    wash = wash * (1 - edge_pool[..., None])
    # ink: outer contour (thick, wobbly, broken) + inner edges from value changes (thin)
    outer = ndimage.binary_dilation(mask > 0.5, iterations=3) & ~ndimage.binary_erosion(mask > 0.5, iterations=1)
    outer = ndimage.gaussian_filter(outer.astype(float), 0.9)
    breaks = np.clip(noise((h, w), 40, seed + 3) * 2 + 1.1, 0, 1)
    sx, sy = ndimage.sobel(ndimage.gaussian_filter(lum, 1.2), 1), ndimage.sobel(ndimage.gaussian_filter(lum, 1.2), 0)
    inner = np.clip((np.hypot(sx, sy) - 0.35) * 2.2, 0, 1) * (mask > 0.5)
    inner = ndimage.gaussian_filter(inner, 0.6)
    # shadow hatching: diagonal strokes where the value is darkest
    yy, xx = np.mgrid[0:h, 0:w]
    hatch = (np.sin((xx + yy) * 0.55 + noise((h, w), 30, seed + 4) * 3) > 0.55).astype(float)
    hatch *= np.clip((0.3 - lum) * 5, 0, 1) * (mask > 0.5) * (0.0 if name in NO_HATCH else 0.55)
    line = np.clip(outer * breaks * 0.95 + inner * 0.7 + hatch, 0, 1)
    out_rgb = wash * (1 - line[..., None]) + INK * line[..., None]
    alpha = np.clip(np.maximum(wmask * 0.96, line), 0, 1)
    # cut out of the journal page: a ragged paper margin round the sketch, a soft shadow under it
    cut = ndimage.gaussian_filter((ndimage.distance_transform_edt(alpha < 0.3) < 16 + noise((h, w), 18, seed + 5) * 5).astype(float), 1.0)
    paper = PAPER * (1 + noise((h, w), 2, seed + 6)[..., None] * 0.025 + noise((h, w), 60, seed + 7)[..., None] * 0.03)
    rgb_all = paper * (1 - alpha[..., None]) + out_rgb * alpha[..., None]
    shadow = ndimage.shift(ndimage.gaussian_filter(cut, 7), (7, 4), order=1) * 0.45
    a_all = np.clip(np.maximum(cut, shadow), 0, 1)
    rgb_all = rgb_all * (cut / np.maximum(a_all, 1e-4))[..., None]      # the shadow is black
    img = np.dstack([rgb_all, a_all])
    if crop:   # crop to content with a margin, square
        ys, xs = np.where(a_all > 0.05)
        y0, y1, x0, x1 = ys.min(), ys.max(), xs.min(), xs.max()
        s = int(max(y1 - y0, x1 - x0) * 1.08) + 8
        cy, cx = (y0 + y1) // 2, (x0 + x1) // 2
        canvas = np.zeros((s, s, 4))
        sy0, sx0 = max(cy - s // 2, 0), max(cx - s // 2, 0)
        part = img[sy0: sy0 + s, sx0: sx0 + s]
        oy, ox = (s - part.shape[0]) // 2, (s - part.shape[1]) // 2
        canvas[oy: oy + part.shape[0], ox: ox + part.shape[1]] = part
    else:      # animation frames keep the full frame so they stay registered
        canvas = img
    im = Image.fromarray((np.clip(canvas, 0, 1) * 255).astype(np.uint8), "RGBA").resize((size, size), Image.LANCZOS)
    path = os.path.join(OUT, f"T_{name}.png" if name.startswith("Pack") else f"T_Item_{name}.png")
    im.save(path)
    return path


if sys.argv[1:2] == ["--pack"]:     # the backpack's opening frames: same seed, no crop, 1024 px
    for f in sorted(glob.glob(os.path.join(RAW, "Pack_f*.png"))):
        print("inked", ink(os.path.splitext(os.path.basename(f))[0], 77, crop=False, size=1024))
    sys.exit(0)
names = sys.argv[1:] or [os.path.splitext(os.path.basename(p))[0] for p in glob.glob(os.path.join(RAW, "*.rgbaf"))]
for i, n in enumerate(names):
    print("inked", ink(n, i * 10))
