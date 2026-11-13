# Runs INSIDE the editor (Tools/RoutePlanning/uepy). Imports the fixed-rope parts made by Tools/Blender/rope_anchors
# (Art/Blender/Export/Rope/*.fbx) into /Game/Environment/FixedRope and builds their materials:
#   M_FixedRope (braid, Tools/Materials/M_FixedRope.hlsl) -> MI_FixedRope_New (yellow, black tracer),
#   MI_FixedRope_Old (faded red, sun-bleached - ropes of past seasons); M_RopeAnchor (constant metal/fabric) -> MIs.
import glob, os
import unreal

MEL = unreal.MaterialEditingLibrary
DIR = "/Game/Environment/FixedRope"
SRC = "F:/UE_project/YggdrasilsPeak/Art/Blender/Export/Rope"
HLSL = "F:/UE_project/YggdrasilsPeak/Tools/Materials/M_FixedRope.hlsl"
tools = unreal.AssetToolsHelpers.get_asset_tools()

# ---- meshes (REIMPORT = True to bring the FBX in again; rebuild the materials with the start-menu map open - deleting
# the expressions of a material the open level draws crashed the editor, 2026-11-13)
REIMPORT = globals().pop('REIMPORT', False)
tasks = []
for f in (sorted(glob.glob(SRC + "/*.fbx")) if REIMPORT else []):
    t = unreal.AssetImportTask()
    t.filename = f.replace("\\", "/"); t.destination_path = DIR
    t.replace_existing = True; t.automated = True; t.save = True
    ui = unreal.FbxImportUI()
    ui.set_editor_property("import_materials", False); ui.set_editor_property("import_textures", False)
    ui.set_editor_property("import_as_skeletal", False)
    ui.static_mesh_import_data.set_editor_property("combine_meshes", True)
    ui.static_mesh_import_data.set_editor_property("auto_generate_collision", False)
    t.options = ui
    tasks.append(t)
tools.import_asset_tasks(tasks)
for t in tasks:
    print("IMPORTED", os.path.basename(t.filename), list(t.imported_object_paths))


def fresh(name):
    path = f"{DIR}/{name}"
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        m = unreal.load_asset(path); MEL.delete_all_material_expressions(m); return m
    return tools.create_asset(name, DIR, unreal.Material, unreal.MaterialFactoryNew())


def param(m, cls, name, value, x, y):
    e = MEL.create_material_expression(m, cls, x, y)
    e.set_editor_property("parameter_name", name)
    e.set_editor_property("default_value", value)
    return e


def mi(name, parent, scalars=None, vectors=None):
    path = f"{DIR}/{name}"
    inst = unreal.load_asset(path) if unreal.EditorAssetLibrary.does_asset_exist(path) else \
        tools.create_asset(name, DIR, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    MEL.set_material_instance_parent(inst, parent)
    for k, v in (scalars or {}).items(): MEL.set_material_instance_scalar_parameter_value(inst, k, v)
    for k, v in (vectors or {}).items(): MEL.set_material_instance_vector_parameter_value(inst, k, unreal.LinearColor(*v))
    unreal.EditorAssetLibrary.save_loaded_asset(inst)
    return inst


# ---- rope material: two Custom nodes on the same code (Mode 0 = colour + roughness, Mode 1 = normal)
m = fresh("M_FixedRope")
m.set_editor_property("used_with_instanced_static_meshes", True)
code = open(HLSL, encoding="utf-8").read()
uv = MEL.create_material_expression(m, unreal.MaterialExpressionTextureCoordinate, -1000, 0)
ins = {
    "UV": (uv, ""),
    "Base": (param(m, unreal.MaterialExpressionVectorParameter, "Base", unreal.LinearColor(0.80, 0.52, 0.03, 1), -1000, 100), ""),
    "Tracer": (param(m, unreal.MaterialExpressionVectorParameter, "Tracer", unreal.LinearColor(0.02, 0.025, 0.04, 1), -1000, 300), ""),
    "Age": (param(m, unreal.MaterialExpressionScalarParameter, "Age", 0.15, -1000, 500), ""),
    "Rnd": (MEL.create_material_expression(m, unreal.MaterialExpressionPerInstanceRandom, -1000, 600), ""),
    "NWS": (MEL.create_material_expression(m, unreal.MaterialExpressionVertexNormalWS, -1000, 700), ""),
    "WP": (MEL.create_material_expression(m, unreal.MaterialExpressionWorldPosition, -1000, 800), ""),
}
nodes = []
for mode, otype, y in ((0, unreal.CustomMaterialOutputType.CMOT_FLOAT4, 100), (1, unreal.CustomMaterialOutputType.CMOT_FLOAT4, 500)):
    c = MEL.create_material_expression(m, unreal.MaterialExpressionCustom, -500, y)
    c.set_editor_property("code", code); c.set_editor_property("output_type", otype); c.set_editor_property("description", "FixedRope mode %d" % mode)
    cins = []
    for n in list(ins.keys()) + ["Mode"]:
        ci = unreal.CustomInput(); ci.set_editor_property("input_name", n); cins.append(ci)
    c.set_editor_property("inputs", cins)
    for k, (e, out) in ins.items():
        MEL.connect_material_expressions(e, out, c, k)
    mk = MEL.create_material_expression(m, unreal.MaterialExpressionConstant, -700, y + 200)
    mk.set_editor_property("r", float(mode))
    MEL.connect_material_expressions(mk, "", c, "Mode")
    nodes.append(c)
rgb = MEL.create_material_expression(m, unreal.MaterialExpressionComponentMask, -250, 50)
rgb.set_editor_property("r", True); rgb.set_editor_property("g", True); rgb.set_editor_property("b", True); rgb.set_editor_property("a", False)
MEL.connect_material_expressions(nodes[0], "", rgb, "")
ra = MEL.create_material_expression(m, unreal.MaterialExpressionComponentMask, -250, 200)
for k, v in zip("rgba", (False, False, False, True)): ra.set_editor_property(k, v)
MEL.connect_material_expressions(nodes[0], "", ra, "")
MEL.connect_material_property(rgb, "", unreal.MaterialProperty.MP_BASE_COLOR)
MEL.connect_material_property(ra, "", unreal.MaterialProperty.MP_ROUGHNESS)
nrm = MEL.create_material_expression(m, unreal.MaterialExpressionComponentMask, -250, 500)
for k, v in zip("rgba", (True, True, True, False)): nrm.set_editor_property(k, v)
MEL.connect_material_expressions(nodes[1], "", nrm, "")
MEL.connect_material_property(nrm, "", unreal.MaterialProperty.MP_NORMAL)
spec = MEL.create_material_expression(m, unreal.MaterialExpressionConstant, -250, 350); spec.set_editor_property("r", 0.35)
MEL.connect_material_property(spec, "", unreal.MaterialProperty.MP_SPECULAR)
MEL.recompile_material(m); unreal.EditorAssetLibrary.save_loaded_asset(m)
mi("MI_FixedRope_New", m, {"Age": 0.12}, {"Base": (0.80, 0.52, 0.03, 1), "Tracer": (0.02, 0.025, 0.04, 1)})
mi("MI_FixedRope_Old", m, {"Age": 0.85}, {"Base": (0.42, 0.08, 0.05, 1), "Tracer": (0.05, 0.08, 0.20, 1)})

# ---- anchors: plain PBR constants
a = fresh("M_RopeAnchor")
a.set_editor_property("used_with_instanced_static_meshes", True)
col = param(a, unreal.MaterialExpressionVectorParameter, "Color", unreal.LinearColor(0.6, 0.6, 0.62, 1), -500, 0)
met = param(a, unreal.MaterialExpressionScalarParameter, "Metallic", 1.0, -500, 200)
rou = param(a, unreal.MaterialExpressionScalarParameter, "Roughness", 0.4, -500, 300)
MEL.connect_material_property(col, "", unreal.MaterialProperty.MP_BASE_COLOR)
MEL.connect_material_property(met, "", unreal.MaterialProperty.MP_METALLIC)
MEL.connect_material_property(rou, "", unreal.MaterialProperty.MP_ROUGHNESS)
MEL.recompile_material(a); unreal.EditorAssetLibrary.save_loaded_asset(a)
mats = {
    "M_RopeAnchor_Alu": mi("MI_RopeAnchor_Alu", a, {"Metallic": 1.0, "Roughness": 0.38}, {"Color": (0.66, 0.68, 0.70, 1)}),
    "M_RopeAnchor_Steel": mi("MI_RopeAnchor_Steel", a, {"Metallic": 1.0, "Roughness": 0.5}, {"Color": (0.42, 0.43, 0.45, 1)}),
    "M_RopeAnchor_Carabiner": mi("MI_RopeAnchor_Carabiner", a, {"Metallic": 0.85, "Roughness": 0.32}, {"Color": (0.75, 0.22, 0.03, 1)}),
    "M_RopeAnchor_Sling": mi("MI_RopeAnchor_Sling", a, {"Metallic": 0.0, "Roughness": 0.85}, {"Color": (0.45, 0.04, 0.03, 1)}),
}

# ---- assign by slot name
for name in ("SM_RopeAnchor_Picket", "SM_RopeAnchor_Screw", "SM_RopeAnchor_Piton", "SM_FixedRope_Piece"):
    sm = unreal.load_asset(f"{DIR}/{name}")
    if not sm: print("MISSING", name); continue
    slots = list(sm.get_editor_property("static_materials"))
    for i, s in enumerate(slots):
        slot = str(s.get_editor_property("material_slot_name"))
        target = mats.get(slot) or (unreal.load_asset(f"{DIR}/MI_FixedRope_New") if "Rope" in slot else None)
        if target:
            sm.set_material(i, target)
    b = sm.get_bounding_box()
    print(name, "slots", [str(s.get_editor_property("material_slot_name")) for s in slots], "bounds cm min", [round(v, 1) for v in (b.min.x, b.min.y, b.min.z)], "max", [round(v, 1) for v in (b.max.x, b.max.y, b.max.z)])
    unreal.EditorAssetLibrary.save_loaded_asset(sm)
