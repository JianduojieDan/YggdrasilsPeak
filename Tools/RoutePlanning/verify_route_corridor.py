# Runs INSIDE the editor (uepy.py). Independent check of the generated content along the climbing line (the thesis'
# "is the route still passable" test): at every fixed-rope point of the Icefall / Western Cwm legs a climber capsule
# (r 40 cm, half height 90 cm) is swept straight down from 2.2 m above the point onto whatever is there - landscape,
# serac, crevasse lip or ladder. A point FAILS when
#   - the capsule already overlaps something at the start (a serac or lip in the way: start penetrating), or
#   - it finds no ground (falls through, e.g. into a crevasse slot), or
#   - the ground is steeper than the rope walking limit (58 deg; ExpeditionComponent.RopeWalkableFloorAngle).
# Also reports how many points are steeper than the unroped limit (44.7 deg) and the steepest one.
# Writes Saved/ScatterReports/route_corridor_check.json.
import unreal, json, math
w = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
actors = eas.get_all_level_actors()
ropes = {a.get_actor_label(): a for a in actors if a.get_class().get_name() == "FixedRope"}
ROPE_LIMIT, WALK_LIMIT = 58.0, 44.7
R, HH = 40.0, 90.0
out = {"legs": {}, "failed": []}
for leg in (2, 3, 4):
    pts = ropes[f"FixedRope_{leg}"].get_editor_property("rope_points")
    failed = steep = 0
    steepest = 0.0
    for p in pts:
        start = unreal.Vector(p.x, p.y, p.z + 220.0 + HH)
        end = unreal.Vector(p.x, p.y, p.z - 300.0)
        hit = unreal.SystemLibrary.capsule_trace_single(w, start, end, R, HH, unreal.TraceTypeQuery.ECC_VISIBILITY, False, [],
                                                        unreal.DrawDebugTrace.NONE, True)
        t = hit.to_tuple() if hit else (False, False) + (None,) * 8
        why = None
        if t[1]:
            why = "blocked"
        elif not t[0]:
            why = "no ground"
        else:
            ang = math.degrees(math.acos(max(-1.0, min(1.0, t[7].z))))
            steepest = max(steepest, ang)
            steep += ang > WALK_LIMIT
            if ang > ROPE_LIMIT:
                why = f"too steep {ang:.0f} deg"
        if why:
            failed += 1
            out["failed"].append({"leg": leg, "at": [round(p.x), round(p.y), round(p.z)], "why": why,
                                  "by": t[9].get_actor_label() + "." + t[10].get_name() if t[9] else None})
    out["legs"][leg] = {"points": len(pts), "failed": failed, "steeper_than_unroped_limit": steep, "steepest_deg": round(steepest, 1)}
    print("leg", leg, out["legs"][leg])
for b in out["failed"][:12]:
    print(" ", b)
json.dump(out, open("F:/UE_project/YggdrasilsPeak/Saved/ScatterReports/route_corridor_check.json", "w"), indent=1)
