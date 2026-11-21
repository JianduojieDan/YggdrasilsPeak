# Removes the Meshy head (face, hair, hat/beanie, bare neck skin) from Meshy-rigged characters so a MetaHuman head
# can be attached to the neck/head bones in UE. Keeps every piece of clothing (collars, hoods, fleece necks, scarves).
#   blender --background --factory-startup --python Tools/Blender/meshy_headless.py -- [<id> ...] [--debug] [--no-export]
# (set TEMP/TMP to Saved/tmp_blender). Input  Downloads/ai_assets/<id>/<id>_rigged.glb (Meshy "ue5_mannequin" rig),
# output Downloads/ai_assets/<id>/<id>_headless.glb + previews in Art/Blender/Preview/headless_<id>_*.png.
#
# How the head is found. Meshy characters are ONE closed shell (head, collar and hood fused, ~26k welded vertices),
# so the cut is a per-vertex decision:
#   1. hard head: above the head bone + HARD_DZ and within R_HEAD of the neck axis (face, hat, hair on the skull);
#   2. ambiguous zone: between the base-of-neck plane and the hard head, within R_AMB of the axis (neck, collars,
#      hoods, hair resting on the hood). Each vertex's texture colour (CIE Lab, blurred base colour) is classified by
#      k-nearest-neighbours against two sample sets taken from the same character: "head" (hat, hair, face above the
#      collar) and "cloth" (shoulders, collar bases, hood bottoms below the neck, skin-coloured samples excluded);
#   3. a graph cut (unary = colour class x spatial prior, pairwise = colour similarity of neighbours) places the cut
#      on colour edges; bare skin near the neck axis that connects to the face is always cut down to the base plane;
#      deletions not connected to the head are undone, floating kept pieces and one-triangle strips are deleted;
#   4. hair that lay fused on the coat (hair_box) leaves a hole: cap_box closes it with a flat cap made of copies
#      of the hole's boundary vertices (same positions and skin weights) using one sampled coat UV.
# The armature, bone rest pose, hierarchy, skin weights of kept vertices, UVs, normals and materials are untouched;
# nothing is decimated, remeshed or smoothed and the neck opening stays open. Absolute corner normals are restored
# after the deletion. The script re-reads input and output glTF and checks skeleton, normals, weights, images.
import bpy, bmesh, sys, os, json, struct, math
import numpy as np
from mathutils import Vector

ROOT = "F:/UE_project/YggdrasilsPeak"
AI = f"{ROOT}/Downloads/ai_assets"
PREV = f"{ROOT}/Art/Blender/Preview"
IDS = ["marek_ai", "nima_ai", "elena_ai", "pema_body"]

DEFAULT = dict(
    axis_dy=0.0,      # neck axis = vertical line through the head bone at (x, y + axis_dy)
    hard_dz=0.10,     # hard head (graph-cut seeds): z > head bone z + hard_dz ...
    r_head=0.12,      # ... and closer than this to the neck axis
    r_amb=0.17,       # ambiguous zone radius (neck skin, collars, hood tops, hair resting on the hood)
    base_front=0.0,   # base-of-neck plane: neck_01 z + base_front at the front (dy = -0.06) ...
    base_back=0.03,   # ... and neck_01 z + base_back at the back (dy = +0.06); nothing below it is deleted ...
    hair_box=None,    # ... except inside this box ((x0,x1),(y0,y1),(z0,z1)) relative to the head bone, where hair
    hair_seed=None,   # lies on the shoulders/hood; hair_seed = a box that is surely hair (extra colour samples)
    cap_box=None,     # box (relative to the head bone) where the hole left by that hair is closed with a flat cap
    r_skin=0.095,     # spatial prior: skin-coloured vertices closer than this to the axis lean "head" (at the
    r_face=0.14,      # base plane, growing to r_face at the jaw) ...
    r_skin_force=0.08,   # ... and closer than this (+ the same growth) are always deleted (bare neck to the base)
    skin_grow=0.4,
    hair_grow=0.5,    # hair probability for hair inside hair_box grown from the skull
    tidy=3,           # majority passes on the cut line    # skin probability for the forced neck-skin region grown down from the face
    skin_dist=10.0,   # Lab distance (L halved) from the face's median skin colour that counts as "near skin"
    knn=9,
    alpha=0.6,        # graph cut: unary weight ...
    lam=1.0,          # ... pairwise weight ...
    sigma=9.0,        # ... and colour-difference scale (Lab, L halved) of the pairwise term
)
CFG = {
    "marek_ai": dict(),
    "nima_ai": dict(),
    "elena_ai": dict(hair_box=((-0.09, 0.22), (-0.07, 0.20), (-0.14, 0.07)),
                     hair_seed=((0.07, 0.16), (-0.03, 0.12), (-0.01, 0.05)),
                     cap_box=((0.075, 0.25), (-0.12, 0.15), (-0.16, 0.02))),
    "pema_body": dict(hair_box=((-0.11, 0.11), (0.03, 0.26), (-0.07, 0.22)),        # bun resting on the hood
                      hair_seed=((-0.06, 0.06), (0.10, 0.22), (0.05, 0.14)),
                      cap_box=((-0.07, 0.06), (0.093, 0.2), (-0.1, 0.05))),
}

ARGS = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
DEBUG = "--debug" in ARGS
EXPORT = "--no-export" not in ARGS
SEL = [a for a in ARGS if not a.startswith("--")] or IDS


def log(*a):
    print("HEADLESS", *a, flush=True)


# ---------------------------------------------------------------- loading and mesh data
def load(gid):
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.ops.import_scene.gltf(filepath=f"{AI}/{gid}/{gid}_rigged.glb")
    arm = next(o for o in bpy.data.objects if o.type == 'ARMATURE')
    ob = max((o for o in bpy.data.objects if o.type == 'MESH'), key=lambda o: len(o.data.vertices))
    for o in list(bpy.data.objects):          # bone-shape icosphere etc. (display only, never exported)
        if o not in (arm, ob):
            bpy.data.objects.remove(o, do_unlink=True)
    assert ob.matrix_world.is_identity and arm.matrix_world.is_identity, "expected identity transforms"
    return arm, ob


def base_color_image(ob):
    for nd in ob.data.materials[0].node_tree.nodes:
        if nd.type == 'BSDF_PRINCIPLED':
            l = nd.inputs['Base Color'].links
            if l:
                return l[0].from_node.image
    raise RuntimeError("no base colour texture")


def srgb_to_lab(c):
    c = np.where(c <= 0.04045, c / 12.92, ((c + 0.055) / 1.055) ** 2.4)
    M = np.array([[0.4124, 0.3576, 0.1805], [0.2126, 0.7152, 0.0722], [0.0193, 0.1192, 0.9505]])
    xyz = c @ M.T / np.array([0.9505, 1.0, 1.089])
    f = np.where(xyz > 0.008856, np.cbrt(xyz), 7.787 * xyz + 16 / 116)
    return np.stack([116 * f[:, 1] - 16, 500 * (f[:, 0] - f[:, 1]), 200 * (f[:, 1] - f[:, 2])], 1)


class MeshData:
    def __init__(self, arm, ob):
        me = ob.data
        self.n = n = len(me.vertices)
        co = np.empty(n * 3); me.vertices.foreach_get("co", co); self.co = co.reshape(n, 3)
        key = np.round(self.co / 1e-5).astype(np.int64)
        _, w = np.unique(key, axis=0, return_inverse=True)
        self.weld = w.ravel(); self.nw = nw = self.weld.max() + 1
        self.wco = np.zeros((nw, 3)); self.wco[self.weld] = self.co
        ev = np.empty(len(me.edges) * 2, np.int64); me.edges.foreach_get("vertices", ev)
        e = self.weld[ev.reshape(-1, 2)]
        e = np.sort(e[e[:, 0] != e[:, 1]], 1)
        self.edges = np.unique(e, axis=0)
        self.deg = np.bincount(self.edges.ravel(), minlength=nw)
        # texture colour per vertex: base colour blurred to 512 px (box 4x4), bilinear at the vertex UV
        img = base_color_image(ob)
        W, H = img.size
        px = np.empty(W * H * 4, np.float32); img.pixels.foreach_get(px)
        px = px.reshape(H, W, 4)[..., :3]
        k = max(1, W // 512)
        small = px[:H // k * k, :W // k * k].reshape(H // k, k, W // k, k, 3).mean((1, 3))
        uvd = me.uv_layers.active.data
        luv = np.empty(len(me.loops) * 2); uvd.foreach_get("uv", luv); luv = luv.reshape(-1, 2)
        lv = np.empty(len(me.loops), np.int64); me.loops.foreach_get("vertex_index", lv)
        vuv = np.zeros((n, 2)); vuv[lv] = luv
        hs, ws = small.shape[:2]
        x = np.clip((vuv[:, 0] % 1.0) * ws - 0.5, 0, ws - 1.001); y = np.clip((vuv[:, 1] % 1.0) * hs - 0.5, 0, hs - 1.001)
        x0 = x.astype(int); y0 = y.astype(int); fx = (x - x0)[:, None]; fy = (y - y0)[:, None]
        col = (small[y0, x0] * (1 - fx) * (1 - fy) + small[y0, x0 + 1] * fx * (1 - fy)
               + small[y0 + 1, x0] * (1 - fx) * fy + small[y0 + 1, x0 + 1] * fx * fy)
        self.vcol = col
        self.small = small
        wc = np.zeros((nw, 3)); np.add.at(wc, self.weld, col)
        self.wcol = wc / np.bincount(self.weld, minlength=nw)[:, None]
        self.wlab = srgb_to_lab(np.clip(self.wcol, 0, 1))
        # head / neck weights per welded vertex
        bones = arm.data.bones
        headset = {b.name for b in bones['head'].children_recursive} | {'head'}
        gname = {g.index: g.name for g in ob.vertex_groups}
        wh = np.zeros(n); wn = np.zeros(n); wha = np.zeros(n)
        handp = ('hand_', 'index_', 'middle_', 'ring_', 'pinky_', 'thumb_')
        for v in me.vertices:
            for g in v.groups:
                nm = gname[g.group]
                if nm in headset: wh[v.index] += g.weight
                elif nm.startswith('neck'): wn[v.index] += g.weight
                elif nm.startswith(handp): wha[v.index] += g.weight
        self.whead = np.zeros(nw); self.whead[self.weld] = wh
        self.wneck = np.zeros(nw); self.wneck[self.weld] = wn
        self.whand = np.zeros(nw); self.whand[self.weld] = wha


def _sample(self, uv):
    hs, ws = self.small.shape[:2]
    x = np.clip((uv[:, 0] % 1.0) * ws - 0.5, 0, ws - 1.001); y = np.clip((uv[:, 1] % 1.0) * hs - 0.5, 0, hs - 1.001)
    x0 = x.astype(int); y0 = y.astype(int); fx = (x - x0)[:, None]; fy = (y - y0)[:, None]
    sm = self.small
    return (sm[y0, x0] * (1 - fx) * (1 - fy) + sm[y0, x0 + 1] * fx * (1 - fy)
            + sm[y0 + 1, x0] * (1 - fx) * fy + sm[y0 + 1, x0 + 1] * fx * fy)


MeshData.sample = _sample


def neighbour_sum(md, val):
    s = np.zeros(md.nw)
    np.add.at(s, md.edges[:, 0], val[md.edges[:, 1]])
    np.add.at(s, md.edges[:, 1], val[md.edges[:, 0]])
    return s


def components(md, mask):
    """Connected components of the welded graph restricted to mask; returns label per vertex (-1 outside)."""
    par = np.arange(md.nw)

    def find(a):
        r = a
        while par[r] != r:
            r = par[r]
        while par[a] != r:
            par[a], a = r, par[a]
        return r
    e = md.edges[mask[md.edges[:, 0]] & mask[md.edges[:, 1]]]
    for a, b in e:
        ra, rb = find(a), find(b)
        if ra != rb:
            par[ra] = rb
    lab = np.array([find(i) for i in range(md.nw)])
    lab[~mask] = -1
    return lab


# ---------------------------------------------------------------- classification
def maxflow_mincut(m, arcs, src_cap, snk_cap):
    """Dinic max-flow on m nodes + source + sink. arcs: (a, b, cap) undirected; returns bool array: node on the
    source side of the minimum cut."""
    S, T = m, m + 1
    to, cap, adj = [], [], [[] for _ in range(m + 2)]

    def add(a, b, c1, c2):
        adj[a].append(len(to)); to.append(b); cap.append(c1)
        adj[b].append(len(to)); to.append(a); cap.append(c2)
    for a, b, c in arcs:
        add(a, b, c, c)
    for i in range(m):
        if src_cap[i] > 0: add(S, i, float(src_cap[i]), 0.0)
        if snk_cap[i] > 0: add(i, T, float(snk_cap[i]), 0.0)
    eps = 1e-9
    while True:
        level = [-1] * (m + 2); level[S] = 0; q = [S]
        for u in q:
            for e in adj[u]:
                if cap[e] > eps and level[to[e]] < 0:
                    level[to[e]] = level[u] + 1; q.append(to[e])
        if level[T] < 0:
            break
        it = [0] * (m + 2)
        while True:                       # find one augmenting path in the level graph (iterative DFS)
            path = []; u = S
            while u != T:
                advanced = False
                while it[u] < len(adj[u]):
                    e = adj[u][it[u]]
                    if cap[e] > eps and level[to[e]] == level[u] + 1:
                        path.append(e); u = to[e]; advanced = True; break
                    it[u] += 1
                if not advanced:
                    if u == S:
                        break
                    level[u] = -1
                    e = path.pop(); u = to[e ^ 1]; it[u] += 1
            if u != T:
                break
            f = min(cap[e] for e in path)
            for e in path:
                cap[e] -= f; cap[e ^ 1] += f
    seen = np.zeros(m + 2, bool); seen[S] = True; q = [S]
    for u in q:
        for e in adj[u]:
            if cap[e] > eps and not seen[to[e]]:
                seen[to[e]] = True; q.append(to[e])
    return seen[:m]


def knn_probs(lab, q, sets, k):
    """Class-balanced k-nearest-neighbour vote of the colours lab[q] against sample index sets -> (len(q), C)."""
    X = np.concatenate([lab[s_] for s_ in sets])
    y = np.concatenate([np.full(len(s_), i) for i, s_ in enumerate(sets)])
    wc = np.array([1.0 / max(len(s_), 1) for s_ in sets])
    out = np.zeros((len(q), len(sets)))
    for i in range(0, len(q), 1500):
        qq = q[i:i + 1500]
        d = ((lab[qq, None, :] - X[None]) ** 2).sum(-1)
        nn = y[np.argpartition(d, k, axis=1)[:, :k]]
        for ci in range(len(sets)):
            out[i:i + len(qq), ci] = (nn == ci).sum(1) * wc[ci]
    return out / out.sum(1, keepdims=True)


def classify(gid, arm, md):
    c = dict(DEFAULT, **CFG.get(gid, {}))
    B = arm.data.bones
    H = np.array(B['head'].head_local); N = np.array(B['neck_01'].head_local)
    p = md.wco
    dx = p[:, 0] - H[0]; dy = p[:, 1] - (H[1] + c['axis_dy']); dzH = p[:, 2] - H[2]
    r = np.hypot(dx, dy)
    t = np.clip((dy + 0.06) / 0.12, 0, 1)
    zbase = N[2] + c['base_front'] * (1 - t) + c['base_back'] * t
    hard = (dzH > c['hard_dz']) & (r < c['r_head'])
    zone = ~hard & (p[:, 2] > zbase) & (r < c['r_amb'])
    def inbox(b):
        (x0, x1), (y0, y1), (z0, z1) = b
        return (dx > x0) & (dx < x1) & (dy > y0) & (dy < y1) & (dzH > z0) & (dzH < z1)
    hairzone = np.zeros(md.nw, bool)
    if c['hair_box']:                                  # hair lying on the shoulders / hood (ponytail, bun)
        hairzone = ~hard & inbox(c['hair_box'])
        zone |= hairzone
    lab = md.wlab * np.array([0.5, 1.0, 1.0])          # shading is baked into Meshy textures: L counts half
    # colour model, three classes sampled from this character: skin (face front, hands), hair/hat (the skull
    # minus skin) and cloth (upper body below the neck)
    # face skin samples: mouth..cheekbones (above the chin, where high collars and fleece necks never reach)
    face = np.where((dzH > 0.025) & (dzH < 0.10) & (dy < -0.06) & (r < c['r_head'] + 0.02) & (md.whead > 0.9))[0]
    bright = face[md.wlab[face, 0] >= np.median(md.wlab[face, 0])]   # brighter half: skin, not beard/shadow
    skin_med = np.median(lab[bright], 0)
    nearskin = np.linalg.norm(lab - skin_med, axis=1) < c['skin_dist']
    skin_s = np.concatenate([face, np.where(md.whand > 0.6)[0]])
    hh_s = np.where(((dzH > 0.035) & (r < c['r_head']) | hard) & ~nearskin)[0]
    if c['hair_seed']:
        hh_s = np.union1d(hh_s, np.where(inbox(c['hair_seed']) & ~nearskin)[0])
    cloth_s = np.where((p[:, 2] < N[2] - 0.04) & (p[:, 2] > N[2] - 0.6) & (np.abs(dx) < 0.3) & (md.whand < 0.1)
                       & ~nearskin)[0]
    rng = np.random.default_rng(1)
    sets = [rng.choice(s_, min(len(s_), 3000), replace=False) for s_ in (skin_s, hh_s, cloth_s)]
    q = np.where(zone)[0]
    P = np.zeros((md.nw, 3)); P[:, 2] = 1
    P[q] = knn_probs(lab, q, sets, c['knn'])
    if c['hair_seed']:      # in the hair box only hair counts (not the hat: a teal beanie must not eat a teal fleece)
        hs = np.where(inbox(c['hair_seed']) & ~nearskin)[0]
        hq = np.where(hairzone)[0]
        ph = knn_probs(lab, hq, [rng.choice(hs, min(len(hs), 3000), replace=False), sets[2]], c['knn'])
        P[hq, 1] = ph[:, 0]; P[hq, 0] = np.minimum(P[hq, 0], 1 - ph[:, 0]); P[hq, 2] = 1 - P[hq, 0] - P[hq, 1]
    # unary: skin counts near the neck axis (collars/hoods stand away from it); hair/hat counts more the higher
    # the vertex sits between the base plane and the skull, and fully on hair lying on the shoulders/hood
    up = np.clip((p[:, 2] - zbase) / np.maximum(H[2] + 0.02 - zbase, 1e-3), 0, 1)   # 0 at the base, 1 at the jaw
    rs = c['r_skin'] + (c['r_face'] - c['r_skin']) * up                              # the face reaches further out
    skin_prior = 1 / (1 + np.exp((r - rs) / 0.012))
    f = np.clip((p[:, 2] - zbase) / np.maximum(H[2] + c['hard_dz'] - zbase, 1e-3), 0, 1) ** 2   # collars sit low
    f[hairzone] = 1.0
    pu = np.clip(P[:, 0] * skin_prior + P[:, 1] * f, 0, 1)
    phead = pu
    # forced: skin-coloured vertices near the axis that connect to the face through skin-coloured vertices
    # (shadowed fleece next to dark skin can look skin-coloured in spots; those spots are not connected)
    m = zone & (P[:, 0] > c['skin_grow']) & (r < rs - (c['r_skin'] - c['r_skin_force']))
    nb_hard = np.zeros(md.nw, bool)
    eh = md.edges[hard[md.edges[:, 0]] != hard[md.edges[:, 1]]]
    nb_hard[eh.ravel()] = True
    seeds = m & ((nb_hard & (P[:, 0] > 0.6)) | ((dzH > 0.0) & (P[:, 0] > 0.6)))
    cl = components(md, m)
    force_skin = m & np.isin(cl, np.unique(cl[seeds]))
    # hair lying on the hood/shoulders: clearly hair-coloured vertices in the hair box connected to the skull
    if hairzone.any():
        mh = hairzone & (P[:, 1] > c['hair_grow'])
        hseeds = mh & (nb_hard | (dzH > 0.0))
        clh = components(md, mh | (hard & np.isin(np.arange(md.nw), eh.ravel())))
        force_skin |= mh & np.isin(clh, np.unique(clh[hseeds]))
    # graph cut over the zone: unary = colour/prior, pairwise = colour similarity between neighbours
    idx = -np.ones(md.nw, np.int64); idx[q] = np.arange(len(q))
    e = md.edges
    d2 = ((lab[e[:, 0]] - lab[e[:, 1]]) ** 2).sum(1)
    w = c['lam'] * np.exp(-d2 / (2 * c['sigma'] ** 2))
    src = c['alpha'] * pu[q]; snk = c['alpha'] * (1 - pu[q])
    # bare neck skin hugging the axis above the base plane always goes (down to the base plane, not to the chin)
    src[force_skin[q]] = 1e6
    arcs = []
    za, zb = zone[e[:, 0]], zone[e[:, 1]]
    for (a, b), wi, ia, ib in zip(e, w, za, zb):
        if ia and ib: arcs.append((idx[a], idx[b], wi))
        elif ia or ib:
            u, o = (a, b) if ia else (b, a)
            if hard[o]: src[idx[u]] += wi
            elif p[o, 2] <= zbase[o]:
                # across the base plane: cloth stays anchored to the body below, skin does not (the neck skin
                # continues into the chest skin of an open collar; the cut there is the base plane itself)
                snk[idx[u]] += wi * (1 - P[u, 0])
            else: snk[idx[u]] += wi
    side = maxflow_mincut(len(q), arcs, src, snk)
    dele = hard.copy(); dele[q[side]] = True
    # tidy the cut line: kept spikes poking into the hole go, small bites into the cloth are kept
    free = zone & ~force_skin
    for _ in range(c['tidy']):
        fr = neighbour_sum(md, dele.astype(float)) / np.maximum(md.deg, 1)
        dele = np.where(free & ~dele & (fr >= 0.67), True, dele)
        dele = np.where(free & dele & (fr <= 0.25), False, dele)
    if os.environ.get("HEADLESS_DIAG"):
        reg = (dy < -0.05) & (p[:, 2] > H[2] - 0.03) & (p[:, 2] < H[2] + 0.06) & (r < 0.13) if gid == 'nima_ai' else               (dy < -0.03) & (p[:, 2] > zbase + 0.01) & (p[:, 2] < H[2] - 0.03) & (r < 0.08)
        sd = np.zeros(md.nw, bool); sd[q[side]] = True
        log("DIAG frontneck n", int(reg.sum()), "zone", int((reg & zone).sum()), "Pskin>0.6", int((reg & (P[:, 0] > 0.6)).sum()),
            "force", int((reg & force_skin).sum()), "side", int((reg & sd).sum()),
            "Pskin mean", float(P[reg, 0].mean()) if reg.any() else None, "rs", float(rs[reg].mean()) if reg.any() else None)
    # deletions must connect to the hard head; kept pieces must connect to the body
    cl = components(md, dele)
    dele &= np.isin(cl, np.unique(cl[hard]))
    if os.environ.get("HEADLESS_DIAG"):
        log("DIAG after connect", int((reg & dele).sum()))
    kl = components(md, ~dele)
    body = np.bincount(kl[kl >= 0]).argmax()
    floating = (~dele) & (kl != body)
    dele |= floating
    info = dict(H=H, N=N, P=P, force=force_skin, zonemask=zone, zbase_front=float(N[2] + c['base_front']), zbase_back=float(N[2] + c['base_back']),
                hard=int(hard.sum()), zone=int(zone.sum()), cut=int(side.sum()), floating=int(floating.sum()),
                skin_med=skin_med.round(1).tolist(), samples=[len(x) for x in sets],
                forced=int(force_skin[q].sum()))
    return dele, pu, info


# ---------------------------------------------------------------- rendering
def setup_render(color_type='TEXTURE'):
    s = bpy.context.scene
    s.render.engine = 'BLENDER_WORKBENCH'
    sh = s.display.shading
    sh.light = 'STUDIO'; sh.color_type = color_type; sh.show_backface_culling = False
    sh.background_type = 'VIEWPORT'; sh.background_color = (0.33, 0.35, 0.38)
    sh.show_cavity = False
    s.render.film_transparent = False
    cam = s.camera
    if cam is None:
        cam = bpy.data.objects.new("Cam", bpy.data.cameras.new("Cam")); s.collection.objects.link(cam); s.camera = cam
    cam.data.clip_start = 0.01
    return s, cam


def shot(path, target, direction, dist, res=(700, 900), lens=50, up='Y'):
    s = bpy.context.scene; cam = s.camera
    s.render.resolution_x, s.render.resolution_y = res
    cam.data.type = 'PERSP'; cam.data.lens = lens
    target = Vector(target)
    cam.location = target + Vector(direction).normalized() * dist
    cam.rotation_euler = (target - cam.location).to_track_quat('-Z', up).to_euler()
    s.render.filepath = path
    bpy.ops.render.render(write_still=True)


def debug_views(gid, ob, md, dele, pu, info, views=("F", "B", "L", "R", "T", "TB"), unary=False):
    me = ob.data
    col = np.ones((md.n, 4))
    col[:, :3] = np.clip(md.vcol * 1.25, 0, 1)
    dv = dele[md.weld]
    col[dv, :3] = (1.0, 0.0, 1.0)
    col[info['force'][md.weld], :3] = (1.0, 0.9, 0.0)
    col[(info['zonemask'] & ~dele)[md.weld], 2] = np.minimum(col[(info['zonemask'] & ~dele)[md.weld], 2] + 0.25, 1)
    a = me.color_attributes.new("dbg", 'FLOAT_COLOR', 'POINT'); a.data.foreach_set("color", col.ravel())
    if unary:                                     # class probabilities: red = skin, green = hair/hat, blue = cloth
        cu = np.ones((md.n, 4)); cu[:, :3] = info['P'][md.weld]
        b = me.color_attributes.new("dbgP", 'FLOAT_COLOR', 'POINT'); b.data.foreach_set("color", cu.ravel())
    setup_render('VERTEX')
    H = Vector(info['H']); N = Vector(info['N']); mid = (H + N) / 2
    dirs = dict(F=(0.25, -1, 0.2), B=(-0.3, 1, 0.3), L=(1, 0.1, 0.15), R=(-1, 0.1, 0.15), T=(0.5, -0.6, 1.2),
                TB=(-0.5, 0.7, 1.0))
    for layer, pre in (("dbg", "dbg"),) + ((("dbgP", "dbgP"),) if unary else ()):
        me.color_attributes.active_color = me.color_attributes[layer]
        for tag in views:
            shot(f"{SCRATCH}/{pre}_{gid}_{tag}.png", mid, dirs[tag], 0.85, res=(600, 600))


# ---------------------------------------------------------------- deletion
def faces_to_delete(ob, md, dele, info):
    """Faces touching a deleted vertex, plus slivers left on the cut (kept triangles with two open edges near the
    neck) and kept face islands no longer attached to the body."""
    me = ob.data
    nf = len(me.polygons)
    fv = np.empty(nf * 3, np.int64); me.polygons.foreach_get("vertices", fv); fv = md.weld[fv.reshape(-1, 3)]
    gone = dele[fv].any(1)
    H = np.asarray(info['H'])
    near = np.linalg.norm(md.wco[fv].mean(1) - H, axis=1) < 0.45
    slivers = 0
    for _ in range(12):                       # erodes one-triangle strips left on the cut from their ends
        keep = np.where(~gone)[0]
        e = np.sort(np.concatenate([fv[keep][:, [0, 1]], fv[keep][:, [1, 2]], fv[keep][:, [2, 0]]]), 1)
        key = e[:, 0] * md.nw + e[:, 1]
        u, inv, cnt = np.unique(key, return_inverse=True, return_counts=True)
        openedges = (cnt[inv.ravel()] == 1).reshape(3, -1).sum(0)
        sl = keep[(openedges >= 2) & near[keep]]
        if not len(sl):
            break
        gone[sl] = True; slivers += len(sl)
    # face islands over welded edges
    keep = np.where(~gone)[0]
    par = np.arange(md.nw)

    def find(a):
        r = a
        while par[r] != r:
            r = par[r]
        while par[a] != r:
            par[a], a = r, par[a]
        return r
    for a, b, c_ in fv[keep]:
        ra = find(a)
        par[ra] = find(b)
        par[find(c_)] = find(b)
    root = np.array([find(f[0]) for f in fv[keep]])
    ur, cnt = np.unique(root, return_counts=True)
    main = ur[cnt.argmax()]
    frag = keep[root != main]
    gone[frag] = True
    return gone, dict(slivers=int(slivers), fragment_faces=int(len(frag)),
                      fragment_islands=sorted(cnt[ur != main].tolist(), reverse=True)[:10])


def delete_faces(ob, gone):
    me = ob.data
    bm = bmesh.new(); bm.from_mesh(me)
    bm.faces.ensure_lookup_table()
    bmesh.ops.delete(bm, geom=[bm.faces[i] for i in np.where(gone)[0]], context='FACES_ONLY')
    loose_e = [e for e in bm.edges if not e.link_faces]
    bmesh.ops.delete(bm, geom=loose_e, context='EDGES')
    loose_v = [v for v in bm.verts if not v.link_faces]
    bmesh.ops.delete(bm, geom=loose_v, context='VERTS')
    bm.to_mesh(me); bm.free(); me.update()


def open_loops(ob):
    """Ordered welded boundary loops: list of arrays of welded ids, plus welded positions and the welded map."""
    me = ob.data
    n = len(me.vertices)
    co = np.empty(n * 3); me.vertices.foreach_get("co", co); co = co.reshape(n, 3)
    _, w = np.unique(np.round(co / 1e-5).astype(np.int64), axis=0, return_inverse=True); w = w.ravel()
    fv = np.empty(len(me.polygons) * 3, np.int64); me.polygons.foreach_get("vertices", fv); fv = w[fv.reshape(-1, 3)]
    e = np.concatenate([fv[:, [0, 1]], fv[:, [1, 2]], fv[:, [2, 0]]])        # directed, face winding
    key = np.sort(e, 1)
    u, inv, cnt = np.unique(key, axis=0, return_inverse=True, return_counts=True)
    od = e[(cnt[inv.ravel()] == 1)]
    nxt = {}
    for a_, b_ in od:
        nxt.setdefault(int(b_), []).append(int(a_))       # walk against the face winding
    wco = np.zeros((w.max() + 1, 3)); wco[w] = co
    seen, loops = set(), []
    for start in list(nxt):
        if start in seen: continue
        loop, v = [], start
        while v not in seen and v in nxt:
            seen.add(v); loop.append(v)
            v = nxt[v][0]
        if len(loop) >= 3: loops.append(np.array(loop))
    return loops, wco, w


def cut_stats(ob, info):
    """World Z of the open boundary around the neck (the cut) and the top of what is left."""
    me = ob.data
    n = len(me.vertices)
    co = np.empty(n * 3); me.vertices.foreach_get("co", co); co = co.reshape(n, 3)
    _, w = np.unique(np.round(co / 1e-5).astype(np.int64), axis=0, return_inverse=True); w = w.ravel()
    fv = np.empty(len(me.polygons) * 3, np.int64); me.polygons.foreach_get("vertices", fv); fv = w[fv.reshape(-1, 3)]
    e = np.sort(np.concatenate([fv[:, [0, 1]], fv[:, [1, 2]], fv[:, [2, 0]]]), 1)
    u, cnt = np.unique(e, axis=0, return_counts=True)
    ov = np.unique(u[cnt == 1].ravel())
    wco = np.zeros((w.max() + 1, 3)); wco[w] = co
    H = np.asarray(info['H'])
    pts = wco[ov]; pts = pts[np.linalg.norm(pts - H, axis=1) < 0.35]
    return dict(cut_z_min=float(pts[:, 2].min()) if len(pts) else None,
                cut_z_mean=float(pts[:, 2].mean()) if len(pts) else None,
                cut_z_max=float(pts[:, 2].max()) if len(pts) else None,
                open_verts_near_neck=int(len(pts)), open_verts_total=int(len(ov)), top_z_after=float(co[:, 2].max()))


# ---------------------------------------------------------------- caps over hair holes
def ear_clip(pts2):
    """Ear-clipping triangulation of a simple polygon (counter-clockwise or clockwise) -> list of index triples."""
    n = len(pts2)
    idx = list(range(n))
    area = 0.5 * sum(pts2[i][0] * pts2[(i + 1) % n][1] - pts2[(i + 1) % n][0] * pts2[i][1] for i in range(n))
    sgn = 1.0 if area > 0 else -1.0
    tris = []

    def cross(o, a, b):
        return (a[0] - o[0]) * (b[1] - o[1]) - (a[1] - o[1]) * (b[0] - o[0])

    def inside(p, a, b, c):
        d1, d2, d3 = cross(a, b, p) * sgn, cross(b, c, p) * sgn, cross(c, a, p) * sgn
        return d1 >= -1e-12 and d2 >= -1e-12 and d3 >= -1e-12
    guard = 0
    while len(idx) > 3 and guard < 10000:
        guard += 1
        found = False
        m = len(idx)
        for k in range(m):
            i0, i1, i2 = idx[(k - 1) % m], idx[k], idx[(k + 1) % m]
            a, b, c_ = pts2[i0], pts2[i1], pts2[i2]
            if cross(a, b, c_) * sgn <= 1e-12:
                continue
            if any(inside(pts2[j], a, b, c_) for j in idx if j not in (i0, i1, i2)):
                continue
            tris.append((i0, i1, i2)); idx.pop(k); found = True
            break
        if not found:
            return None
    tris.append(tuple(idx))
    return tris


def cap_holes(gid, ob, info, md, c):
    """Close the holes left where hair lay on the coat (cap_box, relative to the head bone) with flat caps made of
    copies of the hole's own boundary vertices (same positions, same skin weights), one sampled coat UV."""
    if not c['cap_box']:
        return dict(caps=0)
    me = ob.data
    n = len(me.vertices)
    co = np.empty(n * 3); me.vertices.foreach_get("co", co); co = co.reshape(n, 3)
    _, w = np.unique(np.round(co / 1e-5).astype(np.int64), axis=0, return_inverse=True); w = w.ravel()
    nw = w.max() + 1
    rep = np.zeros(nw, np.int64); rep[w] = np.arange(n)
    wco = np.zeros((nw, 3)); wco[w] = co
    fv = np.empty(len(me.polygons) * 3, np.int64); me.polygons.foreach_get("vertices", fv); fv = w[fv.reshape(-1, 3)]
    e = np.sort(np.concatenate([fv[:, [0, 1]], fv[:, [1, 2]], fv[:, [2, 0]]]), 1)
    u, cnt = np.unique(e, axis=0, return_counts=True)
    H = np.asarray(info['H'])
    (x0, x1), (y0, y1), (z0, z1) = c['cap_box']
    d = wco - H
    inb = (d[:, 0] > x0) & (d[:, 0] < x1) & (d[:, 1] > y0) & (d[:, 1] < y1) & (d[:, 2] > z0) & (d[:, 2] < z1)
    be = u[(cnt == 1) & inb[u[:, 0]] & inb[u[:, 1]]]
    adj = {}
    for a_, b_ in be:
        adj.setdefault(int(a_), set()).add(int(b_)); adj.setdefault(int(b_), set()).add(int(a_))
    polys, seen = [], set()
    for s0 in list(adj):
        if s0 in seen: continue
        comp, st = [], [s0]; seen.add(s0)
        while st:
            v = st.pop(); comp.append(v)
            for o in adj[v]:
                if o not in seen: seen.add(o); st.append(o)
        if len(comp) < 4: continue
        degs = {v: len(adj[v]) for v in comp}
        if max(degs.values()) > 2:
            log(gid, "cap: skipped a non-manifold boundary piece of", len(comp)); continue
        ends = [v for v in comp if degs[v] == 1]
        start = ends[0] if ends else comp[0]
        path, prev, v = [start], None, start
        while True:
            nx = [o for o in adj[v] if o != prev and o not in path[-2:]]
            if not nx or nx[0] == start: break
            prev, v = v, nx[0]; path.append(v)
        polys.append(path)
    # coat UV: of the corners next to the caps, the one whose colour is the median of them
    uvd = me.uv_layers.active.data
    luv = np.empty(len(me.loops) * 2); uvd.foreach_get("uv", luv); luv = luv.reshape(-1, 2)
    lv = np.empty(len(me.loops), np.int64); me.loops.foreach_get("vertex_index", lv)
    bm = bmesh.new(); bm.from_mesh(me)
    bm.verts.ensure_lookup_table()
    dl = bm.verts.layers.deform.active
    uvl = bm.loops.layers.uv.active
    orig = list(bm.verts)
    made = 0
    for path in polys:
        P3 = wco[path]
        ctr = P3.mean(0)
        out = ctr - np.array([H[0], H[1], H[2] - 0.30])                 # away from the chest = outside
        out /= np.linalg.norm(out)
        ring = set(path)
        cand = [li for li in range(len(lv)) if w[lv[li]] in ring]
        cols = md.sample(luv[cand])
        med = np.median(cols, 0)
        uv = luv[cand][np.argmin(np.linalg.norm(cols - med, axis=1))]
        newv = []
        srcs = [orig[int(rep[wid])] for wid in path]
        for src in srcs:
            nv = bm.verts.new(src.co)
            for g, wt in src[dl].items():
                nv[dl][g] = wt
            newv.append(nv)
        edges = [bm.edges.new((newv[i], newv[(i + 1) % len(newv)])) for i in range(len(newv))]
        res = bmesh.ops.triangle_fill(bm, use_beauty=True, use_dissolve=False, edges=edges, normal=Vector(out.tolist()))
        faces = [g for g in res['geom'] if isinstance(g, bmesh.types.BMFace)]
        if not faces:
            log(gid, "cap: fill failed for a hole of", len(path))
            for v in newv: bm.verts.remove(v)
            continue
        for f in faces:
            f.normal_update()
            if f.normal.dot(Vector(out.tolist())) < 0:
                f.normal_flip()
            f.smooth = False; f.material_index = 0
            for lp in f.loops:
                lp[uvl].uv = uv
        made += len(faces)
        log(gid, "cap: hole of", len(path), "boundary verts ->", len(faces), "faces, uv", uv.round(3).tolist())
    bm.to_mesh(me); bm.free(); me.update()
    return dict(caps=len(polys), cap_faces=made)


# ---------------------------------------------------------------- export + checks
def export_glb(arm, ob, path):
    for a in list(ob.data.color_attributes):
        ob.data.color_attributes.remove(a)
    bpy.ops.object.select_all(action='DESELECT')
    arm.select_set(True); ob.select_set(True); bpy.context.view_layer.objects.active = arm
    bpy.ops.export_scene.gltf(filepath=path, export_format='GLB', use_selection=True, export_animations=False,
                              export_skins=True, export_morph=False, export_apply=False, export_yup=True,
                              export_image_format='AUTO', export_materials='EXPORT', export_texcoords=True,
                              export_normals=True, export_tangents=False, export_def_bones=False)


def read_glb(path):
    b = open(path, 'rb').read()
    jl = struct.unpack('<I', b[12:16])[0]
    j = json.loads(b[20:20 + jl])
    return j, b[20 + jl + 8:]


def accessor(j, bin_, i):
    a = j['accessors'][i]; bv = j['bufferViews'][a['bufferView']]
    dt = {5126: np.float32, 5125: np.uint32, 5123: np.uint16, 5121: np.uint8}[a['componentType']]
    nc = {'SCALAR': 1, 'VEC2': 2, 'VEC3': 3, 'VEC4': 4, 'MAT4': 16}[a['type']]
    off = bv.get('byteOffset', 0) + a.get('byteOffset', 0)
    stride = bv.get('byteStride', 0)
    isz = np.dtype(dt).itemsize * nc
    if stride and stride != isz:
        raw = np.frombuffer(bin_, np.uint8, a['count'] * stride, off).reshape(a['count'], stride)[:, :isz]
        arr = raw.copy().view(dt).reshape(a['count'], nc)
    else:
        arr = np.frombuffer(bin_, dt, a['count'] * nc, off).reshape(a['count'], nc)
    if a.get('normalized'):
        return arr.astype(np.float64) / np.iinfo(dt).max
    return arr.astype(np.float64) if dt == np.float32 else arr.astype(np.int64)


def glb_skeleton(j, bin_):
    par = {}
    for i, n in enumerate(j['nodes']):
        for ch in n.get('children', []):
            par[ch] = i
    sk = j['skins'][0]
    ibm = accessor(j, bin_, sk['inverseBindMatrices'])
    out = {}
    for k, ni in enumerate(sk['joints']):
        n = j['nodes'][ni]
        out[n['name']] = dict(parent=j['nodes'][par[ni]]['name'] if ni in par else None,
                              t=np.array(n.get('translation', [0, 0, 0])), r=np.array(n.get('rotation', [0, 0, 0, 1])),
                              s=np.array(n.get('scale', [1, 1, 1])), ibm=ibm[k])
    return out


def glb_verts(j, bin_):
    pr = j['meshes'][0]['primitives'][0]['attributes']
    names = [j['nodes'][ni]['name'] for ni in j['skins'][0]['joints']]
    get = lambda k: accessor(j, bin_, pr[k])
    extra = sorted(k for k in pr if k not in ('POSITION', 'NORMAL', 'TEXCOORD_0', 'JOINTS_0', 'WEIGHTS_0'))
    return get('POSITION'), get('NORMAL'), get('TEXCOORD_0'), get('JOINTS_0'), get('WEIGHTS_0'), names, extra


def check(src, dst):
    """Compare input and output glTF: the skeleton must be identical (names, parents, rest TRS, inverse binds);
    every output vertex must exist in the input (same position + UV) with the same normal and skin weights;
    images byte-identical; same material slots."""
    import hashlib
    js, bs = read_glb(src); jd, bd = read_glb(dst)
    ss = glb_skeleton(js, bs); sd = glb_skeleton(jd, bd)
    sk_err = []
    if set(ss) != set(sd):
        sk_err.append(f"joint sets differ: {sorted(set(ss) ^ set(sd))}")
    for nm in set(ss) & set(sd):
        a, b = ss[nm], sd[nm]
        if a['parent'] != b['parent']: sk_err.append(f"{nm} parent {a['parent']} -> {b['parent']}")
        if np.abs(a['t'] - b['t']).max() > 1e-5: sk_err.append(f"{nm} translation")
        if min(np.abs(a['r'] - b['r']).max(), np.abs(a['r'] + b['r']).max()) > 1e-5: sk_err.append(f"{nm} rotation")
        if np.abs(a['s'] - b['s']).max() > 1e-5: sk_err.append(f"{nm} scale")
        if np.abs(a['ibm'] - b['ibm']).max() > 1e-5: sk_err.append(f"{nm} inverse bind")
    P0, N0, U0, J0, W0, n0, x0 = glb_verts(js, bs)
    P1, N1, U1, J1, W1, n1, x1 = glb_verts(jd, bd)
    from mathutils.kdtree import KDTree
    kd = KDTree(len(P0))
    for i, pp in enumerate(P0):
        kd.insert(pp, i)
    kd.balance()
    missing = bad_n = bad_w = 0
    for i in range(len(P1)):
        cand = [ix for _, ix, d in kd.find_range(P1[i], 1e-5) if np.abs(U0[ix] - U1[i]).max() < 1e-4]
        if not cand:
            missing += 1; continue
        i0 = max(cand, key=lambda ix: np.dot(N0[ix], N1[i]))
        if np.dot(N0[i0], N1[i]) < 0.999: bad_n += 1
        wa, wb = {}, {}
        for jj, ww in zip(J0[i0], W0[i0]):
            if ww > 0: wa[n0[int(jj)]] = wa.get(n0[int(jj)], 0) + ww
        for jj, ww in zip(J1[i], W1[i]):
            if ww > 0: wb[n1[int(jj)]] = wb.get(n1[int(jj)], 0) + ww
        if any(abs(wa.get(kk, 0) - wb.get(kk, 0)) > 2e-3 for kk in set(wa) | set(wb)): bad_w += 1

    def imgs(j, b):
        out = []
        for im in j.get('images', []):
            bv = j['bufferViews'][im['bufferView']]; o = bv.get('byteOffset', 0)
            out.append((im.get('mimeType'), hashlib.md5(b[o:o + bv['byteLength']]).hexdigest()))
        return sorted(out)
    mat = lambda j: [(m.get('name'), sorted(k for k in m if k != 'name')) for m in j.get('materials', [])]
    return dict(skeleton_errors=sk_err, joints=len(ss), verts_in=len(P0), verts_out=len(P1),
                verts_not_in_input=missing, normal_changed=bad_n, weights_changed=bad_w,
                images_identical=imgs(js, bs) == imgs(jd, bd), materials_same=mat(js) == mat(jd),
                extra_attrs=[x0, x1], animations=[len(js.get('animations', [])), len(jd.get('animations', []))])


# ---------------------------------------------------------------- previews
def previews(gid, arm, ob, info, coat=False):
    setup_render('TEXTURE')
    me = ob.data
    vs = np.empty(len(me.vertices) * 3); me.vertices.foreach_get("co", vs); vs = vs.reshape(-1, 3)
    lo, hi = vs.min(0), vs.max(0)
    ctr = Vector(((lo + hi) / 2).tolist()); h = float(hi[2] - lo[2])
    H = Vector(info['H']); N = Vector(info['N'])
    pre = f"{PREV}/headless_{gid}"
    shot(f"{pre}_front.png", ctr, (0, -1, 0.04), h * 1.75, res=(700, 900))
    shot(f"{pre}_back.png", ctr, (0, 1, 0.04), h * 1.75, res=(700, 900))
    shot(f"{pre}_neck.png", (H + N) / 2 - Vector((0, 0, 0.02)), (0.55, -0.75, 1.0), 0.62, res=(800, 800))
    if coat:
        pel = arm.data.bones['pelvis'].head_local
        shot(f"{pre}_coat_front.png", pel - Vector((0, 0, 0.08)), (0, -1, -0.15), 1.3, res=(800, 800))
        shot(f"{pre}_coat_below.png", pel - Vector((0, 0, 0.05)), (0, -0.25, -1), 1.0, res=(800, 800))


# ---------------------------------------------------------------- main
SCRATCH = os.environ.get("HEADLESS_SCRATCH", f"{ROOT}/Saved/tmp_blender")
COAT = {"nima_ai", "elena_ai"}
report = {}
for gid in SEL:
    arm, ob = load(gid)
    md = MeshData(arm, ob)
    height = float(md.wco[:, 2].max() - md.wco[:, 2].min())
    dele, phead, info = classify(gid, arm, md)
    log(gid, "verts", md.n, "welded", md.nw, "delete welded", int(dele.sum()),
        {k: v for k, v in info.items() if k not in ('H', 'N', 'P', 'force', 'zonemask')})
    if DEBUG:
        debug_views(gid, ob, md, dele, phead, info, unary='--unary' in ARGS)
        for a in list(ob.data.color_attributes):
            ob.data.color_attributes.remove(a)
    cs0 = cut_stats(ob, info)
    gone, finfo = faces_to_delete(ob, md, dele, info)
    nv0, nf0 = len(ob.data.vertices), len(ob.data.polygons)
    # custom normals are stored relative to each corner's smooth fan, which deleting faces changes at the cut:
    # remember the absolute corner normals and write them back afterwards
    me_ = ob.data
    cn = np.empty(len(me_.loops) * 3); me_.corner_normals.foreach_get("vector", cn)
    ka = me_.attributes.new("keep_normal", 'FLOAT_VECTOR', 'CORNER'); ka.data.foreach_set("vector", cn)
    delete_faces(ob, gone)
    cinfo = cap_holes(gid, ob, info, md, dict(DEFAULT, **CFG.get(gid, {})))
    me_ = ob.data
    kn = np.empty(len(me_.loops) * 3); me_.attributes["keep_normal"].data.foreach_get("vector", kn); kn = kn.reshape(-1, 3)
    pn = np.empty(len(me_.polygons) * 3); me_.polygons.foreach_get("normal", pn); pn = pn.reshape(-1, 3)
    lp = np.empty(len(me_.loops), np.int64)
    for poly in me_.polygons:
        lp[poly.loop_start:poly.loop_start + poly.loop_total] = poly.index
    zero = np.linalg.norm(kn, axis=1) < 0.5                     # cap corners: flat face normal
    kn[zero] = pn[lp[zero]]
    me_.normals_split_custom_set([tuple(v) for v in kn])
    me_.attributes.remove(me_.attributes["keep_normal"])
    nv1, nf1 = len(ob.data.vertices), len(ob.data.polygons)
    cs = cut_stats(ob, info)
    if "--loops" in ARGS:
        loops, wco, _ = open_loops(ob)
        Hh = np.asarray(info['H'])
        for L in sorted(loops, key=len, reverse=True)[:4]:
            d = wco[L] - Hh
            log(gid, "LOOP", len(L), "mean dist", round(float(np.linalg.norm(d, axis=1).mean()), 3))
            if np.linalg.norm(d, axis=1).mean() < 0.4:
                log("   ", " ".join(f"({x:+.3f},{y:+.3f},{z:+.3f})" for x, y, z in d[::max(1, len(d) // 60)]))
    B = arm.data.bones
    chain, b = [], B['head']
    while b is not None and (b.name.startswith('neck') or b.name == 'head'):
        chain.insert(0, b.name); b = b.parent
    rep = dict(height=height, neck_head_chain=chain, head_children=[ch.name for ch in B['head'].children],
               neck_01_z=float(B['neck_01'].head_local.z), head_z=float(B['head'].head_local.z),
               verts_before=nv0, verts_after=nv1, verts_deleted=nv0 - nv1, faces_before=nf0, faces_after=nf1,
               welded_deleted=int(dele.sum()), open_verts_before=cs0['open_verts_total'], **finfo, **cs, **cinfo,
               classify={k: v for k, v in info.items() if k not in ('H', 'N', 'P', 'force', 'zonemask')})
    out = f"{AI}/{gid}/{gid}_headless.glb"
    if EXPORT:
        export_glb(arm, ob, out)
        rep['check'] = check(f"{AI}/{gid}/{gid}_rigged.glb", out)
        rep['glb_bytes'] = os.path.getsize(out)
    if "--no-previews" not in ARGS:
        previews(gid, arm, ob, info, coat=gid in COAT)
    if "--after" in ARGS:                         # textured close-ups of the result, for checking the cut
        setup_render('TEXTURE')
        Hh = Vector(info['H']); Nn = Vector(info['N']); mid = (Hh + Nn) / 2 - Vector((0, 0, 0.03))
        for tag, d in dict(F=(0.25, -1, 0.35), B=(-0.3, 1, 0.45), L=(1, 0.1, 0.3), R=(-1, 0.1, 0.3),
                           T=(0.4, -0.5, 1.2), TB=(-0.4, 0.6, 1.0)).items():
            shot(f"{SCRATCH}/aft_{gid}_{tag}.png", mid, d, 0.7, res=(600, 600))
        if CFG.get(gid, {}).get('cap_box'):
            (x0, x1), (y0, y1), (z0, z1) = CFG[gid]['cap_box']
            hc = Hh + Vector(((x0 + x1) / 2, (y0 + y1) / 2, (z0 + z1) / 2))
            for tag, d in dict(cap1=(0.5, 0.8, 0.6), cap2=(1, -0.2, 0.4), cap3=(0.2, 0.3, 1.0)).items():
                shot(f"{SCRATCH}/aft_{gid}_{tag}.png", hc, d, 0.45, res=(600, 600))
    report[gid] = rep
    log(gid, json.dumps(rep, default=str))
os.makedirs(SCRATCH, exist_ok=True)
with open(f"{SCRATCH}/headless_report.json", "w") as fh:
    json.dump(report, fh, indent=1, default=str)
