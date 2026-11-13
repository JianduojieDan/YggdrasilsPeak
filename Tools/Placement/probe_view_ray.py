# Runs INSIDE the editor (Tools/RoutePlanning/uepy): what lies along one sight line from a screenshot. Give the camera
# spot, its pitch/yaw, the pixel and the horizontal FOV; prints every actor whose bounds the ray passes through (nearest
# first) and, for instanced components it crosses, the instances closest to the ray.
import unreal, math
CAM = unreal.Vector(338533.0, 2468724.0, -68990.0 + 60.0)
PITCH, YAW, FOV, W, H = 40.0, 225.0, 90.0, 1920, 1080
PIXELS = [(925, 160), (760, 182), (1100, 136)]
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)


def ray_dir(px, py):
    f = (W / 2) / math.tan(math.radians(FOV / 2))
    cx, cy, cz = f, px - W / 2, (H / 2) - py                     # forward, right, up in camera space
    p, y = math.radians(PITCH), math.radians(YAW)
    fwd = (math.cos(p) * math.cos(y), math.cos(p) * math.sin(y), math.sin(p))
    right = (-math.sin(y), math.cos(y), 0.0)
    up = (-math.sin(p) * math.cos(y), -math.sin(p) * math.sin(y), math.cos(p))
    d = [fwd[i] * cx + right[i] * cy + up[i] * cz for i in range(3)]
    n = math.sqrt(sum(v * v for v in d))
    return [v / n for v in d]


def slab(o, d, bmin, bmax):
    t0, t1 = 0.0, 1e12
    for i in range(3):
        if abs(d[i]) < 1e-9:
            if o[i] < bmin[i] or o[i] > bmax[i]:
                return None
            continue
        a, b = (bmin[i] - o[i]) / d[i], (bmax[i] - o[i]) / d[i]
        if a > b:
            a, b = b, a
        t0, t1 = max(t0, a), min(t1, b)
        if t0 > t1:
            return None
    return t0


o = [CAM.x, CAM.y, CAM.z]
actors = eas.get_all_level_actors()
for px, py in PIXELS:
    d = ray_dir(px, py)
    print(f'--- pixel {px},{py}: direction {d[0]:.3f} {d[1]:.3f} {d[2]:.3f} (elevation {math.degrees(math.asin(d[2])):.1f} deg)')
    hits = []
    for a in actors:
        try:
            c, e = a.get_actor_bounds(False)
        except Exception:
            continue
        if e.x <= 0:
            continue
        t = slab(o, d, [c.x - e.x, c.y - e.y, c.z - e.z], [c.x + e.x, c.y + e.y, c.z + e.z])
        if t is not None:
            hits.append((t, a))
    hits.sort(key=lambda h: h[0])
    for t, a in hits[:25]:
        line = f'  {t / 100:9.0f} m  {a.get_class().get_name():28s} {a.get_actor_label()}'
        best = []
        for comp in a.get_components_by_class(unreal.InstancedStaticMeshComponent):
            n = comp.get_instance_count()
            if n == 0 or n > 200000:
                continue
            for i in range(n):
                tl = comp.get_instance_transform(i, True).translation
                v = [tl.x - o[0], tl.y - o[1], tl.z - o[2]]
                s = sum(v[k] * d[k] for k in range(3))
                if s <= 0:
                    continue
                dist = math.sqrt(max(0.0, sum(x * x for x in v) - s * s))
                best.append((dist, s, comp.get_name(), i, tl))
        if best:
            best.sort(key=lambda b: b[0])
            dd, s, cn, i, tl = best[0]
            line += f'   closest instance: {cn}[{i}] {dd / 100:.1f} m off the ray at {s / 100:.0f} m, z {tl.z / 100:.0f} m'
        print(line)
