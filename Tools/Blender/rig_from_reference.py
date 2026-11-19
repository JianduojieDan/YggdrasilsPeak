# Rigs a static Meshy character glb with the skeleton of an already-rigged Meshy character glb
# (same bone names, hierarchy and joint-axis conventions), so one Unreal retarget/IK setup serves both.
#   blender -b --factory-startup -P rig_from_reference.py -- <reference_rigged.glb> <static.glb> <out.glb> <height_m> [<compare_prefix>]
# (set TEMP/TMP to Saved/tmp_blender)
# Method
#  1. The static mesh is scaled to <height_m>, feet on z=0, bbox centred in x/y (Meshy's own convention); UVs,
#     materials and textures are untouched.
#  2. Both meshes are measured the same way with true cross-section contours (triangle/plane intersection,
#     connected loops merged into clusters): landmarks crotch / armpit / neck / top give a piecewise height map,
#     arms are traced slice by slice from the armpit down to the fingertips. Every reference joint keeps its
#     normalised position inside the matching cluster (torso, leg, arm, head box, foot box) of the target; elbows
#     keep their fraction between shoulder and wrist, wrists keep their distance from the fingertips; fingers
#     follow the hand rigidly (rotated with the forearm, scaled with the height).
#  3. Each bone's rest frame = reference frame rotated by the minimal rotation that takes its reference
#     direction-to-(main)child onto the new one (leaf bones inherit the parent's), so bone rolls/axes match.
#  4. Skin: the reference mesh is posed (similarity per bone) into the fitted skeleton; each target vertex averages
#     the weights of its 8 nearest posed-reference vertices of the SAME body region (arm/torso/leg from each mesh's
#     own sections - a plain nearest-face Data Transfer let the jacket flank and fingertips grab wrong bones).
#     Torso drops forearm/hand weights and, below the armpit, upper-arm/clavicle weights (Meshy's reference lets
#     the flank balloon when the arm rises). Max 4 influences, >= 0.01, normalised.
#  5. Export glb (+Y up, skin, no animation); check bone names/parents/axes, images and vertex count against the
#     inputs. Pose test of the exported file: rig_pose_test.py (arms up 60, elbows 70, knees 60, head 40).
import bpy, sys, os, math, json, struct
import numpy as np
from mathutils import Vector, Matrix, Quaternion
from mathutils.bvhtree import BVHTree

A = sys.argv[sys.argv.index("--") + 1:]
REF, SRC, OUT, HEIGHT = os.path.abspath(A[0]), os.path.abspath(A[1]), os.path.abspath(A[2]), float(A[3])
CMP = os.path.abspath(A[4]) if len(A) > 4 else None

def log(*a): print("RIG", *a, flush=True)

def sel(objs, active):
    if bpy.context.object and bpy.context.object.mode != 'OBJECT':
        bpy.ops.object.mode_set(mode='OBJECT')
    bpy.ops.object.select_all(action='DESELECT')
    for o in objs: o.select_set(True)
    bpy.context.view_layer.objects.active = active

# ---------------------------------------------------------------- load
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.gltf(filepath=REF)
arm = next(o for o in bpy.data.objects if o.type == 'ARMATURE')
nmesh = next(o for o in bpy.data.objects if o.type == 'MESH' and o.parent == arm)
for o in list(bpy.data.objects):
    if o not in (arm, nmesh): bpy.data.objects.remove(o)
nmesh.name = nmesh.data.name = "RefSrc"
before = set(bpy.data.objects)
bpy.ops.import_scene.gltf(filepath=SRC)
new = [o for o in bpy.data.objects if o not in before]
mm = max((o for o in new if o.type == 'MESH'), key=lambda o: len(o.data.vertices))
for o in new:
    if o is not mm: bpy.data.objects.remove(o)
mm.data.transform(mm.matrix_world); mm.matrix_world = Matrix.Identity(4)
log("ref", arm.name, len(arm.data.bones), "bones; target", mm.name, len(mm.data.vertices), "verts")

def coords(o):
    co = np.empty(len(o.data.vertices) * 3); o.data.vertices.foreach_get("co", co)
    return co.reshape(-1, 3)

co = coords(mm)
lo, hi = co.min(0), co.max(0)
s0 = HEIGHT / (hi[2] - lo[2])
ctr = Vector(((lo[0] + hi[0]) / 2, (lo[1] + hi[1]) / 2, lo[2]))
mm.data.transform(Matrix.Scale(s0, 4) @ Matrix.Translation(-ctr))
mm.data.update()
log("target scaled x%.4f, moved %s" % (s0, tuple(round(v, 4) for v in -ctr)))

# ---------------------------------------------------------------- measuring
def tris_of(o):
    me = o.data; me.calc_loop_triangles()
    t = np.empty(len(me.loop_triangles) * 3, dtype=np.int64); me.loop_triangles.foreach_get("vertices", t)
    return t.reshape(-1, 3)

def mk(q):
    x0, x1, y0, y1 = q[:, 0].min(), q[:, 0].max(), q[:, 1].min(), q[:, 1].max()
    return dict(x0=x0, x1=x1, y0=y0, y1=y1, n=len(q), p=q, cx=(x0 + x1) / 2, cy=(y0 + y1) / 2,
                hx=max((x1 - x0) / 2, 1e-4), hy=max((y1 - y0) / 2, 1e-4))

class Body:
    def __init__(self, o):
        self.co = coords(o)                    # armature/world space (identity)
        # weld UV-seam duplicates (by position) so contour loops stay closed across seams
        q = np.round(self.co / 1e-5).astype(np.int64)
        _, first, inv = np.unique(q, axis=0, return_index=True, return_inverse=True)
        self.tri = first[inv.ravel()][tris_of(o)]
        self.top = self.co[:, 2].max()

    def slice(self, z, tol=0.003):
        co, tri = self.co, self.tri
        z = z + 1.234e-6
        ab = co[:, 2] > z
        n = ab[tri].sum(1)
        t = tri[(n == 1) | (n == 2)]
        if len(t) == 0: return []
        keys, pts = [], []
        segk = np.full((len(t), 2), -1, dtype=np.int64); fill = np.zeros(len(t), dtype=np.int64)
        NV = len(co)
        for a, b in ((0, 1), (1, 2), (2, 0)):
            va, vb = t[:, a], t[:, b]
            cr = ab[va] != ab[vb]
            k = np.minimum(va, vb) * NV + np.maximum(va, vb)
            idx = np.nonzero(cr)[0]
            segk[idx, fill[idx]] = k[idx]; fill[idx] += 1
        segk = segk[fill == 2]
        uk, inv = np.unique(segk.ravel(), return_inverse=True)
        inv = inv.reshape(-1, 2)
        va, vb = uk // NV, uk % NV
        pa, pb = co[va], co[vb]
        f = ((z - pa[:, 2]) / (pb[:, 2] - pa[:, 2]))[:, None]
        P = pa + (pb - pa) * f
        lab = np.arange(len(uk))
        while True:
            m = np.minimum(lab[inv[:, 0]], lab[inv[:, 1]])
            old = lab.copy()
            np.minimum.at(lab, inv[:, 0], m); np.minimum.at(lab, inv[:, 1], m)
            lab = lab[lab]
            if np.array_equal(lab, old): break
        cl = [P[lab == L][:, :2] for L in np.unique(lab)]
        if tol >= 0:      # merge loops whose boxes touch (within tol); tol<0 = raw loops
            merged = True
            while merged:
                merged = False
                bb = [(q[:, 0].min(), q[:, 0].max(), q[:, 1].min(), q[:, 1].max()) for q in cl]
                for i in range(len(cl)):
                    for j in range(i + 1, len(cl)):
                        a, b = bb[i], bb[j]
                        if a[0] - tol <= b[1] and b[0] - tol <= a[1] and a[2] - tol <= b[3] and b[2] - tol <= a[3]:
                            cl[i] = np.vstack((cl[i], cl[j])); del cl[j]; merged = True; break
                    if merged: break
        return [mk(q) for q in cl if len(q) >= 6]

    def central(self, z):
        cl = self.slice(z)
        inside = [c for c in cl if c["x0"] <= 0 <= c["x1"]]
        if inside: return max(inside, key=lambda c: c["n"])
        return min(cl, key=lambda c: abs(c["cx"]))

    def side(self, z, sgn, near=None):
        all_ = self.slice(z)
        cl = [c for c in all_ if c["cx"] * sgn > 0 and not (c["x0"] <= 0 <= c["x1"])]
        if not cl and near is None:   # limbs fused at the centre (touching thighs): take that half of the section
            q = [c["p"][c["p"][:, 0] * sgn > 0.01] for c in all_ if c["x0"] <= 0 <= c["x1"]]
            q = [x for x in q if len(x)]
            if q: cl = [mk(np.vstack(q))]
        if not cl: return None
        if near is not None:
            c = min(cl, key=lambda c: math.hypot(c["cx"] - near[0], c["cy"] - near[1]))
            return c if math.hypot(c["cx"] - near[0], c["cy"] - near[1]) < 0.06 else None
        return max(cl, key=lambda c: (c["x1"] - c["x0"]) * (c["y1"] - c["y0"]))

    def landmarks(self):
        T = self.top
        z = 0.30 * T
        while z < 0.7 * T:
            # crotch = a single contour crossing x=0 over most of its depth (touching thighs only cross briefly)
            hit = False
            for c in self.slice(z, tol=-1):
                if c["x0"] < -0.01 and c["x1"] > 0.01:
                    m = c["p"][np.abs(c["p"][:, 0]) < 0.015]
                    if len(m) and m[:, 1].max() - m[:, 1].min() > 0.6 * (c["y1"] - c["y0"]): hit = True
            if hit: break
            z += 0.005
        self.crotch = z
        best = None
        for z in np.arange(0.78 * T, 0.95 * T, 0.005):
            c = self.central(z); w = c["x1"] - c["x0"]
            if best is None or w < best[0]: best = (w, z)
        self.neck = best[1]
        z = self.neck - 0.02
        while z > 0.3 * T:
            if self.outer(z, 1) and self.outer(z, -1): break
            z -= 0.005
        self.armpit = z
        self.knots = [0.0, self.armpit, self.neck, T]   # crotch is unreliable when thighs touch: logged only
        hz = self.co[:, 2] > self.neck + 0.02
        h = self.co[hz]; self.headbox = (h.min(0), h.max(0))
        self.arms = {sgn: self.trace_arm(sgn) for sgn in (1, -1)}
        self.tip = {sgn: self.arms[sgn][-1][0] for sgn in (1, -1)}

    def outer(self, z, sgn):
        """Sections fully outside the torso's x-range on one side (= arms; hood strings in front don't count)."""
        cl = self.slice(z); cen = [c for c in cl if c["x0"] <= 0 <= c["x1"]]
        if not cen: return []
        c0 = max(cen, key=lambda c: c["n"])
        return [c for c in cl if c["n"] >= 10 and (c["x0"] > c0["x1"] - 0.005 if sgn > 0 else c["x1"] < c0["x0"] + 0.005)]

    def trace_arm(self, sgn):
        z = self.armpit
        c = max(self.outer(z, sgn), key=lambda c: c["n"])
        out = [(z, c["cx"], c["cy"])]
        self.arm_boxes = getattr(self, "arm_boxes", {}); self.arm_boxes[sgn] = [(z, self.with_lim(z, sgn, c))]
        while z > 0.2:
            z -= 0.01
            c = self.side(z, sgn, near=out[-1][1:])
            if c is None: break
            out.append((z, c["cx"], c["cy"])); self.arm_boxes[sgn].append((z, self.with_lim(z, sgn, c)))
        return out

    def with_lim(self, z, sgn, arm):
        """Grows the arm section by loose bits within 3 cm (thumb tips) and adds 'lim' = how far the rest of the
        body reaches towards the arm at this height."""
        arm = dict(arm); lim = []
        for c in self.slice(z):
            if abs(c["cx"] - arm["cx"]) < 1e-6 and abs(c["cy"] - arm["cy"]) < 1e-6: continue
            gx = max(c["x0"] - arm["x1"], arm["x0"] - c["x1"], 0); gy = max(c["y0"] - arm["y1"], arm["y0"] - c["y1"], 0)
            if math.hypot(gx, gy) < 0.03 and not (c["x0"] <= 0 <= c["x1"]):
                for k, f in (("x0", min), ("y0", min), ("x1", max), ("y1", max)): arm[k] = f(arm[k], c[k])
            elif c["cx"] * sgn < arm["cx"] * sgn:
                lim.append(c["x1"] if sgn > 0 else -c["x0"])
        arm["lim"] = max(lim) if lim else 0.0
        return arm

    def regions(self, hip_z):
        """0 torso/head, 1 left arm, 2 right arm, 3 left leg, 4 right leg (arms = the traced arm sections)."""
        co = self.co; reg = np.zeros(len(co), dtype=np.int8)
        for sgn, rid in ((1, 1), (-1, 2)):
            for z, c in self.arm_boxes[sgn]:
                band = np.abs(co[:, 2] - z) <= 0.0051
                inbox = ((co[:, 0] >= c["x0"] - .01) & (co[:, 0] <= c["x1"] + .01)
                         & (co[:, 1] >= c["y0"] - .01) & (co[:, 1] <= c["y1"] + .01))
                beyond = co[:, 0] * sgn > c["lim"] + 0.015      # past the body's side at this height (thumbs etc.)
                reg[band & (inbox | beyond)] = rid
            z, c = self.arm_boxes[sgn][-1]                     # fingertips below the last traced section
            reg[(co[:, 2] <= z) & (co[:, 2] >= z - 0.08) & (co[:, 0] * sgn > c["lim"] + 0.015)] = rid
        legs = (reg == 0) & (co[:, 2] < hip_z)
        reg[legs & (co[:, 0] >= 0)] = 3; reg[legs & (co[:, 0] < 0)] = 4
        return reg

    def arm_cluster(self, sgn, z):
        tr = self.arms[sgn]
        zs = np.array([p[0] for p in tr])[::-1]
        cx = np.interp(z, zs, np.array([p[1] for p in tr])[::-1])
        cy = np.interp(z, zs, np.array([p[2] for p in tr])[::-1])
        return self.side(z, sgn, near=(cx, cy)) or self.side(z, sgn)

def norm(c, p): return ((p[0] - c["cx"]) / c["hx"], (p[1] - c["cy"]) / c["hy"])
def denorm(c, u): return (c["cx"] + u[0] * c["hx"], c["cy"] + u[1] * c["hy"])
def boxnorm(b, p): return [(p[i] - b[0][i]) / max(b[1][i] - b[0][i], 1e-4) for i in range(3)]
def boxden(b, u): return [b[0][i] + u[i] * (b[1][i] - b[0][i]) for i in range(3)]

R, M = Body(nmesh), Body(mm)
R.landmarks(); M.landmarks()
for nm, b in (("ref", R), ("target", M)):
    log(nm, "crotch %.3f armpit %.3f neck %.3f top %.3f tips L %.3f R %.3f" % (b.crotch, b.armpit, b.neck, b.top, b.tip[1], b.tip[-1]))
S = M.top / R.top
def zmap(z): return float(np.interp(z, R.knots, M.knots))

# ---------------------------------------------------------------- joint fitting
bones = arm.data.bones
JN = {b.name: Vector(b.head_local) for b in bones}
RN = {b.name: b.matrix_local.to_3x3().normalized() for b in bones}
LN = {b.name: b.length for b in bones}
JM = {}

def fit_central(n):
    j = JN[n]; z = zmap(j.z)
    JM[n] = Vector((*denorm(M.central(z), norm(R.central(j.z), j)), z))

for n in ("spine_01", "spine_02", "spine_03", "neck_01", "clavicle_l", "clavicle_r", "upperarm_l", "upperarm_r"):
    fit_central(n)
for n in ("head", "head_end", "headfront"):
    j = JN[n]; u = boxnorm(R.headbox, j); p = boxden(M.headbox, u)
    JM[n] = Vector((p[0], p[1], zmap(j.z)))
# hips: thighs sit over the leg sections just below the crotch, pelvis keeps its offset from their midpoint
for sfx, sgn in (("l", 1), ("r", -1)):
    n = "thigh_" + sfx; j = JN[n]
    cr, cm = R.side(0.40 * R.top, sgn), M.side(0.40 * M.top, sgn)
    JM[n] = Vector((cm["cx"] + (j.x - cr["cx"]) * S, cm["cy"] + (j.y - cr["cy"]) * S, zmap(j.z)))
mid_n, mid_m = (JN["thigh_l"] + JN["thigh_r"]) / 2, (JM["thigh_l"] + JM["thigh_r"]) / 2
JM["pelvis"] = mid_m + (JN["pelvis"] - mid_n) * S
JM["pelvis"].z = zmap(JN["pelvis"].z)

def hand_frame(b, W, E, sgn):
    d = (W - E).normalized(); Wn = np.array(W); dn = np.array(d)
    rel = b.co - Wn; along = rel @ dn
    radial = np.linalg.norm(rel - np.outer(along, dn), axis=1)
    p = b.co[(along > 0) & (along < 0.25) & (radial < 0.10) & (b.co[:, 0] * sgn > 0.1)]
    C = p.mean(0); w, v = np.linalg.eigh(np.cov((p - C).T))
    ax = [Vector(v[:, i]) for i in np.argsort(w)[::-1]]
    if ax[0].dot(d) < 0: ax[0] = -ax[0]
    if ax[1].dot(Vector((0, -1, 0))) < 0: ax[1] = -ax[1]
    ax[2] = ax[0].cross(ax[1])
    F = Matrix((ax[0], ax[1], ax[2])).transposed()
    ext = [np.percentile((p - C) @ np.array(a), 98) - np.percentile((p - C) @ np.array(a), 2) for a in ax]
    return Vector(C), F, ext, len(p)

QH = {}
for sfx, sgn in (("l", 1), ("r", -1)):
    for n in ("calf_" + sfx, "foot_" + sfx):
        j = JN[n]; z = zmap(j.z)
        cr = R.side(j.z, sgn); cm = M.side(z, sgn)
        JM[n] = Vector((*denorm(cm, norm(cr, j)), z))
    ank = JN["foot_" + sfx].z * 0.6
    def footbox(b, zlim):
        q = b.co[(b.co[:, 2] < zlim) & (b.co[:, 0] * sgn > 0.02)]
        return (q.min(0), q.max(0))
    fr, fm = footbox(R, ank), footbox(M, zmap(ank))
    for n in ("ball_" + sfx, "ball_%s_end" % sfx):
        j = JN[n]; p = boxden(fm, boxnorm(fr, j))
        JM[n] = Vector((p[0], p[1], zmap(j.z)))
    # arm: wrist from the fingertips, elbow at the same shoulder-wrist fraction
    sh, el, wr = "upperarm_" + sfx, "lowerarm_" + sfx, "hand_" + sfx
    wz = M.tip[sgn] + (JN[wr].z - R.tip[sgn]) * S
    JM[wr] = Vector((*denorm(M.arm_cluster(sgn, wz), norm(R.arm_cluster(sgn, JN[wr].z), JN[wr])), wz))
    t = (JN[sh].z - JN[el].z) / (JN[sh].z - JN[wr].z)
    ez = JM[sh].z - t * (JM[sh].z - JM[wr].z)
    JM[el] = Vector((*denorm(M.arm_cluster(sgn, ez), norm(R.arm_cluster(sgn, JN[el].z), JN[el])), ez))
    # hand: principal axes of the hand point cloud (length / width / thickness) of both bodies
    Cn, Fn, en, nn = hand_frame(R, JN[wr], JN[el], sgn)
    Cm, Fm, em, nm = hand_frame(M, JM[wr], JM[el], sgn)
    Rh = Fm @ Fn.transposed()
    qf = (JN[wr] - JN[el]).rotation_difference(JM[wr] - JM[el])
    k = [min(max(em[i] / en[i], 0.7), 1.4) for i in range(3)]
    if Rh.to_quaternion().rotation_difference(qf).angle > math.radians(35):
        log("hand_%s: PCA frame disagrees with forearm by >35 deg - rigid forearm fallback" % sfx)
        Rh = qf.to_matrix(); Cn, Cm, Fn, Fm, k = JN[wr], JM[wr], Matrix.Identity(3), qf.to_matrix(), [S] * 3
    log("hand_%s: %d/%d pts, extents ref %s target %s, scale %s, turn %.1f deg" % (sfx, nn, nm,
        [round(e, 3) for e in en], [round(e, 3) for e in em], [round(x, 2) for x in k], math.degrees(Rh.to_quaternion().angle)))
    D = Matrix.Diagonal(Vector(k))
    QH[wr] = Rh.to_quaternion()
    def sub(b):
        for c in b.children:
            JM[c.name] = Cm + Fm @ D @ Fn.transposed() @ (JN[c.name] - Cn); sub(c)
    sub(bones[wr])

missing = [b.name for b in bones if b.name not in JM]
assert not missing, missing

# ---------------------------------------------------------------- rest frames (minimal rotation, keeps rolls)
MAIN = {"pelvis": "spine_01", "spine_03": "neck_01", "head": "head_end",
        "hand_l": "middle_01_l", "hand_r": "middle_01_r"}
def walk(b):
    yield b
    for c in b.children: yield from walk(c)
order = [x for r in bones if r.parent is None for x in walk(r)]
def in_hand(b):
    while b:
        if b.name in QH: return b.name
        b = b.parent
QD = {}
for b in order:
    kids = [c.name for c in b.children]
    main = MAIN.get(b.name, kids[0] if len(kids) == 1 else None)
    h = in_hand(b)
    if b.name in QH:
        QD[b.name] = QH[b.name]
    elif main and h:      # fingers: hand frame, then aligned onto the fitted child
        base = QD[b.parent.name]
        QD[b.name] = (base @ (JN[main] - JN[b.name])).rotation_difference(JM[main] - JM[b.name]) @ base
    elif main:
        QD[b.name] = (JN[main] - JN[b.name]).rotation_difference(JM[main] - JM[b.name])
    else:
        QD[b.name] = QD[b.parent.name] if b.parent else Quaternion()
RM = {n: QD[n].to_matrix() @ RN[n] for n in JN}

for n in ("pelvis", "spine_03", "head", "upperarm_l", "lowerarm_l", "hand_l", "thigh_l", "calf_l", "foot_l", "upperarm_r", "lowerarm_r"):
    log("joint %-11s ref(%.3f %.3f %.3f) -> (%.3f %.3f %.3f) turn %.1f deg" % (n, *JN[n], *JM[n], math.degrees(QD[n].angle)))

# reference mesh posed into the fitted skeleton = source of weights
arm2 = arm.copy(); arm2.data = arm.data.copy(); bpy.context.scene.collection.objects.link(arm2)
nmesh.modifiers[0].object = arm2
sel([arm2], arm2); bpy.ops.object.mode_set(mode='POSE')
for b in order:
    pb = arm2.pose.bones[b.name]
    pb.matrix = Matrix.LocRotScale(JM[b.name], RM[b.name], Vector((S, S, S)))
    bpy.context.view_layer.update()
bpy.ops.object.mode_set(mode='OBJECT')
sel([nmesh], nmesh); bpy.ops.object.modifier_apply(modifier=nmesh.modifiers[0].name)

# fitted rest pose on the exported armature
sel([arm], arm); bpy.ops.object.mode_set(mode='EDIT')
ebs = arm.data.edit_bones
for eb in ebs: eb.use_connect = False
for b in order:
    eb = ebs[b.name]
    eb.matrix = Matrix.LocRotScale(JM[b.name], RM[b.name], None)
    eb.length = LN[b.name] * S
bpy.ops.object.mode_set(mode='OBJECT')

# joints inside the body?
bvh = BVHTree.FromObject(mm, bpy.context.evaluated_depsgraph_get())
outside = []
for n, p in JM.items():
    loc, nor, i, d = bvh.find_nearest(p)
    if loc is not None and (p - loc).dot(nor) > 0: outside.append("%s(%.1fcm)" % (n, d * 100))
log("joints outside target surface:", ", ".join(outside) or "none")

# ---------------------------------------------------------------- skin
sel([mm], mm)
for vg in list(mm.vertex_groups): mm.vertex_groups.remove(vg)
# Region-aware nearest-neighbour transfer from the posed reference: a target vertex only takes weights from
# reference vertices of the same body region (arm / torso / leg, measured on each mesh's own sections), so the
# jacket flank never picks up the arm and touching thighs never swap legs. k nearest, inverse-distance^2 average.
from mathutils.kdtree import KDTree
names = [b.name for b in bones]; gi = {n: i for i, n in enumerate(names)}
src_co = coords(nmesh)                                   # posed reference
NW = np.zeros((len(src_co), len(names)), dtype=np.float32)
gmap = {g.index: gi[g.name] for g in nmesh.vertex_groups if g.name in gi}
for v in nmesh.data.vertices:
    for g in v.groups:
        if g.group in gmap: NW[v.index, gmap[g.group]] = g.weight
reg_n = R.regions(JN["thigh_l"].z)
reg_m = M.regions(JM["thigh_l"].z)
for b_, nm_ in ((R, "ref"), (M, "target")):
    log(nm_, "arm trace L:", " ".join("%.2f:%.3f-%.3f" % (z, c["x0"], c["x1"]) for z, c in b_.arm_boxes[1][::6]))
log("regions ref", np.bincount(reg_n, minlength=5).tolist(), "target", np.bincount(reg_m, minlength=5).tolist())
K = 8
MW = np.zeros((len(M.co), len(names)), dtype=np.float32)
for r in range(5):
    si = np.nonzero(reg_n == r)[0]; ti = np.nonzero(reg_m == r)[0]
    if len(ti) == 0: continue
    if len(si) == 0: si = np.arange(len(src_co))
    kd = KDTree(len(si))
    for j, i in enumerate(si): kd.insert(src_co[i], j)
    kd.balance()
    for i in ti:
        hits = kd.find_n(M.co[i], K)
        w = np.array([1.0 / (h[2] * h[2] + 1e-6) for h in hits]); idx = si[[h[1] for h in hits]]
        MW[i] = (NW[idx] * w[:, None]).sum(0) / w.sum()
# Meshy's reference weights let the jacket flank follow the upper arm (the flank balloons out when the arm is
# raised - the reference shows it too). Torso vertices below the armpit drop arm/clavicle influence (8 cm fade),
# the freed weight goes to the remaining (spine) bones.
tor = np.nonzero(reg_m == 0)[0]
fore = [gi[n] for n in names if n.split("_")[0] in ("lowerarm", "hand", "thumb", "index", "middle", "ring", "pinky")]
upper = [gi[n] for n in names if n.split("_")[0] in ("clavicle", "upperarm")]
before_arm = MW[np.ix_(tor, fore + upper)].sum(1)
MW[np.ix_(tor, fore)] = 0                                   # the torso never follows the forearm/hand
fade = np.clip((M.armpit + 0.02 - M.co[tor, 2]) / 0.06, 0, 1)
MW[np.ix_(tor, upper)] *= (1 - fade)[:, None]                # shoulder keeps them, the flank below doesn't
spines = [gi[n] for n in ("pelvis", "spine_01", "spine_02", "spine_03")]
empty = tor[MW[tor].sum(1) < 1e-3]
for i in empty:
    MW[i, spines[int(np.argmin([abs(M.co[i, 2] - JM[names[c]].z) for c in spines]))]] = 1.0
lost = before_arm - MW[np.ix_(tor, fore + upper)].sum(1)
log("flank fix: %d torso verts lost arm/clavicle weight (mean %.2f), %d needed a spine fallback"
    % (int((lost > 0.01).sum()), float(lost[lost > 0.01].mean()) if (lost > 0.01).any() else 0, len(empty)))
for n in names: mm.vertex_groups.new(name=n)
# keep the 4 strongest, drop < 0.01, normalise
top = np.argsort(-MW, axis=1)[:, :4]
for i in range(len(MW)):
    ws = MW[i, top[i]]; keep = ws >= 0.01
    if not keep.any(): keep[0] = True
    ws = ws[keep] / ws[keep].sum()
    for g, w in zip(top[i][keep], ws):
        mm.vertex_groups[int(g)].add([i], float(w), 'REPLACE')
bpy.ops.object.vertex_group_clean(group_select_mode='ALL', limit=0.01)
bpy.ops.object.vertex_group_limit_total(group_select_mode='ALL', limit=4)
bpy.ops.object.vertex_group_normalize_all(group_select_mode='ALL', lock_active=False)
unw = sum(1 for v in mm.data.vertices if not v.groups)
log("weights: %d groups, unweighted verts %d" % (len(mm.vertex_groups), unw))

nm_mats = list(nmesh.data.materials); nm_data = nmesh.data
bpy.data.objects.remove(nmesh); bpy.data.objects.remove(arm2); bpy.data.meshes.remove(nm_data)
for m_ in nm_mats:
    if m_ and m_.users == 0: bpy.data.materials.remove(m_)
for m_ in mm.data.materials:
    if m_ and "." in m_.name: m_.name = m_.name.rsplit(".", 1)[0]      # Material_0.001 -> Material_0
mm.name = "Mesh_0"; mm.data.name = "Mesh_0"
mm.parent = arm
md = mm.modifiers.new("Armature", 'ARMATURE'); md.object = arm

# ---------------------------------------------------------------- export
os.makedirs(os.path.dirname(OUT), exist_ok=True)
sel([arm, mm], arm)
bpy.ops.export_scene.gltf(filepath=OUT, export_format='GLB', use_selection=True, export_yup=True,
                          export_skins=True, export_animations=False, export_morph=False, export_apply=False)
log("exported", OUT, os.path.getsize(OUT))
blend = (CMP or os.path.splitext(OUT)[0]) + "_rig.blend"

# ---------------------------------------------------------------- checks against the reference glb
def gl(p):
    b = open(p, 'rb').read(); l = struct.unpack('<I', b[12:16])[0]
    return json.loads(b[20:20 + l])
def joints(j):
    nodes = j["nodes"]; par = {}
    for i, n in enumerate(nodes):
        for c in n.get("children", []): par[c] = i
    sk = j["skins"][0]; names = {}
    def glob(i):
        n = nodes[i]
        Mx = Matrix.LocRotScale(Vector(n.get("translation", (0, 0, 0))),
                                Quaternion([n.get("rotation", (0, 0, 0, 1))[k] for k in (3, 0, 1, 2)]),
                                Vector(n.get("scale", (1, 1, 1))))
        return glob(par[i]) @ Mx if i in par else Mx
    for i in sk["joints"]:
        names[nodes[i]["name"]] = (nodes[par[i]]["name"] if i in par else None, glob(i), nodes[i].get("translation", (0, 0, 0)))
    return names
jr, jo = joints(gl(REF)), joints(gl(OUT))
same_names = sorted(jr) == sorted(jo)
same_par = same_names and all(jr[n][0] == jo[n][0] for n in jr)
log("bones ref %d out %d  names identical %s  parents identical %s" % (len(jr), len(jo), same_names, same_par))
log("only in ref:", sorted(set(jr) - set(jo)), "only in out:", sorted(set(jo) - set(jr)))
worst = (0, "")
for n in jr:
    if n not in jo: continue
    Gr, Go = jr[n][1].to_3x3().normalized(), jo[n][1].to_3x3().normalized()
    xr, xo = Gr.col[0], Go.col[0]
    q = xr.rotation_difference(xo)
    yr = q @ Gr.col[1]; tw = math.degrees(yr.angle(Go.col[1]))
    if tw > worst[0]: worst = (tw, n)
log("max twist (roll) difference after aligning bone X axes: %.2f deg (%s)" % worst)
tw2 = []
for n in jr:
    if n in jo and not any(k in n for k in ("thumb", "index", "middle", "ring", "pinky", "hand")):
        Gr, Go = jr[n][1].to_3x3().normalized(), jo[n][1].to_3x3().normalized()
        q = Gr.col[0].rotation_difference(Go.col[0]); tw2.append((math.degrees((q @ Gr.col[1]).angle(Go.col[1])), n))
log("max twist outside the hands: %.2f deg (%s)" % max(tw2))
offx = []
for n in jr:
    if n in jo and jr[n][0]:
        t = Vector(jo[n][2]); tr = Vector(jr[n][2])
        if tr.length > 1e-3: offx.append((abs(math.degrees(tr.angle(t))) if t.length > 1e-6 else 0, n))
offx.sort(); log("largest local-offset direction change vs ref:", ["%s %.1f" % (n, a) for a, n in offx[-5:]])
jout = gl(OUT); jin = gl(SRC)
log("images in:", [(i.get("name"), i.get("mimeType"), jin["bufferViews"][i["bufferView"]]["byteLength"]) for i in jin["images"]])
log("images out:", [(i.get("name"), i.get("mimeType"), jout["bufferViews"][i["bufferView"]]["byteLength"]) for i in jout["images"]])
log("materials out:", jout["materials"], "animations:", len(jout.get("animations", [])))

pos_in = jin["accessors"][jin["meshes"][0]["primitives"][0]["attributes"]["POSITION"]]["count"]
pos_out = jout["accessors"][jout["meshes"][0]["primitives"][0]["attributes"]["POSITION"]]["count"]
log("vertices in %d out %d; scene roots %s; nodes %d; meshes %s" % (pos_in, pos_out,
    [jout["nodes"][i]["name"] for i in jout["scenes"][0]["nodes"]], len(jout["nodes"]), [m["name"] for m in jout["meshes"]]))
if CMP:
    bpy.ops.wm.save_as_mainfile(filepath=blend)
    log("saved", blend, "- pose test: rig_pose_test.py -- <out.glb> <prefix>")
log("DONE")
