# Runs inside the editor. Rock relief was capped near +-1 m: the displacement range was 200 cm (0.21 x landscape Z scale 951.4).
# Range is now 600 cm, the shader divides by the same number, and the rock height is 450 cm. Snow stays at 0.
import unreal
RANGE = 600.0
m = unreal.load_asset("/Game/Materials/M_EverestLandscape")
L = unreal.MaterialEditingLibrary
for e in L.get_material_expressions(m):
    if e.get_name() == "MaterialExpressionCustom_1":
        code = e.get_editor_property("code")
        assert "return 0.5+d/200.0;" in code
        e.set_editor_property("code", code.replace("return 0.5+d/200.0;", "return 0.5+d/%.1f;" % RANGE))
    if e.get_class().get_name() == "MaterialExpressionScalarParameter" and str(e.get_editor_property("parameter_name")) == "RockDisplacementCm":
        e.set_editor_property("default_value", 450.0)
d = m.get_editor_property("displacement_scaling")
print("displacement before", d)
d.set_editor_property("magnitude", RANGE / 951.4)
m.set_editor_property("displacement_scaling", d)
print("displacement after", m.get_editor_property("displacement_scaling"))
L.recompile_material(m)
unreal.EditorAssetLibrary.save_loaded_asset(m, False)
print("saved")
