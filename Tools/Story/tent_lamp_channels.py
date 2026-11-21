import unreal
# Runs INSIDE the editor via Tools/RoutePlanning/uepy, after tent_lamp_move. 2026-11-21: the held compass still glowed in
# the dim tent because the lamp lit its dial directly. Lighting channels: the tent lamp on channel 2 only; the tent and
# everything in it (folder Story/Ch1/Tent) on channels 0 + 2. The held compass (channels 0 + 1) gets only the tent's bounce
# light, as it would from a lamp behind the hand; outdoors it is lit by the sun as before.
W = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
if W.get_name() != "base_map":
    unreal.EditorLoadingAndSavingUtils.load_map("/Game/LevelPrototyping/base_map")
EAS = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
acts = EAS.get_all_level_actors()


def ch(a, b, c):
    lc = unreal.LightingChannels()
    lc.set_editor_property("channel0", a); lc.set_editor_property("channel1", b); lc.set_editor_property("channel2", c)
    return lc


lamp = [a for a in acts if "tentlight" in [str(t) for t in a.tags]][0]
lamp.light_component.set_editor_property("lighting_channels", ch(False, False, True))
L = lamp.get_actor_location()
tents = [a for a in acts if isinstance(a, unreal.StaticMeshActor) and a.static_mesh_component.static_mesh
         and a.static_mesh_component.static_mesh.get_name() == "tent_dome"]
tent = min(tents, key=lambda t: (t.get_actor_location() - L).length())
n = 0
for a in acts:
    if str(a.get_folder_path()) == "Story/Ch1/Tent" or a == tent:
        for c in a.get_components_by_class(unreal.PrimitiveComponent):
            c.set_editor_property("lighting_channels", ch(True, False, True)); n += 1
print("CHANNELS lamp=2, tent parts=0+2:", n, "components; tent", tent.get_actor_label())
unreal.EditorLevelLibrary.save_current_level()
print("done")
