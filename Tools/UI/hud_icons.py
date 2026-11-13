# HUD v6 line icons (2026-11-13, user's reference sheet: serif numerals, line icons, dotted route).
# Drawn at 256 px with round-capped strokes, saved white on transparent at 128 px so the game tints them:
#   Saved/Review/hud6_icons/T_HUD6_<Name>.png and T_HUD6_<Name>_Halo.png (+ a contact sheet hud6_icons.png)
# Then, in the editor: python -I Tools/RoutePlanning/uepy.py Tools/UI/import_hud6_icons.py -> /Game/UI/HUD/Icons6
# Usage: python -I Tools/UI/hud_icons.py
from PIL import Image, ImageDraw, ImageFilter
import math, os, random
OUT = 'F:/UE_project/YggdrasilsPeak/Saved/Review/hud6_icons/'
os.makedirs(OUT, exist_ok=True)
N = 256
SW = 10                                    # stroke at 256 px (~2.5 px at 64)

def bez(p0, p1, p2, p3, n=48):
    pts = []
    for i in range(n + 1):
        t = i / n; u = 1 - t
        pts.append((u**3 * p0[0] + 3 * u * u * t * p1[0] + 3 * u * t * t * p2[0] + t**3 * p3[0],
                    u**3 * p0[1] + 3 * u * u * t * p1[1] + 3 * u * t * t * p2[1] + t**3 * p3[1]))
    return pts
def mirror(pts): return [(N - x, y) for x, y in pts]

class Icon:
    def __init__(self):
        self.im = Image.new('RGBA', (N, N), (0, 0, 0, 0)); self.d = ImageDraw.Draw(self.im)
    def path(self, pts, w=SW):
        self.d.line(pts, fill=(255, 255, 255, 255), width=w, joint='curve')
        r = w / 2
        for x, y in (pts[0], pts[-1]): self.d.ellipse((x - r, y - r, x + r, y + r), fill=(255, 255, 255, 255))
    def poly(self, pts): self.d.polygon(pts, fill=(255, 255, 255, 255))
    def circle(self, c, r, w=SW, fill=False):
        box = (c[0] - r, c[1] - r, c[0] + r, c[1] + r)
        if fill: self.d.ellipse(box, fill=(255, 255, 255, 255))
        else: self.d.ellipse(box, outline=(255, 255, 255, 255), width=w)
    def erase_circle(self, c, r): self.d.ellipse((c[0] - r, c[1] - r, c[0] + r, c[1] + r), fill=(0, 0, 0, 0))
    def save(self, name, size=128):
        self.im.resize((size, size), Image.LANCZOS).save(OUT + f'T_HUD6_{name}.png')
        # the glow under it (white, soft, a little wider than the strokes): keeps dark ink readable on a dark sky
        a = self.im.split()[3].filter(ImageFilter.MaxFilter(13)).filter(ImageFilter.GaussianBlur(10))
        halo = Image.new('RGBA', self.im.size, (255, 255, 255, 0)); halo.putalpha(a)
        halo.resize((size, size), Image.LANCZOS).save(OUT + f'T_HUD6_{name}_Halo.png')
        return self.im

def lungs():
    I = Icon()
    I.path([(128, 26), (128, 104)])
    I.path(bez((128, 104), (118, 116), (108, 120), (98, 134)))
    I.path(bez((128, 104), (138, 116), (148, 120), (158, 134)))
    left = bez((108, 84), (98, 54), (76, 58), (64, 92)) + bez((64, 92), (46, 134), (36, 190), (52, 214))[1:] + \
           bez((52, 214), (70, 230), (102, 222), (108, 200))[1:] + [(108, 84)]
    I.path(left); I.path(mirror(left))
    return I.save('Lungs')

def thermometer():
    I = Icon()
    I.d.rounded_rectangle((102, 22, 154, 196), 26, fill=(255, 255, 255, 255)); I.circle((128, 198), 40, fill=True)
    I.d.rounded_rectangle((112, 32, 144, 196), 16, fill=(0, 0, 0, 0)); I.erase_circle((128, 198), 30)
    I.d.rounded_rectangle((122, 104, 134, 200), 6, fill=(255, 255, 255, 255)); I.circle((128, 198), 19, fill=True)
    return I.save('Thermo')

def heart():
    I = Icon()
    half = bez((128, 216), (66, 168), (26, 126), (36, 88)) + bez((36, 88), (46, 46), (104, 36), (128, 80))[1:]
    I.path(half); I.path(mirror(half))
    I.path([(14, 136), (82, 136), (98, 108), (118, 172), (140, 92), (156, 148), (166, 136), (200, 136)], w=SW - 1)
    return I.save('Heart')

def food():
    I = Icon()
    for x in (66, 90, 114): I.path([(x, 28), (x, 92)])
    I.path(bez((66, 92), (66, 128), (114, 128), (114, 92)))
    I.path([(90, 122), (90, 228)])
    blade = [(170, 228), (170, 28)] + bez((170, 28), (212, 52), (214, 116), (194, 136))[1:] + [(170, 136)]
    I.path(blade)
    return I.save('Food')

def drop():
    I = Icon()
    half = bez((128, 26), (150, 74), (198, 120), (198, 160)) + bez((198, 160), (198, 202), (166, 232), (128, 232))[1:]
    I.path(half); I.path(mirror(half))
    return I.save('Drop')

def snowflake():
    I = Icon()
    c = (128, 128)
    for k in range(6):
        a = math.radians(90 + 60 * k); dx, dy = math.cos(a), -math.sin(a)
        end = (c[0] + dx * 104, c[1] + dy * 104); I.path([c, end], w=SW - 1)
        for f, L in ((0.55, 30), (0.82, 20)):
            p = (c[0] + dx * 104 * f, c[1] + dy * 104 * f)
            for s in (-1, 1):
                b = a + s * math.radians(45)
                I.path([p, (p[0] + math.cos(b) * L, p[1] - math.sin(b) * L)], w=SW - 2)
    return I.save('Snow')

def wind():
    I = Icon()
    I.path([(22, 92), (172, 92)] + bez((172, 92), (212, 92), (212, 46), (182, 46))[1:])
    I.path([(52, 134), (226, 134)], w=SW - 1)
    I.path([(22, 176), (150, 176)] + bez((150, 176), (190, 176), (190, 222), (160, 222))[1:])
    return I.save('Wind')

def tent():
    I = Icon()
    I.path([(22, 218), (128, 40), (234, 218), (22, 218)])
    I.path([(92, 218), (128, 108), (164, 218)])
    I.path([(128, 40), (112, 18)], w=SW - 2); I.path([(128, 40), (144, 18)], w=SW - 2)
    return I.save('Tent')

def summit():
    I = Icon()
    I.path([(16, 232), (78, 150), (96, 168), (138, 104), (170, 150), (186, 140), (240, 232)])
    I.path([(138, 104), (138, 22)], w=SW - 1)
    I.poly([(140, 22), (206, 38), (140, 60)])
    return I.save('Summit')

def tree():
    """The World Tree: a round crown of branching lines over a short trunk and spreading roots."""
    I = Icon(); random.seed(7)
    def branch(x, y, ang, L, w, depth):
        if depth == 0 or L < 5: return
        x2, y2 = x + math.cos(ang) * L, y - math.sin(ang) * L
        I.d.line([(x, y), (x2, y2)], fill=(255, 255, 255, 255), width=max(2, int(w)))
        spread = math.radians(33 + random.uniform(-5, 7))
        branch(x2, y2, ang + spread, L * 0.76, w * 0.68, depth - 1)
        branch(x2, y2, ang - spread, L * 0.76, w * 0.68, depth - 1)
    I.d.line([(128, 200), (128, 132)], fill=(255, 255, 255, 255), width=13)
    for a in (90, 62, 118, 38, 142): branch(128, 134, math.radians(a), 30 if a == 90 else 34, 9, 6)
    def root(x, y, ang, L, w, depth):
        if depth == 0: return
        x2, y2 = x + math.cos(ang) * L, y - math.sin(ang) * L
        I.d.line([(x, y), (x2, y2)], fill=(255, 255, 255, 255), width=max(2, int(w)))
        root(x2, y2, ang - 0.32, L * 0.7, w * 0.66, depth - 1); root(x2, y2, ang + 0.32, L * 0.7, w * 0.66, depth - 1)
    for a in (-150, -118, -62, -30): root(128, 194, math.radians(a), 26, 8, 3)
    return I.save('Tree')

def mountain():
    """A small ink-wash peak: grey faces, white snow streaks, mist at the foot (tinted grey in the game, so it keeps
    its own shading in the alpha and a light/dark split in the colour)."""
    W, H = 512, 320; random.seed(3)
    # ridge by midpoint displacement around a main summit
    pts = [(0, 300), (150, 170), (300, 40), (390, 120), (512, 230)]
    for it in range(6):
        nxt = [pts[0]]
        for (x0, y0), (x1, y1) in zip(pts, pts[1:]):
            d = (x1 - x0) * 0.22 * (0.62 ** it) * 2.2
            nxt += [((x0 + x1) / 2, (y0 + y1) / 2 + random.uniform(-d, d) * 0.5), (x1, y1)]
        pts = nxt
    col = Image.new('L', (W, H), 0); alpha = Image.new('L', (W, H), 0)
    da = ImageDraw.Draw(alpha); dc = ImageDraw.Draw(col)
    da.polygon(pts + [(W, H), (0, H)], fill=255)
    # light face left of the summit line, shade right of it; snow streaks running down the faces
    dc.polygon(pts + [(W, H), (0, H)], fill=120)
    sx = 300
    for i in range(70):
        x = random.uniform(40, 470); top = min((p for p in pts if abs(p[0] - x) < 8), key=lambda p: p[1], default=(x, 200))[1]
        L = random.uniform(30, 140); lean = (-0.55 if x < sx else 0.45) * L
        shade = 235 if x < sx else 175
        dc.line([(x, top + 2), (x + lean, top + L)], fill=shade, width=random.choice((2, 3, 4)))
    col = col.filter(ImageFilter.GaussianBlur(1.6))
    # mist: alpha fades out towards the foot and both sides
    fade = Image.new('L', (W, H))
    fp = fade.load()
    for y in range(H):
        for x in range(0, W):
            v = min(1.0, max(0.0, (H - 20 - y) / 150.0)) * min(1.0, x / 120.0, (W - x) / 120.0)
            fp[x, y] = int(255 * v)
    from PIL import ImageChops
    alpha = ImageChops.multiply(alpha.filter(ImageFilter.GaussianBlur(1.2)), fade)
    im = Image.merge('RGBA', (col, col, col, alpha))
    im.resize((256, 160), Image.LANCZOS).save(OUT + 'T_HUD6_Mountain.png')
    return im

ICONS = [lungs, thermometer, heart, food, drop, snowflake, wind, tent, summit, tree]
if __name__ == '__main__':
    sheet = Image.new('RGB', (len(ICONS) * 140 + 280, 160), (24, 26, 30))
    for i, f in enumerate(ICONS):
        im = f().resize((128, 128), Image.LANCZOS); sheet.paste(im, (6 + i * 140, 16), im)
    m = mountain().resize((256, 160), Image.LANCZOS); sheet.paste(m, (len(ICONS) * 140, 0), m)
    sheet.save('F:/UE_project/YggdrasilsPeak/Saved/Review/hud6_icons.png'); print('ok')
