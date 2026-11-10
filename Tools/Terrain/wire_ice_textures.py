# Runs inside the editor. The landscape's Glacier node takes the Icelandic Glacier surface (inputs IceC, IceN). Safe to rerun.
import unreal
L = unreal.MaterialEditingLibrary
m = unreal.load_asset("/Game/Materials/M_EverestLandscape")
base = "/Game/Fab/Megascans/Surfaces/Icelandic_Glacier_ucnkcgkn/High/ucnkcgkn_tier_1/Textures/T_ucnkcgkn_4K_"
ex = L.get_material_expressions(m)
node = [e for e in ex if e.get_name() == "MaterialExpressionCustom_5"][0]
objs = [e for e in ex if e.get_class().get_name() == "MaterialExpressionTextureObject" and e.get_editor_property("texture") and "veipbcmcb" in e.get_editor_property("texture").get_name() or (e.get_class().get_name() == "MaterialExpressionTextureObject" and e.get_editor_property("texture") and "ucnkcgkn" in e.get_editor_property("texture").get_name())]
for o in objs:
    normal = o.get_editor_property("texture").get_name().endswith("_N")
    o.set_editor_property("texture", unreal.load_asset(base + ("N" if normal else "B")))
    print("connect", "IceN" if normal else "IceC", L.connect_material_expressions(o, "", node, "IceN" if normal else "IceC"))
print([(str(i.get_editor_property("input_name")), i.get_editor_property("input").expression.get_name() if i.get_editor_property("input").expression else None) for i in node.get_editor_property("inputs")][-3:])
