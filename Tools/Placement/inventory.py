# Runs inside the editor, through Tools/RoutePlanning/uepy
# (the first line must not contain "dot py": UE then treats the whole text as a file path). One-line-per-group list of everything placed in the open level: how many actors, single meshes and instances,
# grouped by outliner folder + class, so the ground audit knows what exists without looking at actors one by one.
import re, collections
import unreal

eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
groups = collections.OrderedDict()
for a in eas.get_all_level_actors():
    cls = a.get_class().get_name()
    folder = str(a.get_folder_path())
    label = re.sub(r'[_ ]?\d+$', '', a.get_actor_label())
    key = (folder, cls if cls != 'StaticMeshActor' else 'SMA:' + label.split('_')[0])
    g = groups.setdefault(key, {'actors': 0, 'meshes': 0, 'inst': 0, 'skinned': 0, 'names': set()})
    g['actors'] += 1
    for c in a.get_components_by_class(unreal.PrimitiveComponent):
        if isinstance(c, unreal.InstancedStaticMeshComponent):
            g['inst'] += c.get_instance_count()
            if c.static_mesh: g['names'].add(c.static_mesh.get_name())
        elif isinstance(c, unreal.StaticMeshComponent):
            g['meshes'] += 1
            if c.static_mesh: g['names'].add(c.static_mesh.get_name())
        elif c.get_class().get_name().startswith('InstancedSkinnedMesh'):
            try:
                g['skinned'] += c.get_instance_count()
            except Exception:
                g['skinned'] += 1
    if len(g['names']) > 6:
        g['names'] = set(sorted(g['names'])[:6])
rows = sorted(groups.items(), key=lambda kv: -(kv[1]['inst'] + kv[1]['meshes'] + kv[1]['skinned'] + kv[1]['actors']))
for (folder, cls), g in rows:
    print('%-34s %-28s act %5d mesh %6d inst %7d skin %6d | %s' % (folder[:34], cls[:28], g['actors'], g['meshes'], g['inst'], g['skinned'], ','.join(sorted(g['names']))[:90]))
