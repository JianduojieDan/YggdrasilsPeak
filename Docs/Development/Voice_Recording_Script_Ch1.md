# Chomolungma · Voice Recording Script · Chapter 1 "Day One" + the Summit beat

Version 1.0 · 2026-11-20 · written by Claude for the director, who records every voice in the game.

**Where the lines come from.** Every line below is copied word for word from the game's story files:
`Content/Story/Chapter1.ygs` (made by `Tools/Story/chapter1_story.py`) and `Content/Story/Summit.ygs`. If a line
changes in the story files, this script must change too. Lines marked **[Proposed]** are new ideas. They are not in the
game yet, and you decide if they go in.

**How to use this script**

1. Read section 1 (what to record first) and section 2 (the home recording guide).
2. Record one character per session, using the tables in section 5.
3. Save each chosen take with the exact file name in the table (section 2.7).
4. Claude then imports the files and connects them to the story (section 7).

---

## 1. Record these first

There are **41 core lines**, plus Elena's breathing and the summit line (two versions). That is only about **2 minutes**
of finished audio. With 3 takes, warm-ups and rests, plan for **3 to 4 sessions of about 45 minutes**.

**Priority A: the spine of the chapter (record first)**

| # | File | Who | Line | Why it matters |
|---|---|---|---|---|
| 1 | `VO_Ch1_Nima_21` | Nima | *"Your father did not fall."* | The last spoken line of the chapter, and its shock |
| 2 | `VO_Ch1_Nima_20` | Nima | *"People say he fell."* | Sets up line 1, in the same breath and mood |
| 3 | `VO_Ch1_Father_01` | Father | *"You came too early."* | The hook. The first voice the player hears |
| 4 | `VO_Summit_Father_01` / `VO_Summit_Sangye_01` | Father or Sangye | *"Nothing above you (me) now..."* | The end of the summit demo (thesis demo) |
| 5 | `VO_Ch1_Pema_01` | Aunt Pema | *"Promise me you will come down."* | The promise that the whole game is about |
| 6 | `VO_Ch1_Dolma_01`, `_02` | Dolma | *"Sangye-dai! ..."*, *"Hold tight, Sangye-dai."* | The bond. Also the first voice in daylight |
| 7 | `VO_Ch1_Nima_01`, `_02`, `_03`, `_05` | Nima | load, compass, *"Slow is fast up here..."* | Every player hears them. They teach the body systems |
| 8 | `VO_Ch1_Elena_Breath_01` | Elena | laboured breathing (no words) | Fills a sound slot that is silent today (her "lure") |

**Priority B:** all other Nima lines, Aunt Pema 02-04, Sangye 01-04, Elena 01-04.
**Priority C:** Marek 01, Dr. Hale 01-04 (radio lines forgive small flaws), then the optional lines in section 5.10.

**Tip:** to keep each voice steady, record all of one character's lines in one sitting. Do that character's Priority A
lines first in the session, while your voice is fresh.

---

## 2. Home recording guide

### 2.1 The room

- Use the smallest, softest room you have. A wardrobe full of clothes is ideal. A bedroom with a bed, curtains and a
  rug is good. Avoid kitchens, bathrooms and empty rooms: hard walls make echo, and echo cannot be removed later.
- The microphone hears what is **in front of it**, which is you **and the wall behind you**. Hang a thick duvet or
  blanket behind your head.
- Turn off fans, air conditioning and the fridge (if you can hear it). Close the window.
- The laptop fan is loud under load. Close the Unreal editor and the game, and put the laptop at least 1.5 m away,
  behind the microphone.
- Record 10 seconds of silence at the start of each session (the "room tone"). It helps clean noise later.

### 2.2 The microphone

- Any USB microphone is fine. If you have none, a phone voice-recorder app that saves WAV also works.
- Use a pop filter. A thin sock stretched over a bent wire hanger works too. Many lines start with hard "P" sounds
  (*Promise*, *People*, *Porters*, *Pema*).
- Point the microphone at the **corner of your mouth**, not straight at your lips. This softens "P" and "B" blasts.
- Wear closed headphones to listen. Never play sound through speakers while recording.

### 2.3 Distance: five positions used in the tables

| Code | Distance from the mic | Use it for |
|---|---|---|
| **In-ear** | 8-10 cm, soft voice | inner voices, whispers, the father, the night tent |
| **Close** | 15-20 cm (one hand span), normal voice | talking to someone at arm's length |
| **Across** | 40-60 cm, head turned a little away | talking to a group, or to someone a few metres away |
| **Far** | about 1 m, head turned 90°, voice projected | a call from outside the tent, or from behind |
| **Radio** | Close, a little louder and clipped | Dr. Hale. Speak as if into a handheld radio |

For a loud call, **step back and turn your head**. Do not shout straight into the mic. For a whisper, **move closer**.
Do not turn the gain up.

### 2.4 Levels and format

- **Format:** WAV, **48 kHz**, **mono**. Record at 24-bit. Export the final files as **48 kHz, 16-bit, mono WAV**,
  which Unreal always imports. Keep the 24-bit takes as your masters.
- **Level:** set the input gain so a normal line peaks at about **-12 dB** on the meter. Loud calls must stay below
  **-6 dB**. Never touch 0 dB: a clipped (distorted) take cannot be repaired.
- Set the gain with the loudest line of the session first. Then do not change it for that character.
- Silence between lines should read below about -60 dB. If it is higher, find the noise first.
- **No effects in the files:** no reverb, no EQ, no compression, no noise removal. Turn off "audio enhancements" or
  automatic gain in your recording app. The game adds space and the radio sound later, in one place, so all lines
  match.
- **Do not normalise.** Leave the levels as recorded. Claude matches the loudness across files later and keeps
  whispers quieter than calls on purpose.
- In Audacity (free): set the project rate to 48000 Hz and recording channels to 1 (mono). Export with
  File > Export Audio > WAV.

### 2.5 Takes and slates

- **3 takes for every line.** Take 1 as directed. Take 2 a little different (softer, or slower). Take 3 free: try
  what feels true.
- Before each line, say its number out loud ("Nima twenty-one"), then wait 2 seconds. Leave 2 seconds of silence
  after every take. This makes cutting easy.
- If you know a take is the one, say "keeper" after it.
- Branch lines: the player hears only one answer of each choice, but record **all** of them.

### 2.6 Your body and voice

- Stand up while you record. You have more breath and more energy.
- Do the movement: bend over for sick Elena, hold out a hand for Nima's compass, walk on the spot for walking lines.
- Drink room-temperature water. Avoid milk and coffee before a session. A slice of green apple reduces mouth clicks.
- 45-60 minutes per session at most. Child voices, women's voices and whispers tire the throat faster.
- Your morning voice is lower and rougher. It suits the father and Nima.

### 2.7 Where to save and how to name

| What | Where |
|---|---|
| Whole session recordings (long, raw) | `F:/UE_project/YggdrasilsPeak/Art/Audio/Voice/_sessions/` |
| Cut takes (all 3) | `F:/UE_project/YggdrasilsPeak/Art/Audio/Voice/Ch1/_takes/VO_Ch1_Nima_21_t1.wav` (`_t2`, `_t3`) |
| Final chosen files, Chapter 1 | `F:/UE_project/YggdrasilsPeak/Art/Audio/Voice/Ch1/VO_Ch1_Nima_21.wav` |
| Final chosen files, Summit | `F:/UE_project/YggdrasilsPeak/Art/Audio/Voice/Summit/VO_Summit_Father_01.wav` |

- Use the file names in the tables exactly, including capitals. The game finds sounds by name.
- A final file starts about 0.1 s before the first sound. Include the in-breath if it is part of the acting. It ends
  0.3-0.5 s after the last sound. Add a 10 ms fade at both ends so there is no click.
- **If cutting is too much work:** put the session file in `_sessions/` and write down which takes you liked. Claude
  cuts and names them, and then you check them by ear.
- These folders follow the project's pattern: `Art/Audio/<Folder>` holds the source WAVs for `/Game/Audio/<Folder>`.

### 2.8 How to perform altitude breathlessness

**What thin air really does to speech.** The body needs air every few words. Sentences break in odd places, not only
at commas. The voice gets thinner and quieter, because there is less air to push it. The ends of words fade into
breath. People say less.

**The breath levels used in the tables**

| Code | Feeling | How to get it |
|---|---|---|
| **B0** | calm, no effort you can hear | rest before the take |
| **B1** | walking at 4,400-4,800 m: one small breath before the line, a little air in the voice | walk on the spot for 20 seconds, then speak while still walking slowly |
| **B2** | working hard, or altitude sickness: breaths break the sentence; short, shallow breaths | 15-20 squats or one flight of stairs, then record at once |
| **B3** | extreme, 8,000 m and above: 2-3 words per breath, voice thin, close to tears | 30-40 seconds of fast exercise, then record at once |
| **B-** | the father only: **no breath at all**, even at 8,400 m | rest well. Speak on one calm out-breath. This unnatural calm is what makes him eerie |

- **Real effort works better than acting it.** Do the exercise, then record. Rest until you are calm before the next
  take.
- **Safety:** stop if you feel dizzy or tingly. Do not breathe fast on purpose while standing still (that is
  hyperventilation and it makes you dizzy). Moving is safer.
- Turn your head 10-20° away from the mic for big in-breaths, so the air does not hit it. Keep the breaths in the
  take: they are part of the acting.
- **The Sherpa rule:** Nima's body is used to altitude. At the same height he is always one level calmer than the
  clients (Elena at B2 means Nima at B1). Players will feel this difference.

### 2.9 Voicing other ages, women and accents

- **Dolma (8) and the women.** You voice everyone. Use a higher, lighter placement (speak "from the front of the
  mouth"), shorter breaths and simple stress. Never squeak or use falsetto. A small pitch shift afterwards (about
  +3 semitones, with formant correction) can help. Claude can test this on a take, and you judge by ear.
- If someone else records a character later, the file names stay the same. Their files simply replace yours.
- **Accents.** The characters come from Nepal, Slovakia, Spain and (Dr. Hale) an English-speaking country. Show this
  through **rhythm and attitude**, not a heavy imitated accent. A thick fake accent would sound like mockery,
  especially for the Sherpa characters, and respect is a core rule of this game (design rule P4). Nima's English is
  written simply ("I stay with Elena"). Say it as written. Do not correct the grammar, and do not exaggerate it.

---

## 3. The cast at a glance

| Character | Who they are | Voice | Pace | Breath in Ch1 |
|---|---|---|---|---|
| **Nima** | older Sherpa (about 50s), climbed with the father, hides how he died | low, warm, even; few words; sentences fall at the end; rare dry humour | slow, never hurried | B0-B1 |
| **The father** | about 48, died high on Everest last season; seen only in the prologue, at night, at 8,400 m | low, calm, grave, tender; not a ghost effect, not a threat | very slow | **B-** (none) |
| **Aunt Pema** | about 50, family; came up from the village to say goodbye | low-middle, steady; strength that comes from grief; practical | slow | B0 |
| **Dolma** | 8, Sangye's little cousin; loves him | bright, clear, quick; a little bossy | quick | B0 |
| **Sangye** (the player) | 22, a quiet young Sherpa on his first job | young, quiet, reserved; says very little | slow | B0-B1 |
| **Elena Ruiz** | Spanish client, first time at altitude; gets altitude sickness | warm, quick, jokes to hide nerves; later weak and pressed | quick, then broken | B0, then B2 |
| **Marek Kovač** | Slovak expedition leader, 40s | crisp, firm, dry; has seen "heroes" die | brisk | B0 |
| **Dr. Hale** | Base Camp doctor; radio only | brisk, friendly, professional | normal | B0 |
| **Ji-woo Han** | Korean client who films everything | **silent by design** ("says nothing and films everything") | | |
| **Two porters** | carry huge loads on the narrow trail | no lines today (optional sounds in 5.10) | | |

---

## 4. Pronunciation

Lines in the game are in English (design decision D9). Only names and a few words come from Sherpa, Tibetan or
Nepali. **[Native check]** means a Sherpa or Tibetan speaker should confirm it before release.

| Word | Say it like | Meaning, note |
|---|---|---|
| Sangye | **SAHNG**-gyay | Tibetan name. Sherpa speakers may say "SAHN-gay". **[Native check]** |
| -dai (Sangye-dai) | rhymes with "eye" | Nepali "older brother", a warm word for an older boy. A Sherpa girl might use a Sherpa-language word instead. **[Native check]** |
| Dolma | **DOHL**-mah | |
| Pema | **PEH**-mah | |
| Nima | **NEE**-mah | |
| Lobuche | **LOH**-boo-chay | village above Dingboche (Marek says it) |
| Marek Kovač | **MAH**-rek **KO**-vatch | |
| Elena Ruiz | eh-**LEH**-nah roo-**EETH** | Spanish "z" is like "th" in Spain |
| Ji-woo | **JEE**-woo | |
| oximeter | ok-**SIM**-i-ter | finger clip that measures blood oxygen |
| Chomolungma | **CHO**-mo-**LOONG**-mah | Tibetan name of Everest. Title only, not spoken today |
| Dingboche, Thukla, khata | **DING**-boh-chay, **TOOK**-lah, **KAH**-tah | on screen only, not spoken today |

---

## 5. The lines, per character

**How to read the tables.**
- **Beat** = the story step (its id in the story file) and what happens.
- **Delivery** = breath level · distance · emotion · pace.
- **Target** = the natural length of the finished clip. The number in brackets is how long the subtitle stays on
  screen **today**.
- **Record at the right pace. Do not rush a line to fit the subtitle.** After recording, each subtitle time is set
  from the real clip length (section 7). "(lengthen)" marks lines that will need more time than today.
- *if: ...* = the line plays only after that player choice.

### 5.1 Nima (21 lines)

| ID / file | Beat | Line | Delivery | Target |
|---|---|---|---|---|
| `VO_Ch1_Nima_01` | `job` · gives Sangye his pack at camp | *"Your load. Fifteen kilos."* | B0 · Close · practical and kind, no drama; a small pause after "Your load." as he sets it down · slow | 2.0-2.5 s (2.8) |
| `VO_Ch1_Nima_02` | `job` | *"Carry what you can carry tomorrow too."* | B0 · Close · plain advice from long experience. Not a "wise man" voice, not a proverb; a light stress on "tomorrow" · slow | 2.8-3.3 s (3.5) |
| `VO_Ch1_Nima_03` | `job` · gives him the father's brass compass | *"Here. He never went anywhere without it."* | B0 · Close, a little softer · feeling held back; "Here." then a breath-long pause; the rest lower · slow | 3.0-3.6 s (3.6) |
| `VO_Ch1_Nima_04` | `job` · the team sets off | *"We walk. Slowly."* | B0 · Across (to the whole team) · a calm decision; a full stop between the two parts · slow | 1.8-2.3 s (2.5) |
| `VO_Ch1_Nima_05` | `climb` · at Sangye's right shoulder, first steep climb; plays when the player's breath is loud | *"Slow is fast up here. Step, breathe, step."* | B1 · Close · a steady teacher. Walk slowly on the spot and time "Step... breathe... step" to your feet, with a real in-breath on "breathe" · slow, rhythmic | 3.5-4.5 s (3.7, lengthen) |
| `VO_Ch1_Nima_06` | `stream` · the stream is heard under the ice | *"Drink before you are thirsty."* | B1 · Close, walking · a casual reminder, half to himself · normal | 2.0-2.6 s (3.0) |
| `VO_Ch1_Nima_07` | `elena_talk` · *if: you carried part of Elena's load* | *"Good. That is how it is done."* | B0 · Close · quiet approval; you can hear a small smile; he is proud but says no more · slow | 2.3-2.9 s (3.0) |
| `VO_Ch1_Nima_08` | `why` · walking side by side | *"Why did you take this job?"* | B1 · Close · light on the surface, but he really wants to know; he does not look at you · slow | 2.0-2.6 s (2.8) |
| `VO_Ch1_Nima_09` | `why` · *if: "For the money."* | *"Money is a good reason. An honest one."* | B1 · Close · accepts it without judging; dry warmth on "An honest one." Maybe he does not quite believe you · slow | 3.0-3.6 s (3.5) |
| `VO_Ch1_Nima_10` | `why` · *if: "For him."* | *"...He would not want you up there for him."* | B1 · Close, lower · begin with a slow breath out (that is the "..."); heavy; he knows more than he says; nearly to himself · slow | 3.5-4.2 s (3.7, lengthen) |
| `VO_Ch1_Nima_11` | `why` · *if: you said nothing* (comes after 3 s of silence) | *"Your father was quiet too."* | B0 · Close · a tender memory; a soft breath-laugh before or after · slow | 2.0-2.6 s (2.8) |
| `VO_Ch1_Nima_12` | `porters_come` · porters appear up the trail | *"Porters coming down. Big loads."* | B1 · Across, looking up the trail · alert, practical warning; voice a little raised · normal | 2.0-2.5 s (3.0) |
| `VO_Ch1_Nima_13` | `porters_pass` · *if: you squeezed past on the outside and a load knocked you to one knee* | *"Always the mountain side. Always. A load like that can push you off the trail."* | B1 · Close (he has reached you) · fear turned into sharpness: the first "Always" hard, the second lower and slower; then a calmer explanation, like a father · normal, then slow | 4.8-6.0 s (5.7) |
| `VO_Ch1_Nima_14` | `porters_pass` · *if: you stepped to the mountain side* | *"Good. Let them pass."* | B0 · Close · quiet approval, almost under the porters' bells · slow | 1.6-2.2 s (2.5) |
| `VO_Ch1_Nima_15` | `alone` · the afternoon cloud comes in; he stays back with Elena | *"Go on. I stay with Elena. Follow your father's compass, we will catch up."* | B1 · Across, over rising wind (project a little, do not shout) · decisive and reassuring; a hidden worry on "your father's compass" · normal | 5.0-6.0 s (5.4, lengthen) |
| `VO_Ch1_Nima_16` | `together` · his voice comes from behind before you see him | *"There you are. Good. The pass is above us."* | B2 (he hurried to catch up) · Across, head turned away for "There you are.", then closer · relief under calm: "There you are." lifts, the rest settles · normal | 3.5-4.5 s (3.7, lengthen) |
| `VO_Ch1_Nima_17` | `memorial` · dusk at the Thukla memorials | *"This way."* | B1 · Close, hushed (a sacred place) · respectful, gentle · slow | 1.0-1.5 s (2.5) |
| `VO_Ch1_Nima_18` | `plume` · after a distant boom; the summit plume glowed gold, but only Sangye saw it | *"Did you hear that?"* | B0 · Close · alert, uneasy, low; he has turned his head to the ridge · normal | 1.3-1.8 s (2.5) |
| `VO_Ch1_Nima_19` | `plume` | *"No. Only the wind. Come, it is getting dark."* | B0 · Close · he talks himself out of it, or **chooses** not to believe. "No." after a beat, flat. "Only the wind." closes a door. "Come..." warm again, moving on · slow | 3.5-4.5 s (3.8, lengthen) |
| `VO_Ch1_Nima_20` | `night` · in the tent, 20:40, only his headlamp and the stove; 1 m apart | *"People say he fell."* | B0 · In-ear · very quiet, slow; he has decided to tell you something; looking down. Leave the end open (do not let the voice fall to a full stop) · very slow | 1.8-2.6 s (3.5) |
| `VO_Ch1_Nima_21` | `night` · 2.5 s of silence after line 20; then he switches his light off | *"Your father did not fall."* | B0 · In-ear · **the most important line of the chapter.** A breath before "Your". Very quiet and deliberate; every word has weight. No drama, no shaking voice: a year of silence is behind it · very slow | 2.2-3.2 s (4.0) |

**Extra takes for Nima 20 and 21:** record them together, as one moment, three times. Then record each one alone. The
pause between them stays in the game data (2.5 s), not in the files.

### 5.2 The father (1 line, plus the summit option in 5.9)

| ID / file | Beat | Line | Delivery | Target |
|---|---|---|---|---|
| `VO_Ch1_Father_01` | `father_seen` · prologue, the Balcony at 8,400 m, 02:30, blizzard. A man sits in the snow ahead with no oxygen mask; the headlamp finds him; he turns his head | *"You came too early."* | **B-** (no breath, even here) · In-ear: he sits 20-40 m away in a storm, but the voice is inside your head · calm, grave, tender, a little sad. Fate, not threat. A small pause before "too early" · very slow | 2.0-3.0 s (4.0) |

**Three different takes, please:** (1) half-voice: voiced but very soft; (2) a full whisper; (3) warmer, like a father
waking his child. The choice is made by ear, inside the scene (with the wind and heartbeat under it).

### 5.3 Aunt Pema (4 lines)

| ID / file | Beat | Line | Delivery | Target |
|---|---|---|---|---|
| `VO_Ch1_Pema_01` | `farewell` · dawn at the camp edge; she stands in front of you, about 90 cm | *"Promise me you will come down."* | B0 · Close · low, steady, almost an order. She said goodbye on this trail before, and that person did not come back. Tears held back, never shown. A small pause after "Promise me" · slow | 2.3-3.0 s (3.5) |
| `VO_Ch1_Pema_02` | `farewell` · *if: "I promise."* | *"Then I will keep the tea warm."* | B0 · Close · relief; warmth comes into the voice; a small smile; she half believes you · slow | 2.2-2.8 s (3.0) |
| `VO_Ch1_Pema_03` | `farewell` · *if: "I'll bring him home."* | *"Bring yourself home first."* | B0 · Close · firm, a little sharp, out of love; stress "yourself". She is afraid of losing two · slow | 1.8-2.4 s (2.8) |
| `VO_Ch1_Pema_04` | `farewell` · after the khata and Dolma's drawing; Dolma will not let go | *"Dolma! Come, let him go."* | B0 · "Dolma!" Across (called to the child, a little louder); "Come, let him go." Close and softer · gentle authority. These words echo the game's ending ("let him go"). Play them plain; do not underline it · normal | 2.0-2.7 s (3.0) |

### 5.4 Dolma (2 lines)

| ID / file | Beat | Line | Delivery | Target |
|---|---|---|---|---|
| `VO_Ch1_Dolma_01` | `dolma_calls` · Sangye sits in his tent; she calls from outside (the game places this voice at her spot) | *"Sangye-dai! Sangye-dai, they are waiting for you!"* | B0 · Far (step back 1 m and turn your head; let the voice carry, as through tent fabric) · bright, eager, a bit bossy, proud to bring the message; excited, not alarmed · quick | 2.5-3.3 s (4.0) |
| `VO_Ch1_Dolma_02` | `farewell` · she has run up and takes your hand | *"Hold tight, Sangye-dai."* | B0 · Close; she is small, so tilt your head down · half a game, half serious: she means "do not let go"; softer than the call · normal | 1.5-2.2 s (3.0) |

### 5.5 Sangye, the player (4 spoken lines)

These play after the player picks an answer on screen. They are the **player's own voice**: keep them understated, so
that any player can feel "that is me". They play in the centre with no space around them (no "at" place).
Record each line in three colours: neutral, warmer, harder.

| ID / file | Beat | Line | Delivery | Target |
|---|---|---|---|---|
| `VO_Ch1_Sangye_01` | `farewell` · *if: "I promise."* (to Aunt Pema) | *"I promise."* | B0 · Close · quiet and sincere; maybe he is not sure he can keep it · slow | 0.8-1.3 s (2.4) |
| `VO_Ch1_Sangye_02` | `farewell` · *if: "I'll bring him home."* | *"I'll bring him home."* | B0 · Close · a quiet vow; stubborn, not heroic · normal | 1.2-1.8 s (2.6) |
| `VO_Ch1_Sangye_03` | `why` · *if: "For the money."* (to Nima, walking uphill) | *"For the money."* | B1 · Close · flat, a little too quick: a shield · normal | 0.9-1.3 s (2.4) |
| `VO_Ch1_Sangye_04` | `why` · *if: "For him."* | *"For him."* | B1 · Close · honest and very short; a small catch in the voice, no tears · slow | 0.7-1.1 s (2.0) |

(The third answers, "Say nothing.", are silent on purpose.)

### 5.6 Elena Ruiz (4 lines + breathing)

| ID / file | Beat | Line | Delivery | Target |
|---|---|---|---|---|
| `VO_Ch1_Elena_01` | `job` · the morning briefing at camp | *"Is it always this cold, or is it just me?"* | B0, teeth chattering a little · Across (joking to the team) · light, self-mocking, a small laugh at the end; first-timer nerves under the joke · quick | 2.6-3.4 s (3.7) |
| `VO_Ch1_Elena_02` | `elena_talk` · she has stopped on the trail, hands on her knees: altitude sickness | *"Sorry... my head. It's like a drum. Just give me a minute."* | **B2** · Close · headache and nausea; embarrassed to slow the team down. **Really bend over with your hands on your knees** (keep the mic at your mouth). A breath at each "..." and full stop; voice low, pressed, thin · slow, broken | 4.5-6.0 s (4.6, lengthen) |
| `VO_Ch1_Elena_03` | `elena_talk` · *if: you gave her your water* | *"Thank you. Really."* | B2 · Close · grateful, a little emotional; a breath between the two parts · slow | 1.5-2.2 s (2.5) |
| `VO_Ch1_Elena_04` | `elena_talk` · *if: you told her to keep going* | *"Okay... okay. I'm fine."* | B2 · Close · she is **not** fine; she forces herself up; a shaky breath; a weak laugh helps · slow | 2.2-3.0 s (2.7, lengthen) |
| `VO_Ch1_Elena_Breath_01` | `elena_talk` "lure": her breathing pulls the player's eye before the talk starts | *(no words: laboured breathing)* | B2-B3 · Across (she is 6-13 m away; the game places the sound at her) · fast, shallow breaths through the mouth, a soft groan on one out-breath, one cough at most · steady | 8-12 s, should loop cleanly |

### 5.7 Marek Kovač (1 line)

| ID / file | Beat | Line | Delivery | Target |
|---|---|---|---|---|
| `VO_Ch1_Marek_01` | `job` · briefing the team at camp, about 3 m away | *"Lobuche tomorrow, Base Camp the day after. Nobody plays hero."* | B0 · Across · crisp, firm, a little dry. A short beat before "Nobody plays hero." Land it as a rule, not a joke. A light Slovak colour at most · brisk | 3.8-4.6 s (4.8) |

### 5.8 Dr. Hale, on the radio (4 lines)

Record clean and close. The radio sound is added afterwards (section 7). Before each line, imagine pressing the radio
button and leave a tiny gap before the first word.

| ID / file | Beat | Line | Delivery | Target |
|---|---|---|---|---|
| `VO_Ch1_Hale_01` | `job` · the morning radio check | *"Morning, team. Clip your oximeters on... Sangye, eighty-eight. Good. Elena, eighty-two. Drink more."* | B0 · Radio · a cheerful, efficient routine. The "..." is a 1-second wait while they clip on. "Elena, eighty-two" with a small frown, then a light "Drink more." · normal | 6.5-8.0 s (6.9, lengthen) |
| `VO_Ch1_Hale_02` | `night` · the evening radio check | *"Evening, team. Numbers, please."* | B0 · Radio · the end of a long day, still friendly · normal | 1.8-2.5 s (3.1) |
| `VO_Ch1_Hale_03` | `night` · *if: you told Elena to keep going* | *"Elena is at seventy-six. Watch her tonight."* | B0 · Radio · serious and quieter: a doctor's worry · slow | 2.5-3.2 s (3.8) |
| `VO_Ch1_Hale_04` | `night` · *otherwise* (you helped Elena) | *"Elena's at eighty-one. Good work today."* | B0 · Radio · relieved, warm praise · normal | 2.2-2.9 s (3.5) |

### 5.9 The summit beat (Summit.ygs)

At the summit (8,849 m), the climber stands still for a few seconds. The view may drift once towards the World Tree's
trunk, and one line appears. Today the line has **no speaker**: it shows as text only.

> *"Nothing above you now but the tree, and the sky."* (on screen for 6.0 s)

**Three options. Record A and B, then decide by ear in the scene:**

| Option | ID / file | Line | Delivery | Target |
|---|---|---|---|---|
| **A [Proposed]**: the father's voice | `VO_Summit_Father_01` | *"Nothing above you now but the tree, and the sky."* (as written) | **B-** · In-ear · calm, quiet, warm; not triumphant. It answers "You came too early.": now it is time. Space after "now" and before "and the sky" · very slow | 3.5-4.5 s (6.0) |
| **B [Proposed]**: Sangye's inner voice | `VO_Summit_Sangye_01` | *"Nothing above **me** now but the tree, and the sky."* (one word changed) | **B3** · In-ear · wonder, exhaustion, tears close. Breathe in where the slashes are: "Nothing above me / now / but the tree / and the sky." · very slow | 4.5-5.8 s (6.0) |
| **C**: no voice | (none) | stays as text | | |

Claude's view: A is the strongest. The father's voice is the only other voice that is calm at altitude, so the player
recognises him at once, and the line closes the circle that the prologue opened. But it must fit the ending you plan
for Chapter 12 ("give him back" or "let him go"). B is the safer choice.

### 5.10 Optional lines [Proposed] (Priority C)

None of these is in the game yet. Each one needs a small data change to play. Sherpa, Tibetan and Nepali words need a
**[Native check]** before release (design rule P4: no unchecked words).

| ID / file | Where | What | Delivery | Target |
|---|---|---|---|---|
| `VO_Ch1_Father_X01` | `father_seen` lure (today a synthetic whisper plays at the father) | *"Sangye..."* whispered | B- · In-ear, whisper · a call, not a scare | 1.0-1.6 s |
| `VO_Ch1_Pema_X01` | `farewell`, right after she ties the khata | *"Kale pheb."* (Tibetan **KAH-leh PEH**, "go slowly": what the one who stays says to the one who leaves). It also echoes Nima's "Slow is fast up here". **[Native check]**: the Sherpa-language form may differ | B0 · Close · a soft blessing, almost under the breath | 1.0-1.5 s |
| `VO_Ch1_Nima_X01` | `stone`, while you tie the khata to the father's stone; Nima stands 2 m away | The mantra *"Om mani padme hum"* as Tibetans and Sherpas say it: **OHM MAH-nee PEH-meh HOONG**, murmured 3-4 times. **[Native check]**: is it right at a memorial, and how does a Khumbu Sherpa say it? | B1 · Across · a low murmur under the breath, steady rhythm, not sung | 6-10 s |
| `VO_Ch1_Porter_X01` | `porters_come`: the design says the porters are heard first | a short two-note warning whistle from up the trail | Far | about 1 s |
| `VO_Ch1_Porter_X02` | `porters_pass`, as the first porter passes | *"Namaste."* (Nepali greeting, **nuh-muh-STAY**; very common on the trail; low risk) | B2 (a huge load) · Close · friendly, out of breath | 0.8-1.2 s |
| `VO_Ch1_Porter_X03` | `porters_pass` | breathing under a heavy load, no words | B2 · Across · slow, deep, rhythmic with steps; should loop | 6-10 s |
| `VO_Ch1_Journal_01` to `_18` | `night`, the journal page at the end | Sangye's inner voice reads the page (the lines below) | B0 · In-ear · tired, calm, writing by headlamp. Record each line alone, with a short pause at the end, so the game can join them in any order | 2-4 s each |

**The journal lines** (the page is built from the player's choices, so each line is a separate file):

| File | Plays when | Line |
|---|---|---|
| `VO_Ch1_Journal_01` | always | *"Dingboche. I woke beside his duffel. It still smells of smoke."* |
| `VO_Ch1_Journal_02` | you promised | *"I promised Aunt Pema I would come down."* |
| `VO_Ch1_Journal_03` | "bring him home" | *"I told Aunt Pema I would bring him home."* |
| `VO_Ch1_Journal_04` | you said nothing | *"Aunt Pema asked me to come down. I said nothing."* |
| `VO_Ch1_Journal_05` | butter tea | *"I carry his thermos of butter tea."* |
| `VO_Ch1_Journal_06` | second bottle | *"I carry a second bottle of water."* |
| `VO_Ch1_Journal_07` | first-aid kit | *"I carry the first-aid kit."* |
| `VO_Ch1_Journal_08` | prayer beads | *"I carry his prayer beads."* |
| `VO_Ch1_Journal_09` | carried Elena's load | *"Elena's head hurt below the pass. I took part of her load."* |
| `VO_Ch1_Journal_10` | gave Elena water | *"I gave Elena my water."* |
| `VO_Ch1_Journal_11` | told Elena to keep going | *"Elena was sick. I told her to keep going."* |
| `VO_Ch1_Journal_12` | "For him." | *"Nima asked why I came. For him, I said."* |
| `VO_Ch1_Journal_13` | "For the money." | *"Nima asked why I came. For the money, I said."* |
| `VO_Ch1_Journal_14` | silence | *"Nima asked why I came. I did not answer."* |
| `VO_Ch1_Journal_15` | passed the porters on the outside | *"I passed the porters on the outside. I will not again."* |
| `VO_Ch1_Journal_16` | always | *"At Thukla I tied my khata to his name."* |
| `VO_Ch1_Journal_17` | always | *"For a moment the mountain turned to gold. Nima saw nothing."* |
| `VO_Ch1_Journal_18` | always (the last line; give it weight) | *"He says my father did not fall."* |

---

## 6. Text that is NOT recorded

| Text | Why it stays text |
|---|---|
| Lines told to "you" with no speaker: *"She ties the white khata round your neck. Her hands are cold."*, *"She ties the white khata round your neck."*, *"You tie your khata to his name."* | A narrator saying "you" would break the first-person view. The body and hands show these moments. |
| Captions: the duffel contents, *"The water is so cold it hurts your teeth..."*, *"Day 1 / 49 · Dingboche, 4,410 m"* | Quiet text the player reads at their own pace |
| Things the player inspects: the two photos, the note on the back (*"Next time with Sangye. Not before he is ready."*), Dolma's drawing, the compass | The player reads them while turning the object. A voice would rush them. |
| Hints and prompts (*"[W] follow the rope"*, *"Hold [F] to hold her hand"*, ...), the choices on screen, the titles | Interface text, not story voices |
| The journal page | Text only, unless you choose the optional journal voice (5.10) |

---

## 7. For the coder: how the game will play these files

**What the story machine does today** (`Source/YggdrasilsPeak/Story/YggStory.cpp`, read on 2026-11-20):

- A `"say"` action **shows the subtitle and moves the speaker's mouth only. It plays no audio.** Every line is
  written to the log as `Story [VO] <who>: <text>`, which is how this script was checked.
- A `"sound"` action already plays any sound asset by its game path: flat (2D), or placed in the world with `"at"` (a
  place id or a person id, such as `"dolma_spot"` or `"elena"`), with `"vol"` and `"falloff"` (default 3000 cm).
  `"if"` and `"ifnot"` work on every action, so a voice can follow the same choice as its subtitle.
- A missing file is safe: the log says `Story: no sound <path>` and the subtitle still shows. A path that starts with
  `<` (for example `"<by ear>"`) is a placeholder and plays nothing.
- A `"say"` waits for its subtitle time (`"t"`, or `1.4 s + 0.055 s per character`, between 2.5 and 9 s) before the
  next action starts. **If a clip is longer than its subtitle time, the next line overlaps it.**

**How to connect the files, with no C++ change** (once the current build work is finished):

1. Import the WAVs from `Art/Audio/Voice/Ch1/` to `/Game/Audio/Voice/Ch1/`, and from `Art/Audio/Voice/Summit/` to
   `/Game/Audio/Voice/Summit/`, as Sound Waves (not looping, except `Elena_Breath_01` and `Porter_X03`). Give them the
   sound class `SC_Voices`: add `"Voice": "SC_Voices"` to the folder map in `Tools/Audio/make_sound_classes.py`, or set
   it in the import script.
2. In `Tools/Story/chapter1_story.py`, put one `"sound"` action right before each `"say"`, with the same `"if"` or
   `"ifnot"`. For example: `{"sound": snd("Voice/Ch1/VO_Ch1_Nima_21"), "vol": 1.0}`. The helper `snd()` writes the
   full path (`/Game/Audio/Voice/Ch1/VO_Ch1_Nima_21.VO_Ch1_Nima_21`).
3. NPC lines can play at the speaker (`"at": "nima"`). Dolma's call must use `"at": "dolma_spot"`: its `"say"` already
   has that key, but `"say"` ignores it. Sangye's lines and the inner voices (father, summit, journal) play flat (2D).
4. Elena's breathing: set `BY_EAR["laboured_breath"] = snd("Voice/Ch1/VO_Ch1_Elena_Breath_01")` in the generator. The
   lure then plays at her.
5. Set each `"say"`'s `"t"` to the clip length + about 0.4 s. A small script can read the WAV lengths and write them.
6. `Summit.ygs` has no generator: add the `"sound"` action by hand, and `"who": "Father"` (option A) or the changed
   text (option B).
7. Radio: make the radio sound once (band-pass about 300-3,400 Hz plus light distortion), either baked into the four
   `VO_Ch1_Hale_NN` files (keep the clean recordings as masters in `Art/`), or as one effect in Unreal. Use one way
   for all four.
8. Preload: the story preloads only the cast's meshes. Voice files load the first time they play. Short mono files
   load fast. If the flight recorder shows a hitch on a line, add the voice paths to the preload.

**Better later (a small C++ change, not now):** a `"voice"` key on `"say"` that plays the clip (flat or at the
speaker), takes the subtitle time from the clip length, lowers music and ambience a little while someone speaks, and
stops with the chapter. Then the data needs one key per line instead of two actions.

---

## 8. Open questions for the director

1. **Dr. Hale: a man or a woman?** The lines work for both. It changes your voice choice.
2. **The summit line:** the father (A), Sangye's inner voice (B), or text only (C)?
3. **Aunt Pema: who did she lose?** The father's sister, or a widow herself? The design doc leaves this open, and it
   changes how much grief goes into *"Promise me you will come down."*
4. **"Sangye-dai":** keep the Nepali "dai", or a Sherpa-language word (after a native check)?
5. **Optional lines:** do you want the Tibetan farewell, Nima's mantra, the porters' whistle and greeting, the
   father's whispered "Sangye..."? All need a native check before release.
6. **The journal:** read aloud in Sangye's voice, or text only?
7. **Dolma, Pema and Elena:** will you voice them yourself, with a small pitch shift tested by ear, or ask someone
   later? Either way, the file names stay the same.
8. **Raw session files** are large (about 8-9 MB per minute at 48 kHz/24-bit). Keep them in local git, or only on
   your backup drive? GitHub is code-only (its LFS quota is full).
