# Runs INSIDE the editor (uepy.py). The Western Cwm rumble area (AMountainRumble) over route leg 4 (Camp 1 -> Camp 2)
# with 250 m to spare round it. Re-running replaces it (tag Rumble).
import unreal, json
R = json.load(open("F:/UE_project/YggdrasilsPeak/Tools/RoutePlanning/route.json"))
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
for a in eas.get_all_level_actors():
    if a.actor_has_tag("Rumble"):
        a.destroy_actor()
pts = R[4]["pts"]
xs, ys, zs = [p[0] for p in pts], [p[1] for p in pts], [p[2] for p in pts]
c = unreal.Vector((min(xs) + max(xs)) / 2, (min(ys) + max(ys)) / 2, (min(zs) + max(zs)) / 2)
ext = unreal.Vector((max(xs) - min(xs)) / 2 + 25000, (max(ys) - min(ys)) / 2 + 25000, (max(zs) - min(zs)) / 2 + 20000)
a = eas.spawn_actor_from_class(unreal.load_class(None, "/Script/YggdrasilsPeak.MountainRumble"), c)
a.get_editor_property("area").set_box_extent(ext, False)
a.set_actor_label("Rumble_WesternCwm"); a.set_folder_path("Route/Hazards"); a.tags = ["Rumble"]
print("rumble area", c, ext)
