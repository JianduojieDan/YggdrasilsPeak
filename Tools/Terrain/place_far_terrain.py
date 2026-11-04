# Runs INSIDE the editor: places FarRing + FarBackground actors (no collision, cast shadows), hides the old DistantEverest.
import unreal
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
acts = {a.get_actor_label(): a for a in eas.get_all_level_actors()}
for n in ["FarRing", "FarBackground"]:
    a = acts.get(n) or eas.spawn_actor_from_object(unreal.load_asset(f"/Game/Environment/Distant/Far/{n}"), unreal.Vector(1196616.96, 1196616.96, 0))
    a.set_actor_label(n)
    a.set_actor_location_and_rotation(unreal.Vector(1196616.96, 1196616.96, 0), unreal.Rotator(0, 0, 0), False, True)
    a.set_actor_scale3d(unreal.Vector(1, 1, 1))
    a.set_folder_path("Distant")
    c = a.static_mesh_component
    c.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
    c.set_collision_profile_name("NoCollision")
    c.set_editor_property("cast_shadow", True)
    c.set_editor_property("affect_distance_field_lighting", False)
    c.set_editor_property("affect_dynamic_indirect_lighting", False)   # Lumen stays near; far faces lit by sky + sun
    c.set_editor_property("mobility", unreal.ComponentMobility.STATIC)
old = acts.get("DistantEverest")
if old:
    old.set_actor_hidden_in_game(True); old.set_is_temporarily_hidden_in_editor(True)
    old.static_mesh_component.set_editor_property("visible", False)
print("placed")
