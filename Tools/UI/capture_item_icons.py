# Runs INSIDE the editor (uepy.py), base_map loaded. Renders each pack item's real game mesh alone (show-only list)
# high above the map in the sun, into Saved/ItemIcons/raw/<Item>.png (RGBA: alpha from the capture's opacity).
# Tools/UI/ink_icons.py then turns them into the backpack screen's ink-and-wash sketches.
#   ITEMS = [(name, mesh path, (pitch, yaw, roll) of the item, view yaw)]
import unreal, math, os
ROOT = "F:/UE_project/YggdrasilsPeak"
OUT = f"{ROOT}/Saved/ItemIcons/raw"
os.makedirs(OUT, exist_ok=True)
ITEMS = globals().get("ITEMS") or [
    ("Ration", "/Game/Props/Supplies/SM_Ration", (0, 20, 0), 200),
    ("Water", "/Game/Props/Supplies/SM_Thermos", (0, 0, 15), 200),
    ("Oxygen", "/Game/Fab/Oxygen_Tank/oxygen_tank/StaticMeshes/oxygen_tank", (0, 0, 0), 200),
    ("IceAxe", "/Game/Fab/Climbing_Axe_for_Blender/climbing_axe_422_packed/StaticMeshes/climbing_axe_422_packed", (0, 0, 0), 200),
    ("Rope", "/Game/Fab/Megascans/3D/Bundled_Rope_Coil_vhnmbdcs/Raw/vhnmbdcs_tier_0/StaticMeshes/vhnmbdcs_tier_0", (0, 0, 0), 200),
    ("Stove", "/Game/Props/Blender/SM_Food_Stove", (0, 0, 0), 200),
    ("Gas", "/Game/Props/Blender/SM_Food_GasCanister", (0, 0, 0), 200),
]
SIZE = 768
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
BASE = unreal.Vector(0, 0, 900000)       # far above the map, lit by the sun and sky
rt = unreal.RenderingLibrary.create_render_target2d(world, SIZE, SIZE, unreal.TextureRenderTargetFormat.RTF_RGBA16F)
cap = eas.spawn_actor_from_class(unreal.SceneCapture2D, BASE)
c = cap.capture_component2d
c.set_editor_property("texture_target", rt)
c.set_editor_property("capture_source", unreal.SceneCaptureSource.SCS_SCENE_COLOR_HDR)
c.set_editor_property("capture_every_frame", False)
c.set_editor_property("primitive_render_mode", unreal.SceneCapturePrimitiveRenderMode.PRM_USE_SHOW_ONLY_LIST)
c.set_editor_property("fov_angle", 22.0)
for name, path, (pi, ya, ro), view_yaw in ITEMS:
    mesh = unreal.load_asset(path)
    a = eas.spawn_actor_from_object(mesh, BASE, unreal.Rotator(roll=ro, pitch=pi, yaw=ya))
    b = a.get_actor_bounds(False)
    centre, ext = b[0], b[1]
    r = max(ext.x, ext.y, ext.z) * 1.15
    d = r / math.tan(math.radians(11.0))
    yaw = math.radians(view_yaw)
    loc = centre + unreal.Vector(-math.cos(yaw) * d * 0.94, -math.sin(yaw) * d * 0.94, d * 0.34)
    rot = unreal.MathLibrary.find_look_at_rotation(loc, centre)
    cap.set_actor_location_and_rotation(loc, rot, False, True)
    c.clear_show_only_components()
    c.show_only_actor_components(a)
    for _ in range(3):
        c.capture_scene()
    px = unreal.RenderingLibrary.read_render_target_raw(world, rt, False)
    import struct, zlib
    # write a 16-bit-free simple float dump: RGBA float32 little endian, the ink script reads it
    with open(f"{OUT}/{name}.rgbaf", "wb") as f:
        f.write(struct.pack("<ii", SIZE, SIZE))
        f.write(b"".join(struct.pack("<ffff", p.r, p.g, p.b, p.a) for p in px))
    a.destroy_actor()
    print("captured", name, "pixels", len(px))
cap.destroy_actor()
