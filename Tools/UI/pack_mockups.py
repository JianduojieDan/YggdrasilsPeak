# Pack screen proposals (Development_Plan.md A6; the user: "bad backpack"), in the HUD v8 language: Roboto, white and
# one red, thin lines, line icons, a light shadow, the game blurred behind.
#   1 "Gear list"  - a list (icon, name, count, weight), a weight meter, the chosen item large with its text
#   2 "Gear grid"  - tiles on dark glass with count badges, the chosen tile outlined, details beside
# Out: Saved/Review/pack_proposal_1.png, pack_proposal_2.png, pack_proposals.png
from PIL import Image, ImageDraw, ImageFont, ImageFilter
import math
P = 'F:/UE_project/YggdrasilsPeak/Saved/'
FD = 'E:/UE5.8/UE_5.8/Engine/Content/Slate/Fonts/'
def F(w, px): return ImageFont.truetype(FD + f'Roboto-{w}.ttf', px)
WHITE = (244, 246, 248); RED = (238, 70, 58); SH = (0, 6, 16)

ITEMS = [  # id, name, count text, weight text, kind, description, stats, action
    ('Ration', 'Rations', '6', '0.6 kg each', 'food', 'Tins from the shop in Namche, a twist of tsampa, dried yak cheese. Mother packed food like this for every trek.', '+35 food', 'Enter  eat one'),
    ('Water', 'Water', '4', '1.0 kg each', 'food', 'A steel bottle of melted snow. It freezes if you forget it on the outside of the pack.', '+40 water', 'Enter  drink'),
    ('Oxygen', 'Oxygen bottles', '0 / 3', '3.0 kg each', 'food', 'Spare bottles go on by themselves when the air gets too thin. Camps keep a few.', '30 min each', ''),
    ('IceAxe', 'Ice axe', '', 'gear', 'gear', 'For ice walls, crevasse lips and stopping a slide.', '', ''),
    ('Rope', 'Rope', '', 'gear', 'gear', 'Clip it to the fixed lines on the steep sections.', '', ''),
    ('Stove', 'Stove', '', 'gear', 'gear', 'Melts snow into water at the camps.', '', ''),
    ('Gas', 'Gas cartridge', '', 'gear', 'gear', 'Fuel for the stove.', '', ''),
    ('Journal', "Mother's journal", '2 pages', 'gear', 'gear', 'Read a page where you find it.', '', 'Enter  read'),
    ('Compass', "Father's compass", '', 'gear', 'gear', 'Hold it up with Q. Its needle does not always point north.', '', ''),
]


# ---------------------------------------------------------------- line icons (white, 256 px, downsampled)
def icon(kind, size):
    N = 256; im = Image.new('RGBA', (N, N), (0, 0, 0, 0)); d = ImageDraw.Draw(im); W = (255, 255, 255, 255); w = 11
    def L(pts): d.line(pts, fill=W, width=w, joint='curve')
    def RR(b, r): d.rounded_rectangle(b, r, outline=W, width=w)
    def C(c, r): d.ellipse((c[0] - r, c[1] - r, c[0] + r, c[1] + r), outline=W, width=w)
    if kind == 'Ration':
        RR((48, 84, 208, 196), 22); L([(48, 118), (208, 118)]); d.ellipse((150, 64, 186, 84), outline=W, width=w - 3)
    elif kind == 'Water':
        RR((84, 70, 172, 226), 26); RR((104, 30, 152, 70), 10); L([(84, 120), (172, 120)])
    elif kind == 'Oxygen':
        RR((92, 70, 164, 228), 34); L([(128, 70), (128, 40)]); L([(100, 40), (156, 40)]); L([(92, 190), (164, 190)])
    elif kind == 'IceAxe':
        L([(84, 226), (156, 52)]); L([(96, 56), (156, 52), (214, 78)]); L([(156, 52), (176, 36)])
    elif kind == 'Rope':
        for r in (86, 62, 38):
            C((128, 128), r)
        L([(208, 150), (232, 214)])
    elif kind == 'Stove':
        C((128, 112), 52); L([(128, 164), (128, 214)]); L([(84, 214), (172, 214)]); L([(92, 72), (164, 72)])
    elif kind == 'Gas':
        RR((70, 96, 186, 224), 36); RR((112, 54, 144, 96), 6); L([(70, 150), (186, 150)])
    elif kind == 'Journal':
        RR((70, 44, 190, 216), 10); L([(96, 44), (96, 216)]); L([(160, 44), (160, 216)])
    elif kind == 'Compass':
        C((128, 128), 86); d.polygon([(128, 62), (146, 128), (128, 194), (110, 128)], outline=W, width=w - 2); d.ellipse((120, 120, 136, 136), fill=W)
    return im.resize((size, size), Image.LANCZOS)


def tint(im, col, a=1.0):
    out = Image.new('RGBA', im.size, col + (0,)); out.putalpha(im.split()[3].point(lambda v: int(v * a))); return out


class Layer:
    def __init__(self, size):
        self.im = Image.new('RGBA', size, (0, 0, 0, 0)); self.sh = Image.new('RGBA', size, (0, 0, 0, 0))
        self.d = ImageDraw.Draw(self.im); self.ds = ImageDraw.Draw(self.sh)
    def text(self, xy, s, f, c=WHITE, a=1.0, anchor='ls', track=0):
        if track:
            x, y = xy
            for ch in s:
                self.text((x, y), ch, f, c, a, 'ls'); x += self.d.textlength(ch, font=f) + track
            return x
        self.ds.text((xy[0] + 0.5, xy[1] + 1.2), s, font=f, fill=SH + (int(255 * 0.5 * a),), anchor=anchor)
        self.d.text(xy, s, font=f, fill=c + (int(255 * a),), anchor=anchor)
        return xy[0] + self.d.textlength(s, font=f)
    def w(self, s, f): return self.d.textlength(s, font=f)
    def rect(self, b, c, a):
        self.d.rectangle(b, fill=c + (int(255 * a),))
    def paste(self, im, xy):
        sh = Image.new('RGBA', im.size, SH + (0,)); sh.putalpha(im.split()[3].point(lambda v: int(v * 0.45)))
        self.sh.alpha_composite(sh, (int(xy[0]), int(xy[1]) + 1)); self.im.alpha_composite(im, (int(xy[0]), int(xy[1])))
    def wrap(self, xy, s, f, width, c=WHITE, a=1.0, lh=1.45):
        x, y = xy; line = ''
        for word in s.split():
            t = (line + ' ' + word).strip()
            if self.w(t, f) > width:
                self.text((x, y), line, f, c, a); y += f.size * lh; line = word
            else:
                line = t
        if line:
            self.text((x, y), line, f, c, a); y += f.size * lh
        return y
    def compose(self, base):
        out = base.convert('RGBA')
        out.alpha_composite(self.sh.filter(ImageFilter.GaussianBlur(1.5)))
        out.alpha_composite(self.im)
        return out.convert('RGB')


def backdrop():
    # the game behind the pack: blurred and darkened, a little darker at the left where the text sits
    b = Image.open(P + 'Compare/shot_trees_4800.png').convert('RGB').resize((1920, 1080)).filter(ImageFilter.GaussianBlur(18))
    dark = Image.new('RGBA', b.size, (8, 12, 20, 0)); dd = ImageDraw.Draw(dark)
    for x in range(1920):
        dd.line([(x, 0), (x, 1080)], fill=(8, 12, 20, int(255 * (0.78 - 0.18 * x / 1920))))
    out = b.convert('RGBA'); out.alpha_composite(dark)
    return out.convert('RGB')


def weight_meter(L, x, y, w):
    cap = F('Medium', 11)
    L.text((x, y - 96), 'PACK', cap, WHITE, 0.72, track=2.4)
    ax = L.text((x - 3, y - 18), '17.6', F('Light', 64), WHITE)
    L.text((ax + 6, y - 18), '/ 26 kg', F('Light', 24), WHITE, 0.72)
    L.rect((x, y, x + w, y + 2), WHITE, 0.3)
    L.rect((x, y, x + w * 17.6 / 26, y + 2), WHITE, 0.95)
    L.rect((x + w * 14 / 26, y - 5, x + w * 14 / 26 + 1, y + 7), WHITE, 0.6)
    L.text((x, y + 24), 'heavy: every step costs more above 14 kg', F('Regular', 14), WHITE, 0.72)


def keys(L, W, H):
    f = F('Regular', 14); x = W - 64
    for k, t in (('Tab', 'close'), ('Enter', 'use'), ('W  S', 'choose')):
        tw = L.w(t, f); L.text((x, H - 56), t, f, WHITE, 0.72, anchor='rs'); x -= tw + 10
        kw = L.w(k, F('Medium', 12)) + 14
        L.d.rounded_rectangle((x - kw, H - 74, x, H - 50), 4, outline=WHITE + (110,), width=1)
        L.text((x - kw / 2, H - 56), k, F('Medium', 12), WHITE, 0.9, anchor='ms'); x -= kw + 26


def detail(L, x, y, it, width, big=True):
    rid, name, cnt, wt, kind, desc, stats, act = it
    if big:
        r = Image.open(P + f'ItemIcons/render/{rid}.png').convert('RGBA')
        s = 300 / max(r.size); r = r.resize((int(r.width * s), int(r.height * s)), Image.LANCZOS)
        L.paste(r, (x + (width - r.width) / 2, y)); y += 330
    L.text((x, y), name, F('Light', 40), WHITE)
    y += 34
    sub = '  \u00b7  '.join(t for t in ((f'{cnt} left' if kind == 'food' and cnt else cnt), stats, wt if kind == 'food' else '') if t)
    if sub:
        L.text((x, y), sub, F('Regular', 16), WHITE, 0.72); y += 34
    y = L.wrap((x, y + 6), desc, F('Regular', 18), width, WHITE, 0.92)
    if act:
        L.text((x, y + 18), act, F('Medium', 16), RED)


# ---------------------------------------------------------------- 1: gear list
def proposal1():
    base = backdrop(); W, H = base.size; L = Layer(base.size); M = 96
    weight_meter(L, M, 200, 420)
    cap = F('Medium', 11)
    y = 300; sel = 0
    for i, it in enumerate(ITEMS):
        if i == 3:
            L.text((M, y + 26), 'GEAR', cap, WHITE, 0.55, track=2.4); y += 44
        elif i == 0:
            L.text((M, y + 6), 'SUPPLIES', cap, WHITE, 0.55, track=2.4); y += 24
        rid, name, cnt, wt, kind, *_ = it
        a = 1.0 if i == sel else 0.78
        if i == sel:
            L.rect((M - 20, y + 6, M - 18, y + 44), WHITE, 0.95)
            L.rect((M - 16, y + 4, M + 420, y + 46), WHITE, 0.06)
        empty = kind == 'food' and cnt.startswith('0')
        ic = tint(icon(rid, 26), RED if empty else WHITE, a); L.paste(ic, (M, y + 12))
        L.text((M + 42, y + 33), name, F('Regular', 19), WHITE, a)
        if cnt:
            L.text((M + 300, y + 33), cnt, F('Light', 22), RED if empty else WHITE, a, anchor='rs')
        L.text((M + 416, y + 33), wt if kind == 'food' else '', F('Regular', 13), WHITE, 0.55, anchor='rs')
        y += 50
    detail(L, 1180, 150, ITEMS[sel], 560)
    keys(L, W, H)
    return L.compose(base)


# ---------------------------------------------------------------- 2: gear grid
def proposal2():
    base = backdrop(); W, H = base.size; L = Layer(base.size); M = 96
    weight_meter(L, M, 200, 420)
    tile, gap, x0, y0 = 196, 16, M, 300; sel = 0
    glass = Image.new('RGBA', base.size, (0, 0, 0, 0)); dg = ImageDraw.Draw(glass)
    for i, it in enumerate(ITEMS):
        cx, cy = x0 + (i % 3) * (tile + gap), y0 + (i // 3) * (tile + gap)
        dg.rounded_rectangle((cx, cy, cx + tile, cy + tile), 10, fill=(255, 255, 255, 18 if i != sel else 34))
    base = Image.alpha_composite(base.convert('RGBA'), glass).convert('RGB')
    for i, it in enumerate(ITEMS):
        rid, name, cnt, wt, kind, *_ = it
        cx, cy = x0 + (i % 3) * (tile + gap), y0 + (i // 3) * (tile + gap)
        r = Image.open(P + f'ItemIcons/render/{rid}.png').convert('RGBA')
        s = 118 / max(r.size); r = r.resize((int(r.width * s), int(r.height * s)), Image.LANCZOS)
        empty = kind == 'food' and cnt.startswith('0')
        if empty:
            r.putalpha(r.split()[3].point(lambda v: int(v * 0.25)))
        L.paste(r, (cx + (tile - r.width) / 2, cy + 22 + (118 - r.height) / 2))
        L.text((cx + 14, cy + tile - 16), name, F('Regular', 14), WHITE, 0.9 if i == sel else 0.7)
        if cnt:
            L.text((cx + tile - 12, cy + 30), cnt, F('Light', 22), RED if empty else WHITE, 1.0, anchor='rs')
        if i == sel:
            L.d.rounded_rectangle((cx, cy, cx + tile, cy + tile), 10, outline=WHITE + (235,), width=2)
    detail(L, 1180, 300, ITEMS[sel], 560, big=False)
    keys(L, W, H)
    return L.compose(base)


if __name__ == '__main__':
    a, b = proposal1(), proposal2()
    a.save(P + 'Review/pack_proposal_1.png'); b.save(P + 'Review/pack_proposal_2.png')
    s = Image.new('RGB', (1920, 1080), (10, 12, 16)); s.paste(a.resize((960, 540)), (0, 0)); s.paste(b.resize((960, 540)), (960, 0))
    s.paste(a.crop((60, 140, 1020, 680)), (0, 540)); s.paste(b.crop((60, 140, 1020, 680)), (960, 540))
    s.save(P + 'Review/pack_proposals.png'); print('ok')
