# Runs inside the editor: puts the actors named in LABELS back where Saved/Placement/ground_moves.csv says they were
# (the first logged position of each), then saves the level.
import csv, ast
import unreal
LABELS = set(globals().get('LABELS', []))
first = {}
for r in csv.reader(open('F:/UE_project/YggdrasilsPeak/Saved/Placement/ground_moves.csv')):
    if r[3] == '-1' and r[1] in LABELS and r[1] not in first:
        first[r[1]] = (float(r[4]), float(r[5]), float(r[6]), ast.literal_eval(r[7]))
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
n = 0
for a in eas.get_all_level_actors():
    lab = a.get_actor_label()
    if lab in first:
        x, y, z, q = first[lab]
        a.set_actor_location_and_rotation(unreal.Vector(x, y, z), unreal.Quat(*q).rotator(), False, True)
        n += 1
print('reverted', n, 'of', len(first))
print('saved', unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level())
