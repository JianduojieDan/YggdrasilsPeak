# Blender (headless): the expedition pack for the backpack screen, rendered as the frames of its opening - the
# zipped front panel folds down and the main compartment opens. The game's own ice axe and rope (exported glbs)
# hang on its sides, a foam mat is strapped underneath, the lid sits on top. Output: Saved/ItemIcons/render/
# Pack_f0..f5.png (1200 px, transparent) + Pack_layout.json (where the opening is, as 0..1 of the image) for the UI.
# Tools/UI/ink_icons.py --pack draws them in the same field-sketch style as the items.
#   blender -b -P Tools/UI/backpack_art.py
import bpy, bmesh, math, json, os
from mathutils import Vector
from bpy_extras.object_utils import world_to_camera_view

ROOT = "F:/UE_project/YggdrasilsPeak"
SRC, OUT = f"{ROOT}/Saved/ItemIcons/src", f"{ROOT}/Saved/ItemIcons/render"
os.makedirs(OUT, exist_ok=True)
FLAP_ANGLES = [0, 28, 60, 95, 130, 168]

bpy.ops.wm.read_factory_settings(use_empty=True)
sc = bpy.context.scene


def mat(name, rgb, rough=0.8, bump=0.0, metal=0.0):
    m = bpy.data.materials.new(name); m.use_nodes = True
    b = m.node_tree.nodes["Principled BSDF"]
    b.inputs["Base Color"].default_value = (*rgb, 1); b.inputs["Roughness"].default_value = rough
    b.inputs["Metallic"].default_value = metal
    if bump > 0:   # woven nylon / cordura texture
        nt = m.node_tree
        tex = nt.nodes.new("ShaderNodeTexNoise"); tex.inputs["Scale"].default_value = 180; tex.inputs["Detail"].default_value = 4
        bn = nt.nodes.new("ShaderNodeBump"); bn.inputs["Strength"].default_value = bump
        nt.links.new(tex.outputs["Fac"], bn.inputs["Height"]); nt.links.new(bn.outputs["Normal"], b.inputs["Normal"])
    return m


FABRIC = mat("fabric", (0.055, 0.085, 0.16), 0.85, 0.25)        # deep indigo cordura
ACCENT = mat("accent", (0.78, 0.30, 0.06), 0.7, 0.2)            # orange webbing, like the climber's jacket
INSIDE = mat("inside", (0.012, 0.016, 0.026), 0.95)             # the dark of the main compartment
ZIP = mat("zip", (0.03, 0.03, 0.035), 0.4, 0, 0.6)
MAT = mat("mat", (0.55, 0.12, 0.08), 0.9, 0.15)                 # dusty red foam mat
BUCKLE = mat("buckle", (0.02, 0.02, 0.02), 0.5)


def rounded_box(name, size, loc, material, bevel=0.03, sub=1):
    bpy.ops.mesh.primitive_cube_add(size=1, location=loc)
    o = bpy.context.active_object; o.name = name
    o.scale = size
    bpy.ops.object.transform_apply(scale=True)
    bv = o.modifiers.new("bevel", "BEVEL"); bv.width = bevel; bv.segments = 4
    if sub:
        sd = o.modifiers.new("sub", "SUBSURF"); sd.levels = sub; sd.render_levels = sub
    o.data.materials.append(material)
    return o


def strap(name, size, loc, rot=(0, 0, 0)):
    o = rounded_box(name, size, loc, ACCENT, bevel=0.004, sub=0)
    o.rotation_euler = [math.radians(a) for a in rot]
    return o


# body with the main compartment cut into its front
W, D, H = 0.40, 0.25, 0.66
body = rounded_box("body", (W, D, H), (0, 0, H / 2 + 0.06), FABRIC, bevel=0.06)
# the main compartment: a recessed dark opening (the items are drawn into it by the UI)
rounded_box("opening", (0.29, 0.02, 0.44), (0, -D / 2 - 0.004, 0.36), INSIDE, bevel=0.02, sub=0)


def worn(o, strength=0.01, taper=0.0):
    """Packed fabric: lumpy wrinkles, a little wider at the bottom."""
    t = bpy.data.textures.new(o.name + "_w", "CLOUDS"); t.noise_scale = 0.06
    dm = o.modifiers.new("wrinkle", "DISPLACE"); dm.texture = t; dm.strength = strength; dm.mid_level = 0.5
    if taper:
        sd = o.modifiers.new("taper", "SIMPLE_DEFORM"); sd.deform_method = "TAPER"; sd.factor = taper; sd.deform_axis = "Z"


worn(body, 0.012, -0.12)
# zip running round the opening
for (sx, sz, x, z) in ((0.30, 0.012, 0, 0.585), (0.30, 0.012, 0, 0.135), (0.012, 0.45, -0.15, 0.36), (0.012, 0.45, 0.15, 0.36)):
    rounded_box("zip", (sx, 0.012, sz), (x, -D / 2 - 0.002, z), ZIP, bevel=0.003, sub=0)

# front panel (folds down on a hinge along the bottom of the opening)
hinge = bpy.data.objects.new("hinge", None); sc.collection.objects.link(hinge)
hinge.location = (0, -D / 2 - 0.01, 0.13)
flap = rounded_box("flap", (0.31, 0.03, 0.46), (0, -D / 2 - 0.01, 0.13 + 0.23), FABRIC, bevel=0.025)
worn(flap, 0.006)
flap.parent = hinge; flap.matrix_parent_inverse = hinge.matrix_world.inverted()
pocket = rounded_box("pocket", (0.22, 0.03, 0.17), (0, -D / 2 - 0.035, 0.27), FABRIC, bevel=0.02)
pocket.parent = hinge; pocket.matrix_parent_inverse = hinge.matrix_world.inverted()
for x in (-0.07, 0.07):     # two orange straps over the front panel
    s_ = strap("flapstrap", (0.03, 0.012, 0.44), (x, -D / 2 - 0.05, 0.36))
    s_.parent = hinge; s_.matrix_parent_inverse = hinge.matrix_world.inverted()
tab = rounded_box("pull", (0.02, 0.01, 0.05), (0.15, -D / 2 - 0.03, 0.56), ACCENT, bevel=0.004, sub=0)
tab.parent = hinge; tab.matrix_parent_inverse = hinge.matrix_world.inverted()

# lid on top, side compression straps, mat under the bottom
worn(rounded_box("lid", (W + 0.03, D + 0.04, 0.11), (0, -0.01, H + 0.09), FABRIC, bevel=0.045), 0.01)
for x in (-0.09, 0.09):
    strap("lidstrap", (0.028, 0.012, 0.12), (x, -D / 2 - 0.015, H + 0.02))
    rounded_box("buckle", (0.04, 0.016, 0.03), (x, -D / 2 - 0.02, H - 0.03), BUCKLE, bevel=0.005, sub=0)
for z in (0.22, 0.48):
    strap("side", (W + 0.02, D * 0.6, 0.026), (0, 0.03, z))
bpy.ops.mesh.primitive_cylinder_add(radius=0.075, depth=0.52, location=(0, -0.01, 0.02), rotation=(0, math.radians(90), 0))
mt = bpy.context.active_object; mt.data.materials.append(MAT)
for x in (-0.15, 0.15):
    bpy.ops.mesh.primitive_torus_add(major_radius=0.08, minor_radius=0.008, location=(x, -0.01, 0.02), rotation=(0, math.radians(90), 0))
    bpy.context.active_object.data.materials.append(ACCENT)


def attach(glb, length, loc, rot):
    before = set(sc.objects)
    bpy.ops.import_scene.gltf(filepath=glb)
    new = [o for o in sc.objects if o not in before and o.type == "MESH"]
    tex = f"{SRC}/{os.path.splitext(os.path.basename(glb))[0]}_0.png"
    if os.path.exists(tex):
        img = bpy.data.images.load(tex)
        for o in new:
            for slot in o.material_slots:
                m = slot.material
                if m and m.use_nodes:
                    b = next((n for n in m.node_tree.nodes if n.type == "BSDF_PRINCIPLED"), None)
                    if b:
                        t = m.node_tree.nodes.new("ShaderNodeTexImage"); t.image = img
                        m.node_tree.links.new(t.outputs["Color"], b.inputs["Base Color"])
    bpy.context.view_layer.update()
    pts = [o.matrix_world @ Vector(c) for o in new for c in o.bound_box]
    ext = [max(p[i] for p in pts) - min(p[i] for p in pts) for i in range(3)]
    centre = Vector([(max(p[i] for p in pts) + min(p[i] for p in pts)) / 2 for i in range(3)])
    longest = ext.index(max(ext))
    # inner pivot: centres the item and turns its longest axis to Z; outer: placement and extra turn
    inner = bpy.data.objects.new("att_in", None); sc.collection.objects.link(inner)
    for o in new:
        if o.parent is None:
            o.parent = inner
            o.location -= centre
    inner.rotation_euler = {0: (0, math.radians(90), 0), 1: (math.radians(90), 0, 0), 2: (0, 0, 0)}[longest]
    root = bpy.data.objects.new("att", None); sc.collection.objects.link(root)
    inner.parent = root
    root.scale = [length / max(ext)] * 3
    root.location = loc
    root.rotation_euler = [math.radians(a) for a in rot]


attach(f"{SRC}/IceAxe.glb", 0.62, (-W / 2 - 0.03, -0.02, 0.38), (0, 0, 90))      # strapped along the left side
attach(f"{SRC}/Rope.glb", 0.34, (W / 2 + 0.04, -0.01, 0.40), (0, 0, 70))         # coil on the right side

# camera, light
cam_d = bpy.data.cameras.new("cam"); cam_d.type = "ORTHO"; cam_d.ortho_scale = 1.02
cam = bpy.data.objects.new("cam", cam_d); sc.collection.objects.link(cam); sc.camera = cam
cam.location = (0, -4, 0.42 + 4 * math.tan(math.radians(8))); cam.rotation_euler = (math.radians(82), 0, 0)
for n, e, r, c in (("key", 4.0, (50, 0, -35), (1, 0.93, 0.82)), ("fill", 1.3, (70, 0, 150), (0.75, 0.85, 1)), ("rim", 2.5, (-60, 0, 15), (1, 1, 1))):
    d = bpy.data.lights.new(n, "SUN"); d.energy = e; d.color = c; d.angle = math.radians(8)
    o = bpy.data.objects.new(n, d); o.rotation_euler = [math.radians(a) for a in r]; sc.collection.objects.link(o)
sc.world = bpy.data.worlds.new("w"); sc.world.color = (0.25, 0.26, 0.28)
sc.render.engine = "BLENDER_EEVEE"; sc.render.film_transparent = True
sc.render.resolution_x = sc.render.resolution_y = 1200
sc.render.image_settings.file_format = "PNG"; sc.render.image_settings.color_mode = "RGBA"
sc.view_settings.view_transform = "Standard"

for i, a in enumerate(FLAP_ANGLES):
    hinge.rotation_euler = (math.radians(-a), 0, 0)     # the top of the panel comes towards the camera, then down
    sc.render.filepath = f"{OUT}/Pack_f{i}.png"
    bpy.ops.render.render(write_still=True)
    print("rendered frame", i, a)

# where the opening is on the image (0..1, y down) - the UI drops the items in there
corners = [Vector((x, -D / 2, z)) for x in (-0.145, 0.145) for z in (0.14, 0.58)]
uv = [world_to_camera_view(sc, cam, c) for c in corners]
lay = {"opening": [min(p.x for p in uv), 1 - max(p.y for p in uv), max(p.x for p in uv), 1 - min(p.y for p in uv)]}
json.dump(lay, open(f"{OUT}/Pack_layout.json", "w"))
print("layout", lay)
