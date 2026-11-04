# Runs INSIDE the editor (Tools/RoutePlanning/uepy.py). Places the nine realm planets as sky objects ~5000 km away,
# seen from VIEW (default: the summit). Two layouts:
#   LAYOUT = "summit"  all nine framed together (the World Tree payoff), each ~5 deg across
#   LAYOUT = "single"  only REALM (name) visible, large (~24 deg across) at AZ / EL degrees - the first-sighting study
# Hel's trail is a long thin strip behind Hel. Actors live in outliner folder "Realms", tagged RealmPlanet.
import unreal, math
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
g = globals()
LAYOUT = g.get("LAYOUT", "summit")
VIEW = g.get("VIEW", (1240508.0, 1412617.0, 382663.0))
D = g.get("DIST", 5.0e8)                       # 5000 km (far enough that walking never shifts them)
SPHERE = unreal.load_asset("/Game/Environment/Realms/SM_RealmSphere/StaticMeshes/SM_RealmSphere")
PLANE = unreal.load_asset("/Engine/BasicShapes/Plane")
NAMES = ["Midgard", "Niflheim", "Muspelheim", "Jotunheim", "Alfheim", "Vanaheim", "Svartalfheim", "Hel", "Asgard"]
# summit layout: azimuth (deg, 0 = +X east, 90 = +Y south... UE yaw), elevation, angular diameter
# summit: the nine framed in one wide view (looking north-west from the top, yaw -100), Midgard central and largest,
# the others around it the way the World Tree's branches will hold them
# sizes differ a lot: Jotunheim and Asgard are giants, Svartalfheim and Alfheim small, the rest in between
SUMMIT = {"Midgard": (-100, 24, 9), "Niflheim": (-132, 14, 7), "Muspelheim": (-70, 14, 6), "Jotunheim": (-150, 40, 18),
          "Alfheim": (-66, 40, 4), "Vanaheim": (-118, 50, 8), "Svartalfheim": (-96, 58, 3), "Hel": (-148, 70, 5),
          "Asgard": (-40, 56, 15)}
# single sighting (first study): the same proportions, scaled up
SINGLE_SIZE = {"Midgard": 26, "Niflheim": 34, "Muspelheim": 24, "Jotunheim": 55, "Alfheim": 16, "Vanaheim": 30,
               "Svartalfheim": 11, "Hel": 20, "Asgard": 46}
def direction(az, el):
    a, e = math.radians(az), math.radians(el)
    return unreal.Vector(math.cos(e) * math.cos(a), math.cos(e) * math.sin(a), math.sin(e))
existing = {a.get_actor_label(): a for a in eas.get_all_level_actors() if a.get_actor_label().startswith("Realm_")}
def get(label, mesh):
    a = existing.get(label) or eas.spawn_actor_from_object(mesh, unreal.Vector(0, 0, 0))
    a.set_actor_label(label); a.set_folder_path("Realms")
    c = a.static_mesh_component
    c.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION); c.set_collision_profile_name("NoCollision")
    c.set_editor_property("cast_shadow", False)
    c.set_editor_property("affect_dynamic_indirect_lighting", False)
    c.set_editor_property("affect_distance_field_lighting", False)
    c.set_editor_property("mobility", unreal.ComponentMobility.MOVABLE)
    c.set_editor_property("bounds_scale", 1.0)
    c.set_editor_property("translucency_sort_priority", 100)   # draw over the sky's moon / sun sprites
    return a
V = unreal.Vector(*VIEW)
for name in NAMES:
    a = get(f"Realm_{name}", SPHERE)
    a.static_mesh_component.set_material(0, unreal.load_asset(f"/Game/Environment/Realms/MI_Realm_{name}"))
    if LAYOUT == "summit":
        az, el, size = SUMMIT[name]; show = True
    else:
        az, el = g.get("AZ", -95), g.get("EL", 22); size = g.get("SIZE") or SINGLE_SIZE[name]; show = (name == g.get("REALM", "Midgard"))
    radius = D * math.tan(math.radians(size / 2.0))
    a.set_actor_location(V + direction(az, el) * D, False, True)
    a.set_actor_scale3d(unreal.Vector(radius / 100.0, radius / 100.0, radius / 100.0))
    a.set_actor_hidden_in_game(not show); a.static_mesh_component.set_editor_property("visible", show)
    if name == "Hel":
        hel_loc, hel_r, hel_show = a.get_actor_location(), radius, show
# Hel's trail: a long inclined strip of light behind the planet (a broken river), clears the ridge before Hel rises
t = get("Realm_HelTrail", PLANE)
t.static_mesh_component.set_material(0, unreal.load_asset("/Game/Environment/Realms/MI_Realm_HelTrail"))
to_view = (V - hel_loc).normal()
t.set_actor_location(hel_loc + unreal.Vector(0, 0, -hel_r * 0.4) - to_view * hel_r * 1.2, False, True)
yaw = math.degrees(math.atan2(to_view.y, to_view.x))
t.set_actor_rotation(unreal.Rotator(roll=25.0, pitch=-90.0, yaw=yaw), False)    # plane (one-sided) faces the view, tilted
t.set_actor_scale3d(unreal.Vector(hel_r * 0.035 / 50.0, hel_r * 7.0 / 50.0, 1.0))
t.set_actor_hidden_in_game(not hel_show); t.static_mesh_component.set_editor_property("visible", hel_show)
print("realms placed:", LAYOUT)
