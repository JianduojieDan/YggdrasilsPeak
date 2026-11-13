# HUD v5 mockups (2026-11-13, user: "elegant, advanced, minimalist, modern, futuristic - just make it beautiful").
# Three directions drawn over real game frames at 1920x1080 with Roboto (ships with the engine, free to use):
#   A "Altimeter"  - bottom-left data block: big light numerals, a route line with the camps, four precise readouts
#   B "Horizon"    - bottom-centre helmet-display line: altitude above a fading hairline, tick gauges along it
#   C "Column"     - four slim vertical gauges with values, altitude beside them, route line under
# Out: Saved/Review/hud_v5_<A|B|C>.png (+ a storm frame for legibility) and hud_v5_sheet.png
from PIL import Image, ImageDraw, ImageFont, ImageFilter
import math, os
P = 'F:/UE_project/YggdrasilsPeak/Saved/'
FD = 'E:/UE5.8/UE_5.8/Engine/Content/Slate/Fonts/'
os.makedirs(P + 'Review', exist_ok=True)
def font(name, size): return ImageFont.truetype(FD + name, size)
LIGHT, REG, MED, COND = 'Roboto-Light.ttf', 'Roboto-Regular.ttf', 'Roboto-Medium.ttf', 'Roboto-BoldCondensed.ttf'
WHITE = (238, 243, 248); MIST = (176, 188, 202); ICE = (156, 205, 238); EMBER = (228, 98, 74); SUN = (244, 190, 138)

# readings at a real moment: 6,480 m on the Lhotse Face, thin air, getting cold
R = dict(alt=6480, spo2=81, body=74, core=35.1, food=98, water=97, stamina=0.62, warm=0.38, pack=(18, 26), rations=6, waters=4)
CAMPS = [(4410, 'Dingboche'), (5364, 'Base Camp'), (6065, 'C1'), (6400, 'C2'), (7162, 'C3'), (7906, 'South Col'), (8849, 'Summit')]

class Layer:
    """Draws on a transparent layer; text gets a soft 1 px shadow, so it reads on bright snow without any panel."""
    def __init__(self, size):
        self.im = Image.new('RGBA', size, (0, 0, 0, 0)); self.sh = Image.new('RGBA', size, (0, 0, 0, 0))
        self.d = ImageDraw.Draw(self.im); self.ds = ImageDraw.Draw(self.sh)
    def text(self, xy, s, f, c, a=1.0, anchor='la', shadow=0.45):
        self.ds.text((xy[0] + 1, xy[1] + 1.5), s, font=f, fill=(0, 6, 16, int(255 * shadow * a)), anchor=anchor)
        self.d.text(xy, s, font=f, fill=c + (int(255 * a),), anchor=anchor)
    def line(self, pts, c, w=1, a=1.0, shadow=0.35):
        self.ds.line([(x + 0.5, y + 1) for x, y in pts], fill=(0, 6, 16, int(255 * shadow * a)), width=w + 1)
        self.d.line(pts, fill=c + (int(255 * a),), width=w)
    def rect(self, box, c, a=1.0):
        self.d.rectangle(box, fill=c + (int(255 * a),))
    def compose(self, base):
        out = base.convert('RGBA')
        out.alpha_composite(self.sh.filter(ImageFilter.GaussianBlur(1.2)))
        out.alpha_composite(self.im)
        return out.convert('RGB')
    def width(self, s, f): return self.d.textlength(s, font=f)

def kitline(L, W, H, f=None):
    f = f or font(REG, 15)
    L.text((W - 56, H - 74), f'on the rope   ·   pack {R["pack"][0]} / {R["pack"][1]} kg', f, MIST, 0.85, anchor='ra')
    L.text((W - 56, H - 50), f'food {R["rations"]}  [1]      water {R["waters"]}  [2]', f, MIST, 0.85, anchor='ra')

def route_line(L, x0, y, w, show_labels=True, f=None):
    f = f or font(REG, 11)
    a0, a1 = CAMPS[0][0], CAMPS[-1][0]
    X = lambda a: x0 + (a - a0) / (a1 - a0) * w
    L.line([(x0, y), (x0 + w, y)], MIST, 1, 0.45)
    L.line([(x0, y), (X(R['alt']), y)], WHITE, 2, 0.95)
    for a, n in CAMPS:
        L.line([(X(a), y - 3), (X(a), y + 3)], MIST if a > R['alt'] else WHITE, 1, 0.7)
    cx = X(R['alt'])                                              # you: a small diamond
    L.d.polygon([(cx, y - 5), (cx + 4, y), (cx, y + 5), (cx - 4, y)], fill=WHITE + (255,))
    if show_labels:
        L.text((x0, y + 9), 'Dingboche', f, MIST, 0.6)
        L.text((x0 + w, y + 9), '8,849', f, MIST, 0.6, anchor='ra')

# ---------------------------------------------------------------- A: Altimeter
def concept_a(base):
    W, H = base.size; L = Layer(base.size)
    x, by = 56, H - 60
    route_line(L, x, by, 320)
    # altitude, the hero: big light numerals, the unit small on the same baseline
    fa = font(LIGHT, 74); fu = font(REG, 20)
    alt = f'{R["alt"]:,}'
    L.text((x - 3, by - 16), alt, fa, WHITE, 0.97, anchor='ls')
    L.text((x + L.width(alt, fa) + 6, by - 18), 'm', fu, MIST, 0.9, anchor='ls')
    # readouts above: value, a 2 px bar, a small word - four columns on one grid
    fv = font(REG, 22); fl = font(REG, 13)
    cols = [('SpO2', f'{R["spo2"]}%', R['spo2'] / 100, ICE, False),
            ('core', f'{R["core"]:.1f}\u00b0', R['warm'], EMBER, True),
            ('body', f'{R["body"]}', R['body'] / 100, WHITE, False),
            ('food', f'{R["food"]}', R['food'] / 100, SUN, False),
            ('water', f'{R["water"]}', R['water'] / 100, ICE, False)]
    cx, ty = x, by - 128
    for name, val, v, c, alert in cols:
        col = EMBER if alert else WHITE
        L.text((cx, ty), val, fv, col, 0.95, anchor='ls')
        L.rect((cx, ty + 7, cx + 52, ty + 8), MIST, 0.28)
        L.rect((cx, ty + 7, cx + int(52 * v), ty + 8), c, 0.95)
        L.text((cx, ty + 24), name, fl, EMBER if alert else MIST, 0.95, anchor='ls', shadow=0.6)
        cx += 66
    # what needs you: one small word in the accent, with a hairline under it
    fw = font(MED, 14)
    L.d.ellipse((x, ty - 44, x + 6, ty - 38), fill=EMBER + (255,))
    L.text((x + 13, ty - 34), 'freezing', fw, EMBER, 1.0, anchor='ls', shadow=0.6)
    L.text((x + 13 + L.width('freezing', fw) + 10, ty - 34), 'find shelter', font(REG, 14), WHITE, 0.85, anchor='ls', shadow=0.6)
    # breath: a thin line under the view's centre, only while it is spent
    w2 = 160 * R['stamina']
    L.line([(W / 2 - 80, H - 34), (W / 2 + 80, H - 34)], MIST, 1, 0.3)
    L.line([(W / 2 - w2 / 2, H - 34), (W / 2 + w2 / 2, H - 34)], WHITE, 2, 0.9)
    kitline(L, W, H)
    return L.compose(base)

# ---------------------------------------------------------------- A2: Altimeter, refined (the user's pick)
PALE = (204, 214, 226)
# colour sets for A2 (user, after picking A: "change the color, what we have right now is not very good").
# text = numerals and words, soft = labels and tracks, bar = the readout lines and the climbed part of the route,
# alert = what needs you. 'Glacier' is the first A2 (a different colour per readout).
PALETTES = {
    'Glacier':     dict(text=WHITE, soft=PALE, bars=[ICE, (246, 168, 128), WHITE, SUN, ICE], alert=EMBER),
    'Snow':        dict(text=(246, 248, 250), soft=(200, 208, 218), bar=(246, 248, 250), alert=(240, 96, 78)),
    'Ivory Gold':  dict(text=(246, 240, 228), soft=(230, 202, 148), bar=(232, 186, 102), alert=(234, 92, 62)),
    'Arctic':      dict(text=(238, 248, 252), soft=(146, 214, 238), bar=(84, 210, 246), alert=(255, 176, 70)),
    'Aurora':      dict(text=(238, 250, 246), soft=(150, 226, 202), bar=(108, 232, 188), alert=(255, 118, 104)),
    'Frost Lilac': dict(text=(244, 242, 252), soft=(196, 188, 248), bar=(170, 158, 252), alert=(255, 142, 116)),
}
def concept_a2(base, pal='Glacier'):
    K = PALETTES[pal]; TX, SO, AL = K['text'], K['soft'], K['alert']
    BARS = K.get('bars') or [K['bar']] * 5
    ROUTE = K.get('bar', TX)
    W, H = base.size; L = Layer(base.size)
    x, by = 56, H - 62
    # route line: camps as ticks, the next one taller and named at the line's end with what is left to climb
    w = 330
    a0, a1 = CAMPS[0][0], CAMPS[-1][0]
    X = lambda a: x + (a - a0) / (a1 - a0) * w
    nxt = next((c for c in CAMPS if c[0] > R['alt']), CAMPS[-1])
    L.line([(x, by), (x + w, by)], SO, 1, 0.4)
    L.line([(x, by), (X(R['alt']), by)], ROUTE, 2, 0.95)
    for a, n in CAMPS:
        big = (a, n) == nxt
        L.line([(X(a), by - (6 if big else 3)), (X(a), by + (6 if big else 3))], TX if (a <= R['alt'] or big) else SO, 1, 0.95 if big else 0.6)
    cx = X(R['alt'])
    L.d.polygon([(cx, by - 5), (cx + 4, by), (cx, by + 5), (cx - 4, by)], fill=TX + (255,))
    fn = font(REG, 13)
    L.text((x + w + 14, by + 5), nxt[1], fn, TX, 0.9, anchor='ls', shadow=0.6)
    L.text((x + w + 14 + L.width(nxt[1], fn) + 8, by + 5), f'+{nxt[0] - R["alt"]:,} m', fn, SO, 0.85, anchor='ls', shadow=0.6)
    # altitude, the hero
    fa = font(LIGHT, 76); fu = font(REG, 20)
    alt = f'{R["alt"]:,}'
    L.text((x - 3, by - 18), alt, fa, TX, 0.98, anchor='ls', shadow=0.5)
    L.text((x + L.width(alt, fa) + 6, by - 20), 'm', fu, SO, 0.9, anchor='ls', shadow=0.5)
    # readouts: light numerals, a 2 px line under each, the word under it - one grid, 72 px apart
    fv = font(LIGHT, 27); fl = font(REG, 13); fsub = font(REG, 9)
    cols = [('SpO', '2', f'{R["spo2"]}%', R['spo2'] / 100, False),
            ('core', '', f'{R["core"]:.1f}°', R['warm'], True),
            ('body', '', f'{R["body"]}', R['body'] / 100, False),
            ('food', '', f'{R["food"]}', R['food'] / 100, False),
            ('water', '', f'{R["water"]}', R['water'] / 100, False)]
    cx, ty = x, by - 132
    for i, (name, sub, val, v, alert) in enumerate(cols):
        L.text((cx, ty), val, fv, AL if alert else TX, 0.97, anchor='ls', shadow=0.55)
        L.rect((cx, ty + 8, cx + 54, ty + 9), SO, 0.3)
        L.rect((cx, ty + 8, cx + int(54 * v), ty + 9), AL if alert else BARS[i], 0.95)
        L.text((cx, ty + 26), name, fl, AL if alert else SO, 0.95, anchor='ls', shadow=0.6)
        if sub: L.text((cx + L.width(name, fl) + 1, ty + 29), sub, fsub, SO, 0.95, anchor='ls', shadow=0.6)
        cx += 72
    # what needs you
    fw = font(MED, 14)
    L.d.polygon([(x + 3.5, ty - 45), (x + 7, ty - 41.5), (x + 3.5, ty - 38), (x, ty - 41.5)], fill=AL + (255,))
    L.text((x + 14, ty - 35), 'freezing', fw, AL, 1.0, anchor='ls', shadow=0.65)
    L.text((x + 14 + L.width('freezing', fw) + 10, ty - 35), 'find shelter or warm up', font(REG, 14), TX, 0.9, anchor='ls', shadow=0.65)
    # breath
    w2 = 170 * R['stamina']
    L.line([(W / 2 - 85, H - 34), (W / 2 + 85, H - 34)], SO, 1, 0.3)
    L.line([(W / 2 - w2 / 2, H - 34), (W / 2 + w2 / 2, H - 34)], ROUTE, 2, 0.95)
    f = font(REG, 15)
    L.text((W - 56, H - 74), f'on the rope   ·   pack {R["pack"][0]} / {R["pack"][1]} kg', f, SO, 0.9, anchor='ra', shadow=0.6)
    L.text((W - 56, H - 50), f'food {R["rations"]}  [1]      water {R["waters"]}  [2]', f, SO, 0.9, anchor='ra', shadow=0.6)
    return L.compose(base)

def a2_sheet():
    day = concept_a2(Image.open(P + 'Compare/shot_hud_quiet.png').convert('RGB').resize((1920, 1080)))
    storm = concept_a2(Image.open(P + 'Compare/realm_Jotunheim.png').convert('RGB').resize((1920, 1080)))
    day.save(P + 'Review/hud_v5_A2_day.png'); storm.save(P + 'Review/hud_v5_A2_storm.png')
    s = Image.new('RGB', (1920, 390), (10, 14, 22))
    s.paste(day.crop((0, 690, 960, 1080)), (0, 0)); s.paste(storm.crop((0, 690, 960, 1080)), (960, 0))
    s.save(P + 'Review/hud_v5_A2_crops.png')

def palette_sheet(names=None):
    """Every colour set on a bright day and in a storm: one row each, numbered for the user to pick."""
    names = names or [n for n in PALETTES if n != 'Glacier']
    day0 = Image.open(P + 'Compare/shot_hud_quiet.png').convert('RGB').resize((1920, 1080))
    storm0 = Image.open(P + 'Compare/realm_Jotunheim.png').convert('RGB').resize((1920, 1080))
    rowh = 330
    s = Image.new('RGB', (1920, rowh * len(names)), (10, 14, 22)); d = ImageDraw.Draw(s)
    for i, n in enumerate(names):
        s.paste(concept_a2(day0, n).crop((0, 750, 960, 1080)), (0, i * rowh))
        s.paste(concept_a2(storm0, n).crop((0, 750, 960, 1080)), (960, i * rowh))
        d.rectangle((0, i * rowh, 1920, i * rowh + 1), fill=(10, 14, 22))
        tag = f'{i + 1}  {n}'
        d.rectangle((1580, i * rowh + 14, 1906, i * rowh + 52), fill=(10, 14, 22))
        d.text((1596, i * rowh + 20), tag, font=font(MED, 22), fill=(240, 240, 240))
    s.save(P + 'Review/hud_v5_palettes.png')

if __name__ == '__main__':
    import sys
    if 'palettes' in sys.argv: palette_sheet()

# ---------------------------------------------------------------- B: Horizon
def concept_b(base):
    W, H = base.size; L = Layer(base.size)
    cy = H - 70; half = 330
    # the horizon: a hairline that fades out at both ends
    for i in range(-half, half):
        t = abs(i) / half
        a = 0.55 * (1 - t ** 2.2)
        L.d.point((W / 2 + i, cy), fill=WHITE + (int(255 * a),))
        L.ds.point((W / 2 + i + 0.5, cy + 1), fill=(0, 6, 16, int(255 * a * 0.4)))
    fa = font(LIGHT, 52); fu = font(REG, 16)
    alt = f'{R["alt"]:,}'
    aw = L.width(alt, fa)
    L.text((W / 2 - 8, cy - 14), alt, fa, WHITE, 0.97, anchor='rs')
    L.text((W / 2 - 2, cy - 16), 'm', fu, MIST, 0.85, anchor='ls')
    L.text((W / 2 + 26, cy - 16), f'SpO2 {R["spo2"]}%', font(REG, 16), ICE, 0.9, anchor='ls')
    # four tick gauges on the line: 10 ticks each, lit ticks = the value; two left of centre, two right
    gauges = [('core', R['warm'], EMBER, True, -1), ('body', R['body'] / 100, WHITE, False, -1),
              ('food', R['food'] / 100, SUN, False, 1), ('water', R['water'] / 100, ICE, False, 1)]
    fl = font(REG, 12)
    for k, (name, v, c, alert, side) in enumerate(gauges):
        slot = (k % 2)
        gx = W / 2 + side * (150 + slot * 110) - (90 if side < 0 else 0)
        lit = round(v * 10)
        for t in range(10):
            tx = gx + t * 9
            on = t < lit
            L.line([(tx, cy + 6), (tx, cy + 15)], c if on else MIST, 2, 0.95 if on else 0.25)
        L.text((gx, cy + 30), name, fl, EMBER if alert else MIST, 0.8, anchor='ls')
    # breath: the centre of the line brightens and shrinks as it is spent
    w2 = 120 * R['stamina']
    L.line([(W / 2 - w2 / 2, cy), (W / 2 + w2 / 2, cy)], WHITE, 2, 0.95)
    L.text((W / 2, cy - 84), 'freezing', font(MED, 14), EMBER, 0.95, anchor='ms')
    kitline(L, W, H)
    return L.compose(base)

# ---------------------------------------------------------------- C: Column
def concept_c(base):
    W, H = base.size; L = Layer(base.size)
    x0, bottom, hgt = 60, H - 92, 118
    bars = [('O2', R['spo2'] / 100, f'{R["spo2"]}', ICE, False), ('core', R['warm'], f'{R["core"]:.1f}\u00b0', EMBER, True),
            ('body', R['body'] / 100, f'{R["body"]}', WHITE, False), ('food', R['food'] / 100, f'{R["food"]}', SUN, False),
            ('water', R['water'] / 100, f'{R["water"]}', ICE, False)]
    fv = font(REG, 14); fl = font(REG, 11)
    for k, (name, v, val, c, alert) in enumerate(bars):
        x = x0 + k * 38
        L.rect((x, bottom - hgt, x + 2, bottom), MIST, 0.25)
        L.rect((x, bottom - int(hgt * v), x + 2, bottom), c, 0.95)
        L.text((x + 1, bottom - hgt - 10), val, fv, EMBER if alert else WHITE, 0.95, anchor='ms')
        L.text((x + 1, bottom + 18), name, fl, EMBER if alert else MIST, 0.8, anchor='ms')
    ax = x0 + 5 * 38 + 18
    fa = font(LIGHT, 64)
    alt = f'{R["alt"]:,}'
    L.text((ax, bottom), alt, fa, WHITE, 0.97, anchor='ls')
    L.text((ax + L.width(alt, fa) + 6, bottom - 2), 'm', font(REG, 18), MIST, 0.85, anchor='ls')
    L.text((ax, bottom - 70), 'freezing', font(MED, 13), EMBER, 0.95, anchor='ls')
    route_line(L, ax, bottom + 18, 260, show_labels=False)
    kitline(L, W, H)
    return L.compose(base)

frames = {'day': P + 'Compare/shot_hud_quiet.png', 'storm': P + 'Compare/realm_Jotunheim.png'}
out = {}
for key, fn in (('A', concept_a), ('B', concept_b), ('C', concept_c)):
    for fk, fp in frames.items():
        base = Image.open(fp).convert('RGB').resize((1920, 1080))
        img = fn(base)
        img.save(P + f'Review/hud_v5_{key}_{fk}.png')
        out[(key, fk)] = img
sheet = Image.new('RGB', (1920, 1080 * 3 // 2 * 2), (10, 14, 22))
# sheet: each concept as a bottom-left/bottom crop at full size (the part that matters), day and storm side by side
crop = (0, 1080 - 330, 960, 1080)
rows = []
for key in 'ABC':
    for k, fk in enumerate(('day', 'storm')):
        im = out[(key, fk)]
        c = im.crop((0, 750, 960, 1080)) if key != 'B' else im.crop((480, 750, 1440, 1080))
        rows.append(c)
s = Image.new('RGB', (1920, 330 * 3), (10, 14, 22))
for i, c in enumerate(rows):
    s.paste(c, ((i % 2) * 960, (i // 2) * 330))
s.save(P + 'Review/hud_v5_sheet.png')
for key in 'ABC':
    out[(key, 'day')].resize((1280, 720)).save(P + f'Review/hud_v5_{key}_day_small.png')
print('ok')
