# Runs INSIDE the editor (Tools/RoutePlanning/uepy.py): puts a UYggFootstepNotify on every L / R sync marker (the
# foot-contact frames, LEAD seconds early) of the clips in the climber's locomotion blend space, and lets every clip of the blend send them
# (USnowTrailComponent::OnFootContact drops the duplicates). Safe to run again: old Ygg footstep notifies are replaced.
# Why: on rough rock the foot-height step detection fired mid-swing (play-test 2026-11-13); the animation knows better.
import unreal
AL = unreal.AnimationLibrary
BS = "/Game/Characters/Climber_Rigged/Anims/BS_Idle_Walk_Run_Climber"
TRACK = "Footsteps"
# USnowTrailComponent::StepNotifyLead: snow's approved steps started this long before the boot lands (their crunch
# peaks as it lands); hard ground waits in the code for the rest. Keep the two equal.
LEAD = 0.155
notify_class = unreal.load_class(None, "/Script/YggdrasilsPeak.YggFootstepNotify")
assert notify_class, "build the game code first (UYggFootstepNotify)"

bs = unreal.load_asset(BS)
anims = []
for s in bs.get_editor_property("sample_data"):
    a = s.get_editor_property("animation")
    if a and a not in anims:
        anims.append(a)

changed = []
for a in anims:
    markers = AL.get_animation_sync_markers(a)
    if not markers:
        continue
    if AL.is_valid_anim_notify_track_name(a, TRACK):
        AL.remove_animation_notify_events_by_track(a, TRACK)
    else:
        AL.add_animation_notify_track(a, TRACK, unreal.LinearColor(0.4, 0.8, 1.0, 1.0))
    for m in markers:
        name = str(m.get_editor_property("marker_name"))
        if name not in ("L", "R"):
            continue
        n = AL.add_animation_notify_event(a, TRACK, (m.get_editor_property("time") - LEAD) % a.get_play_length(), notify_class)
        n.set_editor_property("left", name == "L")   # bLeft (Python drops the b)
    changed.append(a.get_path_name().split(".")[0])
    print(a.get_name(), len(markers), "steps")

bs.set_editor_property("notify_trigger_mode", unreal.NotifyTriggerMode.ALL_ANIMATIONS)
changed.append(BS)
print("saved:", unreal.EditorAssetLibrary.save_loaded_assets([unreal.load_asset(p) for p in changed], only_if_is_dirty=False))


