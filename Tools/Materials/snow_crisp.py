# Runs INSIDE the editor (Tools/RoutePlanning/uepy). 2026-11-13, user: "the snow looks blurry; real snow looks a lot better".
# Gentle crispness near the camera, patched into the snow Custom node of BOTH terrain materials (landscape and route
# detail tiles) by exact text replacement, so each keeps its own other changes:
#   - snow grain from the texture a little stronger on soft snow (0.05 -> 0.09)
#   - sparkle (ice-crystal glints) reaches about twice as far (pixel footprint fade 0.6-1.6 cm -> 1.0-3.2 cm)
#   - wind ripples also on soft snow, faint and patchy (22% of the hard-snow sastrugi)
import unreal
MEL = unreal.MaterialEditingLibrary
REPL = [
    ("float3 micro=hx.SN(lerp(n,hx.SN(snowW,n),lerp(0.05,0.28,hard)),n);", "float3 micro=hx.SN(lerp(n,hx.SN(snowW,n),lerp(0.09,0.30,hard)),n);"),
    ("float spark=step(0.988,gh.x)*(1.0-smoothstep(0.6,1.6,fp))*(1.0-ice);", "float spark=step(0.988,gh.x)*(1.0-smoothstep(1.0,3.2,fp))*(1.0-ice);"),
    ("/es*4.0*hard*lerp(0.2,1.0,hx.Fbm(WP.xy/2500.0+7.7));", "/es*4.0*lerp(0.22,1.0,hard)*lerp(0.2,1.0,hx.Fbm(WP.xy/2500.0+7.7));"),
]
for path in ["/Game/Materials/M_EverestLandscape", "/Game/Materials/M_EverestDetailTile"]:
    m = unreal.load_asset(path)
    nodes = [e for e in unreal.ObjectIterator(unreal.MaterialExpressionCustom) if e.get_outer() == m]
    hit = [e for e in nodes if "---- Snow (2026-10-22)" in e.get_editor_property("code")]
    if not hit:
        print(path, "no snow node found among", len(nodes)); continue
    node = hit[0]
    code = node.get_editor_property("code")
    done = []
    for a, b in REPL:
        if a in code:
            code = code.replace(a, b); done.append(True)
        else:
            done.append(b in code)            # already patched
    node.set_editor_property("code", code)
    MEL.recompile_material(m)
    unreal.EditorAssetLibrary.save_loaded_asset(m, False)
    print(path, node.get_name(), "patched", done)
