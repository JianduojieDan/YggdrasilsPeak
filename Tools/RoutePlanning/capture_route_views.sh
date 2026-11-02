#!/usr/bin/env bash
# Captures route views through capture_scene.py, one view at a time: first moves the EDITOR camera to the view
# (the landscape streams its detail around the editor camera, not around a scene capture - far captures show
# coarse fallback terrain with props floating above it), waits for streaming, then captures.
#   capture_route_views.sh <tag> "<name> <leg> <fraction> <height_cm> <yaw_off> <pitch>" ...
# Output: Saved/Compare/<name><tag>.png
cd "$(dirname "$0")/../.."
TAG="$1"; shift
TMP="${TMPDIR:-/tmp}/ygg_capture_view.py"
for V in "$@"; do
  set -- $V
  cat > "$TMP" <<EOF
import unreal, json, math
R = json.load(open(r"F:/UE_project/YggdrasilsPeak/Tools/RoutePlanning/route.json"))
pts = R[$2]["pts"]; i = min(int($3 * (len(pts) - 1)), len(pts) - 2)
a, b = pts[i], pts[min(i + 5, len(pts) - 1)]
yaw = math.degrees(math.atan2(b[1] - a[1], b[0] - a[0])) + $5
unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).set_level_viewport_camera_info(unreal.Vector(a[0], a[1], a[2] + $4), unreal.Rotator(roll=0, pitch=$6, yaw=yaw))
EOF
  PYTHONIOENCODING=utf8 python -I Tools/RoutePlanning/uepy.py "$TMP" > /dev/null 2>&1
  sleep 25
  printf 'VIEWS=[("%s",%s,%s,%s,%s,%s)]\nOUT_TAG="%s"\nexec(open(r"F:/UE_project/YggdrasilsPeak/Tools/RoutePlanning/capture_scene.py").read())\n' "$1" "$2" "$3" "$4" "$5" "$6" "$TAG" > "$TMP"
  PYTHONIOENCODING=utf8 python -I Tools/RoutePlanning/uepy.py "$TMP" 2>&1 | grep -E "captured|Error"
done
