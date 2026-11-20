# Chomolungma · Chapter 3 "The Icefall" · Design v1

Version 1.0 · 2026-11-20 · written by Claude (designer) for the director (the author). It is meant to be built from:
every beat has a trigger, its lines, its camera use and its end. Nothing here is approved yet, and nothing is built
unless it says **[Built]**.

**How to read this.** Section 0 says which chapter this is and what it gets from Chapter 2. Sections 1-3 are the
chapter itself. Sections 4-8 are how it is made. Sections 9-11 are sound, testing, risks and questions.

**Labels used below**

| Label | Meaning |
|---|---|
| **[Built]** | exists in the project today (may not yet be judged by the director) |
| **[To build]** | planned, not started |
| **[Proposed]** | a new idea in this document; the director decides |
| **[Measured]** | read from `route.json` or the level; can be checked again |
| **[To measure]** | a position or number still to read from the real terrain or the game |
| **[VO]** | a spoken line; the director records the voices himself |
| **[To verify]** | a real-world fact we believe but have not checked against a good source |
| **[Cultural check]** | Sherpa or Buddhist practice, word or belief that a Sherpa reader must check before release |
| **[Shared with Ch2]** | Chapter 2 builds or owns it; Chapter 3 only reuses it |

**Words used often**

| Word | Plain meaning |
|---|---|
| Beat | one small story step: "when X happens, do Y" |
| Story machine | our code that plays the beats from a data file (`Content/Story/<Chapter>.ygs`) |
| Rig | the camera code (`UYggViewRigComponent`). It is the **only** thing allowed to move the player's view. The story only asks it |
| Lease | a short, owned permission the story asks the rig for (hold still, a posture, hands, a lens effect...). It always ends by itself |
| Nudge | a slow, weak turn of the view toward something. The last resort; the player can always overrule it. At most 3 per chapter |
| Reflex | a small, fast head turn toward a sudden loud sound (a body reaction, not a nudge) |
| Posture | lie, sit, kneel or crawl: the eye height follows the body |
| Look trigger | a beat that waits until the player looks at something; after a few seconds a sound or a person helps, then it fires anyway |
| Serac | a tower or wall of glacier ice that can fall without warning |
| Crevasse | a deep crack in a glacier. Some are hidden under a thin snow bridge |
| Fixed rope | a rope fixed along the route; climbers clip to it with a short sling and a carabiner (a metal clip) |
| Crampons | steel spikes strapped under the boots for walking on ice |
| Icefall Doctors | the team of Sherpas who build and repair the route (ladders and ropes) through the Icefall each season |
| Lhapso | the stone altar built at Base Camp for the puja, with the prayer-flag pole [Cultural check: spelling] |
| Puja | the Buddhist blessing ceremony held at Base Camp before anyone climbs |
| Sang | the morning smoke offering of juniper branches [Cultural check: term] |
| Miyolangsangma | the goddess who lives on Chomolungma, in Sherpa belief [Cultural check] |
| SpO₂ | blood oxygen saturation in percent, the number on the oximeter |

---

## 0. Which "Chapter 3", and the hand-offs

### 0.1 The numbering

Two chapter lists exist in the project:

| Document | Its Chapter 3 | Status |
|---|---|---|
| `Chapter_Design_v1.md` (17 chapters, 2026-11-14 noon, Norse framing) | "Their Tent": Lobuche → Gorak Shep → Base Camp, the parents' camp, Nima joins | replaced the same afternoon by the Chomolungma direction |
| `Chomolungma_Chapter1.md` Part 1 (12 chapters, 2026-11-14 afternoon) | **"The Icefall"**: *"Before dawn Sangye crosses the Khumbu Icefall for the first time on ladders and fixed ropes, carrying a load to Camp 1 while the ice groans under him."* | current. Chapter 1 v2 (2026-11-19) follows this list |

"Their Tent" cannot be Chapter 3 any more. Nima walks with Sangye from the first morning, and the story is now about
the father who died last season, not parents who vanished twenty years ago. In the 12-chapter list, the walk to Base
Camp and the Base Camp chapter of v1 are both inside the new Chapter 2, "The Puja".

**So this document designs "The Icefall".** It takes its material from v1's Chapter 5 ("The Icefall, rotation 1, up":
ladders, serac zones, the wood fibre, the ice cave) and moves it into the Chomolungma story. This is question Q1
(section 11).

| v1 (17 chapters) | Chomolungma (12 chapters) |
|---|---|
| 1 The Compass, 2 The Memorial Pass | 1 Day One |
| 3 Their Tent, 4 Base Camp | 2 The Puja |
| 5 The Icefall (rotation 1, up) | **3 The Icefall** |
| 6 First Night High (the night part) | end of 3 (the night at Camp 1), then 4 |

### 0.2 What Chapter 3 receives from Chapter 2

`Chapter_Design_v1.md` ends its Base Camp chapter with the beat *"Sleep; the alarm set for 02:30"* and the question
*"Three in the morning. The Icefall."* Chapter 3 starts exactly there.

| Item | Value | If Chapter 2 does not set it |
|---|---|---|
| Place | Sangye's tent in the expedition camp at Base Camp, lying in his sleeping bag | teleport (behind closed eyelids) to the tent nearest the lhapso |
| Time | **02:30**, the night after the puja | 02:30 |
| Day | **Day 5 / 49** (v1 calendar: day 3 arrival, day 4 puja, day 5 the Icefall) | Day 5 |
| Late start | v1's Icefall chapter lets the player choose the start time. If Chapter 2 sets flag `alarm_0600`, the chapter opens at 06:00 instead (section 3.5) | 02:30 |
| Taught in Chapter 2 | fixed rope and ladder, on a training ladder at Base Camp (Chapter 1 v2: "ropes and ladders (Chapter 2)") | flag `ladder_trained` missing → beat B05 and ladder 1 teach them (fallback lines given) |
| Blessed at the puja | flags `puja_axe`, `puja_khata`, `puja_cord` (a protection cord, *sungdi* [Cultural check]) | lines that mention them are skipped |
| Dr. Hale | met in person at Base Camp | she still speaks only on the radio here |
| Realm | the Hungry Ghosts realm has passed | `ygg.Realm.Force none` at start |
| Body | first nights at 5,274 m; the physiology state carries over (no reset) | — |
| Flags from Chapter 1 | `promise / bringhome / silent`, `tea / water2 / firstaid / mala`, `elena_load / elena_water / elena_push`, `money / forhim / quiet`, `safe / risky`, `read_photo` | every line that uses one has a default |

### 0.3 What Chapter 3 hands to Chapter 4

Chapter 4, "The Valley of Silence": *"Up the Western Cwm to Camp 2, a furnace of sun by day and a freezer by night, and
back down: the first rotation."*

| Item | Value |
|---|---|
| Place and time | Camp 1, Sangye's tent, Day 6, about 05:00 |
| Who is there | Nima, Marek, Ji-woo, and Elena unless she was sent down (`elena_down`) |
| The plan, said aloud | Marek: *"Tomorrow we touch Camp 2 and come down."* |
| Open questions | the light on the Lhotse Face; why Nima prayed |
| Flags out | section 7.4 |

---

## 1. One-page summary

### What the chapter is

At 02:30 on Day 5, Sangye drinks sweet tea in the dark, watches Nima burn juniper at the puja altar, and shoulders his
first real load as a climbing Sherpa. The team walks up the glacier by headlamp, straps on crampons where the rubble
ends, and enters the Khumbu Icefall: six ladders over black crevasses, two zones under leaning seracs, and the
constant groaning of moving ice. Elena freezes on a ladder and the player decides what happens to her. Above the
halfway rest, a serac falls where the team stood a minute before. At the longest ladder, Nima stops: this is where he
took the photo of Sangye's father, the one with *"Not before he is ready"* on the back. At the top, the Western Cwm
opens in full sun. At night in Camp 1, Sangye wakes gasping, steps outside, and sees one light high on the Lhotse Face.
Nima says it is a fixing team, prays, and tells him to come inside.

### The feeling

**Fear held in the body, then quiet awe.** The first two thirds are tension: dark, cold, height, the ice clock. The
last third opens up: dawn on Pumori, the gold rim above the father's ladder, the white valley of the Cwm. The myth
touches the chapter only once, at the end, and it can still be explained away.

### The shape

| Part | Minutes | Tension (1-5) | Company |
|---|---|---|---|
| Wake, the juniper fire, the load (Base Camp, 02:30) | 0-10 | 2 | Nima, the team, the kitchen boy |
| The dark glacier to Crampon Point | 10-19 | 2 | the team; Dr. Hale on the radio |
| Lower Icefall: ladders 1-2, serac zone 1, **Elena freezes** | 19-28 | 3-4 | the team |
| The steep ice to the Football Field (ladder 3) | 28-40 | 3 | the team; Nima lets you lead |
| **Dawn at the Football Field** | 40-44 | 1 | the team |
| Middle Icefall: ladder 4, the black wood, **the queue, the serac fall** | 44-56 | 4-5 | the team, strangers |
| **Ladder 5: the father's photo** (the emotional peak) | 56-63 | 3 | Nima |
| Upper Icefall: ice cave (optional), the Icefall Doctors, ladder 6 | 63-68 | 2 | two Icefall Doctors |
| **The Cwm opens**, to Camp 1 | 68-74 | 1 | the team |
| Camp 1: the load, water, the heat | 74-78 | 2 | the team |
| The evening radio call | 78-82 | 3 | Dr. Hale, Nima |
| **Night: the light on the Lhotse Face** (the end) | 82-86 | 4 | Nima |
| The journal | 86-87 | — | — |

### Length

**About 80-90 minutes** (the 12-chapter plan averages 70; Chapter 1 v2 is 70-80). The route from Base Camp to Camp 1
is 4.75 km and +929 / −74 m [Measured: legs 2 and 3]. Walking it takes about 50 minutes at today's speeds (about
2.1 m/s on the flat glacier, about 1.3-1.5 m/s on the steep ice, ×0.45 on ladders). The robot player will time it
(section 10). The cut list is in section 3.6.

### How it serves the thesis

This is the first time the player climbs above 6,000 m. The physiology model shows itself in three ways: the cold at
03:00, the falling SpO₂ on the steep ice, and the first night high (bad sleep, a headache, periodic breathing). The
evening radio call is the first moment where the player can **lie about his own numbers**. Whether players lie, and
what they do afterwards, is direct data for research question RQ3 ("does the physiology feedback change decisions?").
The route walker (`ygg.Bot.Route`, plan block A4) and the virtual climber (block B1) walk this same stretch.

---

## 2. Purpose in the arc and the emotional line

### 2.1 Purpose

1. **The real mountain begins.** Chapters 1-2 are the trek; people live there. The Icefall is where people die. The
   player must feel that the rules changed: stay clipped, never stop under seracs, one person per ladder.
2. **Sangye becomes a climbing Sherpa.** He carries a load for pay. He holds the rope for a client. By the end he goes
   first across a ladder and Nima holds the line for him. Competence grows, one ladder at a time.
3. **The father moves one step closer.** Not by a clue that explains anything, but by a place: the ladder in the photo.
   Sangye stands where his father stood.
4. **A seed of the myth.** The light on the Lhotse Face is the first thing in the game that Nima also sees, and he
   reacts with a prayer, not an explanation. The player starts to watch Nima as closely as the mountain.

### 2.2 The emotional line

Tension over poetry (director, 2026-11-14): the quiet moments are rests before the next danger, never the point.

| Min | What the player should feel | Tension |
|---|---|---|
| 0-10 | sleepy dread, ritual, weight on the back | 2 |
| 10-19 | cold, darkness, a long line of headlamps above: "we are going *up there*?" | 2 |
| 19-28 | the first slot below the rungs; fear for someone else (Elena) | 4 |
| 28-40 | effort, breath, competence ("you first") | 3 |
| 40-44 | relief, dawn, warm tea | 1 |
| 44-56 | pressure (the queue in the seracs), then shock (the fall) | 5 |
| 56-63 | grief and pride: the father's ladder | 3 |
| 63-74 | lightness, awe: the Cwm opens | 1 |
| 74-82 | heat, then the honest-or-lie choice | 2-3 |
| 82-86 | unease, wonder: the light, Nima's prayer | 4 |

**The sense of fate** comes from three places: the father's words on the photo (*"Not before he is ready"*) becoming a
question Nima asks out loud; the serac that falls where you would have been; and the light high on the face, where the
story will take you later whether you like it or not.

### 2.3 Why a player keeps going after Chapter 3

1. **Nima reacted.** He prayed at a light he called a fixing team. He knows more than he says, again.
2. **The answer to "Are you?"** The player said yes, no, or nothing. The game will remember it.
3. **People you chose for.** Elena is above with you, or below without you, because of you.
4. **The Cwm is open.** The next valley is already visible: Lhotse, the face, the light.
5. **Day 5 / 49.**

**The questions it leaves:** What was the light? Why did Nima pray? What did "too early" mean, now that Sangye has
stood on his father's ladder?

---

## 3. The beats

Every beat below has an id (used in `chapter3_story.py`), a trigger, what the player does, sees and hears, the lines
[VO], the camera use and how it ends. **F** = fixed (always happens), **S** = systemic (comes from the systems), **C** =
chosen (the player starts it). Minutes are targets.

#### B01 `wake` · 02:30, Sangye's tent at Base Camp · min 0-4 · F

- **Trigger:** chapter start (`start: true`).
- **Does / sees / hears:** black; a cheap watch alarm beeps twice. Frost on the tent wall in the headlamp beam. A hand
  pushes a tin cup through the door. The first key press sits him up; [E] takes the cup; he drinks; he crawls out
  (crawl posture, as in Chapter 1).
- **Lines:**
  - Pasang (kitchen boy, only his hand and voice) [VO]: *"Sangye-dai. Tea. Black, lots of sugar."* ... *"Nima is
    already at the lhapso."*
- **Camera:** blink wake (Lids lease); posture lie → sit → crawl; hands: cup (Hands lease, at most 60 s).
- **Caption:** *Day 5 / 49 · Base Camp, 5,364 m · 02:30* (the label height, as Chapter 1 does).
- **Ends:** standing outside the tent.

#### B02 `lhapso` · the juniper fire · min 4-8 · F, with C

- **Trigger:** standing, within 30 m of `lhapso`; or 40 s after B01. Lure: the crackle of juniper.
- **Sees:** Nima feeding juniper to a small fire on the stone altar of the puja. The smoke rises straight up in the
  still air. The puja's flag lines run from the pole, black against the stars. Up the glacier, a string of headlamps
  already climbs the Icefall: other teams. The Icefall itself is a pale shape that groans.
- **Does (optional):** walk once round the altar. Keeping it on your right (clockwise) sets `kora`. Hold [F] at the
  fire to add juniper (`sang`).
- **Lines:**
  - Nima [VO]: *"Ask her to let us go up. And to let us come down."* [Cultural check]
  - (if walking round the wrong way) Nima [VO]: *"The other way. Keep it on your right."* [Cultural check]
  - (if `promise`) Nima [VO]: *"You promised your aunt. Ask for that too."*
  - Marek [VO] (arriving, his headlamp in your eyes): *"Three o'clock. Nima leads. Sangye, you carry, and you stay with
    Elena. Nobody stops in the seracs."*
- **Camera:** Nima turns his head toward the Icefall (the companion step of a look trigger). Light as guide: the fire.
  No nudge.
- **Ends:** after Marek's line.

#### B03 `load` · the load · min 8-10 · C · **Decision 1**

- **Trigger:** near `load_pile` (6 m), next to the stores tent.
- **Sees:** loads for the high camps on a tarp: oxygen bottles with orange tops, gas cartridges, rope coils, a rolled
  tent, food bags. Each has a paper tag, "C1" or "C2".
- **Lines:** Nima [VO]: *"Twenty kilos is the job. More is a carry bonus. Less, and somebody else carries it."*
- **Choice** (weight bar as in Chapter 1; timeout 25 s; default 2):
  1. *"Fifteen. Food and rope."* → `load_15`. Nima takes the bottle himself and says nothing.
  2. *"Twenty. A bottle and gas."* → `load_20`. Nima: *"Good."*
  3. *"Twenty-five. Two bottles."* → `load_25`.
     - (with `money`) Nima [VO]: *"You said money. This is where it is."*
     - (with `forhim`) Nima [VO]: *"He carried like that, every time. His knees paid for it."*
     - (otherwise) Nima [VO]: *"Hm. We will see at the top."*
- **Effect:** pack total 15 / 20 / 25 kg (today the walk is comfortable to 16 kg, maximum 26 kg [Built]). Camp 1 stock
  gets the items at B18 [To build: camp stocks].
- **Ends:** the pack goes on with a short grunt of breath.

#### B04 `dark_glacier` · up the glacier in the dark · min 10-17 · F, with S

- **Trigger:** passing `bc_edge` (the last tent).
- **Does / sees / hears:** the team walks in a line: Nima 8-25 m ahead, then Sangye, Elena 5 m behind him, Ji-woo,
  Marek last. Rubble, then hard grey ice in the beam. Breath fog. About −15 to −20 °C [To measure: the body model at
  03:00 and 5,300 m]. Distant ice cracks.
- **At `ev_radio1` (about 400 m):** Dr. Hale [VO] (radio): *"Hale here. Good morning, if it is morning. The sun reaches
  the upper seracs at about eight. I want all of you above them by then."*
- **(if `elena_push`):** Elena coughs quietly in the cold.
- **Ends:** at Crampon Point.

#### B05 `crampon_point` · crampons on · min 17-19 · F (teaches)

- **Trigger:** within 12 m of `crampon_point`.
- **Sees:** a flat rock shelf where the rubble ends and the ice begins. A small cairn with a stick and torn prayer
  flags. Trekking boots and stuff sacks left by other climbers. The first ice blocks of the Icefall loom white.
- **Does:** [E] sits on the rock; holds [F] twice (2 s each) to strap each crampon; stands. From here the footsteps
  bite (crampon footsteps [To build: a footstep set]).
- **Lines:**
  - Nima [VO]: *"Crampons. Harness. From here to Camp 1 you stay clipped. You do not unclip in the ice. Not for a
    photo, not for a pee."*
  - (if `ladder_trained`) Nima [VO]: *"Like the training ladder. Only the hole is real."*
  - (if not) Nima [VO]: *"At a ladder, one person at a time. Flat feet on the rungs. Look at the next rung, never at
    the hole."*
- **Camera:** posture sit (with a Move hold, at most 60 s); hands: crampons; two `holdkey` F; stand.
- **Ends:** the first anchor: the HUD shows "[rope]" (automatic clip within 4 m [Built]).

#### B06 `ladder_1` · the first crevasse · min 19-21 · F

- **Where:** 973 m from Base Camp, 5,318 m, gap 3.0 m [Measured].
- **Trigger:** within 20 m of `ladder1_near`.
- **Does / sees / hears:** Nima crosses first, then stands at the far end and holds both hand lines tight [To build:
  NPC mode "hold_line"]. In the beam: the rungs, and between them, black. Crampon points scrape the aluminium, the
  ladder creaks, wind moves in the slot. He crosses at ladder speed (×0.45, with sway [Built]).
- **Lines:** Nima [VO]: *"Next rung. Next rung."* ... (at the far end) *"Good. Five more."*
- **Camera:** nothing from the story. The rig's step bob locks to the rung sound.
- **Ends:** across.

#### B07 `serac_zone_1` · red pickets in the dark · min 21-23 · S

- **Where:** 1,173 m, 5,323 m [Measured]. Serac collapse zone 0 [Built].
- **Trigger:** 15 m before the lower red pickets.
- **Sees / hears:** red pickets in the beam; above, a leaning ice wall the size of a house, its top lost in the dark.
  A long creak inside the ice.
- **Lines:** Nima [VO]: *"Red sticks. Between them we don't stop. No talking, no drinking. Walk."*
- **System:** the zone allows 30 s, slower before 08:00 (×0.7, so about 43 s), then a 5 s warning and "MOVE!" [Built].
- **Ends:** past the upper pickets.

#### B08 `elena_ladder` · Elena freezes · min 23-28 · S + C · **Decision 2**

- **Where:** ladder 2, 1,353 m, 5,341 m, gap 3.6 m [Measured].
- **Trigger:** Sangye reaches `ladder2_far` (Nima and he are across). Elena steps on the first rung and stops.
- **Sees:** across the slot, Elena's headlamp shaking on the first rung; her breath fast in the beam. Marek behind her.
- **Lines:**
  - Elena [VO]: *"I can't. I can't see the other side."*
  - Marek [VO]: *"Elena. Move. We are on a clock."*
- **Choice** (timeout 18 s; default 3):
  1. *"Look at my light. Only my light."* → `elena_across`. Hold [F] to keep the hand line tight while she crosses
     (about 25 s). Let go and the line sags: she stops until you hold again. You stand still in the cold (body
     temperature drops a little).
     - Elena [VO] (halfway): *"Okay. Okay."* (at the end, gripping your arm) *"Don't tell anyone I cried."*
     - Nima [VO]: *"Hm. Good."*
  2. *"She should go down."* → `elena_down`. Pemba (the team's second Sherpa) walks her back to Base Camp.
     - Marek [VO]: *"Fine. Pemba, take her down. Her rotation, her problem."*
     - Elena [VO]: *"I'm sorry. I'm so sorry."*
  3. Say nothing → `elena_pushed`. Marek shouts her across. On the last rung her crampon slips; the hand line jerks in
     your hands (reflex), and she catches herself. For the rest of the day she is slower (×0.85) and does not speak to
     you.
     - Marek [VO]: *"Go! Go! ... There. You see? Easy."*
- **Camera:** Elena is staged in view at the near end ("present"); while F is held, a hand pull of at most 8°/s toward
  her (Hands lease, at most 60 s); attention −2°.
- **Ends:** Elena across, or Elena walking down with Pemba.

#### B09 `steep_ice` · the climb to the Football Field · min 28-40 · S

- **Where:** 1,450 → 2,312 m from Base Camp, 5,370 → 5,704 m, slopes 14-25°, up to 44-52° just below the rest
  [Measured].
- **Trigger:** B08 ends.
- **Does / sees / hears:** the breath grows loud. The chain of headlamps above zigzags. Small details every 30-60 s:
  ice chips skitter down beside the rope; a hollow boom far away; the first grey light behind the team.
- **At about 1,600 m:** Nima [VO]: *"Bistarai, bistarai. Slowly. The ice moves a metre a day. We are faster than
  that."* [To verify: about 1 m per day]
- **Ladder 3** (1,846 m, 5,529 m, gap 2.6 m): Nima steps aside. Nima [VO]: *"You first. I watch."*
- **Ends:** at the Icefall camp marker (the checkpoint saves [Built]).

#### B10 `football_field` · dawn rest · min 40-44 · F

- **Where:** the camp marker "Khumbu Icefall" (end of leg 2), 5,704 m. Climbers call this flatter area the Football
  Field [To verify: the real one is near 5,800 m].
- **Trigger:** arriving at the camp.
- **Sees:** about 06:00. Down the valley to the west-north-west, Pumori (7,161 m) catches the first apricot light; the
  ice around you stays blue (art direction: "dawn = tiny warm areas far away, near stays blue"). Everyone sits on
  their packs.
- **Does:** drinks and eats. (if `tea`) [E] shares the father's butter tea, with Elena or Nima.
- **Lines:**
  - Nima [VO]: *"The Football Field. Half. Drink now, not later."*
  - (if `elena_across`) Elena [VO]: *"I thought I would die on that ladder."* Nima [VO]: *"Everybody thinks it once.
    Then on every ladder."*
  - (if the tea is shared with Nima) Nima [VO]: *"...His tea. Too much salt. Always too much salt."*
  - Dr. Hale [VO] (radio, 06:30): *"Morning, team. Clear today. The wind comes back on the summit in four days. Sun on
    the upper seracs at eight. Keep moving."*
- **Camera:** look trigger on `pumori_top`, Nima's head turns first; no nudge.
- **Ends:** after 90 s, or when the player walks on.

#### B11 `root` · the black wood (optional) · min about 47 · C

- **Where:** 3,108 m from Base Camp, 5,756 m, 14 m right of the rope. The `WoodFibre` clue [Built].
- **Trigger:** [E] at the clue (it already shows its text panel).
- **New text** (replaces the Norse-era text): *"Where the ice has torn the slope open, the broken face is not granite.
  It is black and fibrous, with a grain as thick as your arm: wood, burnt to charcoal and hard as iron, running
  straight down into the mountain. No tree grows this high."*
- **Lines:** (if Nima is within 25 m) Nima [VO]: *"My grandfather said the mountain has a root. Old people's talk."*
  [Cultural check: this is our invention. It is one family's story, not a Sherpa belief.]
- **Ties to:** the Wish-Fulfilling Tree on the summit (Chapter 12). Flag `saw_root`.

#### B12 `queue` · traffic in the seracs · min 50-54 · S + C · **Decision 3**

- **Where:** serac collapse zone 1 on leg 3, 3,329 m, 5,834 m [Measured].
- **Trigger:** Sangye is inside the zone.
- **Sees:** two Sherpas from another team come down fast (night carry done, empty packs) and block the rope at the
  zone's upper exit. One person at a time at the anchor [To build: NPCs that move on the fixed rope].
- **Lines:**
  - Descending Sherpa [VO]: *"Wait, wait! Two coming down!"*
  - Nima [VO]: *"Inside the red sticks. Bad place to wait."*
- **Choice** (timeout 7 s; default 1):
  1. *"Wait, clipped."* → `queue_wait`. About 25 s of standing still. The zone's clock keeps running: if you came in
     fast, it holds; if you were slow, the warning may start. Nima [VO]: *"Good. Patience is also a rope."*
  2. *"Unclip and pass on the snow bridge."* → `queue_bridge`. Step onto a snow bridge 7 m beside the rope (past the
     6 m let-go distance, so you are unroped [Built]). Keep moving and it holds. Stop on it for more than 2 s and it
     breaks: a hidden crevasse, death, back to the Football Field checkpoint [To build: a snow-bridge actor]. Nima
     [VO]: *"Never unclip in the ice. Never."*
- **Ends:** past the upper pickets.

#### B13 `serac_fall` · the near miss · min 54-56 · F

- **Trigger:** 50 m past the zone's upper pickets, or 20 s after the queue clears.
- **Sees / hears:** a crack like a rifle shot behind. A roar: a serac falls into the zone the team just left. A cloud of
  ice dust rolls up the slope.
  - `queue_wait` (you are close): the blast reaches you. You are knocked sideways, drop to one knee for 2 s, snow
    covers the lens, the world goes white for 2 s, your heart pounds. Health −8.
  - `queue_bridge` (you are further up): the dust only reaches your back.
- **Lines:**
  - Nima [VO]: *"Down! Face in!"* ... (silence; the hiss of settling snow) ... *"Elena? Ji-woo? Marek?"*
  - Ji-woo [VO] (laughing, shaking, phone up): *"Did you see that? I got it. I got all of it."*
  - Marek [VO]: *"Put that away. Move."*
- **Nobody is hurt** in either branch. This must never look like a disaster scene (section 11.3, the 2014 avalanche).
- **Camera:** reflex 10° toward the crack (a body reaction, not a nudge). In the wait branch only: knock (15°), posture
  kneel1 held 2 s, Lens (snow), Exposure (white), and the chapter's single **heartbeat FOV beat**.
- **Ends:** the team stands and moves on. Flag `serac_close` (wait branch). Ji-woo's video comes back in Chapter 5.

#### B14 `photo_spot` · the father's ladder · min 56-63 · F + C · the peak · **Decision 4**

- **Where:** ladder 5, 3,600 m from Base Camp, 5,931 m, gap 4.0 m: the longest, two ladders lashed together
  [Measured].
- **Trigger:** within 25 m of `ladder5_near`. Nima stops at the near end, in your way, his head turned to the ladder.
- **Sees:** about 07:30. The first sun touches the rim of the ice wall above the ladder: a line of gold. The slot below
  is still blue-black.
- **Lines:**
  - Nima [VO]: *"Wait."* ... *"This one. He stood in the middle and made me take his picture. Two ladders, one hand
    line, and he lets go to wave."*
- **Does:** (if `read_photo`) a hint, *[I] his photo*, opens the inspect view of `T_Photo_Icefall` (v2, rendered here,
  section 8.4). When the view lines up with the photo's frame (look cone 10° on `photo_frame`), a soft sound: the
  same wall, the same ladder, empty. (if not) Nima takes out his phone: *"I took it."* The same photo, from his hand.
- **Lines, then:** Nima [VO]: *"He wrote on the back, yes? 'Not before he is ready.'"* (a pause) *"Are you?"*
- **Choice** (timeout 12 s; default 3): *"Yes."* / *"No."* / say nothing.
  - Yes → Nima [VO]: *"Then go first. I hold the line."*
  - No → Nima [VO]: *"Good. Nobody is. Go first anyway. I hold the line."*
  - Silence → he nods and takes the hand line.
- **The crossing:** the long ladder bends and bounces at the lashing (sway ×1.5 here [Proposed, tunable]). The player
  may stop in the middle, where the father stood. After 4 s: Nima [VO]: *"Not there. Walk."*
- **Camera:** Nima's head turn (companion); inspect lease; no nudge.
- **Ends:** across. Flags `ready_yes / ready_no / ready_silent`, `stood_middle`.

#### B15 `ice_cave` · the old cache (optional) · C

- **Where:** the ice cave [Built], 40-60 m off the rope between 55% and 80% of leg 3 (between ladders 5 and 6).
- **Trigger:** its faint blue glow is visible from the rope. Leaving the rope crosses the hidden-crevasse ground
  [Built: unroped = death].
- **Lines:** Nima [VO] (when you leave the rope): *"No. That ground is hollow."* He does not follow.
- **Inside** (recast; the old rune note goes): an old expedition dump: two faded orange oxygen bottles (one still
  holds gas), a frozen food tin, a torn flag string. Choice: take the full bottle (+3 kg; one bottle for later) / also
  carry the empty one down as rubbish (+3 kg) / leave both. Flags `cave_bottle`, `cave_rubbish`.
- **Back on the rope:** Nima's trust drops. (if `cave_rubbish`) Nima [VO]: *"...Good. She sees that."* [Cultural
  check]
- The journal may mention the real rule that each climber must bring down 8 kg of rubbish [To verify].

#### B16 `doctors` · the ladder that moved · min 63-67 · F, with C

- **Where:** ladder 6, 4,053 m, 6,033 m, gap 2.8 m [Measured].
- **Trigger:** within 30 m of `ladder6_near`.
- **Sees:** two Icefall Doctors re-lashing the ladder. The glacier moved it in the night.
- **Lines:**
  - Doctor (the older one) [VO]: *"Forty centimetres since yesterday. Two minutes."* (looks at Sangye) *"You are
    Mingma's son. You walk like him."* [Cultural check: saying a dead man's name. Safe version: *"You are his son. You
    walk like him."*]
- **Does (optional):** hold [F] to hold the ladder's end while they tie it (`helped_doctors`: the wait is shorter and
  Nima nods), or just wait.
- **Ends:** the ladder is fixed; cross.

#### B17 `cwm_opens` · the top of the Icefall · min 68-70 · F

- **Where:** 4,187 m from Base Camp, 6,044 m, slope 3° [Measured].
- **Trigger:** within 30 m of `cwm_top`.
- **Sees:** the ice flattens and the Western Cwm opens: a white valley between Nuptse on the right and Everest's West
  Shoulder on the left, the Lhotse Face shining at the far end. The sun is full. Within a minute the cold turns to heat
  [To build: heat, Chapter_Design_v1 §4.1].
- **Lines:**
  - Nima [VO] (very quietly): *"Lha gyalo."* [Cultural check: is it said at a place like this?]
  - (if `elena_across`) Elena [VO]: *"Oh."* Ji-woo films.
- **Camera:** light as guide; attention −3° for 4 s; the breath calms. No nudge.
- **Sound:** one deep, slow cue (picked by ear, section 9).
- **Ends:** walking on to Camp 1.

#### B18 `camp1` · Camp 1 · min 70-78 · F

- **Where:** the camp marker "Camp 1", 4,754 m from Base Camp, 6,130 m measured (label 6,065).
- **Trigger:** within 15 m of `camp1_cache`, about 09:30.
- **Sees / hears:** a few tents on cut snow platforms; glare off the snow; tent fabric glowing; heat pressing. Alpine
  choughs calling [To verify: these birds do follow climbers this high].
- **Does:** [E] drops the load at the cache (Camp 1 stock + the load's items [To build]). [E] at the stove: fills the
  pot with snow (hands: pot).
- **Lines:**
  - Nima [VO]: *"Load down. Water first. Always water first."*
  - Marek [VO]: *"Clients sleep here tonight. Tomorrow we touch Camp 2 and come down. Sangye, you stay with us."*
- **Ends:** a blink (time skip behind closed eyelids) to 18:30.

#### B19 `radio` · the evening call · min 78-82 · S + C · **Decision 5**

- **Trigger:** after B18's blink. Sangye sits in the tent (posture sit), the stove purring, Nima opposite. The wrist
  shows his true SpO₂ and pulse [Built: physiology model].
- **Lines:** Dr. Hale [VO] (radio): *"Evening, Camp 1. Numbers, please. Sangye first."*
- **Choice** (timeout 15 s; default 3), the numbers filled in live:
  1. *"{spo2}."* (the true number) → `radio_honest`. Hale [VO]: *"{spo2}. Normal for a first night. Drink a litre
     before you sleep."* (if a headache) *"One tablet tonight. Tell me at five."*
  2. *"{spo2+6}."* (a better number) → `radio_better`. Hale [VO]: *"{spo2+6}? Very good."* Nima looks at you and says
     nothing.
  3. Say nothing → `radio_nima`. Nima reads your wrist and says the true number for you.
- **Then:** Elena's number (if she is here) depends on B08. Hale [VO]: *"Sleep well. Hale out."*
- **Ends:** lying down, lids close.

#### B20 `night_light` · the end · min 82-86 · F

- **Trigger:** after B19, the clock jumps to 22:40 behind closed eyelids.
- **Does / sees / hears:** Sangye wakes gasping. His body forgot to breathe for a few seconds (periodic breathing, real
  at this height) [To build: a breath override]. He sits up and crawls out. −20 °C. The Cwm lies silent under the
  stars. High on the Lhotse Face (about 6,900 m [To measure]), one light. Steady. Then it moves up. (if the compass is
  held up [Q]) the needle trembles toward it; only Sangye sees that.
- **Lines** (Nima's voice comes from behind first):
  - Nima [VO]: *"Fixing team, maybe."* ... (a long look) ... *"Om mani padme hum. Om mani padme hum."* ... *"Come
    inside. Lights at night are not our business."*
- **The light goes out.**
- **Camera:** look trigger on `lhotse_light` (no lure: silence is the point); **nudge 1 of 3** (pitch at most 8°) after
  12 s; a reflex to Nima's voice; attention −3°.
- **Ends:** blink to sleep.

#### B21 `page` · the journal · min 86-87 · F

- The page writes itself (head *Day 5 / 49*). Draft lines by flag:
  - always: *"Day 5. The Icefall."*
  - `load_15 / load_20 / load_25`: *"I carried fifteen kilos to Camp 1."* / *"I carried twenty. The job."* / *"I
    carried twenty-five. Nima says my father did too."*
  - `elena_across / elena_down / elena_pushed`: *"Elena froze on the second ladder. I held the line for her."* /
    *"...Pemba took her down."* / *"...Marek shouted her across. I said nothing."*
  - `queue_wait / queue_bridge`: *"We waited inside the seracs. Then one fell where we had stood."* / *"I unclipped on
    a snow bridge. Nima has not forgiven me."*
  - `stood_middle`: *"I stood where he stood, on the long ladder."*
  - `ready_yes / ready_no / ready_silent`: *"Nima asked if I am ready. I said yes."* / *"...I said no."* / *"...I did
    not answer."*
  - `cave_bottle`: *"I took an old bottle out of the ice."*  `cave_rubbish`: *"I carry an empty one down for her."*
  - `radio_better`: *"I lied to Dr. Hale about my number."*
  - always: *"Tonight there was a light on the Lhotse Face. Nima prayed."*
- Title: *CHOMOLUNGMA · Chapter 3 · The Icefall*. End (the chapter stops in the dark, as Chapter 1 does).

### 3.2 The decisions and what they change

| # | Decision | Options | Cost now | What comes back later |
|---|---|---|---|---|
| D1 | The load | 15 / 20 / 25 kg | stamina per step, ladder sway, Nima's fatigue (if 15) | Camp 1 and Camp 2 stock (Chapter 4, Chapter 9 "The Push"); the journal |
| D2 | Elena on ladder 2 | hold the line / send her down / say nothing | time and cold / a smaller team / she is slow and silent | Chapter 4: is she in the Cwm? Chapter 5: thanks or a quarrel. Chapter 11: her acclimatisation, and whom she listens to at 8,400 m |
| D3 | The queue in the seracs | wait clipped / the snow bridge | the serac clock / a death risk | how close the serac fall comes; Nima's trust (Chapter 8: how much truth he tells) |
| D4 | "Are you ready?" | yes / no / silence | — | Nima remembers the answer (Chapters 7-8); the journal |
| D5 | The radio | true number / better number / silence | — | Dr. Hale's trust: shorter forecasts and fewer warnings if she learns (Chapter_Design_v1 §3.2); the illness model ignores lies; logged for RQ3 |
| C | Optional | walk round the altar, add juniper, share the tea, the black wood, the ice cave, help the doctors, stop mid-ladder | — | journal lines, trust, one oxygen bottle |

### 3.3 The teaching ladder

| Mechanic | Introduce | Develop | Twist | Conclude | Taught by |
|---|---|---|---|---|---|
| Crampons | sit and strap them on (B05) | the footsteps change on ice | — | worn to Camp 1 | Nima |
| Stay clipped | the first anchor, "[rope]" | the rope leads everywhere | the queue tempts you off it (B12) | the snow bridge, or the falling serac | Nima |
| Ladders | the training ladder in Chapter 2 (or ladder 1) | ladder 1, Nima holds the line | ladder 2: you hold the line for Elena; ladder 3: you go first | ladder 5: the longest, alone in the middle | Nima |
| Serac zones | zone 1 in the dark | "between them we don't stop" | zone 2: you are forced to wait | the serac falls behind you | Nima |
| The ice clock | Hale: "sun at eight" | the Football Field reminder | the late-start variant | the heat at the Cwm | Dr. Hale |
| Honest numbers | the Chapter 1 radio check | — | the first chance to lie | Chapter 4 onward | Dr. Hale |

### 3.4 Pacing check (rule: no stretch over 6 minutes without a decision, an event or a discovery)

Distances along the route [Measured]; minutes at 2.0 m/s on the glacier and 1.3-1.5 m/s on steep ice.

| From | To | Distance | About | OK? |
|---|---|---|---|---|
| last tent | the radio call | 400 m | 3.3 min | yes |
| radio call | Crampon Point | 430 m | 3.6 min | yes |
| Crampon Point | ladder 1 | 140 m | 1.2 min | yes |
| ladder 1 | serac zone 1 | 200 m | 1.7 min | yes |
| serac zone 1 | ladder 2 (Elena) | 180 m | 1.5 min | yes |
| ladder 2 | "bistarai" line | 250 m | 2.5 min | yes |
| "bistarai" | ladder 3 | 240 m | 2.5 min | yes |
| ladder 3 | Football Field | 466 m, 25-52° | 5.5 min | **just**: add a systemic detail (falling chips, a boom) |
| Football Field | ladder 4 | 497 m | 5.0 min | yes |
| ladder 4 | the black wood | 299 m | 3.0 min | yes |
| black wood | serac zone 2 (queue) | 221 m | 2.3 min | yes |
| queue | serac fall | about 100 m | 1 min | yes |
| serac fall | ladder 5 (photo) | about 170 m | 1.8 min | yes |
| ladder 5 | ladder 6 (doctors) | 453 m | 4.5 min | yes (the cave glow in between) |
| ladder 6 | the Cwm opens | 134 m | 1.3 min | yes |
| the Cwm | Camp 1 | 567 m, uphill | 5.5 min | **just**: the heat and the choughs fill it |

### 3.5 The late-start variant (`alarm_0600`)

If Chapter 2 lets the player sleep until 06:00, the chapter opens at 06:00: Pumori is already lit, so B10 is a plain
rest. The team reaches serac zone 2 near 10:00-11:00, when the serac clock speeds up (×1.7 from 11:00 [Built]). Nima
[VO] at B02: *"Late. We pay for this in the seracs."* Everything else is the same.

### 3.6 Cut list (if the chapter runs long)

1. B15 the ice cave (already optional).
2. B16 the doctors become sound only (voices and hammering behind a serac).
3. B04's radio call merges into B10.
4. B11 the black wood (already optional).

---

## 4. Places with real coordinates

### 4.1 How the numbers were made

All positions come from `Tools/RoutePlanning/route.json` (points are `[x, y, z, slope°]` in Unreal centimetres) and
the terrain georeference (memory note *terrain-georeference*):

- Pixel of a world point: `col = (x + 504000) / 3374.24`, `row = (y + 504000) / 3374.24` (row 0 is north).
- Height: `png = (z − 129770) × 128 / 951.4 + 32768`, then `metres = (png + 52342) / 13.4538`.
  (Check: Dingboche comes out at 4,334 m, the measured ground.)
- Latitude and longitude (33.742 m per pixel, centre pixel 504.5 = 28.008 N, 86.921 E):
  `lat = 28.008 − (row − 504.5) × 33.742 / 110900`, `lon = 86.921 + (col − 504.5) × 33.742 / 98290`.
  (Check: the summit pixel gives 27.988 N, 86.925 E, the real summit.)
- "From BC" is the distance along the route from the start of leg 2. Positions at a leg fraction use the same rule as
  the placement scripts: point index `int(f × (points − 1))`. A placed actor may sit a few metres off (the scripts
  shift it to keep clear of other things); the world builder traces the real ground.
- Heights are measured ground heights. Captions use the label heights, as Chapter 1 does (Base Camp 5,364, Camp 1
  6,065). The two disagree by up to 90 m (Chapter_Design_v1 §4.13: "one source for every height" is still open).

### 4.2 The places

| Place id | What stands there | Route | From BC | Height | World x, y, z (cm) | Lat, lon |
|---|---|---|---|---|---|---|
| `bc_tent_start`, `lhapso`, `load_pile`, `bc_edge` | Sangye's tent, the puja altar and flag pole, the loads by the stores tent | camp "Everest Base Camp" (start of leg 2) | 0-60 m | 5,277 (label 5,364) | 487014, 1289409, 24885 | 27.9998 N, 86.8486 E |
| `ev_radio1` | — (the radio call) | leg 2, point 19 | 405 m | 5,290 | 527014, 1287409, 26200 | 28.0000 N, 86.8527 E |
| `crampon_point` | flat rock shelf, small cairn, left boots | leg 2, point 33 (slope 1°) | 833 m | 5,314 | 567014, 1275409, 28608 | 28.0010 N, 86.8568 E |
| `ladder1_*` | crevasse crossing 0, gap 3.0 m [Built] | leg 2, f 0.45 | 973 m | 5,318 | 581014, 1275409, 28918 | 28.0010 N, 86.8582 E |
| `serac1_*` | serac zone 0, red pickets [Built] | leg 2, f 0.56 | 1,173 m | 5,323 | 601014, 1275409, 29497 | 28.0010 N, 86.8602 E |
| `ladder2_*`, `elena_freeze` | crevasse crossing 1, gap 3.6 m [Built] | leg 2, f 0.66 | 1,353 m | 5,341 | 619014, 1275409, 31278 | 28.0010 N, 86.8621 E |
| `ladder3_*` | crevasse crossing 2, gap 2.6 m [Built] | leg 2, f 0.84 | 1,846 m | 5,529 | 651014, 1309409, 50072 | 27.9980 N, 86.8653 E |
| `football_field` | camp "Khumbu Icefall": 2 tents, 2 oxygen bottles, checkpoint [Built] | end of leg 2 | 2,312 m | 5,704 | 657014, 1343409, 67598 | 27.9949 N, 86.8659 E |
| `pumori_top` | look target: Pumori, 7,161 m | DEM pixel about (233.9, 482.5) [To measure: the DEM peak] | — | 7,161 (real) | about 285179, 1124000 | 28.0147 N, 86.8281 E |
| `ladder4_*` | crevasse crossing 3, gap 3.2 m [Built] | leg 3, f 0.22 | 2,809 m | 5,695 | 694739, 1323059, 66640 | 27.9968 N, 86.8698 E |
| `root` | the `WoodFibre` clue, 14 m right [Built] | leg 3, f 0.35 | 3,108 m | 5,756 | 722739, 1331059, 72783 (+14 m right) | 27.9960 N, 86.8726 E |
| `serac2_*`, `queue_spot`, `snow_bridge` | serac zone 1 [Built]; snow bridge 7 m beside the rope [To build] | leg 3, f 0.47 | 3,329 m | 5,834 | 742739, 1337059, 80557 | 27.9955 N, 86.8747 E |
| (checkpoint) | "Icefall - halfway" [Built] | leg 3, f 0.50 | 3,447 m | 5,876 | 750739, 1345059, 84738 | 27.9948 N, 86.8755 E |
| `ladder5_*`, `photo_frame` | crevasse crossing 4, gap 4.0 m, the lashed double ladder [Built span; lashing look To build] | leg 3, f 0.58 | 3,600 m | 5,931 | 754739, 1359059, 90249 | 27.9935 N, 86.8759 E |
| `ice_cave` | the ice cave and cache [Built] | 40-60 m off leg 3, between f 0.55 and 0.80 | 3,540-4,190 m | 5,911-6,044 | read from the level (tag `IceCave`) [To measure] | — |
| `ladder6_*`, `doctor_a`, `doctor_b` | crevasse crossing 5, gap 2.8 m [Built] | leg 3, f 0.76 | 4,053 m | 6,033 | 768739, 1401059, 100417 | 27.9897 N, 86.8773 E |
| `cwm_top` | the flat top of the Icefall | leg 3, point 62 (slope 3°) | 4,187 m | 6,044 | 774739, 1413059, 101523 | 27.9886 N, 86.8779 E |
| `camp1_cache`, `camp1_tent`, `camp1_tent_sit`, `camp1_out` | camp "Camp 1": 4 tents, 3 oxygen bottles [Built] | end of leg 3 | 4,754 m | 6,130 (label 6,065) | 794739, 1465059, 110153 | 27.9839 N, 86.8799 E |
| `lhotse_light` | a light on the Lhotse Face | leg 5, point 63-64 | — | about 6,890-6,910 | about 1142911, 1624421, 185758 [To measure: line of sight from `camp1_out`, 3.8 km away] | 27.9696 N, 86.9154 E |

### 4.3 What the world builder must add

A new `Tools/Story/build_chapter3_world.py` (same pattern as `build_chapter1_world.py`, run in the editor) writes
`Content/Story/Chapter3_places.ygs`. It must:

1. Trace every place above onto the real ground (detail tiles or ice meshes, skipping initial overlaps; the Chapter 1
   gotchas apply).
2. Find each ladder's near and far ends from the placed `Crevasse_<n>_leg<k>` actors, not from the route.
3. Place `snow_bridge` 7 m to the side of serac zone 2's upper exit, on ground that slopes less than 20°.
4. Measure the line of sight from `camp1_out` to `lhotse_light` and move the light along leg 5 until it is visible.
5. Check the frames: Pumori visible from the Football Field; the ice wall above ladder 5 lit at 07:30; the Lhotse Face
   in view from `cwm_top`.
6. Add the Crampon Point props, the load pile, the doctors' rope coils and the cave's new cache.

---

## 5. Cast and their states

| Who | Role here | States in this chapter | Look |
|---|---|---|---|
| **Sangye** (you, 22) | the new climbing Sherpa | pack 15 / 20 / 25 kg; crampons off → on; clipped or unroped; standing, sitting, kneeling, crawling | first-person body; needs a climbing version (crampons on the boots, harness) [To build] |
| **Nima** (about 45) | the lead, the secret | at the altar → leads 8-25 m ahead → holds the ladder's far end → waits at anchors → rests → stops at the father's ladder → leads → sits in the tent → outside at night, praying | `nima_ai` [Built], climbing version [To build] |
| **Elena Ruiz** (first-timer) | the client in your care | follows 5 m behind → freezes on ladder 2 → across / walks down / pushed (slow, silent) → rests → in the tent at Camp 1 | `elena_ai` [Built], climbing version [To build] |
| **Ji-woo Han** | the witness who films | ahead, fast → filming → shaken and excited after the serac → films the Cwm | MetaHuman `jiwoo` [Built; clothes judged bad]; climbing version [To build] |
| **Marek Kovač** | the leader, the clock | last in line → shouts at ladder 2 → radio → the plan at Camp 1 | `marek_ai` [Built], climbing version [To build] |
| **Dr. Hale** | the doctor at Base Camp | radio: 03:15, 06:30, 18:30 | voice only |
| Pasang | kitchen boy at Base Camp | a hand and a voice at the tent door | an arm only (any body) |
| Pemba | the team's second Sherpa | at the back; takes Elena down if chosen | one Icefall Sherpa model, own tint |
| Two descending Sherpas | traffic at serac zone 2 | come down the rope, pass, go | the same Icefall Sherpa model, two tints |
| Two Icefall Doctors | fixing ladder 6 | lashing, talking, letting you pass | the same model, orange helmets |
| Other teams | the mountain is busy | headlamp chain above at 03:00; tents at Camp 1 | lights only; tents |
| The father (Mingma) | the absence | only in the photo and in Nima's words | the photo v2 (section 8.4) |

**Performance:** at most 6 people close at once (Nima, Elena, Ji-woo, Marek, two strangers). Strangers appear behind a
serac or behind closed eyelids (Chapter 1 rule: never load in view). Headlamps without shadows; the distant chain is
glowing cards, not lights.

---

## 6. Gameplay and physiology hooks

| System | Today | In Chapter 3 | The decision it creates |
|---|---|---|---|
| SpO₂ and the physiology model | [Built] | rest at Base Camp, a dip on the steep ice, the first night at 6,130 m. The radio call reads the true value | lie or not (D5) |
| Body temperature | [Built] (cold); heat [To build] | about −15 to −20 °C at 03:00; standing still at ladder 2 costs warmth; the heat starts at the top of the Icefall | hold the line for Elena, and get cold |
| Stamina and pack weight | [Built] | 15 / 20 / 25 kg; 25 kg is near the 26 kg limit | carry more for the stock and the bonus |
| Fatigue | [To build] (Chapter_Design_v1 §4.1) | woke at 02:30; more ladder sway when tired | (feeds D1) |
| Water and food | [Built] | the butter tea at the Football Field; the stove at Camp 1 | share the tea or keep it |
| Acclimatisation and illness | adapts at rest [Built]; altitude illness [To build] | first time above 6,000 m: a headache at night is likely | tell Dr. Hale or not |
| Fixed ropes | automatic clip within 4 m, let go at 6 m [Built] | clipped from Crampon Point to Camp 1; the snow bridge is 7 m off, so you are unroped | stay clipped or pass |
| Manual clipping | [To build] (Chapter_Design_v1 §4.8) | **not needed**; if built, every anchor becomes a short [F] press with the carabiner sound | — |
| Ladders | ×0.45 speed, sway, a fall kills [Built] | 6 ladders; extra sway on ladder 5 [Proposed] | stop in the middle or not |
| Serac zones | 30 s, ×0.7 before 08:00, ×1.7 from 11:00 [Built] | zone 1 in the dark; zone 2 with the queue | wait in the zone or leave the rope |
| Hidden crevasses | unroped = death [Built] | the snow bridge, the way to the ice cave | the cave's bottle against the risk |
| Checkpoints | [Built] | Base Camp, Football Field, "Icefall - halfway", Camp 1 | — |
| Camp stocks | [To build] (Chapter_Design_v1 §4.4) | the load goes into Camp 1's stock | (D1) |
| Trust (Nima, Dr. Hale) | [To build] (Chapter_Design_v1 §4.10) | changes at D1, D2, D3, D5, the cave, the doctors | — |
| The clock | story `time`, `timehold`, `timeto` [Built] | 02:30 → about 06:00 at the Football Field → 07:30 at ladder 5 → 09:30 at Camp 1: about 6 game minutes per real minute (`timehold` rate about 0.25 [To tune; Chapter 1 used 0.12]) | — |
| Realms | [Built] | forced off during the climb; harmless ones allowed at Camp 1 in the afternoon [Proposed] | — |
| Bottled oxygen | [Built] | carried as a load only. Breathing it is taught in Chapter 10 | — |

**One honest gap.** Real teams take longer: most reach Base Camp after 10-14 days and spend several days there before
the Icefall. Day 5 is the game's compression (Chapter_Design_v1 §2.3). The physiology model must still give a fair
body on Day 5; this is a tuning task for the thesis model, not a story fix.

---

## 7. Camera, story data and systems

### 7.1 The camera budget

The rig is the only camera writer. The story only asks for leases (section 4.4 of the camera spec).

| Tool | Uses | Where |
|---|---|---|
| Nudge (cap 3, validator) | **1** | B20, the light (pitch ≤ 8°) |
| Heartbeat FOV beat (once per chapter) | 1 | B13, wait branch only |
| Knock | 0-1 | B13, wait branch only |
| Reflex | 3-4 | B08 (option 3), B13, B20, ice cracks |
| Postures | lie, sit, crawl, kneel1 | B01, B05, B13, B19, B20 |
| Hands | cup, crampons, photo, pot | B01, B05, B14, B18 |
| Hand pull | 1 | B08 (option 1) |
| Inspect | 1 | B14 |
| Lens, Exposure | 1 each | B13 (snow, white) |
| Cuts (teleports) | 2, both behind closed eyelids | B18 → B19, and the start if Chapter 2 left Sangye elsewhere |
| Cinematic shots | **0** | — |

Ladders get nothing from the story: the body (step bob locked to the rung sound, breath, sway) carries them.

### 7.2 The story files

| File | What | Pattern |
|---|---|---|
| `Tools/Story/chapter3_story.py` | writes `Content/Story/Chapter3.ygs`; beats B01-B21 with the ids above; `validate()` from Chapter 1 (nudges ≤ 3, one FOV beat, every teleport behind closed lids, every NPC spawn behind black or lids) | copy of `chapter1_story.py` |
| `Tools/Story/build_chapter3_world.py` | writes `Content/Story/Chapter3_places.ygs` (section 4.3) | copy of `build_chapter1_world.py` |
| `BY_EAR` table in `chapter3_story.py` | every sound left `None` until the director picks it (section 9) | as Chapter 1 |

Existing story actions used: `say`, `choice` (+ `flags`, `timeout`, `default`, `if` / `ifnot`), `flag`, `npc`,
`npc_mode` (follow, goto, idle, sit, present), `npc_face`, `npc_remove`, `pose`, `hold`, `blink`, `teleport`, `hands`,
`holdkey`, `hand`, `inspect`, `pace`, `reflex`, `knock`, `nudge`, `fovbeat`, `attention`, `exposure`, `breath`,
`light`, `caption`, `hint`, `sound`, `stopsound`, `show`, `hide`, `param`, `compass`, `cmd`, `time`, `timehold`,
`timeto`, `page`, `day`, `title`, `end`. When-fields: `start`, `after`, `delay`, `near`, `r`, `look`, `cone`, `dwell`,
`companion`, `lure`, `nudge`, `fire`, `press`, `prompt`, `posture`, `any`, `flag`, `noflag`.

### 7.3 New features the chapter needs

| Feature | Why | Size |
|---|---|---|
| NPC mode `hold_line`: walk to a ladder's far end, face back, hold the hand lines | B06, B14 | small |
| NPCs crossing a ladder at ladder speed (walk cycle is acceptable at first) | B06, B08, B14 | small-medium |
| NPCs moving on the fixed rope, up and down, one at a time at an anchor | B12, the team in general | medium |
| Live numbers in choice text (`{spo2}`, `{spo2+6}`) | B19 | small |
| A snow-bridge actor: holds while you move, breaks if you stand on it > 2 s | B12 | small (reuse the hidden crevasse) |
| The serac-fall set piece: sound, a powder cloud (Niagara), the knock in range | B13 | medium; watch the frame time |
| Footstep set "crampons" switched by a story flag | B05 onward | small (sounds exist: `Step_Ice_*`) |
| Per-crossing ladder sway multiplier | B14 | small |
| Camp stock: drop a load into a camp's stock | B03, B18 | medium (Chapter_Design_v1 §4.4) |
| Breath override "periodic breathing gasp" | B20 | small |
| Robot player: chapter 3 support and a "force choice N" switch | section 10 | small |

### 7.4 Flags

- **In** (from Chapters 1-2): see section 0.2.
- **Set here:** `kora`, `sang`, `load_15 / load_20 / load_25`, `elena_across / elena_down / elena_pushed`,
  `tea_shared_nima / tea_shared_elena`, `saw_root`, `queue_wait / queue_bridge`, `serac_close`, `ready_yes / ready_no
  / ready_silent`, `stood_middle`, `cave_bottle`, `cave_rubbish`, `helped_doctors`, `radio_honest / radio_better /
  radio_nima`, `saw_light`, `compass_light` (the needle moved for the light).
- **Read later:** Chapter 4 (`elena_*`, `load_*`, `radio_*`), Chapter 5 (`serac_close`, Ji-woo's video, `elena_*`),
  Chapters 7-8 (`ready_*`, `queue_bridge`, `saw_light`), Chapter 9 (`load_*`, `cave_bottle`), Chapter 11 (`elena_*`).

---

## 8. Assets

### 8.1 What exists already

| Need | Asset | Where |
|---|---|---|
| Icefall ice, seracs, crevasse lips | `SM_Ice*` blocks + `M_IcefallIce` (about 70% of the reference look, test patch only); `SM_Serac_*`; crevasse lips | `/Game/Environment/Glacier/...` [Built]; extending the look over the whole route is plan block A4 |
| Ladders, pickets, ropes | `SM_IcefallLadder_Span3`, `SM_IcefallLadder_Section`, `SM_SnowPicket`, `AFixedRope` | `/Game/Props/Blender` [Built] |
| Crevasses, serac zones, hidden crevasses, ice cave, wood fibre, checkpoints | `ACrevasseCrossing` ×6 on legs 2-3, `ASeracCollapseZone` ×2, hidden crevasses ×6 on leg 3, `SM_IceCave`, `WoodFibre` | placed in the level [Built] |
| Camps | Base Camp (9 tents, mess, stores, altar), Icefall camp (2 tents), Camp 1 (4 tents) | [Built]; tent look: Meshy `tent_dome`, `tent_mess`, `tent_kitchen` in `/Game/Story/AI` [Built] |
| Camp props | `barrel_blue`, `duffel`, `bedding`, `headlamp`, `water_bottle`, `glacier_glasses`, `stove_set`, `cook_set`, `daypack`, `butter_lamp`, `tarchen`, `chorten_b` (a small cairn with a stick: Crampon Point) | `/Game/Story/AI` [Built] |
| Loads | `SM_O2_Bottle`, `SM_O2_Mask`, `SM_Food_GasCanister`, `SM_Food_Stove`, `SM_Food_Thermos`, `SM_Food_RationPouch`, Megascans rope coil | `/Game/Props`, `/Game/Fab` [Built] |
| Prayer flags | `M_PrayerFlag` with printed lungta textures | [Built] |
| People | `nima_ai`, `elena_ai`, `marek_ai`, `sangye_ai` (Meshy, rigged); `jiwoo` (MetaHuman) | `Content/Story/Cast.ygs` [Built] |
| The father's photo | `T_Photo_Icefall` | [Built] but a stand-in: it shows only a snow surface. Must be re-made (8.4) |
| Compass | `SM_Compass2_*` | [Built] |
| The lhapso | the camp altar at Base Camp | [Built] as camp dressing; a better one is [Shared with Ch2] |

### 8.2 New assets for Meshy

In the house style of `Tools/Meshy/assets.py` (entries are pasted into a round-3 list; colours and tints are made in
Unreal). Props are sized by their largest side; characters by height.

```python
    # ---------------------------------------------------------------- chapter 3: the Icefall (props)
    dict(id="crampons_pair", kind="prop", size=0.35, tris=8000,
         what="a pair of steel twelve-point mountaineering crampons with long front points, orange anti-balling plates "
              "under them and black nylon straps with buckles, lying side by side, scratched and dull from use"),
    dict(id="ice_anchor", kind="prop", size=0.35, tris=5000,
         what="an ice screw anchor drilled into blue glacier ice, a steel carabiner clipped to its hanger, a faded blue "
              "fixed rope running through the carabiner, frost crystals on the metal"),
    dict(id="load_pack", kind="prop", size=0.85, tris=10000,
         what="a large 70-litre expedition backpack in faded red, packed full, two orange-topped oxygen cylinders "
              "strapped upright on its back, a coiled rope under the lid, a yellow foam mat strapped on the side, dusty"),
    dict(id="old_o2_bottle", kind="prop", size=0.6, tris=4000,
         what="an old weathered orange steel oxygen cylinder from the 1990s, paint chipped and faded to pale orange, a "
              "brass valve with no regulator, frost and small ice crystals on it, lying on its side"),
    dict(id="juniper_offering", kind="prop", size=0.5, tris=6000,
         what="a bundle of fresh green juniper branches smouldering on a flat grey stone, thin white smoke, a small "
              "heap of grey ash and a few grains of rice beside it"),
    dict(id="radio_handset", kind="prop", size=0.2, tris=3000,
         what="a black handheld two-way radio with a short stubby antenna, scuffed rubber sides, a coiled lanyard"),
    # ---------------------------------------------------------------- chapter 3: people in climbing gear (rigged)
    dict(id="sangye_climb", kind="char", size=1.70, tris=50000,
         what="a Sherpa man of about 22 from Nepal with short black hair, wearing a red down jacket with a darker patch "
              "sewn on the left elbow, a black climbing harness with two short slings and carabiners at the waist, dark "
              "grey softshell trousers, thin black liner gloves, and black insulated mountaineering boots with steel "
              "crampons strapped on"),
    dict(id="nima_climb", kind="char", size=1.68, tris=50000,
         what="a Sherpa mountain guide of about 45 from Nepal with a weathered sun-darkened face and a little stubble, "
              "wearing a faded mustard-yellow down jacket with charcoal shoulder panels, a white climbing helmet with a "
              "headlamp on it, glacier glasses pushed up on the helmet, a black climbing harness with carabiners and a "
              "jumar, dark grey softshell trousers worn lighter at the knees, and black mountaineering boots with steel "
              "crampons; real worn fabric, no logos"),
    dict(id="elena_climb", kind="char", size=1.66, tris=50000,
         what="a Spanish woman of about 35 on her first Himalayan expedition, light olive skin with a sunburnt nose, dark "
              "brown hair in a low ponytail, wearing a faded red down jacket one size too big, a white climbing helmet "
              "with a headlamp, a new grey climbing harness, mid-grey trousers and new grey-and-teal mountaineering boots "
              "with steel crampons; real fabric texture, no logos"),
    dict(id="marek_climb", kind="char", size=1.80, tris=50000,
         what="a Slovak expedition leader in his late 40s with fair sunburnt skin and a short untidy brown beard, "
              "wearing a well-used dark olive down jacket, an orange climbing helmet with a headlamp, a black harness "
              "with carabiners, charcoal softshell trousers and black mountaineering boots with steel crampons; real "
              "worn technical fabric, no logos"),
    dict(id="jiwoo_climb", kind="char", size=1.74, tris=50000,
         what="a Korean man of about 25, slim, short black hair under a dark blue climbing helmet with a headlamp, "
              "wearing a new bright blue down jacket, a small action camera clipped to the shoulder strap, a black "
              "harness with carabiners, black softshell trousers and black mountaineering boots with steel crampons; "
              "real fabric texture, no logos"),
    dict(id="icefall_sherpa", kind="char", size=1.66, tris=50000,
         what="a Nepali Sherpa rope fixer of about 35 with a weathered face, wearing a faded blue down jacket, an "
              "orange climbing helmet, a harness hung with ice screws and carabiners, a coil of rope over one shoulder, "
              "dark trousers and black mountaineering boots with steel crampons; worn, dusty clothes, no logos"),
```

### 8.3 Credit estimate

Prices from the brief: a textured prop about 33 credits; a rigged character about 41.

| Set | Items | Credits |
|---|---|---|
| Props, needed | crampons_pair, ice_anchor, load_pack, old_o2_bottle | 4 × 33 = **132** |
| Props, optional | juniper_offering (only if Chapter 2 does not make one), radio_handset | 2 × 33 = 66 |
| Characters, full set | sangye_climb, nima_climb, elena_climb, marek_climb, jiwoo_climb, icefall_sherpa | 6 × 41 = **246** |
| Characters, minimum | sangye_climb, nima_climb, elena_climb (Marek and Ji-woo stay in trekking clothes, in the dark and at distance) | 3 × 41 = 123 |
| **Total** | full: 378 (+66 optional) · minimum: 255 · add about 20% for retries | **about 300-530** |

Meshy credits were nearly used up on 2026-11-20 (the handoff note says 7 left; the director offered 1,000). Buying
credits is the director's money: the plan is to test three models first (crampons, sangye_climb, nima_climb), as in
round 1.

### 8.4 Made without Meshy

| Asset | How |
|---|---|
| **`T_Photo_Icefall` v2** | rendered in game at ladder 5 at 07:30: the father model (`father_ai`) standing mid-ladder, one hand up waving, the lit ice rim behind; then aged with the existing photo look. Chapter 1's duffel uses the same texture, so it changes there too (Q3) |
| Ladder 5 lashing | two existing ladder sections placed end to end, with a rope wrap at the joint (existing rope material); the director judges the look |
| Serac-fall powder cloud | Niagara (Epic sample smoke or snow-burst content first; check the cost at 1080p) |
| Load pile | assembled from existing props (barrels, bottles, gas, rope coils, duffels) |
| Crampon Point | `chorten_b` cairn + existing boot scans (`Downloads/boots_2026-11-14`, CC BY, credited) + stuff sacks |
| Headlamp chain | glowing cards along legs 2-3 above the team, fading out at dawn |

---

## 9. Sound needs (a list: the director picks every sound by ear)

Each item becomes a `BY_EAR` key in `chapter3_story.py`, left empty until chosen. Some candidates already exist in
`/Game/Audio`, but they are synthesized placeholders, never judged.

| Key | Moment | Candidates that exist |
|---|---|---|
| `alarm` | a cheap watch alarm at 02:30 | — |
| `tent_frost` | frozen tent fabric crackling as he sits up | — |
| `juniper_fire` | juniper crackle and hiss at the altar (also the lure) | — |
| `mantra_far` | murmured prayers from another camp (a recorded voice, if any) | — |
| `crampon_strap` | buckles and straps | `Gear/Carabiner_Click_*` (similar) |
| `steps_crampons` | crampon steps on hard ice, on snow, on rubble | `Footsteps/Step_Ice_*`, `Step_Snow_*`, `Step_Rubble_*` |
| `rung_scrape` | crampon points on aluminium rungs; the ladder creak and flex | `Footsteps/Step_Ladder_*` |
| `rope_clip` | carabiner gate at an anchor; rope running through it | `Gear/Carabiner_ClipIn_*`, `Gear/Rope_Slide_*`, `Gear/Rope_Tighten_*` |
| `icefall_bed` | the Icefall's deep groans and creaks; water under the ice after sunrise | `Ice/Ice_Groan_*`, `Ice/Ice_Crack_*` |
| `slot_wind` | hollow wind moving inside a crevasse | `Weather/Wind_Whistle` |
| `serac_creak` | a long creak inside a serac zone (also a lure) | `Ice/Ice_Groan_*` |
| `serac_fall` | rifle-shot crack, roar, powder hiss, ice settling | `Ice/Serac_Collapse_*`, `Ice/Avalanche_*` |
| `heart` | the heartbeat at the near miss | `Story/Story_Heartbeat` |
| `radio` | squelch and static on the radio calls | `Story/Story_Radio_Squelch`, `Story_Radio_Static_Loop` |
| `photo_match` | the soft sound when the photo lines up with the real ladder | — |
| `cwm_cue` | one deep, slow cue when the Cwm opens: epic, a sense of fate, never light (memory: audio identity) | the wheel finalists `UI/UI_Chime_K / L / M` are the same family |
| `choughs` | Alpine choughs calling at Camp 1 | — |
| `stove_gas` | a gas stove roaring in the tent; snow crunching in the pot | `Story/Story_Stove_Loop` (kerosene) |
| `tent_flap` | tent fabric in a light wind | — |
| `gasp` | Sangye waking from periodic breathing (could be the director's own voice) | `Body/Breath_Phrase_*` |
| `cwm_night` | the silence of the Cwm at night; a far avalanche on Nuptse | `Ice/Mountain_Boom_*` |
| `light_drone` | a very low drone while the light is on the face (or nothing at all) | `Story/Story_Plume_Drone` |

**Music:** almost none. One cue at the Cwm. Possibly a drone at the light. Silence at the serac fall (only sound) and
at the end.

**Voices to record (the director):** Nima, Elena, Marek, Ji-woo, Dr. Hale, Pasang, two descending Sherpas, two Icefall
Doctors, Sangye's few spoken answers. Every line is logged as `Story [VO] who: text` when it plays, so the voice script
can be pulled from a robot run.

---

## 10. Robot-player test plan

Two robots exist: the story robot (`ygg.Bot.Run`, `ygg.Story.Auto 1`) and the route walker (`ygg.Bot.Route`, plan
block A4). The editor must be closed for a robot run (one heavy process at a time).

### 10.1 Gate: the route must be walkable first

`powershell -File Tools/Bot/run_bot.ps1 -Chapter route -RouteArgs "from 2 to 3"` must reach Camp 1 with **0 deaths and
0 stuck teleports**, three runs in a row. Today it does not: on 2026-11-20 two runs fell into the crevasses at ladder 2
(1,353 m into leg 2) and ladder 3 (1,846 m), and a leg-3 run was blocked 25 times [Measured: `Saved/Bot/route_*`].
This is A4 work and comes before any Chapter 3 build.

### 10.2 Story runs

`run_bot.ps1 -Chapter chapter3 -Fast` [To build: chapter 3 support in the robot, plus a switch that forces choice N].

| Run | Forced choices | Must reach | Checks |
|---|---|---|---|
| R1 default | all defaults (D1 = 20 kg, D2 = silence, D3 = wait, D4 = silence, D5 = silence) | all of B01-B21 except the optional beats | finishes; total time 75-95 min |
| R2 | D2 = hold the line, D3 = bridge (keeps moving) | B08 hand-hold completes; B13 bridge branch | Elena crosses in under 40 s of holding; no death on the bridge |
| R3 | D2 = send down, D3 = bridge and **stops** on it | death at the snow bridge, respawn at the Football Field, chapter continues to B21 | story state survives the death (flags kept, NPCs re-placed behind lids) |
| R4 | `alarm_0600` set, D1 = 25 kg | the late variant; B12 near 10:00-11:00 game time | serac warnings counted; stamina never stuck at 0 |
| R5 | `read_photo` off; the robot walks to the ice cave | B14 phone branch; B15 cache | the cave detour; an unroped death is allowed and logged |

### 10.3 What each run must measure

1. **Every beat fired**, with its time stamp (beat log).
2. **Pacing:** no gap over 6 minutes between beats or systemic events (computed from the beat log).
3. **Camera (flight recorder):** nudges ≤ 3 (expect 1); one FOV beat at most; knock only in the wait branch; every cut
   behind closed lids; the eye at least 120 cm high whenever moving faster than 0.6 m/s, **including on ladders**; no
   lease left alive after its beat (especially Pace after a ladder).
4. **Body numbers (for the thesis):** SpO₂ at rest at Base Camp, at the Football Field and at Camp 1; body temperature
   at 03:00; hydration at Camp 1; the true SpO₂ shown at B19. Compare with the targets in section 6.
5. **Performance:** frame rate at the Football Field, at ladder 5 with all NPCs, during the serac powder cloud, and at
   Camp 1 at night (target: at least 60 fps at 1080p High on the laptop, as plan block A4 asks).
6. **Validator:** `python -I Tools/Story/chapter3_story.py` prints `validate: OK` before any game run.

---

## 11. Risks and open questions

### 11.1 Questions for the director

| # | Question | Default if no answer |
|---|---|---|
| Q1 | Chapter 3 is "The Icefall" from the 12-chapter list, not v1's "Their Tent". Agreed? | yes, the Icefall |
| Q2 | Sangye stays at Camp 1 overnight with the clients. Real load-carrying Sherpas often go back down the same day. Keep the night at Camp 1? | keep it |
| Q3 | Re-render the father's Icefall photo at ladder 5 with the father on the ladder. This also changes Chapter 1's duffel photo. OK? | yes |
| Q4 | Meshy: full set (about 380 credits + retries) or minimum (about 255)? | test 3 models first, then decide |
| Q5 | The light on the Lhotse Face: right for Chapter 3, or too much myth this early? (Plan B: only the compass trembles at the Cwm) | keep the light |
| Q6 | May the player send Elena down (she misses the first rotation, which changes Chapters 4-5)? | yes |
| Q7 | Names: Pasang (kitchen boy), Pemba (second Sherpa), Mingma (the father's name on the stone, used by the doctor) | keep; the doctor uses the safe line until checked |
| Q8 | Sherpa and Nepali words in the lines (*bistarai*, *dai*, *Lha gyalo*, the mantra), with English subtitles? | yes |
| Q9 | Six ladders (real seasons have more, often 10-30)? | six; more later if the route allows |
| Q10 | Build manual clipping for this chapter (a strong feel upgrade, about 1-2 days)? | no; automatic clip stays |

### 11.2 Cultural check list (before the script is final)

Mark: **R** = likely right, but check; **?** = unsure; **I** = our invention.

| # | Item | Mark |
|---|---|---|
| 1 | The lhapso: its name, its form, and the morning juniper offering before going into the Icefall | R |
| 2 | Walking round the altar clockwise (keeping it on your right) | R |
| 3 | The words *sang* (smoke offering) and *shukpa* (juniper) in Sherpa usage | ? |
| 4 | Asking Miyolangsangma for safe passage; how a Sherpa would say it | R |
| 5 | *Sungdi*, the protection cord from the lama at the puja | R |
| 6 | *Lha gyalo*: when it is said (passes, summits, the end of the puja); is it natural at the top of the Icefall? | ? |
| 7 | Murmuring *Om mani padme hum* when afraid at night | R |
| 8 | Saying a dead man's name on the mountain, less than a year after his death | ? |
| 9 | Any Sherpa belief about lights on the mountain at night. We do **not** invent one; Nima only says "not our business" | — |
| 10 | "The mountain has a root": one grandfather's story, not a belief | I |
| 11 | Sherpas speaking Sherpa among themselves; *bistarai* and *dai* are Nepali | R |
| 12 | Carry bonus per load, as a real practice and word | R |
| 13 | The rule that climbers bring down 8 kg of rubbish | R |
| 14 | Kitchen staff bringing "bed tea" before an Icefall start | R |
| 15 | The Icefall Doctors: a real team; our two are fictional people | R |

### 11.3 Risks

| Risk | Likelihood | Effect | Mitigation |
|---|---|---|---|
| The route is not yet walkable (robot deaths at the ladders; the Icefall look covers only a test patch) | high, today | Chapter 3 cannot be played | Chapter 3 waits for plan block A4 (section 10.1) |
| **Disrespect toward real deaths.** In April 2014 a serac fall in the Icefall killed 16 Nepali mountain workers | medium | deep offence to Sherpa families and players | the serac fall hurts nobody; no bodies, no rescue scene; no reference to 2014; a Sherpa reader checks the scene |
| Cultural mistakes (rituals, words, names) | medium | disrespect to a living culture | section 11.2; a Khumbu Sherpa consultant before the title demo (Chapter 1 v2 plan) |
| NPCs on ladders and ropes look wrong (sliding feet, no ladder animation) | high | breaks belief at the most watched moments | NPCs cross while the player is busy or in the dark; one crossing animation later |
| Characters still in trekking clothes in the Icefall | medium | looks fake (no helmets, no crampons) | Meshy climbing versions (8.2); minimum set first |
| The serac-fall cloud costs too much frame time | medium | a stutter at the scariest moment | measure; a smaller cloud with a lens effect carries most of it |
| Camp stocks and trust are not built, so D1, D3 and D5 have no real later effect | high | choices feel empty | flags and journal lines from day one; Nima's and Hale's lines change; the stock system comes in Chapter 4's build |
| The chapter runs long (queues, slow players on ladders) | medium | boredom | the cut list (3.6); robot timing; the director's play |
| Elena's branch (sent down) multiplies later chapters | medium | more writing | only her presence and a few lines change; no new scenes |
| The light on the face reads as silly or as a UFO | low-medium | breaks the realism | one steady, small light, like a headlamp; it goes out; Nima's reaction carries it |
| Chapter 2, written at the same time, ends differently | medium | the hand-off breaks | section 0.2 lists every fallback; only beat B01 depends on it |

---

## Appendix: sources

| Source | Used for |
|---|---|
| `Docs/Development/Chomolungma_Chapter1.md` Part 1 | the 12 chapters and Chapter 3's sentence |
| `Docs/Development/Chapter_Design_v1.md` §3-5 | cast roles, systems (§4), v1 Chapter 4's end (the hand-off), v1 Chapter 5's Icefall content |
| `Docs/Development/Chapter1_v2_Design.md` | format, rules, the photo with *"Not before he is ready"*, what Chapter 1 does not teach |
| `Docs/Development/Chapter1_Camera_Spec.md` §2, §4.3-4.4, §6, §7 | rules R1-R10, postures, leases, nudge budget, story actions |
| `Tools/Story/chapter1_story.py` | story keys, the duffel photos, flags, the validator |
| `Tools/Story/build_chapter1_world.py` | the father's name on his stone (Mingma Sherpa, 1978-2026) |
| `Tools/RoutePlanning/route.json`, `place_crevasses.py`, `place_hazards.py`, `place_slice_content.py`, `place_ice_cave.py`, `place_camps.py` | every position, gap and zone in section 4 |
| `Saved/Bot/route_20261120_*` | today's route-walker results (section 10.1) |
| `Tools/Meshy/assets.py` | the prompt house style |
| Memory notes: chomolungma-direction, climb-design-decisions, story-machine-chapter1, terrain-georeference, art-direction, audio identity (fate), meshy-pipeline, metahuman-cast-pipeline, icefall rebuild, phase 2 slice | direction, rules, pipelines, formulas, what is built |
