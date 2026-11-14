import unreal
mel = unreal.MaterialEditingLibrary; at = unreal.AssetToolsHelpers.get_asset_tools(); EAL = unreal.EditorAssetLibrary
base = unreal.load_asset("/Game/Characters/Climber_Rigged/M_Climber_Rigged")
texs = []
for e in unreal.ObjectIterator(unreal.MaterialExpressionTextureBase):
    if e.get_outer() == base and e.get_editor_property("texture"):
        texs.append((type(e).__name__, e.get_editor_property("texture").get_path_name(), e.get_editor_property("sampler_type")))
print("climber textures", texs)
col = next((t for t in texs if "Normal" not in t[1] and t[2] != unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL), None)
nrm = next((t for t in texs if t[2] == unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL), None)
D = "/Game/Story/Materials"
m = unreal.load_asset(f"{D}/M_StoryNPC") or at.create_asset("M_StoryNPC", D, unreal.Material, unreal.MaterialFactoryNew())
mel.delete_all_material_expressions(m)
m.set_editor_property("used_with_skeletal_mesh", True)
c = mel.create_material_expression(m, unreal.MaterialExpressionCustom, -400, 0)
c.set_editor_property("code", """float l = dot(B, float3(0.3, 0.59, 0.11));
float3 t = Tint * l * 2.2;                                   // the garment takes the person's colour, keeps the folds

return lerp(B, t, Amount) * Bright;""")
ins = []
for n in ["B", "Tint", "Amount", "Bright"]:
    i = unreal.CustomInput(); i.set_editor_property("input_name", n); ins.append(i)
c.set_editor_property("inputs", ins)
ts = mel.create_material_expression(m, unreal.MaterialExpressionTextureSample, -800, -100)
if col: ts.set_editor_property("texture", unreal.load_asset(col[1].split(".")[0]))
tint = mel.create_material_expression(m, unreal.MaterialExpressionVectorParameter, -800, 100); tint.set_editor_property("parameter_name", "Tint"); tint.set_editor_property("default_value", unreal.LinearColor(0.5, 0.5, 0.5, 1))
amt = mel.create_material_expression(m, unreal.MaterialExpressionScalarParameter, -800, 200); amt.set_editor_property("parameter_name", "Amount"); amt.set_editor_property("default_value", 0.8)
br = mel.create_material_expression(m, unreal.MaterialExpressionScalarParameter, -800, 300); br.set_editor_property("parameter_name", "Bright"); br.set_editor_property("default_value", 1.0)
mel.connect_material_expressions(ts, "RGB", c, "B"); mel.connect_material_expressions(tint, "", c, "Tint")
mel.connect_material_expressions(amt, "", c, "Amount"); mel.connect_material_expressions(br, "", c, "Bright")
mel.connect_material_property(c, "", unreal.MaterialProperty.MP_BASE_COLOR)
if nrm:
    tn = mel.create_material_expression(m, unreal.MaterialExpressionTextureSample, -800, 450)
    tn.set_editor_property("texture", unreal.load_asset(nrm[1].split(".")[0])); tn.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)
    mel.connect_material_property(tn, "RGB", unreal.MaterialProperty.MP_NORMAL)
r = mel.create_material_expression(m, unreal.MaterialExpressionConstant, -800, 600); r.set_editor_property("r", 0.8)
mel.connect_material_property(r, "", unreal.MaterialProperty.MP_ROUGHNESS)
mel.recompile_material(m); EAL.save_loaded_asset(m, False)
print("M_StoryNPC done", col, nrm)
