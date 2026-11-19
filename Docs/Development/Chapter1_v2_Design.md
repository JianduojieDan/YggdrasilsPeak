# Chomolungma · Chapter 1 "Day One" · Design v2

Version 2.0 · 2026-11-19 · written by Claude (designer and technical lead) for the director (the author) and his
thesis supervisor. It replaces the chapter design in `Chomolungma_Chapter1.md` Part 2 (v1, 2026-11-14). The other
chapters in that file are unchanged.

**How to read this.** Sections 1-3 are the chapter itself. Sections 4-7 are how it is made. Section 8 says what changed
since v1 and why. Sections 9-10 are where we stand and what needs a decision.

**Labels used below**

| Label | Meaning |
|---|---|
| **[Built]** | exists in the project today (may not yet be judged by the director) |
| **[Written]** | code or data is written but not yet built, run and measured |
| **[To build]** | planned, not started |
| **[Proposed]** | a new idea in this document; the director decides |
| **[VO]** | a spoken line; the director records the voices himself |
| **[To verify]** | a fact we believe but have not checked against a good source |

**Words used often**

| Word | Plain meaning |
|---|---|
| Beat | one small story step: "when X happens, do Y" |
| Story machine | our code that plays the beats from a data file |
| Rig | the new camera code: the only thing allowed to move the player's view |
| MetaHuman | Epic's tool for realistic human faces and bodies |
| Meshy | a paid AI service that turns a text prompt and an image into a textured 3D model |
| SpO₂ | blood oxygen saturation, in percent; the number on a finger oximeter |
| Frame generation | an NVIDIA feature that invents extra frames between real ones to look smoother |

---

## 1. One-page summary

### What the chapter is

A young Sherpa, **Sangye**, leaves the expedition camp at **Dingboche (4,410 m)** on his first day as a climbing Sherpa
for a foreign team. His father died high on Everest last season and his body is still up there. By night Sangye has
walked the real trail to the **Thukla memorials (about 4,830 m)**, tied a white scarf to his father's new memorial
stone, seen the summit plume turn gold for a few seconds, and heard the older Sherpa **Nima** say in the dark: *"Your
father did not fall."*

Everything is played in first person, on the real 1:1 terrain, built from a real elevation model. There is one
cinematic shot in the whole chapter.

### The feeling

**Quiet awe, with grief underneath and a mystery on top.** The art direction calls it "quiet awe": first *"I could stand
here"*, then *"what am I standing in front of?"*. The body carries the emotion: breath, heartbeat, cold hands, a heavy
pack. The myth enters only once, quietly, and only Sangye sees it.

### The shape

| Part | Minutes | Tension (1-5) | Company |
|---|---|---|---|
| Prologue: the Balcony at 8,400 m, night | 0-4 | 5 | alone, and the father |
| Dawn in Sangye's tent | 4-11 | 2 | alone |
| The farewell (Dolma, Aunt Pema) | 11-17 | 1 | family |
| The job (Nima, the team, the load) | 17-22 | 2 | the team |
| The trail up the valley | 22-60 | 2-4 | Nima and the team, then alone for a stretch |
| The memorials at dusk: **the peak** | 60-68 | 4 | Nima |
| Night in the tent: **the end** | 68-75 | 3 → 5 | Nima, the radio |

### Length

**About 70-80 minutes.** v1 said 65-75. The world builder now places the memorials **5.8 km** up the trail (v1 assumed
about 5 km). At today's walking speed (2.3 m/s) that is about 42 minutes of pure walking, plus the scenes. The robot
player (section 7) will time a full run before the director plays.

### How it serves the thesis

The thesis topic is a **physiology model** for high-altitude climbing, checked with a "virtual climber" (supervisor,
2026-10-26). Chapter 1 is where a player first *feels* that model: SpO₂ on the radio check, breath that grows loud on a
climb, the rest-step, water, a pack that slows you, a client with altitude sickness. Every session also records its
data through the flight recorder (section 7). The chapter is the story wrapper; the physiology is the core.

---

## 2. Design pillars and the research behind them

### 2.1 Five pillars

| # | Pillar | In one line | What it rules out |
|---|---|---|---|
| P1 | **Eyes on a body** | The camera is Sangye's eyes, and every move of the view is explained by his body | camera grabs, spinning views, floating eyes at knee height, hard cuts in the present |
| P2 | **Teach in the moment, by a person** | One new thing per scene, taught when it is needed, by Nima or the radio | tutorial screens, two mechanics at once |
| P3 | **Spaces and objects tell the story** | The duffel, the photos, the stone explain the father. Nobody lectures | exposition dumps |
| P4 | **Real place, real people, respectfully** | The real trail, real gear, real Sherpa life; fictional characters only; a living religion treated with care | fantasy costumes, real names on memorials, unsourced words on screen |
| P5 | **Tension over poetry** | Survival numbers and choices matter more than beauty alone; no stretch without a decision, event or discovery | long empty walks |

P5 comes from the director (2026-11-14): walking the route with nothing to decide is boring, and the survival indexes
and decisions are the most important part.

### 2.2 The research, turned into rules

These are the findings cited in `Chapter1_Design.md` §8 and `research_notes/AI tools for cinematic chapters/`. The
right-hand column is how Chapter 1 v2 obeys them.

| # | What the research says | Source | Rule in v2 |
|---|---|---|---|
| 1 | The first minutes decide whether players keep playing | Google Play developer analysis (industry data) | Control within about 5 seconds of the eyes opening; the real danger is felt in the prologue |
| 2 | Tutorials barely help; help at the moment of need does | Andersen et al., CHI 2012 (peer-reviewed, about 45,000 players) | No tutorial screens. One short line at the moment of need, spoken by a person where possible |
| 3 | Teach without words | Miyamoto on Super Mario World 1-1 | The trail teaches: a steep step teaches breathing; a narrow trail teaches the "mountain side" rule |
| 4 | One thing at a time | cognitive-load writing on onboarding (GDevelop, Wayline) | One new mechanic per scene |
| 5 | Introduce, develop, twist, conclude | Nintendo's four steps (Koichi Hayashida, kishōtenketsu) | Each mechanic follows the ladder in §3.3 |
| 6 | Start in the middle, then show normal life | in medias res (Film Stories, Game Developer) | Summit-night prologue, then "Day 1" at dawn |
| 7 | Spaces tell stories | theme-park design (Carson, via Game Developer); narrative architectures (Jenkins) | The duffel, the photos and the stone carry the father's story |
| 8 | A mechanic can build a bond | *Ico* (Fumito Ueda: holding a hand) | Dolma holds your hand with a held key (F) |
| 9 | People remember the peak and the end | peak-end rule (Kahneman, applied by practitioners) | One designed peak (stone and plume at dusk), one designed end (Nima's line in the dark) |
| 10 | Open loops pull | Zeigarnik effect (Psychology of Games) | Four questions left open; "Day 1 / 49" always visible on the journal |
| 11 | Autonomy, competence, relatedness keep people playing | Ryan, Rigby & Przybylski 2006 (peer-reviewed) | Real choices, a first mastery (the rest-step), people who care |
| 12 | Cinematic without cutscenes: look triggers with a timer, sound before sight, light as a guide, staged distances, exposition in pieces | Valve, *Half-Life: Alyx* developer commentary | The whole camera language (§4) |
| 13 | Dialogue while the player keeps control; silence is an answer | *Firewatch* (GDC 2017, Ewing); *Oxenfree* (Hines, Krankel) | Walk-and-talk with Nima; choices appear while you walk; "Say nothing" is always offered |
| 14 | Natural guides instead of markers | *Ghost of Tsushima* Guiding Wind (Rockenbeck, GDC 2021) | The father's compass, sounds, flags and light guide you; no waypoint marker |
| 15 | In climbing games the body carries the emotion; drama can come from systems | *Cairn* (Emeric Thoa, via Game Developer) | Breath, heartbeat, shaking hands, frost; Elena's altitude sickness comes from the same body systems |
| 16 | Flow: challenge matched to skill | Jenova Chen, *Flow in Games* | The trail asks only for what has been taught |

**How strong the evidence is.** Andersen et al. and the self-determination papers are peer-reviewed. The Google Play
analysis is industry data. Nintendo, *Ico*, *Alyx*, *Firewatch* and flow are designers' practice from talks and
interviews. The peak-end rule is well established in psychology but only applied to games by practitioners.

**One honest gap.** The camera spec keeps steady camera sway out of the 0.2-0.4 Hz band, where visually induced motion
sickness is said to peak. The research note found **no primary source** for comfort limits of camera nudges or FOV
changes on a flat screen (the *Alyx* comfort notes are about VR). So the camera limits are careful starting values,
tested by measurement and by the director's own play, not proven numbers.

### 2.3 Realism sources (people, gear, places)

From `Chapter1_Realism_Spec.md` and its research notes. Every look choice there is tagged as sourced, seen in one dated
photo, plausible, estimated, or a design choice.

- Gear on the trek: IMG autumn Everest Base Camp gear list (medium hiking boots on the trek, double boots only higher);
  Madison Mountaineering 2023 Everest gear list (trek, camp and summit boots).
- Porters: Kathmandu Post, December 2025 ("thin clothing and poor shoes, including canvas shoes"); a dated photo,
  Pheriche, 7 November 2022 (Wikimedia Commons).
- Sherpa men's everyday dress: Wellcome Collection 1986 Khumbu photo set; Gale 2008; Onlinekhabar on the Nepali brand
  Everest Outfit and Thamel copies.
- Dated reference photos: Pangboche woman 2009, Phurte woman 2009, Lobuche Sherpa guide 2007, Namche homestay kitchen
  2009 (all Wikimedia Commons).
- Skin and faces: Singh 2017 (PMC) on sun damage patterns; Nepal Journal of Ophthalmology on pterygium in Upper Mustang.

### 2.4 Why a player keeps going after Chapter 1

1. **A mystery with a face.** The father speaks in the first minute: *"You came too early."*
2. **A secret held by someone you like.** Nima knows how the father died.
3. **A ticking structure.** "Day 1 / 49" on the journal page.
4. **A glimpse of the myth.** The gold plume, seen only by Sangye.
5. **Your choices come back.** The journal writes its first page from what *you* did.
6. **Something no other game has:** the real Everest, on foot, through the eyes of the people who make every climb
   possible.

---

## 3. The player journey, minute by minute

### 3.1 The journey

Minutes are targets. The trail minutes assume 2.3 m/s walking; the robot player will measure the real ones.

| Min | Scene | What the player **does** | **Sees** | **Feels** | **Learns** |
|---|---|---|---|---|---|
| 0-1 | **Prologue** · black, wind, a fast heartbeat; the eyes open on a frozen rope at 8,400 m, 02:30 | walks slowly along the rope (pace held at 0.5 m/s) | headlamp beam, rope lower left, snow wall right, stars | fear, cold | look and walk; the body is slow up here |
| 1-3 | A man sits in the snow ahead, no oxygen mask. The headlamp finds him | looks at him (or a whisper pulls the eye after 7 s) | the father turns his head. **[VO] Father:** *"You came too early."* | awe, a mystery | (nothing to learn: a hook) |
| 3 | **The slip** · the ice cracks 0.3 s before anything moves | nothing: 1.5 s of falling, a short slow-motion middle | snow, the drop, then the eyelids slam shut | shock | |
| 3-4 | **The fly-out**, the only cinematic: out of the body, away into cloud | watches | the moonlit pyramid and its plume; title **CHOMOLUNGMA**, "Goddess Mother of the World" | awe | |
| 4-6 | **Dawn in the tent**, 06:20 · eyes open lying in a sleeping bag | first key press sits him up | light through the tent fabric, the door glowing on the left, frost on the wall, the duffel at his feet; "Day 1 / 49 · Dingboche, 4,410 m" | quiet, grief | look around |
| 6-10 | **The father's duffel** (optional) | [E] open; turns two photos in his hands with the mouse | his red jacket, prayer beads, 2019 summit certificate; a photo of the father and Nima at Base Camp; on the back of the Icefall photo: *"Next time with Sangye. Not before he is ready."* | grief, curiosity | inspect: look closer, turn things over |
| 10-11 | **Dolma calls** from outside: *"Sangye-dai! They are waiting for you!"* | holds W to crawl out through the door; stands up | standing up **is** the reveal: the camp, frozen ground, breath fog, prayer flags | lift, cold air | crawl, stand |
| 11-17 | **The farewell** · Dolma runs up | holds F to hold her hand; she leads him to the camp edge | Aunt Pema: **[VO]** *"Promise me you will come down."* She ties the white khata round his neck; he looks down and sees it on his red jacket. Dolma pushes a drawing into his pocket | love, guilt, a promise | hold a hand; a choice (I promise / I'll bring him home / say nothing) |
| 17-22 | **The job** · Nima: **[VO]** *"Your load. Fifteen kilos. Carry what you can carry tomorrow too."* | picks one thing to add (butter tea, second bottle, first-aid kit, father's prayer beads); hears the radio check | Marek briefs the team; Elena jokes about the cold; Ji-woo films; Dr. Hale on the radio: Sangye 88, Elena 82. Nima gives him the father's brass compass | duty, nerves, belonging | the pack and its weight; SpO₂ and the radio; the compass ([Q] hold it up) |
| 22-27 | **The first climb** out of camp | climbs; his breath grows loud | Nima at his right shoulder: **[VO]** *"Slow is fast up here. Step, breathe, step."* | effort, then control | breathing and the rest-step (slowing down visibly calms the body) |
| 27-31 | **The stream** (heard first) | [E] kneels at the ice hole, fills the bottle | dark water under clear ice; *"The water is so cold it hurts your teeth."* | small satisfaction | water: *"Drink before you are thirsty."* |
| 31-35 | **Elena stops**, hands on her knees (her laboured breath pulls the eye) | chooses: carry part of her load (costs stamina), give her water (costs water), or tell her to keep going | a first-timer with a headache: altitude sickness | care, cost | other people's bodies matter too; choices have a price |
| 35-37 | **Walk-and-talk** · Nima: *"Why did you take this job?"* | answers while walking: for the money / for him / silence | Nima answers each differently | trust begins | silence is also an answer |
| 37-43 | **Porters coming down** · bells and whistles first | chooses: step to the mountain side, or squeeze past on the outside (a load knocks him to one knee; hurt knee, slower for 60 s) | two porters with roped loads and head straps on a narrow trail | a scare, or patience | the rule "always the mountain side", learned by living it |
| 43-51 | **Alone in the cloud** · Nima stays with Elena: *"Follow your father's compass, we will catch up."* | walks alone, compass held up | the afternoon cloud swallows the valley; the view closes in by 2° | solitude, focus | the compass as a guide |
| 51-52 | **Together again** · Nima's voice from behind first | turns to him | *"There you are. Good. The pass is above us."* | relief | |
| 52-60 | **The last climb to the pass** | walks up as the light goes | the shadow climbs the valley from the bottom; the far summits stay warm longest | tired, expectant | (a gap: see §3.4) |
| 60-62 | **Dusk at the memorials** · the flags are heard before they are seen | turns the bend | dozens of stone chortens hung with prayer flags, backlit by the last sun | awe, grief | |
| 62-65 | **The father's stone** · Nima: *"This way."* | [E] kneels; holds F to tie the khata | a new stone, sharp edges, his name | grief | (the hand-hold input returns, now alone) |
| 65-68 | **The gold plume** | looks up the ridge (the compass needle trembles that way in his hand) | the summit plume turns gold for 11 seconds; the heartbeat rises; a distant boom. Nima: *"Did you hear that?"* ... *"No. Only the wind."* | wonder, unease | (the myth is real, and private) |
| 68-75 | **Night in the tent**, 20:40 · a blink at the tent door | sits facing Nima; listens | only Nima's headlamp and the stove glow. Radio: Elena's number depends on minute 31. **[VO] Nima:** *"People say he fell."* ... *"Your father did not fall."* He clicks his light off. Darkness | warmth, then a shock | |
| 75 | **The journal** | reads | the first page writes itself from his choices; *Day 1 / 49*; title **CHOMOLUNGMA · Chapter 1 · Day One** | | |

### 3.2 The four open questions it leaves

What did the father mean by "too early"? How did he really die? What is the golden light? 48 days remain.

### 3.3 The teaching ladder

One mechanic per scene, each in four steps (introduce, develop, twist, conclude). The "line" is the only instruction
text, spoken where possible, shown once as a small caption.

| Mechanic | Introduce | Develop | Twist | Conclude | Taught by |
|---|---|---|---|---|---|
| Look and walk | the rope, still | walk to the father | the slip | wake in the tent | the scene itself |
| Inspect | open the duffel | turn a photo | writing on the back | pocket it | caption "[E] Open" |
| Crawl and stand | sitting in the tent | crawl to the light | stand: the camp appears | walk | hint only after 15 s idle |
| Hold a hand | Dolma takes your hand | she leads you | she stops and won't let go | she lets go when called | Dolma: *"Hold tight, Sangye-dai."* |
| The pack | the 15 kg company load | add one thing | the weight shows on the HUD and in the walk | close it | Nima |
| SpO₂ and the radio | the morning check | your number, 88 | Elena's 82, laughed off | the evening check reflects your choice | Dr. Hale on the radio |
| Breathing, rest-step | breath grows loud on the climb | the line comes at the loudest moment | sprinting empties you | at the top the breath calms | Nima |
| Water | bottle half empty | the stream | kneel on the ice edge | bottle full | Nima |
| The compass | Nima gives it | it points north as you walk | in the cloud it is your only guide | at the stone it trembles toward the summit | Nima: *"He never went anywhere without it."* |
| A choice while walking | Nima asks | the choice waits a few seconds | silence is an answer | he reacts to what you said | the choice appears on screen |

**Not taught in Chapter 1:** ropes and ladders (Chapter 2), bottled oxygen (Chapter 10), the realms (Chapter 2). The trek
has no fixed ropes, as in reality.

### 3.4 Pacing check (rule: no stretch over 6 minutes without a decision, an event or a discovery)

Event positions along the trail come from the world builder. Minutes at 2.3 m/s.

| From | To | Distance | Walking time | OK? |
|---|---|---|---|---|
| camp | first climb | 220 m | 1.6 min | yes |
| first climb | stream | 680 m | 4.9 min | yes |
| stream | Elena | 800 m | 5.8 min | just |
| walk-and-talk | porters | about 800 m | 5.8 min | just |
| porters | the cloud | 700 m | 5.1 min | yes |
| the cloud (alone) | together | 950 m | 6.9 min | yes: solitude is the event |
| together | flags seen | **1,150 m** | **8.3 min** | **no** |

**[Proposed]** fill the last gap with one small, real moment, for example passing a **mani wall** (stones carved with a
prayer) and keeping it on your right, as Buddhists do, with Nima's one-word correction if you pass it on the wrong side.
It teaches respect without a screen, and the mani stones are already on the asset list (P3). The carving must be checked
by a Tibetan reader before release.

---

## 4. The camera language ("Eyes on a Body")

The full technical spec is `Chapter1_Camera_Spec.md` (2026-11-19). It scored three competing designs and combined the
best parts. This section says it in plain words.

### 4.1 The idea

The camera is Sangye's eyes, sitting on a real body. The body explains every change of view the player did not make
himself: a step, a breath, a landing, sitting down, a reflex to a loud sound, a small girl pulling his hand, a slip. If
nothing in the body explains a move, the move does not happen.

### 4.2 Ten rules

1. **One writer.** Only the new camera rig moves the view. The story *asks* the rig; it never turns the head itself.
2. **No spinning.** Angles are always taken the short way round. Scripted shots are aimed at a point, never blended
   between two angles. (Blending angles the long way was the cause of the 360° vertical spin.)
3. **The body explains every move** (see above).
4. **The eye is only low when the body is low.** Lying, sitting, kneeling and crawling are real states. You cannot walk
   while you are low; moving stands you up first.
5. **Gentle speed limits** for anything the story does: a nudge turns at most 20°/s sideways and 8°/s up or down.
6. **Everything the story holds, it gives back.** Holding still, inspecting an object, the slip: each has an owner and a
   time limit, and is released automatically. A watchdog puts the camera back to normal walking if anything is left
   over. (In the code these are called "leases".)
7. **Cuts only with the eyes closed.** Every jump in place or time happens behind closed eyelids or the title's black.
8. **The view is almost never locked.** Only during the slip and the fly-out.
9. **Pulling the gaze, in this order:** staging (people walk into your view), sound, a companion turning his head,
   light, and only then a slow nudge that gives way to the mouse. At most **3 nudges** in the whole chapter, at least
   60 s apart, each only after 12 s of not looking.
10. **One cinematic only:** the prologue slip turns into the out-of-body fly-out. Everything else is played.

### 4.3 What the body does to the view

| Layer | Plain description |
|---|---|
| Height | eye at 156 cm standing; it follows the ground smoothly over rocks and steps |
| Steps | a small bob, timed to the footstep sound; about 40% of real head motion, so it reads on a laptop screen |
| Rest-step | below 0.45 m/s the bob becomes a dip-and-hold: slowing down visibly calms the view |
| Breathing | grows with effort and altitude; at 8,400 m you are always panting |
| Exhaustion | the head hangs a little and sways irregularly; never a steady roll |
| Cold | a fine shiver, strongest in the hands |
| Heart | above 7,000 m a tiny heartbeat push of the field of view; the 2.5° push is used once, at the plume |
| Pack | a heavy pack stoops the eye a few centimetres |
| Slope | uphill leans the eye forward; the horizon always stays level |
| Looking down | you see your boots from about -40°, the red jacket and the khata from about -55° |

### 4.4 Postures

| Posture | Eye height | Where it is used |
|---|---|---|
| Standing | 156 cm | almost everywhere |
| Lying | 32 cm | waking in the tent |
| Sitting | 78 cm | the tent at dawn, the night tent |
| Crawling | 68 cm | out of the tent door |
| Kneeling on one knee | 108 cm | the stream; a knock from a porter's load |
| Kneeling | 110 cm | the father's stone |

Each posture has soft look limits, so you never hit an invisible wall. For now the body is lowered without special
animations; proper sit, kneel and crawl animations come later (phase 2).

### 4.5 The chapter's camera budget

| Item | Count |
|---|---|
| Nudges | at most 3 (father, flags, plume), only as a fallback |
| Reflexes (a 10° turn toward a sudden sound; the player finishes the turn) | 4 |
| Jumps in place, all behind closed eyes | 3 (prologue, dawn, night) |
| Body takeovers | the slip; the porter knock (only on the risky choice) |
| Cinematic shots | 1 (the fly-out) |
| View locks | the slip and the fly-out only |

v1 as played had 9 gaze grabs, 4 teleports (one a hard 326 m cut), 2 tumbles and 3 fake kneels.

### 4.6 Comfort and screen

- New settings: **Head motion** Full / Reduced / Off; **Story camera** Guided / Gentle (no nudges, and the slip becomes a
  plain eyelid close); **Heartbeat zoom** On / Off; a **comfort dot**; field of view 80-110.
- Mouse is 1:1: no smoothing, no lag.
- The screen no longer darkens when you look down at the snow: the vignette is lighter and body effects only change the
  image while they are active.
- **Render defaults [Director decides]:** frame generation "Auto" = 2x (3x or 4x only when the real frame rate stays at
  55 fps or more); motion blur off. The play-tests ran 4x frame generation on about 29-30 real frames per second
  (sessions 20261119_203955 and 20261119_222446), which is the most likely cause of the smearing.

---

## 5. Places and art direction

The art direction (the director's own document, 2026-11-03) is the yardstick: **quiet awe**, attention moving between
the few metres at your feet and the far view, a sky that carries the light, crisp ridge silhouettes, blue snow in shade,
warm light only on the high ridges, the climber small in a big world.

### 5.1 The expedition camp at Dingboche

**[Built]** today, rebuilt 2026-11-19 with Meshy models:
- a gentle **crescent of two-person dome tents**, about 3.9 m apart, all doors to the trail; the last one is Sangye's
- the **mess dome** and the **kitchen tent** side by side behind them
- blue barrels and dried yak-dung fuel behind the kitchen, a pile of duffels at the mess door, porter loads ready at the
  row's end by the trail, the toilet tent at the back edge
- a prayer pole in front of the camp, with flag lines to the mess tent and to two poles at the ends of the row

**Still wrong (director, 2026-11-19):** the camp is not pretty enough; the flags float and look fake; the mess-dome
windows have jagged edges; the tents are scaled up (x1.5, and Sangye's x1.8) so the first-person body fits inside,
which makes them larger than real expedition tents.

**[Proposed] Camp redesign rules:**
1. Real sizes first. A two-person dome is about 2.2 × 1.4 m and 1.1 m tall. Solve the "body pokes through" problem in
   the camera (the seated posture and soft look limits) before scaling a tent.
2. The camp is lived in: boots in vestibules, a pot steaming at the kitchen door, a cook singing, washing on a guy line,
   stones on the tent skirts against the wind, a path worn between tents.
3. Composition from the tent door: when Sangye stands up, the frame holds the crescent, Dolma running in, the flags
   backlit, and a far white summit. The world builder checks it as it already checks other key frames.
4. One warm colour accent per view (an orange tent, a red duffel) against blue shade.

### 5.2 Sangye's tent (the opening)

- He wakes lying, eye 32 cm above the floor, head on a stuff sack. The door glows 30-40° to the left; the light comes
  through the fabric; the frosted inner wall is on the right; the duffel lies past his feet, lower right. The world
  builder checks these angles.
- Standing up outside is the reveal of the camp.
- **[To build]** a proper tent interior (inner tent, mats, sleeping bags). The Meshy domes have no inside yet.

### 5.3 The trail

The real trail from Dingboche up the valley to the Thukla pass, on our 1:1 terrain: river sounds, a frozen stream, a
narrow stretch with a drop on one side where the porters pass, and the afternoon cloud that rises up the valley.

### 5.4 The Thukla memorials (the peak)

- **[Built]** Meshy chortens in three kinds (a 2 m stepped chorten with a plaque, a 1.2 m cairn with a flag stick, a
  3 m chorten with a pole), grouped like the real memorials: a dense field on the father's side of the trail, framing his
  stone, thinning out up the ridge, a few across the trail, flag lines between them.
- **The father's stone:** a new stone, sharp fresh cuts, one smooth face; the name is a decal so it can change.
- **Framing:** kneeling at the stone puts the stone in the lower-left third of the view and the plume 25-35° to the right
  and 12-18° up. The world builder computes the kneeling spot from that.
- **Respect rules [Proposed]:** no real climbers' names on any plaque (unreadable or fictional only); the player can
  kneel only at his father's stone; nothing at the memorials can be climbed or knocked over.

### 5.5 Prayer flags, done realistically

The director asked (2026-11-19) for every flag line on the map to be redesigned. Today the flags are flat cards on a
straight cord; they float and their sag is capped at 45 cm.

**What real prayer flags are like [To verify with a Sherpa consultant]:**
- Horizontal lines (lungta, "wind horse") in five colours in a fixed, repeating order: blue, white, red, green,
  yellow. Our builder already uses this order.
- Each flag is a printed cotton or synthetic rectangle with a wind-horse image and prayers, sewn along its top edge onto
  the cord, close together.
- Lines run from high points: poles, roof corners, chortens, cairns, rocks. They sag in a long curve and never touch the
  ground.
- Old lines are faded and shredded by sun and wind, often next to bright new ones.
- Vertical poles with a long banner (darchor / tarchen) stand near houses and camps.

**How we build them [Proposed]:**

| Part | Plan |
|---|---|
| Cloth | a few Meshy or downloaded flag meshes with real thickness and a frayed edge |
| Print | a texture of a wind-horse block print with **unreadable** script until a Tibetan reader checks real text |
| Motion | wind sway in the material (cheap), not real cloth simulation: laptop budget |
| Cord | a real curve through every flag; flags hang from it, not beside it |
| Ageing | per-line random fade and tatter; some lines new, most old |
| Placement | a tool that strings lines between real anchor points and checks clearance above head height |
| Known bug | the shadow smear under the cords (issue #4): check that the flag material writes motion data, and test with frame generation off |

### 5.6 Light by time of day

The story compresses one day into about 75 minutes: the clock runs slowly during the day, then glides to dusk over
90 seconds when the flags come into hearing.

| Scene | Clock | Light | Art-direction note |
|---|---|---|---|
| Prologue, the Balcony | 02:30 | moonlight; the headlamp is the key light | the father a dim shape until the beam finds him |
| Tent at dawn | 06:20 | sun through orange fabric; the door glowing | stillness is the scene |
| Camp at sunrise | about 06:30 | tiny warm apricot areas far away; the near camp stays blue | "dawn = warm far, blue near" |
| Trail at midday | rising | deep blue upper sky; startling detail; footprints give scale | "midday" |
| The cloud | afternoon | sky and snow close in value; small colours (rust rock, a glove, the brass compass) pop | "overcast" |
| Dusk at the memorials | 17:35 | shadow climbs the valley from the bottom; flags backlit; warm light only on the high ridges; violet sky | "evening", target image 03 (violet dusk) |
| The plume | dusk | the plume glows gold for 11 s; a slight exposure lift | the only myth moment |
| Night tent | 20:40 | only Nima's headlamp (shadowless, to save performance) and the stove glow | his face in the light is the last shot |

---

## 6. Characters and the cast pipeline

### 6.1 The cast

| Who | Role | Seen | How made |
|---|---|---|---|
| **Sangye** (22) | you | hands, sleeves, legs, shadow; red down jacket patched at the elbow (his father bought two) | **[Built]** Meshy whole character, rigged to our skeleton |
| **The father** (about 48) | the mystery | prologue only, far away, at night, sitting in a red down suit with no oxygen mask | **[Built]** Meshy whole character, sitting pose |
| **Dolma** (8) | love, the bond | close: she holds your hand | **[Built]** Meshy whole character (MetaHuman has no child bodies) |
| **Aunt Pema** (about 50) | the promise | close, at dawn | MetaHuman face and body (preset Lani) + clothes |
| **Nima** (older Sherpa, climbed with the father) | the secret | close, all day; his face in the headlamp is the last shot | MetaHuman (preset Aoi) + clothes |
| **Marek Kovač** (Slovak leader, 40s) | authority | the team | MetaHuman (Bruce) + clothes |
| **Elena Ruiz** (Spanish first-timer) | care, a choice | the trail | MetaHuman (Vivian) + clothes |
| **Ji-woo Han** (Korean client, films everything) | a witness | the team | MetaHuman (Aera) + clothes |
| **Two porters** | the trail's real workers | the narrow trail | MetaHuman (Kelvin) + clothes, roped loads with a head strap |
| **Dr. Hale** | Base Camp doctor | radio only | voice |

### 6.2 What the director said about them

- Faces: realistic. **Keep them.**
- Clothes: "extremely bad", "ultra bad" (2026-11-15 and again 2026-11-19): collars float off the neck with a dark gap;
  fabrics show no texture and read as smooth plastic.
- Walking: feet slide and never leave the ground; no prints in the snow under the NPCs' feet.

The reason is known. The clothes were cut from the body surface in Blender by a script. A script cannot fake cloth
simulation and texture craft. Hand-modelling realistic clothes in code has stopped (director, 2026-11-15).

### 6.3 The plan: MetaHuman faces + AI-generated outfits

| Step | What | Status |
|---|---|---|
| 1 | Meshy round 1: 29 props + 3 whole characters (Dolma, father, Sangye), rigged to the UE skeleton | **[Built]** 2026-11-19: all succeeded first try, imported and placed |
| 2 | Meshy round 2: **seven outfits** for the MetaHumans (Nima, Marek, Elena, Ji-woo, Pema, porter A, porter B), as separate pieces (jacket, trousers, hat), in A-pose | **[To build]**: needs a credit decision (§10) |
| 3 | Fit each outfit to its MetaHuman body in Blender with our tested fitting tool, and copy the body's skin weights so the cloth moves with the body | tool exists from the 2026-11-15 research; to run |
| 4 | One new garment material built on Epic's MetaHuman fabric library and better fabric textures, keeping real colour and wear | **[To build]** |
| 5 | Keep the fitted real boot scans (logos painted out) | **[Built]** |
| 6 | Check every character in game: close-ups in sun, overcast and night; back view with hat; feet. Director judges | each round |

**Fallbacks:** free CC BY garments made with cloth software, fitted with the same tool (the hybrid plan of 2026-11-15);
Epic's free MetaHuman outfits on Fab (no down jackets or hats, so only for trousers).

**Aunt Pema is the hardest.** No ready-made Sherpa dress exists anywhere (a long sleeveless wrap dress, a striped front
apron worn by married women, a woven sash with a silver buckle, a floral headscarf). Meshy is the first try; a careful
rebuild is the fallback.

**The outfit rules** (from `Chapter1_Realism_Spec.md`): local people's gear is faded, mismatched and a few seasons old;
no real brand logos or brand-signature colours; hair shows under every hat; medium hiking boots on the trek, never 8,000
m boots below Base Camp; sunglasses on at midday; a khata belongs to the farewell, not to everyday wear.

### 6.4 Performance limits for the cast

The laptop has a 12 GB graphics card. At most 1-2 MetaHuman builds per editor session; clothing textures at 1k; NPC
assets load in the background at chapter start and NPCs appear only behind closed eyelids, so a 2.7 s load freeze can
never land on screen again.

---

## 7. Systems used

| System | What it does in Chapter 1 | Status |
|---|---|---|
| **Story machine** | Plays the chapter from a data file of beats. Each beat says *when* (after another beat, near a place, when the player looks at something, after a delay) and *what* (a line, a choice, a sound, a posture, a teleport behind closed eyes...). The beats are written by a Python script (`Tools/Story/chapter1_story.py`), which **validates** them against the camera rules and refuses to write a file that breaks one | **[Built]**; beats being rewritten tonight for the new camera vocabulary **[Written]** |
| **World builder** | Places the camp, the memorials, the stream hole and every story place on the real ground by tracing it, then checks the key frames (door 30-40° left from the tent, flags straight ahead at the bend, plume framed from the kneel) | **[Built]**; needs one re-run in the editor for the new places |
| **Camera rig ("Eyes on a Body")** | Section 4 | **[Written]**: about 150 KB of new code; not yet built, self-tested or measured at the time of writing |
| **Survival and physiology** | Blood oxygen from altitude and effort, heart rate, breathing, core temperature, stamina, water, food; pack weight slows the walk; body effects on the screen (tunnel vision, frost, shiver) at high altitude. The thesis core; being calibrated against published data (plan block A2) | **[Built]**, calibration in progress |
| **Choices and the journal** | Choices appear while you walk; flags remember them; the journal writes the first page from them; Dr. Hale's evening number for Elena depends on your choice | **[Built]** |
| **Sound** | Wind, breath and heartbeat tied to effort, footsteps per surface, stove, stream, flags, radio. Four "lure" sounds (tent flap, laboured breath, flag gust, ridge wind) are still to be picked by the director's ear | partly **[Built]** |
| **Flight recorder** (session monitor) | Records every play session: every frame's timing and camera, every second's body state, every key and screen, hitches with their likely cause, screenshots every 2 minutes, plus a system log; writes a readable report. This is how we found the spinning camera, the knee-height camera and the 4x frame-generation smear | **[Built]** |
| **Robot player** (`ygg.Story.Auto 1`) | Plays the whole chapter by itself and answers every choice, under the flight recorder. Proves the chapter can be finished and times it | **[Built]**; to run on the new camera |
| **Camera self-test** (`ygg.Story.CamTest`) | Runs every camera situation (lie → sit → stand, crawl out, kneel, nudge, slip, fly-out, inspect, a menu or death in the middle of a pose) and checks that the camera always comes back right | **[Written]** |
| **Review shots** | Jumps to each key frame and saves a screenshot for the director to approve before he plays | **[Built]** (`story_shots.ps1`); camera frames **[Written]** |
| **AI asset pipeline** | Meshy → Blender clean-up (real size, pivot at the bottom) → import into Unreal → placement by the world builder | **[Built]** |

**Link to the thesis.** The robot player is a first, simple version of the thesis's "virtual climber" (plan block B1):
an automated player that uses the same physiology model. The flight recorder already produces the per-second data the
thesis charts need.

**The 17 measured checks** before the director plays (camera spec §11) include: no view change the player did not cause
beyond the speed limits; never walking at knee height (eye at least 120 cm in every frame above 0.6 m/s); every cut
behind closed eyes; at most 3 nudges; no freeze over 100 ms with the eyes open; the rig costs at most 0.05 ms per
frame; the look of the body and the key frames is judged by the director.

---

## 8. What changed from v1, and why

Each change is tied to something the director saw in the play-tests of 2026-11-19 (flight recorder sessions
20261119_203955 and 20261119_222446) or reported earlier.

| # | Play-test observation | What v1 did | What v2 does |
|---|---|---|---|
| 1 | **The walk-in house trapped players** at the start | opened in the family kitchen of a Sherpa house (built after v1 left it "to discuss") | **opens in Sangye's tent.** You crawl out yourself; nothing can block the way out. The house assets are kept for later |
| 2 | **The camera spun 360° vertically** when the story turned the head (measured: full loops at about 390°/s, spikes to 5,000°/s; the fly-out ended upside down) | the story blended camera angles directly, and took the long way round | the story never touches the camera; one rig owns it; scripted shots are aimed at a point; speed limits |
| 3 | **The camera stayed at knee height** after the tent (walking 48 s at an eye height of 52 cm) | "kneel" only lowered the camera; there was no stand-up | postures are real states; moving stands you up first; a watchdog catches any low walking |
| 4 | **A huge boot right under the lens, screen dark** | standing legs under a lowered eye; heavy vignette; auto exposure adapting to snow | what you see of yourself depends on the posture; lighter vignette; steady exposure when looking down |
| 5 | **Smearing and streaks** when turning | 4x frame generation on about 30 real fps, motion blur on, fast story spins, no camera-cut flag on teleports | default 2x frame generation, motion blur off, speed limits, a cut flag on every cut; A/B tests to confirm the cause |
| 6 | **Freezes mid-scene** (2.7 s and 0.6 s while NPCs were dressed) | NPCs loaded when they appeared | all NPCs load in the background at the start and appear only behind closed eyes |
| 7 | **Props looked hand-made and fake**: every tent one recoloured dome, the father's name as floating 3D text, the stream as two flat discs | props built in code | **AI-generated assets (Meshy round 1)**: real tents (dome, mess, kitchen, toilet), chortens, the father's stone, khata, duffels, barrels, stove sets, the stream hole, porter loads |
| 8 | **NPC clothes still bad** (neck gap, no texture) | clothes cut from the body by a script | Meshy outfits fitted to the MetaHumans (§6.3) |
| 9 | **NPCs slide**, no snow prints under them | a walk animation not matched to speed | match walk speed to the animation; footprints for NPCs (next steps) |
| 10 | **Prayer flags float and look fake; the camp is not pretty** | flat cards on a straight cord | realistic flags everywhere, camp redesign (§5.1, §5.5) |
| 11 | Too many gaze grabs | 9 grabs that turned the head | at most 3 gentle nudges, only as a fallback after sound, people and light |
| 12 | A hard 326 m teleport after the porters | teleport to the end of the porter path | deleted; everything is walked |
| 13 | Text cards for objects | 6 full-screen cards | 4 objects you hold and turn (photos, compass, drawing) and lower-third captions |
| 14 | Story-placement bugs from the same session: Marek walked 2.3 km alone, porters left the trail by 12 m, Dolma and the aunt stood inside each other, Nima froze 50 m away at night, the river sound followed you | — | each fixed in the beats or the world builder (comments marked "audit" in the script) |
| 15 | (not a play-test) No yak asset exists yet | a yak train on the narrow trail | **porters** with roped loads. Real, and the "mountain side" rule is the same |
| 16 | (story detail) | three photos, one of the father on the summit | two photos: Base Camp 2019, and the Icefall photo with the writing on the back |

What did **not** change: the story, the people, the peak and the end, the teaching ladder, the choices, and "Day 1 / 49".

---

## 9. Production status and next steps

### 9.1 Status (2026-11-19, late evening)

| Area | Status |
|---|---|
| Story beats (data) | being rewritten tonight for the new camera vocabulary; the validator is written |
| Camera rig | code written by an implementation run that was paused mid-way; **not yet built or measured** |
| World (camp, trail places, memorials, night camp) | built; needs one re-run for the new places |
| AI assets round 1 (32 models + 3 rigs) | imported and placed |
| Tent interior, sleeping-bag fit in the tent | missing |
| NPC outfits (round 2) | not started; waiting on a credit decision |
| Prayer flags, camp beauty, window edges, flag shadow smear | logged as issues #4, #6, #7; after the camera |
| NPC walk and footprints | logged as #5; after the camera |
| Voices | the director records them; every line is logged as "Story [VO]" |
| Lure sounds (4) | to be chosen by the director's ear |
| Git | **nothing committed today** |

### 9.2 Next steps, in order

| # | Step | Who | Done when |
|---|---|---|---|
| 1 | Finish and build the camera rig; fix compile errors | Claude | the project builds |
| 2 | Re-run the world builder; write the story file (validator prints OK) | Claude | `validate: OK` |
| 3 | Camera self-test, then one full robot-player run under the flight recorder | Claude | the 17 checks pass by measurement; the chapter length is known |
| 4 | Review shots of the key frames | Claude → director | the director approves the frames |
| 5 | **The director plays the chapter** (about 75 min, or by scenes) | director | his notes are logged |
| 6 | Realistic prayer flags everywhere, camp redesign, window edges, flag shadow smear | Claude | before/after shots; the director judges |
| 7 | NPC walk speed and footprints under NPC feet | Claude | measured: no sliding; prints visible |
| 8 | Meshy round 2: seven outfits, fitted and skinned; new garment material | Claude, after the credit decision | close-ups in three lights; the director judges |
| 9 | Tent interior for dawn and night | Claude (Meshy or download) | the tent shots pass |
| 10 | Fill the 8-minute pacing gap (§3.4) | Claude, if approved | robot run shows no gap over 6 min |
| 11 | Commit and push; update the development plan | Claude | committed |

### 9.3 Against the thesis calendar

The interim check is on **17-18 December 2026** (internal deadline 14 December). The development plan's Phase A is
about the physiology model, its data logger, two demo routes and an interim package. Chapter 1 sits on demo route 1
(Dingboche onward), so a working chapter is a strong opening for the interim demo, but it must not push out block A2
(the physiology write-up and calibration), which is the thesis core. Steps 1-5 above are the minimum for a clean demo;
steps 6-10 can follow.

---

## 10. Risks and open questions

### 10.1 For the director

| # | Question | Default if no answer |
|---|---|---|
| D1 | Frame generation default "Auto" = 2x, motion blur off? | yes |
| D2 | Mouse sensitivity ×0.7 (fast flicks feed the streaks)? | keep today's value |
| D3 | Slow motion in the slip? | on (0.35 speed for 0.7 s) |
| D4 | Plume nudge strength: up to 12° upward, or sound only? | 12° |
| D5 | **Meshy credits:** 167 are left. Seven outfits need about 250 credits plus retries. Buying more is your money | wait |
| D6 | Real tent sizes (and fix the fit in the camera), or keep the larger tents? | real sizes, if the camera allows |
| D7 | Season: v1 says **spring**; the realism research defaults to **late autumn**. It changes the fields, festivals and stores in the village | spring (the climbing season) |
| D8 | Fill the pacing gap with a mani wall (§3.4)? | yes |
| D9 | How do the people speak: English, or Nepali with subtitles? | English |
| D10 | Chapter length 70-80 min, or shorten the trail part? | keep, measure first |

### 10.2 For the professor (thesis questions)

1. **How much of Chapter 1 counts toward the interim?** Our proposal: show Chapter 1 as the place where the physiology
   model is felt (SpO₂, breathing, the rest-step, water, load, altitude sickness), plus the testing tools (flight
   recorder, robot player). The physiology model itself remains the main result.
2. **AI use and disclosure.** Claude writes most of the code and drafts; Meshy generates 3D assets. How should this be
   disclosed under the university's rules? Meshy's licence for paid plans must also be checked for commercial use later.
3. **Is the robot player a fair first step toward the "AI approach"** (the virtual climber with an optimised strategy)?
4. **Does a later player study of this chapter need ethics approval?**
5. The supervisor asked us to study **Steep** and **PEAK**. A short written comparison is still to do.

### 10.3 Risks

| Risk | Likelihood | Effect | Mitigation |
|---|---|---|---|
| The paused camera code does not build or has bugs | medium | delays the play-test | built in new files; the old camera stays available with one switch (`ygg.View.Rig 0`); self-test and measured checks before handing over |
| Postures without animations look odd (hidden legs, lowered torso) | medium | looks cheap close up | look limits, the sleeping bag and the hands cover it; real animations in phase 2 |
| AI outfits still look wrong on MetaHumans | medium | the cast still looks fake | test one outfit first; fallback to cloth-software garments |
| Cultural mistakes (dress, flags, memorials, religion) | medium | disrespect to a living culture | sourced realism spec; no real names; unreadable script until checked; a Khumbu Sherpa consultant before the title demo |
| The 5.8 km trail feels long | medium | boredom (the director's own 2026-11-14 concern) | the 6-minute rule, the pacing table, the mani-wall proposal; real timing from the robot player |
| Performance on the laptop (about 30 real fps measured at High on a 4K-wide viewport) | high | smear, stutter | 2x frame generation default, shadowless headlamps, rig cost ≤ 0.05 ms; measure before and after |
| Thesis time: chapter work crowds out the physiology core | medium | a weak interim | steps 1-5 only before the interim; A2 stays first in the plan |
| Meshy deletes files after 3 days | low | lost source models | everything is already downloaded to the project |

### 10.4 Facts to check before the script is final

- **Can the summit be seen from Thukla on our terrain?** [To verify] In reality the Nuptse-Lhotse wall hides it from
  much of this valley. The plume card is raised above the ridge so it can be seen. If that is a liberty, it should be a
  deliberate one (the myth showing itself), not an accident.
- The Thukla memorials (layout, what is written on them), the khata at farewells, the five-colour flag order, the mani
  wall custom (walk with it on your right), the Bardo and its 49 days, the Six Realms, the goddess Miyolangsangma.
- Whether a widowed Sherpa woman keeps the front apron (it matters if Pema is a widow).

---

## Appendix: source documents

| Document | Used for |
|---|---|
| `Docs/Development/Chomolungma_Chapter1.md` (v1, 2026-11-14) | story, scenes, teaching ladder, research table |
| `Docs/Development/Chapter1_Design.md` §8 | research sources with links |
| `Docs/Development/Chapter1_Camera_Spec.md` (2026-11-19) | camera rules, postures, shot cards, checks, risks |
| `Docs/Development/Chapter1_Realism_Spec.md` (2026-11-14) | dress, boots, faces, sources, open questions |
| `Docs/Development/Chapter1_Asset_List.md` (2026-11-19) | the Meshy asset list and outfit table |
| `Tools/Story/chapter1_story.py` | the beats, lines and choices as they stand tonight |
| `Tools/Story/build_chapter1_world.py` | camp layout, event distances along the trail, memorial framing |
| `Docs/Development/Development_Plan.md` | thesis topic, calendar, blocks |
| `research_notes/AI tools for cinematic chapters/` | *Half-Life: Alyx*, *Firewatch*, *Oxenfree*, *Ghost of Tsushima*, *Cairn* |
| `research_notes/NPC clothing realism/DECISION.md` | the clothing fallback plan |
| `Saved/Monitor/20261119_203955/`, `20261119_222446/` | the play-test measurements |
