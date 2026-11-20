# Runs INSIDE the editor (uepy.py). #69 (2026-11-20): the trail trench and its shading faded only linearly with rock
# ((1.0 - Rock)), so moraine/rubble (rock mask ~0.04-0.2 where it already looks like rubble) still got a 80-96% trench with
# blue walls and white banks. Every TrailDisp / TrailShade Custom node in the landscape and the detail-tile material now
# uses (1.0 - smoothstep(0.03, 0.2, Rock)). Only the code text changes (inputs and wires stay), then the materials are
# recompiled and saved. Matches the edited Tools/Materials/M_EverestLandscape_Trail{Disp,Shade}.hlsl.
import unreal
MEL = unreal.MaterialEditingLibrary
OLD = ["* fade * (1.0 - Rock) *", "max(edge.x, edge.y))) * (1.0 - Rock) *"]
NEW = ["* fade * (1.0 - smoothstep(0.03, 0.2, Rock)) *", "max(edge.x, edge.y))) * (1.0 - smoothstep(0.03, 0.2, Rock)) *"]
report = []
for path in ["/Game/Materials/M_EverestLandscape", "/Game/Materials/M_EverestDetailTile"]:
    m = unreal.load_asset(path)
    changed = 0
    for e in unreal.ObjectIterator(unreal.MaterialExpressionCustom):
        if e.get_outer() != m:
            continue
        code = e.get_editor_property("code")
        new = code
        for o, n in zip(OLD, NEW):
            new = new.replace(o, n)
        if new != code:
            e.set_editor_property("code", new)
            changed += 1
            report.append(f"{path}: {e.get_name()} updated")
        elif "th.Height(Trail" in code or "max(edge.x, edge.y)))" in code:
            report.append(f"{path}: {e.get_name()} trail node already faded or different: check by hand")
    if changed:
        MEL.recompile_material(m)
        unreal.EditorAssetLibrary.save_asset(path)
print("\n".join(report) if report else "no trail nodes found")
