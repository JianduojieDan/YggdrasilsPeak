# Runs inside the editor. LandscapeRebuilder.ApplyTerrainMath with the values below; each run replaces the last.
# Then run Tools/Art/reseat_rocks (GROW 1.0), save, restart the editor, and rerun verify_route_corridor.
import unreal
P = dict(MathRoughnessM=20.0, MathSharpness=0.5, MathScaleM=300.0, MathSeed=1, MathGentleGround=0.3,
         MathAboveM=3000.0, MathFullAboveM=5000.0, MathRouteClearM=30.0, MathCampClearM=120.0, MathKeepSummitM=20.0,
         MathSmoothAboveM=7800.0, MathSmoothFullAboveM=8200.0, MathSmoothPasses=48)
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
r = [a for a in eas.get_all_level_actors() if a.get_class().get_name() == "LandscapeRebuilder"][0]
r.set_editor_property("auto_rebuild", False)   # any other value would rebuild the whole landscape
for k, v in P.items():
    r.set_editor_property(k, v)
assert r.get_editor_property("auto_rebuild") is False
r.call_method("ApplyTerrainMath")
print(r.get_editor_property("last_result"))
