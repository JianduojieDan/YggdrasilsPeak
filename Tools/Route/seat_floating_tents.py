# Runs INSIDE the editor (Tools/RoutePlanning/uepy) with base_map open. 2026-11-14: the route check
# (Saved/RouteCheck/20261114_023557/objects.csv) measured 14 camp tents 42-109 cm above the ground by tracing their
# real collision (method "trace"): EBC, Camp 1, Camp 2, Camp 3 and the South Col - the user's report #29. Each one is
# lowered by its gap plus 3 cm, so the floor sits a little into the snow. Only tents with a traced gap are touched.
import csv, unreal

CSV = "F:/UE_project/YggdrasilsPeak/Saved/RouteCheck/20261114_023557/objects.csv"
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
acts = {a.get_actor_label(): a for a in eas.get_all_level_actors()}
done = set()
for r in csv.DictReader(open(CSV, encoding="utf-8")):
    if r["kind"] != "tent" or r["floating"] not in ("1", "True", "true") or r["method"] != "trace":
        continue
    name = r["actor"]
    if name in done:
        continue
    a = acts.get(name)
    gap = float(r["gap_cm"])
    if not a or gap <= 10.0 or gap > 300.0:
        print("skip", name, gap); continue
    loc = a.get_actor_location()
    a.set_actor_location(unreal.Vector(loc.x, loc.y, loc.z - gap - 3.0), False, False)
    done.add(name)
    print("lowered %-26s by %.1f cm" % (name, gap + 3.0))
print("tents lowered:", len(done))
print("base_map saved", les.save_current_level())
