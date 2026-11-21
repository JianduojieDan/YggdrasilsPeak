# Deforms a Meshy character with the EXACT pose the game plays and renders it (#82, 2026-11-21).
#   blender --background --factory-startup --python Tools/Blender/ue_pose_preview.py -- <id> <glb> <tag> [clip:frame ...]
# Poses come from Tools/Story/dump_npc_pose (Saved/tmp_blender/pose_<id>.json: component-space bone transforms of
# the retargeted clips + the reference pose). Each bone gets delta = anim * ref^-1 in UE space, carried into Blender
# space by the map fitted from the reference bone heads (UE cm, left-handed -> Blender m), so bone-axis conventions
# never matter. Renders front / side / low-front views per pose -> Saved/tmp_blender/uepose/<id>_<tag>_<clip>_<f>_<v>.png
import bpy, sys, os, json, math
import numpy as np
from mathutils import Matrix, Vector, Quaternion

ROOT = "F:/UE_project/YggdrasilsPeak"
OUT = f"{ROOT}/Saved/tmp_blender/uepose"
os.makedirs(OUT, exist_ok=True)
ARGS = sys.argv[sys.argv.index("--") + 1:]
gid, src, tag = ARGS[0], ARGS[1], ARGS[2]
want = ARGS[3:] or ["MF_Unarmed_Walk_Fwd:2", "MF_Unarmed_Walk_Fwd:6", "MF_Unarmed_Jog_Fwd:2"]
P = json.load(open(f"{ROOT}/Saved/tmp_blender/pose_{gid}.json"))

bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.gltf(filepath=src)
arm = next(o for o in bpy.data.objects if o.type == 'ARMATURE')
ob = max((o for o in bpy.data.objects if o.type == 'MESH'), key=lambda o: len(o.data.vertices))
for o in list(bpy.data.objects):
    if o not in (arm, ob):
        bpy.data.objects.remove(o, do_unlink=True)
B = arm.data.bones

# fit p_bl = M p_ue + t on the reference bone heads
names = [n for n in P["ref"] if n in B]
U = np.array([P["ref"][n][:3] for n in names]); Bl = np.array([list(B[n].head_local) for n in names])
A = np.hstack([U, np.ones((len(U), 1))])
X, *_ = np.linalg.lstsq(A, Bl, rcond=None)
M3 = X[:3].T; t3 = X[3]
err = np.abs(A @ X - Bl).max()
print("UEPOSE fit max err (m)", round(float(err), 5), "M", np.round(M3 * 100, 3).tolist())
C = Matrix.Identity(4)
for i in range(3):
    for j in range(3):
        C[i][j] = M3[i, j]
    C[i][3] = t3[i]
Ci = C.inverted()


def ue_mat(v):
    q = Quaternion((v[6], v[3], v[4], v[5]))           # UE quat (x,y,z,w) -> mathutils (w,x,y,z)
    m = q.to_matrix().to_4x4()
    m.translation = Vector(v[:3])
    return m


def ue_mat_lh(v):
    # UE stores rotations for a left-handed frame; as a 3x3 acting on UE coordinates the quaternion formula is the
    # same, so build it directly (the fitted C carries the handedness flip).
    return ue_mat(v)


order = [b for b in B]                                  # armature bones are listed parents-first
s = bpy.context.scene; s.render.engine = 'BLENDER_WORKBENCH'
sh = s.display.shading; sh.light = 'STUDIO'; sh.color_type = 'TEXTURE'; sh.show_backface_culling = False
sh.background_type = 'VIEWPORT'; sh.background_color = (0.33, 0.35, 0.38)
cam = bpy.data.objects.new("Cam", bpy.data.cameras.new("Cam")); s.collection.objects.link(cam); s.camera = cam
cam.data.clip_start = 0.01


def shot(path, target, d, dist, res=(460, 640)):
    s.render.resolution_x, s.render.resolution_y = res; cam.data.lens = 50
    target = Vector(target); cam.location = target + Vector(d).normalized() * dist
    cam.rotation_euler = (target - cam.location).to_track_quat('-Z', 'Y').to_euler(); s.render.filepath = path
    bpy.ops.render.render(write_still=True)


for w in want:
    clip, f = w.split(":"); f = int(f)
    fr = P["clips"][clip]["frames"][f]["bones"]
    for b in order:
        pb = arm.pose.bones[b.name]
        if b.name not in fr:
            continue
        D = ue_mat_lh(fr[b.name]) @ ue_mat_lh(P["ref"][b.name]).inverted()
        pb.matrix = (C @ D @ Ci) @ b.matrix_local
        bpy.context.view_layer.update()
    # centre the camera on the pelvis as posed
    dg = bpy.context.evaluated_depsgraph_get(); ev = ob.evaluated_get(dg); me = ev.to_mesh()
    V = np.empty(len(me.vertices) * 3); me.vertices.foreach_get("co", V); V = V.reshape(-1, 3); ev.to_mesh_clear()
    lo, hi = V.min(0), V.max(0)
    print("UEPOSE", w, "pelvis", [round(x, 3) for x in arm.pose.bones["pelvis"].head], "mesh z", round(lo[2], 3), round(hi[2], 3))
    tgt = ((lo[0] + hi[0]) / 2, (lo[1] + hi[1]) / 2, lo[2] + 0.4 * (hi[2] - lo[2]))
    for v, d in (("F", (0.1, -1, 0.12)), ("S", (1, -0.15, 0.08)), ("U", (0.1, -0.6, -0.35))):
        shot(f"{OUT}/{gid}_{tag}_{clip.split('_')[-2]}{f}_{v}.png", tgt, d, 1.9)
    for b in order:
        arm.pose.bones[b.name].matrix_basis = Matrix.Identity(4)
    bpy.context.view_layer.update()
print("UEPOSE DONE")
