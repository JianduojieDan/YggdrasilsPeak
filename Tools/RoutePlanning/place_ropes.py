# Runs inside the editor. Turns the planned route (ROUTE_JSON) into AFixedRope actors, one per leg.
import json, math
import unreal

SPACING = 400.0       # cm between rope points
LIFT = 30.0           # cm above the snow
CAMP_GAP = 1200.0     # rope stops this far (horizontal) from each camp centre

eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
marks = sorted([a for a in eas.get_all_level_actors() if a.get_class().get_name() == 'RouteLandmark'], key=lambda a: a.get_actor_label())
camp_xy = [(m.get_actor_location().x, m.get_actor_location().y) for m in marks]
rope_cls = unreal.load_class(None, '/Script/YggdrasilsPeak.FixedRope')

def landscape_z(x, y):
    hits = unreal.SystemLibrary.line_trace_multi(world, unreal.Vector(x, y, 600000), unreal.Vector(x, y, -200000),
        unreal.TraceTypeQuery.ECC_VISIBILITY, False, [], unreal.DrawDebugTrace.NONE, True)
    for h in hits:
        t = h.to_tuple()
        if t[9] and t[9].get_class().get_name().startswith('Landscape'):
            return t[5].z
    return None

def chaikin(pts, rounds=3):
    for _ in range(rounds):
        out = [pts[0]]
        for a, b in zip(pts, pts[1:]):
            out.append((0.75 * a[0] + 0.25 * b[0], 0.75 * a[1] + 0.25 * b[1]))
            out.append((0.25 * a[0] + 0.75 * b[0], 0.25 * a[1] + 0.75 * b[1]))
        out.append(pts[-1])
        pts = out
    return pts

def resample(pts, step):
    out = [pts[0]]
    carry = 0.0
    for a, b in zip(pts, pts[1:]):
        seg = math.dist(a, b)
        d = step - carry
        while d <= seg:
            t = d / seg
            out.append((a[0] + (b[0] - a[0]) * t, a[1] + (b[1] - a[1]) * t))
            d += step
        carry = seg - (d - step)
    out.append(pts[-1])
    return out

# Replace ropes from a previous run.
for a in eas.get_all_level_actors():
    if a.get_class() == rope_cls:
        eas.destroy_actor(a)

legs = json.load(open(ROUTE_JSON))
total = 0
for leg in legs:
    xy = chaikin([(p[0], p[1]) for p in leg['pts']])
    xy = resample(xy, SPACING)
    xy = [p for p in xy if min(math.hypot(p[0] - c[0], p[1] - c[1]) for c in camp_xy) > CAMP_GAP]
    pts = []
    for x, y in xy:
        z = landscape_z(x, y)
        if z is not None:
            pts.append(unreal.Vector(x, y, z + LIFT))
    rope = eas.spawn_actor_from_class(rope_cls, unreal.Vector(pts[0].x, pts[0].y, pts[0].z), unreal.Rotator(0, 0, 0))
    rope.set_actor_label('FixedRope_%d' % leg['leg'])
    rope.set_folder_path('Route/FixedRopes')
    rope.set_editor_property('rope_points', pts)
    total += len(pts)
    isms = rope.get_components_by_class(unreal.InstancedStaticMeshComponent)
    print('  instances', [c.get_instance_count() for c in isms])
    print('%s: %d points, %.1f km' % (leg['name'], len(pts), len(pts) * SPACING / 100000.0))
print('total rope points', total)
