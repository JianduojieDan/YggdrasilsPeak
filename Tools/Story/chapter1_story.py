# Offline (python -I): writes Content/Story/Chapter1.ygs, the data the story machine (Source/.../Story/YggStory) plays.
# Chomolungma, Chapter 1 "Day One" (Docs/Development/Chomolungma_Chapter1.md). Places come from
# Content/Story/Chapter1_places.ygs (Tools/Story/build_chapter1_world.py, traced onto the real ground).
# Spoken lines are subtitles ("say"); the author records the voices later (grep the log for "Story [VO]").
#
# 2026-11-19 camera redesign (Docs/Development/Chapter1_Camera_Spec.md, sections 7 and 8): the beats follow the shot cards
# and use only the section 7 camera vocabulary (pose / hold / blink / fall / flyout / nudge / reflex / lure / inspect / ...).
# The story never turns the head: people, sound and light pull the gaze, a budgeted nudge is the last resort, and every
# cut and every spawn happens behind closed eyelids. validate() checks the section 7.4 rules and refuses to write a file
# that breaks them (V15: prints "validate: OK").
import json, math, os, sys
ROOT = "F:/UE_project/YggdrasilsPeak"
places = json.load(open(f"{ROOT}/Content/Story/Chapter1_places.ygs"))
A = "/Game/Audio"
def snd(name): return f"{A}/{name}.{name.split('/')[-1]}"

# Lure sounds the director picks by ear (Chapter1_Camera_Spec 12.6; memory rule: sounds are judged by ear, never chosen
# here). None = the lure / sound is left out and validate() lists it as a warning. Fill in a snd("Folder/Name") path.
BY_EAR = {
    "tent_flap": None,         # duffel lure: the door flap and wind
    "laboured_breath": None,   # Elena's lure: her laboured breathing
    "flag_gust": None,         # memorial lure: a gust snapping the flags
    "ridge_wind": None,        # the ridge wind rising at the stone
}

WARN = []        # reported by validate() (fallback places, sounds still to pick)

# ------------------------------------------------------------------ places the builder adds (2026-11-19)
# Until the lead re-runs build_chapter1_world.py in the editor, a missing new place falls back to a position derived from
# the places that exist (route heights, not traced) so the story still plays; validate() lists every fallback.
def _bearing(a, b): return math.degrees(math.atan2(b[1] - a[1], b[0] - a[0]))
def _wrap(a): return (a + 180.0) % 360.0 - 180.0
def _dir(yaw): return math.cos(math.radians(yaw)), math.sin(math.radians(yaw))
def _off(p, yaw, fwd, right=0.0, up=0.0):
    c, s = _dir(yaw)
    return [round(p[0] + c * fwd - s * right, 1), round(p[1] + s * fwd + c * right, 1), round(p[2] + up, 1)]

_ROUTE = None
def _on_route(leg, metres, side=0.0):
    """[x, y, z, yaw] on the route at `metres` (route z, not traced: fallbacks only)"""
    global _ROUTE
    if _ROUTE is None: _ROUTE = json.load(open(f"{ROOT}/Tools/RoutePlanning/route.json"))
    pts = _ROUTE[leg]["pts"]; s = 0.0
    for i in range(1, len(pts)):
        L = math.dist(pts[i][:2], pts[i - 1][:2])
        if s + L >= metres * 100.0:
            k = (metres * 100.0 - s) / max(1e-3, L)
            p = [pts[i - 1][j] + (pts[i][j] - pts[i - 1][j]) * k for j in range(3)]
            yaw = math.degrees(math.atan2(pts[i][1] - pts[i - 1][1], pts[i][0] - pts[i - 1][0]))
            return _off(p, yaw, 0.0, side) + [round(yaw, 1)]
        s += L
    p = pts[-1]; return [p[0], p[1], p[2], 0.0]

def need(name, make, why):
    if name in places: return
    v = make()
    if v is None: WARN.append(f"place {name} missing and no fallback ({why}) - run build_chapter1_world.py in the editor"); return
    places[name] = [round(x, 1) for x in v]
    WARN.append(f"place {name} missing: fallback {why} - run build_chapter1_world.py in the editor")

CAMP = (339710.0, 2469225.0, -69212.0)                  # the builder's camp centre
FYAW = places["camp_front"][3]
def _camp(f, p): return _off(CAMP, FYAW, f, p) + [FYAW + 180.0]
need("farewell_dolma", lambda: _camp(380, 380), "camp frame (380, 380)")
need("farewell_aunt", lambda: _camp(290, 560), "camp frame (290, 560)")
_ps = 2780.0                                             # porter_start on the route (builder default)
for k in range(8):
    need(f"porter_w{k}", lambda k=k: _on_route(0, _ps - (k + 1) * (_ps - 2450.0) / 9.0, 150), "route leg 0, side 1.5 m")
need("nima_stone", lambda: _off(places["kneel_spot"], places["kneel_spot"][3] - 70.0, 180.0) + [_bearing(places["kneel_spot"], places["father_stone"])],
     "1.8 m front-left of kneel_spot")
need("night_w0", lambda: _on_route(0, 5730), "route leg 0 at 5,730 m")
need("night_w1", lambda: _on_route(0, 5665, 900), "route leg 0 at 5,665 m, 9 m off")
def _night_sit():
    n, t = places["night_nima"], places["night_tent"]
    b = _bearing(n, t)
    return _off(n, b, 100.0) + [_bearing(_off(n, b, 100.0), n)]
need("night_tent_sit", _night_sit, "1 m from night_nima toward night_tent")
need("slip_drop", lambda: None, "the drop side is traced by the builder; the fall goes left")
need("porter_valley", lambda: None, "the valley side is traced by the builder; the knock goes right")
# the fly-out path (section 8.3): out over the drop, then rising and pulling away from the summit
_sl, _su = places["slip_spot"], places["summit"]
_away = _bearing(_su, _sl)
_out = _bearing(_sl, places["slip_drop"]) if "slip_drop" in places else places["slip_spot"][3] - 90.0
need("fly_0", lambda: [a + b for a, b in zip(_off(_sl, _out, 300.0), (0, 0, 400.0))], "4 m above slip_spot, 3 m out, not clearance-checked")
need("fly_1", lambda: _off(_sl, _away, 9000.0, 0.0, 4000.0), "90 m out, +40 m, not clearance-checked")
need("fly_2", lambda: _off(_sl, _away, 42000.0, 0.0, 16000.0), "420 m out, +160 m, not clearance-checked")
need("fly_3", lambda: _off(_sl, _away, 120000.0, 0.0, _su[2] - _sl[2]), "1.2 km out, level with the summit, not clearance-checked")
need("ridge_mid", lambda: [(a + b) * 0.5 for a, b in zip(_sl[:3], _su[:3])], "halfway slip_spot -> summit")

def _side(origin, facing_yaw, toward, default):
    """'left' / 'right': which side of someone facing `facing_yaw` at `origin` the place `toward` lies on"""
    if toward not in places: return default
    c, s = _dir(facing_yaw)
    v = (places[toward][0] - origin[0], places[toward][1] - origin[1])
    return "right" if (-s * v[0] + c * v[1]) > 0 else "left"
# the slip falls toward the drop; the player walks the rope from balcony_start to the father
# (the builder's traced place when there is one: the C++ resolves a place's side from the live view at the moment of the fall,
# which is right wherever the player stands on the rope; left/right is the offline guess for an old places file)
SLIP_SIDE = "slip_drop" if "slip_drop" in places else _side(places["slip_spot"], _bearing(places["balcony_start"], places["father_sit"]), "slip_drop", "left")
# the porter's load knocks you toward the valley; you face up the trail when they come down
KNOCK_SIDE = "porter_valley" if "porter_valley" in places else _side(places["ev_porters"], places["ev_porters"][3], "porter_valley", "right")

# Balcony framing (section 8.1): pitch +4 when the father sits within 8 deg of it; on a steeper rope the head comes up
# far enough to keep him in the upper third (the builder prints the elevation it measured)
def _balcony_pitch():
    b, f = places["balcony_start"], places["father_sit"]
    elev = math.degrees(math.atan2((f[2] + 85.0) - (b[2] + 156.0), math.dist(b[:2], f[:2])))   # his head (sitting) from your eye
    if abs(elev - 4.0) <= 8.0: return 4.0
    WARN.append(f"father_sit is {elev:.1f} deg above the balcony eye: teleport pitch {min(18.0, max(4.0, elev - 6.0)):.1f} instead of +4")
    return round(min(18.0, max(4.0, elev - 6.0)), 1)
BALCONY_PITCH = _balcony_pitch()

def lure(key, at=None, vol=None):
    """a section 7.2 'lure' for a when, or {} while the director has not picked its sound"""
    path = BY_EAR.get(key, key)
    if path is None: WARN.append(f"lure sound '{key}' not picked yet (by ear): that lure is silent"); return {}
    d = {"sound": path}
    if at: d["at"] = at
    if vol is not None: d["vol"] = vol
    return {"lure": d}

beats = []
def beat(id_, when, *do): beats.append({"id": id_, "when": when, "do": [a for a in do if a]})

# ------------------------------------------------------------------ PROLOGUE: the Balcony, 8,400 m, night (card 1)
# Black from frame 0, the cut behind it, 2 s of wind and a fast heart, then the eyes open. Look is free from the start.
beat("prologue", {"start": True},
     {"blink": "closed", "t": 0}, {"preload": "cast"},
     {"cmd": ["ygg.Realm.Force none", "ygg.Survival.Reset", "ygg.Survival.Bottle 900"]},
     {"time": 230}, {"timehold": True, "rate": 0}, {"hide": "worldtree_flags"},
     # audit: no lit stove at an empty camp in daylight, no tent glow from 13 km; props the story shows later start hidden
     {"hide": "nightfire"}, {"hide": "tentlight"}, {"hide": "nima_headlamp"}, {"hide": "sleeping_bag"}, {"hide": "khata_on_stone"},
     {"teleport": "balcony_start", "pitch": BALCONY_PITCH, "cut": True},
     {"npc": "father", "look": "father_ai", "at": "father_sit", "tint": [0.32, 0.05, 0.04], "mode": "sit", "sink": 4, "face": False},
     {"light": "headlamp", "on": True}, {"pace": 0.5, "keep": True},
     {"sound": snd("Weather/Wind_Blizzard"), "vol": 0.75, "fadein": 3, "id": "wind"},
     {"sound": snd("Story/Story_Heartbeat"), "vol": 0.55, "id": "heart"},
     {"wait": 2.0}, {"blink": "wake", "t": 2.8},
     {"hint": "[W]  follow the rope", "idle": 12})                        # only if the player stands still (no Clip key exists)
beat("father_seen", {"after": "prologue", "near": "father", "r": 4500,     # father_sit may be ~42 m out: see him from the start "look": "father", "cone": 12, "dwell": 0.35,
                     **lure(snd("Mind/Whisper_02"), "father", 0.9), "nudge": True},           # nudge 1 of 3
     {"npc_face": "father", "to": "player"}, {"attention": -3, "t": 4},
     {"wait": 1.5}, {"say": "You came too early.", "who": "Father", "t": 4})

# ------------------------------------------------------------------ the slip and the fly-out (cards 2 and 3): the one cinematic
# The ice cracks before anything moves; the fall takes the view (Fall + Look + Move leases) and ends with the eyes shut;
# the fly-out opens from that black at fly_0 and pulls away to the summit; the title rides over it; the lids close.
beat("slip", {"after": "father_seen", "delay": 2.5},
     {"lookhold": True}, {"hold": True},
     {"sound": snd("Ice/Ice_Crack_03"), "vol": 1.0}, {"wait": 0.3},
     {"fall": 1.5, "side": SLIP_SIDE},                                   # toward the drop (builder: slip_drop)
     {"sound": snd("Ice/Avalanche_01"), "vol": 0.6}, {"stopsound": "heart", "fadeout": 0.5},
     {"wait": 1.0},
     {"flyout": "mother", "path": ["fly_0", "fly_1", "fly_2", "fly_3"], "look": ["slip_spot", "ridge_mid", "summit"],
      "t": 13, "fov": [70, 50], "open": 1.5, "wait": False},
     {"wait": 6.5},
     {"title": "CHOMOLUNGMA", "sub": "Goddess Mother of the World", "t": 4.5},
     # "keep": the closed lids outlive this beat, so the dawn's spawns and cut never show (a beat's leases end with its run)
     {"blink": "closed", "t": 2.0, "keep": True}, {"stopsound": "wind", "fadeout": 2.5},
     {"npc_remove": "father"}, {"light": "headlamp", "on": False}, {"pace": False},
     {"lookhold": False}, {"hold": False})

# ------------------------------------------------------------------ DAWN in Sangye's tent, the Dingboche camp (card 4)
# (2026-11-19: the walk-in family room trapped players; the chapter opens in his tent and nothing can block the way out)
# Everyone of day one is dressed here, in the black (the DressAs stall of 2.7 s landed in the open before; audit), then the
# cut to the tent, lying in his bag; the eyes open; the first input (or 4 s) sits him up facing the glowing door.
beat("dawn", {"after": "slip"},
     {"blink": "closed", "t": 0},                                        # this beat owns the closed lids from here
     {"cmd": ["ygg.Survival.Reset"]}, {"show": "worldtree_flags"}, {"show": "tentlight"},
     {"time": 620}, {"timehold": True, "rate": 0.12},                    # audit: 0.2 overtook the dusk glide for slow players
     {"npc": "nima", "look": "nima_ai", "at": "nima_spot", "tint": [0.12, 0.14, 0.17]},
     {"npc": "marek", "look": "marek_ai", "at": "marek_spot", "tint": [0.2, 0.22, 0.08]},
     {"npc": "elena", "look": "elena_ai", "at": "elena_spot", "tint": [0.55, 0.14, 0.05]},
     {"npc": "jiwoo", "look": "jiwoo", "at": "jiwoo_spot", "tint": [0.05, 0.18, 0.3]},
     {"npc": "dolma", "look": "dolma_ai", "at": "dolma_spot", "tint": [0.55, 0.12, 0.08]},
     {"npc": "aunt", "look": "pema", "at": "aunt_spot", "tint": [0.3, 0.06, 0.12]},
     # "up": the climber's capsule centre over the floor. The default 100 fits a capsule half height of 96 cm; the climber's
     # capsule now measures ~110 (bot runs 2026-11-20), so it started 10 cm inside the ground and the physics pushed it out
     # sideways with whatever else it touched (1.2-1.7 m off the spot: the duffel out of reach). 112 = on the floor.
     # Docs/Development/patches/chapter1_duffel_crawl_diagnosis.md
     {"teleport": "tent_start", "cut": True, "up": 112},
     {"pose": "lie", "snap": True, "face": "tent_door", "prop": "sleeping_bag"},
     {"sound": snd("Weather/Wind_Calm"), "vol": 0.35, "fadein": 5, "id": "wind"},
     {"sound": snd("Story/Story_Stove_Loop"), "at": "camp_kitchen", "vol": 0.45, "fadein": 3, "id": "stove"},
     {"caption": "Day 1 / 49 · Dingboche, 4,410 m", "pos": "lowleft", "t": 6, "wait": False},
     {"wait": 2.0}, {"blink": "wake", "t": 2.8},
     {"pose": "sit", "t": 2.2, "after_input": True, "auto": 4, "face": "tent_door", "exit": "tent_out", "exit_ev": 0.4, "prop": "sleeping_bag"},
     {"hint": "Look around.", "idle": 20})
# card 5: the father's duffel, optional (no nudge, no fire-anyway: it opens on [E] only)
beat("duffel", {"after": "dawn", "posture": "sit", "near": "tent_duffel", "r": 160, "look": "tent_duffel", "cone": 18,
                "press": "Interact", "prompt": "[E]  Open", **lure("tent_flap", "tent_duffel"), "nudge": False},
     # (was the card "Father's duffel")
     {"caption": "His red down jacket, patched at the elbow. His prayer beads. His summit certificate, 2019. Under them, two photographs.", "t": 6},
     # (was the card "The first photograph")
     {"inspect": "photo_basecamp", "front": "/Game/Story/Photos/T_Photo_BaseCamp.T_Photo_BaseCamp",
      "text": "Father and Nima at Base Camp, 2019. The mountain behind them, the whole Khumbu below."},
     # (was the card "On the back of the second")
     {"inspect": "photo_icefall", "front": "/Game/Story/Photos/T_Photo_Icefall.T_Photo_Icefall",
      "back_text": "Next time with Sangye. Not before he is ready.", "need_back": True},
     {"flag": "read_photo"})
# card 6: Dolma calls from outside; you crawl out yourself (no fade, no teleport). Her voice is the pull; the reflex is the
# body's turn to it, at the sound's onset (section 6.3)
beat("dolma_calls", {"after": "dawn", "any": [{"after": "duffel", "delay": 3}, {"after": "dawn", "delay": 45}, {"posture": "crawl"}]},
     {"say": "Sangye-dai! Sangye-dai, they are waiting for you!", "who": "Dolma", "at": "dolma_spot", "t": 4, "wait": False},
     {"wait": 0.25}, {"reflex": "tent_door", "deg": 10},
     {"wait": 3.75},
     {"hint": "Hold  [W]  to crawl out", "idle": 15, "t": 6})

# ------------------------------------------------------------------ the farewell: Dolma's hand, the aunt's khata (card 7)
# Standing up outside is the reveal of the camp; Dolma runs into your view. Dolma and the aunt each get their own spot
# by Nima (audit: they ended up inside each other at nima_spot).
beat("farewell", {"after": "dolma_calls", "posture": "stand", "near": "dolma", "r": 800},
     {"stopsound": "stove", "fadeout": 3}, {"hide": "tentlight"},
     {"npc_mode": "dolma", "mode": "present", "dist": 200},
     {"npc_mode": "aunt", "mode": "goto", "to": "farewell_aunt", "speed": 0.8},
     {"say": "Hold tight, Sangye-dai.", "who": "Dolma", "t": 3},
     {"hand": "dolma", "to": "farewell_dolma", "hint": "Hold  [F]  to hold her hand", "pull": 8, "pace": 0.8},
     {"npc_mode": "aunt", "mode": "present", "dist": 90}, {"wait": 1.5},
     {"say": "Promise me you will come down.", "who": "Aunt Pema", "t": 3.5},
     # echo off: Sangye says his answer as a real line (audit: the echo was overwritten at once, or showed "Say nothing.")
     {"choice": ["I promise.", "I'll bring him home.", "Say nothing."], "flags": ["promise", "bringhome", "silent"], "timeout": 16, "default": 2},
     {"say": "I promise.", "who": "Sangye", "if": "promise", "t": 2.4},
     {"say": "I'll bring him home.", "who": "Sangye", "if": "bringhome", "t": 2.6},
     {"say": "Then I will keep the tea warm.", "who": "Aunt Pema", "if": "promise"},
     {"say": "Bring yourself home first.", "who": "Aunt Pema", "if": "bringhome"},
     {"say": "She ties the white khata round your neck. Her hands are cold.", "who": "", "if": "silent"},
     {"say": "She ties the white khata round your neck.", "who": "", "ifnot": "silent"},
     {"wear": "khata", "on": True},
     # (was the card "Dolma's drawing"; no drawing texture yet: the inspect shows its caption)
     {"inspect": "dolma_drawing", "text": "A folded drawing, pushed into your pocket: the mountain, a big figure, and a small one holding its hand."},
     {"say": "Dolma! Come, let him go.", "who": "Aunt Pema", "t": 3},
     {"npc_mode": "dolma", "mode": "goto", "to": "village_way"},
     {"npc_mode": "aunt", "mode": "goto", "to": "village_way"})

# ------------------------------------------------------------------ the job: Nima, the team, the load, the compass (card 8)
# (the team is already standing at the camp: dressed in the dawn's black)
beat("job", {"after": "farewell", "delay": 2},
     {"npc_mode": "nima", "mode": "present", "dist": 150},
     {"say": "Your load. Fifteen kilos.", "who": "Nima"},
     {"say": "Carry what you can carry tomorrow too.", "who": "Nima"},
     {"choice": ["Father's thermos of butter tea", "A second water bottle", "The first-aid kit", "Father's prayer beads"],
      "flags": ["tea", "water2", "firstaid", "mala"], "prompt": "Add one thing to the load", "timeout": 25, "default": 1},
     {"say": "Lobuche tomorrow, Base Camp the day after. Nobody plays hero.", "who": "Marek"},
     {"say": "Is it always this cold, or is it just me?", "who": "Elena"},
     {"sound": snd("Story/Story_Radio_Squelch"), "vol": 0.8},
     {"say": "Morning, team. Clip your oximeters on... Sangye, eighty-eight. Good. Elena, eighty-two. Drink more.", "who": "Dr. Hale (radio)"},
     {"say": "Here. He never went anywhere without it.", "who": "Nima"},
     # (was the card "From Nima's hands")
     {"inspect": "compass", "mesh": "/Game/Props/Compass/SM_Compass2_Case.SM_Compass2_Case", "text": "Father's brass compass. The needle still points north."},
     {"npc_mode": "nima", "mode": "follow", "behind": 260, "side": 150, "talk_side": 110},
     {"npc_mode": "elena", "mode": "follow", "behind": 520, "side": -160},
     {"npc_mode": "jiwoo", "mode": "follow", "behind": 760, "side": 60},
     # audit: Marek walked 2.3 km cross-country to ev_porters and stood alone there; he walks with the team now
     {"npc_mode": "marek", "mode": "follow", "behind": 1000, "side": -60},
     {"say": "We walk. Slowly.", "who": "Nima"},
     {"hint": "Follow the trail up the valley.", "t": 6},
     {"wait": 6}, {"hint": "[Q]  hold up the compass", "t": 5})

# ------------------------------------------------------------------ the trail (card 9)
beat("climb", {"after": "job", "near": "ev_climb", "r": 1600},
     # audit: Dolma and the aunt ticked (MetaHuman animation, grooms) for the rest of the chapter: they go home here, ~200 m
     # behind you (village_way lies 26 m out of the camp; at that distance they are a few pixels if you look back)
     {"npc_remove": "dolma"}, {"npc_remove": "aunt"},
     {"say": "Slow is fast up here. Step, breathe, step.", "who": "Nima"},
     {"hint": "When your breath is loud, slow down: it comes back faster than it goes.", "t": 6})
beat("stream", {"after": "job", "near": "stream_hole", "r": 1500},
     {"sound": snd("Story/Story_Stream_Loop"), "at": "stream_hole", "vol": 0.9, "fadein": 2, "id": "stream"},
     {"say": "Drink before you are thirsty.", "who": "Nima"},
     {"hint": "The stream runs under the ice. Walk to the hole to fill your bottle.", "t": 6})
beat("fill", {"after": "stream", "near": "stream_hole", "r": 200, "look": "stream_hole", "cone": 25,
              "press": "Interact", "prompt": "[E]  Fill your bottle", "nudge": False},
     {"pose": "kneel1", "t": 0.9, "face": "stream_hole", "hold": True}, {"hands": "bottle"},
     {"cmd": "ygg.Survival.Set Hydration 100"},
     {"caption": "The water is so cold it hurts your teeth. Bottle full.", "t": 4},
     {"wait": 2.2}, {"hands": False}, {"hold": False}, {"hint": "[W]  stand up", "idle": 6},
     {"stopsound": "stream", "fadeout": 6})
# Elena stops at a bend ahead (builder: ev_elena); her breathing is the lure, no nudge, the talk fires by 15 s anyway
beat("elena", {"after": "job", "near": "ev_elena", "r": 1300},
     {"stopsound": "stream", "fadeout": 6},       # audit: the river loop must not follow you up the valley if you skip the hole
     {"npc_mode": "elena", "mode": "goto", "to": "ev_elena", "speed": 0.6})
beat("elena_talk", {"after": "elena", "near": "elena", "r": 600, "look": "elena", "cone": 14, "dwell": 0.35, **lure("laboured_breath", "elena", 0.8), "nudge": False},
     {"say": "Sorry... my head. It's like a drum. Just give me a minute.", "who": "Elena"},
     {"choice": ["Carry part of her load", "Give her your water", "Tell her to keep going"],
      "flags": ["elena_load", "elena_water", "elena_push"], "timeout": 18, "default": 0},
     {"cmd": "ygg.Survival.Set Stamina 35", "if": "elena_load"},
     {"say": "Good. That is how it is done.", "who": "Nima", "if": "elena_load"},
     {"cmd": "ygg.Survival.Set Hydration 55", "if": "elena_water"},
     {"say": "Thank you. Really.", "who": "Elena", "if": "elena_water"},
     {"say": "Okay... okay. I'm fine.", "who": "Elena", "if": "elena_push"},
     {"npc_mode": "elena", "mode": "follow", "behind": 520, "side": -160, "speed": 0.85})
# walk-and-talk: Nima at your right shoulder (talk_side), no camera change
beat("why", {"after": "elena_talk", "delay": 25},
     {"say": "Why did you take this job?", "who": "Nima"},
     {"choice": ["For the money.", "For him.", "Say nothing."], "flags": ["money", "forhim", "quiet"], "timeout": 14, "default": 2},
     {"say": "For the money.", "who": "Sangye", "if": "money", "t": 2.4},
     {"say": "For him.", "who": "Sangye", "if": "forhim", "t": 2},
     {"say": "Money is a good reason. An honest one.", "who": "Nima", "if": "money"},
     {"say": "...He would not want you up there for him.", "who": "Nima", "if": "forhim"},
     {"wait": 3, "if": "quiet"},
     {"say": "Your father was quiet too.", "who": "Nima", "if": "quiet"})
# The porters are dressed in a blink while you walk (every spawn behind closed eyes), at porter_start, which the trail cannot
# see until 25 m (builder). They walk the trail's waypoints down (audit: a straight goto left the trail by 12 m), both in
# the porter look (porter2 had none: the old climber body).
# "path": the waypoints walked before "to" (YggStory.cpp npc/npc_mode), "to": the final goal
_porter_path = [f"porter_w{k}" for k in range(8)]
beat("porters_come", {"after": "job", "near": "ev_porters", "r": 2200},
     {"blink": "blink", "t": 0.15, "hold": 0.3, "during": [
         {"npc": "porter1", "look": "porter_a", "at": "porter_start", "tint": [0.22, 0.15, 0.08], "mode": "goto", "to": "porter_end",
          "path": _porter_path, "speed": 1.0},
         {"npc": "porter2", "look": "porter_a", "at": "porter_start", "tint": [0.15, 0.12, 0.1], "mode": "goto", "to": "porter_end",
          "path": _porter_path, "speed": 0.9}]},
     {"say": "Porters coming down. Big loads.", "who": "Nima", "t": 3})
beat("porters_pass", {"after": "porters_come", "near": "porter1", "r": 900},
     {"reflex": "porter1", "deg": 10},
     {"choice": ["Step to the mountain side and wait", "Squeeze past on the outside"], "flags": ["safe", "risky"], "timeout": 7, "default": 0},
     {"cmd": "ygg.Survival.Set Health 78", "if": "risky"},
     {"sound": snd("Footsteps/Step_Rubble_04"), "vol": 1.0, "if": "risky"},
     # the load knocks you toward the valley and you go down on one knee, breathing hard; no teleport (audit: the old one threw
     # you 200-330 m back down the trail)
     {"knock": KNOCK_SIDE, "if": "risky"}, {"pose": "kneel1", "t": 0.6, "hold": True, "if": "risky"},
     {"breath": 0.8, "t": 4, "if": "risky"},
     {"wait": 1.5, "if": "risky"}, {"hold": False, "if": "risky"}, {"pose": "stand", "t": 1.0, "if": "risky"},
     {"pace": 0.8, "t": 60, "keep": True, "if": "risky"},                 # the hurt knee
     {"say": "Always the mountain side. Always. A load like that can push you off the trail.", "who": "Nima", "if": "risky"},
     {"say": "Good. Let them pass.", "who": "Nima", "if": "safe"})
beat("porters_gone", {"after": "porters_come", "delay": 120},          # audit: removed even if the pass never fired
     {"npc_remove": "porter1"}, {"npc_remove": "porter2"})
beat("alone", {"after": "job", "near": "ev_cloud", "r": 1500},
     {"npc_mode": "nima", "mode": "goto", "to": "ev_cloud"},
     {"npc_mode": "elena", "mode": "goto", "to": "ev_cloud"},
     {"npc_mode": "jiwoo", "mode": "goto", "to": "ev_cloud"},
     {"npc_mode": "marek", "mode": "goto", "to": "ev_cloud"},
     {"say": "Go on. I stay with Elena. Follow your father's compass, we will catch up.", "who": "Nima"},
     {"sound": snd("Weather/Wind_Gusty"), "vol": 0.6, "fadein": 6, "id": "wind2"},
     {"stopsound": "wind", "fadeout": 6},
     {"hint": "[Q]  hold up the compass", "t": 5},
     {"fov": -2, "t": 20, "keep": True})                                  # a quiet closing-in while you are alone
beat("together", {"after": "alone", "near": "ev_alone_end", "r": 1600},
     {"npc_mode": "nima", "mode": "follow", "behind": 260, "side": 150, "talk_side": 110},
     {"npc_mode": "elena", "mode": "follow", "behind": 520, "side": -160},
     {"npc_mode": "jiwoo", "mode": "follow", "behind": 760, "side": 60},
     {"npc_mode": "marek", "mode": "follow", "behind": 1000, "side": -60},
     {"wait": 8},
     # his voice from behind first, then the body turns to it
     {"say": "There you are. Good. The pass is above us.", "who": "Nima", "wait": False},
     {"wait": 0.3}, {"reflex": "nima", "deg": 10}, {"fov": 0, "t": 4})

# ------------------------------------------------------------------ dusk at the memorials (card 10)
# The flags are heard before they are seen; ev_flags_seen is a turn where they lie straight ahead (builder)
beat("dusk_comes", {"after": "job", "near": "ev_flags_seen", "r": 2600},
     {"timeto": 1735, "t": 90},                                           # audit: 35 s replayed the sunset for slow players
     {"sound": snd("Story/Story_Flags_Loop"), "at": "memorial", "vol": 1.0, "fadein": 6, "id": "flags"})
beat("flags_seen", {"after": "dusk_comes", "look": "memorial", "cone": 14, "dwell": 0.35,
                    **lure("flag_gust", "memorial"), "nudge": True},      # nudge 2 of 3
     {"attention": -3, "t": 5})
beat("memorial", {"after": "job", "near": "memorial", "r": 1500},
     # audit: followers swung round the kneeling player; they wait here and follow again after the plume
     {"npc_mode": "elena", "mode": "idle"}, {"npc_mode": "jiwoo", "mode": "idle"}, {"npc_mode": "marek", "mode": "idle"},
     # audit: Nima's goal was the player's own kneeling spot; he stands at your left (nima_stone) and turns to the stone
     {"npc_mode": "nima", "mode": "goto", "to": "nima_stone", "speed": 0.8},
     {"say": "This way.", "who": "Nima"},
     {"npc_mode": "nima", "mode": "goto", "to": "nima_stone", "speed": 0.8, "arrive": True, "max": 15},
     {"npc_face": "nima", "to": "father_stone"},
     {"hint": "Kneel at his stone.", "idle": 20})

# ------------------------------------------------------------------ the father's stone (card 11): a played kneel
# [E] only at kneel_spot (builder: facing the stone from there puts the plume 25-35 deg right, 12-18 deg up). Look stays free;
# the move lease is kept until the plume ends.
beat("stone", {"after": "memorial", "near": "kneel_spot", "r": 300,     # 3D from the capsule centre: covers the stone's front and sides "look": "father_stone", "cone": 20,
               "press": "Interact", "prompt": "[E]  Kneel", "nudge": False},
     {"pose": "kneel2", "t": 1.0, "face": "father_stone", "hold": "keep"},
     {"hands": "khata"}, {"holdkey": "F", "t": 2, "prompt": "Hold  [F]  to tie the khata"},
     {"show": "khata_on_stone"}, {"wear": "khata", "on": False}, {"hands": False},
     {"say": "You tie your khata to his name.", "who": ""},
     {"sound": BY_EAR["ridge_wind"], "id": "ridge", "fadein": 3} if BY_EAR["ridge_wind"] else None,
     {"wait": 2.5},
     {"hands": "compass"}, {"compass": {"tremble": 1, "toward": "summit", "t": 20}})
if not BY_EAR["ridge_wind"]: WARN.append("sound 'ridge_wind' not picked yet (by ear): the stone beat has no ridge wind")

# ------------------------------------------------------------------ the gold plume (card 12)
beat("plume", {"after": "stone", "look": "plume_view", "cone": 16, "dwell": 0.35,
               **lure(snd("Story/Story_Plume_Drone"), "plume_view", 0.35), "nudge": {"pitchmax": 12}},   # nudge 3 of 3
     {"param": "Glow", "tag": "plume", "to": 1.0, "t": 11, "pulse": True},
     {"sound": snd("Story/Story_Plume_Drone"), "vol": 0.85},
     {"sound": snd("Story/Story_Heartbeat"), "vol": 0.6, "id": "heart"},
     {"fovbeat": 2.5, "bpm": 96, "t": 7, "wait": False}, {"attention": -3, "t": 4},
     {"exposure": 0.3, "t": 1.5, "hold": 6}, {"breath": 0.4, "t": 6},
     {"wait": 5.5}, {"sound": snd("Ice/Mountain_Boom_02"), "vol": 0.7}, {"wait": 2.0},
     {"reflex": "nima", "deg": 10}, {"say": "Did you hear that?", "who": "Nima"},
     {"stopsound": "heart", "fadeout": 2}, {"stopsound": "ridge", "fadeout": 4},
     {"say": "No. Only the wind. Come, it is getting dark.", "who": "Nima"},
     {"hands": False}, {"compass": {"tremble": 0}}, {"hold": False},
     {"show": "nightfire"},                                               # the stove below is lit: a light to walk down to
     {"npc_mode": "elena", "mode": "follow", "behind": 520, "side": -160},
     {"npc_mode": "jiwoo", "mode": "follow", "behind": 760, "side": 60},
     {"npc_mode": "marek", "mode": "follow", "behind": 1000, "side": -60},
     # audit: a straight goto to night_nima cut through the chorten field; he walks the trail down ("path": waypoints walked before "to")
     {"npc_mode": "nima", "mode": "goto", "to": "night_nima", "path": ["night_w0", "night_w1"], "speed": 0.9},
     # a plain hint clears a pending idle hint: "Follow" first, so "[W] stand" stays armed for a player who stays kneeling
     {"hint": "Follow Nima down to the tents.", "t": 6}, {"hint": "[W]  stand", "idle": 8})

# ------------------------------------------------------------------ night with Nima: the end (card 13)
# A blink at the tent: behind it the clock, the cut to the tent door, sitting on your bag; Nima sits 1 m across from you under
# his headlamp, hung from the door loop (audit: he used to freeze wherever he was, often 50 m away); the others are in their
# tents. (builder: night_tent_sit and night_nima are in the night tent's door, the stove lower right)
beat("night", {"after": "plume", "near": "night_tent", "r": 450},
     {"blink": "blink", "t": 0.15, "hold": 1.2, "during": [
         {"time": 2040}, {"teleport": "night_tent_sit"},
         {"pose": "sit", "snap": True, "face": "night_nima", "prop": "sleeping_bag"},
         {"npc": "nima", "look": "nima_ai", "at": "night_nima", "tint": [0.12, 0.14, 0.17], "mode": "sit", "face": True},
         {"npc_remove": "elena"}, {"npc_remove": "jiwoo"}, {"npc_remove": "marek"},
         {"show": "nima_headlamp"}, {"light": "nima_headlamp", "on": True},
         {"show": "nightfire"}]},
     {"hold": "end"},
     {"stopsound": "flags", "fadeout": 4}, {"stopsound": "wind2", "fadeout": 6},
     {"sound": snd("Story/Story_Stove_Loop"), "at": "night_stove", "vol": 0.55, "fadein": 3, "id": "stove"},
     {"wait": 3},
     {"sound": snd("Story/Story_Radio_Squelch"), "vol": 0.8},
     {"say": "Evening, team. Numbers, please.", "who": "Dr. Hale (radio)"},
     {"say": "Elena is at seventy-six. Watch her tonight.", "who": "Dr. Hale (radio)", "if": "elena_push"},
     {"say": "Elena's at eighty-one. Good work today.", "who": "Dr. Hale (radio)", "ifnot": "elena_push"},
     {"sound": snd("Story/Story_Radio_Squelch"), "vol": 0.6},
     {"wait": 4},
     {"attention": -3, "t": 4, "wait": False}, {"wait": 2},
     {"say": "People say he fell.", "who": "Nima", "t": 3.5},
     {"wait": 2.5},
     {"say": "Your father did not fall.", "who": "Nima", "t": 4},
     {"wait": 1.0},
     # he clicks his headlamp off: darkness, not a fade ("hide" too, so it goes dark even where 'light' is not built yet)
     {"light": "nima_headlamp", "on": False, "t": 0.1}, {"hide": "nima_headlamp"},
     {"wait": 2.0},
     {"blink": "sleep", "t": 2.5}, {"stopsound": "stove", "fadeout": 3},
     {"fade": 1, "t": 0.5},          # the end page's black: closed lids alone reopen after 8 s (Lids lease), the page takes longer
     {"wait": 1.0},
     {"day": 1},
     {"page": [
         "Dingboche. I woke beside his duffel. It still smells of smoke.",
         {"if": "promise", "text": "I promised Aunt Pema I would come down."},
         {"if": "bringhome", "text": "I told Aunt Pema I would bring him home."},
         {"if": "silent", "text": "Aunt Pema asked me to come down. I said nothing."},
         {"if": "tea", "text": "I carry his thermos of butter tea."},
         {"if": "water2", "text": "I carry a second bottle of water."},
         {"if": "firstaid", "text": "I carry the first-aid kit."},
         {"if": "mala", "text": "I carry his prayer beads."},
         {"if": "elena_load", "text": "Elena's head hurt below the pass. I took part of her load."},
         {"if": "elena_water", "text": "I gave Elena my water."},
         {"if": "elena_push", "text": "Elena was sick. I told her to keep going."},
         {"if": "forhim", "text": "Nima asked why I came. For him, I said."},
         {"if": "money", "text": "Nima asked why I came. For the money, I said."},
         {"if": "quiet", "text": "Nima asked why I came. I did not answer."},
         {"if": "risky", "text": "I passed the porters on the outside. I will not again."},
         "At Thukla I tied my khata to his name.",
         "For a moment the mountain turned to gold. Nima saw nothing.",
         "He says my father did not fall."], "head": "Day {day} / 49"},
     {"title": "CHOMOLUNGMA", "sub": "Chapter 1  ·  Day One", "t": 7},
     {"wait": 1.0},
     {"timehold": False},
     # still black: Nima leaves (audit: 'end' destroyed him on screen), you stand up, then the eyes open on the night camp
     {"npc_remove": "nima"},
     # ends in the dark: Stop() keeps the black and fades it out over 2 s, the one reveal of the night camp (no double fade)
     {"blink": "closed", "t": 0}, {"hold": False}, {"pose": "stand", "snap": True},
     {"end": True})


# ================================================================== validate (Chapter1_Camera_Spec 7.4)
# action keys in the order an action is recognised: the ones that carry other actions' names as options come first
# (the C++ dispatch reads "option keys first" the same way)
KINDS = ["pose", "hand", "inspect", "choice", "npc", "npc_mode", "flyout", "holdkey", "blink", "fade", "exposure", "caption", "title",
         "teleport", "say", "hint", "fall", "knock", "nudge", "reflex", "lure", "fovbeat", "attention", "fov", "breath", "hands",
         "pace", "light", "compass", "wear", "preload", "time", "timehold", "timeto", "cmd", "page", "day", "sound", "stopsound",
         "param", "show", "hide", "flag", "unflag", "waitnear", "npc_remove", "npc_face", "lookhold", "hold", "end", "wait"]
REMOVED = {"kneel": "pose", "tumble": "fall / knock", "lock": "hold (+ lookhold in fall/flyout beats)", "card": "caption / inspect"}
WHEN_KEYS = {"start", "after", "delay", "near", "far", "r", "look", "cone", "dwell", "seen", "lure", "lure_at", "nudge", "companion",
             "fire", "press", "prompt", "posture", "any", "idle", "flag", "noflag", "never"}
POSTURES = {"lie", "sit", "kneel1", "kneel2", "crawl", "stand"}
BUILDER_TAGS = {"worldtree_flags", "nightfire", "tentlight", "nima_headlamp", "sleeping_bag", "khata_on_stone", "plume", "duffel",
                "photo_father", "stream", "memorial", "father_stone", "father_name", "sangye_tent", "camp_tent", "night_tent"}

def kind(a):
    for k in KINDS:
        if k in a: return k
    return None

def asset_file(path):
    """/Game/X/Y.Y -> Content/X/Y.uasset"""
    if not isinstance(path, str) or not path.startswith("/Game/"): return None
    return f"{ROOT}/Content/{path[6:].split('.')[0]}.uasset"

def validate(beats, places):
    err, warn = [], list(WARN)
    ids = [b["id"] for b in beats]
    cast = json.load(open(f"{ROOT}/Content/Story/Cast.ygs", encoding="utf-8"))
    people = {"player"}
    for b in beats:
        def walk(acts):
            for a in acts:
                if "npc" in a: people.add(a["npc"])
                walk(a.get("during", []))
        walk(b["do"])
    def place_ok(name, where):
        if not isinstance(name, str): return
        if name not in places and name not in people: err.append(f"{where}: unknown place or person '{name}'")
    budget = {"nudge": 0, "fovbeat": 0, "teleport": 0, "reflex": 0}
    holds = []                     # open Move leases: (beat, condition)
    last = beats[-1]["id"]

    def check_when(w, where, top=True):
        for k in w:
            if k == "timeout": err.append(f"{where}: removed when-key 'timeout' (use lure_at <= 8 / fire <= 16)")
            elif k not in WHEN_KEYS: warn.append(f"{where}: unknown when-key '{k}'")
        for aft in ([w["after"]] if isinstance(w.get("after"), str) else w.get("after", [])):
            if aft not in ids: err.append(f"{where}: after unknown beat '{aft}'")
        for k in ("near", "far", "look", "companion"): place_ok(w.get(k), f"{where} {k}")
        if isinstance(w.get("lure"), dict):
            place_ok(w["lure"].get("at"), f"{where} lure")
            fp = asset_file(w["lure"].get("sound"))
            if fp is None or not os.path.exists(fp): err.append(f"{where}: lure sound not found '{w['lure'].get('sound')}'")
        if w.get("lure_at", 0) > 8: err.append(f"{where}: lure_at {w['lure_at']} > 8")
        if w.get("fire", 0) > 16: err.append(f"{where}: fire {w['fire']} > 16")
        if w.get("posture") and w["posture"] not in POSTURES: err.append(f"{where}: unknown posture '{w['posture']}'")
        if w.get("press") and not w.get("prompt"): warn.append(f"{where}: press without a prompt")
        if w.get("nudge") and top: budget["nudge"] += 1
        # a look / press trigger says whether it may nudge: the C++ escalation (YggStory Conditions) nudges at 12 s unless it
        # reads "nudge": false, so a forgotten key would spend the chapter's budget of 3 on a beat that must never pull
        if top and (w.get("look") or w.get("press")) and "nudge" not in w:
            err.append(f"{where}: look/press trigger without an explicit 'nudge' (true, {{pitchmax}} or false)")
        for i, sub in enumerate(w.get("any", [])): check_when(sub, f"{where} any[{i}]", False)

    for b in beats:
        bid = b["id"]; w = b.get("when", {})
        check_when(w, f"beat {bid} when")
        kinds_here = set()
        def scan(acts):
            for a in acts:
                kinds_here.add(kind(a)); scan(a.get("during", []))
        scan(b["do"])
        state = {"lids": False, "black": False}
        def act(acts, where, in_during=False):
            for i, a in enumerate(acts):
                k = kind(a); at = f"beat {bid} {where}[{i}] {k}"
                cond = tuple(sorted(a.get("if", []) if isinstance(a.get("if"), list) else [a["if"]] if "if" in a else [])) + \
                       tuple("!" + f for f in (a.get("ifnot", []) if isinstance(a.get("ifnot"), list) else [a["ifnot"]] if "ifnot" in a else []))
                for r in REMOVED:
                    if r in a: err.append(f"{at}: removed key '{r}' (use {REMOVED[r]})")
                if k is None:
                    if "look" in a: err.append(f"beat {bid} {where}[{i}]: removed action 'look' (use a when-lure; the story never pulls the head)")
                    else: err.append(f"beat {bid} {where}[{i}]: unknown action {json.dumps(a)[:80]}")
                    continue
                if "look" in a and k not in ("npc", "npc_mode", "flyout"):
                    err.append(f"{at}: removed action 'look' (use a when-lure; the story never pulls the head)")
                closed = state["lids"] or state["black"] or in_during
                if k == "blink":
                    mode = a["blink"]
                    if not isinstance(mode, str) or mode not in ("blink", "closed", "open", "wake", "sleep"):
                        err.append(f"{at}: blink must be blink|closed|open|wake|sleep (old numeric blink)"); continue
                    if mode == "blink":
                        if not a.get("during"): warn.append(f"{at}: blink without 'during'")
                        act(a.get("during", []), f"{where}[{i}].during", True)
                    elif mode in ("closed", "sleep"):
                        if not cond: state["lids"] = True
                    else:
                        state["lids"] = False
                elif k == "fade":
                    if not ({"title", "page"} & kinds_here): err.append(f"{at}: fade only for the title card and the end page (use blink)")
                elif k == "teleport":
                    budget["teleport"] += 1
                    place_ok(a["teleport"], at)
                    if not (closed or (a.get("cut") and w.get("start"))):
                        err.append(f"{at}: teleport '{a['teleport']}' with the eyes open (put it in a blink 'during', after blink closed or under fade 1)")
                elif k == "npc":
                    place_ok(a.get("at"), at); place_ok(a.get("to"), at)
                    for p in a.get("path", []): place_ok(p, at)
                    if a.get("look") and a["look"] not in cast: err.append(f"{at}: look '{a['look']}' is not in Cast.ygs")
                    if not closed: err.append(f"{at}: npc '{a['npc']}' spawned with the eyes open (spawns only in black or behind closed lids)")
                elif k == "npc_mode":
                    place_ok(a["npc_mode"], at); place_ok(a.get("to"), at)
                    for p in a.get("path", []): place_ok(p, at)
                elif k == "pose":
                    if a["pose"] not in POSTURES: err.append(f"{at}: unknown posture '{a['pose']}'")
                    if a.get("snap") and not closed: err.append(f"{at}: pose snap with the eyes open")
                    place_ok(a.get("face"), at); place_ok(a.get("exit"), at)
                    if a.get("hold") in (True, "keep"): holds.append((bid, cond, a["hold"]))
                    elif a.get("hold") == "end": err.append(f"{at}: hold 'end' belongs on a plain hold action")
                elif k == "hold":
                    h = a["hold"]
                    if h in (True, "keep"): holds.append((bid, cond, h))
                    elif h == "end":
                        if bid != last: err.append(f"{at}: hold 'end' only in the last beat")
                    elif h is False:
                        holds[:] = [x for x in holds if cond and x[1] != cond]
                    else: err.append(f"{at}: hold must be true|'keep'|'end'|false")
                elif k == "lookhold":
                    if not ({"fall", "flyout"} & kinds_here): err.append(f"{at}: lookhold only in beats with fall or flyout")
                elif k == "fall":
                    # left / right, or a place: the C++ takes the side that place is on from the live view (spec 8.2: the drop)
                    if a.get("side") not in ("left", "right"):
                        if isinstance(a.get("side"), str) and a["side"] in places: pass
                        else: err.append(f"{at}: fall side must be left|right or a place")
                    if not cond: state["lids"] = True              # the fall ends with the eyelids shut
                elif k == "knock":
                    if a["knock"] not in ("left", "right") and not (isinstance(a["knock"], str) and a["knock"] in places):
                        err.append(f"{at}: knock side must be left|right or a place")
                elif k == "flyout":
                    if not isinstance(a["flyout"], str): err.append(f"{at}: flyout takes an id (old numeric flyout)")
                    for old in ("rise", "back", "face"):
                        if old in a: err.append(f"{at}: removed flyout key '{old}'")
                    for p in a.get("path", []) + a.get("look", []): place_ok(p, at)
                    if not closed: err.append(f"{at}: flyout must start behind closed lids or black")
                    state["lids"] = False                         # it opens from the black
                elif k == "nudge":
                    budget["nudge"] += 1; place_ok(a["nudge"], at)
                elif k == "reflex":
                    budget["reflex"] += 1; place_ok(a["reflex"], at)
                elif k == "lure":
                    place_ok(a["lure"], at)
                elif k == "fovbeat":
                    budget["fovbeat"] += 1
                elif k == "fov":
                    if abs(a["fov"]) > 3: err.append(f"{at}: fov {a['fov']} outside +-3 (story FOV is clamped)")
                elif k in ("npc_face",):
                    place_ok(a["npc_face"], at); place_ok(a.get("to"), at)
                elif k == "hand":
                    place_ok(a["hand"], at); place_ok(a.get("to"), at)
                elif k == "compass":
                    if isinstance(a["compass"], dict): place_ok(a["compass"].get("toward"), at)
                elif k in ("show", "hide"):
                    if a[k] not in BUILDER_TAGS: warn.append(f"{at}: tag '{a[k]}' is not one the builder makes")
                elif k == "end":
                    if bid != last: err.append(f"{at}: end only in the last beat")
                if k in ("sound", "say") and a.get("at"): place_ok(a["at"], at)
                if k == "fade" and not cond:
                    state["black"] = a["fade"] >= 0.99 if a["fade"] >= 0.99 or a["fade"] <= 0.01 else state["black"]
                for f in ("sound", "front", "back", "mesh"):
                    path = a.get(f)
                    if f == "sound" and k != "sound" and k != "lure": continue
                    fp = asset_file(path)
                    if path is not None and (fp is None or not os.path.exists(fp)): err.append(f"{at}: asset not found '{path}'")
        act(b["do"], "do")
        # a plain hold is released in its own beat; only a "keep" hold may wait for a later {"hold": false}
        for x in [x for x in holds if x[0] == bid and x[2] is True]:
            err.append(f"beat {bid}: hold{' if ' + ','.join(x[1]) if x[1] else ''} is not released in its beat (add hold false, or hold 'keep')")
            holds.remove(x)
    for bid, cond, h in holds:
        err.append(f"beat {bid}: hold 'keep'{' if ' + ','.join(cond) if cond else ''} is never released (hold false in a later beat)")
    if budget["nudge"] > 3: err.append(f"{budget['nudge']} nudges (chapter 1 allows 3)")
    if budget["fovbeat"] > 1: err.append(f"{budget['fovbeat']} fovbeats (once per chapter)")
    if budget["teleport"] > 3: warn.append(f"{budget['teleport']} teleports (the shot cards plan 3)")
    if budget["reflex"] > 4: warn.append(f"{budget['reflex']} reflexes (the shot cards plan 4)")
    return err, warn, budget


err, warn, budget = validate(beats, places)
for w_ in warn: print("  warning:", w_)
if err:
    for e in err: print("  ERROR:", e)
    print(f"validate: {len(err)} error(s) - Chapter1.ygs NOT written")
    sys.exit(1)
print("validate: OK", f"({budget['nudge']} nudges, {budget['reflex']} reflexes, {budget['teleport']} teleports, {len(warn)} warnings)")
# "places_file": the places the builder last wrote; the C++ may refresh this embedded copy from it (audit: stale copies)
story = {"chapter": 1, "title": "Day One", "places_file": "Chapter1_places.ygs", "places": places, "beats": beats}
open(f"{ROOT}/Content/Story/Chapter1.ygs", "w", encoding="utf-8").write(json.dumps(story, indent=1, ensure_ascii=False))
print("beats", len(beats), "places", len(places))
