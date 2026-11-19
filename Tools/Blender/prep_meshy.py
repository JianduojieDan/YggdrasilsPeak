# blender -b --factory-startup -P Tools/Blender/prep_meshy.py -- [<id> ...]
# (set TEMP/TMP to Saved/tmp_blender). Meshy delivers every model at ~1.9 m. For each prop this
#  1. scales it to its real size (Tools/Meshy/assets.py "size" = the largest side in metres), pivot at the bottom centre;
#  2. cleans the mesh: welds the UV-seam duplicates (merge by distance), fills SMALL closed boundary holes (cracks,
#     pin holes) with the neighbouring material and UVs, keeps big openings, recalculates outward normals;
#  3. tents only: a gentle smooth of the fabric (poles, ropes, rims, rocks masked out) against Meshy's lumps, each
#     vertex limited to 30 % of the shell thickness so the inner wall never pokes through the outer one (self-checked
#     by ray test, undone per tent if it still pokes - tent_toilet);
#  4. props with windows (tent_mess): finds the see-through holes in the walls (Meshy makes thin fabric a thick
#     closed shell, so a window opening is a tunnel, not a boundary loop), and closes the small ones with a round
#     pane that follows the fabric 2 mm proud, overlaps the hole by >= 3.5 cm and carries Meshy's own painted-window
#     texture (UVs borrowed from a closed painted window), plus a cap over the tunnel's inner end. The windows Meshy
#     painted closed stay as they are. Door-sized openings and anything touching the ground stay open;
#  5. keeps Meshy's own (high-res baked) normals through all of this and puts them back (NORMALS = "angle" instead
#     shades smooth by angle: 70 deg soft props, 38 deg hard props - that showed the remesh lumps as facets);
# and writes Art/Meshy/<id>.glb for the UE import (Tools/Story/import_ai_assets) plus Art/Meshy/prep_report.json.
# Never touches the originals in Downloads/ai_assets.
import bpy, bmesh, sys, os, math, json
from mathutils import Vector, Matrix
from mathutils.bvhtree import BVHTree

ROOT = "F:/UE_project/YggdrasilsPeak"
sys.path.insert(0, f"{ROOT}/Tools/Meshy")
from assets import ASSETS  # noqa: E402

TENTS = {"tent_dome", "tent_mess", "tent_kitchen", "tent_toilet"}
WINDOW_PROPS = {"tent_mess"}          # props whose concept has clear windows: plug their small see-through holes
SCAN_ONLY = TENTS - WINDOW_PROPS      # report their see-through openings, change nothing
FILL_PERIM = 0.12                     # fill a closed boundary loop if perimeter < 12 % of the largest side ...
FILL_EDGES = 48                       # ... and it has at most this many edges
# Normals: "meshy" keeps Meshy's own normals (baked from its high-res model: smooth, they hide the remesh lumps;
# tested 2026-11-19, smooth-by-angle made the lumps show as facets), "angle" = Smooth by Angle below.
NORMALS = "meshy"
# Smooth by angle (degrees, only when NORMALS = "angle"): creases sharper than this stay hard. Soft (fabric) props
# get a wide angle - a 38 deg split on lumpy cloth creates hard facets - hard props (stone, metal, wood) 38 deg.
SMOOTH_ANGLE = {"soft": 70.0, "hard": 38.0}
SOFT = TENTS | {"duffel", "bedding", "porter_load", "daypack", "khata_folded", "khata_tied", "tarchen", "meat_slats",
                "dung_heap", "water_bottle", "headlamp"}
WINDOW_MAX = 0.05                     # see-through opening is a window if its radius < 5 % of the largest side
# tents: smooth along the normal (factor, iterations) on vertices whose edges all bend less than "bend" degrees
# (None = all). The mess dome has no exposed poles, so it is smoothed everywhere; the others keep poles/ropes/stone
# walls masked. Meshy's fabric is a closed shell 12-35 mm thick, so a vertex may move along its normal at most
# "shell" x the distance to the opposite wall (measured both ways), and the move fades in over "ramp" edge rings
# from open seams / masked vertices. v1 (no limit) pushed the inner wall out through the outer wall (toilet roof
# 10 % of view rays, mess dome 0.9 %) and tore the seam slits. A self-check (SHELL_POKE) undoes it if it still pokes.
SMOOTH_FABRIC = {"tent_mess": dict(bend=None, factor=0.5, iterations=4, shell=0.3, ramp=3),
                 "default": dict(bend=45.0, factor=0.5, iterations=3, shell=0.3, ramp=3)}
SHELL_POKE_MAX = 0.05                 # % of view rays allowed to newly hit an inner wall poking through the outer one
# windows: the pane overlaps the hole by at least this much, so the rim can't show a sliver of the hole
PANE_MARGIN = 0.035
PANE_CLEAR = 0.002                    # pane sits 2 mm proud of the highest fabric within 2.5 cm (follows the fabric)


def log(*a):
    print("PREP", *a, flush=True)


def import_join(src):
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.ops.import_scene.gltf(filepath=src)
    meshes = [o for o in bpy.context.scene.objects if o.type == "MESH"]
    bpy.ops.object.select_all(action="DESELECT")
    for o in meshes:
        o.select_set(True)
    bpy.context.view_layer.objects.active = meshes[0]
    if len(meshes) > 1:
        bpy.ops.object.join()
    ob = bpy.context.view_layer.objects.active
    ob.parent = None
    bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
    for o in list(bpy.context.scene.objects):   # drop empties left by the importer
        if o != ob:
            bpy.data.objects.remove(o, do_unlink=True)
    return ob


def bounds(me):
    lo = Vector((min(v.co[i] for v in me.vertices) for i in range(3)))
    hi = Vector((max(v.co[i] for v in me.vertices) for i in range(3)))
    return lo, hi


def scale_to_size(ob, size):
    lo, hi = bounds(ob.data)
    k = size / max(hi - lo)
    off = Vector(((lo.x + hi.x) / 2, (lo.y + hi.y) / 2, lo.z))
    for v in ob.data.vertices:
        v.co = (v.co - off) * k
    ob.data.update()


def boundary_cycles(bm):
    """Connected groups of boundary edges; returns (edges, is_closed_simple_cycle)."""
    bnd = {e for e in bm.edges if e.is_boundary}
    seen, out = set(), []
    for e0 in bnd:
        if e0 in seen:
            continue
        comp, stack = [], [e0]
        seen.add(e0)
        while stack:
            e = stack.pop()
            comp.append(e)
            for v in e.verts:
                for e2 in v.link_edges:
                    if e2 in bnd and e2 not in seen:
                        seen.add(e2)
                        stack.append(e2)
        cs = set(comp)
        closed = len(comp) >= 3 and all(sum(1 for e in v.link_edges if e in cs) == 2
                                         for v in {v for e in comp for v in e.verts})
        out.append((comp, closed))
    return out


def fill_small_holes(bm, maxdim):
    uv = bm.loops.layers.uv.active
    filled = kept_big = open_chains = 0
    for comp, closed in boundary_cycles(bm):
        if not closed:
            open_chains += 1                         # slit ending at a non-manifold vertex: nothing to fill
            continue
        per = sum(e.calc_length() for e in comp)
        if per >= FILL_PERIM * maxdim or len(comp) > FILL_EDGES:
            kept_big += 1
            continue
        nb = [f for e in comp for f in e.link_faces]
        mat = max(set(f.material_index for f in nb), key=[f.material_index for f in nb].count) if nb else 0
        res = bmesh.ops.holes_fill(bm, edges=comp, sides=0)
        new = res["faces"]
        if not new:
            continue
        tri = bmesh.ops.triangulate(bm, faces=new)
        for f in tri["faces"]:
            f.material_index = mat
            if uv:
                for lp in f.loops:                   # borrow the UV the neighbour already has at this vertex
                    for l2 in lp.vert.link_loops:
                        if l2.face not in tri["faces"]:
                            lp[uv].uv = l2[uv].uv
                            break
        filled += 1
    return filled, kept_big, open_chains


def recalc_normals(bm):
    """Blender's outward recalc, but every face it wants to flip is checked with two rays: the flip is kept only
    if the new normal clearly faces open space (its ray runs > 1.5x farther than the ray out of the back). Unchecked,
    recalc wrongly flipped 259 of 271 faces on stove_set (open pots, non-manifold bits). Returns (kept, reverted)."""
    lay = bm.faces.layers.int.new("flipkept")       # restore_meshy_normals turns Meshy's normals on these faces
    before = {f: f.normal.copy() for f in bm.faces}
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces[:])
    bm.normal_update()
    flipped = [f for f in bm.faces if f.normal.dot(before[f]) < 0]
    tree = BVHTree.FromBMesh(bm)
    revert = []
    for f in flipped:
        c, n = f.calc_center_median(), f.normal
        fwd = tree.ray_cast(c + n * 1e-4, n, 20)[3] or 99.0
        bwd = tree.ray_cast(c - n * 1e-4, -n, 20)[3] or 99.0
        if not fwd > bwd * 1.5:
            revert.append(f)
    if revert:
        bmesh.ops.reverse_faces(bm, faces=revert)
        bm.normal_update()
    rs = set(revert)
    for f in flipped:
        if f not in rs:
            f[lay] = 1
    return [len(flipped) - len(revert), len(revert)]


def fib_dirs(n, zmin=-0.25):
    ga = math.pi * (3 - math.sqrt(5))
    for i in range(n):
        z = 1 - 2 * (i + 0.5) / n
        if z < zmin:
            continue
        r = math.sqrt(1 - z * z)
        yield Vector((r * math.cos(ga * i), r * math.sin(ga * i), z))


def perp_basis(n):
    a = Vector((0, 0, 1)) if abs(n.z) < 0.9 else Vector((1, 0, 0))
    u = n.cross(a).normalized()
    return u, n.cross(u).normalized()


def find_openings(bm, H, maxdim):
    """See-through openings in a closed tent shell, found with rays from points on the inside axis."""
    tree = BVHTree.FromBMesh(bm)
    exits = []
    for hz in (0.25, 0.4, 0.55):
        P = Vector((0, 0, H * hz))
        for d in fib_dirs(120000):
            if tree.ray_cast(P, d, 100)[0] is not None:
                continue
            u, w = perp_basis(d)
            ts = []
            for k in range(8):                       # wall distance from slightly tilted neighbour rays
                a = k * math.pi / 4
                d2 = (d + (u * math.cos(a) + w * math.sin(a)) * math.tan(math.radians(4))).normalized()
                h = tree.ray_cast(P, d2, 100)
                if h[0] is not None:
                    ts.append(h[3])
            if ts:
                ts.sort()
                exits.append((P + d * ts[len(ts) // 2], d))
    # cluster exit points (grid hash, 8 cm link)
    cell, link = 0.05, 0.08
    grid = {}
    for i, (p, _) in enumerate(exits):
        grid.setdefault(tuple(int(math.floor(c / cell)) for c in p), []).append(i)
    lab = [-1] * len(exits)
    clusters = []
    for i in range(len(exits)):
        if lab[i] >= 0:
            continue
        lab[i] = len(clusters)
        mem, stack = [], [i]
        while stack:
            j = stack.pop()
            mem.append(j)
            g = tuple(int(math.floor(c / cell)) for c in exits[j][0])
            for dx in (-2, -1, 0, 1, 2):
                for dy in (-2, -1, 0, 1, 2):
                    for dz in (-2, -1, 0, 1, 2):
                        for m in grid.get((g[0] + dx, g[1] + dy, g[2] + dz), ()):
                            if lab[m] < 0 and (exits[m][0] - exits[j][0]).length < link:
                                lab[m] = lab[i]
                                stack.append(m)
        clusters.append(mem)
    out = []
    for mem in clusters:
        c = sum((exits[j][0] for j in mem), Vector()) / len(mem)
        n = sum((exits[j][1] for j in mem), Vector()).normalized()
        r = max(((exits[j][0] - c) - n * (exits[j][0] - c).dot(n)).length for j in mem)
        low = min(exits[j][0].z for j in mem)
        out.append(dict(c=c, n=n, r=r, rays=len(mem), low=low))
    out.sort(key=lambda o: -o["rays"])
    return tree, out


def base_color_image(ob):
    """Meshy's base-colour texture as a numpy array (rows from the bottom, like Blender UVs), or None."""
    import numpy as np
    for m in ob.data.materials:
        if not (m and m.use_nodes):
            continue
        for nd in m.node_tree.nodes:
            if nd.type == "BSDF_PRINCIPLED" and nd.inputs["Base Color"].is_linked:
                src = nd.inputs["Base Color"].links[0].from_node
                if src.type == "TEX_IMAGE" and src.image:
                    img = src.image
                    px = np.empty(img.size[0] * img.size[1] * img.channels, dtype=np.float32)
                    img.pixels.foreach_get(px)
                    return px.reshape(img.size[1], img.size[0], img.channels)
    return None


def _grow(seed, allowed):
    comp, stack = {seed}, [seed]
    while stack:
        a_, b_ = stack.pop()
        for k in ((a_ + 1, b_), (a_ - 1, b_), (a_, b_ + 1), (a_, b_ - 1)):
            if k in allowed and k not in comp:
                comp.add(k)
                stack.append(k)
    return comp


def _tex_px(tex, uv):
    hgt, wid = tex.shape[0], tex.shape[1]
    return tex[min(hgt - 1, max(0, int(uv[1] % 1.0 * hgt))), min(wid - 1, max(0, int(uv[0] % 1.0 * wid)))]


def _is_grey(px):
    mx, mn = max(px[:3]), min(px[:3])
    return mx > 0.05 and (mx - mn) / mx < 0.16                 # neutral grey = the window Meshy painted


def ray_scan(bm, tree, o0, d, u, w, R, g, maxt, tex=None):
    """Rays along d from a g grid (in the u/w plane through o0) within R. Returns depth t per sample (99 = no hit),
    texture UV and material per hit, and the samples whose base colour is the painted-window grey."""
    from mathutils.geometry import barycentric_transform
    uvl = bm.loops.layers.uv.active
    bm.faces.ensure_lookup_table()
    steps = int(R / g)
    t, uv, mats, grey = {}, {}, {}, set()
    for i in range(-steps, steps + 1):
        for j in range(-steps, steps + 1):
            if (i * i + j * j) * g * g > R * R:
                continue
            loc, _nr, fi, tt = tree.ray_cast(o0 + u * (i * g) + w * (j * g), d, maxt)
            t[(i, j)] = tt if loc is not None else 99.0
            if loc is None:
                continue
            mats[(i, j)] = bm.faces[fi].material_index
            if uvl:
                ls = bm.faces[fi].loops[:3]
                p = barycentric_transform(loc, *(lp.vert.co for lp in ls),
                                          *(Vector((lp[uvl].uv.x, lp[uvl].uv.y, 0)) for lp in ls))
                uv[(i, j)] = (p.x, p.y)
                if tex is not None and _is_grey(_tex_px(tex, uv[(i, j)])):
                    grey.add((i, j))
    return t, uv, mats, grey


def window_scan(bm, tree, c, n, R, tex, g=0.008, back=0.4):
    """A window seen from outside: rays along -n on an 8 mm grid. Finds the see-through hole (rays that pass the
    wall) and the grey window Meshy painted around it, their common centre (ci, cj in grid units), the hole radius rh
    and the painted radius rg (per 10 deg sector the farthest window sample, median sector: keeps grey dust out)."""
    u, w = perp_basis(n)
    o0 = c + n * back
    t, uv, mats, grey = ray_scan(bm, tree, o0, -n, u, w, R, g, back + 2.0, tex)
    wall_t = sorted(v for v in t.values() if v < back + 0.3)
    if not wall_t:
        return None
    t_wall = wall_t[len(wall_t) // 2]
    through = {k for k, v in t.items() if v > t_wall + 0.25 and (k[0] * k[0] + k[1] * k[1]) * g * g < R * R / 4}
    grey = {k for k in grey if t[k] < back + 0.3}
    if through:          # see-through window: start from the hole, grow over the painted grey
        hole = _grow(min(through, key=lambda k: k[0] * k[0] + k[1] * k[1]), through)
        win = _grow(next(iter(hole)), hole | grey)
    elif grey:           # closed window (painted on solid fabric)
        hole = set()
        win = _grow(min(grey, key=lambda k: k[0] * k[0] + k[1] * k[1]), grey)
    else:
        return None
    ci = sum(k[0] for k in win) / len(win)
    cj = sum(k[1] for k in win) / len(win)
    rh = (max(math.hypot(k[0] - ci, k[1] - cj) for k in hole) * g + g) if hole else 0.0
    S = 36
    sect = [0.0] * S
    for k in win:
        dx, dy = (k[0] - ci) * g, (k[1] - cj) * g
        sct = int(((math.atan2(dy, dx) + 2 * math.pi) % (2 * math.pi)) / (2 * math.pi) * S) % S
        sect[sct] = max(sect[sct], math.hypot(dx, dy))
    rg = sorted(sect)[S // 2] + g * 0.5
    mat = max(set(mats.values()), key=list(mats.values()).count) if mats else 0
    return dict(c=c, n=n, u=u, w=w, o0=o0, d=-n, g=g, back=back, t=t, uv=uv, grey=grey, hole=hole, win=win,
                ci=ci, cj=cj, rh=rh, rg=rg, mat=mat, R=R)


def _lsq3(rows):
    """Least squares a + b*x + c*y = z over rows (x, y, z)."""
    sx = sum(r_[0] for r_ in rows)
    sy = sum(r_[1] for r_ in rows)
    sz = sum(r_[2] for r_ in rows)
    sxx = sum(r_[0] ** 2 for r_ in rows)
    syy = sum(r_[1] ** 2 for r_ in rows)
    sxy = sum(r_[0] * r_[1] for r_ in rows)
    sxz = sum(r_[0] * r_[2] for r_ in rows)
    syz = sum(r_[1] * r_[2] for r_ in rows)
    A = Matrix(((len(rows), sx, sy), (sx, sxx, sxy), (sy, sxy, syy)))
    try:
        return tuple(A.inverted() @ Vector((sz, sxz, syz)))
    except ValueError:
        return (sz / max(len(rows), 1), 0.0, 0.0)


def window_uv_map(sc, texsize):
    """How Meshy's texture lies on a closed painted window: an affine map from window coordinates (x, y divided by
    the painted radius rg) to texture UV, fitted over the window and its beige frame ring (up to 1.45 rg). Robust
    refit without samples off by > 10 texels; 'ok' = one continuous UV island (>= 90 % fit, median < 3 texels)."""
    g, ci, cj, rg = sc["g"], sc["ci"], sc["cj"], sc["rg"]
    pts = []
    for k, (uu, vv) in sc["uv"].items():
        x, y = (k[0] - ci) * g / rg, (k[1] - cj) * g / rg
        if x * x + y * y <= 1.45 ** 2 and sc["t"][k] < sc["back"] + 0.3:
            pts.append((x, y, uu, vv))
    if len(pts) < 30:
        return None
    keep = pts
    for _ in range(3):
        Mu = _lsq3([(p_[0], p_[1], p_[2]) for p_ in keep])
        Mv = _lsq3([(p_[0], p_[1], p_[3]) for p_ in keep])
        res = [math.hypot(Mu[0] + Mu[1] * p_[0] + Mu[2] * p_[1] - p_[2],
                          Mv[0] + Mv[1] * p_[0] + Mv[2] * p_[1] - p_[3]) * texsize for p_ in pts]
        keep = [p_ for p_, r_ in zip(pts, res) if r_ < 10.0]
        if len(keep) < 30:
            return None
    rs = sorted(res)
    med, p95, frac = rs[len(rs) // 2], rs[int(len(rs) * 0.95)], len(keep) / len(pts)
    return dict(Mu=Mu, Mv=Mv, med_tx=round(med, 2), p95_tx=round(p95, 2), fit_frac=round(frac, 3),
                ok=frac >= 0.9 and med < 3.0, centre=[round(x, 3) for x in sc["c"]], rg=round(rg, 3))


def build_disc(bm, sc, rh, rp, mat, uvf, nlay, seg=48, step=0.012, env=0.025):
    """A round pane facing back along the scan rays (towards sc['o0']), conforming to the fabric: each vertex sits
    PANE_CLEAR proud of the highest fabric sample within 'env' (over the hole: the plane fitted through the fabric).
    Rings every 'step' from just inside the hole edge out to rp; UVs from uvf(x, y); custom normal = plane normal."""
    g, ci, cj, o0, u, w, d = sc["g"], sc["ci"], sc["cj"], sc["o0"], sc["u"], sc["w"], sc["d"]
    lim = sc["back"] + 0.3
    fab = {k: v for k, v in sc["t"].items() if v < lim}
    rows = [((k[0] - ci) * g, (k[1] - cj) * g, v) for k, v in fab.items()
            if math.hypot(k[0] - ci, k[1] - cj) * g < rp + 0.03]
    if len(rows) < 10:
        return None
    pa, pb, pc = _lsq3(rows)
    ne = int(env / g) + 1

    def depth(x, y):
        i0, j0 = ci + x / g, cj + y / g
        best = None
        for i in range(int(math.floor(i0)) - ne, int(math.ceil(i0)) + ne + 1):
            for j in range(int(math.floor(j0)) - ne, int(math.ceil(j0)) + ne + 1):
                v = fab.get((i, j))
                if v is not None and ((i - i0) ** 2 + (j - j0) ** 2) * g * g <= env * env:
                    best = v if best is None else min(best, v)
        pl = pa + pb * x + pc * y
        return (pl if best is None else min(best, pl + 0.03)) - PANE_CLEAR

    def pos(x, y):
        return o0 + u * (ci * g + x) + w * (cj * g + y) + d * depth(x, y)
    pn = (u + d * pb).cross(w + d * pc).normalized()
    if pn.dot(d) > 0:
        pn = -pn
    radii = []
    r_ = max(step, rh - 0.02) if rh > 0.03 else step
    while r_ < rp - 1e-4:
        radii.append(r_)
        r_ += step
    radii.append(rp)
    uvl = bm.loops.layers.uv.active
    xy = {}
    cv = bm.verts.new(pos(0, 0))
    xy[cv] = (0.0, 0.0)
    rings = []
    for rr in radii:
        ring = []
        for s_ in range(seg):
            a = 2 * math.pi * s_ / seg
            x, y = math.cos(a) * rr, math.sin(a) * rr
            vv = bm.verts.new(pos(x, y))
            xy[vv] = (x, y)
            ring.append(vv)
        rings.append(ring)
    faces = [bm.faces.new((cv, rings[0][s_], rings[0][(s_ + 1) % seg])) for s_ in range(seg)]
    for a_, b_ in zip(rings, rings[1:]):
        for s_ in range(seg):
            s2 = (s_ + 1) % seg
            faces.append(bm.faces.new((a_[s_], b_[s_], b_[s2])))
            faces.append(bm.faces.new((a_[s_], b_[s2], a_[s2])))
    for f in faces:
        f.normal_update()
        if f.normal.dot(pn) < 0:
            f.normal_flip()
        f.material_index = mat
        f.smooth = True
        for lp in f.loops:
            if uvl:
                lp[uvl].uv = uvf(*xy[lp.vert])
            if nlay is not None:
                lp[nlay] = pn
    gaps = [depth(x, y) + PANE_CLEAR - (pa + pb * x + pc * y) for x, y in
            ((math.cos(a) * rp, math.sin(a) * rp) for a in (2 * math.pi * k / 24 for k in range(24)))]
    return dict(tris=len(faces), rings=len(radii), plane_tilt_deg=round(math.degrees(math.atan(math.hypot(pb, pc))), 1),
                rim_vs_plane_mm=[round(min(gaps) * 1000, 1), round(max(gaps) * 1000, 1)])


def plug_window(bm, tree, op, tex, refs, nlay, flat_mat):
    """Close a see-through window hole (Meshy's thick fabric shell has a tunnel there): an outer pane over the hole
    and the grey window Meshy painted round it, at least PANE_MARGIN beyond the hole edge, following the fabric 2 mm
    proud; plus a cap over the tunnel's inner end, so no ray can slip under the pane rim into the tent. Both use
    Meshy's own painted-window texture (UVs mapped from a closed painted window, ref) so the plugged windows look
    like the closed ones: of the clean painted windows (refs) the one closest in size; without one they get the flat
    'window' material (flat_mat())."""
    R = max(op["r"] * 4.0, 0.3)
    sc = window_scan(bm, tree, op["c"], op["n"], R, tex)
    if sc is None or not sc["hole"]:
        return None
    rh, rg = sc["rh"], sc["rg"]
    rp = min(max(rg, rh + PANE_MARGIN), max(rh * 3.0, rh + PANE_MARGIN, 0.25), R * 0.8)
    ref = min(refs, key=lambda m: (abs(m["rg"] - rg), m["p95_tx"])) if refs else None
    if ref is not None:
        mat, s_ = sc["mat"], 1.0 / max(rg, rh + 0.01)
        Mu, Mv = ref["Mu"], ref["Mv"]

        def uvf(x, y):
            return (Mu[0] + (Mu[1] * x + Mu[2] * y) * s_, Mv[0] + (Mv[1] * x + Mv[2] * y) * s_)
    else:
        mat = flat_mat()

        def uvf(x, y):
            return (0.0, 0.0)
    # inner side, same lateral frame: rays from inside the tent towards the window
    n, c, back = op["n"], op["c"], sc["back"]
    t_in, _uv, _m, _g = ray_scan(bm, tree, c - n * back, n, sc["u"], sc["w"], rh + PANE_MARGIN + 0.05, sc["g"],
                                 back + 0.3)
    inner = dict(sc, o0=c - n * back, d=n, t=t_in)
    outer_info = build_disc(bm, sc, rh, rp, mat, uvf, nlay, seg=48, step=0.015)
    if outer_info is None:
        return None
    # the cap is only seen from inside the tent (or as a dark tunnel end): coarser
    cap_info = build_disc(bm, inner, rh, rh + PANE_MARGIN, mat, lambda x, y: uvf(-x, y), nlay, seg=32, step=0.025)
    g = sc["g"]
    return dict(centre=[round(x, 3) for x in (c + sc["u"] * (sc["ci"] * g) + sc["w"] * (sc["cj"] * g))],
                normal=[round(x, 3) for x in n], hole_r=round(rh, 3), painted_r=round(rg, 3), pane_r=round(rp, 3),
                margin_over_hole_mm=round((rp - rh) * 1000), cap_r=round(rh + PANE_MARGIN, 3),
                texture=("meshy painted window at %s" % ref["centre"]) if ref is not None else "flat window material",
                pane=outer_info, cap=cap_info, height=round(c.z, 3))


def find_painted_windows(bm, tree, tex, H, skip):
    """Closed windows: round grey discs Meshy painted on the fabric (inside a beige frame ring), found on a cylinder
    scan of the outer wall (1.5 cm grid) by sampling the base-colour texture. Returns [(centre, outward normal, area)]."""
    from mathutils.geometry import barycentric_transform
    uvl = bm.loops.layers.uv.active
    if tex is None or not uvl:
        return []
    bm.faces.ensure_lookup_table()
    hgt, wid = tex.shape[0], tex.shape[1]
    step, Rc = 0.015, 3.2
    nth = int(2 * math.pi * Rc / step)
    zs = [H * 0.15 + k * step for k in range(int(H * 0.7 / step))]
    cls, hit = {}, {}
    for zi, z in enumerate(zs):
        for ti in range(nth):
            a = 2 * math.pi * ti / nth
            d = Vector((-math.cos(a), -math.sin(a), 0))
            loc, nrm, fi, t = tree.ray_cast(Vector((0, 0, z)) - d * 8.0, d, 8.0)
            if loc is None:
                continue
            ls = bm.faces[fi].loops[:3]
            uvp = barycentric_transform(loc, *(lp.vert.co for lp in ls),
                                        *(Vector((lp[uvl].uv.x, lp[uvl].uv.y, 0)) for lp in ls))
            px = tex[min(hgt - 1, max(0, int(uvp.y % 1.0 * hgt))), min(wid - 1, max(0, int(uvp.x % 1.0 * wid)))]
            mx, mn = max(px[:3]), min(px[:3])
            sat = (mx - mn) / mx if mx > 0.05 else 0
            cls[(ti, zi)] = "g" if (mx > 0.05 and sat < 0.16) else ("y" if sat > 0.3 and px[0] > px[2] else "o")
            hit[(ti, zi)] = (loc, nrm if nrm.dot(-d) > 0 else -nrm)
    seen, out = set(), []
    for k0, c0 in cls.items():
        if c0 != "g" or k0 in seen:
            continue
        comp, stack, border = [k0], [k0], []
        seen.add(k0)
        while stack:
            ti, zi = stack.pop()
            for k in (((ti + 1) % nth, zi), ((ti - 1) % nth, zi), (ti, zi + 1), (ti, zi - 1)):
                ck = cls.get(k)
                if ck == "g" and k not in seen:
                    seen.add(k)
                    comp.append(k)
                    stack.append(k)
                elif ck != "g":
                    border.append(ck)
        area = len(comp) * step * step
        if not (0.02 < area < 0.2) or not border:      # r ~8-25 cm; whole grey panels are far bigger
            continue
        locs = [hit[k][0] for k in comp]
        c = sum(locs, Vector()) / len(locs)
        n = sum((hit[k][1] for k in comp), Vector()).normalized()
        ext = max((p_ - c).length for p_ in locs)
        round_ok = area / (math.pi * ext * ext) > 0.6             # a filled disc, not a strip or a panel edge
        log("  grey blob", [round(x, 2) for x in c], "area", round(area, 4), "round", round(area / (math.pi * ext * ext), 2),
            "kept" if round_ok else "rejected (not round)")
        if round_ok and all((c - s_).length > 0.3 for s_ in skip):
            out.append((c, n, round(area, 4)))
    return out


def window_material(rgb=(0.32, 0.33, 0.33)):
    """Fallback only (no usable painted window to borrow the texture from): flat frosted-plastic grey."""
    m = bpy.data.materials.new("window")
    m.use_nodes = True
    b = m.node_tree.nodes.get("Principled BSDF")
    b.inputs["Base Color"].default_value = (*rgb, 1.0)
    b.inputs["Roughness"].default_value = 0.45
    b.inputs["Metallic"].default_value = 0.0
    m.use_backface_culling = False                      # glTF doubleSided: the pane reads from inside too
    m.diffuse_color = (*rgb, 1.0)
    return m


def roughness_mm(me):
    """Mean distance of a vertex from its neighbours' average, along its normal (lumpiness at triangle scale)."""
    bm = bmesh.new()
    bm.from_mesh(me)
    bm.normal_update()
    s = n = 0
    for v in bm.verts:
        nb = [e.other_vert(v).co for e in v.link_edges]
        if len(nb) >= 3:
            s += abs((v.co - sum(nb, Vector()) / len(nb)).dot(v.normal))
            n += 1
    bm.free()
    return round(s / max(n, 1) * 1000, 2)


def shell_poke_pct(bm):
    """% of view rays (from all round and above, aimed at random points in the bounds) whose first hit is a
    back face with a front face right behind it = an inner wall poking out through the outer wall. Same test the
    2026-11-19 review used; Meshy's raw tents score 0-4 % (the kitchen's open sides show inner walls legitimately)."""
    import random
    bm.normal_update()
    bm.faces.ensure_lookup_table()
    tree = BVHTree.FromBMesh(bm)
    lo = Vector((min(v.co[i] for v in bm.verts) for i in range(3)))
    hi = Vector((max(v.co[i] for v in bm.verts) for i in range(3)))
    R = (hi - lo).length
    rnd = random.Random(1)
    tot = poke = 0
    ga, N = math.pi * (3 - math.sqrt(5)), 4000
    for i in range(N):
        z = 1 - 2 * (i + 0.5) / N
        if z < -0.1:
            continue
        r = math.sqrt(1 - z * z)
        d0 = Vector((r * math.cos(ga * i), r * math.sin(ga * i), z))
        for _k in range(3):
            tgt = Vector((rnd.uniform(lo.x, hi.x), rnd.uniform(lo.y, hi.y), rnd.uniform(lo.z, hi.z)))
            o = tgt + d0 * R * 2
            d = (tgt - o).normalized()
            loc, _n, fi, _t = tree.ray_cast(o, d, R * 4)
            if loc is None:
                continue
            tot += 1
            if bm.faces[fi].normal.dot(d) > 0:
                l2 = tree.ray_cast(loc + d * 1e-5, d, 0.05 * R)
                if l2[0] is not None and bm.faces[l2[2]].normal.dot(d) < 0:
                    poke += 1
    return round(100 * poke / max(tot, 1), 3)


def fabric_smooth(ob, ident):
    """Gentle smooth of the tent fabric (see SMOOTH_FABRIC). Each vertex moves only along its ORIGINAL normal
    towards its neighbours' average, so lumps flatten but nothing slides sideways (the texture stays where Meshy
    baked it). The move is limited to a fraction of the shell thickness on that side and fades in from open seams,
    so the two walls of Meshy's thick fabric shell never cross. Self-checked with shell_poke_pct: if the result pokes
    through more than SHELL_POKE_MAX above the unsmoothed mesh, the limit is halved (then smoothing is skipped).
    Blender's Laplacian Smooth with 'volume preserve' inflated the tents by 6 %; plain Smooth smeared the texture."""
    me = ob.data
    r0 = roughness_mm(me)
    lo0, hi0 = bounds(me)
    cfg = SMOOTH_FABRIC.get(ident, SMOOTH_FABRIC["default"])
    bm = bmesh.new()
    bm.from_mesh(me)
    bm.normal_update()
    bm.verts.ensure_lookup_table()
    poke0 = shell_poke_pct(bm)
    hard = {v for v in bm.verts if v.is_boundary or not v.is_manifold or len(v.link_edges) < 3}
    if cfg["bend"] is not None:
        lim = math.radians(cfg["bend"])
        hard |= {v for e in bm.edges if not e.is_manifold or e.calc_face_angle(0) > lim for v in e.verts}
    # fade-in weight: 0 on pinned vertices, k/ramp on the k-th edge ring away from them, 1 beyond
    ramp = cfg["ramp"]
    ring = {v: 0 for v in hard}
    front = list(hard)
    for k in range(1, ramp + 1):
        nxt = []
        for v in front:
            for e in v.link_edges:
                o = e.other_vert(v)
                if o not in ring:
                    ring[o] = k
                    nxt.append(o)
        front = nxt
    soft = [v for v in bm.verts if v not in hard]
    wgt = [ring.get(v, ramp) / ramp for v in soft]
    co0 = [v.co.copy() for v in soft]
    n0 = [v.normal.copy() for v in soft]
    # room on each side: distance along -n / +n to the next surface (the opposite wall of the shell), capped at 5 cm
    tree = BVHTree.FromBMesh(bm)
    room_in, room_out = [], []
    for c, n in zip(co0, n0):
        hi_ = tree.ray_cast(c - n * 1e-4, -n, 0.05)
        ho_ = tree.ray_cast(c + n * 1e-4, n, 0.05)
        room_in.append(hi_[3] if hi_[0] is not None else 0.05)
        room_out.append(ho_[3] if ho_[0] is not None else 0.05)
    tried = []
    for scale in (1.0, 0.5, 0.0):
        sh = cfg["shell"] * scale
        disp = [0.0] * len(soft)
        for v, c in zip(soft, co0):
            v.co = c
        if sh > 0:
            for _ in range(cfg["iterations"]):
                new = []
                for idx, v in enumerate(soft):
                    avg = sum((e.other_vert(v).co for e in v.link_edges), Vector()) / len(v.link_edges)
                    dd = disp[idx] + (avg - v.co).dot(n0[idx]) * cfg["factor"] * wgt[idx]
                    new.append(min(max(dd, -sh * room_in[idx]), sh * room_out[idx]))
                disp = new
                for idx, v in enumerate(soft):
                    v.co = co0[idx] + n0[idx] * disp[idx]
        poke1 = shell_poke_pct(bm) if sh > 0 else poke0
        tried.append([round(sh, 3), poke1])
        if poke1 <= poke0 + SHELL_POKE_MAX:
            break
    bm.to_mesh(me)
    nv = len(bm.verts)
    bm.free()
    me.update()
    lo1, hi1 = bounds(me)
    return dict(fabric_verts_pct=round(100 * len(soft) / nv, 1), rough_mm=[r0, roughness_mm(me)],
                shell_limit=round(sh, 3), shell_poke_pct=[poke0, poke1], tried_limit_poke=tried,
                max_move_mm=round(max((abs(x) for x in disp), default=0.0) * 1000, 1),
                size_change_pct=[round(((hi1 - lo1)[i] / (hi0 - lo0)[i] - 1) * 100, 2) for i in range(3)])


def keep_meshy_normals(ob):
    """Store Meshy's normals per face corner. They were baked from Meshy's high-res model, so they are smoother than
    normals computed from the lumpy 30k remesh (recomputed normals expose every lump as a facet)."""
    me = ob.data
    flat = [c for cn in me.corner_normals for c in cn.vector]
    at = me.attributes.new("meshy_n", "FLOAT_VECTOR", "CORNER")
    at.data.foreach_set("vector", flat)


def restore_meshy_normals(ob):
    """Meshy's normals back as custom normals (turned round on faces whose winding recalc_normals corrected); corners
    that are new (filled holes, window panes) get Blender's own normal."""
    me = ob.data
    bpy.context.view_layer.objects.active = ob
    ob.select_set(True)
    bpy.ops.object.shade_smooth(keep_sharp_edges=False)
    at = me.attributes.get("meshy_n")
    flat = [0.0] * (len(me.loops) * 3)
    at.data.foreach_get("vector", flat)
    fk = [0] * len(me.polygons)
    if me.attributes.get("flipkept"):
        me.attributes["flipkept"].data.foreach_get("value", fk)
        me.attributes.remove(me.attributes["flipkept"])
    out, fixed = [], 0
    for p_ in me.polygons:
        for li in p_.loop_indices:
            nv = Vector(flat[li * 3:li * 3 + 3])
            if nv.length < 0.5:
                out.append((0.0, 0.0, 0.0))
                continue
            if fk[p_.index] and nv.dot(p_.normal) < 0:
                nv.negate()
                fixed += 1
            out.append(tuple(nv.normalized()))
    me.attributes.remove(me.attributes["meshy_n"])
    me.normals_split_custom_set(out)
    return fixed


def smooth_by_angle(ob, angle):
    bpy.context.view_layer.objects.active = ob
    ob.select_set(True)
    try:
        bpy.ops.mesh.customdata_custom_splitnormals_clear()
    except Exception as e:  # noqa: BLE001
        log("  no custom normals to clear", e)
    if getattr(ob.data, "has_custom_normals", False):
        for nm in ("custom_normal",):
            if nm in ob.data.attributes:
                ob.data.attributes.remove(ob.data.attributes[nm])
    bpy.ops.object.shade_smooth_by_angle(angle=math.radians(angle), keep_sharp_edges=False)


def prep(a, report):
    i = a["id"]
    ob = import_join(f"{ROOT}/Downloads/ai_assets/{i}/{i}.glb")
    tris0 = sum(len(p.vertices) - 2 for p in ob.data.polygons)
    scale_to_size(ob, a["size"])
    if NORMALS == "meshy":
        keep_meshy_normals(ob)
    lo, hi = bounds(ob.data)
    maxdim, H = max(hi - lo), hi.z - lo.z
    r = dict(tris_in=tris0)
    bm = bmesh.new()
    bm.from_mesh(ob.data)
    nv = len(bm.verts)
    bmesh.ops.remove_doubles(bm, verts=bm.verts[:], dist=min(1e-3, max(1e-5, 1e-4 * maxdim)))
    r["welded_verts"] = nv - len(bm.verts)
    r["holes_filled"], r["holes_kept_big"], r["open_slits"] = fill_small_holes(bm, maxdim)
    r["normals_flipped_kept_reverted"] = recalc_normals(bm)
    bm.to_mesh(ob.data)
    bm.free()
    if i in TENTS:
        r["fabric_smooth"] = fabric_smooth(ob, i)
    if i in WINDOW_PROPS or i in SCAN_ONLY:
        bm = bmesh.new()
        bm.from_mesh(ob.data)
        bm.normal_update()
        tree, ops = find_openings(bm, H, maxdim)
        tex = base_color_image(ob) if i in WINDOW_PROPS else None
        nlay = bm.loops.layers.float_vector.get("meshy_n")
        win = []

        def flat_mat():           # fallback 'window' material, created once on demand
            if not win:
                ob.data.materials.append(window_material())
                win.append(len(ob.data.materials) - 1)
            return win[0]
        r["openings"] = []
        smalls = []
        for op in ops:
            rec = dict(rays=op["rays"], radius=round(op["r"], 3), height=round(op["c"].z, 2),
                       dir=[round(x, 2) for x in op["n"]])
            small = op["r"] < WINDOW_MAX * maxdim and op["low"] > 0.05 * H and op["rays"] >= 2
            if i in WINDOW_PROPS and small:
                smalls.append((op, rec))
            else:
                rec["action"] = ("kept: too big or at the ground (door/open side/base)" if not small
                                 else "kept: scan only, not a window prop")
            r["openings"].append(rec)
        if i in WINDOW_PROPS:
            # closed windows Meshy painted on solid fabric stay as they are; the best one lends its texture
            # (UV map) to the panes, so plugged and closed windows look the same
            refs = []
            r["painted_windows"] = []
            for c, n, area in find_painted_windows(bm, tree, tex, H, [op["c"] for op, _ in smalls]):
                pw = dict(centre=[round(x, 3) for x in c], area_m2=area, action="kept: Meshy's painted closed window")
                sc = window_scan(bm, tree, c, n, 0.35, tex)
                if sc is not None and not sc["hole"] and tex is not None:
                    m = window_uv_map(sc, tex.shape[1])
                    if m is not None:
                        pw["uv_fit"] = {k: m[k] for k in ("rg", "med_tx", "p95_tx", "fit_frac", "ok")}
                        if m["ok"]:
                            refs.append(m)
                r["painted_windows"].append(pw)
            r["pane_texture_refs"] = [m["centre"] for m in refs] or "flat window material (no clean painted window)"
            for op, rec in smalls:
                rec["plug"] = plug_window(bm, tree, op, tex, refs, nlay, flat_mat)
                rec["action"] = "plugged (window)" if rec["plug"] else "skipped (no through-hole on the grid)"
        r["windows_plugged"] = sum(1 for o in r["openings"] if o.get("plug"))
        bm.to_mesh(ob.data)
        bm.free()
    # re-ground after smoothing / plugging: bottom at z = 0, centred in x/y
    lo, hi = bounds(ob.data)
    off = Vector(((lo.x + hi.x) / 2, (lo.y + hi.y) / 2, lo.z))
    for v in ob.data.vertices:
        v.co -= off
    ob.data.update()
    if NORMALS == "meshy":
        r["normals"] = "meshy"
        r["normals_turned"] = restore_meshy_normals(ob)
    else:
        r["normals"] = "smooth_by_angle %d" % SMOOTH_ANGLE["soft" if i in SOFT else "hard"]
        smooth_by_angle(ob, SMOOTH_ANGLE["soft" if i in SOFT else "hard"])
    ob.name = ob.data.name = i
    lo, hi = bounds(ob.data)
    r["dims_m"] = [round(d, 3) for d in (hi - lo)]
    r["tris_out"] = sum(len(p.vertices) - 2 for p in ob.data.polygons)
    r["materials"] = [m.name for m in ob.data.materials]
    bpy.ops.export_scene.gltf(filepath=f"{ROOT}/Art/Meshy/{i}.glb", export_format="GLB", use_selection=False,
                              export_apply=True, export_yup=True)
    report[i] = r
    log(i, json.dumps(r))


if __name__ == "__main__":
    want = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
    os.makedirs(f"{ROOT}/Art/Meshy", exist_ok=True)
    rp = f"{ROOT}/Art/Meshy/prep_report.json"
    try:
        report = json.load(open(rp))
    except Exception:  # noqa: BLE001
        report = {}
    for a in ASSETS:
        if a["kind"] == "prop" and (not want or a["id"] in want):
            prep(a, report)
            json.dump(report, open(rp, "w"), indent=1)
