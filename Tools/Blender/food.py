# Expedition food + water kit: foil ration pouch, gas canister, integrated stove/pot, thermos.
# Every origin = base centre so they sit on the ground/table.
import sys, os, math
sys.path.append(os.path.dirname(__file__))
from ygg_blend import *

reset()
FOIL = mat("M_Food_Foil", (0.55, 0.12, 0.10), metal=0.6, rough=0.4)
SILVER = mat("M_Food_Silver", (0.75, 0.75, 0.76), metal=1.0, rough=0.3)
BLUE = mat("M_Food_Canister", (0.10, 0.28, 0.60), metal=0.5, rough=0.4)
BLACK = mat("M_Food_Black", (0.04, 0.04, 0.045), rough=0.6)
ORANGE = mat("M_Food_Cozy", (0.90, 0.45, 0.05), rough=0.8)
STEEL = mat("M_Food_Steel", (0.55, 0.56, 0.58), metal=1.0, rough=0.25)

# ration pouch: pillow shape lying flat with crimped top seal and tear notch
p = [sphere("pouch", 0.1, (0, 0, 0.018), seg=12, rings=6, scale=(0.75, 1.0, 0.18), material=FOIL),
     box("seal", (0.15, 0.025, 0.003), (0, 0.095, 0.018), material=SILVER),
     box("label", (0.09, 0.08, 0.002), (0, -0.01, 0.036), material=SILVER)]
pouch = join("SM_Food_RationPouch", p)
export(pouch, "Food_RationPouch")

# 230 g isobutane canister
c = [cyl("can", 0.05, 0.075, (0, 0, 0.0375 + 0.005), seg=20, material=BLUE),
     cyl("base", 0.048, 0.01, (0, 0, 0.005), seg=20, material=SILVER),
     sphere("dome", 0.05, (0, 0, 0.08), seg=20, rings=8, scale=(1, 1, 0.3), material=SILVER),
     cyl("thread", 0.012, 0.012, (0, 0, 0.1), seg=10, material=SILVER)]
can = join("SM_Food_GasCanister", c)
export(can, "Food_GasCanister")

# integrated stove: canister + burner + insulated pot with lid
s = [cyl("can", 0.05, 0.075, (0, 0, 0.042), seg=20, material=BLUE),
     cyl("burner", 0.035, 0.03, (0, 0, 0.095), seg=16, material=BLACK),
     cyl("pot", 0.052, 0.17, (0, 0, 0.195), seg=20, material=STEEL),
     cyl("cozy", 0.054, 0.12, (0, 0, 0.19), seg=20, material=ORANGE),
     cyl("lid", 0.055, 0.015, (0, 0, 0.287), seg=20, material=BLACK),
     box("handle", (0.02, 0.06, 0.01), (0, 0.075, 0.2), material=BLACK)]
stove = join("SM_Food_Stove", s)
export(stove, "Food_Stove")

# thermos flask
t = [cyl("flask", 0.037, 0.24, (0, 0, 0.12), seg=18, material=STEEL),
     cyl("grip", 0.038, 0.08, (0, 0, 0.09), seg=18, material=BLACK),
     cyl("cup", 0.04, 0.06, (0, 0, 0.27), seg=18, material=BLACK)]
thermos = join("SM_Food_Thermos", t)
export(thermos, "Food_Thermos")

can.location = (0.2, 0, 0); stove.location = (0.4, 0, 0); thermos.location = (0.6, 0, 0)
preview("food", [pouch, can, stove, thermos])
save_blend("food")
