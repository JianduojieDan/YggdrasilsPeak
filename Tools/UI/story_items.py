# Blender (headless): the two story items for the pack (Phase1_Design_Plan: the rune-covered compass from the parents'
# belongings; the mother's journal whose pages are the clues). Saves Saved/ItemIcons/src/Compass.blend and
# Journal.blend, one joined object each, for render_item_icons.py.
#   blender -b -P Tools/UI/story_items.py
import bpy, bmesh, math, os
from mathutils import Vector

SRC = "F:/UE_project/YggdrasilsPeak/Saved/ItemIcons/src"


def reset():
    bpy.ops.wm.read_factory_settings(use_empty=True)


def mat(name, rgb, rough=0.5, metal=0.0, emit=None):
    m = bpy.data.materials.new(name); m.use_nodes = True
    b = m.node_tree.nodes["Principled BSDF"]
    b.inputs["Base Color"].default_value = (*rgb, 1); b.inputs["Roughness"].default_value = rough; b.inputs["Metallic"].default_value = metal
    if emit:
        b.inputs["Emission Color"].default_value = (*emit, 1); b.inputs["Emission Strength"].default_value = 2.0
    return m


def cyl(r, h, loc, m, rot=(0, 0, 0), seg=48):
    bpy.ops.mesh.primitive_cylinder_add(radius=r, depth=h, location=loc, rotation=rot, vertices=seg)
    o = bpy.context.active_object; o.data.materials.append(m); return o


def box(size, loc, m, bevel=0.0, rot=(0, 0, 0)):
    bpy.ops.mesh.primitive_cube_add(size=1, location=loc, rotation=rot)
    o = bpy.context.active_object; o.scale = size; bpy.ops.object.transform_apply(scale=True)
    if bevel:
        bv = o.modifiers.new("b", "BEVEL"); bv.width = bevel; bv.segments = 3
    o.data.materials.append(m); return o


def stroke(pts, r, m):
    """A rune stroke: polyline in the XY plane as a thin round tube."""
    cu = bpy.data.curves.new("rune", "CURVE"); cu.dimensions = "3D"; cu.bevel_depth = r; cu.bevel_resolution = 2
    sp = cu.splines.new("POLY"); sp.points.add(len(pts) - 1)
    for p, q in zip(sp.points, pts):
        p.co = (*q, 1)
    o = bpy.data.objects.new("rune", cu); bpy.context.scene.collection.objects.link(o); o.data.materials.append(m)
    return o


def join(name, objs):
    for o in objs:
        o.select_set(True)
        bpy.context.view_layer.objects.active = o
        if o.type == "CURVE":
            bpy.ops.object.convert(target="MESH")
    bpy.ops.object.select_all(action="DESELECT")
    for o in bpy.context.scene.objects:
        if o.type == "MESH":
            o.select_set(True); bpy.context.view_layer.objects.active = o
    for o in bpy.context.scene.objects:
        for md in list(getattr(o, "modifiers", [])):
            bpy.context.view_layer.objects.active = o
            bpy.ops.object.modifier_apply(modifier=md.name)
    bpy.ops.object.join()
    o = bpy.context.active_object; o.name = name
    return o


# ---------------------------------------------------------------- rune compass
reset()
BRASS = mat("brass", (0.62, 0.45, 0.2), 0.35, 1.0)
DARKB = mat("darkbrass", (0.25, 0.17, 0.08), 0.5, 1.0)
DIAL = mat("dial", (0.86, 0.8, 0.66), 0.7)
GOLD = mat("rune", (0.95, 0.7, 0.25), 0.3, 1.0, emit=(1.0, 0.62, 0.18))
RED = mat("needle", (0.6, 0.07, 0.05), 0.4)
STEEL = mat("steel", (0.2, 0.22, 0.25), 0.35, 1.0)
objs = [cyl(0.045, 0.014, (0, 0, 0.007), BRASS), cyl(0.04, 0.002, (0, 0, 0.0145), DIAL),
        cyl(0.006, 0.006, (0, 0.049, 0.01), BRASS, rot=(0, math.radians(90), 0))]
# lid standing open behind, its inside engraved
lid = cyl(0.045, 0.006, (0, 0.052, 0.045), DARKB, rot=(math.radians(100), 0, 0)); objs.append(lid)
# a ring of Elder Futhark runes round the dial (strokes in a 1 x 1 cell, scaled to ~5 mm)
RUNES = {"F": [[(0, 0), (0, 1)], [(0, 0.75), (0.5, 1)], [(0, 0.45), (0.5, 0.7)]],
         "U": [[(0, 0), (0, 1), (0.5, 0.7), (0.5, 0)]],
         "TH": [[(0, 0), (0, 1)], [(0, 0.7), (0.4, 0.5), (0, 0.3)]],
         "A": [[(0, 0), (0, 1)], [(0, 1), (0.45, 0.75)], [(0, 0.7), (0.45, 0.45)]],
         "R": [[(0, 0), (0, 1), (0.4, 0.8), (0, 0.55), (0.4, 0)]],
         "K": [[(0.4, 1), (0, 0.5), (0.4, 0)]],
         "G": [[(0, 0), (0.6, 1)], [(0, 1), (0.6, 0)]],
         "Y": [[(0.3, 0), (0.3, 1)], [(0, 0.7), (0.3, 1), (0.6, 0.7)]]}
keys = list(RUNES) * 2
for i, k in enumerate(keys):
    a = i / len(keys) * 2 * math.pi
    c = Vector((math.cos(a), math.sin(a), 0)) * 0.031
    t = Vector((-math.sin(a), math.cos(a), 0)); n = Vector((math.cos(a), math.sin(a), 0))
    for st in RUNES[k]:
        pts = [c + t * ((x - 0.25) * 0.006) + n * ((y - 0.5) * 0.008) + Vector((0, 0, 0.0158)) for x, y in st]
        objs.append(stroke(pts, 0.0008, GOLD))
# needle pointing up the mountain, a pin
bm = bmesh.new()
for p in ((0, 0.024, 0), (0.0035, 0, 0), (0, -0.024, 0), (-0.0035, 0, 0)):
    bm.verts.new(p)
bm.faces.new(bm.verts); me = bpy.data.meshes.new("needle"); bm.to_mesh(me)
nd = bpy.data.objects.new("needle", me); bpy.context.scene.collection.objects.link(nd); nd.location = (0, 0, 0.0175)
nd.rotation_euler = (0, 0, math.radians(-20)); nd.data.materials.append(RED)
sol = nd.modifiers.new("s", "SOLIDIFY"); sol.thickness = 0.0012; objs.append(nd)
objs.append(cyl(0.0025, 0.004, (0, 0, 0.018), STEEL))
join("Compass", objs)
bpy.ops.wm.save_as_mainfile(filepath=f"{SRC}/Compass.blend")
print("saved compass")

# ---------------------------------------------------------------- mother's journal
reset()
LEATHER = mat("leather", (0.23, 0.12, 0.06), 0.6)
PAGES = mat("pages", (0.86, 0.82, 0.7), 0.9)
BAND = mat("band", (0.06, 0.05, 0.05), 0.6)
PENCIL = mat("pencil", (0.75, 0.55, 0.15), 0.6)
PHOTO = mat("photo", (0.78, 0.74, 0.64), 0.8)
o = [box((0.15, 0.21, 0.004), (0, 0, 0.0), LEATHER, 0.002), box((0.15, 0.21, 0.004), (0, 0, 0.026), LEATHER, 0.002),
     box((0.145, 0.204, 0.022), (0.002, 0, 0.013), PAGES, 0.001), box((0.006, 0.212, 0.03), (-0.075, 0, 0.013), LEATHER, 0.003),
     box((0.012, 0.215, 0.032), (0.05, 0, 0.013), BAND, 0.0),
     box((0.07, 0.05, 0.001), (0.02, 0.11, 0.015), PHOTO, 0.0, rot=(0, 0, math.radians(12))),      # an old photo sticking out
     cyl(0.0035, 0.17, (0.083, 0, 0.012), PENCIL, rot=(math.radians(90), 0, 0), seg=6)]
# pages bulging (a used notebook)
d = o[2].modifiers.new("bulge", "SIMPLE_DEFORM"); d.deform_method = "BEND"; d.angle = math.radians(4); d.deform_axis = "Y"
join("Journal", o)
bpy.ops.wm.save_as_mainfile(filepath=f"{SRC}/Journal.blend")
print("saved journal")
