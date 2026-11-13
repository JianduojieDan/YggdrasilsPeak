# Pack screen, round 2 (2026-11-13). The user: proposal 1 (list) is better but not industrial standard; not Cairn's random
# pile, not a PUBG list; "add some funny part". Three directions, each tidy, each with one playful idea:
#   A "Field journal"  - the pack as a page of Mother's notebook: ink sketches, handwritten counts, used-up things crossed
#                        out in pencil, and her notes in the margin (the playful part)
#   B "Gear rack"      - items hang from carabiners on a rack, HUD style; the weight is a spring scale that stretches
#   C "Pocket map"     - a line drawing of the real pack, each item in its own pocket; the pack bulges when heavy
# Out: Saved/Review/pack_v2_A.png, pack_v2_B.png, pack_v2_C.png, pack_v2_sheet.png
import math, random, sys
sys.path.insert(0, 'F:/UE_project/YggdrasilsPeak/Tools/UI')
from PIL import Image, ImageDraw, ImageFont, ImageFilter
from pack_mockups import icon, tint, Layer, backdrop, ITEMS, F, WHITE, RED, P

ROOT = 'F:/UE_project/YggdrasilsPeak/'
def caveat(px): return ImageFont.truetype(ROOT + 'Imports/Fonts/Caveat.ttf', px)
def alegreya(px): return ImageFont.truetype(ROOT + 'Imports/Fonts/Alegreya.ttf', px)
INK = (44, 40, 36); PENCIL = (96, 92, 88); NOTE = (150, 62, 40); PAPER = (236, 228, 210)


def _line(self, pts, c, w=1, a=1.0):
    self.ds.line([(x + 0.5, y + 1) for x, y in pts], fill=(0, 6, 16, int(255 * 0.4 * a)), width=w + 1)
    self.d.line(pts, fill=c + (int(255 * a),), width=w)
Layer.line = _line


def render(rid, size):
    r = Image.open(P + f'ItemIcons/render/{rid}.png').convert('RGBA')
    s = size / max(r.size)
    return r.resize((max(1, int(r.width * s)), max(1, int(r.height * s))), Image.LANCZOS)


def keys(L, W, H, col=WHITE, a=0.72):
    f = F('Regular', 14); x = W - 64
    for k, t in (('Tab', 'close'), ('Enter', 'use'), ('W  S', 'choose')):
        tw = L.w(t, f); L.text((x, H - 56), t, f, col, a, anchor='rs'); x -= tw + 10
        kw = L.w(k, F('Medium', 12)) + 14
        L.d.rounded_rectangle((x - kw, H - 74, x, H - 50), 4, outline=col + (110,), width=1)
        L.text((x - kw / 2, H - 56), k, F('Medium', 12), col, 0.9, anchor='ms'); x -= kw + 26


# ---------------------------------------------------------------- A: field journal
def design_a():
    base = backdrop(); W, H = base.size
    page = Image.new('RGBA', (1500, 900), PAPER + (255,))
    noise = Image.effect_noise((1500, 900), 18).convert('L').point(lambda v: int(v * 0.12))
    page = Image.composite(Image.new('RGBA', page.size, (200, 190, 170, 255)), page, noise)
    d = ImageDraw.Draw(page)
    for y in range(120, 880, 38):                                     # faint ruled lines
        d.line([(60, y), (700, y)], fill=(170, 180, 196, 70), width=1); d.line([(800, y), (1440, y)], fill=(170, 180, 196, 70), width=1)
    shade = Image.new('L', (1500, 900), 0); ds = ImageDraw.Draw(shade)  # the binding
    for i in range(60):
        ds.line([(750 - i, 0), (750 - i, 900)], fill=int(90 * (1 - i / 60) ** 2)); ds.line([(750 + i, 0), (750 + i, 900)], fill=int(90 * (1 - i / 60) ** 2))
    page = Image.composite(Image.new('RGBA', page.size, (120, 100, 80, 255)), page, shade)
    # shadow under the notebook, then the notebook
    sh = Image.new('RGBA', base.size, (0, 0, 0, 0)); ImageDraw.Draw(sh).rounded_rectangle((210 + 10, 90 + 14, 1710 + 10, 990 + 14), 12, fill=(0, 0, 0, 150))
    out = base.convert('RGBA'); out.alpha_composite(sh.filter(ImageFilter.GaussianBlur(18))); out.alpha_composite(page, (210, 90))
    d = ImageDraw.Draw(out)
    ox, oy = 210, 90
    d.text((ox + 70, oy + 92), 'The pack', font=alegreya(52), fill=INK, anchor='ls')
    d.text((ox + 72, oy + 132), '17.6 of 26 kg  \u2014  heavy, every step costs', font=caveat(30), fill=PENCIL, anchor='ls')
    # a hand-drawn weight line
    random.seed(4)
    pts = [(ox + 72 + i * 6, oy + 150 + random.uniform(-1, 1)) for i in range(0, 76)]
    d.line(pts, fill=PENCIL, width=2)
    d.line([(p[0], p[1] + 5) for p in pts[:int(76 * 17.6 / 26)]], fill=INK, width=3)
    # items: a 3x3 grid of ink sketches with handwritten labels
    sel = 0
    for i, it in enumerate(ITEMS):
        rid, name, cnt, wt, kind, *_ = it
        cx, cy = ox + 90 + (i % 3) * 205, oy + 200 + (i // 3) * 215
        ink = icon(rid, 96)
        ink = ink.rotate(random.uniform(-6, 6), resample=Image.BICUBIC, expand=False)
        out.alpha_composite(tint(ink, INK, 0.92), (cx + 30, cy))
        d.text((cx + 78, cy + 136), name.split("'s ")[-1].capitalize() if "'s" in name else name, font=caveat(30), fill=INK, anchor='ms')
        if cnt:
            d.text((cx + 150, cy + 28), ('\u00d7' + cnt) if cnt[0].isdigit() and '/' not in cnt else cnt, font=caveat(30), fill=NOTE if cnt.startswith('0') else INK, anchor='ms')
        if kind == 'food' and cnt.startswith('0'):                    # used up: crossed out in pencil
            d.line([(cx + 20, cy + 110), (cx + 140, cy + 10)], fill=PENCIL, width=3)
        if i == sel:                                                  # the chosen one: circled in pencil
            c0 = (cx + 78, cy + 70)
            arc = [(c0[0] + 92 * math.cos(t / 30 * 2 * math.pi) + random.uniform(-2, 2), c0[1] + 82 * math.sin(t / 30 * 2 * math.pi) + random.uniform(-2, 2)) for t in range(33)]
            d.line(arc, fill=NOTE, width=3)
    # right page: the chosen item
    rid, name, cnt, wt, kind, desc, stats, act = ITEMS[sel]
    big = tint(icon(rid, 210), INK, 0.95); out.alpha_composite(big, (ox + 900, oy + 120))
    d.text((ox + 860, oy + 400), name, font=alegreya(46), fill=INK, anchor='ls')
    d.text((ox + 862, oy + 440), f'{cnt} left  \u00b7  {stats}  \u00b7  {wt}', font=caveat(30), fill=PENCIL, anchor='ls')
    y = oy + 492; line = ''
    for w in desc.split():
        t = (line + ' ' + w).strip()
        if d.textlength(t, font=alegreya(26)) > 540:
            d.text((ox + 860, y), line, font=alegreya(26), fill=INK, anchor='ls'); y += 38; line = w
        else:
            line = t
    d.text((ox + 860, y), line, font=alegreya(26), fill=INK, anchor='ls')
    # Mother's margin note (the playful part: her notes change with what you carry and where you are)
    note = Image.new('RGBA', (520, 140), (0, 0, 0, 0)); dn = ImageDraw.Draw(note)
    dn.text((10, 40), "Don't eat the yak cheese", font=caveat(36), fill=NOTE + (255,))
    dn.text((10, 86), 'before Base Camp.  \u2014 Mum', font=caveat(36), fill=NOTE + (255,))
    out.alpha_composite(note.rotate(4, resample=Image.BICUBIC, expand=False), (ox + 900, oy + 650))
    d.text((ox + 860, oy + 830), 'Enter: eat one', font=F('Medium', 16), fill=NOTE, anchor='ls')
    L = Layer(base.size); keys(L, W, H)
    return L.compose(out.convert('RGB'))


# ---------------------------------------------------------------- B: gear rack + spring scale
def design_b():
    base = backdrop(); W, H = base.size; L = Layer(base.size)
    # the spring scale: hook, spring stretched by the load, the dial
    x, y0 = 150, 120; load = 17.6 / 26
    L.line([(x, y0 - 40), (x, y0)], WHITE, 2, 0.9)
    L.d.arc((x - 10, y0 - 60, x + 10, y0 - 40), 180, 360, fill=WHITE + (230,), width=2)
    stretch = 70 + 90 * load
    coil = [(x + (12 if (i % 2) else -12), y0 + i * stretch / 20) for i in range(21)]
    L.line([(x, y0)] + coil + [(x, y0 + stretch + 10)], WHITE, 2, 0.9)
    cy = y0 + stretch + 70
    L.d.ellipse((x - 54, cy - 54, x + 54, cy + 54), outline=WHITE + (230,), width=2)
    for k in range(27):
        a = math.radians(-225 + 270 * k / 26); r0 = 44 if k % 5 else 38
        L.line([(x + r0 * math.cos(a), cy + r0 * math.sin(a)), (x + 50 * math.cos(a), cy + 50 * math.sin(a))], WHITE, 1, 0.7)
    a = math.radians(-225 + 270 * load); L.line([(x, cy), (x + 42 * math.cos(a), cy + 42 * math.sin(a))], RED, 3, 1.0)
    L.text((x + 80, cy - 6), '17.6', F('Light', 54), WHITE); L.text((x + 80, cy + 24), 'of 26 kg  \u00b7  heavy above 14', F('Regular', 15), WHITE, 0.72)
    # the rack: a bar, carabiners, items hanging in a tidy row
    rx0, rx1, ry = 470, 1200, 250
    L.line([(rx0, ry), (rx1, ry)], WHITE, 3, 0.85)
    sel = 0; slot = (rx1 - rx0) / 5
    for i, it in enumerate(ITEMS):
        rid, name, cnt, wt, kind, *_ = it
        row, col = divmod(i, 5)
        cx = rx0 + slot * (col + 0.5) + (slot / 2 if row else 0); top = ry + row * 360
        if row:
            L.line([(rx0 + slot / 2, top), (rx1, top)], WHITE, 3, 0.85) if col == 0 else None
        L.d.ellipse((cx - 9, top - 4, cx + 9, top + 24), outline=WHITE + (220,), width=2)       # carabiner
        lift = -10 if i == sel else 0
        img = render(rid, 150)
        empty = kind == 'food' and cnt.startswith('0')
        if empty:
            img.putalpha(img.split()[3].point(lambda v: int(v * 0.22)))
        L.paste(img, (cx - img.width / 2, top + 30 + lift))
        L.text((cx, top + 214), name, F('Regular', 15), WHITE, 0.95 if i == sel else 0.7, anchor='ms')
        if cnt:
            L.text((cx, top + 240), cnt, F('Light', 22), RED if empty else WHITE, 1.0, anchor='ms')
        if i == sel:
            L.d.rounded_rectangle((cx - 76, top + 20 + lift, cx + 76, top + 254), 10, outline=WHITE + (200,), width=2)
    # details
    rid, name, cnt, wt, kind, desc, stats, act = ITEMS[sel]
    L.text((1300, 360), name, F('Light', 40), WHITE)
    L.text((1300, 396), f'{cnt} left  \u00b7  {stats}  \u00b7  {wt}', F('Regular', 16), WHITE, 0.72)
    L.wrap((1300, 440), desc, F('Regular', 18), 520, WHITE, 0.92)
    L.text((1300, 560), 'Enter  eat one', F('Medium', 16), RED)
    keys(L, W, H)
    return L.compose(base)


# ---------------------------------------------------------------- C: pocket map
def design_c():
    base = backdrop(); W, H = base.size; L = Layer(base.size)
    load = 17.6 / 26
    bx, by, bw, bh = 300, 150, 520, 760
    bulge = 26 * load                                                # the pack swells with the load
    # silhouette: lid, body (bulging), front pocket, straps, side bottle sleeve
    L.d.rounded_rectangle((bx + 30 - bulge, by + 140, bx + bw - 30 + bulge, by + bh), 70, outline=WHITE + (220,), width=2)   # body
    L.d.rounded_rectangle((bx + 60, by + 30, bx + bw - 60, by + 170), 60, outline=WHITE + (220,), width=2)       # lid
    L.d.arc((bx + 150, by - 40, bx + 250, by + 60), 180, 360, fill=WHITE + (160,), width=2)                       # haul loop
    for yy in (by + 260, by + 420):                                                                               # compression straps
        L.line([(bx + 30 - bulge, yy), (bx + 90 - bulge, yy)], WHITE, 2, 0.5); L.line([(bx + bw - 90 + bulge, yy), (bx + bw - 30 + bulge, yy)], WHITE, 2, 0.5)
    L.d.rounded_rectangle((bx + 120, by + 470, bx + bw - 120, by + bh - 70), 30, outline=WHITE + (160,), width=2)  # front pocket
    L.d.rounded_rectangle((bx + bw - 10 + bulge, by + 380, bx + bw + 70 + bulge, by + 620), 26, outline=WHITE + (160,), width=2)  # sleeve
    L.line([(bx + 30 - bulge, by + 300), (bx - 50 - bulge, by + 640)], WHITE, 2, 0.6)                                # axe strap
    L.text((bx + bw / 2, by + 20), 'LID', F('Medium', 10), WHITE, 0.5, anchor='ms', track=0)
    pockets = {  # item -> (x, y) inside its pocket
        'Compass': (bx + 190, by + 70), 'Journal': (bx + 300, by + 70),
        'Ration': (bx + 160, by + 210), 'Water': (bx + 290, by + 210), 'Stove': (bx + 160, by + 340), 'Gas': (bx + 290, by + 340),
        'Rope': (bx + 220, by + 520), 'Oxygen': (bx + bw + 4 + bulge, by + 440), 'IceAxe': (bx - 110 - bulge, by + 420)}
    sel = 0
    for i, it in enumerate(ITEMS):
        rid, name, cnt, wt, kind, *_ = it
        px, py = pockets[rid]
        empty = kind == 'food' and cnt.startswith('0')
        ic = icon(rid, 64)
        if i == sel:
            L.rect((px - 14, py - 12, px + 110, py + 80), WHITE, 0.07)
        if empty:                                                    # used up: only its outline, faint
            L.paste(tint(ic, WHITE, 0.28), (px, py))
        else:
            L.paste(tint(ic, WHITE, 1.0 if i == sel else 0.85), (px, py))
        if cnt:
            L.text((px + 70, py + 22), cnt, F('Light', 20), RED if empty else WHITE, 1.0)
        if i == sel:
            L.d.rounded_rectangle((px - 14, py - 12, px + 110, py + 80), 12, outline=WHITE + (230,), width=2)
    # weight, top left, as in the HUD
    L.text((64, 96), 'PACK', F('Medium', 11), WHITE, 0.72, track=2.4)
    ax = L.text((61, 150), '17.6', F('Light', 54), WHITE); L.text((ax + 6, 150), '/ 26 kg', F('Light', 22), WHITE, 0.72)
    L.text((64, 176), 'the pack swells as it fills', F('Regular', 14), WHITE, 0.6)
    rid, name, cnt, wt, kind, desc, stats, act = ITEMS[sel]
    L.paste(render(rid, 240), (1260, 180))
    L.text((1200, 470), name, F('Light', 40), WHITE)
    L.text((1200, 506), f'{cnt} left  \u00b7  {stats}  \u00b7  main compartment', F('Regular', 16), WHITE, 0.72)
    L.wrap((1200, 550), desc, F('Regular', 18), 560, WHITE, 0.92)
    L.text((1200, 670), 'Enter  eat one', F('Medium', 16), RED)
    keys(L, W, H)
    return L.compose(base)


if __name__ == '__main__':
    a, b, c = design_a(), design_b(), design_c()
    a.save(P + 'Review/pack_v2_A.png'); b.save(P + 'Review/pack_v2_B.png'); c.save(P + 'Review/pack_v2_C.png')
    s = Image.new('RGB', (1920, 1620), (10, 12, 16))
    s.paste(a.resize((960, 540)), (0, 0)); s.paste(b.resize((960, 540)), (960, 0)); s.paste(c.resize((960, 540)), (0, 540))
    s.paste(a.crop((230, 100, 1190, 640)), (960, 540)); s.paste(b.crop((60, 80, 1020, 620)), (0, 1080)); s.paste(c.crop((180, 120, 1140, 660)), (960, 1080))
    s.save(P + 'Review/pack_v2_sheet.png'); print('ok')
