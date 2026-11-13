# Reads one route-check run (Saved/RouteCheck/<yyyyMMdd_HHmmss>/, written by the game's ygg.RouteCheck,
# Source/YggdrasilsPeak/Survival/YggRouteCheck.*) and writes into the same folder:
#   report.md      the summary; the worst floating and buried objects and the props a climber walks through (with
#                  BugItGo to go and look); holes and missing collision; ground too steep to stand on; stretches with no
#                  rope, flag, clue or checkpoint ahead; frame time along the route
#   sheet_NN.jpg   contact sheets of the pictures along the route: red = a ground problem, orange = no guidance ahead
#   python -I Tools/Route/analyze_route_check.py [run folder | latest]
# Standard library only; PIL (already used by Tools/Art and Tools/Monitor) for the sheets, skipped when missing.
import collections, csv, glob, math, os, statistics, sys

ROOT = 'F:/UE_project/YggdrasilsPeak/'
RUN_ROOTS = [ROOT + 'Saved/RouteCheck', os.path.join(os.environ.get('LOCALAPPDATA', ''), 'UnrealEngine', '5.8', 'Saved', 'RouteCheck')]
# ground problems at a point (the game also notes "on_object" and "far_from_route_z", which are only facts)
SERIOUS = {
    'no_ground': 'nothing to stand on under the point',
    'pawn_falls_through': "the climber's collision finds no ground where the eye sees it",
    'pawn_sinks': "the climber's collision is more than 15 cm below what is seen (a visible surface that is not solid)",
    'pawn_floats': "the climber's collision is more than 15 cm above what is seen (invisible collision)",
    'steep': 'the ground plane is steeper than the climber can walk',
    'capsule_blocked': 'the climber does not fit standing there (inside a rock or prop)',
    'tile_hole': 'a detail tile covers the point but its collision is missing there',
    'no_tile': 'no detail tile here: only the coarse landscape',
    'land_above_tile': 'the landscape pokes more than 20 cm through the detail tile',
}
SHORT = {'no_ground': 'NOGROUND', 'pawn_falls_through': 'FALL', 'pawn_sinks': 'SINK', 'pawn_floats': 'FLOATCOL', 'steep': 'STEEP',
         'capsule_blocked': 'STUCK', 'tile_hole': 'HOLE', 'no_tile': 'NOTILE', 'land_above_tile': 'LAND'}
TOP = 30


def num(v):
    try:
        return float(v)
    except (TypeError, ValueError):
        return float('nan')


def ok(v):
    return not math.isnan(v)


def load(path):
    if not os.path.exists(path):
        return []
    csv.field_size_limit(10 ** 9)
    with open(path, encoding='utf-8-sig', newline='', errors='replace') as f:
        return [{k: (v or '') for k, v in r.items() if k is not None} for r in csv.DictReader(f)]


def find_run(arg):
    if arg and arg != 'latest':
        if os.path.isdir(arg):
            return arg
        for r in RUN_ROOTS:
            if os.path.isdir(os.path.join(r, arg)):
                return os.path.join(r, arg)
        return None
    runs = [d for r in RUN_ROOTS for d in glob.glob(os.path.join(r, '*')) if os.path.isdir(d)]
    return max(runs, key=os.path.getmtime) if runs else None


def issues(p):
    return [i for i in p.get('issues', '').split(';') if i]


def serious(p):
    return [i for i in issues(p) if i in SERIOUS]


def stretch_bad(p):
    return num(p.get('seg_holes')) > 0 or num(p.get('seg_no_ground')) > 0


def where(p):
    return f"leg {p.get('leg')} {num(p.get('leg_dist_m')):.0f} m"


def code(s):
    return f'`{s}`' if s else ''


def table(head, rows):
    out = ['| ' + ' | '.join(head) + ' |', '|' + '|'.join('---' for _ in head) + '|']
    for r in rows:
        out.append('| ' + ' | '.join(str(c).replace('|', '/').replace('\n', ' ') for c in r) + ' |')
    return out


def pct(vals, p):
    s = sorted(vals)
    return s[min(len(s) - 1, int(len(s) * p))] if s else float('nan')


def sheets(run, pts):
    """Contact sheets of the pictures along the route; red frame = a ground problem, orange = no guidance ahead."""
    try:
        from PIL import Image, ImageDraw
    except ImportError:
        return [], 'PIL is not installed: no contact sheets.'
    items = [p for p in pts if p.get('shot')]
    W, H, LABEL, COLS, PER = 320, 180, 16, 6, 48
    names = []
    for s in range(0, len(items), PER):
        chunk = items[s:s + PER]
        rows = (len(chunk) + COLS - 1) // COLS
        sheet = Image.new('RGB', (COLS * W, rows * (H + LABEL)), (22, 22, 22))
        d = ImageDraw.Draw(sheet)
        for i, p in enumerate(chunk):
            x, y = (i % COLS) * W, (i // COLS) * (H + LABEL)
            try:
                sheet.paste(Image.open(os.path.join(run, p['shot'])).convert('RGB').resize((W, H)), (x, y))
            except OSError:
                d.text((x + 8, y + 8), 'picture missing', fill=(200, 200, 200))
            bad = serious(p) or stretch_bad(p)
            lost = p.get('guided') == '0'
            colour = (225, 60, 60) if bad else (235, 160, 40) if lost else (110, 110, 110)
            d.rectangle([x, y, x + W - 1, y + H + LABEL - 1], outline=colour, width=3 if (bad or lost) else 1)
            flags = [SHORT[i] for i in serious(p)]
            if stretch_bad(p):
                flags.append('HOLE-BEHIND')
            if num(p.get('floating')) > 0:
                flags.append(f"F{p['floating']}")
            if num(p.get('walk_into')) > 0:
                flags.append(f"W{p['walk_into']}")
            if lost:
                flags.append('NOGUIDE')
            label = f"{int(num(p['idx'])):03d} L{p['leg']} {num(p['leg_dist_m']):.0f}m " + ' '.join(flags)
            d.text((x + 4, y + H + 2), label[:56], fill=colour if (bad or lost) else (215, 215, 215))
        name = f'sheet_{s // PER + 1:02d}.jpg'
        sheet.save(os.path.join(run, name), quality=85)
        names.append(name)
    return names, ''


def main():
    arg = sys.argv[1] if len(sys.argv) > 1 else 'latest'
    run = find_run(arg)
    if not run:
        print('no route-check run found (Saved/RouteCheck/<time>/)')
        return 1
    pts = load(os.path.join(run, 'points.csv'))
    objs = load(os.path.join(run, 'objects.csv'))
    done = {}
    if os.path.exists(os.path.join(run, 'done.txt')):
        for line in open(os.path.join(run, 'done.txt'), encoding='utf-8', errors='replace'):
            k, _, v = line.strip().partition(' ')
            if k:
                done[k] = v
    head = []
    if os.path.exists(os.path.join(run, 'log.txt')):
        head = [l.rstrip() for l in open(os.path.join(run, 'log.txt'), encoding='utf-8', errors='replace')][:6]

    L = [f'# Route check {os.path.basename(run.rstrip("/").rstrip(chr(92)))}', '']
    L.append(f"Status: **{done.get('status', 'not finished (no done.txt)')}**, {done.get('minutes', '?')} min. "
             f"Made by `ygg.RouteCheck` (Source/YggdrasilsPeak/Survival/YggRouteCheck.cpp); read Tools/Route/README.md for what each check means.")
    L.append('')
    if head:
        L += ['```'] + head + ['```', '']
    if not pts:
        L.append('No points were recorded.')
        open(os.path.join(run, 'report.md'), 'w', encoding='utf-8').write('\n'.join(L) + '\n')
        print('\n'.join(L))
        return 0

    # ---- summary
    bad_pts = [p for p in pts if serious(p)]
    hole_pts = [p for p in pts if stretch_bad(p)]
    steep = [p for p in pts if p.get('walkable') == '0' and 'no_ground' not in issues(p)]
    unguided = [p for p in pts if p.get('guided') == '0']
    things = [o for o in objs if o.get('kind') != 'tree']
    trees = [o for o in objs if o.get('kind') == 'tree']
    floating = sorted([o for o in things if o.get('floating') == '1'], key=lambda o: -num(o.get('gap_cm')))
    buried = sorted([o for o in things if o.get('buried') == '1'], key=lambda o: -num(o.get('frac_under')))
    walk = sorted([o for o in things if o.get('walk_into') == '1'], key=lambda o: -num(o.get('height_cm')))
    tree_float = sorted([o for o in trees if o.get('floating') == '1'], key=lambda o: -num(o.get('gap_cm')))
    issue_count = collections.Counter(i for p in pts for i in serious(p))
    km = (num(pts[-1].get('run_dist_m')) - num(pts[0].get('run_dist_m'))) / 1000.0

    L += ['## Summary', '']
    L += table(['what', 'count', ''], [
        ['points checked', len(pts), f'{km:.1f} km of route, legs {pts[0].get("leg")}..{pts[-1].get("leg")}'],
        ['points with a ground problem', len(bad_pts), ', '.join(f'{k} {v}' for k, v in issue_count.most_common())],
        ['stretches with a hole or no ground (5 m samples)', len(hole_pts), 'between a point and the one before it'],
        ['points too steep to stand on', len(steep), ''],
        ['points with no rope, flag, clue or checkpoint within 100 m ahead', len(unguided), ''],
        ['objects measured', len(things), f'plus {len(trees)} trees'],
        ['floating (closest contact more than 10 cm off what is under it)', len(floating), ''],
        ['buried (more than half under the ground)', len(buried), 'method "bounds" = rough (no collision to trace)'],
        ['props walked through (no collision for the climber)', len(walk), 'trees left out: they never have collision'],
        ['floating trees', len(tree_float), ''],
    ])
    L.append('')

    # ---- objects by kind
    kinds = collections.OrderedDict()
    for o in objs:
        k = kinds.setdefault(o.get('kind', '?'), [0, 0, 0, 0])
        k[0] += 1
        k[1] += o.get('floating') == '1'
        k[2] += o.get('buried') == '1'
        k[3] += o.get('walk_into') == '1'
    L += ['### Objects by kind', '']
    L += table(['kind', 'measured', 'floating', 'buried', 'walked through'], [[k] + v for k, v in sorted(kinds.items(), key=lambda kv: -kv[1][0])])
    L += ['', 'Kind "hanging" (flags, cords, poles) and the camp markers\' cords and flags ("landmark" without collision) hang in the '
          'air by design and are never called floating.', '']

    # ---- floating
    L += [f'## Floating objects (worst {min(TOP, len(floating))} of {len(floating)})', '',
          'gap = how far the closest contact is above whatever is under it (terrain or another object). Type `Ghost` first, then the BugItGo.', '']
    L += table(['gap cm', 'kind', 'actor', 'mesh', 'how', 'near', 'go'],
               [[f"{num(o['gap_cm']):.0f}", o['kind'], o['actor'], o['mesh'] + (f" #{o['instance']}" if o.get('instance') not in ('', '-1') else ''),
                 o['method'], f"leg {o['leg']} {num(o['leg_dist_m']):.0f} m", code(o['bugitgo'])] for o in floating[:TOP]])
    by_mesh = collections.Counter(o['mesh'] for o in floating)
    if by_mesh:
        L += ['', 'Floating by mesh (a placement script to fix, when one mesh dominates): ' + ', '.join(f'{m} {n}' for m, n in by_mesh.most_common(10))]
    L.append('')

    # ---- buried
    L += [f'## Buried objects (worst {min(TOP, len(buried))} of {len(buried)})', '',
          'under = share of its height below the terrain. "bounds" = measured from the bounding box (no collision): rough, often too high for tilted rocks.', '']
    L += table(['under', 'pen cm', 'kind', 'actor', 'mesh', 'how', 'near', 'go'],
               [[f"{num(o['frac_under']):.2f}", f"{num(o['pen_cm']):.0f}", o['kind'], o['actor'],
                 o['mesh'] + (f" #{o['instance']}" if o.get('instance') not in ('', '-1') else ''), o['method'],
                 f"leg {o['leg']} {num(o['leg_dist_m']):.0f} m", code(o['bugitgo'])] for o in buried[:TOP]])
    L.append('')

    # ---- walked through
    L += [f'## Props a climber walks through (tallest {min(TOP, len(walk))} of {len(walk)})', '',
          'No collision for the climber, standing on the ground, over 50 cm tall and 30 cm wide. Snow drifts are left out (soft by design).', '']
    L += table(['height cm', 'kind', 'actor', 'mesh', 'near', 'go'],
               [[f"{num(o['height_cm']):.0f}", o['kind'], o['actor'], o['mesh'], f"leg {o['leg']} {num(o['leg_dist_m']):.0f} m", code(o['bugitgo'])]
                for o in walk[:TOP]])
    by_mesh = collections.Counter(o['mesh'] for o in walk)
    if by_mesh:
        L += ['', 'By mesh: ' + ', '.join(f'{m} {n}' for m, n in by_mesh.most_common(12))]
    if tree_float:
        L += ['', f'Floating trees ({len(tree_float)}): ' + '; '.join(f"{num(o['gap_cm']):.0f} cm {code(o['bugitgo'])}" for o in tree_float[:10])]
    L.append('')

    # ---- ground
    L += [f'## Ground problems at the points ({len(bad_pts)})', '']
    L += [f'- **{k}**: {v}' for k, v in SERIOUS.items() if issue_count.get(k)]
    L.append('')
    L += table(['point', 'where', 'problems', 'ground', 'slope', 'player vs seen cm', 'tile z', 'land z', 'go'],
               [[p['idx'], where(p), ', '.join(serious(p)), f"{p['ground']} / {p['surface']}", f"{num(p['slope_deg']):.0f} deg",
                 p.get('pawn_dz_cm', ''), p.get('tile_z', ''), p.get('land_z', ''), code(p.get('bugitgo'))] for p in bad_pts])
    L.append('')
    L += [f'## Holes between points ({len(hole_pts)} stretches)', '',
          'The stretch since the previous point, every 5 m: "holes" = a tile covers the spot but the ray found only the landscape under it; '
          '"no ground" = nothing at all. The BugItGo looks down on the first bad sample.', '']
    rows = []
    for p in hole_pts:
        x, y, z = num(p.get('seg_bad_x')), num(p.get('seg_bad_y')), num(p.get('stand_z'))
        go = f'BugItGo {x:.0f} {y:.0f} {z + 600:.0f} -60 {num(p.get("yaw")):.1f} 0' if ok(x) and ok(y) and ok(z) else ''
        rows.append([p['idx'], where(p), p.get('seg_samples'), p.get('seg_holes'), p.get('seg_no_ground'), p.get('seg_no_tile'), code(go)])
    L += table(['point', 'where', 'samples', 'holes', 'no ground', 'no tile', 'go'], rows)
    L.append('')
    L += [f'## Too steep to stand on ({len(steep)})', '']
    L += table(['point', 'where', 'slope', 'along the route', 'surface', 'go'],
               [[p['idx'], where(p), f"{num(p['slope_deg']):.0f} deg", f"{num(p['grade_deg']):.0f} deg", p['surface'], code(p.get('bugitgo'))]
                for p in sorted(steep, key=lambda p: -num(p['slope_deg']))])
    steps = sorted([p for p in pts if num(p.get('seg_steep')) > 0], key=lambda p: -num(p['seg_steep']))
    if steps:
        L += ['', 'Stretches with steep 5 m samples (count of unwalkable samples, biggest step between samples): '
              + '; '.join(f"{where(p)} {p['seg_steep']} / {num(p['seg_max_step_cm']):.0f} cm" for p in steps[:15])]
    L.append('')

    # ---- guidance
    L += [f'## No guidance ahead ({len(unguided)} points)', '',
          'No fixed rope, prayer flag / camp, clue or checkpoint within 100 m in front of the climber. Consecutive points are joined.', '']
    runs, cur = [], None
    for p in pts:
        if p.get('guided') == '0':
            if cur and cur['leg'] == p['leg'] and int(num(p['idx'])) == cur['last'] + 1:
                cur['to'] = num(p['leg_dist_m'])
                cur['last'] = int(num(p['idx']))
                cur['n'] += 1
            else:
                cur = {'leg': p['leg'], 'from': num(p['leg_dist_m']), 'to': num(p['leg_dist_m']), 'last': int(num(p['idx'])), 'n': 1,
                       'go': p.get('bugitgo', '')}
                runs.append(cur)
        else:
            cur = None
    L += table(['leg', 'from m', 'to m', 'points', 'go (first)'],
               [[r['leg'], f"{r['from']:.0f}", f"{r['to']:.0f}", r['n'], code(r['go'])] for r in runs])
    seen = [p for p in pts if p.get('rope_los') == '0' or p.get('flag_los') == '0']
    if seen:
        L += ['', f'{len(seen)} points have the nearest rope or flag ahead hidden from the eye (rope_los / flag_los = 0 in points.csv).']
    L.append('')

    # ---- frame time
    ms = [(num(p.get('frame_ms')), p) for p in pts if num(p.get('frame_ms')) > 0]
    if ms:
        vals = [m for m, _ in ms]
        L += ['## Frame time along the route', '',
              f'Settled frame time at the points (standalone, the test window size): median {statistics.median(vals):.1f} ms, '
              f'p95 {pct(vals, 0.95):.1f} ms, worst {max(vals):.1f} ms. Slowest points:', '']
        L += table(['ms', 'point', 'where', 'go'], [[f'{m:.1f}', p['idx'], where(p), code(p.get('bugitgo'))] for m, p in sorted(ms, key=lambda t: -t[0])[:8]])
        slow = [p for p in pts if num(p.get('streaming_left')) > 0]
        if slow:
            L += ['', f'{len(slow)} points still had textures streaming when measured (streaming_left > 0): their pictures may be blurry.']
        L.append('')

    # ---- pictures
    names, note = sheets(run, pts)
    L += ['## Pictures', '']
    if note:
        L.append(note)
    for n in names:
        L += [f'![{n}]({n})', '']
    L.append('Each picture is shots/NNN_leg<L>_<dist>m.jpg (and _back.jpg with Shots=2), looking along the route from eye height.')

    open(os.path.join(run, 'report.md'), 'w', encoding='utf-8').write('\n'.join(str(l) for l in L) + '\n')
    print(f'{run}/report.md: {len(pts)} points, {len(bad_pts)} with ground problems, {len(hole_pts)} stretches with holes, '
          f'{len(steep)} steep, {len(unguided)} unguided; {len(things)} objects: {len(floating)} floating, {len(buried)} buried, '
          f'{len(walk)} walked through; {len(names)} contact sheets')
    return 0


if __name__ == '__main__':
    sys.exit(main())
