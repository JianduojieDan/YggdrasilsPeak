# Runs INSIDE the editor on base_map. Builds Chomolungma chapter 1's places and props, all tagged "Ch1" (re-running first
# removes the old ones): the family room at Dingboche, new expedition tents in place of the old camp tents (hidden, not
# deleted), the ice hole on the trail, the Thukla memorials (cairns from the project's photogrammetry stones, prayer-flag
# lines, the father's engraved stone, his khata), the night camp, the golden plume over the summit. Every position is
# traced onto the real ground. Writes Content/Story/Chapter1_places.ygs for the story file.
import unreal, json, math, random
random.seed(14)
ROOT = "F:/UE_project/YggdrasilsPeak"
EAS = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
W = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
if W.get_name() != "base_map":
    unreal.EditorLoadingAndSavingUtils.load_map("/Game/LevelPrototyping/base_map")
    W = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
assert W.get_name() == "base_map", W.get_name()
V = unreal.Vector; R = unreal.Rotator
MD = "/Game/Story/Materials"
MI = lambda k: unreal.load_asset(f"{MD}/MI_Story_{k}")
CUBE = unreal.load_asset("/Engine/BasicShapes/Cube"); CYL = unreal.load_asset("/Engine/BasicShapes/Cylinder"); PLANE = unreal.load_asset("/Engine/BasicShapes/Plane")
DOME = unreal.load_asset("/Game/Story/Meshes/SM_Tent_Dome/StaticMeshes/SM_Tent_Dome")
MESS = unreal.load_asset("/Game/Story/Meshes/SM_Tent_Mess/StaticMeshes/SM_Tent_Mess")
KHATA = unreal.load_asset("/Game/Story/Meshes/SM_Khata/StaticMeshes/SM_Khata")
SLAB = unreal.load_asset("/Game/Fab/FREE_Stone_-_8K_Textures/stone/StaticMeshes/stone")
ROCK = unreal.load_asset("/Game/Fab/Alaskan_Cliff_Rock_1_Free/alaskan_cliff_rock_1_free/StaticMeshes/alaskan_cliff_rock_1_free")
FLAG = unreal.load_asset("/Game/Environment/Landmarks/SM_PrayerFlag")
FLAGMI = [unreal.load_asset(f"/Game/Environment/Landmarks/MI_PrayerFlag_{c}") for c in ("Blue", "White", "Red", "Green", "Yellow")]
PLUME = unreal.load_asset(f"{MD}/M_StoryPlume")

# ---------------------------------------------------------------- clean up a previous run
for a in EAS.get_all_level_actors():
    if a.actor_has_tag("Ch1"):
        EAS.destroy_actor(a)
places = {}

def is_terrain(actor, comp):
    if actor is None: return False
    if isinstance(actor, unreal.Landscape) or isinstance(actor, unreal.LandscapeProxy) or "Landscape" in actor.get_class().get_name(): return True
    if isinstance(comp, unreal.StaticMeshComponent) and comp.static_mesh and "/DetailTiles/" in comp.static_mesh.get_path_name(): return True
    return False

def ground(x, y, z_hint=0.0):
    hits = unreal.SystemLibrary.line_trace_multi(W, V(x, y, z_hint + 30000), V(x, y, z_hint - 30000), unreal.TraceTypeQuery.TRACE_TYPE_QUERY1,
                                                  False, [], unreal.DrawDebugTrace.NONE, True)
    for h in hits or []:
        t = h.to_tuple()
        if t[1]: continue                                     # started inside something: not the ground
        if is_terrain(t[9], t[10]): return t[5], t[7]       # impact point, impact normal
    for h in hits or []:
        t = h.to_tuple()
        if not t[1]: return t[5], t[7]
    print("NO GROUND at", round(x), round(y)); return V(x, y, z_hint), V(0, 0, 1)

def spawn(mesh, loc, rot=R(0, 0, 0), scale=(1, 1, 1), mats=None, tags=(), folder="Story/Ch1", shadow=True, collide=True, hidden=False):
    a = EAS.spawn_actor_from_object(mesh, loc, rot)
    a.set_actor_scale3d(V(*scale))
    c = a.static_mesh_component
    if mats:
        for i, m in enumerate(mats if isinstance(mats, (list, tuple)) else [mats]): c.set_material(i, m)
    c.set_editor_property("cast_shadow", shadow)
    if not collide: c.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
    a.tags = ["Ch1"] + list(tags)
    a.set_folder_path(folder)
    if hidden: a.set_actor_hidden_in_game(True)
    return a

def light(cls, loc, rot, intensity, color, radius, tags=(), shadows=True, folder="Story/Ch1"):
    a = EAS.spawn_actor_from_class(cls, loc, rot)
    lc = a.light_component
    lc.set_editor_property("intensity", intensity)
    lc.set_editor_property("light_color", unreal.Color(*color))
    lc.set_editor_property("attenuation_radius", radius)
    lc.set_editor_property("cast_shadows", shadows)
    a.tags = ["Ch1"] + list(tags); a.set_folder_path(folder)
    return a

def place(name, loc, yaw=0.0):
    places[name] = [round(loc.x, 1), round(loc.y, 1), round(loc.z, 1), round(yaw, 1)]

def local(o, yaw, x, y, z):
    c, s = math.cos(math.radians(yaw)), math.sin(math.radians(yaw))
    return V(o.x + x * c - y * s, o.y + x * s + y * c, o.z + z)

def box(o, yaw, x, y, z, sx, sy, sz, mi, folder="Story/Ch1/Room", extra_yaw=0.0, pitch=0.0, roll=0.0, tags=()):
    return spawn(CUBE, local(o, yaw, x, y, z), R(roll, pitch, yaw + extra_yaw), (sx / 100.0, sy / 100.0, sz / 100.0), MI(mi), tags, folder)

def cyl(o, yaw, x, y, z, d, h, mi, folder="Story/Ch1/Room", pitch=0.0, tags=(), shadow=True):
    return spawn(CYL, local(o, yaw, x, y, z), R(0, pitch, yaw), (d / 100.0, d / 100.0, h / 100.0), MI(mi), tags, folder, shadow=shadow)

def flag_line(p0, p1, n, size=(28.0, 34.0), folder="Story/Ch1/Flags", sag=0.12):
    d = p1 - p0; L = d.length(); yaw = math.degrees(math.atan2(d.y, d.x))
    mid = (p0 + p1) * 0.5
    cord = spawn(CYL, mid - V(0, 0, L * sag * 0.6), unreal.MathLibrary.make_rot_from_z(d), (0.012, 0.012, L / 100.0), MI("Cord"), (), folder, shadow=False, collide=False)
    for i in range(n):
        t = (i + 0.5) / n
        p = p0 + d * t - V(0, 0, 4 * sag * L * t * (1 - t))
        spawn(FLAG, p - V(0, 0, size[1] * 0.5), R(90, 0, yaw + 90), (size[0] / 100.0, size[1] / 100.0, 1), FLAGMI[i % 5], (), folder, collide=False)

def route(leg):
    data = json.load(open(f"{ROOT}/Tools/RoutePlanning/route.json"))
    pts = data[leg]["pts"]; out, s = [], 0.0
    for i, p in enumerate(pts):
        if i: s += math.dist(p[:2], pts[i - 1][:2])
        out.append((s, p))
    return out

def at_dist(rt, metres):
    target = metres * 100.0
    for i in range(1, len(rt)):
        if rt[i][0] >= target:
            (s0, p0), (s1, p1) = rt[i - 1], rt[i]
            k = (target - s0) / max(1e-3, s1 - s0)
            x = p0[0] + (p1[0] - p0[0]) * k; y = p0[1] + (p1[1] - p0[1]) * k
            yaw = math.degrees(math.atan2(p1[1] - p0[1], p1[0] - p0[0]))
            return x, y, p0[2], yaw
    p = rt[-1][1]; return p[0], p[1], p[2], 0.0

def on_route(rt, metres, side=0.0, ahead=0.0):
    x, y, z, yaw = at_dist(rt, metres)
    c, s = math.cos(math.radians(yaw)), math.sin(math.radians(yaw))
    x += c * ahead - s * side; y += s * ahead + c * side
    g, n = ground(x, y, z)
    return g, yaw, n

# ---------------------------------------------------------------- Dingboche: hide the old tents, put expedition tents there
CAMP = V(339710, 2469225, -69212)
cols = ["TentYellow", "TentOrange", "TentRed"]
k = 0
for a in EAS.get_all_level_actors():
    if isinstance(a, unreal.StaticMeshActor):
        m = a.static_mesh_component.static_mesh
        if m and "tent" in m.get_name().lower() and (a.get_actor_location() - CAMP).length() < 8000:
            loc = a.get_actor_location(); yaw = a.get_actor_rotation().yaw
            a.set_actor_hidden_in_game(True); a.set_is_temporarily_hidden_in_editor(True)
            a.static_mesh_component.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
            if "Ch1_OldTent" not in [str(t) for t in a.tags]: a.tags = list(a.tags) + ["Ch1_OldTent"]
            g, n = ground(loc.x, loc.y, loc.z)
            spawn(DOME, g + V(0, 0, 2), R(0, 0, yaw), (1, 1, 1), [MI(cols[k % 3]), MI("Pole"), MI("Cord")], ("camp_tent",), "Story/Ch1/Camp")
            place(f"camp_tent_{k}", g, yaw); k += 1
print("camp tents replaced", k)

def flattest(center, rmin, rmax, half, avoid, tries=90):
    best = None
    for i in range(tries):
        ang = random.uniform(0, 2 * math.pi); r = random.uniform(rmin, rmax)
        x, y = center.x + math.cos(ang) * r, center.y + math.sin(ang) * r
        if any(math.dist((x, y), (q.x, q.y)) < q.z for q in avoid): continue
        zs = []
        for dx in (-half[0], 0, half[0]):
            for dy in (-half[1], 0, half[1]):
                g, n = ground(x + dx, y + dy, center.z); zs.append(g.z)
        spread = max(zs) - min(zs)
        if best is None or spread < best[0]: best = (spread, x, y, min(zs), max(zs))
    return best

avoid = [V(CAMP.x, CAMP.y, 1200)] + [V(p[0], p[1], 900) for n_, p in places.items() if n_.startswith("camp_tent")]
spread, mx, my, zmin, zmax = flattest(CAMP, 1000, 2600, (300, 250), avoid)
g, n = ground(mx, my, CAMP.z)
myaw = math.degrees(math.atan2(CAMP.y - my, CAMP.x - mx))
spawn(MESS, V(mx, my, zmin + 2), R(0, 0, myaw), (1, 1, 1), [MI("TentBlue"), MI("Pole"), MI("Cord")], ("camp_mess",), "Story/Ch1/Camp")
place("camp_mess", V(mx, my, g.z), myaw)
print("mess tent spread", round(spread))


def clear_snow_props(center, radius):
    """removes baked snow piles / drift mounds (instances) that would stand inside a room or around a person"""
    n = 0
    for a in EAS.get_all_level_actors():
        for comp in a.get_components_by_class(unreal.InstancedStaticMeshComponent):
            m = comp.static_mesh
            if not m: continue
            nm = (m.get_name() + " " + a.get_name()).lower()
            if not any(k in nm for k in ("snow", "drift", "vcytca0fa", "pile", "mound")): continue
            idx = comp.get_instances_overlapping_sphere(center, radius, True)
            for i in sorted(idx, reverse=True):
                comp.remove_instance(i); n += 1
    return n

# ---------------------------------------------------------------- the family room (one walk-in Sherpa room)
avoid.append(V(mx, my, 1000))
spread, rx, ry, rzmin, rzmax = flattest(CAMP, 1800, 4200, (330, 260), avoid, 140)
print("snow props cleared at the room", clear_snow_props(V(rx, ry, rzmax), 700))
O = V(rx, ry, rzmax + 165)                 # on a stone plinth: the visible detail tile lies ~1-1.5 m above the landscape collision here                  # floor clear of the drawn snow (it lies ~30-60 cm above its collision here)
RY = math.degrees(math.atan2(CAMP.y - ry, CAMP.x - rx))   # the door (+x) looks at the camp
print("room spread", round(spread), "at", rx, ry)
L_, W_, H_, T_ = 600.0, 460.0, 240.0, 40.0
iL, iW = L_ - 2 * T_, W_ - 2 * T_
# floor, footing, ceiling
box(O, RY, 0, 0, -160, L_ + 60, W_ + 60, 320, "Stone")
for i in range(5):
    box(O, RY, L_ / 2 + 55 + i * 42, 0, -i * 34 - 17 - 160 + 160 - 0, 44, 160, 34 + i * 0, "Stone")   # steps down from the door
box(O, RY, 0, 0, 5, iL, iW, 10, "WoodDark")
box(O, RY, 0, 0, H_ + 6, iL, iW, 12, "WoodDark")
for bx in (-180, -60, 60, 180):
    box(O, RY, bx, 0, H_ - 8, 18, iW, 16, "Wood")
# back wall (-x), side walls, front wall with a door, a window in the +y wall
box(O, RY, -(L_ - T_) / 2, 0, H_ / 2, T_, W_, H_, "Stone")
box(O, RY, 0, -(W_ - T_) / 2, H_ / 2, L_, T_, H_, "Stone")
wx, ww, wz, wh = 80.0, 90.0, 100.0, 70.0    # window centre x, width, sill, height
yw = (W_ - T_) / 2
for (x0, x1) in ((-L_ / 2, wx - ww / 2), (wx + ww / 2, L_ / 2)):
    box(O, RY, (x0 + x1) / 2, yw, H_ / 2, x1 - x0, T_, H_, "Stone")
box(O, RY, wx, yw, wz / 2, ww, T_, wz, "Stone")
box(O, RY, wx, yw, (wz + wh + H_) / 2, ww, T_, H_ - wz - wh, "Stone")
box(O, RY, wx, yw - T_ / 2 + 4, wz + wh / 2, ww + 8, 6, 8, "Wood"); box(O, RY, wx, yw - T_ / 2 + 4, wz, ww + 10, 10, 6, "Wood")
dw, dh = 100.0, 190.0
xf = (L_ - T_) / 2
for (y0, y1) in ((-W_ / 2, -dw / 2), (dw / 2, W_ / 2)):
    box(O, RY, xf, (y0 + y1) / 2, H_ / 2, T_, y1 - y0, H_, "Stone")
box(O, RY, xf, 0, (dh + H_) / 2, T_, dw, H_ - dh, "Stone")
box(O, RY, xf + T_ / 2 + 3, 0, dh + 4, 8, dw + 20, 10, "Wood")                     # lintel
# roof: blue tin, shallow gable along x, eaves over the walls; gable ends filled with timber
pitch = 14.0; half = W_ / 2 + 40
rise = math.tan(math.radians(pitch)) * half
for sgn in (-1, 1):
    box(O, RY, 0, sgn * half / 2, H_ + 20 + rise / 2, L_ + 70, half / math.cos(math.radians(pitch)) + 6, 5, "TinBlue", roll=-sgn * pitch)
for sgn in (-1, 1):
    box(O, RY, sgn * (L_ / 2 - 6), 0, H_ + 14 + rise * 0.35, 10, W_ - 40, rise * 0.7, "WoodDark")
# stove, pipe, kettle, the ember glow
SX, SY = -95.0, 95.0
cyl(O, RY, SX, SY, 0, 62, 62, "Iron"); cyl(O, RY, SX, SY, 62, 66, 4, "Iron")
cyl(O, RY, SX - 12, SY, 66, 12, H_ + 70 - 66, "Iron")
cyl(O, RY, SX + 14, SY + 10, 66, 22, 18, "Copper")
box(O, RY, SX + 31, SY, 20, 4, 26, 16, "Ember", tags=("ember",))
box(O, RY, 20, -20, 11, 180, 120, 2, "Rug")
light(unreal.PointLight, local(O, RY, SX + 60, SY, 45), R(0, 0, 0), 420.0, (255, 140, 70, 255), 700, ("roomlight",))
# benches with rugs, low tables
for sgn in (-1, 1):
    box(O, RY, 10 + (60 if sgn > 0 else 0), sgn * (iW / 2 - 32), 21, iL - 120 - (120 if sgn > 0 else 0), 64, 42, "Wood")
    box(O, RY, 10 + (60 if sgn > 0 else 0), sgn * (iW / 2 - 32), 44, iL - 140 - (120 if sgn > 0 else 0), 58, 4, "Rug")
    box(O, RY, 40, sgn * (iW / 2 - 100), 22, 110, 55, 44, "WoodDark")
# altar cabinet on the back wall, butter lamps, thangka, the photo wall, a khata over the middle photo
AX = -(iL / 2) + 24
box(O, RY, AX, 0, 60, 46, 220, 120, "Wood")
box(O, RY, AX + 2, 0, 121, 50, 226, 4, "WoodDark")
for i, y in enumerate((-60, -30, 0, 30, 60)):
    cyl(O, RY, AX + 8, y, 123, 7, 6, "Copper"); cyl(O, RY, AX + 8, y, 129, 2.4, 3.5, "Flame", shadow=False)
light(unreal.PointLight, local(O, RY, AX + 40, 0, 140), R(0, 0, 0), 90.0, (255, 170, 90, 255), 360, ("roomlight",))
spawn(PLANE, local(O, RY, -(iL / 2) + 1.5, 0, 185), R(90, 0, RY - 90), (0.7, 1.0, 1), MI("Thangka"), (), "Story/Ch1/Room", collide=False)
photos = [("Photo_Khumbu", -78), ("Photo_BaseCamp", 78), ("Photo_Valley", -78 - 10)]
spawn(PLANE, local(O, RY, -(iL / 2) + 1.5, -78, 160), R(90, 0, RY - 90), (0.42, 0.32, 1), MI("Photo_Khumbu"), ("photo_wall",), "Story/Ch1/Room", collide=False)
spawn(PLANE, local(O, RY, -(iL / 2) + 1.5, 78, 160), R(90, 0, RY - 90), (0.42, 0.32, 1), MI("Photo_BaseCamp"), ("photo_wall",), "Story/Ch1/Room", collide=False)
spawn(KHATA, local(O, RY, -(iL / 2) + 9, 0, 236), R(0, 0, RY + 90), (0.55, 0.55, 0.55), MI("Silk"), (), "Story/Ch1/Room", collide=False)
# copper pots on a wall shelf (+y wall, by the back), a water drum, the father's duffel and photo on the bench
box(O, RY, -150, iW / 2 - 16, 160, 150, 30, 4, "WoodDark")
for i, (x, d, h) in enumerate(((-205, 30, 26), (-175, 24, 20), (-150, 34, 30), (-122, 20, 16), (-100, 26, 22))):
    cyl(O, RY, x, iW / 2 - 18, 162, d, h, "Copper")
cyl(O, RY, 150, iW / 2 - 40, 0, 50, 70, "Copper")
cyl(O, RY, 130, -(iW / 2 - 34), 46 + 20, 40, 78, "TentOrange", pitch=90.0, tags=("duffel",))
spawn(PLANE, local(O, RY, 70, -(iW / 2 - 40), 47), R(0, 0, RY + 12), (0.14, 0.105, 1), MI("Photo_Icefall"), ("photo_father",), "Story/Ch1/Room", collide=False)
# dawn through the window, a soft fill so the room reads
light(unreal.SpotLight, local(O, RY, wx, yw + 260, wz + 150), R(0, -28, RY - 90), 140.0, (255, 214, 180, 255), 900, ("roomlight",))
light(unreal.PointLight, local(O, RY, 0, 0, H_ - 40), R(0, 0, 0), 25.0, (255, 210, 170, 255), 520, ("roomlight",), shadows=False)
# inside only: a warm, dim, smoky room (sky light floods in through the door and window otherwise)
ppv = EAS.spawn_actor_from_class(unreal.PostProcessVolume, local(O, RY, 0, 0, H_ / 2), R(0, 0, RY))
ppv.set_actor_scale3d(V(iL / 200.0, iW / 200.0, H_ / 200.0))
ppv.set_editor_property("priority", 20.0); ppv.set_editor_property("blend_radius", 60.0); ppv.set_editor_property("unbound", False)
ps = ppv.get_editor_property("settings")
for k, v in (("override_auto_exposure_bias", True), ("auto_exposure_bias", -1.3), ("override_white_temp", True), ("white_temp", 4300.0),
             ("override_indirect_lighting_intensity", True), ("indirect_lighting_intensity", 0.35),
             ("override_color_saturation", True), ("color_saturation", unreal.Vector4(0.9, 0.9, 0.9, 1.0))):
    try: ps.set_editor_property(k, v)
    except Exception as e: print("ppv", k, e)
ppv.set_editor_property("settings", ps)
ppv.tags = ["Ch1"]; ppv.set_folder_path("Story/Ch1/Room")
# prayer flags from the roof corner to a pole by the door
pole = local(O, RY, L_ / 2 + 380, W_ / 2 + 120, 0); pg, pn = ground(pole.x, pole.y, O.z)
spawn(CYL, pg + V(0, 0, 230), R(0, 0, 0), (0.08, 0.08, 4.6), MI("WoodDark"), (), "Story/Ch1/Room")
flag_line(local(O, RY, L_ / 2, W_ / 2, H_ + 20), pg + V(0, 0, 440), 14, folder="Story/Ch1/Room")
place("room_start", local(O, RY, 175, -120, 10), RY + 168)
place("room_photos", local(O, RY, -(iL / 2), 0, 165), 0)
place("room_duffel", local(O, RY, 130, -(iW / 2 - 34), 60), 0)
place("room_door", local(O, RY, xf - 60, 0, 0), RY)
place("room_out", local(O, RY, xf + 240, 0, 0), RY)
dg, dn = ground(local(O, RY, xf + 300, 120, 0).x, local(O, RY, xf + 300, 120, 0).y, O.z)
place("dolma_spot", dg, RY + 180)
ag, an = ground(local(O, RY, xf + 340, -110, 0).x, local(O, RY, xf + 340, -110, 0).y, O.z)
place("aunt_spot", ag, RY + 180)

# ---------------------------------------------------------------- the trail (leg 0) and its events
r0 = route(0)
for name, m, side in (("trail_start", 60, 0), ("ev_climb", 220, 0), ("ev_stream", 900, 0), ("ev_elena", 1700, 0), ("ev_talk", 1850, 0),
                      ("ev_porters", 2650, 0), ("porter_start", 2780, 1.5), ("porter_end", 2450, 1.5), ("ev_cloud", 3350, 0),
                      ("ev_alone_end", 4300, 0), ("ev_flags_seen", 5450, 0), ("night_camp", 5650, 18), ("memorial", 5800, 0)):
    g, yaw, n = on_route(r0, m, side * 100)
    place(name, g, yaw)
# meeting points with the team at camp (baked snow mounds cleared around them)
for name, (dx, dy) in (("nima_spot", (900, 300)), ("marek_spot", (1100, -200)), ("elena_spot", (1250, 60)), ("jiwoo_spot", (1150, 380))):
    g, n = ground(CAMP.x - dx * 0.3 + dy * 0.2, CAMP.y - dy * 0.3 + dx * 0.15, CAMP.z)
    place(name, g, 0)
    clear_snow_props(g, 350)
for nm in ("dolma_spot", "aunt_spot", "room_out"):
    p = places[nm]; clear_snow_props(V(p[0], p[1], p[2]), 300)
# the ice hole by the stream, 3 m off the trail
g, yaw, n = on_route(r0, 900, 300)
spawn(CYL, g + V(0, 0, -2), R(0, 0, yaw), (1.7, 1.2, 0.06), MI("SnowWhite"), (), "Story/Ch1/Trail")
spawn(CYL, g + V(0, 0, -0.5), R(0, 0, yaw), (1.0, 0.65, 0.03), MI("Water"), ("stream",), "Story/Ch1/Trail", shadow=False)
place("stream_hole", g, yaw)

# ---------------------------------------------------------------- the memorials (Thukla pass, ~4,840 m on our trail)
mg, myaw_, mn = on_route(r0, 5800, 0)
c, s = math.cos(math.radians(myaw_)), math.sin(math.radians(myaw_))
cairn_tops = []
for i in range(22):
    for attempt in range(8):
        along = random.uniform(-2600, 2600); side = random.choice((-1, 1)) * random.uniform(500, 3200)
        x = mg.x + c * along - s * side; y = mg.y + s * along + c * side
        g, n = ground(x, y, mg.z)
        if n.z > 0.86: break
    base = 0.55 + random.random() * 0.45
    z = g.z; layers = random.randint(7, 13); yaw0 = random.uniform(0, 360)
    rock = spawn(ROCK, V(x, y, z + 20 * base), R(random.uniform(-6, 6), random.uniform(-6, 6), yaw0), (base * 1.1, base * 1.1, base * 0.7), None, ("memorial",), "Story/Ch1/Memorial")
    z += 70 * base * 0.7
    for L in range(layers):
        k = 1.0 - L / (layers + 2)
        sc = (2.2 * k * base * random.uniform(0.85, 1.15))
        spawn(SLAB, V(x + random.uniform(-6, 6), y + random.uniform(-6, 6), z), R(random.uniform(-5, 5), random.uniform(-5, 5), random.uniform(0, 360)),
              (sc, sc, sc * random.uniform(1.0, 1.6)), None, ("memorial",), "Story/Ch1/Memorial")
        z += 7.9 * sc * 1.25
    cairn_tops.append(V(x, y, z))
    if random.random() < 0.45:
        spawn(CYL, V(x, y, z + 110), R(0, 0, 0), (0.05, 0.05, 2.2), MI("WoodDark"), (), "Story/Ch1/Memorial")
        cairn_tops[-1] = V(x, y, z + 215)
for i in range(len(cairn_tops) - 1):
    a, b = cairn_tops[i], cairn_tops[i + 1]
    if (a - b).length() < 2600 and random.random() < 0.7:
        flag_line(a, b, max(5, int((a - b).length() / 55)), folder="Story/Ch1/Memorial")
# the father's stone: new, sharp-edged, close to the trail, with his name; his khata appears when you tie it
fg, fyaw, fn = on_route(r0, 5830, 380)
face = myaw_ + 90 + 180      # faces the trail
st = spawn(ROCK, fg + V(0, 0, 25), R(0, 0, face), (0.8, 0.36, 0.85), None, ("father_stone",), "Story/Ch1/Memorial")
tr = EAS.spawn_actor_from_class(unreal.TextRenderActor, fg + V(0, 0, 62) + V(math.cos(math.radians(face)) * 20, math.sin(math.radians(face)) * 30, 0), R(0, 0, face))
t = tr.text_render
t.set_editor_property("text", "MINGMA SHERPA\n1978 - 2026")
t.set_editor_property("world_size", 9.0); t.set_editor_property("horizontal_alignment", unreal.HorizTextAligment.EHTA_CENTER)
t.set_editor_property("text_render_color", unreal.Color(40, 38, 36, 255))
tr.tags = ["Ch1", "father_name"]; tr.set_folder_path("Story/Ch1/Memorial")
spawn(KHATA, fg + V(0, 0, 108), R(0, 0, face + 90), (0.6, 0.6, 0.6), MI("Silk"), ("khata_on_stone",), "Story/Ch1/Memorial", collide=False, hidden=True)
place("father_stone", fg, face)
kg, kyaw, kn = on_route(r0, 5830, 240)
place("kneel_spot", kg, face + 180)

# ---------------------------------------------------------------- night camp just below the memorials
ng, nyaw, nn = on_route(r0, 5650, 1800)
spawn(DOME, ng + V(0, 0, 2), R(0, 0, nyaw + 90), (1, 1, 1), [MI("TentOrange"), MI("Pole"), MI("Cord")], ("night_tent",), "Story/Ch1/Night")
n2 = local(ng, nyaw, 420, 260, 0); g2, _ = ground(n2.x, n2.y, ng.z)
spawn(DOME, g2 + V(0, 0, 2), R(0, 0, nyaw + 60), (1, 1, 1), [MI("TentYellow"), MI("Pole"), MI("Cord")], (), "Story/Ch1/Night")
n3 = local(ng, nyaw, -380, 300, 0); g3, _ = ground(n3.x, n3.y, ng.z)
spawn(DOME, g3 + V(0, 0, 2), R(0, 0, nyaw + 120), (1, 1, 1), [MI("TentRed"), MI("Pole"), MI("Cord")], (), "Story/Ch1/Night")
st0 = local(ng, nyaw, 150, -260, 0); sg, _ = ground(st0.x, st0.y, ng.z)
cyl(sg, 0, 0, 0, 0, 34, 22, "Iron", folder="Story/Ch1/Night"); cyl(sg, 0, 0, 0, 22, 20, 16, "Copper", folder="Story/Ch1/Night")
cyl(sg, 0, 0, 0, 8, 22, 6, "Ember", folder="Story/Ch1/Night", shadow=False)
light(unreal.PointLight, sg + V(0, 0, 50), R(0, 0, 0), 30.0, (255, 130, 60, 255), 900, ("nightfire",), folder="Story/Ch1/Night")
place("night_tent", local(ng, nyaw, 0, -160, 0), nyaw + 90)
place("night_stove", sg, 0)
place("night_nima", local(ng, nyaw, 120, -150, 0), nyaw - 90)

# ---------------------------------------------------------------- prologue: the Balcony (leg 7) and the summit
r7 = route(7)
for name, m, side in (("balcony_start", 1380, 0), ("father_sit", 1420, -260), ("slip_spot", 1412, 0)):
    g, yaw, n = on_route(r7, m, side * 1.0)
    place(name, g, yaw)
sx_, sy_, sz_, syaw = at_dist(r7, 99999)
sg_, sn_ = ground(sx_, sy_, sz_)
place("summit", sg_, 0)
# the golden plume, streaming east off the summit (invisible until its Glow rises)
d = V(mg.x - sg_.x, mg.y - sg_.y, 0); d = d / d.length()
xax = V(-d.y, d.x, 0)
if xax.x < 0: xax = xax * -1.0
pl = spawn(PLANE, sg_ + xax * 250000 + V(0, 0, 250000), unreal.MathLibrary.make_rot_from_zx(d, xax), (5000, 2000, 1), PLUME, ("plume",), "Story/Ch1/Plume", shadow=False, collide=False)
place("plume_view", sg_ + xax * 150000 + V(0, 0, 250000), 0)

for a in EAS.get_all_level_actors():
    if a.get_actor_label() == "WorldTree_Flags" and "worldtree_flags" not in [str(t) for t in a.tags]:
        a.tags = list(a.tags) + ["worldtree_flags"]; print("tagged WorldTree_Flags")
json.dump(places, open(f"{ROOT}/Content/Story/Chapter1_places.ygs", "w"), indent=1)
unreal.EditorLevelLibrary.save_current_level()
print("places", len(places))
