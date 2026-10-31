# Turns an AI-generated (unrigged, very dense) GLB character into a game skeletal mesh on SK_Mannequin.
#   blender --background --factory-startup --python rig_glb_character.py -- <in.glb> <Name> [target_tris]
# The Manny skeleton is kept EXACTLY as in UE (bone positions untouched): ABP_FP_Copy copies poses bone by bone,
# so a moved bone puts the first-person arms in the wrong place. Instead the MESH is bent into Manny's rest pose:
#   weld seams -> decimate -> scale to Manny's height -> weight against a copy of the skeleton whose arms are aimed
#   at the mesh's hands -> pose that copy back to Manny's rest, baking the mesh into Manny's A-pose.
import sys, os, math, bpy
from mathutils import Vector, Matrix, Quaternion
sys.path.append(os.path.dirname(__file__))
from ygg_blend import reset, OUT, export_skeletal

args = sys.argv[sys.argv.index("--") + 1:]
SRC, NAME = args[0], args[1]
TARGET = int(args[2]) if len(args) > 2 else 10**9   # default: keep every triangle of the original
STRAIGHTEN_HANDS = "--straighten-hands" in args
EXP = os.path.join(OUT, "Export", "Skel", NAME); os.makedirs(EXP, exist_ok=True)
MANNY_TOP = 1.805      # SKM_Manny_Simple height (m)
HAIR = 0.03            # AI hair sticks out above the skull

def sel(*objs, active=None):
    if bpy.context.object and bpy.context.object.mode != 'OBJECT':
        bpy.ops.object.mode_set(mode='OBJECT')
    bpy.ops.object.select_all(action='DESELECT')
    for o in objs: o.select_set(True)
    bpy.context.view_layer.objects.active = active or objs[0]

def about(pivot, R):
    return Matrix.Translation(pivot) @ R.to_matrix().to_4x4() @ Matrix.Translation(-pivot)

reset()
bpy.ops.import_scene.fbx(filepath=os.path.join(OUT, "Source", "SKM_Manny_Simple.fbx"))
arm = bpy.data.objects["root"]
mw = arm.matrix_world.copy(); arm.parent = None; arm.matrix_world = mw
# Manny's own LOD0 body is kept as the source of the final skin weights (Epic's hand-made weights).
manny = bpy.data.objects["SKM_Manny_Simple_LOD0"]
for o in [o for o in bpy.data.objects if o not in (arm, manny)]:
    bpy.data.objects.remove(o)
mw = manny.matrix_world.copy(); manny.parent = None; manny.matrix_world = mw
for m in list(manny.modifiers): manny.modifiers.remove(m)
sel(arm); bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)  # cm -> m only
sel(manny); bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
manny.hide_render = True
B = arm.data.bones

bpy.ops.import_scene.gltf(filepath=SRC)
parts = [o for o in bpy.context.selected_objects if o.type == 'MESH']
sel(*parts)
if len(parts) > 1: bpy.ops.object.join()
body = bpy.context.active_object; body.name = NAME; body.parent = None
bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)

# weld the texture-seam splits first: decimating/skinning split pieces separately is what tore the surface
bpy.ops.object.mode_set(mode='EDIT'); bpy.ops.mesh.select_all(action='SELECT')
bpy.ops.mesh.remove_doubles(threshold=0.00005)
bpy.ops.object.mode_set(mode='OBJECT')
tris = sum(len(p.vertices) - 2 for p in body.data.polygons)
if tris > TARGET:
    d = body.modifiers.new("dec", 'DECIMATE'); d.ratio = TARGET / tris; d.use_collapse_triangulate = True
    bpy.ops.object.modifier_apply(modifier="dec")
for p in body.data.polygons: p.use_smooth = True

# Manny's size, feet on the ground, centred (GLB and Manny both face -Y in Blender)
zs = [v.co.z for v in body.data.vertices]
s = (MANNY_TOP + HAIR) / (max(zs) - min(zs))
body.scale = (s, s, s); body.location.z = -min(zs) * s
sel(body); bpy.ops.object.transform_apply(location=True, scale=True)
xs = [v.co.x for v in body.data.vertices]
body.location.x = -(max(xs) + min(xs)) / 2
bpy.ops.object.transform_apply(location=True)

# fitting skeleton = copy of Manny with each arm chain rotated about the shoulder to point at the mesh's hand
fit = arm.copy(); fit.data = arm.data.copy(); fit.name = "fit"; bpy.context.collection.objects.link(fit)
verts = [v.co for v in body.data.vertices]
R = {}
for side, sgn in (("l", 1), ("r", -1)):
    sh = B[f"upperarm_{side}"].head_local.copy()
    tip = max((v for v in verts if v.x * sgn > 0), key=lambda v: v.x * sgn)
    manny_tip = B[f"middle_03_{side}"].tail_local
    R[side] = (sh, (manny_tip - sh).rotation_difference(tip - sh))
    print(f"ARM {side}: aim {math.degrees(R[side][1].angle):.1f} deg")
sel(fit); bpy.ops.object.mode_set(mode='POSE')
for side, (sh, rot) in R.items():
    pb = fit.pose.bones[f"upperarm_{side}"]; pb.matrix = about(sh, rot) @ pb.matrix
bpy.context.view_layer.update()
bpy.ops.pose.armature_apply(selected=False)
bpy.ops.object.mode_set(mode='OBJECT')

# weights: watertight voxel proxy auto-weighted on the fitted skeleton, transferred to the real mesh
proxy = body.copy(); proxy.data = body.data.copy(); proxy.name = "proxy"; bpy.context.collection.objects.link(proxy)
sel(proxy); proxy.data.remesh_voxel_size = 0.015; bpy.ops.object.voxel_remesh()
sel(proxy, fit, active=fit); bpy.ops.object.parent_set(type='ARMATURE_AUTO')
for g in proxy.vertex_groups: body.vertex_groups.new(name=g.name)
dt = body.modifiers.new("dt", 'DATA_TRANSFER'); dt.object = proxy
dt.use_vert_data = True; dt.data_types_verts = {'VGROUP_WEIGHTS'}; dt.vert_mapping = 'POLYINTERP_NEAREST'
dt.layers_vgroup_select_src = 'ALL'; dt.layers_vgroup_select_dst = 'NAME'
sel(body); bpy.ops.object.modifier_apply(modifier="dt")
bpy.data.objects.remove(proxy)

# everything above the neck (face, hair, hood top) belongs to the head bone, so hiding "head" in first person
# removes all of it from in front of the camera
neck_top = fit.data.bones["head"].head_local.z + 0.015
head_g = body.vertex_groups["head"]
others = [g for g in body.vertex_groups if g.name != "head"]
hidx = [v.index for v in body.data.vertices if v.co.z > neck_top and abs(v.co.x) < 0.2]
for g in others: g.remove(hidx)
head_g.add(hidx, 1.0, 'REPLACE')
sel(body)
bpy.ops.object.vertex_group_limit_total(group_select_mode='ALL', limit=4)
bpy.ops.object.vertex_group_normalize_all(lock_active=False)
print("HEAD verts", len(hidx))

# bend the mesh into Manny's rest pose: pose the fitted skeleton's arms back, bake the deformation
sel(fit); bpy.ops.object.mode_set(mode='POSE')
for side, (sh, rot) in R.items():
    pb = fit.pose.bones[f"upperarm_{side}"]; pb.matrix = about(sh, rot.inverted()) @ pb.matrix
bpy.context.view_layer.update()
bpy.ops.object.mode_set(mode='OBJECT')
m = body.modifiers.new("bake", 'ARMATURE'); m.object = fit
sel(body); bpy.ops.object.modifier_apply(modifier="bake")
bpy.data.objects.remove(fit)

# Final skin weights: copied from Manny's own body (nearest surface point, interpolated). The proxy auto-weights
# above were only good enough to bend the arms; on 9800 loose AI pieces they let the legs bleed into each other.
# The climber now stands in Manny's rest pose and size, so each point's nearest Manny point is on the same limb.
def manny_weights():
    for g in list(body.vertex_groups): body.vertex_groups.remove(g)
    for g in manny.vertex_groups: body.vertex_groups.new(name=g.name)
    dt = body.modifiers.new("manny", 'DATA_TRANSFER'); dt.object = manny
    dt.use_vert_data = True; dt.data_types_verts = {'VGROUP_WEIGHTS'}; dt.vert_mapping = 'POLYINTERP_NEAREST'
    dt.layers_vgroup_select_src = 'ALL'; dt.layers_vgroup_select_dst = 'NAME'
    sel(body); bpy.ops.object.modifier_apply(modifier="manny")
manny_weights()

# Hands: turn each climber hand (about the wrist) so it lies like Manny's hand - same pointing direction AND same
# palm roll. A hand rotated against its bone gets wrung at the wrist as soon as the animation turns the bone.
# Only the pose at the wrist changes (blended by hand weight), not the hand's shape. Then the weights are re-copied.
def hand_frame(obj, side):
    names = {g.index: g.name for g in obj.vertex_groups}
    def centroid(prefixes, wmin):
        pts = [v.co for v in obj.data.vertices
               if sum(g.weight for g in v.groups if names[g.group].startswith(prefixes) and names[g.group].endswith("_" + side)) > wmin]
        return sum(pts, Vector()) / len(pts) if pts else None
    wrist = B[f"hand_{side}"].head_local
    hand_c = centroid(("hand", "index", "middle", "ring", "pinky", "thumb"), 0.9)
    thumb_c = centroid(("thumb",), 0.6)
    d = (hand_c - wrist).normalized()
    t = (thumb_c - hand_c); t = (t - d * t.dot(d)).normalized()
    return Matrix((d, t, d.cross(t))).transposed()   # columns = pointing, thumb side, palm normal
for s in ("l", "r"):
    rot = (hand_frame(manny, s) @ hand_frame(body, s).inverted()).to_quaternion()
    wrist = B[f"hand_{s}"].head_local
    names = {g.index: g.name for g in body.vertex_groups}
    moved = 0
    for v in body.data.vertices:
        w = min(1.0, sum(g.weight for g in v.groups if names[g.group].startswith(("hand", "index", "middle", "ring", "pinky", "thumb")) and names[g.group].endswith("_" + s)))
        if w > 0.01:
            v.co = wrist + Quaternion().slerp(rot, w) @ (v.co - wrist); moved += 1
    print(f"HAND {s}: turned {math.degrees(rot.angle):.1f} deg to Manny's hand pose ({moved} verts)")
manny_weights()
hidx = [v.index for v in body.data.vertices if v.co.z > neck_top and abs(v.co.x) < 0.2]
for g in body.vertex_groups:
    if g.name != "head": g.remove(hidx)
body.vertex_groups["head"].add(hidx, 1.0, 'REPLACE')
bpy.ops.object.vertex_group_limit_total(group_select_mode='ALL', limit=4)
bpy.ops.object.vertex_group_normalize_all(lock_active=False)
bpy.data.objects.remove(manny)
print("WEIGHTS from Manny body,", len(body.vertex_groups), "groups")

# straighten the hands: the AI hand hangs bent at the wrist, while code poses the hand BONE in line with the forearm,
# so the mesh hand must lie along Manny's hand bone (blended by hand weight so the wrist stays smooth)
HANDS = ("hand", "index", "middle", "ring", "pinky", "thumb")
gi = {g.index: g.name for g in body.vertex_groups}
for s in (("l", "r") if STRAIGHTEN_HANDS else ()):   # off by default: the user wants the model's own hand shape
    wrist = B[f"hand_{s}"].head_local
    bone_dir = (B[f"middle_01_{s}"].head_local - wrist).normalized()
    hw = {}
    for v in body.data.vertices:
        w = sum(g.weight for g in v.groups if gi[g.group].startswith(HANDS) and gi[g.group].endswith("_" + s))
        if w > 0.01: hw[v.index] = min(w, 1.0)
    core = [body.data.vertices[i].co for i, w in hw.items() if w > 0.9]
    mesh_dir = (sum(core, Vector()) / len(core) - wrist).normalized()
    rot = mesh_dir.rotation_difference(bone_dir)
    print(f"HAND {s}: straightened {math.degrees(rot.angle):.1f} deg ({len(hw)} verts)")
    for i, w in hw.items():
        v = body.data.vertices[i]
        v.co = wrist + Quaternion().slerp(rot, w) @ (v.co - wrist)

body.parent = arm
m = body.modifiers.new("Armature", 'ARMATURE'); m.object = arm

for img in bpy.data.images:
    if img.size[0] == 0 or "Manny" in img.name: continue
    tag = "Normal" if "normal" in img.name else ("MetalRough" if "metallic" in img.name else "BaseColor")
    img.filepath_raw = os.path.join(EXP, f"T_{NAME}_{tag}.png"); img.file_format = 'PNG'; img.save()
    print("TEX", tag, tuple(img.size))

tris = sum(len(p.vertices) - 2 for p in body.data.polygons)
print(f"BODY tris={tris} dims={tuple(round(x, 3) for x in body.dimensions)}")
sel(arm, body, active=arm)
export_skeletal(arm, body, os.path.join(EXP, f"SKM_{NAME}.fbx"))
print("EXPORTED")
bpy.ops.wm.save_as_mainfile(filepath=os.path.join(OUT, f"{NAME}.blend"))

# renders: rest pose (should be Manny's A-pose) and a bent test pose, close on the head for cracks
s = bpy.context.scene; s.render.engine = 'BLENDER_EEVEE'; s.render.resolution_x, s.render.resolution_y = 800, 800
w = bpy.data.worlds.new("W"); s.world = w; w.color = (0.6, 0.6, 0.65)
sun = bpy.data.objects.new("Sun", bpy.data.lights.new("Sun", 'SUN')); s.collection.objects.link(sun)
sun.rotation_euler = (0.8, 0.2, 0.6); sun.data.energy = 3
cam = bpy.data.objects.new("Cam", bpy.data.cameras.new("Cam")); s.collection.objects.link(cam); s.camera = cam
def shot(tag, loc, target):
    cam.location = loc; cam.rotation_euler = (Vector(target) - Vector(loc)).to_track_quat('-Z', 'Y').to_euler()
    s.render.filepath = os.path.join(OUT, "Preview", f"{NAME}_{tag}.png"); bpy.ops.render.render(write_still=True)
shot("rest", (0, -4.2, 1.0), (0, 0, 0.9))
P = arm.pose.bones
for n, ax, deg in (("upperarm_l", 'x', -60), ("lowerarm_r", 'z', 70), ("calf_l", 'z', 60),
                   ("thigh_l", 'z', -40), ("head", 'x', 30), ("spine_03", 'y', 20)):
    P[n].rotation_mode = 'XYZ'; setattr(P[n].rotation_euler, ax, math.radians(deg))
shot("testpose", (1.2, -3.2, 1.2), (0, 0, 0.95))
shot("headclose", (0.35, -0.75, 1.7), (0, 0, 1.6))
