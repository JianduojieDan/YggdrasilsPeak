# Runs INSIDE the editor (uepy.py). The Icefall ice cave with an old expedition's cache (Phase1_Design_Plan S3
# exploration). Imports Art/Blender/Export/Cave/SM_IceCave.fbx (Tools/Blender/ice_cave.py; walkable, so not Nanite,
# complex collision) with M_SeracIce, finds the flattest ground 40-60 m off route leg 3, turns the entrance to the
# trail, and stocks the cache: old oxygen bottles and a rope (props), supplies you can take (rations, water, an
# oxygen bottle), and a note. A faint blue light inside (sun through the ice). Tag IceCave; re-running replaces it.
import unreal, json, math
R = json.load(open("F:/UE_project/YggdrasilsPeak/Tools/RoutePlanning/route.json"))
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
at = unreal.AssetToolsHelpers.get_asset_tools()
SMS = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
for a in eas.get_all_level_actors():
    if a.actor_has_tag("IceCave"):
        a.destroy_actor()
D = "/Game/Environment/Glacier/Cave"
unreal.SystemLibrary.execute_console_command(None, "Interchange.FeatureFlags.Import.FBX 0")
o = unreal.FbxImportUI(); o.import_mesh = True; o.import_materials = False; o.import_textures = False
o.mesh_type_to_import = unreal.FBXImportType.FBXIT_STATIC_MESH
o.static_mesh_import_data.combine_meshes = True; o.static_mesh_import_data.auto_generate_collision = False
t = unreal.AssetImportTask(); t.filename = "F:/UE_project/YggdrasilsPeak/Art/Blender/Export/Cave/SM_IceCave.fbx"
t.destination_path = D; t.destination_name = "SM_IceCave"; t.automated = True; t.save = True; t.replace_existing = True; t.options = o
at.import_asset_tasks([t])
unreal.SystemLibrary.execute_console_command(None, "Interchange.FeatureFlags.Import.FBX 1")
cave = unreal.load_asset(f"{D}/SM_IceCave")
cave.set_material(0, unreal.load_asset("/Game/Environment/Glacier/Seracs/M_SeracIce"))
SMS.remove_collisions(cave)
cave.get_editor_property("body_setup").set_editor_property("collision_trace_flag", unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE)
unreal.EditorAssetLibrary.save_loaded_asset(cave, False)

IGN = [a for a in eas.get_all_level_actors() if a.get_class().get_name() in ("ConstrainedScatter", "CrevasseCrossing", "SeracCollapseZone")]
def ground(x, y, z):
    h = unreal.SystemLibrary.line_trace_single(world, unreal.Vector(x, y, z + 6000), unreal.Vector(x, y, z - 8000),
        unreal.TraceTypeQuery.ECC_VISIBILITY, True, IGN, unreal.DrawDebugTrace.NONE, True).to_tuple()
    return (h[4].z, h[7].z) if h[0] else (z, 1.0)
avoid = [a.get_actor_location() for a in eas.get_all_level_actors()
         if a.get_class().get_name() in ("TrailCheckpoint", "StoryClue", "RouteLandmark", "CrevasseCrossing", "SeracCollapseZone")]
pts = R[3]["pts"]
best = None
for k in range(int(0.55 * len(pts)), int(0.8 * len(pts))):
    a_, b_ = pts[k], pts[min(k + 2, len(pts) - 1)]
    yaw = math.degrees(math.atan2(b_[1] - a_[1], b_[0] - a_[0]))
    for side in (-1, 1):
        for d in (4000, 5000, 6000):
            sx = a_[0] + math.cos(math.radians(yaw + 90 * side)) * d; sy = a_[1] + math.sin(math.radians(yaw + 90 * side)) * d
            if min((math.hypot(sx - v.x, sy - v.y) for v in avoid), default=1e9) < 4000:
                continue
            zs = [ground(sx + dx, sy + dy, a_[2])[0] for dx in (-600, 0, 600) for dy in (-500, 0, 500)]
            spread = max(zs) - min(zs)
            if best is None or spread < best[0]:
                best = (spread, sx, sy, min(zs), yaw + 90 * side, a_)
spread, cx, cy, gz, out_yaw, rp = best
# entrance (-X of the mesh) faces back towards the trail
face = math.degrees(math.atan2(rp[1] - cy, rp[0] - cx))
# the cave's flat floor (mesh z = 0) a little under the highest ground beneath it
floor_z = max(ground(cx + dx, cy + dy, gz)[0] for dx in (-400, 0, 400) for dy in (-300, 0, 300)) - 5
c = eas.spawn_actor_from_object(cave, unreal.Vector(cx, cy, floor_z), unreal.Rotator(roll=0, pitch=0, yaw=face + 180))
c.set_actor_label("IceCave"); c.set_folder_path("Route/IceCave"); c.tags = ["IceCave", "ScatterProtectBox"]
IGN.append(c)        # ground probes for the cache must go through the roof
fwd = unreal.Vector(math.cos(math.radians(face + 180)), math.sin(math.radians(face + 180)), 0)
side = unreal.Vector(-fwd.y, fwd.x, 0)
def inside(ax, ay, z=0):   # metres along the cave (+X = deeper) / across, from its centre; on the actual ground there
    x, y = cx + fwd.x * ax * 100 + side.x * ay * 100, cy + fwd.y * ax * 100 + side.y * ay * 100
    return unreal.Vector(x, y, max(ground(x, y, gz)[0], floor_z) + 2 + z)
def prop(path, ax, ay, rot, scale=1.0, label="CacheProp"):
    p = eas.spawn_actor_from_object(unreal.load_asset(path), inside(ax, ay), unreal.Rotator(roll=rot[0], pitch=rot[1], yaw=face + 180 + rot[2]))
    p.set_actor_scale3d(unreal.Vector(scale, scale, scale)); p.set_actor_label(label); p.set_folder_path("Route/IceCave"); p.tags = ["IceCave"]
    p.get_component_by_class(unreal.StaticMeshComponent).set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
    return p
prop("/Game/Fab/Oxygen_Tank/oxygen_tank/StaticMeshes/oxygen_tank", 2.6, -1.4, (90, 0, 20), 1.0, "Cache_OldBottle1")
prop("/Game/Fab/Oxygen_Tank/oxygen_tank/StaticMeshes/oxygen_tank", 2.9, -1.0, (90, 0, -15), 1.0, "Cache_OldBottle2")
prop("/Game/Fab/Megascans/3D/Bundled_Rope_Coil_vhnmbdcs/Raw/vhnmbdcs_tier_0/StaticMeshes/vhnmbdcs_tier_0", 2.2, 1.3, (0, 0, 40), 1.0, "Cache_Rope")
prop("/Game/Props/Blender/SM_Food_Stove", 1.6, 1.5, (0, 0, 0), 1.0, "Cache_Stove")
# what you can take
SP = unreal.load_class(None, "/Script/YggdrasilsPeak.SupplyPickup")
OP = unreal.load_class(None, "/Script/YggdrasilsPeak.OxygenPickup")
for i, (item, n, ax, ay, mesh) in enumerate((("Ration", 2, 2.0, -0.4, "/Game/Props/Supplies/SM_Ration"), ("Water", 1, 2.4, 0.4, "/Game/Props/Supplies/SM_Thermos"))):
    s = eas.spawn_actor_from_class(SP, inside(ax, ay, 10), unreal.Rotator(roll=0, pitch=0, yaw=face))
    s.set_editor_property("item", item); s.set_editor_property("count", n)
    m = s.get_component_by_class(unreal.StaticMeshComponent)
    if m: m.set_static_mesh(unreal.load_asset(mesh))
    s.set_actor_label(f"Cache_{item}"); s.set_folder_path("Route/IceCave"); s.tags = ["IceCave"]
o2 = eas.spawn_actor_from_class(OP, inside(3.1, -0.2, 20), unreal.Rotator(roll=0, pitch=0, yaw=face))
o2.set_actor_label("Cache_Oxygen"); o2.set_folder_path("Route/IceCave"); o2.tags = ["IceCave"]
# the note
SC = unreal.load_class(None, "/Script/YggdrasilsPeak.StoryClue")
note = eas.spawn_actor_from_class(SC, inside(1.4, 0.0, 0), unreal.Rotator(roll=0, pitch=0, yaw=face))
note.set_editor_property("clue_id", "IceCave_Cache")
note.set_editor_property("title", unreal.Text("A cache in the ice"))
note.set_editor_property("body", unreal.Text("A tin box, its lid frozen shut. Scratched into it with a knife:\n\n"
    "\"Cache - Korean expedition, spring '06. Back for it after the summit.\"\n\n"
    "Nobody came back for it. Under the frost, a second line, older, in another hand - "
    "the same runes as on Father's compass."))
note.set_editor_property("read_radius", 350.0)
note.set_actor_label("Clue_IceCaveCache"); note.set_folder_path("Route/IceCave"); note.tags = ["IceCave"]
# blue light inside: the sun coming through the ice
L = eas.spawn_actor_from_class(unreal.PointLight, inside(1.5, 0.0, 200), unreal.Rotator())
lc = L.get_component_by_class(unreal.PointLightComponent)
lc.set_editor_property("intensity", 260.0); lc.set_editor_property("light_color", unreal.Color(r=120, g=190, b=255, a=255))
lc.set_editor_property("attenuation_radius", 650.0); lc.set_editor_property("cast_shadows", False)
L.set_actor_label("IceCave_Glow"); L.set_folder_path("Route/IceCave"); L.tags = ["IceCave"]
print("ice cave at", (round(cx), round(cy), round(gz)), "ground spread", round(spread), "cm, facing", round(face))
