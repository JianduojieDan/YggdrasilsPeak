# Runs INSIDE the editor. Icefall seracs read as small saturated-blue "popsicles" standing in rows: bigger (scale 0.1-0.3 -> 0.16-0.52)
# and more varied blocks (a chaotic jumble, like the reference art), sunk a bit deeper; then re-generate with the same seed
# and verify the rules (route corridor stays open - the scatter rejects any block that would close it).
import unreal
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
s = [a for a in eas.get_all_level_actors() if a.get_actor_label() == "Scatter_IcefallSeracs"][0]
ms = s.get_editor_property("meshes")
for m in ms:
    print(m.get_editor_property("mesh").get_name(), m.get_editor_property("min_scale"), m.get_editor_property("max_scale"))
    m.set_editor_property("min_scale", 0.16); m.set_editor_property("max_scale", 0.52); m.set_editor_property("embed_fraction", 0.22)
    m.set_editor_property("max_lean_deg", 14.0); m.set_editor_property("shape_variance", 0.8)   # jumble, not rows of pillars
s.set_editor_property("meshes", ms)
print("target", s.get_editor_property("target_count"), "mingap", s.get_editor_property("min_gap_cm"), "corridor", s.get_editor_property("route_corridor_cm"))
s.call_method("Generate")
s.call_method("Verify")
print("accepted", s.get_editor_property("last_accepted"))
