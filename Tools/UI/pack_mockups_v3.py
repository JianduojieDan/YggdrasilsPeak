# Pack screen, round 3 (2026-11-14). The user liked none of round 2 and asked for five different backpack SYSTEMS to
# choose from (how the pack works, not only how it looks). Not industrial, not Cairn, not PUBG, with a playful touch.
#   A "Fate wheel"     hold Tab: a ring of items while the world keeps moving; flick and release to use; used-up things
#                      stay in the ring as ghosts
#   B "Prayer flags"   the items hang as prayer flags on a line across the sky; they flap in the wind, the line sags
#                      with the weight, a used-up flag fades and frays
#   C "Foam case"      an expedition case with a cut-out for every item: an empty shape shows what is missing at once;
#                      Mother's tape labels on the lid
#   D "On the body"    no bag screen: gear sits where a climber keeps it; inner pockets are warm (water does not freeze),
#                      the harness is quick, the pack is slow
#   E "Camp blanket"   at a camp the pack is emptied on a woven blanket: choose what to carry and what to leave; the camp
#                      dog sleeps on what stays behind; on the trail only four quick pockets
# Out: Saved/Review/pack_v3_A..E.png and pack_v3_sheet.png
import math, random, sys
sys.path.insert(0, 'F:/UE_project/YggdrasilsPeak/Tools/UI')
from PIL import Image, ImageDraw, ImageFont, ImageFilter
from pack_mockups import icon, tint, Layer, ITEMS, F, WHITE, RED, P

ROOT = 'F:/UE_project/YggdrasilsPeak/'
OUT = ROOT + 'Saved/Review/'
def caveat(px): return ImageFont.truetype(ROOT + 'Imports/Fonts/Caveat.ttf', px)
def alegreya(px): return ImageFont.truetype(ROOT + 'Imports/Fonts/Alegreya.ttf', px)
W, H = 1920, 1080
ITEM = {it[0]: it for it in ITEMS}


def scene(blur=10, dark=0.55):
    b = Image.open(P + 'Compare/shot_trees_4800.png').convert('RGB').resize((W, H))
    if blur:
        b = b.filter(ImageFilter.GaussianBlur(blur))
    ov = Image.new('RGBA', (W, H), (8, 12, 20, int(255 * dark)))
    out = b.convert('RGBA'); out.alpha_composite(ov)
    return out


def render(rid, size):
    r = Image.open(P + f'ItemIcons/render/{rid}.png').convert('RGBA')
    s = size / max(r.size)
    return r.resize((max(1, int(r.width * s)), max(1, int(r.height * s))), Image.LANCZOS)


def ghost(im, a=0.28):
    g = im.convert('LA').convert('RGBA'); g.putalpha(im.split()[3].point(lambda v: int(v * a))); return g


def shadow_paste(out, im, xy, blur=10, off=(0, 10), a=0.55):
    sh = Image.new('RGBA', out.size, (0, 0, 0, 0))
    m = Image.new('RGBA', im.size, (0, 0, 0, 0)); m.putalpha(im.split()[3].point(lambda v: int(v * a)))
    sh.alpha_composite(m, (int(xy[0] + off[0]), int(xy[1] + off[1])))
    out.alpha_composite(sh.filter(ImageFilter.GaussianBlur(blur)))
    out.alpha_composite(im, (int(xy[0]), int(xy[1])))


def caption(out, letter, title, lines, col=WHITE):
    L = Layer(out.size)
    L.text((64, 92), letter, F('Light', 64), col, 0.95)
    L.text((120, 76), title.upper(), F('Medium', 15), col, 0.9, track=3)
    y = 104
    for ln in lines:
        L.text((120, y), ln, F('Regular', 17), col, 0.78); y += 26
    return L.compose(out).convert('RGBA')


def info(out, x, y, rid, width=420, col=WHITE, extra=None):
    it = ITEM[rid]; L = Layer(out.size)
    L.text((x, y), it[1], F('Light', 40), col)
    sub = '  \u00b7  '.join(t for t in ((f'{it[2]} left' if it[4] == 'food' and it[2] else it[2]), it[6], it[3] if it[4] == 'food' else '') if t)
    yy = y + 34
    if sub:
        L.text((x, yy), sub, F('Regular', 16), col, 0.72); yy += 32
    yy = L.wrap((x, yy + 4), it[5], F('Regular', 18), width, col, 0.9)
    if extra:
        yy = L.wrap((x, yy + 10), extra, F('Regular', 16), width, (255, 196, 140), 0.95)
    if it[7]:
        L.text((x, yy + 18), it[7], F('Medium', 16), RED)
    return L.compose(out).convert('RGBA')


# ---------------------------------------------------------------- A: fate wheel
def design_a():
    out = scene(blur=4, dark=0.35)
    # vignette that darkens toward the ring, the world still visible behind
    v = Image.new('L', (W, H), 0); dv = ImageDraw.Draw(v)
    for r in range(520, 0, -8):
        dv.ellipse((960 - r, 540 - r, 960 + r, 540 + r), fill=int(150 * (1 - r / 520) ** 0.6))
    out = Image.composite(Image.new('RGBA', (W, H), (6, 10, 18, 255)), out, v.filter(ImageFilter.GaussianBlur(30)))
    cx, cy, R = 960, 540, 300
    L = Layer((W, H))
    # outer rings with fine tick marks, like the realm wheel
    for rr, a in ((R + 92, 0.18), (R + 86, 0.45), (R - 86, 0.2)):
        L.d.ellipse((cx - rr, cy - rr, cx + rr, cy + rr), outline=WHITE + (int(255 * a),), width=1)
    for k in range(108):
        ang = k / 108 * math.tau; r0 = R + 86; r1 = r0 + (10 if k % 12 == 0 else 4)
        L.d.line([(cx + r0 * math.cos(ang), cy + r0 * math.sin(ang)), (cx + r1 * math.cos(ang), cy + r1 * math.sin(ang))], fill=WHITE + (110,), width=1)
    # the pack's weight as an arc on the outer ring (17.6 of 26 kg)
    a0 = -90; a1 = a0 + 360 * 17.6 / 26
    L.d.arc((cx - R - 92, cy - R - 92, cx + R + 92, cy + R + 92), a0, a1, fill=(255, 220, 170, 230), width=3)
    L.text((cx + (R + 120) * math.cos(math.radians(a1)), cy + (R + 120) * math.sin(math.radians(a1))), '17.6 kg', F('Regular', 16), WHITE, 0.8, anchor='mm')
    out = L.compose(out).convert('RGBA')
    order = ['Water', 'Ration', 'Oxygen', 'Compass', 'Journal', 'IceAxe', 'Rope', 'Stove', 'Gas']
    sel = 'Water'
    for i, rid in enumerate(order):
        ang = -math.pi / 2 + i / len(order) * math.tau
        x, y = cx + R * math.cos(ang), cy + R * math.sin(ang)
        big = rid == sel
        s = 128 if big else 92
        Lc = Layer((W, H))
        rr = 70 if big else 54
        Lc.d.ellipse((x - rr, y - rr, x + rr, y + rr), fill=(255, 255, 255, 34 if big else 14), outline=WHITE + (230 if big else 90,), width=2 if big else 1)
        out = Lc.compose(out).convert('RGBA')
        im = render(rid, s)
        if rid == 'Oxygen':          # used up: stays as a ghost in its place
            im = ghost(im, 0.35)
        out.alpha_composite(im, (int(x - im.width / 2), int(y - im.height / 2)))
        Lt = Layer((W, H))
        it = ITEM[rid]
        cnt = it[2]
        if cnt:
            Lt.text((x + rr * 0.7, y + rr * 0.85), cnt, F('Medium', 15), WHITE, 0.95, anchor='mm')
        out = Lt.compose(out).convert('RGBA')
    # selection pointer from the centre toward the chosen slot (mouse direction)
    Lp = Layer((W, H))
    Lp.d.line([(cx, cy - 40), (cx, cy - R + 78)], fill=(255, 225, 180, 200), width=2)
    Lp.d.ellipse((cx - 5, cy - 5, cx + 5, cy + 5), fill=(255, 225, 180, 230))
    Lp.text((cx, cy + 10), 'Water', F('Light', 44), WHITE, anchor='mm')
    Lp.text((cx, cy + 52), '4 left  \u00b7  +40 water', F('Regular', 17), WHITE, 0.75, anchor='mm')
    Lp.text((cx, cy + 86), 'let go of Tab to drink', F('Medium', 16), RED, 1, anchor='mm')
    Lp.text((cx + R * math.cos(-math.pi / 2 + 2 / 9 * math.tau), cy + R * math.sin(-math.pi / 2 + 2 / 9 * math.tau) + 84), 'used up', caveat(26), (220, 220, 220), 0.7, anchor='mm')
    out = Lp.compose(out).convert('RGBA')
    return caption(out, 'A', 'Fate wheel', ['Hold Tab: your gear opens as a ring and the world does NOT stop (thin air, falling light).',
                                           'Flick the mouse to a thing, let go to use it. The outer arc is the weight you carry.',
                                           'Playful part: what you used up stays in the ring as a ghost, so you remember what you had.'])


# ---------------------------------------------------------------- B: prayer flags
def design_b():
    out = scene(blur=3, dark=0.2)
    random.seed(3)
    cols = [(38, 92, 178), (236, 234, 226), (190, 48, 42), (40, 132, 78), (230, 186, 44)]
    order = ['Ration', 'Water', 'Oxygen', 'IceAxe', 'Rope', 'Stove', 'Gas', 'Journal', 'Compass']
    x0, x1, ytop, sag = 120, 1800, 215, 120 + 40       # the line sags more when the pack is heavier
    def line_y(x):
        t = (x - x0) / (x1 - x0); return ytop + sag * 4 * t * (1 - t)
    L = Layer((W, H))
    pts = [(x, line_y(x)) for x in range(x0, x1 + 1, 8)]
    L.d.line(pts, fill=(230, 220, 200, 230), width=2)
    out = L.compose(out).convert('RGBA')
    n = len(order); fw, fh = 150, 190
    sel = 'Journal'
    for i, rid in enumerate(order):
        x = x0 + (i + 0.5) / n * (x1 - x0)
        y = line_y(x)
        c = cols[i % 5]
        big = rid == sel
        used = rid == 'Oxygen'
        flag = Image.new('RGBA', (fw, fh + (40 if big else 0)), (0, 0, 0, 0)); d = ImageDraw.Draw(flag)
        fhh = flag.height
        # cloth with a little wave at the bottom edge
        poly = [(0, 0), (fw, 0)] + [(fw - k, fhh - 12 + 8 * math.sin(k / 18 + i)) for k in range(0, fw + 1, 6)] + [(0, fhh - 10)]
        d.polygon(poly, fill=c + (235 if not used else 110,))
        if used:   # frayed: notches along the bottom
            for k in range(6, fw, 14):
                d.polygon([(k - 6, fhh), (k, fhh - 26 - random.randint(0, 14)), (k + 6, fhh)], fill=(0, 0, 0, 0))
        # woodblock print of the item, dark on light flags, light on dark flags
        inkc = (30, 30, 34) if sum(c) > 500 or c[0] > 200 and c[1] > 150 else (250, 248, 240)
        ic = tint(icon(rid, 96 if big else 84), inkc, 0.9 if not used else 0.4)
        flag.alpha_composite(ic, ((fw - ic.width) // 2, 34))
        it = ITEM[rid]
        fd = ImageDraw.Draw(flag)
        if it[2]:
            fd.text((fw // 2, fhh - 36), it[2], font=F('Medium', 20), fill=inkc + (230,), anchor='mm')
        # cloth shading
        sh = Image.new('L', flag.size, 0); ImageDraw.Draw(sh).rectangle((0, 0, fw, 14), fill=60)
        flag = Image.composite(Image.new('RGBA', flag.size, (0, 0, 0, 255)), flag, Image.composite(sh, Image.new('L', flag.size, 0), flag.split()[3]))
        rot = random.uniform(-7, 7) if not big else -2
        flag = flag.rotate(rot, resample=Image.BICUBIC, expand=True)
        shadow_paste(out, flag, (x - flag.width / 2, y - 6), blur=8, off=(6, 12), a=0.35)
    # the chosen flag's text under the line
    out = info(out, 1210, 620, 'Journal', width=560, extra='Two pages so far. A third flag-page waits somewhere above Camp 2.')
    L = Layer((W, H))
    L.text((1210, 560), 'the line sags with the weight you carry  \u2014  17.6 kg', caveat(30), (240, 230, 210), 0.85)
    out = L.compose(out).convert('RGBA')
    return caption(out, 'B', 'Prayer flags', ['Tab: your things hang as prayer flags on a line across the sky; the game keeps running behind.',
                                              'They flap with the real wind (storms make them snap), the line sags with the weight you carry.',
                                              'Playful part: a used-up thing leaves a faded, frayed flag; new finds get a fresh, bright one.'])


# ---------------------------------------------------------------- C: foam case
def design_c():
    out = scene(blur=16, dark=0.62)
    bx0, by0, bx1, by1 = 300, 370, 1260, 1010
    # lid strip above the foam, with Mother's tape labels
    lid = Image.new('RGBA', (bx1 - bx0, 150), (52, 56, 50, 255)); dl = ImageDraw.Draw(lid)
    dl.rounded_rectangle((0, 0, lid.width - 1, lid.height - 1), 18, outline=(90, 96, 88, 255), width=3)
    def tape(img, xy, text, rot):
        t = Image.new('RGBA', (300, 56), (226, 214, 170, 235)); dt = ImageDraw.Draw(t)
        dt.text((150, 30), text, font=caveat(34), fill=(40, 36, 60, 255), anchor='mm')
        t = t.rotate(rot, resample=Image.BICUBIC, expand=True); img.alpha_composite(t, xy)
    tape(lid, (40, 30), 'GAS! don\u2019t forget', 4)
    tape(lid, (380, 46), 'for Dawa \u2014 eat something', -3)
    tape(lid, (680, 24), 'compass stays IN', 2)
    shadow_paste(out, lid, (bx0, by0 - 175), blur=14, off=(0, 14), a=0.6)
    # the foam
    foam = Image.new('RGBA', (bx1 - bx0, by1 - by0), (34, 36, 40, 255))
    noise = Image.effect_noise(foam.size, 40).convert('L').point(lambda v: int(v * 0.25))
    foam = Image.composite(Image.new('RGBA', foam.size, (54, 56, 62, 255)), foam, noise)
    ImageDraw.Draw(foam).rounded_rectangle((0, 0, foam.width - 1, foam.height - 1), 22, outline=(70, 72, 78, 255), width=4)
    layout = [  # rid, x, y, size, filled
        ('IceAxe', 40, 40, 300, True), ('Rope', 360, 40, 200, True), ('Stove', 580, 40, 170, True), ('Gas', 770, 50, 130, True),
        ('Compass', 360, 270, 130, False), ('Journal', 520, 250, 170, True),
        ('Oxygen', 40, 320, 230, False), ('Oxygen', 40, 410, 230, False), ('Oxygen', 40, 500, 230, False),
        ('Water', 720, 250, 190, True), ('Water', 820, 250, 190, True), ('Water', 720, 440, 190, True), ('Water', 820, 440, 190, True),
    ]
    rations = [(380 + 92 * k, 470 + (0 if k < 4 else 0), k < 6) for k in range(0)]
    for rid, x, y, s, filled in layout:
        im = render(rid, s)
        a = im.split()[3].filter(ImageFilter.MaxFilter(9))
        hole = Image.new('RGBA', im.size, (12, 12, 14, 255)); hole.putalpha(a)
        foam.alpha_composite(hole, (x, y))
        if filled:
            foam.alpha_composite(im, (x, y))
    for k in range(8):   # eight ration slots, six filled
        x, y = 360 + (k % 4) * 90, 440 + (k // 4) * 100
        im = render('Ration', 84)
        a = im.split()[3].filter(ImageFilter.MaxFilter(9)); hole = Image.new('RGBA', im.size, (12, 12, 14, 255)); hole.putalpha(a)
        foam.alpha_composite(hole, (x, y))
        if k < 6:
            foam.alpha_composite(im, (x, y))
    shadow_paste(out, foam, (bx0, by0), blur=18, off=(0, 18), a=0.65)
    # the chosen item lifted out of its cut-out
    lift = render('Compass', 170)
    shadow_paste(out, lift, (bx0 + 340, by0 + 200), blur=16, off=(14, 30), a=0.6)
    L = Layer((W, H))
    L.d.rounded_rectangle((bx0 + 352, by0 + 262, bx0 + 506, by0 + 416), 12, outline=(255, 220, 170, 200), width=2)
    L.text((bx0 + 40, by1 + 46), 'three empty oxygen shapes: you would see it in one glance', caveat(32), (240, 230, 210), 0.85)
    out = L.compose(out).convert('RGBA')
    out = info(out, 1360, 420, 'Compass', width=470, extra='Its needle does not always point north. Mother\u2019s tape on the lid says it stays in the case.')
    return caption(out, 'C', 'Foam case', ['Tab: an expedition case opens, every item in its own cut-out. The case IS the capacity.',
                                           'An empty cut-out tells you what is missing before you read a word; finds fill them back in.',
                                           'Playful part: Mother\u2019s masking-tape labels on the lid, and her notes change as you climb.'])


# ---------------------------------------------------------------- D: on the body
def figure(L, cx, top, s=1.0, col=WHITE):
    a = 210
    def l(pts, w=2): L.d.line([(cx + x * s, top + y * s) for x, y in pts], fill=col + (a,), width=w, joint='curve')
    # a climber in a down suit, front, in thin lines (a technical drawing)
    L.d.ellipse((cx - 46 * s, top, cx + 46 * s, top + 100 * s), outline=col + (a,), width=2)            # hood
    L.d.ellipse((cx - 28 * s, top + 26 * s, cx + 28 * s, top + 82 * s), outline=col + (120,), width=1)  # face opening
    l([(-46, 90), (-120, 130), (-140, 330), (-118, 470)])                                                 # left arm
    l([(46, 90), (120, 130), (140, 330), (118, 470)])                                                     # right arm
    l([(-100, 120), (-110, 420), (-96, 470), (96, 470), (110, 420), (100, 120)])                          # torso
    l([(-96, 470), (-104, 760), (-60, 770), (-14, 520), (14, 520), (60, 770), (104, 760), (96, 470)])     # legs
    l([(-110, 420), (110, 420)], 3)                                                                       # harness belt
    l([(-60, 470), (-14, 520)]); l([(60, 470), (14, 520)])                                               # leg loops
    l([(-40, 120), (-40, 300), (40, 300), (40, 120)], 1)                                                  # chest zip pocket


def design_d():
    out = scene(blur=16, dark=0.66)
    cx, top = 760, 170
    # warm zones (inside the suit) and cold zones (outside)
    glow = Image.new('RGBA', (W, H), (0, 0, 0, 0)); dg = ImageDraw.Draw(glow)
    dg.ellipse((cx - 70, top + 150, cx + 70, top + 330), fill=(255, 150, 70, 90))
    dg.ellipse((cx + 170, top + 160, cx + 330, top + 300), fill=(120, 180, 255, 80))
    out.alpha_composite(glow.filter(ImageFilter.GaussianBlur(30)))
    L = Layer((W, H))
    figure(L, cx, top)
    # the pack outline behind the shoulders
    L.d.rounded_rectangle((cx - 150, top + 70, cx - 106, top + 400), 18, outline=WHITE + (110,), width=1)
    out = L.compose(out).convert('RGBA')
    spots = [  # rid, item xy, anchor on body, label, note, warm?
        ('Water', (cx - 60, top + 170), (cx, top + 230), 'inner pocket', 'warm: stays liquid', True),
        ('Compass', (cx - 40, top + 20), (cx, top + 104), 'on a cord, at the neck', 'Q: hold it up', True),
        ('Journal', (cx + 20, top + 300), (cx + 30, top + 290), 'chest pocket', '', True),
        ('IceAxe', (cx + 150, top + 400), (cx + 110, top + 430), 'harness loop', 'quick: 0.3 s', False),
        ('Rope', (cx - 300, top + 390), (cx - 110, top + 430), 'harness loop', 'quick', False),
        ('Water', (cx + 200, top + 180), (cx + 140, top + 240), 'outside pocket', 'freezes in 20 min', False),
        ('Ration', (cx - 380, top + 110), (cx - 128, top + 160), 'pack: 6 rations', 'slow: 2 s to open', False),
        ('Oxygen', (cx - 380, top + 230), (cx - 128, top + 280), 'pack: 0 / 3 bottles', '', False),
        ('Stove', (cx - 380, top + 560), (cx - 128, top + 380), 'pack: stove and gas', 'camp only', False),
    ]
    for rid, (ix, iy), (ax, ay), lab, note, warm in spots:
        im = render(rid, 96 if rid != 'IceAxe' else 120)
        Lk = Layer((W, H))
        mx, my = ix + im.width / 2, iy + im.height / 2
        Lk.d.line([(mx, my), (ax, ay)], fill=(255, 220, 170, 150) if warm else (170, 210, 255, 150), width=1)
        Lk.d.ellipse((ax - 4, ay - 4, ax + 4, ay + 4), fill=(255, 220, 170, 230) if warm else (170, 210, 255, 230))
        out = Lk.compose(out).convert('RGBA')
        out.alpha_composite(im, (int(ix), int(iy)))
        Lt = Layer((W, H))
        tx = ix + im.width + 10 if ix > cx - 200 else ix - 10
        anc = 'ls' if ix > cx - 200 else 'rs'
        Lt.text((tx, iy + 40), lab, F('Regular', 16), WHITE, 0.9, anchor=anc)
        if note:
            Lt.text((tx, iy + 62), note, F('Regular', 14), (255, 200, 150) if warm else (170, 210, 255), 0.95, anchor=anc)
        out = Lt.compose(out).convert('RGBA')
    out = info(out, 1300, 640, 'Water', width=520, extra='In the inner pocket it stays liquid. Move it to the outside pocket to reach it faster, and it starts to freeze.')
    return caption(out, 'D', 'On the body', ['No bag screen. Tab shows your climber: gear sits where a mountaineer really keeps it.',
                                             'Inner pockets are warm (water stays liquid), harness loops are quick, the pack is slow to open.',
                                             'Playful part: where you put a thing matters: the bottle on the outside freezes, the compass swings on its cord.'])


# ---------------------------------------------------------------- E: camp blanket
def dog(size=260):
    im = Image.new('RGBA', (size, size), (0, 0, 0, 0)); d = ImageDraw.Draw(im)
    fur, dark, nose = (186, 140, 92, 255), (122, 84, 52, 255), (40, 30, 24, 255)
    d.ellipse((14, 90, 214, 226), fill=fur)                       # curled body
    d.arc((4, 120, 120, 236), 110, 250, fill=dark, width=20)      # tail tucked around
    d.ellipse((140, 84, 232, 168), fill=fur)                      # head resting on the paws
    d.ellipse((196, 118, 262, 160), fill=fur)                     # long muzzle
    d.ellipse((246, 126, 262, 140), fill=nose)                    # nose
    d.ellipse((140, 92, 176, 156), fill=dark)                     # floppy ear
    d.arc((186, 104, 212, 120), 200, 340, fill=nose, width=3)     # closed eye
    d.ellipse((150, 196, 236, 222), fill=fur)                     # front paws
    return im


def design_e():
    out = scene(blur=16, dark=0.6)
    bx0, by0, bw, bh = 220, 210, 1180, 720
    blanket = Image.new('RGBA', (bw, bh), (150, 40, 36, 255)); d = ImageDraw.Draw(blanket)
    stripes = [(150, 40, 36), (32, 70, 130), (226, 186, 60), (40, 110, 70), (236, 228, 210), (150, 40, 36)]
    y = 0; k = 0
    while y < bh:
        h = 18 if k % 3 else 46
        d.rectangle((0, y, bw, y + h), fill=stripes[k % len(stripes)] + (255,)); y += h; k += 1
    for x in range(0, bw, 26):                                    # woven texture
        d.line([(x, 0), (x, bh)], fill=(0, 0, 0, 22), width=1)
    noise = Image.effect_noise((bw, bh), 30).convert('L').point(lambda v: int(v * 0.18))
    blanket = Image.composite(Image.new('RGBA', blanket.size, (20, 16, 14, 255)), blanket, noise)
    shadow_paste(out, blanket.rotate(-1.5, resample=Image.BICUBIC, expand=True), (bx0, by0), blur=20, off=(0, 20), a=0.6)
    L = Layer((W, H))
    split = bx0 + 700
    L.d.line([(split, by0 + 30), (split, by0 + bh - 30)], fill=(250, 244, 230, 200), width=2)
    L.text((bx0 + 40, by0 - 24), 'CARRY  \u00b7  14.2 kg', F('Medium', 16), WHITE, 0.95, track=2)
    L.text((split + 30, by0 - 24), 'LEAVE AT DINGBOCHE', F('Medium', 16), WHITE, 0.95, track=2)
    out = L.compose(out).convert('RGBA')
    carry = [('IceAxe', 60, 60, 260), ('Rope', 330, 60, 170), ('Water', 520, 70, 150), ('Compass', 520, 260, 110), ('Journal', 340, 270, 150),
             ('Ration', 60, 380, 110), ('Ration', 180, 380, 110), ('Ration', 300, 400, 110), ('Water', 440, 420, 150)]
    leave = [('Stove', 740, 70, 170), ('Gas', 930, 80, 120), ('Ration', 760, 330, 110), ('Ration', 880, 340, 110), ('Ration', 1000, 330, 110), ('Water', 760, 480, 150)]
    for rid, x, y, s in carry + leave:
        im = render(rid, s)
        shadow_paste(out, im, (bx0 + x, by0 + y), blur=6, off=(4, 8), a=0.55)
    dg = dog(300)
    shadow_paste(out, dg, (bx0 + 860, by0 + 410), blur=10, off=(6, 10), a=0.5)
    L = Layer((W, H))
    L.text((bx0 + 1110, by0 + 430), 'z', caveat(40), WHITE, 0.8); L.text((bx0 + 1135, by0 + 400), 'z', caveat(52), WHITE, 0.7)
    L.text((bx0 + 760, by0 + bh + 50), 'the camp dog sleeps on what you leave behind', caveat(32), (240, 230, 210), 0.9)
    # the trail strip: four quick pockets
    L.text((1480, 820), 'ON THE TRAIL', F('Medium', 13), WHITE, 0.8, track=2)
    out = L.compose(out).convert('RGBA')
    for i, rid in enumerate(['Water', 'Ration', 'Compass', 'IceAxe']):
        Lb = Layer((W, H)); x = 1480 + i * 92
        Lb.d.rounded_rectangle((x, 836, x + 80, 916), 10, outline=WHITE + (140,), width=1)
        Lb.text((x + 8, 930), str(i + 1), F('Medium', 12), WHITE, 0.7)
        out = Lb.compose(out).convert('RGBA')
        im = render(rid, 64); out.alpha_composite(im, (x + (80 - im.width) // 2, 836 + (80 - im.height) // 2))
    out = info(out, 1480, 380, 'Ration', width=380, extra='Three come with you, three wait here with the dog. Every kg above 14 costs breath.')
    return caption(out, 'E', 'Camp blanket', ['At a camp you empty the pack on a woven blanket and drag things: carry, or leave here.',
                                              'On the trail there is no pack screen, only four quick pockets (keys 1-4) that you chose at camp.',
                                              'Playful part: the camp dog sleeps on what you leave and is there when you come back down.'])


if __name__ == '__main__':
    shots = []
    for name, fn in (('A', design_a), ('B', design_b), ('C', design_c), ('D', design_d), ('E', design_e)):
        im = fn().convert('RGB')
        im.save(OUT + f'pack_v3_{name}.png')
        shots.append(im)
        print('saved', name)
    sheet = Image.new('RGB', (1920 * 3 // 2, 1080 * 2 // 2 + 540), (16, 18, 22))
    for i, im in enumerate(shots):
        t = im.resize((960, 540), Image.LANCZOS)
        sheet.paste(t, ((i % 3) * 960 if i < 3 else (i - 3) * 960 + 480, (i // 3) * 540))
    sheet = sheet.crop((0, 0, 2880, 1080))
    sheet.save(OUT + 'pack_v3_sheet.png')
    print('sheet saved')
