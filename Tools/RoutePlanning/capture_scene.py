# Runs INSIDE the editor (uepy.py). Renders views through a temporary SceneCapture2D into PNGs - works while the
# editor window is in the background (the level viewport / HighResShot only render when it is visible).
# No TSR in scene captures: fine for judging materials, slightly noisier than the game.
#   VIEWS = [(name, leg, fraction along leg, height above route cm, yaw offset deg, pitch deg)]  (from route.json)
#   or VIEWS_ABS = [(name, x, y, z, yaw, pitch)]
# Output: <project>/Saved/Compare/<name><OUT_TAG>.png
import unreal, json, math
ROOT = "F:/UE_project/YggdrasilsPeak"
R = json.load(open(f"{ROOT}/Tools/RoutePlanning/route.json"))
TAG = globals().get("OUT_TAG", "")
views = []
for name, leg, f, up, yaw_off, pitch in globals().get("VIEWS", []):
    pts = R[leg]["pts"]
    i = min(int(f * (len(pts) - 1)), len(pts) - 2)
    a, b = pts[i], pts[min(i + 5, len(pts) - 1)]
    yaw = math.degrees(math.atan2(b[1] - a[1], b[0] - a[0])) + yaw_off
    views.append((name, unreal.Vector(a[0], a[1], a[2] + up), yaw, pitch))
for name, x, y, z, yaw, pitch in globals().get("VIEWS_ABS", []):
    views.append((name, unreal.Vector(x, y, z), yaw, pitch))

world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
rt = unreal.RenderingLibrary.create_render_target2d(world, 1600, 900, unreal.TextureRenderTargetFormat.RTF_RGBA8_SRGB)
cap = unreal.get_editor_subsystem(unreal.EditorActorSubsystem).spawn_actor_from_class(unreal.SceneCapture2D, unreal.Vector(0, 0, 0))
c = cap.capture_component2d
c.set_editor_property("texture_target", rt)
c.set_editor_property("capture_source", unreal.SceneCaptureSource.SCS_FINAL_TONE_CURVE_HDR)
c.set_editor_property("capture_every_frame", False)
c.set_editor_property("fov_angle", globals().get("FOV", 90.0))
for name, loc, yaw, pitch in views:
    cap.set_actor_location_and_rotation(loc, unreal.Rotator(roll=0, pitch=pitch, yaw=yaw), False, True)
    for _ in range(3):  # a few captures so exposure / streaming settle
        c.capture_scene()
    unreal.RenderingLibrary.export_render_target(world, rt, f"{ROOT}/Saved/Compare", f"{name}{TAG}.png")
    print("captured", name, loc, yaw)
cap.destroy_actor()
