# Runs INSIDE the editor: M_RealmMote (particles) and M_PrismBow (Alfheim) -> /Game/Environment/Realms
import unreal
mel = unreal.MaterialEditingLibrary; at = unreal.AssetToolsHelpers.get_asset_tools()
D = "/Game/Environment/Realms"; T = "F:/UE_project/YggdrasilsPeak/Tools/Materials/"
def fresh(name):
    m = unreal.load_asset(f"{D}/{name}") or at.create_asset(name, D, unreal.Material, unreal.MaterialFactoryNew())
    mel.delete_all_material_expressions(m)
    m.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    m.set_editor_property("blend_mode", unreal.BlendMode.BLEND_ADDITIVE)
    m.set_editor_property("two_sided", True)
    return m
def custom(m, name, inputs):
    c = mel.create_material_expression(m, unreal.MaterialExpressionCustom, -400, 0)
    c.set_editor_property("code", open(T + name + ".hlsl").read())
    c.set_editor_property("output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT3)
    ins = []
    for n in inputs:
        i = unreal.CustomInput(); i.set_editor_property("input_name", n); ins.append(i)
    c.set_editor_property("inputs", ins)
    return c
def finish(m, c):
    mel.connect_material_property(c, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    mel.recompile_material(m); unreal.EditorAssetLibrary.save_loaded_asset(m)
# mote
m = fresh("M_RealmMote"); m.set_editor_property("used_with_niagara_sprites", True)
c = custom(m, "M_RealmMote", ["UV", "Col", "T", "Rand"])
for cls, n in [(unreal.MaterialExpressionTextureCoordinate, "UV"), (unreal.MaterialExpressionParticleColor, "Col"),
               (unreal.MaterialExpressionTime, "T"), (unreal.MaterialExpressionParticleRandom, "Rand")]:
    e = mel.create_material_expression(m, cls, -800, 0)
    if n == "Col":
        a = mel.create_material_expression(m, unreal.MaterialExpressionAppendVector, -600, 0)
        mel.connect_material_expressions(e, "RGB", a, "A"); mel.connect_material_expressions(e, "A", a, "B"); e = a
    mel.connect_material_expressions(e, "", c, n)
finish(m, c)
# bow
m = fresh("M_PrismBow")
c = custom(m, "M_PrismBow", ["UV", "T", "Bright", "Fade"])
for cls, n, v in [(unreal.MaterialExpressionTextureCoordinate, "UV", 0), (unreal.MaterialExpressionTime, "T", 0),
               (unreal.MaterialExpressionScalarParameter, "Bright", 0.22), (unreal.MaterialExpressionScalarParameter, "Fade", 1.0)]:
    e = mel.create_material_expression(m, cls, -800, 0)
    if cls is unreal.MaterialExpressionScalarParameter:
        e.set_editor_property("parameter_name", n); e.set_editor_property("default_value", v)
    mel.connect_material_expressions(e, "", c, n)
finish(m, c)
# the two particle systems start as copies of the spindrift (stripped and tuned by Tools/Realms/make_realm_particles)
for n in ["NS_RealmEmbers", "NS_RealmMotes"]:
    if not unreal.EditorAssetLibrary.does_asset_exist(f"{D}/{n}"):
        unreal.EditorAssetLibrary.duplicate_asset("/Game/Environment/Snow/NS_Spindrift", f"{D}/{n}")
print("ok")
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
print(unreal.EditorLevelLibrary.get_editor_world().get_name())
for a in eas.get_all_level_actors():
    if a.get_actor_label().startswith("Landmark_"):
        l = a.get_actor_location(); print(a.get_actor_label(), round(l.x), round(l.y), round(l.z))
