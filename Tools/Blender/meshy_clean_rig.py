# #82 (2026-11-21): skin Meshy characters to the standard UE5-mannequin bones only.
#   blender --background --factory-startup --python Tools/Blender/meshy_clean_rig.py -- [<id> ...]
# (set TEMP/TMP to Saved/tmp_blender). Input  Downloads/ai_assets/<id>/<id>_rigged.glb (Meshy "ue5_mannequin" rig),
# output Downloads/ai_assets/<id>/clean/<id>_rigged.glb (same file name, so it re-imports over the same UE asset).
#
# Why. Meshy's auto-rigger sometimes adds stray bones named Bone_NNN. On Elena they sit INSIDE the leg chains
# (thigh_l -> Bone_004 -> calf_l) and most of her hip/coat weights hang on a bone parented to that knee-level bone.
# Blender never rotates such a bone, so the model looks fine there, but Unreal's IK retargeter bends every bone of a
# chain: the knee-level bone turns and swings the hip/coat vertices down -> the long torn coat with the flap between
# the legs. Games skin every character to one standard skeleton; extra bones only for parts that get their own
# physics. So: every stray bone's weight moves to the standard bones its vertices lie on (a smooth split by distance
# to the bone segments; leg strays -> thigh/calf/foot, Elena's shoulder strays -> clavicle/upperarm) and dangling
# chains under the pelvis/head keep moving rigidly with it; the stray bones become inert. The skeleton itself
# (names, hierarchy, rest pose, inverse binds) is untouched, so the UE skeleton, IK rig, retargeter and the
# retargeted clips stay valid: only the skin weights change on re-import.
import bpy, sys, os, json, struct, ast
import numpy as np
from mathutils import Vector

ROOT = "F:/UE_project/YggdrasilsPeak"
AI = f"{ROOT}/Downloads/ai_assets"
IDS = ["elena_ai", "pema_body", "hale", "nima_climb", "sangye_climb"]
ARGS = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
SEL = [a for a in ARGS if not a.startswith("--")] or IDS

# reuse the glTF export + input/output comparison from the headless-body tool (functions only, no side effects)
_src = open(f"{ROOT}/Tools/Blender/meshy_headless.py", encoding="utf-8").read()
_keep = [n for n in ast.parse(_src).body if isinstance(n, ast.FunctionDef)
         and n.name in ("export_glb", "read_glb", "accessor", "glb_skeleton", "glb_verts", "check")]
exec(compile(ast.Module(body=_keep, type_ignores=[]), "meshy_headless", "exec"), globals())


def log(*a):
    print("CLEANRIG", *a, flush=True)


def stray(name):
    return name.startswith("Bone_")


report = {}
for gid in SEL:
    src = f"{AI}/{gid}/{gid}_rigged.glb"
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.ops.import_scene.gltf(filepath=src)
    arm = next(o for o in bpy.data.objects if o.type == 'ARMATURE')
    ob = max((o for o in bpy.data.objects if o.type == 'MESH'), key=lambda o: len(o.data.vertices))
    for o in list(bpy.data.objects):
        if o not in (arm, ob):
            bpy.data.objects.remove(o, do_unlink=True)
    bones = arm.data.bones
    vg = ob.vertex_groups
    me = ob.data
    co = np.empty(len(me.vertices) * 3); me.vertices.foreach_get("co", co); co = co.reshape(-1, 3)
    # standard bone segments (head -> first standard child head)
    segs = {}
    for b in bones:
        if stray(b.name) or b.name.endswith("_end"):
            continue
        kids = [c for c in b.children if not stray(c.name)]
        segs[b.name] = (np.array(b.head_local), np.array(kids[0].head_local if kids else b.tail_local))

    def segdist(P, name):
        h, t = segs[name]; d = t - h; L2 = max(float(d @ d), 1e-12)
        f = np.clip((P - h) @ d / L2, 0, 1)
        return np.linalg.norm(P - (h + f[:, None] * d), axis=1)

    # where each stray bone's skin really lies: Elena's shoulder/upper-sleeve bones hang off the FOREARM, so the
    # ancestor rule would keep the sleeve following the forearm. Candidates = the standard bones nearest to the
    # vertices it holds (>= 90 % of them, at most 3); dangling chains under the pelvis or head (apron, hair, hood) keep
    # their ancestor (they move rigidly with it in game today).
    target = {}
    for b in bones:
        if not stray(b.name):
            continue
        a = b.parent
        while a is not None and stray(a.name):
            a = a.parent
        anc = a.name if a else None
        cand = [anc]
        if anc not in ("pelvis", "head", None) and b.name in vg:
            gi = vg[b.name].index
            idx = np.array([v.index for v in me.vertices for x in v.groups if x.group == gi and x.weight > 0.4], dtype=np.int64)
            if len(idx):
                idx = idx[:: max(1, len(idx) // 600)]
                names = list(segs)
                D = np.stack([segdist(co[idx], n) for n in names], 1)
                votes = np.bincount(D.argmin(1), minlength=len(names))
                order = np.argsort(-votes); cum = 0; cand = []
                for k in order[:3]:
                    if votes[k] == 0: break
                    cand.append(names[k]); cum += votes[k]
                    if cum >= 0.9 * votes.sum(): break
        target[b.name] = cand
    if not target:
        log(gid, "no stray bones, skipped"); continue
    SIG = 0.03                               # soft split between candidates: exp(-((d - dmin) / 3 cm)^2)
    moved, touched = {}, 0
    for v in me.vertices:
        add = {}
        for g in list(v.groups):
            nm = vg[g.group].name
            if nm in target and g.weight > 0:
                cand = [c for c in target[nm] if c]
                if len(cand) == 1:
                    share = {cand[0]: 1.0}
                elif cand:
                    p = co[v.index][None, :]
                    d = np.array([segdist(p, c)[0] for c in cand]); s = np.exp(-((d - d.min()) / SIG) ** 2)
                    share = dict(zip(cand, s / s.sum()))
                else:
                    share = {}
                for c, f in share.items():
                    add[c] = add.get(c, 0.0) + g.weight * float(f)
                moved[nm] = moved.get(nm, 0) + 1
                vg[g.group].remove([v.index])
        if add:
            touched += 1
            for nm, w in add.items():
                if nm not in vg:
                    vg.new(name=nm)
                g = vg[nm]
                cur = next((x.weight for x in v.groups if x.group == g.index), 0.0)
                g.add([v.index], cur + w, 'REPLACE')
    for nm in target:                       # drop the now-empty stray groups (the bones stay in the skeleton)
        if nm in vg:
            vg.remove(vg[nm])
    left = [g.name for g in vg if stray(g.name)]
    os.makedirs(f"{AI}/{gid}/clean", exist_ok=True)
    out = f"{AI}/{gid}/clean/{gid}_rigged.glb"
    export_glb(arm, ob, out)
    chk = check(src, out)
    report[gid] = dict(stray_to=target, verts_moved_per_bone=moved, verts_touched=touched, stray_groups_left=left,
                       check=chk)
    log(gid, json.dumps(report[gid], default=str))
os.makedirs(f"{ROOT}/Saved/tmp_blender", exist_ok=True)
with open(f"{ROOT}/Saved/tmp_blender/clean_rig_report.json", "w") as fh:
    json.dump(report, fh, indent=1, default=str)
log("DONE", len(report))
