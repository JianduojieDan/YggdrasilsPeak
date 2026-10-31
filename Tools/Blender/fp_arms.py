# Post-pass on a character rigged by rig_glb_character.py:
#  1) verts weighted to an arm but lying outside the arm (e.g. an ice axe hanging at the hip next to the hand)
#     are re-weighted to the nearest torso/leg bone, so they stop swinging with the arm;
#  2) replaces the AI hands with the Survival Character pack's rigged gloves (on the full body too);
#  3) builds SKM_<Name>_FPArms: forearms + gloves only, for the first-person view body (no shoulders, torso,
#     head or hip gear in front of the camera).
#   blender --background --factory-startup --python fp_arms.py -- <Name>
import sys, os, bpy, bmesh
from mathutils import Vector, Matrix
from mathutils.geometry import intersect_point_line
sys.path.append(os.path.dirname(__file__))
from ygg_blend import OUT, export_skeletal

NAME = sys.argv[sys.argv.index("--") + 1]
EXP = os.path.join(OUT, "Export", "Skel", NAME)
bpy.ops.wm.open_mainfile(filepath=os.path.join(OUT, f"{NAME}.blend"))
arm = bpy.data.objects["root"]; body = bpy.data.objects[NAME]
for pb in arm.pose.bones:  # the saved file may hold the test pose
    pb.rotation_mode = 'QUATERNION'; pb.rotation_quaternion = (1, 0, 0, 0); pb.location = (0, 0, 0)
B = arm.data.bones
ARM_PREFIX = ("clavicle", "upperarm", "lowerarm", "hand", "index", "middle", "ring", "pinky", "thumb")
is_arm = lambda n: n.startswith(ARM_PREFIX)

def seg_dist(p, a, b):
    t = max(0.0, min(1.0, intersect_point_line(p, a, b)[1]))
    return (p - a.lerp(b, t)).length

caps = {}  # side -> list of (a, b, radius, part)
for s in ("l", "r"):
    caps[s] = [(B[f"upperarm_{s}"].head_local, B[f"lowerarm_{s}"].head_local, 0.10, "upper"),
               (B[f"lowerarm_{s}"].head_local, B[f"hand_{s}"].head_local, 0.085, "fore"),
               (B[f"hand_{s}"].head_local, B[f"middle_03_{s}"].tail_local, 0.075, "hand"),
               (B[f"hand_{s}"].head_local, B[f"thumb_03_{s}"].tail_local, 0.05, "hand")]
others = ["pelvis", "spine_01", "spine_02", "spine_03", "thigh_l", "thigh_r", "calf_l", "calf_r"]
oseg = {n: (B[n].head_local, B[n].tail_local) for n in others}

names = [g.name for g in body.vertex_groups]
me = body.data
fixed = 0
for v in me.vertices:
    # any arm influence on a point that lies closer to the torso/legs than to that arm (the hand rests on the thigh
    # in the rest pose, so the auto-weights leak onto the leg and the hip axe) is moved to the nearest torso/leg bone
    arm_groups = [g for g in v.groups if is_arm(names[g.group]) and g.weight > 0]
    if not arm_groups: continue
    n = names[max(arm_groups, key=lambda g: g.weight).group]
    side = n[-1] if n[-2] == "_" else "l"
    d_arm = min(seg_dist(v.co, a, b) for a, b, r, _ in caps[side])
    best = min(others, key=lambda o: seg_dist(v.co, *oseg[o]))
    if d_arm <= seg_dist(v.co, *oseg[best]): continue   # really part of the arm
    moved = sum(g.weight for g in arm_groups)
    for g in arm_groups:
        body.vertex_groups[g.group].remove([v.index])
    body.vertex_groups[best].add([v.index], moved, 'ADD')
    fixed += 1
bpy.context.view_layer.objects.active = body
bpy.ops.object.vertex_group_normalize_all(lock_active=False)
print("REWEIGHTED off-arm verts:", fixed)

# Gloves: the AI hands are fused lumps (no separate fingers), so they are cut off and replaced by the rigged gloves
# from the Survival Character pack. The glove mesh is moved from its own skeleton's rest pose onto Manny's by
# linear blend skinning with per-bone (Manny rest * glove rest^-1) matrices; bone names match.
HAND_BONES = ("hand", "index", "middle", "ring", "pinky", "thumb")
GLOVES = os.path.join(OUT, "Source", "SK_Survival_Character_Gloves.fbx")
ARGS = sys.argv[sys.argv.index("--") + 1:]
if "--gloves" in ARGS and "GLOVE" not in body.vertex_groups and os.path.exists(GLOVES):
    names = [g.name for g in body.vertex_groups]
    bm = bmesh.new(); bm.from_mesh(body.data); bm.verts.ensure_lookup_table()
    dl = bm.verts.layers.deform.active
    cut = [v for v in bm.verts if sum(w for gi_, w in v[dl].items() if names[gi_].startswith(HAND_BONES)) > 0.33]
    bmesh.ops.delete(bm, geom=cut, context='VERTS'); bm.to_mesh(body.data); bm.free()
    print("AI hand verts removed:", len(cut))

    before = set(bpy.data.objects)
    bpy.ops.import_scene.fbx(filepath=GLOVES)
    new = [o for o in bpy.data.objects if o not in before]
    garm = next(o for o in new if o.type == 'ARMATURE'); gmesh = next(o for o in new if o.type == 'MESH')
    bpy.context.view_layer.update()
    gnames = {g.index: g.name for g in gmesh.vertex_groups}
    M = {}
    for b in garm.data.bones:
        if b.name in arm.data.bones:
            g = garm.matrix_world @ b.matrix_local   # carries the import's 0.01 scale: keep only position + rotation
            g = Matrix.LocRotScale(g.translation, g.to_quaternion(), None)
            M[b.name] = (arm.matrix_world @ arm.data.bones[b.name].matrix_local) @ g.inverted()
    gw = gmesh.matrix_world.copy()
    for v in gmesh.data.vertices:
        p = gw @ v.co
        acc = Vector(); tot = 0.0
        for g in v.groups:
            n = gnames[g.group]
            if n in M and g.weight > 0:
                acc += (M[n] @ p) * g.weight; tot += g.weight
        v.co = acc / tot if tot > 0 else p
    gmesh.parent = None; gmesh.matrix_world = Matrix.Identity(4)
    for m in list(gmesh.modifiers): gmesh.modifiers.remove(m)
    mark = gmesh.vertex_groups.new(name="GLOVE"); mark.add(list(range(len(gmesh.data.vertices))), 1.0, 'REPLACE')
    for o in new:
        if o is not gmesh: bpy.data.objects.remove(o)
    bpy.ops.object.select_all(action='DESELECT'); gmesh.select_set(True); body.select_set(True)
    bpy.context.view_layer.objects.active = body
    bpy.ops.object.join()
    print("gloves merged:", len(M), "bones mapped")

# arms-only copy (only with --fp-arms): forearms + hands, and the upper arm only within 12 cm of the elbow
fp = body.copy(); fp.data = body.data.copy(); fp.name = f"{NAME}_FPArms"
bpy.context.collection.objects.link(fp)
FP_ARMS = "--fp-arms" in ARGS
FORE = ("lowerarm",) + HAND_BONES
names = [g.name for g in fp.vertex_groups]
glove_idx = fp.vertex_groups["GLOVE"].index if "GLOVE" in fp.vertex_groups else -1
def keep(v):
    if not v.groups: return False
    if any(g.group == glove_idx for g in v.groups): return True   # the gloves are the hands
    n = names[max(v.groups, key=lambda g: g.weight).group]
    if n.startswith(FORE): return True
    if n.startswith("upperarm"):  # only the sleeve just above the elbow
        s = n[-1]; return (v.co - B[f"lowerarm_{s}"].head_local).length < 0.12
    return False
gone_idx = {v.index for v in fp.data.vertices if not keep(v)}
bm = bmesh.new(); bm.from_mesh(fp.data); bm.verts.ensure_lookup_table()
gone = [bm.verts[i] for i in gone_idx]
bmesh.ops.delete(bm, geom=gone, context='VERTS')
bm.to_mesh(fp.data); bm.free()
print("FP arms tris:", sum(len(p.vertices) - 2 for p in fp.data.polygons))

for obj, fname in ((body, f"SKM_{NAME}"), (fp, f"SKM_{NAME}_FPArms"))[:2 if FP_ARMS else 1]:
    export_skeletal(arm, obj, os.path.join(EXP, f"{fname}.fbx"))
    print("EXPORTED", fname)
bpy.ops.wm.save_as_mainfile(filepath=os.path.join(OUT, f"{NAME}.blend"))

# check: walk-like pose, axe should stay at the hip; arms-only render
s = bpy.context.scene
P = arm.pose.bones
for n, ax, deg in (("upperarm_r", 'z', -50), ("upperarm_l", 'z', 50), ("lowerarm_r", 'z', 60)):
    P[n].rotation_mode = 'XYZ'; setattr(P[n].rotation_euler, ax, deg * 3.14159 / 180)
cam = bpy.data.objects.new("Cam", bpy.data.cameras.new("Cam")); s.collection.objects.link(cam); s.camera = cam
sun = bpy.data.objects.new("Sun", bpy.data.lights.new("Sun", 'SUN')); s.collection.objects.link(sun)
sun.rotation_euler = (0.8, 0.2, 0.6); sun.data.energy = 3
s.render.engine = 'BLENDER_EEVEE'; s.render.resolution_x = s.render.resolution_y = 700
for tag, hide in (("armswing", fp), ("fparms", body)):
    for o in (body, fp): o.hide_render = o is hide
    cam.location = (1.4, -2.8, 1.3); cam.rotation_euler = (Vector((0, 0, 1.0)) - cam.location).to_track_quat('-Z', 'Y').to_euler()
    s.render.filepath = os.path.join(OUT, "Preview", f"{NAME}_{tag}.png"); bpy.ops.render.render(write_still=True)
