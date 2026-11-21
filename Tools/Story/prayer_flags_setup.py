import unreal
# Runs INSIDE the editor via Tools/RoutePlanning/uepy. 2026-11-21 (#6 the prayer flags hovered and looked fake):
#  - imports the cloth card (Tools/Blender/prayer_flag_cloth -> Art/Flags/SM_PrayerFlagCloth.fbx, pivot = sewn top edge)
#  - imports the five block prints (Meshy text-to-image, cropped: Art/Flags/T_Flag_<colour>.png)
#  - builds M_PrayerFlagCloth: two-sided foliage shading (sunlight glows through the thin cotton), the print as colour,
#    and a wind flutter in World Position Offset: along the cloth's normal, zero at the sewn top edge and growing toward
#    the free bottom edge (v^1.6), a wave running down the flag, phase from world position + per-instance random so no two
#    flags move together; amplitude "Flutter" (cm), speed "FlutterSpeed"
#  - one MI per colour: /Game/Environment/Flags/MI_PrayerFlagCloth_<colour>
ROOT = "F:/UE_project/YggdrasilsPeak"
D = "/Game/Environment/Flags"
EAL = unreal.EditorAssetLibrary; MEL = unreal.MaterialEditingLibrary
at = unreal.AssetToolsHelpers.get_asset_tools()
COLS = ("blue", "white", "red", "green", "yellow")


def imp(src, dest, opts=None):
    t = unreal.AssetImportTask()
    for k, v in (("filename", src), ("destination_path", dest), ("automated", True), ("replace_existing", True), ("save", True)):
        t.set_editor_property(k, v)
    if opts: t.set_editor_property("options", opts)
    at.import_asset_tasks([t])
    p = list(t.get_editor_property("imported_object_paths") or [])
    return unreal.load_asset(str(p[0])) if p else None


mesh = imp(f"{ROOT}/Art/Flags/SM_PrayerFlagCloth.fbx", D)
mesh = unreal.load_asset(f"{D}/SM_PrayerFlagCloth") or mesh
bs = mesh.get_editor_property("body_setup")
if bs: bs.set_editor_property("collision_trace_flag", unreal.CollisionTraceFlag.CTF_USE_SIMPLE_AS_COMPLEX)
EAL.save_loaded_asset(mesh, False)
b = mesh.get_bounding_box(); print("MESH", mesh.get_path_name(), b.min, b.max)
texs = {}
for c in COLS:
    t = imp(f"{ROOT}/Art/Flags/T_Flag_{c}.png", D)
    t.set_editor_property("max_texture_size", 512); EAL.save_loaded_asset(t, False)
    texs[c] = t

mp = f"{D}/M_PrayerFlagCloth"
mat = unreal.load_asset(mp)
if mat:
    MEL.delete_all_material_expressions(mat)      # re-run: rebuild the graph in place (assets are never deleted)
else:
    mat = at.create_asset("M_PrayerFlagCloth", D, unreal.Material, unreal.MaterialFactoryNew())
mat.set_editor_property("two_sided", True)
mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_TWO_SIDED_FOLIAGE)
E = lambda cls, x, y: MEL.create_material_expression(mat, cls, x, y)
C = MEL.connect_material_expressions
tex = E(unreal.MaterialExpressionTextureSampleParameter2D, -900, -200)
tex.set_editor_property("parameter_name", "Flag"); tex.set_editor_property("texture", texs["blue"])
MEL.connect_material_property(tex, "RGB", unreal.MaterialProperty.MP_BASE_COLOR)
sss_k = E(unreal.MaterialExpressionConstant, -900, 40); sss_k.set_editor_property("r", 0.55)
sss = E(unreal.MaterialExpressionMultiply, -600, 0); C(tex, "RGB", sss, "A"); C(sss_k, "", sss, "B")
MEL.connect_material_property(sss, "", unreal.MaterialProperty.MP_SUBSURFACE_COLOR)
rough = E(unreal.MaterialExpressionConstant, -600, 120); rough.set_editor_property("r", 0.88)
MEL.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
# --- flutter
uv = E(unreal.MaterialExpressionTextureCoordinate, -1600, 400)
v = E(unreal.MaterialExpressionComponentMask, -1400, 400)
for k, on in (("r", False), ("g", True), ("b", False), ("a", False)): v.set_editor_property(k, on)
C(uv, "", v, "")
pexp = E(unreal.MaterialExpressionConstant, -1400, 480); pexp.set_editor_property("r", 1.6)
mask = E(unreal.MaterialExpressionPower, -1200, 420); C(v, "", mask, "Base"); C(pexp, "", mask, "Exponent")
wp = E(unreal.MaterialExpressionWorldPosition, -1600, 600)
k3 = E(unreal.MaterialExpressionConstant3Vector, -1600, 680); k3.set_editor_property("constant", unreal.LinearColor(0.013, 0.017, 0.0, 0.0))
dot = E(unreal.MaterialExpressionDotProduct, -1400, 620); C(wp, "", dot, "A"); C(k3, "", dot, "B")
pir = E(unreal.MaterialExpressionPerInstanceRandom, -1600, 760)
k6 = E(unreal.MaterialExpressionConstant, -1600, 820); k6.set_editor_property("r", 6.283)
ph = E(unreal.MaterialExpressionMultiply, -1400, 760); C(pir, "", ph, "A"); C(k6, "", ph, "B")
tm = E(unreal.MaterialExpressionTime, -1600, 900)
spd = E(unreal.MaterialExpressionScalarParameter, -1600, 960); spd.set_editor_property("parameter_name", "FlutterSpeed"); spd.set_editor_property("default_value", 2.4)
tsp = E(unreal.MaterialExpressionMultiply, -1400, 920); C(tm, "", tsp, "A"); C(spd, "", tsp, "B")
vw = E(unreal.MaterialExpressionMultiply, -1400, 1020); kv = E(unreal.MaterialExpressionConstant, -1600, 1060); kv.set_editor_property("r", 2.2)
C(v, "", vw, "A"); C(kv, "", vw, "B")
a1 = E(unreal.MaterialExpressionAdd, -1200, 700); C(dot, "", a1, "A"); C(ph, "", a1, "B")
a2 = E(unreal.MaterialExpressionAdd, -1050, 820); C(a1, "", a2, "A"); C(tsp, "", a2, "B")
a3 = E(unreal.MaterialExpressionSubtract, -900, 900); C(a2, "", a3, "A"); C(vw, "", a3, "B")    # the wave runs DOWN the flag
sn = E(unreal.MaterialExpressionSine, -750, 900); sn.set_editor_property("period", 6.283); C(a3, "", sn, "")
amp = E(unreal.MaterialExpressionScalarParameter, -750, 1000); amp.set_editor_property("parameter_name", "Flutter"); amp.set_editor_property("default_value", 6.0)
m1 = E(unreal.MaterialExpressionMultiply, -600, 900); C(sn, "", m1, "A"); C(amp, "", m1, "B")
m2 = E(unreal.MaterialExpressionMultiply, -450, 820); C(m1, "", m2, "A"); C(mask, "", m2, "B")
nrm = E(unreal.MaterialExpressionVertexNormalWS, -600, 700)
wpo = E(unreal.MaterialExpressionMultiply, -300, 760); C(nrm, "", wpo, "A"); C(m2, "", wpo, "B")
MEL.connect_material_property(wpo, "", unreal.MaterialProperty.MP_WORLD_POSITION_OFFSET)
MEL.recompile_material(mat)
EAL.save_loaded_asset(mat, False)
for c in COLS:
    p = f"{D}/MI_PrayerFlagCloth_{c}"
    mi = unreal.load_asset(p) or at.create_asset(f"MI_PrayerFlagCloth_{c}", D, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    MEL.set_material_instance_parent(mi, mat)
    MEL.set_material_instance_texture_parameter_value(mi, "Flag", texs[c])
    MEL.update_material_instance(mi)
    EAL.save_loaded_asset(mi, False)
    print("MI", p)
print("done")
