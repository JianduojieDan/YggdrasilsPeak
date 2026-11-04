# Runs INSIDE the editor: Alfheim's prism bow, a huge additive disc up the Western Cwm from just past Camp 1. Its +Z faces the
# one spot it shows from (ARealmSky reads spot = location + up * BowDistance). The ground cuts the disc into an arc.
import unreal, math
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
acts = {a.get_actor_label(): a for a in eas.get_all_level_actors()}
c1 = acts["Landmark_4_Water"].get_actor_location(); c2 = acts["Landmark_5_Water"].get_actor_location()
dx, dy = c2.x - c1.x, c2.y - c1.y; n = math.hypot(dx, dy); dx /= n; dy /= n
spot = unreal.Vector(c1.x + dx * 15000.0, c1.y + dy * 15000.0, c1.z + 200.0)
DIST = 100000.0; DIAM = 180000.0
a = acts.get("Realm_AlfheimBow") or eas.spawn_actor_from_object(unreal.load_asset("/Engine/BasicShapes/Plane"), spot)
a.set_actor_label("Realm_AlfheimBow"); a.set_folder_path("Realms")
m = a.static_mesh_component
m.set_material(0, unreal.load_asset("/Game/Environment/Realms/M_PrismBow"))
m.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION); m.set_collision_profile_name("NoCollision")
m.set_editor_property("cast_shadow", False); m.set_editor_property("mobility", unreal.ComponentMobility.MOVABLE)
a.set_actor_location(unreal.Vector(spot.x + dx * DIST, spot.y + dy * DIST, spot.z), False, True)
# plane +Z must point back along the view: pitch the plane up 90 about the view's side axis
a.set_actor_rotation(unreal.MathLibrary.make_rot_from_z(unreal.Vector(-dx, -dy, 0.0)), False)
a.set_actor_scale3d(unreal.Vector(DIAM / 100.0, DIAM / 100.0, 1.0))
a.set_actor_hidden_in_game(True); m.set_editor_property("visible", False)   # RealmSky unhides it; until then nothing may show
print("spot", spot, "bow", a.get_actor_location(), "up", a.get_actor_up_vector())
unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
unreal.EditorAssetLibrary.save_directory("/Game/Environment/Realms")
