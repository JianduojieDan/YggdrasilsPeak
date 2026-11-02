# Runs INSIDE the editor (uepy.py). The UE glTF export drops most textures; this writes each pack item's base
# colour texture(s) as PNG next to its .glb (Saved/ItemIcons/src/<Item>_<slot>.png) for render_item_icons.py.
import unreal, os
OUT = "F:/UE_project/YggdrasilsPeak/Saved/ItemIcons/src"
exec(open("F:/UE_project/YggdrasilsPeak/Tools/UI/export_item_meshes.py").read().split("for name, path in ITEMS.items():")[0].split("os.makedirs(OUT, exist_ok=True)")[1])
KEYS = ("basecolor", "albedo", "diffuse", "base_color", "color")
for name, path in ITEMS.items():
    m = unreal.load_asset(path)
    for i, sm in enumerate(m.static_materials):
        mi = sm.material_interface
        tex = None
        if isinstance(mi, unreal.MaterialInstance):
            for p in mi.texture_parameter_values:
                if p.parameter_value and any(k in str(p.parameter_info.name).lower() for k in KEYS):
                    tex = p.parameter_value; break
        if tex is None:
            # plain material: first texture it uses whose name looks like a colour map
            for t in unreal.MaterialEditingLibrary.get_used_textures(mi):
                if any(k in t.get_name().lower() for k in KEYS + ("_d", "texture_0")):
                    tex = t; break
        if tex is None:
            print(name, i, "no colour texture", mi.get_name()); continue
        t = unreal.AssetExportTask(); t.object = tex; t.filename = f"{OUT}/{name}_{i}.png"
        t.automated = True; t.prompt = False; t.replace_identical = True
        t.exporter = unreal.TextureExporterPNG()
        print(name, i, tex.get_name(), unreal.Exporter.run_asset_export_task(t))
