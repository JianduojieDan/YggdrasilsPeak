# Runs INSIDE the editor (Tools/RoutePlanning/uepy). 2026-11-14, snow step P2d, tuning after the first ripple shots:
#   - ripples a little gentler (x1.5 -> x1.25, shading and height alike) and in clearer patches with calm smooth snow
#     between them (they covered every flat metre and looked busy near the feet)
#   - per-cell turn +-0.25 -> +-0.15 rad: fewer crossing crests where two cells meet
#   - crust cracks softer: in shade their ambient occlusion and blue tint drew dark lines like dried mud
import unreal
MEL = unreal.MaterialEditingLibrary

STRUCT = [("a=(float3(q1.x,q2.x,q3.x)-0.5)*0.5;", "a=(float3(q1.x,q2.x,q3.x)-0.5)*0.3;"),
          ("// a +-0.25 rad turn and +-12 % size.", "// a +-0.15 rad turn and +-12 % size.")]
PATCHES = ("smoothstep(0.30,0.62,hx.Fbm(WP.xy/2200.0+61.3))", "smoothstep(0.40,0.66,hx.Fbm(WP.xy/1600.0+61.3))")
COLOUR = STRUCT + [
    PATCHES,
    ("gR=rpT*rpV*(300.0*rpt)*1.5*ripK*ripF; // x1.5:", "gR=rpT*rpV*(300.0*rpt)*1.25*ripK*ripF; // x1.25:"),
    ("sCol*=lerp(float3(1,1,1),float3(0.84,0.91,1.0),cav*0.8);", "sCol*=lerp(float3(1,1,1),float3(0.84,0.91,1.0),cav*0.45); // softer (P2d)"),
    ("lerp(1.0,0.55,cav)", "lerp(1.0,0.78,cav)"),
]
DISP = STRUCT + [
    PATCHES,
    ("d+=(ripH-0.5)*3.0*1.5*ripK; // map 0..1 = -1.5..+1.5 cm; x1.5 as the shading",
     "d+=(ripH-0.5)*3.0*1.25*ripK; // map 0..1 = -1.5..+1.5 cm; x1.25 as the shading"),
]


def patch(node, repl):
    code = node.get_editor_property("code"); crlf = "\r\n" in code; res = []
    for a, b in repl:
        if crlf:
            a = a.replace("\r\n", "\n").replace("\n", "\r\n"); b = b.replace("\r\n", "\n").replace("\n", "\r\n")
        if code.count(a) == 1:
            code = code.replace(a, b); res.append("ok")
        elif b in code:
            res.append("already")
        else:
            res.append("MISSING" if a not in code else "TWICE")
    if all(r in ("ok", "already") for r in res):
        node.set_editor_property("code", code)
    return res


for path in ["/Game/Materials/M_EverestDetailTile", "/Game/Materials/M_EverestLandscape"]:
    m = unreal.load_asset(path)
    ex = {e.get_name(): e for e in unreal.ObjectIterator(unreal.MaterialExpressionCustom) if e.get_outer() == m}
    print(path, "colour", patch(ex["MaterialExpressionCustom_0"], COLOUR))
    if "WindRip" in ex["MaterialExpressionCustom_1"].get_editor_property("code"):
        print(path, "displacement", patch(ex["MaterialExpressionCustom_1"], DISP))
    MEL.recompile_material(m)
    unreal.EditorAssetLibrary.save_loaded_asset(m, False)
    print(path, "recompiled + saved")
