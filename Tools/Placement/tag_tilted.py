# Runs inside the editor (one-off, 2026-11-13): tags the actors the first ground_pass run tilted, so later runs never
# tilt them again.
import csv
import unreal
rows = list(csv.reader(open('F:/UE_project/YggdrasilsPeak/Saved/Placement/ground_moves.csv')))
tilted = {r[1] for r in rows if r[3] == '-1' and r[7] != r[9]}
n = 0
for a in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors():
    if a.get_actor_label() in tilted and 'GroundTilted' not in [str(t) for t in a.tags]:
        a.tags = list(a.tags) + [unreal.Name('GroundTilted')]
        n += 1
print('tagged', n, 'of', len(tilted))
print('saved', unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level())
