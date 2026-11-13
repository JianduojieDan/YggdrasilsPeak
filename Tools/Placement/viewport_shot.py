# Runs inside the editor: points the level viewport at SPOT = (x, y, z) from a few object-sizes away (SIZE cm, YAW deg)
# and asks for a high-res screenshot (lands in Saved/Screenshots/WindowsEditor on the next frames - one shot per run).
import math, unreal
x, y, z = SPOT
size = globals().get('SIZE', 100.0)
yaw = globals().get('YAW', 30.0)
dist = max(160.0, size * 4.0)
cx, cy = x - math.cos(math.radians(yaw)) * dist, y - math.sin(math.radians(yaw)) * dist
w = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
h = unreal.SystemLibrary.line_trace_single(w, unreal.Vector(cx, cy, z + 50000), unreal.Vector(cx, cy, z - 50000),
    unreal.TraceTypeQuery.ECC_VISIBILITY, True, [], unreal.DrawDebugTrace.NONE, True)
gz = h.to_tuple()[4].z if h else z
cz = max(z + dist * 0.12, gz + 60.0)
pitch = -math.degrees(math.atan2(cz - z, dist))
unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).set_level_viewport_camera_info(unreal.Vector(cx, cy, cz), unreal.Rotator(roll=0, pitch=pitch, yaw=yaw))
unreal.SystemLibrary.execute_console_command(w, 'HighResShot 1600x900')
print('camera', round(cx), round(cy), round(cz), 'pitch', round(pitch, 1))
