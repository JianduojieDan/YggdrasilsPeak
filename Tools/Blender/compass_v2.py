# Blender (headless): Father's rune compass, version 2 - an antique brass pocket compass.
#   blender -b -P Tools/Blender/compass_v2.py
# Rounded case with a knurled bezel, recessed enamel dial (T_CompassDial), glass, a two-tone needle on a jewelled
# cap, winding crown and bow ring at 3 o'clock, domed lid on a hinge barrel at 12 o'clock standing open behind,
# its inside engraved with the World Tree (T_CompassLidIn). Metres; +Y = 12 o'clock (north, the lid side).
# Exports (origin on the needle pivot) Art/Blender/Export/Compass/SM_Compass_Case.fbx and SM_Compass_Needle.fbx;
# saves Saved/ItemIcons/src/Compass.blend (one object "Compass") for the pack icon.
import bpy, bmesh, math, os
from mathutils import Vector, Matrix

ROOT = "F:/UE_project/YggdrasilsPeak"
OUT = f"{ROOT}/Art/Blender/Export/Compass"
os.makedirs(OUT, exist_ok=True)
TEX = f"{ROOT}/Art/UI/Compass"
PIVOT = 0.0158
bpy.ops.wm.read_factory_settings(use_empty=True)
sc = bpy.context.scene


def mat(name, rgb, rough, metal, image=None, alpha=1.0):
    m = bpy.data.materials.new(name); m.use_nodes = True
    b = m.node_tree.nodes["Principled BSDF"]
    b.inputs["Base Color"].default_value = (*rgb, 1); b.inputs["Roughness"].default_value = rough; b.inputs["Metallic"].default_value = metal
    if image:
        t = m.node_tree.nodes.new("ShaderNodeTexImage"); t.image = bpy.data.images.load(image)
        m.node_tree.links.new(t.outputs["Color"], b.inputs["Base Color"])
    if alpha < 1:
        b.inputs["Alpha"].default_value = alpha
        if hasattr(m, "surface_render_method"):
            m.surface_render_method = "BLENDED"        # Blender 4.2+/5 EEVEE: alpha needs blended rendering
        elif hasattr(m, "blend_method"):
            m.blend_method = "BLEND"
    return m


BRASS = mat("brass", (0.66, 0.47, 0.20), 0.32, 1.0)
DIAL = mat("dial", (0.9, 0.85, 0.72), 0.6, 0.0, f"{TEX}/T_CompassDial.png")
LIDIN = mat("lidin", (0.4, 0.28, 0.12), 0.4, 1.0, f"{TEX}/T_CompassLidIn.png")
GLASS = mat("glass", (0.9, 0.95, 1.0), 0.02, 0.0, alpha=0.12)
NEEDLE_N = mat("needle_n", (0.62, 0.07, 0.05), 0.35, 0.3)
NEEDLE_S = mat("needle_s", (0.10, 0.13, 0.20), 0.25, 1.0)      # blued steel
JEWEL = mat("jewel", (0.6, 0.05, 0.08), 0.05, 0.0)


def obj(name, me, m):
    o = bpy.data.objects.new(name, me); sc.collection.objects.link(o); o.data.materials.append(m); return o


def cyl(name, r, h, z, m, seg=96, bevel=0.0):
    bpy.ops.mesh.primitive_cylinder_add(radius=r, depth=h, location=(0, 0, z), vertices=seg)
    o = bpy.context.active_object; o.name = name; o.data.materials.append(m)
    if bevel:
        bv = o.modifiers.new("b", "BEVEL"); bv.width = bevel; bv.segments = 4; bv.limit_method = "ANGLE"
    return o


def planar_uv(o, radius):
    """UVs straight down from above: the disc of this radius fills 0..1 (texture top = +Y)."""
    me = o.data
    uv = me.uv_layers.new(name="UV") if not me.uv_layers else me.uv_layers[0]
    for poly in me.polygons:
        for li in poly.loop_indices:
            v = me.vertices[me.loops[li].vertex_index].co
            uv.data[li].uv = (v.x / (2 * radius) + 0.5, v.y / (2 * radius) + 0.5)


parts_case, parts_needle = [], []
# case body: rounded drum
parts_case.append(cyl("body", 0.045, 0.012, 0.006, BRASS, bevel=0.0035))    # top at 12 mm, under the dial
# bezel ring on top (v3, 2026-11-12: the old 1 mm zig-zag knurl read as a jagged saw edge): a rounded coin-like
# profile swept round, its outer side finely reeded (0.1 mm), smooth shaded - a clean round silhouette
bm = bmesh.new()
K = 720
PROFILE = [(0.0405, 0.0110), (0.0405, 0.0188), (0.0409, 0.0197), (0.0417, 0.0203), (0.0430, 0.0206), (0.0443, 0.0205),
           (0.0453, 0.0201), (0.0460, 0.0194), (0.0463, 0.0184), (0.0464, 0.0172), (0.0464, 0.0151), (0.0464, 0.0130),
           (0.0462, 0.0117), (0.0456, 0.0110)]
grid = []
for i in range(K):
    t = 2 * math.pi * i / K
    ring = []
    for (r, z) in PROFILE:
        if 0.0125 <= z <= 0.0176 and r > 0.0462:
            r += 0.00010 * math.cos(240 * t)            # fine reeding on the outer side only
        ring.append(bm.verts.new((r * math.cos(t), r * math.sin(t), z)))
    grid.append(ring)
n = len(PROFILE)
for i in range(K):
    a, b = grid[i], grid[(i + 1) % K]
    for k in range(n):
        k2 = (k + 1) % n                                  # the profile closes on itself (bottom face)
        bm.faces.new((a[k], a[k2], b[k2], b[k]))
bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
me = bpy.data.meshes.new("bezel"); bm.to_mesh(me); bm.free()
parts_case.append(obj("bezel", me, BRASS))
# dial (recessed), glass over it
dial = cyl("dial", 0.0405, 0.001, 0.0133, DIAL, seg=128); planar_uv(dial, 0.0405); parts_case.append(dial)
bpy.ops.mesh.primitive_uv_sphere_add(radius=0.0405, location=(0, 0, 0.0170), segments=64, ring_count=16)
gl = bpy.context.active_object; gl.name = "glass"; gl.scale = (1, 1, 0.08); gl.data.materials.append(GLASS)
bpy.ops.object.transform_apply(scale=True)
parts_case.append(gl)
# winding crown and bow ring at 3 o'clock
bpy.ops.mesh.primitive_cylinder_add(radius=0.0042, depth=0.006, location=(0.0485, 0, 0.008), rotation=(0, math.radians(90), 0), vertices=32)
cr = bpy.context.active_object; cr.data.materials.append(BRASS); parts_case.append(cr)
bpy.ops.mesh.primitive_cylinder_add(radius=0.0028, depth=0.004, location=(0.0535, 0, 0.008), rotation=(0, math.radians(90), 0), vertices=24)
cr2 = bpy.context.active_object; cr2.data.materials.append(BRASS); parts_case.append(cr2)
bpy.ops.mesh.primitive_torus_add(major_radius=0.0075, minor_radius=0.0013, location=(0.0625, 0, 0.008), rotation=(math.radians(90), 0, 0))
bow = bpy.context.active_object; bow.data.materials.append(BRASS); parts_case.append(bow)
# hinge barrel at 12 o'clock and the lid standing open behind it
HY, HZ = 0.0475, 0.0185
bpy.ops.mesh.primitive_cylinder_add(radius=0.0028, depth=0.018, location=(0, HY, HZ), rotation=(0, math.radians(90), 0), vertices=24)
hb = bpy.context.active_object; hb.data.materials.append(BRASS); parts_case.append(hb)
lid_parts = []
lid = cyl("lid", 0.0462, 0.005, 0.0, BRASS, bevel=0.0025)
for v in lid.data.vertices:                               # domed outside
    if v.co.z > 0:
        v.co.z += 0.003 * (1 - (v.co.x ** 2 + v.co.y ** 2) / 0.0462 ** 2)
lid_parts.append(lid)
lin = cyl("lidin", 0.0425, 0.0006, -0.0028, LIDIN, seg=128); planar_uv(lin, 0.0425); lid_parts.append(lin)
# closed position: lid over the case (inside face down), then swung open about the hinge by 100 degrees
for p in lid_parts:
    p.location = (0, 0, 0.0235 + p.location.z)
    bpy.context.view_layer.update()
    p.data.transform(p.matrix_world); p.matrix_world = Matrix.Identity(4)
    p.data.transform(Matrix.Translation((0, HY, HZ)) @ Matrix.Rotation(math.radians(-100), 4, "X") @ Matrix.Translation((0, -HY, -HZ)))
parts_case += lid_parts
# needle: north half red, south half blued steel, on a brass cap with a red jewel
def half(name, length, m, sign):
    bm = bmesh.new()
    w = 0.0024
    vs = [bm.verts.new(v) for v in ((0, 0, PIVOT), (w, sign * 0.006, PIVOT), (0, sign * length, PIVOT), (-w, sign * 0.006, PIVOT))]
    bm.faces.new(vs if sign > 0 else list(reversed(vs)))
    me = bpy.data.meshes.new(name); bm.to_mesh(me); bm.free()
    o = obj(name, me, m)
    s = o.modifiers.new("s", "SOLIDIFY"); s.thickness = 0.0009
    return o
parts_needle.append(half("needle_n", 0.031, NEEDLE_N, 1))
parts_needle.append(half("needle_s", 0.027, NEEDLE_S, -1))
cap = cyl("cap", 0.0032, 0.0022, PIVOT + 0.001, BRASS, seg=32, bevel=0.0006); parts_needle.append(cap)
bpy.ops.mesh.primitive_uv_sphere_add(radius=0.0013, location=(0, 0, PIVOT + 0.0023), segments=16, ring_count=8)
jw = bpy.context.active_object; jw.data.materials.append(JEWEL); parts_needle.append(jw)


def finish(objs, name):
    bpy.ops.object.select_all(action="DESELECT")
    for o in objs:
        o.select_set(True)
    bpy.context.view_layer.objects.active = objs[0]
    for o in objs:
        bpy.context.view_layer.objects.active = o
        for md in list(o.modifiers):
            bpy.ops.object.modifier_apply(modifier=md.name)
    bpy.context.view_layer.objects.active = objs[0]
    bpy.ops.object.join()
    j = bpy.context.active_object; j.name = name
    # cube UVs only where a part has none (brass); dial and lid keep their planar UVs
    return j


case = finish(parts_case, "SM_Compass_Case")
needle = finish(parts_needle, "SM_Compass_Needle")
for o in (case, needle):
    o.data.shade_smooth() if hasattr(o.data, "shade_smooth") else None
    bpy.context.scene.cursor.location = (0, 0, PIVOT)
    bpy.ops.object.select_all(action="DESELECT"); o.select_set(True); bpy.context.view_layer.objects.active = o
    bpy.ops.object.origin_set(type="ORIGIN_CURSOR")
    loc = o.location.copy(); o.location = (0, 0, 0)
    bpy.ops.export_scene.fbx(filepath=f"{OUT}/{o.name}.fbx", use_selection=True, apply_unit_scale=True, mesh_smooth_type="FACE", add_leaf_bones=False)
    o.location = loc
    print("exported", o.name, len(o.data.polygons), [s.material.name for s in o.material_slots])
# the pack icon model: everything as one object, without the glass (in a studio render it only mirrors the grey)
gi = [i for i, sl in enumerate(case.material_slots) if sl.material.name.startswith("glass")]
if gi:
    bm = bmesh.new(); bm.from_mesh(case.data)
    bmesh.ops.delete(bm, geom=[f for f in bm.faces if f.material_index in gi], context="FACES")
    bm.to_mesh(case.data); bm.free()
bpy.ops.object.select_all(action="DESELECT"); case.select_set(True); needle.select_set(True); bpy.context.view_layer.objects.active = case
bpy.ops.object.join(); bpy.context.active_object.name = "Compass"
bpy.ops.wm.save_as_mainfile(filepath=f"{ROOT}/Saved/ItemIcons/src/Compass.blend")
print("saved icon model")
