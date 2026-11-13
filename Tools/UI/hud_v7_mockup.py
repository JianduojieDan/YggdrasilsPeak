# HUD v7 "Thread" mockup (2026-11-13). The user, after the reference copy (v6) was "very bad": "make one with your vision";
# the reference was only for the feeling (elegance, serif values, quiet labels, one red).
# The system: every value has a small tracked-capitals label above it (Roboto Medium, ships with the engine) and the value
# itself in a book serif; warm ivory light with a soft dark aura (no panels, no outlines); vermilion only for danger.
# The route is a thread of beads (camps) with the climbed part lit; the objective top left becomes the need when there
# is one (so there is no separate alert line); the weather top right. Bottom right: the pack, quietly.
# PREVIEW serif = Monotype Garamond (Windows, not shippable) standing in for EB Garamond / Cormorant Garamond (OFL).
# Out: Saved/Review/hud_v7_<frame>.png and hud_v7_sheet.png
from PIL import Image, ImageDraw, ImageFont, ImageFilter
import math, sys
P = 'F:/UE_project/YggdrasilsPeak/Saved/'
FD = 'E:/UE5.8/UE_5.8/Engine/Content/Slate/Fonts/'
SERIF = dict(R='C:/Windows/Fonts/GARA.TTF', I='C:/Windows/Fonts/GARAIT.TTF', B='C:/Windows/Fonts/GARABD.TTF')
def serif(st, px): return ImageFont.truetype(SERIF[st], px)
def sans(px): return ImageFont.truetype(FD + 'Roboto-Medium.ttf', px)

IVORY = (246, 240, 228); RED = (236, 92, 66); AURA = (6, 12, 24)
R = dict(alt=6480, spo2=81, core=35.1, body=74, food=98, water=97, warm=0.32, air=-23, weather='Snow',
         rations=6, waters=4, pack=(18, 26), stamina=0.62)
CAMPS = [('Dingboche', 4410), ('Gorak Shep', 5164), ('Base Camp', 5364), ('Camp 1', 6065), ('Camp 2', 6400),
         ('Camp 3', 7160), ('South Col', 7906), ('Summit', 8849)]

class Layer:
    def __init__(self, size):
        self.im = Image.new('RGBA', size, (0, 0, 0, 0)); self.sh = Image.new('RGBA', size, (0, 0, 0, 0))
        self.d = ImageDraw.Draw(self.im); self.ds = ImageDraw.Draw(self.sh)
    def w(self, s, f, track=0): return self.d.textlength(s, font=f) + track * max(0, len(s) - 1)
    def text(self, xy, s, f, c, a=1.0, anchor='ls', track=0, aura=1.0):
        if track:
            x, y = xy
            if anchor == 'rs': x -= self.w(s, f, track)
            for ch in s:
                self.text((x, y), ch, f, c, a, 'ls', 0, aura); x += self.d.textlength(ch, font=f) + track
            return x
        small = f.size < 26                                      # thin small letters need a firmer aura on bright snow
        self.ds.text((xy[0], xy[1] + 1), s, font=f, fill=AURA + (int(255 * (0.8 if small else 0.55) * a * aura),), anchor=anchor, stroke_width=3 if small else 2)
        self.d.text(xy, s, font=f, fill=c + (int(255 * a),), anchor=anchor)
        return xy[0] + (self.d.textlength(s, font=f) if anchor[0] == 'l' else 0)
    def line(self, pts, c, w=1, a=1.0):
        self.ds.line([(x, y + 1) for x, y in pts], fill=AURA + (int(255 * 0.5 * a),), width=w + 2)
        self.d.line(pts, fill=c + (int(255 * a),), width=w)
    def bead(self, c, r, col, a=1.0, ring=False):
        box = (c[0] - r, c[1] - r, c[0] + r, c[1] + r)
        self.ds.ellipse((box[0] - 1, box[1], box[2] + 1, box[3] + 2), fill=AURA + (int(255 * 0.5 * a),))
        if ring: self.d.ellipse(box, outline=col + (int(255 * a),), width=1)
        else: self.d.ellipse(box, fill=col + (int(255 * a),))
    def glow(self, c, r, col, a):
        g = Image.new('RGBA', self.im.size, (0, 0, 0, 0)); ImageDraw.Draw(g).ellipse((c[0] - r, c[1] - r, c[0] + r, c[1] + r), fill=col + (int(255 * a),))
        self.im.alpha_composite(g.filter(ImageFilter.GaussianBlur(r * 0.6)))
    def compose(self, base):
        out = base.convert('RGBA')
        # the aura: a tight soft shadow and a wide faint one (reads on bright snow, invisible on dark sky)
        out.alpha_composite(self.sh.filter(ImageFilter.GaussianBlur(9)).point(lambda v: v) if False else self.sh.filter(ImageFilter.GaussianBlur(9)))
        out.alpha_composite(self.sh.filter(ImageFilter.GaussianBlur(2.2)))
        out.alpha_composite(self.im)
        return out.convert('RGB')

def hud(base):
    W, H = base.size; L = Layer(base.size); M = 64
    cap = sans(12); TR = 2.8; LA = 0.9
    urgent = True
    # ---- top left: the objective (the need takes it over, in vermilion)
    L.text((M, 70), 'OBJECTIVE', cap, RED if urgent else IVORY, 0.95 if urgent else LA, track=TR)
    if urgent: L.line([(M - 16, 60), (M - 16, 104)], RED, 2, 0.95)
    L.text((M, 104), 'Find shelter, or warm up', serif('I', 30), IVORY)
    # ---- top right: the weather and the air
    tf = serif('R', 32); t = f'\u2212{abs(R["air"])}\u00b0'
    L.text((W - M, 104), t, tf, RED, anchor='rs')
    L.text((W - M, 70), 'AIR', cap, IVORY, LA, track=TR, anchor='rs')
    xr = W - M - L.w(t, tf) - 30
    L.line([(xr + 15, 58), (xr + 15, 104)], IVORY, 1, 0.35)
    wf = serif('I', 30)
    L.text((xr, 104), R['weather'], wf, IVORY, anchor='rs')
    L.text((xr, 70), 'WEATHER', cap, IVORY, LA, track=TR, anchor='rs')
    # ---- bottom left: five readings on one grid, label over value over a fine line
    vy = H - 220; step = 92
    cols = [('OXYGEN', f'{R["spo2"]}', '%', R['spo2'] / 100, False), ('CORE', f'{R["core"]:.1f}', '\u00b0', R['warm'], True),
            ('BODY', f'{R["body"]}', '', R['body'] / 100, False), ('FOOD', f'{R["food"]}', '', R['food'] / 100, False),
            ('WATER', f'{R["water"]}', '', R['water'] / 100, False)]
    vf = serif('R', 36); uf = serif('R', 20)
    for i, (lab, val, unit, v, alert) in enumerate(cols):
        x = M + i * step; c = RED if alert else IVORY
        L.text((x, vy - 36), lab, cap, c, 0.95 if alert else LA, track=TR)
        nx = L.text((x, vy), val, vf, c)
        if unit: L.text((nx + 2, vy), unit, uf, c, 0.9)
        L.line([(x, vy + 13), (x + 62, vy + 13)], IVORY, 1, 0.4)
        if alert: L.glow((x + 62 * v / 2, vy + 13), 12, RED, 0.35)
        L.line([(x, vy + 13), (x + 62 * v, vy + 13)], c, 2, 1.0)
    # ---- the altitude
    L.text((M, H - 168), 'ALTITUDE', cap, IVORY, LA, track=TR)
    af = serif('R', 92)
    ax = L.text((M - 4, H - 82), f'{R["alt"]:,}', af, IVORY)
    L.text((ax + 8, H - 82), 'm', serif('I', 40), IVORY, 0.9)
    # ---- the thread: the route from Dingboche to the summit, a bead for every camp, the climbed part lit
    ty = H - 50; tw = 440; e0, e1 = CAMPS[0][1], CAMPS[-1][1]
    X = lambda e: M + (e - e0) / (e1 - e0) * tw
    nxt = next(c for c in CAMPS if c[1] > R['alt'])
    L.line([(M, ty), (M + tw, ty)], IVORY, 1, 0.32)
    L.line([(M, ty), (X(R['alt']), ty)], IVORY, 2, 0.95)
    for name, e in CAMPS[:-1]:
        if e <= R['alt']: L.bead((X(e), ty), 3, IVORY, 0.95)
        else: L.bead((X(e), ty), 3.5, IVORY, 0.8 if (name, e) == nxt else 0.5, ring=True)
    sx = X(CAMPS[-1][1])                                         # the summit: a small open triangle
    L.line([(sx - 5, ty + 4), (sx, ty - 5), (sx + 5, ty + 4), (sx - 5, ty + 4)], IVORY, 1, 0.6)
    L.glow((X(R['alt']), ty), 10, IVORY, 0.45); L.bead((X(R['alt']), ty), 4.5, IVORY, 1.0)
    nf = serif('I', 22)
    nx = L.text((M + tw + 22, ty + 6), nxt[0], nf, IVORY)
    L.text((nx + 10, ty + 6), f'+{nxt[1] - R["alt"]:,} m', serif('I', 18), IVORY, 0.7)
    # ---- breath, under the middle of the view, only while it is spent
    hw = 90; v = R['stamina']
    L.line([(W / 2 - hw, H - 34), (W / 2 + hw, H - 34)], IVORY, 1, 0.28)
    L.line([(W / 2 - hw * v, H - 34), (W / 2 + hw * v, H - 34)], IVORY, 2, 0.95)
    # ---- bottom right: the pack
    kf = serif('I', 22)
    L.text((W - M, H - 82), f'on the rope  \u00b7  pack {R["pack"][0]} of {R["pack"][1]} kg', kf, IVORY, 0.95, anchor='rs')
    L.text((W - M, ty + 6), f'{R["rations"]} rations  1     {R["waters"]} water  2', kf, IVORY, 0.95, anchor='rs')
    return L.compose(base)

FRAMES = [('valley', 'Compare/shot_trees_4800.png'), ('sky', 'Compare/realm_Midgard.png'), ('storm', 'Compare/realm_Jotunheim.png')]
if __name__ == '__main__':
    outs = []
    for tag, f in FRAMES:
        im = hud(Image.open(P + f).convert('RGB').resize((1920, 1080))); im.save(P + f'Review/hud_v7_{tag}.png'); outs.append(im)
    s = Image.new('RGB', (1920, 1080 + 540), (10, 12, 16))
    s.paste(outs[0], (0, 0)); s.paste(outs[1].resize((960, 540)), (0, 1080)); s.paste(outs[2].resize((960, 540)), (960, 1080))
    s.save(P + 'Review/hud_v7_sheet.png'); print('ok')
