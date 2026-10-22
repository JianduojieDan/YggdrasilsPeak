# Runs inside the editor. Finds a walkable, game-friendly path between consecutive RouteLandmarks
# with A* over a traced height grid, and writes the result to OUT_JSON.
import heapq, json, math, time
import unreal

CELL = 2000.0          # cm
MAX_SLOPE = 50.0       # deg, surface slope that is never used
COMFY_SLOPE = 30.0     # deg, slope penalty grows fast above this

eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
marks = sorted([a for a in eas.get_all_level_actors() if a.get_class().get_name() == 'RouteLandmark'], key=lambda a: a.get_actor_label())
# Camp props, landmark pieces and the user's spawn cube are not terrain obstacles for planning.
ignore = list(marks) + [a for a in eas.get_all_level_actors() if unreal.Name('CampProp') in a.tags or a.get_actor_label() == 'Cube']

def sample(x, y):
    """Ground z, or None if the first thing hit is not the landscape (rock, prop)."""
    h = unreal.SystemLibrary.line_trace_single(world, unreal.Vector(x, y, 500000), unreal.Vector(x, y, -200000),
        unreal.TraceTypeQuery.ECC_VISIBILITY, False, ignore, unreal.DrawDebugTrace.NONE, True)
    t = h.to_tuple()
    if not t[0]:
        return None, False
    a = t[9]
    return t[5].z, bool(a and a.get_class().get_name().startswith('Landscape'))

def plan_leg(a, b):
    pa, pb = a.get_actor_location(), b.get_actor_location()
    dist = math.hypot(pb.x - pa.x, pb.y - pa.y)
    pad = max(150000.0, dist * 0.4)
    x0, x1 = min(pa.x, pb.x) - pad, max(pa.x, pb.x) + pad
    y0, y1 = min(pa.y, pb.y) - pad, max(pa.y, pb.y) + pad
    nx, ny = int((x1 - x0) / CELL) + 1, int((y1 - y0) / CELL) + 1
    H = [[0.0] * nx for _ in range(ny)]
    land = [[False] * nx for _ in range(ny)]
    for j in range(ny):
        y = y0 + j * CELL
        for i in range(nx):
            z, ok = sample(x0 + i * CELL, y)
            H[j][i] = z if z is not None else -1e9
            land[j][i] = ok
    # surface slope from central differences
    S = [[90.0] * nx for _ in range(ny)]
    for j in range(1, ny - 1):
        for i in range(1, nx - 1):
            if not land[j][i]:
                continue
            dzx = (H[j][i + 1] - H[j][i - 1]) / (2 * CELL)
            dzy = (H[j + 1][i] - H[j - 1][i]) / (2 * CELL)
            S[j][i] = math.degrees(math.atan(math.hypot(dzx, dzy)))
    def cell(p):
        return (min(max(int(round((p.x - x0) / CELL)), 1), nx - 2), min(max(int(round((p.y - y0) / CELL)), 1), ny - 2))
    start, goal = cell(pa), cell(pb)
    # Camps may sit on steep ground: let the first/last 60 m be walkable regardless.
    def near_end(i, j):
        return math.hypot(i - start[0], j - start[1]) * CELL < 6000 or math.hypot(i - goal[0], j - goal[1]) * CELL < 6000
    def step_cost(i, j, ni, nj):
        if not (0 < ni < nx - 1 and 0 < nj < ny - 1):
            return None
        s = S[nj][ni]
        end = near_end(ni, nj)
        if (s > MAX_SLOPE or not land[nj][ni]) and not end:
            return None
        d = CELL * math.hypot(ni - i, nj - j)
        grade = abs(H[nj][ni] - H[j][i]) / d
        slope_pen = 0.0 if end else (max(0.0, s - 12.0) / (COMFY_SLOPE - 12.0)) ** 3
        return d * (1.0 + 6.0 * grade * grade + slope_pen)
    nbrs = [(1, 0), (-1, 0), (0, 1), (0, -1), (1, 1), (1, -1), (-1, 1), (-1, -1), (2, 1), (1, 2), (-2, 1), (-1, 2), (2, -1), (1, -2), (-2, -1), (-1, -2)]
    g = {start: 0.0}
    came = {}
    pq = [(0.0, start)]
    while pq:
        f, cur = heapq.heappop(pq)
        if cur == goal:
            break
        gc = g[cur]
        if f - CELL * math.hypot(cur[0] - goal[0], cur[1] - goal[1]) > gc + 1e-6:
            continue
        for di, dj in nbrs:
            ni, nj = cur[0] + di, cur[1] + dj
            c = step_cost(cur[0], cur[1], ni, nj)
            if c is None:
                continue
            ng = gc + c
            if ng < g.get((ni, nj), 1e30):
                g[(ni, nj)] = ng
                came[(ni, nj)] = cur
                heapq.heappush(pq, (ng + CELL * math.hypot(ni - goal[0], nj - goal[1]), (ni, nj)))
    if goal not in came:
        return None, (nx, ny)
    path = [goal]
    while path[-1] != start:
        path.append(came[path[-1]])
    path.reverse()
    pts = [(x0 + i * CELL, y0 + j * CELL, H[j][i], S[j][i]) for i, j in path]
    return pts, (nx, ny)

result = []
t0 = time.time()
for k in range(len(marks) - 1):
    if LEGS and k not in LEGS:
        continue
    pts, dims = plan_leg(marks[k], marks[k + 1])
    name = '%s -> %s' % (marks[k].get_editor_property('landmark_name'), marks[k + 1].get_editor_property('landmark_name'))
    if pts is None:
        print('NO PATH', name, dims)
        continue
    length = sum(math.dist(pts[i][:3], pts[i + 1][:3]) for i in range(len(pts) - 1)) / 100.0
    print('%s: %d pts, %.0f m walked, max slope %.0f deg, grid %s, %.0fs' % (name, len(pts), length, max(p[3] for p in pts[3:-3] or [0]), dims, time.time() - t0))
    result.append({'leg': k, 'name': name, 'pts': pts})
json.dump(result, open(OUT_JSON, 'w'))
