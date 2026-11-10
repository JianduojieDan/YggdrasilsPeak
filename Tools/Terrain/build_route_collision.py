# Route-aware collision for the route detail tiles (RAM: the 185 full 2 m collision meshes were 1.7 GB of Chaos trimesh).
# The player walks the route; the rest of the 34 km map is scenery. So the collision keeps every 2 m sample near the
# route and gets coarser away from it, using an adaptive right-triangle mesh (Martini / RTIN): a triangle is only split
# where the flat triangle would be further from the real ground than the allowed error at that spot.
#   allowed error: WALK_M from the route -> EXACT_ERR (2 m grid kept, ~lossless)
#                  WALK_M .. FAR_M        -> ramps up to FAR_ERR
# Tile borders and the mesh's outer edge keep every sample, so neighbouring tiles still join without gaps.
# Reads  Saved/DetailTiles/route/<tile>_col.obj (the 2 m grid written by build_route_terrain.py)
# Writes Saved/DetailTiles/route_col/<tile>_col.obj + stats.json
# Usage: python build_route_collision.py [walk_m exact_err far_m far_err]
import sys, os, json, numpy as np
from scipy.spatial import cKDTree

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
SRC = os.path.join(ROOT, 'Saved/DetailTiles/route'); OUT = os.path.join(ROOT, 'Saved/DetailTiles/route_col')
os.makedirs(OUT, exist_ok=True)
a = [float(v) for v in sys.argv[1:5]] if len(sys.argv) >= 5 else []
WALK_M, EXACT_ERR, FAR_M, FAR_ERR = a if a else (100.0, 0.02, 400.0, 1.5)
ROCK_U = 0.5                        # rock weight (UV0.x of the tiles) above which a collision face is rock
TILE, STEP = 500, 2                 # 251 x 251 samples per tile
N = TILE // STEP + 1; SIZE = 257; TS = SIZE - 1   # RTIN needs 2^k + 1; the extra rows/cols are padding, never emitted

route = np.array([p[:2] for l in json.load(open(os.path.join(ROOT, 'Tools/RoutePlanning/route.json'))) for p in l['pts']]) / 100.0
dense = np.concatenate([np.linspace(p, q, max(2, int(np.hypot(*(q - p)) / 3)), endpoint=False) for p, q in zip(route[:-1], route[1:])] + [route[-1:]])
info = json.load(open(os.path.join(ROOT, 'Saved/Compare/ygg_info.json')))
extra = [np.array(r) / 100.0 for r in info.get('ropes', []) if len(r) > 1]
pts = [dense] + [np.concatenate([np.linspace(p, q, max(2, int(np.hypot(*(q - p)))), endpoint=False) for p, q in zip(r[:-1], r[1:])]) for r in extra]
pts.append(np.array([(x / 100, y / 100) for _, x, y in info['camps']]))
walk_tree = cKDTree(np.concatenate(pts))

# ---- RTIN triangle table (shared by all tiles): coords of a, b for every triangle in the implicit binary tree
nT = TS * TS * 2 - 2; nParent = nT - TS * TS
tri = np.zeros((nT, 4), np.int32)
for i in range(nT):
    tid = i + 2; ax = ay = bx = by = cx = cy = 0
    if tid & 1: bx = by = cx = TS
    else: ax = ay = cy = TS
    tid >>= 1
    while tid > 1:
        mx, my = (ax + bx) >> 1, (ay + by) >> 1
        if tid & 1: bx, by, ax, ay = ax, ay, cx, cy
        else: ax, ay, bx, by = bx, by, cx, cy
        cx, cy = mx, my; tid >>= 1
    tri[i] = ax, ay, bx, by
level = np.array([(i + 2).bit_length() for i in range(nT)])
ax_, ay_, bx_, by_ = tri.T
mx_, my_ = (ax_ + bx_) >> 1, (ay_ + by_) >> 1
cx_, cy_ = mx_ + my_ - ay_, my_ + ax_ - mx_
mid = my_ * SIZE + mx_
lch = ((ay_ + cy_) >> 1) * SIZE + ((ax_ + cx_) >> 1)
rch = ((by_ + cy_) >> 1) * SIZE + ((bx_ + cx_) >> 1)
levels = sorted(set(level.tolist()), reverse=True)
by_level = {L: np.nonzero(level == L)[0] for L in levels}

def rtin_errors(H, allow, force):
    """Normalised error per vertex (>1 = must be in the mesh). H, allow, force: SIZE x SIZE."""
    h = H.ravel(); err = np.zeros(SIZE * SIZE)
    own = np.abs((h[ay_ * SIZE + ax_] + h[by_ * SIZE + bx_]) * 0.5 - h[mid]) / allow.ravel()[mid]
    own[force.ravel()[mid]] = np.inf
    for L in levels:                                   # deepest first: children are always one level deeper
        ix = by_level[L]
        v = own[ix]
        par = ix[ix < nParent]
        if len(par):
            vp = np.maximum(err[lch[par]], err[rch[par]])
            v = v.copy(); v[ix < nParent] = np.maximum(v[ix < nParent], vp)
        np.maximum.at(err, mid[ix], v)
    return err

def rtin_mesh(err):
    out = []; stack = [(0, 0, TS, TS, TS, 0), (TS, TS, 0, 0, 0, TS)]
    while stack:
        ax, ay, bx, by, cx, cy = stack.pop()
        mx, my = (ax + bx) >> 1, (ay + by) >> 1
        if abs(ax - cx) + abs(ay - cy) > 1 and err[my * SIZE + mx] > 1.0:
            stack.append((cx, cy, ax, ay, mx, my)); stack.append((bx, by, cx, cy, mx, my))
        else:
            out.append((ay * SIZE + ax, by * SIZE + bx, cy * SIZE + cx))
    return np.array(out, np.int64)

def read_grid(path):
    H = np.full((N, N), np.nan); U = np.zeros((N, N))
    vs, us = [], []
    with open(path) as f:
        for line in f:
            if line.startswith('v '): vs.append(line.split()[1:4])
            elif line.startswith('vt '): us.append(float(line.split()[1]))
    v = np.array(vs, float)
    i = np.rint((v[:, 0] / 100 + TILE / 2) / STEP).astype(int)          # column: +x
    j = np.rint((TILE / 2 - v[:, 1] / 100) / STEP).astype(int)          # row: world Y = piv - y_obj
    H[j, i] = v[:, 2]; U[j, i] = us[:len(v)]
    return H, U

def write_obj(path, H, U, tris, used):
    jj, ii = np.nonzero(used)
    idx = np.full(used.shape, -1, np.int64); idx[jj, ii] = np.arange(len(jj)) + 1
    x = (ii * STEP - TILE / 2) * 100.0; y = (TILE / 2 - jj * STEP) * 100.0
    with open(path, 'w') as f:
        f.write(''.join('v %.1f %.1f %.1f\n' % t for t in zip(x, y, H[jj, ii])))
        f.write(''.join('vt %.4f 0\n' % u for u in U[jj, ii]))
        r, c = np.divmod(tris, SIZE)
        A, B, C = idx[r[:, 0], c[:, 0]], idx[r[:, 1], c[:, 1]], idx[r[:, 2], c[:, 2]]
        # two material groups: the game reads rock vs snow from the face's physical material (slot 0 snow, slot 1 rock)
        rock = (U[r[:, 0], c[:, 0]] + U[r[:, 1], c[:, 1]] + U[r[:, 2], c[:, 2]]) / 3.0 > ROCK_U
        for name, sel in (('a_snow', ~rock), ('b_rock', rock)):
            if sel.any():
                f.write('usemtl %s\n' % name)
                f.write(''.join('f %d/%d %d/%d %d/%d\n' % (p, p, q, q, s, s) for p, q, s in zip(A[sel], B[sel], C[sel])))
        return int(rock.sum()), int((~rock).sum())

if __name__ == '__main__':
    tiles = json.load(open(os.path.join(SRC, 'tiles.json')))
    stats = {}; before = after = 0
    for k, (name, piv) in enumerate(sorted(tiles.items())):
        H0, U0 = read_grid(os.path.join(SRC, name + '_col.obj'))
        have = ~np.isnan(H0)
        H = np.zeros((SIZE, SIZE)); H[:N, :N] = np.where(have, H0, 0.0) / 100.0      # metres
        U = np.zeros((SIZE, SIZE)); U[:N, :N] = U0
        # world position of each sample (generator space, metres) -> distance to the walkable route
        jj, ii = np.mgrid[0:N, 0:N]
        X = piv[0] - TILE / 2 + ii * STEP; Y = piv[1] - TILE / 2 + jj * STEP
        d = walk_tree.query(np.c_[X.ravel(), Y.ravel()])[0].reshape(N, N)
        t = np.clip((d - WALK_M) / (FAR_M - WALK_M), 0, 1); t = t * t * (3 - 2 * t)
        allow = np.full((SIZE, SIZE), 1e9); allow[:N, :N] = EXACT_ERR + (FAR_ERR - EXACT_ERR) * t
        force = np.zeros((SIZE, SIZE), bool)
        force[N:, :] = True; force[:, N:] = True                                      # padding
        border = np.zeros((N, N), bool); border[0, :] = border[-1, :] = border[:, 0] = border[:, -1] = True
        hole = ~have
        near_hole = hole.copy()
        near_hole[1:, :] |= hole[:-1, :]; near_hole[:-1, :] |= hole[1:, :]; near_hole[:, 1:] |= hole[:, :-1]; near_hole[:, :-1] |= hole[:, 1:]
        f2 = near_hole.copy()
        f2[1:, :] |= near_hole[:-1, :]; f2[:-1, :] |= near_hole[1:, :]; f2[:, 1:] |= near_hole[:, :-1]; f2[:, :-1] |= near_hole[:, 1:]
        force[:N, :N] = border | f2
        err = rtin_errors(H, allow, force)
        tris = rtin_mesh(err)
        r, c = np.divmod(tris, SIZE)
        inside = (r < N).all(1) & (c < N).all(1)
        tris = tris[inside]; r, c = r[inside], c[inside]
        ok = have[r, c].all(1); tris = tris[ok]
        used = np.zeros((N, N), bool); rr, cc = np.divmod(tris.ravel(), SIZE); used[rr, cc] = True
        Hc = np.zeros((N, N)); Hc[:N, :N] = H0                                         # original cm values
        nrock, nsnow = write_obj(os.path.join(OUT, name + '_col.obj'), np.where(have, H0, 0), U0, tris, used)
        if nrock and not nsnow: print('  ALL ROCK', name)
        n0 = int((have[:-1, :-1] & have[1:, :-1] & have[:-1, 1:] & have[1:, 1:]).sum()) * 2
        stats[name] = dict(before=n0, after=len(tris), rock=nrock, near_route_m=float(d.min()))
        before += n0; after += len(tris)
        print(f'{k + 1}/{len(tiles)} {name} tris {n0} -> {len(tris)} ({100 * len(tris) / max(n0, 1):.0f}%) route {d.min():.0f} m', flush=True)
    json.dump(dict(params=[WALK_M, EXACT_ERR, FAR_M, FAR_ERR], before=before, after=after, tiles=stats), open(os.path.join(OUT, 'stats.json'), 'w'), indent=0)
    print(f'TOTAL {before} -> {after} triangles ({100 * after / before:.1f}%)')
