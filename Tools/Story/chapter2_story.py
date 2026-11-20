# Offline (python -I): writes Content/Story/Chapter2.ygs, the data the story machine (Source/.../Story/YggStory) plays.
# Chomolungma, Chapter 2 "The Hidden Mountain" (Docs/Development/Chapter2_Design.md): day 2, the Thukla night camp ->
# Lobuche (1.9 km), the moraine hike or rest, the sick trekker, the night on the bench, Ji-woo's video.
# Same schema and the same camera rules as Chapter 1 (Tools/Story/chapter1_story.py, Chapter1_Camera_Spec.md section 7):
# the story never turns the head, every spawn and every cut happens behind closed eyelids, at most 3 nudges.
# Spoken lines are subtitles ("say"); the director records the voices later (grep the log for "Story [VO]").
#
# Places: Content/Story/Chapter2_places.ygs when Tools/Story/build_chapter2_world.py has run in the editor (it traces every
# spot onto the real ground). Until then each place falls back to the design's DEM coordinates (Chapter2_Design.md 4.1) or
# to an offset from a Chapter 1 place, and validate() lists every fallback as a warning.
#
# Only keys the C++ runtime reads today are used. Where the design asks for a key that does not exist yet ("side", "roll",
# "{spo2:player}", weather, sanity...), the beat uses the nearest existing key or leaves the effect out, and RUNTIME_TODO
# below says what is missing. validate() refuses to write a file that breaks the rules and prints "validate: OK".
import json, math, os, sys
ROOT = "F:/UE_project/YggdrasilsPeak"
A = "/Game/Audio"
def snd(name): return f"{A}/{name}.{name.split('/')[-1]}"

# ------------------------------------------------------------------ switches the director decides (design section 12)
TREKKER_NAME = "Jonas"        # Q5 placeholder; the person id is the lower-case name (subtitles move his body when he talks)
SAY_FATHER_NAME = True        # B11 [Cultural check]: may Pasang say "Mingma" aloud? False -> "You are his son."
GAP_ALWAYS = True             # B14a: no "roll" key yet, so waiting at the cairn always gets the cloud gap (Q10 option)

# Lure and one-shot sounds the director picks by ear (memory rule: sounds are judged by ear, never chosen here). None = the
# sound is left out and validate() lists it. Fill in a snd("Folder/Name") path once a sound is chosen.
BY_EAR = {
    "mantra": None,            # B01/B03: a low murmured Om mani padme hum, one male voice (the lure to the sang smoke)
    "juniper": None,           # B04: juniper crackle on embers
    "camp_morning": None,      # B05: porters talking, a kettle
    "glacier_groan": None,     # B08 on: glacier behind the moraine wall, far and muffled (Ice_Groan_01-04 need muffled cuts)
    "whistle": None,           # B10: a porter's whistle, empty dokos creaking
    "dogs": None,              # B12: dogs far off, Nepali pop from a kitchen radio (must be free or made)
    "lodge_room": None,        # B13-B18: cups, benches, trekkers murmuring
    "sur_hiss": None,          # B16: flour and butter hissing on embers
    "retch": None,             # B18: retching, muffled, off-screen
    "phone_clip": None,        # B21: wind, crampons, "Don't look. Keep moving." (a recorded line)
    "door_wind": None,         # B21: a door, cold wind rushing in
    "roof_snow": None,         # B22: snow on a tin roof, soft
}
# Looks for people who have no model yet (Meshy characters to make, design 9.2). None = the default climber body with a
# tint (validate() lists it). Put the Cast.ygs name here once the character is imported.
LOOKS = {"diki": None, TREKKER_NAME.lower(): None, "pasang": None, "climber": None}
PHONE_STILL = None            # B21: the in-engine render of father_sit at the Balcony, e.g. "/Game/Story/Photos/T_Phone_Balcony.T_Phone_Balcony"
BOTTLE_MESH = "/Game/Story/AI/water_bottle/water_bottle/StaticMeshes/water_bottle.water_bottle"

# What the design needs that the runtime cannot do yet (printed by validate, copied into the hand-off report)
RUNTIME_TODO = [
    "chapter chaining: YggStory Start() clears Flags and sets Day = 1, so Chapter 1's flags (firstaid, elena_push, ...) never reach "
    "Chapter 2, and Chapter 2's flags never reach Chapter 3. Needed: carry flags + body state across chapters, a save at the break, "
    "and a test preset (e.g. ygg.Story.Start Chapter2 flags=firstaid,elena_push)",
    "'side' when-key (B06 chorten clockwise, B10 mountain side): approximated with 'near' + 'any' over builder spots (narrow_drop_k, "
    "chorten_wrong_k)",
    "'roll' (B14a cloud gap, the trekker's fate): GAP_ALWAYS shows the gap every time; on 'leftdiki' trekker_lives is not decided",
    "'{spo2:player}' text token for Dr. Hale's numbers: fixed numbers by flags for now",
    "weather from the story (B14a cloud veil, B15 afternoon snow, the night walk in snow): no key; sounds and lines only",
    "relative stat changes (hydration +5 for the slush, health -10 at the knock, sanity +/-): ygg.Survival.Set is absolute and there "
    "is no sanity command; left out",
    "a stamina when-key (B07: Nima's line only when stamina hits empty): left out",
    "a 'compass raised' when-key or flag (B06/B08 lines 'if the compass is raised'): the B08 line plays always",
    "hands poses for 'juniper' (B04) and for holding the trekker's arm (B19a 'hand jonas'): the rig knows compass/photo/bottle/khata/dolma "
    "only, so the hold works but the arms do not pose",
    "a light that follows a person (Ji-woo's phone glow, B21): left out",
    "TalkBy matches a speaker by id: 'Ji-woo' does not match the person id 'jiwoo', so his body does not 'talk' (strip the hyphen in C++)",
    "trust values (Marek -1, Nima +1): plain flags marek_trust_down / nima_trust_up for now",
    "stamina cap carried to Chapter 3 ('tired'): flag only",
]

WARN = []

# ------------------------------------------------------------------ places
CH1 = json.load(open(f"{ROOT}/Content/Story/Chapter1_places.ygs"))
CH2_FILE = f"{ROOT}/Content/Story/Chapter2_places.ygs"
BUILT = json.load(open(CH2_FILE)) if os.path.exists(CH2_FILE) else {}
if not BUILT: WARN.append("Content/Story/Chapter2_places.ygs not found: every chapter 2 place is a fallback - run build_chapter2_world.py in the editor")
REUSED = ["night_tent", "night_tent_sit", "night_nima", "night_stove", "nima_headlamp", "night_camp", "memorial", "father_stone",
          "kneel_spot", "nima_stone", "summit"]
places = {k: CH1[k] for k in REUSED}
places.update({k: v for k, v in BUILT.items()})          # the builder's copy of a reused place is fresher

def _bearing(a, b): return math.degrees(math.atan2(b[1] - a[1], b[0] - a[0]))
def _dir(yaw): return math.cos(math.radians(yaw)), math.sin(math.radians(yaw))
def _off(p, yaw, fwd, right=0.0, up=0.0):
    c, s = _dir(yaw)
    return [p[0] + c * fwd - s * right, p[1] + s * fwd + c * right, p[2] + up]
def _lerp(a, b, k): return [a[i] + (b[i] - a[i]) * k for i in range(3)]
def _along(a, b, metres, right=0.0):
    """metres from a toward b (right = cm to the right of that direction), facing along it"""
    yaw = _bearing(a, b); L = math.dist(a[:2], b[:2])
    p = _lerp(a, b, min(1.0, metres * 100.0 / max(1.0, L)))
    return _off(p, yaw, 0.0, right) + [yaw]

def need(name, make, why):
    if name in places: return
    v = make()
    if v is None: WARN.append(f"place {name} missing and no fallback ({why})"); return
    if len(v) == 3: v = v + [0.0]
    places[name] = [round(x, 1) for x in v]
    WARN.append(f"place {name}: fallback {why}")

# the story path S1-S5 and the places off leg 0 (Chapter2_Design 4.1, 34 m DEM heights; the builder traces the real ground)
DEM = {"s1": [145541, 2014870, -11507], "s2": [126983, 1992938, -10851], "s3": [115173, 1962569, -10249],
       "s4": [110112, 1925453, -12858], "s5": [111799, 1888336, -12108], "lobuche": [113486, 1855269, -9487],
       "knoll": [153977, 1763489, 2290]}
PATH = ["memorial", "s1", "s2", "s3", "s4", "s5", "lobuche"]
for i, k in enumerate(PATH[1:], 1):
    need(k, lambda k=k, i=i: DEM[k] + [_bearing(DEM.get(PATH[i - 1], places.get(PATH[i - 1])), DEM[k])], "design 4.1 DEM point")
need("knoll", lambda: DEM["knoll"] + [_bearing(DEM["lobuche"], DEM["knoll"])], "design 4.1 DEM point")
P = lambda n: places[n]

# dawn in the night tent and the sang fire at the father's stone
need("night_tent_lie", lambda: P("night_tent_sit")[:3] + [_bearing(P("night_tent_sit"), P("night_tent"))], "night_tent_sit, facing the door")
need("tent2_out", lambda: _off(P("night_tent"), _bearing(P("night_tent_sit"), P("night_tent")), 150.0) + [_bearing(P("night_tent_sit"), P("night_tent"))],
     "1.5 m out of the night tent's door")
need("bottle_ch2", lambda: _lerp(P("night_tent_sit"), P("night_tent"), 0.45), "45% from night_tent_sit to the door")
_kb = _bearing(P("father_stone"), P("kneel_spot"))
need("sang_fire", lambda: _off(P("father_stone"), _kb + 75.0, 160.0), "1.6 m from the father's stone, beside kneel_spot")
need("nima_sang", lambda: _off(P("sang_fire"), _bearing(P("night_camp"), P("sang_fire")), 110.0) + [_bearing(P("night_camp"), P("sang_fire")) + 180.0],
     "1.1 m past the fire, facing the camp")
need("sang_edge", lambda: _off(P("tent2_out"), _bearing(P("tent2_out"), P("memorial")), 900.0), "9 m from the tent door toward the memorials")
_sy = _bearing(P("night_stove"), P("memorial"))
for n, (f, r) in {"camp2_marek": (250, -250), "camp2_elena": (180, 260), "camp2_jiwoo": (-200, 300), "camp2_nima": (220, 60),
                  "camp2_porter1": (-120, -160), "camp2_porter2": (-320, -60)}.items():
    need(n, lambda f=f, r=r: _off(P("night_stove"), _sy, f, r) + [_sy + 180.0], f"night_stove frame ({f}, {r})")

# the valley
need("s1_climb", lambda: _along(P("memorial"), P("s1"), 115.0), "halfway up the climb out of the hollow")
need("lobuche_far", lambda: P("lobuche")[:3], "the Lobuche anchor (look target)")
need("ev_narrow", lambda: P("s3")[:3] + [_bearing(P("s3"), P("s4"))], "S3 (the builder searches the narrowest stretch)")
need("glacier_groan", lambda: _off(P("s2"), 0.0, 25000.0), "250 m east of S2, behind the moraine wall")
# drop-side spots of the narrow stretch: the builder finds the drop; the fallback GUESSES east (toward the moraine)
for k in range(6):
    need(f"narrow_drop_{k}", lambda k=k: _along(P("ev_narrow"), P("s4"), -6.0 + k * 3.6, 90.0), "0.9 m EAST of the path - drop side GUESSED")
need("porter3_start", lambda: _along(P("ev_narrow"), P("s4"), 45.0)[:3] + [_bearing(P("s4"), P("s3"))], "45 m north of ev_narrow")
need("porter3_end", lambda: _along(P("ev_narrow"), P("s2"), 70.0), "70 m south of ev_narrow")
for k in range(4):
    need(f"porter3_w{k}", lambda k=k: _along(P("porter3_start"), P("porter3_end"), (k + 1) * 23.0), "on the straight start-end line")
need("ev_pasang", lambda: _along(P("s4"), P("s5"), 180.0), "180 m past S4")
need("pasang_start", lambda: _along(P("s4"), P("s5"), 260.0)[:3] + [_bearing(P("s5"), P("s4"))], "80 m past ev_pasang")
need("pasang_meet", lambda: _along(P("s4"), P("s5"), 200.0)[:3] + [_bearing(P("s5"), P("s4"))], "20 m past ev_pasang (Nima waits at ev_pasang)")
need("pasang_end", lambda: _along(P("s4"), P("s3"), 120.0), "120 m south of S4 (down to Pheriche)")
need("ev_handover", lambda: _along(P("s5"), P("s4"), 70.0), "70 m south of S5")
need("handover_porter", lambda: _along(P("s5"), P("s4"), 72.0), "beside ev_handover")
for k, m in enumerate((100.0, 220.0, 330.0)):
    need(f"walkdown_w{k}", lambda m=m: _along(P("lobuche"), P("s5"), m), f"{m:.0f} m down the path from Lobuche")

# Lobuche: a local frame at the anchor, +forward toward the path you arrive on (S5), +right; the builder puts the lodge and
# its dining room on the real ground and overwrites all of these
LY = _bearing(P("lobuche"), P("s5"))
def lob(f, r, up=0.0, face=None): return _off(P("lobuche"), LY, f, r, up) + [LY + (face if face is not None else 0.0)]
DIN = (-300.0, -800.0)                                   # the dining room's centre in the Lobuche frame
def din(f, r, face=0.0): return lob(DIN[0] + f, DIN[1] + r, 0.0, face)
for n, v in {"diki_door": lob(600, -300, 0, 180), "lodge_door": lob(300, -650), "terrace": lob(450, 0, 0, 180),
             "trek_bg1": lob(420, 250, 0, 180), "trek_bg2": lob(420, 380, 0, 180), "lodge_porter1": lob(650, 200, 0, 180),
             "trekker_room": lob(-900, -800), "rooms_door": lob(-650, -800), "diki_burner": lob(300, -1100),
             "trekker_arrive": _along(P("lobuche"), P("s5"), 60.0)[:3] + [LY + 180.0],
             "dining": din(0, 0), "lodge_kettle": din(-250, -150), "dining_stove": din(0, 250),
             "dining_window_seat": din(150, -250, 90), "dining_nima": din(-100, 120, -90), "dining_marek": din(-100, -60, -90),
             "dining_elena": din(100, -60, 90), "dining_jiwoo": din(100, 120, 90), "dining_porter1": din(-200, 300, 0),
             "dining_porter2": din(-260, 300, 0), "dining_diki": din(-250, -100, 0), "trekker_corner": din(250, 300, 180),
             "trekker_line_a": din(150, 0, 180), "trekker_line_b": din(-150, 0, 180),
             "bench_sangye": din(0, 330, 180), "bench_nima": din(-150, 330, 180), "bench_bottle": din(0, 300, 0),
             "bench_jiwoo_end": din(80, 330, 180), "bench_porter1": din(-300, 330, 180), "bench_porter2": din(-300, 200, 0),
             "lobuche_porters": lob(500, 450, 0, 180)}.items():
    need(n, lambda v=v: v, "Lobuche frame (lodge not built)")
# the hike
need("knoll_w0", lambda: _lerp(P("lobuche"), P("knoll"), 0.35) + [_bearing(P("lobuche"), P("knoll"))], "35% Lobuche -> knoll")
need("knoll_w1", lambda: _lerp(P("lobuche"), P("knoll"), 0.70) + [_bearing(P("lobuche"), P("knoll"))], "70% Lobuche -> knoll")
need("knoll_wait", lambda: _lerp(P("lobuche"), P("knoll"), 0.97) + [_bearing(P("knoll"), P("lobuche"))], "30 m below the cairn")
need("cloud_front", lambda: _off(P("s2"), 0.0, 0.0, 0.0, 6000.0), "60 m above S2 (the veil climbing from Thukla)")
# the lit face of Nuptse for the cloud gap: 27.9672 N, 86.8869 E (from memory, [To verify]) by the design's pixel formulas
_col = 183.0 + (86.8869 - 86.8106) * 2912.4; _row = 699.2 - (27.9672 - 27.9486) * 3277.1
need("gap_view", lambda: [-504000 + _col * 3374.24, -504000 + _row * 3374.24, (13.4538 * 7200 - 52342 - 32768) * 951.4 / 128 + 129770],
     "Nuptse face at 7,200 m from lat/lon (from memory) - check in the editor")
# B06: the wrong side of the chortens beside the path (builder only: no guessing where chortens stand)
CHORTEN_WRONG = sorted(k for k in places if k.startswith("chorten_wrong_"))
if not CHORTEN_WRONG: WARN.append("no chorten_wrong_* places (builder): the B06 'keep them on your right' line is left out")

# ------------------------------------------------------------------ helpers
def lure(key, at=None, vol=None):
    path = BY_EAR.get(key, key)
    if path is None: WARN.append(f"lure sound '{key}' not picked yet (by ear): that lure is silent"); return {}
    d = {"sound": path}
    if at: d["at"] = at
    if vol is not None: d["vol"] = vol
    return {"lure": d}

def ear(key, **kw):
    """a one-shot or loop the director picks by ear, or None (left out, listed once)"""
    if BY_EAR[key] is None:
        if f"sound '{key}'" not in " ".join(WARN): WARN.append(f"sound '{key}' not picked yet (by ear): left out")
        return None
    return dict({"sound": BY_EAR[key]}, **kw)

def person(id_, at, tint, look=None, **kw):
    """spawn (only inside a blink 'during' or behind closed lids): look from Cast.ygs, or a pending one from LOOKS"""
    a = {"npc": id_, "at": at, "tint": tint}
    lk = look if look is not None else LOOKS.get(id_, LOOKS.get(id_.rstrip("0123456789_ab"), None))
    if lk: a["look"] = lk
    elif id_ in LOOKS or id_.rstrip("0123456789_ab") in LOOKS:
        WARN.append(f"person {id_}: no look yet (default climber body + tint) - asset missing")
    a.update(kw); return a

def say(text, who, **kw): return dict({"say": text, "who": who}, **kw)
def hale(text, **kw): return say(text, "Dr. Hale (radio)", **kw)
RADIO = {"sound": snd("Story/Story_Radio_Squelch"), "vol": 0.8}
TREK = TREKKER_NAME.lower()

beats = []
def beat(id_, when, *do): beats.append({"id": id_, "when": when, "do": [a for a in do if a]})

# ------------------------------------------------------------------ B01 wake2: the tent, 06:10
beat("wake2", {"start": True},
     {"blink": "closed", "t": 0}, {"preload": "cast"},
     {"cmd": ["ygg.Realm.Force none"]},
     {"time": 610}, {"timehold": True, "rate": 0.12},
     {"show": "worldtree_flags"}, {"hide": "tentlight"}, {"hide": "nima_headlamp"}, {"show": "sleeping_bag"},
     {"show": "khata_on_stone"}, {"show": "nightfire"}, {"show": "sang_fire"}, {"show": "bottle_ch2"},
     # everyone is placed in the black: Nima already at the fire by the stone, the team round the stove
     person("nima", "nima_sang", [0.12, 0.14, 0.17], look="nima_ai", mode="idle"),
     person("marek", "camp2_marek", [0.2, 0.22, 0.08], look="marek_ai", mode="idle"),
     person("elena", "camp2_elena", [0.55, 0.14, 0.05], look="elena_ai", mode="idle"),
     person("jiwoo", "camp2_jiwoo", [0.05, 0.18, 0.3], look="jiwoo", mode="idle"),
     person("porter1", "camp2_porter1", [0.22, 0.15, 0.08], look="porter_a", mode="idle"),
     person("porter2", "camp2_porter2", [0.15, 0.12, 0.1], look="porter_a", mode="idle"),
     {"npc_face": "nima", "to": "sang_fire"},
     # "up" 112: the 1.9 m climber's capsule centre over the floor (chapter 1 fix, bot runs 2026-11-20)
     {"teleport": "night_tent_lie", "cut": True, "up": 112},
     {"pose": "lie", "snap": True, "face": "night_tent", "prop": "sleeping_bag"},
     {"sound": snd("Weather/Wind_Calm"), "vol": 0.3, "fadein": 5, "id": "wind"},
     {"sound": snd("Story/Story_Stove_Loop"), "at": "night_stove", "vol": 0.5, "fadein": 3, "id": "stove"},
     {"sound": snd("Story/Story_Flags_Loop"), "at": "memorial", "vol": 0.25, "fadein": 6, "id": "flags"},
     ear("mantra", at="sang_fire", vol=0.5, id="mantra"),
     {"caption": "Day 2 / 49 · Thukla, 4,830 m", "pos": "lowleft", "t": 6, "wait": False},
     {"wait": 2.0}, {"blink": "wake", "t": 2.8},
     {"pose": "sit", "t": 2.2, "after_input": True, "auto": 5, "face": "night_tent", "exit": "tent2_out", "exit_ev": 0.4, "prop": "sleeping_bag"},
     {"hint": "Look around.", "idle": 20})

# B02 bottle_slush: optional; it opens on [E] only (no nudge, no fire-anyway)
beat("bottle_slush", {"after": "wake2", "near": "bottle_ch2", "r": 220, "look": "bottle_ch2", "cone": 20, "seen": True,
                      "press": "Interact", "prompt": "[E]  Pick up", "nudge": False},
     {"hide": "bottle_ch2"},
     {"inspect": "bottle_ch2", "mesh": BOTTLE_MESH, "text": "Half ice. You left it outside your bag."},
     {"hands": "bottle"}, {"caption": "Ice knocks inside. You drink the slush. It hurts your teeth.", "t": 4},
     {"hands": False}, {"flag": "bottle_slush"})

# B03 sang_seen: juniper smoke at dawn (nudge 1 of 3). Outside and standing, the mantra pulls the eye up to the stone.
beat("sang_seen", {"after": "wake2", "near": "tent2_out", "r": 300, "posture": "stand",
                   "look": "sang_fire", "cone": 14, "dwell": 0.35, **lure("mantra", "sang_fire", 0.8), "lure_at": 7,
                   "nudge": True, "fire": 15},
     {"timeto": 630, "t": 60},
     {"hint": "Walk up to Nima at the stone.", "idle": 25})
# nobody walks up: after 90 s Nima comes down to the camp edge, and B04 plays there without the juniper
beat("sang_come", {"after": "sang_seen", "delay": 90, "far": "nima", "r": 600},
     {"flag": "sang_late"},
     {"npc_mode": "nima", "mode": "goto", "to": "sang_edge", "speed": 0.8})

# ------------------------------------------------------------------ B04 sang_offer: at the stone
beat("sang_offer", {"after": "sang_seen", "near": "nima", "r": 300},
     {"npc_face": "nima", "to": "player"},
     say("Every morning he did this. A little smoke for her, so she lets us pass.", "Nima"),
     {"pose": "kneel1", "t": 0.9, "face": "sang_fire", "ifnot": "sang_late"},
     {"holdkey": "F", "t": 2.0, "prompt": "Hold  [F]  to add juniper", "ifnot": "sang_late"},
     ear("juniper", at="sang_fire", vol=0.7),
     say("The green sprigs crackle. The smoke thickens and smells sweet.", "", ifnot="sang_late"),
     {"choice": ["What did you mean last night?", "Teach me the prayer.", "Say nothing."],
      "flags": ["ask_nima", "learn_mani", "quiet2"], "timeout": 16, "default": 2},
     say("What did you mean last night?", "Sangye", **{"if": "ask_nima"}, t=2.6),
     {"npc_face": "nima", "to": "father_stone", "if": "ask_nima"}, {"wait": 2.5, "if": "ask_nima"},
     say("Not here. Not in front of him.", "Nima", **{"if": "ask_nima"}),
     {"wait": 1.5, "if": "ask_nima"},
     say("Walk with me today.", "Nima", **{"if": "ask_nima"}),
     say("Teach me the prayer.", "Sangye", **{"if": "learn_mani"}, t=2.2),
     say("Om mani padme hum. Slowly. With me.", "Nima", **{"if": "learn_mani"}),
     say("Om mani padme hum.", "Nima and Sangye", **{"if": "learn_mani"}, t=4),
     say("He said it fast. Like a man running for a bus.", "Nima", **{"if": "learn_mani"}),
     say("Good. She hears enough talk.", "Nima", **{"if": "quiet2"}),
     {"hint": "[W]  stand", "idle": 8, "ifnot": "sang_late"})

# ------------------------------------------------------------------ B05 break_camp: tea and the brief
beat("break_camp", {"after": "sang_offer"},
     {"npc_mode": "nima", "mode": "goto", "to": "camp2_nima", "speed": 0.8},
     {"timeto": 740, "t": 90},
     ear("camp_morning", at="night_stove", vol=0.6, id="camp"),
     {"sound": snd("Gear/Rope_Tighten_03"), "at": "camp2_porter2", "vol": 0.7},
     say("Short day. Lobuche by lunch, then a little walk up and down again. Climb high, sleep low. Elena, the head?", "Marek"),
     say("Like someone is in there with a hammer. I was sick in the night. Sorry.", "Elena", **{"if": "elena_push"}),
     say("Better. Still a small drum.", "Elena", ifnot="elena_push"),
     say("Thanks for yesterday.", "Elena", ifnot="elena_push"),
     RADIO,
     hale("Morning, Thukla. Numbers, please... Sangye, eighty-five. Good."),
     hale("Elena, seventy-four. Take the Diamox now, and walk behind Nima, slowly.", **{"if": "elena_push"}),
     hale("Elena, eighty. Drink.", ifnot="elena_push"),
     {"sound": snd("Story/Story_Radio_Squelch"), "vol": 0.6},
     say("Say something for the camera?", "Ji-woo"),
     say("Later.", "Nima", t=1.8),
     {"hide": "nightfire"}, {"stopsound": "stove", "fadeout": 4}, {"stopsound": "camp", "fadeout": 4}, {"stopsound": "mantra", "fadeout": 3},
     {"npc_mode": "nima", "mode": "follow", "behind": 260, "side": 150, "talk_side": 110},
     {"npc_mode": "elena", "mode": "follow", "behind": 520, "side": -160},
     {"npc_mode": "jiwoo", "mode": "follow", "behind": 760, "side": 60},
     {"npc_mode": "marek", "mode": "follow", "behind": 1000, "side": -60},
     # the team's porters go ahead with the loads (removed in the porters' blink, waiting again at Lobuche: two MetaHumans
     # walking 1.9 km ahead of you cost VRAM and pathing for nothing)
     {"npc_mode": "porter1", "mode": "goto", "to": "s2", "path": ["s1"], "speed": 1.0},
     {"npc_mode": "porter2", "mode": "goto", "to": "s2", "path": ["s1"], "speed": 0.95},
     {"hint": "Follow the trail up out of the hollow.", "t": 6})

# ------------------------------------------------------------------ B06 pass_stone: the memorials in morning light
beat("pass_stone", {"after": "break_camp", "near": "memorial", "r": 1200},
     {"timeto": 800, "t": 60},
     {"stopsound": "flags", "fadeout": 20},
     {"hint": "[Q]  hold up the compass", "idle": 15})
beat("stone_linger", {"after": "pass_stone", "delay": 20, "near": "father_stone", "r": 400},
     say("He is not in the stone. Come.", "Nima"))
if CHORTEN_WRONG:   # the wrong (anticlockwise) side of the chortens beside the path, from the builder ('side' does not exist)
    beat("chorten_side", {"after": "break_camp", "near": "memorial", "r": 3000,
                          "any": [{"near": k, "r": 150} for k in CHORTEN_WRONG]},
         say("Other side. Keep them on your right.", "Nima"))

# B07 climb_out: 90 m up out of the hollow; the rest-step is yours now (no line: no stamina when-key yet)
beat("climb_out", {"after": "break_camp", "near": "s1_climb", "r": 2000},
     {"timeto": 830, "t": 60})

# ------------------------------------------------------------------ B08 valley_open: the hidden mountain (no nudge)
beat("valley_open", {"after": "climb_out", "near": "s1", "r": 600, "look": "lobuche_far", "cone": 20, "dwell": 0.35,
                     "fire": 10, "nudge": False},
     ear("glacier_groan", at="glacier_groan", vol=0.5),
     say("Lobuche.", "Nima", t=2), {"wait": 2.0},
     say("She is behind Nuptse. Up here she shows herself only when she wants.", "Nima"))

# ------------------------------------------------------------------ B09 nima_or_elena: ahead or behind, chosen by walking
beat("nima_or_elena", {"after": "valley_open", "near": "s2", "r": 800},
     {"timeto": 900, "t": 120},
     # Nima walks ahead alone and waits where Pasang will meet the team; Elena slows down with Marek behind her
     {"npc_mode": "nima", "mode": "goto", "to": "ev_pasang", "path": ["s3", "s4"], "speed": 1.15},
     {"npc_mode": "elena", "mode": "goto", "to": "lobuche", "path": ["s3", "s4", "s5"], "speed": 0.55, "ifnot": "elena_push"},
     {"npc_mode": "elena", "mode": "goto", "to": "lobuche", "path": ["s3", "s4", "s5"], "speed": 0.4, "if": "elena_push"},
     {"npc_mode": "marek", "mode": "goto", "to": "lobuche", "path": ["s3", "s4", "s5"], "speed": 0.55},
     {"hint": "Nima is far ahead. Elena is falling behind.", "idle": 10})
beat("nima_walk", {"after": "nima_or_elena", "delay": 20, "near": "nima", "r": 400, "noflag": ["elena_walk", "elena_called"]},
     {"flag": "nima_walk"},
     {"npc_mode": "nima", "mode": "follow", "behind": 260, "side": 150, "talk_side": 110},
     say("I know what I said.", "Nima", **{"if": "ask_nima"}), {"wait": 25, "if": "ask_nima"},
     say("Your father hummed when he climbed. Always the same song. A very bad singer.", "Nima"),
     say("On the Lhotse Face you could hear him two ropes away.", "Nima"),
     {"wait": 3},
     say("On the mountain he was the best of us. Remember that, whatever you hear.", "Nima"),
     {"flag": "nima_memory"},
     # Elena and Marek come on behind, slowly
     {"npc_mode": "elena", "mode": "follow", "behind": 1500, "side": -160, "speed": 0.9},
     {"npc_mode": "marek", "mode": "follow", "behind": 1800, "side": -60, "speed": 0.9})
ELENA_TALK = [
    {"npc_mode": "elena", "mode": "goto", "to": "lobuche", "path": ["s3", "s4", "s5"], "speed": 0.55},
    say("You don't have to babysit me.", "Elena"),
    {"wait": 6},
    say("Step, breathe, step... Nima taught you that?", "Elena"),
    {"choice": ["My father did.", "Nima did.", "Say nothing."], "flags": ["taught_father", "taught_nima", ""], "timeout": 10, "default": 2},
    say("My father did.", "Sangye", **{"if": "taught_father"}, t=2),
    say("Nima did.", "Sangye", **{"if": "taught_nima"}, t=1.8),
    {"flag": "elena_helped"},
    {"wait": 8},
    {"npc_mode": "elena", "mode": "follow", "behind": 300, "side": -160, "speed": 0.85},
    {"npc_mode": "marek", "mode": "follow", "behind": 1000, "side": -60}]
beat("elena_walk", {"after": "nima_or_elena", "delay": 45, "near": "elena", "r": 1500, "noflag": ["nima_walk", "elena_called"],
                    "any": [{"far": "nima", "r": 400}]},
     {"flag": "elena_walk"}, *ELENA_TALK)
# neither by 90 s: Marek calls you back, and the Elena talk plays when you reach her
beat("elena_default", {"after": "nima_or_elena", "delay": 90, "noflag": ["nima_walk", "elena_walk"], "far": "elena", "r": 1500},
     {"flag": "elena_called"},
     say("Sangye! Stay with us.", "Marek"))
beat("elena_walk2", {"after": "elena_default", "near": "elena", "r": 1500, "noflag": ["nima_walk", "elena_walk"]},
     {"flag": "elena_walk"}, *ELENA_TALK)

# ------------------------------------------------------------------ B10 porters_down: the rule, alone (no prompt)
# Three porters are dressed in a blink 90 m before the narrow stretch, then wait until you are close, then come down.
# 'side' does not exist: being within 1.5 m of a drop-side spot (narrow_drop_k, builder) when the lead porter is 3 m away
# is the wrong side; the safe beat needs him closer (2.2 m), so the two cannot fire in the same frame.
_drop = [f"narrow_drop_{k}" for k in range(6)]
_pp = [f"porter3_w{k}" for k in range(4)]
beat("porters_spawn", {"after": "valley_open", "near": "ev_narrow", "r": 9000},
     {"blink": "blink", "t": 0.15, "hold": 0.3, "during": [
         {"npc_remove": "porter1"}, {"npc_remove": "porter2"},
         person("porter3", "porter3_start", [0.25, 0.16, 0.08], look="porter_a", mode="idle"),
         person("porter4", "porter3_start", [0.18, 0.14, 0.1], mode="idle"),
         person("porter5", "porter3_start", [0.3, 0.2, 0.12], mode="idle")]})
beat("porters_walk", {"after": "porters_spawn", "near": "ev_narrow", "r": 2500},
     ear("whistle", at="porter3", vol=0.9),
     {"npc_mode": "porter3", "mode": "goto", "to": "porter3_end", "path": _pp, "speed": 1.0},
     {"npc_mode": "porter4", "mode": "goto", "to": "porter3_end", "path": _pp, "speed": 0.95},
     {"npc_mode": "porter5", "mode": "goto", "to": "porter3_end", "path": _pp, "speed": 0.9},
     {"wait": 0.25}, {"reflex": "porter3", "deg": 10})
beat("porters_risky", {"after": "porters_walk", "near": "porter3", "r": 300, "noflag": "safe2",
                       "any": [{"near": k, "r": 150} for k in _drop]},
     {"flag": "risky2"},
     {"sound": snd("Footsteps/Step_Rubble_04"), "vol": 1.0},
     {"knock": "narrow_drop_2"}, {"pose": "kneel1", "t": 0.6, "hold": True},
     {"breath": 0.8, "t": 4},
     {"wait": 1.5}, {"hold": False}, {"pose": "stand", "t": 1.0},
     say("The mountain side. Yesterday you knew it.", "Nima", **{"if": "nima_walk"}),
     say("The mountain side, Sangye. Always.", "Marek", ifnot="nima_walk"))
beat("porters_safe", {"after": "porters_walk", "near": "porter3", "r": 220, "noflag": "risky2"},
     {"flag": "safe2"},
     say("Namaste, dai.", "Porter", t=2))
beat("porters_gone", {"after": "porters_walk", "delay": 150},
     {"npc_remove": "porter3"}, {"npc_remove": "porter4"}, {"npc_remove": "porter5"})

# ------------------------------------------------------------------ B11 pasang: a man who knew him
beat("pasang_spawn", {"after": "valley_open", "near": "s4", "r": 4000},
     {"blink": "blink", "t": 0.15, "hold": 0.3, "during": [
         person("pasang", "pasang_start", [0.45, 0.06, 0.05], mode="goto", to="pasang_meet", speed=0.8),
         person("climber_a", "pasang_start", [0.1, 0.25, 0.45], look=LOOKS["climber"] or "", mode="goto", to="pasang_meet", speed=0.75),
         person("climber_b", "pasang_start", [0.5, 0.45, 0.1], look=LOOKS["climber"] or "", mode="goto", to="pasang_meet", speed=0.7)]})
beat("pasang", {"after": "pasang_spawn", "near": "pasang", "r": 600},
     {"npc_mode": "pasang", "mode": "present", "dist": 150},
     {"npc_mode": "climber_a", "mode": "idle"}, {"npc_mode": "climber_b", "mode": "idle"},
     {"npc_mode": "nima", "mode": "goto", "to": "player", "speed": 0.9},
     {"npc_face": "jiwoo", "to": "pasang"},
     say("You are Mingma's son. You have his face." if SAY_FATHER_NAME else "You are his son. You have his face.", "Pasang", wait=False),
     {"wait": 0.25}, {"reflex": "pasang", "deg": 10}, {"wait": 3.5},
     {"caption": "He takes your hands in both of his.", "t": 3},
     {"npc_face": "pasang", "to": "nima"},
     say("[in Sherpa]", "Pasang", t=3),
     {"npc_face": "nima", "to": "pasang"},
     say("[in Sherpa]", "Nima", t=1.6),
     {"npc_face": "pasang", "to": "player"},
     say("The Icefall is bad this year. A kitchen boy from Base Camp died there last week.", "Pasang"),
     say("The doctors moved the ladders twice. Go slowly in there.", "Pasang"),
     {"npc_mode": "pasang", "mode": "goto", "to": "pasang_end", "speed": 0.8},
     {"npc_mode": "climber_a", "mode": "goto", "to": "pasang_end", "speed": 0.75},
     {"npc_mode": "climber_b", "mode": "goto", "to": "pasang_end", "speed": 0.7},
     {"npc_mode": "nima", "mode": "follow", "behind": 260, "side": 150, "talk_side": 110},
     {"npc_mode": "jiwoo", "mode": "follow", "behind": 760, "side": 60})
beat("pasang_gone", {"after": "pasang", "delay": 120},
     {"npc_remove": "pasang"}, {"npc_remove": "climber_a"}, {"npc_remove": "climber_b"})

# ------------------------------------------------------------------ B12 lobuche: midday (the village is dressed in a blink 330 m out)
beat("lobuche_spawn", {"after": "valley_open", "near": "s5", "r": 4000},
     {"timeto": 1030, "t": 150},
     {"blink": "blink", "t": 0.15, "hold": 0.35, "during": [
         person("diki", "diki_door", [0.4, 0.05, 0.1], mode="idle"),
         person("porter1", "lobuche_porters", [0.22, 0.15, 0.08], look="porter_a", mode="idle"),
         person("porter2", "lobuche_porters", [0.15, 0.12, 0.1], look="porter_a", mode="idle"),
         person("lodge_porter", "lodge_porter1", [0.2, 0.18, 0.12], mode="idle"),
         person("trek_bg1", "trek_bg1", [0.2, 0.35, 0.6], mode="sit"),
         person("trek_bg2", "trek_bg2", [0.6, 0.3, 0.2], mode="sit"),
         person(TREK, "trekker_room", [0.3, 0.45, 0.65], mode="idle")]},
     ear("dogs", at="lobuche", vol=0.7, id="dogs"))
beat("lobuche", {"after": "lobuche_spawn", "near": "lobuche", "r": 4000},
     {"npc_face": "diki", "to": "player"},
     say("Tashi delek! Nima-dai! Come, come, the tea is ready.", "Diki"),
     say("You stayed with her. Good. That is the job.", "Nima", **{"if": "elena_helped"}),
     say("Where was my Sherpa?", "Marek", **{"if": "nima_walk"}),
     {"flag": "marek_trust_down", "if": "nima_walk"},
     {"stopsound": "dogs", "fadeout": 8},
     ear("lodge_room", at="dining", vol=0.5, id="room"),
     {"npc_mode": "nima", "mode": "goto", "to": "dining_nima", "speed": 0.9},
     {"npc_mode": "diki", "mode": "goto", "to": "dining_diki", "speed": 0.8},
     {"hint": "Diki has boiled water inside.", "t": 6})

# ------------------------------------------------------------------ B13 refill_lunch: water, dal bhat, the AMS check
beat("refill_lunch", {"after": "lobuche", "near": "diki", "r": 400},
     say("Boiled water. Fifty rupees, but for Nima's boy, nothing.", "Diki"),
     {"holdkey": "Interact", "t": 1.5, "prompt": "Hold  [E]  to fill your bottle"},
     {"hands": "bottle"}, {"cmd": "ygg.Survival.Set Hydration 100"},
     {"caption": "Hot water. The bottle warms your hands.", "t": 3.5},
     {"hands": False},
     # lunch: the team sits down in a blink (they come in from different distances; nobody walks through a wall)
     {"blink": "blink", "t": 0.2, "hold": 0.6, "during": [
         {"npc_mode": "nima", "at": "dining_nima", "mode": "sit"},
         {"npc_mode": "marek", "at": "dining_marek", "mode": "sit"},
         {"npc_mode": "elena", "at": "dining_elena", "mode": "sit"},
         {"npc_mode": "jiwoo", "at": "dining_jiwoo", "mode": "sit"},
         {"npc_mode": "porter1", "at": "dining_porter1", "mode": "sit"},
         {"npc_mode": "porter2", "at": "dining_porter2", "mode": "sit"},
         {"time": 1130}]},
     {"caption": "Dal bhat. A second plate comes without asking.", "t": 4},
     say("Dal bhat power, twenty-four hour!", "Porter", t=2.6),
     say("Headache, one to three?", "Marek"),
     {"flag": "elena_sick", "if": "elena_push", "ifnot": "elena_helped"},
     say("Three.", "Elena", **{"if": "elena_sick"}, t=1.6),
     say("One. Maybe two.", "Elena", ifnot="elena_sick", t=2),
     say("Then you rest this afternoon, and if it is worse tonight we talk about going down.", "Marek", **{"if": "elena_sick"}))

# ------------------------------------------------------------------ B14 afternoon: climb high, sleep low
beat("afternoon", {"after": "refill_lunch", "delay": 20},
     {"timeto": 1300, "t": 30},
     say("Who walks up the moraine with Nima? One hour. Up, look, back. It helps the night.", "Marek"),
     {"choice": ["I'll go up.", "I'll rest here."], "flags": ["hike", "rest"], "timeout": 16, "default": 1},
     say("I'll go up.", "Sangye", **{"if": "hike"}, t=1.8),
     say("I'll rest here.", "Sangye", **{"if": "rest"}, t=1.8),
     {"npc_mode": "nima", "mode": "goto", "to": "knoll", "path": ["knoll_w0", "knoll_w1"], "speed": 0.9, "if": "hike"},
     {"npc_mode": "jiwoo", "mode": "follow", "behind": 400, "side": -120, "if": "hike"},
     {"hint": "Follow Nima up the moraine.", "t": 6, "if": "hike"})

# B14a hike: the moraine knoll, 5,051 m
beat("hike_ask", {"after": "afternoon", "flag": "hike", "near": "knoll_w1", "r": 2500},
     say("Can I ask about your father?", "Ji-woo"),
     {"choice": ["Ask.", "Not today."], "flags": ["jiwoo_talk", "jiwoo_no"], "timeout": 12, "default": 1},
     say("Not today.", "Sangye", **{"if": "jiwoo_no"}, t=1.8),
     say("What was he like?", "Ji-woo", **{"if": "jiwoo_talk"}),
     say("He sang when he climbed. Badly.", "Sangye", **{"if": ["jiwoo_talk", "nima_memory"]}),
     say("He was always leaving. Every spring.", "Sangye", **{"if": "jiwoo_talk"}, ifnot="nima_memory"),
     {"caption": "Ji-woo goes quiet and looks at his phone.", "t": 3, "if": "jiwoo_talk"})
beat("hike_top", {"after": "afternoon", "flag": "hike", "near": "knoll", "r": 1200},
     {"timeto": 1400, "t": 40})
beat("cloud", {"after": "hike_top", "delay": 20, "look": "cloud_front", "cone": 18, "dwell": 0.35,
               "nudge": {"pitchmax": 4}, "fire": 15},                                        # nudge 2 of 3
     {"sound": snd("Weather/Wind_Gusty"), "vol": 0.6, "fadein": 8, "id": "wind2"},
     say("Down now.", "Nima", t=1.6),
     {"npc_mode": "nima", "mode": "goto", "to": "knoll_wait", "speed": 0.9})
beat("cloud_down", {"after": "cloud", "far": "knoll", "r": 2500, "noflag": "stayed"},
     {"flag": "went_down"},
     {"npc_mode": "nima", "mode": "follow", "behind": 260, "side": 150, "talk_side": 110})
beat("cloud_stay", {"after": "cloud", "delay": 25, "near": "knoll", "r": 2500, "noflag": "went_down"},
     {"flag": "stayed"},
     say("Sangye!", "Nima", t=1.5),
     {"wait": 35},
     # no 'roll' yet: GAP_ALWAYS (the no-gap outcome needs 'roll' and story weather; RUNTIME_TODO)
     {"flag": "gap_seen"} if GAP_ALWAYS else None,
     {"exposure": 0.3, "t": 2.0, "hold": 6} if GAP_ALWAYS else None,
     {"caption": "A hole opens in the cloud. Sunlight runs across the face of Nuptse.", "t": 5} if GAP_ALWAYS else None,
     {"wait": 4},
     say("Now we go.", "Nima", t=1.6),
     {"npc_mode": "nima", "mode": "follow", "behind": 260, "side": 150, "talk_side": 110})
beat("hike_back", {"after": "afternoon", "flag": "hike", "near": "lobuche", "r": 3000, "any": [{"after": "cloud"}, {"delay": 900}]},
     {"flag": "hike_done"}, {"stopsound": "wind2", "fadeout": 8},
     {"npc_mode": TREK, "at": "trekker_corner", "mode": "sit"},          # he came up while you were away (inside, out of view)
     {"npc_mode": "nima", "mode": "goto", "to": "dining_nima", "speed": 0.9},
     {"npc_mode": "jiwoo", "mode": "goto", "to": "dining_jiwoo", "speed": 0.9})

# B14b rest: the lodge in the afternoon
beat("rest_sit", {"after": "afternoon", "flag": "rest", "near": "dining_window_seat", "r": 250, "look": "dining_window_seat",
                  "cone": 30, "press": "Interact", "prompt": "[E]  Sit by the window", "nudge": False},
     {"pose": "sit", "t": 1.2, "face": "dining", "hold": False},
     say("How do you say thank you, in your language?", "Elena", **{"if": "elena_helped"}),
     say("Thuk-je-che.", "Sangye", **{"if": "elena_helped"}, t=2),
     say("Thuk... thuk-je-che?", "Elena", **{"if": "elena_helped"}, t=2.4),
     {"caption": "The porters laugh, kindly.", "t": 3, "if": "elena_helped"})
beat("rest_trekker", {"after": "afternoon", "flag": "rest", "any": [{"after": "rest_sit", "delay": 40}, {"delay": 240}]},
     {"npc_mode": TREK, "at": "trekker_arrive", "mode": "goto", "to": "dining", "speed": 1.1, "arrive": True, "max": 50},
     {"npc_mode": TREK, "mode": "present", "dist": 180},
     say("Came up from Pheriche this morning. Piece of cake!", TREKKER_NAME),
     {"choice": ["Go slower. Drink.", "Say nothing."], "flags": ["warned", ""], "timeout": 10, "default": 1},
     say("Go slower. Drink.", "Sangye", **{"if": "warned"}, t=2),
     say("Yeah, yeah. Thanks, mate.", TREKKER_NAME, **{"if": "warned"}),
     {"npc_mode": TREK, "mode": "goto", "to": "trekker_corner", "speed": 0.9})

# ------------------------------------------------------------------ B15 snow: the afternoon closes in
beat("snow", {"after": "afternoon", "any": [{"flag": "rest", "delay": 600}, {"after": "hike_back"}, {"delay": 1500}]},
     {"timeto": 1600, "t": 60},
     {"npc_mode": "diki", "mode": "goto", "to": "dining_stove", "speed": 0.8, "arrive": True, "max": 20},
     {"show": "dining_stove"},
     {"sound": snd("Story/Story_Stove_Loop"), "at": "dining_stove", "vol": 0.5, "fadein": 4, "id": "stove"})
beat("sur_start", {"after": "snow", "delay": 60},
     {"npc_mode": "diki", "mode": "goto", "to": "diki_burner", "speed": 0.8},
     {"show": "sur_fire"},
     ear("sur_hiss", at="diki_burner", vol=0.6, id="sur"),
     {"hint": "Diki has gone outside to the burner.", "t": 5})
# B16 sur: optional, you walk out to her
beat("sur", {"after": "sur_start", "near": "diki_burner", "r": 500, "any": [{"near": "diki", "r": 600}]},
     {"npc_face": "diki", "to": "player"},
     say("Sur. For the hungry ones who wander. They cannot eat, only smell. So we give them smoke.", "Diki"),
     {"holdkey": "F", "t": 1.5, "prompt": "Hold  [F]  to sprinkle tsampa"},
     {"flag": "sur_joined"},
     say("Your father ate here every spring. Always two plates of dal bhat. Always he paid for the porters' tea.", "Diki"),
     {"npc_mode": "diki", "mode": "goto", "to": "dining_diki", "speed": 0.8})

# B17 radio_dusk: evening numbers (fixed by flags until the {spo2:player} token exists)
beat("radio_dusk", {"after": "snow", "delay": 90, "near": "dining", "r": 600},
     {"timeto": 1830, "t": 40},
     {"npc_mode": "diki", "mode": "goto", "to": "dining_diki", "speed": 0.8},
     RADIO,
     hale("Evening, Lobuche. Numbers..."),
     hale("Sangye, eighty-six. The walk up did you good.", **{"if": "hike"}),
     hale("Sangye, eighty-four.", ifnot="hike"),
     hale("Elena, seventy-three. I don't like that.", **{"if": ["elena_sick", "nima_walk"]}),
     hale("Elena, seventy-six. Watch her tonight.", **{"if": "elena_sick"}, ifnot="nima_walk"),
     hale("Elena, eighty-four. Very good.", **{"if": "elena_helped"}),
     hale("Elena, seventy-eight. Drink, and sleep sitting up a little.", **{"if": "nima_walk"}, ifnot="elena_sick"),
     hale("Elena, eighty-one. Good.", ifnot=["elena_sick", "elena_helped", "nima_walk"]),
     hale("Base Camp tomorrow, then. I'll have the kettle on."),
     {"sound": snd("Story/Story_Radio_Squelch"), "vol": 0.6})

# ------------------------------------------------------------------ B18 stranger: the heel-to-toe test, the peak
beat("stranger", {"after": "radio_dusk", "delay": 40},
     {"timeto": 1930, "t": 30},
     {"npc_mode": TREK, "mode": "sit"},
     ear("retch", at="trekker_corner", vol=0.6),
     say("That boy. He is not right.", "Diki"),
     say("Sangye. Ask him to walk.", "Nima"))
beat("stranger_test", {"after": "stranger", "near": TREK, "r": 400, "look": TREK, "cone": 25, "press": "Interact",
                       "prompt": "[E]  Ask him to walk the line", "nudge": False, "fire": 30},
     say("Stand up. Walk heel to toe, along this board.", "Sangye"),
     {"npc_mode": TREK, "mode": "goto", "to": "trekker_line_b", "path": ["trekker_line_a"], "speed": 0.4},
     {"wait": 3},
     # no stumbling clip yet (design 9.2: a Mixamo 'drunk walk'): the caption says what the body cannot show
     {"caption": "He sways, steps off the board and grabs a chair.", "t": 3.5},
     say("You told me... go slower. I'm fine. I'm just... tired.", TREKKER_NAME, **{"if": "warned"}),
     say("I'm fine. Just tired. Is it... is it night?", TREKKER_NAME, ifnot="warned"),
     {"npc_mode": TREK, "mode": "sit"},
     say("His brain is swelling. If he sleeps here tonight, maybe he does not wake.", "Nima"))
# optional: radio Base Camp while the decision is still open
# (T, not the design's R: R is "Rest at camp" in YggSettings)
beat("stranger_radio", {"after": "stranger", "noflag": "decided", "press": "T", "prompt": "[T]  Call Base Camp", "nudge": False},
     RADIO,
     hale("Dexamethasone, eight milligrams, if you have it. Oxygen if the lodge has a bottle."),
     hale("And get him down. Down is the medicine."),
     {"flag": "called_hale"})
beat("stranger_decide", {"after": "stranger_test"},
     {"choice": ["Give him my dexamethasone.", "I'll walk him down tonight.", "Leave him with Diki."],
      "flags": ["gavedex", "walkdown", "leftdiki"], "timeout": 20, "default": 2, "if": "firstaid"},
     {"caption": "You did not pack the first-aid kit.", "t": 3, "wait": False, "ifnot": "firstaid"},
     {"choice": ["I'll walk him down tonight.", "Leave him with Diki."], "flags": ["walkdown", "leftdiki"], "timeout": 20,
      "default": 1, "ifnot": "firstaid"},
     {"flag": "decided"},
     {"npc_face": "jiwoo", "to": TREK},
     say("Not this.", "Nima", t=1.6),
     {"caption": "Ji-woo lowers his camera.", "t": 2.5})

# B19a walk_down: at night, in the snow, his arm in your hand
beat("walk_down", {"after": "stranger_decide", "flag": "walkdown"},
     {"blink": "blink", "t": 0.15, "hold": 0.3, "during": [
         person("thukla_porter", "handover_porter", [0.2, 0.16, 0.1], mode="idle")]},
     {"flag": ["marek_trust_down", "nima_trust_up"]},
     say("You are paid to look after this team.", "Marek"),
     say("I stay with Elena. Go. Keep his arm.", "Nima", **{"if": "elena_sick"}),
     {"npc_mode": "nima", "mode": "follow", "behind": 300, "side": 150, "ifnot": "elena_sick"},
     {"timeto": 2030, "t": 60},
     {"light": "headlamp", "on": True},
     {"sound": snd("Weather/Wind_Gusty"), "vol": 0.45, "fadein": 6, "id": "wind3"},
     {"npc_mode": TREK, "mode": "idle"},
     {"hand": TREK, "to": "ev_handover", "hint": "Hold  [F]  to hold his arm", "pull": 8, "pace": 0.8, "max": 640},
     {"npc_mode": "thukla_porter", "mode": "present", "dist": 160},
     say("I take him. Go, dai, you are cold.", "Porter"),
     {"npc_mode": "thukla_porter", "mode": "goto", "to": "s4", "speed": 0.6},
     {"npc_mode": TREK, "mode": "goto", "to": "s4", "speed": 0.6},
     {"flag": ["tired", "trekker_lives"]},
     {"cmd": ["ygg.Survival.Set Stamina 30", "ygg.Survival.Set BodyTemperature 35.1"]},
     {"wait": 4})
for i, line in enumerate(["Is this the way to the airport?", "My mum's waiting. She worries.", "Why is it so dark?"]):
    beat(f"walk_line{i}", {"after": "stranger_decide", "flag": "walkdown", "near": f"walkdown_w{i}", "r": 1500},
         say(line, TREKKER_NAME))

# B19b dex / B19c diki_o2
beat("dex", {"after": "stranger_decide", "flag": "gavedex"},
     {"npc_mode": TREK, "mode": "sit"},
     {"holdkey": "Interact", "t": 1.5, "prompt": "Hold  [E]  to give him the tablets"},
     {"flag": ["firstaid_used", "trekker_lives", "heli_dawn"]},
     {"npc_mode": "diki", "mode": "goto", "to": "trekker_corner", "speed": 0.8},
     {"timeto": 2130, "t": 30},
     {"caption": "An hour later he can hold his cup.", "t": 4},
     say("In the morning, the helicopter.", "Diki"))
beat("diki_o2", {"after": "stranger_decide", "flag": "leftdiki"},
     {"npc_mode": "diki", "mode": "goto", "to": "trekker_corner", "speed": 0.8, "arrive": True, "max": 20},
     {"show": "diki_o2"},
     say("I have done this before. In the morning, the helicopter. If it can fly.", "Diki"),
     {"flag": "heli_dawn"},                       # trekker_lives stays open: 'roll' does not exist yet (RUNTIME_TODO)
     {"timeto": 2130, "t": 30})

# ------------------------------------------------------------------ B20 bench: staff sleep on the benches
beat("bench", {"any": [{"after": "walk_down"}, {"after": "dex"}, {"after": "diki_o2"}]},
     {"blink": "blink", "t": 0.15, "hold": 1.2, "during": [
         {"time": 2340, "if": "walkdown"}, {"time": 2300, "ifnot": "walkdown"},
         {"teleport": "bench_sangye", "up": 112},
         {"pose": "sit", "snap": True, "face": "dining_stove", "prop": "sleeping_bag"},
         {"light": "headlamp", "on": False}, {"stopsound": "wind3", "fadeout": 1}, {"stopsound": "room", "fadeout": 1},
         {"npc_mode": "nima", "at": "bench_nima", "mode": "sit"},
         {"npc_mode": "porter1", "at": "bench_porter1", "mode": "sit"},
         {"npc_mode": "porter2", "at": "bench_porter2", "mode": "sit"},
         {"npc_mode": "jiwoo", "at": "rooms_door", "mode": "idle"},
         {"npc_remove": "marek"}, {"npc_remove": "elena"}, {"npc_remove": "diki"}, {"npc_remove": TREK},
         {"npc_remove": "thukla_porter"}, {"npc_remove": "trek_bg1"}, {"npc_remove": "trek_bg2"}, {"npc_remove": "lodge_porter"},
         {"show": "bench_bottle"}, {"hide": "diki_o2"}, {"hide": "sur_fire"}]},
     {"hold": "keep"},
     {"stopsound": "stove", "fadeout": 6},
     {"sound": snd("Story/Story_Stove_Loop"), "at": "dining_stove", "vol": 0.22, "fadein": 3, "id": "stove2"},
     {"wait": 3},
     say("Your bottle. Inside the bag, or it is a stone by morning.", "Nima"))
beat("bottle_in", {"after": "bench", "noflag": "bottle_out", "look": "bench_bottle", "cone": 30, "seen": True,
                   "press": "Interact", "prompt": "[E]  Bottle into the bag", "nudge": False},
     {"hide": "bench_bottle"}, {"flag": "bottle_in"},
     {"caption": "Into the bag, against your belly. Cold.", "t": 3})
beat("bottle_out", {"after": "bench", "delay": 20, "noflag": "bottle_in"},
     {"flag": "bottle_out"})

# ------------------------------------------------------------------ B21 video: the phone
beat("video", {"after": "bench", "delay": 25},
     {"npc_mode": "jiwoo", "mode": "goto", "to": "bench_jiwoo_end", "speed": 0.6, "arrive": True, "max": 15},
     {"npc_mode": "jiwoo", "mode": "sit"}, {"npc_face": "jiwoo", "to": "player"},
     say("Sangye... I have to tell you something.", "Ji-woo"),
     say("Before we came, I watched every video of the route. There is one from last May. At the Balcony.", "Ji-woo"),
     {"wait": 1.5},
     say("Today that man said the name. People wrote it under the video." if SAY_FATHER_NAME else
         "Today I read the name on the stone. People wrote it under the video.", "Ji-woo"),
     {"wait": 1.0},
     say("I'm sorry. I didn't know he was your father.", "Ji-woo"),
     {"choice": ["Show me.", "Delete it.", "Say nothing."], "flags": ["video_seen", "video_deleted", "video_unseen"],
      "timeout": 16, "default": 2},
     say("Show me.", "Sangye", **{"if": "video_seen"}, t=1.8),
     ear("phone_clip", vol=0.8, **{"if": "video_seen"}),
     dict({"inspect": "phone_video", "if": "video_seen",
           "text": "Headlamp light. A man in a red down suit sits by the fixed rope, facing up the ridge. His mask is off."},
          **({"front": PHONE_STILL} if PHONE_STILL else {})),
     say("Don't look. Keep moving.", "(a man's voice, on the video)", **{"if": "video_seen"}),
     say("I'm sorry.", "Ji-woo", **{"if": "video_seen"}),
     {"npc_mode": "nima", "mode": "goto", "to": "lodge_door", "speed": 0.8, "if": "video_seen"},
     {"wait": 4, "if": "video_seen"},
     ear("door_wind", at="lodge_door", vol=0.8, **{"if": "video_seen"}),
     {"reflex": "lodge_door", "deg": 10, "if": "video_seen"},
     {"attention": -3, "t": 4, "if": "video_seen"},
     say("Delete it.", "Sangye", **{"if": "video_deleted"}, t=1.6),
     say("Done. It's gone.", "Ji-woo", **{"if": "video_deleted"}),
     say("Good.", "Nima", **{"if": "video_deleted"}, t=1.4),
     say("I'll keep it. If you ever want...", "Ji-woo", **{"if": "video_unseen"}),
     {"npc_mode": "jiwoo", "mode": "goto", "to": "rooms_door", "speed": 0.6, "ifnot": "video_seen"},
     {"wait": 3})

# ------------------------------------------------------------------ B22 end2: sleep and the page
beat("end2", {"after": "video"},
     {"hold": False},
     {"pose": "lie", "t": 2.0},
     ear("roof_snow", vol=0.5, id="roof"),
     {"wait": 4},
     {"blink": "sleep", "t": 2.5}, {"stopsound": "stove2", "fadeout": 3},
     {"fade": 1, "t": 0.5},
     {"wait": 1.0},
     {"day": 2},
     {"page": [
         "Nima burned juniper at his stone at dawn.",
         {"if": "ask_nima", "text": "I asked him what he meant. 'Not in front of him,' he said."},
         {"if": "learn_mani", "text": "He taught me the prayer. He says my father said it too fast."},
         {"if": "nima_memory", "text": "My father sang when he climbed. Badly. Nima says he was the best of them."},
         {"if": "elena_helped", "text": "I walked with Elena. I taught her the step."},
         {"if": "risky2", "text": "I stood on the wrong side of the porters again."},
         "A man who knew him spoke to Nima in Sherpa. Nima would not translate.",
         {"if": ["hike", "gap_seen"], "text": "From the moraine I saw Nuptse burn in the sun. Not her. She hides."},
         {"if": "gavedex", "text": "I gave the boy my medicine."},
         {"if": "walkdown", "text": "I walked a stranger down in the dark. My legs are stone."},
         {"if": "leftdiki", "text": "I left the sick boy with Diki. I don't know if he will live."},
         {"if": "video_seen", "text": "In Ji-woo's video he is sitting. Facing up. People who fall do not sit down."},
         {"if": "video_deleted", "text": "Ji-woo had a video of him. I made him delete it. I see it anyway."},
         {"if": "video_unseen", "text": "Ji-woo has a video of him. I did not look."}], "head": "Day {day} / 49"},
     {"title": "CHOMOLUNGMA", "sub": "Chapter 2  ·  The Hidden Mountain", "t": 7},
     {"wait": 1.0},
     {"timehold": False},
     {"npc_remove": "nima"}, {"npc_remove": "jiwoo"}, {"npc_remove": "porter1"}, {"npc_remove": "porter2"},
     {"blink": "closed", "t": 0}, {"pose": "stand", "snap": True},
     {"end": True})


# ================================================================== validate (Chapter1_Camera_Spec 7.4 + chapter 2 checks)
# action keys in the order an action is recognised (the C++ dispatch reads "option keys first" the same way)
KINDS = ["pose", "hand", "inspect", "choice", "npc", "npc_mode", "flyout", "holdkey", "blink", "fade", "exposure", "caption", "title",
         "teleport", "say", "hint", "fall", "knock", "nudge", "reflex", "lure", "fovbeat", "attention", "fov", "breath", "hands",
         "pace", "light", "compass", "wear", "preload", "time", "timehold", "timeto", "cmd", "page", "day", "sound", "stopsound",
         "param", "show", "hide", "flag", "unflag", "waitnear", "npc_remove", "npc_face", "lookhold", "hold", "end", "wait"]
REMOVED = {"kneel": "pose", "tumble": "fall / knock", "lock": "hold (+ lookhold in fall/flyout beats)", "card": "caption / inspect"}
WHEN_KEYS = {"start", "after", "delay", "near", "far", "r", "look", "cone", "dwell", "seen", "lure", "lure_at", "nudge", "companion",
             "fire", "press", "prompt", "posture", "any", "idle", "flag", "noflag", "never", "up"}
POSTURES = {"lie", "sit", "kneel1", "kneel2", "crawl", "stand"}
NPC_MODES = {"follow", "goto", "lead", "sit", "idle", "present"}
HANDS = {"compass", "photo", "bottle", "khata", "dolma", False}          # FirstPersonBodyComponent::DefaultHandsFor
BUILDER_TAGS = {"worldtree_flags", "nightfire", "tentlight", "nima_headlamp", "sleeping_bag", "khata_on_stone",      # chapter 1
                "sang_fire", "bottle_ch2", "dining_stove", "sur_fire", "diki_o2", "bench_bottle"}                   # chapter 2
CH1_FLAGS = {"promise", "bringhome", "silent", "tea", "water2", "firstaid", "mala", "elena_load", "elena_water", "elena_push",
             "money", "forhim", "quiet", "safe", "risky", "read_photo"}
# what Chapter 3 reads (design 6): each must be set somewhere in this chapter
CARRY = ["bottle_in", "bottle_out", "tired", "firstaid_used", "heli_dawn", "trekker_lives", "hike", "gap_seen", "elena_sick",
         "elena_helped", "nima_memory", "video_seen", "video_deleted", "video_unseen", "jiwoo_talk", "warned"]

def kind(a):
    for k in KINDS:
        if k in a: return k
    return None

def asset_file(path):
    if not isinstance(path, str) or not path.startswith("/Game/"): return None
    return f"{ROOT}/Content/{path[6:].split('.')[0]}.uasset"

def _list(v): return v if isinstance(v, list) else ([v] if isinstance(v, str) else [])

def validate(beats, places):
    err, warn = [], list(WARN)
    ids = [b["id"] for b in beats]
    if len(set(ids)) != len(ids): err.append("duplicate beat ids: " + ", ".join(sorted({i for i in ids if ids.count(i) > 1})))
    if sum(1 for b in beats if b["when"].get("start")) != 1: err.append("exactly one beat must have 'start'")
    cast = json.load(open(f"{ROOT}/Content/Story/Cast.ygs", encoding="utf-8"))
    people = {"player"}
    set_flags, read_flags = set(), set()
    def walk(acts):
        for a in acts:
            if "npc" in a: people.add(a["npc"])
            if "flag" in a and kind(a) == "flag": set_flags.update(_list(a["flag"]))
            if "choice" in a: set_flags.update(f for f in a.get("flags", []) if f)
            for k in ("if", "ifnot"): read_flags.update(_list(a.get(k)))
            if "page" in a:
                for ln in a["page"]:
                    if isinstance(ln, dict):
                        for k in ("if", "ifnot"): read_flags.update(_list(ln.get(k)))
            walk(a.get("during", []))
    def walk_when(w):
        for k in ("flag", "noflag"): read_flags.update(_list(w.get(k)))
        for sub in w.get("any", []): walk_when(sub)
    for b in beats: walk(b["do"]); walk_when(b["when"])
    def place_ok(name, where):
        if not isinstance(name, str): return
        if name not in places and name not in people: err.append(f"{where}: unknown place or person '{name}'")
    budget = {"nudge": 0, "fovbeat": 0, "teleport": 0, "reflex": 0}
    holds = []
    last = beats[-1]["id"]

    def check_when(w, where, top=True):
        for k in w:
            if k == "timeout": err.append(f"{where}: removed when-key 'timeout' (use lure_at <= 8 / fire <= 16)")
            elif k not in WHEN_KEYS: err.append(f"{where}: when-key '{k}' does not exist in the runtime")
        for aft in _list(w.get("after")):
            if aft not in ids: err.append(f"{where}: after unknown beat '{aft}'")
        for k in ("near", "far", "look", "companion"): place_ok(w.get(k), f"{where} {k}")
        if isinstance(w.get("lure"), dict):
            place_ok(w["lure"].get("at"), f"{where} lure")
            fp = asset_file(w["lure"].get("sound"))
            if fp is None or not os.path.exists(fp): err.append(f"{where}: lure sound not found '{w['lure'].get('sound')}'")
        if w.get("lure_at", 0) > 8: err.append(f"{where}: lure_at {w['lure_at']} > 8")
        if isinstance(w.get("fire"), (int, float)) and not isinstance(w.get("fire"), bool) and w["fire"] > 16 and not w.get("press"):
            err.append(f"{where}: fire {w['fire']} > 16 on a look trigger")
        if w.get("posture") and w["posture"] not in POSTURES: err.append(f"{where}: unknown posture '{w['posture']}'")
        if w.get("press") and not w.get("prompt"): warn.append(f"{where}: press without a prompt")
        if w.get("nudge") and top: budget["nudge"] += 1
        if top and (w.get("look") or w.get("press")) and "nudge" not in w:
            err.append(f"{where}: look/press trigger without an explicit 'nudge' (true, {{pitchmax}} or false)")
        if not top and (w.get("look") or w.get("press")): err.append(f"{where}: look/press inside 'any' is ignored by the runtime")
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
                cond = tuple(sorted(_list(a.get("if")))) + tuple("!" + f for f in _list(a.get("ifnot")))
                for r in REMOVED:
                    if r in a: err.append(f"{at}: removed key '{r}' (use {REMOVED[r]})")
                if k is None:
                    if "look" in a: err.append(f"beat {bid} {where}[{i}]: removed action 'look' (use a when-lure)")
                    else: err.append(f"beat {bid} {where}[{i}]: unknown action {json.dumps(a)[:80]}")
                    continue
                if "look" in a and k not in ("npc", "npc_mode", "flyout"):
                    err.append(f"{at}: removed action 'look' (use a when-lure; the story never pulls the head)")
                closed = state["lids"] or state["black"] or in_during
                if k == "blink":
                    mode = a["blink"]
                    if not isinstance(mode, str) or mode not in ("blink", "closed", "open", "wake", "sleep"):
                        err.append(f"{at}: blink must be blink|closed|open|wake|sleep"); continue
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
                        err.append(f"{at}: teleport '{a['teleport']}' with the eyes open")
                elif k == "npc":
                    place_ok(a.get("at"), at); place_ok(a.get("to"), at)
                    for p in a.get("path", []): place_ok(p, at)
                    if a.get("look") and a["look"] not in cast: err.append(f"{at}: look '{a['look']}' is not in Cast.ygs")
                    if a.get("mode") and a["mode"] not in NPC_MODES: err.append(f"{at}: unknown npc mode '{a['mode']}'")
                    if not closed: err.append(f"{at}: npc '{a['npc']}' spawned with the eyes open (spawns only in black or behind closed lids)")
                elif k == "npc_mode":
                    if a["npc_mode"] not in people: err.append(f"{at}: npc_mode for '{a['npc_mode']}', who is never spawned")
                    place_ok(a.get("to"), at); place_ok(a.get("at"), at)
                    for p in a.get("path", []): place_ok(p, at)
                    if a.get("mode") and a["mode"] not in NPC_MODES: err.append(f"{at}: unknown npc mode '{a['mode']}'")
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
                    if a.get("side") not in ("left", "right") and not (isinstance(a.get("side"), str) and a["side"] in places):
                        err.append(f"{at}: fall side must be left|right or a place")
                    if not cond: state["lids"] = True
                elif k == "knock":
                    if a["knock"] not in ("left", "right") and not (isinstance(a["knock"], str) and a["knock"] in places):
                        err.append(f"{at}: knock side must be left|right or a place")
                elif k == "flyout":
                    err.append(f"{at}: chapter 2 has no fly-out (design 6.1)")
                elif k == "nudge":
                    budget["nudge"] += 1; place_ok(a["nudge"], at)
                elif k == "reflex":
                    budget["reflex"] += 1; place_ok(a["reflex"], at)
                elif k == "lure":
                    place_ok(a["lure"], at)
                elif k == "fovbeat":
                    budget["fovbeat"] += 1
                elif k == "fov":
                    if abs(a["fov"]) > 3: err.append(f"{at}: fov {a['fov']} outside +-3")
                elif k == "npc_face":
                    place_ok(a["npc_face"], at); place_ok(a.get("to"), at)
                elif k == "hand":
                    place_ok(a["hand"], at); place_ok(a.get("to"), at)
                    if a["hand"] not in HANDS: warn.append(f"{at}: the rig has no hands pose for holding '{a['hand']}' (the hold works, the arms do not pose)")
                elif k == "hands":
                    if a["hands"] not in HANDS: err.append(f"{at}: hands item '{a['hands']}' unknown to the rig ({sorted(h for h in HANDS if h)})")
                elif k == "choice":
                    opts, fl = a["choice"], a.get("flags", [])
                    if len(fl) != len(opts): err.append(f"{at}: {len(opts)} options but {len(fl)} flags")
                    if not (0 <= a.get("default", -1) < len(opts)): err.append(f"{at}: default {a.get('default')} not an option index")
                    if not a.get("timeout"): err.append(f"{at}: a choice needs a timeout (every choice resolves, design: defaults)")
                    if len(opts) > 4: err.append(f"{at}: more than 4 options (keys 1-4)")
                elif k == "compass":
                    if isinstance(a["compass"], dict): place_ok(a["compass"].get("toward"), at)
                elif k in ("show", "hide"):
                    if a[k] not in BUILDER_TAGS: warn.append(f"{at}: tag '{a[k]}' is not one the builders make")
                elif k == "light":
                    if a["light"] != "headlamp" and a["light"] not in places: warn.append(f"{at}: light '{a['light']}' is no place and not the headlamp")
                elif k == "end":
                    if bid != last: err.append(f"{at}: end only in the last beat")
                elif k == "cmd":
                    for c in _list(a["cmd"]):
                        if c.startswith("ygg.Survival.Set ") and c.split()[1] not in ("Health", "Stamina", "Satiety", "Hydration", "BodyTemperature", "Oxygen"):
                            err.append(f"{at}: unknown survival stat in '{c}'")
                if k in ("sound", "say") and a.get("at"): place_ok(a["at"], at)
                if k == "fade" and not cond:
                    state["black"] = a["fade"] >= 0.99 if a["fade"] >= 0.99 or a["fade"] <= 0.01 else state["black"]
                for f in ("sound", "front", "back", "mesh"):
                    path = a.get(f)
                    if f == "sound" and k not in ("sound", "lure"): continue
                    fp = asset_file(path)
                    if path is not None and (fp is None or not os.path.exists(fp)): err.append(f"{at}: asset not found '{path}'")
        act(b["do"], "do")
        for x in [x for x in holds if x[0] == bid and x[2] is True]:
            err.append(f"beat {bid}: hold{' if ' + ','.join(x[1]) if x[1] else ''} is not released in its beat")
            holds.remove(x)
    for bid, cond, h in holds:
        err.append(f"beat {bid}: hold 'keep'{' if ' + ','.join(cond) if cond else ''} is never released")
    # flags: every flag read is set in this chapter or comes from chapter 1; Chapter 3's flags are all set here
    for f in sorted(read_flags - set_flags - CH1_FLAGS): err.append(f"flag '{f}' is read but never set (typo?)")
    for f in sorted(read_flags & CH1_FLAGS): warn.append(f"flag '{f}' comes from chapter 1: absent until chapter chaining exists (RUNTIME_TODO)")
    for f in CARRY:
        if f not in set_flags: err.append(f"flag '{f}' (read by chapter 3) is never set")
    for f in ("risky", "safe", "quiet"):
        if f in set_flags: err.append(f"flag '{f}' is a chapter 1 flag: chapter 2 must not overwrite it (use {f}2)")
    if budget["nudge"] > 3: err.append(f"{budget['nudge']} nudges (3 per chapter)")
    if budget["fovbeat"] > 0: err.append(f"{budget['fovbeat']} fovbeats (chapter 2 has none, design 6.1)")
    if budget["teleport"] > 3: warn.append(f"{budget['teleport']} teleports (the design plans 3 cuts)")
    if budget["reflex"] > 4: warn.append(f"{budget['reflex']} reflexes (the design plans 3)")
    return err, warn, budget


err, warn, budget = validate(beats, places)
for w_ in warn: print("  warning:", w_)
for t in RUNTIME_TODO: print("  runtime work:", t)
if err:
    for e in err: print("  ERROR:", e)
    print(f"validate: {len(err)} error(s) - Chapter2.ygs NOT written")
    sys.exit(1)
print("validate: OK", f"({budget['nudge']} nudges, {budget['reflex']} reflexes, {budget['teleport']} teleports, {len(warn)} warnings)")
story = {"chapter": 2, "title": "The Hidden Mountain", "places_file": "Chapter2_places.ygs", "places": places, "beats": beats}
open(f"{ROOT}/Content/Story/Chapter2.ygs", "w", encoding="utf-8").write(json.dumps(story, indent=1, ensure_ascii=False))
print("beats", len(beats), "places", len(places))
