# Runs INSIDE the editor on base_map. Builds Chomolungma chapter 1's places and props, all tagged "Ch1" (re-running first
# removes the old ones): Sangye's tent at the Dingboche camp (where the chapter opens), the Meshy expedition tents and
# camp kit in place of the old camp tents (hidden, not deleted), the ice hole on the trail, the Thukla memorials (Meshy
# chortens, prayer-flag lines, the father's stone with his name, his khata), the night camp, the golden plume. Every position is
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
# the Meshy models (Tools/Story/import_ai_assets): real size, pivot at the bottom centre
AIINFO = json.load(open(f"{ROOT}/Saved/AIAssets.json"))
def AI(i): return unreal.load_asset(AIINFO[i]["mesh"])
TENT_S = 1.5      # the dome came out 94 cm tall: x1.5 = a roomy 3-person expedition dome, 3.3 m long, 1.4 m tall (user: "tent so small")
SANGYE_S = 1.8    # Sangye's own dome is bigger (2026-11-19: bags/mat/floor/bottle poked through the x1.5 walls). The dome's floor is a
                  # hexagon (side walls straight for |x| < 44 % of the half length, end walls tapering to points at the door and the back),
                  # measured from Art/Meshy/tent_dome.glb: at x1.8 two x0.9 bags side by side keep ~16 cm inside the fabric
_tints = {}
def tint(i, name, rgb):
    """A coloured child of the model's own material (BaseColorFactor multiplies the texture)."""
    key = (i, name)
    if key in _tints: return _tints[key]
    mel = unreal.MaterialEditingLibrary; at_ = unreal.AssetToolsHelpers.get_asset_tools()
    base = AI(i).get_material(0); d = AIINFO[i]["mesh"].rsplit("/StaticMeshes/", 1)[0]
    nm = f"MI_{i}_{name}"
    mi = unreal.load_asset(f"{d}/{nm}") or at_.create_asset(nm, d, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    mel.set_material_instance_parent(mi, base)
    pn = [str(p) for p in mel.get_vector_parameter_names(base)]
    fac = [p for p in pn if "basecolor" in p.lower().replace(" ", "") and "factor" in p.lower()] or [p for p in pn if "color" in p.lower()]
    if fac: mel.set_material_instance_vector_parameter_value(mi, fac[0], unreal.LinearColor(rgb[0], rgb[1], rgb[2], 1.0))
    else: print("no colour parameter on", base.get_name(), pn)
    unreal.EditorAssetLibrary.save_loaded_asset(mi, False)
    _tints[key] = mi; return mi
TENT_COLS = [("orange", (1.0, 0.62, 0.42)), ("yellow", (1.0, 0.9, 0.35)), ("red", (0.95, 0.38, 0.3))]
DUF_COLS = [("yellow", (1, 1, 1)), ("dark", (0.42, 0.44, 0.42)), ("red", (1.25, 0.32, 0.25)), ("orange", (1.2, 0.62, 0.3))]

# ---------------------------------------------------------------- clean up a previous run
for a in EAS.get_all_level_actors():
    if a.actor_has_tag("Ch1"):
        EAS.destroy_actor(a)
_probe = None
try:
    import json as _json
    _probe = _json.load(open("F:/UE_project/YggdrasilsPeak/Content/Story/Chapter1_places.ygs")).get("nima_spot")
except Exception: pass
if _probe:   # refuse to trace before the detail tiles have collision (they cook after the map loads): everything landed 1 m low once
    _h = unreal.SystemLibrary.line_trace_single(W, V(_probe[0], _probe[1], _probe[2] + 3000), V(_probe[0], _probe[1], _probe[2] - 3000),
                                                unreal.TraceTypeQuery.ECC_VISIBILITY, True, [], unreal.DrawDebugTrace.NONE, True)
    _c = _h.to_tuple()[10] if _h else None
    if not (isinstance(_c, unreal.StaticMeshComponent) and _c.static_mesh and "/DetailTiles/" in _c.static_mesh.get_path_name()):
        raise SystemExit("detail tile collision not ready yet (the camp probe hit " + (str(_c.get_name()) if _c else "nothing") + ") - run again in a few seconds")
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
    if hidden:   # collision only: the actor flag alone did not stick (the boxes drew over the house), so hide the component too
        a.set_actor_hidden_in_game(True)
        c.set_visibility(False, False); c.set_hidden_in_game(True, False)
        c.set_editor_property("cast_shadow", False)
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


# ---------------------------------------------------------------- staging checks (Chapter1_Camera_Spec 10.2, 2026-11-19)
# Every assert prints what it measured; failures are collected and listed again at the end (the run still writes the places:
# a failed framing is the director's call, not a reason to lose the build)
ASSERTS = []
def check(name, ok, measured):
    ASSERTS.append((name, bool(ok)))
    print(("ASSERT PASS " if ok else "ASSERT FAIL ") + name + ": " + measured)

def wrap(a): return (a + 180.0) % 360.0 - 180.0
def bearing(a, b): return math.degrees(math.atan2(b.y - a.y, b.x - a.x))
def elev(a, b): return math.degrees(math.atan2(b.z - a.z, math.hypot(b.x - a.x, b.y - a.y)))
def dist2(a, b): return math.hypot(b.x - a.x, b.y - a.y)
def unit(yaw): return V(math.cos(math.radians(yaw)), math.sin(math.radians(yaw)), 0)
def in_band(v, lo, hi): return lo <= v <= hi
def band_miss(v, lo, hi): return max(0.0, lo - v, v - hi)

def los_blocked(a, b, margin=60.0):
    """True when something solid lies between a and b (more than `margin` cm short of b)"""
    h = unreal.SystemLibrary.line_trace_single(W, a, b, unreal.TraceTypeQuery.TRACE_TYPE_QUERY1, True, [], unreal.DrawDebugTrace.NONE, True)
    t = h.to_tuple() if h else None
    return bool(t and t[0] and (t[5] - b).length() > margin)

_FAR = None
def far_z(x, y):
    """the visual far terrain (Saved/FarTerrain/FarRing.glb, a regular grid; actor at the playable centre, no rotation) in cm,
    or None outside it. Inside the playable square it lies 300 m under the landscape, so the traced ground wins there."""
    global _FAR
    if _FAR is None:
        _FAR = False
        try:
            import struct, array
            data = open(f"{ROOT}/Saved/FarTerrain/FarRing.glb", "rb").read()
            jlen = struct.unpack_from("<I", data, 12)[0]
            gl = json.loads(data[20:20 + jlen].decode("utf-8"))
            acc = gl["accessors"][gl["meshes"][0]["primitives"][0]["attributes"]["POSITION"]]
            bv = gl["bufferViews"][acc["bufferView"]]
            o = 20 + jlen + 8 + bv.get("byteOffset", 0) + acc.get("byteOffset", 0)
            pos = array.array("f"); pos.frombytes(data[o:o + acc["count"] * 12])
            n = int(round(math.sqrt(acc["count"])))
            C = 1196616.96                                       # Tools/Terrain/place_far_terrain.py
            x0, y0 = C + acc["min"][0] * 100.0, C + acc["min"][2] * 100.0
            step = (acc["max"][0] - acc["min"][0]) * 100.0 / (n - 1)
            _FAR = (n, pos[1::3], x0, y0, step)                  # glTF (x, Z, y) in metres
            print("far terrain heightfield", n, "x", n, "step", round(step / 100), "m")
        except Exception as e:
            print("far terrain heightfield not read:", e)
    if not _FAR: return None
    n, hz, x0, y0, step = _FAR
    fi, fj = (x - x0) / step, (y - y0) / step
    i, j = int(math.floor(fi)), int(math.floor(fj))
    if i < 0 or j < 0 or i >= n - 1 or j >= n - 1: return None
    u, v = fi - i, fj - j
    h = lambda ii, jj: hz[jj * n + ii] * 100.0
    return (h(i, j) * (1 - u) + h(i + 1, j) * u) * (1 - v) + (h(i, j + 1) * (1 - u) + h(i + 1, j + 1) * u) * v

def terrain_z(x, y):
    """the highest visible ground under (x, y) for clearance checks: traced from far above (landscape / detail tiles, else the
    first solid hit such as a rock), or the far-terrain heightfield, whichever is higher"""
    hits = unreal.SystemLibrary.line_trace_multi(W, V(x, y, 1200000), V(x, y, -400000), unreal.TraceTypeQuery.TRACE_TYPE_QUERY1,
                                                 False, [], unreal.DrawDebugTrace.NONE, True)
    zt = None
    for h in hits or []:
        t = h.to_tuple()
        if t[1]: continue
        if is_terrain(t[9], t[10]): zt = t[5].z; break
    if zt is None:
        for h in hits or []:
            t = h.to_tuple()
            if not t[1] and t[0]: zt = t[5].z; break
    zs = [z for z in (zt, far_z(x, y)) if z is not None]
    return max(zs) if zs else None

def clearance(p, radius=15000.0, rings=3):
    """the nearest terrain to p (cm): ground sampled on a disc of `radius` (centre + `rings` rings of 8), 3D distance"""
    best = None
    for r_i in range(rings + 1):
        r = radius * r_i / max(1, rings)
        for a in ([0.0] if r_i == 0 else [k * 45.0 for k in range(8)]):
            x, y = p.x + r * math.cos(math.radians(a)), p.y + r * math.sin(math.radians(a))
            z = terrain_z(x, y)
            if z is None: continue
            d = math.sqrt(r * r + (p.z - z) ** 2) if p.z > z else -1.0      # under the ground counts as no clearance
            best = d if best is None else min(best, d)
    return best

def box(o, yaw, x, y, z, sx, sy, sz, mi, folder="Story/Ch1/Room", extra_yaw=0.0, pitch=0.0, roll=0.0, tags=(), hidden=False):
    m = HMI(mi[2:]) if mi.startswith("H:") else MI(mi)
    return spawn(CUBE, local(o, yaw, x, y, z), R(roll, pitch, yaw + extra_yaw), (sx / 100.0, sy / 100.0, sz / 100.0), m, tags, folder, hidden=hidden)

def invisible_material():
    """Masked material whose opacity mask is 0: draws nothing in any view mode (the hidden flag alone still let the
    collision boxes draw over the house in -game runs, 2026-11-14)."""
    path = "/Game/Story/House/M_InvisibleCollision"
    m = unreal.load_asset(path)
    if m: return m
    at_ = unreal.AssetToolsHelpers.get_asset_tools(); mel_ = unreal.MaterialEditingLibrary
    m = at_.create_asset("M_InvisibleCollision", "/Game/Story/House", unreal.Material, unreal.MaterialFactoryNew())
    m.set_editor_property("blend_mode", unreal.BlendMode.BLEND_MASKED); m.set_editor_property("used_with_nanite", True)
    zero = mel_.create_material_expression(m, unreal.MaterialExpressionConstant, -300, 0); zero.set_editor_property("r", 0.0)
    mel_.connect_material_property(zero, "", unreal.MaterialProperty.MP_OPACITY_MASK)
    mel_.recompile_material(m); unreal.EditorAssetLibrary.save_loaded_asset(m, False)
    return m

INVISIBLE = invisible_material()

def shell(o, yaw, x, y, z, sx, sy, sz, mi, folder="Story/Ch1/Room", extra_yaw=0.0, pitch=0.0, roll=0.0, tags=()):
    """The room's walls/floor/roof as invisible collision: the Blender Sherpa house (Tools/Blender/sherpa_house) is what is seen."""
    return spawn(CUBE, local(o, yaw, x, y, z), R(roll, pitch, yaw + extra_yaw), (sx / 100.0, sy / 100.0, sz / 100.0), INVISIBLE, tags, folder,
                 shadow=False, hidden=True)

HD = "/Game/Story/House"
HMI = lambda k: unreal.load_asset(f"{HD}/Materials/MI_H_{k}")

def cyl(o, yaw, x, y, z, d, h, mi, folder="Story/Ch1/Room", pitch=0.0, tags=(), shadow=True):
    return spawn(CYL, local(o, yaw, x, y, z), R(0, pitch, yaw), (d / 100.0, d / 100.0, h / 100.0), MI(mi), tags, folder, shadow=shadow)

FLAG_SAG_MAX = 45.0   # cm. (audit 2026-11-19) 12 % of the span hung the camp flags at eye level and sank the memorial ones
CORD_SEGMENTS = 8     # the cord follows the same curve as the flags (one straight rod floated 1.2 m off the poles)
def flag_line(p0, p1, n, size=(23.0, 38.0), folder="Story/Ch1/Flags", sag=0.12):   # printed lungta texture is ~0.6 wide:tall (2026-11-20)
    d = p1 - p0; L = d.length(); yaw = math.degrees(math.atan2(d.y, d.x))
    n = max(n, int(L / (size[0] + 7.0)))    # real lungta strings hang nearly edge to edge, a few cm apart
    drop = min(sag * L, FLAG_SAG_MAX)
    def on_cord(t): return p0 + d * t - V(0, 0, 4 * drop * t * (1 - t))
    for k in range(1, CORD_SEGMENTS + 1):
        qa, qb = on_cord((k - 1) / CORD_SEGMENTS), on_cord(k / CORD_SEGMENTS)
        seg = qb - qa
        spawn(CYL, (qa + qb) * 0.5, unreal.MathLibrary.make_rot_from_z(seg), (0.012, 0.012, seg.length() / 100.0), MI("Cord"), (), folder,
              shadow=False, collide=False)
    for i in range(n):
        t = (i + 0.5) / n
        p = on_cord(t)
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

# ---------------------------------------------------------------- Dingboche: an expedition camp laid out like a real one
# (2026-11-19) A crescent of two-person domes, all doors to the trail; the mess and kitchen tents behind them, side by side;
# barrels and the dung fuel behind the kitchen, the duffel pile at the mess door, porter loads ready at the row's end
# by the trail, the toilet tent at the back edge, a prayer pole in front with flag lines to the mess tent and two poles.
# Sangye's tent is the last dome of the row, nearest the trail. The old place_camps tents and props here are hidden
# (tags Ch1_OldTent / Ch1_OldProp), not deleted.
CAMP = V(339710, 2469225, -69212)
n_old = 0
for a in EAS.get_all_level_actors():
    if not isinstance(a, unreal.StaticMeshActor) or a.actor_has_tag("Ch1"): continue
    if (a.get_actor_location() - CAMP).length() > 6000: continue
    m = a.static_mesh_component.static_mesh
    old_tent = bool(m) and "tent" in m.get_name().lower()
    if not (old_tent or a.actor_has_tag("CampProp")): continue
    a.set_actor_hidden_in_game(True); a.set_is_temporarily_hidden_in_editor(True)
    a.static_mesh_component.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
    tg = "Ch1_OldTent" if old_tent else "Ch1_OldProp"
    if tg not in [str(t) for t in a.tags]: a.tags = list(a.tags) + [tg]
    n_old += 1
print("old camp actors hidden", n_old)


def clear_snow_props(center, radius):
    """removes baked snow piles / drift mounds (instances) that would stand inside a tent or around a person"""
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


def settle(x, y, half, search=180.0, tries=26):
    """the flattest spot within `search` cm of (x, y) for a footprint of +-half cm; returns x, y, lowest ground z, spread"""
    best = None
    for i in range(tries):
        if i == 0: dx = dy = 0.0
        else:
            ang = random.uniform(0, 2 * math.pi); r = random.uniform(0.0, search); dx, dy = math.cos(ang) * r, math.sin(ang) * r
        zs = [ground(x + dx + ox, y + dy + oy, CAMP.z)[0].z for ox in (-half, 0, half) for oy in (-half, 0, half)]
        sp = max(zs) - min(zs)
        score = sp + 0.04 * math.hypot(dx, dy)
        if best is None or score < best[0]: best = (score, x + dx, y + dy, min(zs), sp)
    return best[1], best[2], best[3], best[4]


TENT_FP = []   # (centre, yaw, half x, half y) of every camp tent spawned so far: camp props are nudged out of these
def tent_fp(i, c, yaw, s):
    TENT_FP.append((V(c.x, c.y, 0), yaw, AIINFO[i]["size_cm"][0] * 0.5 * s, AIINFO[i]["size_cm"][1] * 0.5 * s))


def out_of_tents(x, y, r, what=""):
    """moves a prop of radius r out of every tent footprint (plus 8 cm), along the shorter way out"""
    x0, y0 = x, y
    for _ in range(4):
        moved = False
        for c, yaw, hx, hy in TENT_FP:
            cs, sn = math.cos(math.radians(yaw)), math.sin(math.radians(yaw))
            dx, dy = x - c.x, y - c.y
            lx, ly = dx * cs + dy * sn, -dx * sn + dy * cs
            ex, ey = hx + r + 8.0 - abs(lx), hy + r + 8.0 - abs(ly)
            if ex <= 0 or ey <= 0: continue
            if ex < ey: lx += math.copysign(ex, lx if lx != 0 else 1.0)
            else: ly += math.copysign(ey, ly if ly != 0 else 1.0)
            x, y = c.x + lx * cs - ly * sn, c.y + lx * sn + ly * cs
            moved = True
        if not moved: break
    if (x, y) != (x0, y0): print("nudged out of a tent:", what, round(math.hypot(x - x0, y - y0)), "cm")
    return x, y


def put_ai(i, x, y, yaw=0.0, s=1.0, mat=None, tags=(), folder="Story/Ch1/Camp", collide=True, dz=-2.0, half=0.0):
    r = 0.5 * math.hypot(AIINFO[i]["size_cm"][0], AIINFO[i]["size_cm"][1]) * s
    x, y = out_of_tents(x, y, r, i)
    if half > 0:
        x, y, z, sp = settle(x, y, half, search=60.0, tries=10)
        x2, y2 = out_of_tents(x, y, r, i)     # settle may have wandered back in
        if (x2, y2) != (x, y): x, y = x2, y2; z = ground(x, y, CAMP.z)[0].z
        g = V(x, y, z)
    else:
        g, n = ground(x, y, CAMP.z)
    a = spawn(AI(i), g + V(0, 0, dz), R(0, 0, yaw), (s, s, s), mat, tags, folder, collide=collide)
    return a, g


def pole_at(p, h, folder="Story/Ch1/Camp"):
    g, n = ground(p.x, p.y, CAMP.z)
    spawn(CYL, g + V(0, 0, h / 2), R(random.uniform(-2, 2), random.uniform(-2, 2), 0), (0.07, 0.07, h / 100.0), MI("WoodDark"), (), folder)
    return g + V(0, 0, h - 10)


r0 = route(0)
_tx, _ty, _tz, _ = at_dist(r0, 150)                    # where the trail leaves the camp
F = V(_tx - CAMP.x, _ty - CAMP.y, 0); F = F / F.length()  # the camp's front: towards the trail
P = V(-F.y, F.x, 0)
FYAW = math.degrees(math.atan2(F.y, F.x))
def at(f, p, z=0.0): return V(CAMP.x + F.x * f + P.x * p, CAMP.y + F.y * f + P.y * p, CAMP.z + z)
places["camp_front"] = [round(at(400, 0).x, 1), round(at(400, 0).y, 1), round(CAMP.z, 1), round(FYAW, 1)]

# the row of domes: a gentle crescent opening to the trail, 3.9 m apart; the last one (+P end) is Sangye's
ROW = 5
tents = []
SANGYE_TENT = None
for j in range(ROW):
    p = (j - (ROW - 1) / 2) * 490.0
    f = -330.0 - 0.00022 * p * p
    sangye = (j == ROW - 1)
    s_ = SANGYE_S if sangye else TENT_S
    x, y, z, sp = settle(at(f, p).x, at(f, p).y, 165.0 * s_ / TENT_S)
    yaw = FYAW + random.uniform(-6, 6)
    cn, cc = TENT_COLS[0] if sangye else TENT_COLS[j % 3]
    clear_snow_props(V(x, y, z), 330)
    a = spawn(AI("tent_dome"), V(x, y, z - 3), R(0, 0, yaw), (s_,) * 3, tint("tent_dome", cn, cc),
              ("sangye_tent",) if sangye else ("camp_tent",), "Story/Ch1/Tent" if sangye else "Story/Ch1/Camp", collide=not sangye)
    if sangye: SANGYE_TENT = a
    tent_fp("tent_dome", V(x, y, z), yaw, s_)
    tents.append((V(x, y, z), yaw))
    place(f"camp_tent_{j}", V(x, y, z), yaw)
    print("dome", j, "ground spread", round(sp))
avoid = [V(t[0].x, t[0].y, 330) for t in tents]

# mess tent (door +y in the model) behind the row's middle, kitchen tent (door +y) beside it
mx, my, mz, msp = settle(at(-1400, -380).x, at(-1400, -380).y, 300.0, search=260.0, tries=34)
myaw = FYAW - 90.0
clear_snow_props(V(mx, my, mz), 500)
spawn(AI("tent_mess"), V(mx, my, mz - 4), R(0, 0, myaw), (1, 1, 1), tint("tent_mess", "warm", (1.12, 0.98, 0.72)), ("camp_mess",), "Story/Ch1/Camp")   # Meshy painted it pale: warmer yellow so it reads as fabric, not stone
tent_fp("tent_mess", V(mx, my, mz), myaw, 1.0)
place("camp_mess", V(mx, my, mz), myaw)
print("mess spread", round(msp))
MESS_C = V(mx, my, mz)
kx, ky, kz, ksp = settle(at(-1400, 470).x, at(-1400, 470).y, 210.0, search=200.0, tries=30)
kyaw = FYAW - 90.0 + random.uniform(-4, 4)
clear_snow_props(V(kx, ky, kz), 380)
spawn(AI("tent_kitchen"), V(kx, ky, kz - 4), R(0, 0, kyaw), (1, 1, 1), None, ("camp_kitchen",), "Story/Ch1/Camp")
tent_fp("tent_kitchen", V(kx, ky, kz), kyaw, 1.0)
place("camp_kitchen", V(kx, ky, kz), kyaw)
print("kitchen spread", round(ksp))
def near(c, f, p): return V(c.x + F.x * f + P.x * p, c.y + F.y * f + P.y * p, c.z)
# the cook's stove and pots at the kitchen door, barrels and fuel behind it
cs = near(V(kx, ky, kz), 300, 40); put_ai("cook_set", cs.x, cs.y, FYAW + 160, half=40)
for j in range(6):
    b = near(V(kx, ky, kz), -290 - 48 * (j // 3), -60 + 48 * (j % 3) + random.uniform(-4, 4))
    put_ai("barrel_blue", b.x, b.y, random.uniform(0, 360))
dh = near(V(kx, ky, kz), -60, 330); put_ai("dung_heap", dh.x, dh.y, random.uniform(0, 360), collide=False, dz=-6)
# the duffel pile at the mess door: two rows of three, one thrown on top
for j in range(7):
    dn, dc = DUF_COLS[(j + 1) % 4]
    if j < 6: d = near(MESS_C, 470 + 48 * (j // 3), -130 + 88 * (j % 3)); dz = -2
    else: d = near(MESS_C, 490, -60); dz = 42
    put_ai("duffel", d.x, d.y, FYAW + 90 + random.uniform(-12, 12), 1.0, tint("duffel", dn, dc), dz=dz)
# porter loads roped and ready beyond Sangye's tent, by the trail
for j in range(3):
    lp = near(tents[-1][0], 150 + 15 * j, 400 + 62 * j)
    put_ai("porter_load", lp.x, lp.y, FYAW + random.uniform(-30, 30))
# toilet tent at the back edge
tx, ty, tz, tsp = settle(at(-1900, -1500).x, at(-1900, -1500).y, 60.0, search=300.0)
spawn(AI("tent_toilet"), V(tx, ty, tz - 3), R(0, 0, FYAW + random.uniform(-20, 20)), (1, 1, 1), None, (), "Story/Ch1/Camp")
# the prayer pole in front of the camp, flag lines from its top to the mess tent and to two poles at the row's ends
tc = at(260, 0); tg_, tn_ = ground(tc.x, tc.y, CAMP.z)
spawn(AI("tarchen"), tg_ - V(0, 0, 6), R(0, 0, FYAW), (1, 1, 1), None, (), "Story/Ch1/Camp")
top = tg_ + V(0, 0, 470)
flag_line(top, MESS_C + V(0, 0, 265), 26, folder="Story/Ch1/Camp")
flag_line(top, pole_at(at(-60, -1450), 400), 28, folder="Story/Ch1/Camp")
flag_line(top, pole_at(at(-60, 1500), 400), 28, folder="Story/Ch1/Camp")
# where the team waits in front of the camp; farewell_dolma / farewell_aunt: where Dolma (leading you by the hand) and the aunt
# stop at the farewell, each >= 1.8 m from Nima, Ji-woo and the tarchen (audit 2026-11-19: both stopped at nima_spot, inside
# each other)
for name, (f, p) in (("nima_spot", (520, 260)), ("marek_spot", (470, -280)), ("elena_spot", (600, -60)), ("jiwoo_spot", (560, 520)),
                     ("farewell_dolma", (380, 380)), ("farewell_aunt", (290, 560))):
    g, n = ground(at(f, p).x, at(f, p).y, CAMP.z)
    place(name, g, FYAW + 180)
    clear_snow_props(g, 300)
for a_, b_ in (("farewell_dolma", "nima_spot"), ("farewell_aunt", "jiwoo_spot"), ("farewell_aunt", "farewell_dolma")):
    pa, pb = places[a_], places[b_]
    dd = math.hypot(pa[0] - pb[0], pa[1] - pb[1])
    check(f"{a_} apart from {b_}", dd >= 180.0, f"{dd:.0f} cm (>= 180)")

# ---------------------------------------------------------------- Sangye's tent: the chapter opens inside it
# (the walk-in family room trapped players and was removed; its assets stay in /Game/Story/House and /Game/Story/PH)
TO, SYAW = tents[-1]
SS = SANGYE_S
def tl(x, y, z): return local(TO, SYAW, x, y, z)      # Sangye's tent frame: +x = the door, origin = the ground under the centre
# (2026-11-19) everything inside is fitted to the fabric: offsets are laid out for the hexagonal floor (side walls straight,
# end walls tapering), then every footprint is traced from the tent's centre outwards (the tent gets collision for that and
# loses it again: the player wakes inside) and an item that pokes out is moved in / scaled down until 5 cm inside.
# the floor sits on the highest ground under the tent (the dome stands on the lowest of its settle samples), at most 6 cm up;
# measured BEFORE the tent gets collision (ground() would take the roof for the ground)
_gz = []
for _gx, _gy in ((0, 0), (70 * SS, 0), (-70 * SS, 0), (0, 55 * SS), (0, -55 * SS), (35 * SS, 40 * SS), (-35 * SS, -40 * SS)):
    _p = tl(_gx, _gy, 0); _gz.append(ground(_p.x, _p.y, TO.z)[0].z)
FL = min(6.0, max(0.0, max(_gz) - TO.z))
print("Sangye tent scale", SS, "floor lift", round(FL, 1), "cm")


def tent_hit(a, b):
    """True when the segment a -> b (world) crosses Sangye's tent; anything else in the way (ground, rocks) is skipped"""
    ign = []
    for _ in range(6):
        h = unreal.SystemLibrary.line_trace_single(W, a, b, unreal.TraceTypeQuery.TRACE_TYPE_QUERY1, True, ign, unreal.DrawDebugTrace.NONE, True)
        t = h.to_tuple() if h else None
        if not t or not t[0] or t[9] is None: return False
        if t[9] == SANGYE_TENT: return True
        ign.append(t[9])
    return False


def tent_item(n, x, y, z, yaw, s, size):
    """an item inside the tent: centre x, y and bottom z in the tent frame, yaw relative to the tent, footprint from the model size"""
    return dict(n=n, x=float(x), y=float(y), z=float(z), yaw=float(yaw), s=float(s), s0=float(s), hx=size[0] * 0.5, hy=size[1] * 0.5, h=float(size[2]))


def item_points(it):
    hx, hy = it["hx"] * it["s"], it["hy"] * it["s"]
    c, s_ = math.cos(math.radians(it["yaw"])), math.sin(math.radians(it["yaw"]))
    return [(it["x"] + u * hx * c - v * hy * s_, it["y"] + u * hx * s_ + v * hy * c)
            for u, v in ((1, 1), (1, -1), (-1, 1), (-1, -1), (1, 0), (-1, 0), (0, 1), (0, -1))]


def poking(it, margin=5.0):
    """the footprint corners / edge middles that are not `margin` cm inside the fabric, tested at the item's bottom and top"""
    top = it["z"] + it["h"] * it["s"]
    bad = []
    for hz in sorted({max(6.0, it["z"] + 4.0), max(6.0, top - 2.0)}):   # 6 cm and up: clear of the dome's own floor
        for px, py in item_points(it):
            d = math.hypot(px, py)
            if d < 1.0: continue
            k = (d + margin) / d
            if tent_hit(tl(0, 0, FL + hz), tl(px * k, py * k, FL + hz)): bad.append((px, py))
    return bad


def fit(it, move=True, max_move=30.0, min_frac=0.75, keep_y=None):
    moved = 0.0
    for _ in range(60):
        bad = poking(it) if TRACE_OK else []
        if not bad: break
        if move and moved < max_move:     # step 3 cm away from the side that pokes out
            mx = sum(p[0] for p in bad) / len(bad) - it["x"]; my = sum(p[1] for p in bad) / len(bad) - it["y"]
            L = math.hypot(mx, my)
            if L < 1.0: mx, my = it["x"], it["y"]; L = max(1.0, math.hypot(mx, my))
            it["x"] -= mx / L * 3.0
            ny = it["y"] - my / L * 3.0
            if keep_y and abs(ny) < keep_y(it): ny = math.copysign(keep_y(it), it["y"] if it["y"] != 0 else 1.0)
            it["y"] = ny; moved += 3.0
        elif it["s"] > it["s0"] * min_frac:
            it["s"] *= 0.97
        else:
            print("  STILL POKING OUT:", it["n"], len(bad), "test points outside"); break
    print("tent item", it["n"], "offset", round(it["x"], 1), round(it["y"], 1), round(FL + it["z"], 1), "scale", round(it["s"], 3),
          "(laid out", round(it["s0"], 3), ") moved", round(moved), "cm")
    return it


TC = SANGYE_TENT.static_mesh_component
TC.set_collision_enabled(unreal.CollisionEnabled.QUERY_AND_PHYSICS)   # complex-as-simple: the traces see the real fabric
try:
    TRACE_OK = (tent_hit(tl(0, 0, 40), tl(0, 400, 40)) and tent_hit(tl(0, 0, 40), tl(-400, 0, 40))
                and not tent_hit(tl(0, 0, 40), tl(0, 30, 40)))
    print("Sangye tent trace check", "ok" if TRACE_OK else "FAILED (no fabric hits) - keeping the laid-out offsets")
    # two sleeping bags side by side along the tent, heads at the back (-x), 6 cm apart; the father's old orange duffel lies
    # on the empty one's foot, his photo on the duffel
    BAG_S = 0.9; BAG = AIINFO["bedding"]["size_cm"]
    bag_gap = lambda it: it["hy"] * it["s"] + 2.0       # the bags never slide over each other
    bag_a = fit(tent_item("bag_sangye", -2, -(BAG[1] * 0.5 * BAG_S + 3), 1, 0, BAG_S, BAG), keep_y=bag_gap)
    bag_b = fit(tent_item("bag_father", -2, BAG[1] * 0.5 * BAG_S + 3, 1, 1.5, BAG_S, BAG), keep_y=bag_gap)
    duf = fit(tent_item("duffel", bag_b["x"] + 50 * bag_b["s"] / BAG_S, bag_b["y"], 27, 8, 0.8, AIINFO["duffel"]["size_cm"]), max_move=20.0)
    bag_back = min(bag_a["x"] - BAG[0] * 0.5 * bag_a["s"], bag_b["x"] - BAG[0] * 0.5 * bag_b["s"])
    lamp = fit(tent_item("headlamp", bag_back - 10, 0, 1, 30, 1.0, AIINFO["headlamp"]["size_cm"]))
    bottle = fit(tent_item("water_bottle", bag_a["x"] - 25, bag_a["y"] - BAG[1] * 0.5 * bag_a["s"] - 16, 1, 0, 1.0, AIINFO["water_bottle"]["size_cm"]))
    # the tent's floor (the snow showed through the dome's own): the hexagon as a middle slab and two diamonds to the ends
    D = 48 * SS * math.sqrt(2.0)
    floors = [fit(tent_item("floor_mid", 0, 0, 0.2, 0, 1.0, (84 * SS, 128 * SS, 0.8)), move=False, min_frac=0.5),
              fit(tent_item("floor_back", -44 * SS, 0, 0.1, 45, 1.0, (D, D, 0.8)), move=False, min_frac=0.5),
              fit(tent_item("floor_door", 44 * SS, 0, 0.1, 45, 1.0, (D, D, 0.8)), move=False, min_frac=0.5)]
    # where the chapter opens: sitting up in his bag, facing the door; head room above the eyes (~65 cm) and 25 cm around them
    cx, cy = bag_a["x"] - 44 * bag_a["s"] / BAG_S, bag_a["y"]
    for _ in range(30):
        if not TRACE_OK: break
        d = max(1.0, math.hypot(cx, cy)); k = (d + 25.0) / d
        if not (tent_hit(tl(cx, cy, FL + 20), tl(cx, cy, FL + 95)) or tent_hit(tl(0, 0, FL + 65), tl(cx * k, cy * k, FL + 65))): break
        cx += 3.0 if cx < 0 else -3.0
    else:
        print("  tent_start: no head room found")
    print("tent item tent_start offset", round(cx, 1), round(cy, 1), round(FL, 1))
finally:
    TC.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)   # the player wakes inside

for it in (bag_a, bag_b):
    spawn(AI("bedding"), tl(it["x"], it["y"], FL + it["z"]), R(0, 0, SYAW + it["yaw"]), (it["s"],) * 3, None, (), "Story/Ch1/Tent", collide=False)
spawn(AI("duffel"), tl(duf["x"], duf["y"], FL + duf["z"]), R(0, 0, SYAW + duf["yaw"]), (duf["s"],) * 3, tint("duffel", "fatherorange", (1.35, 0.42, 0.1)),
      ("duffel",), "Story/Ch1/Tent", collide=False)
for it in floors:
    spawn(CUBE, tl(it["x"], it["y"], FL + it["z"] + 0.4), R(0, 0, SYAW + it["yaw"]), (2 * it["hx"] * it["s"] / 100.0, 2 * it["hy"] * it["s"] / 100.0, 0.008),
          MI("TentGrey"), (), "Story/Ch1/Tent", shadow=False, collide=False)
duf_top = duf["z"] + AIINFO["duffel"]["size_cm"][2] * duf["s"]
ps = duf["s"] / 0.8
spawn(PLANE, tl(duf["x"] - 5 * ps, duf["y"] - 6 * ps, FL + duf_top + 1), R(0, 0, SYAW + 96), (0.14, 0.105, 1), MI("Photo_Icefall"), ("photo_father",),
      "Story/Ch1/Tent", collide=False)
spawn(AI("headlamp"), tl(lamp["x"], lamp["y"], FL + lamp["z"]), R(0, 0, SYAW + lamp["yaw"]), (lamp["s"],) * 3, None, (), "Story/Ch1/Tent", collide=False)
spawn(AI("water_bottle"), tl(bottle["x"], bottle["y"], FL + bottle["z"]), R(0, 0, SYAW + bottle["yaw"]), (bottle["s"],) * 3, None, (), "Story/Ch1/Tent",
      collide=False)
# dawn through orange fabric: a soft warm fill inside (shadowed, so nothing leaks onto the snow outside)
light(unreal.PointLight, tl(0, 0, 63 * SS), R(0, 0, 0), 7.0, (255, 196, 150, 255), 220 * SS / 1.5, ("tentlight",), shadows=True,
      folder="Story/Ch1/Tent")
place("tent_duffel", tl(duf["x"], duf["y"], FL + duf["z"] + 0.5 * AIINFO["duffel"]["size_cm"][2] * duf["s"]), 0)

# ---------------------------------------------------------------- the tent's places, aimed for the camera (Chapter1_Camera_Spec 8.4-8.6, 10.2)
# (2026-11-19) the interior above stays as fitted and verified; only the places that frame it are set and checked here.
# The chapter wakes lying at tent_start (eye 32 cm), sits up (eye 78 cm) and crawls out along the line tent_start -> tent_out.
HALF_L = AIINFO["tent_dome"]["size_cm"][0] * 0.5 * SS    # the door end of the dome on the tent axis (+x)
TS = tl(cx, cy, FL)                                      # the pillow end of Sangye's bag (head room found above)
DOOR = tl(HALF_L, 0, FL + 60)                            # the middle of the door gap
DUF = tl(duf["x"], duf["y"], FL + duf["z"] + 0.5 * AIINFO["duffel"]["size_cm"][2] * duf["s"])
b_door = bearing(TS, DOOR); b_duf = b_door + wrap(bearing(TS, DUF) - b_door)
# the wake frame wants the door glowing 30-40 deg left and the duffel 20-35 deg right: the overlap of both yaw bands when there
# is one, else the least-squares compromise between the two band middles (the fitted interior decides how far apart they are)
lo_, hi_ = max(b_door + 30.0, b_duf - 35.0), min(b_door + 40.0, b_duf - 20.0)
ts_yaw = ((b_door + 35.0) + (b_duf - 27.5)) * 0.5
if lo_ <= hi_: ts_yaw = min(hi_, max(lo_, ts_yaw))
place("tent_start", TS, ts_yaw)
dr_, fr_ = wrap(b_door - ts_yaw), wrap(b_duf - ts_yaw)
_why = "" if lo_ <= hi_ else f" (door and duffel are only {b_duf - b_door:.0f} deg apart from the pillow: compromise yaw)"
check("tent_start: door 30-40 deg left", in_band(dr_, -40, -30), f"door at {dr_:+.1f} deg{_why}")
check("tent_start: duffel 20-35 deg right", in_band(fr_, 20, 35), f"duffel at {fr_:+.1f} deg")
for eye_h in (32.0, 78.0):
    pe_ = elev(TS + V(0, 0, eye_h), DUF)
    check(f"tent_start: duffel pitch from the {eye_h:.0f} cm eye", in_band(pe_, -35, -25),
          f"{pe_:+.1f} deg, {dist2(TS, DUF):.0f} cm away (spec -25..-35 deg at 50-70 cm; the interior fitting is frozen)")
place("tent_door", DOOR, SYAW)
# Dolma waits outside 6-8 m out of the door, within 25 deg of its outward axis: in front of you as you stand up
_d = local(TO, SYAW, HALF_L + 700, 150, 0); dx_, dy_ = out_of_tents(_d.x, _d.y, 40.0, "dolma_spot")
dg, dn = ground(dx_, dy_, CAMP.z)
place("dolma_spot", dg, bearing(dg, TS))
_dd, _da = dist2(dg, tl(HALF_L, 0, 0)), wrap(bearing(tl(HALF_L, 0, 0), dg) - SYAW)
check("dolma_spot 6-8 m out of the door, within 25 deg of its axis", in_band(_dd, 600, 800) and abs(_da) <= 25, f"{_dd:.0f} cm, {_da:+.1f} deg")
_a = local(TO, SYAW, HALF_L + 900, -130, 0); ax_, ay_ = out_of_tents(_a.x, _a.y, 40.0, "aunt_spot")
ag, an = ground(ax_, ay_, CAMP.z)
place("aunt_spot", ag, bearing(ag, TS))


def tent_dist(a, b):
    """how far from a the segment a -> b first meets Sangye's tent (cm), or None; anything else in the way is skipped"""
    ign = []
    for _ in range(6):
        h = unreal.SystemLibrary.line_trace_single(W, a, b, unreal.TraceTypeQuery.TRACE_TYPE_QUERY1, True, ign, unreal.DrawDebugTrace.NONE, True)
        t = h.to_tuple() if h else None
        if not t or not t[0] or t[9] is None: return None
        if t[9] == SANGYE_TENT: return (t[5] - a).length()
        ign.append(t[9])
    return None


OUT_Y = 0.0
TC.set_collision_enabled(unreal.CollisionEnabled.QUERY_AND_PHYSICS)   # the real fabric again, for the checks only
try:
    # tent_out: 60 cm out of the door on its axis (the crawl ends within 60 cm of it); the crawl line from the pillow must pass
    # the door gap at the crawling eye (68 cm) and at the body (35 cm), else tent_out slides a little off the axis
    _clear = None
    for oy in (0, 6, -6, 12, -12, 18, -18, 24, -24, 30, -30):
        if not any(tent_hit(tl(cx, cy, FL + h), tl(HALF_L + 60, oy, FL + h)) for h in (35.0, 68.0)):
            _clear = oy; break
    OUT_Y = float(_clear) if _clear is not None else 0.0
    check("crawl line tent_start -> tent_out passes the door gap", _clear is not None and TRACE_OK,
          f"tent_out {HALF_L + 60:.0f} cm out, {OUT_Y:+.0f} cm off the axis" + ("" if TRACE_OK else " (tent traces failed: unverified)"))
    # 8 rays from the sitting eye, 45 deg up all round, must meet the fabric within 1.5 m: the tent closes round you
    _eye = tl(cx, cy, FL + 78)
    _ds = []
    for k in range(8):
        _r = math.radians(ts_yaw + 45 * k)
        _ds.append(tent_dist(_eye, _eye + V(math.cos(_r) * 212.1, math.sin(_r) * 212.1, 212.1)))   # 3 m rays
    check("8 rays from the tent_start eye meet the tent within 1.5 m", all(d is not None and d <= 150 for d in _ds),
          ", ".join("-" if d is None else f"{d:.0f}" for d in _ds) + " cm")
finally:
    TC.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)   # pawn collision stays off: the player wakes inside
og, on_ = ground(tl(HALF_L + 60, OUT_Y, 0).x, tl(HALF_L + 60, OUT_Y, 0).y, CAMP.z)
place("tent_out", og, bearing(og, dg))              # standing up there faces Dolma: the reveal of the camp
clear_snow_props(og, 300)


def two_sided(m, depth=0):
    try:
        if m is None or depth > 6: return None
        if isinstance(m, unreal.MaterialInstance):
            ov = m.get_editor_property("base_property_overrides")
            if ov.get_editor_property("override_two_sided"): return bool(ov.get_editor_property("two_sided"))
            return two_sided(m.get_editor_property("parent"), depth + 1)
        return bool(m.get_editor_property("two_sided"))
    except Exception as e:
        print("  two-sided check:", e); return None
_tm = TC.get_material(0)
_t2 = two_sided(_tm)
check("Sangye's tent draws its inside (two-sided material)", _t2 is True,
      f"two_sided={_t2} on {_tm.get_name() if _tm else None} (else it needs an inner shell: the director's eye)")
print("sleeping_bag at tent_start: Sangye's own bag (bag_sangye) is the prop there and stays visible; the tagged one is at the night camp")
vg, vn = ground(local(TO, SYAW, 2600, 900, 0).x, local(TO, SYAW, 2600, 900, 0).y, CAMP.z)
place("village_way", vg, 0)      # where Dolma and the aunt walk back to


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

# ---------------------------------------------------------------- the trail (leg 0) and its events
for name, m, side in (("trail_start", 60, 0), ("ev_climb", 220, 0), ("ev_stream", 900, 0), ("ev_elena", 1700, 0), ("ev_talk", 1850, 0),
                      ("ev_porters", 2650, 0), ("porter_start", 2780, 1.5), ("porter_end", 2450, 1.5), ("ev_cloud", 3350, 0),
                      ("ev_alone_end", 4300, 0), ("ev_flags_seen", 5450, 0), ("night_camp", 5650, 18), ("memorial", 5800, 0)):
    g, yaw, n = on_route(r0, m, side * 100)
    place(name, g, yaw)
# (2026-11-19 camera spec 8.9-8.10, 10.2) three of those are staged for the eye: searched along the trail, then checked
EYE = V(0, 0, 156)
def heading(rt, m0, m1):
    x0, y0, _, _ = at_dist(rt, m0); x1, y1, _, _ = at_dist(rt, m1)
    return math.degrees(math.atan2(y1 - y0, x1 - x0))
# Elena stops at a bend: the sharpest turn near 1,700 m, so walking up you find her straight ahead, 10 m on
best = None
for m in range(1550, 1860, 10):
    turn_ = abs(wrap(heading(r0, m, m + 30) - heading(r0, m - 30, m)))
    score = -turn_ + 0.02 * abs(m - 1700)
    if best is None or score < best[0]: best = (score, m, turn_)
EL_M, EL_TURN = best[1], best[2]
g, yaw, n = on_route(r0, EL_M, 0); place("ev_elena", g, yaw)
gb, _, _ = on_route(r0, EL_M - 10, 0)
_rel = wrap(bearing(gb, g) - heading(r0, EL_M - 20, EL_M - 10))
check("ev_elena at a bend, within 20 deg ahead from 10 m before", abs(_rel) <= 20 and EL_TURN >= 15,
      f"route {EL_M} m, the trail turns {EL_TURN:.0f} deg there, {_rel:+.1f} deg from 10 m before")
# the porters start where the trail cannot see them until 25 m (their bells come first), then walk the trail down by waypoints
# (audit: a straight goto porter_start -> porter_end left the trail by 12 m and the pass beat never fired)
_eyes = {}
def trail_eye(m):
    if m not in _eyes: _eyes[m] = on_route(r0, m, 0)[0] + EYE
    return _eyes[m]
best = None
for m in range(2740, 2961, 20):
    for side in (150, 400, -150):
        g, yaw, n = on_route(r0, m, side)
        head = g + V(0, 0, 160)
        seen = [dd for dd in range(25, 201, 15) if not los_blocked(trail_eye(m - dd), head)]
        score = len(seen) * 10 + (max(seen) if seen else 0) * 0.05 + abs(side) / 200.0 + (m - 2740) / 100.0
        if best is None or score < best[0]: best = (score, m, side, g, yaw, seen)
_, PS_M, PS_SIDE, g, yaw, _seen = best
place("porter_start", g, yaw + 180)
check("porter_start out of sight from the trail until 25 m", not _seen,
      f"route {PS_M} m, {PS_SIDE / 100:+.1f} m off; in sight from {_seen if _seen else 'nowhere'} m before it")
for k in range(8):
    g, yaw, n = on_route(r0, PS_M - (k + 1) * (PS_M - 2450.0) / 9.0, 150)
    place(f"porter_w{k}", g, yaw + 180)
# the valley side where you meet them: the risky pass knocks you toward it
_gl, _, _ = on_route(r0, 2650, -800); _gr, _, _ = on_route(r0, 2650, 800)
place("porter_valley", _gl if _gl.z < _gr.z else _gr, 0)
print("porter_valley", "left" if _gl.z < _gr.z else "right", "of the trail going up; 16 m across it drops", round(abs(_gl.z - _gr.z)), "cm")
# the memorial flags come into view at a turn, straight ahead (+-15 deg), heard before (the flags loop starts 26 m earlier)
_flags = on_route(r0, 5810, 600)[0] + V(0, 0, 250)             # the flag web over the field on the father's side
best = None
for m in range(5250, 5661, 10):
    g, yaw, n = on_route(r0, m, 0)
    h_ = heading(r0, m, m + 15)
    rel = wrap(bearing(g, _flags) - h_)
    vis = not los_blocked(g + EYE, _flags, 300.0)
    turn_ = abs(wrap(heading(r0, m - 30, m) - h_))
    score = band_miss(abs(rel), 0, 15) * 10 + (0 if vis else 60) - min(turn_, 40) * 0.2 + abs(m - 5450) * 0.01
    if best is None or score < best[0]: best = (score, m, g, yaw, rel, vis, turn_)
_, FS_M, g, yaw, _rel, _vis, _turn = best
place("ev_flags_seen", g, yaw)
check("ev_flags_seen: the flags within 15 deg ahead and in sight", abs(_rel) <= 15 and _vis,
      f"route {FS_M} m, flags {_rel:+.1f} deg, {'in sight' if _vis else 'hidden'}, turn {_turn:.0f} deg, {dist2(g, _flags) / 100:.0f} m away")
for nm in ("dolma_spot", "aunt_spot", "tent_out"):
    p = places[nm]; clear_snow_props(V(p[0], p[1], p[2]), 300)
# the ice hole by the stream, 3 m off the trail: a ring of stream ice (Meshy) over dark running water
g, yaw, n = on_route(r0, 900, 300)
# the water sits just above the drawn snow (the snow surface covered it at -6 cm); the ice ring rides 5 cm higher
spawn(CYL, g + V(0, 0, 1.5), R(0, 0, yaw), (0.85, 0.85, 0.01), MI("Water"), ("stream",), "Story/Ch1/Trail", shadow=False, collide=False)
spawn(AI("stream_hole"), g + V(0, 0, -3), R(0, 0, yaw), (1.3, 1.3, 1.0), None, (), "Story/Ch1/Trail", collide=False)
place("stream_hole", g, yaw)

# ---------------------------------------------------------------- the memorials (Thukla pass, ~4,840 m on our trail)
# stone chortens and cairns from Meshy: a (2 m stepped, plaque), b (1.2 m cairn with a flag stick), c (3 m, pole).
# Grouped like the real Thukla memorials: a dense field on the father's side of the trail, framing his stone, thinning
# out up the ridge; a few across the trail; flag lines between neighbours and a web from the tallest ones.
mg, myaw_, mn = on_route(r0, 5800, 0)
c, s = math.cos(math.radians(myaw_)), math.sin(math.radians(myaw_))
KINDS = [("chorten_a", 200.0, 0.38), ("chorten_b", 120.0, 0.42), ("chorten_c", 300.0, 0.20)]
# (2026-11-19 camera spec 8.11, 10.2) the kneel is framed from the stone outwards: kneeling at kneel_spot and facing the stone puts
# plume_view 25-35 deg right and 12-18 deg up (the stone in the lower-left third, the plume upper right). So the summit comes
# first, the kneeling spot is set round the stone by it, and the plaque turns to the kneeling player.
r7 = route(7)
sx_, sy_, sz_, syaw = at_dist(r7, 99999)
sg_, sn_ = ground(sx_, sy_, sz_)
d = V(mg.x - sg_.x, mg.y - sg_.y, 0); d = d / d.length()          # the plume quad faces the memorials ...
xax = V(-d.y, d.x, 0)
if xax.x < 0: xax = xax * -1.0                                     # ... and streams east off the summit
PLUME_VIEW_ALONG = 40000.0   # cm along the plume from the summit: the look target sits at its root
PLUME_ROOT_UP = 15000.0      # cm above the summit: the plume's root (the quad below now starts at summit height)
PLUME_YAW_REL = 30.0         # deg right of the stone (spec band 25-35)
PLUME_HALF_H = 100000.0      # the plume quad is 2 km tall: its bottom edge at the summit, so it streams from the top
KNEEL_DIST, KNEEL_EYE = 140.0, 110.0
fg, fyaw, fn = on_route(r0, 5830, 380)                             # the father's stone, 3.8 m off the trail
def plume_target(eye):
    """the look target: the plume's root, at the pitch it really has from the eye, held inside the spec band 12-18 deg (the
    summit itself stands ~18 deg above the Thukla kneel, so the root sits at the band's top edge)"""
    base = sg_ + xax * PLUME_VIEW_ALONG
    pitch = min(18.0, max(12.0, elev(eye, V(base.x, base.y, sg_.z + PLUME_ROOT_UP))))
    return V(base.x, base.y, eye.z + dist2(eye, base) * math.tan(math.radians(pitch)))
k_eye = fg + V(0, 0, KNEEL_EYE)
for _ in range(3):
    PV = plume_target(k_eye)
    KYAW = bearing(k_eye, PV) - PLUME_YAW_REL                      # facing the stone from the kneeling spot
    _k = fg - unit(KYAW) * KNEEL_DIST
    kg, kn = ground(_k.x, _k.y, fg.z)
    k_eye = kg + V(0, 0, KNEEL_EYE)
PV = plume_target(k_eye)
face = KYAW + 180.0          # the direction the plaque looks: at the kneeling player (it used to face the trail)
# Nima waits front-left of you, outside the stone's frame, turned to it (audit: his goal was your own kneeling spot)
_n = kg + unit(KYAW - 70.0) * 180.0
nsg, _ = ground(_n.x, _n.y, fg.z)
def mframe(p): return ((p.x - mg.x) * c + (p.y - mg.y) * s, -(p.x - mg.x) * s + (p.y - mg.y) * c)
STONE_ALONG, STONE_SIDE = mframe(fg)
taken = [mframe(fg) + (260.0,), mframe(kg) + (200.0,), mframe(nsg) + (160.0,)]   # the stone, the kneeling spot and Nima stay clear
_py, _pp = wrap(bearing(k_eye, PV) - bearing(k_eye, fg)), elev(k_eye, PV)
check("plume_view 25-35 deg right of the stone from the kneeling eye", in_band(_py, 25, 35), f"{_py:+.1f} deg")
check("plume_view 12-18 deg up from the kneeling eye", in_band(_pp, 12, 18), f"{_pp:+.1f} deg (the summit {elev(k_eye, sg_):+.1f} deg)")
_sy, _sp = wrap(bearing(k_eye, fg) - bearing(k_eye, PV)), elev(k_eye, fg + V(0, 0, 50))
check("the stone in the lower left while you look at the plume", _sy < -10 and _sp - _pp < -20, f"stone {_sy:+.1f} deg, {_sp - _pp:+.1f} deg below the view")
_ny, _nd = wrap(bearing(k_eye, nsg) - bearing(k_eye, PV)), dist2(kg, nsg)
check("nima_stone at your left: 30-120 deg off the plume view (his reflex), >= 1.5 m away", -120 <= _ny <= -30 and _nd >= 150,
      f"{_ny:+.1f} deg, {_nd:.0f} cm; ground {(nsg.z - kg.z):+.0f} cm")
print("kneel_spot ground", round(kg.z - fg.z), "cm against the stone's")
chortens = []
def free(al, sd, r):
    return all(math.hypot(al - a_, sd - s_) > r + r_ for a_, s_, r_ in taken) and abs(sd) > 260
tries = 0
while len(chortens) < 26 and tries < 900:
    tries += 1
    if len(chortens) < 18:   # the dense field around and beyond the stone
        al = STONE_ALONG + random.gauss(0, 900); sd = STONE_SIDE + abs(random.gauss(0, 650)) + 150
    elif len(chortens) < 22: # thinning out up the ridge
        al = STONE_ALONG + random.uniform(-2600, 2600); sd = random.uniform(1200, 2800)
    else:                    # a few across the trail
        al = STONE_ALONG + random.uniform(-1600, 1600); sd = -random.uniform(350, 1100)
    r_ = random.random(); acc = 0.0
    for kind, h, w in KINDS:
        acc += w
        if r_ <= acc: break
    sc = random.uniform(0.8, 1.15)
    rad = {"chorten_a": 95, "chorten_b": 45, "chorten_c": 120}[kind] * sc
    if not free(al, sd, rad): continue
    x = mg.x + c * al - s * sd; y = mg.y + s * al + c * sd
    zs = [ground(x + ox, y + oy, mg.z)[0].z for ox in (-rad, 0, rad) for oy in (-rad, 0, rad)]
    if max(zs) - min(zs) > 0.9 * rad: continue           # too steep: a stone chorten is built on a level spot
    taken.append((al, sd, rad))
    g = V(x, y, min(zs))
    face_trail = myaw_ + (-90 if sd > 0 else 90) - 90 + random.uniform(-25, 25)   # the model's front (+y, the plaque) to the trail
    spawn(AI(kind), g - V(0, 0, 4), R(0, 0, face_trail), (sc, sc, sc), None, ("memorial",), "Story/Ch1/Memorial")
    chortens.append((V(x, y, g.z + h * sc * 0.97), kind, al, sd))
print("chortens placed", len(chortens), "after", tries, "tries")
links = set()
for i, (p1, k1, a1, s1) in enumerate(chortens):     # each chorten strung to its nearest neighbour
    best = min(((p1 - p2).length(), j) for j, (p2, k2, a2, s2) in enumerate(chortens) if j != i)
    if best[0] < 1500: links.add(tuple(sorted((i, best[1]))))
tall = [i for i, ch in enumerate(chortens) if ch[1] == "chorten_c"][:3]
for i in tall:                                       # and a web from the tallest ones
    near_ = sorted(((chortens[i][0] - chortens[j][0]).length(), j) for j in range(len(chortens)) if j != i)[:3]
    for d_, j in near_:
        if d_ < 2200: links.add(tuple(sorted((i, j))))
for i, j in links:
    a_, b_ = chortens[i][0], chortens[j][0]
    flag_line(a_, b_, max(5, int((a_ - b_).length() / 50)), folder="Story/Ch1/Memorial")
print("flag lines", len(links))
# the father's stone: freshly cut, close to the trail, his name on its plaque; his khata appears when you tie it
# (fg and face are set above, with the kneeling spot)
PLAQUE_YAW = 90.0           # the plaque faces the model's +y (Meshy front); stone yaw = face - this
st = spawn(AI("father_stone"), fg - V(0, 0, 6), R(0, 0, face - PLAQUE_YAW), (1.15, 1.15, 1.15), None, ("father_stone",), "Story/Ch1/Memorial")
fdir = V(math.cos(math.radians(face)), math.sin(math.radians(face)), 0)
PZ = 62.0                    # plaque centre height above the ground (cm), 1.15 x the model's ~54
front = fg + fdir * 60 + V(0, 0, PZ); hit = unreal.SystemLibrary.line_trace_single(W, front, fg + V(0, 0, PZ), unreal.TraceTypeQuery.TRACE_TYPE_QUERY1,
                                                                                  True, [], unreal.DrawDebugTrace.NONE, True)
ht = hit.to_tuple() if hit else None
surf = ht[5] if ht and ht[9] == st else fg + fdir * 18 + V(0, 0, PZ)
print("father stone plaque surface", round((surf - fg).length(), 1), "cm from the centre", "(traced)" if ht and ht[9] == st else "(guess)")
tr = EAS.spawn_actor_from_class(unreal.TextRenderActor, surf + fdir * 0.6, R(0, 0, face))
t = tr.text_render
t.set_editor_property("text", "MINGMA SHERPA\n1978 - 2026")
t.set_editor_property("world_size", 6.0); t.set_editor_property("horizontal_alignment", unreal.HorizTextAligment.EHTA_CENTER)   # audit: 4.2 was ~10 px at the kneel
t.set_editor_property("vertical_alignment", unreal.VerticalTextAligment.EVRTA_TEXT_CENTER)
t.set_editor_property("text_render_color", unreal.Color(28, 26, 24, 255))
tr.tags = ["Ch1", "father_name"]; tr.set_folder_path("Story/Ch1/Memorial")
# hidden at the ACTOR only: spawn(hidden=True) also hid the component, and the story's 'show' un-hides only the actor, so the
# khata never appeared (audit 2026-11-19)
khata = spawn(AI("khata_tied"), fg + fdir * 14 + V(0, 0, 30), R(0, 0, face - 90), (0.62, 0.62, 0.62), None, ("khata_on_stone",), "Story/Ch1/Memorial",
              collide=False)
khata.set_actor_hidden_in_game(True)
place("father_stone", fg, face)
place("kneel_spot", kg, KYAW)                       # facing the stone
place("nima_stone", nsg, bearing(nsg, fg))          # turned to the stone
place("plume_view", PV, 0)                          # the staged look target (the old fixed one sat 24-32 deg up, off the band)

# ---------------------------------------------------------------- night camp just below the memorials
# (2026-11-19 camera spec 8.13, 10.2) The night scene is played in the main dome's door, where the tents are: you sit on your
# bag in the vestibule, Nima sits 1 m across from you, his headlamp hangs from the door loop between you, the stove glows lower
# right. (audit: the old night_tent place lay BEHIND the tent, on the back wall side.) Night frame: x along nyaw (the tent
# front), y out of the door; the main dome's door is the model's +x, turned to nyaw + 90.
ng, nyaw, nn = on_route(r0, 5650, 1800)
spawn(AI("tent_dome"), ng - V(0, 0, 3), R(0, 0, nyaw + 90), (TENT_S,) * 3, tint("tent_dome", "orange", TENT_COLS[0][1]), ("night_tent",), "Story/Ch1/Night")
n2 = local(ng, nyaw, 420, 260, 0); g2, _ = ground(n2.x, n2.y, ng.z)
spawn(AI("tent_dome"), g2 - V(0, 0, 3), R(0, 0, nyaw + 60), (TENT_S,) * 3, tint("tent_dome", "yellow", TENT_COLS[1][1]), (), "Story/Ch1/Night")
n3 = local(ng, nyaw, -380, 300, 0); g3, _ = ground(n3.x, n3.y, ng.z)
spawn(AI("tent_dome"), g3 - V(0, 0, 3), R(0, 0, nyaw + 120), (TENT_S,) * 3, tint("tent_dome", "red", TENT_COLS[2][1]), (), "Story/Ch1/Night")
N_HALF = AIINFO["tent_dome"]["size_cm"][0] * 0.5 * TENT_S          # the door end of the main dome, from its centre
def nl(x, y, z=0.0):
    p = local(ng, nyaw, x, y, 0); g_, _ = ground(p.x, p.y, ng.z); return g_ + V(0, 0, z)
NSIT, NNIMA = nl(-50, N_HALF + 60), nl(50, N_HALF + 60)               # 1 m apart, both 60 cm out of the door
sg = nl(150, N_HALF + 160)                                           # the stove: front-right of you, ~2 m
spawn(AI("stove_set"), sg - V(0, 0, 1), R(0, 0, nyaw), (1.1, 1.1, 1.1), None, (), "Story/Ch1/Night", collide=False)
for j in range(3):
    dn, dc = DUF_COLS[j]
    spawn(AI("duffel"), local(ng, nyaw, -60 + 50 * j, -330, 0) - V(0, 0, 2) + V(0, 0, ground(local(ng, nyaw, -60 + 50 * j, -330, 0).x, local(ng, nyaw, -60 + 50 * j, -330, 0).y, ng.z)[0].z - ng.z),
          R(0, 0, nyaw + random.uniform(-25, 25)), (1, 1, 1), tint("duffel", dn, dc), (), "Story/Ch1/Night")
light(unreal.PointLight, sg + V(0, 0, 50), R(0, 0, 0), 30.0, (255, 130, 60, 255), 900, ("nightfire",), folder="Story/Ch1/Night")
place("night_tent", nl(0, N_HALF + 100), nyaw + 90)                 # the trigger: the main dome's door
place("night_stove", sg, 0)
place("night_tent_sit", NSIT, bearing(NSIT, NNIMA))
place("night_nima", NNIMA, bearing(NNIMA, NSIT))
for p_ in (NSIT, NNIMA): clear_snow_props(p_, 250)
# Nima's headlamp hangs from the door's top loop, aimed down between you: spot 15 deg, no shadows (a shadowed light costs
# 0.5-1 ms of virtual shadow maps), movable (switched on and off by the story), hidden until the story shows it
HL = local(ng, nyaw, 0, N_HALF + 10, 0)
HL = V(HL.x, HL.y, max(NSIT.z, NNIMA.z) + 120.0)
_aim = (NSIT + NNIMA) * 0.5 + V(0, 0, 40)
hl = EAS.spawn_actor_from_class(unreal.SpotLight, HL, unreal.MathLibrary.make_rot_from_x(_aim - HL))
hlc = hl.light_component
hlc.set_mobility(unreal.ComponentMobility.MOVABLE)
hlc.set_editor_property("intensity_units", unreal.LightUnits.LUMENS)
hlc.set_editor_property("intensity", 300.0)
hlc.set_editor_property("inner_cone_angle", 8.0)
hlc.set_editor_property("outer_cone_angle", 15.0)
hlc.set_editor_property("attenuation_radius", 800.0)
hlc.set_editor_property("cast_shadows", False)
hlc.set_editor_property("light_color", unreal.Color(255, 230, 200, 255))
hl.tags = ["Ch1", "nima_headlamp"]; hl.set_folder_path("Story/Ch1/Night")
hl.set_actor_hidden_in_game(True)
place("nima_headlamp", V(HL.x, HL.y, max(NSIT.z, NNIMA.z)), bearing(HL, _aim))
# the sitting prop (spec 4.3, 10.2): a sleeping bag tagged sleeping_bag, hidden until the sit posture shows it. Its foot end
# runs 60 cm toward Nima (where the hidden legs would be), the rest lies behind you; at tent_start Sangye's own fitted bag is
# the prop and is left untouched (the interior fitting above is frozen)
BAG_N = AIINFO["bedding"]["size_cm"]; BAG_NS = 0.9
_bc = local(NSIT, nyaw, 60 - BAG_N[0] * 0.5 * BAG_NS, 0, 0)
_bg, _ = ground(_bc.x, _bc.y, NSIT.z)
bag_n = spawn(AI("bedding"), _bg, R(0, 0, nyaw), (BAG_NS,) * 3, None, ("sleeping_bag",), "Story/Ch1/Night", collide=False)
bag_n.set_actor_hidden_in_game(True)    # the actor only: the rig's ShowProp un-hides actor and components
_nd = dist2(NSIT, NNIMA)
check("night_tent_sit 100 cm from night_nima", abs(_nd - 100.0) <= 15.0, f"{_nd:.0f} cm, ground {NNIMA.z - NSIT.z:+.0f} cm")
_se = NSIT + V(0, 0, 78)
_sy, _sp = wrap(bearing(_se, sg) - bearing(_se, NNIMA)), elev(_se, sg + V(0, 0, 30))
check("the stove lower right of the night frame", 10 <= _sy <= 40 and _sp < -5, f"{_sy:+.1f} deg right, {_sp:+.1f} deg")
_ne = elev(_se, NNIMA + V(0, 0, 160))
print(f"night frame: Nima's face {_ne:+.1f} deg up from the sitting eye (he stands until sitting clips exist, P2)")
check("nothing between you and Nima", not los_blocked(_se, NNIMA + V(0, 0, 120), 40.0), "line of sight from the sitting eye")
# Nima walks the trail down from the stone (audit: a straight goto cut through the chorten field)
for name, m, side in (("night_w0", 5730, 0), ("night_w1", 5665, 900)):
    g, yaw, n = on_route(r0, m, side)
    place(name, g, yaw)

# ---------------------------------------------------------------- prologue: the Balcony (leg 7), the slip, the fly-out
# (2026-11-19 camera spec 8.1-8.3, 10.2) Staged for the eye: you wake on the rope with the father sitting ahead of you, 25-35 m
# up the rope and 4-6 m above you; the ice gives under you short of him; the one cinematic pulls away from there to the summit.
r7 = route(7)
sx_, sy_, sz_, syaw = at_dist(r7, 99999)
sg_, sn_ = ground(sx_, sy_, sz_)
place("summit", sg_, 0)
FATHER_SIDE = -260.0                                    # he sits 2.6 m off the rope
SIT_HEAD = V(0, 0, 85)                                  # a sitting man's head above the ground
_fcache = {}
best = None
for mf in range(1360, 1521, 10):
    if mf not in _fcache: _fcache[mf] = on_route(r7, mf, FATHER_SIDE)
    fgp, fyw, _ = _fcache[mf]
    for dd in range(20, 48, 3):
        bgp, byw, _ = on_route(r7, mf - dd, 0)
        _di, _ri = dist2(bgp, fgp), fgp.z - bgp.z
        score = band_miss(_di, 2500, 3500) / 100.0 + 2.0 * band_miss(_ri, 400, 600) / 100.0 + abs(mf - 1420) / 100.0
        if best is None or score < best[0]: best = (score, mf, dd, bgp, fgp, fyw, _di, _ri)
_, BF_M, BD, bgp, fgp, fyw, _di, _ri = best
BEYE = bgp + V(0, 0, 156)
place("balcony_start", bgp, bearing(BEYE, fgp + SIT_HEAD))      # facing him (the story derives the pitch from these places)
place("father_sit", fgp, fyw)
_be = elev(BEYE, fgp + SIT_HEAD)
check("balcony_start: the father 25-35 m ahead, 4-6 m up", in_band(_di, 2500, 3500) and in_band(_ri, 400, 600),
      f"route {BF_M - BD} -> {BF_M} m: {_di / 100:.1f} m ahead, {_ri / 100:+.1f} m (the rope is steep here: closest found)")
check("balcony_start: pitch +4 frames the father within 8 deg", abs(_be - 4.0) <= 8.0,
      f"his head {_be:+.1f} deg from the eye (yaw 0 by construction; the story lifts the pitch when this fails)")
check("balcony_start: the father in sight", not los_blocked(BEYE, fgp + SIT_HEAD, 60.0), "line of sight from the eye")
# the slip: on the rope 10 m short of him (father_seen fires within 15 m); the drop is the lower side of the ridge there
SL_M = max(BF_M - 10, BF_M - BD + 5)
slp, slyaw, _ = on_route(r7, SL_M, 0)
place("slip_spot", slp, slyaw)
_dl, _, _ = on_route(r7, SL_M, -1500); _dr, _, _ = on_route(r7, SL_M, 1500)
SDROP = _dl if _dl.z < _dr.z else _dr
place("slip_drop", SDROP, 0)
print("slip_drop", "left" if _dl.z < _dr.z else "right", "of the rope going up;", round(slp.z - SDROP.z), "cm down 15 m out,",
      round(abs(_dl.z - _dr.z)), "cm between the sides")
# the look path of the fly-out: slip_spot -> ridge_mid (the ridge halfway to the summit) -> summit
_rm = (SL_M + r7[-1][0] / 100.0) * 0.5
rmg, rmyaw, _ = on_route(r7, _rm, 0)
place("ridge_mid", rmg, rmyaw)

# -- the fly-out path (spec 6.10, 8.3): mirrored from YggViewRig (uniform Catmull-Rom, arc-length smootherstep, 64 samples)
FLY_T, FLY_MAX_TURN, FLY_CLEAR = 13.0, 12.0, 12000.0
def t_add(a, b): return (a[0] + b[0], a[1] + b[1], a[2] + b[2])
def t_sub(a, b): return (a[0] - b[0], a[1] - b[1], a[2] - b[2])
def t_mul(a, k): return (a[0] * k, a[1] * k, a[2] * k)
def t_len(a): return math.sqrt(a[0] * a[0] + a[1] * a[1] + a[2] * a[2])
def tv(v): return (v.x, v.y, v.z)
def cr(P, S):
    N = len(P)
    if N == 1: return P[0]
    S = min(max(S, 0.0), N - 1.0); i = min(int(math.floor(S)), N - 2); T = S - i
    P0, P1, P2, P3 = P[max(i - 1, 0)], P[i], P[i + 1], P[min(i + 2, N - 1)]
    return tuple(0.5 * ((2 * P1[j]) + (-P0[j] + P2[j]) * T + (2 * P0[j] - 5 * P1[j] + 4 * P2[j] - P3[j]) * T * T
                        + (-P0[j] + 3 * P1[j] - 3 * P2[j] + P3[j]) * T ** 3) for j in range(3))
def build_arc(P):
    segs = max(len(P) - 1, 1); M = 32 * segs; out = [0.0]; last = cr(P, 0.0); tot = 0.0
    for j in range(1, M + 1):
        now = cr(P, j / M * segs); tot += t_len(t_sub(now, last)); out.append(tot); last = now
    return [a / tot for a in out] if tot > 1.0 else [j / M for j in range(M + 1)]
def arc_point(P, arc, U):
    U = min(max(U, 0.0), 1.0); lo, hi = 0, len(arc) - 1
    while hi - lo > 1:
        mid = (lo + hi) // 2
        if arc[mid] <= U: lo = mid
        else: hi = mid
    J = lo + (U - arc[lo]) / max(arc[hi] - arc[lo], 1e-6)
    return cr(P, J / (len(arc) - 1) * max(len(P) - 1, 1))
def smoother(t): t = min(max(t, 0.0), 1.0); return t * t * t * (t * (t * 6 - 15) + 10)
def fly_turn(path, look):
    """(max deg/s at FLY_T, the length the rig retimes it to)"""
    pa, la = build_arc(path), build_arc(look); K = 64; dirs = []
    for i in range(K):
        U = smoother(i / (K - 1)); d = t_sub(arc_point(look, la, U), arc_point(path, pa, U)); L = t_len(d)
        dirs.append(t_mul(d, 1.0 / L) if L > 1e-3 else (dirs[-1] if dirs else (1.0, 0.0, 0.0)))
    planned = FLY_T / (K - 1); mx = 0.0; tot = 0.0
    for i in range(1, K):
        dot = sum(dirs[i - 1][j] * dirs[i][j] for j in range(3))
        deg = math.degrees(math.acos(min(1.0, max(-1.0, dot))))
        mx = max(mx, deg / planned); tot += max(planned, deg / FLY_MAX_TURN)
    return mx, tot
_tz = {}
def tz_cached(x, y):
    k = (int(round(x / 200.0)), int(round(y / 200.0)))
    if k not in _tz: _tz[k] = terrain_z(k[0] * 200.0, k[1] * 200.0)
    return _tz[k]
DISC = [(0.0, 0.0)] + [(r * math.cos(math.radians(a)), r * math.sin(math.radians(a))) for r in (4000.0, 8000.0, 11500.0) for a in range(0, 360, 45)]
_need = {}
def clear_need(x, y):
    """the lowest z at (x, y) that keeps FLY_CLEAR (3D) from every sampled ground point of the disc (visual far terrain included)"""
    k = (int(round(x / 500.0)), int(round(y / 500.0)))
    if k not in _need:
        zr = -1e9
        for dx, dy in DISC:
            z = tz_cached(x + dx, y + dy)
            if z is None: continue
            zr = max(zr, z + math.sqrt(max(0.0, FLY_CLEAR ** 2 - dx * dx - dy * dy)))
        _need[k] = zr
    return _need[k]
def fly_clear(path, s0, s1, per_seg=24):
    """the smallest 3D clearance (cm) along the path curve between curve parameters s0 and s1"""
    worst = None
    n = max(2, int((s1 - s0) * per_seg))
    for i in range(n + 1):
        p = cr(path, s0 + (s1 - s0) * i / n)
        for dx, dy in DISC:
            z = tz_cached(p[0] + dx, p[1] + dy)
            if z is None: continue
            d = math.sqrt(dx * dx + dy * dy + (p[2] - z) ** 2) if p[2] > z else -1.0
            worst = d if worst is None else min(worst, d)
    return worst
SL_T, SUM_T = tv(slp), tv(sg_)
AWAY = bearing(sg_, slp)                                 # straight away from the summit, through the slip spot
DROPB = bearing(slp, SDROP)
def mixdir(a):
    u = unit(AWAY) * math.cos(math.radians(a)) + unit(DROPB) * math.sin(math.radians(a))
    L = max(1e-3, math.hypot(u.x, u.y)); return (u.x / L, u.y / L, 0.0)
LOOK = [SL_T, tv(rmg), SUM_T]
cands = []
for a in (0, 10, 20, 30, 40, 50, 60):
    d_ = mixdir(a)
    def at_out(dist, up, d_=d_): return (SL_T[0] + d_[0] * dist, SL_T[1] + d_[1] * dist, SL_T[2] + up)
    base = [None, at_out(9000.0, 4000.0), at_out(42000.0, 16000.0), (SL_T[0] + d_[0] * 120000.0, SL_T[1] + d_[1] * 120000.0, SUM_T[2])]
    # raise fly_1..fly_3 (2 m steps) until every curve point from fly_1 on keeps 120 m; fly_0 does not move the curve after fly_1
    # much, so the raise is found with a provisional fly_0 and checked again for every fly_0 below
    ex = [0.0, 0.0, 0.0]
    for _ in range(120):
        P = [t_add(SL_T, (0, 0, 400.0))] + [t_add(base[i + 1], (0, 0, ex[i])) for i in range(3)]
        bad = None
        for i in range(49):
            s = 1.0 + 2.0 * i / 48.0; p = cr(P, s)
            if p[2] < clear_need(p[0], p[1]): bad = s; break
        if bad is None: break
        if bad < 2.0: ex[0] += 2000.0; ex[1] += 2000.0          # between fly_1 and fly_2: lift both ends
        else: ex[1] += 2000.0; ex[2] += 2000.0                  # between fly_2 and fly_3
    for a0 in (30, 50, 70, 90):
        d0 = mixdir(a0)
        for out0 in (300.0, 450.0, 600.0):
            f0 = (SL_T[0] + d0[0] * out0, SL_T[1] + d0[1] * out0, SL_T[2] + 400.0)
            P = [f0] + [t_add(base[i + 1], (0, 0, ex[i])) for i in range(3)]
            mx, ln = fly_turn(P, LOOK)
            cands.append((mx, a, a0, out0, P, ex[:], ln))
cands.sort(key=lambda c: (c[0], c[1], abs(c[2] - 90), c[3]))
chosen = None
for c in cands:
    mx, a, a0, out0, P, ex, ln = c
    g0 = tz_cached(P[0][0], P[0][1])
    if g0 is not None and P[0][2] - g0 < 300.0: continue                      # fly_0 over solid ground: not out over the drop
    if los_blocked(V(*SL_T) + V(0, 0, 80), V(*P[0]), 30.0): continue          # the camera inside the snow
    if (fly_clear(P, 0.0, 1.0) or 0.0) < 300.0: continue                      # the first 90 m skims the ridge
    chosen = c; break
if chosen is None:
    chosen = cands[0]; print("fly-out: no candidate kept fly_0 3 m clear: using the smoothest one, CHECK IT")
mx, a, a0, out0, P, ex, ln = chosen
for i, nm in enumerate(("fly_0", "fly_1", "fly_2", "fly_3")):
    place(nm, V(*P[i]), bearing(V(*P[i]), V(*LOOK[min(i, 2)])))
print(f"fly-out: path {a} deg toward the drop, fly_0 {out0:.0f} cm out at {a0} deg, raised fly_1..3 by",
      ", ".join(f"{e / 100:.0f} m" for e in ex))
_c0 = P[0][2] - (tz_cached(P[0][0], P[0][1]) or -1e9)
_c1 = fly_clear(P, 1.0, 3.0)
check("fly_0 >= 3 m above the ground, out over the drop", _c0 >= 300.0, f"{_c0 / 100:.1f} m above the ground under it")
check("fly-out clearance >= 120 m from fly_1 on (far terrain included)", _c1 is not None and _c1 >= FLY_CLEAR,
      f"{(_c1 or 0) / 100:.0f} m at the closest")
check("fly-out turns <= 12 deg/s over 13 s", mx <= FLY_MAX_TURN + 0.05,
      f"{mx:.1f} deg/s at the fastest (the rig retimes 13 s -> {ln:.1f} s)")
check("fly_0 sees the father", not los_blocked(V(*P[0]), fgp + SIT_HEAD, 60.0), "line of sight fly_0 -> father_sit")

# the golden plume, streaming east off the summit (invisible until its Glow rises). (2026-11-19) its bottom edge sits at the
# summit's height, so plume_view (staged from the kneel above) lies on its root; it used to float 1.5-3.5 km above the top
d = V(mg.x - sg_.x, mg.y - sg_.y, 0); d = d / d.length()
xax = V(-d.y, d.x, 0)
if xax.x < 0: xax = xax * -1.0
pl = spawn(PLANE, sg_ + xax * 250000 + V(0, 0, PLUME_HALF_H), unreal.MathLibrary.make_rot_from_zx(d, xax), (5000, 2000, 1), PLUME, ("plume",), "Story/Ch1/Plume", shadow=False, collide=False)
_pk = elev(k_eye, PV)
print(f"plume_view {_pk:+.1f} deg from the kneel; the plume quad spans {elev(k_eye, sg_):+.1f} .. {elev(k_eye, sg_ + V(0, 0, 2 * PLUME_HALF_H)):+.1f} deg")

for a in EAS.get_all_level_actors():
    if a.get_actor_label() == "WorldTree_Flags" and "worldtree_flags" not in [str(t) for t in a.tags]:
        a.tags = list(a.tags) + ["worldtree_flags"]; print("tagged WorldTree_Flags")
json.dump(places, open(f"{ROOT}/Content/Story/Chapter1_places.ygs", "w"), indent=1)
unreal.EditorLevelLibrary.save_current_level()
print("places", len(places))
_fails = [n for n, ok in ASSERTS if not ok]
print(f"staging asserts: {len(ASSERTS) - len(_fails)} of {len(ASSERTS)} passed" + ("" if not _fails else " - FAILED: " + "; ".join(_fails)))
print("REGENERATE THE STORY: run Tools/Story/chapter1_story with python -I (Chapter1.ygs embeds a copy of these places)")
