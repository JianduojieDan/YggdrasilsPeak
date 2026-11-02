# The engraving inside the rune compass's lid: the World Tree (Yggdrasil) - roots, trunk and crown branching
# inside a ring of runes - as a mask (white = engraved line) for M_CompassLid. Output Art/UI/Compass/T_CompassEmblem.png
#   python Tools/UI/compass_emblem.py
import numpy as np, os, math
from PIL import Image, ImageDraw, ImageFilter

OUT = os.path.join(os.path.dirname(__file__), "..", "..", "Art", "UI", "Compass")
os.makedirs(OUT, exist_ok=True)
S = 1024
im = Image.new("L", (S, S), 0)
d = ImageDraw.Draw(im)
c = S / 2
rng = np.random.default_rng(9)

# outer rings
for r, w in ((470, 34), (420, 12), (330, 18)):   # bold: seen at arm's length on a 9 cm lid
    d.ellipse([c - r, c - r, c + r, c + r], outline=255, width=w)
# runes between the rings (Elder Futhark-like strokes)
RUNES = [[[(0, 0), (0, 1)], [(0, .75), (.5, 1)], [(0, .45), (.5, .7)]], [[(0, 0), (0, 1), (.5, .7), (.5, 0)]],
         [[(0, 0), (0, 1)], [(0, .7), (.4, .5), (0, .3)]], [[(0, 0), (0, 1)], [(0, 1), (.45, .75)], [(0, .7), (.45, .45)]],
         [[(0, 0), (0, 1), (.4, .8), (0, .55), (.4, 0)]], [[(.4, 1), (0, .5), (.4, 0)]], [[(0, 0), (.6, 1)], [(0, 1), (.6, 0)]],
         [[(.3, 0), (.3, 1)], [(0, .7), (.3, 1), (.6, .7)]], [[(0, 0), (0, 1)], [(.5, 0), (.5, 1)], [(0, .6), (.5, .4)]]]
N = 16
for i in range(N):
    a = i / N * 2 * math.pi - math.pi / 2
    rune = RUNES[i % len(RUNES)]
    tx, ty = -math.sin(a), math.cos(a)      # tangent
    nx, ny = math.cos(a), math.sin(a)       # outward
    for st in rune:
        pts = [(c + nx * (378 + (y - .5) * 64) + tx * (x - .25) * 60, c + ny * (378 + (y - .5) * 64) + ty * (x - .25) * 60) for x, y in st]
        d.line(pts, fill=255, width=20, joint="curve")


def branch(x, y, ang, length, width, depth, up=True):
    if depth == 0 or length < 8:
        return
    x2 = x + math.cos(ang) * length
    y2 = y + math.sin(ang) * length
    d.line([(x, y), (x2, y2)], fill=255, width=max(9, int(width)))
    n = 2 if depth > 2 else rng.integers(2, 4)
    spread = 0.55 if up else 0.65
    for k in range(n):
        da = (k - (n - 1) / 2) * spread + rng.uniform(-0.15, 0.15)
        branch(x2, y2, ang + da, length * rng.uniform(0.66, 0.78), width * 0.68, depth - 1, up)


# trunk, crown (up) and roots (down), all kept inside the inner ring
d.line([(c, c + 120), (c, c - 60)], fill=255, width=70)
branch(c, c - 60, -math.pi / 2, 120, 48, 4, True)
branch(c, c + 120, math.pi / 2, 80, 40, 3, False)
for side in (-1, 1):
    branch(c, c + 100, math.pi / 2 + side * 0.9, 90, 34, 3, False)
    branch(c, c - 20, -math.pi / 2 + side * 0.85, 105, 36, 4, True)
# clip to the inner ring
mask = Image.new("L", (S, S), 0)
ImageDraw.Draw(mask).ellipse([c - 318, c - 318, c + 318, c + 318], fill=255)
tree_layer = Image.composite(im, Image.new("L", (S, S), 0), mask)
rings = Image.composite(Image.new("L", (S, S), 0), im, mask)
out = Image.fromarray(np.maximum(np.asarray(tree_layer), np.asarray(rings))).filter(ImageFilter.GaussianBlur(1.2))
out.save(os.path.join(OUT, "T_CompassEmblem.png"))
print("wrote emblem")
