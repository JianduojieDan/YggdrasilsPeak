import unreal, math, random
W_ = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
if W_.get_name() != "base_map":
    unreal.EditorLoadingAndSavingUtils.load_map("/Game/LevelPrototyping/base_map")
# Runs INSIDE the editor via Tools/RoutePlanning/uepy. 2026-11-21 (#86, user: "make sure there's a fire" - the stone fire
# ring at the Dingboche camp stood cold). No free fire VFX on Fab downloads without the launcher, so it is built here:
#  M_CampFireFlame  unlit additive flame on upright cards: a flame silhouette narrowing upward, eaten by rising noise,
#                   colour from deep red at the edges to yellow-white in the core, "Glow" strength
#  M_CampFireSmoke  unlit translucent grey wisps rising slower, fading in above the flames and out at the top
#  M_CampFireFlicker light function: the fire's light breathes and flickers (no shadow cost)
# and at every stone fire ring in the level (mesh stone_fireplace): yak-dung fuel (the Meshy dung_heap, small, as the
# Khumbu burns), 3 crossed flame cards, 2 smoke cards, a warm flickering point light (no shadows, 7 m, culled far away).
# Everything it spawns is tagged "CampFire" and replaced on a re-run.
ROOT = "F:/UE_project/YggdrasilsPeak"
D = "/Game/Environment/Fire"
EAL = unreal.EditorAssetLibrary; MEL = unreal.MaterialEditingLibrary
EAS = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
at = unreal.AssetToolsHelpers.get_asset_tools()
random.seed(86)


def imp(src, dest):
    t = unreal.AssetImportTask()
    for k, v in (("filename", src), ("destination_path", dest), ("automated", True), ("replace_existing", True), ("save", True)):
        t.set_editor_property(k, v)
    at.import_asset_tasks([t])
    p = list(t.get_editor_property("imported_object_paths") or [])
    return unreal.load_asset(str(p[0])) if p else None


def material(name):
    p = f"{D}/{name}"
    m = unreal.load_asset(p)
    if m: MEL.delete_all_material_expressions(m)
    else: m = at.create_asset(name, D, unreal.Material, unreal.MaterialFactoryNew())
    return m


card = imp(f"{ROOT}/Art/Fire/SM_FireCard.fbx", D) or unreal.load_asset(f"{D}/SM_FireCard")
card = unreal.load_asset(f"{D}/SM_FireCard")
bs = card.get_editor_property("body_setup")
if bs: bs.set_editor_property("collision_trace_flag", unreal.CollisionTraceFlag.CTF_USE_SIMPLE_AS_COMPLEX)
EAL.save_loaded_asset(card, False)


def build_noise_fire(m, smoke=False):
    E = lambda cls, x, y: MEL.create_material_expression(m, cls, x, y)
    C = MEL.connect_material_expressions
    K = lambda val, x, y: (lambda e: (e.set_editor_property("r", val), e)[1])(E(unreal.MaterialExpressionConstant, x, y))
    uv = E(unreal.MaterialExpressionTextureCoordinate, -2000, 0)
    u = E(unreal.MaterialExpressionComponentMask, -1800, -60); [u.set_editor_property(k, k == "r") for k in "rgba"]
    v = E(unreal.MaterialExpressionComponentMask, -1800, 60); [v.set_editor_property(k, k == "g") for k in "rgba"]
    C(uv, "", u, ""); C(uv, "", v, "")
    h = E(unreal.MaterialExpressionOneMinus, -1600, 60); C(v, "", h, "")                       # 0 bottom .. 1 top
    um = E(unreal.MaterialExpressionSubtract, -1600, -60); C(u, "", um, "A"); C(K(0.5, -1800, -120), "", um, "B")
    ax = E(unreal.MaterialExpressionAbs, -1450, -60); C(um, "", ax, "")
    x2 = E(unreal.MaterialExpressionMultiply, -1300, -60); C(ax, "", x2, "A"); C(K(2.0, -1450, -120), "", x2, "B")   # 0 centre .. 1 edge
    # rising noise: position = (u*3, v*2 + time*speed, seed)
    tm = E(unreal.MaterialExpressionTime, -2000, 300)
    sp = E(unreal.MaterialExpressionMultiply, -1800, 300); C(tm, "", sp, "A"); C(K(0.45 if smoke else 1.25, -2000, 360), "", sp, "B")
    us = E(unreal.MaterialExpressionMultiply, -1800, 180); C(u, "", us, "A"); C(K(1.6 if smoke else 3.0, -2000, 200), "", us, "B")
    vs = E(unreal.MaterialExpressionMultiply, -1800, 240); C(v, "", vs, "A"); C(K(1.4 if smoke else 2.4, -2000, 260), "", vs, "B")
    vt = E(unreal.MaterialExpressionAdd, -1600, 260); C(vs, "", vt, "A"); C(sp, "", vt, "B")
    ap = E(unreal.MaterialExpressionAppendVector, -1450, 220); C(us, "", ap, "A"); C(vt, "", ap, "B")
    pir = E(unreal.MaterialExpressionObjectPositionWS, -1600, 400)
    seed = E(unreal.MaterialExpressionComponentMask, -1450, 400); [seed.set_editor_property(k, k == "r") for k in "rgba"]; C(pir, "", seed, "")
    sd = E(unreal.MaterialExpressionMultiply, -1300, 400); C(seed, "", sd, "A"); C(K(0.013, -1450, 460), "", sd, "B")
    p3 = E(unreal.MaterialExpressionAppendVector, -1300, 260); C(ap, "", p3, "A"); C(sd, "", p3, "B")
    nz = E(unreal.MaterialExpressionNoise, -1100, 260)
    nz.set_editor_property("scale", 1.0); nz.set_editor_property("levels", 4); nz.set_editor_property("output_min", 0.0); nz.set_editor_property("output_max", 1.0)
    nz.set_editor_property("turbulence", True)
    C(p3, "", nz, "Position")
    if not smoke:
        # flame silhouette: width shrinks upward, fades out toward the top, eaten by the noise
        w = E(unreal.MaterialExpressionSubtract, -1300, 60); C(K(1.0, -1450, 0), "", w, "A")
        wh = E(unreal.MaterialExpressionMultiply, -1450, 100); C(h, "", wh, "A"); C(K(0.82, -1600, 140), "", wh, "B"); C(wh, "", w, "B")
        dv = E(unreal.MaterialExpressionDivide, -1150, -20); C(x2, "", dv, "A"); C(w, "", dv, "B")
        side = E(unreal.MaterialExpressionOneMinus, -1000, -20); C(dv, "", side, "")
        sides = E(unreal.MaterialExpressionSaturate, -880, -20); C(side, "", sides, "")
        top = E(unreal.MaterialExpressionOneMinus, -1150, 120); C(h, "", top, "")
        topp = E(unreal.MaterialExpressionPower, -1000, 120); C(top, "", topp, "Base"); C(K(0.7, -1150, 180), "", topp, "Exponent")
        base = E(unreal.MaterialExpressionMultiply, -800, 40); C(sides, "", base, "A"); C(topp, "", base, "B")
        b2 = E(unreal.MaterialExpressionMultiply, -680, 40); C(base, "", b2, "A"); C(K(1.7, -800, 100), "", b2, "B")
        nn = E(unreal.MaterialExpressionMultiply, -800, 260); C(nz, "", nn, "A"); C(K(0.95, -950, 320), "", nn, "B")
        it = E(unreal.MaterialExpressionSubtract, -560, 120); C(b2, "", it, "A"); C(nn, "", it, "B")
        its = E(unreal.MaterialExpressionSaturate, -440, 120); C(it, "", its, "")
        red = E(unreal.MaterialExpressionConstant3Vector, -560, 260); red.set_editor_property("constant", unreal.LinearColor(1.0, 0.22, 0.03, 1))
        org = E(unreal.MaterialExpressionConstant3Vector, -560, 340); org.set_editor_property("constant", unreal.LinearColor(1.0, 0.62, 0.16, 1))
        wht = E(unreal.MaterialExpressionConstant3Vector, -560, 420); wht.set_editor_property("constant", unreal.LinearColor(1.0, 0.92, 0.62, 1))
        l1 = E(unreal.MaterialExpressionLinearInterpolate, -320, 300); C(red, "", l1, "A"); C(org, "", l1, "B"); C(its, "", l1, "Alpha")
        hot = E(unreal.MaterialExpressionPower, -320, 420); C(its, "", hot, "Base"); C(K(3.0, -440, 470), "", hot, "Exponent")
        l2 = E(unreal.MaterialExpressionLinearInterpolate, -180, 340); C(l1, "", l2, "A"); C(wht, "", l2, "B"); C(hot, "", l2, "Alpha")
        ip = E(unreal.MaterialExpressionPower, -320, 140); C(its, "", ip, "Base"); C(K(1.4, -440, 200), "", ip, "Exponent")
        g = E(unreal.MaterialExpressionScalarParameter, -320, 60); g.set_editor_property("parameter_name", "Glow"); g.set_editor_property("default_value", 14.0)
        e1 = E(unreal.MaterialExpressionMultiply, -180, 120); C(ip, "", e1, "A"); C(g, "", e1, "B")
        em = E(unreal.MaterialExpressionMultiply, -40, 220); C(l2, "", em, "A"); C(e1, "", em, "B")
        MEL.connect_material_property(em, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    else:
        # smoke: soft column (wider at the top), fades in above the flames and out high up, thin by the noise
        side = E(unreal.MaterialExpressionOneMinus, -1100, -60); C(x2, "", side, "")
        sides = E(unreal.MaterialExpressionSaturate, -980, -60); C(side, "", sides, "")
        fin = E(unreal.MaterialExpressionMultiply, -1150, 100); C(h, "", fin, "A"); C(K(5.0, -1300, 140), "", fin, "B")
        fins = E(unreal.MaterialExpressionSaturate, -1000, 100); C(fin, "", fins, "")
        fo = E(unreal.MaterialExpressionOneMinus, -1150, 180); C(h, "", fo, "")
        mask = E(unreal.MaterialExpressionMultiply, -850, 60); C(sides, "", mask, "A"); C(fins, "", mask, "B")
        mask2 = E(unreal.MaterialExpressionMultiply, -720, 100); C(mask, "", mask2, "A"); C(fo, "", mask2, "B")
        nn = E(unreal.MaterialExpressionMultiply, -720, 260); C(nz, "", nn, "A"); C(mask2, "", nn, "B")
        op = E(unreal.MaterialExpressionScalarParameter, -720, 340); op.set_editor_property("parameter_name", "Density"); op.set_editor_property("default_value", 0.32)
        o2 = E(unreal.MaterialExpressionMultiply, -560, 300); C(nn, "", o2, "A"); C(op, "", o2, "B")
        MEL.connect_material_property(o2, "", unreal.MaterialProperty.MP_OPACITY)
        col = E(unreal.MaterialExpressionConstant3Vector, -560, 120); col.set_editor_property("constant", unreal.LinearColor(0.22, 0.21, 0.2, 1))
        MEL.connect_material_property(col, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)


fl = material("M_CampFireFlame")
fl.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
fl.set_editor_property("blend_mode", unreal.BlendMode.BLEND_ADDITIVE)
fl.set_editor_property("two_sided", True)
build_noise_fire(fl, smoke=False); MEL.recompile_material(fl); EAL.save_loaded_asset(fl, False)
sm = material("M_CampFireSmoke")
sm.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
sm.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
sm.set_editor_property("two_sided", True)
build_noise_fire(sm, smoke=True); MEL.recompile_material(sm); EAL.save_loaded_asset(sm, False)
lf = material("M_CampFireFlicker")
lf.set_editor_property("material_domain", unreal.MaterialDomain.MD_LIGHT_FUNCTION)
E = lambda cls, x, y: MEL.create_material_expression(lf, cls, x, y)
tm = E(unreal.MaterialExpressionTime, -900, 0)
ap = E(unreal.MaterialExpressionAppendVector, -700, 0)
k = E(unreal.MaterialExpressionConstant, -900, 80); k.set_editor_property("r", 0.0)
s1 = E(unreal.MaterialExpressionMultiply, -800, -60); kk = E(unreal.MaterialExpressionConstant, -950, -100); kk.set_editor_property("r", 3.1)
MEL.connect_material_expressions(tm, "", s1, "A"); MEL.connect_material_expressions(kk, "", s1, "B")
MEL.connect_material_expressions(s1, "", ap, "A"); MEL.connect_material_expressions(k, "", ap, "B")
ap3 = E(unreal.MaterialExpressionAppendVector, -560, 0); MEL.connect_material_expressions(ap, "", ap3, "A"); MEL.connect_material_expressions(k, "", ap3, "B")
nz = E(unreal.MaterialExpressionNoise, -400, 0)
nz.set_editor_property("levels", 3); nz.set_editor_property("output_min", 0.62); nz.set_editor_property("output_max", 1.0)
MEL.connect_material_expressions(ap3, "", nz, "Position")
MEL.connect_material_property(nz, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
MEL.recompile_material(lf); EAL.save_loaded_asset(lf, False)
print("MATERIALS ok")

# ---- place a fire in every stone fire ring
for a in EAS.get_all_level_actors():
    if "CampFire" in [str(t) for t in a.tags]:
        EAS.destroy_actor(a)
dung = None
try:
    import json
    dung = unreal.load_asset(json.load(open(f"{ROOT}/Saved/AIAssets.json"))["dung_heap"]["mesh"])
except Exception as e:
    print("no dung heap", e)
rings = []
for a in EAS.get_all_level_actors():
    if isinstance(a, unreal.StaticMeshActor):
        m = a.static_mesh_component.static_mesh
        if m and m.get_name() == "stone_fireplace":
            rings.append(a)
print("fire rings", len(rings), [r.get_actor_label() for r in rings][:12])


def put(mesh, loc, rot, scale, mat, label, shadow=False):
    s = EAS.spawn_actor_from_object(mesh, loc, rot)
    s.set_actor_scale3d(scale)
    c = s.static_mesh_component
    if mat: c.set_material(0, mat)
    c.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
    c.set_editor_property("cast_shadow", shadow)
    s.tags = ["CampFire"]; s.set_folder_path("Environment/CampFire"); s.set_actor_label(label)
    return s


for r in rings:
    o, ext = r.get_actor_bounds(False)
    base = unreal.Vector(o.x, o.y, o.z - ext.z + 4.0)               # inside the ring, a little above its floor
    ring_r = min(ext.x, ext.y)
    fw = max(28.0, ring_r * 0.75)                                    # flame card width (cm) from the ring size
    if dung:
        db = dung.get_bounding_box(); dsz = max(db.max.x - db.min.x, 1.0)
        s_ = ring_r * 0.95 / dsz
        put(dung, base - unreal.Vector(0, 0, 3), unreal.Rotator(0, 0, random.uniform(0, 360)), unreal.Vector(s_, s_, s_ * 0.6), None, "CampFire_Fuel", True)
    for i in range(3):
        yaw = i * 60.0 + random.uniform(-8, 8)
        put(card, base + unreal.Vector(0, 0, 2), unreal.Rotator(0, 0, yaw), unreal.Vector(fw / 100.0, 1, fw * 1.35 / 100.0), fl, f"CampFire_Flame{i}")
    for i in range(2):
        yaw = 45.0 + i * 90.0
        put(card, base + unreal.Vector(0, 0, fw * 0.6), unreal.Rotator(0, 0, yaw), unreal.Vector(fw * 2.2 / 100.0, 1, fw * 5.0 / 100.0), sm, f"CampFire_Smoke{i}")
    L = EAS.spawn_actor_from_class(unreal.PointLight, base + unreal.Vector(0, 0, fw * 0.55), unreal.Rotator(0, 0, 0))
    lc = L.light_component
    lc.set_editor_property("intensity", 22.0)
    lc.set_editor_property("light_color", unreal.Color(r=255, g=150, b=72, a=255))
    lc.set_editor_property("attenuation_radius", 700.0)
    lc.set_editor_property("cast_shadows", False)
    lc.set_editor_property("light_function_material", lf)
    lc.set_editor_property("max_draw_distance", 6000.0)
    L.tags = ["CampFire"]; L.set_folder_path("Environment/CampFire"); L.set_actor_label("CampFire_Light")
    print("FIRE at", r.get_actor_label(), round(base.x), round(base.y), round(base.z), "ring r", round(ring_r), "flame w", round(fw))
unreal.EditorLevelLibrary.save_current_level()
print("done")
