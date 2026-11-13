# Fixed-rope parts (2026-11-13, user: the old orange cylinders on tall posts floated and "were not good looking"; real
# Everest fixed ropes lie along the snow, tied to short anchors). Real sizes:
#   SM_FixedRope_Piece   open tube, 100 cm long x 100 cm across, along +Z, centred (scaled per instance like the engine
#                        cylinder); 10 sides; UV u = around (0..1), v = along (0..1) for the braid material.
#   SM_RopeAnchor_Picket aluminium T-section snow stake, 75 cm, origin at the snow line (50 cm under it), a red sling
#                        through its top hole and an orange screwgate carabiner hanging where the rope runs.
#   SM_RopeAnchor_Screw  ice screw (tube in the ice along -Z), hanger + crank above the surface, carabiner.
#   SM_RopeAnchor_Piton  steel blade peg driven into rock along -Z, eye + carabiner.
# Run: blender.exe --background --factory-startup --python Tools/Blender/rope_anchors.py
import sys, os, math
sys.path.append(os.path.dirname(__file__))
from ygg_blend import *
import bpy, bmesh
from mathutils import Vector, Matrix

def mats():
    """Materials die with every reset() (factory settings), so they are made again after each one."""
    global ALU, STEEL, KARA, SLING, ROPEM
    ALU = mat("M_RopeAnchor_Alu", (0.66, 0.68, 0.70), metal=1.0, rough=0.38)
    STEEL = mat("M_RopeAnchor_Steel", (0.45, 0.46, 0.48), metal=1.0, rough=0.45)
    KARA = mat("M_RopeAnchor_Carabiner", (0.85, 0.30, 0.05), metal=0.8, rough=0.35)
    SLING = mat("M_RopeAnchor_Sling", (0.62, 0.06, 0.05), rough=0.85)
    ROPEM = mat("M_FixedRope", (0.85, 0.66, 0.08), rough=0.75)


def export_raw(o, fname):
    """Like ygg_blend.export but keeps the hand-made UVs."""
    os.makedirs(os.path.join(OUT, "Export"), exist_ok=True)
    bpy.ops.object.select_all(action='DESELECT')
    o.select_set(True)
    bpy.context.view_layer.objects.active = o
    path = os.path.join(OUT, "Export", "Rope", f"SM_{fname}.fbx")
    os.makedirs(os.path.dirname(path), exist_ok=True)
    bpy.ops.export_scene.fbx(filepath=path, use_selection=True, apply_scale_options='FBX_SCALE_UNITS',
                             mesh_smooth_type='FACE', add_leaf_bones=False, bake_anim=False)
    tris = sum(len(p.vertices) - 2 for p in o.data.polygons)
    d = o.dimensions
    print(f"EXPORTED {path} tris={tris} dims=({d.x:.3f},{d.y:.3f},{d.z:.3f})m")


def smart_uv(objs):
    for o in objs:
        bpy.ops.object.select_all(action='DESELECT')
        o.select_set(True); bpy.context.view_layer.objects.active = o
        bpy.ops.object.mode_set(mode='EDIT'); bpy.ops.mesh.select_all(action='SELECT')
        bpy.ops.uv.smart_project(angle_limit=math.radians(66), island_margin=0.02)
        bpy.ops.object.mode_set(mode='OBJECT')


# ------------------------------------------------------------------ rope piece
reset(); mats()
SIDES = 10
me = bpy.data.meshes.new("FixedRope_Piece")
bm = bmesh.new()
uvl = bm.loops.layers.uv.new("UVMap")
rings = []
for zi, z in enumerate((-0.5, 0.5)):
    rings.append([bm.verts.new((0.5 * math.cos(2 * math.pi * i / SIDES), 0.5 * math.sin(2 * math.pi * i / SIDES), z)) for i in range(SIDES)])
for i in range(SIDES):
    j = (i + 1) % SIDES
    f = bm.faces.new((rings[0][i], rings[0][j], rings[1][j], rings[1][i]))
    for loop in f.loops:
        v = loop.vert
        k = rings[0].index(v) if v in rings[0] else rings[1].index(v)
        u = k / SIDES
        if k == 0 and (i == SIDES - 1):
            u = 1.0
        loop[uvl].uv = (u, 0.0 if v in rings[0] else 1.0)
bm.normal_update()
bm.to_mesh(me); bm.free()
for p in me.polygons: p.use_smooth = True
rope = bpy.data.objects.new("FixedRope_Piece", me); bpy.context.collection.objects.link(rope)
me.materials.append(ROPEM)
export_raw(rope, "FixedRope_Piece")


# ------------------------------------------------------------------ shared: carabiner + sling
def carabiner(top, name, lie=0.0):
    """Screwgate D carabiner 10.5 x 6 cm hanging from `top` (centre of its top bend), in the XZ plane, rod 1 cm:
    a round top bend (r 3 cm), a straight spine (+X side), a tighter bottom bend and the gate (-X side) leaning in a
    little - the D shape. lie = degrees it is swung about Y towards +X (90 = lying flat along +X).
    Returns (objects, where a rope through it rests = inside of the bottom bend)."""
    W, H = 0.060, 0.105
    rt, rb = W * 0.5, W * 0.36
    ct, cb = (0.0, -rt), (W * 0.5 - rb, -H + rb)
    loop = [(ct[0] + rt * math.cos(a), 0, ct[1] + rt * math.sin(a)) for a in [math.pi * k / 12 for k in range(13)]]   # top bend, +X to -X
    loop += [(-W * 0.5 + 0.004 * t, 0, ct[1] - (ct[1] - (cb[1] - rb * 0.2)) * t) for t in (0.33, 0.66, 1.0)]               # gate side down
    loop += [(cb[0] + rb * math.cos(a), 0, cb[1] + rb * math.sin(a)) for a in [math.pi + math.pi * 0.5 * k / 6 for k in range(1, 13)]]   # bottom bend
    loop += [(W * 0.5, 0, ct[1])]                                                                               # spine up
    loop.append(loop[0])
    R = Matrix.Rotation(math.radians(lie), 4, 'Y')
    def place(p):
        v = R @ Vector(p)
        return (v.x + top[0], v.y + top[1], v.z + top[2])
    k = tube_along(name, [place(p) for p in loop], 0.005, seg=8, material=KARA)
    g = place((-W * 0.5 + 0.002, 0, ct[1] - 0.012))
    sleeve = cyl(name + "_Gate", 0.0072, 0.022, loc=g, rot=(0, math.radians(-lie), 0), seg=10, material=KARA)
    rest = place((cb[0], 0, cb[1] - rb + 0.006))
    return [k, sleeve], rest


def sling(a, b, name, r=0.006):
    """Flat webbing loop between points a and b (a thin flattened tube)."""
    mid = Vector(a).lerp(Vector(b), 0.5)
    side = Vector((0.012, 0, 0))
    pts = [Vector(a) + side, mid + side * 1.6, Vector(b) + side * 0.8, Vector(b) - side * 0.8, mid - side * 1.6, Vector(a) - side, Vector(a) + side]
    t = tube_along(name, [tuple(p) for p in pts], r, seg=6, material=SLING)
    t.scale = (1.0, 0.45, 1.0)
    return t


# ------------------------------------------------------------------ snow picket
reset(); mats()
L, BELOW = 0.75, 0.50
web = box("Picket_Web", (0.004, 0.030, L), loc=(0, 0.015, L / 2 - BELOW), material=ALU)
flange = box("Picket_Flange", (0.036, 0.004, L), loc=(0, 0, L / 2 - BELOW), material=ALU)
tip = cyl("Picket_Tip", 0.02, 0.06, loc=(0, 0.008, -BELOW - 0.02), seg=4, r2=0.0005, material=ALU)
tip.rotation_euler = (math.pi, 0, math.radians(45))
top = L - BELOW
s = sling((0, -0.006, top - 0.035), (0, -0.05, top - 0.16), "Picket_Sling")
kb, rest = carabiner((0, -0.05, top - 0.15), "Picket_Carabiner")
print("ATTACH Picket", tuple(round(v * 100, 2) for v in rest))
parts = [web, flange, tip, s] + kb
smart_uv(parts)
picket = join("RopeAnchor_Picket", parts)
export_raw(picket, "RopeAnchor_Picket")
preview("RopeAnchor_Picket", [picket])

# ------------------------------------------------------------------ ice screw
reset(); mats()
tube = cyl("Screw_Tube", 0.0085, 0.16, loc=(0, 0, -0.08), seg=12, material=STEEL)
thread = tube_along("Screw_Thread", [(0.0098 * math.cos(t * 0.7), 0.0098 * math.sin(t * 0.7), -0.01 - t * 0.0045) for t in range(32)], 0.0015, seg=4, material=STEEL)
hanger = box("Screw_Hanger", (0.045, 0.004, 0.028), loc=(0.012, 0, 0.004), material=STEEL)
crank = cyl("Screw_Crank", 0.003, 0.03, loc=(-0.012, 0, 0.012), seg=6, material=STEEL)
knob = cyl("Screw_Knob", 0.006, 0.012, loc=(-0.012, 0, 0.03), seg=8, material=KARA)
kb, rest = carabiner((0.030, 0, 0.014), "Screw_Carabiner", lie=-86)
print("ATTACH Screw", tuple(round(v * 100, 2) for v in rest))
parts = [tube, thread, hanger, crank, knob] + kb
smart_uv(parts)
screw = join("RopeAnchor_Screw", parts)
export_raw(screw, "RopeAnchor_Screw")
preview("RopeAnchor_Screw", [screw])

# ------------------------------------------------------------------ rock piton
reset(); mats()
blade = box("Piton_Blade", (0.004, 0.018, 0.11), loc=(0, 0, -0.05), material=STEEL)
blade_tip = cyl("Piton_Tip", 0.009, 0.02, loc=(0, 0, -0.115), seg=4, r2=0.0005, material=STEEL)
blade_tip.scale = (0.3, 1.0, 1.0)
eye = tube_along("Piton_Eye", [(0.018 * math.cos(a), 0, 0.012 + 0.018 * math.sin(a)) for a in [2 * math.pi * k / 16 for k in range(17)]], 0.0035, seg=6, material=STEEL)
kb, rest = carabiner((0.018, 0, 0.016), "Piton_Carabiner", lie=-86)
print("ATTACH Piton", tuple(round(v * 100, 2) for v in rest))
parts = [blade, blade_tip, eye] + kb
smart_uv(parts)
piton = join("RopeAnchor_Piton", parts)
export_raw(piton, "RopeAnchor_Piton")
preview("RopeAnchor_Piton", [piton])
