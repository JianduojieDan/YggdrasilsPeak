# The Spine: Yggdrasil's Peak — Phase 1 Design Plan

Version 0.1 · 2026-11-02 · Project `F:\UE_project\YggdrasilsPeak` (UE 5.8)

**How to read this document.** Every statement carries one of four labels:

| Label | Meaning |
|---|---|
| **[Established]** | From the original setting document `游戏设定.docx` (the creative source of truth) or a decision the author has made. Do not change without the author. |
| **[Measured]** | Read from project data (route.json, terrain georeference, source code). Numbers can be re-checked. |
| **[Proposed]** | A design suggestion. Not approved until the author accepts it. |
| **[Open]** | A question still to decide. Listed in section 3. |

Where the original setting and later notes disagree, **the original setting wins** (author's decision, 2026-11-02).

---

## 1. Game identity

**[Established]** *The Spine: Yggdrasil's Peak* (脊梁：世界树之巅) is a single-player, first-person mountain-climbing survival game on Everest. Sangye, a young man from a village at the foot of the Himalaya, climbs to the summit to ask his parents, who vanished there twenty years ago, one question: *"Was what you saw worth it?"*

The climb starts grounded and physical: hunger, thirst, cold, rockfall. As the air thins, the mountain becomes psychological (sanity, hallucinations, the parents' ghosts in the storm) and finally mythic: the Himalaya is a wall Odin built from Ymir's bones to hide the withering World Tree. At the summit, Sangye finds his parents kept alive as "fuel" in a rune-powered life-support machine, and must choose between three endings.

**Pillars** (agreed 2026-11-02): danger, fun, interesting decisions, exploration, cinematic moments, and in every case **credible first, mythic later**.

---

## 2. Established decisions

### 2.1 From the original setting [Established]

| Topic | Decision |
|---|---|
| Title | 《脊梁：世界树之巅》 The Spine: Yggdrasil's Peak |
| Myth | The Himalaya is the last wall Odin built from Ymir's bones after Ragnarök, hiding the withering Yggdrasil. Village taboo: "Do not try to touch the spine of the sky." |
| Protagonist | **Sangye (桑吉)**, once the village rebel. Twenty years ago he mocked his parents' obsession and refused their last hug. |
| Parents | Top explorers who believed the summit held a "miracle" that could save the world, and climbed despite the village's curse. |
| Hook (2026) | The world is shifting: Antarctic ice melting, earthquakes worldwide. Sorting his parents' belongings, Sangye finds a **rune-covered compass**. When it points at the summit, a **golden tree-vein mark** appears on his palm. |
| Priest | The village priest tells him: the World Tree is withering, the black dragon **Níðhöggr** is gnawing its roots, and his parents did not disappear: they became **"fuel"**. The fire is going out; the world will fall. |
| Motive | Not to save the world, not for redemption: only to ask his parents, at the forbidden place, "Was it worth it?" |
| Ch. 2 Mortal Realm, 4000–6000 m | Hardcore climbing. Scree slopes, sparse shrubs, the **parents' abandoned camp**. Hunger, thirst, basic body temperature. Natural dangers: landslides, rockfall. Discovery: where the rock is broken, it is not granite but **carbonised giant wood fibre, hard as iron**. |
| Ch. 3 Path of Hallucination, 6000–8000 m | Psychological thriller and extreme survival. Death zone, blue ice everywhere, **limbs of frozen mythical creatures** deep in the ice. **Sanity and oxygen.** Hypoxia distorts vision; the **parents' ghosts walk in the blizzard**; Níðhöggr's impacts boom inside the mountain. The compass shakes harder with height. Conflict: is this path the parents' guidance, or death's lure? |
| Ch. 4 The Myth Revealed, 8000–8848 m | Spectacle and broken laws. The mountain splits open; a **golden trunk** reaching heaven and earth appears. **Gravity turns unstable**, glowing **gold dust** in the blizzard. Challenge: escape the **"law-level avalanche"** caused by the dragon's impacts. Summit: the **canopy temple**, with no god, only a giant rune-driven **life-support system**; the parents, wrapped in translucent sap, kept barely alive to slow the tree's death. |
| Ch. 5 Final Judgement, three choices | The tree's will (or the parents' remaining minds) says: the pillar will fall, the dragon will break free, Ragnarök comes tonight. **1. Sacrifice:** Sangye gives his soul as the new fuel; the tree revives, the world calms, the parents' souls are freed as points of light, Sangye stays at the summit forever. **2. Obsession:** drain the tree's last energy to wake the parents; they wake in his arms, the tree crumbles to ash, the dragon breaks out, fire on the horizon; he leads them down into the end of the world. **3. The Mortal's Strike:** the "sacrifice" is an ancient lie; the tree feeds on mortal lives to keep its false divinity. Sangye smashes the **rune core** with his climbing tools (or a piece of his parents' gear); the golden veins go dark, the dragon flies into deep space, the Himalaya becomes ordinary mountains, the quakes stop. The parents fall from the amber, old and weak but truly alive; Sangye carries his father and holds his mother's hand down through the snow. *"Humans need not buy survival with sacrifice; we walk on our own feet."* |

### 2.2 Author decisions made during development [Established]

| Decision | Date / source |
|---|---|
| Real Everest geography (South Col route), 1:1 scale terrain from DEM | Oct 2026, terrain work |
| First-person view with the AI-rigged climber model and plain UE first-person template behaviour | 2026-11-01 |
| Glaciers (Khumbu) are wanted on the route | 2026-11-02 |
| No crafting; resource management = preparation, carrying capacity, consumption, conservation, continue/rest decisions | Phase 1 brief |
| Solo developer; reuse and acquire assets wherever it saves time; laptop / Steam performance target | Phase 1 brief, standing rule |
| Procedural generation is an important technical (thesis) component | Phase 1 brief, supervisor |
| Thesis direction: physiology (hypoxia, cold, exhaustion) as the main topic with some of the geodata pipeline | Supervisor, 2026-10-26 |

---

## 3. Open questions and recommendations

Recommendations are **[Proposed]**, not decisions. **Q1–Q4 accepted by the author on 2026-11-02** (now [Established]).

| # | Question | Recommendation [Proposed] |
|---|---|---|
| Q1 | **Scale and play time.** How much of the 27.7 km route is walked? | Keep the whole geography visible and reachable, but make only the distinctive stages full gameplay. Shorten the 11.5 km approach (Dingboche→Gorak Shep) to a compact start near Gorak Shep or EBC, with the village as a separate small scene. Set durations from a prototype, never from km. |
| Q2 | **Recovery model.** | Checkpoint at each camp, plus extra checkpoints before long technical sections (Icefall middle, Lhotse Face middle, Hillary Step). On death: resume at last checkpoint with its saved supplies, body state and weather; discovered clues stay recorded; the checkpoint save is refused if the player would load into a lethal state (e.g. oxygen empty above 8000 m → nudge back to the previous camp). No permadeath. |
| Q3 | **Human presence.** | Solitary climb. Other people only through camps, gear, written records, radio voices, and the priest in the village. The parents' ghosts (Ch. 3) are the only "companions". No following AI. |
| Q4 | **Survival depth.** Which variables? | Keep what already exists (health, stamina, satiety, hydration, body temperature, oxygen). Add **sanity** (required by the setting, Ch. 3) and **fatigue/sleep** (brief). Injury, equipment wear: optional, only if they create a clear decision. |
| Q5 | **First unusual event: when, how obvious?** | Ch. 2, at the parents' abandoned camp: the compass needle twitches toward the summit once. Explainable (magnetic rock, fatigue). The carbonised wood fibre near Camp 1 is the first thing that cannot be explained. |
| Q6 | **When does doubt become evidence?** | Ch. 3, Lhotse Face: a frozen limb in the ice large enough to be undeniable, seen in clear weather (not in hypoxia). After this the player knows the myth is real. |
| Q7 | **Ending 3 requirement.** Must the player discover the "lie" to unlock it? (old plan END04) | Yes, but through main-route evidence: the parents' journal pages found on the main route reveal the tree feeds on people. Optional finds deepen it but are not required. |
| Q8 | **Ending 3 action.** Climbing tool or the parents' gear? (END05) | The **ice axe**: the tool the player has used all game breaks the core. Simple, thematically "mortal". |
| Q9 | **Níðhöggr presentation.** | Sound, ground shake, distant silhouette and scripted avalanches only. No boss fight. |
| Q10 | **Gravity anomalies.** | Limited zones in Ch. 4 only: lower gravity jumps and floating snow/gold dust. No physics puzzles. |
| Q11 | **Village (Ch. 1) size.** | One small scene: Sangye's house, the priest, a short path out. Not a populated town. |
| Q12 | **Where does Ch. 1 sit on the map?** | The terrain has real villages (Dingboche at the spawn). Use Dingboche as "the village". |
| Q13 | **Oxygen bottles vs. setting.** | Bottles already exist and fit Ch. 3 "oxygen management". Keep. |

---

## 4. Narrative outline

### 4.1 Why the World Tree is in the Himalaya [Established]
Odin built the Himalaya from Ymir's bones as the last wall hiding the dying Yggdrasil. The village guards the taboo without knowing why. This answers the brief's "Himalaya vs. Norse" question.

### 4.2 Evidence trail [Proposed] — each step uses only [Established] story content

| Stage | What Sangye finds (main route) | What it means | Still uncertain |
|---|---|---|---|
| Village | Compass, palm mark, priest's words | The parents may be alive in some sense; the world is breaking | Is the priest just superstitious? |
| EBC / Gorak Shep area | **The parents' abandoned camp**: gear, a journal page about "the miracle" | They really came this way, full of hope | Did they die here? |
| Icefall / Camp 1 | Broken rock shows **carbonised wood fibre** | The mountain is not stone | Could be a fossil? |
| Western Cwm | Rumbling inside the mountain; compass shaking | Something lives inside | Avalanches also rumble |
| Lhotse Face | **Frozen mythical limb** in the blue ice, in clear light | The myth is real | What are the parents to it? |
| Above Camp 3 | **The parents' ghosts** in the blizzard, sometimes leading true, sometimes false | They are calling him, or something uses their image | Guidance or lure? |
| South Col | Second journal page: the parents learned the tree feeds on people; they went up anyway, hoping to stop it | Plants the seed of Ending 3 | Did they succeed? |
| 8000 m+ | Mountain splits, golden trunk, gravity breaks | The scale of the stakes | — |
| Temple | Parents in sap, the machine, the tree's message | The three choices | Which truth is the real one? |

### 4.3 The three endings [Established] — presentation plan [Proposed]
All three use the same temple set; differences are lighting, VFX, audio and a short scripted sequence, no physical simulation of world destruction.
- **Sacrifice:** Sangye steps into the sap; camera pulls out; points of light rise; the golden veins brighten; the world rumble stops; final view of the summit at dawn.
- **Obsession:** the trunk greys to ash (material dissolve), the dragon's silhouette bursts out, the horizon glows red (sky/fog change); a short playable descent in falling ash.
- **Mortal's Strike:** ice axe hits the core (guarded hold-to-confirm input); the veins go dark, the dragon leaves into the sky; the sky returns to the normal day; a short playable descent carrying the father (slow walk, mother following).
- Safeguards: pre-ending save; each choice needs a deliberate hold input with a stated consequence; survival meters frozen in the temple.

---

## 5. Project facts

### 5.1 Terrain [Measured]
- Heightmap: real Everest region, centre 28.008 N 86.921 E, 1009 px at 33.742 m/px (~34 km square), elevation 3890–8762 m. 1:1 scale.
- Elevation in game = 6326.3 m + (world Z − 129770) / 100 (SurvivalConfig).
- Playable boundary: x 171000..1385000, y 1183000..2590000 (Dingboche → summit corridor). Distant terrain ring (120 km) is visual only.
- **Known limitation:** the 34 m/px source averages away sharp peaks; the route summit measures **8752 m** (real 8849 m), Base Camp measures **5278 m** (label 5364 m).

### 5.2 Route legs (route.json, A* route, max slope 50°) [Measured]

| Leg | Real reference | Plan length | Elevation (measured) | Mean / max slope | Start → end (world x, y cm) |
|---|---|---|---|---|---|
| 0 | Dingboche → Gorak Shep | 11.5 km | 4334 → 5158 m | 12° / 35° | 339733, 2468724 → 291733, 1500724 |
| 1 | Gorak Shep → Everest Base Camp | 3.0 km | 5159 → 5278 m | 9° / 21° | 291646, 1501409 → 487646, 1289409 |
| 2 | EBC → Khumbu Icefall | 2.3 km | 5278 → 5704 m | 12° / 52° | 487014, 1289409 → 657014, 1343409 |
| 3 | Icefall → Camp 1 | 2.4 km | 5703 → 6130 m | 19° / 50° | 656739, 1343059 → 794739, 1465059 |
| 4 | Camp 1 → Camp 2 (Western Cwm) | 2.2 km | 6131 → 6452 m | 10° / 17° | 795082, 1465544 → 1001082, 1509544 |
| 5 | Camp 2 → Camp 3 (Lhotse Face) | 2.6 km | 6453 → 7049 m | 18° / 42° | 1000911, 1510421 → 1168911, 1594421 |
| 6 | Camp 3 → South Col / Camp 4 | 1.5 km | 7048 → 7860 m | 31° / 47° | 1168948, 1592655 → 1286948, 1584655 |
| 7 | South Col → Summit | 2.2 km | 7861 → 8752 m | 24° / 45° | 1287108, 1585317 → 1237108, 1419317 |
| | **Total** | **27.7 km** | | | |

Leg start/end points are the fixed-rope ends, ~12 m from each camp landmark. **Unverified:** whether the A* route follows the real climbing line in the Icefall and on the Lhotse Face; glacier extent (no glacier data in the project yet).

### 5.3 Systems that already exist [Measured — in source and verified in PIE earlier]
| System | Where | Notes |
|---|---|---|
| Survival stats: health, stamina, satiety, hydration, body temperature, oxygen; sprint; warnings | `Survival/SurvivalComponent` | Speed modified by uphill grade, low oxygen, weather, deep snow |
| HUD | Survival HUD | Stat icons, warnings, altitude |
| Fixed ropes + clip-in | `AFixedRope`, `UExpeditionComponent::UpdateRope` | 8 ropes along route; clipped: walkable 58°, uphill cost ×0.5 |
| Oxygen bottle pickups | `AOxygenPickup` | 30 placed at camps; pack max 3 |
| Camps | 9 `RouteLandmark`s + 125 Fab props | Placed by `Tools/RoutePlanning/place_camps.py` |
| Weather | Ultra Dynamic Sky / Weather; storm strength in ExpeditionComponent | Storm fills footprints |
| Snow trail, footprints, sinking, step puffs | `SnowTrailComponent` | Per-foot plant detection (2026-11-02) |
| First-person climber | `BP_FirstPersonCharacter`, `SKM_Climber_Rigged` | Retargeted template locomotion + Mixamo run |
| Procedural rock scatter | PCG (Oct 2026) | Rocks on the landscape |
| Route planner | `Tools/RoutePlanning/plan_route.py` | A* over a 20 m height grid; avoids >50° and rocks |
| Mirror | `AYggMirror` | Test feature |

### 5.4 Not yet built [Measured — absent from source]
Inventory/backpack with weight, food and water items as pickups, rest/sleep/fatigue, sanity, saving and checkpoints, clue/journal system, story triggers, NPC dialogue, compass, glacier surfaces, crevasses/ladders, climbing beyond walking on ropes, gravity zones, temple, endings, audio design.

---

## 6. Chapter and stage plan

The five **chapters** are the original story chapters [Established]. Each chapter contains route **stages** (the brief's eight sections) [Proposed]. Every stage uses the same template.

| Chapter [Established] | Stages [Proposed] | Elevation |
|---|---|---|
| 1 Origin: The Burden | S1 Village and departure | 4334 m (Dingboche) |
| 2 Mortal Realm | S2 Approach and Base Camp · S3 Khumbu Icefall → Camp 1 | 5158–6130 m |
| 3 Path of Hallucination | S4 Western Cwm → Camp 2 · S5 Lhotse Face → Camp 3 · S6 Camp 3 → South Col | 6131–7860 m |
| 4 The Myth Revealed | S7 Summit approach (Balcony → South Summit → Hillary Step) | 7861–8752 m |
| 5 Final Judgement | S8 Canopy temple | Summit |

Note: the setting puts the "death zone" (6000–8000 m) in Ch. 3. Real-world death zone starts at 8000 m; the game follows the setting.

---

### S1 — Village and departure (Ch. 1)

**Identity and location**
- Real reference: Dingboche village. Entry: game start (PlayerStart_2 at Dingboche [Measured]). Exit: the trail north out of the village.
- Elevation: 4334 m [Measured, route start]. Shelter: Sangye's house.
- Adaptation [Proposed]: compact area of a few hundred metres; the 11.5 km Dingboche → Gorak Shep walk is skipped by a travel transition (Q1).

**Objective:** Find the compass in the parents' belongings, speak with the priest, and pack for the climb.

**Emotional progression:** quiet, dusty house and old memories (guilt) → the palm mark and priest's words (unease, purpose) → leaving the village, the peak on the horizon (anticipation).

**Main gameplay (introduces):** movement, interaction, reading items, first inventory packing (food, water, warm layer, compass).

**Danger:** none; safe area.

**Meaningful decision:** what to pack within the first carrying limit (e.g. extra food vs. extra water). Consequence visible in the next stage.

**Exploration:** the parents' room (photos, letters, optional), the village shrine with the taboo inscription.

**Narrative contribution:** the whole hook [Established]. Remains uncertain: whether the parents are alive.

**Visual / audio:** stone houses, prayer flags, warm interior light vs. cold blue outside; yak bells, wind, butter-lamp crackle.

**Cinematic moment:** stepping out of the door, the first unobstructed view of Everest's summit plume.

**Assets and systems**
| Need | Status |
|---|---|
| Village houses, interior props | Acquire (Fab Himalayan / Nepali village kit) |
| Compass (rune-engraved) | Create (simple mesh + emissive runes) |
| Palm-mark VFX on first-person hand | Create (material decal on hand) |
| Priest character | Acquire + adapt (one character; idle + talk animations) |
| Dialogue / text UI | Build |
| Inventory + carrying limit | Build (essential system) |

**Procedural contribution:** none; fully authored.

**Duration [Proposed]:** 5–10 min (reading, packing, talking).

**Completion and testing:** player has compass + minimum supplies; leaving triggers the transition. Test: a new player finds the compass without help; packing UI is understood without explanation.

---

### S2 — Approach and Base Camp (Ch. 2)

**Identity and location:** Gorak Shep → EBC (leg 1, 3.0 km, 5159–5278 m, mean 9° [Measured]). Shelter: EBC camp (mess tents placed [Measured]).
- Adaptation [Proposed]: the parents' abandoned camp placed on a side spur of this leg.

**Objective:** Reach Base Camp; on the way, find the parents' old camp.

**Emotional progression:** open moraine, effort, thin air beginning → the parents' camp (grief, hope) → EBC tents (relief, the Icefall towering above).

**Main gameplay:** walking pace and stamina, eating and drinking, cold and shelter (introduces), first weather cue (wind rising, clouds).

**Danger**
| Hazard | Trigger | Warning | Response | Consequence | Recovery |
|---|---|---|---|---|---|
| Rockfall (scripted spot) [Established danger type] | Passing under a scree wall | Pebbles trickle, clatter sound | Wait or move fast | Damage | Health recovers at camp |
| Landslide (scripted) [Established] | Optional shortcut across loose slope | Visible cracks, creaking | Use main trail | Knocked down, damage | Checkpoint at EBC |
| Cold | Evening, no shelter | Temperature warning, breath fog, shivering | Reach a tent / warm layer | Body temp drops | Tent warms |

**Meaningful decision:** a loose-slope shortcut saves time before the weather turns but risks a landslide; the main trail is longer but safe.

**Exploration:** the **parents' abandoned camp** [Established] (journal page 1, an old photo, maybe supplies); a viewpoint on the moraine crest.

**Narrative contribution:** the parents really came this way; their hope was real. **First unusual hint [Proposed, Q5]:** the compass needle twitches toward the summit at their camp.

**Visual / audio:** grey moraine, sparse shrubs [Established], first glacier ice; wind, footsteps on gravel, flapping tent fabric.

**Cinematic moment:** arriving at EBC at dusk: tents glowing, the Icefall lit pink above.

**Assets and systems**
| Need | Status |
|---|---|
| Moraine / debris-covered glacier surface | Build (landscape layer, glacier mask) |
| Abandoned camp props (torn tent, old gear) | Exists partly (Fab camp props) + adapt |
| Journal pickup + reader UI | Build (clue system, essential) |
| Rockfall event | Build (simple scripted physics rocks + trigger) |
| Shelter / tent warming | Build (rest system) |

**Procedural contribution:** boulder and shrub scatter on moraine (PCG, keeps trail corridor clear); camp positions authored.

**Duration [Proposed]:** 15–20 min.

**Completion and testing:** arrival at EBC sets checkpoint 1. Test: players notice the parents' camp; they use shelter before cold becomes dangerous; the shortcut risk is understood before choosing it.

---

### S3 — Khumbu Icefall → Camp 1 (Ch. 2)

**Identity and location:** legs 2–3 (EBC → Icefall → Camp 1), 4.7 km, 5278 → 6130 m, max slopes 50–52° [Measured]. Shelter: Camp 1. Glacier extent: unverified (RGI outline to be fetched).

**Objective:** Cross the Icefall and reach Camp 1 before the afternoon sun weakens the ice.

**Emotional progression:** a maze of ice towers (awe, disorientation) → ladders over deep crevasses (tension, exposure) → Camp 1 on the flat snow at the top (relief).

**Main gameplay:** route reading (rope colours, flags), ladder crossings (introduces balance), clipping onto fixed ropes (introduces), timing.

**Danger**
| Hazard | Trigger | Warning | Response | Consequence | Recovery |
|---|---|---|---|---|---|
| Ladder fall | Losing balance on a ladder | Sway increases, creaks | Slow down, re-balance | Fall into crevasse = death | Checkpoint before Icefall + mid-Icefall |
| Serac collapse (scripted) | Time of day / lingering in marked zone | Cracking sound, falling ice chips | Move through quickly | Death if caught | Mid checkpoint |
| Hidden crevasse | Unroped off-route | Sagging snow, faint blue | Stay on route | Fall | Checkpoint |

**Meaningful decision:** go early (cold, dark, safer ice) or later (warm, visible, more serac activity).

**Exploration:** an ice cave off-route with an old expedition's cache.

**Narrative contribution:** at a broken ice-rock wall: the **carbonised wood fibre, hard as iron** [Established]. First unexplainable sign.

**Visual / audio:** blue-white seracs, ladders, ropes; ice creaks and groans, dripping water, distant collapses.

**Cinematic moment:** halfway across a ladder, looking down into the blue depth.

**Assets and systems**
| Need | Status |
|---|---|
| Serac / ice block kit | Exists partly (6 upright seracs at Icefall) + acquire more (Fab/Megascans ice) |
| Crevasse meshes | Acquire or create |
| Ladder + crossing mechanic | Exists partly (ladder prop) + build mechanic |
| Fixed ropes | Exist |
| Wood-fibre rock formation | Create (rock mesh with wood material) |
| Glacier ice surface | Build (landscape layer) |

**Procedural contribution:** serac and ice-block scatter **around** a protected corridor; crevasses and ladders authored.

**Duration [Proposed]:** 20–30 min.

**Completion and testing:** Camp 1 checkpoint. Test: ladder failure feels fair; players read the route without a HUD marker; PCG never blocks the corridor (automatic check).

---

### S4 — Western Cwm → Camp 2 (Ch. 3)

**Identity and location:** leg 4, 2.2 km, 6131 → 6452 m, mean 10°, max 17° [Measured]. Shelter: Camp 2 (mess tents placed [Measured]).

**Objective:** Cross the valley to Camp 2 while managing heat, water and oxygen.

**Emotional progression:** release after the Icefall (wide, quiet) → the silence becomes strange: rumbles inside the mountain → Camp 2 under the Lhotse Face (determination).

**Main gameplay:** long-range pacing, water management (glare and heat), oxygen introduction (thin-air effects begin), sanity introduction.

**Danger:** hidden crevasses (warning: sag; response: stay roped); heat/dehydration (warning: thirst stage; response: drink, slow down); first hallucination sound (not deadly).

**Meaningful decision:** spend a bottle of oxygen now to move faster, or save it for higher.

**Exploration:** a memorial cairn; a side moraine with a better view of Lhotse.

**Narrative contribution:** **Níðhöggr's impacts** booming in the mountain [Established]; the compass shakes harder [Established]. Could still be avalanches.

**Visual / audio:** huge white valley, walls on both sides, glare; near-silence, own breathing, then a deep boom.

**Cinematic moment:** turning around to see the Icefall and the whole route below.

**Assets/systems:** compass shaking (create), sanity stat (build), hypoxia screen effects (build), sound design (acquire).

**Procedural:** small snow and rock detail along the valley; crevasse positions authored.

**Duration [Proposed]:** 15–20 min. **Completion:** Camp 2 checkpoint. **Test:** the long walk feels purposeful (players report something to do every few minutes).

---

### S5 — Lhotse Face → Camp 3 (Ch. 3)

**Identity and location:** leg 5, 2.6 km, 6453 → 7049 m, mean 18°, max 42° [Measured]. Shelter: Camp 3 on the face.

**Objective:** Climb the fixed ropes up the blue ice to Camp 3.

**Emotional progression:** commitment at the foot of the wall → rhythm of climbing, breath and burn → the frozen limb in the ice (dread, certainty) → tents clinging to the slope (fragile shelter).

**Main gameplay:** rope ascending (develops ropes into an active mechanic [Proposed]), stamina and oxygen interplay, rests at anchors.

**Danger:** falling ice (warning: whistle, shout); stamina collapse while unclipped (warning: heavy breath, HUD); a slip on blue ice off-rope.

**Meaningful decision:** rest at an anchor (costs time and cold) or push on (risk collapse).

**Exploration:** a ledge with an old tent and a supply cache.

**Narrative contribution:** **the frozen mythical limb** seen in clear light [Established; timing Proposed Q6]. Doubt becomes evidence.

**Visual / audio:** steep blue ice, ropes vanishing upward; ice chips, wind, the boom again.

**Cinematic moment:** a break in the cloud reveals the Western Cwm far below.

**Assets/systems:** rope ascending mechanic + hand animation (build + Mixamo clip), blue-ice material (build), frozen limb mesh (acquire/create), anchors (create).

**Procedural:** ice texture/detail only; route, anchors and landing ledges authored.

**Duration [Proposed]:** 20–30 min. **Completion:** Camp 3 checkpoint. **Test:** climbing doesn't become monotonous (vary angle and stops); fall/recovery is fair.

---

### S6 — Camp 3 → South Col / Camp 4 (Ch. 3)

**Identity and location:** leg 6, 1.5 km, 7048 → 7860 m, **mean 31°**, max 47° (the steepest leg) [Measured]. Shelter: Camp 4 at the South Col.

**Objective:** Cross the Yellow Band and Geneva Spur to the South Col, then prepare for summit night.

**Emotional progression:** fear of exhaustion → the parents' ghosts in the blizzard (longing, confusion) → the bleak, windy col (isolation, the cost of going on).

**Main gameplay:** ropes + oxygen + sanity combined; following or ignoring the ghosts.

**Danger:** false paths led by the ghosts (warning: compass disagrees, no rope); wind gusts; hypoxia.

**Meaningful decision:** **follow the ghost** (sometimes it leads to a shortcut or a clue, sometimes off a cliff edge) or trust the compass and ropes [Established conflict].

**Exploration:** abandoned oxygen depot; the second journal page (main route, required for Ending 3 setup [Proposed Q7]).

**Narrative contribution:** the parents learned the tree feeds on people and went up anyway; the summit will demand a decision beyond survival.

**Visual / audio:** yellow rock bands, scattered old gear at the col, howling wind; the ghosts as soft shapes in the snow.

**Cinematic moment:** at Camp 4, night falls; headlamps of no one; the summit ridge black against stars.

**Assets/systems:** ghost figures (adapt climber model + translucent material), fatigue/sleep (build), weather window choice (build, simplified).

**Procedural:** debris and rock dressing at the col; ghost route paths authored.

**Duration [Proposed]:** 20–25 min including camp preparation. **Completion:** Camp 4 checkpoint (pre-summit save). **Test:** players can tell ghost paths from real ones by at least one cue.

---

### S7 — Summit approach (Ch. 4)

**Identity and location:** leg 7, 2.2 km, 7861 → 8752 m, mean 24°, max 45° [Measured]. Real references: Balcony, South Summit, Hillary Step (positions on our terrain unverified).

**Objective:** Climb the ridge to the summit as the mountain opens.

**Emotional progression:** the dark ridge by headlamp → sunrise; the mountain **splits open** and the golden trunk appears (awe) [Established] → gravity breaks, gold dust, the dragon's avalanche (terror, wonder) → the temple entrance.

**Main gameplay:** every learned skill under pressure; escape sequence from the **law-level avalanche** [Established]; low-gravity zones [Proposed Q10].

**Danger:** the scripted avalanche (warning: dragon roar + ground shake; response: run to marked shelter; consequence: death; recovery: checkpoint just before); cornice edges.

**Meaningful decision:** spend the last oxygen to outrun the avalanche or hide and risk the cold.

**Exploration:** minimal; the focus is momentum.

**Narrative contribution:** the myth is fully revealed [Established].

**Visual / audio:** golden veins in the rock, gold dust in the snow, floating ice; roar, cracking, choir-like hum.

**Cinematic moment:** sunrise at the Balcony, then the mountain splitting to show the trunk.

**Assets/systems:** golden trunk (create: large mesh + emissive), mountain-split effect (authored mesh swap, not simulation), gold dust (Niagara), gravity zones (build), avalanche sequence (authored).

**Procedural:** none on the critical path; dust/particles only.

**Duration [Proposed]:** 20–30 min. **Completion:** temple entry checkpoint. **Test:** collision is reliable on the ridge; the avalanche escape is fair on the first or second try.

---

### S8 — Canopy temple and the three choices (Ch. 5)

**Identity and location:** summit, 8752 m [Measured terrain; real 8849 m]. Inside the canopy temple [Established].

**Objective:** Reach the core, understand the machine, choose.

**Emotional progression:** silence and awe → seeing the parents in sap (grief, love) → the tree's message → the choice.

**Main gameplay:** slow exploration, reading the machine, final decision (hold-to-confirm).

**Danger:** none; survival meters frozen [Proposed].

**Narrative contribution:** the full truth; the three choices [Established].

**Cinematic moment:** the parents' faces through the amber sap, lit gold.

**Assets/systems:** temple set (create/acquire kit), parent figures in sap (adapt), rune core (create), three ending sequences (Sequencer), ending save.

**Duration [Proposed]:** 10–15 min + ending. **Completion:** each ending reachable, skippable, replayable from the pre-ending save.

---

## 7. Pacing plan [Proposed]

Physical pressure (P) and narrative pressure (N), 1 = low, 5 = high.

| Stage | P | N | Activity mix |
|---|---|---|---|
| S1 Village | 1 | 3 | Talk, read, pack |
| S2 Approach + EBC | 2 | 3 | Walk, eat/drink, shelter, find parents' camp |
| S3 Icefall | 4 | 2 | Route reading, ladders, timing |
| S4 Western Cwm | 2 | 3 | Pacing, water, first booms |
| S5 Lhotse Face | 4 | 4 | Rope climbing, rest decisions, frozen limb |
| S6 To South Col | 4 | 4 | Ghosts, compass, oxygen, camp prep |
| S7 Summit approach | 5 | 5 | Escape, spectacle |
| S8 Temple | 1 | 5 | Observe, decide |

Rest stops always have something to do: check supplies, read journals, look at the next section, sleep through a storm.

---

## 8. System introduction and dependencies [Proposed order]

| System | Introduced | Tested under pressure | Depends on | Status |
|---|---|---|---|---|
| Movement, interaction | S1 | all | — | Exists (template + climber) |
| Inventory, carrying limit | S1 | S2, S6 | Interaction | **Build** (essential) |
| Hunger, thirst | S2 | S4 | Inventory | Exists (stats) + needs items |
| Cold and shelter | S2 | S6 | Weather, camps | Exists (temperature) + build shelter |
| Clue / journal | S2 | S6 | Interaction, save | **Build** (essential) |
| Fixed ropes, clip-in | S3 | S5, S6 | — | Exists |
| Ladder crossing | S3 | S3 | Ropes | Build |
| Oxygen | S4 | S6, S7 | Inventory | Exists |
| Sanity, hallucinations | S4 | S6 | Oxygen | **Build** (setting requires) |
| Rope ascending (active) | S5 | S6, S7 | Ropes, stamina | Build |
| Fatigue / sleep | S5 | S6 | Camps, time | Build |
| Compass | S1 | S6 | — | Build |
| Save / checkpoints | S2 | all | All state | **Build** (essential) |
| Gravity zones, avalanche | S7 | S7 | Movement | Build (simplified) |
| Endings | S8 | — | Save, story flags | Build |

Rule: no system is lethal before the player has met it safely.

---

## 9. Procedural generation specification [Proposed]

**Role:** dress the mountain with believable rocks, ice and snow detail around **authored** routes, camps and story spots, reproducibly from a seed.

**Constraint priority (highest first):**
1. Keep route access (corridor around route.json polyline, e.g. 6 m wide; wider at ladders).
2. Keep camp and interaction space (radius around RouteLandmarks and story items).
3. Valid placement and collision (on ground, no overlap, not floating).
4. Environmental suitability (slope, elevation band, surface: rock / snow / glacier).
5. Visual density and variety.

A candidate that breaks a higher rule is **rejected**, never forced.

**Where it runs:** in the editor (generate, inspect, save into the level); runtime only for small cosmetic detail. Reason: reproducibility, testability, and no runtime cost on a laptop.

**Debug output:** seed and settings, accepted/rejected counts, rejection reasons per rule, protected-area visualisation, generation time, overlap/float report.

**Existing base:** PCG rock scatter (Oct 2026) and the A* route planner. Need: protected-corridor exclusion from route.json, rejection logging, a seed-based report.

### 9.1 Research framing [Proposed, for supervisor]
Two candidate questions (the supervisor favoured physiology):
- **A (physiology, main):** "How can an integrated physiological model (hypoxia, cold, exhaustion) in a geodata-based Everest environment produce readable, testable survival decisions?"
- **B (PCG, from the brief):** "How can constraint-based procedural environment generation be integrated with climbing and survival systems to produce reproducible, traversable mountain environments while preserving authored routes and narrative locations?"

Evaluation for B: protected-route violations, invalid/overlapping placements, generation time, frame time on a fixed camera path, reproducibility with a fixed seed, manual fixes needed, successful traversal of test routes.

---

## 10. Assets and production priorities

### 10.1 Asset list [Proposed]

| Asset | Purpose | Status | Source / plan | Performance note |
|---|---|---|---|---|
| Village kit | S1 | Missing | Fab (Himalayan village) | One small scene |
| Priest character | S1 | Missing | Fab / AI-generated + Mixamo rig | Single NPC |
| Compass | S1–S8 | Missing | Create in Blender / AI | Small |
| Camp props, tents, O2 | All camps | **Exist** | Fab (125 placed) | — |
| Debris / glacier surfaces | S2–S4 | Missing | Landscape layer + Megascans textures | Shader only |
| Seracs, ice blocks | S3 | Partial (6) | Megascans ice | Nanite |
| Crevasses | S3, S4 | Missing | Create / Fab | — |
| Ladders | S3 | Partial (prop) | Exists + mechanic | — |
| Wood-fibre rock | S3 | Missing | Create (material on rock mesh) | — |
| Frozen limb in ice | S5 | Missing | AI-generated creature limb + ice material | One hero asset |
| Parents' ghosts | S6 | Missing | Climber model + translucent material | — |
| Golden trunk, veins | S7, S8 | Missing | Create | Large, emissive |
| Temple, rune core | S8 | Missing | Fab kit + create core | — |
| Parents in sap | S8 | Missing | AI-generated models | Two hero assets |
| Climbing animations | S3, S5 | Missing | Mixamo (as done 2026-11-02) | — |
| Sound: wind, ice, breath, booms | All | Missing | Acquire libraries | — |

Every acquired asset: record source, licence, collision needs and triangle count before placing.

### 10.2 Feature priorities [Proposed]

| Essential | Simplifiable | Optional |
|---|---|---|
| Inventory + carrying limit | Sanity (visual + sound effects only) | Persistent injuries |
| Save / checkpoints | Ladder balance (simple sway) | Equipment wear |
| Clue / journal system | Rope ascending (rhythm input) | AI companion |
| Shelter and rest | Gravity zones (lower gravity only) | Large village |
| Story triggers + three endings | Avalanche (authored sequence) | Ice destruction |
| Compass | Dragon (sound + silhouette) | Dragon boss |
| Hunger/thirst items | Weather windows | Photo mode |
| PCG with route protection (thesis) | Glacier surfaces | — |

---

## 11. Representative section (vertical slice) [Proposed]

**Choice: S2 end → S3 (EBC → Icefall → Camp 1).** It contains every element the brief asks for:

| Required element | In the slice |
|---|---|
| Preparation / shelter | EBC tents: pack, rest |
| Traversal challenge | Icefall maze, ladders, ropes |
| Survival pressure | Cold, stamina, thirst |
| Resource decision | Carry more water vs. lighter pack; early vs. late start |
| Story discovery | Carbonised wood fibre (+ parents' camp just before EBC) |
| Memorable moment | Ladder over the blue crevasse |
| Procedural dressing | Seracs around the protected corridor |
| Checkpoint and recovery | EBC + mid-Icefall checkpoints |

**Build order:**
1. Glacier outline (RGI) → landscape glacier layer at EBC/Icefall.
2. Inventory + carrying limit + food/water items.
3. Save/checkpoint at EBC and mid-Icefall.
4. Ladder crossing mechanic + 2–3 authored crossings.
5. PCG serac dressing with route-corridor exclusion and rejection log.
6. Wood-fibre discovery + journal UI.
7. Sound pass (wind, ice).
8. Play-test against the questions in section 12.

---

## 12. Testing and evaluation plan [Proposed]

### 12.1 Per-stage playability
| Question | Method |
|---|---|
| Is movement enjoyable and reliable? | Play-test notes; count stuck/fall-through bugs |
| Can players read the route? | Time to find the way without markers; wrong turns |
| Do survival pressures create decisions? | Ask players what they chose and why |
| Are resource costs understandable? | Players predict a cost before acting |
| Is danger fair? | After each death, can the player say why? |
| Does the story fit play? | Players retell the story after the slice |
| Can players recover from failure? | Time lost per death |

### 12.2 Technical
| Measure | Target [Proposed] |
|---|---|
| Frame rate on the laptop (RTX 5070 Ti laptop) | ≥ 60 fps at target settings on a fixed camera path |
| PCG route violations | 0 |
| PCG reproducibility | Same seed → identical placement list |
| PCG generation time | Recorded per region |
| Save/load | 20 save/load cycles, state identical |

### 12.3 Change rule
When a test reveals a problem, change the specific rule, and record what changed and why in this document's change log.

---

## Change log
- 0.2 (2026-11-02): author accepted Q1 (compact start, approach walk skipped), Q2 (camp + mid-section checkpoints, clues kept, no permadeath), Q3 (solitary climb), Q4 (current stats + sanity + fatigue/sleep; injuries optional).
- 0.1 (2026-11-02): first version. Story from `游戏设定.docx`; route numbers from `route.json`; systems from source.
