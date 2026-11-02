# Blender (headless): the frozen mythical limb for the Lhotse Face (Phase1_Design_Plan S5: "limbs of frozen mythical
# creatures deep in the ice ... the myth is real"). A huge scaled forelimb - upper arm, elbow, forearm - ending in a
# four-fingered claw with long hooked talons, reaching up through the ice as if it froze mid-climb. Built from a
# skin-modifier skeleton (organic joints), subdivided, with overlapping scales from Voronoi displacement; talons are
# curved tapered horns. ~13 m long. Exports Art/Blender/Export/Limb/SM_FrozenLimb.fbx (plain metres) with two
# material slots: "hide" (scaled skin) and "talon" (keratin).   blender -b -P Tools/Blender/frozen_limb.py
import bpy, bmesh, math, os
from mathutils import Vector

OUT = "F:/UE_project/YggdrasilsPeak/Art/Blender/Export/Limb"
os.makedirs(OUT, exist_ok=True)
bpy.ops.wm.read_factory_settings(use_empty=True)
sc = bpy.context.scene


def mat(name, rgb, rough):
    m = bpy.data.materials.new(name); m.use_nodes = True
    b = m.node_tree.nodes["Principled BSDF"]
    b.inputs["Base Color"].default_value = (*rgb, 1); b.inputs["Roughness"].default_value = rough
    return m


HIDE = mat("hide", (0.10, 0.11, 0.10), 0.6)
TALON = mat("talon", (0.06, 0.05, 0.045), 0.35)

# skeleton: (position, radius); edges between consecutive points; fingers branch from the wrist
arm = [((0, 0, -2.0), 1.35), ((0, 0.3, 0.2), 1.45), ((0, 0.6, 2.4), 1.15), ((0, 1.0, 4.4), 1.3),       # bulging upper arm, elbow knot
       ((0, 0.7, 5.6), 1.0), ((0, 0.0, 6.9), 1.15), ((0, -0.6, 8.3), 0.8), ((0, -0.9, 9.3), 0.62),     # forearm muscle, narrow wrist
       ((0, -1.1, 10.1), 0.95)]                                                                           # broad palm
fingers = []
for k, (spread, lift, length) in enumerate(((-0.95, 0.0, 1.0), (-0.32, 0.15, 1.25), (0.32, 0.12, 1.2), (0.95, -0.1, 0.9))):
    dirv = Vector((math.sin(spread), -0.35 + lift, math.cos(spread))).normalized()
    base = Vector(arm[-1][0])
    # three segments with knuckles, curling forward (-Y) as a grasping claw would
    pts = [(tuple(base + dirv * 0.7), 0.5), (tuple(base + dirv * (0.7 + 0.9 * length)), 0.36),
           (tuple(base + dirv * (0.7 + 1.0 * length)), 0.42),
           (tuple(base + dirv * (0.7 + 1.8 * length) + Vector((0, -0.4, 0))), 0.32),
           (tuple(base + dirv * (0.7 + 1.9 * length) + Vector((0, -0.45, 0))), 0.36),
           (tuple(base + dirv * (0.7 + 2.5 * length) + Vector((0, -0.95, 0))), 0.3)]
    fingers.append((pts, (dirv + Vector((0, -0.6, 0))).normalized()))

bm = bmesh.new()
rad = {}
def chain(points, start=None):
    prev = start
    for p, r in points:
        v = bm.verts.new(p); rad[v] = r
        if prev is not None:
            bm.edges.new((prev, v))
        prev = v
    return prev
wrist = chain(arm)
tips = [chain(pts, wrist) for pts, _ in fingers]
me = bpy.data.meshes.new("limb"); bm.to_mesh(me)
radii = [rad[v] for v in bm.verts]
bm.free()
o = bpy.data.objects.new("SM_FrozenLimb", me); sc.collection.objects.link(o)
bpy.context.view_layer.objects.active = o; o.select_set(True)
sk = o.modifiers.new("skin", "SKIN")
for i, r in enumerate(radii):
    o.data.skin_vertices[0].data[i].radius = (r, r)
sub = o.modifiers.new("sub", "SUBSURF"); sub.levels = 3; sub.render_levels = 3
bpy.ops.object.modifier_apply(modifier="skin"); bpy.ops.object.modifier_apply(modifier="sub")
# scales: overlapping plates from Voronoi distance, larger on the back of the arm, small on the fingers
tex = bpy.data.textures.new("scales", "VORONOI"); tex.noise_scale = 0.16; tex.distance_metric = "DISTANCE"
d = o.modifiers.new("scales", "DISPLACE"); d.texture = tex; d.strength = -0.16; d.mid_level = 0.0
bpy.ops.object.modifier_apply(modifier="scales")
tex2 = bpy.data.textures.new("ridges", "CLOUDS"); tex2.noise_scale = 0.8
d2 = o.modifiers.new("ridges", "DISPLACE"); d2.texture = tex2; d2.strength = 0.18
bpy.ops.object.modifier_apply(modifier="ridges")
o.data.materials.append(HIDE)

# talons: long curved horns from each finger tip, curling forward and down
talon_objs = []
for (pts, dirv), k in zip(fingers, range(4)):
    tip = Vector(pts[-1][0])
    bm = bmesh.new()
    rings, N = 14, 12
    prev = None
    for i in range(rings + 1):
        t = i / rings
        r = 0.34 * (1 - t) ** 1.2 + 0.006
        bend = Vector((0, -2.2 * t * t, 0))                    # a strong hook forward
        centre = tip + dirv * (2.2 * t) + bend
        ring = [bm.verts.new(centre + Vector((math.cos(a) * r, math.sin(a) * r * 0.8, 0))) for a in [2 * math.pi * j / N for j in range(N)]]
        if prev:
            for j in range(N):
                bm.faces.new((prev[j], prev[(j + 1) % N], ring[(j + 1) % N], ring[j]))
        prev = ring
    tm = bpy.data.meshes.new(f"talon{k}"); bm.to_mesh(tm); bm.free()
    to = bpy.data.objects.new(f"talon{k}", tm); sc.collection.objects.link(to); to.data.materials.append(TALON)
    talon_objs.append(to)
bpy.ops.object.select_all(action="DESELECT")
for t in talon_objs:
    t.select_set(True)
o.select_set(True); bpy.context.view_layer.objects.active = o
bpy.ops.object.join()
dec = o.modifiers.new("dec", "DECIMATE"); dec.ratio = min(1.0, 60000 / max(len(o.data.polygons), 1))
bpy.ops.object.modifier_apply(modifier="dec")
bpy.ops.object.shade_smooth()
bpy.ops.object.mode_set(mode="EDIT"); bpy.ops.mesh.select_all(action="SELECT"); bpy.ops.uv.smart_project(angle_limit=1.15); bpy.ops.object.mode_set(mode="OBJECT")
bpy.ops.export_scene.fbx(filepath=f"{OUT}/SM_FrozenLimb.fbx", use_selection=True, apply_unit_scale=True, mesh_smooth_type="FACE", add_leaf_bones=False)
bpy.ops.wm.save_as_mainfile(filepath=f"{OUT}/FrozenLimb.blend")
print("exported limb", len(o.data.polygons))
