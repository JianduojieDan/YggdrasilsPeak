# Runs inside the editor: does each route detail tile's collision mesh match its render mesh? Compares the source
# geometry (mesh descriptions) at the collision vertices (2 m grid, every one of them is also a 1 m render vertex).
# TILES = list of actor labels (default: all). Prints per tile the render-minus-collision height stats (cm).
import time, unreal
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
want = globals().pop('TILES', None)
t0 = time.time()
rows = []
def verts(sm):
    md = sm.get_static_mesh_description(0)
    n = md.get_vertex_count()
    out = {}
    for i in range(n):
        p = md.get_vertex_position(unreal.VertexID(i))
        out[(round(p.x / 50.0), round(p.y / 50.0))] = p.z
    return out
for a in eas.get_all_level_actors():
    lab = a.get_actor_label()
    if not lab.startswith('DetailTile') or (want and lab not in want):
        continue
    sm = a.static_mesh_component.static_mesh
    col = sm.get_editor_property('complex_collision_mesh')
    if not col:
        rows.append((lab, 'no separate collision mesh')); continue
    R = verts(sm); C = verts(col)
    d = sorted(R[k] - C[k] for k in C if k in R)
    if not d:
        rows.append((lab, 'no shared vertices (R %d, C %d)' % (len(R), len(C)))); continue
    n = len(d)
    rows.append((lab, 'shared %d  render-col cm: p1 %.1f p50 %.1f p99 %.1f  |>10cm| %.1f%%' % (n, d[int(n * .01)], d[n // 2], d[int(n * .99)], 100.0 * sum(1 for v in d if abs(v) > 10) / n)))
for r in rows:
    print(*r)
print('%.0f s' % (time.time() - t0))
