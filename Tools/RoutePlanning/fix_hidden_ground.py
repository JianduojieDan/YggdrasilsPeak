# Runs inside the editor. The old hidden "Landscape" still had collision, up to metres above the visible Landscape_Smooth in places:
# props seated on it, and players standing on it, floated. Removes that collision for good, then seats camp props and pickups on the real ground.
import unreal, math
ues = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
w = ues.get_editor_world()
acts = eas.get_all_level_actors()
old = [a for a in acts if a.get_actor_label() == "Landscape"][0]
smooth = [a for a in acts if a.get_actor_label() == "Landscape_Smooth"][0]
old.set_actor_enable_collision(False)
n = 0
for c in old.get_components_by_class(unreal.PrimitiveComponent):
    c.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
    n += 1
print("old landscape components without collision", n, "actor collision", old.get_actor_enable_collision())
def ground(x, y, z, ig):
    h = unreal.SystemLibrary.line_trace_single(w, unreal.Vector(x, y, z + 30000), unreal.Vector(x, y, z - 30000), unreal.TraceTypeQuery.ECC_VISIBILITY, False, ig, unreal.DrawDebugTrace.NONE, True)
    if not h: return None
    t = h.to_tuple()
    return t[4].z, (t[9].get_actor_label() if t[9] else None)
lms = {a.get_actor_label(): a for a in acts if a.get_class().get_name() == "RouteLandmark"}
others = [a for a in acts if a.actor_has_tag("CampProp") or a.get_class().get_name() in ("OxygenPickup", "SupplyPickup")]
others_ignore = others + list(lms.values())
moved = {}
biggest = 0
for a in others:
    l = a.get_actor_location()
    near = min(lms, key=lambda k: math.hypot(lms[k].get_actor_location().x - l.x, lms[k].get_actor_location().y - l.y))
    o, e = a.get_actor_bounds(False)
    g = ground(o.x, o.y, o.z, others_ignore)
    if not g or g[1] != "Landscape_Smooth": continue
    gap = (o.z - e.z) - g[0]
    if a.get_class().get_name() != "StaticMeshActor":
        continue_gap = gap > 60 or gap < -20
    else:
        continue_gap = gap > 0.0 or gap < -60   # sunk a little is right; any air under it is not
    if continue_gap:
        target = 10.0 if a.get_class().get_name() != "StaticMeshActor" else -12.0
        a.set_actor_location(l + unreal.Vector(0, 0, target - gap), False, True)
        moved[near] = moved.get(near, 0) + 1
        biggest = max(biggest, abs(gap))
print("props moved per camp", moved, "largest gap m", round(biggest / 100, 2))
for k, a in lms.items():
    l = a.get_actor_location()
    g = ground(l.x, l.y, l.z, others_ignore + [a])
    if g and g[1] == "Landscape_Smooth" and abs(l.z - g[0]) > 5:
        a.set_actor_location(unreal.Vector(l.x, l.y, g[0]), False, True)
ropes = 0
for a in acts:
    if a.get_class().get_name() == "FixedRope":
        pts = a.get_editor_property("rope_points")
        new = []
        ch = False
        for p in pts:
            g = ground(p.x, p.y, p.z, others_ignore + [a])
            if g and g[1] == "Landscape_Smooth" and p.z - g[0] > 200:
                new.append(unreal.Vector(p.x, p.y, g[0] + 90)); ch = True
            else:
                new.append(p)
        if ch:
            a.set_editor_property("rope_points", new); ropes += 1
print("ropes re-seated", ropes)
unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
print("saved")
