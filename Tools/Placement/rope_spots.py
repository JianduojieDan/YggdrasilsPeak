# Runs inside the editor: prints a few anchor positions per kind (for viewport_shot checks of the laid ropes).
import random
import unreal
rng = random.Random(5)
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
for a in sorted([a for a in eas.get_all_level_actors() if a.get_class().get_name() == 'FixedRope'], key=lambda a: a.get_actor_label()):
    for prop in ('picket_anchors', 'screw_anchors', 'piton_anchors'):
        trs = a.get_editor_property(prop)
        if trs:
            t = trs[rng.randrange(len(trs))].translation
            print('SPOT', a.get_actor_label(), prop.split('_')[0], round(t.x), round(t.y), round(t.z))
