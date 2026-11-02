# Blender (headless): a small ice cave for the Icefall (Phase1_Design_Plan S3 exploration: "an ice cave off-route
# with an old expedition's cache"). A lumpy ice dome (~14 x 10 x 7 m) hollowed out (~3.5 m high inside) with a
# low arched entrance on -X; the floor inside is flat for walking. Voxel-remeshed and displaced like the seracs.
# Plain-metre FBX to Art/Blender/Export/Cave/SM_IceCave.fbx.   blender -b -P Tools/Blender/ice_cave.py
import bpy, bmesh, os, math
from mathutils import Vector, noise
OUT = "F:/UE_project/YggdrasilsPeak/Art/Blender/Export/Cave"
os.makedirs(OUT, exist_ok=True)
bpy.ops.wm.read_factory_settings(use_empty=True)

def blob(name, size, loc, seed, amp):
    bpy.ops.mesh.primitive_ico_sphere_add(subdivisions=5, radius=1, location=loc)
    o = bpy.context.active_object; o.name = name; o.scale = size
    bpy.ops.object.transform_apply(scale=True)
    off = Vector((seed * 3.1, seed * 1.7, seed * 2.3))
    for v in o.data.vertices:
        p = v.co
        v.co = p + p.normalized() * (noise.fractal(p * 0.35 + off, 0.6, 2.0, 4) * amp)
    return o

outer = blob("cave", (7.0, 5.0, 4.2), (0, 0, 0.8), 1, 0.9)
inner = blob("hollow", (5.0, 3.4, 2.6), (0.4, 0, 0.9), 2, 0.35)
# entrance: an arched tunnel out through the -X side
bpy.ops.mesh.primitive_cylinder_add(radius=1.5, depth=6, location=(-6.0, 0, 1.45), rotation=(0, math.radians(90), 0))
door = bpy.context.active_object; door.scale = (1.0, 1.0, 1.0)
# base: everything below z = -1 goes; a flat ice floor fills the hollow up to z = 0 (covers the ground's bumps)
bpy.ops.mesh.primitive_cube_add(size=1, location=(0, 0, -6.0)); floor_cut = bpy.context.active_object; floor_cut.scale = (30, 30, 10.0)
for cutter in (inner, door, floor_cut):
    m = outer.modifiers.new("cut", "BOOLEAN"); m.object = cutter; m.operation = "DIFFERENCE"
    bpy.context.view_layer.objects.active = outer
    bpy.ops.object.modifier_apply(modifier=m.name)
    cutter.hide_set(True)
for c in (inner, door, floor_cut):
    bpy.data.objects.remove(c, do_unlink=True)
bpy.ops.mesh.primitive_cube_add(size=1, location=(-0.5, 0, -0.5)); slab = bpy.context.active_object; slab.scale = (12.0, 7.0, 1.0)
u = outer.modifiers.new("floor", "BOOLEAN"); u.object = slab; u.operation = "UNION"
bpy.context.view_layer.objects.active = outer
bpy.ops.object.modifier_apply(modifier=u.name)
bpy.data.objects.remove(slab, do_unlink=True)
rm = outer.modifiers.new("remesh", "REMESH"); rm.mode = "VOXEL"; rm.voxel_size = 0.12
bpy.context.view_layer.objects.active = outer
bpy.ops.object.modifier_apply(modifier="remesh")
for v in outer.data.vertices:          # fractured ice surface, flat floor kept flat
    if v.co.z > 0.15:
        v.co += v.normal * noise.fractal(v.co * 0.9, 0.55, 2.2, 3) * 0.12
dec = outer.modifiers.new("dec", "DECIMATE"); dec.ratio = min(1.0, 30000 / max(len(outer.data.polygons), 1))
bpy.ops.object.modifier_apply(modifier="dec")
bpy.ops.object.shade_smooth()
bpy.ops.object.mode_set(mode="EDIT"); bpy.ops.mesh.select_all(action="SELECT"); bpy.ops.uv.cube_project(cube_size=6.0); bpy.ops.object.mode_set(mode="OBJECT")
outer.name = "SM_IceCave"
outer.select_set(True)
bpy.ops.export_scene.fbx(filepath=f"{OUT}/SM_IceCave.fbx", use_selection=True, apply_unit_scale=True, mesh_smooth_type="FACE", add_leaf_bones=False)
print("exported cave", len(outer.data.polygons))
