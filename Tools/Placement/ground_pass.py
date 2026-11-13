# Runs inside the editor (Tools/RoutePlanning/uepy): ONE pass over everything placed on the ground.
#   APPLY = False (default): measure only, print a short summary per kind, write Saved/Placement/ground_audit.csv.
#   APPLY = True: also seat what floats (or is buried too deep), by rules per kind, save the level, and log every move
#                 to Saved/Placement/ground_moves.csv (old and new transform, so any of it can be put back).
# Run it after any terrain edit or scatter/placement script. Ropes and trees are only reported (they are rebuilt by
# their own tools: Tools/RoutePlanning/place_ropes, Tools/Art/plant_forest).
#
# Rules (cm; "height" = the object's own height):
#   small stones (support): sunk 20% of their thickness at the deepest point; moved if under 8%
#   ice blocks (support):   sunk 30%; moved if under 12%
#   rocks (exact):          deepest point 12% of height under the ground; moved if under 6%
#   upright props (poles, pickets, landmarks: taller than 1.5x their footprint): 15% of height in the snow, max 25 cm
#   other props (tents, bottles, boxes): tilted to the slope (if > 4 deg and air under them), then lowered until
#                           three quarters of their underside touches. Nothing is ever raised; route landmarks are
#                           only reported (their signs and flag lines hang in the air by design).
import csv, math, os, time, collections
import unreal
exec(open('F:/UE_project/YggdrasilsPeak/Tools/Placement/ground_lib.py').read())

APPLY = globals().pop('APPLY', False)        # pop: the editor keeps globals between runs, so it must be set every time
OUT = 'F:/UE_project/YggdrasilsPeak/Saved/Placement'
os.makedirs(OUT, exist_ok=True)
T0 = time.time()

rows, moves = [], []
summary = collections.OrderedDict()
def note(kind, m, dz=0.0, tilted=False):
    s = summary.setdefault(kind, {'ms': [], 'moved': [], 'tilted': 0})
    s['ms'].append(m)
    if abs(dz) > 0.5: s['moved'].append(dz)
    if tilted: s['tilted'] += 1

def fix_support(m, ice):
    target, trigger = (0.30, 0.12) if ice else (0.20, 0.08)
    if m['pen'] >= trigger * m['thick']:
        return 0.0
    return (target * m['thick'] - m['pen']) / max(m['nz'], 0.3)

def fix_rock(m):
    if m['pen'] >= 0.06 * m['height']:
        return 0.0
    return 0.12 * m['height'] - m['pen']

def fix_upright(m):
    # only ever lowered: a deep pen can be a pole or a buried rock meant to stand that way (2026-11-13: raising sent a
    # story rock 21 m up)
    target = min(25.0, 0.15 * m['height'])
    return target - m['pen'] if m['pen'] < 0.5 * target else 0.0

def fix_prop(m):
    down = m['p75'] + 2.0
    if down > 0.5 and m['lift'] > max(2.0, 0.02 * m['size']):
        return down
    return 0.0                                  # never raised (see fix_upright)

for a in acts:
    if skip_actor(a):
        continue
    lab = a.get_actor_label(); cls = a.get_class().get_name(); folder = str(a.get_folder_path())
    if cls == 'FixedRope':
        pts = a.get_editor_property('rope_points')
        for i in range(len(pts) - 1):
            p0, p1 = pts[i], pts[i + 1]
            n = max(1, int(math.dist((p0.x, p0.y, p0.z), (p1.x, p1.y, p1.z)) / 100))
            for k in range(n):
                t = k / n
                x, y, z = p0.x + (p1.x - p0.x) * t, p0.y + (p1.y - p0.y) * t, p0.z + (p1.z - p0.z) * t
                g = ground(x, y)
                if g:
                    rows.append(('rope', lab, i, k, round(x), round(y), round(z), round(z - g[0], 1), '', '', '', g[2]))
                    note('rope', {'h': z - g[0]})
        continue
    if cls == 'ForestStreamer':
        for si, fs in enumerate(a.get_editor_property('sets')):
            for ii, tr in enumerate(fs.get_editor_property('instances')):
                L = tr.translation; s = tr.scale3d.x
                gs = [g for g in (ground(L.x + dx, L.y + dy) for dx, dy in ((0, 0), (60 * s, 0), (-60 * s, 0), (0, 60 * s), (0, -60 * s))) if g]
                if gs:
                    gap = L.z - min(g[0] for g in gs)
                    rows.append(('tree', lab, si, ii, round(L.x), round(L.y), round(L.z), round(gap, 1), '', round(s, 2), '', gs[0][2]))
                    note('tree', {'gap': gap})
        continue
    comps = [c for c in a.get_components_by_class(unreal.StaticMeshComponent) if c.static_mesh and c.is_visible()]
    isms = [c for c in comps if isinstance(c, unreal.InstancedStaticMeshComponent)]
    singles = [c for c in comps if not isinstance(c, unreal.InstancedStaticMeshComponent)]
    for c in isms:
        sm = c.static_mesh
        cnt = c.get_instance_count()
        ice = folder.startswith('Environment/Icefall')
        big = cnt >= 20000 or ice
        kind = ('ice' if ice else 'rock ' + sm.get_name()[:16] + (' (small)' if big else ''))
        trs = [c.get_instance_transform(i, True) for i in range(cnt)]
        changed = False
        for i, tr in enumerate(trs):
            q, s, L = tr_parts(tr)
            m = measure_support(sm, q, s[0], L) if big else measure_exact([(sm, (q, s, L))])
            if not m: continue
            dz = (fix_support(m, ice) if big else fix_rock(m)) if APPLY else 0.0
            note(kind, m, dz)
            rows.append((kind, lab, c.get_name(), i, round(L[0]), round(L[1]), round(L[2]), round(m['pen'], 1),
                         round(m.get('thick', m.get('height', 0)), 1), round(m.get('air', m.get('frac', 0)), 2), round(m['slope'], 1), m['src']))
            if dz:
                moves.append((lab, c.get_name(), i, L[0], L[1], L[2], q, L[2] - dz, q))
                tr.translation = unreal.Vector(L[0], L[1], L[2] - dz)
                changed = True
        if changed:
            c.batch_update_instances_transforms(0, trs, True, True, True)
    if not singles or isms:
        continue
    # one actor = one object (a landmark pole with its signs, a pickup with its bottle, a tent)
    parts = [(c.static_mesh, tr_parts(c.get_world_transform())) for c in singles]
    m = measure_exact(parts)
    if not m: continue
    upright = m['height'] > 1.5 * m['foot']
    kind = ('upright ' if upright else 'prop ') + (folder.split('/')[1] if folder.count('/') else folder)[:20]
    atr = a.get_actor_transform()
    q0, _, P = tr_parts(atr)
    tilted = False
    dz = 0.0
    if APPLY:
        newq = q0
        tags = [str(t) for t in a.tags]
        if not upright and 'GroundTilted' not in tags and m['slope'] > 4.0 and m['lift'] > max(2.0, 0.02 * m['size']):
            # world Z -> ground normal, once per actor (tagged GroundTilted: applying it again would double the tilt;
            # the prop's own up axis can't be used - a lying bottle's local Z is sideways)
            R = q_from_to((0.0, 0.0, 1.0), m['normal'])
            a.tags = list(a.tags) + [unreal.Name('GroundTilted')]
            newq = qmul(R, q0)
            parts2 = []
            for sm, (q, s, L) in parts:
                d = qrot(R, (L[0] - P[0], L[1] - P[1], L[2] - P[2]))
                parts2.append((sm, (qmul(R, q), s, (P[0] + d[0], P[1] + d[1], P[2] + d[2]))))
            m2 = measure_exact(parts2)
            if m2:
                m, tilted = m2, True
        dz = 0.0 if cls == 'RouteLandmark' else (fix_upright(m) if upright else fix_prop(m))   # landmarks: signs and flag lines hang in the air by design
        if dz or tilted:
            a.set_actor_location_and_rotation(unreal.Vector(P[0], P[1], P[2] - dz), unreal.Quat(*newq).rotator(), False, True)
            moves.append((lab, '', -1, P[0], P[1], P[2], q0, P[2] - dz, newq))
    note(kind, m, dz, tilted)
    rows.append((kind, lab, '', -1, round(P[0]), round(P[1]), round(P[2]), round(m['pen'], 1), round(m['height'], 1), round(m['air'], 2), round(m['slope'], 1), m['src']))

with open(OUT + '/ground_audit.csv', 'w', newline='') as f:
    w = csv.writer(f)
    w.writerow(['kind', 'actor', 'comp', 'index', 'x', 'y', 'z', 'pen|h|gap', 'height|thick', 'air|frac|scale', 'slope', 'src(1=tile)'])
    w.writerows(rows)
if moves:
    with open(OUT + '/ground_moves.csv', 'a', newline='') as f:
        csv.writer(f).writerows([(time.strftime('%Y-%m-%d %H:%M'),) + tuple(mv) for mv in moves])

def pct(xs, p):
    xs = sorted(xs); return xs[min(len(xs) - 1, int(p * len(xs)))] if xs else 0
print('%s: %d tiles, %d landscape parts, %d traces, %.0f s' % ('APPLY' if APPLY else 'audit', NTILES, NLAND, TRACES[0], time.time() - T0))
for kind, s in summary.items():
    ms = s['ms']
    mv = ' | moved %d (median %.0f cm, max %.0f), tilted %d' % (len(s['moved']), pct([abs(v) for v in s['moved']], .5), max([abs(v) for v in s['moved']] or [0]), s['tilted']) if APPLY else ''
    if kind == 'rope':
        hs = [m['h'] for m in ms]
        print('%-30s n %6d  above snow: median %.0f cm, p90 %.0f, max %.0f; >1 m: %d, buried: %d' % (kind, len(hs), pct(hs, .5), pct(hs, .9), max(hs), sum(h > 100 for h in hs), sum(h < 0 for h in hs)))
    elif kind == 'tree':
        gs = [m['gap'] for m in ms]
        print('%-30s n %6d  air under trunk: median %.0f cm, p90 %.0f, max %.0f; >10 cm: %d' % (kind, len(gs), pct(gs, .5), pct(gs, .9), max(gs), sum(g > 10 for g in gs)))
    elif 'thick' in ms[0]:
        fr = [m['frac'] for m in ms]
        print('%-30s n %6d  sunk/thickness median %.2f; not touching %d, under 8%% %d%s' % (kind, len(fr), pct(fr, .5), sum(f <= 0 for f in fr), sum(f < 0.08 for f in fr), mv))
    else:
        fl = [m for m in ms if m['pen'] < 0]
        print('%-30s n %6d  not touching %d (worst %.0f cm), barely in %d, air under >=25%%: %d%s' % (kind, len(ms), len(fl), max([-m['pen'] for m in fl] or [0]),
              sum(1 for m in ms if 0 <= m['pen'] < 0.06 * m['height']), sum(1 for m in ms if m['air'] >= 0.25), mv))
if APPLY:
    print('saved level', unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level())
