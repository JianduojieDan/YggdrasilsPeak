# Runs INSIDE the editor (uepy.py), base_map loaded. Phase 2 slice content (Docs/Development/Phase1_Design_Plan.md):
#   - AConstrainedScatter "Scatter_IcefallSeracs": ice-cliff seracs on the glacier around Base Camp -> Camp 1,
#     keeping the rope corridor, camps and pickups clear; Generate() writes Saved/ScatterReports/*.json
#   - ATrailCheckpoints halfway through the long / technical legs
#   - AStoryClues: the parents' abandoned camp (journal page 1) and the carbonised wood fibre in the Icefall
# Re-running replaces what it placed (actors tagged SliceContent).
import unreal, json, math
ROOT = "F:/UE_project/YggdrasilsPeak"
R = json.load(open(f"{ROOT}/Tools/RoutePlanning/route.json"))
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
MEL = unreal.MaterialEditingLibrary
at = unreal.AssetToolsHelpers.get_asset_tools()

for a in eas.get_all_level_actors():
    if a.actor_has_tag("SliceContent"):
        a.destroy_actor()

def ground(x, y, z):
    hit = unreal.SystemLibrary.line_trace_single(world, unreal.Vector(x, y, z + 5000), unreal.Vector(x, y, z - 8000),
        unreal.TraceTypeQuery.TRACE_TYPE_QUERY1, False, [], unreal.DrawDebugTrace.NONE, True)
    t = hit.to_tuple()
    return (t[4], t[7]) if t[0] else (unreal.Vector(x, y, z), unreal.Vector(0, 0, 1))  # impact point, impact normal

def on_route(leg, f, side_m=0.0):
    """Ground point on leg at fraction f, pushed side_m metres to the right of the walking direction."""
    pts = R[leg]["pts"]
    i = min(int(f * (len(pts) - 1)), len(pts) - 2)
    a, b = pts[i], pts[i + 1]
    d = unreal.Vector(b[0] - a[0], b[1] - a[1], 0).normal()
    right = unreal.Vector(-d.y, d.x, 0)
    p, n = ground(a[0] + right.x * side_m * 100, a[1] + right.y * side_m * 100, a[2])
    return p, n, math.degrees(math.atan2(d.y, d.x))

def tag(actor, label, folder):
    actor.tags = list(actor.tags) + ["SliceContent"]
    actor.set_actor_label(label)
    actor.set_folder_path(folder)

# ---------------------------------------------------------------- seracs
# procedural seracs (Tools/Blender/serac.py -> Tools/Materials/import_seracs.py, material M_SeracIce on the meshes)
# (mesh, weight, min scale, max scale)
ice = [(f"/Game/Environment/Glacier/Seracs/SM_Serac_{i:02d}", 1.0, 0.7, 1.9) for i in range(1, 7)]
# the same blocks small: broken ice rubble filling the gaps between seracs
ice += [(f"/Game/Environment/Glacier/Seracs/SM_Serac_{i:02d}", 1.6, 0.1, 0.3) for i in range(1, 7)]
xs = [p[0] for leg in (2, 3) for p in R[leg]["pts"]]; ys = [p[1] for leg in (2, 3) for p in R[leg]["pts"]]
zs = [p[2] for leg in (2, 3) for p in R[leg]["pts"]]
cx, cy, cz = (min(xs) + max(xs)) / 2, (min(ys) + max(ys)) / 2, (min(zs) + max(zs)) / 2
SC = unreal.load_class(None, "/Script/YggdrasilsPeak.ConstrainedScatter")
sc = eas.spawn_actor_from_class(SC, unreal.Vector(cx, cy, cz))
area = sc.get_editor_property("area")
area.set_box_extent(unreal.Vector((max(xs) - min(xs)) / 2 + 30000, (max(ys) - min(ys)) / 2 + 30000, (max(zs) - min(zs)) / 2 + 60000))
meshes = []
for p, wgt, lo, hi in ice:
    m = unreal.ScatterMesh()
    m.set_editor_property("mesh", unreal.load_asset(p))
    m.set_editor_property("weight", wgt)
    m.set_editor_property("min_scale", lo); m.set_editor_property("max_scale", hi)
    m.set_editor_property("embed_fraction", 0.12)
    meshes.append(m)
sc.set_editor_property("meshes", meshes)
for k, v in (("seed", 7), ("target_count", 2800), ("attempts_per_placement", 30), ("route_corridor_cm", 900.0),
             ("camp_clear_cm", 2500.0), ("spot_clear_cm", 800.0), ("min_gap_cm", -250.0),  # negative: neighbours may overlap and merge into ridges
             ("min_elevation_m", 5380.0), ("max_elevation_m", 5960.0), ("max_slope_deg", 40.0), ("mask_threshold", 0.5),
             ("max_route_distance_cm", 30000.0)):  # the maze along the climbing line, not the whole glacier
    sc.set_editor_property(k, v)
sc.set_editor_property("surface_mask", unreal.load_asset("/Game/Environment/Glacier/T_GlacierMask"))
tag(sc, "Scatter_IcefallSeracs", "Route/Scatter")
sc.call_method("Generate")
print("seracs", sc.get_editor_property("last_accepted"), "rejected", list(sc.get_editor_property("last_rejected")),
      "ms", round(sc.get_editor_property("last_milliseconds"), 1), sc.get_editor_property("last_report_path"))
sc.call_method("Verify")

# ---------------------------------------------------------------- trail checkpoints
TC = unreal.load_class(None, "/Script/YggdrasilsPeak.TrailCheckpoint")
coil = unreal.load_asset("/Game/Fab/Megascans/3D/Bundled_Rope_Coil_vhnmbdcs/Raw/vhnmbdcs_tier_0/StaticMeshes/vhnmbdcs_tier_0")
for leg, f, name in ((3, 0.5, "Icefall - halfway"), (5, 0.5, "Lhotse Face - halfway"), (6, 0.5, "Yellow Band"), (7, 0.5, "Summit ridge - Balcony")):
    p, n, yaw = on_route(leg, f, 2.5)
    c = eas.spawn_actor_from_class(TC, p, unreal.Rotator(roll=0, pitch=0, yaw=yaw))
    c.set_editor_property("checkpoint_name", name)
    c.get_editor_property("marker").set_static_mesh(coil)
    tag(c, "Checkpoint_" + name.split(" ")[0], "Route/Checkpoints")
print("checkpoints placed")

# ---------------------------------------------------------------- wood fibre material
exec(open(f"{ROOT}/Tools/Materials/make_wood_fibre.py").read())
wood = unreal.load_asset("/Game/Environment/Story/M_CarbonisedWood")
exec(open(f"{ROOT}/Tools/Materials/make_weathered_tent.py").read())
weathered_tent = unreal.load_asset("/Game/Environment/Story/M_WeatheredTent")

# ---------------------------------------------------------------- story clues
SC_ = unreal.load_class(None, "/Script/YggdrasilsPeak.StoryClue")
def clue(cid, title, body, mesh, loc, rot, scale, label, material=None):
    c = eas.spawn_actor_from_class(SC_, loc, rot)
    c.set_editor_property("clue_id", cid)
    c.set_editor_property("title", title)
    c.set_editor_property("body", body)
    mc = c.get_editor_property("mesh")
    if mesh:
        mc.set_static_mesh(mesh)
        mc.set_world_scale3d(unreal.Vector(scale, scale, scale))
        if material:
            for i in range(mesh.get_num_sections(0) if hasattr(mesh, "get_num_sections") else 1):
                mc.set_material(i, material)
    tag(c, label, "Route/Story")
    return c

# 1. the parents' abandoned camp, on a spur beside the Gorak Shep -> Base Camp trail (Ch. 2, [Established])
p, n, yaw = on_route(1, 0.62, 35.0)
tent = unreal.load_asset("/Game/Fab/Modern_Camping_Tent/modern_camping_tent__GLTF_/StaticMeshes/modern_camping_tent__GLTF_")
clue("ParentsCamp_Journal1", "A torn tent, twenty years old",
     "Two sleeping bags, frozen stiff. A stove with the fuel line cut. Under a stone, a notebook in Mother's hand:\n\n"
     "\"Day 31. The village spat at our feet when we left. Sangye would not look at us. Maybe he is right - "
     "maybe it is madness. But the needle keeps pointing up, and Father says the miracle is real. If it can stop "
     "what is coming, then the climb is worth everything. Even this.\"\n\n"
     "The last line is pressed so hard the pencil tore the paper.",
     tent, unreal.Vector(p.x, p.y, p.z - 35), unreal.Rotator(roll=14, pitch=-11, yaw=yaw + 70), 1.0, "Clue_ParentsCamp", weathered_tent)
# (dome tent tipped over by twenty years of wind and half buried in the moraine)
# their things around the tent: (mesh, metres along / across from the tent, yaw, roll, scale)
camp_props = [("/Game/Fab/Oxygen_Tank/oxygen_tank/StaticMeshes/oxygen_tank", 2.4, -1.2, 30, 90, 1.0),
              ("/Game/Fab/Megascans/3D/Bundled_Rope_Coil_vhnmbdcs/Raw/vhnmbdcs_tier_0/StaticMeshes/vhnmbdcs_tier_0", -1.8, 1.6, 0, 0, 1.0),
              ("/Game/Props/Supplies/SM_Thermos", 1.6, 1.9, 75, 90, 1.0),
              ("/Game/Props/Supplies/SM_Ration", 2.1, 0.6, 140, 0, 1.0),
              ("/Game/Props/Supplies/SM_Ration", -0.8, -2.3, 200, 180, 1.0)]
fwd = unreal.Vector(math.cos(math.radians(yaw)), math.sin(math.radians(yaw)), 0)
side = unreal.Vector(-fwd.y, fwd.x, 0)
for k, (mp, al, ac, py, pr, sc_) in enumerate(camp_props):
    x = p.x + (fwd.x * al + side.x * ac) * 100; y = p.y + (fwd.y * al + side.y * ac) * 100
    g, gn = ground(x, y, p.z)
    a_ = eas.spawn_actor_from_object(unreal.load_asset(mp), unreal.Vector(g.x, g.y, g.z - 3), unreal.Rotator(roll=pr, pitch=0, yaw=yaw + py))
    a_.set_actor_scale3d(unreal.Vector(sc_, sc_, sc_))
    a_.get_component_by_class(unreal.StaticMeshComponent).set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
    tag(a_, f"ParentsCamp_Prop{k}", "Route/Story")
print("parents' camp at", p)

# 2. carbonised wood fibre where the Icefall rock breaks (Ch. 2 discovery, [Established]): a broken trunk segment
#    standing out of the ice (~4 x 4 m, ~13 m showing) and a toppled piece beside it: the closed cliff-rock scan
#    stretched tall gives the broken, fibrous silhouette; M_CarbonisedWood runs the grain vertically.
p, n, yaw = on_route(3, 0.35, 14.0)
rock = unreal.load_asset("/Game/Fab/Alaskan_Cliff_Rock_1_Free/alaskan_cliff_rock_1_free/StaticMeshes/alaskan_cliff_rock_1_free")
c = clue("WoodFibre", "This is not rock",
     "Where the ice has torn the slope open, the broken face is not granite. It is black and fibrous, split along a "
     "grain as thick as your arm - wood, burnt to charcoal and hard as iron, running straight down into the mountain.\n\n"
     "No tree ever grew this high. No tree was ever this big.",
     rock, unreal.Vector(p.x, p.y, p.z), unreal.Rotator(roll=0, pitch=6, yaw=yaw + 90), 1.0, "Clue_WoodFibre", wood)
mc = c.get_editor_property("mesh")
mc.set_world_scale3d(unreal.Vector(8.0, 8.0, 25.0))
mc.set_relative_location(unreal.Vector(0, 0, -400), False, True)   # the trunk sunk, the read trigger at the surface
c.set_editor_property("read_radius", 1100.0)                              # ~7 m trunk radius + a few steps
# the toppled piece lies on the far side of the trunk from the trail (it used to sit on the rope line itself -
# found by verify_route_corridor.py), roughly along the slope
pf, _, _ = on_route(3, 0.355, 25.0)
fallen = eas.spawn_actor_from_object(rock, unreal.Vector(pf.x, pf.y, pf.z - 80), unreal.Rotator(roll=0, pitch=78, yaw=yaw + 15))
fallen.set_actor_scale3d(unreal.Vector(5.0, 5.0, 16.0))
fallen.get_component_by_class(unreal.StaticMeshComponent).set_material(0, wood)
tag(fallen, "WoodFibre_Fallen", "Route/Story")
print("wood fibre at", p)

# 3. journal page 2 at the South Col (Phase1_Design_Plan evidence trail, main route, sets up Ending 3): the
#    parents' high camp, torn flat by the wind, beside the abandoned oxygen depot
p, n, yaw = on_route(6, 0.96, 28.0)
clue("SouthCol_Journal2", "The second page",
     "Pinned under an empty oxygen bottle, the paper gone soft as cloth. Father's hand this time, shaking:\n\n"
     "\"We understand now. The miracle is not given, it is paid for. Every light in the veins was a person once - "
     "climbers, pilgrims, whoever came close enough. The tree does not save the world. It feeds on it.\n\n"
     "She says we go on anyway. If we can reach the heart of it we can stop it taking more. If we do not come "
     "back - Sangye, do not come looking.\"\n\n"
     "Under it, in Mother's hand, one line: \"He will come.\"",
     tent, unreal.Vector(p.x, p.y, p.z - 45), unreal.Rotator(roll=-8, pitch=6, yaw=yaw + 40), 0.9, "Clue_SouthColPage", weathered_tent)
depot = [("/Game/Fab/Oxygen_Tank/oxygen_tank/StaticMeshes/oxygen_tank", 2.0, -1.0, 20, 90), ("/Game/Fab/Oxygen_Tank/oxygen_tank/StaticMeshes/oxygen_tank", 2.4, -0.5, 75, 90),
         ("/Game/Fab/Oxygen_Tank/oxygen_tank/StaticMeshes/oxygen_tank", 1.7, -1.6, 140, 90), ("/Game/Fab/Oxygen_Tank/oxygen_tank/StaticMeshes/oxygen_tank", -1.5, 1.8, 10, 0),
         ("/Game/Fab/Megascans/3D/Bundled_Rope_Coil_vhnmbdcs/Raw/vhnmbdcs_tier_0/StaticMeshes/vhnmbdcs_tier_0", -2.0, -1.2, 0, 0)]
fwd = unreal.Vector(math.cos(math.radians(yaw)), math.sin(math.radians(yaw)), 0); side = unreal.Vector(-fwd.y, fwd.x, 0)
for k, (mp, al, ac, py, pr) in enumerate(depot):
    x = p.x + (fwd.x * al + side.x * ac) * 100; y = p.y + (fwd.y * al + side.y * ac) * 100
    g, gn = ground(x, y, p.z)
    a_ = eas.spawn_actor_from_object(unreal.load_asset(mp), unreal.Vector(g.x, g.y, g.z - 3), unreal.Rotator(roll=pr, pitch=0, yaw=yaw + py))
    a_.get_component_by_class(unreal.StaticMeshComponent).set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
    tag(a_, f"SouthCol_Depot{k}", "Route/Story")
print("south col page at", p)
unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
print("done")
