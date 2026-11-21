import unreal, json
# Runs INSIDE the editor via Tools/RoutePlanning/uepy. Dingboche field walls at 4,400 m carry grey-olive lichen, not lush
# green moss: desaturate + cool the scanned walls' colour (M_MS_Base Saturation / Brightness / BaseColor Tint), and turn
# the material AO on. Also prints the parameters of the project's snow-dusted rock material (for snow on the wall tops).
ROOT = "F:/UE_project/YggdrasilsPeak"
EAL = unreal.EditorAssetLibrary; MEL = unreal.MaterialEditingLibrary
info = json.load(open(f"{ROOT}/Saved/Fab_2026_11_21.json"))
LOOK = {"wall_mossy1": (0.30, 0.92), "wall_mossy2": (0.30, 0.92), "wall_jp1": (0.55, 0.95), "wall_jp2": (0.55, 0.95)}
for name, (sat, bri) in LOOK.items():
    mi = unreal.load_asset(info[name]["mi"])
    MEL.set_material_instance_scalar_parameter_value(mi, "Saturation", sat)
    MEL.set_material_instance_scalar_parameter_value(mi, "Brightness", bri)
    MEL.set_material_instance_vector_parameter_value(mi, "BaseColor Tint", unreal.LinearColor(0.97, 0.96, 0.93, 1.0))
    MEL.set_material_instance_static_switch_parameter_value(mi, "Enable Material AO", True)
    MEL.update_material_instance(mi)
    EAL.save_loaded_asset(mi, False)
    print("LOOK", name, sat, bri)
rs = unreal.load_asset("/Game/Environment/RockSnow/MI_RockSnow_Stone_Dusted")
p = rs.get_editor_property("parent") if rs else None
while p and isinstance(p, unreal.MaterialInstance):
    p = p.get_editor_property("parent")
if p:
    print("ROCKSNOW parent", p.get_path_name())
    print("tex", [str(x) for x in MEL.get_texture_parameter_names(p)])
    print("scal", [str(x) for x in MEL.get_scalar_parameter_names(p)])
    print("vec", [str(x) for x in MEL.get_vector_parameter_names(p)])
    print("mi tex values", [(str(x), (MEL.get_material_instance_texture_parameter_value(rs, x) or unreal.Texture()).get_name()) for x in MEL.get_texture_parameter_names(p)])
