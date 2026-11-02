# Runs INSIDE the editor (uepy.py). Moves the level viewport camera to named views and takes high-res screenshots
# (TSR on, unlike SceneCapture). Views come from route.json so the camera sits on the real route:
#   VIEWS = [(name, leg, fraction along leg, height above route cm, yaw offset deg, pitch deg)]
# Output: <project>/Saved/Screenshots/WindowsEditor/<name>.png  (set OUT_TAG to suffix names)
import unreal, json, math
ROOT = "F:/UE_project/YggdrasilsPeak"
R = json.load(open(f"{ROOT}/Tools/RoutePlanning/route.json"))
VIEWS = globals().get("VIEWS") or [
    ("gorakshep_moraine", 1, 0.25, 300, 0, -12),
    ("ebc_icefall", 2, 0.15, 400, 0, -6),
    ("icefall_mid", 3, 0.3, 400, 0, -8),
    ("khumbu_overview", 2, 0.0, 60000, -150, -35),
]
TAG = globals().get("OUT_TAG", "")
ues = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
for name, leg, f, up, yaw_off, pitch in VIEWS:
    pts = R[leg]["pts"]
    i = min(int(f * (len(pts) - 1)), len(pts) - 2)
    a, b = pts[i], pts[min(i + 5, len(pts) - 1)]
    yaw = math.degrees(math.atan2(b[1] - a[1], b[0] - a[0])) + yaw_off
    loc = unreal.Vector(a[0], a[1], a[2] + up)
    ues.set_level_viewport_camera_info(loc, unreal.Rotator(roll=0, pitch=pitch, yaw=yaw))
    unreal.AutomationLibrary.take_high_res_screenshot(1600, 900, f"{name}{TAG}.png", delay=2.0)
    print(name, loc, yaw)
