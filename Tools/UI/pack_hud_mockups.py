# Pack screen in the HUD's own language (2026-11-14). The user, after round 3 (wheel, flags, case, body, blanket):
# "they all look very bad... what I need is an ordinary backpack system that fits my HUD system". So: an ordinary
# inventory, drawn exactly like HUD v8 (hud_v8_mockup.py): Roboto only, light numerals, small tracked caps labels,
# white with a tight shadow, fine underline bars, line icons, red only for danger, no boxes or frames.
# Five ordinary layouts of the same parts:
#   1 Grid + detail      a 4 x 3 grid of slots on the left, the chosen thing on the right
#   2 List + detail      rows grouped Food and water / Gear / Notes, the chosen thing on the right
#   3 Tabs + big grid    All / Food / Gear / Notes, larger slots with the real items, details in one line below
#   4 Side panel         the game and the HUD stay visible; the pack is a panel on the right
#   5 Bottom row         one row of slots along the bottom (the HUD's kit grown), the chosen thing above it
# Out: Saved/Review/pack_hud_1..5.png, pack_hud_sheet.png
import math, sys
sys.path.insert(0, 'F:/UE_project/YggdrasilsPeak/Tools/UI')
from PIL import Image, ImageDraw, ImageFilter
from hud_v8_mockup import Layer, font, icon as hud_icon, WHITE, RED

P = 'F:/UE_project/YggdrasilsPeak/Saved/'
OUT = P + 'Review/'
W, H = 1920, 1080
M = 56
SOFT = 0.72
CAP = font('Medium', 11); TR = 2.4

# id, name, group, count text, weight (kg, each or total), effect icon/value, description, action, danger
ITEMS = [
    ('Ration', 'Rations', 'Food and water', '6', '0.6 kg each', ('Food', '+35'), 'Tins from the shop in Namche, a twist of tsampa, dried yak cheese.', 'eat one', False),
    ('Water', 'Water', 'Food and water', '4', '1.0 kg each', ('Drop', '+40'), 'A steel bottle of melted snow. It freezes if you leave it outside the pack.', 'drink', False),
    ('Oxygen', 'Oxygen bottles', 'Food and water', '0 / 3', '3.0 kg each', ('Lungs', '30 min'), 'Spare bottles go on by themselves when the air gets too thin. Camps keep a few.', '', True),
    ('IceAxe', 'Ice axe', 'Gear', '', '0.5 kg', None, 'For ice walls, crevasse lips and stopping a slide.', '', False),
    ('Rope', 'Rope', 'Gear', '', '2.4 kg', None, 'Clip it to the fixed lines on the steep sections.', '', False),
    ('Stove', 'Stove', 'Gear', '', '0.4 kg', None, 'Melts snow into water at the camps.', '', False),
    ('Gas', 'Gas cartridge', 'Gear', '2', '0.3 kg each', None, 'Fuel for the stove. One lasts about four camps.', '', False),
    ('Journal', "Mother's journal", 'Notes', '2 pages', '0.2 kg', None, 'Read a page where you find it.', 'read', False),
    ('Compass', "Father's compass", 'Notes', '', '0.1 kg', None, 'Hold it up with Q. Its needle does not always point north.', '', False),
]
BY = {it[0]: it for it in ITEMS}


# ---------------------------------------------------------------- line icons for the kit, in the HUD's stroke
def kit_icon(name, h, col=WHITE, a=1.0):
    N = 256; im = Image.new('RGBA', (N, N), (0, 0, 0, 0)); d = ImageDraw.Draw(im); F = (255, 255, 255, 255); w = 10
    def L(pts): d.line(pts, fill=F, width=w, joint='curve')
    def RR(b, r): d.rounded_rectangle(b, r, outline=F, width=w)
    def C(c, r): d.ellipse((c[0] - r, c[1] - r, c[0] + r, c[1] + r), outline=F, width=w)
    if name == 'Ration':
        RR((52, 92, 204, 200), 18); L([(52, 122), (204, 122)]); d.ellipse((150, 70, 184, 88), outline=F, width=7)
    elif name == 'Water':
        RR((88, 74, 168, 226), 24); RR((106, 36, 150, 74), 8); L([(88, 122), (168, 122)])
    elif name == 'Oxygen':
        RR((96, 76, 160, 228), 30); L([(128, 76), (128, 44)]); L([(104, 44), (152, 44)])
    elif name == 'IceAxe':
        L([(88, 222), (156, 56)]); L([(98, 60), (156, 56), (212, 82)]); L([(156, 56), (174, 40)])
    elif name == 'Rope':
        for r in (82, 58, 34):
            C((128, 128), r)
    elif name == 'Stove':
        C((128, 108), 48); L([(128, 156), (128, 210)]); L([(86, 210), (170, 210)]); L([(94, 70), (162, 70)])
    elif name == 'Gas':
        RR((74, 100, 182, 222), 34); RR((114, 60, 142, 100), 6)
    elif name == 'Journal':
        RR((74, 46, 186, 214), 10); L([(100, 46), (100, 214)])
    elif name == 'Compass':
        C((128, 128), 82); d.polygon([(128, 66), (144, 128), (128, 190), (112, 128)], outline=F, width=7); d.ellipse((121, 121, 135, 135), fill=F)
    im = im.resize((h, h), Image.LANCZOS)
    out = Image.new('RGBA', im.size, col + (0,)); out.putalpha(im.split()[3].point(lambda v: int(v * a)))
    return out


def render(rid, size):
    r = Image.open(P + f'ItemIcons/render/{rid}.png').convert('RGBA')
    s = size / max(r.size)
    return r.resize((max(1, int(r.width * s)), max(1, int(r.height * s))), Image.LANCZOS)


def backdrop(dim=0.62, blur=14, keep_hud=False):
    b = Image.open(OUT + 'hud_v8_valley.png').convert('RGB').resize((W, H))
    if keep_hud:
        return b.convert('RGBA')
    b = b.filter(ImageFilter.GaussianBlur(blur))
    ov = Image.new('RGBA', (W, H), (6, 10, 18, int(255 * dim)))
    out = b.convert('RGBA'); out.alpha_composite(ov)
    return out


# ---------------------------------------------------------------- shared parts
def weight(L, x, y, w=300, align='l'):
    if align == 'r':
        x -= w
    L.text((x, y - 58), 'PACK', CAP, WHITE, SOFT, track=TR)
    ax = L.text((x - 2, y - 12), '17.6', font('Light', 44), WHITE, 0.98)
    L.text((ax + 6, y - 12), '/ 26 kg', font('Light', 20), WHITE, SOFT)
    L.rect((x, y, x + w, y + 1), WHITE, 0.3)
    L.rect((x, y, x + int(w * 17.6 / 26), y + 1), WHITE, 0.95)
    hx = x + int(w * 14 / 26)
    L.rect((hx, y - 4, hx + 1, y + 5), WHITE, 0.6)
    L.text((x, y + 24), 'heavier than 14 kg: every step costs more', font('Regular', 14), WHITE, SOFT)


def keys(L, x, y, pairs=(('W A S D', 'choose'), ('Enter', 'use'), ('Tab', 'close')), align='r'):
    f = font('Regular', 15); parts = [f'{t}  [{k}]' for k, t in pairs]
    s = '      '.join(parts)
    L.text((x, y), s, f, WHITE, SOFT, anchor='rs' if align == 'r' else 'ls')


def slot(L, out, x, y, s, it, sel=False, photo=False):
    """a slot: a faint square, the icon, the count bottom right; chosen = brighter and a fine line under it"""
    L.rect((x, y, x + s, y + s), WHITE, 0.10 if sel else 0.045)
    if sel:
        L.rect((x, y + s - 1, x + s, y + s + 1), WHITE, 0.95)
    if it is None:
        return
    rid = it[0]
    if photo:
        im = render(rid, int(s * 0.74))
        out.alpha_composite(im, (int(x + (s - im.width) / 2), int(y + (s - im.height) / 2 - 4)))
    else:
        im = kit_icon(rid, int(s * 0.42), WHITE, 1.0 if sel else 0.85)
        L.paste(im, (x + (s - im.width) / 2, y + (s - im.height) / 2 - 6))
    if it[3]:
        c = RED if it[8] else WHITE
        L.text((x + s - 10, y + s - 12), it[3], font('Regular', 15), c, 1.0 if it[8] else 0.9, anchor='rs')


def detail(L, out, x, y, it, width=520, photo=True, big=230):
    rid = it[0]
    if photo:
        im = render(rid, big)
        out.alpha_composite(im, (int(x), int(y)))
        y += im.height + 34
    else:
        im = kit_icon(rid, 96, WHITE, 0.95); L.paste(im, (x, y)); y += 130
    L.text((x, y), it[2].upper(), CAP, WHITE, SOFT, track=TR)
    L.text((x - 2, y + 48), it[1], font('Light', 46), WHITE, 0.98)
    y += 92
    # readings in the HUD's way: icon + light numeral on one baseline, then the weight
    rx = x
    if it[5]:
        ic = hud_icon(it[5][0], 22, WHITE, 0.92); L.paste(ic, (rx, y - 22))
        rx = L.text((rx + ic.width + 9, y), it[5][1], font('Light', 30), WHITE) + 34
    if it[3]:
        c = RED if it[8] else WHITE
        rx = L.text((rx, y), it[3], font('Light', 30), c) + 8
        L.text((rx, y), 'left' if it[0] != 'Journal' else '', font('Regular', 15), c, SOFT); rx += 60
    L.text((rx, y), it[4], font('Regular', 15), WHITE, SOFT)
    y += 40
    y = L.wrap((x, y), it[6], font('Regular', 18), width, WHITE, 0.88) if hasattr(L, 'wrap') else wrap(L, x, y, it[6], width)
    if it[8]:
        L.dot((x + 4, y + 18), 3.5, RED, 1.0)
        L.text((x + 16, y + 23), 'None left', font('Medium', 16), RED)
        L.text((x + 100, y + 23), 'find bottles at the camps above 7,000 m', font('Regular', 16), WHITE, 0.88)
        y += 40
    if it[7]:
        L.text((x, y + 30), f'{it[7]}  [Enter]', font('Regular', 17), WHITE, 0.95)
    return y


def wrap(L, x, y, s, width, f=None, a=0.88, lh=1.5):
    f = f or font('Regular', 18); line = ''
    for word in s.split():
        t = (line + ' ' + word).strip()
        if L.w(t, f) > width:
            L.text((x, y), line, f, WHITE, a); y += f.size * lh; line = word
        else:
            line = t
    if line:
        L.text((x, y), line, f, WHITE, a); y += f.size * lh
    return y


def title(L, x, y, s='Pack'):
    L.text((x - 3, y), s, font('Light', 54), WHITE, 0.98)


# ---------------------------------------------------------------- 1: grid + detail
def layout1():
    out = backdrop(); L = Layer((W, H))
    title(L, 150, 190)
    s, g = 128, 14; x0, y0 = 150, 250
    order = ['Ration', 'Water', 'Oxygen', 'Gas', 'IceAxe', 'Rope', 'Stove', 'Journal', 'Compass', None, None, None]
    for i, rid in enumerate(order):
        x, y = x0 + (i % 4) * (s + g), y0 + (i // 4) * (s + g)
        slot(L, out, x, y, s, BY.get(rid), sel=(rid == 'Ration'))
    weight(L, x0, y0 + 3 * (s + g) + 92, w=4 * s + 3 * g)
    out = Image.fromarray  # placeholder to keep linters calm
    return L


def compose(L, base):
    return L.compose(base).convert('RGBA')


def build1():
    base = backdrop(); L = Layer((W, H))
    title(L, 150, 190)
    s, g = 128, 14; x0, y0 = 150, 250
    order = ['Ration', 'Water', 'Oxygen', 'Gas', 'IceAxe', 'Rope', 'Stove', 'Journal', 'Compass', None, None, None]
    for i, rid in enumerate(order):
        x, y = x0 + (i % 4) * (s + g), y0 + (i // 4) * (s + g)
        slot(L, base, x, y, s, BY.get(rid), sel=(rid == 'Ration'))
    weight(L, x0, y0 + 3 * (s + g) + 92, w=4 * s + 3 * g)
    detail(L, base, 1000, 230, BY['Ration'], width=600)
    keys(L, W - M, H - 52)
    return L.compose(base)


def build2():
    base = backdrop(); L = Layer((W, H))
    title(L, 150, 190)
    x0, y, rw = 150, 262, 640
    groups = ['Food and water', 'Gear', 'Notes']
    sel = 'Water'
    for gname in groups:
        L.text((x0, y), gname.upper(), CAP, WHITE, SOFT, track=TR); y += 18
        for it in [i for i in ITEMS if i[2] == gname]:
            on = it[0] == sel
            if on:
                L.rect((x0, y, x0 + rw, y + 52), WHITE, 0.08)
            ic = kit_icon(it[0], 30, WHITE, 1.0 if on else 0.82); L.paste(ic, (x0 + 14, y + 11))
            L.text((x0 + 62, y + 34), it[1], font('Regular', 20), WHITE, 1.0 if on else 0.86)
            if it[3]:
                c = RED if it[8] else WHITE
                L.text((x0 + rw - 150, y + 35), it[3], font('Light', 26), c, 1.0, anchor='rs')
            L.text((x0 + rw - 14, y + 34), it[4].replace(' each', ''), font('Regular', 15), WHITE, SOFT, anchor='rs')
            L.rect((x0, y + 52, x0 + rw, y + 53), WHITE, 0.95 if on else 0.12)
            y += 56
        y += 26
    weight(L, 1000, 250, w=420)
    detail(L, base, 1000, 380, BY['Water'], width=600, big=200)
    keys(L, W - M, H - 52)
    return L.compose(base)


def build3():
    base = backdrop(); L = Layer((W, H))
    title(L, 150, 170)
    tx = 150; ty = 236
    for i, t in enumerate(['All', 'Food and water', 'Gear', 'Notes']):
        on = i == 0
        f = font('Regular', 19)
        L.text((tx, ty), t, f, WHITE, 1.0 if on else 0.62)
        if on:
            L.rect((tx, ty + 12, tx + L.w(t, f), ty + 14), WHITE, 0.95)
        tx += L.w(t, f) + 40
    weight(L, W - 150, 236, w=360, align='r')
    s, g = 196, 18; x0, y0 = 150, 290
    order = ['Ration', 'Water', 'Oxygen', 'Gas', 'IceAxe', 'Rope', 'Stove', 'Journal', 'Compass', None, None, None]
    for i, rid in enumerate(order):
        x, y = x0 + (i % 6) * (s + g), y0 + (i // 6) * (s + g)
        slot(L, base, x, y, s, BY.get(rid), sel=(rid == 'Compass'), photo=True)
    it = BY['Compass']
    y = y0 + 2 * (s + g) + 58
    L.text((150, y), it[2].upper(), CAP, WHITE, SOFT, track=TR)
    nx = L.text((148, y + 50), it[1], font('Light', 44), WHITE, 0.98)
    L.text((nx + 26, y + 50), it[4], font('Regular', 15), WHITE, SOFT)
    wrap(L, 150, y + 92, it[6], 1100)
    keys(L, W - M, H - 52, pairs=(('W A S D', 'choose'), ('Q', 'hold up'), ('Tab', 'close')))
    return L.compose(base)


def build4():
    base = backdrop(keep_hud=True)
    # a dark panel that fades in from the right; the HUD and the view stay on the left
    g = Image.new('RGBA', (W, H), (0, 0, 0, 0)); dg = ImageDraw.Draw(g)
    for x in range(1100, W):
        t = min(1.0, (x - 1100) / 260)
        dg.line([(x, 0), (x, H)], fill=(6, 10, 18, int(255 * 0.80 * t)))
    base.alpha_composite(g)
    L = Layer((W, H))
    px = 1360; rw = 500
    L.text((px, 120), 'PACK', CAP, WHITE, SOFT, track=TR)
    ax = L.text((px - 2, 166), '17.6', font('Light', 44), WHITE, 0.98); L.text((ax + 6, 166), '/ 26 kg', font('Light', 20), WHITE, SOFT)
    L.rect((px, 182, px + rw, 183), WHITE, 0.3); L.rect((px, 182, px + int(rw * 17.6 / 26), 183), WHITE, 0.95)
    L.rect((px + int(rw * 14 / 26), 178, px + int(rw * 14 / 26) + 1, 187), WHITE, 0.6)
    y = 214; sel = 'Water'
    for it in ITEMS:
        on = it[0] == sel
        if on:
            L.rect((px - 12, y, px + rw, y + 46), WHITE, 0.08)
        ic = kit_icon(it[0], 26, WHITE, 1.0 if on else 0.82); L.paste(ic, (px, y + 10))
        L.text((px + 44, y + 30), it[1], font('Regular', 19), WHITE, 1.0 if on else 0.86)
        if it[3]:
            c = RED if it[8] else WHITE
            L.text((px + rw - 10, y + 31), it[3], font('Light', 24), c, 1.0, anchor='rs')
        L.rect((px - 12, y + 46, px + rw, y + 47), WHITE, 0.9 if on else 0.1)
        y += 46
    it = BY['Water']; y += 70
    L.text((px, y), it[1], font('Light', 36), WHITE, 0.98); y += 48
    ic = hud_icon('Drop', 22, WHITE, 0.92); L.paste(ic, (px, y - 22))
    rx = L.text((px + ic.width + 9, y), '+40', font('Light', 30), WHITE) + 30
    L.text((rx, y), '1.0 kg each', font('Regular', 15), WHITE, SOFT)
    y = wrap(L, px, y + 40, it[6], rw, a=0.88)
    L.text((px, y + 24), 'drink  [Enter]', font('Regular', 17), WHITE, 0.95)
    keys(L, W - M, H - 52)
    return L.compose(base)


def build5():
    base = backdrop(dim=0.45, blur=8); L = Layer((W, H))
    s, g = 112, 12; n = 12
    x0 = (W - (n * s + (n - 1) * g)) / 2; y0 = H - 210
    order = ['Ration', 'Water', 'Oxygen', 'Gas', 'IceAxe', 'Rope', 'Stove', 'Journal', 'Compass', None, None, None]
    for i, rid in enumerate(order):
        slot(L, base, x0 + i * (s + g), y0, s, BY.get(rid), sel=(rid == 'Oxygen'))
        if i < 9:
            L.text((x0 + i * (s + g) + 10, y0 + 22), str(i + 1), font('Regular', 13), WHITE, 0.55)
    it = BY['Oxygen']
    cx = x0 + 2 * (s + g) + s / 2
    im = render('Oxygen', 260); base.alpha_composite(im, (int(cx - im.width / 2), int(y0 - 400)))
    L.text((cx, y0 - 150), it[2].upper(), CAP, WHITE, SOFT, anchor='ms')
    L.text((cx, y0 - 100), it[1], font('Light', 46), WHITE, 0.98, anchor='ms')
    L.dot((cx - 200, y0 - 62), 3.5, RED, 1.0)
    L.text((cx - 188, y0 - 56), 'None left', font('Medium', 16), RED)
    L.text((cx - 100, y0 - 56), 'spare bottles go on by themselves; camps keep a few', font('Regular', 16), WHITE, 0.88)
    weight(L, x0 + n * (s + g) - g, y0 - 60, w=300, align='r')
    keys(L, W - M, H - 40)
    return L.compose(base)


if __name__ == '__main__':
    shots = []
    for i, fn in enumerate((build1, build2, build3, build4, build5), 1):
        im = fn().convert('RGB'); im.save(OUT + f'pack_hud_{i}.png'); shots.append(im); print('saved', i)
    sheet = Image.new('RGB', (2880, 1080), (16, 18, 22))
    for i, im in enumerate(shots):
        t = im.resize((960, 540), Image.LANCZOS)
        sheet.paste(t, ((i % 3) * 960 if i < 3 else (i - 3) * 960 + 480, (i // 3) * 540))
    sheet.save(OUT + 'pack_hud_sheet.png'); print('sheet')
