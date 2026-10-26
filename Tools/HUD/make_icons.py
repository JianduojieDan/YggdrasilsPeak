"""Minimal line icons for the survival HUD (white, alpha = coverage) + an edge frost texture."""
import math, sys, os
import numpy as np
from PIL import Image, ImageDraw, ImageFilter

OUT = sys.argv[1]
os.makedirs(OUT, exist_ok=True)
S = 1024          # drawing size (8x supersampled)
FINAL = 128
W = 58            # stroke width at drawing size (~7 px at 128)


def canvas():
    return Image.new("L", (S, S), 0)


def stroke(d, pts, w=W):
    """Polyline with round caps and joins."""
    pts = [(float(x), float(y)) for x, y in pts]
    if len(pts) > 1:
        d.line(pts, fill=255, width=int(w), joint="curve")
    r = w / 2
    for x, y in pts:
        d.ellipse((x - r, y - r, x + r, y + r), fill=255)


def bez(p0, p1, p2, p3, n=48):
    out = []
    for i in range(n + 1):
        t = i / n
        a = (1 - t) ** 3; b = 3 * (1 - t) ** 2 * t; c = 3 * (1 - t) * t * t; e = t ** 3
        out.append((a * p0[0] + b * p1[0] + c * p2[0] + e * p3[0], a * p0[1] + b * p1[1] + c * p2[1] + e * p3[1]))
    return out


def save(img, name):
    a = img.resize((FINAL, FINAL), Image.LANCZOS)
    rgba = Image.merge("RGBA", (Image.new("L", a.size, 255),) * 3 + (a,))
    rgba.save(os.path.join(OUT, name + ".png"))
    # preview on a dark frosted swatch
    bg = Image.new("RGBA", (FINAL, FINAL), (40, 52, 66, 255))
    bg.alpha_composite(rgba)
    return bg


# ---- food: fork and knife ----
img = canvas(); d = ImageDraw.Draw(img)
fx = 372
for x in (fx - 92, fx, fx + 92):                       # tines
    stroke(d, [(x, 170), (x, 372)])
arc = [(fx + 92 * math.cos(math.pi * t / 32), 372 + 120 * math.sin(math.pi * t / 32)) for t in range(33)]
stroke(d, arc)                                          # cup under the tines
stroke(d, [(fx, 492), (fx, 862)])                       # handle
kx = 640
blade = [(kx, 862), (kx, 170)] + bez((kx, 170), (kx + 120, 220), (kx + 128, 400), (kx + 110, 540))[1:] + [(kx, 552)]
stroke(d, blade)
food = save(img, "T_HUD_Food")

# ---- water: droplet with a small highlight ----
img = canvas(); d = ImageDraw.Draw(img)
pts = []
for i in range(241):
    t = 2 * math.pi * i / 240
    x = math.sin(t) * math.sin(t / 2) ** 1.6
    y = -math.cos(t)
    pts.append((512 + 300 * x, 540 + 330 * y))
stroke(d, pts)
hl = [(512 + 180 * math.cos(a), 600 + 180 * math.sin(a)) for a in np.linspace(math.radians(110), math.radians(165), 16)]
stroke(d, hl, W * 0.8)
water = save(img, "T_HUD_Water")

# ---- cold: snowflake ----
img = canvas(); d = ImageDraw.Draw(img)
c = (512, 512); R = 372
for k in range(6):
    a = math.radians(90 + 60 * k)
    ux, uy = math.cos(a), -math.sin(a)
    stroke(d, [c, (c[0] + ux * R, c[1] + uy * R)], W * 0.86)
    for at, ln in ((0.6, 104),):
        bx, by = c[0] + ux * R * at, c[1] + uy * R * at
        for s in (-1, 1):
            b = a + s * math.radians(45)
            stroke(d, [(bx + math.cos(b) * ln, by - math.sin(b) * ln), (bx, by)], W * 0.86)
cold = save(img, "T_HUD_Cold")

# ---- health: heart ----
img = canvas(); d = ImageDraw.Draw(img)
heart = bez((512, 860), (300, 700), (140, 560), (150, 390))[:-1] + bez((150, 390), (160, 220), (420, 150), (512, 330))[:-1] \
    + bez((512, 330), (604, 150), (864, 220), (874, 390))[:-1] + bez((874, 390), (884, 560), (724, 700), (512, 860))
stroke(d, heart)
health = save(img, "T_HUD_Health")

# ---- stamina: lightning bolt ----
img = canvas(); d = ImageDraw.Draw(img)
stroke(d, [(590, 150), (300, 560), (520, 560), (430, 874), (724, 440), (504, 440), (590, 150)])
stamina = save(img, "T_HUD_Stamina")

# ---- body temperature: thermometer ----
img = canvas(); d = ImageDraw.Draw(img)
bulb = [(512 + 150 * math.cos(a), 720 + 150 * math.sin(a)) for a in np.linspace(math.radians(-60), math.radians(240), 64)]
tube = [(587, 590), (587, 230)] + [(512 + 75 * math.cos(a), 230 - 75 * math.sin(a)) for a in np.linspace(0, math.pi, 24)] + [(437, 590)]
stroke(d, tube + bulb[::-1] + [(587, 590)])
d.ellipse((512 - 72, 720 - 72, 512 + 72, 720 + 72), fill=255)    # mercury
stroke(d, [(512, 700), (512, 400)], W * 0.9)
temperature = save(img, "T_HUD_Temperature")

# ---- altitude: two peaks with a snow line ----
img = canvas(); d = ImageDraw.Draw(img)
stroke(d, [(120, 800), (420, 300), (590, 570), (690, 440), (904, 800), (120, 800)])
stroke(d, [(300, 500), (370, 560), (430, 500), (490, 560), (530, 505)], W * 0.8)
altitude = save(img, "T_HUD_Altitude")

# ---- oxygen: lungs ----
img = canvas(); d = ImageDraw.Draw(img)
stroke(d, [(512, 150), (512, 470)])
for s in (-1, 1):
    m = lambda x: 512 + s * (x - 512)
    stroke(d, [(512, 470), (m(430), 540)])
    lobe = [(m(440), 380)] + bez((m(440), 380), (m(250), 330), (m(140), 650), (m(170), 820))[1:] \
        + bez((m(170), 820), (m(200), 920), (m(430), 900), (m(440), 800))[1:] + [(m(440), 380)]
    stroke(d, lobe)
oxygen = save(img, "T_HUD_Oxygen")

sheet = Image.new("RGBA", (FINAL * 8 + 90, FINAL + 20), (40, 52, 66, 255))
for i, im in enumerate((food, water, cold, health, stamina, temperature, altitude, oxygen)):
    sheet.paste(im, (10 + i * (FINAL + 10), 10))
sheet = sheet.resize((sheet.width * 2, sheet.height * 2), Image.LANCZOS)
sheet.save(os.path.join(OUT, "_preview_icons.png"))

# ---- frost along the screen edges ----
rng = np.random.default_rng(7)
FW, FH = 2048, 1152
yy, xx = np.mgrid[0:FH, 0:FW].astype(np.float32)
u = xx / FW; v = yy / FH
edge = np.minimum.reduce([u, 1 - u, v * FH / FW * 1.0, (1 - v) * FH / FW])  # distance to nearest edge (in widths)
corner = np.sqrt(((u - 0.5) * 2) ** 2 + ((v - 0.5) * 2) ** 2) / math.sqrt(2)


def fbm(shape, octaves=5, base=8):
    acc = np.zeros(shape, np.float32); amp = 1.0; tot = 0
    for o in range(octaves):
        n = base * 2 ** o
        g = rng.random((n + 1, int(n * shape[1] / shape[0]) + 1)).astype(np.float32)
        im = Image.fromarray((g * 255).astype(np.uint8)).resize((shape[1], shape[0]), Image.BICUBIC)
        acc += amp * np.asarray(im, np.float32) / 255; tot += amp; amp *= 0.55
    return acc / tot


n = fbm((FH, FW))
P = 3.2
sq = (np.abs(2 * u - 1) ** P + np.abs(2 * v - 1) ** P) ** (1 / P)      # rounded-rectangle distance, 1 at the edges
start = 0.80 + 0.06 * (n - 0.5) * 2
mask = np.clip((sq - start) / (1.0 - start + 0.12), 0, 1) ** 1.4

# crystals: small hexagonal ferns, more of them where the frost is thick
cr = Image.new("L", (FW, FH), 0); d = ImageDraw.Draw(cr)


def fern(x, y, ang, ln, depth, shade):
    if depth == 0 or ln < 3:
        return
    x2, y2 = x + math.cos(ang) * ln, y + math.sin(ang) * ln
    d.line([(x, y), (x2, y2)], fill=shade, width=1 if depth < 3 else 2)
    steps = 3
    for k in range(1, steps + 1):
        t = k / (steps + 1)
        bx, by = x + (x2 - x) * t, y + (y2 - y) * t
        for sgn in (-1, 1):
            fern(bx, by, ang + sgn * math.pi / 3, ln * 0.42 * (1.1 - t * 0.5), depth - 1, int(shade * 0.85))


ys, xs = np.nonzero(mask > 0.15)
pick = rng.choice(len(xs), 2600, p=(mask[ys, xs] ** 1.5) / (mask[ys, xs] ** 1.5).sum())
for i in pick:
    x, y = float(xs[i]), float(ys[i])
    toward = math.atan2(FH / 2 - y, FW / 2 - x)                  # grow inwards, loosely
    fern(x, y, toward + rng.uniform(-1.2, 1.2), rng.uniform(14, 46) * (0.6 + mask[int(y), int(x)]), 3, int(rng.uniform(120, 230)))
cr = np.asarray(cr.filter(ImageFilter.GaussianBlur(0.8)), np.float32) / 255

grain = fbm((FH, FW), 3, 96)
alpha = np.clip(mask * (0.25 + 0.5 * grain) * 0.8 + np.clip(mask * 2, 0, 1) * cr * 0.85, 0, 1)
col = np.stack([0.86 + 0.1 * cr, 0.93 + 0.05 * cr, np.ones_like(cr)], -1)
rgba = np.concatenate([col, alpha[..., None]], -1)
frost = Image.fromarray((rgba * 255).astype(np.uint8), "RGBA").resize((1024, 576), Image.LANCZOS)
frost.save(os.path.join(OUT, "T_HUD_FrostEdge.png"))
prev = Image.new("RGBA", frost.size, (70, 80, 90, 255)); prev.alpha_composite(frost)
prev.save(os.path.join(OUT, "_preview_frost.png"))
# ---- glass backing: navy, lighter at the top (drawn as a rounded box, so only the gradient matters) ----
t = np.linspace(0, 1, 64)[:, None]
glass = np.concatenate([np.broadcast_to(np.stack([0.16 - 0.11 * t, 0.21 - 0.14 * t, 0.29 - 0.18 * t], -1), (64, 4, 3)),
                        np.broadcast_to((0.62 - 0.17 * t)[..., None], (64, 4, 1))], -1)
Image.fromarray((glass * 255).astype(np.uint8), "RGBA").save(os.path.join(OUT, "T_HUD_Glass.png"))

# ---- bar fill: white, a touch darker at the start and a soft sheen on the upper half (tinted by code) ----
u = np.linspace(0, 1, 128)[None, :]; v = np.linspace(0, 1, 16)[:, None]
lum = (0.78 + 0.22 * u) * (1.0 + 0.18 * np.exp(-((v - 0.25) / 0.18) ** 2)) * (1.0 - 0.15 * np.clip((v - 0.6) / 0.4, 0, 1))
lum = np.clip(lum / lum.max(), 0, 1)
fill = np.stack([lum, lum, lum, np.ones_like(lum)], -1)
Image.fromarray((fill * 255).astype(np.uint8), "RGBA").save(os.path.join(OUT, "T_HUD_BarFill.png"))
print("ok")
