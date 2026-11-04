# Runs INSIDE the editor: LandscapeRebuilder.SculptHighRidges (C++) on Landscape_Smooth above SculptAboveM - breaks the
# big flat triangles / saw-tooth shadow edges high up into wind-cut ridges and gullies; props + rope points move with it.
# Passes run 2026-11-03: (7300 m, 6 m, 150 m) then (7600 m, 8 m, 90 m). Each run adds on top of the last.
# After a run: settle_rocks.py, save, and RESTART the editor - the landscape's Nanite mesh draws nothing until then.
import unreal
P = globals().get("PASS", dict(SculptAboveM=7600.0, SculptFullAboveM=8000.0, SculptHeightM=8.0, SculptWavelengthM=90.0))
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
r = [a for a in eas.get_all_level_actors() if a.get_class().get_name() == "LandscapeRebuilder"][0]
# !! bAutoRebuild rebuilds the WHOLE landscape from Landscape_0 on any property change (2026-11-03: that wiped the
# summit raise once). Switch it off before touching anything.
r.set_editor_property("auto_rebuild", False)
for k, v in P.items():
    r.set_editor_property(k, v)
r.call_method("SculptHighRidges")
print(r.get_editor_property("last_result"))
