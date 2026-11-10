# Runs inside the editor. Fab's ice cliff material instances are translucent (colored transmittance), which draws see-through ghost walls.
# Forces them opaque and caps their 8K textures at 2K.
import unreal
for name in ("uhpocesfa", "uhupbgxfa", "veipbcmcb"):
    base = f"/Game/Fab/Megascans/3D/Ice_Cliff_{name}/Raw/{name}_tier_0"
    mi = unreal.load_asset(f"{base}/Materials/MI_{name}")
    o = mi.get_editor_property("base_property_overrides")
    o.set_editor_property("override_blend_mode", True)
    o.set_editor_property("blend_mode", unreal.BlendMode.BLEND_OPAQUE)
    mi.set_editor_property("base_property_overrides", o)
    unreal.MaterialEditingLibrary.update_material_instance(mi)
    print(name, mi.get_blend_mode())
    for v in mi.get_editor_property("texture_parameter_values"):
        t = v.parameter_value
        if t:
            t.set_editor_property("max_texture_size", 2048)
            unreal.EditorAssetLibrary.save_loaded_asset(t, False)
    unreal.EditorAssetLibrary.save_loaded_asset(mi, False)
print("done")
