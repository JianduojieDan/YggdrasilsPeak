# Runs INSIDE the editor (Tools/RoutePlanning/uepy). 2026-11-14, snow step P2b: after P2 the crust plates and cracks
# were there (contrast-stretched shots show them) but far too faint to see at normal contrast, especially in shade.
#   - near the eye the crust's slopes are exaggerated (1.6-1.9x the texture instead of 0.5-0.7x): the normals were made
#     physically from mm-cm heights, which read flat at game exposure
#   - cracks and dips below the crust's mean height get ambient occlusion and a cool tint (the mean stays untouched, so
#     the near snow is not darker on average than the far snow - no ring around the player)
import unreal
MEL = unreal.MaterialEditingLibrary

REPL = [
    ("float3 micro=hx.SN(lerp(n,hx.SN(snowW,n),lerp(lerp(0.09,0.30,hard),lerp(0.50,0.70,hard),nearK)),n);",
     "float3 micro=hx.SN(lerp(n,hx.SN(snowW,n),lerp(lerp(0.09,0.30,hard),lerp(1.6,1.9,hard),nearK)),n); // >1 = steeper than the map, near only (P2b)"),
    ("float cav=saturate((0.52-snowH)*3.0)*nearK;", "float cav=saturate((0.47-snowH)*5.0)*nearK; // cracks and dips only (P2b)"),
    ("sCol*=lerp(float3(1,1,1),float3(0.86,0.92,1.0),cav*0.6);", "sCol*=lerp(float3(1,1,1),float3(0.84,0.91,1.0),cav*0.8);"),
    ("AO=lerp(lerp(1.0,0.9,hollow)*lerp(1.0,0.75,cav),rockM.r,rm);", "AO=lerp(lerp(1.0,0.9,hollow)*lerp(1.0,0.55,cav),rockM.r,rm);"),
]

def patch(node, repl):
    code = node.get_editor_property("code"); crlf = "\r\n" in code; res = []
    for a, b in repl:
        if crlf:
            a = a.replace("\n", "\r\n"); b = b.replace("\n", "\r\n")
        if a in code: code = code.replace(a, b); res.append("ok")
        elif b in code: res.append("already")
        else: res.append("MISSING")
    node.set_editor_property("code", code)
    return res

for path in ["/Game/Materials/M_EverestDetailTile", "/Game/Materials/M_EverestLandscape"]:
    m = unreal.load_asset(path)
    for c in [e for e in unreal.ObjectIterator(unreal.MaterialExpressionCustom) if e.get_outer() == m]:
        if "---- Snow (2026-10-22)" in c.get_editor_property("code"):
            print(path, patch(c, REPL))
    MEL.recompile_material(m)
    unreal.EditorAssetLibrary.save_loaded_asset(m, False)
    print(path, "recompiled + saved")
