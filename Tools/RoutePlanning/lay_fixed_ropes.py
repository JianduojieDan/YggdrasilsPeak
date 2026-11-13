# Runs inside the editor (Tools/RoutePlanning/uepy), after place_ropes (which gives each AFixedRope its route line).
# Lays every fixed rope the way real Everest fixed lines lie (2026-11-13):
#   - anchors every 10-25 m along the line, and wherever the line turns (the rope runs straight from anchor to anchor);
#     snow stakes on snow, ice screws on the Khumbu Icefall and the Lhotse Face, rock pegs on the Yellow Band and the
#     Geneva Spur (by altitude) and on steep ground near the summit;
#   - between anchors the rope is pulled taut over the ground's bumps (upper convex hull of the ground profile), sags
#     1.2% of each free span, and otherwise lies on the snow (half its thickness pressed in);
#   - about a third of the spans have an older, faded rope of an earlier season beside them, half buried, vanishing
#     under the snow in stretches.
# Ground = Tools/Placement/ground_lib (detail tiles, else landscape). Writes LayPoints / OldLayPoints / anchors and
# sets RopePoints (the clip-in line) to the laid rope; the actor (C++ AFixedRope) draws it. Re-run after terrain edits.
import math, random
import unreal
exec(open('F:/UE_project/YggdrasilsPeak/Tools/Placement/ground_lib.py').read())

STEP = 50.0          # cm between lay points
R = 0.7              # rope radius, cm
OLD_R = 0.55
rng = random.Random(1953)

def elev(z):
    return 6326.3 + (z - 129770.0) / 100.0

# where the rope rests in each anchor's carabiner, anchor-local cm (from Tools/Blender/rope_anchors: Blender -Y = UE +Y)
def attach_of(sm, blender_rest):
    b = sm.get_bounding_box()
    x, y, z = blender_rest
    flipped = b.max.y > -b.min.y            # the picket's carabiner side is the long side in y
    return (x, -y if flipped else y, z)
DIR = '/Game/Environment/FixedRope/'
PICKET = attach_of(unreal.load_asset(DIR + 'SM_RopeAnchor_Picket'), (0.84, -5.0, 0.1))
SCREW = (12.93, 0.0, 1.55)
PITON = (11.73, 0.0, 1.75)
print('picket attach', PICKET)

def kind_at(z, slope):
    e = elev(z)
    if 5400 < e < 5950 or 6900 < e < 7350: return 'screw'           # Khumbu Icefall, Lhotse Face (hard ice)
    if 7350 <= e < 7650 or 7750 < e < 7960: return 'piton'          # Yellow Band, Geneva Spur (rock)
    if e > 8600 and slope > 32: return 'piton'                      # rock steps near the summit
    if slope > 40: return 'screw'
    return 'picket'

def horiz(a, b):
    return math.hypot(b[0] - a[0], b[1] - a[1])

def chord_dev(path, i, j):
    ax, ay = path[i][0], path[i][1]; bx, by = path[j][0], path[j][1]
    L = math.hypot(bx - ax, by - ay) or 1.0
    return max(abs((bx - ax) * (ay - path[k][1]) - (ax - path[k][0]) * (by - ay)) / L for k in range(i + 1, j)) if j > i + 1 else 0.0

def pick_anchors(path):
    idx = [0]
    i = 0
    while i < len(path) - 1:
        e = elev(path[i][2])
        target = rng.uniform(10, 16) * 100 if (5400 < e < 5950 or 6900 < e < 7960) else rng.uniform(14, 25) * 100
        j, run = i + 1, 0.0
        while j < len(path):
            run += horiz(path[j - 1], path[j])
            if run >= target or (run > 600 and chord_dev(path, i, j) > 120):
                break
            j += 1
        j = min(j, len(path) - 1)
        if chord_dev(path, i, j) > 120 and j - 1 > i:
            j -= 1
        idx.append(j); i = j
    return idx

def upper_hull(pts):
    h = []
    for p in pts:
        while len(h) >= 2 and (h[-1][0] - h[-2][0]) * (p[1] - h[-2][1]) - (h[-1][1] - h[-2][1]) * (p[0] - h[-2][0]) >= 0:
            h.pop()
        h.append(p)
    return h

def anchor_transform(kind, P, d):
    """Anchor whose carabiner holds the rope at P (x, y), rope direction d (unit, horizontal).
    Returns (unreal.Transform, rope point z) - the rope rests in the carabiner."""
    if kind == 'picket':
        yaw = math.degrees(math.atan2(-d[0], d[1]))
        rot = unreal.Rotator(roll=rng.uniform(-4, 4), pitch=rng.uniform(-4, 4), yaw=yaw)
        X = rot.get_forward_vector(); Y = rot.get_right_vector(); Z = rot.get_up_vector()
        att = PICKET
        lx = P[0] - (X.x * att[0] + Y.x * att[1]); ly = P[1] - (X.y * att[0] + Y.y * att[1])
        g = ground(lx, ly)
        lz = (g[0] if g else P[2]) - 1.0
        rope_z = lz + X.z * att[0] + Y.z * att[1] + Z.z * att[2] + R
        return unreal.Transform(unreal.Vector(lx, ly, lz), rot, unreal.Vector(1, 1, 1)), rope_z
    att = SCREW if kind == 'screw' else PITON
    g0 = ground(P[0], P[1])
    # ground normal from 4 traces around
    gs = [ground(P[0] + dx, P[1] + dy) for dx, dy in ((40, 0), (-40, 0), (0, 40), (0, -40))]
    if all(gs):
        b = (gs[0][0] - gs[1][0]) / 80.0; c = (gs[2][0] - gs[3][0]) / 80.0
        ln = math.sqrt(b * b + c * c + 1); n = (-b / ln, -c / ln, 1 / ln)
    else:
        n = (0.0, 0.0, 1.0)
    dv = (d[0], d[1], 0.0)
    dn = dv[0] * n[0] + dv[1] * n[1]
    Yv = (dv[0] - dn * n[0], dv[1] - dn * n[1], -dn * n[2])
    yl = math.sqrt(sum(v * v for v in Yv)) or 1.0
    Yv = tuple(v / yl for v in Yv)
    if rng.random() < 0.5:                   # the carabiner lies to either side of the rope
        Yv = tuple(-v for v in Yv)
    rot = unreal.MathLibrary.make_rot_from_yz(unreal.Vector(*Yv), unreal.Vector(*n))
    X = rot.get_forward_vector(); Y = rot.get_right_vector(); Z = rot.get_up_vector()
    lx = P[0] - (X.x * att[0] + Y.x * att[1]); ly = P[1] - (X.y * att[0] + Y.y * att[1])
    g = ground(lx, ly)
    lz = (g[0] if g else (g0[0] if g0 else P[2])) - 0.3
    rope_z = lz + X.z * att[0] + Y.z * att[1] + Z.z * att[2] + R
    return unreal.Transform(unreal.Vector(lx, ly, lz), rot, unreal.Vector(1, 1, 1)), rope_z

def vnoise(t, seed):
    i = math.floor(t); f = t - i; f = f * f * (3 - 2 * f)
    h = lambda k: (math.sin((k + seed * 17.13) * 127.1) * 43758.5453) % 1.0
    return h(i) * (1 - f) + h(i + 1) * f

ropes = [a for a in acts if a.get_class().get_name() == 'FixedRope']
total = {'lay': 0, 'old': 0, 'picket': 0, 'screw': 0, 'piton': 0}
lifted = []
for rope in sorted(ropes, key=lambda a: a.get_actor_label()):
    src = rope.get_editor_property('rope_points')
    path = [(p.x, p.y, p.z) for p in src]
    # keep only the route line's first lay source: if already laid, rebuild from the coarse line kept in RopePoints?
    # (RopePoints becomes the dense lay below; thin it back to ~4 m so re-runs work from the same kind of line)
    if len(path) > 2 and horiz(path[0], path[1]) < 100:
        path = path[::8]
    ai = pick_anchors(path)
    anchors = {'picket': [], 'screw': [], 'piton': []}
    lay, old = [], []
    ends = []
    for a_i in ai:
        p = path[a_i]
        prev = path[max(0, a_i - 1)]; nxt = path[min(len(path) - 1, a_i + 1)]
        dx, dy = nxt[0] - prev[0], nxt[1] - prev[1]; dl = math.hypot(dx, dy) or 1.0
        g = ground(p[0], p[1])
        gz = g[0] if g else p[2]
        slope = math.degrees(math.acos(max(-1.0, min(1.0, g[1])))) if g else 0.0
        kind = kind_at(gz, slope)
        tr, rz = anchor_transform(kind, (p[0], p[1], gz), (dx / dl, dy / dl))
        anchors[kind].append(tr)
        ends.append((p[0], p[1], rz))
    for k in range(len(ends) - 1):
        A, B = ends[k], ends[k + 1]
        L = horiz(A, B)
        n = max(2, int(math.ceil(L / STEP)))
        xs = [(A[0] + (B[0] - A[0]) * t / n, A[1] + (B[1] - A[1]) * t / n) for t in range(n + 1)]
        gz = []
        for x, y in xs:
            g = ground(x, y)
            gz.append(g[0] if g else None)
        for t in range(n + 1):                     # fill holes (outside every ground part) by interpolation
            if gz[t] is None:
                gz[t] = A[2] + (B[2] - A[2]) * t / n
        prof = [(0.0, A[2])] + [(L * t / n, gz[t] + R * 0.6) for t in range(1, n)] + [(L, B[2])]
        hull = upper_hull(prof)
        out = []
        hi = 0
        for t in range(n + 1):
            s = L * t / n
            while hi < len(hull) - 2 and s > hull[hi + 1][0]:
                hi += 1
            s0, z0 = hull[hi]; s1, z1 = hull[hi + 1]
            u = (s - s0) / (s1 - s0) if s1 > s0 else 0.0
            z = z0 + (z1 - z0) * u - 4 * 0.012 * (s1 - s0) * u * (1 - u)
            if 0 < t < n:
                z = max(z, gz[t] + R * 0.3)
                lifted.append(z - gz[t])
            out.append((xs[t][0], xs[t][1], z))
        lay += out if k == 0 else out[1:]
        # an old rope of an earlier season beside about a third of the spans
        if rng.random() < 0.35 and L > 400:
            side = rng.choice((-1, 1)) * rng.uniform(25, 60)
            px, py = -(B[1] - A[1]) / L, (B[0] - A[0]) / L
            seed = rng.random() * 100
            run = []
            for t in range(n + 1):
                s = L * t / n
                wob = side + 15 * math.sin(s / 260.0 + seed)
                x, y = xs[t][0] + px * wob, xs[t][1] + py * wob
                vis = vnoise(s / 300.0, seed)
                g = ground(x, y)
                if not g or vis < 0.42:
                    if len(run) > 1: old += run + [None]
                    run = []
                    continue
                dive = max(0.0, 1.0 - (vis - 0.42) / 0.1)       # dives into the snow where it disappears
                run.append((x, y, g[0] + OLD_R * (0.15 - 1.6 * dive)))
            if len(run) > 1: old += run + [None]
    # gaps in the old rope: a far jump between stretches (the actor skips pieces longer than MaxPieceCm)
    old_pts = []
    for p in old:
        if p is None:
            if old_pts: old_pts.append(unreal.Vector(old_pts[-1].x + 1e5, old_pts[-1].y, old_pts[-1].z))
            continue
        old_pts.append(unreal.Vector(*p))
    lay_v = [unreal.Vector(*p) for p in lay]
    rope.set_editor_property('lay_points', lay_v)
    rope.set_editor_property('old_lay_points', old_pts)
    rope.set_editor_property('picket_anchors', anchors['picket'])
    rope.set_editor_property('screw_anchors', anchors['screw'])
    rope.set_editor_property('piton_anchors', anchors['piton'])
    rope.set_editor_property('rope_points', lay_v)        # each set re-runs the actor's construction (redraw)
    total['lay'] += len(lay_v); total['old'] += len(old_pts)
    for k in anchors: total[k] += len(anchors[k])
    print('%s: %d lay points (%.1f km), anchors stake %d / screw %d / peg %d, old rope points %d' % (
        rope.get_actor_label(), len(lay_v), len(lay_v) * STEP / 1e5, len(anchors['picket']), len(anchors['screw']), len(anchors['piton']), len(old_pts)))
lifted.sort()
print('TOTAL', total, 'rope above snow: median %.1f cm, p90 %.1f, p99 %.1f, max %.0f' % (lifted[len(lifted) // 2], lifted[int(len(lifted) * .9)], lifted[int(len(lifted) * .99)], lifted[-1]))
print('saved', unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level())
