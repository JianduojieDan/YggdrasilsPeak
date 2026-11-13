# Shared by ground_audit and ground_fix (exec'd inside the editor). Measures how everything placed meets the ground.
#
# Ground = the route detail tiles where they exist (their complex collision; on snow and ice that is the visible surface),
# else Landscape_Smooth. Each ground component is traced on its own (line_trace_component, ~7 us), so other rocks,
# props or the object itself never get in the way.
#
# Two ways to measure:
#  - "exact" (rocks in components under 20k instances, props, pickups, landmarks): the mesh vertices in world space are
#    binned into a grid under the object; the lowest vertex per cell is its underside there; gap = underside - ground.
#    pen = -min(gap) (how deep its deepest part is, < 0 = not touching at all), air = share of cells with air under them.
#  - "support" (20k+ instances: the small route stones; ice blocks): ground = plane from 5 traces; the mesh's support
#    function (furthest extent per direction, tabulated on a cube map) gives in O(1) how deep it reaches into that plane.
import math, collections
import unreal

eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
acts = eas.get_all_level_actors()

CELL = 25000.0
grid_tiles = collections.defaultdict(list)
grid_land = collections.defaultdict(list)
def _add(grid, comp):
    o, e, _ = unreal.SystemLibrary.get_component_bounds(comp)
    box = (o.x - e.x, o.y - e.y, o.x + e.x, o.y + e.y, o.z + e.z + 500.0, o.z - e.z - 500.0)
    for i in range(int(math.floor(box[0] / CELL)), int(math.floor(box[2] / CELL)) + 1):
        for j in range(int(math.floor(box[1] / CELL)), int(math.floor(box[3] / CELL)) + 1):
            grid[(i, j)].append((box, comp))
NTILES = NLAND = 0
for a in acts:
    if a.get_actor_label().startswith('DetailTile'):
        _add(grid_tiles, a.static_mesh_component); NTILES += 1
    elif a.get_class().get_name() == 'Landscape':
        for c in a.get_components_by_class(unreal.PrimitiveComponent):
            if c.get_class().get_name() == 'LandscapeHeightfieldCollisionComponent':
                _add(grid_land, c); NLAND += 1
TRACES = [0]
def ground(x, y):
    """(z, normal_z, source 1=tile 0=landscape) of the ground at x, y, or None."""
    key = (int(math.floor(x / CELL)), int(math.floor(y / CELL)))
    for grid, src in ((grid_tiles, 1), (grid_land, 0)):
        best = None
        for box, comp in grid.get(key, ()):
            if box[0] <= x <= box[2] and box[1] <= y <= box[3]:
                TRACES[0] += 1
                r = comp.line_trace_component(unreal.Vector(x, y, box[4]), unreal.Vector(x, y, box[5]), True, False, False)
                if r and r[0] is not None:
                    if best is None or r[0].z > best[0]:
                        best = (r[0].z, r[1].z, src)
        if best:
            return best
    return None

def qrot(q, v):
    qx, qy, qz, qw = q
    vx, vy, vz = v
    tx = 2 * (qy * vz - qz * vy); ty = 2 * (qz * vx - qx * vz); tz = 2 * (qx * vy - qy * vx)
    return (vx + qw * tx + (qy * tz - qz * ty), vy + qw * ty + (qz * tx - qx * tz), vz + qw * tz + (qx * ty - qy * tx))
def qmul(a, b):
    ax, ay, az, aw = a; bx, by, bz, bw = b
    return (aw * bx + ax * bw + ay * bz - az * by, aw * by - ax * bz + ay * bw + az * bx, aw * bz + ax * by - ay * bx + az * bw, aw * bw - ax * bx - ay * by - az * bz)
def q_from_to(u, v):
    """Shortest rotation taking unit vector u to unit vector v."""
    d = u[0] * v[0] + u[1] * v[1] + u[2] * v[2]
    c = (u[1] * v[2] - u[2] * v[1], u[2] * v[0] - u[0] * v[2], u[0] * v[1] - u[1] * v[0])
    w = 1.0 + d
    L = math.sqrt(c[0] ** 2 + c[1] ** 2 + c[2] ** 2 + w * w)
    return (c[0] / L, c[1] / L, c[2] / L, w / L)

MESH = {}
def mesh_verts(sm, limit):
    key = (sm.get_path_name(), limit)
    if key in MESH: return MESH[key]
    vs = []
    try:
        for s in range(sm.get_num_sections(0)):
            sec = unreal.ProceduralMeshLibrary.get_section_from_static_mesh(sm, 0, s)
            vs += [(v.x, v.y, v.z) for v in sec[0]]
    except Exception:
        pass
    if not vs:
        b = sm.get_bounding_box()
        vs = [(x, y, z) for x in (b.min.x, b.max.x) for y in (b.min.y, b.max.y) for z in (b.min.z, b.max.z)]
    if len(vs) > limit:
        # keep the extremes in 26 directions (the true bottom is never lost), then an even sample
        dirs = [(i, j, k) for i in (-1, 0, 1) for j in (-1, 0, 1) for k in (-1, 0, 1) if (i, j, k) != (0, 0, 0)]
        ext = {max(range(len(vs)), key=lambda n: vs[n][0] * d[0] + vs[n][1] * d[1] + vs[n][2] * d[2]) for d in dirs}
        step = len(vs) / float(limit)
        vs = [vs[n] for n in sorted(ext | {int(i * step) for i in range(limit)})]
    MESH[key] = vs
    return vs

N_CUBE = 20
def cube_cell(d):
    ax = max(range(3), key=lambda i: abs(d[i]))
    m = d[ax]
    face = ax * 2 + (0 if m > 0 else 1)
    o = [d[i] / abs(m) for i in range(3) if i != ax]
    u = min(N_CUBE - 1, int((o[0] + 1) * 0.5 * N_CUBE)); v = min(N_CUBE - 1, int((o[1] + 1) * 0.5 * N_CUBE))
    return (face * N_CUBE + u) * N_CUBE + v
SUP = {}
def support_table(sm):
    k = sm.get_path_name()
    if k in SUP: return SUP[k]
    vs = mesh_verts(sm, 1500)
    tab = [0.0] * (6 * N_CUBE * N_CUBE)
    for face in range(6):
        ax, sgn = face // 2, (1 if face % 2 == 0 else -1)
        oi = [i for i in range(3) if i != ax]
        for u in range(N_CUBE):
            for v in range(N_CUBE):
                d = [0.0, 0.0, 0.0]; d[ax] = sgn
                d[oi[0]] = (u + 0.5) / N_CUBE * 2 - 1; d[oi[1]] = (v + 0.5) / N_CUBE * 2 - 1
                L = math.sqrt(d[0] ** 2 + d[1] ** 2 + d[2] ** 2); d = (d[0] / L, d[1] / L, d[2] / L)
                tab[(face * N_CUBE + u) * N_CUBE + v] = max(p[0] * d[0] + p[1] * d[1] + p[2] * d[2] for p in vs)
    b = sm.get_bounding_box()
    SUP[k] = (tab, max(b.max.x - b.min.x, b.max.y - b.min.y) * 0.5)
    return SUP[k]

def world_verts(parts):
    """parts: [(static_mesh, (q, s, L))] -> world-space sample vertices of all of them."""
    ws = []
    for sm, (q, s, L) in parts:
        for v in mesh_verts(sm, 500):
            r = qrot(q, (v[0] * s[0], v[1] * s[1], v[2] * s[2]))
            ws.append((r[0] + L[0], r[1] + L[1], r[2] + L[2]))
    return ws

def measure_exact(parts):
    ws = world_verts(parts)
    xs = [p[0] for p in ws]; ys = [p[1] for p in ws]; zs = [p[2] for p in ws]
    x0, x1, y0, y1 = min(xs), max(xs), min(ys), max(ys)
    height = max(zs) - min(zs)
    size = max(x1 - x0, y1 - y0, height)
    n = 6 if size > 800 else 4
    cells = {}
    for p in ws:
        c = (min(n - 1, int((p[0] - x0) / max(x1 - x0, 1e-3) * n)), min(n - 1, int((p[1] - y0) / max(y1 - y0, 1e-3) * n)))
        if c not in cells or p[2] < cells[c][2]:
            cells[c] = p
    tol = max(2.0, 0.02 * size)
    gaps = []; gpts = []
    src = -1
    for p in cells.values():
        g = ground(p[0], p[1])
        if g:
            gaps.append(p[2] - g[0]); gpts.append((p[0], p[1], g[0])); src = max(src, g[2])
    if not gaps:
        return None
    # ground plane normal from the cell grounds (least squares in x, y)
    mx = sum(p[0] for p in gpts) / len(gpts); my = sum(p[1] for p in gpts) / len(gpts); mz = sum(p[2] for p in gpts) / len(gpts)
    sxx = sum((p[0] - mx) ** 2 for p in gpts); syy = sum((p[1] - my) ** 2 for p in gpts); sxy = sum((p[0] - mx) * (p[1] - my) for p in gpts)
    sxz = sum((p[0] - mx) * (p[2] - mz) for p in gpts); syz = sum((p[1] - my) * (p[2] - mz) for p in gpts)
    det = sxx * syy - sxy * sxy
    b, c = ((sxz * syy - syz * sxy) / det, (syz * sxx - sxz * sxy) / det) if abs(det) > 1e-6 else (0.0, 0.0)
    ln = math.sqrt(b * b + c * c + 1)
    nrm = (-b / ln, -c / ln, 1 / ln)
    sg = sorted(gaps)
    return {'size': size, 'height': height, 'foot': max(x1 - x0, y1 - y0), 'pen': -sg[0], 'lift': sg[-1],
            'p75': sg[min(len(sg) - 1, int(0.75 * len(sg)))], 'air': sum(1 for g in gaps if g > tol) / float(len(gaps)),
            'slope': math.degrees(math.acos(nrm[2])), 'normal': nrm, 'src': src, 'cells': len(gaps)}

def measure_support(sm, q, s, L):
    tab, rxy = support_table(sm)
    qi = (-q[0], -q[1], -q[2], q[3])
    r = max(30.0, 0.6 * s * rxy)
    pts = [ground(L[0], L[1]), ground(L[0] + r, L[1]), ground(L[0] - r, L[1]), ground(L[0], L[1] + r), ground(L[0], L[1] - r)]
    if any(p is None for p in pts):
        return None
    a = sum(p[0] for p in pts) / 5.0
    b = (pts[1][0] - pts[2][0]) / (2 * r); c = (pts[3][0] - pts[4][0]) / (2 * r)
    ln = math.sqrt(b * b + c * c + 1); n = (-b / ln, -c / ln, 1 / ln)
    h = (L[2] - a) * n[2]
    down_ext = s * tab[cube_cell(qrot(qi, (-n[0], -n[1], -n[2])))]
    up_ext = s * tab[cube_cell(qrot(qi, n))]
    pen = down_ext - h
    thick = down_ext + up_ext
    return {'size': thick, 'pen': pen, 'thick': thick, 'frac': pen / max(thick, 1.0), 'nz': n[2], 'slope': math.degrees(math.acos(n[2])), 'src': max(p[2] for p in pts)}

def tr_parts(tr):
    return ((tr.rotation.x, tr.rotation.y, tr.rotation.z, tr.rotation.w), (tr.scale3d.x, tr.scale3d.y, tr.scale3d.z),
            (tr.translation.x, tr.translation.y, tr.translation.z))

# what is checked, and how it is grouped
SKIP_LABEL = ('DetailTile', 'Distant', 'Far', 'Horizon', 'WorldTree', 'Realm', 'Cube', 'IceCave', 'Landscape', 'GameShotCam')
SKIP_CLASS = ('Ultra_Dynamic_Sky_C', 'Ultra_Dynamic_Weather_C', 'CameraActor', 'CrevasseCrossing', 'Landscape', 'BP_LoadingArrival_C',
              'NiagaraActor', 'PointLight', 'PlayerStart', 'BlockingVolume', 'PCGVolume', 'PCGWorldActor', 'LandscapeRebuilder')
SKIP_FOLDER = ('Environment/WorldTree', 'Realms', 'Distant', 'Route/IceCave', 'Route/Crevasses', 'Tools')
def skip_actor(a):
    lab = a.get_actor_label(); cls = a.get_class().get_name(); folder = str(a.get_folder_path())
    if lab.startswith(SKIP_LABEL) or cls in SKIP_CLASS or folder.startswith(SKIP_FOLDER):
        return True
    l = a.get_actor_location()
    return l.z > 600000.0                      # parked far above the world (templates)
