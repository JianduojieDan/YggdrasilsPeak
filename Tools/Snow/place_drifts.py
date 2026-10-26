"""Bake wind-blown snow drifts behind every PCG rock and camp tent into one ASnowDriftField (run inside the editor
with base_map open; re-run after the rocks are regenerated). Drifts go on the downwind side of Ultra Dynamic
Weather's 'Wind Direction' (the yaw the wind blows towards), so they match the spindrift in game."""
import math
import unreal

world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
ess = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
actors = ess.get_all_level_actors()

udw = [a for a in actors if a.get_class().get_name() == "Ultra_Dynamic_Weather_C"]
wind_yaw = udw[0].get_editor_property("Wind Direction") if udw else 15.0
to = unreal.Vector(math.cos(math.radians(wind_yaw)), math.sin(math.radians(wind_yaw)), 0.0)
side = unreal.Vector(-to.y, to.x, 0.0)
print("wind blows towards yaw", wind_yaw)

MAX_SLOPE_DEG = 30.0     # no drifts on steep ground (rock faces, couloirs)
MIN_RADIUS = 60.0        # cm; pebbles get none

skip = set()
for a in actors:
    if a.get_class().get_name() == "SnowDriftField":
        skip.add(a)
field = next(iter(skip), None)
if field is None:
    cls = unreal.load_class(None, "/Script/YggdrasilsPeak.SnowDriftField")
    field = ess.spawn_actor_from_class(cls, unreal.Vector(0, 0, 0))
    field.set_actor_label("SnowDrifts")
    field.set_folder_path("Environment")
hism = field.get_component_by_class(unreal.HierarchicalInstancedStaticMeshComponent)
hism.clear_instances()

pile = hism.static_mesh
pb = pile.get_bounding_box()
p_min, p_max = pb.min, pb.max
pile_len, pile_wid, pile_h = p_max.x - p_min.x, p_max.y - p_min.y, p_max.z - p_min.z
pile_mid = unreal.Vector((p_min.x + p_max.x) / 2, (p_min.y + p_max.y) / 2, p_min.z)


def ground(x, y, ignore):
    ign = list(ignore)
    for _ in range(8):
        h = unreal.SystemLibrary.line_trace_single(world, unreal.Vector(x, y, 2000000), unreal.Vector(x, y, -300000),
                                                    unreal.TraceTypeQuery.ECC_VISIBILITY, True, ign, unreal.DrawDebugTrace.NONE, True)
        if h is None:
            return None
        t = h.to_tuple()
        if not t[0]:
            return None
        if isinstance(t[9], unreal.LandscapeProxy):
            return t[4].z
        ign.append(t[9])
    return None


def drift(cx, cy, length, width, height, ignore):
    """Transform for one drift centred at (cx, cy), stretched along the wind and tilted to the ground; None on steep ground."""
    hl, hw = length / 2, width / 2
    pts = {k: (cx + to.x * a + side.x * b, cy + to.y * a + side.y * b) for k, (a, b) in
           {"c": (0, 0), "f": (hl, 0), "b": (-hl, 0), "l": (0, -hw), "r": (0, hw)}.items()}
    z = {k: ground(x, y, ignore) for k, (x, y) in pts.items()}
    if None in z.values():
        return None
    along = unreal.Vector(length, 0, z["f"] - z["b"])
    across = unreal.Vector(0, width, z["r"] - z["l"])
    slope = math.degrees(math.atan(max(abs(along.z) / length, abs(across.z) / width)))
    if slope > MAX_SLOPE_DEG:
        return None
    # follow the ground: X along the wind, Y across it, both lying in the sampled slope
    rot = unreal.MathLibrary.make_rot_from_xy(to * length + unreal.Vector(0, 0, z["f"] - z["b"]),
                                             side * width + unreal.Vector(0, 0, z["r"] - z["l"]))
    scale = unreal.Vector(length / pile_len, width / pile_wid, height / pile_h)
    # sink the mound a third of its height so it rises out of the snow instead of sitting on it
    base = unreal.Vector(cx, cy, z["c"] - height * 0.33)
    offset = unreal.MathLibrary.greater_greater_vector_rotator(unreal.Vector(pile_mid.x * scale.x, pile_mid.y * scale.y, pile_mid.z * scale.z), rot)
    return unreal.Transform(base - offset, rot, scale)


xfs = []
counts = {"rocks": 0, "rocks_skipped": 0, "tents": 0}

pcg = [a for a in actors if a.get_actor_label() == "PCG_Rocks"]
for comp in (pcg[0].get_components_by_class(unreal.InstancedStaticMeshComponent) if pcg else []):
    ext = comp.static_mesh.get_bounding_box()
    ex, ey = (ext.max.x - ext.min.x) / 2, (ext.max.y - ext.min.y) / 2
    for i in range(comp.get_instance_count()):
        xf = comp.get_instance_transform(i, True)
        r = max(ex, ey) * max(xf.scale3d.x, xf.scale3d.y)
        if r < MIN_RADIUS:
            continue
        r = min(r, 1500.0)
        length = min(max(2.2 * r, 150.0), 2500.0)
        width = min(max(1.3 * r, 120.0), 1400.0)
        height = min(max(0.35 * r, 30.0), 250.0)
        c = xf.translation + to * (0.6 * r + 0.4 * length)
        t = drift(c.x, c.y, length, width, height, pcg)
        if t is None:
            counts["rocks_skipped"] += 1
            continue
        xfs.append(t)
        counts["rocks"] += 1

for a in actors:
    if not (isinstance(a, unreal.StaticMeshActor) and a.actor_has_tag("CampProp")):
        continue
    name = a.static_mesh_component.static_mesh.get_name()
    if name not in ("modern_camping_tent__GLTF_", "camping_tent", "Camp_props"):
        continue
    o, e = a.get_actor_bounds(False)
    r = max(e.x, e.y)
    length, width, height = 1.6 * r, 1.1 * r, min(0.22 * e.z * 2, 70.0)
    c = o + to * (0.8 * r + 0.3 * length)
    t = drift(c.x, c.y, length, width, height, [a])
    if t is not None:
        xfs.append(t)
        counts["tents"] += 1

hism.add_instances(xfs, False, True)
print(counts, "instances", hism.get_instance_count())
