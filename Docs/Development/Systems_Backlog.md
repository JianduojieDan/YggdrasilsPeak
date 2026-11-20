# Systems backlog (all "to build" systems in one list)

Version 1.0 · 2026-11-20 · written by Claude for the director. Read-only summary of other documents; nothing was built.

**Sources read:** `B3_Climbing_Spec.md` §3, §9-13 · `B4_Camps_Rest_Spec.md` §0-1, §12-19 · `B5_Sound_Plan.md` §1, §5-10 ·
`B7_Visual_Review.md` top 10, §12 · `Physiology_Game_Integration_Spec.md` §5, §6, §10, §11 · `Chapter2_Design.md` ...
`Chapter7_Design.md` (systems, hooks, build and test sections) · `Development_Plan.md` status board (read only) ·
`Chapter_Design_v1.md` §4 (the shared systems the chapter docs point to). Two facts were checked in code (read only):
story flags live only in memory (`YggStory.h` `TSet<FName> Flags`; no flags in `YggSaveGame.h`), and the story has no
`{spo2}` text token yet.

**How to read the table**

| Column | Meaning |
|---|---|
| ID | group + number: **ST** story machine · **BD** body and physiology · **CP** camps and supplies · **CL** climbing · **HZ** hazards and world set pieces · **NP** other people (NPCs) · **AU** sound · **VS** view and visual |
| Needed by | chapter docs: **C1** Chapter 1 v2 · **C2** Chapter2_Design (Thukla → Lobuche) · **C3** Chapter3_Design (Icefall) · **C4** Chapter4_Design (Base Camp) · **C5** Chapter5_Design (Icefall, second design) · **C6** Chapter6_Design (Camp 1 night, descent) · **C7** Chapter7_Design (Base Camp recovery); plan blocks A2, A4, A5, B1, B3, B4, B5, B7 |
| Size | **S** = 0.5 day or less · **M** = 0.75-1.5 days · **L** = 2 days or more. Days = Claude working days (plan rule 7) |
| Editor / build | **Build** = C++ build (the editor and game must be closed) · **Editor** = editor scripting or content work · **No** = Python, scripts or docs only |
| Risk | Low / Med / High, with the main reason |

---

## 1. First, the chapter-number problem (it changes what "chapters 1-3" means)

Two chapter lists exist. The six chapter docs were written the same day and do not agree:

| Doc | Numbering it uses | In the 12-chapter Chomolungma list it is |
|---|---|---|
| Chapter1_v2_Design | 12-chapter | Ch1 "Day One" (built, open items only) |
| Chapter2_Design (Thukla → Lobuche, day 2) | 17-chapter v1 | first part of Ch2 "The Puja" |
| *(nobody)* Lobuche → Gorak Shep → Base Camp, day 3 | — | middle of Ch2: **not designed** |
| Chapter4_Design (Base Camp, puja, days 3-4) | 17-chapter v1 | second part of Ch2 "The Puja" |
| Chapter3_Design (Icefall, day 5) | 12-chapter | Ch3 "The Icefall" |
| Chapter5_Design (Icefall, day 5) | 17-chapter v1 | **the same** Ch3 "The Icefall", a second design |
| Chapter6_Design, Chapter7_Design | 17-chapter v1 | after Ch3 (Camp 1 night and descent; Base Camp rest) |

**This backlog uses the 12-chapter list.** So the **chapters 1-3 vertical slice** = Ch1 + Chapter2_Design + a short
day-3 walk + Chapter4_Design + one Icefall design (C3 or C5), from Dingboche to Camp 1. That is the same ground as
demo route 1 (plan block A4). Systems that only C6 and C7 need come after the slice (milestone M5).

---

## 2. The backlog (deduplicated: each system once, with every chapter or block that needs it)

### 2.1 Story machine (`Story/YggStory.*`, `YggPlayBot.*`)

| ID | System | What it does (one line) | Needed by | Depends on | Size (days) | Editor / build | Risk |
|---|---|---|---|---|---|---|---|
| ST1 | Flags saved + chapter chaining | Story flags go into the save; a chapter's `end` starts the next one; the body carries over; `ygg.Story.Start <Ch> flags=...` presets for tests | C2-C7 (every hand-off) | — | M 1.0 | Build | Med: never bump the save `Version`; flags must survive deaths |
| ST2 | Live numbers in text | `{spo2}`, `{pulse}`, `{alt}`, `{day}`, `{spo2+6}` inside `say`, `choice`, `page` | C2, C3, C6, C7 | — | S 0.25 | Build | Low |
| ST3 | New story keys | `side`, `roll`, `stat`, `when.stat`, `when.clock`, `task` (timed interaction), `stock`, `trust`, new `hands` items list | C2, C4, C5, C6, C7 | — | M 1.0 | Build | Low |
| ST4 | New NPC modes | `hold_line` (holds a ladder's far end), `sleep`, `shortrope`, work idles, and the `holdline` ladder hold (crosser moves while F is held) | C3, C6, C7 | animation clips | M 1.0 | Build | Med: clips missing (Mixamo or Meshy) |
| ST5 | Story robot per chapter | `run_bot.ps1 -Chapter chapterN`, forced answers (`ygg.Bot.Answers`), fast mode also shortens `timeto` and sleeps, `story_shots.ps1` shot lists | C2-C7 tests | — | M 0.75 | Build + No | Low |
| ST6 | Story survives a death | After respawn: fired beats stay fired, NPCs re-placed behind closed eyes, set pieces fire once | C3 (run R3), C5 §6.6, C6 | — | S 0.5 | Build | Med: not verified in `YggStory` yet |

### 2.2 Body and physiology (`Survival/SurvivalComponent.*`, `SurvivalConfig.h`, new body files)

| ID | System | What it does (one line) | Needed by | Depends on | Size (days) | Editor / build | Risk |
|---|---|---|---|---|---|---|---|
| BD1 | Physiology parity + inner-state log | `ygg.Physio.Sweep csv` + `physio_model.py --check` (pass ≤ 0.05 points); log columns `adapted_m, acclim, felt_alt_m, gap_torr, spo2_target, core_target_c`; update `Physiology_Model.md` | A2, B1, B2 | — | M 0.75 | Build + No | Low |
| BD2 | A2 game-layer tuning | Cold option B, heart-rate maximum falling with altitude, cited bottle value; re-run bots | A2, A5 timings | your D2, D4, D5, D6; sources | S 0.5 (+ a bot night) | Build | Med: no-oxygen death moves 300-700 m |
| BD3 | Game clock subsystem | Expedition hours, day number, 48-hour high-point ring, `Skip(h)` with the UDS instant change, weather moves on during sleep, time of day saved, `ygg.Time` | B4, C2-C7, B1 | your clock-speed choice | M 0.75 | Build | Med: UDS lights step every 4 s; realm schedule runs in real seconds |
| BD4 | Acclimatisation from nights | `ApplySleep`: gain per night by altitude, sleep quality from night SpO₂, recovery scaled by quality; `ygg.Accl.Mode/JumpGrants/JumpPlan`; old rule kept behind a switch; `accl_sim.py` twin | B4, A2 step 5, C2 (the hike), C4, C6, C7, B1 | BD3; your D3 (value of a night) | M 0.75 | Build + No | Med: thesis numbers; `ygg.Physio.Sweep` must not change |
| BD5 | Story sleep | `rest` story action (eyes close, clock to the wake time, `ApplySleep`, day + 1) + morning oximeter caption + SpO₂ dip in sleep | C3 (Camp 1), C4, C6, C7 | BD4, ST3 | S 0.5 | Build | Low |
| BD6 | Altitude illness | AMS → HAPE / HACE load from the altitude gap; effects through existing hooks (SpO₂ offset, speed, rig, sanity, HACE yaw drift); medicine items (dexamethasone, painkiller) | B4, C2 (medicine), C3, C4, C5, C6 (Paul) | BD4 | M 1.0 | Build | Med: medical accuracy; may it kill? (your call) |
| BD7 | Fatigue | +4 per game hour awake; sleep removes it less high up; lowers maximum stamina, more ladder sway; carries between chapters | C2 (`tired`), C3, C4, C5, C6, C7 | BD3, BD5 | S 0.5 | Build | Low |
| BD8 | Heat | Western Cwm sun: core towards 38.5 °C, water loss ×2, stamina recovery ×0.5; an open tent door cools | C3 (top), C5, C6 | — | S 0.5 | Build | Low |
| BD9 | Gloves and cold fingers / toes | Key swaps mitts / liners (HUD icon); finger-cold meter; frostnip makes clips ×1.5 slower; cold-toes index | C5, C6, B3 P2 | CL1 (clip timing) | M 1.0 | Build | Med: one more key; must teach, not punish |
| BD10 | Company and trust | Nima within 25 m restores sanity, radio call +3; fix the always-on wind sanity drain; trust 0-100 (Nima, Dr. Hale, Marek, neighbours), saved | C3, C4, C6, C7 | ST1 | S 0.5 | Build | Low |
| BD11 | Death recovery v2 | A checkpoint gives back the body as it was there (not full); failure forward (collapse near Nima → wake at camp, day lost); refuse a lethal checkpoint | C5, C6, C7 | ST6, BD3 | M 1.0 | Build | Med: must not trap the player in a death loop |

### 2.3 Camps and supplies (block B4 and the chapters)

| ID | System | What it does (one line) | Needed by | Depends on | Size (days) | Editor / build | Risk |
|---|---|---|---|---|---|---|---|
| CP1 | Camp stocks | Stock per camp (food, gas, oxygen, medicine) from `CampStocks.json`; drop a load into a stock; items Gas, Battery; frozen-bottle item state; `ygg.Camp.FreeRefill` | B4, C2 (bottle), C3, C4, C5, C6, C7 | ST1; your free-refill choice | M 0.75 | Build | Med: balance |
| CP2 | Stove | Melt snow: 15 game minutes and 1/8 cartridge per thermos; a cold canister must be warmed first (through `task`) | B4, C3, C6, C7 | CP1, ST3, BD3 | S 0.25 | Build | Low |
| CP3 | Load plan board | Choose which loads go to which camps; writes the stocks | C4, C7 | CP1 | M 1.0 | Build | Med: the screen's look is your call |
| CP4 | Player tent + camp life | `AYggCampTent` + `UYggCampLifeComponent`: enter, inside, sleep panel, wake, leave, on the existing postures; `place_player_tents.py` | B4, C3, C4, C6 (20 min in a tent), C7 | BD3, BD4 | M 1.25 | Build + Editor | Med: a 1.9 m capsule does not fit a dome tent |
| CP5 | Headlamp component | Modes, battery (if wanted), cold, flicker; story `light headlamp` moves onto it | B4, C2 (night walk), C3, C5, C6 | — | S 0.5 | Build | Low |
| CP6 | Camp HUD | Body line on the altimeter, sleep panel, morning line, watch glance, battery mark | B4 | BD3, BD4, CP4 | M 0.75 | Build | Med: look is your eye |
| CP7 | Rotation robot + chart | `ygg.Bot.Rotation` (plans A, B, C); "climb high, sleep low" staircase in `plot_session.py` | B4 test T2, B1 | BD4, BD6 | S 0.5 | Build + No | Low |

### 2.4 Climbing (block B3 and the chapters)

| ID | System | What it does (one line) | Needed by | Depends on | Size (days) | Editor / build | Risk |
|---|---|---|---|---|---|---|---|
| CL1 | Climbing foundation | `YggClimbTypes`, `SetActivity` costs that replace the walking terms, adrenaline, left / right mouse = hands, 4 settings rows, fixed-rope length / anchors / projection | B3, C5, C6 | — | M 1.0 | Build | Med: also edits `SurvivalComponent` (see section 5) |
| CL2 | View-rig motion layers | `SetMotion` (ascend, ladder, hang, slide), slip lease, slide posture, `OnHardReset`, watchdog; ground tremor; optional storm buffet | B3, C3, C5, C7 | CL1 (types) | M 0.75 | Build | Med: camera rules R1-R10; keep out of the 0.2-0.4 Hz sickness band |
| CL3 | Ladder crossing | Clip the hand line, balance and wobble, crampon catch, slip → hang or fall, pull-up, sway per crossing, Nima-holds hook | B3, C3 (6 ladders), C5, C6 | CL1, CL2, HZ1 | M 1.0 | Build | Med: "unclipped = death" is your call |
| CL4 | Rope ascending (jumar) | Push, step, step, breathe on rope of 30° or more; anchors, leash, Hold mode | B3, A5 (leg 7), C3 / C5 (leg 2) | CL1, CL2 | M 1.0 | Build | Med: summit gets slower; must not feel like a quick-time event |
| CL5 | Slip and self-arrest | Footing watch, slide, ice-axe arrest on the right mouse button, held, tumble, rope catch | B3, C3, C5, C6 | CL1, CL2 | M 1.0 | Build | Med: surprise slips on the trail |
| CL6 | Clipping by hand at key anchors | Ladder ends, serac-zone ends, steps over 25° (about 40 in the Icefall); automatic in between; setting "Steep only / Everywhere" | C5, B3 setting, (C3 optional) | CL1, BD9 | M 0.75 | Build | Med: default is your call |
| CL7 | Descending | Arm wrap below 35°, rappel on steep ice (×2 speed, brake input, too fast = slip) + the rig's Rappel state | C6, B3 P2 | CL2, CL5 | M 1.5 | Build | Med |
| CL8 | Climb robot, feel and tuning | `UYggClimbTest` cases C1-C14, A4 / A5 regression bots, jumar prop, HUD state line, `met / footing / balance` log columns, tuning from the robot tables, your test play | B3 | CL3, CL4, CL5 | L 2.5 | Build | Low |

### 2.5 Hazards and world set pieces

| ID | System | What it does (one line) | Needed by | Depends on | Size (days) | Editor / build | Risk |
|---|---|---|---|---|---|---|---|
| HZ1 | Legs 2-3 walkable (A4 ground) | Ladder 3 collision (507 cm too high), Football Field approach (49-52°), falls at ladders 2-3, leg-3 blocks; fixed rope starts at Crampon Point; trek rope per your choice | A4; **gate** for C3 / C5 | — | M 1.5 | Editor (+ Build) | High: the Icefall chapter cannot be tested before it |
| HZ2 | Snow bridge + hidden-crevasse variants | Holds a steady walk, breaks on a 2 s stand or a sprint; `hc_own`, `hc_cave` | C3, C5 | HZ1 | S 0.5 | Build + Editor | Low |
| HZ3 | Serac set pieces | Queue zone (no collapse, chip damage), `ygg.Serac.Drop` scripted collapse, detour rope, near miss fires once | C3, C5 | HZ1, ST6 | M 1.0 | Build + Editor | Med: frame time |
| HZ4 | Niagara effects pack | Smoke (juniper, sang, chimneys, kitchen: none exists today), serac powder cloud, spindrift burst, tsampa flour cloud, Icefall dust plume card | C1, C2, C3, C4, C5, C7 | — | M 1.5 | Editor | Med: look and 60 fps |
| HZ5 | Travelling avalanche cloud | Cloud moves along a path in ~20 s; white-out by fog; shelter test; glitter after; snow dust on tents and people | C7 | HZ4 | M 1.0 | Editor + Build | High: may look like plain fog (judge alone first) |
| HZ6 | Line of lights | Headlamp chain above as glowing cards that fade at dawn; at most 4 real lights | C3, C5 | — | S 0.5 | Editor | Low |
| HZ7 | Tent-holding storm event | Three gust waves, hold F, wall bulge (material offset), guy-line choice | C7 | ST3 | M 0.75 | Build + Editor | Med: cheap without cloth motion |
| HZ8 | Realms as story weather | Harmless-only realms per chapter, the Hungry Ghost wind look and effects, the 19:00 forecast card, Tibetan realm names | C4, C5, C6, C7 | your naming choice | M 1.5 | Build + Editor | Med: cultural check |
| HZ9 | Crampon steps + new hand items | `ygg.Feet.Crampons` by flag; hands for cup, crampons, carabiner, tsampa, juniper, snow sack, thermos, canister, phone | C2, C3, C4, C5, C6 | some props | M 0.75 | Build | Low |

### 2.6 Other people (NPCs)

| ID | System | What it does (one line) | Needed by | Depends on | Size (days) | Editor / build | Risk |
|---|---|---|---|---|---|---|---|
| NP1 | Team on ladders and ropes | Story NPCs cross ladders at ladder speed and move on the fixed rope up and down, one at a time at an anchor | C3, C5, C6 | HZ1, ST4, CL3 frames | M 1.0 | Build | Med-High: feet sliding, clips missing |
| NP2 | Route followers (other teams) | NPC climbers on `route.json` on their own schedule, same speed rules, headlamps, same body model as the virtual climber | C5 (big team), C6 (Paul), B1 | NP1, B1 | M 1.5 | Build | High: 6-9 NPCs on a 12 GB laptop |

### 2.7 Sound (block B5)

| ID | System | What it does (one line) | Needed by | Depends on | Size (days) | Editor / build | Risk |
|---|---|---|---|---|---|---|---|
| AU1 | Sound classes + Music slider | `SC_Music`, `SC_Mind` (whispers), classes for the Story folder, Music slider in settings and the self-test | B5, A7 | — | S 0.5 | Build + Editor | Low |
| AU2 | Music system | One music component; cues of 1-3 layers + intensity (M3 by SpO₂); triggers from data (place, altitude, body, story `music` key, summit, death); the §6.6 rules; `ygg.Music.*` | B5, all chapters | AU1, ST3; your music-source choice | M 1.5 | Build | Med: no music exists yet |
| AU3 | Mix system | Voice duck, altitude low-pass + hypoxia muffle tied to Body effects, wind exposure per place, tent muffle for all world sounds, oxygen-mask breath + regulator hiss | B5, CP4, A5 | AU1 | M 1.5 | Build + Editor | Med: judged by ear |
| AU4 | Place sounds on the route | Script places river, flags, camp voices, tent sounds; plays only near the player; long loops stream | B5, C1-C7 | sounds picked | S 0.5 | Editor | Low |
| AU5 | Sound candidates pipeline | Download list → cut and check → tour groups → import; covers B3, B4 and chapter sound lists | B5, B3, B4, all chapters | your OK for downloads, your ear | L 2.0 (+ your sessions) | Editor + Build + No | Med: needs your time |

### 2.8 View and visual systems (block B7; the art items themselves stay in `B7_Visual_Review.md`)

| ID | System | What it does (one line) | Needed by | Depends on | Size (days) | Editor / build | Risk |
|---|---|---|---|---|---|---|---|
| VS1 | One soft subtitle veil | One veil instead of grey bands; the HUD hides while a subtitle shows (G6) | every story chapter | — | S 0.25 | Build | Low |
| VS2 | Calmer body effects | Colour split later, frost at the edges only, a "clarity moment" lease at the summit and in dialogue (G1, S2) | A5, B7, all | — | S 0.5 | Build | Low (look is yours) |
| VS3 | Camera under the ground | Check first, then fix (G5) | all | — | S 0.5 | Build / Editor | Med: may not be real |
| VS4 | Sky by time and stretch | UDS real sun, moon and night (B4 §7); time of day per stretch, partly cloudy demo weather, valley clouds (G2); dawn moment with the Everest shadow wedge (B7 §12) | B4, B7, C2-C7 | BD3 | M 1.5 | Editor | Med: must measure fps |

### 2.9 Totals

| Group | Items | Days |
|---|---|---|
| ST story machine | 6 | 4.5 |
| BD body | 11 | 7.75 |
| CP camps | 7 | 5.0 |
| CL climbing | 8 | 9.5 |
| HZ hazards and set pieces | 9 | 9.0 |
| NP people | 2 | 2.5 |
| AU sound | 5 | 6.0 |
| VS view and visual | 4 | 2.75 |
| **All systems** | **52** | **47.0** |

Not in this total: B7 art polish (5 days in the plan), B1 virtual climber (9 days), and chapter content (section 3).

**Against the plan's estimates:** B3 items (CL1-CL5, CL8) = 7.25 days, plan says 7: fine. B5 items (AU1-AU5) = 6 days,
plan says 5. **B4 items (BD3-BD6, CP1, CP2, CP4-CP7, half of VS4) = about 7.75 days, plan says 4**: the B4 spec's own
4-day order has no room for the tent capsule problem, the story `rest` key or HUD look rounds. Re-estimate B4.

---

## 3. Build order: five milestones

Order rule: the gate (walkable route + story plumbing) first, then the smallest set that makes chapters 1-3 playable,
then the thesis systems, then sound, then what only later chapters need. Inside a milestone, items are in build order.

### M1 · Ground and plumbing (7.25 days)

| Order | Items | Days |
|---|---|---|
| 1 | HZ1 legs 2-3 walkable (this is A4 work) | 1.5 |
| 2 | ST1 flags saved + chaining, ST2 live numbers, ST3 story keys, ST6 survives death | 2.75 |
| 3 | ST5 story robot per chapter | 0.75 |
| 4 | BD1 physiology parity + log columns (A2), BD3 game clock | 1.5 |
| 5 | VS1 subtitle veil, VS3 camera-under-ground check | 0.75 |

**Done when:** the route robot reaches Camp 1 three runs in a row with 0 deaths and 0 stuck; `ygg.Story.Start Chapter2
flags=firstaid` starts with those flags and a chapter end starts the next; `physio_model.py --check` passes.

### M2 · Vertical slice: chapters 1-3 playable, Dingboche → Camp 1 (10.25 days systems + about 8.75 days content)

Minimum versions: illness, fatigue, camp stocks and trust are **flags only** here (the chapter docs allow this; every
beat has a fallback). The story tents and the story headlamp that exist are used.

| Order | Items | Days |
|---|---|---|
| 1 | BD4 acclimatisation from nights, BD5 story sleep (the C2 hike and the two Base Camp nights need them) | 1.25 |
| 2 | HZ4 Niagara pack (smoke first), HZ9 crampon steps + hand items | 2.25 |
| 3 | CL1 foundation, CL2 rig motion layers, CL3 ladder crossing | 2.75 |
| 4 | ST4 NPC modes, NP1 team on ladders and ropes | 2.0 |
| 5 | HZ2 snow bridge, HZ3 serac set pieces, HZ6 line of lights | 2.0 |

Chapter content in M2 (not systems; from the chapter docs): Chapter 2 story + world builder 2.0 · Base Camp chapter
(Chapter4_Design, Phase 1 = flags) 2.0 · day-3 walk as condensed travel (blink + teleport + short walks) 0.25 · Icefall
story + world builder 1.5 · Chapter 1 open items (staging asserts, tent wake) 0.5 · tent interiors (download first) and
climbing versions of the cast 1.0 · robot runs and fixes 1.5.

**Done when:** one save plays Ch1 → Lobuche → Base Camp → Icefall → Camp 1 with flags carried; every story run of the
chapter docs' test plans reaches its last beat; ≥ 60 fps at 1080p High at the worst spot; you have played it.
**If late:** CL3 moves to M3 (−1 day); the ladders then use today's slow walk plus the CL2 balance layer. CL1 stays,
because CL2 needs its types.

### M3 · Thesis body loop and climbing feel (B3 + B4 complete) (13.0 days)

| Order | Items | Days |
|---|---|---|
| 1 | BD2 A2 tuning, BD6 altitude illness, BD7 fatigue, BD10 company and trust | 2.5 |
| 2 | CP1 camp stocks, CP2 stove, CP5 headlamp, CP4 player tent, CP6 camp HUD | 3.5 |
| 3 | CP7 rotation robot (feeds B1) | 0.5 |
| 4 | CL4 rope ascending, CL5 slip and arrest, CL8 climb robot and tuning | 4.5 |
| 5 | VS2 calmer body effects, VS4 sky by time and stretch | 2.0 |

**Done when:** B3 and B4 "Done when" in the plan are met (an acclimatised climber does measurably better; each climbing
action works, fails with a clear cause and recovers); A4 / A5 bots still pass.

### M4 · Sound and music (6.0 days, plus B7 art polish 5 days from the plan)

AU1 classes + slider → AU5 candidates (your sessions) → AU2 music system → AU3 mix → AU4 place sounds.
Runs **beside M3** (different files, section 5). **Done when:** B5 "Done when" (you accepted every sound by ear).

### M5 · Systems only the later chapters need (C5-C7 extras) (10.5 days)

BD8 heat · BD9 gloves and cold fingers · CL6 clipping by hand · CL7 descending · BD11 death recovery v2 · CP3 load
plan board · HZ8 realms as story weather · HZ7 tent-holding event · HZ5 avalanche cloud · NP2 route followers.
**Done when:** the C6 and C7 test plans pass in their "full" version.

### Timeline check

| | Days | Fits where |
|---|---|---|
| M1 | 7.25 | Phase A (to 2026-12-14): HZ1 is A4, BD1 is A2; the rest is new |
| M2 | about 19 | **Not in the plan.** New scope; before or after the interim is your decision |
| M3 | 13.0 | Phase B blocks B3 + B4 (plan: 11 days) |
| M4 | 6.0 (+5) | Phase B blocks B5 + B7 |
| M5 | 10.5 | not in the plan; after the title demo or cut |

---

## 4. Not designed by any document (gaps found)

| Gap | Effect | Proposal |
|---|---|---|
| Day 3, Lobuche → Gorak Shep → Base Camp | Chapter 2 ends at Lobuche; Chapter 4 starts at Base Camp | condensed travel for the slice (0.25 day), a real design later |
| Two Icefall designs (C3, C5) | double work if both are built | pick one; the systems list is the same for both, except C5 adds gloves (BD9) and clipping (CL6) |
| Oxygen flow dial (`Chapter_Design_v1` §4.5) and pace from SpO₂ (§4.2) | not asked by chapters 1-7 | later chapters; B4 question 11 (sleeping oxygen) touches it |

---

## 5. What can run in parallel safely

**Hard limits:** only one C++ build at a time, and a build needs the editor closed. Only one agent in the editor at a
time. So code can be **written** in parallel, but one build owner merges and builds (for example two build windows a
day). Today another agent holds the editor: only lane P can run now.

### 5.1 Lanes (each lane = its own files; one agent per lane)

| Lane | Files it owns | Items | Editor? |
|---|---|---|---|
| **P** Python, scripts, docs | `Tools/Thesis/*.py`, `Tools/Story/chapterN_story.py` (+ `validate()`), `Tools/Bot/*.ps1`, `Tools/Audio/` cutting | BD1 Python half (`--check`), BD4 `accl_sim.py`, CP7 chart, all chapter story scripts, ST5 script half, AU5 cutting (after your OK), B1 fast simulation, B6 study scripts | **No: safe any time, even now** |
| **S** story code | `Story/YggStory.*`, `Survival/YggPlayBot.*` | ST1-ST6, plus the story keys of BD5 (`rest`), AU2 (`music`), HZ7 (hold) | build only |
| **B** body code | `SurvivalComponent.*`, `SurvivalConfig.h`, new `YggClock`, `AltitudeIllnessComponent`, `YggCampTent`, `YggCampLife`, `YggHeadlamp`, `InventoryComponent`, `SanityComponent` | BD2-BD11, CP1-CP7 | build only |
| **C** climbing code | new `YggClimbTypes`, `YggRopeAscend`, `YggLadder`, `YggSlip`, `YggClimbTest`; `FixedRope`, `CrevasseCrossing`, `ClimbingComponent`, `ClimberControlsComponent`, `FirstPersonBodyComponent` | CL1, CL3-CL8, HZ9 hands, NP1 ladder frames | build only |
| **R** view rig | `YggViewRig.*` | CL2, the CL7 Rappel state, the VS2 clarity lease | build only |
| **A** audio code | `MountainAudioComponent`, new music component, `SoundTourComponent` | AU2, AU3, AU5 tour groups | build only |
| **E** editor | levels, assets, Niagara, UDS, placement and world-builder scripts | HZ1, HZ4, HZ5, HZ6, VS4, AU1 / AU4 / AU5 imports, CP4 placement, HZ2 / HZ3 placement, `build_chapterN_world.py` runs | **one owner, never while another agent is in the editor** |

### 5.2 Safe pairs and unsafe pairs

| Together | Safe? | Why |
|---|---|---|
| P with anything | yes | no shared files, no editor, no build |
| S + C, S + A, C + A, B + A | yes | different files; one build owner |
| B + C | **only after CL1's `SetActivity` edit** | CL1 edits `SurvivalComponent` and `SurvivalConfig`; let lane B make that edit, or give lane C that file for one day |
| C + R | yes if one agent owns both, or CL2 lands first | CL3-CL5 call the rig's new `SetMotion` |
| E + any other E | **no** | one editor |
| Any build + E or the other agent's editor session | **no** | the build needs the editor closed |

### 5.3 Shared hot files (one lane at a time; the others hand in a small patch)

| File | Touched by |
|---|---|
| `Survival/ExpeditionComponent.cpp/.h` | CL1, BD3, BD11, CP4, CP5 |
| `Survival/SurvivalHUDWidget.cpp` | VS1, CP6, CL8 (state line), BD9 (glove icon) |
| `Survival/YggSettings.cpp`, `SettingsWidget.cpp` | CL1 (4 rows), CP5 (keys), AU1 (Music slider), BD9 (glove key) |
| `Survival/YggSaveGame.h` | ST1, BD3, BD10, CP1, CP5 (never bump `Version`) |
| `Survival/YggTelemetryComponent.cpp` | BD1, BD4, CL8 (append columns at the end only) |
| `Config/DefaultGame.ini` | CL1-CL5 sections, CP4, CP5 |

The existing `Docs/Development/patches/` folder is a good place for those hand-in patches.

---

## 6. Decisions for you (they block items above)

| # | Decision | Blocks | My proposal |
|---|---|---|---|
| 1 | Which chapter list: 12-chapter Chomolungma or 17-chapter v1? And which Icefall design: C3 or C5? | the whole slice (M2) | 12-chapter; C3 as the base, with C5's start-time choice |
| 2 | Build the chapters 1-3 slice **before** the 2026-12-14 interim, or after it (thesis first)? | M2 timing | M1 now (it is mostly A2 / A4 work); M2 after A8 |
| 3 | What one night at a camp is worth (Physiology D3: +300 m; B4: 400 / 250 / 150 by height) and the clock speed (24 or 6 game minutes per real minute) | BD3, BD4 | B4's values; 6 game minutes in story chapters |
| 4 | Free food and water in camps: off or on? | CP1, CP2 | off (stove and stock decide) |
| 5 | May altitude illness kill? Is an unclipped ladder fall death? | BD6, CL3 | illness forces you down; ladder fall = death |
| 6 | Clipping by hand: "Steep only" or "Everywhere"? | CL6 | Steep only |
| 7 | Music source (I make it, free downloads, you, or an AI service) | AU2 cues | your pick (B5 question 1) |
| 8 | Remove the yellow rope along the trek? | HZ1, route bots | yes, cairns and flags instead |
| 9 | Meshy credits (7 left, about 41 per character) for climbing versions of the cast and gear props | M2 content, CL8 prop | spend on Nima and Sangye's gear first |
| 10 | Realm names: keep Norse names in code but show Tibetan names to the player? | HZ8 | yes, after a cultural check |
