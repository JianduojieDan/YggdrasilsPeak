# Runs inside the editor (via uepy.py). Places Fab camp props around the RouteLandmark actors.
# Set ONLY = [indices] before exec to limit which camps are built.
import math, random
import unreal

exec(open(ASSETS_PY).read())  # defines A

eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
# The fire-ring scan is stored tilted; this turns its flat side to +Z (found from the vertices' flattest axis).
FIRE_UP = unreal.MathLibrary.make_rot_from_xz(unreal.Vector(0.8212, 0.5537, 0.1379), unreal.Vector(0.5106, -0.6052, -0.6108))
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
meshes = {k: unreal.load_asset(p) for k, p in A.items()}
bounds = {k: m.get_bounding_box() for k, m in meshes.items()}

landmarks = sorted([a for a in eas.get_all_level_actors() if a.get_class().get_name() == 'RouteLandmark'], key=lambda a: a.get_actor_label())
ignore = list(landmarks)

# Prayer-flag strings leave the pole at these angles (RouteLandmark.cpp: 0.4 rad + i * 2pi/n).
def flag_angles(kind_summit):
    n = 6 if kind_summit else 2
    return [math.degrees(0.4 + i * 2 * math.pi / n) % 360 for i in range(n)]

def RV(rot, v):
    return unreal.MathLibrary.greater_greater_vector_rotator(v, rot)

def ground(x, y, z_ref):
    hits = unreal.SystemLibrary.line_trace_multi(world, unreal.Vector(x, y, z_ref + 30000), unreal.Vector(x, y, z_ref - 30000),
        unreal.TraceTypeQuery.ECC_VISIBILITY, True, ignore, unreal.DrawDebugTrace.NONE, True)
    for h in hits:
        t = h.to_tuple()
        a = t[9]
        if a and a.get_class().get_name().startswith('Landscape'):
            return t[5], t[7]
    return None, None

def place(camp, key, local_xy, yaw, scale=1.0, collide=True, align=1.0, sink=10.0, pitch=0.0, roll=0.0, label=None, pre=None):
    base = camp.get_actor_location()
    x, y = base.x + local_xy[0], base.y + local_xy[1]
    p, n = ground(x, y, base.z)
    if p is None:
        print('  no ground for', key); return None
    # Tilt toward the ground normal by 'align' (0 = upright, 1 = fully on the slope).
    up = unreal.Vector(n.x * align, n.y * align, 1.0 - align + n.z * align)
    up = up.normal()
    fwd = unreal.Vector(math.cos(math.radians(yaw)), math.sin(math.radians(yaw)), 0.0)
    rot = unreal.MathLibrary.make_rot_from_zx(up, fwd)
    rot = unreal.MathLibrary.compose_rotators(unreal.Rotator(roll=roll, pitch=pitch, yaw=0.0), rot)
    if pre is not None:
        rot = unreal.MathLibrary.compose_rotators(pre, rot)
    b = bounds[key]
    s = unreal.Vector(scale, scale, scale)
    # Put the rotated bounds' centre over the ground point and its lowest corner on the ground.
    centre = RV(rot, unreal.Vector((b.min.x + b.max.x) * 0.5 * scale, (b.min.y + b.max.y) * 0.5 * scale, (b.min.z + b.max.z) * 0.5 * scale))
    low = min(RV(rot, unreal.Vector(cx * scale, cy * scale, cz * scale)).z
              for cx in (b.min.x, b.max.x) for cy in (b.min.y, b.max.y) for cz in (b.min.z, b.max.z))
    loc = unreal.Vector(p.x - centre.x, p.y - centre.y, p.z - low - sink)
    a = eas.spawn_actor_from_object(meshes[key], loc, rot)
    a.set_actor_scale3d(s)
    c = a.static_mesh_component
    c.set_collision_profile_name('BlockAll' if collide else 'NoCollision')
    c.set_editor_property('mobility', unreal.ComponentMobility.STATIC)
    camp_name = camp.get_editor_property('landmark_name').split(' (')[0]
    a.set_actor_label('%s_%s' % (camp_name.replace(' ', ''), label or key))
    a.set_folder_path('Route/CampProps/' + camp_name)
    a.tags = [unreal.Name('CampProp')]
    ignore.append(a)
    return a

def free_angle(rng, used, flags, min_gap=22):
    for _ in range(200):
        ang = rng.uniform(0, 360)
        if all(min(abs(ang - u) % 360, 360 - abs(ang - u) % 360) > min_gap for u in used + flags):
            used.append(ang); return ang
    ang = rng.uniform(0, 360); used.append(ang); return ang

def polar(ang, r):
    return (math.cos(math.radians(ang)) * r, math.sin(math.radians(ang)) * r)

# Per-camp kit: tents, green tents, mess, fire, o2 bottles, ropes, wind walls, snow piles, axe, seracs
KITS = {
    0: dict(tents=2, green=1, fire=True, o2=0, rope=1, strap=1, walls=0, piles=1, axe=False),   # Dingboche
    1: dict(tents=3, green=1, fire=True, o2=0, rope=1, strap=0, walls=1, piles=1, axe=False),   # Gorak Shep
    2: dict(tents=7, green=2, mess=True, fire=True, o2=6, rope=2, strap=1, walls=2, piles=2, axe=True),  # EBC
    3: dict(tents=2, green=0, o2=2, rope=2, strap=1, walls=1, piles=1, axe=True, seracs=6),    # Icefall
    4: dict(tents=4, green=1, o2=3, rope=1, strap=1, walls=2, piles=2, axe=True),              # Camp 1
    5: dict(tents=4, green=1, mess=True, o2=4, rope=2, strap=1, walls=2, piles=2, axe=True),   # Camp 2 (ABC)
    6: dict(tents=4, green=0, o2=5, rope=1, strap=0, walls=2, piles=1, axe=True),              # Camp 3
    7: dict(tents=5, green=0, o2=8, rope=1, strap=0, walls=3, piles=1, axe=True),              # South Col
    8: dict(tents=0, green=0, o2=2, rope=1, strap=0, walls=0, piles=0, axe=False),             # Summit
}

for idx, camp in enumerate(landmarks):
    if ONLY and idx not in ONLY:
        continue
    kit = KITS[idx]
    rng = random.Random(1000 + idx)
    summit = str(camp.get_editor_property('kind')).endswith('SUMMIT: 2>')
    flags = flag_angles(summit)
    used = []
    print('camp', idx, camp.get_editor_property('landmark_name'))
    folder = 'Route/CampProps/' + camp.get_editor_property('landmark_name').split(' (')[0]
    old = [a for a in eas.get_all_level_actors() if unreal.Name('CampProp') in a.tags and str(a.get_folder_path()) == folder]
    for a in old: eas.destroy_actor(a)
    if old: print('  removed', len(old))
    camp.set_editor_property('num_tents', 0)
    tent_spots = []
    for i in range(kit['tents']):
        ang = free_angle(rng, used, flags, 360.0 / max(kit['tents'] + kit['green'], 1) * 0.55)
        r = rng.uniform(650, 1150) if kit['tents'] < 6 else rng.uniform(750, 1500)
        xy = polar(ang, r)
        # door (+X of the dome) faces the pole
        place(camp, 'tent_modern', xy, ang + 180 + rng.uniform(-25, 25), scale=rng.uniform(1.15, 1.3), align=0.8, label='Tent%d' % i)
        tent_spots.append((ang, r))
    for i in range(kit['green']):
        ang = free_angle(rng, used, flags, 25)
        xy = polar(ang, rng.uniform(800, 1300))
        place(camp, 'tent_green', xy, ang + 90 + rng.uniform(-20, 20), scale=1.0, align=0.8, roll=180.0, label='GreenTent%d' % i)
    if kit.get('mess'):
        ang = free_angle(rng, used, flags, 30)
        place(camp, 'mess', polar(ang, 2000), ang + 90, scale=0.85, align=0.5, sink=20, label='MessTents')
    if kit.get('fire'):
        ang = free_angle(rng, used, flags, 20)
        place(camp, 'fire', polar(ang, 380), rng.uniform(0, 360), scale=0.12, collide=False, align=1.0, sink=8, label='FireRing', pre=FIRE_UP)
    # Oxygen bottles: a depot next to the first tent, mix of standing and lying.
    if kit['o2']:
        da, dr = tent_spots[0] if tent_spots else (free_angle(rng, used, flags, 15), 300)
        cx, cy = polar(da + 9, dr - 200)
        for i in range(kit['o2']):
            ox, oy = cx + rng.uniform(-70, 70), cy + rng.uniform(-70, 70)
            standing = i % 3 != 2
            place(camp, 'o2', (ox, oy), rng.uniform(0, 360), scale=1.3, collide=False, align=0.6 if standing else 1.0,
                  pitch=90.0 if standing else 0.0, sink=4, label='O2_%d' % i)
    for i in range(kit['rope']):
        ang = free_angle(rng, used, flags, 12)
        place(camp, 'rope', polar(ang, rng.uniform(150, 300)), rng.uniform(0, 360), scale=1.8, collide=False, sink=3, label='Rope%d' % i)
    for i in range(kit.get('strap', 0)):
        ang = free_angle(rng, used, flags, 12)
        place(camp, 'strap', polar(ang, rng.uniform(200, 350)), rng.uniform(0, 360), scale=2.0, collide=False, sink=2, label='Strap%d' % i)
    if kit.get('axe') and tent_spots:
        ta, tr = tent_spots[-1]
        # Planted in the snow beside a tent: shaft tilted ~25 degrees from vertical.
        place(camp, 'axe', polar(ta - 10, tr - 180), ta, scale=1.5, collide=False, align=0.0, pitch=-65.0, sink=18, label='IceAxe')
    # Wind walls: snow embankments on the west (prevailing jet-stream side), tangent to the camp.
    for i in range(kit['walls']):
        ang = 180 + (i - (kit['walls'] - 1) / 2.0) * 32 + rng.uniform(-6, 6)
        a = place(camp, 'rockysnow', polar(ang, 1500 + rng.uniform(0, 250)), ang + 90, scale=1.0, align=1.0, sink=25, label='SnowWall%d' % i)
        if a: a.set_actor_scale3d(unreal.Vector(rng.uniform(1.5, 1.9), rng.uniform(1.1, 1.4), rng.uniform(2.0, 2.6)))
    for i in range(kit['piles']):
        ang = free_angle(rng, used, flags, 15)
        key = 'snowpile' if i % 2 == 0 else 'rockysnow'
        place(camp, key, polar(ang, rng.uniform(1700, 2300)), rng.uniform(0, 360), scale=rng.uniform(0.6, 0.9) if key == 'snowpile' else rng.uniform(1.2, 1.6),
              collide=(key == 'rockysnow'), align=1.0, sink=25, label='SnowPile%d' % i)
    for i in range(kit.get('seracs', 0)):
        ang = 20 + i * (300.0 / kit['seracs']) + rng.uniform(-12, 12)
        sc = rng.uniform(1.8, 2.6)
        place(camp, ['ice1', 'ice2', 'ice3'][i % 3], polar(ang, rng.uniform(2200, 3800)), ang + 90 + rng.uniform(-30, 30),
              scale=sc, align=0.1, sink=120 * sc, label='Serac%d' % i)

print('placed total', len([a for a in eas.get_all_level_actors() if unreal.Name('CampProp') in a.tags]))
