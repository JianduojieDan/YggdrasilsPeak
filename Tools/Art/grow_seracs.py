# Runs inside the editor. Big seracs were 10-18 m tall (real Khumbu seracs reach 30-40 m). Scales them 1.6x, keeping their bases where
# they are, except any that would then reach the route or ropes. Small blocks (scale under 0.5) stay. Run once.
import unreal, json, math
GROW = 1.6
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
acts = eas.get_all_level_actors()
pts = [p for leg in json.load(open("F:/UE_project/YggdrasilsPeak/Tools/RoutePlanning/route.json")) for p in leg["pts"]]
for a in acts:
    if a.get_class().get_name() == "FixedRope":
        pts += [[p.x, p.y, p.z] for p in a.get_editor_property("rope_points")]
CELL = 10000.0
grid = {}
for p in pts:
    grid.setdefault((int(p[0] // CELL), int(p[1] // CELL)), []).append(p)
def near(x, y, r):
    cx, cy = int(x // CELL), int(y // CELL)
    reach = int(r // CELL) + 1
    return any(math.hypot(p[0] - x, p[1] - y) < r for i in range(cx - reach, cx + reach + 1) for j in range(cy - reach, cy + reach + 1) for p in grid.get((i, j), ()))
s = [a for a in acts if a.get_actor_label() == "Scatter_IcefallSeracs"][0]
grown = kept = 0
hs = []
for c in s.get_components_by_class(unreal.InstancedStaticMeshComponent):
    b = c.static_mesh.get_bounding_box()
    w = max(b.max.x - b.min.x, b.max.y - b.min.y) / 2
    for i in range(c.get_instance_count()):
        t = c.get_instance_transform(i, True)
        k = t.scale3d.x
        if k < 0.5:
            continue
        if near(t.translation.x, t.translation.y, w * k * GROW + 300.0):
            kept += 1; continue
        ns = t.scale3d * GROW
        z = t.translation.z + b.min.z * (k - k * GROW)
        c.update_instance_transform(i, unreal.Transform(unreal.Vector(t.translation.x, t.translation.y, z), t.rotation.rotator(), ns), True, True, True)
        grown += 1
        hs.append((b.max.z - b.min.z) * k * GROW / 100)
hs.sort()
print("grown", grown, "kept near route", kept, "height m median/max", round(hs[len(hs) // 2], 1), round(hs[-1], 1))
