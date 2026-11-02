# Blender (headless): renders the pack items' game meshes (Saved/ItemIcons/src/*.glb from export_item_meshes.py)
# in one small studio - warm key, cool fill, rim light, transparent background, 3/4 view - into
# Saved/ItemIcons/render/<name>.png. Tools/UI/ink_icons.py then draws them as field-journal sketches.
#   blender -b -P Tools/UI/render_item_icons.py -- [name ...]
import bpy, sys, os, math
from mathutils import Vector

ROOT = "F:/UE_project/YggdrasilsPeak"
SRC, OUT = f"{ROOT}/Saved/ItemIcons/src", f"{ROOT}/Saved/ItemIcons/render"
os.makedirs(OUT, exist_ok=True)
# name: (rotation of the item in degrees XYZ, camera azimuth, camera elevation)
VIEW = {
    "Ration": ((0, 0, 25), 35, 22), "Water": ((0, 0, 0), 35, 18), "Oxygen": ((0, 0, 0), 30, 14),
    "IceAxe": ((0, 0, 0), 90, 10), "Rope": ((0, 0, 0), 30, 50), "Stove": ((0, 0, 0), 35, 28),
    "Gas": ((0, 0, 0), 35, 22), "Tent": ((0, 0, 0), 35, 25), "Compass": ((0, 0, 0), 20, 55), "Journal": ((0, 0, 0), 15, 60),
    "Backpack": ((0, 0, 0), 0, 8),
}
args = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
names = args or [os.path.splitext(f)[0] for f in os.listdir(SRC) if f.endswith((".glb", ".blend"))]


def light(name, kind, energy, rot, color, size=2.0):
    d = bpy.data.lights.new(name, kind)
    d.energy = energy; d.color = color
    if kind == "AREA":
        d.size = size
    if kind == "SUN":
        d.angle = math.radians(8)
    o = bpy.data.objects.new(name, d)
    o.rotation_euler = [math.radians(a) for a in rot]
    bpy.context.scene.collection.objects.link(o)
    return o


for name in names:
  try:
        bpy.ops.wm.read_factory_settings(use_empty=True)
        sc = bpy.context.scene
        src = f"{SRC}/{name}.glb"
        if os.path.exists(src):
            bpy.ops.import_scene.gltf(filepath=src)
        else:
            bpy.ops.wm.append(directory=f"{SRC}/{name}.blend/Object/", filename=name)
        objs = [o for o in sc.objects if o.type == "MESH"]
        # base colour texture lost by UE's glTF export: put it back (exported by export_item_textures.py, or the
        # Poly Haven source for the ration tins and the thermos)
        tex = {"Ration": f"{ROOT}/Imports/PolyHaven/long_life_food/textures/long_life_food_diff_1k.jpg",
               "Water": f"{ROOT}/Imports/PolyHaven/modified_thermos/textures/modified_thermos_diff_1k.jpg"}.get(name, f"{SRC}/{name}_0.png")
        if os.path.exists(tex):
            img = bpy.data.images.load(tex)
            for o in objs:
                for slot in o.material_slots:
                    m = slot.material
                    if not m or not m.use_nodes:
                        continue
                    bsdf = next((n for n in m.node_tree.nodes if n.type == "BSDF_PRINCIPLED"), None)
                    if bsdf:
                        tn = m.node_tree.nodes.new("ShaderNodeTexImage"); tn.image = img
                        m.node_tree.links.new(tn.outputs["Color"], bsdf.inputs["Base Color"])
        rot, az, el = VIEW.get(name, ((0, 0, 0), 35, 22))
        # parent everything to an empty to rotate the item as a whole
        root = bpy.data.objects.new("root", None); sc.collection.objects.link(root)
        for o in objs:
            if o.parent is None:
                o.parent = root
        root.rotation_euler = [math.radians(a) for a in rot]
        bpy.context.view_layer.update()
        pts = [o.matrix_world @ Vector(c) for o in objs for c in o.bound_box]
        lo = Vector((min(p.x for p in pts), min(p.y for p in pts), min(p.z for p in pts)))
        hi = Vector((max(p.x for p in pts), max(p.y for p in pts), max(p.z for p in pts)))
        centre, radius = (lo + hi) / 2, (hi - lo).length / 2
        # camera: orthographic 3/4 view framing the bounding sphere
        cam_d = bpy.data.cameras.new("cam"); cam_d.type = "ORTHO"; cam_d.ortho_scale = radius * 2.15
        cam = bpy.data.objects.new("cam", cam_d); sc.collection.objects.link(cam); sc.camera = cam
        a, e = math.radians(az), math.radians(el)
        cam.location = centre + Vector((math.sin(a) * math.cos(e), -math.cos(a) * math.cos(e), math.sin(e))) * radius * 6
        cam.rotation_euler = (centre - cam.location).to_track_quat("-Z", "Y").to_euler()
        cam_d.clip_end = radius * 20
        # studio: warm key from upper left, cool fill, white rim from behind
        s = radius / 0.3
        light("key", "SUN", 4.0, (50, 0, -40), (1.0, 0.93, 0.82))
        light("fill", "SUN", 1.2, (70, 0, 140), (0.75, 0.85, 1.0))
        light("rim", "SUN", 3.0, (-60, 0, 20), (1.0, 1.0, 1.0))
        sc.world = bpy.data.worlds.new("w"); sc.world.color = (0.25, 0.26, 0.28)
        sc.render.engine = "BLENDER_EEVEE"
        sc.render.film_transparent = True
        sc.render.resolution_x = sc.render.resolution_y = 768
        sc.render.image_settings.file_format = "PNG"; sc.render.image_settings.color_mode = "RGBA"
        sc.view_settings.view_transform = "Standard"
        sc.render.filepath = f"{OUT}/{name}.png"
        bpy.ops.render.render(write_still=True)
        print("rendered", name, "objects", len(objs), "radius", round(radius, 3))
  except Exception as ex:
    print('FAILED', name, ex)
