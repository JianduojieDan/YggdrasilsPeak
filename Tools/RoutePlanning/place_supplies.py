# Runs INSIDE the editor (uepy.py). Imports the Poly Haven ration tin and thermos (CC0) once, then places
# ASupplyPickup actors at every camp: rations and water beside the camp centre, on the ground (landscape trace).
# Re-running deletes the old SupplyPickup actors first. Outliner folder Route/Supplies.
import unreal, math
ROOT = "F:/UE_project/YggdrasilsPeak"
D = "/Game/Props/Supplies"
at = unreal.AssetToolsHelpers.get_asset_tools()
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

def import_mesh(fbx, name):
    path = f"{D}/{name}"
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        return unreal.load_asset(path)
    unreal.SystemLibrary.execute_console_command(None, "Interchange.FeatureFlags.Import.FBX 0")
    t = unreal.AssetImportTask()
    t.filename = fbx; t.destination_path = D; t.destination_name = name
    t.automated = True; t.save = True; t.replace_existing = True
    o = unreal.FbxImportUI()
    o.import_mesh = True; o.import_materials = True; o.import_textures = True; o.import_animations = False
    o.mesh_type_to_import = unreal.FBXImportType.FBXIT_STATIC_MESH
    o.static_mesh_import_data.combine_meshes = True
    t.options = o
    at.import_asset_tasks([t])
    unreal.SystemLibrary.execute_console_command(None, "Interchange.FeatureFlags.Import.FBX 1")
    return unreal.load_asset(path)

ration = import_mesh(f"{ROOT}/Imports/PolyHaven/long_life_food/long_life_food_1k.fbx", "SM_Ration")
thermos = import_mesh(f"{ROOT}/Imports/PolyHaven/modified_thermos/modified_thermos_1k.fbx", "SM_Thermos")
print("meshes", ration, thermos, ration and ration.get_bounding_box(), thermos and thermos.get_bounding_box())

for a in eas.get_all_level_actors():
    if a.get_class().get_name() == "SupplyPickup":
        a.destroy_actor()

world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
def ground(x, y, z):
    hit = unreal.SystemLibrary.line_trace_single(world, unreal.Vector(x, y, z + 3000), unreal.Vector(x, y, z - 5000),
        unreal.TraceTypeQuery.TRACE_TYPE_QUERY1, False, [], unreal.DrawDebugTrace.NONE, True)
    return hit.to_tuple()[4].z if hit and hit.to_tuple()[0] else z  # impact point

SupplyClass = unreal.load_class(None, "/Script/YggdrasilsPeak.SupplyPickup")
camps = [a for a in eas.get_all_level_actors() if a.get_class().get_name() == "RouteLandmark"
         and "SUMMIT" not in str(a.get_editor_property("kind")).upper()]  # no supplies on the summit
n = 0
for c in camps:
    p = c.get_actor_location()
    for item, mesh, count, ang, scale in (("Ration", ration, 3, 40, 1.0), ("Water", thermos, 2, 70, 1.0)):
        x = p.x + 650 * math.cos(math.radians(ang)); y = p.y + 650 * math.sin(math.radians(ang))
        z = ground(x, y, p.z)
        s = eas.spawn_actor_from_class(SupplyClass, unreal.Vector(x, y, z + 2), unreal.Rotator(roll=0, pitch=0, yaw=ang * 3))
        s.set_editor_property("item", item)
        s.set_editor_property("count", count)
        m = s.get_editor_property("mesh")
        m.set_static_mesh(mesh)
        m.set_world_scale3d(unreal.Vector(scale, scale, scale))
        s.set_actor_label(f"{c.get_actor_label()}_{item}")
        s.set_folder_path("Route/Supplies")
        n += 1
print("sample", s.get_actor_location(), "camp", p)
print("placed", n, "supplies at", len(camps), "camps")
unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
