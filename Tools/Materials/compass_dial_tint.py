import unreal
# Runs INSIDE the editor via Tools/RoutePlanning/uepy. 2026-11-21 (user: "the compass looks like the flashlight itself -
# make it look natural"): the dial's enamel texture went straight into Base Colour at near-white, so under any lamp the
# dial read as a light source. Puts a "Tint" vector parameter between the texture and Base Colour, default an aged, carried
# cream (the runes' glow stays emissive, untouched). Safe to run again: an existing Tint is only updated.
MEL = unreal.MaterialEditingLibrary
m = unreal.load_asset("/Game/Props/Compass/M_CompassDial")
TINT = unreal.LinearColor(0.72, 0.65, 0.52, 1.0)
src = MEL.get_material_property_input_node(m, unreal.MaterialProperty.MP_BASE_COLOR)
if isinstance(src, unreal.MaterialExpressionMultiply):
    print("tint already in place")
    for e in [src]:
        pass
else:
    tint = MEL.create_material_expression(m, unreal.MaterialExpressionVectorParameter, -450, -120)
    tint.set_editor_property("parameter_name", "Tint"); tint.set_editor_property("default_value", TINT)
    mul = MEL.create_material_expression(m, unreal.MaterialExpressionMultiply, -250, -40)
    MEL.connect_material_expressions(src, "RGB", mul, "A"); MEL.connect_material_expressions(tint, "", mul, "B")
    MEL.connect_material_property(mul, "", unreal.MaterialProperty.MP_BASE_COLOR)
    print("tint added", src.get_class().get_name())
MEL.recompile_material(m); unreal.EditorAssetLibrary.save_loaded_asset(m, False)
print("done")
