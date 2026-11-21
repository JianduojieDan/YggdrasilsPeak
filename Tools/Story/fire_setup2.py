import unreal, json, random
# Runs INSIDE the editor via Tools/RoutePlanning/uepy. 2026-11-21 fire, take 2 (take 1's shader-noise flames never drew).
# Painted shapes instead (Art/Fire, PIL): T_FlameShape (soft teardrop, hot core, alpha = intensity), T_SmokeShape (soft
# column), T_FireNoise (distortion). Flames = unlit translucent cards whose UVs are pushed by a rising panned noise and
# flicker; smoke = faint grey column with its own noise. A visible stone ring of its own (the camp's old ring is hidden in
# chapter 1 as an old prop), yak-dung fuel, a warm flickering point light (light function from take 1). Tag "CampFire".
ROOT = "F:/UE_project/YggdrasilsPeak"
D = "/Game/Environment/Fire"
W_ = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
if W_.get_name() != "base_map":
    unreal.EditorLoadingAndSavingUtils.load_map("/Game/LevelPrototyping/base_map")
    W_ = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
EAL = unreal.EditorAssetLibrary; MEL = unreal.MaterialEditingLibrary
EAS = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
at = unreal.AssetToolsHelpers.get_asset_tools()
random.seed(87)


def imp(src):
    t = unreal.AssetImportTask()
    for k, v in (("filename", src), ("destination_path", D), ("automated", True), ("replace_existing", True), ("save", True)):
        t.set_editor_property(k, v)
    at.import_asset_tasks([t])
    p = list(t.get_editor_property("imported_object_paths") or [])
    return unreal.load_asset(str(p[0])) if p else None


flame_t = imp(f"{ROOT}/Art/Fire/T_FlameShape.png")
smoke_t = imp(f"{ROOT}/Art/Fire/T_SmokeShape.png")
noise_t = imp(f"{ROOT}/Art/Fire/T_FireNoise.png")
for t, srgb in ((flame_t, True), (smoke_t, True), (noise_t, False)):
    t.set_editor_property("srgb", srgb)
    t.set_editor_property("address_x", unreal.TextureAddress.TA_CLAMP if t != noise_t else unreal.TextureAddress.TA_WRAP)
    t.set_editor_property("address_y", unreal.TextureAddress.TA_CLAMP if t != noise_t else unreal.TextureAddress.TA_WRAP)
    if t == noise_t: t.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_GRAYSCALE)
    EAL.save_loaded_asset(t, False)


def material(name):
    m = unreal.load_asset(f"{D}/{name}")
    if m: MEL.delete_all_material_expressions(m)
    else: m = at.create_asset(name, D, unreal.Material, unreal.MaterialFactoryNew())
    m.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    m.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
    m.set_editor_property("two_sided", True)
    return m


def build(m, shape_tex, speed, dist_amt, glow, opacity_k, grey=None):
    E = lambda cls, x, y: MEL.create_material_expression(m, cls, x, y)
    C = MEL.connect_material_expressions
    def K(v, x, y):
        e = E(unreal.MaterialExpressionConstant, x, y); e.set_editor_property("r", v); return e
    uv = E(unreal.MaterialExpressionTextureCoordinate, -1400, 0)
    pan = E(unreal.MaterialExpressionPanner, -1200, 200)
    pan.set_editor_property("speed_x", 0.07); pan.set_editor_property("speed_y", speed)
    C(uv, "", pan, "Coordinate")
    nz = E(unreal.MaterialExpressionTextureSample, -1000, 200); nz.set_editor_property("texture", noise_t)
    nz.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_GRAYSCALE)   # sRGB off -> LINEAR (plain GRAYSCALE fails to compile)
    C(pan, "", nz, "UVs")
    nm = E(unreal.MaterialExpressionSubtract, -800, 200); C(nz, "R", nm, "A"); C(K(0.5, -1000, 320), "", nm, "B")
    # stronger toward the top (V = 0 at the top of the card)
    vmask = E(unreal.MaterialExpressionComponentMask, -1200, 400)
    for k in "rgba": vmask.set_editor_property(k, k == "g")
    C(uv, "", vmask, "")
    top = E(unreal.MaterialExpressionOneMinus, -1000, 400); C(vmask, "", top, "")
    d1 = E(unreal.MaterialExpressionMultiply, -650, 260); C(nm, "", d1, "A"); C(top, "", d1, "B")
    d2 = E(unreal.MaterialExpressionMultiply, -500, 260); C(d1, "", d2, "A"); C(K(dist_amt, -650, 340), "", d2, "B")
    duv = E(unreal.MaterialExpressionAdd, -350, 100); C(uv, "", duv, "A"); C(d2, "", duv, "B")
    tex = E(unreal.MaterialExpressionTextureSample, -150, 100); tex.set_editor_property("texture", shape_tex)
    C(duv, "", tex, "UVs")
    # flicker: 0.82..1 from time + where the card stands
    tm = E(unreal.MaterialExpressionTime, -800, 600)
    ts = E(unreal.MaterialExpressionMultiply, -650, 600); C(tm, "", ts, "A"); C(K(11.0, -800, 680), "", ts, "B")
    sn = E(unreal.MaterialExpressionSine, -500, 600); sn.set_editor_property("period", 6.283); C(ts, "", sn, "")
    fl = E(unreal.MaterialExpressionMultiply, -350, 600); C(sn, "", fl, "A"); C(K(0.09, -500, 680), "", fl, "B")
    fl2 = E(unreal.MaterialExpressionAdd, -200, 600); C(fl, "", fl2, "A"); C(K(0.91, -350, 680), "", fl2, "B")
    a = E(unreal.MaterialExpressionMultiply, 50, 300); C(tex, "A", a, "A"); C(fl2, "", a, "B")
    op = E(unreal.MaterialExpressionMultiply, 200, 300); C(a, "", op, "A"); C(K(opacity_k, 50, 380), "", op, "B")
    MEL.connect_material_property(op, "", unreal.MaterialProperty.MP_OPACITY)
    if grey is None:
        g = E(unreal.MaterialExpressionScalarParameter, 50, 0); g.set_editor_property("parameter_name", "Glow"); g.set_editor_property("default_value", glow)
        e1 = E(unreal.MaterialExpressionMultiply, 200, 0); C(tex, "RGB", e1, "A"); C(g, "", e1, "B")
        e2 = E(unreal.MaterialExpressionMultiply, 350, 0); C(e1, "", e2, "A"); C(fl2, "", e2, "B")
        MEL.connect_material_property(e2, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    else:
        c = E(unreal.MaterialExpressionConstant3Vector, 200, 0); c.set_editor_property("constant", unreal.LinearColor(grey, grey, grey * 0.97, 1))
        MEL.connect_material_property(c, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    MEL.recompile_material(m); EAL.save_loaded_asset(m, False)


flame_m = material("M_CampFireFlame2"); build(flame_m, flame_t, 0.9, 0.16, 12.0, 1.0)   # 26 blew out to white-yellow
smoke_m = material("M_CampFireSmoke2"); build(smoke_m, smoke_t, 0.25, 0.30, 0.0, 0.16, grey=0.07)   # faint: 0.28/0.32 read as a white beam at dawn
card = unreal.load_asset(f"{D}/SM_FireCard")
light_fn = unreal.load_asset(f"{D}/M_CampFireFlicker")
ring_mesh = unreal.load_asset("/Game/Fab/Stone_fireplace/stone_fireplace/StaticMeshes/stone_fireplace")
dung = unreal.load_asset(json.load(open(f"{ROOT}/Saved/AIAssets.json"))["dung_heap"]["mesh"])
print("MATS ok")

for a in EAS.get_all_level_actors():
    if "CampFire" in [str(t) for t in a.tags]:
        EAS.destroy_actor(a)
spots = []
olds = [a for a in EAS.get_all_level_actors() if isinstance(a, unreal.StaticMeshActor) and a.static_mesh_component.static_mesh
        and a.static_mesh_component.static_mesh.get_name() == "stone_fireplace"]
for a in olds:
    o, e = a.get_actor_bounds(False)
    # the old ring's bottom is sunk ~45 cm under the snow -> stand the fire on the snow surface itself (trace, ignoring old rings)
    hit = unreal.SystemLibrary.line_trace_single(W_, unreal.Vector(o.x, o.y, o.z + 800), unreal.Vector(o.x, o.y, o.z - 800),
                                                 unreal.TraceTypeQuery.TRACE_TYPE_QUERY1, False, olds, unreal.DrawDebugTrace.NONE, True)
    z = o.z - e.z
    if hit:
        t = hit.to_tuple(); vs = [x for x in t if isinstance(x, unreal.Vector)]; ha = [x for x in t if isinstance(x, unreal.Actor)]
        z = vs[1].z - 3.0                                            # impact point, 3 cm into the snow
        print("GROUND", a.get_actor_label(), round(vs[1].z), "old bottom", round(o.z - e.z), "hit", ha[0].get_actor_label() if ha else None)
    spots.append((a.get_actor_label(), unreal.Vector(o.x, o.y, z)))


def put(mesh, loc, rot, scale, mat, label, shadow=False, collide=False):
    s = EAS.spawn_actor_from_object(mesh, loc, rot)
    s.set_actor_scale3d(scale)
    c = s.static_mesh_component
    if mat: c.set_material(0, mat)
    if not collide: c.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
    c.set_editor_property("cast_shadow", shadow)
    s.tags = ["CampFire"]; s.set_folder_path("Environment/CampFire"); s.set_actor_label(label)
    return s


rb = ring_mesh.get_bounding_box(); rsz = max(rb.max.x - rb.min.x, rb.max.y - rb.min.y, 1.0)
for label, base in spots:
    rs = 115.0 / rsz                                                  # a 1.15 m ring of fire stones
    put(ring_mesh, base, unreal.Rotator(0, 0, random.uniform(0, 360)), unreal.Vector(rs, rs, rs), None, "CampFire_Ring", True, True)
    db = dung.get_bounding_box(); ds = 55.0 / max(db.max.x - db.min.x, 1.0)
    put(dung, base + unreal.Vector(0, 0, 2), unreal.Rotator(0, 0, random.uniform(0, 360)), unreal.Vector(ds, ds, ds * 0.5), None, "CampFire_Fuel", True)
    for i in range(3):
        put(card, base + unreal.Vector(random.uniform(-6, 6), random.uniform(-6, 6), 6), unreal.Rotator(0, 0, i * 60.0 + random.uniform(-10, 10)),
            unreal.Vector(0.42 + random.uniform(-0.05, 0.05), 1, 0.62 + random.uniform(-0.08, 0.08)), flame_m, f"CampFire_Flame{i}")
    for i in range(2):
        put(card, base + unreal.Vector(0, 0, 45), unreal.Rotator(0, 0, 45.0 + i * 90.0), unreal.Vector(0.9, 1, 2.6), smoke_m, f"CampFire_Smoke{i}")
    L = EAS.spawn_actor_from_class(unreal.PointLight, base + unreal.Vector(0, 0, 45), unreal.Rotator(0, 0, 0))
    lc = L.light_component
    lc.set_editor_property("intensity", 5.0)       # 16 lit a 6 m orange pool on the snow at dawn
    lc.set_editor_property("light_color", unreal.Color(r=255, g=146, b=66, a=255))
    lc.set_editor_property("attenuation_radius", 450.0)
    lc.set_editor_property("cast_shadows", False)
    if light_fn: lc.set_editor_property("light_function_material", light_fn)
    lc.set_editor_property("max_draw_distance", 6000.0)
    L.tags = ["CampFire"]; L.set_folder_path("Environment/CampFire"); L.set_actor_label("CampFire_Light")
    print("FIRE", label, round(base.x), round(base.y), round(base.z))
unreal.EditorLevelLibrary.save_current_level()
print("done")
