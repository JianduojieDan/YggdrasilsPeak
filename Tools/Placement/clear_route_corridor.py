# Runs inside the editor (Tools/RoutePlanning/uepy.py, base_map open). Finds INSTANCES of scenery (default: the World
# Tree's root pieces, WorldTree_Roots) that stand in the walking corridor of a route leg - above the detail tile's snow
# within CORRIDOR_M of the route line (the fixed rope lies on it), up to HEAD_M over the snow - and, with APPLY = True,
# slides each one sideways out of the corridor (perpendicular to the route, away from it, in 1 m steps until no trace in
# the corridor hits it any more; at most MAX_SHIFT_M). Nothing is deleted. Moves are appended to
# Saved/Placement/ground_moves.csv in ground_pass's layout (run, actor, component, index, old x, y, z, old quat, new z,
# new quat, new x, new y), so Tools/Placement/revert_instance_moves.py (RUN = '<run>') puts them back.
# 2026-11-20 (A5): root pieces crossed the summit route 2,140-2,175 m along leg 7; the climber stuck 55 m below the top.
#   LEG = 7, S_FROM_M = 2100, S_TO_M = 2185, PREFIXES = ('WorldTree_Roots',), APPLY = False   (globals before exec)
import unreal, json, math, csv, datetime

LEG = globals().get('LEG', 7)
S_FROM_M = globals().get('S_FROM_M', 2100.0)
S_TO_M = globals().get('S_TO_M', 2185.0)
CORRIDOR_M = globals().get('CORRIDOR_M', 3.0)
HEAD_M = globals().get('HEAD_M', 2.5)
PREFIXES = globals().get('PREFIXES', ('WorldTree_Roots',))
APPLY = globals().pop('APPLY', False)
MAX_SHIFT_M = globals().get('MAX_SHIFT_M', 25.0)
OUT = 'F:/UE_project/YggdrasilsPeak/Saved/Placement'

w = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
pts = json.load(open(r"F:/UE_project/YggdrasilsPeak/Tools/RoutePlanning/route.json"))[LEG]['pts']
cum = [0.0]
for a, b in zip(pts, pts[1:]): cum.append(cum[-1] + math.hypot(b[0] - a[0], b[1] - a[1]))
def at(s):
    for i in range(len(pts) - 1):
        if cum[i + 1] >= s:
            t = (s - cum[i]) / max(cum[i + 1] - cum[i], 1)
            d = (pts[i + 1][0] - pts[i][0], pts[i + 1][1] - pts[i][1]); L = math.hypot(*d) or 1
            return pts[i][0] + d[0] * t, pts[i][1] + d[1] * t, (d[0] / L, d[1] / L)
    return pts[-1][0], pts[-1][1], (1, 0)

acts = [a for a in eas.get_all_level_actors() if a.get_actor_label().startswith(PREFIXES)]
comps = {}
for a in acts:
    for c in a.get_components_by_class(unreal.InstancedStaticMeshComponent):
        comps[c.get_path_name()] = (a, c)
tiles = [a for a in eas.get_all_level_actors() if a.get_actor_label().startswith('DetailTile_')]
OT = [unreal.ObjectTypeQuery.OBJECT_TYPE_QUERY1, unreal.ObjectTypeQuery.OBJECT_TYPE_QUERY2]

def snow(x, y):
    # the detail tile's surface: the first tile hit from above (whatever stands on it is skipped)
    for h in unreal.SystemLibrary.line_trace_multi_for_objects(w, unreal.Vector(x, y, 900000), unreal.Vector(x, y, 0), OT, True, [], unreal.DrawDebugTrace.NONE, True) or []:
        tt = h.to_tuple()
        if tt[9] and tt[9].get_actor_label().startswith('DetailTile_'): return tt[4].z
    return None

# the corridor samples: every 1 m along, every 0.75 m across
samples = []
s = S_FROM_M * 100
while s <= S_TO_M * 100:
    x, y, d = at(s)
    n = (-d[1], d[0])
    for k in range(-int(CORRIDOR_M / 0.75), int(CORRIDOR_M / 0.75) + 1):
        px, py = x + n[0] * k * 75, y + n[1] * k * 75
        z = snow(px, py)
        if z is not None: samples.append((s / 100, px, py, z, n))
    s += 100
print('corridor samples', len(samples))

def blockers():
    found = {}
    for (sm, px, py, z, n) in samples:
        hits = unreal.SystemLibrary.line_trace_multi_for_objects(w, unreal.Vector(px, py, z + HEAD_M * 100), unreal.Vector(px, py, z + 5), OT, True, [], unreal.DrawDebugTrace.NONE, True) or []
        for h in hits:
            tt = h.to_tuple()
            c = tt[10]
            if c and c.get_path_name() in comps:
                key = (c.get_path_name(), tt[13])
                f = found.setdefault(key, {'n': 0, 's': sm, 'top': 0.0, 'normal': n})
                f['n'] += 1; f['top'] = max(f['top'], tt[4].z - z)
    return found

found = blockers()
for (cp, i), f in sorted(found.items(), key=lambda kv: kv[1]['s']):
    a, c = comps[cp]
    tr = c.get_instance_transform(i, True)
    print('%-18s %-22s #%-4d at %.0f m along: %d corridor hits, up to %.1f m over the snow; instance at %s' % (a.get_actor_label(), c.get_name(), i, f['s'], f['n'], f['top'] / 100, tr.translation))

if APPLY and found:
    run = datetime.datetime.now().strftime('%Y-%m-%d %H:%M')
    rows = []
    for (cp, i), f in found.items():
        a, c = comps[cp]
        tr = c.get_instance_transform(i, True)
        old = unreal.Vector(tr.translation.x, tr.translation.y, tr.translation.z)   # a copy: tr.translation is changed below
        q = tr.rotation
        x, y, d = at(f['s'] * 100)
        n = (-d[1], d[0])
        side = 1 if (old.x - x) * n[0] + (old.y - y) * n[1] >= 0 else -1   # away from the route
        moved = False
        for step in range(1, int(MAX_SHIFT_M) + 1):
            tr.translation = unreal.Vector(old.x + n[0] * side * step * 100, old.y + n[1] * side * step * 100, old.z)
            c.update_instance_transform(i, tr, True, True, True)
            still = [k for k in blockers() if k == (cp, i)]
            if not still:
                moved = True
                break
        new = tr.translation
        rows.append([run, a.get_actor_label(), c.get_name(), i, old.x, old.y, old.z, str((q.x, q.y, q.z, q.w)), new.z, str((q.x, q.y, q.z, q.w)), new.x, new.y])
        print('moved #%d %s %.0f m sideways%s' % (i, c.get_name(), math.hypot(new.x - old.x, new.y - old.y) / 100, '' if moved else ' (STILL IN THE CORRIDOR at the limit)'))
    with open(OUT + '/ground_moves.csv', 'a', newline='') as fh:
        csv.writer(fh).writerows(rows)
    print('run', run, '- left in the corridor:', len(blockers()))
    print('saved', unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level())
