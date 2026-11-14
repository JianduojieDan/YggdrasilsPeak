# Yggdrasil's Peak: The Climb, Chapter Design v1 (14 hours)

Version 1.1 · 2026-11-14 · written for the author to react to and cut. (1.1: section 1.6, the loops, from research.)
Builds on `Docs/Development/Phase1_Design_Plan.md` (story, endings, stage template) and **changes two of its
decisions** at the author's request (section 1.2).

**Labels** (same as the Phase 1 plan):

| Label | Meaning |
|---|---|
| **[Established]** | The author decided it (original setting `游戏设定.docx`, or a decision in chat). |
| **[Measured]** | Read from project data or code. Can be re-checked. |
| **[Proposed]** | My design. Not approved until the author says so. |
| **[Open]** | Still to decide (section 12). |

Times are **targets for a first-time player at normal skill**. They are planning numbers made from the measured
route and movement speed (section 2). They must be checked by play-tests and the session monitor; any chapter that runs
more than 25% off its target gets re-paced, not padded.

---

## 1. What the game is now

### 1.1 One paragraph

**[Established]** Sangye climbs the real Everest, at real 1:1 scale, from his village Dingboche to the summit, to ask
his vanished parents one question: *"Was what you saw worth it?"*
**[Proposed]** The climb is an **expedition**, not a walk. Like a real Everest season, it goes up and down the mountain
in **rotations**: up to sleep high, back down to recover, and up again when the body and the weather allow. Every
stretch of walking carries a decision: when to leave, what to carry, how fast to go, how much oxygen to breathe, when to
turn back, and who to help. You are not alone: a Sherpa partner climbs with you, a doctor talks to you on the radio,
another expedition shares the ropes, and your parents' ghosts walk in the storms above 7,000 m.

### 1.2 Decisions taken on 2026-11-14 [Established]

| # | Decision (author, in chat) | What it replaces |
|---|---|---|
| D1 | **The whole real route is walked at 1:1.** "That's exactly why we're doing this game." | Phase 1 Q1 (skip the 11.5 km approach) |
| D2 | **Tension over poetry.** Survival indexes and decisions are the most important part. Quiet moments stay, but they serve the tension (rest before the storm). | Pillar order |
| D3 | **A full climb takes at least 14 hours.** | Phase 1 per-stage durations (about 2.5 h total) |
| D4 | **The player must not be alone for the whole climb, from start to end.** "It is extremely lonely." Solitude is fine in parts; it must not be the entire game. (Corrected by the author: not "never alone".) | Phase 1 Q3 (solitary climb, no following AI) |

Everything else in the Phase 1 plan still stands: the story, the three endings, no crafting, camp checkpoints, no
permadeath, first person, laptop performance.

### 1.3 Pillars, in order [Proposed, from D2]

1. **Survive the body.** Oxygen, cold, food, water, sleep and sanity are always the first thing on the player's mind
   above Base Camp. The physiology model (thesis) is the engine of the game, not a background number.
2. **Decide under pressure.** Every chapter has at least three real decisions with visible consequences later.
   No decision is "obviously right".
3. **The mountain is dangerous in specific ways.** Each danger has a cue, a counter and a consequence; deaths are fair
   and explainable.
4. **People come and go.** Company in some stretches (a partner beside you, a teahouse, a crowded Base Camp,
   headlamps on the face, a stranger who needs help), real solitude in others. Being alone becomes a state the game
   uses on purpose (a storm night in a tent, the last hours to the summit), not the default for 14 hours.
5. **Credible first, mythic later.** The myth grows out of hypoxia and grief until it cannot be denied (Phase 1 plan).

### 1.4 Pacing rules [Proposed, extends the 2026-11-02 rule]

| Unit | How often | Example |
|---|---|---|
| Small detail | every 30-60 s of walking | a crevasse edge, a prayer flag, a dropped glove, a line from the partner |
| Decision or obstacle | every 3-5 min | ladder or detour, drink now or save, clip/unclip, rest at an anchor |
| Event | every 10-15 min | serac crack, gust front, a team coming down, radio forecast change |
| Landmark / set piece | every 20-30 min | Thukla memorials, the parents' camp, the Icefall "Popcorn field", Camp 2 under Lhotse |
| Chapter end | 45-75 min | a camp, a night, a save |

**No stretch longer than 6 minutes without a decision**, measured by the session monitor (section 11).

### 1.5 The heart of the design: the player writes the novel

**[Established]** The author's goal (2026-11-14): a player who steps into this world does not want to leave. He is not
only playing; he is writing his own novel, and he is excited to keep going and finish it. "Like me playing Cairn."

**[Proposed]** Players feel they are writing the story when the story is made of **their** decisions, **their**
mistakes and **their** luck, and when the game remembers all of it. Nine principles follow from that, and every system
and chapter in this document is checked against them.

1. **Systems collide, so no two climbs are the same.** Body, weather, supplies, people and terrain are systems, not
   scripts. A storm arriving while Nima is coughing and the gas is low is a crisis nobody wrote; it belongs to the
   player.
2. **Every decision is remembered and answered.** Nima, Dr. Hale, the other team, the journal and the ending remember.
   Consequences come back hours later: the medicine given to a sick trekker on day 2 is missing on day 22.
3. **Your own line.** The 1:1 mountain is open. The fixed ropes are the safe line, not a rail. Leaving them is always
   possible: shortcuts, caches, views, lost ground, deadly falls. (This is what makes Cairn's climbs personal: the route
   is yours.)
4. **The game writes it down.** Every night in camp Sangye writes a journal entry from what really happened that day:
   the decisions, the numbers, the people, the fear (section 4.12). At the end the player owns a book of their climb in
   Sangye's handwriting. This is the novel, literally.
5. **Failure continues the story.** Most failures are setbacks with lasting marks, not reloads: frostbitten fingers clip
   slower for the rest of the game; a failed summit push means a second attempt with less oxygen; Nima short-roping you
   down is a chapter, not a game over. Real deaths stay (falls, seracs, crevasses), and so do the camp checkpoints.
6. **Small rituals make it lived in.** Melting snow, tea, the radio call, the oximeter, prayer flags at the puja,
   writing by headlamp. The camp is where the day's story settles.
7. **Every night ends on a question.** The forecast, Nima's cough, the route above, a light on the ridge. "One more
   day" is the hook, the way "one more turn" is in strategy games.
8. **Solitude and company are tools** (section 3), used on purpose.
9. **The mystery pulls, the body pushes.** The parents' story leaves a question at the end of every chapter; the body
   makes every metre towards the answer cost something.

**The four hook loops** that keep a player from quitting:

| Loop | Length | What keeps you in it |
|---|---|---|
| The step | seconds | footing on steep ground, the clip at each anchor, breath and heartbeat, the next ladder rung |
| The stretch | minutes | the next decision, the next landmark already visible, the weather wheel counting down |
| The day | one chapter | reaching camp, the work there, the journal entry, tomorrow's question |
| The climb | the whole game | the parents, Nima's father, the people you helped or left, the ending you are shaping |

**How this shapes the chapters.** Every beat in section 5 carries one of three marks:
- **Fixed**: always happens (the story spine, landmarks). Few: at most a third of a chapter's beats.
- **Systemic**: happens when the systems produce it (weather, body, supplies, other people's states). Most of the
  tension.
- **Chosen**: the player starts it (detours, help, risk lines, own lines). Most of the story.

### 1.6 The loops: why a player cannot stop [Proposed, from research, sources in section 13]

**[Established]** The author (2026-11-14): there are known game-design mechanisms that, completed as a loop, make a
player obsessed. Design the game around them.

**What the research says** (summarised; sources in section 13):

| Finding | What it means for us |
|---|---|
| Games run on **nested loops**: a seconds-long core loop, a minutes-to-an-hour loop, a long progression loop. Each feeds the next, and when a game is not fun, the fault is usually in the smallest loop | build and test the smallest loop first |
| The **compulsion loop** is anticipation → activity → reward, and the **anticipation** is the most gripping part. Unpredictable rewards hold attention most strongly; that is honest when the unpredictability comes from gameplay and exploitative when it is tied to money | our surprises come from the mountain's systems, never from loot or shops |
| Players enjoy games, and feel better after them, when three needs are met: **autonomy** (real choice), **competence** (getting better, fair feedback) and **relatedness** (connection to others). Loneliness counts as frustration of the third | the author's "extremely lonely" is a known motivation gap; Nima is the fix |
| An **unfinished task** stays on the mind (the Zeigarnik effect, the "one more turn" of strategy games). Healthy open loops are ones the player wants closed and can close; loops that never close turn into anxiety | every chapter ends on a question, and every question gets answered |
| **Emergent stories** (RimWorld's "story generator"): a small cast of named characters, a simulation of health and relationships simple enough to watch, and players read more story into it than is really there | exactly our systems: a few named people, bodies and trust; the journal tells it back |
| **Cairn's** strength is its climb-and-camp loop: the bivouac is where the survival systems matter most, preparing in the tent for the next climb | camp is not a pause; it is half the loop |
| **Death Stranding:** route planning, load and terrain make every trip a decision; its weak point is when planning stops mattering late in the game and it becomes "hold the stick forward" | conditions must keep escalating so planning never stops mattering |
| **The Long Dark:** scarcity drives action, and rest is a resource; its weak point is repetition without an end goal | stocks are limited; the summit and the parents are always the goal |

#### The five loops of Yggdrasil's Peak

Each loop sits inside the next one. A reward in a small loop is progress in the bigger one.

**Loop 1 · The step** (seconds). *Read → act → the body answers.*
Read the ground ahead (snow, ice, rock, the next anchor, the next rung) → act (step, clip, push the jumar, rest-step,
breathe) → the body answers at once (breath, heartbeat, the SpO₂ number, stamina). The small rewards: the clean click
of a clip, a steady rhythm, breath settling. The small anticipation: the next anchor fifteen metres up.
**Today this loop is weak** (walking only; ropes clip by themselves), so it is the biggest risk in the whole design.
Built by: clipping by hand, rest-step, the jumar rhythm, feet that land on the slope (exists), crampon, rope and breath
sounds (4.8).

**Loop 2 · The stretch** (2-10 minutes). *See → decide → commit → outcome.*
See the next thing (a serac zone, a ladder, the weather climbing the valley, a queue) → decide (line, timing, oxygen
flow, wait or go) → commit under pressure (the serac clock, a gust, the wheel's countdown) → outcome: safe (relief), a
close call (adrenaline, and something worth writing tonight), or a cost (time, an injury, a bottle). The
unpredictability comes from systems, so it is the honest kind. The anticipation (the creak of a ladder, "MOVE!", the
wheel spinning) is the gripping part.

**Loop 3 · The day** (one chapter, camp to camp, 40-100 minutes). *Plan → climb → arrive → tend → write → wonder.*
**This is the heart of the game.**
- **Plan:** forecast, start time, what to carry.
- **Climb:** a chain of stretches (loop 2).
- **Arrive:** reaching camp is the strongest reward in the game, as every real climber knows: the tent, the stove's
  roar, warmth.
- **Tend:** water, food, Nima, the radio call, injuries, tomorrow's loads. As in Cairn's bivouac, this is where the
  survival systems matter most.
- **Write:** the journal turns the day into a page of the player's story (4.12). The reward is the story itself.
- **Wonder:** tomorrow's open question (the forecast, the cough, a light on the ridge). That is the open loop that
  makes "one more day" irresistible. Then sleep, and the next plan.

**Loop 4 · The rotation** (several days, three or four chapters). *Go high → stock → come down → recover → plan.*
Progress the player can see, earned in loops 2 and 3 and spent on the next push:
- **The body's ceiling:** "adapted to 6,200 m" on the oximeter screen; a higher ceiling means better SpO₂ and longer
  stretches next time.
- **Camp stocks:** "Camp 2: 3 bottles, 6 rations, 2 gas."
- **Route knowledge:** the Icefall's state known after helping the doctors.
- **Trust:** Nima's and Dr. Hale's.
These are the game's "upgrades", but they are real physiology and real logistics, not points.

**Loop 5 · The climb** (the whole game). *A question → evidence → doubt → the answer.*
Every chapter answers one small question and opens a bigger one (the names on the stone → their tent → wood in the
ice → page 3 → the limb → page 2 → the light). Alongside: the people's stories (Nima, Elena, Ji-woo, the trekker)
and the book growing night by night. It closes with the ending the player shaped, and a new climb can be different
(other choices, other lines, another book).

#### How the three needs are met

| Need | In this game |
|---|---|
| Autonomy | your own line off the rope, start times, the window, who to help, the oxygen dial, the turnaround time; never one obviously right answer |
| Competence | mastering the step loop (clean clips, rhythm), a rotation plan that works, numbers the player can read (oximeter, thermometer), deaths that are fair |
| Relatedness | Nima, Dr. Hale, the other team, the aunt; the bond is built by play (catching each other, sharing loads), not only by dialogue |

#### Eight rules that follow

1. **Loop 1 must be fun on its own.** Build and test it first (the vertical slice, section 9).
2. **Every reward sets up the next anticipation:** camp sets up tomorrow's question; the summit sets up the descent.
3. **Surprises come from systems,** never from random loot.
4. **Every loop closes:** camp closes the day, a stocked camp and a ready body close the rotation, the ending and the
   book close the climb. No chores that never end.
5. **Planning never stops mattering** (the Death Stranding lesson): conditions escalate with altitude, and no item
   makes walking trivial.
6. **Scarcity drives action** (the Long Dark lesson): stocks and gas run out; rest is a resource.
7. **A small named cast, simulated bodies and relationships** (the RimWorld lesson); the journal tells their story.
8. **Measure the loops** (section 11): time between clips, between decisions (under 6 minutes), per chapter, and
   progress per rotation.

**What we do not do:** no rewards for money, no daily log-ins, no timers that punish being away, no loops that never
close. The pull comes from the climb itself.

---

## 2. Where 14+ hours come from

### 2.1 The facts [Measured, 2026-11-14]

| Fact | Value | Source |
|---|---|---|
| Route, Dingboche → summit | 27.74 km on the map (28.58 km along the ground), +4,720 m / −213 m | route.json, 8 legs |
| Walk / sprint speed | 230 / 500 cm/s, slowed by grade, low oxygen, storm, deep snow, pack weight | SurvivalConfig.h, SurvivalComponent.cpp |
| Pure walking time, whole route one way | about **4.7 h** roped, 5.1 h unroped | computed from the above |
| Vertical speed on a 25° slope at 6,100-8,400 m | about 1,600-2,900 m per hour | computed (real climbers: 100-300 m per hour up there) |
| Game clock | **1 real minute = about 24 game minutes** (a 24-hour day passes in about 60 real minutes) | session monitor, uds_time |
| Food / water, walking | empty in 1.85 h / 1.23 h; **camps refill both for free** at 1%/s; pickups come back after 10-15 min | SurvivalConfig.h, place_supplies.py |
| Acclimatisation | resting in a camp adapts at 5 m per real second: Camp 2 in about **220 real seconds**; sleeping gives none | SurvivalComponent.cpp |

So walking up once is about five hours. **A 14-hour game cannot come from walking the route once, and padding it by
slowing everything would be boring.** The hours have to come from what makes a real Everest season long and
dangerous.

### 2.2 The five sources of time [Proposed]

1. **Rotations.** Like real expeditions: up to sleep high, back down to recover, up again. The Icefall is crossed five
   times on foot (three up, two down), the Western Cwm and the Lhotse Face twice. Each pass is different (section 8).
2. **Load carries.** Rotations also stock the high camps. What the player and Nima carried up decides how much food,
   gas and oxygen waits at Camps 2, 3 and 4 on the summit push. Every pass has a purpose.
3. **Camps and nights with work in them.** Melting snow for water, eating, the radio call, checking Nima, medicine,
   bad sleep at altitude, storms that pin you down.
4. **The body slows you down where it really would.** Above 7,000 m the physiology model makes every step expensive
   (real climbers gain 100-300 m per hour there). Summit night becomes the long, desperate climax it is in reality
   instead of a 28-minute walk.
5. **Events, set pieces and people,** one every 10-15 minutes (section 1.4).

### 2.3 The expedition calendar [Proposed]

Clock proposal: **6 game minutes per real minute while awake** (today 24). With it, the chapters line up with real
timings: an Icefall crossing from 03:00 reaches Camp 1 about 10:00 after 70 minutes of play; summit night from 22:00
reaches the top about 08:00 after 100 minutes. Sleeping and rest days skip time.

| Game day | Where | What happens | Chapter |
|---|---|---|---|
| 1 | Dingboche | the compass, the priest, packing, first night at home | 1 |
| 2 | Dingboche → Lobuche (4,940 m) | the memorial pass, teahouse night | 2 |
| 3 | Lobuche → Gorak Shep → Base Camp | Nima, the parents' tent, arrival at dusk | 3 |
| 4 | Base Camp | doctor's check, puja, the plan, first forecast | 4 |
| 5 | Base Camp → Camp 1 | rotation 1: first Icefall crossing, pre-dawn | 5 |
| 6 | Camp 1 → Base Camp | first night high, descent | 6 |
| 7-9 | Base Camp (or down to thicker air) | rest days, Base Camp life | 7 |
| 10 | Base Camp → Camp 1 | rotation 2: the Icefall has changed | 8 |
| 11 | Camp 1 → Camp 2 | the Western Cwm | 9 |
| 12 | Camp 2 → Camp 3 → Camp 2 | first touch of the Lhotse Face | 10 |
| 13 | Camp 2 → Base Camp | retreat, a rescue | 11 |
| 14-18 | Base Camp | waiting for the window (skipped in a few scenes) | 12 (start) |
| 19 | Base Camp → Camp 2 | the summit push starts at night | 12 |
| 20-21 | Camp 2 → Camp 3 | blue ice, a storm night | 13 |
| 22 | Camp 3 → South Col | the death zone, the second page, Nima's lungs | 14 |
| 22-23 | South Col → summit | summit night | 15 |
| 23 | Summit | the temple, the choice | 16 |
| 23-24 | Summit → South Col (→ Base Camp) | down | 17 |

About **24 game days** (a real South Col expedition from Dingboche takes about six weeks). Real dates are never
shown, so the compression does not break belief.

### 2.4 The chapter budget [Proposed]

"Walk" = moving along the route. "Other" = camps, set pieces, talking, decisions while stopped. Company levels from
section 3.

| Ch | Name | Route | Walk | Other | **Total** | Company |
|---|---|---|---|---|---|---|
| 1 | The Compass | Dingboche | 5 | 35 | **40** | crowd |
| 2 | The Memorial Pass | Dingboche → Thukla → Lobuche | 70 | 15 | **85** | crowd, then alone |
| 3 | Their Tent | Lobuche → Gorak Shep → parents' camp → EBC | 70 | 15 | **85** | crowd → partner |
| 4 | Base Camp | EBC | 0 | 40 | **40** | crowd |
| 5 | The Icefall | EBC → Camp 1 (rotation 1) | 50 | 20 | **70** | partner |
| 6 | First Night High | Camp 1 → EBC | 25 | 20 | **45** | partner, other team |
| 7 | Thick Air | EBC (or lower) | 0 | 25 | **25** | crowd |
| 8 | The Icefall, Changed | EBC → Camp 1 (rotation 2) | 50 | 15 | **65** | partner, other team |
| 9 | The Western Cwm | Camp 1 → Camp 2 | 22 | 18 | **40** | partner, other team |
| 10 | The Lhotse Face | Camp 2 → Camp 3 → Camp 2 | 45 | 10 | **55** | partner |
| 11 | Retreat | Camp 2 → EBC | 36 | 14 | **50** | partner, other team |
| 12 | The Window | EBC → Camp 2 (summit push, at night) | 72 | 18 | **90** | partner, voice |
| 13 | Blue Ice | Camp 2 → Camp 3 | 30 | 30 | **60** | partner, then voice lost |
| 14 | The Yellow Band | Camp 3 → South Col | 40 | 30 | **70** | partner, or alone |
| 15 | Summit Night | South Col → Balcony → South Summit → Hillary Step → summit | 75 | 25 | **100** | alone or partner; other team; ghosts |
| 16 | The Canopy Temple | summit | 0 | 25 | **25** | the parents |
| 17 | Down | summit → South Col, then an epilogue | 38 | 12 | **50** | depends on the ending |
| | **Total** | | **628** | **367** | **995 min ≈ 16 h 35** | |

Walking is 63% of the time, and every walking stretch carries the decisions and events of its chapter.

### 2.5 Walking passes behind those numbers [Proposed]

Today's pure walking times [Measured] with the proposed changes: descents twice as fast as climbs (rappel and
arm-wrap on the fixed ropes, section 4.8), and the physiology slowdown above 7,000 m (section 4.2).

| Leg | Today, one way (min) | Proposed up / down (min) | Passes up / down | Minutes in total |
|---|---|---|---|---|
| 0 Dingboche → Gorak Shep, 11.5 km | 109-115 | 110 / — | 1 / 0 | 110 |
| 1 Gorak Shep → EBC, 3.0 km | 28 | 27 / — | 1 / 0 | 27 |
| 2+3 EBC → Icefall → Camp 1, 4.8 km | 48 | 50 / 25 | 3 / 2 | 200 |
| 4 Camp 1 → Camp 2, 2.2 km | 22 | 22 / 11 | 2 / 1 | 55 |
| 5 Camp 2 → Camp 3, 2.6 km | 27 | 30 / 15 | 2 / 1 | 75 |
| 6 Camp 3 → South Col, 1.5 km | 18 | 40 / — | 1 / 0 | 40 |
| 7 South Col → summit, 2.2 km | 28 | 75 / 38 | 1 / 1 | 113 |
| | | | **Total** | **620 min**, plus 8 min in the village and the detour to the parents' camp = **628** |

Leg 2+3 passes: rotation 1 up and down, rotation 2 up and down, summit push up. The last descent below the South Col
is an epilogue (chapter 17), not a sixth walk through the Icefall.

### 2.6 Shorter and longer players, and the cut list

| Player | Expected |
|---|---|
| Experienced, few deaths | 11-12 h |
| First time, normal skill (the table above) | about 16.5 h |
| First time, many deaths and side trips | 18-19 h |

**Cut list to reach 14 h for a first-time player** (if 16.5 h feels too long; each line is independent):

| Cut | Saves |
|---|---|
| a. Rotation 1 turns back at the top of the Icefall instead of Camp 1 | 35 min |
| b. Rest days (chapter 7) become one short scene | 20 min |
| c. Rotation 2 does not touch Camp 3; the Lhotse Face is first climbed on the push (chapter 10 removed) | 45 min |
| d. Trek pace ×1.25 below Base Camp (porters walk fast) | 28 min |
| e. Faster descent in chapter 11 | 10 min |
| f. Chapter 9 merged into chapter 8 | 10 min |
| **All six** | **about 2 h 30 → 14 h** |

**Optional content outside the budget** (adds time for players who want it): the acclimatisation walk up the
Nangkartshang ridge above Dingboche (+25 min), Kala Patthar at Gorak Shep for a view of the whole route (+30 min),
the Icefall cave (+10 min), going down to Dingboche for thick air in chapter 7 (+5 min, condensed).

### 2.7 One rule about skipping

**Below Base Camp, a stretch already walked may be travelled in a short condensed scene. Above Base Camp nothing is
skipped until the summit is reached.** The mountain itself is the game; the epilogue after the summit is the only
exception.

---

## 3. The cast: people come and go [Proposed, from D4]

Real Everest is crowded: Sherpas, doctors, cooks, other teams, trekkers, yaks. A game that is lonely from the first
step to the summit is the opposite of the real place. The cast below is small enough for a solo developer: **one
companion who matters, one voice, one rival team, and the crowd as background**.

**Company and solitude alternate (D4).** People are present in some stretches and absent in others, and the change
itself is a tool: walking out of a busy teahouse into an empty valley, Nima going ahead out of sight in a whiteout,
the radio going silent in a storm, Nima turning back with HAPE so the player climbs the last hours alone. Section 5
gives each chapter a **company level**: *crowd* · *partner* · *voice only* · *alone*. No more than two chapters in a
row are *alone*.

### 3.1 Nima Sherpa, the partner (the most important new thing in this document)

- **Who:** 31, from Pangboche (the village below Dingboche), eleven summits, quiet, dry humour, practical. He climbs
  for his family's living and respects the mountain in a way Sangye does not yet.
- **Story link:** Nima's father **Lhakpa** was the sirdar (lead Sherpa) of Sangye's parents' expedition twenty years ago.
  Lhakpa came down alone and never told anyone what happened on summit night. He died five years ago. Nima has one thing
  his father said, and gives it to Sangye only at the South Col: *"He said they did not fall. He said they walked towards
  a light, and the light knew their names."* Nima joins the climb partly for his father.
- **What he does in play:**
  - **Teaches** in Act I and the first Icefall crossing: pace ("bistarai, bistarai": slowly, slowly), clipping, ladders,
    reading seracs. After that, he only speaks when it matters.
  - **Leads or follows.** In the Icefall and on the Lhotse Face he climbs ahead on the safest line; the player may follow
    him or take their own line (faster, riskier, sometimes a find).
  - **Shares the load.** At Base Camp the player decides how to split supplies between the two packs. If Nima is hurt,
    what he carries is at risk.
  - **Saves you, if you let him.** Roped together and close, he catches a slip, steadies a ladder, short-ropes you down
    when you collapse. Far ahead or unroped, he cannot.
  - **Has his own body.** He is better acclimatised than you, but he is not a machine. He can get frostbite, snow
    blindness or altitude illness, especially if the player pushes the schedule or takes his supplies.
  - **Has his own judgement (trust).** He respects decisions he understands. Ignore the turnaround time, refuse to drink,
    climb in a deadly realm without shelter, and he argues; push him past a line and he refuses to go on. Real Sherpas
    do turn clients around.
- **The big decision he carries (Act III):** at Camp 3 or the South Col he starts coughing (HAPE, fluid in the lungs).
  The player chooses: go down with him (this summit push ends, days and supplies lost, he lives), send him down alone
  (he might make it), or continue together (he might die, and the player carries that to the summit). There is no free
  option; each changes the ending scenes and what Nima says about the parents.

### 3.2 Dr. Ingrid Hale, the voice on the radio

- **Who:** Base Camp doctor and expedition manager, Norwegian, calm and dry. She grew up with the Eddas and knows the
  Yggdrasil story as a story. She is the credible voice that slowly stops being sure.
- **Where:** in person in the Base Camp medical tent; on the radio everywhere else.
- **Radio schedule:** two calls a day at 07:00 and 19:00 game time, plus emergencies.
  - **Forecast:** the next realm (weather), its warning time, the wind on the summit for the next days.
  - **Medical check:** she asks for SpO₂ and pulse. The game shows the true numbers; the player can report them
    honestly or lie. Lying keeps her from ordering you down, but her advice is then based on false numbers.
  - **News:** other teams, accidents, the state of the Icefall.
- **Authority:** she can order you down. You can disobey. Each disobeyed order costs her trust: shorter forecasts,
  fewer warnings, and at zero she stops calling until you are back at Base Camp.

### 3.3 The Kovač expedition, the other team

- **Who:** a commercial team: leader **Marek Kovač** (ambitious, on a schedule, not a villain), six clients and five
  Sherpas. Two clients have names: **Elena Ruiz** (44, a doctor, slow and stubborn) and **Ji-woo Han** (25, fast,
  careless). The rest are figures in orange suits.
- **Why they matter:** they make the mountain feel inhabited and they create traffic, help and dilemmas:
  - They share camps and fixed ropes. Their lights are on the Lhotse Face at night; their tents are lit in Camp 2.
  - Their schedule makes **queues** on the Lhotse Face and at the Hillary Step on summit night: wait in the cold, or
    unclip and pass.
  - Marek asks for help fixing a rope section in the Western Cwm: spend time and energy now for goodwill (they share a
    bottle or a tent later).
  - **Elena's collapse** below the South Summit on summit night: give her your spare bottle (and risk your own descent),
    stay with her until her Sherpa comes back (time, cold), or walk on. What happened is told at Base Camp afterwards.
- **How they are built:** the same climber model with suit colours, helmets, goggles and masks (above Camp 2 every real
  climber's face is covered, so no face work is needed). They move along the route with the **same physiology model**
  as the player. This is the thesis's **virtual climber** (block B1) put to work in the game: their speed, turnarounds
  and collapses come out of the model, not out of a script.

### 3.4 The parents' ghosts [Established]

Father and mother, in the storms above Camp 3, sometimes guiding true, sometimes luring off the route (Phase 1 plan).
With a partner beside you there is a new test: **Nima cannot see them.** When he can, late on summit night, the player
knows the myth has become real.

### 3.5 Background people

| Where | Who | Built as |
|---|---|---|
| Dingboche | the priest, Sangye's aunt who runs a teahouse, yak herders | 3 village NPCs with idle and talk animations |
| The trail to Base Camp | porters with loads, yak trains (bells), trekkers coming down with news ("the Icefall is bad this year") | path followers on the trail, a few lines each |
| Base Camp | cooks in the mess tent, the puja (blessing) ceremony, other teams' tents with lights and voices | mostly sound, lights and a few idle figures |
| Everywhere | rescue helicopters (sound and a far silhouette), radio chatter between other teams | sound |

### 3.6 How people talk [Proposed]

- **Subtitles** bottom centre, no box (the HUD brief of 2026-11-04). **Radio** lines get a crackle and an icon.
- **Barks by state:** Nima reacts to the player's body ("Drink. Now."), to danger ("Wait. Listen."), to places
  ("My father's tent was there.").
- **Voice:** text plus breath, grunt and laughter sounds first; voice acting later (sound rule: the author judges by ear).
- All lines live in one data table with conditions (place, time, stat, story flag), so writing more lines is cheap.

---

## 4. The systems the chapters stand on

Each table says what exists today **[Measured]**, what changes **[Proposed]**, and which decision the change creates.
A change that creates no decision is not worth building.

### 4.1 The body: the survival indexes

| Index | Today [Measured] | Proposed change | The decision it creates |
|---|---|---|---|
| Health 0-100 | damage from starvation 0.5/s, dehydration 0.8/s, hypothermia 0.8/s, hypoxia 0.6/s after 10 s grace; regenerates 0.2/s when fed and watered | add injuries (4.9) | push on hurt, or rest and lose the weather |
| Stamina 0-100 | sprint −12/s, uphill −20/s per unit of grade; at 0 only sprint is blocked | at 0, walking slows to ×0.6 until 20; **rest-step** (4.8) | rush and stall, or keep a rhythm |
| Satiety, hydration | walking empties them in 1.85 h / 1.23 h; **camps refill both for free** | no free refill: rations from stock, water only from the stove (4.4); the Western Cwm's sun doubles water loss 10:00-15:00 | carry food or gas; drink now or later |
| Body temperature | drifts towards a target set by altitude, storm and realm; cold warning 35 °C, damage 33 °C; camps warm ×4 | add **heat** in the Western Cwm (sun, little wind: towards 38.5 °C, water ×2, stamina recovery ×0.5); a warm drink +0.5 °C; the tent and sleeping bag at night | walk the Cwm at dawn in the cold, or at noon in the oven |
| Oxygen (SpO₂) | physiology model: pressure → alveolar gas → saturation; effort widens the gap; bottle = 2,500 m lower | oxygen flow dial (4.5); SpO₂ dips during sleep | how much to breathe, and when |
| Sanity 0-100 | drains with altitude, low SpO₂, night, wind (bug: always on), realms; hallucinations below 60, ghosts below 30 | fix the wind bug; **company restores it** (Nima within 25 m +0.5/min, each radio call +3) | stay with people, or go alone and pay for it |
| **Fatigue** (new) 0-100 | not found | rises with time awake (+4 per game hour) and effort; sleep removes it, less high up (×0.6 at Camp 2, ×0.4 at Camp 3 and the South Col, ×0.8 sleeping on oxygen); high fatigue lowers maximum stamina, makes ladders sway more and brings hallucinations sooner | sleep longer and lose the window, or go tired |
| **Altitude illness** (new) | not found (only the SpO₂ thresholds) | from the physiology model: climbing far above the adapted altitude for hours brings AMS (headache), then HACE (confused controls, unsteady walk) or HAPE (cough, SpO₂ −10 at rest). Cure: go down 500-1,000 m, oxygen, medicine. The same model runs for Nima and the other team | ignore the headache, or turn back |

### 4.2 The body sets the pace [Proposed]

Today low oxygen slows walking to at most ×0.7 (below SpO₂ 72, floor at 40). Proposed: **×1 at SpO₂ 80 down to ×0.35 at
35**, so the climb above 7,000 m takes as long and hurts as much as it should. Targets: about 400-600 m of height per
hour at 7,000-8,000 m on oxygen, 150-250 m per hour without oxygen above 8,000 m. That makes leg 6 (Camp 3 → South Col)
about 40 minutes and leg 7 (to the summit) about 75 minutes of walking (section 2.5). Below 6,000 m nothing changes.

### 4.3 Acclimatisation comes from nights [Proposed]

Today the adapted altitude rises 5 m per real second while resting in a camp (Camp 2 in about 220 seconds) and
sleeping gives nothing. That makes rotations pointless. Proposed: the adapted altitude rises **only while sleeping**,
towards *(highest point reached in the last 48 game hours − 300 m)*, capped at 7,000 m, by 150-250 m per night: more
when you sleep lower than you climbed ("climb high, sleep low"), less when you sleep at the highest point. It falls
about 50 m per game day spent more than 1,000 m below it. **Without the two rotations no player can reach the South
Col in a healthy state.** The numbers are placeholders; the curve comes from the thesis model
(`Docs/Thesis/Physiology_Model.md`).

### 4.4 Supplies: camp stocks, load carries, the stove [Proposed]

Today camps refill food and water for free, pickups come back after 10-15 minutes, and the stove and gas do nothing.
Proposed:
- **Every camp has a stock**: food, gas, oxygen bottles, medicine. It is shown when you arrive. Nothing comes back by
  itself.
- **Stocks grow by load carries**: what you and Nima carry up and leave on the rotations, plus what the Sherpa team
  carries for you, arranged at Base Camp within a budget (you choose the priorities).
- **Water comes from the stove**: melting snow for one thermos costs 15 game minutes and an eighth of a gas
  cartridge. No gas means no water, and no water is a real reason to go down.
- The other team has its own stocks; borrowing or trading depends on goodwill (section 3.3).
- Weights: base gear 10 kg, ration 0.6 kg, water 1.0 kg, bottle 3.0 kg [Measured]; new: gas cartridge 0.45 kg, medicine
  kit 0.3 kg. Comfortable up to 16 kg, at most 26 kg [Measured].

### 4.5 Oxygen: the flow dial [Proposed]

Today a bottle lasts 1,800 real seconds, acts as 2,500 m lower, and turns on by itself below SpO₂ 58. Proposed: a flow
dial in the pack and on a key: **0.5 / 1 / 2 / 3 / 4 litres per minute**, about 1,000 / 1,700 / 2,500 / 3,000 / 3,500 m
lower, and the bottle lasts 120 / 60 / 30 / 20 / 15 real minutes. Sleep on 0.5 at Camp 3 and the South Col. In storms
the regulator can ice up (the hiss stops; knock it clear or swap it). Summit night (100 minutes) at 2 l/min needs
three to four bottles: you carry three at most (9 kg), Nima two if he is with you, plus whatever you cached at the
Balcony. Every bottle is a decision.

### 4.6 The clock and sleep [Proposed]

**6 game minutes per real minute while awake** (today about 24). Sleep in a camp: choose the wake-up time (02:00 for an
Icefall start, 21:00 for summit night). Rest days at Base Camp skip whole days, while the forecast changes. Night means
cold and sanity loss [Measured] and safer seracs (the serac clock runs ×0.7 before 08:00 and ×1.7 from 11:00 to 18:00
[Measured]).

### 4.7 Weather: the forecast and the realms

**[Established]** Realms are this world's weather: random, about 10-12 minutes each, some harmless, some deadly.
**[Proposed]** Two layers:
- **The forecast (days ahead).** At the 07:00 and 19:00 radio calls Dr. Hale reads the next three days: which realms are
  likely, the summit wind, and how sure she is. **The window** is two or more days of harmless realms with summit wind
  under 40 km/h. Choosing which window to use is the biggest decision of Act III.
- **The passing realms (minutes ahead).** The existing wheel and warning. Their frequency and danger grow with altitude
  and act: harmless and risky realms only in Act II; the deadly ones (Jotunheim, Svartalfheim, and Muspelheim, which
  kills an unsheltered climber in about 5.5 minutes) only from Act III, and the forecast says which are likely today.
  Walking out into a "Muspelheim day" is the player's choice, not bad luck.
- Today [Measured]: first realm after 240 s; warning 150 s if dangerous, 45 s if harmless; 600-720 s each; 360-600 s of
  ordinary weather between; shelter blocks every effect; all nine together within 600 m of the summit.

### 4.8 Moving on the mountain

| Mechanic | Today [Measured] | Proposed | Decision |
|---|---|---|---|
| Fixed ropes | clip on by itself within 4 m, off at 6 m; roped: 58° walkable, no deadly fall, hidden crevasses harmless | **clip by hand at every anchor** (every 10-25 m): unclip, re-clip, about 1 s, longer in mitts or with frostbitten fingers; a missed clip leaves you unroped until the next anchor | rush the clips, or be safe and slow |
| Ropes on the trek | ropes run along legs 0 and 1 too | remove them (there are none in reality); cairns, prayer flags and the yak path guide | none (realism) |
| Rope ascending | not built | on the Lhotse Face and the Geneva Spur: push the jumar, step, step, breathe; a steady rhythm saves stamina, rushing burns it | speed against breath |
| Descending | as walking, no bonus | arm-wrap below 35°, rappel on steep ice: ×2 speed with a brake input; too fast is a slip | fast and risky, or slow and cold |
| Ladders | ×0.45 speed, sway 0.8° + 2.5° in wind, a fall kills | tied double ladders in later passes, crampon points catching (timing), gusts, Nima holding the far end when present | cross now in the gust, or wait |
| Self-arrest | not found | unroped slip on snow or ice: an ice-axe timing input to stop | (skill) |
| Own lines | the climbing component handles faces from 48° (rock 70 cm/s; hard snow ×0.85, ice ×0.7, soft snow ×0.45) | leave the rope wherever the terrain allows (principle 3): shortcuts, caches, views, falls | the safe line or your own |
| Rest-step | not found | hold a key: ×0.6 speed, ×0.3 stamina cost, steady breath; the real high-altitude technique | speed or reserve |

### 4.9 Injuries and illness [Proposed]

- **Frostbite** (fingers, toes): long cold plus gloves off (cooking, fast clipping in liners). Fingers clip and use items
  slower for the rest of the game, and the ending scenes show the hands.
- **Snow blindness:** goggles off in glare. The view blurs for hours.
- **Sprain** after a fall: ×0.8 speed until a night's rest.
- **AMS, HACE, HAPE** from the physiology model (4.1).
- **The medicine kit:** dexamethasone (HACE, two doses), nifedipine (HAPE, two doses), painkillers. Who gets the doses
  is part of the story.

### 4.10 People systems [Proposed]

- **Route followers:** NPC climbers move along `route.json` (the same line the route check walks) with a sideways
  offset, the same speed rules (grade, oxygen, storm) and foot traces onto the ground (like the ghosts today).
  Animations on the climber skeleton already: walk and jog in 8 directions, the Mixamo climbing set (up and down wall,
  slope, hang, shimmy, top out). New: rope ascending, rest-step, sitting, talking gestures, an injured walk, carrying.
- **Nima's states:** lead · follow · wait at an anchor · rest · help (catch, short-rope, hold the ladder) · sick ·
  refuse. Leading he stays 8-25 m ahead; he waits when the player is more than 40 m behind and comes back if the player
  stops for long.
- **Trust** (Nima, Dr. Hale), 0-100: rises with safe choices and help given, falls with ignored warnings and broken
  turnaround times. Below 30, Nima refuses the next stage and Dr. Hale stops the forecasts.
- **The other team:** four to six route followers on their own schedule with the same body model (the virtual climber,
  block B1). Queues, help and accidents come out of it.
- **Lines:** one data table with conditions (section 3.6).

### 4.11 Death, failure and recovery

**[Measured] today:** on death, a death screen; "return" puts you at the last camp or trail checkpoint (four exist:
Icefall halfway, Lhotse Face halfway, Yellow Band, Balcony) with every stat reset to full; the pack rolls back; clues
and acclimatisation stay.
**[Proposed] failure forward first:** collapsing from exhaustion, cold or HACE with Nima close by means he short-ropes or
drags you to the last camp. You live, the day is lost, supplies are used, and a mark stays (frostbite, his trust, a
journal page). Without Nima it is death, as today. A return to a checkpoint restores the body **as it was at the
checkpoint**, not full, so pushing on with low reserves is a real decision; a checkpoint that would load into a lethal
state is refused (Phase 1 Q2).

### 4.12 The climb journal: the player's novel [Proposed, principle 4]

- **Every night in camp** (or at a forced bivouac) Sangye writes one page. The page is put together from the day's
  event log (the session monitor already records every event): where he went, the worst moment (lowest SpO₂, the
  closest call), the decisions and the reasons the game knows, who was with him, the weather, the oximeter and
  thermometer readings, what he found, and a closing line of feeling picked by the day's tension and his sanity.
- **Sangye's voice,** built from sentence templates (dozens of variants per kind of event, joined by rules), in the
  hand-written font already in the game (Caveat). No AI text generation inside the game: deterministic, offline,
  translatable.
- **Example** (rotation 1, after a near miss): *"Day 5. Left Base Camp at three with Nima's light ahead of me. The ice
  talked all night. At the Popcorn field a tower came down behind us, close enough that the snow hit my neck. Nima said
  nothing. He just looked at me and kept walking. Camp 1 at ten. 71% on the clip. My head is a drum. Tomorrow we go
  down, before the sun wakes the seracs."*
- **One book:** the Journal in the pack opens it. The parents' pages (the found clues) are glued in between Sangye's own
  days, so his story and theirs sit side by side.
- **At the end:** the whole book, the summit picture and the decisions that shaped the ending. Later: export as
  "My climb" (text or PDF).

### 4.13 Facts in today's code that block this design [Measured, 2026-11-14]

| Fact | Why it blocks | Fix (section) |
|---|---|---|
| Camps refill food and water for free; pickups come back after 10-15 min | no supply decisions | camp stocks (4.4) |
| Acclimatisation in about 220 s of standing in a camp; sleep gives none | rotations pointless | nights (4.3) |
| 24 game minutes per real minute | nights fall in the middle of chapters | 6 per minute (4.6) |
| Exhaustion does not slow walking; no fatigue | no reason to rest | fatigue, rest-step (4.1, 4.8) |
| Stove and gas have no mechanics | water is free | the stove (4.4) |
| Sanity's wind drain is always on (wind is 0-10, threshold 0.6) | sanity falls everywhere | bug fix (4.1) |
| Fixed ropes on the trek | unreal, and a rail | remove (4.8) |
| Death resets every stat to full | pushing on low costs nothing | restore the checkpoint state (4.11) |
| Name labels differ from measured heights (EBC 5,364 vs 5,274 m; Camp 3 7,160 vs 7,049 m); some screens use the labels | numbers disagree | one source for every height |
| The journal counts every clue as a page | the book mixes things | parents' pages apart (4.12) |
| The realm schedule ignores altitude and act | deadly weather at Dingboche | 4.7 |

---

## 5. The chapters

Every chapter has the same parts. **Beats** are timed in real minutes from the chapter's start and marked **F** (fixed),
**S** (systemic) or **C** (chosen), as in section 1.5. Heights and positions marked [Measured] come from route.json, the
route check of 2026-11-14 and the level; places marked [Open] must be measured from real coordinates before they are
built (rule from 2026-11-02: never invent a position or a height).

The five acts are the original story chapters [Established]: I Origin, II The Mortal Realm, III The Path of
Hallucination, IV The Myth Revealed, V Final Judgement.

---

### ACT I · ORIGIN

### Chapter 1 · The Compass

| | |
|---|---|
| Where | Dingboche (spawn camp, ground ≈ 4,336 m [Measured]; the label says 4,410) and a few hundred metres of village |
| Time | 40 min (walk 5, other 35) · game day 1, 07:50 → night |
| Company | crowd |
| Goal | find the compass in the parents' things, hear the priest, pack, sleep before leaving |

**The body.** Nothing presses yet. The oximeter is introduced: about 84% resting at Dingboche [Measured, physiology
model]. Breath and heartbeat are calm; the player learns what "normal" looks like.

**Decisions**
1. **Pack within the first limit.** Choose three of: two extra rations, an extra thermos, the medicine kit, a down layer
   (body temperature target +1 °C), spare gloves (protects against frostbite once), the parents' photo (+sanity at each
   camp). Consequences show over the next three chapters (the medicine kit decides Chapter 2's sick trekker; the gloves
   save fingers in Chapter 13).
2. **Tell the aunt the truth or not.** Truth: she gives a khata (the white scarf, blessed at the puja; +sanity when
   worn) and phones Dr. Hale, who is warmer to Sangye from the start (+trust). A lie: no scarf; Dr. Hale learns it later
   (−trust in Chapter 4).
3. **Optional, outside the budget:** the acclimatisation walk up the Nangkartshang ridge (+25 min [Open: path and
   height to measure]): +adaptation, the first view of the summit plume and the route.

**Beats**

| Min | Beat | |
|---|---|---|
| 0 | Wake in the parents' old room: dust, a box of their belongings | F |
| 3 | The compass. Pointed at the summit, the golden tree-vein mark burns in Sangye's palm | F |
| 6 | Letters and photos; the memory of the refused last hug | C |
| 12 | The aunt's teahouse: warm light, trekkers talking about "the Icefall this year"; the aunt has found Sangye a place in Dr. Hale's small physiology study at Base Camp (the in-world reason he is allowed on the mountain, and why he wears an oximeter) | F |
| 20 | The priest at the gompa: the withering tree, the parents as "fuel", "do not touch the spine of the sky" | F |
| 27 | The village shrine and its taboo inscription | C |
| 30 | Packing at home (decision 1) | C |
| 36 | Dusk: the summit plume lit pink above the valley. The first journal page is written | F |

**Dangers.** None. A safe chapter.
**People.** The aunt (who runs the teahouse), the priest, trekkers, yak herders.
**Story.** The whole hook [Established].
**New for the player.** Moving, interacting, reading, the pack, the oximeter, the compass (Q), the journal.
**Ends on the question.** *The priest says they are not dead. Then what are they?*
**Build.** Village houses and the teahouse interior (Fab Himalayan village kit; Dingboche terrain exists), the aunt and
priest (one character base, idle and talk animations), the palm mark (decal on the first-person hand), packing choices
in the existing pack screen. Compass and pack exist.

---

### ACT II · THE MORTAL REALM (4,300-6,100 m)

### Chapter 2 · The Memorial Pass

| | |
|---|---|
| Where | Leg 0 from Dingboche to Lobuche. Leg 0 as a whole: 11.49 km, +928 / −104 m, 4,335 → 5,158 m, slope mean 11.7°, max 34.8° [Measured]. Thukla memorials and Lobuche: [Open] (real Lobuche ≈ 4,940 m; the point on leg 0 at that height to be measured) |
| Time | 85 min (walk 70, other 15) · game day 2, 07:00 → 15:30 |
| Company | crowd (porters, yaks, trekkers), then alone on the pass in the snow |
| Goal | reach Lobuche before the afternoon snow |

**The body.** The first real altitude: SpO₂ about 80 resting. Water empties in a little over an hour of walking
[Measured], so the Thukla teahouse refill matters. Ascend too fast and AMS starts (a headache that grows). Cold comes
with the afternoon cloud.

**Decisions**
1. **Stop at Lobuche or push on to Gorak Shep today.** Stopping keeps the ascent safe. Pushing saves a game day but
   triples the AMS risk, and the Chapter 2 night scene is missed.
2. **The sick trekker at Lobuche** (if you stop): a young trekker stumbling and confused, failing the heel-to-toe test
   (HACE). Give one of your two dexamethasone doses (if you packed the kit); walk him down to Thukla in the dark (three
   game hours of sleep lost, cold, fatigue, but he lives); or leave him to the teahouse owner (his fate is told later and
   depends on chance).
3. **Yaks on the narrow trail:** step to the mountain side or the valley side.
4. **Cloud on the pass:** wait in the Thukla teahouse, or push on navigating by cairns and prayer flags.

**Beats**

| Min | Beat | |
|---|---|---|
| 0 | Dawn in Dingboche, frost, yak bells | F |
| 5 | Porters overtake with huge baskets; trekkers coming down with a rumour: someone died in the Icefall last week | S |
| 15 | The Thukla teahouse: water, tea with a porter (+sanity) | C |
| 25 | A yak train on a narrow ledge (decision 3) | S |
| 35 | **The Thukla memorials:** dozens of stone chortens and prayer flags with climbers' names. One stone carries **the parents' names**, set up by the village twenty years ago. The compass trembles | F |
| 45 | Cloud climbs the valley before it arrives (art direction); a realm warning (only harmless or risky realms in Act II); the trail fades in new snow; nobody else around | S |
| 60 | AMS headache if the ascent was too fast | S |
| 70 | Lobuche: teahouse lights in the dusk, people, warmth | F |
| 78 | The sick trekker (decision 2) | S/C |
| 85 | Sleep; journal page | F |

**Dangers**

| Danger | Cue | Counter | Consequence |
|---|---|---|---|
| Yak on a ledge | bells, hooves | stand on the mountain side | knocked down the slope: fall damage |
| Rockfall below the lateral moraine (new) | clatter, dust | wait, then move fast | damage |
| AMS → HACE | headache, then unsteady walk and drifting controls | stop, descend, medicine | forced descent the next day |
| Losing the trail in cloud | cairns disappear | compass, prayer flags, wait | lost time, cold |

**People.** Porters, yak herders, trekkers, the Thukla and Lobuche teahouse keepers, the sick trekker.
**Story.** The memorial stone: the village believes they are dead; the compass disagrees [Proposed].
**New for the player.** Water and refills, the AMS headache, reading weather, yak etiquette.
**Ends on the question.** *They buried my parents without bodies. The compass points up. Who is right?*
**Build.** Remove the trek's fixed ropes and add cairns and prayer flags; Thukla memorials (chorten meshes); Lobuche as
a new teahouse camp; yak trains and porters as route followers on leg 0 (Fab yak); the trekker; AMS/HACE (4.1); the
heel-to-toe test.

### Chapter 3 · Their Tent

| | |
|---|---|
| Where | The rest of leg 0 from Lobuche to Gorak Shep (camp ≈ 5,160 m [Measured]), then leg 1: 3.02 km, +149 / −31 m, 5,159 → 5,278 m [Measured]. The parents' camp: leg 1 at 62%, 35 m right of the route, about 13.4 km from Dingboche [Measured] |
| Time | 85 min (walk 70, other 15) · game day 3, 08:30 → 17:00 (Base Camp at dusk) |
| Company | crowd at Gorak Shep, then **partner**: Nima joins |
| Goal | reach Base Camp; find the parents' camp |

**The body.** SpO₂ about 76-79. Moraine and glacier walking; cold at Gorak Shep; water.

**Decisions**
1. **Kala Patthar** (optional, +30 min, outside the budget [Open: path and height]): at dawn, the whole route from above:
   the Icefall, the Cwm, the far realm weather. Adaptation +, a "route seen" bonus (the serac zones' pickets visible
   later), Nima: "My father liked to look from here." Costs light: Base Camp after dark.
2. **At the parents' camp: take the father's ice axe** (1.2 kg for the rest of the game; it is the tool of the Mortal's
   Strike ending) or leave it at the memorial. Mother's journal page 1 is always taken.
3. **Tell Nima the real reason** (the parents, the compass) or "just climbing". The truth builds trust; a lie he learns
   from Dr. Hale later costs it.

**Beats**

| Min | Beat | |
|---|---|---|
| 0 | Lobuche at dawn, frozen bottles | F |
| 10 | Moraine, the first glacier ice, rockfall from the lateral moraine | S |
| 30 | Gorak Shep: last teahouses. Nima waiting outside: "Dr. Hale sent me. My father was Lhakpa." | F |
| 35 | Kala Patthar (decision 1) | C |
| 45 | The Khumbu glacier; the Icefall towers ahead, groaning; Nima teaches "bistarai" | F |
| 50 | Frozen melt ponds across the path (the glacier material already draws them) | S |
| 55 | **The parents' camp:** the torn twenty-year-old tent (clue 1, exists); the compass swings to the summit once (exists); Nima goes silent (decision 2) | F |
| 70 | **Base Camp at dusk:** tents glowing, the Icefall lit pink above (Phase 1 cinematic); Dr. Hale meets them | F |

**Dangers**

| Danger | Cue | Counter | Consequence |
|---|---|---|---|
| Thin ice on a melt pond (new) | turquoise ice, a crack | go around | soaked boots: body temperature target −2 °C until dried in camp; frostbite risk |
| Moraine rockfall | clatter | wait, move fast | damage |

**People.** Nima (joins), teahouse keepers, the other team arriving at Base Camp with their yaks.
**Story.** The parents' camp and the compass twitch (both exist); Nima's father [Proposed].
**New for the player.** The partner, the glacier, the compass's pull.
**Ends on the question.** *Nima's father came down alone. What did he see up there?*
**Build.** Nima v1 (lead, follow, barks); melt-pond ice; the father's axe item; Kala Patthar path (optional).

### Chapter 4 · Base Camp

| | |
|---|---|
| Where | Everest Base Camp (ground ≈ 5,274 m [Measured]; the label says 5,364) |
| Time | 40 min (other 40) · game day 3 evening → day 4 |
| Company | crowd |
| Goal | permission to climb: the doctor's check, the puja, the plan, the stocking |

**The body.** SpO₂ about 76-79 at rest; the first night at 5,274 m; AMS check.

**Decisions**
1. **The medical check:** oximeter, pulse, heel-to-toe test. In person Dr. Hale measures herself; later checks come by
   radio, where the player can lie (section 3.2).
2. **The load plan:** the Sherpa team can carry a limited number of loads (for example 12) to Camps 1-4 before the
   summit push. Split them: oxygen to the South Col, gas to Camp 3, food to Camp 2... Plus what you and Nima carry
   yourselves on the rotations (4.4).
3. **The pack split with Nima:** who carries what. If he is hurt later, what he carries is at risk.
4. **At the puja:** what to have blessed (the khata, the father's axe, the photo): small sanity and trust effects.

**Beats**

| Min | Beat | |
|---|---|---|
| 0 | Arrival: the mess tent dinner, cooks, other teams' lights | F |
| 5 | Dr. Hale's medical tent: oximeter, pulse, heel-to-toe; she explains her study; she sees the compass's runes: "Old Norse? Here?" | F |
| 15 | Dawn: **the puja**: juniper smoke, prayer flags strung over camp, the lama, rice thrown; the other team attends: Marek, Elena, Ji-woo | F |
| 25 | The plan board: rotations, loads (decision 2), the first three-day forecast | C |
| 32 | Walk the camp: the Icefall doctors' tent ("the ladders at the Popcorn field moved"), the memorial for last week's death (the rumour was true) | C/S |
| 40 | Sleep; the alarm set for 02:30 | F |

**Dangers.** Only the cold night.
**People.** Dr. Hale, cooks, the lama, the other team, the Icefall doctors.
**Story.** Dr. Hale and the Eddas: the first adult who takes the runes seriously, as a story.
**New for the player.** The plan board, load carries, the forecast, the radio.
**Ends on the question.** *Three in the morning. The Icefall.*
**Build.** Base Camp interactions (tents and props exist), Dr. Hale, the puja scene, plan and load screen, forecast
screen, idle figures of the other team.

### Chapter 5 · The Icefall (rotation 1, up)

| | |
|---|---|
| Where | Legs 2 and 3, Base Camp → Icefall camp (≈ 5,723 m) → Camp 1 (≈ 6,134 m; label 6,065): 4.75 km, +929 / −74 m [Measured]. Six ladders: leg 2 at 45 / 66 / 84% (gaps 3.0 / 3.6 / 2.6 m), leg 3 at 22 / 58 / 76% (3.2 / 4.0 / 2.8 m). Serac zones: leg 2 at about 56%, leg 3 at about 47%. Checkpoint "Icefall - halfway" at leg 3 50%. Wood fibre at leg 3 about 0.8 km (≈ 5,760 m). Ice cave 40-60 m off leg 3 between 55 and 80%. Six hidden crevasses 18-35 m off leg 3's rope [all Measured] |
| Time | 70 min (walk 50, other 20) · game day 5, 03:00 → 10:00 |
| Company | partner (and the other team's headlamps ahead) |
| Goal | reach Camp 1 before the sun warms the seracs; carry a first load to Camp 1 |

**The body.** About −20 °C at 03:00 (body temperature), SpO₂ about 70-75 while climbing, stamina on the steep ice,
cold fingers at every clip.

**Decisions**
1. **Start time** (the alarm set in Chapter 4): 02:00-04:00 in the cold and dark with safer ice, or 06:00 with light and
   warmth but later in the serac zones (their clock runs ×0.7 before 08:00 and ×1.7 from 11:00 [Measured]).
2. **Nima's line or your own:** a shorter line through a serac corridor saves about 8 minutes, unroped, across hidden
   crevasses.
3. **The ice cave** (optional): unrope across the hidden-crevasse ground for its cache: 2 rations, 1 water, 1 oxygen
   bottle and the Korean 2006 expedition's runes [Measured].
4. **Traffic at a ladder:** wait inside the serac zone (the timer runs) or pass on an exposed snow bridge.

**Beats**

| Min | Beat | |
|---|---|---|
| 0 | Juniper fire at the Icefall entrance; Nima prays; headlamps | F |
| 5 | Ladder 1 (3.0 m): Nima holds the far end; clipping at anchors taught | S |
| 12 | Serac zone 1: red pickets, 30 s before "MOVE!" (exists); Nima: "Don't stop here." | S |
| 18 | Ladder 2 (3.6 m) with the other team queuing (decision 4) | S |
| 24 | Ladder 3 (2.6 m) | S |
| 26 | The Icefall camp, "the Football Field": drink, eat; sunrise on Nuptse | F |
| 30 | **The wood fibre** "This is not rock" (exists). Nima: "Old ice-rock. My father said the mountain has bones." | F |
| 33 | Ladder 4 (3.2 m) | S |
| 35 | The ice cave (decision 3) | C |
| 40 | Serac zone 2; checkpoint at halfway | S |
| 44 | Ladder 5 (4.0 m, the longest in the Icefall) | S |
| 48 | Ladder 6 (2.8 m) | S |
| 52 | A serac collapses behind them, close enough that the snow hits their necks; if they are late, it falls on the route ahead and forces a detour | F/S |
| 62 | The top of the Icefall; Camp 1 on the flat snow; the heat begins | F |
| 70 | Drop the load (Camp 1 stock +); the tent | F |

**Dangers.** Serac zones, ladders, hidden crevasses off the rope (all exist); falling ice chips; frostbite at the
clips in liner gloves (new).
**People.** Nima (teaches, then goes quiet); the other team (Ji-woo overtakes recklessly, Marek is irritated).
**Story.** The wood fibre; the cave's runes (both exist).
**New for the player.** Clipping at anchors, ladders, serac timing, load carries.
**Ends on the question.** *First night above 6,000 m.*
**Build.** Manual clipping (4.8), Nima at ladders, the other team as route followers, the near-miss collapse, and a
fix for the two points too steep to stand on at the Icefall camp approach (leg 2 at 2,312 m, 51°; leg 3 at 0 m, 50°
[Measured]).

### Chapter 6 · First Night High

| | |
|---|---|
| Where | Camp 1 (≈ 6,134 m [Measured]; 3 oxygen bottles there today), then down legs 3 and 2 to Base Camp |
| Time | 45 min (walk 25, other 20) · game day 5, 10:00 → day 6, 07:30 |
| Company | partner, the other team at Camp 1, Dr. Hale on the radio |
| Goal | survive the first night high; get down safely |

**The body.** The Cwm's sun turns the tent into an oven by day (heat, 4.1); at night SpO₂ dips and periodic breathing
wakes Sangye gasping (screen effect); a headache (AMS); sleep recovers fatigue badly; the stove decides the water.

**Decisions**
1. **The day at Camp 1:** rest in the hot tent, or walk a little way into the Cwm (climb high: more adaptation; heat;
   hidden crevasses).
2. **The headache:** a painkiller hides it, and hiding AMS is how people die (the illness keeps growing unseen).
3. **When to go down:** 05:00 on frozen, safer ice, or after the sun at 09:00: warmer, but later in the serac zones.
4. **The other team's sick client:** Marek wants to push him up; Elena (a doctor) asks for your opinion. Your answer
   changes what happens to that client.

**Beats**

| Min | Beat | |
|---|---|---|
| 0 | The tent; melting snow for the first time (gas, 15 game minutes per thermos) | F |
| 6 | The sun: the tent becomes an oven | S |
| 10 | A walk into the Cwm (decision 1) | C |
| 14 | The 19:00 radio call: the first remote check (honest or lie) and the forecast: a dark realm in three days | F |
| 16 | Night: periodic breathing, Nima snoring, a distant avalanche (exists), the compass glowing faintly | S |
| 18 | Journal page by headlamp | F |
| 20 | 04:30: frozen boots; the descent | F |
| 25 | Arm-wrap and rappel taught; ladders downwards (harder: you look down) | S |
| 40 | The serac zones on the way down, timed by decision 3 | S |
| 45 | Base Camp: breakfast | F |

**Dangers.** Heat and dehydration by day, cold at night if the stove fails, slips on the descent, seracs.
**People.** Nima tells the first small piece about his father's silence; the other team; Dr. Hale.
**New for the player.** The stove and water, the night radio check, sleeping high, descending on the ropes.
**Ends on the question.** *Three days to rest. A dark realm is coming.*
**Build.** Stove screen, heat, sleep effects, rappel and arm-wrap, radio-call screen.

### Chapter 7 · Thick Air

| | |
|---|---|
| Where | Base Camp, or (condensed) down to Dingboche |
| Time | 25 min (other 25) · game days 7-9 |
| Company | crowd |
| Goal | recover, plan rotation 2, live in Base Camp |

**The body.** Fatigue and health recover; the adapted altitude falls slowly if you go far down (4.3), but thick air
heals faster.

**Decisions**
1. **Rest at Base Camp** (one to three days) **or go down to Dingboche** for thick air (better recovery, the aunt's
   teahouse, company), costing two extra days, and the window may pass.
2. **Re-plan the loads** (more oxygen high, or more food).
3. **Help the Icefall doctors** rebuild a collapsed section: one rest day spent, but the changed route in Chapter 8 is
   known (safer) and the doctors owe you.

**Scenes** (the order follows the forecast and the player's choices)

| Scene | |
|---|---|
| A realm storm hits Base Camp at night: hold the mess tent's poles with the cooks | S |
| A powder cloud from an avalanche on Nuptse sweeps over camp: shelter in a tent; frightening, nobody of the cast dies | S |
| Nima's radio call home to his wife and daughter in Pangboche | F |
| Dr. Hale's tent: the Eddas, and she sees the palm mark; her certainty cracks | C |
| The next forecast: when rotation 2 can go | F |

**Ends on the question.** *Rotation 2: Camp 2, and a look at the Lhotse Face.*
**Build.** Rest-day screen (skip days with events), Base Camp scenes, powder-cloud effect, the tent-holding event.

---

### ACT III · THE PATH OF HALLUCINATION (6,100-7,900 m)

### Chapter 8 · The Icefall, Changed (rotation 2, up)

| | |
|---|---|
| Where | Legs 2 and 3 again, Base Camp → Camp 1 (same numbers as Chapter 5) |
| Time | 65 min (walk 50, other 15) · game day 10, 03:00 → 09:30 |
| Company | partner, heavier traffic from the other team |
| Goal | reach Camp 1 with a heavier load (an oxygen bottle for Camp 2) |

**What changed** (a glacier moves about a metre a day): a section near the Popcorn field collapsed and the Icefall
doctors re-routed it with a **tied double ladder** over a 6-7 m crevasse (more sway); an old ladder hangs broken in the
void; a new crevasse opened near the Icefall camp; in the collapse debris lies a second, larger piece of wood: a
carbonised branch with growth rings. Nima says nothing this time. If the player helped the Icefall doctors in
Chapter 7, they know the new line and the double ladder has a guide rope.

**The body.** The heavier pack (22.8 kg with three bottles walks at ×0.83 [Measured]), cold, SpO₂.

**Decisions**
1. **Carry the bottle for Camp 2** (3 kg: slower, more tired) or leave it to the Sherpa team (uses one of the load
   budget).
2. **The double ladder in a gust:** cross now, or wait while the serac zone behind you runs its clock.
3. **A rescue team coming down** with a stretcher: step aside onto a narrow ledge (unclip, risk) or wait clipped (time,
   cold).

**Beats**

| Min | Beat | |
|---|---|---|
| 0 | The Icefall entrance again; it looks different in the headlamp | F |
| 8 | The first changed section; the broken old ladder hanging below | F |
| 20 | Serac zone 1 with traffic | S |
| 26 | The Icefall camp; the new crevasse | F |
| 32 | **The double ladder** (decision 2) | F/S |
| 38 | The carbonised branch in the debris | F |
| 40 | Serac zone 2 | S |
| 45 | The rescue team (decision 3) | S |
| 55 | The top | F |
| 65 | Camp 1: load swap, short rest; Ji-woo's bravado | F |

**Dangers.** As in Chapter 5, plus the double ladder and the narrow ledge.
**Story.** The branch: the mountain is wood.
**New for the player.** The Icefall is alive; nothing stays the same.
**Ends on the question.** *The Western Cwm: the valley that talks.*
**Build.** The double ladder, an **Icefall state per pass** (data that swaps ladders, seracs and crevasses between
passes, section 8), stretcher-carrying figures.

### Chapter 9 · The Western Cwm

| | |
|---|---|
| Where | Leg 4, Camp 1 → Camp 2 (≈ 6,454 m; label 6,400): 2.16 km, +321 m, slope mean 9.8°, max 17.4°. One ladder at 30% (4.4 m, the longest on the route). Eight hidden crevasses. The rumble area "Rumble_WesternCwm" covers the leg (a boom every 45-100 s) [all Measured] |
| Time | 40 min (walk 22, other 18) · game day 10-11 |
| Company | partner, the other team; Camp 2 is busy (cooks, mess tents) |
| Goal | reach Camp 2, the advanced base camp |

**The body.** **Heat:** the Cwm is a solar oven at midday (body temperature up, water ×2), glare (snow blindness with the
goggles off; the goggles fog when you breathe hard), SpO₂ about 66-69 [Measured, computed], the booms wear at sanity.

**Decisions**
1. **Go now in the noon heat, or wait at Camp 1 until evening** (colder, firmer snow bridges, but walking into the
   night).
2. **Marek asks for help** fixing a rope across a crevasse field: an hour and some stamina now, for goodwill later (a
   bottle at the South Col, a place in their tent in a storm).
3. **Goggles:** fogged and half-blind, or off for a minute and risk snow blindness.

**Beats**

| Min | Beat | |
|---|---|---|
| 0 | Leaving Camp 1 | F |
| 5 | The 4.4 m ladder | S |
| 10 | The heat builds | S |
| 12 | **The first boom** inside the mountain: camera shake, the compass jolts (exists). "An avalanche?" Nima: "No avalanche sounds like that." | F |
| 18 | Marek's request (decision 2) | C |
| 25 | A Kovač Sherpa probes a snow bridge that collapses under his pole: everyone sees what is under the snow | S |
| 32 | More booms; sanity falls | S |
| 36 | The Lhotse Face fills the view: blue ice up to the sky | F |
| 40 | Camp 2: cooks, a mess tent, other teams. Company again | F |

**Dangers.** Hidden crevasses off the rope (exist), heat, glare.
**Story.** Níðhöggr's impacts [Established]; the compass shakes harder [Established].
**New for the player.** Heat, glare and goggles.
**Ends on the question.** *Tomorrow the Face.*
**Build.** Heat and glare, fogging goggles, Marek's request, cook and mess figures at Camp 2.

### Chapter 10 · The Lhotse Face (first touch)

| | |
|---|---|
| Where | Leg 5, Camp 2 → Camp 3 (≈ 7,049 m; label 7,160): 2.59 km, +600 m, slope mean 18.3°, max 41.6°. Checkpoint "Lhotse Face - halfway" [Measured]. Then back down to Camp 2 |
| Time | 55 min (walk 45: up 30, down 15; other 10) · game day 12, 05:00 → 14:00 |
| Company | partner |
| Goal | touch Camp 3 and come back down to sleep at Camp 2 (climb high, sleep low); stock Camp 3 |

**The body.** SpO₂ about 60 at Camp 3 [Measured, computed]; 40° blue ice; the jumar rhythm (new).

**Decisions**
1. **Sleep at Camp 3** (more adaptation, but risk, cold and supplies) **or come back to Camp 2** the same day (the
   default).
2. **Stock Camp 3** with two bottles (heavy now, saves Sherpa loads) or one.
3. **Ice falling from climbers above** (someone shouts "Ice!"): press into the slope, or unclip and step aside.

**Beats**

| Min | Beat | |
|---|---|---|
| 0 | The bergschrund at the foot of the face: a short steep wall (the climbing component) | F |
| 5 | Jumar rhythm taught: push, step, step, breathe | F |
| 12 | Falling ice (decision 3) | S |
| 18 | Checkpoint halfway; the Yellow Band visible far above | F |
| 28 | A queue on the ropes behind the other team | S |
| 30 | Camp 3: tents on ledges cut into a 40° slope. Nima: "At night, never leave the tent without clipping in." In the parents' old ledge, under ice: **the mother's third page** [Proposed]: *"Lhakpa wants to turn back. We cannot ask him to come further."* Nima reads it twice | F |
| 34 | Stock Camp 3 (decision 2); then down on the rappel | C |
| 50 | Camp 2 at dusk; the radio; the journal | F |

**Dangers.** A slip while unclipped on blue ice (self-arrest, 4.8), falling ice, cold.
**Story.** Page 3: Nima's father and the parents in one sentence.
**New for the player.** Rope ascending (jumar), the steep face.
**Ends on the question.** *Down to Base Camp, and the long wait.*
**Build.** The jumar mechanic, the bergschrund wall, falling ice, page 3 as a fifth clue.

### Chapter 11 · Retreat

| | |
|---|---|
| Where | Leg 4 down to Camp 1, then legs 3 and 2 down to Base Camp |
| Time | 50 min (walk 36, other 14) · game day 13, 07:00 → 13:00 |
| Company | partner, the other team |
| Goal | get down to Base Camp to recover before the summit push |

**The incident.** In the Cwm, Ji-woo unclips to pass a slow climber and a snow bridge breaks under him. He survives
on an ice ledge inside the crevasse with a broken leg.

**Decisions**
1. **Help haul him out** (a rope system: about ten game minutes and much stamina, Nima leads) **and get him to the
   helicopter landing at Camp 1**: two game hours lost, and the Icefall descent then happens in the afternoon, when the
   seracs are most active.
2. **Radio Dr. Hale and go on down**, leaving it to his team. Whether he lives is decided by their state (the virtual
   climber model), not by a script.

**Consequences.** Help: Marek owes you (a bottle or a tent place on the push), Elena trusts you, Nima's trust rises.
Leave: Marek is cold to you; if Ji-woo dies, his stone joins the memorial at Base Camp, and Sangye writes about it.

**Beats**

| Min | Beat | |
|---|---|---|
| 0 | Leaving Camp 2 | F |
| 8 | Ji-woo's fall | F |
| 10 | The rescue (decision 1) | C |
| 30 | A helicopter lands at Camp 1: rotor wash, noise, snow blown everywhere | S/F |
| 35 | The Icefall down in the heat | S |
| 50 | Base Camp | F |

**Dangers.** The crevasse edge (unclipped near it), the late Icefall.
**New for the player.** A rescue; a team's life in your hands.
**Ends on the question.** *Now we wait for the sky.*
**Build.** The haul interaction, a helicopter set piece (Fab model), an injured figure being carried.

### Chapter 12 · The Window

| | |
|---|---|
| Where | Base Camp (waiting days), then Base Camp → Camp 1 → Camp 2 in one night push (legs 2, 3 and 4 up) |
| Time | 90 min (walk 72, other 18) · game days 14-19; the push starts at 01:00 and reaches Camp 2 about 10:00 |
| Company | partner, Dr. Hale on the radio |
| Goal | choose the window and get to Camp 2 for the push |

**Decisions**
1. **Which window** (the biggest decision of Act III): the early window in two days (short, low wind for two days, the
   jet stream may come back; few teams) or the later one in five days (longer, but the other team and three more go
   then: crowds on summit night; worse realms as the season turns).
2. **The final pack and loads:** how many bottles each, medicine, gas.
3. **The other team's offer:** go on their schedule (shared rope fixers and an oxygen cache at the Balcony, but their
   pace and their crowd) or go alone with Nima.

**Beats**

| Min | Beat | |
|---|---|---|
| 0 | Waiting days in a few scenes: forecast boards, Dr. Hale and the Eddas, Nima's call home, Elena admitting she is afraid | F/S |
| 12 | The decision (decision 1) | C |
| 15 | 01:00: **the Icefall at night**, the fifth pass: only Nima's light ahead, the ice talking, a crack opening near a ladder | F/S |
| 45 | The Icefall camp | F |
| 60 | Camp 1 at dawn; no sleep, keep going (fatigue) | F |
| 70 | The Cwm at sunrise: cold and silent before the heat; the booms | S |
| 90 | Camp 2: rest | F |

**Dangers.** The Icefall in the dark, fatigue.
**New for the player.** Choosing the window.
**Ends on the question.** *The Face again tomorrow, and then nothing but sky.*
**Build.** The window screen (forecast with confidence), night lighting and the headlamp (B4), the other team's
schedule.

### Chapter 13 · Blue Ice

| | |
|---|---|
| Where | Leg 5 up again, Camp 2 → Camp 3 |
| Time | 60 min (walk 30, other 30) · game days 20-21: 05:00 → Camp 3 about 10:00 → a storm night |
| Company | partner; the radio dies in the storm (voice lost) |
| Goal | reach Camp 3 and survive the night |

**The body.** SpO₂ about 60; the first oxygen decision; cold at night on the face.

**Decisions**
1. **Start oxygen for sleeping at Camp 3** (0.5 l/min: better sleep and warmth) or save the bottles for summit night.
2. **The storm night** (a deadly realm, Jotunheim or Svartalfheim): the tent tears. Hold the poles all night (fatigue
   soars), dig in deeper, go down into the storm (could kill you), or crawl to the other team's tent (if there is
   goodwill).
3. **Nima's cough begins:** give nifedipine now (one of two doses) or wait and watch.

**Beats**

| Min | Beat | |
|---|---|---|
| 0 | Jumar again, faster now (the player has the rhythm) | F |
| 10 | **The frozen limb** in the blue ice, in clear morning light: huge, scaled, clawed (Phase 1 Q6). **Nima sees it too.** They look at each other; Nima prays. The compass shakes hard | F |
| 25 | Camp 3, the stove | F |
| 35 | The storm warning (the wheel, 150 s) | S |
| 40 | The storm night: the tent, the wind, **the radio goes silent** (solitude used as a tool), Nima coughing | S/C |
| 58 | Dawn: the storm passes; the other team's tent is wrecked; Elena is alive | S |

**Dangers.** The deadly realm without shelter, a torn tent, a slip at night unclipped.
**Story.** Doubt becomes evidence: the myth is real, and someone else saw it.
**New for the player.** Sleeping on oxygen, losing the radio.
**Ends on the question.** *Nima is coughing. The South Col is 800 m above.*
**Build.** The frozen limb (Blender mesh exists, not placed), the storm-tent event, radio loss, sleeping oxygen.

### Chapter 14 · The Yellow Band

| | |
|---|---|
| Where | Leg 6, Camp 3 → South Col (≈ 7,865 m; label 7,906): 1.53 km on the map, 1.75 km on the ground, +813 m, slope mean 30.7°, max 47.2°, **the steepest leg**. Checkpoint "Yellow Band" (≈ 7,509 m). The second journal page at 96%, 28 m off the route, by an old oxygen depot. Eight bottles at the South Col today [all Measured; becomes the camp's stock] |
| Time | 70 min (walk 40, other 30) · game day 22, 06:00 → South Col about 13:00 → evening preparation |
| Company | partner, or alone after the HAPE decision |
| Goal | reach the South Col on oxygen and prepare for summit night |

**The body.** On 2 l/min SpO₂ stays at 66 or more; without oxygen 45-52 [Measured, computed]. Sanity falling
(hallucinations below 60, the ghosts possible below 30). Wind and cold at the Col.

**Decisions**
1. **Oxygen flow on the Yellow Band:** 2 or 3 l/min (speed against bottles).
2. **The HAPE decision** (section 3.1): go down with Nima (this push ends; days and supplies lost; a second attempt may
   still be possible with less), send him down alone (radio Dr. Hale; a Kovač Sherpa may go with him if there is
   goodwill), or go on together (he insists, for his father; he may die).
3. **The turnaround time:** set it before leaving the Col (for example 11:00). The game holds you to it: Nima,
   Dr. Hale and the journal remember.
4. **Sleep four hours on 0.5 l/min or leave early,** ahead of the crowd.

**Beats**

| Min | Beat | |
|---|---|---|
| 0 | The Yellow Band: limestone, ropes, the jumar | F |
| 12 | Checkpoint | F |
| 15 | The parents walk ahead in the spindrift, if sanity is below 30 (the first sighting, "SawParents", exists) | S |
| 22 | The Geneva Spur: black rock, a short scramble (the climbing component) | F |
| 32 | **The old oxygen depot and the second page** (exists): the father's hand, the tree feeds on people; one line from the mother: *"He will come."* | F |
| 38 | The South Col: wind, old tents, empty bottles everywhere | F |
| 45 | **Nima gives his father's words:** *"He said they did not fall. He said they walked towards a light, and the light knew their names."* | F |
| 55 | The HAPE decision (decision 2) | C |
| 60 | Preparation: water, food, bottles, headlamp; the turnaround time (decision 3) | C |
| 70 | 21:30: leave | F |

**Dangers.** The steepest leg (a slip off the rope), hypoxia, the first ghosts.
**Story.** Page 2 (exists), Lhakpa's words (new): Ending 3's seed (Phase 1 Q7).
**New for the player.** Oxygen flow, the turnaround time.
**Ends on the question.** *Summit night.*
**Build.** The Geneva Spur scramble, South Col wind, the HAPE scene, the turnaround screen.

---

### ACT IV · THE MYTH REVEALED (7,900 m to the summit)

### Chapter 15 · Summit Night

| | |
|---|---|
| Where | Leg 7, South Col → summit: 2.20 km on the map, 2.47 km on the ground, +980 m, 7,861 → 8,841 m, slope mean 23.9°, max 45.1°. Checkpoint "Summit ridge - Balcony" at 50%. The "8,000 metres. The death zone" message exists. Summit landmark ≈ 8,838-8,842 m (label 8,849). All nine realms show at once within 600 m of the summit; the World Tree stands at the summit [all Measured]. The Balcony, South Summit, Cornice Traverse and Hillary Step on our terrain: [Open] |
| Time | 100 min (walk 75, other 25) · game day 22-23, 21:30 → about 08:00 |
| Company | alone or with Nima (the HAPE decision); the other team's headlamps and the queue; Elena; the ghosts |
| Goal | reach the summit before the turnaround time, and live |

**The body.** On 2 l/min SpO₂ in the 60s; without oxygen below 40 (damage). Standing still at the summit the body heads
for 32.4 °C and reaches the damage line in about 12 minutes [Measured]. Fatigue high, sanity low, bottle changes.

**Decisions**
1. **Flow, bottle by bottle** (4.5), all night.
2. **The queue** at the Balcony and the Hillary Step: wait in the cold (heat and oxygen burn away) or unclip and pass
   on exposed ground.
3. **Elena collapses** below the South Summit: give her your spare bottle (and risk your own descent), stay with her
   until her Sherpa returns (time, cold, the turnaround), or walk on. What happened to her is told at Base Camp.
4. **The ghosts or the rope:** the parents walk off the ridge into the wind. Sometimes they lead to a shortcut or a
   relic of theirs; sometimes to the edge of a cornice. The compass and the rope are the counter-cue (Phase 1 S6).
5. **The turnaround time:** keep it (go down without the summit; with supplies left, a second attempt from the Col is
   possible) or break it (the summit, at the price of trust, and a descent in the afternoon wind).

**Beats**

| Min | Beat | |
|---|---|---|
| 0 | Leaving the Col: the other team's headlamps above like a string of lights | F |
| 10 | "8,000 metres. The death zone." (exists) | F |
| 20 | The Triangle Face in the dark, the cold | S |
| 30 | The Balcony (checkpoint): bottle change, the cache if you stocked one; the queue | F/S |
| 40 | **Sunrise from the Balcony:** the mountain's shadow lies across the clouds to the horizon | F |
| 50 | Elena (decision 3) | S/C |
| 60 | The South Summit; the ghosts (decision 4); the Cornice Traverse in the wind | S/C |
| 70 | The Hillary Step: a short rock step on the rope (the climbing component) | F |
| 80 | **The mountain opens:** golden veins in the rock, gold dust in the wind, all nine realms at once (exists), gravity breaks in small zones (Phase 1 Q10), Níðhöggr roars and the law-level avalanche must be escaped (Phase 1 S7) | F |
| 95 | The summit: the World Tree (exists) and the door of the canopy temple | F |

**Dangers.** Hypoxia, cold while waiting, a slip off the ridge, a cornice, the ghosts' false line, the avalanche
escape.
**Story.** The myth is fully revealed [Established].
**New for the player.** Nothing new: everything learned, under the most pressure.
**Ends on the question.** None: the answer is behind the door.
**Build.** The queue (followers that stop and wait), Elena's scene, the ghosts' guide-or-lure logic (does not exist
today), the Hillary Step, golden veins and gold dust (Niagara), low-gravity zones, the avalanche escape, the cornice.

---

### ACT V · FINAL JUDGEMENT

### Chapter 16 · The Canopy Temple

| | |
|---|---|
| Where | The summit, inside the canopy temple [Established] |
| Time | 25 min (other 25) |
| Company | the parents |
| Goal | reach the core, understand the machine, choose |

As Phase 1 stage S8 [Established]: silence, the parents wrapped in sap, the tree's message, **the three choices**
(Sacrifice, Obsession, the Mortal's Strike), each with a held, deliberate input; survival meters frozen.
**What the player's novel changes [Proposed]:**
- If the father's axe came up the mountain, the Mortal's Strike is made with it (its own scene); otherwise with
  Sangye's axe.
- The tree's visions show what the player did: Nima (down, alive, or dead), Elena, Ji-woo, the sick trekker on day 2.
  The tree's argument for sacrifice is built from the player's own sacrifices.
- The journal's last page before the choice is written on the summit.

**Build.** Temple set, the parents in sap, the rune core, three ending sequences (Sequencer), the pre-ending save.

### Chapter 17 · Down

| | |
|---|---|
| Where | Leg 7 down to the South Col (played), then an epilogue down to Base Camp with one played beat: the last ladder in the Icefall |
| Time | 50 min (walk 38, other 12) |
| Company | depends on the ending |
| Goal | get down. Most real Everest deaths happen on the descent |

| Ending | The descent |
|---|---|
| Sacrifice | Sangye stays. The player walks down **as Nima** (if alive) or reads the epilogue: the last page in Nima's hand [Proposed] |
| Obsession | Down in falling ash with the parents, slowly, the horizon burning |
| Mortal's Strike | Sangye carries his father, his mother follows; the realms are gone, the mountain is only a mountain; the hardest walk of the game, and the gentlest |

**Beats.** The last bottle and the cold on the way down; whoever waited at the Col; Dr. Hale's voice coming back on
the radio; Base Camp: who is there, who is not (Elena, Ji-woo, Marek, the trekker); the memorial stones; **the last
journal page, and the whole book** (4.12).
**Build.** The three descents, the epilogue, the book screen.

---

## 6. The tension curve and the company

Physical pressure (P) and mental pressure (M) from 1 (low) to 5 (high).

| Ch | P | M | Company | Solitude used on purpose |
|---|---|---|---|---|
| 1 The Compass | 1 | 3 | crowd | the parents' empty room |
| 2 The Memorial Pass | 2 | 3 | crowd → alone | alone on the pass in the snow, at the memorial stone |
| 3 Their Tent | 2 | 4 | crowd → partner | Nima goes silent at the tent |
| 4 Base Camp | 1 | 2 | crowd | — |
| 5 The Icefall | 4 | 3 | partner | — |
| 6 First Night High | 3 | 3 | partner, team | awake alone at 03:00, gasping |
| 7 Thick Air | 1 | 2 | crowd | — |
| 8 The Icefall, Changed | 4 | 3 | partner, team | — |
| 9 The Western Cwm | 3 | 4 | partner, team | the booms in the silence |
| 10 The Lhotse Face | 4 | 3 | partner | — |
| 11 Retreat | 3 | 4 | partner, team | — |
| 12 The Window | 3 | 3 | partner, voice | the night Icefall: only one light ahead |
| 13 Blue Ice | 4 | 5 | partner, then voice lost | the radio dies in the storm |
| 14 The Yellow Band | 5 | 5 | partner or alone | maybe alone from the Col |
| 15 Summit Night | 5 | 5 | alone or partner, team far ahead | the last hours |
| 16 The Canopy Temple | 1 | 5 | the parents | — |
| 17 Down | 4 | 4 | depends | — |

Rule check: never more than two chapters in a row *alone* [OK: at most 14-15]. Rests (4, 7, 12's waiting days) follow
the hardest stretches.

## 7. The story trail across the chapters

| Ch | Find | Mark | Today |
|---|---|---|---|
| 1 | compass, palm mark, the priest | F | compass exists |
| 2 | the parents' names on a memorial stone | F | new |
| 3 | the parents' camp, page 1; the compass twitch; Nima's father | F | camp and twitch exist |
| 5 | the wood fibre; the cave's runes | F / C | both exist |
| 8 | the carbonised branch with growth rings | F | new |
| 9 | the booms; the compass shaking | F / S | exist |
| 10 | page 3: "Lhakpa wants to turn back" | F | new |
| 13 | the frozen limb, seen by Nima too | F | mesh exists, not placed |
| 14 | page 2 (the tree feeds on people); Lhakpa's words; the ghosts | F / S | page 2 and ghosts exist |
| 15 | the mountain opens, gold, gravity, the dragon | F | new |
| 16 | the temple, the parents, the choice | F | new |

## 8. Passing the same place again

Every pass must feel different. The Icefall's state is data per pass (ladders, seracs, crevasses), so changing it is
cheap.

| Pass | Chapter | Direction, time | What is different |
|---|---|---|---|
| Icefall 1 | 5 | up, 03:00 | learning: Nima holds the ladders, the near-miss collapse |
| Icefall 2 | 6 | down, 05:00 or later | looking down the ladders, rappel, the serac clock by your choice |
| Icefall 3 | 8 | up, 03:00, heavy | collapsed section, double ladder, broken ladder, the branch, a stretcher |
| Icefall 4 | 11 | down, afternoon | after the rescue, in the heat; a collapse may force a detour |
| Icefall 5 | 12 | up, 01:00 | night: one light ahead, a crack opening by a ladder |
| Western Cwm | 9, 11, 12 | up noon, down morning, up at sunrise | heat, then a fall into a crevasse, then cold silence |
| Lhotse Face | 10, 13 | up and down; up again | learning the jumar; then the limb and the storm |

## 9. Build order and size [Proposed, rough, in Claude working days]

| Step | What | Days |
|---|---|---|
| M1 Expedition core | clock 6 min/min, acclimatisation from nights, camp stocks and no respawn, stove and water, fatigue, oxygen flow dial, forecast at the radio calls, checkpoint keeps the body's state, the fixes in 4.13 | 6-8 |
| M2 People core | route followers, Nima v1 (lead, follow, wait, catch, barks), subtitles and radio, trust, Dr. Hale's calls | 7-9 |
| M3 The journal v1 | event log → templates → the book with the parents' pages | 3-4 |
| M4 Vertical slice: chapters 4-6 | Base Camp hub, Icefall rotation 1 fully (manual clips, ladders with Nima, serac timing, near miss), first night high, the other team scripted for now. **Then a play-test: does it grip?** | 8-10 |
| M5 Rest of Act II: chapters 1-3 and 7 | village, trek (yaks, porters, Thukla, Lobuche), the parents' camp with Nima, rest days | 10-12 |
| M6 Act III: chapters 8-14 | Icefall states, Cwm heat, jumar, Camp 3 storm, the limb, the HAPE scene; the other team on the virtual climber (B1) | 15-20 |
| M7 Acts IV-V: chapters 15-17 | summit night, queue, Elena, ghost logic, the myth revealed, temple, three endings, descents, epilogue | 15-20 |
| | **Total** | **about 64-83** |

M1 to M4 also produce what the thesis needs (the physiology model in play, route 1 to Camp 1). The development plan
(`Development_Plan.md`) should be re-ordered after the author approves this document.

## 10. Risks, honestly

- **The partner is the hardest part.** An AI companion on a 1:1 mountain is a big job. Keeping Nima on the route line
  (no free pathfinding) makes it possible. When the player leaves the rope, Nima waits: that is both a scope saver and
  solitude used on purpose.
- **Repetition.** Five Icefall passes can bore. Section 8 is the answer, and the session monitor will show if it is
  not enough.
- **Writing volume.** Barks, radio lines and journal templates are a lot of text. Start with the slice (M4) and grow.
- **Representation.** Nima and the Sherpa crews must be competent, with their own reasons and dignity, never servants
  or props. Real Everest tragedies are not copied; all events here are fictional.
- **Performance.** Each figure is a skeletal mesh with foot traces. Keep at most about eight visible, with distance
  levels of detail; measure at the worst spot (Development plan rule).
- **Length.** 16.5 hours is a lot for a solo developer to fill; the cut list (2.6) is the safety valve.

## 11. How we check the hours

The session monitor (exists) gets a few more fields, and every play-test is judged by numbers, not feelings alone:

| Measure | Target |
|---|---|
| Real time per chapter | within ±25% of section 2.4 |
| Longest stretch without a decision (needs a "decision" event in the log) | under 6 minutes |
| Deaths per chapter, and their causes | every death explainable by the player afterwards |
| Share of time walking | about 60-65% |
| Share of time with company (a person within 30 m or the radio active) | every chapter's company level met |
| Where players stop playing | none in the middle of a chapter |

## 12. Open questions for the author

1. **Length:** about 16.5 hours for a first-time player, or apply the cut list to reach about 14?
2. **Names:** Nima, his father Lhakpa, Dr. Ingrid Hale, Marek Kovač, Elena Ruiz, Ji-woo Han, the aunt: keep or change?
3. **Nima joins at Gorak Shep** (Chapter 3), or already at Dingboche?
4. **The other team's two moral events** (Ji-woo's fall, Elena's collapse): right in tone?
5. **The father's axe** as the tool of the Mortal's Strike: yes?
6. **Realms by act:** gentler low, deadly only from Act III, foretold by the forecast. This adjusts the 2026-11-04
   decision (random, any order): yes?
7. **Altitude illness and the medicine kit** (AMS, HACE, HAPE): in?
8. **Clipping by hand at every anchor:** in (more real, more input)?
9. **The journal that writes itself every night:** in? Which tone: plain and factual, or more literary?
10. **Positions to measure from real coordinates:** the Thukla memorials, Lobuche, the Nangkartshang ridge, Kala
    Patthar, and the Balcony, South Summit and Hillary Step on our terrain.
11. **The last descent as an epilogue** after the summit (Chapter 17), or walk all of it?
12. **Should Nima see the ghosts** at the very end?

## 13. Sources (section 1.6)

Loops and motivation:
- Nested loops (micro, macro, meta): [Medium, HAMSOFT](https://medium.com/@HAMSOFT/game-design-using-micro-macro-meta-grouping-bff507b1c77e),
  [Games Alchemy](https://gamesalchemy.substack.com/p/48-macro-and-micro-designing-the),
  [hitem3d blog](https://blog.hitem3d.ai/blog/What-is-a-Game-Loop-The-Core-Concept-Every-Game-Designer-Must-Understand)
- The compulsion loop and its ethics: [GameAnalytics](https://www.gameanalytics.com/blog/the-compulsion-loop-explained),
  [Game Developer: exploitative design](https://www.gamedeveloper.com/design/exploitative-game-design-beyond-the-f2p-debate),
  [Deceptive Design, ch. 11](https://deceptive.design/book/contents/chapter-11)
- Self-determination theory and games (Ryan, Rigby, Przybylski 2006):
  [paper (PDF)](https://www.rochester.edu/warner/lida/wp-content/uploads/2022/11/02bfe513dd59366750000000.pdf),
  [Przybylski, Ryan, Rigby 2009 (PDF)](https://selfdeterminationtheory.org/wp-content/uploads/2014/04/2009_PrzbylskiRyanRigby_PSPB-1.pdf),
  [Rigby and Ryan 2017 (PDF)](https://selfdeterminationtheory.org/wp-content/uploads/2020/10/2017_RigbyRyan_TimeWellSpent.pdf)
- The Zeigarnik effect and open loops: [Psychology of Games](https://www.psychologyofgames.com/2013/03/the-zeigarnik-effect-and-quest-logs),
  [Yu-kai Chou](https://yukaichou.com/behavioral-analysis/zeigarnik-effect-incomplete-tasks-memory-tension/)
- Emergent stories (RimWorld): [Rock Paper Shotgun, via reader](https://hn.jakechampion.name/reader/https://www.rockpapershotgun.com/2016/08/12/how-rimworld-generates-great-stories),
  [Game Developer](https://gamedeveloper.com/design/rimworld-dwarf-fortress-and-procedurally-generated-story-telling),
  [Game Developer: the simulation dream](https://gamedeveloper.com/design/the-simulation-dream)

Games compared:
- Cairn (2026): [Slant](https://www.slantmagazine.com/games/cairn-review/), [The Outerhaven](https://www.theouterhaven.net/cairn-review-survive-the-climb/),
  [PC Gamer](https://pcgamer.com/games/action/cairn-review)
- Death Stranding: [Game Developer design discussion](https://www.gamedeveloper.com/design/a-design-discussion-on-death-stranding),
  [Epilogue Gaming](https://epiloguegaming.com/distinctly-human-a-critical-review-of-death-stranding/)
- The Long Dark: [Game Design Skills: survival](https://gamedesignskills.com/game-design/survival/),
  [PC Gamer interview](https://pcgamer.com/the-long-darks-creative-director-explains-why-it-doesnt-need-zombies/2)

Many of these are blogs and reviews, not peer-reviewed research; the self-determination papers and the Zeigarnik
work are the strongest evidence, and the loop models are practitioners' rules of thumb.
