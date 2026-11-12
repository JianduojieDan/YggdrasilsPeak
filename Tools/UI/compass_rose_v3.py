# Father's rune compass, dial v3 (2026-11-12, user: "the compass is not good looking"; reference
# Docs/References/compass_rose_reference.webp, an antique hand-coloured wind rose - inspiration, not a copy).
#   python Tools/UI/compass_rose_v3.py      -> Art/UI/Compass/T_CompassDial.png + T_CompassDialRunes.png (+ _preview.png)
# What it takes from the reference: a 32-point rose of long tapering points, each split in a light and a dark half;
# concentric painted bands (gilt rim, madder tick band, faded teal degree band); a medallion in the middle;
# an ornament at north; warm hand-coloured, aged paper. What makes it ours:
# - the whole Elder Futhark (24 runes) is the rune ring (it glows in the game: T_CompassDialRunes),
# - the four cardinal points carry the four dwarfs who hold up the sky in Norse myth: NORDRI, AUSTRI, SUDRI, VESTRI,
# - north is marked by the Tiwaz rune (an arrow, Tyr's rune) instead of a fleur-de-lis,
# - the medallion is the World Tree (the lid's engraving, T_CompassEmblem) in gold on deep blue.
import math, os
import numpy as np
from PIL import Image, ImageDraw, ImageFilter, ImageFont
from scipy import ndimage

ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..")
OUT = os.path.join(ROOT, "Art", "UI", "Compass")
SERIF = os.path.join(ROOT, "Imports", "Fonts", "Alegreya.ttf")
FINAL = 2048
S = 4096                       # drawn at twice the size, then reduced: clean edges
c = S / 2
R = S / 2
rng = np.random.default_rng(12)

INK = (44, 30, 22)
PAPER = np.array([0.93, 0.875, 0.73])


def noise(scale, seed, size=S):
    r = np.random.default_rng(seed).standard_normal((size // scale + 3, size // scale + 3))
    n = ndimage.zoom(r, scale, order=3)[:size, :size]
    return n / (np.abs(n).max() + 1e-6)


def at(rad, ang):
    """ang 0 = north (up), clockwise; rad in pixels"""
    return (c + math.sin(ang) * rad, c - math.cos(ang) * rad)


yy, xx = np.mgrid[0:S, 0:S].astype(np.float32)
rr = np.hypot(xx - c, yy - c) / R
ang_map = np.arctan2(xx - c, -(yy - c))          # 0 north, clockwise

# ------------------------------------------------------------------ paper
paper = np.ones((S, S, 3), np.float32) * PAPER
paper *= (1 + noise(512, 1)[..., None] * 0.035 + noise(64, 2)[..., None] * 0.018 + noise(8, 3)[..., None] * 0.012)
fibre = ndimage.gaussian_filter(np.random.default_rng(4).standard_normal((S, S)).astype(np.float32), (1.0, 6.0))
paper *= (1 + fibre[..., None] * 0.02)
paper *= (1 - np.clip((rr - 0.55) / 0.45, 0, 1)[..., None] ** 2 * 0.10)       # the edge a little browner
paper = paper * (1 - 0.06 * np.clip(noise(700, 5) * 1.5 - 0.4, 0, 1)[..., None]) # big soft tea stains

# paint (watercolour) and ink are drawn as separate layers
paint = Image.new("RGBA", (S, S), (0, 0, 0, 0))
pd = ImageDraw.Draw(paint)
ink = Image.new("L", (S, S), 0)
idr = ImageDraw.Draw(ink)
gild = Image.new("L", (S, S), 0)
gd = ImageDraw.Draw(gild)


def ring(r0, r1, fill, draw=pd):
    draw.ellipse([c - r1, c - r1, c + r1, c + r1], fill=fill)
    draw.ellipse([c - r0, c - r0, c + r0, c + r0], fill=(0, 0, 0, 0) if draw is pd else 0)


def circle_line(r, w):
    idr.ellipse([c - r, c - r, c + r, c + r], outline=255, width=int(w))


# ------------------------------------------------------------------ bands (outside in)
B_RIM0, B_RIM1 = 0.935 * R, 0.995 * R          # gilt rim
B_RED0 = 0.885 * R                             # madder tick band
B_TEAL0 = 0.795 * R                            # faded teal degree band
B_RUNE0 = 0.695 * R                            # rune ring on paper
ring(B_RIM0, B_RIM1, (205, 160, 70, 255))
gd.ellipse([c - B_RIM1, c - B_RIM1, c + B_RIM1, c + B_RIM1], fill=255)
gd.ellipse([c - B_RIM0, c - B_RIM0, c + B_RIM0, c + B_RIM0], fill=0)
# madder band: alternate painted and bare cells every 5 degrees (like an old chart's degree scale)
for k in range(72):
    a0, a1 = math.radians(k * 5), math.radians(k * 5 + 5)
    if k % 2 == 0:
        pts = [at(B_RED0, a0)] + [at(B_RIM0, a0 + (a1 - a0) * t / 8) for t in range(9)] + [at(B_RED0, a1)]
        pd.polygon(pts, fill=(176, 84, 86, 235))
    else:
        pts = [at(B_RED0, a0)] + [at(B_RIM0, a0 + (a1 - a0) * t / 8) for t in range(9)] + [at(B_RED0, a1)]
        pd.polygon(pts, fill=(222, 196, 170, 140))
ring(B_TEAL0, B_RED0, (104, 160, 150, 225))
for r_, w_ in ((B_RIM1, 14), (B_RIM0, 9), (B_RIM0 - 12, 3), (B_RED0, 7), (B_TEAL0, 8), (B_TEAL0 - 14, 3), (B_RUNE0, 8)):
    circle_line(r_, w_)
# ticks: every degree in the madder band, numbers every ten in the teal band
for deg in range(360):
    a = math.radians(deg)
    l = 0.62 if deg % 10 == 0 else (0.42 if deg % 5 == 0 else 0.26)
    idr.line([at(B_RED0, a), at(B_RED0 + (B_RIM0 - B_RED0) * l, a)], fill=255, width=6 if deg % 10 == 0 else 4)
num_font = ImageFont.truetype(SERIF, 92)


def radial_text(text, rad, a, font, layer, fill=255, upright=True, spacing=0):
    """text centred at (rad, a), its baseline along the circle, reading clockwise (bottom half flipped to stay upright)"""
    w = int(font.getlength(text)) + 40 + spacing * len(text)
    h = int(font.size * 1.6)
    im = Image.new("L", (w, h), 0)
    d = ImageDraw.Draw(im)
    if spacing:
        x = 20
        for ch in text:
            d.text((x, h / 2), ch, font=font, fill=255, anchor="lm")
            x += font.getlength(ch) + spacing
    else:
        d.text((w / 2, h / 2), text, font=font, fill=255, anchor="mm")
    deg = math.degrees(a) % 360
    flip = upright and 100 < deg < 260
    im = im.rotate(-(deg + (180 if flip else 0)), resample=Image.BICUBIC, expand=True)
    x, y = at(rad, a)
    if isinstance(layer, ImageDraw.ImageDraw):
        raise ValueError
    layer.paste(Image.new("L", im.size, fill) if layer.mode == "L" else Image.new("RGBA", im.size, fill),
                (int(x - im.size[0] / 2), int(y - im.size[1] / 2)), im)


for deg in range(10, 360, 10):
    if deg % 90 == 0:
        continue
    radial_text(str(deg), (B_TEAL0 + B_RED0) / 2, math.radians(deg), num_font, ink)

# ------------------------------------------------------------------ Elder Futhark ring (24 runes, the glowing ring)
# strokes in a 0.6 x 1 box, y up
FUTHARK = [
    [[(0, 0), (0, 1)], [(0, .55), (.45, .8)], [(0, .8), (.45, 1.02)]],                     # fehu
    [[(0, 0), (0, 1), (.5, .72), (.5, 0)]],                                                 # uruz
    [[(0, 0), (0, 1)], [(0, .72), (.42, .5), (0, .28)]],                                    # thurisaz
    [[(0, 0), (0, 1)], [(0, 1), (.45, .75)], [(0, .72), (.45, .47)]],                       # ansuz
    [[(0, 0), (0, 1), (.42, .8), (0, .56), (.45, 0)]],                                      # raidho
    [[(.38, .82), (0, .5), (.38, .18)]],                                                    # kenaz
    [[(0, 0), (.6, 1)], [(0, 1), (.6, 0)]],                                                 # gebo
    [[(0, 0), (0, 1), (.42, .8), (0, .58)]],                                                # wunjo
    [[(0, 0), (0, 1)], [(.5, 0), (.5, 1)], [(0, .66), (.5, .38)]],                          # hagalaz
    [[(.3, 0), (.3, 1)], [(.05, .68), (.55, .36)]],                                         # naudiz
    [[(.3, 0), (.3, 1)]],                                                                   # isa
    [[(.28, 1), (0, .74), (.28, .5)], [(.32, .5), (.6, .26), (.32, 0)]],                    # jera
    [[(.3, 0), (.3, 1)], [(.3, 1), (.58, .8)], [(.3, 0), (.02, .2)]],                       # eihwaz
    [[(0, 0), (0, 1)], [(0, 1), (.36, .72), (.56, .92)], [(0, 0), (.36, .28), (.56, .08)]], # perthro
    [[(.3, 0), (.3, 1)], [(.3, .6), (0, 1)], [(.3, .6), (.6, 1)]],                          # algiz
    [[(.48, 1), (.06, .66), (.52, .34), (.1, 0)]],                                          # sowilo
    [[(.3, 0), (.3, 1)], [(0, .7), (.3, 1), (.6, .7)]],                                     # tiwaz
    [[(0, 0), (0, 1), (.42, .76), (0, .5), (.42, .24), (0, 0)]],                            # berkano
    [[(0, 0), (0, 1), (.3, .66), (.6, 1), (.6, 0)]],                                        # ehwaz
    [[(0, 0), (0, 1)], [(.6, 0), (.6, 1)], [(0, 1), (.6, .55)], [(.6, 1), (0, .55)]],       # mannaz
    [[(0, 0), (0, 1), (.42, .72)]],                                                         # laguz
    [[(.3, .8), (.56, .5), (.3, .2), (.04, .5), (.3, .8)]],                                 # ingwaz
    [[(0, 0), (0, 1), (.6, 0), (.6, 1), (0, 0)]],                                           # dagaz
    [[(0, 0), (.5, .48), (.3, .78), (.1, .48), (.6, 0)]],                                   # othala
]
rune_mask = Image.new("L", (S, S), 0)
rmd = ImageDraw.Draw(rune_mask)
rune_paint = Image.new("L", (S, S), 0)
rpd = ImageDraw.Draw(rune_paint)
RUNE_MID = (B_RUNE0 + B_TEAL0) / 2 - 6
RUNE_H = (B_TEAL0 - B_RUNE0) * 0.62
for i, strokes in enumerate(FUTHARK):
    a = (i + 0.5) / 24 * 2 * math.pi
    t = (math.cos(a), math.sin(a))                  # along the ring (clockwise)
    n = (math.sin(a), -math.cos(a))                 # outward
    for st in strokes:
        pts = [(c + n[0] * (RUNE_MID + (y - .5) * RUNE_H) + t[0] * (x - .3) * RUNE_H * 0.62,
                c + n[1] * (RUNE_MID + (y - .5) * RUNE_H) + t[1] * (x - .3) * RUNE_H * 0.62) for x, y in st]
        rpd.line(pts, fill=255, width=20, joint="curve")
        rmd.line(pts, fill=255, width=30, joint="curve")
    # a small diamond between runes, like the stops of an inscription
    b = (i + 1) / 24 * 2 * math.pi
    x, y = at(RUNE_MID, b)
    rpd.polygon([(x + 10 * math.cos(b), y + 10 * math.sin(b)), at(RUNE_MID + 14, b), (x - 10 * math.cos(b), y - 10 * math.sin(b)), at(RUNE_MID - 14, b)], fill=255)

# ------------------------------------------------------------------ the 32-point rose
# Drawn back to front on a layer of its own, outlines included, so a front point hides the lines behind it;
# each point casts a soft shadow on what lies under it (the rose reads raised, as in old engraved charts).
ROSE = B_RUNE0 - 16
COL = {
    "card_l": (228, 176, 78), "card_d": (70, 40, 22),        # north / south: gilt and umber
    "cew_l": (240, 224, 192), "cew_d": (54, 34, 24),         # east / west: cream and dark umber
    "int_l": (206, 148, 84), "int_d": (100, 54, 28),         # NE SE SW NW
    "by_l": (220, 172, 116), "by_d": (136, 80, 40),          # NNE ...
    "q_l": (232, 204, 158), "q_d": (166, 114, 66),           # the 16 quarter points
}
rose = Image.new("RGBA", (S, S), (0, 0, 0, 0))
ROSE_INK = INK + (255,)


def rose_point(a, length, half_w, light, dark, ink_w, shadow):
    global rose
    tip = at(length, a)
    left = at(half_w, a - math.pi / 2)
    right = at(half_w, a + math.pi / 2)
    o = (c, c)
    if shadow > 0:
        sh = Image.new("L", (S, S), 0)
        ImageDraw.Draw(sh).polygon([(x + shadow, y + shadow * 1.3) for x, y in (left, tip, right, o)], fill=95)
        sh = sh.filter(ImageFilter.GaussianBlur(shadow * 0.9))
        layer = Image.new("RGBA", (S, S), (40, 24, 12, 0))
        layer.putalpha(sh)
        rose = Image.alpha_composite(rose, layer)
    d = ImageDraw.Draw(rose)
    d.polygon([o, left, tip], fill=light + (255,))
    d.polygon([o, tip, right], fill=dark + (255,))
    d.line([left, tip, right], fill=ROSE_INK, width=ink_w, joint="curve")
    d.line([o, tip], fill=ROSE_INK, width=max(3, ink_w - 4))


for k in range(16):      # quarter points
    rose_point((k + 0.5) / 16 * 2 * math.pi, ROSE * 0.50, ROSE * 0.045, COL["q_l"], COL["q_d"], 6, 0)
for k in range(8):       # by-points
    rose_point((k + 0.5) / 8 * 2 * math.pi, ROSE * 0.66, ROSE * 0.065, COL["by_l"], COL["by_d"], 7, 10)
for k in range(4):       # intercardinals
    rose_point((k + 0.5) / 4 * 2 * math.pi, ROSE * 0.80, ROSE * 0.095, COL["int_l"], COL["int_d"], 8, 16)
for k in range(4):       # cardinals
    lt, dk = (COL["card_l"], COL["card_d"]) if k % 2 == 0 else (COL["cew_l"], COL["cew_d"])
    rose_point(k / 4 * 2 * math.pi, ROSE * 0.985, ROSE * 0.125, lt, dk, 10, 24)

# names of the four dwarfs along the cardinal points, written on the light half, reading outwards (upright)
name_font = ImageFont.truetype(SERIF, 104)
for k, name in enumerate(("NORÐRI", "AUSTRI", "SUÐRI", "VESTRI")):
    a = k / 4 * 2 * math.pi
    w = int(name_font.getlength(name)) + 26 * len(name) + 40
    h = 160
    im = Image.new("L", (w, h), 0)
    d = ImageDraw.Draw(im)
    x = 20
    for ch in name:
        d.text((x, h / 2), ch, font=name_font, fill=255, anchor="mm" if False else "lm")
        x += name_font.getlength(ch) + 26
    deg = math.degrees(a) % 360
    rot = -deg + 90
    if 90 < deg <= 270:
        rot += 180
    im2 = im.rotate(rot, resample=Image.BICUBIC, expand=True)
    px, py = at(ROSE * 0.55, a)
    side = at(ROSE * 0.04, a - math.pi / 2)
    px += side[0] - c
    py += side[1] - c
    rose.paste(Image.new("RGBA", im2.size, INK + (255,)), (int(px - im2.size[0] / 2), int(py - im2.size[1] / 2)), im2)

# medallion: the World Tree in gold on deep blue, inside a gilt ring
MED = ROSE * 0.17
_rd = ImageDraw.Draw(rose)
_rd.ellipse([c - MED * 1.14, c - MED * 1.14, c + MED * 1.14, c + MED * 1.14], fill=(196, 150, 62, 255), outline=ROSE_INK, width=9)
_rd.ellipse([c - MED, c - MED, c + MED, c + MED], fill=(34, 54, 84, 255), outline=ROSE_INK, width=7)
emb_path = os.path.join(OUT, "T_CompassEmblem.png")
tree_gold = Image.new("L", (S, S), 0)
if os.path.exists(emb_path):
    em = Image.open(emb_path).convert("L").resize((int(MED * 1.9), int(MED * 1.9)), Image.LANCZOS)
    tree_gold.paste(em, (int(c - em.size[0] / 2), int(c - em.size[1] / 2)))

# north: the Tiwaz rune as an arrow standing on the rim, in madder red and gold
def tiwaz(cx, cy, size, fill_draw, ink_draw):
    stem = [(cx, cy + size), (cx, cy - size)]
    head = [(cx - size * 0.62, cy - size * 0.32), (cx, cy - size), (cx + size * 0.62, cy - size * 0.32)]
    fill_draw.line(stem, fill=(168, 52, 44, 255), width=int(size * 0.28))
    fill_draw.line(head, fill=(168, 52, 44, 255), width=int(size * 0.28), joint="curve")
    ink_draw.line(stem, fill=255, width=6)
    ink_draw.line(head, fill=255, width=6, joint="curve")


nx, ny = at((B_TEAL0 + B_RED0) / 2, 0)
tiwaz(nx, ny, (B_RED0 - B_TEAL0) * 0.42, pd, idr)
# small rosettes at east, south, west on the teal band
for k in (1, 2, 3):
    x, y = at((B_TEAL0 + B_RED0) / 2, k * math.pi / 2)
    rr_ = (B_RED0 - B_TEAL0) * 0.22
    for j in range(8):
        aa = j / 8 * 2 * math.pi
        px, py = x + math.cos(aa) * rr_, y + math.sin(aa) * rr_
        pd.ellipse([px - rr_ * 0.45, py - rr_ * 0.45, px + rr_ * 0.45, py + rr_ * 0.45], fill=(205, 160, 70, 255))
        idr.ellipse([px - rr_ * 0.45, py - rr_ * 0.45, px + rr_ * 0.45, py + rr_ * 0.45], outline=255, width=4)
    pd.ellipse([x - rr_ * 0.5, y - rr_ * 0.5, x + rr_ * 0.5, y + rr_ * 0.5], fill=(168, 52, 44, 255))

# ------------------------------------------------------------------ compose: watercolour, misregistered, then ink
P = np.asarray(paint).astype(np.float32) / 255.0
# the colourist's hand: paint sits a few pixels off the ink, pools darker at its edges, varies in strength
P = np.roll(P, (5, -4), axis=(0, 1))
alpha = P[..., 3]
edge = np.clip(alpha - ndimage.gaussian_filter(alpha, 7), 0, 1)
strength = 0.82 + 0.12 * noise(96, 21) + 0.06 * noise(16, 22)
col = P[..., :3] * (1 - edge[..., None] * 0.9) * strength[..., None]
a_eff = np.clip(alpha * (0.88 + 0.1 * noise(48, 23)), 0, 1)[..., None]
img = paper * (1 - a_eff) + (paper * 0.25 + col * 0.85) * a_eff         # watercolour: the paper shows through a little
# the rose: a richer, opaque layer with the same uneven hand (fills vary, outlines stay dark)
RO = np.asarray(rose).astype(np.float32) / 255.0
ra = RO[..., 3:4]
rcol = RO[..., :3] * (0.9 + 0.1 * strength[..., None]) * (1 + 0.04 * noise(12, 26)[..., None])
img = img * (1 - ra) + (rcol * 0.93 + paper * 0.07) * ra
# gilt: warmer, with a soft sheen across the rim and the medallion ring
G = np.asarray(gild).astype(np.float32)[..., None] / 255.0
sheen = 0.85 + 0.25 * np.cos(ang_map * 2 - 0.6)[..., None] + 0.05 * noise(32, 24)[..., None]
img = img * (1 - G * 0.7) + np.array([0.83, 0.63, 0.27]) * sheen * G * 0.7
# runes: dark umber with a little red, the tree: gold
RP = np.asarray(rune_paint.filter(ImageFilter.GaussianBlur(1.2))).astype(np.float32)[..., None] / 255.0
img = img * (1 - RP) + np.array([0.36, 0.16, 0.08]) * RP
TG = np.asarray(tree_gold.filter(ImageFilter.GaussianBlur(1.0))).astype(np.float32)[..., None] / 255.0
img = img * (1 - TG) + np.array([0.90, 0.72, 0.34]) * TG
# ink: slightly uneven, bleeding a hair into the paper
I = np.asarray(ink.filter(ImageFilter.GaussianBlur(1.4))).astype(np.float32) / 255.0
I = np.clip(I * (0.85 + 0.2 * noise(24, 25)), 0, 1)[..., None]
img = img * (1 - I * 0.92) + np.array(INK) / 255.0 * I * 0.92
# age: foxing spots, a faint craquelure only in the paint, darker rim
fox = np.zeros((S, S), np.float32)
for _ in range(70):
    x, y = rng.uniform(0, S, 2)
    rad = rng.uniform(6, 34)
    fox += np.exp(-((xx - x) ** 2 + (yy - y) ** 2) / (2 * rad ** 2)) * rng.uniform(0.2, 0.6)
img *= (1 - np.clip(fox, 0, 1)[..., None] * np.array([0.12, 0.2, 0.3]))
craze = Image.new("L", (S, S), 0)
cd = ImageDraw.Draw(craze)
for _ in range(260):
    x, y = rng.uniform(0, S, 2)
    a = rng.uniform(0, 6.28)
    pts = [(x, y)]
    for _ in range(int(rng.integers(3, 7))):
        a += rng.uniform(-0.9, 0.9)
        x += math.cos(a) * rng.uniform(30, 90)
        y += math.sin(a) * rng.uniform(30, 90)
        pts.append((x, y))
    cd.line(pts, fill=255, width=2)
C = np.asarray(craze.filter(ImageFilter.GaussianBlur(1))).astype(np.float32) / 255.0
img *= (1 - (C * alpha * 0.18)[..., None])
img *= (1 - np.clip((rr - 0.97) / 0.03, 0, 1)[..., None] * 0.25)
out = Image.fromarray((np.clip(img, 0, 1) * 255).astype(np.uint8), "RGB").resize((FINAL, FINAL), Image.LANCZOS)
out.save(os.path.join(OUT, "T_CompassDial.png"))
rune_mask.filter(ImageFilter.GaussianBlur(6)).resize((FINAL, FINAL), Image.LANCZOS).save(os.path.join(OUT, "T_CompassDialRunes.png"))
out.resize((1024, 1024), Image.LANCZOS).save(os.path.join(ROOT, "Saved", "Compare", "compass_dial_v3_preview.png"))
print("wrote dial v3")
