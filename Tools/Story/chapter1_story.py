# Offline (python -I): writes Content/Story/Chapter1.ygs, the data the story machine (Source/.../Story/YggStory) plays.
# Chomolungma, Chapter 1 "Day One" (Docs/Development/Chomolungma_Chapter1.md). Places come from
# Content/Story/Chapter1_places.ygs (Tools/Story/build_chapter1_world.py, traced onto the real ground).
# Spoken lines are subtitles ("say"); the author records the voices later (grep the log for "Story [VO]").
import json
ROOT = "F:/UE_project/YggdrasilsPeak"
places = json.load(open(f"{ROOT}/Content/Story/Chapter1_places.ygs"))
A = "/Game/Audio"
def snd(name): return f"{A}/{name}.{name.split('/')[-1]}"

beats = []
def beat(id_, when, *do): beats.append({"id": id_, "when": when, "do": list(do)})

# ------------------------------------------------------------------ PROLOGUE: the Balcony, 8,400 m, night
beat("prologue", {"start": True},
     {"fade": 1, "t": 0}, {"lock": True, "look": True},
     {"cmd": ["ygg.Realm.Force none", "ygg.Survival.Reset", "ygg.Survival.Bottle 900"]},
     {"time": 230}, {"timehold": True, "rate": 0}, {"hide": "worldtree_flags"},
     {"teleport": "balcony_start", "pitch": 8},
     {"npc": "father", "at": "father_sit", "tint": [0.32, 0.05, 0.04], "mode": "sit", "sink": 50, "face": False},
     {"wait": 3.5},
     {"sound": snd("Weather/Wind_Blizzard"), "vol": 0.75, "fadein": 3, "id": "wind"},
     {"sound": snd("Story/Story_Heartbeat"), "vol": 0.55, "id": "heart"},
     {"fade": 0, "t": 3.5},
     {"lock": False},
     {"hint": "W  climb      Follow the rope.", "t": 6})
beat("father_seen", {"after": "prologue", "look": "father", "timeout": 30, "cone": 12},
     {"sound": snd("Mind/Whisper_02"), "at": "father", "vol": 0.9},
     {"fov": 4, "t": 3},
     {"wait": 1.5},
     {"say": "You came too early.", "who": "Father", "t": 4})
beat("slip", {"after": "father_seen", "delay": 2.5},
     {"lock": True, "look": True},
     {"sound": snd("Ice/Ice_Crack_03"), "vol": 1.0},
     {"tumble": 1.7},
     {"sound": snd("Ice/Avalanche_01"), "vol": 0.6},
     {"fade": 1, "t": 0.6},
     {"stopsound": "heart", "fadeout": 0.5},
     {"wait": 1.2},
     {"flyout": 11, "rise": 26000, "back": 52000, "face": "summit", "wait": False},
     {"fade": 0, "t": 3.5},
     {"wait": 2.0},
     {"title": "CHOMOLUNGMA", "sub": "Goddess Mother of the World", "t": 7.5},
     {"fade": 1, "t": 2.5},
     {"stopsound": "wind", "fadeout": 2.5},
     {"wait": 1.0},
     {"npc_remove": "father"})

# ------------------------------------------------------------------ DAWN in the family room, Dingboche
beat("dawn", {"after": "slip"},
     {"cmd": ["ygg.Survival.Reset"]}, {"show": "worldtree_flags"},
     {"time": 620}, {"timehold": True, "rate": 0.2},
     {"teleport": "room_start", "pitch": -6},
     {"wait": 3.0},
     {"sound": snd("Weather/Wind_Calm"), "vol": 0.35, "fadein": 5, "id": "wind"},
     {"sound": snd("Story/Story_Stove_Loop"), "at": "room_photos", "vol": 0.45, "fadein": 3, "id": "stove"},
     {"title": "Day 1 / 49", "sub": "Dingboche, 4,410 m", "t": 5, "wait": False},
     {"fade": 0, "t": 4},
     {"lock": False},
     {"wait": 4},
     {"hint": "Look around.   [F]  look closer when something catches your eye", "t": 7})
beat("photos", {"after": "dawn", "delay": 9, "look": "room_photos", "timeout": 600, "cone": 14},
     {"wait": 1.0},
     {"card": "Father and Nima at Base Camp, 2019.|The mountain behind them, the whole Khumbu below.", "title": "The photo wall", "image": "/Game/Story/Photos/T_Photo_BaseCamp.T_Photo_BaseCamp"})
beat("duffel", {"after": "dawn", "delay": 6, "near": "room_duffel", "r": 170},
     {"card": "His red down jacket, patched at the elbow.|His prayer beads. His summit certificate, 2019.|Under them, a photograph.", "title": "Father's duffel"},
     {"card": "Next time with Sangye.|Not before he is ready.", "title": "On the back of the photograph", "image": "/Game/Story/Photos/T_Photo_Icefall.T_Photo_Icefall"},
     {"flag": "read_photo"})
beat("dolma_calls", {"after": "dawn", "delay": 25, "flag": "read_photo"},
     {"npc": "dolma", "at": "dolma_spot", "tint": [0.55, 0.12, 0.08], "scale": 0.62},
     {"npc": "aunt", "at": "aunt_spot", "tint": [0.3, 0.06, 0.12]},
     {"say": "Sangye-dai! Sangye-dai, they are waiting for you!", "who": "Dolma", "t": 4, "wait": False},
     {"look": "room_door", "strength": 0.45, "t": 1.6})
beat("dolma_calls_late", {"after": "dawn", "delay": 150, "noflag": "read_photo"},
     {"flag": "read_photo"})

# ------------------------------------------------------------------ the farewell: Dolma's hand, the aunt's khata
beat("farewell", {"after": "dolma_calls", "near": "dolma", "r": 420},
     {"stopsound": "stove", "fadeout": 3},
     {"say": "Hold tight, Sangye-dai.", "who": "Dolma", "t": 3},
     {"hand": "dolma", "to": "nima_spot", "hint": "Hold  [F]  to hold her hand"},
     {"npc_mode": "aunt", "mode": "goto", "to": "nima_spot"},
     {"wait": 1.5},
     {"look": "aunt", "strength": 0.7, "t": 1.4},
     {"say": "Promise me you will come down.", "who": "Aunt Pema", "t": 3.5},
     {"choice": ["I promise.", "I'll bring him home.", "Say nothing."], "flags": ["promise", "bringhome", "silent"], "timeout": 16, "default": 2, "echo": True},
     {"say": "Then I will keep the tea warm.", "who": "Aunt Pema", "if": "promise"},
     {"say": "Bring yourself home first.", "who": "Aunt Pema", "if": "bringhome"},
     {"say": "She ties the white khata round your neck. Her hands are cold.", "who": "", "if": "silent"},
     {"say": "She ties the white khata round your neck.", "who": "", "ifnot": "silent"},
     {"card": "A folded drawing, pushed into your pocket:|the mountain, a big figure, and a small one holding its hand.", "title": "Dolma's drawing"},
     {"say": "Dolma! Come, let him go.", "who": "Aunt Pema", "t": 3},
     {"npc_mode": "dolma", "mode": "goto", "to": "room_out"},
     {"npc_mode": "aunt", "mode": "goto", "to": "room_out"})

# ------------------------------------------------------------------ the job: Nima, the team, the load, the compass
beat("team", {"after": "dawn", "delay": 2},
     {"npc": "nima", "at": "nima_spot", "tint": [0.12, 0.14, 0.17]},
     {"npc": "marek", "at": "marek_spot", "tint": [0.2, 0.22, 0.08]},
     {"npc": "elena", "at": "elena_spot", "tint": [0.55, 0.14, 0.05]},
     {"npc": "jiwoo", "at": "jiwoo_spot", "tint": [0.05, 0.18, 0.3]})
beat("job", {"after": "farewell", "delay": 2},
     {"npc_mode": "nima", "mode": "idle", "face": True},
     {"look": "nima", "strength": 0.6, "t": 1.4},
     {"say": "Your load. Fifteen kilos.", "who": "Nima"},
     {"say": "Carry what you can carry tomorrow too.", "who": "Nima"},
     {"choice": ["Father's thermos of butter tea", "A second water bottle", "The first-aid kit", "Father's prayer beads"],
      "flags": ["tea", "water2", "firstaid", "mala"], "prompt": "Add one thing to the load", "timeout": 25, "default": 1},
     {"say": "Lobuche tomorrow, Base Camp the day after. Nobody plays hero.", "who": "Marek"},
     {"say": "Is it always this cold, or is it just me?", "who": "Elena"},
     {"sound": snd("Story/Story_Radio_Squelch"), "vol": 0.8},
     {"say": "Morning, team. Clip your oximeters on... Sangye, eighty-eight. Good. Elena, eighty-two. Drink more.", "who": "Dr. Hale (radio)"},
     {"say": "Here. He never went anywhere without it.", "who": "Nima"},
     {"card": "Father's brass compass.|The needle still points north.", "title": "From Nima's hands"},
     {"hint": "[Q]  hold up the compass", "t": 5},
     {"npc_mode": "nima", "mode": "follow", "behind": 260, "side": 150},
     {"npc_mode": "elena", "mode": "follow", "behind": 520, "side": -160},
     {"npc_mode": "jiwoo", "mode": "follow", "behind": 760, "side": 60},
     {"npc_mode": "marek", "mode": "goto", "to": "ev_porters", "speed": 0.9},
     {"say": "We walk. Slowly.", "who": "Nima"},
     {"hint": "Follow the trail up the valley.", "t": 6})

# ------------------------------------------------------------------ the trail
beat("climb", {"after": "job", "near": "ev_climb", "r": 1600},
     {"say": "Slow is fast up here. Step, breathe, step.", "who": "Nima"},
     {"hint": "When your breath is loud, slow down: it comes back faster than it goes.", "t": 6})
beat("stream", {"after": "job", "near": "stream_hole", "r": 1500},
     {"sound": snd("Story/Story_Stream_Loop"), "at": "stream_hole", "vol": 0.9, "fadein": 2, "id": "stream"},
     {"say": "Drink before you are thirsty.", "who": "Nima"},
     {"hint": "The stream runs under the ice. Walk to the hole to fill your bottle.", "t": 6})
beat("fill", {"after": "stream", "near": "stream_hole", "r": 260},
     {"lock": True}, {"kneel": 60, "t": 0.9},
     {"cmd": "ygg.Survival.Set Hydration 100"},
     {"hint": "The water is so cold it hurts your teeth. Bottle full.", "t": 4},
     {"wait": 2.2}, {"kneel": 0, "t": 0.9}, {"lock": False},
     {"stopsound": "stream", "fadeout": 6})
beat("elena", {"after": "job", "near": "ev_elena", "r": 1300},
     {"npc_mode": "elena", "mode": "goto", "to": "ev_elena", "speed": 0.6},
     {"wait": 2},
     {"look": "elena", "strength": 0.6, "t": 1.5},
     {"say": "Sorry... my head. It's like a drum. Just give me a minute.", "who": "Elena"},
     {"choice": ["Carry part of her load", "Give her your water", "Tell her to keep going"],
      "flags": ["elena_load", "elena_water", "elena_push"], "timeout": 18, "default": 0},
     {"cmd": "ygg.Survival.Set Stamina 35", "if": "elena_load"},
     {"say": "Good. That is how it is done.", "who": "Nima", "if": "elena_load"},
     {"cmd": "ygg.Survival.Set Hydration 55", "if": "elena_water"},
     {"say": "Thank you. Really.", "who": "Elena", "if": "elena_water"},
     {"say": "Okay... okay. I'm fine.", "who": "Elena", "if": "elena_push"},
     {"npc_mode": "elena", "mode": "follow", "behind": 520, "side": -160, "speed": 0.85})
beat("why", {"after": "elena", "delay": 25},
     {"say": "Why did you take this job?", "who": "Nima"},
     {"choice": ["For the money.", "For him.", "Say nothing."], "flags": ["money", "forhim", "quiet"], "timeout": 14, "default": 2, "echo": True},
     {"say": "Money is a good reason. An honest one.", "who": "Nima", "if": "money"},
     {"say": "...He would not want you up there for him.", "who": "Nima", "if": "forhim"},
     {"wait": 3, "if": "quiet"},
     {"say": "Your father was quiet too.", "who": "Nima", "if": "quiet"})
beat("porters_come", {"after": "job", "near": "ev_porters", "r": 2200},
     {"npc": "porter1", "at": "porter_start", "tint": [0.22, 0.15, 0.08], "mode": "goto", "to": "porter_end", "speed": 1.0},
     {"wait": 1.2},
     {"npc": "porter2", "at": "porter_start", "tint": [0.15, 0.12, 0.1], "mode": "goto", "to": "porter_end", "speed": 1.0},
     {"say": "Porters coming down. Big loads.", "who": "Nima", "t": 3})
beat("porters_pass", {"after": "porters_come", "near": "porter1", "r": 700},
     {"choice": ["Step to the mountain side and wait", "Squeeze past on the outside"], "flags": ["safe", "risky"], "timeout": 7, "default": 0},
     {"cmd": "ygg.Survival.Set Health 78", "if": "risky"},
     {"sound": snd("Footsteps/Step_Rubble_04"), "vol": 1.0, "if": "risky"},
     {"tumble": 0.35, "if": "risky"}, {"teleport": "porter_end", "if": "risky", "up": 60},
     {"say": "Always the mountain side. Always. A load like that can push you off the trail.", "who": "Nima", "if": "risky"},
     {"say": "Good. Let them pass.", "who": "Nima", "if": "safe"})
beat("porters_gone", {"after": "porters_pass", "delay": 30},
     {"npc_remove": "porter1"}, {"npc_remove": "porter2"})
beat("alone", {"after": "job", "near": "ev_cloud", "r": 1500},
     {"npc_mode": "nima", "mode": "goto", "to": "ev_cloud"},
     {"npc_mode": "elena", "mode": "goto", "to": "ev_cloud"},
     {"npc_mode": "jiwoo", "mode": "goto", "to": "ev_cloud"},
     {"say": "Go on. I stay with Elena. Follow your father's compass, we will catch up.", "who": "Nima"},
     {"sound": snd("Weather/Wind_Gusty"), "vol": 0.6, "fadein": 6, "id": "wind2"},
     {"stopsound": "wind", "fadeout": 6},
     {"hint": "[Q]  hold up the compass", "t": 5})
beat("together", {"after": "alone", "near": "ev_alone_end", "r": 1600},
     {"npc_mode": "nima", "mode": "follow", "behind": 260, "side": 150},
     {"npc_mode": "elena", "mode": "follow", "behind": 520, "side": -160},
     {"npc_mode": "jiwoo", "mode": "follow", "behind": 760, "side": 60},
     {"wait": 8},
     {"say": "There you are. Good. The pass is above us.", "who": "Nima"})
beat("dusk_comes", {"after": "job", "near": "ev_flags_seen", "r": 2600},
     {"timeto": 1735, "t": 35},
     {"sound": snd("Story/Story_Flags_Loop"), "at": "memorial", "vol": 1.0, "fadein": 6, "id": "flags"},
     {"look": "memorial", "strength": 0.45, "t": 2.5})

# ------------------------------------------------------------------ the peak: the memorials at dusk
beat("memorial", {"after": "job", "near": "memorial", "r": 1500},
     {"npc_mode": "nima", "mode": "goto", "to": "kneel_spot", "speed": 0.8},
     {"say": "This way.", "who": "Nima"},
     {"look": "father_stone", "strength": 0.5, "t": 2.0})
beat("stone", {"after": "memorial", "near": "father_stone", "r": 330},
     {"lock": True, "look": True},
     {"look": "father_stone", "strength": 1.0, "t": 1.4, "up": 40, "wait": True},
     {"kneel": 70, "t": 1.1},
     {"wait": 2.5},
     {"show": "khata_on_stone"},
     {"say": "You tie your khata to his name.", "who": ""},
     {"wait": 2.5},
     {"look": "plume_view", "strength": 0.85, "t": 3.2, "wait": True},
     {"param": "Glow", "tag": "plume", "to": 1.0, "t": 11, "pulse": True},
     {"sound": snd("Story/Story_Plume_Drone"), "vol": 0.85},
     {"sound": snd("Story/Story_Heartbeat"), "vol": 0.6, "id": "heart"},
     {"fov": 4, "t": 7},
     {"wait": 5.5},
     {"sound": snd("Ice/Mountain_Boom_02"), "vol": 0.7},
     {"wait": 5.5},
     {"stopsound": "heart", "fadeout": 2},
     {"look": "night_nima", "strength": 0.6, "t": 1.6},
     {"say": "Did you hear that?", "who": "Nima"},
     {"say": "No. Only the wind. Come, it is getting dark.", "who": "Nima"},
     {"kneel": 0, "t": 1.2},
     {"lock": False},
     {"npc_mode": "nima", "mode": "goto", "to": "night_nima", "speed": 0.9},
     {"hint": "Follow Nima down to the tents.", "t": 6})

# ------------------------------------------------------------------ night: the end
beat("night", {"after": "stone", "near": "night_tent", "r": 450},
     {"blink": 1.6},
     {"timeto": 2040, "t": 6},
     {"lock": True, "look": False},
     {"stopsound": "flags", "fadeout": 4}, {"stopsound": "wind2", "fadeout": 6},
     {"sound": snd("Story/Story_Stove_Loop"), "at": "night_stove", "vol": 0.55, "fadein": 3, "id": "stove"},
     {"npc_mode": "nima", "mode": "idle", "face": True},
     {"wait": 3},
     {"sound": snd("Story/Story_Radio_Squelch"), "vol": 0.8},
     {"say": "Evening, team. Numbers, please.", "who": "Dr. Hale (radio)"},
     {"say": "Elena is at seventy-six. Watch her tonight.", "who": "Dr. Hale (radio)", "if": "elena_push"},
     {"say": "Elena's at eighty-one. Good work today.", "who": "Dr. Hale (radio)", "ifnot": "elena_push"},
     {"sound": snd("Story/Story_Radio_Squelch"), "vol": 0.6},
     {"wait": 4},
     {"look": "nima", "strength": 0.8, "t": 1.6},
     {"wait": 2},
     {"say": "People say he fell.", "who": "Nima", "t": 3.5},
     {"wait": 2.5},
     {"say": "Your father did not fall.", "who": "Nima", "t": 4},
     {"wait": 1.0},
     {"fade": 1, "t": 3.5},
     {"stopsound": "stove", "fadeout": 3},
     {"wait": 1.5},
     {"day": 1},
     {"page": [
         "Dingboche. His duffel still smells of smoke.",
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
     {"npc_remove": "marek"}, {"npc_remove": "jiwoo"},
     {"fade": 0, "t": 3},
     {"end": True})

story = {"chapter": 1, "title": "Day One", "places": places, "beats": beats}
open(f"{ROOT}/Content/Story/Chapter1.ygs", "w", encoding="utf-8").write(json.dumps(story, indent=1, ensure_ascii=False))
print("beats", len(beats), "places", len(places))
