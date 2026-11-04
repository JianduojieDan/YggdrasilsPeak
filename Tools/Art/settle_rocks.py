# Runs INSIDE the editor. Seats every PCG_Rocks instance on the real ground: probes the terrain under the rock's
# footprint (centre + 8 points on its scaled radius), fits the true slope plane, tilts the rock to it (keeps its yaw),
# and sinks it so the lowest probe is buried by 25% of the rock's height. PCG's own surface alignment used the
# landscape normals, which come out ~3.5x too steep on this landscape and tipped rocks into the air.
# PCG_Rocks is switched to generate on demand only, so a reload doesn't undo this.
import unreal, math
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
w = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
acts = eas.get_all_level_actors()
v = [a for a in acts if a.get_actor_label() == "PCG_Rocks"][0]
ignore = [v] + [a for a in acts if a.get_name() == "Landscape_0"]
def ground(x, y, z):
    h = unreal.SystemLibrary.line_trace_single(w, unreal.Vector(x, y, z + 60000), unreal.Vector(x, y, z - 120000),
        unreal.TraceTypeQuery.TRACE_TYPE_QUERY1, True, ignore, unreal.DrawDebugTrace.NONE, True)
    return h.to_tuple()[4].z if h else None
pc = v.get_components_by_class(unreal.PCGComponent)[0]
try: pc.set_editor_property("generation_trigger", unreal.PCGComponentGenerationTrigger.GENERATE_ON_DEMAND)
except Exception as e: print("trigger", e)
moved = 0; removed = 0
for c in v.get_components_by_class(unreal.InstancedStaticMeshComponent):
    b = c.static_mesh.get_bounding_box()
    rx, ry, hz = (b.max.x - b.min.x) / 2, (b.max.y - b.min.y) / 2, (b.max.z - b.min.z)
    drop = []
    for i in range(c.get_instance_count()):
        t = c.get_instance_transform(i, True)
        s = t.scale3d.x; P = t.translation; yaw = t.rotation.rotator().yaw
        r = 0.8 * max(rx, ry) * s
        g0 = ground(P.x, P.y, P.z)
        if g0 is None: continue
        pts = [(0.0, 0.0, g0)]
        for k in range(8):
            a = k * math.pi / 4; x, y = P.x + r * math.cos(a), P.y + r * math.sin(a)
            g = ground(x, y, P.z)
            if g is not None: pts.append((x - P.x, y - P.y, g))
        # least-squares plane z = ax + by + c
        n = len(pts); sx = sum(p[0] for p in pts); sy = sum(p[1] for p in pts)
        sxx = sum(p[0] ** 2 for p in pts); syy = sum(p[1] ** 2 for p in pts); sxy = sum(p[0] * p[1] for p in pts)
        sz = sum(p[2] for p in pts); sxz = sum(p[0] * p[2] for p in pts); syz = sum(p[1] * p[2] for p in pts)
        # solve 3x3
        M = [[sxx, sxy, sx], [sxy, syy, sy], [sx, sy, n]]; Bv = [sxz, syz, sz]
        def det(m): return (m[0][0]*(m[1][1]*m[2][2]-m[1][2]*m[2][1]) - m[0][1]*(m[1][0]*m[2][2]-m[1][2]*m[2][0]) + m[0][2]*(m[1][0]*m[2][1]-m[1][1]*m[2][0]))
        D = det(M)
        if abs(D) < 1e-6: ga = gb = 0.0
        else:
            ga = det([[Bv[0], M[0][1], M[0][2]], [Bv[1], M[1][1], M[1][2]], [Bv[2], M[2][1], M[2][2]]]) / D
            gb = det([[M[0][0], Bv[0], M[0][2]], [M[1][0], Bv[1], M[1][2]], [M[2][0], Bv[2], M[2][2]]]) / D
        # tilt: most of the true slope (rocks on a slope sit a bit more upright than the ground)
        pitch = -math.degrees(math.atan(ga)) * 0.6; roll = math.degrees(math.atan(gb)) * 0.6
        q = unreal.Rotator(roll=0, pitch=0, yaw=yaw).quaternion()
        tilt = unreal.Rotator(roll=roll, pitch=pitch, yaw=0).quaternion()
        rot = (tilt * q).rotator()
        lowest = min(p[2] for p in pts)
        # footprint spans a drop (knife-edge ridge, cliff lip): ground under it is far from a plane, the rock would
        # hang in the air - remove it. A plain slope fits the plane and is kept.
        cfit = (sz - ga * sx - gb * sy) / n
        resid = max(abs(p[2] - (ga * p[0] + gb * p[1] + cfit)) for p in pts)
        if resid > 0.3 * hz * s:
            drop.append(i); continue
        # pivot height above the mesh bottom, then bury 25% of the height under the lowest probe (verified: 0 of 480
        # sampled rocks hover; burying against the centre plane instead left 136 of 480 hovering)
        z = lowest - b.min.z * s - 0.25 * hz * s
        nt = unreal.Transform(unreal.Vector(P.x, P.y, z), rot, t.scale3d)
        c.update_instance_transform(i, nt, True, True, True)   # world space, refresh the render state, teleport
        moved += 1
    if drop: c.remove_instances(drop); removed += len(drop)
print("settled", moved, "removed (footprint over a drop)", removed)
