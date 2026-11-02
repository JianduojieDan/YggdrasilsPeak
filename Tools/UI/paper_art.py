# The backpack screen's paper pieces, in the same field-journal style as the item sketches (Tools/UI/ink_icons.py):
#   T_Card      - a torn journal page with faint ruled lines (the selected item's card)
#   T_Scale     - a porter's hanging spring scale drawn in ink on a paper strip, 0-30 kg (the pack's weight)
#   T_ScaleMark - the scale's pointer
#   T_Vignette  - dark blue-black vignette laid over the blurred world behind the pack
#   T_Pocket    - soft inner shadow for the pack's open compartment
# Output: Art/UI/Backpack/*.png.   python Tools/UI/paper_art.py
import numpy as np, os
from PIL import Image, ImageDraw, ImageFont
from scipy import ndimage

ROOT = os.path.join(os.path.dirname(__file__), "..", "..")
OUT = os.path.join(ROOT, "Art", "UI", "Backpack")
os.makedirs(OUT, exist_ok=True)
PAPER = np.array([0.93, 0.905, 0.84])
INK = (34, 30, 27)
FONT = os.path.join(ROOT, "Imports", "Fonts", "Caveat.ttf")
rng = np.random.default_rng(5)


def noise(h, w, scale, seed):
    r = np.random.default_rng(seed).standard_normal((h // scale + 2, w // scale + 2))
    n = ndimage.zoom(r, scale, order=3)[:h, :w]
    return n / (np.abs(n).max() + 1e-6)


def paper(h, w, seed, torn=12):
    """Paper with fibre grain, uneven tone, a torn ragged outline and a soft shadow."""
    pad = torn * 3
    H, W = h + pad * 2, w + pad * 2
    yy, xx = np.mgrid[0:H, 0:W]
    inside = np.minimum.reduce([yy - pad, H - pad - yy, xx - pad, W - pad - xx]).astype(float)
    edge = inside + noise(H, W, 6, seed) * torn * 0.5 + noise(H, W, 40, seed + 1) * torn
    cut = ndimage.gaussian_filter((edge > 0).astype(float), 0.8)
    tone = 1 + noise(H, W, 2, seed + 2) * 0.03 + noise(H, W, 90, seed + 3) * 0.05
    rgb = PAPER * tone[..., None]
    # foxing / water marks, darker towards the torn edge
    rgb *= (1 - np.clip(1 - inside / (torn * 4), 0, 1) * 0.12)[..., None]
    shadow = ndimage.shift(ndimage.gaussian_filter(cut, 9), (8, 5), order=1) * 0.5
    a = np.clip(np.maximum(cut, shadow), 0, 1)
    rgb = rgb * (cut / np.maximum(a, 1e-4))[..., None]
    return np.dstack([np.clip(rgb, 0, 1), a]), pad


def save(arr, name):
    Image.fromarray((np.clip(arr, 0, 1) * 255).astype(np.uint8), "RGBA").save(os.path.join(OUT, name + ".png"))
    print("wrote", name)


# card: a journal page, faint blue ruled lines and a red margin line
card, pad = paper(860, 600, 11)
im = Image.fromarray((card * 255).astype(np.uint8), "RGBA")
ink_layer = Image.new("RGBA", im.size, (0, 0, 0, 0))     # drawn on its own layer, then blended onto the paper
d = ImageDraw.Draw(ink_layer)
for y in range(pad + 300, pad + 820, 34):
    d.line([(pad + 30, y), (pad + 575, y)], fill=(120, 150, 190, 70), width=2)
d.line([(pad + 70, pad + 20), (pad + 70, pad + 840)], fill=(190, 80, 70, 70), width=2)
im.alpha_composite(ink_layer)
im.save(os.path.join(OUT, "T_Card.png")); print("wrote T_Card", im.size)

# spring scale: hook, barrel, 0-30 kg ticks on a paper strip; comfortable (<16) and max (26) marked
H, W = 900, 230
strip, pad = paper(H - 60, W - 60, 21, torn=8)
im = Image.fromarray((strip * 255).astype(np.uint8), "RGBA")
ink_layer = Image.new("RGBA", im.size, (0, 0, 0, 0))
d = ImageDraw.Draw(ink_layer)
font = ImageFont.truetype(FONT, 34)
small = ImageFont.truetype(FONT, 26)
cx = im.width // 2
top, bot = pad + 140, pad + H - 190            # 0 kg at the top (the spring hangs), 30 kg at the bottom
def ky(kg): return top + (bot - top) * kg / 30.0
# hook and ring
d.ellipse([cx - 22, pad + 18, cx + 22, pad + 62], outline=INK, width=5)
d.arc([cx - 26, pad + 70, cx + 18, pad + 120], 300, 200, fill=INK, width=6)
# barrel
d.rounded_rectangle([cx - 46, top - 30, cx + 46, bot + 30], radius=30, outline=INK, width=5)
# zones: comfortable wash (sage), heavy wash (ochre), over max (red)
for a_, b_, col in ((0, 16, (120, 150, 110, 70)), (16, 26, (200, 150, 60, 80)), (26, 30, (180, 60, 50, 90))):
    d.rectangle([cx - 38, ky(a_), cx - 8, ky(b_)], fill=col)
for kg in range(0, 31):
    w_ = 26 if kg % 5 == 0 else 12
    d.line([(cx - 8 - w_ * 0.2, ky(kg)), (cx - 8 + w_, ky(kg))], fill=INK, width=3 if kg % 5 == 0 else 2)
    if kg % 5 == 0:
        d.text((cx + 26, ky(kg) - 20), str(kg), font=small, fill=INK)
d.text((cx - 22, bot + 44), "kg", font=font, fill=INK)
im.alpha_composite(ink_layer)
im.save(os.path.join(OUT, "T_Scale.png")); print("wrote T_Scale", im.size, "zero/full px", round(ky(0)), round(ky(30)))
open(os.path.join(OUT, "T_Scale_layout.txt"), "w").write(f"{ky(0) / im.height:.4f} {ky(30) / im.height:.4f}\n")

# pointer: an orange-red wedge with an ink edge
pm = Image.new("RGBA", (120, 60), (0, 0, 0, 0))
d = ImageDraw.Draw(pm)
d.polygon([(4, 30), (70, 8), (116, 30), (70, 52)], fill=(205, 85, 30, 255), outline=INK, width=4)
pm.save(os.path.join(OUT, "T_ScaleMark.png")); print("wrote T_ScaleMark")

# vignette over the blurred world
h, w = 540, 960
yy, xx = np.mgrid[0:h, 0:w]
r = np.hypot((xx - w / 2) / (w / 2), (yy - h / 2) / (h / 2))
a = np.clip(0.55 + (r - 0.3) * 0.45, 0.55, 0.92)
v = np.dstack([np.full((h, w), 0.03), np.full((h, w), 0.045), np.full((h, w), 0.07), a])
save(v, "T_Vignette")

# compartment inner shadow: dark at the rim, a little lighter in the middle
h, w = 256, 256
yy, xx = np.mgrid[0:h, 0:w]
dist = np.minimum.reduce([yy, h - 1 - yy, xx, w - 1 - xx]) / 40.0
a = np.clip(0.97 - np.clip(dist, 0, 1) * 0.25, 0, 1)
save(np.dstack([np.full((h, w), 0.012), np.full((h, w), 0.016), np.full((h, w), 0.028), a]), "T_Pocket")
