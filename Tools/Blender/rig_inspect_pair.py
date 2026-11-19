# Inspect two glbs (rigged reference + static target) for rig fitting.
import bpy, sys
from mathutils import Vector
args = sys.argv[sys.argv.index("--")+1:]
for f in args:
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.ops.import_scene.gltf(filepath=f)
    print("=====", f)
    for o in bpy.data.objects:
        print("OBJ", o.name, o.type, "parent", o.parent.name if o.parent else None, "loc", tuple(round(v,3) for v in o.location), "rot", tuple(round(v,3) for v in o.rotation_euler), "scale", tuple(round(v,3) for v in o.scale))
        if o.type == 'MESH':
            ws = [o.matrix_world @ v.co for v in o.data.vertices]
            mn = Vector((min(w.x for w in ws), min(w.y for w in ws), min(w.z for w in ws)))
            mx = Vector((max(w.x for w in ws), max(w.y for w in ws), max(w.z for w in ws)))
            print("  verts", len(ws), "polys", len(o.data.polygons), "bbox", tuple(round(v,3) for v in mn), tuple(round(v,3) for v in mx))
            print("  mats", [m.name for m in o.data.materials], "uvs", [u.name for u in o.data.uv_layers], "vgroups", len(o.vertex_groups), "mods", [m.type for m in o.modifiers])
        if o.type == 'ARMATURE':
            print("  bones", len(o.data.bones))
            for b in o.data.bones:
                h = o.matrix_world @ b.head_local; t = o.matrix_world @ b.tail_local
                print("  B %-16s par %-14s h(%.3f %.3f %.3f) t(%.3f %.3f %.3f)" % (b.name, b.parent.name if b.parent else "-", *h, *t))
