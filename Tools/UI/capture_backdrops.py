# Runs INSIDE the editor (uepy.py) on base_map: renders candidate start-menu backdrops of our own mountain at 3840x2160
# (Saved/Backdrops/raw_<name>.png; reduced to 1920x1080 outside for clean edges, then imported as T_Backdrop_NN).
import unreal, math
ROOT = "F:/UE_project/YggdrasilsPeak"
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
def ground(x, y):
    h = unreal.SystemLibrary.line_trace_single(world, unreal.Vector(x, y, 1500000), unreal.Vector(x, y, -200000), unreal.TraceTypeQuery.TRACE_TYPE_QUERY1, True, [], unreal.DrawDebugTrace.NONE, True)
    return h.to_tuple()[4].z if h else 0.0
def look(a, b):
    d = (b[0] - a[0], b[1] - a[1], b[2] - a[2]); hz = math.hypot(d[0], d[1])
    return math.degrees(math.atan2(d[1], d[0])), math.degrees(math.atan2(d[2], hz))
SUMMIT = (1236165, 1417075, 381261 + 30000)
views = []
SX, SY, SZ = SUMMIT[0], SUMMIT[1], 381261
VIEWS = globals().get("VIEWS_OVERRIDE") or [
    # name, x, y, height above ground (or absolute z if negative up), look-at, extra yaw, extra pitch, fov
    ("massif", SX - 640000, SY - 640000, 22000, (SX, SY, SZ + 12000), 0, -2, 55),
    ("cwm_high", 1000911, 1510421, 6000, (SX, SY, SZ + 15000), 8, -3, 70),
    ("icefall_low", 470000, 1280000, 900, (657014, 1343408, 70000), -4, -6, 65),
    ("summit_east", SX - 6000, SY + 2000, 25000, (SX + 900000, SY + 200000, SZ - 60000), 0, 0, 80),
    ("ridges_west", SX + 600000, SY + 300000, 30000, (SX, SY, SZ + 10000), 0, -1, 50),
]
for name, x, y, up, target, dyaw, dpitch, fov in VIEWS:
    z = ground(x, y) + up
    yaw, pitch = look((x, y, z), target)
    views.append((name, unreal.Vector(x, y, z), yaw + dyaw, pitch + dpitch, fov))
rt = unreal.RenderingLibrary.create_render_target2d(world, 3840, 2160, unreal.TextureRenderTargetFormat.RTF_RGBA8_SRGB)
cap = eas.spawn_actor_from_class(unreal.SceneCapture2D, unreal.Vector(0, 0, 0))
c = cap.capture_component2d
c.set_editor_property("texture_target", rt)
c.set_editor_property("capture_source", unreal.SceneCaptureSource.SCS_FINAL_TONE_CURVE_HDR)
c.set_editor_property("capture_every_frame", False)
for name, loc, yaw, pitch, fov in views:
    c.set_editor_property("fov_angle", float(fov))
    cap.set_actor_location_and_rotation(loc, unreal.Rotator(roll=0, pitch=pitch, yaw=yaw), False, True)
    for _ in range(6): c.capture_scene()
    unreal.RenderingLibrary.export_render_target(world, rt, ROOT + "/Saved/Backdrops", "raw_" + name + ".png")
    print("captured", name, loc, "yaw %.1f pitch %.1f" % (yaw, pitch))
cap.destroy_actor()
