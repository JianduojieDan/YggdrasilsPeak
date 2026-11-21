import unreal
# Runs INSIDE the editor via Tools/RoutePlanning/uepy. 2026-11-21 fire follow-up: (1) SM_FireCard came in as a Nanite mesh,
# and Nanite drops additive / translucent materials (no flames, a grey smoke square) -> Nanite off; (2) the Dingboche stone
# fire ring stood around the prayer pole's base -> moved 3.5 m onto open snow (a fire never burns at a tarchen).
W_ = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
if W_.get_name() != "base_map":
    unreal.EditorLoadingAndSavingUtils.load_map("/Game/LevelPrototyping/base_map")
EAL = unreal.EditorAssetLibrary
EAS = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
card = unreal.load_asset("/Game/Environment/Fire/SM_FireCard")
ns = card.get_editor_property("nanite_settings"); ns.set_editor_property("enabled", False); card.set_editor_property("nanite_settings", ns)
EAL.save_loaded_asset(card, False)
flag = unreal.load_asset("/Game/Environment/Flags/SM_PrayerFlagCloth")
print("card nanite", card.get_editor_property("nanite_settings").get_editor_property("enabled"), "flag nanite", flag.get_editor_property("nanite_settings").get_editor_property("enabled"))
for a in EAS.get_all_level_actors():
    if a.get_actor_label() == "Dingboche_FireRing":
        l = a.get_actor_location()
        a.set_actor_location(unreal.Vector(l.x - 350.0, l.y, l.z), False, False)
        # settle it on the ground below
        h = unreal.SystemLibrary.line_trace_single(W_, a.get_actor_location() + unreal.Vector(0, 0, 300), a.get_actor_location() - unreal.Vector(0, 0, 600),
                                                   unreal.TraceTypeQuery.TRACE_TYPE_QUERY1, False, [a], unreal.DrawDebugTrace.NONE, True)
        t = h.to_tuple() if h else (False,)
        if t[0]:
            l2 = a.get_actor_location(); a.set_actor_location(unreal.Vector(l2.x, l2.y, t[4].z - 4.0), False, False)
        print("MOVED ring", l, "->", a.get_actor_location())
unreal.EditorLevelLibrary.save_current_level()
print("done")
