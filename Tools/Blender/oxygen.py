# Bottled oxygen kit: Poisk-style 4 L cylinder with valve + regulator, face mask with reservoir bag + hose.
# Bottle origin = base centre (stands upright). Mask origin = where it sits on the face (+X = forward).
import sys, os, math
sys.path.append(os.path.dirname(__file__))
from ygg_blend import *

reset()
ORANGE = mat("M_O2_Bottle", (0.85, 0.30, 0.05), metal=0.7, rough=0.35)
BRASS = mat("M_O2_Brass", (0.80, 0.62, 0.30), metal=1.0, rough=0.3)
BLACK = mat("M_O2_Rubber", (0.03, 0.03, 0.035), rough=0.75)
GREY = mat("M_O2_Plastic", (0.25, 0.27, 0.30), rough=0.5)
GLASS = mat("M_O2_Gauge", (0.85, 0.87, 0.85), rough=0.15)
BAG = mat("M_O2_Bag", (0.10, 0.25, 0.65), rough=0.6)

R, H = 0.058, 0.50
b = [cyl("body", R, H - 2 * R, (0, 0, H / 2), seg=20, material=ORANGE),
     sphere("bot", R, (0, 0, R), seg=20, rings=10, scale=(1, 1, 0.6), material=ORANGE),
     sphere("top", R, (0, 0, H - R), seg=20, rings=10, scale=(1, 1, 0.9), material=ORANGE),
     cyl("neck", 0.014, 0.04, (0, 0, H - R * 0.1 + 0.02), seg=10, material=BRASS),
     cyl("valve", 0.022, 0.05, (0, 0, H + 0.035), seg=12, material=BRASS),
     cyl("reg", 0.028, 0.06, (0, 0, H + 0.085), seg=14, material=GREY),
     cyl("knob", 0.02, 0.012, (0, 0, H + 0.12), seg=14, material=BLACK),
     cyl("gauge", 0.022, 0.015, (0.035, 0, H + 0.085), (0, math.pi / 2, 0), seg=16, material=GLASS),
     cyl("outlet", 0.006, 0.03, (0, -0.04, H + 0.08), (math.pi / 2, 0, 0), seg=8, material=BRASS)]
for z in (0.12, 0.40):  # two label bands
    b.append(cyl(f"band{z}", R + 0.001, 0.03, (0, 0, z), seg=20, material=GREY))
bottle = join("SM_O2_Bottle", b)
export(bottle, "O2_Bottle")

# mask: rounded shell over nose/mouth, exhale valve, reservoir bag below, hose
m = [sphere("shell", 0.065, (0, 0, 0), seg=20, rings=10, scale=(0.75, 1.0, 1.2), material=GREY),
     sphere("seal", 0.066, (-0.01, 0, 0), seg=20, rings=10, scale=(0.55, 1.02, 1.22), material=BLACK),
     cyl("valve", 0.022, 0.025, (0.05, 0, -0.01), (0, math.pi / 2, 0), seg=14, material=BLACK),
     cyl("valvecap", 0.026, 0.008, (0.064, 0, -0.01), (0, math.pi / 2, 0), seg=14, material=GREY),
     sphere("bag", 0.045, (0.02, 0, -0.14), seg=14, rings=8, scale=(0.8, 1.2, 1.5), material=BAG),
     cyl("bagneck", 0.012, 0.04, (0.03, 0, -0.08), seg=8, material=BLACK)]
for side in (-1, 1):  # head straps
    m.append(box(f"strap{side}", (0.10, 0.004, 0.02), (-0.06, side * 0.062, 0.02), (0, 0, side * 0.25), material=BLACK))
hose = [(0.02, 0, -0.2)] + [(0.02 - 0.03 * t, -0.03 * t, -0.2 - 0.35 * t) for t in (0.3, 0.6, 1.0)]
m.append(tube_along("hose", hose, 0.007, material=BLACK))
mask = join("SM_O2_Mask", m)
export(mask, "O2_Mask")

mask.location = (0.3, 0, 0.4)
preview("oxygen", [bottle, mask])
save_blend("oxygen")
