# TEMPORARY 8000 m down suit built on the mannequin body (SKM_Manny_Simple LOD1).
# The mannequin is remeshed into one closed skin, inflated per zone (suit / gloves / boots / hood),
# given horizontal down baffles, then re-weighted from the original mesh so it skins to SK_Mannequin.
# Input:  Art/Blender/Source/SKM_Manny_Simple.fbx (exported from UE)
# Output: Art/Blender/Export/Skel/SKM_DownSuit_Temp.fbx
import sys, os, math
sys.path.append(os.path.dirname(__file__))
from ygg_blend import *
import bmesh

reset()
bpy.ops.import_scene.fbx(filepath=os.path.join(OUT, "Source", "SKM_Manny_Simple.fbx"))
arm = bpy.data.objects["root"]
src = bpy.data.objects["SKM_Manny_Simple_LOD1"]
for n in ("SKM_Manny_Simple_LOD0", "SKM_Manny_Simple_LOD2"):
    bpy.data.objects.remove(bpy.data.objects[n])

# drop the 0.01 empty so everything is plain metres
mw = arm.matrix_world.copy(); arm.parent = None; arm.matrix_world = mw
bpy.data.objects.remove(bpy.data.objects["SKM_Manny_Simple"])
for n in ("SKM_Manny_Simple_LodGroup",):
    if n in bpy.data.objects: bpy.data.objects.remove(bpy.data.objects[n])
bpy.ops.object.select_all(action='DESELECT')
arm.select_set(True); bpy.context.view_layer.objects.active = arm
bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
src.parent = arm
src.matrix_parent_inverse.identity()

# --- one closed skin -------------------------------------------------------
suit = src.copy(); suit.data = src.data.copy(); suit.name = "SKM_DownSuit_Temp"
bpy.context.collection.objects.link(suit)
for m in list(suit.modifiers): suit.modifiers.remove(m)
suit.vertex_groups.clear()
suit.data.materials.clear()
suit.parent = None; suit.matrix_world = src.matrix_world.copy()
bpy.ops.object.select_all(action='DESELECT')
suit.select_set(True); bpy.context.view_layer.objects.active = suit
bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)  # mesh data was in cm
# guard: a ~1.8 m body must really be ~1.8 units tall before voxel remeshing (RAM blow-up otherwise)
assert 1.5 < suit.dimensions.z < 2.2 and max(v.co.z for v in suit.data.vertices) < 3, suit.dimensions
suit.data.remesh_voxel_size = 0.015
bpy.ops.object.voxel_remesh()
dec = suit.modifiers.new("dec", 'DECIMATE'); dec.ratio = 0.35
bpy.ops.object.modifier_apply(modifier="dec")

# weights from the original body (nearest surface, interpolated)
dt = suit.modifiers.new("dt", 'DATA_TRANSFER')
dt.object = src; dt.use_vert_data = True; dt.data_types_verts = {'VGROUP_WEIGHTS'}
dt.vert_mapping = 'POLYINTERP_NEAREST'
dt.layers_vgroup_select_src = 'ALL'; dt.layers_vgroup_select_dst = 'NAME'
for g in src.vertex_groups: suit.vertex_groups.new(name=g.name)
bpy.ops.object.modifier_apply(modifier="dt")

# --- zones from dominant bone ---------------------------------------------
names = [g.name for g in suit.vertex_groups]
def zone(bone):
    if bone in ("head", "neck_02"): return "hood"
    if bone.startswith(("hand", "index", "middle", "ring", "pinky", "thumb")): return "glove"
    if bone.startswith(("foot", "ball")): return "boot"
    if bone.startswith("calf"): return "gaiter"
    return "suit"

OFF = {"suit": 0.055, "gaiter": 0.035, "boot": 0.025, "glove": 0.014, "hood": 0.022}
MAT = {"suit": mat("M_DownSuit_Red", (0.70, 0.04, 0.03), rough=0.55),
       "gaiter": mat("M_DownSuit_Black", (0.03, 0.03, 0.035), rough=0.7),
       "glove": mat("M_DownSuit_Black", (0.03, 0.03, 0.035), rough=0.7),
       "boot": mat("M_DownSuit_Boot", (0.90, 0.72, 0.05), rough=0.5),
       "hood": mat("M_DownSuit_Hood", (0.10, 0.10, 0.12), rough=0.8)}
order = ["suit", "gaiter", "boot", "glove", "hood"]
for z in order:
    if MAT[z].name not in [m.name for m in suit.data.materials]:
        suit.data.materials.append(MAT[z])
mat_index = {z: [m.name for m in suit.data.materials].index(MAT[z].name) for z in order}

me = suit.data
vz = []
for v in me.vertices:
    best = max(v.groups, key=lambda g: g.weight, default=None)
    vz.append(zone(names[best.group]) if best else "suit")

# smooth the per-vertex offset a little across zone borders, add down baffles on the suit
off = [OFF[z] for z in vz]
nbr = [[] for _ in me.vertices]
for e in me.edges:
    a, b = e.vertices; nbr[a].append(b); nbr[b].append(a)
for _ in range(6):
    off = [(off[i] + sum(off[j] for j in nbr[i])) / (1 + len(nbr[i])) if nbr[i] else off[i] for i in range(len(off))]
for i, v in enumerate(me.vertices):
    v.co += v.normal * off[i]
for p in me.polygons:
    zs = [vz[i] for i in p.vertices]
    p.material_index = mat_index[max(set(zs), key=zs.count)]
    p.use_smooth = True

# heavy smoothing hides the mannequin's muscles -> puffy down shape
sm = suit.modifiers.new("cs", 'SMOOTH'); sm.factor = 0.6; sm.iterations = 14
bpy.ops.object.modifier_apply(modifier="cs")
me.update()
for i, v in enumerate(me.vertices):  # 13 cm horizontal down baffles, added after smoothing
    if vz[i] in ("suit", "gaiter"):
        v.co += v.normal * 0.013 * (0.5 + 0.5 * math.cos(v.co.z * 2 * math.pi / 0.13)) ** 2

# re-sample weights after inflating (inflated verts moved away from the body)
for g in list(suit.vertex_groups): suit.vertex_groups.remove(g)
for g in src.vertex_groups: suit.vertex_groups.new(name=g.name)
dt = suit.modifiers.new("dt2", 'DATA_TRANSFER')
dt.object = src; dt.use_vert_data = True; dt.data_types_verts = {'VGROUP_WEIGHTS'}
dt.vert_mapping = 'POLYINTERP_NEAREST'
dt.layers_vgroup_select_src = 'ALL'; dt.layers_vgroup_select_dst = 'NAME'
bpy.ops.object.modifier_apply(modifier="dt2")
bpy.ops.object.vertex_group_limit_total(group_select_mode='ALL', limit=4)
bpy.ops.object.vertex_group_normalize_all(lock_active=False)

suit.parent = arm
am = suit.modifiers.new("Armature", 'ARMATURE'); am.object = arm
bpy.data.objects.remove(src)

tris = sum(len(p.vertices) - 2 for p in me.polygons)
print(f"SUIT tris={tris} dims={tuple(round(x,2) for x in suit.dimensions)}")

os.makedirs(os.path.join(OUT, "Export", "Skel"), exist_ok=True)
bpy.ops.object.select_all(action='DESELECT')
arm.select_set(True); suit.select_set(True)
bpy.ops.export_scene.fbx(filepath=os.path.join(OUT, "Export", "Skel", "SKM_DownSuit_Temp.fbx"),
                         use_selection=True, object_types={'ARMATURE', 'MESH'}, add_leaf_bones=False,
                         apply_scale_options='FBX_SCALE_UNITS', bake_anim=False, mesh_smooth_type='FACE')
print("EXPORTED suit")
preview("suit_temp", [suit])
save_blend("suit_temp")
