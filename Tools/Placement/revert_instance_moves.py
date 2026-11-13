# Runs inside the editor (Tools/RoutePlanning/uepy): puts INSTANCES (rocks, stones, ice) moved by one ground_pass run back
# where they were. revert_last_run only undoes actor moves and drops the run's instance lines from the log, so this
# reads a copy of the log made before that. 2026-11-14 03:11: a ground pass lowered 83,893 instances by a median ~5 m
# although the in-game route check found ~80 floating rocks - a wrong measurement, so the whole run is undone.
#   RUN = '2026-11-14 03:11', LOG = 'F:/.../ground_moves_backup_0311.csv' (set as globals before exec, or edit below)
import csv
import unreal

RUN = globals().get('RUN', '2026-11-14 03:11')
LOG = globals().get('LOG', 'F:/UE_project/YggdrasilsPeak/Saved/Placement/ground_moves_backup_0311.csv')
old = {}   # (actor label, component name) -> {instance index: (x, y, z)}
for r in csv.reader(open(LOG)):
    if r[0] == RUN and r[3] != '-1':
        old.setdefault((r[1], r[2]), {})[int(r[3])] = (float(r[4]), float(r[5]), float(r[6]))
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
total = 0
for a in eas.get_all_level_actors():
    lab = a.get_actor_label()
    for c in a.get_components_by_class(unreal.InstancedStaticMeshComponent):
        want = old.get((lab, c.get_name()))
        if not want:
            continue
        trs = [c.get_instance_transform(i, True) for i in range(c.get_instance_count())]
        n = 0
        for i, p in want.items():
            if i < len(trs):
                trs[i].translation = unreal.Vector(*p); n += 1
        c.batch_update_instances_transforms(0, trs, True, True, True)
        total += n
        print('%-28s %-28s put back %d' % (lab, c.get_name(), n))
print('instances put back:', total, 'of', sum(len(v) for v in old.values()))
print('saved', unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level())
