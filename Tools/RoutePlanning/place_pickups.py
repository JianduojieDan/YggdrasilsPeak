# Runs inside the editor. Swaps the decorative oxygen bottles at the camps for AOxygenPickup actors in the same spots.
import unreal

eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
pickup_cls = unreal.load_class(None, '/Script/YggdrasilsPeak.OxygenPickup')
n = 0
for a in list(eas.get_all_level_actors()):
    if unreal.Name('CampProp') not in a.tags or '_O2_' not in a.get_actor_label() or a.get_class() == pickup_cls:
        continue
    p = eas.spawn_actor_from_class(pickup_cls, a.get_actor_location(), a.get_actor_rotation())
    p.set_actor_scale3d(a.get_actor_scale3d())
    p.set_actor_label(a.get_actor_label().replace('_O2_', '_O2Pickup_'))
    p.set_folder_path(a.get_folder_path())
    p.tags = [unreal.Name('CampProp')]
    eas.destroy_actor(a)
    n += 1
print('pickups placed', n)
