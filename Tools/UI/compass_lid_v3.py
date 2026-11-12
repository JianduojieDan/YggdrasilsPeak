# Inside of the compass lid, v3 (2026-11-12, user: the old engraving was ugly - blocky tree, a guilloche that shimmered).
#   python Tools/UI/compass_lid_v3.py  -> Art/UI/Compass/T_CompassLidIn.png (colour) + T_CompassLidInMask.png (glow)
# A finely engraved World Tree in raised gilt on dark blued brass: a trunk that splits again and again into a crown
# whose finest twigs touch the circle of the world, roots mirroring it below, three small wells between the roots,
# the Elder Futhark around it between hairlines (as on the dial), a polished rim. The metal is lit by an emboss of
# the line work so it reads raised; the background is a soft, irregular brushing (no regular pattern: no moire).
import math, os
import numpy as np
from PIL import Image, ImageDraw, ImageFilter
from scipy import ndimage

ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..")
OUT = os.path.join(ROOT, "Art", "UI", "Compass")
FINAL, S = 2048, 4096
c = S / 2
R = S / 2
rng = np.random.default_rng(21)
yy, xx = np.mgrid[0:S, 0:S].astype(np.float32)
rr = np.hypot(xx - c, yy - c) / R
ang = np.arctan2(yy - c, xx - c)


def noise(scale, seed):
    r = np.random.default_rng(seed).standard_normal((S // scale + 3, S // scale + 3))
    n = ndimage.zoom(r, scale, order=3)[:S, :S]
    return n / (np.abs(n).max() + 1e-6)


def P(x, y):
    """unit coords (-1..1, y up) -> pixels"""
    return (c + x * R, c - y * R)


lines = Image.new("L", (S, S), 0)          # raised gilt line work
ld = ImageDraw.Draw(lines)


def stroke(pts, w):
    ld.line([P(*p) for p in pts], fill=255, width=max(2, int(w)), joint="curve")
    for p in (pts[0], pts[-1]):
        x, y = P(*p)
        ld.ellipse([x - w / 2, y - w / 2, x + w / 2, y + w / 2], fill=255)


def ring_line(r, w):
    ld.ellipse([c - r * R, c - r * R, c + r * R, c + r * R], outline=255, width=int(w))


# ---- rings
for r_, w_ in ((0.935, 16), (0.905, 6), (0.775, 6), (0.745, 14)):
    ring_line(r_, w_)

# ---- Elder Futhark between the hairlines (same letter shapes as the dial)
FUTHARK = [
    [[(0, 0), (0, 1)], [(0, .55), (.45, .8)], [(0, .8), (.45, 1.02)]], [[(0, 0), (0, 1), (.5, .72), (.5, 0)]],
    [[(0, 0), (0, 1)], [(0, .72), (.42, .5), (0, .28)]], [[(0, 0), (0, 1)], [(0, 1), (.45, .75)], [(0, .72), (.45, .47)]],
    [[(0, 0), (0, 1), (.42, .8), (0, .56), (.45, 0)]], [[(.38, .82), (0, .5), (.38, .18)]], [[(0, 0), (.6, 1)], [(0, 1), (.6, 0)]],
    [[(0, 0), (0, 1), (.42, .8), (0, .58)]], [[(0, 0), (0, 1)], [(.5, 0), (.5, 1)], [(0, .66), (.5, .38)]],
    [[(.3, 0), (.3, 1)], [(.05, .68), (.55, .36)]], [[(.3, 0), (.3, 1)]], [[(.28, 1), (0, .74), (.28, .5)], [(.32, .5), (.6, .26), (.32, 0)]],
    [[(.3, 0), (.3, 1)], [(.3, 1), (.58, .8)], [(.3, 0), (.02, .2)]], [[(0, 0), (0, 1)], [(0, 1), (.36, .72), (.56, .92)], [(0, 0), (.36, .28), (.56, .08)]],
    [[(.3, 0), (.3, 1)], [(.3, .6), (0, 1)], [(.3, .6), (.6, 1)]], [[(.48, 1), (.06, .66), (.52, .34), (.1, 0)]],
    [[(.3, 0), (.3, 1)], [(0, .7), (.3, 1), (.6, .7)]], [[(0, 0), (0, 1), (.42, .76), (0, .5), (.42, .24), (0, 0)]],
    [[(0, 0), (0, 1), (.3, .66), (.6, 1), (.6, 0)]], [[(0, 0), (0, 1)], [(.6, 0), (.6, 1)], [(0, 1), (.6, .55)], [(.6, 1), (0, .55)]],
    [[(0, 0), (0, 1), (.42, .72)]], [[(.3, .8), (.56, .5), (.3, .2), (.04, .5), (.3, .8)]],
    [[(0, 0), (0, 1), (.6, 0), (.6, 1), (0, 0)]], [[(0, 0), (.5, .48), (.3, .78), (.1, .48), (.6, 0)]],
]
MID, H = 0.84, 0.085
for i, st in enumerate(FUTHARK):
    a = math.pi / 2 - (i + 0.5) / 24 * 2 * math.pi           # clockwise from the top
    t = (math.sin(a), -math.cos(a))                           # along the ring
    n = (math.cos(a), math.sin(a))                            # outward
    for s in st:
        pts = [(n[0] * (MID + (y - .5) * H) + t[0] * (x - .3) * H * 0.62, n[1] * (MID + (y - .5) * H) + t[1] * (x - .3) * H * 0.62) for x, y in s]
        stroke(pts, 13)
    b = math.pi / 2 - (i + 1) / 24 * 2 * math.pi
    x, y = math.cos(b) * MID, math.sin(b) * MID
    px, py = P(x, y)
    ld.ellipse([px - 7, py - 7, px + 7, py + 7], fill=255)

# ---- the World Tree: recursive, tapering, a little irregular; crown up, roots down
leaves = []


def branch(x, y, a, length, w, depth, up):
    # a slightly curved limb in a few segments
    pts = [(x, y)]
    bend = rng.uniform(-0.25, 0.25)
    for k in range(1, 6):
        aa = a + bend * k / 5
        x += math.cos(aa) * length / 5
        y += math.sin(aa) * length / 5
        pts.append((x, y))
    # a twig that would reach past the circle of the world stops short of it and ends in a bud
    edge = False
    if math.hypot(x, y) > 0.70:
        keep = [q for q in pts if math.hypot(*q) <= 0.70]
        if len(keep) < 2:
            return
        pts = keep; x, y = pts[-1]; edge = True
    stroke(pts, w)
    if depth == 0 or w < 2.5 or edge:
        if up:
            leaves.append((x, y))
        return
    n = 2 if depth > 2 else (2 if rng.random() < 0.7 else 3)
    spread = (0.42 if up else 0.55) * (1.0 + 0.15 * (depth < 3))
    for k in range(n):
        off = (k - (n - 1) / 2) * spread * 2 / max(n - 1, 1) + rng.uniform(-0.08, 0.08)
        # the crown leans outward and up, the roots outward and down
        na = a + off
        branch(x, y, na, length * rng.uniform(0.66, 0.78), w * 0.66, depth - 1, up)


TRUNK_W = 70
stroke([(0, -0.18), (0.006, -0.05), (-0.004, 0.08), (0, 0.16)], TRUNK_W)
for k, a0 in enumerate((math.pi / 2 - 0.62, math.pi / 2 - 0.2, math.pi / 2 + 0.2, math.pi / 2 + 0.62)):
    branch(0, 0.14 - 0.03 * (k in (0, 3)), a0, 0.2, TRUNK_W * 0.62, 7, True)
for k, a0 in enumerate((-math.pi / 2 - 0.75, -math.pi / 2 - 0.28, -math.pi / 2 + 0.28, -math.pi / 2 + 0.75)):
    branch(0, -0.16, a0, 0.17, TRUNK_W * 0.55, 6, False)
# leaves: tiny round buds at the twig tips (they glow with the runes)
for (x, y) in leaves:
    px, py = P(x, y)
    r = rng.uniform(5, 9)
    ld.ellipse([px - r, py - r, px + r, py + r], fill=255)
# three wells between the roots (Urd, Mimir, Hvergelmir): small rings
for wx in (-0.3, 0.0, 0.3):
    wy = -0.6 + 0.08 * abs(wx) / 0.3
    px, py = P(wx, wy)
    rad = 0.035 * R
    ld.ellipse([px - rad, py - rad * 0.55, px + rad, py + rad * 0.55], outline=255, width=9)

# ---- light the metal: emboss the line work (light from the upper left) so the gilt reads raised
L = np.asarray(lines).astype(np.float32) / 255.0
Ls = ndimage.gaussian_filter(L, 3.0)
gy, gx = np.gradient(Ls)
light = np.clip(0.5 + (-gx * 0.7 + -gy * 0.7) * 10.0, 0, 1)
L = np.clip(ndimage.gaussian_filter(L, 0.8), 0, 1)

# background: dark blued brass, a soft irregular brushing along the circle, darker towards the rim
brush = ndimage.gaussian_filter(np.random.default_rng(5).standard_normal((S, S)).astype(np.float32), (0.8, 0.8))
# smear the noise along circles: average over a few rotations of the angle (cheap tangential blur)
tang = np.zeros_like(brush)
for k in range(-4, 5):
    tang += ndimage.rotate(brush, k * 0.35, reshape=False, order=1, mode="reflect")
tang = tang / np.abs(tang).max()
bg = np.ones((S, S, 3), np.float32) * np.array([0.085, 0.075, 0.07])
bg *= (1 + 0.10 * tang[..., None] + 0.06 * noise(256, 8)[..., None])
bg *= (1 - 0.35 * np.clip((rr - 0.6) / 0.35, 0, 1)[..., None])
# a faint warm glow in the middle, behind the tree
bg += np.array([0.06, 0.04, 0.015]) * np.exp(-(rr / 0.45) ** 2)[..., None]

gold = np.array([0.88, 0.66, 0.30])
gilt = gold[None, None, :] * (0.55 + 0.75 * light[..., None]) * (1 + 0.05 * noise(32, 9)[..., None])
img = bg * (1 - L[..., None]) + gilt * L[..., None]
# polished rim
rim = np.clip((rr - 0.955) / 0.02, 0, 1)[..., None]
sheen = 0.75 + 0.35 * np.cos(ang - 2.4)[..., None]
img = img * (1 - rim) + np.array([0.80, 0.60, 0.30]) * sheen * rim
img = np.clip(img, 0, 1)

col = Image.fromarray((img * 255).astype(np.uint8), "RGB").resize((FINAL, FINAL), Image.LANCZOS)
col.save(os.path.join(OUT, "T_CompassLidIn.png"))
mask = np.clip(np.maximum(L * (rr < 0.93), rim[..., 0] * 0.0), 0, 1)
Image.fromarray((mask * 255).astype(np.uint8), "L").resize((FINAL, FINAL), Image.LANCZOS).save(os.path.join(OUT, "T_CompassLidInMask.png"))
col.resize((1024, 1024), Image.LANCZOS).save(os.path.join(ROOT, "Saved", "Compare", "compass_lid_v3_preview.png"))
print("wrote lid v3")
