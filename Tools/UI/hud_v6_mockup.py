# HUD v6 mockup (2026-11-13): the user's reference sheet ("Let's try something like this") laid over real game frames.
# Dark ink + one red, a high-contrast serif with italics, line icons (Tools/UI/hud_icons.py), dotted route with camps.
# The reference is a layout sheet with the readouts across the middle; in play the centre of the view stays clear, so
# the readouts, altitude and route stack along the bottom left and the objective / weather sit in the top corners.
# Fonts: PREVIEW uses Bodoni MT from Windows (not shippable) as a stand-in for Bodoni Moda (OFL, free) until downloaded.
# Out: Saved/Review/hud_v6_day.png, hud_v6_storm.png, hud_v6_sheet.png
from PIL import Image, ImageDraw, ImageFont, ImageFilter
import os, sys
P = 'F:/UE_project/YggdrasilsPeak/Saved/'
IC = P + 'Review/hud6_icons/'
FONTS = dict(R='C:/Windows/Fonts/BOD_R.TTF', I='C:/Windows/Fonts/BOD_I.TTF')
def F(style, size): return ImageFont.truetype(FONTS[style], size)

INK = (30, 32, 36); GREY = (78, 82, 90); TRACK = (190, 193, 200); RED = (212, 38, 44)
R = dict(alt=6480, spo2=81, core=35.1, body=74, food=98, water=97, warm=0.32, air=-23, weather='Snow',
         rations=6, waters=4, pack=(18, 26))
CAMPS = [('Dingboche', 4410), ('Gorak Shep', 5164), ('Base', 5364), ('C1', 6065), ('C2', 6400), ('C3', 7160),
         ('South Col', 7906), ('Summit', 8849)]

def icon(name, h, col, a=1.0):
    im = Image.open(IC + f'T_HUD6_{name}.png').convert('RGBA')
    w = int(im.width * h / im.height); im = im.resize((w, h), Image.LANCZOS)
    r, g, b, al = im.split()
    out = Image.new('RGBA', im.size, col + (0,)); out.putalpha(al.point(lambda v: int(v * a)))
    return out

class Layer:
    """Ink on a transparent layer with a soft light halo under it (the reference sits on white; in the game the halo
    keeps dark ink readable when the scene behind it is dark or busy)."""
    def __init__(self, size, halo=0.72):
        self.im = Image.new('RGBA', size, (0, 0, 0, 0)); self.ha = Image.new('RGBA', size, (0, 0, 0, 0))
        self.d = ImageDraw.Draw(self.im); self.dh = ImageDraw.Draw(self.ha); self.halo = halo
    def text(self, xy, s, f, c, a=1.0, anchor='ls', track=0):
        if track:
            x, y = xy
            for ch in s:
                self.text((x, y), ch, f, c, a, anchor); x += self.d.textlength(ch, font=f) + track
            return x
        self.dh.text(xy, s, font=f, fill=(255, 255, 255, int(255 * self.halo * a)), anchor=anchor, stroke_width=4)
        self.d.text(xy, s, font=f, fill=c + (int(255 * a),), anchor=anchor)
        return xy[0] + self.d.textlength(s, font=f)
    def w(self, s, f): return self.d.textlength(s, font=f)
    def line(self, pts, c, w=1, a=1.0):
        self.dh.line(pts, fill=(255, 255, 255, int(255 * self.halo * a)), width=w + 4)
        self.d.line(pts, fill=c + (int(255 * a),), width=w)
    def bar(self, x, y, w, v, c, h=4):
        self.dh.rounded_rectangle((x - 2, y - 2, x + w + 2, y + h + 2), h, fill=(255, 255, 255, int(255 * self.halo)))
        self.d.rounded_rectangle((x, y, x + w, y + h), h / 2, fill=TRACK + (200,))
        self.d.rounded_rectangle((x, y, x + max(h, w * v), y + h), h / 2, fill=c + (255,))
    def dot(self, c, r, col, ring=False, fill_white=False):
        box = (c[0] - r, c[1] - r, c[0] + r, c[1] + r)
        self.dh.ellipse((box[0] - 3, box[1] - 3, box[2] + 3, box[3] + 3), fill=(255, 255, 255, int(255 * self.halo)))
        if ring:
            self.d.ellipse(box, fill=(248, 248, 250, 255) if fill_white else None, outline=col + (255,), width=3 if fill_white else 2)
        else:
            self.d.ellipse(box, fill=col + (255,))
    def paste(self, im, xy):
        a = im.split()[3]
        halo = Image.new('RGBA', im.size, (255, 255, 255, 0)); halo.putalpha(a.point(lambda v: int(v * self.halo)))
        self.ha.alpha_composite(halo, (int(xy[0]), int(xy[1]))); self.im.alpha_composite(im, (int(xy[0]), int(xy[1])))
    def compose(self, base):
        out = base.convert('RGBA')
        out.alpha_composite(self.ha.filter(ImageFilter.GaussianBlur(8)))
        out.alpha_composite(self.im)
        return out.convert('RGB')

def arc_glow(L, cx, cy, r, a0, a1, col):
    """the red arc over an alerting readout: brightest at the top, fading at both ends, with a soft glow"""
    import math
    g = Image.new('RGBA', L.im.size, (0, 0, 0, 0)); dg = ImageDraw.Draw(g)
    n = 60
    for i in range(n):
        t0, t1 = i / n, (i + 1) / n
        fade = math.sin(math.pi * (t0 + t1) / 2) ** 1.5
        b0 = math.radians(a0 + (a1 - a0) * t0); b1 = math.radians(a0 + (a1 - a0) * t1)
        p0 = (cx + r * math.cos(b0), cy + r * math.sin(b0)); p1 = (cx + r * math.cos(b1), cy + r * math.sin(b1))
        dg.line([p0, p1], fill=col + (int(255 * fade * 0.35),), width=12)
        L.d.line([p0, p1], fill=col + (int(255 * fade),), width=3)
    L.im.alpha_composite(g.filter(ImageFilter.GaussianBlur(8)))

def hud(base, quiet=False):
    W, H = base.size; L = Layer(base.size)
    # top left: the name, then the objective
    tr = icon('Tree', 50, INK); L.paste(tr, (50, 22))
    L.text((112, 58), "YGGDRASIL'S PEAK", F('R', 17), INK, 0.95, track=5)
    L.text((82, 112), 'OBJECTIVE \u2014', F('R', 14), GREY, 1.0, track=4)
    L.d.rectangle((56, 124, 60, 154), fill=RED + (255,))
    L.text((80, 151), 'Find shelter or warm up', F('I', 36), INK)
    # top right: the weather now and the air
    x = W - 56
    wi = icon('Wind', 30, GREY); x -= wi.width; L.paste(wi, (x, 40))
    x -= 22; x0 = x - L.w(f'\u2212{abs(R["air"])}\u00b0C', F('I', 36)); L.text((x0, 66), f'\u2212{abs(R["air"])}\u00b0C', F('I', 36), RED); x = x0
    x -= 22; L.line([(x, 36), (x, 72)], GREY, 1, 0.7); x -= 22
    x0 = x - L.w(R['weather'], F('I', 30)); L.text((x0, 65), R['weather'], F('I', 30), INK); x = x0 - 16
    sn = icon('Snow', 32, INK); L.paste(sn, (x - sn.width, 38))
    # readouts: icon, numeral with a small unit, a rounded bar; slashes between
    y = 790; X0 = 64; step = 196
    rows = [('Lungs', f'{R["spo2"]}', '%', R['spo2'] / 100, False), ('Thermo', f'{R["core"]:.1f}', '\u00b0C', R['warm'], True),
            ('Heart', f'{R["body"]}', '', R['body'] / 100, False), ('Food', f'{R["food"]}', '', R['food'] / 100, False),
            ('Drop', f'{R["water"]}', '', R['water'] / 100, False)]
    for i, (ic, val, unit, v, alert) in enumerate(rows):
        x = X0 + i * step; col = RED if alert else INK
        if alert: arc_glow(L, x + 78, y + 46, 96, 200, 340, RED)
        im = icon(ic, 40, col); L.paste(im, (x, y - 38))
        nx = L.text((x + im.width + 12, y), val, F('R', 48), col)
        if unit: L.text((nx + 3, y), unit, F('R', 24), col)
        L.bar(x, y + 22, 150, v, RED if alert else (64, 68, 74))
        if i < len(rows) - 1: L.line([(x + step - 26, y - 2), (x + step - 34, y + 20)], TRACK, 1, 0.9)
    # altitude: the word, the big number, the mountain beside it, the next camp
    mt = Image.open(IC + 'T_HUD6_Mountain.png').convert('RGBA').resize((180, 112), Image.LANCZOS); L.paste(mt, (40, 858))
    L.text((222, 880), 'ALTITUDE', F('R', 14), GREY, track=5)
    ax = L.text((216, 958), f'{R["alt"]:,}', F('R', 96), INK)
    ax = L.text((ax + 8, 958), 'm', F('I', 46), INK)
    L.line([(ax + 34, 904), (ax + 34, 956)], GREY, 1, 0.8)
    nxt = next(c for c in CAMPS if c[1] > R['alt'])
    cx = L.text((ax + 66, 948), nxt[0], F('I', 42), INK)
    tn = icon('Tent', 36, INK); L.paste(tn, (cx + 16, 914))
    L.text((cx + 16 + tn.width + 18, 946), f'+{nxt[1] - R["alt"]:,} m', F('I', 28), GREY)
    # the route: camps as dots at equal spacing, passed dark, you a ring, the summit a flag
    ly = 994; lx0, lx1 = 92, 1010; n = len(CAMPS); sx = lambda i: lx0 + (lx1 - lx0) * i / (n - 1)
    k = max(i for i, c in enumerate(CAMPS) if c[1] <= R['alt'])
    f = (R['alt'] - CAMPS[k][1]) / (CAMPS[k + 1][1] - CAMPS[k][1]); px = sx(k) + (sx(k + 1) - sx(k)) * f
    L.line([(px, ly), (lx1, ly)], TRACK, 2, 0.9)
    L.line([(lx0, ly), (px, ly)], INK, 2, 0.95)
    for i, (name, e) in enumerate(CAMPS):
        x = sx(i)
        if i == n - 1:
            sm = icon('Summit', 34, INK if e <= R['alt'] else GREY); L.paste(sm, (x - sm.width / 2, ly - 30))
        else:
            L.dot((x, ly), 6 if e <= R['alt'] else 5, INK if e <= R['alt'] else GREY, ring=e > R['alt'])
        nc = INK if (e <= R['alt'] or (name, e) == nxt) else GREY
        L.text((x, ly + 30), name, F('I', 20), nc, anchor='ms')
        L.text((x, ly + 52), f'{e:,} m', F('I', 16), GREY, anchor='ms')
    L.dot((px, ly), 10, INK, ring=True, fill_white=True)
    # bottom right: the pack, quietly
    L.text((W - 56, H - 70), f'on the rope  \u00b7  pack {R["pack"][0]} / {R["pack"][1]} kg', F('I', 20), GREY, anchor='rs')
    L.text((W - 56, H - 44), f'food {R["rations"]}  [1]     water {R["waters"]}  [2]', F('I', 20), GREY, anchor='rs')
    return L.compose(base)

if __name__ == '__main__':
    day = hud(Image.open(P + 'Compare/shot_hud_quiet.png').convert('RGB').resize((1920, 1080)))
    storm = hud(Image.open(P + 'Compare/realm_Jotunheim.png').convert('RGB').resize((1920, 1080)))
    day.save(P + 'Review/hud_v6_day.png'); storm.save(P + 'Review/hud_v6_storm.png')
    s = Image.new('RGB', (1920, 1080 + 540), (10, 12, 16))
    s.paste(day, (0, 0)); s.paste(storm.resize((960, 540)), (0, 1080)); s.paste(day.crop((0, 700, 1080, 1080)).resize((960, 338)), (960, 1080 + 100))
    s.save(P + 'Review/hud_v6_sheet.png'); print('ok')
