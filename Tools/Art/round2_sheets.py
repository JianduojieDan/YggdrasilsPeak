# Builds the round-2 review sheets (Saved/Review/round2_world.png, round2_ui.png) from the shots in Saved/Compare and
# Saved/Placement/shots, each panel labelled.
from PIL import Image, ImageDraw, ImageFont
import os
P = 'F:/UE_project/YggdrasilsPeak/Saved/'
os.makedirs(P + 'Review', exist_ok=True)
try:
    F = ImageFont.truetype('C:/Windows/Fonts/georgia.ttf', 26)
except Exception:
    F = ImageFont.load_default()

def panel(path, label, size=(800, 450), crop=None):
    im = Image.open(P + path).convert('RGB')
    if crop: im = im.crop(crop)
    im = im.resize(size)
    d = ImageDraw.Draw(im)
    d.rectangle((0, 0, size[0], 40), fill=(10, 14, 22))
    d.text((14, 6), label, font=F, fill=(238, 232, 220))
    return im

def sheet(panels, cols, out):
    w, h = panels[0].size
    rows = (len(panels) + cols - 1) // cols
    s = Image.new('RGB', (w * cols, h * rows), (10, 14, 22))
    for k, p in enumerate(panels):
        s.paste(p, ((k % cols) * w, (k // cols) * h))
    s.save(P + 'Review/' + out)
    print('saved', out, s.size)

sheet([
    panel('Placement/shots/D_o2_bottle.png', 'Before: O2 bottles floating at Base Camp'),
    panel('Placement/shots/v1_bc_o2.png', 'After: standing on the snow'),
    panel('Placement/shots/A_small_stone.png', 'Before: a boulder 1 m over the glacier ice'),
    panel('Placement/shots/v6_glacier_rock.png', 'After: sitting on the ice'),
    panel('Placement/shots/q2b_picket.png', 'New fixed rope: lying on the snow, snow stake + carabiner'),
    panel('Placement/shots/q4_piton.png', 'Rock peg (Yellow Band) and the rope through its carabiner'),
    panel('Compare/shot_trees_4800.png', 'Trees at 4,800 m: golden ginkgo groves, dead pines, spruce'),
    panel('Compare/shot_trees_valley.png', 'Trees near Dingboche: groves and lone trees, mixed sizes'),
    panel('Compare/realm_Jotunheim_before.png', 'Jotunheim planet, before the shader pass'),
    panel('Compare/realm_Jotunheim.png', 'After (same look, cheaper)'),
], 2, 'round2_world.png')

sheet([
    panel('Compare/shot_hud_watch.png', 'HUD "watch" (default): rings around altitude + O2, bottom left'),
    panel('Compare/shot_hud_strip.png', 'HUD "strip": one line at the bottom centre'),
    panel('Compare/shot_hud_watch.png', 'Watch, close up', crop=(0, 760, 560, 1075)),
    panel('Compare/shot_hud_strip.png', 'Strip, close up', crop=(600, 675, 1320, 1080)),
    panel('Compare/shot_pack2.png', 'Pack: flat lay, used-up oxygen leaves its chalk outline'),
    panel('Compare/shot_hud_quiet.png', 'HUD "Quiet": nothing while you are fine'),
    panel('Compare/shot_snowB_oldsettings.png', 'Snow with your old saved settings (Low, 50% - the bug)', crop=(700, 700, 1220, 990)),
    panel('Compare/shot_snowB_after.png', 'Snow now (High, DLSS Quality, crisper grain + glints)', crop=(700, 700, 1220, 990)),
], 2, 'round2_ui.png')
