# Runs INSIDE the editor: Asgard's Skygate pillar over Camp 2 (Landmark_5): a tall additive cylinder from ~3 km above
# down to the tents. Hidden by default (shown with the Asgard realm). Actor "Realm_AsgardPillar".
import unreal
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
acts = {a.get_actor_label(): a for a in eas.get_all_level_actors()}
camp = acts["Landmark_5"].get_actor_location()
H = 300000.0; R = 11000.0                    # 3 km tall, ~220 m wide soft beam
a = acts.get("Realm_AsgardPillar") or eas.spawn_actor_from_object(unreal.load_asset("/Engine/BasicShapes/Cylinder"), camp)
a.set_actor_label("Realm_AsgardPillar"); a.set_folder_path("Realms")
c = a.static_mesh_component
c.set_material(0, unreal.load_asset("/Game/Environment/Realms/M_LightShaft"))
c.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION); c.set_collision_profile_name("NoCollision")
c.set_editor_property("cast_shadow", False)
c.set_editor_property("mobility", unreal.ComponentMobility.MOVABLE)
# basic cylinder: 100 cm wide/tall, pivot at the centre; UV v runs bottom->top, flip by rotating 180 on X
a.set_actor_location(unreal.Vector(camp.x, camp.y, camp.z + H / 2.0), False, True)
a.set_actor_rotation(unreal.Rotator(roll=180.0, pitch=0.0, yaw=0.0), False)
a.set_actor_scale3d(unreal.Vector(R / 50.0, R / 50.0, H / 100.0))
a.set_actor_hidden_in_game(bool(globals().get("HIDE", True))); c.set_editor_property("visible", not globals().get("HIDE", True))
print("pillar at", camp)
