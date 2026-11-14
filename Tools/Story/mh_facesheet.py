import unreal, os
# Runs INSIDE the editor via Tools/RoutePlanning/uepy (no dot-p-y text in this file). Portrait of every MetaHuman preset
# (or the env YGG_FACES list) in a small lit studio level -> Saved/Compare/faces/<Preset>.png, so faces can be chosen by
# eye instead of by name. Spawned preview actors are transient; nothing in the presets is saved.
# MEMORY: all 29 at once filled 32 GB + page file and crashed the editor (2026-11-14): render <= 10 per editor session.
LIST = "F:/UE_project/YggdrasilsPeak/Saved/Compare/faces_list.txt"   # optional: names to render (one line)
NAMES = (open(LIST).read() if os.path.exists(LIST) else os.environ.get("YGG_FACES", "Bo Bruce Cameron Dominic Isaiah Jorge Kelvin Lorenzo Mateo Mikel Omari Orlando Trey Victor Walter "
                                    "Ada Aera Aoi Asha Celeste Etta Grace Jelena Lani Sook-ja Sunita Tuya Vivian Zuri")).split()
OUT = "F:/UE_project/YggdrasilsPeak/Saved/Compare/faces"
os.makedirs(OUT, exist_ok=True)
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
EAS = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
lvl = "/Game/Story/MetaHumans/L_FaceSheet"
if unreal.EditorAssetLibrary.does_asset_exist(lvl): les.load_level(lvl)
else: les.new_level(lvl)
W = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
for a in EAS.get_all_level_actors(): EAS.destroy_actor(a)
V, R = unreal.Vector, unreal.Rotator
sun = EAS.spawn_actor_from_class(unreal.DirectionalLight, V(0, 0, 500), R(-35, 0, 150))   # warm key from front-left
sun.light_component.set_intensity(6.0); sun.light_component.set_light_color(unreal.LinearColor(1.0, 0.92, 0.82, 1))
fill = EAS.spawn_actor_from_class(unreal.SkyLight, V(0, 0, 300), R(0, 0, 0))
fill.light_component.set_editor_property("source_type", unreal.SkyLightSourceType.SLS_SPECIFIED_CUBEMAP)
fill.light_component.set_editor_property("cubemap", unreal.load_asset("/Engine/MapTemplates/Sky/SunsetAmbientCubemap"))
fill.light_component.set_intensity(1.2); fill.light_component.recapture_sky()
bg = EAS.spawn_actor_from_object(unreal.load_asset("/Engine/BasicShapes/Plane"), V(-300, 0, 160), R(0, 90, 0))
bg.set_actor_scale3d(V(6, 6, 1))
rt = unreal.RenderingLibrary.create_render_target2d(W, 768, 960, unreal.TextureRenderTargetFormat.RTF_RGBA8_SRGB)
cap = EAS.spawn_actor_from_class(unreal.SceneCapture2D, V(0, 0, 0), R(0, 0, 0))
cc = cap.capture_component2d
cc.set_editor_property("texture_target", rt); cc.set_editor_property("fov_angle", 22.0)
cc.set_editor_property("capture_source", unreal.SceneCaptureSource.SCS_FINAL_COLOR_LDR)
s = unreal.get_editor_subsystem(unreal.MetaHumanCharacterEditorSubsystem)
done = []
for n in NAMES:
    p = f"/MetaHumanCharacter/Optional/Presets/{n}"
    ch = unreal.load_asset(p)
    if not ch: continue
    try:
        if not s.is_object_added_for_editing(ch): s.try_add_object_to_edit(ch)
        ss = ch.get_editor_property("skin_settings"); s.commit_skin_settings(ch, ss)   # synthesises the skin textures
        s.assemble_for_preview(ch)
        a = s.spawn_meta_human_actor(ch, True)
    except Exception as e:
        print("skip", n, e); continue
    a.set_actor_location(V(0, 0, 0), False, False); a.set_actor_rotation(R(0, 0, -90), False)   # its face -> world +X, towards the camera
    # face height from the spawned meshes' bounds
    top = 0.0
    for c in a.get_components_by_class(unreal.SkeletalMeshComponent):
        o, e, _ = unreal.SystemLibrary.get_component_bounds(c)
        top = max(top, o.z + e.z)
    top = top or 170.0
    eye = top - 14.0
    cap.set_actor_location(V(118, 13, eye), False, False)
    cap.set_actor_rotation(R(-2, 0, 186), False)    # looks back at the face, slightly from the side
    cc.capture_scene()
    unreal.RenderingLibrary.export_render_target(W, rt, OUT, f"{n}.png")
    done.append((n, round(top)))
    EAS.destroy_actor(a)
    s.remove_object_to_edit(ch)
print("faces", done)
