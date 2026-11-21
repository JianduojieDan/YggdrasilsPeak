import unreal
# Runs INSIDE the editor via Tools/RoutePlanning/uepy, after fire_ring_pca. 2026-11-21: the stone_fireplace asset's
# bounds box is inflated (101 cm tall around stones that span 14 cm), so placing it by bounds left it floating. Size and
# seat it by its real vertices: 115 cm across, centred on the fire light, the lowest 2% of the stones 2 cm into the snow.
W = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
EAS = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
ML = unreal.MathLibrary
acts = EAS.get_all_level_actors()
lights = [a for a in acts if a.get_actor_label() == "CampFire_Light"]
fire = [a for a in acts if "CampFire" in [str(t) for t in a.tags]] + [a for a in acts if a.get_actor_label().endswith("FireRing")]
sm = unreal.load_asset("/Game/Fab/Stone_fireplace/stone_fireplace/StaticMeshes/stone_fireplace")
desc = sm.get_static_mesh_description(0)
N = desc.get_vertex_count(); step = max(1, N // 20000)
V = [desc.get_vertex_position(unreal.VertexID(i)) for i in range(0, N, step)]


def stats(T):
    w = [ML.transform_location(T, v) for v in V]
    xs = sorted(p.x for p in w); ys = sorted(p.y for p in w); zs = sorted(p.z for p in w); L = len(w)
    return xs[0], xs[-1], ys[0], ys[-1], zs[int(0.02 * (L - 1))], zs[-1]


for r in [a for a in acts if a.get_actor_label() == "CampFire_Ring"]:
    Lt = min(lights, key=lambda l: (l.get_actor_location() - r.get_actor_location()).length()).get_actor_location()
    hit = unreal.SystemLibrary.line_trace_single(W, unreal.Vector(Lt.x, Lt.y, Lt.z + 600), unreal.Vector(Lt.x, Lt.y, Lt.z - 900),
                                                 unreal.TraceTypeQuery.TRACE_TYPE_QUERY1, False, fire, unreal.DrawDebugTrace.NONE, True)
    gz = [x for x in hit.to_tuple() if isinstance(x, unreal.Vector)][1].z
    x0, x1, y0, y1, z2, zt = stats(r.get_actor_transform())
    k = 115.0 / max(x1 - x0, y1 - y0)
    r.set_actor_scale3d(r.get_actor_scale3d() * k)
    x0, x1, y0, y1, z2, zt = stats(r.get_actor_transform())
    p = r.get_actor_location()
    r.set_actor_location(unreal.Vector(p.x + Lt.x - (x0 + x1) / 2, p.y + Lt.y - (y0 + y1) / 2, p.z + (gz - 2.0) - z2), False, False)
    x0, x1, y0, y1, z2, zt = stats(r.get_actor_transform())
    print("RING across", round(x1 - x0), round(y1 - y0), "low2%-ground", round(z2 - gz, 1), "top-ground", round(zt - gz, 1),
          "centre off", round((x0 + x1) / 2 - Lt.x, 1), round((y0 + y1) / 2 - Lt.y, 1))
unreal.EditorLevelLibrary.save_current_level()
print("done")
