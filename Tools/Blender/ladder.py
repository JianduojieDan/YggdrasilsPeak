# Khumbu Icefall aluminium ladders: one 3 m section, a 3-section lashed span, and a snow picket.
# Origin = centre of the span at rail bottom, ladder runs along +X, so placing it at a crevasse
# centre and yawing it to the crossing direction is enough.
import sys, os, math
sys.path.append(os.path.dirname(__file__))
from ygg_blend import *

reset()
ALU = mat("M_Ladder_Aluminium", (0.62, 0.64, 0.66), metal=1.0, rough=0.35)
ROPE = mat("M_Ladder_Rope", (0.75, 0.12, 0.08), rough=0.9)
RED = mat("M_Ladder_Picket", (0.70, 0.08, 0.06), metal=0.6, rough=0.45)

L, W, RH, RT = 3.0, 0.42, 0.075, 0.025   # section length, width, rail height/thickness
PITCH = 0.30


def section(x0, z0=0.0, tag=""):
    parts = []
    for side in (-1, 1):
        y = side * W / 2
        # rail = web + two flanges (C-channel), open side facing in
        parts.append(box(f"web{tag}{side}", (L, 0.004, RH), (x0, y + side * RT / 2, z0 + RH / 2), material=ALU))
        for fz in (0.002, RH - 0.002):
            parts.append(box(f"fl{tag}{side}{fz}", (L, RT, 0.004), (x0, y, z0 + fz), material=ALU))
    n = int(L / PITCH)
    for i in range(n):
        x = x0 - L / 2 + PITCH / 2 + i * PITCH
        parts.append(cyl(f"rung{tag}{i}", 0.015, W, (x, 0, z0 + RH / 2), (math.pi / 2, 0, 0), seg=8, material=ALU))
    return parts


def lashing(x, z0, tag):
    # rope figure wraps around both stacked rails at each overlap
    out = []
    for side in (-1, 1):
        y = side * W / 2
        for k in range(3):
            dx = (k - 1) * 0.05
            pts = []
            for a in range(13):
                t = a / 12 * 2 * math.pi
                pts.append((x + dx + 0.01 * math.sin(t * 0.5),
                            y + 0.034 * math.cos(t), z0 + RH + 0.09 * math.sin(t)))
            out.append(tube_along(f"lash{tag}{side}{k}", pts, 0.006, material=ROPE))
    return out


# single section
single = join("SM_IcefallLadder_Section", section(0))
export(single, "IcefallLadder_Section")

# 3 sections overlapping 0.5 m, alternate ones stacked on top (how they are really lashed)
OV = 0.5
step = L - OV
span = []
for i in range(3):
    x = (i - 1) * step
    span += section(x, z0=(RH if i == 1 else 0.0), tag=f"s{i}")
for j in range(2):
    span += lashing((j - 0.5) * step, 0.0, f"j{j}")
span3 = join("SM_IcefallLadder_Span3", span)
span3.location = (0, 0, 0)
export(span3, "IcefallLadder_Span3")

# aluminium T-section snow picket (anchor for the ladder hand lines)
pk = [box("pk_a", (0.03, 0.003, 0.9), (0, 0, 0.45), material=RED),
      box("pk_b", (0.003, 0.03, 0.9), (0, 0.015, 0.45), material=RED)]
for h in range(4):
    pk.append(cyl(f"pk_h{h}", 0.006, 0.004, (0, 0.016, 0.62 + h * 0.08), (math.pi / 2, 0, 0), seg=8, material=ALU))
picket = join("SM_SnowPicket", pk)
export(picket, "SnowPicket")

span3.location = (0, 0, 0); single.location = (0, 1.2, 0); picket.location = (2.0, 1.2, 0)
preview("ladder", [span3, single, picket])
save_blend("ladder")
