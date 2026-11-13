# Runs INSIDE the editor (Tools/RoutePlanning/uepy): for every fixed rope, compares each instanced piece (rope segments,
# old ropes, stakes, ice screws, rock pegs) with the rope's own path. A piece far from the path is floating somewhere it
# should not be (for example a straight dotted line across the sky). Prints a summary per rope and component.
import unreal
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
ropes = [a for a in eas.get_all_level_actors() if a.get_class().get_name() == 'FixedRope']
print('fixed ropes:', len(ropes))
for r in ropes:
    tr = r.get_actor_transform()
    path = list(r.get_editor_property('lay_points'))
    old = list(r.get_editor_property('old_lay_points'))
    sample = path[::4] + old[::4]
    if not sample:
        print(r.get_actor_label(), 'no path'); continue
    # coarse grid of the path for nearest-point lookups
    cell = 5000.0
    grid = {}
    for p in sample:
        grid.setdefault((int(p.x // cell), int(p.y // cell)), []).append(p)

    def nearest(w):
        best = 1e18
        cx, cy = int(w.x // cell), int(w.y // cell)
        for dx in (-1, 0, 1):
            for dy in (-1, 0, 1):
                for p in grid.get((cx + dx, cy + dy), []):
                    d = (p.x - w.x) ** 2 + (p.y - w.y) ** 2 + (p.z - w.z) ** 2
                    best = min(best, d)
        return best ** 0.5 / 100.0 if best < 1e17 else 9999.0

    print(f'{r.get_actor_label()}  path {len(path)}  old {len(old)}  actor at {r.get_actor_location()}')
    for c in r.get_components_by_class(unreal.InstancedStaticMeshComponent):
        n = c.get_instance_count()
        far, worst, worst_at = 0, 0.0, None
        for i in range(0, n, max(1, n // 400)):
            t = c.get_instance_transform(i, True)
            d = nearest(t.translation)
            if d > 10.0:
                far += 1
                if d > worst:
                    worst, worst_at = d, t.translation
        print(f'    {c.get_name():22s} instances {n:6d}  sampled far(>10 m) {far:4d}  worst {worst:8.1f} m  at {worst_at}')
