import unreal, random, math
# Runs INSIDE the editor via Tools/RoutePlanning/uepy. 2026-11-21: the Fab stone_fireplace scan has an arbitrary frame
# (bounds 1818 x 1179 x 1280 raw, pivot far off the stones), so neither a plain yaw nor the old ring's tilt stands it flat
# (121 cm tall at 120 cm wide -> floated over the fuel). Find its real "up" from the vertices: the flattest principal axis
# (PCA, smallest variance) is the ring's normal; the side with the flat scanned ground disc is the bottom. Then size the
# ring to 1.2 m, centre it on the fire light, sit it 3 cm into the snow, and re-seat the dung fuel and the cards on it.
# Run right after fire_setup2 (which respawns the parts; its smoke is now fainter - it read as a white light beam).
W = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
if W.get_name() != "base_map":
    unreal.EditorLoadingAndSavingUtils.load_map("/Game/LevelPrototyping/base_map")
    W = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
EAS = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
EAL = unreal.EditorAssetLibrary
ML = unreal.MathLibrary
random.seed(89)
sm = unreal.load_asset("/Game/Fab/Stone_fireplace/stone_fireplace/StaticMeshes/stone_fireplace")
desc = sm.get_static_mesh_description(0)
n = desc.get_vertex_count()
step = max(1, n // 30000)
P = []
for i in range(0, n, step):
    v = desc.get_vertex_position(unreal.VertexID(i))
    P.append((v.x, v.y, v.z))
m = [sum(p[k] for p in P) / len(P) for k in range(3)]
C = [[sum((p[a] - m[a]) * (p[b] - m[b]) for p in P) / len(P) for b in range(3)] for a in range(3)]


def jacobi(A):
    A = [r[:] for r in A]; V = [[1.0 if i == j else 0.0 for j in range(3)] for i in range(3)]
    for _ in range(60):
        p, q = max(((0, 1), (0, 2), (1, 2)), key=lambda t: abs(A[t[0]][t[1]]))
        if abs(A[p][q]) < 1e-9: break
        th = 0.5 * math.atan2(2 * A[p][q], A[q][q] - A[p][p]); c, s = math.cos(th), math.sin(th)
        for k in range(3):
            akp, akq = A[k][p], A[k][q]; A[k][p] = c * akp - s * akq; A[k][q] = s * akp + c * akq
        for k in range(3):
            apk, aqk = A[p][k], A[q][k]; A[p][k] = c * apk - s * aqk; A[q][k] = s * apk + c * aqk
        for k in range(3):
            vkp, vkq = V[k][p], V[k][q]; V[k][p] = c * vkp - s * vkq; V[k][q] = s * vkp + c * vkq
    return [A[i][i] for i in range(3)], [[V[r][i] for r in range(3)] for i in range(3)]


ev, vecs = jacobi(C)
order = sorted(range(3), key=lambda i: ev[i])
nrm = vecs[order[0]]
h = [sum((p[k] - m[k]) * nrm[k] for k in range(3)) for p in P]
lo, hi = min(h), max(h); span = hi - lo
near_lo = sum(1 for x in h if x < lo + 0.12 * span); near_hi = sum(1 for x in h if x > hi - 0.12 * span)
if near_hi > near_lo: nrm = [-x for x in nrm]                       # the dense flat side (scanned ground disc) is the bottom
print("PCA eig", [round(e) for e in sorted(ev)], "normal", [round(x, 3) for x in nrm], "lo/hi counts", near_lo, near_hi)
up = unreal.Vector(*nrm)
R0 = ML.make_rot_from_z(up)                                          # local Z -> mesh normal
q0 = R0.quaternion(); qinv = unreal.Quat(-q0.x, -q0.y, -q0.z, q0.w)   # unit quat inverse = conjugate

acts = EAS.get_all_level_actors()
lights = [a for a in acts if a.get_actor_label() == "CampFire_Light"]
for r in [a for a in acts if a.get_actor_label() == "CampFire_Ring"]:
    L = min(lights, key=lambda l: (l.get_actor_location() - r.get_actor_location()).length()).get_actor_location()
    qy = unreal.Rotator(0, 0, random.uniform(0, 360)).quaternion()
    q = ML.multiply_quat_quat(qy, qinv) if hasattr(ML, 'multiply_quat_quat') else qy * qinv                                                   # mesh normal -> world up, then a random yaw
    r.set_actor_rotation(q.rotator(), False)
    r.set_actor_scale3d(unreal.Vector(1, 1, 1))
    o, e = r.get_actor_bounds(False)
    k = 120.0 / max(2 * e.x, 2 * e.y, 1.0)
    r.set_actor_scale3d(unreal.Vector(k, k, k))
    # ground under the light (ignore all fire parts)
    fire = [a for a in acts if "CampFire" in [str(t) for t in a.tags]] + [a for a in acts if a.get_actor_label().endswith("FireRing")]
    hit = unreal.SystemLibrary.line_trace_single(W, unreal.Vector(L.x, L.y, L.z + 600), unreal.Vector(L.x, L.y, L.z - 900),
                                                 unreal.TraceTypeQuery.TRACE_TYPE_QUERY1, False, fire, unreal.DrawDebugTrace.NONE, True)
    gz = [x for x in hit.to_tuple() if isinstance(x, unreal.Vector)][1].z if hit else L.z - 45
    o, e = r.get_actor_bounds(False); p = r.get_actor_location()
    r.set_actor_location(unreal.Vector(p.x + L.x - o.x, p.y + L.y - o.y, p.z + (gz - 3.0) - (o.z - e.z)), False, False)
    o, e = r.get_actor_bounds(False)
    print("RING", round(2 * e.x), round(2 * e.y), "h", round(2 * e.z), "bottom-ground", round(o.z - e.z - gz, 1), "centre off", round(o.x - L.x, 1), round(o.y - L.y, 1))
    # fuel: a 45 cm heap of dung patties, bottom 2 cm into the snow, in the ring's middle
    for a in acts:
        if a.get_actor_label() == "CampFire_Fuel" and (a.get_actor_location() - L).length() < 300:
            a.set_actor_scale3d(unreal.Vector(1, 1, 1)); fo, fe = a.get_actor_bounds(False)
            s = 45.0 / max(2 * fe.x, 2 * fe.y, 1.0); a.set_actor_scale3d(unreal.Vector(s, s, s * 0.7))
            fo, fe = a.get_actor_bounds(False); ap = a.get_actor_location()
            a.set_actor_location(unreal.Vector(ap.x + L.x - fo.x, ap.y + L.y - fo.y, ap.z + (gz - 2.0) - (fo.z - fe.z)), False, False)
            fo, fe = a.get_actor_bounds(False); print("FUEL top over ground", round(fo.z + fe.z - gz, 1))
            fuel_top = fo.z + fe.z
        if a.get_actor_label().startswith("CampFire_Flame") and (a.get_actor_location() - L).length() < 300:
            ap = a.get_actor_location(); a.set_actor_location(unreal.Vector(ap.x, ap.y, gz + 4.0), False, False)
            sc = a.get_actor_scale3d(); a.set_actor_scale3d(unreal.Vector(sc.x * 1.25, 1, max(sc.z, 0.75)))
        if a.get_actor_label().startswith("CampFire_Smoke") and (a.get_actor_location() - L).length() < 300:
            ap = a.get_actor_location(); a.set_actor_location(unreal.Vector(ap.x, ap.y, gz + 55.0), False, False)
            a.set_actor_scale3d(unreal.Vector(1.7, 1, 3.0))
        if a.get_actor_label() == "CampFire_Light" and (a.get_actor_location() - L).length() < 300:
            ap = a.get_actor_location(); a.set_actor_location(unreal.Vector(ap.x, ap.y, gz + 55.0), False, False)

unreal.EditorLevelLibrary.save_current_level()
print("done")
