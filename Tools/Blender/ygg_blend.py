# Shared helpers for the prop scripts. Run props with:
#   blender.exe --background --factory-startup --python Tools/Blender/<prop>.py
# Units are metres in Blender; the FBX exporter writes cm-compatible scale for UE.
import bpy, bmesh, math, os
from mathutils import Vector, Matrix

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
OUT = os.path.join(ROOT, "Art", "Blender")


def reset():
    bpy.ops.wm.read_factory_settings(use_empty=True)
    s = bpy.context.scene
    s.unit_settings.system = 'METRIC'
    s.unit_settings.scale_length = 1.0


def mat(name, rgb, metal=0.0, rough=0.5):
    m = bpy.data.materials.get(name) or bpy.data.materials.new(name)
    m.use_nodes = True
    b = m.node_tree.nodes["Principled BSDF"]
    b.inputs["Base Color"].default_value = (*rgb, 1)
    b.inputs["Metallic"].default_value = metal
    b.inputs["Roughness"].default_value = rough
    m.diffuse_color = (*rgb, 1)
    return m


def _obj(name, mesh, material):
    o = bpy.data.objects.new(name, mesh)
    bpy.context.collection.objects.link(o)
    if material:
        mesh.materials.append(material)
    return o


def box(name, size, loc=(0, 0, 0), rot=(0, 0, 0), material=None):
    me = bpy.data.meshes.new(name)
    bm = bmesh.new()
    bmesh.ops.create_cube(bm, size=1.0)
    bmesh.ops.scale(bm, vec=Vector(size), verts=bm.verts)
    bm.to_mesh(me); bm.free()
    o = _obj(name, me, material)
    o.location = loc; o.rotation_euler = rot
    return o


def cyl(name, r, depth, loc=(0, 0, 0), rot=(0, 0, 0), seg=12, material=None, r2=None):
    me = bpy.data.meshes.new(name)
    bm = bmesh.new()
    bmesh.ops.create_cone(bm, cap_ends=True, segments=seg, radius1=r,
                          radius2=r if r2 is None else r2, depth=depth)
    bm.to_mesh(me); bm.free()
    for p in me.polygons:
        p.use_smooth = True
    o = _obj(name, me, material)
    o.location = loc; o.rotation_euler = rot
    return o


def sphere(name, r, loc=(0, 0, 0), seg=16, rings=8, scale=(1, 1, 1), material=None):
    me = bpy.data.meshes.new(name)
    bm = bmesh.new()
    bmesh.ops.create_uvsphere(bm, u_segments=seg, v_segments=rings, radius=r)
    bm.to_mesh(me); bm.free()
    for p in me.polygons:
        p.use_smooth = True
    o = _obj(name, me, material)
    o.location = loc; o.scale = scale
    return o


def tube_along(name, pts, r, seg=6, material=None):
    """Rope/hose: curve with bevel, converted to mesh."""
    cu = bpy.data.curves.new(name, 'CURVE')
    cu.dimensions = '3D'
    sp = cu.splines.new('POLY')
    sp.points.add(len(pts) - 1)
    for i, p in enumerate(pts):
        sp.points[i].co = (*p, 1)
    cu.bevel_depth = r
    cu.bevel_resolution = max(0, seg // 2 - 2)
    o = bpy.data.objects.new(name, cu)
    bpy.context.collection.objects.link(o)
    bpy.context.view_layer.objects.active = o
    o.select_set(True)
    bpy.ops.object.convert(target='MESH')
    o = bpy.context.active_object
    if material:
        o.data.materials.append(material)
    o.select_set(False)
    return o


def join(name, objs):
    bpy.ops.object.select_all(action='DESELECT')
    for o in objs:
        o.select_set(True)
    bpy.context.view_layer.objects.active = objs[0]
    bpy.ops.object.join()
    o = bpy.context.active_object
    o.name = name
    o.data.name = name
    bpy.ops.object.transform_apply(location=False, rotation=True, scale=True)
    return o


def uv_and_finish(o):
    bpy.ops.object.select_all(action='DESELECT')
    o.select_set(True)
    bpy.context.view_layer.objects.active = o
    bpy.ops.object.mode_set(mode='EDIT')
    bpy.ops.mesh.select_all(action='SELECT')
    bpy.ops.uv.smart_project(angle_limit=math.radians(66), island_margin=0.02)
    bpy.ops.object.mode_set(mode='OBJECT')


def export(o, fname):
    """Export one object as SM_<fname>.fbx (origin = object origin) + save .blend."""
    os.makedirs(os.path.join(OUT, "Export"), exist_ok=True)
    uv_and_finish(o)
    bpy.ops.object.select_all(action='DESELECT')
    o.select_set(True)
    path = os.path.join(OUT, "Export", f"SM_{fname}.fbx")
    bpy.ops.export_scene.fbx(filepath=path, use_selection=True, apply_scale_options='FBX_SCALE_UNITS',
                             mesh_smooth_type='FACE', add_leaf_bones=False, bake_anim=False)
    tris = sum(len(p.vertices) - 2 for p in o.data.polygons)
    d = o.dimensions
    print(f"EXPORTED {path} tris={tris} dims=({d.x:.2f},{d.y:.2f},{d.z:.2f})m")
    return path


def export_skeletal(arm, mesh, path):
    """Skeletal FBX for UE with the bones in CENTIMETRES and root scale 1, like UE's own Manny export.
    A metre armature exported with a unit scale ends up as root-bone scale 100 in UE; animations reset the
    root scale to 1 and the whole body collapses to 1/100. Works on copies so the .blend stays in metres."""
    s = bpy.context.scene
    a2 = arm.copy(); a2.data = arm.data.copy(); s.collection.objects.link(a2)
    m2 = mesh.copy(); m2.data = mesh.data.copy(); s.collection.objects.link(m2)
    m2.parent = None; m2.matrix_world = mesh.matrix_world.copy()
    for o in (a2, m2):
        o.matrix_world = Matrix.Scale(100, 4) @ o.matrix_world
        bpy.ops.object.select_all(action='DESELECT'); o.select_set(True); bpy.context.view_layer.objects.active = o
        bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
    m2.parent = a2
    for md in m2.modifiers:
        if md.type == 'ARMATURE': md.object = a2
    name = arm.name; arm.name = name + "_orig"; a2.name = name   # FBX root bone must be called "root"
    old = s.unit_settings.scale_length; s.unit_settings.scale_length = 0.01
    bpy.ops.object.select_all(action='DESELECT'); a2.select_set(True); m2.select_set(True)
    bpy.context.view_layer.objects.active = a2
    bpy.ops.export_scene.fbx(filepath=path, use_selection=True, object_types={'ARMATURE', 'MESH'},
                             add_leaf_bones=False, bake_anim=False, apply_unit_scale=True,
                             apply_scale_options='FBX_SCALE_NONE', mesh_smooth_type='FACE', path_mode='STRIP')
    s.unit_settings.scale_length = old
    print("SKEL EXPORT pelvis z(cm)", round(a2.data.bones["pelvis"].head_local.z, 1), "mesh h(cm)", round(m2.dimensions.z, 1))
    bpy.data.objects.remove(m2); bpy.data.objects.remove(a2); arm.name = name


def save_blend(fname):
    os.makedirs(OUT, exist_ok=True)
    bpy.ops.wm.save_as_mainfile(filepath=os.path.join(OUT, f"{fname}.blend"))


def preview(fname, objs, res=640):
    """Quick Workbench render framing all objs, for a sanity check."""
    s = bpy.context.scene
    s.render.engine = 'BLENDER_WORKBENCH'
    s.display.shading.color_type = 'MATERIAL'
    s.display.shading.light = 'STUDIO'
    s.display.shading.show_cavity = True
    s.render.resolution_x = res; s.render.resolution_y = int(res * 0.75)
    lo = Vector((1e9,) * 3); hi = Vector((-1e9,) * 3)
    for o in objs:
        for c in o.bound_box:
            w = o.matrix_world @ Vector(c)
            lo = Vector(map(min, lo, w)); hi = Vector(map(max, hi, w))
    ctr = (lo + hi) / 2; rad = (hi - lo).length / 2
    cam = bpy.data.objects.new("Cam", bpy.data.cameras.new("Cam"))
    s.collection.objects.link(cam)
    cam.location = ctr + Vector((1.0, -1.3, 0.6)).normalized() * rad * 3.4
    cam.rotation_euler = (ctr - cam.location).to_track_quat('-Z', 'Y').to_euler()
    s.camera = cam
    s.render.filepath = os.path.join(OUT, "Preview", f"{fname}.png")
    bpy.ops.render.render(write_still=True)
    bpy.data.objects.remove(cam)
