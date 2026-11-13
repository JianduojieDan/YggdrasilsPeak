# HUD v8 mockup (2026-11-13): back to the Altimeter (v5, "acceptable"), developed - the user after Thread (v7): "very
# visible, very clear, but very ugly... what we had before is acceptable, develop based on that. Think more".
# What carried over and why:
#   Altimeter (liked): one modern sans (Roboto), big light altitude, thin lines, one block bottom left.
#   The reference (feeling): monochrome + one red, line icons, the route as camps on a line, the next camp beside the
#   altitude, a soft red arc over what is in danger.
#   Dropped: per-readout colours, cyan/gold (cheap), serif + caps + italic (dated), heavy letter auras (smudgy).
# Shadow: a tight soft drop (1 px, blur 1.5) and a very faint wide one; nothing that reads as a halo.
# Out: Saved/Review/hud_v8_<frame>.png, hud_v8_sheet.png, hud_v8_detail.png
from PIL import Image, ImageDraw, ImageFont, ImageFilter
import math
P = 'F:/UE_project/YggdrasilsPeak/Saved/'
FD = 'E:/UE5.8/UE_5.8/Engine/Content/Slate/Fonts/'
IC = P + 'Review/hud6_icons/'
def font(w, px): return ImageFont.truetype(FD + f'Roboto-{w}.ttf', px)

WHITE = (244, 246, 248); RED = (238, 70, 58); SHADOW = (0, 6, 16)
R = dict(alt=6480, spo2=81, core=35.1, body=74, food=98, water=97, warm=0.32, air=-23, weather='Snow',
         rations=6, waters=4, pack=(18, 26), stamina=0.62)
CAMPS = [('Dingboche', 4410), ('Gorak Shep', 5164), ('Base Camp', 5364), ('Camp 1', 6065), ('Camp 2', 6400),
         ('Camp 3', 7160), ('South Col', 7906), ('Summit', 8849)]

def icon(name, h, col, a=1.0):
    im = Image.open(IC + f'T_HUD6_{name}.png').convert('RGBA')
    w = max(1, int(im.width * h / im.height)); im = im.resize((w, h), Image.LANCZOS)
    out = Image.new('RGBA', im.size, col + (0,)); out.putalpha(im.split()[3].point(lambda v: int(v * a)))
    return out

class Layer:
    def __init__(self, size):
        self.im = Image.new('RGBA', size, (0, 0, 0, 0)); self.sh = Image.new('RGBA', size, (0, 0, 0, 0))
        self.d = ImageDraw.Draw(self.im); self.ds = ImageDraw.Draw(self.sh)
    def w(self, s, f, track=0): return self.d.textlength(s, font=f) + track * max(0, len(s) - 1)
    def text(self, xy, s, f, c, a=1.0, anchor='ls', track=0):
        if track:
            x, y = xy
            if anchor == 'rs': x -= self.w(s, f, track)
            for i, ch in enumerate(s):
                self.text((x, y), ch, f, c, a, 'ls'); x += self.d.textlength(ch, font=f) + (track if i < len(s) - 1 else 0)
            return x
        self.ds.text((xy[0] + 0.5, xy[1] + 1.2), s, font=f, fill=SHADOW + (int(255 * 0.5 * a),), anchor=anchor)
        self.d.text(xy, s, font=f, fill=c + (int(255 * a),), anchor=anchor)
        return xy[0] + (self.d.textlength(s, font=f) if anchor[0] == 'l' else 0)
    def line(self, pts, c, w=1, a=1.0):
        self.ds.line([(x + 0.5, y + 1) for x, y in pts], fill=SHADOW + (int(255 * 0.4 * a),), width=w + 1)
        self.d.line(pts, fill=c + (int(255 * a),), width=w)
    def rect(self, box, c, a=1.0):
        self.ds.rectangle((box[0], box[1] + 1, box[2], box[3] + 1), fill=SHADOW + (int(255 * 0.35 * a),))
        self.d.rectangle(box, fill=c + (int(255 * a),))
    def dot(self, c, r, col, a=1.0, ring=0):
        box = (c[0] - r, c[1] - r, c[0] + r, c[1] + r)
        self.ds.ellipse((box[0], box[1] + 1, box[2], box[3] + 1), fill=SHADOW + (int(255 * 0.4 * a),))
        if ring: self.d.ellipse(box, outline=col + (int(255 * a),), width=ring)
        else: self.d.ellipse(box, fill=col + (int(255 * a),))
    def paste(self, im, xy):
        sh = Image.new('RGBA', im.size, SHADOW + (0,)); sh.putalpha(im.split()[3].point(lambda v: int(v * 0.45)))
        self.sh.alpha_composite(sh, (int(xy[0]), int(xy[1]) + 1)); self.im.alpha_composite(im, (int(xy[0]), int(xy[1])))
    def glow(self, draw_fn, blur):
        g = Image.new('RGBA', self.im.size, (0, 0, 0, 0)); draw_fn(ImageDraw.Draw(g))
        self.im.alpha_composite(g.filter(ImageFilter.GaussianBlur(blur)))
    def compose(self, base):
        out = base.convert('RGBA')
        wide = self.sh.filter(ImageFilter.GaussianBlur(6)); wide.putalpha(wide.split()[3].point(lambda v: int(v * 0.35)))
        out.alpha_composite(wide)
        out.alpha_composite(self.sh.filter(ImageFilter.GaussianBlur(1.5)))
        out.alpha_composite(self.im)
        return out.convert('RGB')

def arc(L, cx, cy, r, a0, a1, col):
    """a thin red arc over the reading in danger: brightest in the middle, gone at both ends, a soft glow under it"""
    n = 48
    segs = []
    for i in range(n):
        t0, t1 = i / n, (i + 1) / n; f = math.sin(math.pi * (t0 + t1) / 2) ** 1.6
        b0, b1 = math.radians(a0 + (a1 - a0) * t0), math.radians(a0 + (a1 - a0) * t1)
        segs.append(((cx + r * math.cos(b0), cy + r * math.sin(b0)), (cx + r * math.cos(b1), cy + r * math.sin(b1)), f))
    L.glow(lambda d: [d.line([p, q], fill=col + (int(255 * f * 0.55),), width=7) for p, q, f in segs], 5)
    for p, q, f in segs: L.d.line([p, q], fill=col + (int(255 * f),), width=2)

def hud(base):
    W, H = base.size; L = Layer(base.size); M = 56
    soft = 0.72                                             # the second voice: units, hints, the unclimbed route
    # ---------------- the route: Dingboche to the summit, a dot per camp, you a ring, the summit a flag
    ly = H - 58; lw = 404; e0, e1 = CAMPS[0][1], CAMPS[-1][1]
    X = lambda e: M + (e - e0) / (e1 - e0) * lw
    you = X(R['alt'])
    L.line([(you, ly), (M + lw, ly)], WHITE, 1, 0.35)
    L.glow(lambda d: d.line([(M, ly), (you, ly)], fill=WHITE + (110,), width=5), 3)
    L.line([(M, ly), (you, ly)], WHITE, 2, 0.95)
    for name, e in CAMPS[:-1]:
        if e <= R['alt']: L.dot((X(e), ly), 3, WHITE, 0.95)
        else: L.dot((X(e), ly), 3, WHITE, 0.5, ring=1)
    sx = X(e1); L.line([(sx - 5, ly + 3), (sx, ly - 6), (sx + 5, ly + 3), (sx - 5, ly + 3)], WHITE, 1, 0.6)
    L.dot((you, ly), 6.5, WHITE, 1.0, ring=2)
    # ---------------- the altitude, and the next camp beside it
    cap = font('Medium', 11); TR = 2.4
    L.text((M, ly - 104), 'ALTITUDE', cap, WHITE, soft, track=TR)
    fa = font('Light', 80); alt = f'{R["alt"]:,}'
    ax = L.text((M - 4, ly - 22), alt, fa, WHITE, 0.98, track=-2)
    ax = L.text((ax + 7, ly - 22), 'm', font('Light', 26), WHITE, soft)
    nxt = next(c for c in CAMPS if c[1] > R['alt'])
    dx = ax + 30
    L.line([(dx, ly - 110), (dx, ly - 24)], WHITE, 1, 0.3)
    cx = dx + 26
    L.text((cx, ly - 104), 'NEXT CAMP', cap, WHITE, soft, track=TR)
    tn = icon('Tent', 20, WHITE, 0.95); L.paste(tn, (cx, ly - 72))
    L.text((cx + tn.width + 10, ly - 54), nxt[0], font('Regular', 20), WHITE, 0.97)
    L.text((cx, ly - 22), f'+{nxt[1] - R["alt"]:,} m', font('Light', 20), WHITE, soft)
    # ---------------- five readings: an icon and a light numeral on one baseline, a fine line under both
    ty = ly - 158; step = 122; bw = 86
    rows = [('Lungs', f'{R["spo2"]}', '%', R['spo2'] / 100, False), ('Thermo', f'{R["core"]:.1f}\u00b0', '', R['warm'], True),
            ('Heart', f'{R["body"]}', '', R['body'] / 100, False), ('Food', f'{R["food"]}', '', R['food'] / 100, False),
            ('Drop', f'{R["water"]}', '', R['water'] / 100, False)]
    fv = font('Light', 32); fu = font('Regular', 15)
    for i, (ic, val, unit, v, alert) in enumerate(rows):
        x = M + i * step; c = RED if alert else WHITE
        if alert: L.glow(lambda d: d.line([(x, ty + 11), (x + bw * v, ty + 11)], fill=RED + (200,), width=6), 4)
        im = icon(ic, 22, c, 1.0 if alert else 0.92); L.paste(im, (x, ty - 22))
        nx = L.text((x + im.width + 9, ty), val, fv, c, 1.0)
        if unit: L.text((nx + 2, ty), unit, fu, c, 0.8 if not alert else 1.0)
        L.rect((x, ty + 11, x + bw, ty + 12), WHITE, 0.3)
        L.rect((x, ty + 11, x + int(bw * v), ty + 12), c, 0.95)
    # ---------------- what needs you: one line above the readings, the only words on the left
    ay = ty - 52
    L.dot((M + 4, ay - 5), 3.5, RED, 1.0)
    wx = L.text((M + 16, ay), 'Freezing', font('Medium', 16), RED)
    L.text((wx + 10, ay), 'find shelter or warm up', font('Regular', 16), WHITE, 0.88)
    # ---------------- top right: the weather and the air, small
    x = W - M; fy = 60
    t = f'\u2212{abs(R["air"])}\u00b0'; tf = font('Light', 26)
    L.text((x, fy), t, tf, WHITE, 0.98, anchor='rs'); x -= L.w(t, tf) + 16
    L.line([(x, fy - 22), (x, fy + 2)], WHITE, 1, 0.35); x -= 16
    wf = font('Light', 22); L.text((x, fy), R['weather'], wf, WHITE, 0.95, anchor='rs'); x -= L.w(R['weather'], wf) + 10
    sn = icon('Snow', 22, WHITE, 0.9); L.paste(sn, (x - sn.width, fy - 20))
    # ---------------- breath, under the middle of the view, only while it is spent
    hw = 90; v = R['stamina']
    L.line([(W / 2 - hw, H - 34), (W / 2 + hw, H - 34)], WHITE, 1, 0.25)
    L.line([(W / 2 - hw * v, H - 34), (W / 2 + hw * v, H - 34)], WHITE, 2, 0.95)
    # ---------------- bottom right: the pack
    kf = font('Regular', 15)
    L.text((W - M, ly - 26), f'on the rope   \u00b7   pack {R["pack"][0]} / {R["pack"][1]} kg', kf, WHITE, 0.78, anchor='rs')
    L.text((W - M, ly + 5), f'food {R["rations"]}  [1]      water {R["waters"]}  [2]', kf, WHITE, 0.78, anchor='rs')
    return L.compose(base)

FRAMES = [('valley', 'Compare/shot_trees_4800.png'), ('sky', 'Compare/realm_Midgard.png'), ('storm', 'Compare/realm_Jotunheim.png')]
if __name__ == '__main__':
    outs = []
    for tag, f in FRAMES:
        im = hud(Image.open(P + f).convert('RGB').resize((1920, 1080))); im.save(P + f'Review/hud_v8_{tag}.png'); outs.append(im)
    s = Image.new('RGB', (1920, 1080 + 540), (10, 12, 16))
    s.paste(outs[0], (0, 0)); s.paste(outs[1].resize((960, 540)), (0, 1080)); s.paste(outs[2].resize((960, 540)), (960, 1080))
    s.save(P + 'Review/hud_v8_sheet.png')
    d = Image.new('RGB', (1920, 340 + 110), (10, 12, 16))
    d.paste(outs[0].crop((0, 740, 960, 1080)), (0, 0)); d.paste(outs[2].crop((0, 740, 960, 1080)), (960, 0))
    d.paste(outs[0].crop((960, 20, 1920, 130)), (0, 340)); d.paste(outs[0].crop((960, 970, 1920, 1080)), (960, 340))
    d.save(P + 'Review/hud_v8_detail.png'); print('ok')
