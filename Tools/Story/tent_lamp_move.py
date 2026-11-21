import unreal, json, math
# Runs INSIDE the editor via Tools/RoutePlanning/uepy, after tent_lamp. 2026-11-21: the lamp straight over the bags lit the
# held compass from above (its dial faces up to the eye) so it still glowed in the dim tent. Hang it from a loop in the
# door half instead (a third of the way from the middle to the door, under the fabric there), aimed down and back across
# the bags: light from in front, the back of the tent in shade, the compass dial not facing the lamp.
ROOT = "F:/UE_project/YggdrasilsPeak"
W = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
if W.get_name() != "base_map":
    unreal.EditorLoadingAndSavingUtils.load_map("/Game/LevelPrototyping/base_map")
EAS = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
acts = EAS.get_all_level_actors()
P = json.load(open(ROOT + "/Content/Story/Chapter1_places.ygs"))
out = P["tent_out"]
lamp = [a for a in acts if "tentlight" in [str(t) for t in a.tags]][0]
tents = [a for a in acts if isinstance(a, unreal.StaticMeshActor) and a.static_mesh_component.static_mesh
         and a.static_mesh_component.static_mesh.get_name() == "tent_dome"]
L = lamp.get_actor_location()
tent = min(tents, key=lambda t: (t.get_actor_location() - L).length())
to, te = tent.get_actor_bounds(False)
cx, cy = to.x, to.y
dx, dy = out[0] - cx, out[1] - cy; d = math.hypot(dx, dy); dx, dy = dx / d, dy / d
half = abs(te.x * dx) + abs(te.y * dy)                      # the dome's half length along the door axis (bounds, projected)
f = 0.33
H = 2 * te.z; base = to.z - te.z
z = base + H * math.sqrt(max(0.0, 1 - f * f)) - 20.0          # under the fabric a third of the way to the door
at = unreal.Vector(cx + dx * half * f, cy + dy * half * f, z)
yaw_back = math.degrees(math.atan2(-dy, -dx))
lamp.set_actor_location(at, False, False)
lamp.set_actor_rotation(unreal.Rotator(0, -62, yaw_back), False)
print("LAMP at", round(at.x), round(at.y), round(z - base), "cm up; half", round(half), "H", round(H), "aim yaw", round(yaw_back), "fwd", lamp.get_actor_forward_vector())
unreal.EditorLevelLibrary.save_current_level()
print("done")
