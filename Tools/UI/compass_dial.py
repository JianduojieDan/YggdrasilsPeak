# Textures for Father's rune compass (Tools/Blender/compass_v2.py), both 2048 px, mapped 0..1 across their discs:
#   T_CompassDial  - aged cream enamel dial: degree ticks, an eight-point rose in blued steel and brass, cardinal
#                    letters, the ring of runes, faint water stains and crazing of old enamel
#   T_CompassLidIn - the inside of the lid: the World Tree engraving (Tools/UI/compass_emblem.py) as gold lines on
#                    dark brass, with a fine guilloche (engine-turned) background and a polished rim
# Output Art/UI/Compass/.   python Tools/UI/compass_dial.py
import numpy as np, os, math
from PIL import Image, ImageDraw, ImageFilter, ImageFont, ImageChops
from scipy import ndimage

ROOT = os.path.join(os.path.dirname(__file__), "..", "..")
OUT = os.path.join(ROOT, "Art", "UI", "Compass")
os.makedirs(OUT, exist_ok=True)
S = 2048
c = S / 2
SERIF = os.path.join(ROOT, "Imports", "Fonts", "Alegreya.ttf")
rng = np.random.default_rng(3)


def noise(scale, seed):
    r = np.random.default_rng(seed).standard_normal((S // scale + 2, S // scale + 2))
    n = ndimage.zoom(r, scale, order=3)[:S, :S]
    return n / (np.abs(n).max() + 1e-6)


# ---------------------------------------------------------------- dial
base = np.ones((S, S, 3)) * np.array([0.90, 0.85, 0.72])
base *= (1 + noise(256, 1)[..., None] * 0.035 + noise(24, 2)[..., None] * 0.015)
yy, xx = np.mgrid[0:S, 0:S]
r = np.hypot(xx - c, yy - c) / (S / 2)
base *= (1 - np.clip((r - 0.75) / 0.25, 0, 1)[..., None] * 0.12)          # darker towards the rim
stain = np.clip(noise(320, 5) * 2 - 1.1, 0, 1)                             # old water stains
base = base * (1 - stain[..., None] * 0.12) + np.array([0.55, 0.42, 0.25]) * stain[..., None] * 0.08
dial = Image.fromarray((np.clip(base, 0, 1) * 255).astype(np.uint8), "RGB")
d = ImageDraw.Draw(dial)
INK = (38, 32, 28)
BLUE = (34, 46, 74)
BRASS = (150, 110, 50)

def at(rad, ang):     # ang 0 = up (north), clockwise
    return (c + math.sin(ang) * rad, c - math.cos(ang) * rad)

R = S / 2
d.ellipse([c - R * 0.985, c - R * 0.985, c + R * 0.985, c + R * 0.985], outline=INK, width=10)
d.ellipse([c - R * 0.955, c - R * 0.955, c + R * 0.955, c + R * 0.955], outline=INK, width=4)
for deg in range(0, 360, 2):
    a = math.radians(deg)
    l = 0.09 if deg % 30 == 0 else (0.06 if deg % 10 == 0 else 0.035)
    w = 6 if deg % 30 == 0 else (4 if deg % 10 == 0 else 2)
    d.line([at(R * 0.955, a), at(R * (0.955 - l), a)], fill=INK, width=w)
# runes in a band
d.ellipse([c - R * 0.80, c - R * 0.80, c + R * 0.80, c + R * 0.80], outline=INK, width=4)
d.ellipse([c - R * 0.66, c - R * 0.66, c + R * 0.66, c + R * 0.66], outline=INK, width=4)
RUNES = [[[(0, 0), (0, 1)], [(0, .75), (.5, 1)], [(0, .45), (.5, .7)]], [[(0, 0), (0, 1), (.5, .7), (.5, 0)]],
         [[(0, 0), (0, 1)], [(0, .7), (.4, .5), (0, .3)]], [[(0, 0), (0, 1)], [(0, 1), (.45, .75)], [(0, .7), (.45, .45)]],
         [[(0, 0), (0, 1), (.4, .8), (0, .55), (.4, 0)]], [[(.4, 1), (0, .5), (.4, 0)]], [[(0, 0), (.6, 1)], [(0, 1), (.6, 0)]],
         [[(.3, 0), (.3, 1)], [(0, .7), (.3, 1), (.6, .7)]], [[(0, 0), (0, 1)], [(.5, 0), (.5, 1)], [(0, .6), (.5, .4)]],
         [[(0, 0), (0, 1)]], [[(.3, 0), (.3, 1)], [(0, .3), (.6, .7)]], [[(0, 1), (.3, .5), (0, 0)], [(.6, 1), (.3, .5), (.6, 0)]]]
rune_mask = Image.new("L", (S, S), 0); rm = ImageDraw.Draw(rune_mask)       # where the runes are: they glow in game
N = 24
for i in range(N):
    a = (i + 0.5) / N * 2 * math.pi
    if any(abs(((a - k * math.pi / 2) + math.pi) % (2 * math.pi) - math.pi) < 0.2 for k in range(4)):   # leave room round N E S W
        continue
    t = (math.cos(a), math.sin(a)); n = (math.sin(a), -math.cos(a))
    for st in RUNES[i % len(RUNES)]:
        pts = [(c + n[0] * R * (0.73 + (y - .5) * 0.09) + t[0] * (x - .25) * R * 0.07,
                c + n[1] * R * (0.73 + (y - .5) * 0.09) + t[1] * (x - .25) * R * 0.07) for x, y in st]
        d.line(pts, fill=(120, 80, 30), width=9, joint="curve")
        rm.line(pts, fill=255, width=13, joint="curve")
# cardinal letters
font = ImageFont.truetype(SERIF, 150)
for k, L in enumerate("NESW"):
    a = k * math.pi / 2
    x, y = at(R * 0.73, a)
    d.text((x, y), L, font=font, fill=(150, 28, 22) if L == "N" else INK, anchor="mm")
# eight-point rose: long points blued steel / brass halves, short points in between
def point(ang, length, width, col_l, col_r):
    tip = at(length, ang); l = at(width, ang - math.pi / 2); rr = at(width, ang + math.pi / 2)
    d.polygon([(c, c), l, tip], fill=col_l); d.polygon([(c, c), tip, rr], fill=col_r)
    d.line([(c, c), tip], fill=INK, width=3); d.line([l, tip, rr], fill=INK, width=3)
for k in range(4):
    point(k * math.pi / 2 + math.pi / 4, R * 0.42, R * 0.07, (200, 165, 95), BRASS)
for k in range(4):
    point(k * math.pi / 2, R * 0.6, R * 0.1, BLUE, (70, 90, 130))
d.ellipse([c - R * 0.06, c - R * 0.06, c + R * 0.06, c + R * 0.06], fill=BRASS, outline=INK, width=4)
dial = dial.filter(ImageFilter.GaussianBlur(0.8))
# crazing of old enamel: very fine dark cracks
craze = Image.new("L", (S, S), 0); cd = ImageDraw.Draw(craze)
for _ in range(160):
    x, y = rng.uniform(0, S, 2); ang = rng.uniform(0, 6.28)
    pts = [(x, y)]
    for _ in range(rng.integers(3, 8)):
        ang += rng.uniform(-0.8, 0.8); x += math.cos(ang) * rng.uniform(20, 70); y += math.sin(ang) * rng.uniform(20, 70); pts.append((x, y))
    cd.line(pts, fill=40, width=2)
dial = ImageChops.subtract(dial, Image.merge("RGB", [craze] * 3))
dial.save(os.path.join(OUT, "T_CompassDial.png"))
rune_mask.filter(ImageFilter.GaussianBlur(3)).save(os.path.join(OUT, "T_CompassDialRunes.png"))
print("wrote dial")

# ---------------------------------------------------------------- lid inside
em = np.asarray(Image.open(os.path.join(OUT, "T_CompassEmblem.png")).convert("L").resize((int(S * 0.86), int(S * 0.86)))) / 255.0
pad = (S - em.shape[0]) // 2
mask = np.zeros((S, S)); mask[pad:pad + em.shape[0], pad:pad + em.shape[1]] = em
ang = np.arctan2(yy - c, xx - c)
guilloche = 0.5 + 0.5 * np.sin(r * 180 + np.sin(ang * 24) * 3.0)            # engine-turned waves
dark = np.array([0.30, 0.20, 0.09]) * (0.8 + 0.2 * guilloche[..., None]) * (1 + noise(128, 7)[..., None] * 0.08)
gold = np.array([0.86, 0.64, 0.28]) * (1 + noise(32, 8)[..., None] * 0.05)
lid = dark * (1 - mask[..., None]) + gold * mask[..., None]
rim = np.clip((r - 0.93) / 0.04, 0, 1)                                    # polished rim
lid = lid * (1 - rim[..., None]) + np.array([0.80, 0.60, 0.30]) * rim[..., None]
Image.fromarray((np.clip(lid, 0, 1) * 255).astype(np.uint8), "RGB").save(os.path.join(OUT, "T_CompassLidIn.png"))
Image.fromarray((np.clip(np.maximum(mask, rim), 0, 1) * 255).astype(np.uint8), "L").save(os.path.join(OUT, "T_CompassLidInMask.png"))
print("wrote lid")
