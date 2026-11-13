# Runs inside the editor: how fast are ground traces and instance reads from Python, and is numpy here?
import time, unreal
try:
    import numpy
    print('numpy', numpy.__version__)
except Exception as e:
    print('no numpy', e)
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
acts = eas.get_all_level_actors()
tiles = [a for a in acts if a.get_actor_label().startswith('DetailTile')]
land = [a for a in acts if a.get_class().get_name() == 'Landscape']
print('tiles', len(tiles), 'landscape', [a.get_actor_label() for a in land])
t = tiles[0].static_mesh_component
o, e = tiles[0].get_actor_bounds(False)
print('tile0', tiles[0].get_actor_label(), t.static_mesh.get_name(), 'centre', o, 'ext', e)
cols = [c for c in land[0].get_components_by_class(unreal.PrimitiveComponent)]
print('landscape comps', len(cols), sorted(set(c.get_class().get_name() for c in cols)))
t0 = time.time()
n = 0; hits = 0
for i in range(2000):
    x = o.x - e.x * 0.9 + (i % 50) * e.x * 1.8 / 50
    y = o.y - e.y * 0.9 + (i // 50) * e.y * 1.8 / 40
    r = t.line_trace_component(unreal.Vector(x, y, o.z + e.z + 1000), unreal.Vector(x, y, o.z - e.z - 1000), True, False, False)
    n += 1
    if r and r[0]: hits += 1
dt = time.time() - t0
print('line_trace_component: %d traces %.1f us each, hits %d, sample %s' % (n, dt / n * 1e6, hits, str(r)[:200]))
rocks = [a for a in acts if a.get_folder_path() and str(a.get_folder_path()) == 'Environment/DetailTiles/Route' and a.get_class().get_name() == 'Actor'][0]
isms = rocks.get_components_by_class(unreal.InstancedStaticMeshComponent)
print('rocks actor', rocks.get_actor_label(), [(c.get_name(), c.static_mesh.get_name(), c.get_instance_count()) for c in isms])
c = max(isms, key=lambda c: c.get_instance_count())
t0 = time.time()
for i in range(5000):
    tr = c.get_instance_transform(i, True)
dt = time.time() - t0
print('get_instance_transform %.1f us each' % (dt / 5000 * 1e6), tr)
w = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
t0 = time.time()
for i in range(2000):
    x = o.x - e.x * 0.9 + (i % 50) * e.x * 1.8 / 50
    y = o.y - e.y * 0.9 + (i // 50) * e.y * 1.8 / 40
    h = unreal.SystemLibrary.line_trace_single(w, unreal.Vector(x, y, o.z + e.z + 1000), unreal.Vector(x, y, o.z - e.z - 1000), unreal.TraceTypeQuery.TRACE_TYPE_QUERY1, True, [rocks], unreal.DrawDebugTrace.NONE, True)
dt = time.time() - t0
print('line_trace_single world %.1f us each' % (dt / 2000 * 1e6))
sm = c.static_mesh
t0 = time.time()
try:
    sec = unreal.ProceduralMeshLibrary.get_section_from_static_mesh(sm, 0, 0)
    print('section verts', len(sec[0]), 'in %.2f s' % (time.time() - t0))
except Exception as ex:
    print('section failed', ex)
print('mesh bounds', sm.get_bounding_box(), 'nsections', sm.get_num_sections(0))
