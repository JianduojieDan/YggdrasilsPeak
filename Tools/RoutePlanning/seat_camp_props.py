# Runs inside the editor. Seats camp props and pickups by their real mesh vertices, not bounding boxes (a box can be far bigger than the shape).
# Stones sit a fifth into the ground, everything else touches it. Meshes without a simple collision box get one; props block the player.
import unreal, statistics as st
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
w = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
acts = eas.get_all_level_actors()
props = [a for a in acts if (a.actor_has_tag("CampProp") and a.get_class().get_name() == "StaticMeshActor") or a.get_class().get_name() in ("OxygenPickup", "SupplyPickup")]
SKIP = ("flag", "cord", "pole", "banner", "altartop")
cache = {}
def verts(mesh):
    p = mesh.get_path_name()
    if p in cache:
        return cache[p]
    out = []
    lib = unreal.EditorStaticMeshLibrary
    n = mesh.get_num_sections(0)
    for s in range(n):
        r = unreal.ProceduralMeshLibrary.get_section_from_static_mesh(mesh, 0, s)
        vs = list(r[0])
        step = max(1, len(vs) // 1500)
        out += list(vs[::step])
    cache[p] = out
    return out
def ground(x, y, z):
    h = unreal.SystemLibrary.line_trace_single(w, unreal.Vector(x, y, z + 20000), unreal.Vector(x, y, z - 30000), unreal.TraceTypeQuery.ECC_VISIBILITY, True, props, unreal.DrawDebugTrace.NONE, True)
    if not h: return None
    t = h.to_tuple()
    return t[4].z if t[9] and t[9].get_actor_label() == "Landscape_Smooth" else None
moved = {}
boxed = set()
report = []
for a in props:
    lab = a.get_actor_label()
    low = lab.lower()
    comp = a.static_mesh_component if a.get_class().get_name() == "StaticMeshActor" else a.get_components_by_class(unreal.StaticMeshComponent)[0]
    mesh = comp.static_mesh
    if not mesh:
        continue
    if not any(k in low for k in ("rope",)) and unreal.EditorStaticMeshLibrary.get_simple_collision_count(mesh) == 0 and mesh.get_path_name() not in boxed:
        unreal.EditorStaticMeshLibrary.add_simple_collisions(mesh, unreal.ScriptingCollisionShapeType.BOX)
        unreal.EditorAssetLibrary.save_loaded_asset(mesh, False)
        boxed.add(mesh.get_path_name())
    if a.get_class().get_name() == "StaticMeshActor" and not any(k in low for k in SKIP) and "rope" not in low:
        comp.set_collision_profile_name("BlockAll")
    if any(k in low for k in SKIP):
        continue
    xf = comp.get_world_transform()
    pts = [xf.transform_location(v) for v in verts(mesh)]
    if not pts:
        continue
    pts.sort(key=lambda p: p.z)
    height = pts[-1].z - pts[0].z
    gaps = []
    for p in pts[:: max(1, len(pts) // 60)]:
        g = ground(p.x, p.y, p.z)
        if g is not None:
            gaps.append(p.z - g)
    if not gaps:
        continue
    gaps.sort()
    gap = gaps[len(gaps) // 10]       # the lowest tenth of the shape, ignoring stray vertices
    target = -0.2 * height if "stone" in low or "altar" in low else (0.0 if "Pickup" in a.get_class().get_name() else -5.0)
    d = target - gap
    if abs(d) > 4.0:
        a.set_actor_location(a.get_actor_location() + unreal.Vector(0, 0, d), False, True)
        kind = low.split("_")[-1].rstrip("0123456789") or low
        moved[kind] = moved.get(kind, 0) + 1
        report.append((round(abs(d) / 100, 2), lab, round(d / 100, 2)))
report.sort(reverse=True)
print("moved by kind", moved)
print("largest", report[:14])
print("collision boxes added to", len(boxed), [p.split(".")[-1] for p in boxed])
unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
print("saved")

