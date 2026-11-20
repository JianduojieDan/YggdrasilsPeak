# Chomolungma · Chapter 5 "The Icefall" · Design v1

Version 1.0 · 2026-11-20 · written by Claude (designer and technical lead) for the director (the author).
Source of the chapter: `Chapter_Design_v1.md` §5, Chapter 5 "The Icefall (rotation 1, up)". The format follows
`Chapter1_v2_Design.md`. The camera words follow `Chapter1_Camera_Spec.md` (the view rig is the only camera writer).

**Numbering note.** This document uses the 17-chapter numbering of `Chapter_Design_v1.md` (as asked). In the 12-chapter
list of `Chomolungma_Chapter1.md` the same chapter is **Chapter 3, "The Icefall"**. The content is the same; only the
number changes when the director picks a list (open question Q2).

**How to read this.** Sections 1-3 are the chapter. Section 4 is where things stand on the real ground. Sections 5-7 are
people, systems and camera. Sections 8-10 are assets, sound and tests. Section 11 is the build list. Section 12 is risks,
the cultural check list and the open questions.

**Labels used below**

| Label | Meaning |
|---|---|
| **[Built]** | exists in the project today (read from code, data or the level) |
| **[To build]** | planned, not started |
| **[Proposed]** | a new idea in this document; the director decides |
| **[VO]** | a spoken line; the director records the voices himself |
| **[To measure]** | a position or number that must be read from the terrain before it is built |
| **[To verify]** | a fact we believe but have not checked against a good source |
| **[Cultural check]** | Sherpa or Tibetan Buddhist detail that a Khumbu Sherpa reader must check before release |

**Words used often**

| Word | Plain meaning |
|---|---|
| Beat | one small story step: "when X happens, do Y" |
| Story machine | our code that plays the beats from a data file (`.ygs`) |
| Rig | the camera code; the only thing allowed to move the player's view |
| Lease | a time-limited permission the story asks from the rig (hold still, inspect, breath...) |
| Nudge | a slow, gentle turn of the view toward something; the mouse always wins; a last resort |
| Reflex | a 10° body turn toward a sudden loud sound; the player finishes the turn |
| Serac | a tower or wall of glacier ice that can fall at any time |
| Fixed rope | a rope the route team ties along the route; climbers clip to it with a carabiner |
| Anchor | the point where the fixed rope is tied to the ice (an ice screw or a snow stake) |
| Crampons | steel spikes strapped under the boots for walking on ice |
| Liners / mitts | thin gloves (good fingers, cold) / thick down mittens (warm, clumsy) |
| SpO₂ | blood oxygen saturation in percent (the oximeter number) |
| AMS | acute mountain sickness: headache, sickness; the first step of altitude illness |

---

## 1. One-page summary

### What the chapter is

Day 5. At 02:30 an alarm wakes **Sangye** in his tent at **Base Camp (5,274 m)**. He and **Nima** burn juniper at the
camp's stone altar, walk out onto the glacier in the dark, strap on crampons, clip into the fixed rope and climb the
**Khumbu Icefall**: six aluminium ladders over crevasses, two marked serac zones, a queue at a ladder inside the
danger, the sunrise at the **Football Field (about 5,726 m)**, a tower of ice that falls behind them, and at last the
flat snow of the **Western Cwm** and **Camp 1 (about 6,134 m)** at about 10:00. Sangye drops his first load there.
The chapter ends at the tent door with Nima's warning about the first night above 6,000 m.

Distance **4.75 km**, climb **+929 m / −74 m**, all on the real 1:1 terrain [Built, measured from `route.json`].

### The feeling (the emotional line)

**Fear held in the hands, then awe.** The Icefall is the first place in the game that can kill you at any moment, and
the body knows it: cold fingers at every clip, breath loud in the dark, the ladder moving under the crampons, the ice
groaning underneath. The awe comes in two short breaths: the sunrise turning Pumori gold behind them, and the silence
of the Cwm when the ice noise stops. The father is not seen. He is felt: his photo of an Icefall ladder, his habit of
touching the black wood, the thermos he filled with butter tea.

Rules from the director that shape it:
- **Tension over poetry** (D2, 2026-11-14): every quiet moment is short and serves the next danger.
- **Not alone the whole climb** (D4): Nima is beside Sangye all chapter; the team and other climbers' lights come and go.
- **A sense of fate** (sound identity): the Icefall's sound is deep and slow, never "UI-cute".
- **Quiet awe** (art direction): attention moves between the boots on the ladder rung and the far, lit peak.

### The shape

| Part | Minutes | Tension (1-5) | Company |
|---|---|---|---|
| Wake, tea, the altar (02:30-03:00) | 0-5 | 2 | Nima, the cook, the team |
| The lower glacier in the dark: crampons, the first clip, ladder 1 | 5-15 | 3 | Nima; headlamps far ahead |
| Serac zone 1 and the queue at ladder 2 | 15-22 | 4 | Nima, a big team, Marek's group behind |
| The steep lower Icefall, ladder 3, the own-line choice | 22-33 | 3-4 | Nima |
| The Football Field at sunrise | 33-37 | 1 | Nima, then the clients arrive |
| The upper Icefall: Ji-woo, ladder 4, the radio, the black wood, serac zone 2 | 37-52 | 3 | Nima, Ji-woo, Dr. Hale's voice |
| Ladder 5 (the longest) and the father's photo | 52-55 | 4 → 2 | Nima |
| **The serac falls behind them: the peak** | 57-58 | 5 | Nima |
| Ladder 6, the top, the Cwm opens; the ice cave (optional) | 58-64 | 2 | Nima, or alone in the cave |
| The Cwm, Camp 1, the load: **the end** | 64-74 | 2 → 3 | Nima; the clients far behind |

### Length

**About 70-75 minutes** (v1 target: 70). Walk about 52 minutes, other about 20; the optional cave adds about 3. The
robot player measures the real numbers (section 10). The route robot of 2026-11-20 walked leg 2 in 21:27 game minutes
with no story, at about 1.8-2.1 m/s [Built, `Saved/Bot/route_20261120_110212_L2-3`].

### Hand-offs (kept exactly as `Chapter_Design_v1.md` says)

| | From Chapter 4 "Base Camp" | To Chapter 6 "First Night High" |
|---|---|---|
| Where | Sangye's tent at Base Camp | Camp 1, at the tent door |
| When | game day 4 night; alarm set for 02:30 | game day 5, about 10:00 (later if the player waited for light) |
| Ends on | *"Three in the morning. The Icefall."* | *"First night above 6,000 m."* |
| Carried | the load plan and pack split (Ch4 decisions 2-3), what was blessed at the puja (decision 4), Ch1 flags | Camp 1 stock (+ the load), Nima's trust, the body (fatigue, water, the start of AMS), the team still arriving |

The **first remote medical check** (honest or lie) stays in Chapter 6 at 19:00. Chapter 5's 07:00 radio call is news and
weather only.

### How it serves the thesis

The Icefall is the first stretch where the physiology model matters minute by minute: SpO₂ falling from about 80 to
about 70 while climbing, cold, effort on steep ice, load, the first steps of AMS. It sits on **demo route 1** of the
development plan (block A4, Dingboche → Camp 1), so the A4 fixes (route walker, Icefall path, collision) are this
chapter's foundation. Marek's clients climb with the **same body model** as the player: the first use of the
"virtual climber" (block B1) as people in the game.

---

## 2. Place in the arc

### 2.1 Why this chapter exists

- **Act II (the Mortal Realm), the first real danger.** Chapters 1-4 were the trek and Base Camp. Chapter 5 is the
  first time above Base Camp, the first ladder, the first rope, the first place where a mistake is death.
- **It teaches the mountain's grammar** that every later chapter uses: clipping at anchors, ladders, serac timing,
  load carries. It teaches them once, by a person, in the moment of need (Chapter 1 pillar P2).
- **It starts the rotation idea.** Up to sleep high, down to recover. The body feels why: Camp 1 at 6,134 m is about
  1,100 m above what Sangye is used to, and the headache begins as he drops the load.
- **It deepens the father's thread without answers.** Chapter 1 left: *"Your father did not fall."* Chapter 5 puts
  Sangye where his father worked for twenty seasons, through things the father touched, and Nima keeps the secret.

### 2.2 The emotional line, minute by minute

| Minute | What the player should feel | What carries it |
|---|---|---|
| 0-5 | sleepy dread, warmth of tea | the alarm in the dark, the stove, Nima's few words |
| 5-15 | the dark is huge; the light is small | headlamp tunnel, the line of other lights crawling up a black wall |
| 15-22 | first real fear, and a choice under pressure | the queue in the serac zone, the ice groaning |
| 22-33 | effort, focus, a temptation | steep ice, breath, the old boot track going straight up |
| 33-37 | relief and awe | sitting down, butter tea, Pumori turning gold behind |
| 37-52 | routine becomes skill; people around | Ji-woo's recklessness, Dr. Hale's voice, the black wood |
| 52-55 | vertigo, then grief | the longest ladder; the father's photo |
| 57-58 | **terror** | the serac falls behind them; snow on the neck; Nima shouting |
| 58-64 | silence, wonder, a second temptation | the Cwm opens; the blue light of the cave |
| 64-74 | tired pride, then unease | the load dropped; the heat; the first headache; Nima's warning |

### 2.3 The questions it leaves open

1. What did Nima mean in Chapter 1? (He still does not say. At the photo he says: *"Ask me at the top."* At the top
   he does not answer.)
2. What is the black wood inside the mountain?
3. Will Sangye sleep at 6,134 m? (The headache has started.)

---

## 3. The player journey

### 3.1 The journey, minute by minute

Minutes are targets from the measured distances (section 4) at about 1.9 m/s on the lower glacier, about 1.35 m/s on
the steep Icefall and about 1.5 m/s in the Cwm. Game clock: **6 game minutes per real minute** (v1 §4.6), start 03:00
at the altar. "Along" = metres along the route leg.

| Min | Scene | What the player **does** | **Sees / hears** | **Feels** | **Learns** |
|---|---|---|---|---|---|
| 0-2 | **Wake** in the Base Camp tent, 02:30 | sits up, crawls out (taught in Ch1) | a watch alarm, frost on the tent wall in the headlamp; *"Day 5 / 49 · Base Camp, 5,274 m"* | cold dread | |
| 2-4 | **Tea in the kitchen tent** | takes the cup; **choice: go now or wait for light** | the cook's stove roar; Nima eating tsampa; the load waiting | warmth | the start time matters |
| 4-5 | **The altar**: juniper smoke | walks round it (clockwise, optional) | smoke in the headlamp, prayer-flag lines into the dark, Nima's murmured mantra | stillness | respect; keep holy things on your right |
| 5-10 | The lower glacier in the dark | walks; **at the crampon point (min 8) kneels and straps crampons; choice: mitts or liners** | the Icefall a black wall with a line of lights crawling up it | smallness | crampons; gloves cost something either way |
| 10 | **The first clip** | clips at the first anchor (hold a key) | Nima's two carabiners in the headlamp | care | clipping at anchors |
| 12 | **The ice talks** | keeps walking | a deep groan under the feet | a shiver | the ice is alive |
| 14-15 | **Ladder 1** (3.0 m) | crosses; Nima holds the far end | the slot in the headlamp, black and blue; crampons on aluminium | vertigo | ladders: look at the next rung |
| 16 | **Serac zone 1**: red pickets | walks through without stopping | leaning towers above in the beam | pressure | serac zones: don't stop |
| 18-21 | **The queue at ladder 2** (3.6 m) | **choice: wait inside the danger, step back and lose the place, or cross a snow bridge** | six climbers of a big team, one crossing slowly; ice chips ticking down; Marek's group arrives behind | fear, impatience | time inside a danger is the cost |
| 24-25 | **The loose strap** (only if mitts were chosen) | kneels and re-straps, or risks it | a metallic clack, then a scrape in every step | annoyance, then care | small mistakes grow |
| 27-28 | **Ladder 3** (2.6 m), alone | crosses without Nima holding it; may leave the rope for the old track | an old boot track going straight up the slope | temptation | your own line is possible, and deadly |
| 33-37 | **The Football Field** at sunrise (about 05:50) | sits on the pack; drinks, eats; **choice: give Nima part of the load** | Pumori behind turns gold, then the Nuptse ridge; Ji-woo arrives filming; Elena far below | relief, awe | rest is a resource |
| 40 | **Ji-woo overtakes** on steep ice | holds the line (or steps aside) | Ji-woo unclips and swings past on the outside; ice chips hit Sangye | anger, worry | why the rule "always one carabiner on" |
| 43 | **Ladder 4** (3.2 m) | crosses | | competence | |
| 45 | **Radio, 07:00** | listens while walking | Dr. Hale: weather, news, *"ladder five sags, step light"* | company | |
| 46-47 | **The black wood** | may touch it ([E]) | a burnt, fibrous trunk standing out of the ice; an old faded khata tied to it | wonder | the mountain has a secret |
| 49-51 | **Serac zone 2**, the halfway checkpoint | walks through | | grim routine | |
| 52-53 | **Ladder 5** (4.0 m, the longest) | crosses; the ladder sags in the middle | a creak; the heartbeat | real vertigo | |
| 53-54 | **The father's photo** | turns the photo over (if he read it in Ch1); **answers Nima (or not)** | *"Next time with Sangye. Not before he is ready."* | grief | |
| 57-58 | **The serac falls** behind them (the peak, about 08:15) | runs, or freezes | a crack, a roar, snow blowing past the neck; Nima: *"Move! Move!"* | terror | |
| 60 | **Ladder 6** (2.8 m), last | crosses alone; Nima says nothing | the slope eases | pride | |
| 60-61 | **The top**: the Western Cwm opens | stops, looks | flat snow between huge walls; the ice noise stops | silence, awe | |
| 61-64 | **The ice cave** (optional, 50 m off the rope) | leaves the rope across hidden-crevasse ground; **choice at the cache** | blue light inside a serac, an old cache tin | temptation, guilt | off the rope Nima cannot save you |
| 64-70 | **The Cwm walk** | opens the jacket in the heat | a faint sag line in the snow (a hidden crevasse); the sun on the snow | heat, a dull headache | heat in the Cwm; hidden crevasses |
| 70-74 | **Camp 1** (about 09:30-10:00): drop the load | kneels at the stock pile; walks to the tent | the stock panel grows; Nima at the tent door | tired pride, unease | load carries build the camps |

### 3.2 Beat specifications (implementation-ready)

Each beat says: **id** · **when** (story machine trigger) · **does / sees / hears** · **lines** ([VO], speaker) · **camera**
· **ends**. Place names are made by the world builder (section 4.3). Distances are route metres. New trigger keys and
actions are marked [To build] and listed in section 11.

**B01 `wake`** · when: `start` · in black: clock 02:30 (`time`, `timehold` at 6 game min per real min), cast preload,
spawn Nima at `bc_kitchen`, the cook in the kitchen tent, Marek's group at `bc_mess`; cut to `bc_tent_start`; pose `lie`
snap; headlamp on. Watch alarm (3 beeps), wind calm. Caption *"Day 5 / 49 · Base Camp, 5,274 m"* (low left). Eyes open
(`blink wake`). First input sits him up facing the door (`pose sit` with `exit`).
- Lines: **[VO] Nima** (outside): *"Sangye. Tea."*
- Camera: one cut behind closed lids (the chapter start). Lie → sit → crawl → stand, as in Chapter 1.
- Ends: standing outside the tent.

**B02 `kitchen`** · when: after `wake`, near `bc_kitchen` r 600 · the cook hands a cup (`hands: "cup"` [To build], or a
caption); Nima eats; the loaded pack by the door shows its weight (HUD). Marek at the mess-tent door.
- Lines: **[VO] Cook:** *"Chiya, dai."* (Nepali: "Tea, brother.") [Cultural check] · **Nima:** *"Eat. Even if you are not
  hungry."* · **Nima:** *"We go at three. The ice sleeps now. At ten it wakes up."* · **Marek:** *"Camp 1 by ten, Sangye.
  Then you rest. We follow at half past."*
- **Choice D1** (timeout 15 s, default 0): *"We go now."* · *"Let's wait for the light."* · flags `go_now`, `start_late`.
  - `start_late`: **Nima:** *"...Then we go at six. And we walk fast under the seracs."* Nima's trust −5. Blink; clock
    05:50; Marek's group leaves first (no queue at ladder 2 later; section 6.4).
- Camera: none. Ends: the pack goes on (pack weight shown); Nima walks to the altar.

**B03 `altar`** · when: after `kitchen`, near `bc_altar` r 1000 · juniper smoke (new Niagara, section 8.3) rising from
the stone altar at the flag pole; Nima stands at it, murmurs, throws a pinch of rice. The player may walk round it.
Passing it with the altar on the left (anticlockwise) triggers Nima's correction once. If Ch1 flag `mala`: caption
*"Your father's beads are in your pocket. You count three of them."*
- Lines: **[VO] Nima** (murmured): *"Om mani padme hum..."* · **Nima:** *"We ask her to let us pass."* · if anticlockwise:
  **Nima:** *"Other way. Keep it on your right."* [Cultural check: juniper offering (sang) before the Icefall; walking
  round the altar; the mantra; "her" = the mountain goddess Miyolangsangma]
- Camera: none (Nima's position and the smoke draw the eye). Ends: Nima walks out onto the glacier (`lead`, 8-25 m).

**B04 `crampons`** · when: near `crampon_point` r 500 (leg 2, where bare ice begins; candidate 360 m [To measure]) ·
Nima kneels and straps his crampons. Prompt *"[E] Put on crampons"* → `pose kneel1` hold; **choice D2**: *"Keep the mitts
on (slow, warm)"* · *"Take the mitts off (fast, cold)"* (flags `mitts`, `liners`; default 0); then `holdkey F` 6 s
(mitts) or 3 s (liners) with `hands: "crampons"` [To build]. Footsteps switch to crampons on ice.
- Lines: **[VO] Nima:** *"Crampons."* · **Nima:** *"Liners for the fingers, mitts for the cold. You cannot have both."*
- The glove state stays a live choice: a key swaps mitts and liners at any time (section 6.2).
- Camera: kneel1 posture (108 cm); look free. Ends: standing; the first anchor shines in the headlamp ahead.

**B05 `first_clip`** · when: near `rope_start` r 400 (the first anchor, about 450 m [To measure]) · manual clipping
starts here (section 6.3). Nima clips first, side by side with the player, and waits.
- Lines: **[VO] Nima:** *"Two carabiners. One is always on the rope. Always."* · hint once: *"Hold [C] at the anchor to
  clip"* (key to assign).
- Camera: `hands: "carabiner"` [To build] while clipping. Ends: clipped (rope icon on the HUD).

**B06 `ice_talks`** · when: near `ev_groan` r 800 (about 700 m) · a deep groan from under the feet, a crack far left
(`Ice/Ice_Groan_02`, `Ice/Ice_Crack_06`); heartbeat up for a few seconds (the body system).
- Lines: **[VO] Nima:** *"Only talking. When she shouts, you run."*
- Camera: `reflex` 10° toward the crack if it is 30-120° off-centre. Ends after 4 s.

**B07 `ladder1`** · when: near `ladder1_near` r 1200 (Crevasse_0_leg2, 973 m, gap 3.0 m) · Nima crosses first, then
stands at the far lip holding the hand line, facing the player (`npc_mode idle` + `npc_face player`). The player crosses
(ladder speed ×0.45 [Built]; ladder balance layer [To build]). If the player looks down past −50° for over 1 s while on
the ladder, the void fills the headlamp beam; Nima speaks once.
- Lines: **[VO] Nima:** *"Crampon points between the rungs. Look at the next rung."* · on looking down: *"Not down. The
  next rung."* · at the far lip: *"Good."*
- Camera: no nudge; look free; ladder balance layer. Ends: across.

**B08 `serac1`** · when: entering `SeracZone_0_leg2` (red pickets, 1,151-1,195 m) · the existing zone runs (creaks,
chips; warning after 30 s ×0.7 before 08:00 = about 43 s; collapse 5 s later) [Built].
- Lines: at the first pickets **[VO] Nima:** *"Red pickets. Between them we don't stop."* · if the player stops 8 s inside:
  *"Don't stop here."*
- Ends: leaving the zone.

**B09 `queue`** · when: near `queue_tail` r 300 (about 20 m before ladder 2: Crevasse_1_leg2, 1,353 m, gap 3.6 m), and
not `start_late` · six climbers of a big team (generic climbers in yellow, section 5) stand in line; one is on the
ladder. The new zone `SeracZone_Q_leg2` covers the queue [To build: a "no collapse" zone, groans and chips only, chips do
small damage]. **Choice D3** when the player reaches the tail (timeout 20 s, default 0):
- *"Wait in the line"* (`queue_wait`): about 75 s inside the zone; the ice groans at about 43 s; chips tick off the
  helmet (health −2 per hit, at most 4 hits); cold while standing (core temperature falls; fingers if liners).
- *"Step back behind the pickets"* (`queue_back`): safe; about 135 s; Marek's group arrives behind and waits too.
- *"Cross on the snow bridge"* (`queue_bridge`): unclip, 15 m left, a snow bridge over the same crevasse [To build]: it
  holds if you walk across without stopping; it breaks if you stand on it for 2 s or sprint onto it (death by crevasse,
  [Built] message). Nima does not follow; Nima's trust −10.
- Lines: **[VO] Nima:** *"A big team. We wait."* · if `queue_wait` and the groan comes: *"Not good... not good."* · if
  `queue_bridge`: *"Not there!"*, and afterwards at the top of the ladder: *"Do that again and you go down alone."* ·
  **Marek** (behind, if `queue_wait` or `queue_back`): *"Every year the same. One ladder, fifty people."*
- Camera: none forced; the queue is staged in front of the player. Ends: across ladder 2.

**B10 `strap`** · when: flag `mitts`, near `ev_strap` r 400 (about 1,600 m, 25° ice) · the left crampon strap loosens:
a clack, then a scrape in every step (footstep variant), hint *"Your crampon is loose."* Prompt *"[E] Fix it"* →
`pose kneel1`, `holdkey F` 4 s (mitts) or 2 s (liners). If the player walks on 30 m: a stumble (`knock`), health −5; if
unclipped at that moment, a 2 m slide.
- Lines: **[VO] Nima:** *"Your foot. Fix it now, not on the ladder."*
- Ends: fixed, or after the stumble.

**B11 `ladder3` and the own line** · when: near `ladder3_near` (Crevasse_2_leg2, 1,846 m, gap 2.6 m) · Nima crosses and
walks on without holding the ladder (the "develop" step). Past the far lip an **old boot track** leaves the rope and goes
straight up the slope toward the upper Icefall (decal [To build]; real reason in Nima's line). Choice by walking, not by
menu: if the player leaves the rope by more than 15 m here, flag `own_line`.
- Own line: 309 m straight to leg 3 at 420 m instead of 790 m by the route (saves about 480 m, about 5-6 minutes)
  [measured from `route.json`; walkability **[To measure]**]. It skips the Football Field rest. One new hidden crevasse
  `hc_own` lies across it (a visible sag for a careful eye) [To build]. Nima's trust −10.
- Lines: **[VO] Nima:** *"Football Field. We rest there, we drink."* · on leaving: *"Sangye! That track is old. The doctors
  moved the rope."* · if he goes on: *"...I meet you at the ladder above."* (Nima walks his own route; flag `own_line`.)
- Ends: either way at leg 3 about 420-500 m.

**B12 `football_field`** · when: near `ff_sit` r 800 (Landmark_3, the Icefall camp, about 5,726 m), not `own_line` ·
prompt *"[E] Sit and rest"* → `pose sit` on the pack (78 cm). Drink and eat (hydration and food up from the thermos and a
ration; no free refill). Sunrise: the clock is near 05:50 in the default run; Pumori (behind, to the west) turns gold
first, then the Nuptse ridge.
- If Ch1 flag `tea`: caption *"Your father's thermos. The cook filled it with butter tea."* **[VO] Nima:** *"He always put
  in too much salt."*
- Look moment: `look` on `pumori_view` (cone 16, dwell 0.35); T0+3 s `companion`: Nima turns to look back; T0+7 s lure (a
  wind shift); T0+12 s **nudge 1 of 2** (`pitchmax` 6); fire anyway at 15 s. Then **Nima:** *"Pumori. 'The daughter.'
  Your father liked this light."* [To verify: Pumori means "unmarried daughter" in Sherpa]
- If the sun is already up (`start_late`, or a slow player): **Nima:** *"Late. The sun is already on the seracs."*
- The clients arrive (if they have caught up, section 5.3): Ji-woo first, filming, loud; Elena far below with a team
  Sherpa.
- **Choice D5** (load): **Nima:** *"Give me the rope. Three kilos."* → *"Thank you."* (`rope_given`: pack −2.5 kg, Nima's
  load +2.5 kg and his fatigue up) · *"I carry my own load."* (`rope_kept`).
- After about 3 real minutes, or when the player stands: **Nima:** *"Up. The sun is coming for the seracs."*
- Camera: sit posture; Attention −3 while still; nudge only as the last step. Ends: standing, clipped.

**B13 `jiwoo_pass`** · when: near `ev_jiwoo` r 600 (leg 3 about 250 m, 31° ice) and Ji-woo is within 40 m behind ·
Ji-woo unclips, swings past on the outside of the rope on steep ice, re-clips ahead; his crampons kick ice chips onto
Sangye (sound). If the player stands on the line, Ji-woo waits, swearing quietly, and passes at the next anchor.
- Lines: **[VO] Ji-woo:** *"Sorry! Sorry, coming through—"* · **Nima** (sharp): *"Clip in!"* · **Ji-woo:** *"I'm fine, I'm
  fine!"*
- Camera: `reflex` toward his voice (he comes from behind-left, 30-120°). Ends: he is 10 m ahead.

**B14 `ladder4`** · Crevasse_3_leg3 (497 m, 3.2 m) · a plain crossing, Nima ahead. No line. (A pitched ladder if the lips
differ in height [Built].)

**B15 `fibre`** · when: near `Clue_WoodFibre` r 1100 (796 m, 14 m right of the rope) [Built] · the black trunk standing
out of the ice; an old, faded khata knotted round a spur of it (reuse Meshy `khata_tied` with an aged tint). Nima stops a
moment. Prompt *"[E] Touch the wood"* (optional, flag `touched_wood`, sanity +3). The clue text is rewritten without the
Norse runes (section 12.4).
- Lines: **[VO] Nima:** *"Old people say Chomolungma has a tree inside her."* · if the player touches it: *"Your father
  touched it every time. For luck."*
- Ends: walking on.

**B16 `radio_7`** · when: clock 07:00 [To build: `clock` trigger] and no other line playing · squelch.
- Lines: **[VO] Dr. Hale (radio):** *"Morning, Icefall. Hale here. Quiet weather today and tomorrow. Seventy on the summit,
  nobody's going up there."* · *"The doctors say ladder five sags on the far side. Step light."* · *"Nima, numbers from
  Camp 1, please."* (Nima: *"Copy."*)
- Ends: squelch out.

**B17 `serac2` + checkpoint** · SeracZone_1_leg3 (995-1,039 m) and the "Icefall - halfway" checkpoint (1,135 m) [Built].
Nima says *"Don't stop."* only if the player stops inside. The checkpoint saves.

**B18 `ladder5` and `photo`** · when: near `ladder5_near` (Crevasse_4_leg3, 1,288 m, 4.0 m, the longest) · Nima crosses
and holds the far end. The ladder balance layer is stronger in the middle (the sag Dr. Hale warned of); a creak at the
middle; heartbeat.
- Lines: **[VO] Nima:** *"The long one. One at a time."*
- **`photo`** (at the far lip, after the crossing): if Ch1 flag `read_photo` → `inspect` of the Icefall photo with its back
  (*"Next time with Sangye. Not before he is ready."*). **Nima:** *"That was not this ladder. The ice moves every day.
  That one fell in years ago."* [To verify: Khumbu Icefall speed, often given as about 1 m per day]
- **Choice** (while walking, timeout 12 s, default 2): *"Was he ready?"* · *"Am I ready?"* · *"Say nothing."* (flags
  `ask_was`, `ask_am`, `ask_none`). Replies: *"Ask me at the top."* · *"You are on the ladder. That is the answer for
  today."* · (3 s silence) *"Come."*
- Without `read_photo`: **Nima:** *"Your father had a photo from here. Not this ladder. The ice moves every day."*
- Camera: Inspect lease (the photo); look free otherwise. Ends: walking on.

**B19 `collapse`: the peak** · when: the player passes `ev_collapse` (leg 3 about 1,560 m, 5,997 m) · flag `late` is set
by a clock check at 08:30 [To build]. 0.4 s crack from behind, then the serac roar and a scripted block fall
`ygg.Serac.Drop <place> <blocks>` [To build]:
- on time (not `late`): 9 blocks at `ev_collapse_fall`, 25-35 m **behind** on the route just walked; a spindrift cloud
  overtakes the player from behind (snow on the neck), ground tremor 1.2 s, shiver, breath catches (Breath lease 0.8, 4 s);
- late: the blocks land at `ev_collapse_ahead`, 40 m **ahead**, burying the rope → B19b.
- The fall is never placed within 20 m of the player (it cannot kill; the ordinary serac zones still can).
- Lines: **[VO] Nima** (shouting, during): *"Move! Move!"* · after 3 s of silence (quiet): *"...Keep walking. Don't look
  back."* · if the player stands frozen for 5 s, Nima comes back two steps: *"Sangye. Walk."* · late: *"Back! Back!"* ·
  *"We go round. Stay on my boots."*
- Camera: sound first; no grab. `reflex` only if the source is 30-120° off-centre (behind it is the player's own turn).
  Tremor layer [To build]. Ends: 10 s later, or at B19b's start.

**B19b `detour`** (late only) · Nima leads off the buried rope along `detour_w0..w3` (80-120 m round the debris, measured
clear of hidden crevasses [To measure]) on a short hidden rope `Rope_Detour` that is shown only now [To build]; rejoins
leg 3 about 1,650 m. Adds 3-4 minutes. **Nima:** *"Clip to the old line. Slowly."*

**B20 `ladder6`** · Crevasse_5_leg3 (1,741 m, 2.8 m) · the last ladder. Nima crosses ahead and does not wait, does not
speak (the "conclude" step: the player can do it alone).

**B21 `top`** · when: near `cwm_view` r 1500 (leg 3 about 1,785 m, 6,037 m: the slope stays below 8° from here) · the ice
noise fades out over 8 s; the wind drops; the Western Cwm opens: flat snow between Nuptse (right) and the West Shoulder
(left), the Lhotse Face far ahead in sun. Nima stops.
- Lines: **[VO] Nima:** *"Western Cwm."* (pause) *"The Valley of Silence."* [To verify: the common English nickname] · if
  `ask_was` was chosen: the player gets a 10 s chance to press *"[E] Ask him"* → Sangye **[VO]**: *"You said ask you at the
  top."* Nima: *"...Camp 1 first. Then water. Then maybe."* (he does not answer; the question carries to Chapter 6)
- If the player holds up the compass ([Q], Ch1), the needle trembles toward the summit for 2 s. No line.
- Camera: no nudge (the view is straight ahead). Ends: walking on.

**B22 `cave`** (optional, chosen) · when: the player looks at `cave_glow` for 1 s (IceCave, 50 m left of the rope at
1,875 m [Built]) · Nima speaks; if the player unclips and goes, Nima stays on the rope. The approach crosses `hc_cave`, a
new hidden crevasse with a visible sag and a safe way round it [To build]. Inside: blue light, dripping, old oxygen
bottles and a rope coil [Built], and a battered cache tin [new prop] holding 2 rations, 1 water, 1 oxygen bottle [Built
contents]. The note is rewritten (section 12.4): team tape on the lid, the same red as the tents of the team that went
home after last week's death (Chapter 4's memorial).
- Lines: **[VO] Nima:** *"Leave it. Off the rope is not ours."* · when the player unclips: *"I wait here. If you fall, I
  cannot reach you."*
- **Choice D6** at the tin (`pose kneel1`): *"Take it for Camp 1"* (`cave_taken`: +5.2 kg to the pack, Camp 1 stock +1
  bottle, +2 rations, +1 water) · *"Take it, and say a prayer for him"* (`cave_prayer`: same, plus a 20 s kneel; Nima's
  trust +5 when he hears it) · *"Leave it"* (`cave_left`). [Cultural check: how Sherpas see taking a dead climber's cache]
- On return: if `cave_prayer` and the player tells him (automatic line): **Sangye:** *"I said a prayer for him."* **Nima:**
  *"Good."* · if `cave_taken`: Nima looks at the pack and says nothing.
- Camera: kneel1; inspect the note. Ends: clipped to the rope again.

**B23 `cwm_walk`** · when: after `top`, through leg 3 1,875 → 2,442 m · after 09:00 the sun heats the snow; body
temperature arrow rises (heat in the Cwm, v1 §4.1 [To build]). At `ev_hc1_sag` (about 2,000 m) Nima points at the sag of
HiddenCrevasse_leg3_1, 21 m left [Built]. The AMS model starts a faint headache cue near Camp 1 (flag `ams_onset`)
[To build].
- Lines: **[VO] Nima:** *"Open your jacket. Now you sweat, tonight you freeze."* · at the sag: *"See the line in the snow?
  A crevasse. Never walk where you don't see the rope."*
- Ends: at Camp 1.

**B24 `camp1`: the end** · when: near `c1_drop` r 600 (Landmark_4, about 6,134 m) · 4 tents and 3 oxygen bottles
[Built]. Prompt *"[E] Drop the load"* → `pose kneel1`; the load moves into the Camp 1 stock; the stock panel shows what was
added [To build, v1 §4.4]. Nima goes to the tent door (`c1_tent_door`). Caption with the clock: *"Camp 1 · 6,134 m"*.
- Lines: **[VO] Nima:** *"Good load."* · (at the tent) *"Now we drink. And we wait for the sun to finish with us."* ·
  (looking up the Cwm) *"Tonight you will not sleep. Nobody sleeps the first night up here."*
- Camera: the end is a blink at the tent door (`blink closed`, `keep`) → save "Camp 1" → Chapter 6 opens inside the tent.
- Ends: chapter end. No journal page here: the night's page belongs to Chapter 6 (v1 §4.12).

### 3.3 Story-machine sketch (two beats in the `chapter1_story.py` style)

Keys in **bold comments** do not exist yet (section 11).

```python
beat("crampons", {"after": "altar", "near": "crampon_point", "r": 500, "look": "crampon_point", "cone": 30,
                  "press": "Interact", "prompt": "[E]  Put on crampons", "nudge": False},
     {"say": "Crampons.", "who": "Nima"},
     {"pose": "kneel1", "t": 0.9, "face": "crampon_point", "hold": True},
     {"say": "Liners for the fingers, mitts for the cold. You cannot have both.", "who": "Nima"},
     {"choice": ["Keep the mitts on (slow, warm)", "Take the mitts off (fast, cold)"],
      "flags": ["mitts", "liners"], "timeout": 15, "default": 0},
     {"cmd": "ygg.Body.Gloves liners", "if": "liners"},            # NEW console command
     {"hands": "crampons"},                                        # NEW hands item
     {"holdkey": "F", "t": 6, "prompt": "Hold  [F]  to strap the crampons", "if": "mitts"},
     {"holdkey": "F", "t": 3, "prompt": "Hold  [F]  to strap the crampons", "if": "liners"},
     {"hands": False}, {"hold": False}, {"cmd": "ygg.Feet.Crampons 1"},  # NEW: crampon footsteps
     {"hint": "[W]  stand", "idle": 6})

beat("late_check", {"after": "wake", "clock": "08:30"}, {"flag": "late"})   # NEW 'clock' trigger
beat("collapse", {"after": "ladder5", "near": "ev_collapse", "r": 400},
     {"sound": snd("Ice/Ice_Crack_05"), "at": "ev_collapse_fall", "vol": 1.0}, {"wait": 0.4},
     {"cmd": "ygg.Serac.Drop ev_collapse_fall 9", "ifnot": "late"},   # NEW scripted fall, never within 20 m
     {"cmd": "ygg.Serac.Drop ev_collapse_ahead 9", "if": "late"},
     {"sound": snd("Ice/Serac_Collapse_02"), "at": "ev_collapse_fall", "vol": 1.0},
     {"say": "Move! Move!", "who": "Nima", "t": 1.5, "wait": False},
     {"tremor": 1.2},                                               # NEW rig layer (ground tremor)
     {"cmd": "ygg.FX.Spindrift behind", "ifnot": "late"},           # NEW spindrift burst
     {"breath": 0.8, "t": 4},
     {"wait": 3.0},
     {"say": "...Keep walking. Don't look back.", "who": "Nima", "ifnot": "late"},
     {"say": "Back! Back! We go round. Stay on my boots.", "who": "Nima", "if": "late"},
     {"flag": "collapse_seen"})
```

### 3.4 Decisions and who remembers them

| # | Decision | Options | Cost / gain | Remembered by |
|---|---|---|---|---|
| D1 | Start time (B02) | go now · wait for light | light and warmth vs. serac clock ×1 to ×1.7, heat in the upper Icefall, no queue | Nima's trust, the journal, Chapter 6's start time |
| D2 | Gloves (B04, then any time) | mitts · liners | slow clips and the loose strap vs. fast clips and cold fingers (frostnip) | the body, the journal |
| D3 | Ladder 2 queue (B09) | wait inside · step back · snow bridge | chips, cold vs. time vs. a deadly gamble | Nima's trust, Marek, the journal |
| D4 | Own line (B11) | the rope · the old track | 5-6 min saved vs. no rest, a hidden crevasse | Nima's trust, the journal |
| D5 | Load (B12) | give Nima the rope · keep it | speed vs. Nima's fatigue (matters later) | Nima's body |
| D6 | The cache (B22) | take · take and pray · leave (or never go) | Camp 1 stock vs. weight and a moral weight | Nima, the stock, the journal |

No option is free. Every decision changes a number the player can see later.

### 3.5 The teaching ladder

One new mechanic per scene, each in four steps (introduce, develop, twist, conclude), taught by Nima.

| Mechanic | Introduce | Develop | Twist | Conclude |
|---|---|---|---|---|
| Crampons and gloves | the crampon point | every clip after | the loose strap (mitts) / cold fingers (liners) | the sun: liners become safe |
| Clipping at anchors | the first anchor, side by side | every key anchor | Ji-woo unclips to pass | at Camp 1 the rope ends; you clipped alone all the way |
| Ladders | ladder 1, Nima holds it | ladder 2 with a queue, ladder 3 alone | ladder 5, the longest, sagging | ladder 6, alone, in silence |
| Serac zones | zone 1, red pickets | the queue zone | the tower that falls behind you | zone 2 crossed without a word |
| Load carries | the pack at Base Camp | the weight on steep ice | Nima's offer; the cache's extra weight | dropping it at Camp 1: the stock grows |

**Taught here, used later:** hidden crevasses (only shown: the sag at B23), heat in the Cwm (begins here, a system in
Chapter 6). **Not taught here:** bottled oxygen (no one breathes oxygen in the Icefall on a first rotation), rappel and
arm-wrap (Chapter 6, going down), the remote medical check (Chapter 6).

### 3.6 Pacing check (rule: no stretch over 6 minutes without a decision, an event or a discovery)

| From | To | Distance | Time | OK? |
|---|---|---|---|---|
| altar | crampon point | about 300 m | 2.6 min | yes |
| crampon point | first clip | about 90 m | 1 min | yes |
| first clip | ice talks | about 250 m | 2.2 min | yes |
| ice talks | ladder 1 | 273 m | 2.4 min | yes |
| ladder 1 → zone 1 → ladder 2 | | 380 m | 4 min | yes |
| ladder 2 | loose strap / ladder 3 | 247 / 493 m | 3 / 6 min | yes with the strap; **just** without it (mitts chosen = strap) |
| ladder 3 | Football Field | 466 m | 5.8 min | yes (the own-line choice at the start) |
| Football Field | Ji-woo | 250 m | 3 min | yes (if he caught up; otherwise ladder 4 at 497 m, 6.1 min: **just**) |
| ladder 4 → fibre → radio → zone 2 → checkpoint → ladder 5 | | 791 m | 9.8 min with 4 events | yes |
| ladder 5 → collapse → ladder 6 → top | | 497 m | 6.1 min with 2 events | yes |
| top / cave | Camp 1 | 567 m | 6.3 min | yes, with the sag and the heat lines |

---

## 4. Places on the real ground

### 4.1 Formulas (from `terrain-georeference` notes; checked here)

- World units are centimetres, 1:1. Landscape location (−504,000, −504,000), scale 3,374.24 cm per heightmap pixel,
  z offset +129,770 cm.
- **Pixel** from world: `col = (x + 504,000) / 3,374.24`, `row = (y + 504,000) / 3,374.24` (row 0 = north, so north is −Y).
- **Height** from world z: `elevation_m = ((z − 129,770) × 128 / 951.4 + 85,110) / 13.4538`
  (from `png = 13.4538 × elev − 52,342` and `z = (png − 32,768) × 951.4 / 128 + 129,770`).
  Check: Base Camp landmark z 24,503 → 5,273 m (v1 says 5,274 [Measured]).
- **Latitude / longitude**, approximate: the heightmap is a Web Mercator zoom-12 tile set centred at 28.008 N, 86.921 E
  (33.742 m per pixel is exactly zoom 12 at 28° N; this is inferred, [To verify]):
  `lat ≈ 28.008 − (row − 504) × 33.742 / 110,819`, `lon ≈ 86.921 + (col − 504) × 33.742 / 98,362`.
  Check: our summit pixel (517, 568) → 27.9885 N, 86.9255 E; the real summit is 27.9881 N, 86.9250 E (about 60 m off).
  Use the lat/lon below to compare with real maps, not to place anything: our terrain is the truth for the game.
- `route.json` points are `[x, y, z, slope°]`. Legs 2 and 3 have 91 and 79 points; the placement scripts use **point-index
  fractions** (`i = int(f × (n − 1))`), so v1's "45%" is a point index, not a distance. The "along" metres below are real
  distances.

### 4.2 What stands where

| Place | World (x, y, z) cm | Height | Along | Lat / lon (approx.) | What stands there |
|---|---|---|---|---|---|
| Base Camp, Landmark_2 | 487,014 · 1,289,409 · 24,503 | 5,273 m | leg 2: 0 | 27.9996 N, 86.8489 E | 9-tent arc, mess, kitchen, stores, the puja altar at the flag pole, 6 O2 pickups [Built, `place_camps.py`] |
| Crampon point | candidate 523,014 · 1,289,409 · 25,490 | 5,283 m | leg 2: ~360 m | 27.9996 N, 86.8525 E | [To measure: where bare ice starts]; a stone ledge to sit on, other teams' crampon bags |
| First anchor (rope start) | candidate 531,275 · 1,286,278 · 26,703 | 5,295 m | leg 2: ~450 m | 27.9999 N, 86.8534 E | the fixed rope begins [Proposed; today FixedRope_2 starts 16 m from camp] |
| Ice talks | 553,847 · 1,275,992 · 28,069 | 5,309 m | leg 2: 700 m | 28.0008 N, 86.8557 E | sound place only |
| **Ladder 1**, Crevasse_0_leg2 | 581,014 · 1,275,409 · 28,918 | 5,318 m | leg 2: 973 m | 28.0009 N, 86.8584 E | 3.0 m gap, ladder, pickets, hand line [Built] |
| **Serac zone 1**, SeracZone_0_leg2 | 601,014 · 1,275,409 · ~29,500 | 5,323 m | leg 2: 1,151-1,195 m | 28.0009 N, 86.8605 E | 44 m zone, red pickets both ends [Built] |
| Queue zone (new) | ends ~6 m before ladder 2 | ~5,338 m | leg 2: ~1,323-1,347 m | | `SeracZone_Q_leg2`, 24 m, no collapse [To build] |
| Snow bridge (new) | 15 m left of ladder 2 | ~5,341 m | leg 2: 1,353 m | | over the same slot [To build; the slot must be extended sideways] |
| **Ladder 2**, Crevasse_1_leg2 | 619,014 · 1,275,409 · 31,278 | 5,341 m | leg 2: 1,353 m | 28.0009 N, 86.8623 E | 3.6 m gap [Built] |
| Loose strap | 638,664 · 1,289,234 · 40,209 | 5,430 m | leg 2: 1,600 m | 27.9996 N, 86.8643 E | 25° ice; sound place only |
| **Ladder 3**, Crevasse_2_leg2 | 651,014 · 1,309,409 · 50,072 | 5,529 m | leg 2: 1,846 m | 27.9978 N, 86.8655 E | 2.6 m gap [Built]; **bug**: the route robot died here (simple collision 5.07 m above the visible surface) |
| Own line start | 657,768 · 1,316,162 · 51,877 | 5,547 m | leg 2: 1,942 m | 27.9972 N, 86.8662 E | old boot track decal, `hc_own` across it [To build] |
| **Football Field** (Icefall camp), Landmark_3 | 656,739 · 1,343,059 · 69,727 | 5,726 m | leg 2: 2,312 / leg 3: 0 | 27.9948 N, 86.8661 E | 2 tents, 2 O2 pickups, supplies [Built]; **the ground is 49-52°** (v1 known issue; the robot got stuck here 3 times) |
| Own line rejoin | 687,452 · 1,324,702 · 64,965 | 5,678 m | leg 3: 420 m | 27.9964 N, 86.8692 E | |
| Ji-woo overtakes | 671,849 · 1,329,949 · 61,960 | 5,648 m | leg 3: 250 m | 27.9960 N, 86.8677 E | 31° ice |
| **Ladder 4**, Crevasse_3_leg3 | 694,739 · 1,323,059 · 66,640 | 5,695 m | leg 3: 497 m | 27.9966 N, 86.8700 E | 3.2 m gap [Built] |
| **The black wood**, Clue_WoodFibre | 721,749 · 1,332,049 · 73,668 | 5,765 m | leg 3: 796 m, 14 m right | 27.9958 N, 86.8727 E | carbonised trunk + fallen piece [Built]; old khata [To build: placement] |
| **Serac zone 2**, SeracZone_1_leg3 | 742,739 · 1,337,059 · ~80,950 | 5,834 m | leg 3: 995-1,039 m | 27.9953 N, 86.8749 E | 44 m zone [Built] |
| Checkpoint "Icefall - halfway" | 750,562 · 1,345,236 · 85,258 | 5,876 m | leg 3: 1,135 m | 27.9946 N, 86.8757 E | trail checkpoint [Built] |
| **Ladder 5**, Crevasse_4_leg3 | 754,739 · 1,359,059 · 90,249 | 5,931 m | leg 3: 1,288 m | 27.9933 N, 86.8761 E | 4.0 m gap, the longest [Built] |
| **Collapse trigger** `ev_collapse` | 760,657 · 1,384,897 · 96,892 | 5,997 m | leg 3: ~1,560 m | 27.9910 N, 86.8767 E | [To measure]; fall points 25-35 m behind / 40 m ahead on the route; no anchor or ladder within 15 m of a fall point |
| **Ladder 6**, Crevasse_5_leg3 | 768,739 · 1,401,059 · 100,417 | 6,033 m | leg 3: 1,741 m | 27.9896 N, 86.8775 E | 2.8 m gap [Built] |
| **Top of the Icefall** `cwm_view` | 770,720 · 1,405,021 · 100,862 | 6,037 m | leg 3: 1,785 m | 27.9892 N, 86.8777 E | the slope stays below 8° from here |
| **Ice cave**, IceCave | 779,211 · 1,410,823 · 101,808 | 6,046 m | leg 3: 1,875 m, 50 m left | 27.9887 N, 86.8786 E | cave mesh, blue light, cache props and supplies, a note [Built]; `hc_cave` on the approach [To build] |
| Sag lesson, HiddenCrevasse_leg3_1 | 782,629 · 1,424,114 · 102,864 | 6,057 m | leg 3: 2,009 m, 21 m left | 27.9875 N, 86.8789 E | hidden crevasse with sag decal [Built] |
| **Camp 1**, Landmark_4 | 795,082 · 1,465,544 · 110,561 | 6,134 m | leg 3: 2,442 m | 27.9837 N, 86.8802 E | 4 tents, 3 O2 bottles + 3 pickups [Built]; stock pile [To build] |

**The six hidden crevasses of leg 3** [Built, `place_hazards.py`, seed 11]: at 477 m (28 m left), 666 m (29 m left),
824 m (35 m right), 1,428 m (29 m left), 2,009 m (21 m left), 2,098 m (28 m left). None lies on the own line or between
the rope and the cave; the two new ones (`hc_own`, `hc_cave`) give those two choices their real risk.

### 4.3 New places for the world builder (`Tools/Story/build_chapter5_world.py` [To build])

Same method as `build_chapter1_world.py`: trace the real ground, skip initial overlaps, accept only landscape and detail
tiles, write `Content/Story/Chapter5_places.ygs`, and check the staging rules:

| Place | Rule the builder checks |
|---|---|
| `bc_tent_start`, `bc_kitchen`, `bc_mess`, `bc_altar` | from Chapter 4's Base Camp layout (whichever tent Chapter 4 puts Sangye in); the altar = the flag pole |
| `crampon_point`, `rope_start` | first leg-2 point on bare ice (glacier mask), then the first anchor 60-120 m later |
| `queue_tail`, queue spots ×6 | single file on the rope, 2.5 m apart, the last spot inside the queue zone |
| `snow_bridge` | 12-18 m left of ladder 2 over the slot; the slot extended there |
| `ev_strap`, `ev_jiwoo`, `ev_groan`, `ev_hc1_sag` | on the route at the metres above |
| `own_line_track`, `hc_own` | the 309 m straight line is under 35° everywhere and clear of serac instances; `hc_own` at 55-65% of it, visible sag |
| `ff_sit`, `pumori_view`, `nuptse_view` | a flat sitting spot (under 12°) within 15 m of the camp; Pumori's summit visible from the sitting eye (line-of-sight trace), 3-10° up; if the Football Field is moved (section 12.1), recompute |
| `fibre_touch` | 1.5 m from the trunk on the rope side |
| `ev_collapse`, `ev_collapse_fall`, `ev_collapse_ahead`, `detour_w0..w3` | fall points on the route 25-35 m behind / 40 m ahead; no anchor, ladder or checkpoint within 15 m; the detour walkable (under 35°, no hidden crevasse within 10 m) |
| `cwm_view` | the Cwm visible straight ahead along the route |
| `cave_glow`, `hc_cave`, `cave_tin` | `hc_cave` across the direct line rope → cave mouth with a 5 m gap round one end |
| `c1_drop`, `c1_tent_door`, `c1_nima` | at Camp 1's stock pile and Sangye's tent door |

### 4.4 Light by time (art direction: quiet awe)

| Scene | Clock (default run) | Light | Art-direction note |
|---|---|---|---|
| Base Camp, the altar | 02:30-03:00 | headlamps, the stove glow through the kitchen tent, stars; juniper smoke lit by the beam | warm small light in a huge blue-black |
| Lower glacier, ladders 1-2 | 03:00-04:30 | headlamp key light; the line of other lights on the black Icefall wall; snow faintly blue under the stars | the iconic image of the Icefall at night |
| Ladder 3 | ~05:00 | first grey light; the seracs get shapes | blue ice appears |
| Football Field | ~05:50 | alpenglow: Pumori behind turns gold first, then the Nuptse ridge; the near ice stays deep blue | "dawn = warm far, blue near" |
| Upper Icefall | 06:00-08:00 | full morning light high up; the Icefall still in shadow; blue serac ice | crisp silhouettes, rust and cord colours pop |
| The collapse | ~08:10 | sun reaching the top seracs | the light is the danger |
| The Cwm, Camp 1 | 08:30-10:00 | blinding sun on snow, deep blue sky, heat shimmer near the ground | "midday: startling detail" |

**[To verify]** our sky's sun direction at 05:50 (the Ultra Dynamic Sky settings) so that Pumori really catches the first
light, and the moon at 03:00 (a waning moon would make the dark readable on a laptop screen).

---

## 5. The cast and their states

### 5.1 Who is in the chapter

The cast follows the **Chomolungma** direction (as in `Chapter1_v2_Design.md`): Sangye is a climbing Sherpa for
Marek Kovač's expedition; Marek, Elena and Ji-woo are its clients. (In `Chapter_Design_v1.md` they were "the other
team". The beats work either way; only the words "our clients" change. Open question Q1.)

| Who | Role in Chapter 5 | Seen | How made |
|---|---|---|---|
| **Sangye** (you) | first Icefall crossing, first load carry | hands, sleeves, harness, boots with crampons | Meshy `sangye` [Built]; Icefall kit variant [new, section 8] |
| **Nima** | teacher, then the quiet guide; keeps the secret | close, all chapter | Meshy `nima_ai` [Built]; Icefall kit variant [new] |
| **Ji-woo Han** | the reckless one; overtakes | close for 20 s | MetaHuman + garments [Built]; helmet prop [new] |
| **Marek Kovač** | the schedule; irritated at the queue | Base Camp, the queue (if you wait) | Meshy `marek_ai` [Built] |
| **Elena Ruiz** | slow, struggling; still on the way at the end | far, at the Football Field | Meshy `elena_ai` [Built] |
| **Two team Sherpas** | with the clients | far | generic climber [new] |
| **The big team** (6) | the line of lights; the queue at ladder 2 | the queue, close | generic climber, yellow tint [new] |
| **The cook** | tea at 02:30 | a figure in the kitchen tent | reuse a porter or cook look [Built] |
| **Dr. Hale** | 07:00 radio | voice | voice |
| **The father** | absent; his photo, his thermos, his habit | — | — |

### 5.2 Nima's states over the chapter

| Section | Mode (story machine) | Distance | Talks | Notes |
|---|---|---|---|---|
| Base Camp | `present` / `goto` | 1.5 m | teaches | |
| Glacier → ladder 2 | `lead` | 8-15 m | teaches at each new thing | waits at an anchor if the player is over 40 m behind [Built rule, v1 §4.10] |
| At ladders 1 and 5 | `idle` at the far lip, `npc_face player` | the ladder length | one line | "holding the ladder" is a pose at the far anchor; a real hold animation is phase 2 |
| Ladders 3-6, upper Icefall | `lead` | 15-25 m | only when it matters | the "goes quiet" part (v1) |
| Football Field | `sit` | 1.5 m | 2-3 lines | |
| The collapse | `lead`, comes back 2 steps if you freeze | 10 m | shouts | |
| The cave | `idle` on the rope | 50 m | 2 lines | he never leaves the rope |
| Cwm, Camp 1 | `lead`, then `goto c1_tent_door` | 10 m | 3 lines | |

**Nima's trust** (0-100, v1 §4.10 [To build]) starts at Chapter 4's value (default 50). In this chapter: wait for light
−5, snow bridge −10, own line −10, each stop of over 8 s inside a serac zone −3, prayer at the cache +5, a clean chapter
(no zone stop, no unclipped stretch over 20 m on the rope) +5.

**Nima's body** (the same model as the player, v1 §3.1): better adapted (about 5,800 m); his fatigue rises more if he
takes the rope (D5).

### 5.3 The clients as route followers

Marek's group (Marek, Elena, Ji-woo and two team Sherpas) leaves Base Camp at 03:30 (or at 05:45 before you if
`start_late`) and walks `route.json` with the same speed rules and body model (v1 §4.10 [To build]). Expected with the
default run: Ji-woo catches the player during the Football Field rest; Marek reaches the ladder 2 queue if the player
waits there; Elena falls behind and reaches Camp 1 about an hour after the chapter ends (Chapter 6 has the team at
Camp 1). If the Chapter 1 flag `elena_push` is set, Elena is slower still.

### 5.4 The big team and the line of lights

Six generic climbers in yellow, 150-900 m ahead from 03:00, at about 0.9 m/s; four of them queue at ladder 2 when the
player arrives. Their headlamps make the line of lights (each a small emissive sprite; at most four real lights,
shadowless). After sunrise they switch the lamps off and are out of reach ahead.

---

## 6. Gameplay and physiology hooks

### 6.1 The body in this chapter

| Index | Today | In Chapter 5 | To build |
|---|---|---|---|
| SpO₂ | physiology model [Built] | about 80 at Base Camp at rest, **70-75 while climbing**, about 75-78 resting at Camp 1 (v1 targets; calibrate with block A2) | — |
| Heart rate, breath | [Built] | loud breath on the steep ice and on ladders; heartbeat sound on ladder 5 and the collapse | — |
| Body temperature | [Built] | air about −20 °C at 03:00 (v1), warming after the sun; core falls when standing still in the queue | heat in the Cwm from 09:00 (v1 §4.1) |
| Fingers (new) | — | a finger-cold meter rises with liners, bare-hand straps and clips; **frostnip** at the top: clips ×1.5 slower for 30 game min and a journal line. No lasting frostbite in Chapter 5 (it is taught, not punished) | [To build] |
| Stamina | [Built] | steep ice drains it; the rest-step (Ch1) helps | — |
| Fatigue | — | +4 per game hour awake from 02:30 (about +30 by Camp 1) | [To build, v1 §4.1] |
| Water and food | [Built] | the thermos and a ration at the Football Field; no free refill at the Icefall camp | camp stock instead of free refill (v1 §4.4) |
| Acclimatisation | [Built, old rule] | adapted about 5,000 m (nights at Lobuche and Base Camp); Camp 1 is about 1,100 m higher → AMS onset near Camp 1 (flag `ams_onset`, a faint headache cue) for Chapter 6's headache decision | illness model (v1 §4.1) |
| Pack and speed | [Built] | base gear 10 kg + the load from Chapter 4 (default 11 kg) ≈ 21 kg: over "comfortable" (16), under the maximum (26) | — |

### 6.2 Gloves

A key swaps **mitts** and **liners** at any time (about 2 s, a small hands animation or just a sound and HUD icon).
Mitts: clip 1.6 s, strap 6 s, fingers warm. Liners: clip 0.8 s, strap 3 s, fingers cool fast before sunrise and slowly
after. [To build: `ygg.Body.Gloves`, HUD icon]

### 6.3 Fixed ropes and clipping

- **Today** [Built]: auto-clip within 4 m, off at 6 m; while roped: 58° walkable, no deadly fall, hidden crevasses harmless.
- **Chapter 5** [To build, v1 §4.8]: **manual clipping at key anchors**: both ends of every ladder, both ends of every
  serac zone, and every steep step over 25°: about 40 in the chapter. Between key anchors the change-over happens by
  itself with a click (a full "every anchor by hand" mode is a setting; open question Q3). A missed key clip leaves you
  unroped until the next one.
- The fixed rope of leg 2 should start at the crampon point, not 16 m from camp (real ropes start at the Icefall's foot).

### 6.4 Ladders, seracs, crevasses

| Danger | Today | Chapter 5 adds |
|---|---|---|
| Ladders (6) | ×0.45 speed; a fall kills; sway is waiting for a rig layer [Built] | ladder balance layer (section 7.2); Nima at the far end of 1 and 5; ladder 5 sags in the middle |
| Serac zones (2) | 30 s linger → 5 s warning → collapse; clock ×0.7 before 08:00, ×1.7 from 11:00 to 18:00 [Built] | the queue zone (no collapse); the scripted near-miss |
| Hidden crevasses (6 on leg 3) | deadly unroped, harmless roped [Built] | `hc_own`, `hc_cave`; the sag lesson |
| Snow bridge | — | one, at ladder 2 [To build] |

**Start time and the serac clock.** Default start 03:00: everything up to the collapse happens before 08:30 (×0.7: the
safe choice). With `start_late` (06:00): serac zone 2 at about 10:00 (×1), the collapse at about 11:05 (×1.7, so it falls
**ahead**), sun in the upper Icefall, Camp 1 at about 12:30. There is no queue at ladder 2 (the big team went at 03:00).

### 6.5 Weather and realms

The realm wheel is **held** (`ygg.Realm.Force none`) until the Football Field: the dark and the cold are enough. After
that the wheel may land on **harmless realms only** in this chapter (v1 §4.7 allows harmless and risky in Act II; the first
crossing stays fair while the player learns). Dr. Hale's forecast says "quiet". [Proposed]

### 6.6 Death and recovery

- Camps on the way: Base Camp, the Football Field (Icefall camp), Camp 1; trail checkpoint "Icefall - halfway" [Built].
- Deaths here: a fall from a ladder, a serac zone collapse, a hidden crevasse off the rope, the snow bridge.
- v1 §4.11 (restore the body as it was at the checkpoint, failure forward with Nima close) is [To build]; until then
  today's rule applies (full reset).
- The story must resume correctly after a death: beats already fired stay fired; the near-miss cannot fire twice
  [To verify in `YggStory`].

---

## 7. Camera

### 7.1 Budget for the chapter

| Item | Count | Where |
|---|---|---|
| Nudges | at most 2 (plan uses 1) | Pumori at the Football Field, only after companion and lure |
| Reflexes | up to 4 | the ice crack (B06), Ji-woo from behind-left (B13), the collapse (only if 30-120°), the snow bridge crack |
| Cuts behind closed eyes | 3 | the start, the late-start time skip, the end |
| Body takeovers | the loose-strap stumble, the collapse tremor, a crevasse fall (death) | |
| View locks | none (a crevasse fall uses Fall + Look leases, then death) | |
| Inspect | the photo (B18), the clue notes (B15, B22) | |
| Postures | lie, sit, crawl (wake); kneel1 (crampons, strap, cache, load); sit (Football Field) | as in `Chapter1_Camera_Spec.md` §4.3 |

### 7.2 New rig layers [To build]

| Layer | What the body does | Limits (starting values) |
|---|---|---|
| **Ladder balance** | the ladder flexes under the feet | lateral eye sway ±2 cm, yaw wobble ≤ 0.5°, roll ≤ 0.5°; irregular, 0.6-0.9 Hz (outside the 0.2-0.4 Hz sickness band); ×1.8 in the middle of ladder 5; ×(1 + wind) in gusts; scaled by "Head motion" |
| **Ground tremor** | the ice shakes when the serac falls | vertical 1-2 cm at 8-12 Hz for 1.2 s, decaying; no rotation; off when "Head motion: Off" |

The horizon stays level (camera rule). `CrevasseCrossing` must not write control rotation while the rig exists (it
already skips its old roll in that case [Built]).

---

## 8. Assets

### 8.1 What exists [Built]

| Need | Asset |
|---|---|
| Base Camp | camp from `place_camps.py` (tents, mess, kitchen, stores, altar, flag pole); Meshy `tent_dome`, `tent_mess`, `tent_kitchen`, `tent_toilet`, `barrel_blue`, `duffel`, `cook_set`, `stove_set`, `tarchen` (`/Game/Story/AI`); real printed prayer flags |
| The Icefall | serac scatter (`Icefall_S0x`, `M_SeracIce`, `M_IcefallIce`), detail tiles, crevasse crossings (`SM_IcefallLadder_Span3`, `SM_IcefallLadder_Section`, `SM_SnowPicket`, `M_Ladder_*`), fixed ropes, serac zones and red pickets, hidden crevasses (`M_HiddenCrevasseSag`), mountain rumble |
| Story places | the ice cave (`SM_IceCave`, glow, Fab oxygen tanks, Megascans rope coil, `SM_Food_Stove`, supplies), the black wood (`M_CarbonisedWood`), the checkpoint |
| Camps 1 and Icefall | tents, O2 bottles (`SM_O2_Bottle`, `SM_O2_Mask`), supplies (`SM_Ration`, `SM_Thermos`, `SM_Food_GasCanister`, `SM_Food_RationPouch`) |
| People | Meshy `sangye`, `nima_ai`, `marek_ai`, `elena_ai` (rigged); MetaHuman `jiwoo`, `porter_a` |
| Small props | `headlamp`, `water_bottle`, `daypack`, `glacier_glasses`, `khata_tied`, the compass (`SM_Compass2_Case`), the Icefall photo (`T_Photo_Icefall`) |

### 8.2 New: Meshy prompts (house style of `Tools/Meshy/assets.py`)

Each line is the `what` text; `PROP_STYLE` / `CHAR_STYLE` wrap it. Colour variants are made in Unreal. **Before spending
credits, look for free models on Fab / Poly Haven first** (director's rule: download quality assets first).

**Priority A (the chapter needs them):**

```python
dict(id="crampons", kind="prop", size=0.35, tris=8000,
     what="a pair of 12-point steel mountaineering crampons with two front points, black anti-balling plates and black nylon binding straps with buckles, scratched and dulled from use, lying side by side"),
dict(id="carabiner_sling", kind="prop", size=0.6, tris=5000,
     what="a climber's safety leash: a short sewn grey nylon sling with two steel locking carabiners at its ends and a mechanical rope ascender with a dark grey handle, scuffed and worn"),
dict(id="ice_anchor", kind="prop", size=0.35, tris=4000,
     what="a steel ice screw with a bent hanger plate and a short faded red nylon sling knotted to it, a steel carabiner clipped to the sling, rust spots and scratches"),
dict(id="climbing_helmet", kind="prop", size=0.3, tris=5000,
     what="a scuffed white mountaineering helmet with a black headlamp strapped around it, the plastic shell scratched and dirty, the chin strap hanging open"),
dict(id="nima_icefall", kind="char", size=1.68, tris=50000,
     what="a Sherpa mountain guide of about 45 from Nepal with a weathered sun-darkened face and short black hair with some grey, wearing a faded mustard-yellow hooded down jacket with charcoal shoulder panels, a climbing harness with two slings and carabiners at the waist, dark grey softshell trousers, black and dark red insulated double mountaineering boots with steel crampons, a scuffed white climbing helmet with a headlamp, glacier glasses pushed up on the helmet, down mittens hanging from wrist cords; real worn fabric, no logos"),
dict(id="sangye_icefall", kind="char", size=1.70, tris=50000,
     what="a Sherpa man of about 22 from Nepal with short black hair, wearing a red down jacket with a darker patch sewn on the left elbow, a climbing harness with two slings and carabiners, dark grey softshell trousers, black and orange insulated double mountaineering boots with steel crampons, thin black liner gloves; real worn fabric, no logos"),
dict(id="climber_icefall", kind="char", size=1.75, tris=45000,
     what="a mountaineer in a faded orange hooded down jacket and black insulated trousers, a climbing harness with slings and carabiners, a white helmet with a headlamp, a neck gaiter pulled over the nose and dark glacier goggles hiding the face, black insulated double mountaineering boots with steel crampons, big black down mittens; real worn fabric, no logos"),
```

**Priority B (nice to have):**

```python
dict(id="route_wand", kind="prop", size=1.2, tris=2000,
     what="a thin bamboo marker wand about 1.2 metres tall with a small faded red-orange nylon flag taped to its top, slightly bent, the lower end dirty with ice"),
dict(id="expedition_pack", kind="prop", size=0.8, tris=10000,
     what="a heavy 70-litre expedition rucksack in faded orange and black with a coiled blue climbing rope strapped under the lid, a closed-cell foam mat strapped on the side and an ice axe on the back, dusty and worn, standing upright"),
dict(id="cache_tin", kind="prop", size=0.4, tris=4000,
     what="a battered square steel storage box used as a mountain cache, chipped olive paint, frost in the seams, a strip of faded red tape across the lid"),
dict(id="juniper_sang", kind="prop", size=0.5, tris=6000,
     what="a heap of dried juniper branches smouldering on a flat stone ledge, grey ash and glowing embers among grey-green needles, a few grains of rice scattered around"),
```

**Credits** (props about 33, characters about 41 with rig): A = 4 × 33 + 3 × 41 = **255**; B = 4 × 33 = **132**;
both = **387**, about **445** with 15% for retries. The last note says **7 credits** are left (2026-11-20); buying more is
the director's money (open question Q8). Test three models first (house rule), then the rest in one batch.

Notes: `climber_icefall` is recoloured in Unreal for the big team (yellow), the team Sherpas and as a stand-in for any
client seen in the dark. The helmet and crampons can also be attached to the existing NPCs' head and foot sockets
(rigid props), which is cheaper than new outfits for Marek, Elena and Ji-woo.

### 8.3 Non-Meshy things to build

| Item | How |
|---|---|
| Juniper and kitchen smoke | a Niagara smoke system (none exists in the project yet); cheap, lit by the headlamp |
| The line of lights | moving sprites on the route ahead, at most 4 real shadowless lights |
| Spindrift burst | a Niagara snow cloud that overtakes the camera from behind (also usable for avalanches later) |
| Old boot track | a decal strip along the own line |
| Snow bridge | a HiddenCrevasse variant that breaks on a 2 s stand or a sprint; the ladder 2 slot extended sideways |
| Queue zone | `ASeracCollapseZone` with a "no collapse" switch and chip damage |
| Scripted collapse | `ygg.Serac.Drop <place> <count>` reusing the zone's block drop |
| Aged khata on the wood | `khata_tied` with a faded, torn material instance |

---

## 9. Sound needs (a list only: the director picks every sound by ear)

Existing sounds that fit [Built]: `Ice/Ice_Groan_01-04`, `Ice/Ice_Crack_01-08`, `Ice/Serac_Collapse_01-02`,
`Ice/Avalanche_01-05`, `Ice/Mountain_Boom_01-03`, `Footsteps/Step_Ice_*`, `Step_Snow_*`, `Step_Ladder_*`,
`Gear/Carabiner_Click_*`, `Carabiner_ClipIn_*`, `Rope_Tighten_*`, `Body/Breath_Phrase_*`, `Story/Story_Heartbeat`,
`Story/Story_Radio_Squelch`, `Story_Radio_Static_Loop`, `Story/Story_Stove_Loop`, `Weather/Wind_Calm`, `Wind_Gusty`.

New candidates to find or record:
1. A cheap digital watch alarm in a cold tent (diegetic, muffled by the sleeping bag)
2. Tent zip in frost; frozen fabric crackle
3. Kerosene stove roar and pot lid rattle in the kitchen tent; the cook humming
4. Juniper crackle; a pinch of rice falling on stone
5. Crampon buckles and straps; crampon points biting hard ice; points scraping aluminium rungs
6. **Ladder creak and flex** under weight (key for tension), and the hollow ring of a rung
7. Ice chips ticking on a helmet
8. A snow bridge settling ("whumpf") and breaking
9. A close serac collapse with debris rain and a snow hiss passing the ears (the peak; must carry fate, not action-film noise)
10. Distant climbers' voices in the dark (murmur, a laugh, "Rope free!"); Ji-woo's camera beep
11. Headlamp click
12. **The sunrise**: a deep, slow tone under the alpenglow at the Football Field, in the "fate" family (like the chosen C,
    E, H, K, L, M cues); never bright
13. The Cwm's silence: the ice noise fading out, then only breath and the creak of the pack
14. Melt water dripping inside the ice cave; the cave's soft resonance
15. Sun-warmed tent fabric ticking (Camp 1)
16. A dull pulse for the headache cue (very subtle)

Voices [VO]: Nima, Ji-woo, Marek, the cook, Dr. Hale, Sangye (two lines). All lines are in section 3.2 and are logged as
`Story [VO]` when played.

---

## 10. Robot-player test plan

Tools [Built]: the route robot `ygg.Bot.Route` (walks `route.json` legs, checks every 200 m, names blockers), the story
robot `ygg.Story.Auto 1` (answers every choice), the flight recorder (`Saved/Monitor`), `Tools/Bot/run_bot.ps1 -Fast
-YggCamRec`, `perf_run.ps1`. New [To build]: a `-Chapter chapter5` option for `run_bot.ps1`, and a choice script
(`ygg.Story.AutoPick <beat>=<index>`) so each run takes a chosen branch.

### 10.1 Before the chapter: the ground (route robot, legs 2-3)

Known blockers from the 2026-11-20 runs (must be fixed first; they belong to block A4):
- **Ladder 3** (Crevasse_2_leg2): simple collision 507 cm above the visible surface; the robot fell into the crevasse.
- **The Football Field approach** (leg 2, 2,200-2,290 m): blocked 5 times, stuck 3 times, 5 falls (the 49-52° ground).

Pass: 0 stuck teleports, 0 deaths, 0 "blocked", at most 1 short slide; legs 2-3 in 40-55 game minutes.

### 10.2 Story runs (branch coverage, pairwise)

| Run | D1 start | D2 gloves | D3 queue | D4 line | D5 load | D6 cave | Late collapse |
|---|---|---|---|---|---|---|---|
| 1 (default) | now | mitts | wait | rope | keep | skip | no |
| 2 | now | liners | back | own line | give | take | no |
| 3 | now | mitts | bridge | rope | give | take + pray | no |
| 4 | late | liners | (no queue) | rope | keep | leave | yes |
| 5 | late | mitts | (no queue) | own line | give | take | yes |
| 6 | now | liners | wait | rope | keep | take | forced late (`ygg.Story.Flag late`) |

### 10.3 Measured checks

1. **Completion:** every run reaches B24; the end state matches the hand-off table (day 5, Camp 1, stock, flags, Nima present).
2. **Length:** default run 65-80 min; no stretch over 6 min without a beat or decision (session monitor).
3. **Camera:** 0 rig incidents (spin, roll, low eye); nudges ≤ 2; every cut behind closed lids; eye ≥ 120 cm while walking
   above 0.6 m/s; ladder layer roll ≤ 0.5° and no energy in 0.2-0.4 Hz; tremor ≤ 1.5 s.
4. **Hazards:** queue wait → the groan at 40-46 s at about 04:20 and no collapse with NPCs inside; snow bridge holds a
   steady walk and breaks on a 3 s stand; collapse never within 20 m of the player, behind when on time, ahead + a
   walkable detour when late; `hc_own` and `hc_cave` kill only on the sag.
5. **Body:** SpO₂ 68-78 while climbing between 5,700 and 6,100 m; core ≥ 35.5 °C in mitts; liners → frostnip only before
   about 06:00; hydration at Camp 1 above 30 after the rest, lower on the own line; `ams_onset` set in the default run.
6. **People:** Nima never more than 45 m ahead on the rope; Ji-woo's pass happens in runs 1, 3, 6; no NPC stands inside
   another; NPCs do not slide (feet planted) on ladders.
7. **Performance:** ≥ 60 fps at 1080p High at the worst spot (the ladder 2 queue with 9 NPCs and the line of lights; the
   Football Field with the clients); no hitch over 100 ms with the eyes open (all spawns behind a blink or far away).
8. **Death recovery:** die on ladder 1, in serac zone 1 and in `hc_cave` → respawn at the right camp or checkpoint; the
   story resumes without replaying fired beats.

---

## 11. Build list (rough sizes in Claude working days, [estimate])

| # | Item | Size | Depends on |
|---|---|---|---|
| 1 | A4 ground fixes: ladder 3 collision, the Football Field ground (move the camp to flat ice or re-plan the approach) | 1 | — |
| 2 | Manual clipping at key anchors + glove swap + finger-cold meter | 2 | — |
| 3 | Route followers for the clients and the big team (v1 §4.10), with ladders and headlamps | 2 | 1 |
| 4 | Rig layers: ladder balance, ground tremor | 0.5 | — |
| 5 | Serac changes: queue zone switch, `ygg.Serac.Drop`, snow bridge, `hc_own`, `hc_cave`, detour rope | 1 | 1 |
| 6 | Story keys: `clock` trigger, hands items (cup, crampons, carabiner), `ygg.Feet.Crampons`, `ygg.Body.Gloves` | 0.5 | 2 |
| 7 | `build_chapter5_world.py` (places + staging checks) and `chapter5_story.py` (beats + the existing validator) | 1.5 | 3, 5, 6 |
| 8 | Niagara: smoke, spindrift burst; the line of lights; decals | 1 | — |
| 9 | Camp stock panel and load drop (v1 §4.4, minimal) | 1 | — |
| 10 | Meshy round (A, then B), import, sockets | 1 | credits |
| 11 | Robot runs, fixes, review shots for the director | 1.5 | all |
| | **Total** | **about 13** | |

Heat, fatigue, the AMS model and trust are shared systems from v1 §4; this chapter only needs their first, simple
versions (a flag and a cue each).

---

## 12. Risks, cultural check and open questions

### 12.1 Risks

| Risk | Likelihood | Effect | Mitigation |
|---|---|---|---|
| The Football Field sits on 49-52° ground; the route robot gets stuck there | high (measured) | the rest scene cannot be played | fix in A4 first: move Landmark_3 to the flattest ice within 150 m, or re-plan the approach with `plan_route.py` |
| Ladder 3 collision kills | high (measured) | deaths that are not the player's fault | fix the ladder's simple collision (A4) |
| Clip fatigue | medium | boredom | key anchors only (about 40); full mode as a setting |
| 27 minutes of night play is too dark on a laptop | medium | murk, lost players | stars, a waning moon, snow albedo, the line of lights, a stronger headlamp than today's 35 lm |
| NPCs crossing ladders look wrong | medium | breaks belief close up | keep NPC crossings far or brief; Nima crosses before the player watches |
| Performance with 9-12 NPCs among seracs | medium | under 60 fps | far NPCs as low LOD figures; sprites for most lamps; measure (check 7) |
| Scope (about 13 days) against the interim (17-18 Dec) | medium | thesis time lost | items 1, 2, 4, 7 serve demo route 1 (A4) anyway; the rest can follow |
| Story continuity with chapters 2-4 and 6 written in parallel | medium | flags and names do not match | the hand-off table (section 1) and the flag list below; one alignment pass |

**Flags this chapter reads** (names to align with chapters 1-4): `tea`, `mala`, `read_photo`, `elena_push` (Ch1, exist);
Chapter 4: the load contents, the pack split, what was blessed, Nima's trust (names [To agree]).
**Flags it writes:** `go_now`/`start_late`, `mitts`/`liners`, `queue_wait`/`queue_back`/`queue_bridge`, `own_line`,
`rope_given`/`rope_kept`, `touched_wood`, `ask_was`/`ask_am`/`ask_none`, `late`, `collapse_seen`,
`cave_taken`/`cave_prayer`/`cave_left`, `frostnip`, `ams_onset`.

### 12.2 Cultural check list (for a Khumbu Sherpa reader)

1. Juniper smoke offering (*sang*, with juniper, *shukpa*) at the Base Camp altar before entering the Icefall: done each
   morning, and by whom? The altar's name (often written *lhap-so*) [To verify].
2. Walking round the altar clockwise; Nima's correction.
3. Nima murmuring *Om mani padme hum*, and "We ask her to let us pass" (the mountain goddess Jomo Miyolangsangma).
4. The cook's *"Chiya, dai"*: Nepali or Sherpa words between Sherpas at Base Camp.
5. Taking a cache that belonged to a climber who died: is it fine, unlucky, or a matter for a prayer? (D6)
6. The black wood as a hint of the Wish-Fulfilling Tree (in Wheel of Life paintings its roots are in the demigods' realm
   and its fruit in the gods' realm) [To verify]: is this a respectful use?
7. Pumori = "unmarried daughter" in Sherpa; the Western Cwm as "the Valley of Silence".
8. **The serac collapse.** In April 2014 an ice avalanche in the Khumbu Icefall killed sixteen Nepali mountain workers,
   most of them Sherpas. Our near-miss must not echo that day: no deaths, no named team, no date. Ask the reader whether
   the scene is acceptable as written.
9. An old khata tied to a natural feature in the Icefall: plausible?
10. Whether Sherpas would rebuke loud behaviour in the Icefall (Ji-woo).

### 12.3 Open questions for the director

| # | Question | Default if no answer |
|---|---|---|
| Q1 | Are Marek, Elena and Ji-woo **our clients** (Chomolungma, Ch1 v2) or **the other team** (v1)? | our clients |
| Q2 | Chapter number: 5 (17-chapter v1) or 3 (12-chapter Chomolungma list)? | keep 5 until you choose |
| Q3 | Clipping: key anchors only (about 40), or every anchor by hand? | key anchors; "every anchor" as a setting |
| Q4 | Allow the 06:00 start (D1)? | yes |
| Q5 | Let the own line skip the Football Field rest? | yes, it is the cost of the shortcut |
| Q6 | The cache as a dead team's cache (moral weight), or a neutral old cache? | dead team's, after the cultural check |
| Q7 | The black wood as the Wish-Fulfilling Tree's roots? | yes, never explained in this chapter |
| Q8 | Meshy credits: A only (255), or A + B (387)? About 445 with retries. 7 credits are left | wait |
| Q9 | Can the near-miss ever kill? | never |
| Q10 | Ladder balance strength (roll ≤ 0.5°, sway ±2 cm)? | as written; you judge in play |
| Q11 | Does Sangye carry his father's ice axe (if Chapter 4 blesses it)? | yes, if Chapter 4 does |

### 12.4 Text to rewrite in the level (the Norse layer is gone)

- **The black wood** (`Clue_WoodFibre`, [Built] text keeps its first paragraph): end it with *"No tree ever grew this
  high. No tree was ever this big."* Nothing else; Nima's line carries the rest.
- **The cave note** (`Clue_IceCaveCache`): replace the "Korean expedition, spring '06" and "runes" text with:
  *"A steel box, the lid frozen shut. A strip of red tape on it: the colour of the tents of the team that went home after
  last week's accident. Inside: two rations, a bottle of water, an oxygen bottle."* (No real expedition, nationality or
  year.)

### 12.5 What changed from `Chapter_Design_v1.md` and why

| v1 said | This design | Why |
|---|---|---|
| The ice cave at minute 35, before serac zone 2 | after ladder 6, at the top (minute 58) | measured: the cave is 50 m off leg 3 at 1,875 m |
| Own line: a serac corridor saving about 8 min | the old track past ladder 3, skipping the Football Field: saves about 480 m, 5-6 min | measured from `route.json`; the corridor is not on the map |
| Traffic: wait inside the serac zone | a new no-collapse queue zone at ladder 2 | the placement rule keeps every serac zone at least 52 m from a ladder; a collapse onto a queue of people would be a mass death |
| The cave's runes, the Korean 2006 cache | rewritten without runes or real teams | the Chomolungma direction; no real expeditions |
| Nima: "My father said the mountain has bones" | "Old people say Chomolungma has a tree inside her" / "Your father touched it" | the Chomolungma story: the father is Sangye's |
| The other team | our clients (Q1) | `Chapter1_v2_Design.md` |
| Sunrise on Nuptse | Pumori behind first, then Nuptse | the look back (art direction); first light hits peaks to the west [To verify in our sky] |
| 07:00 radio not specified | news and weather only | the first remote medical check stays in Chapter 6 |

---

## Appendix: source documents

| Document | Used for |
|---|---|
| `Docs/Development/Chapter_Design_v1.md` §1-5 | the chapter, hand-offs, systems, cast, pacing rules |
| `Docs/Development/Chapter1_v2_Design.md` | format, cast (Chomolungma), flags from Chapter 1 |
| `Docs/Development/Chomolungma_Chapter1.md` Part 1 | the 12-chapter list |
| `Docs/Development/Chapter1_Camera_Spec.md` §4.3, 4.4, 6, 7 | postures, leases, nudge, reflex, story keys, validator |
| `Tools/RoutePlanning/route.json`, `place_crevasses.py`, `place_hazards.py`, `place_slice_content.py`, `place_ice_cave.py`, `place_camps.py` | every position and number in section 4 |
| `Saved/DetailTiles/route/props_now.json` | actor positions in the level |
| `Saved/Bot/route_20261120_104624_L1-3`, `route_20261120_110212_L2-3` | the route robot's times and the known blockers |
| `Source/YggdrasilsPeak/.../SeracCollapseZone.h/.cpp`, `CrevasseCrossing.cpp` | serac timings, ladder speed and sway |
| `Tools/Story/chapter1_story.py` | story machine keys |
| `Tools/Meshy/assets.py` | prompt house style, credit prices |
| Memory notes: chomolungma-direction, climb-design-decisions, story-machine-chapter1, terrain-georeference, art-direction, feedback-audio-identity-fate, meshy-pipeline, nine-realms-planets | direction, rules, tools |
