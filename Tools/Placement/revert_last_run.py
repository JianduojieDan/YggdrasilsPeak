# Runs inside the editor: undoes the actor moves of the LAST logged ground_pass run (Saved/Placement/ground_moves.csv),
# then saves the level and drops those lines from the log.
import csv, ast
import unreal
P = 'F:/UE_project/YggdrasilsPeak/Saved/Placement/ground_moves.csv'
rows = list(csv.reader(open(P)))
last = rows[-1][0]
undo = {r[1]: r for r in rows if r[0] == last and r[3] == '-1'}
inst = [r for r in rows if r[0] == last and r[3] != '-1']
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
n = 0
for a in eas.get_all_level_actors():
    r = undo.get(a.get_actor_label())
    if r:
        a.set_actor_location_and_rotation(unreal.Vector(float(r[4]), float(r[5]), float(r[6])), unreal.Quat(*ast.literal_eval(r[7])).rotator(), False, True)
        n += 1
print('run', last, 'actors put back', n, 'instance moves in that run (not undone here):', len(inst))
with open(P, 'w', newline='') as f:
    csv.writer(f).writerows([r for r in rows if r[0] != last])
print('saved', unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level())
