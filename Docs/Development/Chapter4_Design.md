# Chomolungma · Chapter 4 "Base Camp" · Design v1

Version 1.0 · 2026-11-20 · written by Claude (designer) for the director (the author). Implementation-ready draft:
every beat, place, line, asset and test is here, but nothing is approved until the director says so.

**How to read this.** Section 0 says which chapter list this follows (please read it first: there is a conflict).
Sections 1-3 are the chapter itself. Sections 4-6 are places, people and systems. Sections 7-9 are assets, sound and
tests. Section 10 is risks and questions.

**Labels used below** (the same as `Chapter1_v2_Design.md`, plus one)

| Label | Meaning |
|---|---|
| **[Built]** | exists in the project today |
| **[To build]** | planned, not started |
| **[Proposed]** | a new idea in this document; the director decides |
| **[VO]** | a spoken line; the director records the voices himself |
| **[To verify]** | a fact we believe but have not checked against a good source |
| **[Cultural check]** | a Sherpa or Tibetan Buddhist detail that a Sherpa reader must check before release |
| **F / S / C** | beat marks from `Chapter_Design_v1.md` §1.5: **F**ixed (always happens), **S**ystemic (the systems make it happen), **C**hosen (the player starts it) |

**Words used often**

| Word | Plain meaning |
|---|---|
| Puja | the Buddhist blessing ceremony at Base Camp before anyone climbs; a lama asks the mountain for permission |
| Lama | a Buddhist teacher or priest |
| Lhap-so | the stone altar the Sherpas build at their camp for the puja (spellings vary) [Cultural check] |
| Khata | the white silk scarf given as a blessing or greeting |
| Sungdi | a thin red protection cord the lama ties around your neck [Cultural check: spelling] |
| Tsampa | roasted barley flour, the Sherpa staple food; thrown into the air at the end of the puja |
| Sirdar | the head Sherpa of an expedition |
| Lease | the camera rig's rule that anything the story holds (a pose, a look, the walk) has an owner and a time limit, and is given back automatically (`Chapter1_Camera_Spec.md` §4.4) |
| Nudge | the last-resort slow turn of the view toward something, which gives way to the mouse at once (§6.2) |
| Posture | a real body state that sets the eye height: stand 156 cm, sit 78, kneel 108-110, lie 32, crawl 68 (§4.3) |
| Load | one porter's or Sherpa's carry, about 15-20 kg, taken up to a higher camp and left there |
| Realm | this world's weather: random spells of a few minutes, each with its own look and danger; in Chomolungma the Six Realms of Buddhist cosmology |

---

## 0. Which chapter list this follows (read first)

The task asked for **Chapter 4 as `Chapter_Design_v1.md` describes it**, with every hand-off kept exactly. There, Chapter
4 is **"Base Camp"**: 40 minutes, no walking, game day 3 evening to day 4, a crowd, and the goal "permission to climb:
the doctor's check, the puja, the plan, the stocking".

But `Chapter_Design_v1.md` is the **old 17-chapter plan with the old story** (parents vanished twenty years ago, Norse
runes on the compass, Nima's father as sirdar). On 2026-11-14 the director moved to **Chomolungma** with **12 chapters**
(`Chomolungma_Chapter1.md` Part 1). There, Chapter 4 is **"The Valley of Silence"** (the Western Cwm to Camp 2), and the
puja sits in Chapter 2. The finished Chapter 1 (`Chapter1_v2_Design.md`) uses the Chomolungma story and cast.

**What this document does.** It keeps the **slot** of `Chapter_Design_v1.md` Chapter 4 exactly (place, time, company,
goal, decisions, beats, hand-offs in and out), and tells it with the **Chomolungma story and cast** of the finished
Chapter 1, because Chapter 1 is what the player has already played. The table shows each translation.

| `Chapter_Design_v1.md` Chapter 4 says | This design uses | Why |
|---|---|---|
| Sangye arrives as a client who needs "permission to climb" | Sangye is the **newest climbing Sherpa** of Marek Kovač's expedition (Chapter 1 v2). "Permission" = Dr. Hale's medical clearance + the puja (no Sherpa enters the Icefall before it) + Marek's plan | Chapter 1 made him a worker, not a client |
| Dr. Hale sees the compass's Norse runes: "Old Norse? Here?" | Dr. Hale is the credible voice: she explains the gold light as hypoxia, nods once to her own people's world-tree, and (if asked) reads the father's last radio call | the Norse layer is re-rooted in Tibetan Buddhism (Chomolungma) |
| "The other team attends: Marek, Elena, Ji-woo" | Marek, Elena and Ji-woo are **Sangye's own team** (Chapter 1 v2). A **neighbouring team** next door takes the "other team" role in this chapter | Chapter 1 v2 cast |
| "The memorial for last week's death" | the Icefall doctors' small altar for a doctor who died last week (only confirms a rumour if Chapter 2 planted one) | Chapter 1 v2 has no such rumour yet |
| (not in v1) | the first realm at Base Camp, the **Hungry Ghost wind**, turning the camp's water into a fight | from the Chomolungma sentence for the puja chapter; gives this hub chapter its tension (D2: tension over poetry) |

**The hand-offs, kept exactly as `Chapter_Design_v1.md` says them:**
- **In** (end of Chapter 3): *"Base Camp at dusk: tents glowing, the Icefall lit pink above; Dr. Hale meets them."* Game
  day 3, about 17:00. Nima is with Sangye.
- **Out** (start of Chapter 5): *"Sleep; the alarm set for 02:30"* (the default of the alarm choice), and the question
  *"Three in the morning. The Icefall."* Chapter 5 starts game day 5, 03:00, at the Icefall entrance, and its decision 1
  ("start time, the alarm set in Chapter 4") reads the alarm flag written here.

**Decision for the director (Q1, section 10):** if the 12-chapter Chomolungma list is the real plan, this document is the
design for the **Base Camp half of Chomolungma Chapter 2 "The Puja"**, and nothing in it needs to change except the
chapter number and the day count.

---

## 1. One-page summary

### What the chapter is

Sangye walks into Everest Base Camp at dusk: a town of tents on a moving glacier, under the groaning Khumbu Icefall. In
one evening and one day he eats with the cooks, is measured by Dr. Hale, survives his first night at 5,274 m, kneels at the
puja where a lama asks the goddess of the mountain for permission, helps decide what the team carries up the mountain,
and stands in a dry, thirsty wind that turns the camp's water into an argument. He goes to sleep with the alarm set for
the Icefall.

### The feeling

**Quiet awe and belonging, with a clock ticking.** For the first time Sangye is inside the world his father worked in
for many seasons. People know his father's name. The puja is the most peaceful and the most crowded moment of the game
so far. Underneath, every scene is a quiet test: of his body (the oximeter, the heel-to-toe line, the night), of his
judgement (the loads, the water) and of what he will give away. The Icefall is always above, always audible.

### The shape

| Part | Real min | Game clock | Tension (1-5) | Company |
|---|---|---|---|---|
| A · Arrival, the kitchen tent | 0-3 | day 3, 17:10-18:40 | 2 | crowd |
| B · The medical tent | 3-10 | 19:00 | 3 | Dr. Hale |
| C · First night at 5,274 m | 10-12 | 21:00 → 01:40 → 06:30 | 4 | alone |
| D · The puja: **the peak** | 12-21 | day 4, 08:30-10:40 | 1 → 3 (awe) | crowd |
| E · The plan board | 21-27 | 11:00 | 3 | the team |
| F · Walk the camp | 27-31 | 12:00-15:30 | 2 | Nima, strangers |
| G · The Hungry Ghost wind | 31-37 | 15:30-18:30 | 4 | crowd, tense |
| H · Night: the alarm: **the end** | 37-40 | 19:00-21:00 | 3 | Nima, Dr. Hale |

### Length

**40 minutes** (`Chapter_Design_v1.md` §2.4: walk 0, other 40). Base Camp is small: the longest walk is 409 m to the
Icefall doctors' tent and back (about 3 minutes each way at 2.3 m/s). The robot player measures the real time.

### How it serves the thesis

Base Camp is where the physiology model is measured **in person** for the first time: SpO₂, pulse, the Lake Louise
headache questions and the heel-to-toe test, by a doctor who runs a study. The first night shows periodic breathing (the
breath stops and starts during sleep, normal at this height). Two nights here are the first step of acclimatisation. And
the one mythic hint of the chapter is a **number**: the father's last radio reading, 54% at 8,400 m, which is exactly the
value Grocott et al. measured there in 2009 (the published point already in `Tools/Thesis/physio_model.py`).

---

## 2. Purpose in the arc and the emotional line

### 2.1 Purpose

1. **The last safe place.** Above Base Camp nothing is skipped and nothing is safe (`Chapter_Design_v1.md` §2.7). This
   chapter is the deep breath before it. Rest before the storm, but every rest carries a decision (pillar P5).
2. **Permission.** Three permissions, from three kinds of authority: the body (Dr. Hale), the mountain (the puja), the
   employer (Marek's plan). The player can bend the first, cannot skip the second, and can argue with the third.
3. **Teach the expedition.** Loads, camp stocks, the forecast, the 19:00 check: the systems of Loop 4 (the rotation) are
   introduced here, before the Icefall needs them.
4. **Deepen the father.** People at Base Camp knew Mingma. The cook, the lama and Dr. Hale each give one piece. None of
   them says how he died (that belongs to Nima, later).
5. **Company.** The most crowded chapter after Chapter 1 (D4: not alone the whole climb). It makes the Icefall's
   loneliness felt by contrast.

### 2.2 The emotional line

| Moment | What the player should feel | How |
|---|---|---|
| Arrival | relief, warmth, a little lost | noise and light after the empty glacier; the cook says the father's name |
| Medical tent | being judged | numbers you cannot hide; one question you can lie to |
| 01:40 | fear of your own body | the breath stops; nobody is there; Dr. Hale's warning was true |
| The puja | quiet awe, belonging | smoke, chant, the sun reaching the altar as the pole goes up, tsampa on every face, laughter |
| The compass on the altar (if placed) | a shiver of fate | the needle points up the mountain, then slowly back to north; Nima says "Put it away" |
| The plan board | weight of responsibility | the loads you choose will be there, or not, on the summit push |
| The Hungry Ghost wind | tension, guilt | everyone is thirsty; someone asks for water; every answer costs |
| Night | resolve | the alarm; Nima's "stay close to me"; the Icefall talking in the dark |

**Sense of fate.** The father's presence is everywhere and nowhere: his name in the cook's mouth, his last words in Dr.
Hale's log, his compass pointing up. Sangye is walking into a story that was already written for his father.

**Tension over poetry.** The puja is the chapter's poetry. It is placed between the night gasp and the plan board, and it
carries a decision (what to bless) and a small mystery (the compass). The Hungry Ghost wind keeps the second half tense.

### 2.3 The decisions (pillar 2: at least three real decisions with consequences later)

| # | Decision | Options | Costs and consequences | Read by |
|---|---|---|---|---|
| D1 | **The headache** (only if the body has AMS) | tell the truth · "I'm fine" · say nothing | Truth: Dr. Hale caps your first carry at 10 kg, Nima takes more; Hale trust +10. Lie: full load, the illness keeps growing unseen into the Icefall; if the heel-to-toe test shows it, Hale trust −10 and the cap anyway | Ch5 load, Hale trust, illness model |
| D2 | **The twelve loads** | Marek: oxygen first · Nima: tents and rope first · half and half | Which camp stocks exist on later rotations. Marek: fewer tents at Camp 2 on rotation 2. Nima: fewer bottles at the South Col if the window comes early. Half: both thin, more carries later. Marek respect / Nima trust ± | Ch8-12 camp stocks |
| D3 | **The pack split with Nima** for rotation 1 | I take the rope (17 kg / Nima 13) · even (15 / 15) · Nima takes the rope (13 / 17) | Your stamina and pace in Ch5; Nima's fatigue; what is lost if he is hurt later | Ch5, Nima's body |
| D4 | **What to place at the altar** | father's compass · father's prayer beads (if packed in Ch1) · Dolma's drawing · only axe and crampons | Each becomes "blessed" in the journal and in later lines; small sanity effects [Proposed, Cultural check]; the compass opens the myth beat | journal, Ch5+ lines |
| D5 | **The water** (in the Hungry Ghost wind) | give your own thermos · take the boy to Dr. Hale · say no | Thermos: Ch5 starts thirsty (hydration 60). Dr. Hale: a walk in the dry wind and 40 min less sleep (fatigue at 02:30). No: Marek respect +, the neighbours' client's fate is left to the systems, neighbour goodwill − | Ch5 hydration and fatigue, neighbour goodwill |
| D6 | **The alarm** | 02:00 · 02:30 · 03:30 · 05:30 | Chapter 5 decision 1: earlier = colder and safer ice, later = light and warmth but the sun on the towers (serac clock ×1.7 from 11:00) | Ch5 start |

There are also two optional questions to Dr. Hale (the father's last call, the gold light): story, not mechanics.

### 2.4 The teaching ladder (one new thing per scene, taught by a person)

| Mechanic | Introduce | Develop | Twist | Conclude | Taught by |
|---|---|---|---|---|---|
| Measured by a doctor | the oximeter on your finger | your number, read aloud | the question you can lie to | the heel-to-toe line tells the truth anyway | Dr. Hale |
| Periodic breathing | Dr. Hale warns you | 01:40: the breath stops | moving makes it worse | holding still, it settles | Dr. Hale, then the body |
| The puja (participation, not a skill) | sit with the Sherpas | rice thrown during the chant | the lama ties your cord | "Lha gyalo!" and the tsampa | Nima |
| Loads and camp stocks | the plan board | three plans argued | your choice moves the magnets | the stock appears under each camp | Marek and Nima |
| The pack split | your 15 kg | rope is heavy, gas is light | the doctor's cap, if any | the pack weight on the HUD | Nima |
| The forecast | Dr. Hale reads three days | "something from the west" | the wind arrives today | it ends at dusk, as she said | Dr. Hale |
| Ladders (introduce only, optional) | a practice ladder over a 1 m trough | crampon points on the rungs | it sways | across: Chapter 5 develops it | Nima, the Icefall doctors |

**Not taught here:** clipping at anchors and real ladders (Chapter 5), the stove (Chapter 6), oxygen (later).

### 2.5 The open questions it leaves (Loop 5)

1. *"He said he could see the way."* What way? (only if the player asked Dr. Hale)
2. Why did the compass point up the mountain at the altar? Why did Nima tell him to hide it?
3. Will the loads chosen today be enough?
4. **The chapter's question, from `Chapter_Design_v1.md`: *"Three in the morning. The Icefall."***

---

## 3. The beats

Minutes are real minutes from the chapter start. Every scene ends where the next one's trigger begins. Place names in
`code` are story places (section 4.2). Camera words come from `Chapter1_Camera_Spec.md` §4 and §6: the story only asks
the view rig; it never turns the head.

### Scene A · Arrival and the kitchen tent (0-3 min) · day 3, 17:10 → 18:40

| Beat id | Trigger | What happens (does / sees / hears) | Lines [VO] | Camera | Mark |
|---|---|---|---|---|---|
| `c4_arrive` | chapter start (continues from Chapter 3's last beat at `bc_gate`) | A short blink (0.15 / 0.5 s) in which the chapter's cast is dressed (`preload cast`; Nima and Dr. Hale are reused if Chapter 3 left them standing). Then control stays with the player. Base Camp at dusk: tents glowing orange and yellow up the glacier, voices in many languages, a generator, a pressure-cooker whistle, the Icefall groaning above in pink light. Caption *"Day 3 / 49 · Everest Base Camp, 5,274 m"* lower left. Dr. Hale walks off to her tent; Nima leads to the kitchen. | **Dr. Hale:** *"Tea first. Then I want your finger. Both of you."* **Nima:** *"Kitchen is there. Pasang will feed you."* | no lease; Nima `lead` | F |
| `c4_kitchen` | near `kitchen_door` r 4 m | The Sherpas and cooks eat in the kitchen tent; the clients eat in the dining tent [To verify: common practice]. Steam, a roaring kerosene burner, a Nepali radio, sacks of rice. Pasang, the head cook, sees him. Sangye sits on the stone bench. Dal bhat and garlic soup: food and water refill. | **Pasang:** *"Mingma's son? Sit. Eat. Your father always ate two plates before the Icefall."* | `pose sit` (face Pasang), `hands bowl` (fallback: caption); reflex 10° to the pressure-cooker whistle (S, once) | F |
| `c4_elena_echo` | after `c4_kitchen` 25 s; flag-dependent | **If `elena_load` or `elena_water` (Ch1):** Elena looks in at the door. **If `elena_push`:** Ji-woo looks in instead, filming. | Elena: *"There you are. Marek says the Sherpas eat here. Can I sit with you for one minute? It's warmer."* · Ji-woo: *"The real kitchen of Everest! Say hi, Sangye."* | none | S |
| `c4_kitchen_end` | after the echo, or 60 s | Nima stands. | **Nima:** *"Doctor is waiting. Go now, before you sleep sitting."* | moving stands you up (rig) | F |

### Scene B · The medical tent (3-10 min) · day 3, 19:00

The medical tent is a big dome with a warm lamp: oxygen cylinders, a folded Gamow bag (a portable pressure chamber for
altitude illness), a laptop with charts, a whiteboard of names and numbers. **No red cross emblem** anywhere (it is
protected by law); a green cross or the word "Clinic" instead.

| Beat id | Trigger | What happens | Lines [VO] | Camera | Mark |
|---|---|---|---|---|---|
| `c4_med_enter` | near `med_door` r 2 m, look + `press` "[E] Go in" | A blink; inside, sitting on a box facing Dr. Hale. | **Dr. Hale:** *"Sit. Give me your hand. Not that one, the warm one."* | `blink` 0.15/0.5 with `teleport med_sit` + `pose sit snap` + Hale moved to `med_hale` in `during` | F |
| `c4_oximeter` | after enter 2 s | She clips the oximeter on his finger. He holds it up: the number is the **true** model value (section 6). | Band ≥ 84: *"Eighty-five. Pulse ninety. For a boy from Dingboche, good."* · 78-83: *"Eighty-one. Normal for here."* · < 78: *"Seventy-six. Low. We watch that tonight."* (the spoken number is the live value; one line per band, the director records the numbers 70-90) | `hands oximeter` + `inspect oximeter` (the screen shows SpO₂ and pulse; closes on E) | F |
| `c4_lakelouise` | after oximeter | **Only if the body has AMS** (illness model, or the `ams` flag from Ch3): a thought caption first, so the player knows the truth: *"Your head has pounded since Gorak Shep."* Decision **D1**. Without AMS she just notes it. | **Dr. Hale:** *"Headache? Sick stomach? Dizzy? Tell me true."* → choice *Tell the truth · "I'm fine." · Say nothing* · Truth: *"Then you carry ten kilos tomorrow, not fifteen. Not a punishment. Arithmetic."* · Fine: *"Good."* (she writes) · Nothing: *"I'll take that as a yes."* (counts as truth) · No AMS: *"No headache. Good."* | none (choice while sitting) | S/C |
| `c4_heeltoe` | after Lake Louise | *"Stand. Walk the tape. Heel to toe."* A 3 m tape line on the floor. The player walks it. The rig's exhaustion sway and a small input drift grow with the illness and fatigue state; Dr. Hale watches the feet. | Pass: *"Straight as a Sherpa."* · Wobble after "I'm fine": *"You told me no headache. Your feet say yes. Ten kilos."* · Wobble after truth: *"Rest. Drink. We look again in the morning."* | stand; `pace 0.3` for the line (lease, released at the line's end); look free | F/S |
| `c4_ask` | after heel-toe | Choice: *"Were you here last season?" · "At Thukla I saw a light on the summit." · Say goodnight.* One question only; the other stays open for Chapter 7 ("her certainty cracks"). | **Last season:** *"I was. I was the doctor on your father's team."* (pause) *"His last call came at ten past two, from the Balcony. Fifty-four percent. He said he was fine. He said he could see the way."* **Sangye:** *"The way?"* **Hale:** *"That is what he said. Then nothing."* · **The light:** *"At that height the eye does beautiful things. Your number at Thukla was eighty-four."* (the live value logged on Day 1) (pause) *"Where I come from, we also had a tree that holds up the world. Every people has one."* | `inspect hale_log` for the first (her radio log page: *"02:10 · Mingma · Balcony · 54 % · 'fine, can see the way'"*) | C |
| `c4_med_end` | after the question | | **Dr. Hale:** *"Sleep. At two you will wake and think you are dying. You are not. It is the altitude. Lie still, breathe, and sleep again."* | blink out to `med_door` | F |

**[Proposed, director decides]** The "last season" answer adds canon: Dr. Hale was the father's team doctor, and his
last call came from the Balcony at 02:10. It fits the prologue (the father sits at the Balcony at 02:30) and does not say
how he died. If the director does not want it, she answers *"No. I was in Kathmandu with a broken wrist. I'm sorry."*
and the question stays open.

### Scene C · First night at 5,274 m (10-12 min) · day 3, 21:00 → day 4, 06:30

| Beat id | Trigger | What happens | Lines [VO] | Camera | Mark |
|---|---|---|---|---|---|
| `c4_night1` | near `sangye_tent` r 3 m, `press` "[E] Sleep" | A blink; lying in his bag in his tent. The headlamp clicks off. Ice creaks under the tent. Time glides 21:00 → 01:40 behind closed eyes. | none | `blink sleep` 2.5 s; `during`: `teleport tent_c4`, `pose lie snap`, `time 100` (01:40) | F |
| `c4_gasp` | after `c4_night1` | 01:40. The eyes open (`wake`). The breath stops, then three deep gasps; the heart pounds. Darkness, the Icefall cracking far above, one distant avalanche boom. **If the player moves or presses keys, the gasps return; holding still for 6 s lets the breath settle.** | 50% chance (S): **Nima** through the tent wall: *"Sangye? ...Breathe slow. It passes."* | `blink wake`; `breath 0.8 t 6` (Breath lease), heartbeat sound; reflex 10° toward the avalanche boom; the posture stays `lie` (look limits 70°) | S |
| `c4_dawn` | 6 s still, or 40 s | The breath settles; `blink sleep`; 06:30; eyes open; sit up; crawl out of the tent door into the blue morning shade; the sun is warm only on the tops of Pumori and the Icefall. Caption *"Day 4 / 49"*. | **Nima** (outside): *"Today, no Icefall. Today we ask."* | `blink sleep` → `time 390` → `wake`; `pose sit` with `exit tent_out_c4` (the Chapter 1 crawl-out, reused) | F |

### Scene D · The puja (12-21 min) · day 4, 08:30 → 10:40 · the peak

The lama and his helper have come up from a gompa down the valley [fictional; no real lama]. The altar has been built
by the Sherpas at the centre of camp, with a tall pole. Juniper smoke rises. The clock glides 08:30 → 10:40 over the
scene, so the sun line crosses the camp **during** it (staged with light, section 4.3).

| Beat id | Trigger | What happens | Lines [VO] | Camera | Mark |
|---|---|---|---|---|---|
| `c4_puja_call` | after `c4_dawn`, the player stands | A hand drum and a bell from the altar (sound first, then the smoke). The clock glides to 08:30. People walk toward the altar carrying gear. | **Nima:** *"Bring your axe and your crampons. Everything that will bite her, she must bless."* | lure: the drum at `puja_altar` (7 s), companion Nima turns his head (3 s); **no nudge** | F |
| `c4_puja_place` | near `puja_altar` r 5 m, `press` "[E] Place your gear" | Sangye leans his axe and crampons against the altar with everyone's (automatic). Decision **D4**. | choice *Father's compass · Father's prayer beads* (only if `mala`) *· Dolma's drawing · Only my axe and crampons* · Nima, by choice: compass *"...Good."* (he looks at it a beat too long) · beads *"He would like that."* · drawing *"She will be happy."* · only gear *"Good. Keep him in your pocket."* | `pose kneel1 face puja_altar`, `hold` for 2 s, released | C |
| `c4_puja_chant` | after place | Sangye sits with the Sherpas on a mat facing the altar; the clients sit on camp chairs behind. The lama chants; his helper keeps the drum. Offerings on a low table: tsampa, rice, biscuits, fruit, beer cans, a bottle, butter lamps, torma (offering cakes). At intervals everyone tosses a pinch of rice (automatic, Sangye's hand too). Ji-woo films quietly. The light: the sun line creeps down the glacier toward the altar. | **Elena** (whisper, behind): *"What is he saying?"* **Nima** (low): *"He asks the mother goddess to let us walk on her. And to forgive our crampons."* (later, if the player looks at Nima): **Nima** murmurs *"Om mani padme hum"* | `pose sit face puja_altar`; `attention -3 t 4` (only while still) | F |
| `c4_puja_pole` | after chant 90 s | The Sherpas raise the flag lines from the pole out to the tents; the sun reaches the altar at the same moment; everyone looks up; the flags snap against the deep blue sky with the Icefall behind. | (no words: the flags and a rising "ah" from the crowd) | staging + companion Nima looks up; **nudge 1 of 2** to `pole_top` only after 12 s of not looking (pitch max 8°) | F |
| `c4_puja_sungdi` | after pole 20 s | The lama walks along the rows tying a red cord around each neck and laying a khata over it. He reaches Sangye. **If `blessed_compass`:** he lays his hand on the compass in Sangye's lap for a moment. | **The lama** (soft, in Sherpa language, subtitled): *"For Mingma's son. Go slowly, come back."* [Cultural check: wording, language] | `attention -3 t 4`; `wear khata on` (replaces the khata left at Thukla in Ch1) + `wear sungdi on` (P2; fallback caption) | F |
| `c4_lha_gyalo` | after sungdi, all rows done | Everyone stands with a handful of tsampa. Three shouts, *"Lha gyalo!"* ("victory to the gods" [Cultural check]). On the third, the player throws (prompt "[F] Throw"); if he does not, Nima throws for him on the fourth second. The air turns white; laughter; Nima smears tsampa on Sangye's cheek; Elena laughs with a white face; even Marek. | **Crowd:** *"Lha gyalo! Lha gyalo! Lha gyalo!"* **Nima:** *"Until your hair is white like this."* | stand; reflex 10° at the first shout; `hands tsampa` → throw (a Niagara flour puff); no lease | F |
| `c4_compass_back` | only if `blessed_compass`; after `c4_lha_gyalo` 10 s, near `puja_altar` r 3 m, look + press "[E] Take the compass" | Sangye picks the compass off the altar. The needle points **up the glacier, toward the summit** (bearing ≈ 100°), not north. Over 4 s it swings slowly back to north. Only he is looking. Nima is beside him. | **Nima** (quietly): *"...Put it away. People will ask."* | `pose kneel1`; `inspect compass` with `compass {"toward": "summit", "t": 4}` then free; no fovbeat (the chapter has none) | C |

### Scene E · The plan board (21-27 min) · day 4, 11:00

The board stands outside the dining tent in the noon sun (glare: glacier glasses on). Four columns: Camp 1, Camp 2,
Camp 3, South Col. Twelve load magnets: oxygen, tent, rope, gas, food, medicine. Marek, Nima and Dr. Hale stand around it.

| Beat id | Trigger | What happens | Lines [VO] | Camera | Mark |
|---|---|---|---|---|---|
| `c4_board` | near `plan_board` r 3 m | Marek explains. Nima disagrees. They turn to Sangye. Decision **D2**. Phase 1: three presets as a choice. Phase 2: the plan screen, where the player may also move magnets by hand. | **Marek:** *"Twelve loads before the push. Oxygen first. I want eight bottles at the South Col before anything else goes up."* **Nima:** *"Tents and rope first. Without tents at Camp 2, your oxygen sleeps in the snow."* **Marek:** *"Sangye. You will carry it. What do you say?"* → choice *Oxygen first · Tents and rope first · Half and half* · replies: Marek *"Good man."* / Nima *"Hm. Good."* / Marek *"A diplomat. Fine."* | Phase 2: the plan screen (menus do not break leases) | C |
| `c4_forecast` | after board | Dr. Hale reads the first three-day forecast from a printout; the board shows three day-cards (weather, summit wind, how sure she is). | **Dr. Hale:** *"Tomorrow, clear and cold until noon. The day after, the same. Then wind on the summit, sixty kilometres, and something I don't like coming from the west."* (pause) *"And this afternoon, a dry wind here. Drink more than you think."* | none | F |
| `c4_split` | after forecast | Nima kneels by two packs and the first carry for Camp 1: a 100 m rope coil, two gas cartridges, rations, a tent. Decision **D3**. If `load_light`: the choice is replaced. | **Nima:** *"Tomorrow, first carry to Camp 1. Fifteen kilos each. Rope is heavy, gas is light. Who takes what?"* → *I take the rope · Split it even · You take the rope* · rope: *"Strong is not the same as clever. But good."* / even: *"Good."* / Nima: *"Hm. My knees remember your father."* · if `load_light`: *"Doctor says ten. Doctor is boss."* | none | C |

### Scene F · Walk the camp (27-31 min) · day 4, 12:00 → 15:30

Free time. The clock runs while walking. One optional place on the way and one at the end; Nima comes if the player
walks up-glacier (`follow`).

| Beat id | Trigger | What happens | Lines [VO] | Camera | Mark |
|---|---|---|---|---|---|
| `c4_neighbours` | near `neighbour_camp` r 15 m | The team next door: music, a satellite dish, a dog asleep on a duffel [To verify: dogs live at Base Camp]. Their kitchen boy, **Dorje**, struggles with a frozen water pump. (Seeds Scene G.) | **Dorje:** *"Pump is dead. Ice everywhere. Ah, this mountain."* | none | S |
| `c4_doctors` | near `icefall_doctors` r 12 m | Two Icefall doctors (the Sherpas who fix the ladders and ropes through the Icefall every season) lash two aluminium ladders together. A small altar beside their tent: a fresh khata and a butter lamp for one of them who died last week. **If Ch2 left `rumour_death`**, this confirms the rumour. | **Icefall doctor:** *"The ladders at the Popcorn field moved. We tied a new one this morning. Tell your clients: one at a time, and nobody stops under the towers."* · at the altar (if asked or looked at 3 s): *"Our brother. A tower came down at six in the morning."* | lure: the clank of ladder rungs; no nudge | C |
| `c4_ladder` | at the doctors' tent, look at `practice_ladder` + press "[E] Try it" | A practice ladder across a 1 m ice trough. Crampon points scrape on the rungs; it sways a little. A fall here only drops 1 m (harmless). | **Nima:** *"Look at the rung, not the hole. Today the hole is small."* | the existing ladder walk (×0.45 speed, sway); no lease | C |
| `c4_wind_warn` | after `c4_doctors`, or 5 min after `c4_split`, whichever comes first | The realm wheel turns and warns (existing wheel and warning; the HUD): the **Hungry Ghost wind**. Wherever the player is, he walks back to camp in it. | **Nima** (radio, or beside him): *"Wind is coming. Come back to the kitchen."* | none | S (forced on a first play) |

### Scene G · The Hungry Ghost wind (31-37 min) · day 4, 15:30 → 18:30

**What it is in the world:** a dry, pale wind off the glacier. The sky loses colour, the light goes flat and dusty, ice
crystals hiss on the tent fabric. Everything is thirsty: water drains 2.5 times faster outside shelter, the water barrels
freeze solid, the stoves cannot keep up. **What it means** (only for those who know): in the Six Realms, the hungry
ghosts (Sanskrit *preta*; Tibetan *yi dwags* [Cultural check]) suffer endless hunger and thirst. No ghosts are shown:
this is still the credible half of the game. The meaning lives in Nima's one line and the cook's offering.

| Beat id | Trigger | What happens | Lines [VO] | Camera | Mark |
|---|---|---|---|---|---|
| `c4_wind` | realm active | The look and effects start (section 6). Thirst on the HUD within minutes if the player stays outside. | **Nima:** *"My grandmother said on days like this the hungry ghosts walk. Everything they touch is thirsty."* [Cultural check: a folk belief we must not invent] | none | S |
| `c4_barrels` | near `water_barrels` r 6 m | Pasang hammers at a frozen blue barrel. There is just enough melted water for the team's thermoses for the morning. Then **Dorje** runs in from next door: their client is sick, their pump and gas line froze, they need hot water. Marek and Pasang argue. Elena watches. Ji-woo films. Decision **D5**. In the background Pasang burns a little tsampa on the kitchen fire (a *sur* offering for the hungry ones [Cultural check]). | **Dorje:** *"Please, dai. Our client is sick. Hot water, only one thermos."* **Marek:** *"We have twelve thermoses for twelve people. Not our problem."* **Pasang** (to no one): *"His father would give."* → choice *Give him my thermos · Take him to Dr. Hale · Say no* · thermos: **Dorje** *"Thank you, dai. Thank you."* · Dr. Hale: **Hale** *"Right. Show me."* (she grabs her bag; you carry the pot) · no: **Dorje** goes without a word; **Marek** *"Hard, but right."* | reflex 10° at the barrel cracking; no lease | S/C |
| `c4_hale_walk` | only if "Take him to Dr. Hale" | Sangye carries a pot of water 155 m to the neighbours' camp and back beside Dr. Hale, in the wind. The client lies in a tent, coughing. Hale works quickly. The walk costs water, warmth and 40 minutes of sleep tonight. | **Dr. Hale** (inside the neighbours' tent): *"Oxygen, two litres. And he goes down tomorrow, with or without his summit."* | blink back to camp when the scene ends (condensed walk, Base Camp only) | C |
| `c4_wind_end` | realm ends (about 6 min of real time, held by the story) | The wind drops at dusk. Colour comes back: violet sky, the shadow climbing the valley, the Icefall top warm. Breath fog. Silence, then the camp's voices again. | none | none | S |

### Scene H · Night: the alarm (37-40 min) · day 4, 19:00 → 21:00 · the end

| Beat id | Trigger | What happens | Lines [VO] | Camera | Mark |
|---|---|---|---|---|---|
| `c4_1900` | near `kitchen_door` r 5 m after `c4_wind_end` | The 19:00 check, in person this time: Dr. Hale at the kitchen door with the oximeter, a number for everyone. News from next door by the D5 choice. | **Hale:** *"Elena, seventy-six. Drink. Sangye, eighty-two."* (live values) · if helped: *"Their client is on oxygen. He walks down tomorrow."* · if not: *"Their client went down at dusk with two Sherpas. We'll see."* | none | F |
| `c4_alarm` | near `sangye_tent` r 3 m, `press` "[E] Sleep" | A blink; sitting in his tent, the two packs at the door, Nima kneeling in the doorway with his headlamp. Decision **D6**. | **Nima:** *"We leave when the ice is coldest. What time do you want to wake?"* → *02:00 · 02:30 · 03:30 · 05:30* · 02:00: *"Good. Your father liked the first ladder in the dark."* · 02:30: *"Good."* · 03:30: *"Late. We will be in the Popcorn when the sun comes."* · 05:30: *"Then the sun will be on the towers. I will not like it."* (Nima trust −5) | `blink` with `teleport tent_c4_sit` + `pose sit snap` in `during`; Nima spawned in the same blink | C |
| `c4_last` | after alarm | Nima stands to go. He stops. His headlamp goes off. The Icefall groans in the dark. | **Nima:** *"Sangye."* (pause) *"Tomorrow, stay close to me."* | `hold "end"` (the last beat only) | F |
| `c4_page` | after last | `blink sleep`; the journal page writes itself from the flags (below). Title *"CHOMOLUNGMA · Chapter 4 · Base Camp"*. The chapter's last line on the page: *"At three we go into the Icefall."* | none | `fade 1` for the page (as Chapter 1); `end` | F |

### 3.1 The journal page (Day 4 / 49), built from flags

*"Base Camp. Pasang says my father ate two plates before the Icefall. I ate one."*
(+ one line per flag: `med_honest` *"I told the doctor about my head. Ten kilos tomorrow."* · `med_lied` *"I told the doctor I was fine."* ·
`hale_log` *"His last call came from the Balcony. He said he could see the way."* · `hale_light` *"The doctor says the gold light was my blood."* ·
`blessed_compass` *"The lama blessed his compass. Afterwards it pointed up the mountain. Nima told me to hide it."* · `blessed_mala` / `blessed_drawing` / `blessed_gear` ·
*"Marek wanted oxygen first. Nima wanted tents."* + `loads_marek` *"I said oxygen."* / `loads_nima` *"I said tents."* / `loads_half` *"I said half and half. Nobody was happy."* · `water_given` *"I gave my thermos to the boy from next door."* · `water_hale` *"I carried water for a stranger in the wind."* · `water_refused` *"I said no to the boy from next door."*)
*"The ice talked all night. At three we go into the Icefall."*

### 3.2 Flags read and written (coordination with Chapters 3 and 5)

| Direction | Flags | Notes |
|---|---|---|
| **Read from Ch1** [Built] | `promise` `bringhome` `silent` · `tea` `water2` `firstaid` `mala` · `elena_load` `elena_water` `elena_push` · `money` `forhim` `quiet` · `safe` `risky` · `read_photo` | `mala` enables the prayer beads at the altar; Elena echo in Scene A |
| **Read from Ch2/Ch3** [Proposed names, to agree with those designs] | `rumour_death` (a trekker's rumour of a death in the Icefall) · `ams` (if the illness model is not built yet) · any item of the father's that Ch3 gives (v1: the ice axe) → it is placed at the altar too | if absent, the beats use their defaults |
| **Written for Ch5 and later** | `med_honest` `med_lied` `load_light` · `blessed_compass` `blessed_mala` `blessed_drawing` `blessed_gear` · `loads_marek` `loads_nima` `loads_half` · `carry_rope` `carry_even` `carry_nima` · `knows_icefall` (visited the doctors) · `ladder_practice` · `water_given` `water_hale` `water_refused` · `alarm_0200` `alarm_0230` `alarm_0330` `alarm_0530` · `hale_log` `hale_light` | Ch5 decision 1 reads `alarm_*`; Ch5's carry reads `carry_*` and `load_light`; `ladder_practice` lets Nima shorten his first ladder lesson in Ch5 |
| **Trust and goodwill** [To build; flags in Phase 1] | Nima, Dr. Hale (`Chapter_Design_v1.md` §4.10), plus Marek's respect and the neighbours' goodwill | D1 ±10 Hale · D2 ±5 Marek / Nima · D5 ±10 goodwill · D6 05:30 −5 Nima |

### 3.3 The camera budget for the chapter

| Item | Count | Where |
|---|---|---|
| Nudges | **at most 2** (validator), 1 planned | the flag pole at the puja; a spare for the director |
| Reflexes (10°, the player finishes the turn) | 4 | pressure-cooker whistle, avalanche boom at 01:40, "Lha gyalo!", the barrel cracking |
| Cuts, all behind closed eyelids | 6 | into the medical tent, out of it, into the tent for night 1, the dawn wake, the Dr. Hale walk back, into the tent for night 2 |
| Postures | sit (kitchen, medical, puja, night tent), lie (night 1), kneel1 (altar ×2), crawl (out of the tent at dawn) | |
| Inspect | 3 | the oximeter, Dr. Hale's log, the compass |
| Body takeovers, view locks, cinematics, FOV beat | **0** | none needed |
| NPC spawns | only behind closed eyelids or black (validator) | the cast in the opening blink of `c4_arrive`; the lama, Dorje, the Icefall doctors and the neighbours in the night-1 or dawn blinks |

---

## 4. Places

### 4.1 The georeference (how a real place becomes a world position)

From `terrain-georeference` (memory) and the measured landmarks [Measured]:

| Quantity | Formula |
|---|---|
| World X, Y (cm) from heightmap pixel (col, row) | `x = −504,000 + col × 3,374.24` · `y = −504,000 + row × 3,374.24` (row 0 = north, so **north = −Y, east = +X**) |
| Elevation (m) from world Z (cm) | `elev ≈ (z + 502,836) / 100` (from `z = (png − 32768) × 951.4/128 + 129,770` and `png = 13.4538 × elev − 52,342`). Check: Base Camp landmark z 24,491 → 5,273 m ✓ |
| Latitude, longitude (approximate) | `lat ≈ 28.008 − (row − 504) × 33.742 / 110,850` · `lon ≈ 86.921 + (col − 504) × 33.742 / 98,290`. About ±1 km: our Base Camp lands roughly 1 km west-south-west of the commonly published Base Camp position [To verify] |
| Compass bearing from Base Camp | `atan2(Δx, −Δy)` in degrees (0 = north, 90 = east) |

Route data: `Tools/RoutePlanning/route.json`: leg 1 Gorak Shep → Base Camp, 3,015 m, 5,159 → 5,277 m; leg 2 Base Camp →
Icefall camp, 2,312 m, 5,277 → 5,704 m. The Base Camp landmark (`Landmark_2`, a RouteLandmark actor) is at
**(487,014, 1,289,409, 24,491)** = 5,273 m, pixel (293.7, 531.5), about 27.9996 N, 86.849 E.

### 4.2 The places of Chapter 4

Positions are computed from route.json and the landmark. The **world builder** (`Tools/Story/build_chapter4_world.py`,
[To build], a copy of the Chapter 1 builder) traces each onto the real ground and writes `Content/Story/Chapter4_places.ygs`.
Rule from 2026-11-02: never invent a position or a height; [To measure] items are measured by the builder.

| Place (story name) | What stands there | World x, y, z (cm) | Elev. | From camp centre | Status |
|---|---|---|---|---|---|
| `bc_centre` | the pole and the puja altar (`puja_altar`, `pole_top` = pole tip, about 6 m up) | 487,014 · 1,289,409 · 24,491 | 5,273 m | 0 | altar **[Built]** (place_camps.py `altar=True`, scaled ×2 by base_camp_ground.py); Meshy altar [To build] |
| `bc_gate` | where Chapter 3 ends and Chapter 4 starts | 485,625 · 1,293,430 · 24,718 (leg 1 at 2,970 m) | 5,276 m | 43 m, bearing 199° | must match Chapter 3's last place |
| The Kovač camp | sleeping domes in an arc 10-25 m uphill of the altar (`sangye_tent`, `nima_tent` side by side; `tent_c4`, `tent_c4_sit`, `tent_out_c4` inside/outside Sangye's), the dining dome and the kitchen tent 15-20 m downhill (`kitchen_door`), the medical dome at the arc's east end (`med_door`, `med_sit`, `med_hale`), the toilet tent at the back, `water_barrels` behind the kitchen, `plan_board` at the dining door facing east | builder: within 60 m of `bc_centre`, ≥ 6.5 m clear of the route (place_camps.py rule) | ≈ 5,270-5,280 m | ≤ 60 m | today 9 old Fab tents + mess **[Built]**; swap to the Meshy tents of Chapter 1 [To build] |
| `glacier_pond` | the kitchen's water source: a frozen melt pond | the lowest point within 100 m of the kitchen [To measure] | | ≤ 100 m | [To build] (reuse `stream_hole`) |
| `neighbour_camp` | the team next door: 5 domes, a dining dome, a satellite dish, lights at night; Dorje's frozen pump | 502,014 · 1,285,409 · 24,637 (leg 2 at 150 m, 40 m left) | 5,275 m | 155 m, bearing 75° | [To build] |
| `icefall_doctors` | the Icefall doctors' tent, spare ladders, their small altar; `practice_ladder` over a 1 m trough beside it [To measure: a natural dip] | 527,934 · 1,290,303 · 26,125 (leg 2 at 400 m, 30 m right) | 5,290 m | 409 m, bearing 91° | [To build]; ladder mesh **[Built]** (Icefall ladders) |
| `crampon_point` | where the ice begins; **Chapter 5's place**: seen from camp at night as the first headlamps | 628,829 · 1,283,223 · 34,944 (leg 2 at 1,480 m) | 5,378 m | 1.42 km, bearing 88° | Chapter 5 |
| Icefall (view) | the towers above camp; at the Icefall camp | 657,014 · 1,343,409 · 67,598 (end of leg 2) | 5,704 m | 1.78 km, bearing 108°, **13.6° above the eye** | **[Built]** (Icefall patch, ~70% of the reference look) |
| `summit` (compass target only) | | 1,236,165 · 1,417,075 · 381,261 (end of leg 7) | 8,841 m (label 8,849) | 7.6 km, bearing 100°, 25° up | [To verify]: in reality the summit **cannot be seen** from Base Camp (the West Shoulder hides it). The compass points there anyway; nothing in this chapter needs it to be visible |
| `helipad` (background) | a flat stone pad with a painted circle; a helicopter lands once (sound first) | 465,217 · 1,333,210 · 22,290 (leg 1 at 2,505 m, 40 m right) | 5,251 m | 489 m, bearing 207° | optional [To build] |

### 4.3 Light by time of day

| Scene | Clock | Light | Art direction |
|---|---|---|---|
| Arrival | 17:10 | camp in blue shade, tents glowing, the Icefall top pink (Chapter 3's last frame) | "evening: shadow climbs the valley" |
| Kitchen | 18:30 | warm kerosene light, steam, smoke under the roof | one warm accent against blue |
| Medical tent | 19:00 | a cool white lamp; her face half in shadow | |
| 01:40 | night | almost black; faint starlight through the fabric; no headlamp | darkness is the scene |
| Dawn | 06:30 | camp in deep blue shade; tiny warm apricot tips on Pumori and the Icefall | "dawn = warm far, blue near" |
| Puja | 08:30-10:40 | the sun line moves down the glacier and **reaches the altar as the pole goes up** (story clock glide; the builder measures the sun time at the altar from the terrain) [To verify: when the sun reaches real Base Camp in spring] | light as a guide (Alyx) |
| Plan board | 11:00 | deep blue sky, hard glare off the ice, every detail sharp; glacier glasses on | "midday" |
| Hungry Ghost wind | 15:30-18:30 | sky and snow close in value, pale and dusty, the sun a dull disc; small colours (red cord, orange tent, blue barrels) pop | "overcast", but dry |
| Wind ends | 18:30 | violet dusk, warm light only on the high ridges | target image 03 (violet dusk) |
| Night | 20:30 | tent glow, headlamps moving between tents, the Icefall a dark wall, stars | |

---

## 5. Cast and their states

| Who | Role in Ch4 | States used (story `npc_mode`) | How made | Status |
|---|---|---|---|---|
| **Sangye** (you, 22) | the newest Sherpa, measured, blessed, consulted | postures sit / lie / kneel1 / crawl / stand; wears the new khata and the red cord after the puja | Meshy `sangye` (first-person body) | **[Built]** |
| **Nima** (about 45) | guide and conscience; argues for tents; the last line | `lead` → `present` → `sit` (puja, mats) → `follow` (walk) → `present` (doorway) | Meshy `nima_ai` | **[Built]** |
| **Dr. Ingrid Hale** (Norwegian, about 52) | **first time in person.** Measures, warns, reads the forecast; maybe reads the father's last call | `idle` in her tent → `sit` → `goto` (water walk) → `present` at the kitchen door | Meshy character `hale_ai` | [To build] |
| **Marek Kovač** (Slovak leader, late 40s) | the employer: "oxygen first", "not our problem" | `idle` at the board, `sit` (chair) at the puja | Meshy `marek_ai` | **[Built]** |
| **Elena Ruiz** (Spanish first-timer, about 35) | the warm one; laughs with tsampa on her face; lower SpO₂ | `idle`, `sit` (chair), `present` (kitchen echo) | Meshy `elena_ai` | **[Built]** |
| **Ji-woo Han** (Korean client) | films everything, quietly at the puja | `idle`, `sit` | MetaHuman + garment (`jiwoo` in Cast.ygs) | **[Built]** |
| **Pasang** (head cook, about 50) | knew Mingma; feeds Sangye; hammers the barrels; the *sur* offering | `idle` (kitchen loop), `sit` | Meshy character `pasang_ai` | [To build] |
| **The lama** (about 60) and his helper | leads the puja; ties the cords | `sit` (chant loop), `goto` along the rows | Meshy character `lama_ai`; the helper reuses `porter_a` in a maroon tint (fallback) | [To build] |
| **Dorje** (18, the neighbours' kitchen boy) | asks for water | `goto` (runs in), `present` | reuse the `porter_a` MetaHuman with a new tint | **[Built]** (reuse) |
| **Icefall doctors** (2) | the ladders moved; their dead brother | `idle` (lashing loop) | optional Meshy `icefall_doctor`; fallback `porter_a` + helmet | [To build] |
| Kovač Sherpas (2-3, unnamed), neighbours (3-4) | the crowd at the puja and next door | `idle`, `sit` | reuse with tints; no faces closer than 8 m | reuse |

**Animations needed** [To build, phase 2 of the camera spec]: sit cross-legged (the father's Meshy sitting animation is
the first try), kneel, a chanting idle with hand drum, a cooking loop (stirring), hammering, lashing a ladder, a throw.
Phase 1 uses idles and the camera's postures, as Chapter 1 does.

**Performance limit (12 GB laptop):** at most **8 full characters** close to the player at the puja; the rest beyond 15 m
with grooms off and low LODs. All NPCs of the chapter load at the start (`preload cast`) and appear only behind closed
eyes.

---

## 6. Gameplay and physiology hooks

| Hook | In Chapter 4 | Numbers (targets, to calibrate) | Status |
|---|---|---|---|
| **SpO₂ at rest** | measured in person; the 19:00 check | `physio_model.py` at 5,274 m: acclimatised 79.8%, half 74.9%, just arrived 68.1%. Sangye (born at 4,410 m): about 80. Elena (two weeks on the trek): about 75. The game's current linear rule reads 92.5% here (known A2 finding) | game rule **[Built]**; model in A2 |
| **Pulse** | read with SpO₂ | about 85-100 at rest at this height [To verify] | [To build] (heart rate exists in body effects) |
| **AMS (Lake Louise 2018)** | the headache question; true state from the ascent of Chapters 2-3 | four symptoms: headache, stomach, tiredness, dizziness (Roach et al., High Alt Med Biol 2018) | illness model [To build] (`Chapter_Design_v1.md` §4.1); Phase 1: flag `ams` |
| **HACE screen** | the heel-to-toe tape | sway and input drift scale with illness + fatigue | [To build] (uses the rig's exhaustion layer) |
| **Periodic breathing** | 01:40 wake | Breath lease B 0.8 for 6 s; holding still settles it; SpO₂ dips about 6 points in sleep (§4.1 proposal) | Breath lease **[Built]**; sleep dip [To build] |
| **Acclimatisation** | two nights at 5,274 m | §4.3 proposal: +150-250 m per night toward (highest point in 48 h − 300 m); Chapter 5 starts adapted to about 5,000-5,100 m [placeholder] | [To build]; today resting in camp adapts 5 m/s (all done in 220 s) |
| **Fatigue** | night 1 cut by the gasp; night 2 shortened 40 min by `water_hale` | +4 per game hour awake; sleep at Base Camp ×1.0 | [To build] |
| **Cold** | nights about −12 °C [To verify: spring nights at Base Camp]; the tent and bag; the wind | Hungry Ghost wind: cold +1 step outside | body temperature **[Built]** |
| **Water and food** | dinner and breakfast in the kitchen (story sets satiety and hydration, as Chapter 1 does); the D5 thermos | `ygg.Survival.Set Hydration 60` at Ch5 start if `water_given` | Set command **[Built]** |
| **Loads and camp stocks** | the plan board (12 loads to 4 camps) and the pack split | weights [Measured, §4.4]: bottle 3.0 kg, ration 0.6, gas 0.45; new: 100 m of 8 mm rope ≈ 4.5 kg [estimate], tent 3.5 kg [estimate] | [To build] (§4.4); Phase 1: flags |
| **Ladders** | optional practice over a 1 m trough | the existing ladder walk (×0.45, sway 0.8° + 2.5° in wind) | **[Built]** |
| **Realm** | the Hungry Ghost wind, about 6 real minutes, held by the story | hydration drain ×2.5 outside shelter, cold +1, sanity −0.5/min; harmless to health directly (Act II: harmless or risky only, §4.7) | wheel and warning **[Built]**; this realm's look and effects [To build]; Phase 1: `ygg.Realm.Force` with the closest existing look (the director picks) |
| **Company and sanity** | crowd all day; alone at 01:40 | §4.1 proposal: Nima within 25 m +0.5/min | [To build] |
| **Trust** | Nima, Dr. Hale, Marek, the neighbours | section 3.2 | [To build]; flags in Phase 1 |
| **Clock** | day 3, 17:10 → day 4, 21:00, two sleeps | story `time` / `timeto` / `timehold` | **[Built]** |
| **Oxygen** | none used. Bottles seen in the medical tent and on the plan board | | |

---

## 7. Assets

### 7.1 Existing assets to reuse [Built]

| Need | Asset |
|---|---|
| Tents | `/Game/Story/AI/tent_dome`, `tent_mess` (also re-coloured as the medical dome), `tent_kitchen`, `tent_toilet` |
| Camp clutter | `duffel`, `barrel_blue` (water barrels; an ice material on top for the frozen ones), `porter_load`, `doko`, `dung_heap`, `daypack`, `water_bottle`, `glacier_glasses`, `headlamp`, `bedding` |
| Kitchen | `cook_set`, `stove_set`, `copper_drum`; Poly Haven `PH/` brass pots, enamel pot, thermoses, jerrycans, propane tank, crates, `Lantern_01` |
| Puja | `tarchen` (prayer pole), `khata_folded`, `khata_tied`, `butter_lamp`, `offering_bowl`, `choktse` (low table for offerings); the prayer-flag system (`M_PrayerFlag`, real printed lungta textures); the old Fab altar stones at the pole |
| Inspect items | the father's compass `SM_Compass2_Case`; a card for Dr. Hale's log (`inspect` with text) |
| Characters | `nima_ai`, `marek_ai`, `elena_ai`, `sangye`; MetaHumans `jiwoo`, `porter_a` (Dorje, the lama's helper, background Sherpas) |
| Ladders, ropes, oxygen bottles | the Icefall ladder mesh; the Fab rope coil and oxygen bottle used by place_camps.py |
| Sounds | see section 8 |

### 7.2 New assets (Meshy prompts in the house style of `Tools/Meshy/assets.py`)

Prices (memory `meshy-pipeline`): a prop ≈ **33 credits** (concept 3 + model 30); a rigged character ≈ **41** (concept 6 +
model 30 + rig 5). Credits left: **about 7** (handoff 2026-11-20); the director offered 1,000. **Buying credits is the
director's money: nothing here is sent until he says so.** Test three first, as the pipeline rule says.

**Must have (Phase 1): 3 characters + 9 props = 3 × 41 + 9 × 33 = 420 credits**

```python
    # ---------------------------------------------------------------- chapter 4: Base Camp
    dict(id="lhapso_altar", kind="prop", size=1.6, tris=20000,
         what="a square Sherpa puja altar of stacked flat grey granite stones at Everest Base Camp, about 1.6 metres "
              "tall and 1.5 metres wide, a small open fire niche on the front blackened with soot, a flat stone shelf "
              "on top for offerings, a socket for a tall wooden pole in the middle, standing on glacier rubble"),
    dict(id="juniper_bundle", kind="prop", size=0.5, tris=6000,
         what="a bundle of fresh dwarf juniper branches with dark green scale-like needles and small blue-grey "
              "berries, tied with a strip of cloth, a few twigs charred black at one end, lying on a flat stone"),
    dict(id="puja_offerings", kind="prop", size=0.8, tris=12000,
         what="a puja offering spread on a low wooden tray covered with a folded white cloth: a brass bowl heaped "
              "with roasted barley flour, a bowl of raw white rice, a few plain beer cans, an unlabelled bottle, "
              "packets of biscuits, apples and oranges"),
    dict(id="gear_pile", kind="prop", size=1.2, tris=15000,
         what="mountaineering gear leaning against a low stone wall: three ice axes with steel picks and worn shafts, "
              "two pairs of steel crampons with nylon straps, a climbing harness with carabiners, a coiled orange "
              "rope, slightly dusty"),
    dict(id="gamow_bag", kind="prop", size=2.1, tris=10000,
         what="a portable hyperbaric chamber for altitude sickness: a long red and grey coated nylon tube about 2.1 "
              "metres long with a clear plastic window at the head end, half inflated, lying on a foam mat, a foot "
              "pump with a hose beside it"),
    dict(id="med_station", kind="prop", size=1.2, tris=15000,
         what="an expedition doctor's station: a folding aluminium table with an open grey plastic hard case of "
              "medicine boxes, a blood pressure cuff, a finger pulse oximeter, a stethoscope, and a small orange "
              "oxygen cylinder with a regulator standing beside the table; no cross symbols"),
    dict(id="plan_whiteboard", kind="prop", size=1.6, tris=5000,
         what="a portable whiteboard on a folding aluminium easel standing on glacier rubble, the white surface blank "
              "and slightly smudged, a marker tray with three markers and a few round magnets"),
    dict(id="kitchen_counter", kind="prop", size=2.5, tris=20000,
         what="an expedition kitchen counter at Everest Base Camp: a long low dry-stone wall topped with flat stone "
              "slabs and a plywood board, two big dented pressure cookers, a kerosene burner under a large aluminium "
              "pot, stacked steel plates and cups, two sacks of rice"),
    dict(id="camp_table_chairs", kind="prop", size=1.8, tris=12000,
         what="a long aluminium folding camp table with four folding camp chairs in faded blue fabric, dusty, "
              "standing on rubble"),
    dict(id="hale_ai", kind="char", size=1.70, tris=50000,
         what="a Norwegian expedition doctor of about 52, fair weather-reddened skin, fine lines around the eyes, "
              "straight ash-blond hair with grey tied back under a dark blue knit beanie, wearing a well-used "
              "slate-blue hooded down jacket over a grey fleece, dark softshell trousers and insulated brown leather "
              "boots; real worn technical fabric, no logos"),
    dict(id="pasang_ai", kind="char", size=1.65, tris=50000,
         what="a Sherpa cook of about 50 from Nepal, a round friendly weather-darkened face, short black hair with "
              "some grey, wearing an old red down vest over a faded navy fleece with the sleeves pushed up, a stained "
              "cotton apron, dark grey track trousers, a black knit beanie and worn trekking shoes; no logos"),
    dict(id="lama_ai", kind="char", size=1.65, tris=50000,
         what="a Sherpa Buddhist lama of about 60 from the Khumbu in Nepal, a calm weathered face, close-cropped "
              "grey hair, wearing traditional maroon monastic robes with a saffron-yellow sleeveless inner shirt, a "
              "maroon shawl wrapped over the left shoulder, a thick maroon down jacket worn open over the robes, and "
              "old brown trekking boots under the robe"),
```

**Nice to have: 2 characters + 3 props = 2 × 41 + 3 × 33 = 181 credits**

```python
    dict(id="icefall_doctor", kind="char", size=1.68, tris=50000,
         what="a Sherpa Icefall doctor of about 35 from Nepal, a lean sunburnt face, wearing a scuffed orange climbing "
              "helmet with glacier glasses on it, a faded red down jacket, a climbing harness with carabiners and ice "
              "screws, dark trousers and yellow double mountaineering boots; real worn fabric, no logos"),
    dict(id="sherpa_climber_b", kind="char", size=1.66, tris=50000,
         what="a young Sherpa climbing guide of about 28 from Nepal, short black hair, wearing a faded blue hooded "
              "down jacket, black softshell trousers, a grey knit beanie and brown leather mountaineering boots; real "
              "worn fabric, no logos"),
    dict(id="torma_plate", kind="prop", size=0.3, tris=6000,
         what="three Tibetan Buddhist torma offering cakes made of barley flour and butter on a round brass plate, "
              "cone shapes about 25 cm tall, coloured red with small white butter disc decorations"),
    dict(id="solar_panel", kind="prop", size=1.6, tris=6000,
         what="two dusty solar panels on a simple wooden frame on glacier rubble, angled toward the sun, a car "
              "battery and black cables at the foot"),
    dict(id="satellite_dish", kind="prop", size=1.2, tris=6000,
         what="a small grey satellite dish on a tripod weighted with stones on glacier rubble, a cable running away, "
              "dusty"),
```

**Total:** 420 (must) + 181 (nice) = **601 credits**, plus about 15% for retries ≈ **690**. The `torma_plate` prompt needs a
[Cultural check] before it is sent (torma shapes and colours differ by ritual).

**Not Meshy (no credits):** the juniper smoke (*sang*) and the tsampa cloud as Niagara effects (smoke Niagara was already
on the list for the Chapter 1 altar); the red cord (a 3 mm spline mesh, as simple as a rope); the Hungry Ghost dust (the
existing spindrift system with a pale tint); the plan-board drawing and Dr. Hale's log page (textures); the "Clinic"
sign (a texture). **Tent interiors** for the medical dome and Sangye's tent are shared with Chapter 1's open item
(step 9 of `Chapter1_v2_Design.md` §9.2): build once, reuse.

---

## 8. Sound needs (list only: the director picks every sound by ear)

Existing candidates are named; nothing here is chosen. Game-wide cues must stay deep and slow, never light (memory
`feedback-audio-identity-fate`).

| Scene | Need | Existing candidate |
|---|---|---|
| Base Camp bed | voices in many languages, a generator hum, distant music, pots clinking, a helicopter far down the valley | — |
| Kitchen | a **pressure-cooker whistle**; a roaring kerosene burner; a radio playing Nepali songs (**licensing: original or cleared music only**) | `Story/Story_Stove_Loop` |
| Medical tent | a soft oximeter beep (not "UI cute"); a pen on paper; the lamp's hum | — |
| Night | ice creaking under the tent; wind on the fabric; zips; sleeping-bag rustle; the **periodic-breathing gasp**; heartbeat; the Icefall cracking; a distant avalanche | `Ice/Ice_Groan_01-04`, `Ice/Ice_Crack_01-08`, `Ice/Avalanche_01-05`, `Ice/Mountain_Boom_01-03`, `Story/Story_Heartbeat`, `Body/Breath_Phrase_01-06` (a gasp variant is missing) |
| Puja | the lama's recitation; a hand drum and a bell; cymbals; juniper crackle; rice falling on stone; the crowd's "ah" as the pole goes up; flags snapping; **"Lha gyalo!" ×3** with laughter; a flour puff. **[Cultural check + consent]:** a real chant must be a licensed or consented recording, never made-up syllables | `Story/Story_Flags_Loop` |
| Plan board | marker squeak, magnets clicking on the board, wind in the flag lines | — |
| Walk | aluminium ladder rungs clanking; crampons scraping on rungs; rope being lashed | `Gear/Rope_Tighten_*` |
| Hungry Ghost wind | a dry hissing wind; ice crystals on fabric; a thin high whistle; a hammer on ice; a frozen barrel cracking; the wheel landing cue by danger (risky) | `Weather/Wind_Gusty`, `Weather/Wind_Whistle`, `UI/UI_Chime_E` or `_H` (current mapping) |
| Night 2 | radio squelch (the neighbours' news); the headlamp click; the Icefall in the dark; the chapter-end cue | `Story/Story_Radio_Squelch`, the `UI_Chime_*` family |

---

## 9. Robot-player test plan

Tools: `Tools/Bot/run_bot.ps1` (the robot player), the flight recorder (`Saved/Monitor/…/report.md`), `story_shots.ps1`
(review shots). One heavy process at a time: the editor closed, as the bot script asks.

### 9.1 Set-up

1. Write `Tools/Story/chapter4_story.py` → `Content/Story/Chapter4.ygs` (validator prints `validate: OK`; the nudge cap
   for this chapter is 2, fovbeat 0).
2. Run the world builder `build_chapter4_world.py` once in the editor → `Chapter4_places.ygs`.
3. Start options: from Chapter 3's end save, or directly with `ygg.Expedition.Go 2` (the Base Camp camp jump) and
   `ygg.Story.Start Chapter4`. A `-Chapter chapter4` switch in run_bot.ps1 [To build] wraps this.

### 9.2 Runs

| Run | How | Checks |
|---|---|---|
| R1 default | `run_bot.ps1 -Chapter chapter4` (normal speed, `ygg.Story.Auto 1` takes the default choices) | every F beat fires in order (log `Story: beat`); chapter time **35-45 min**; ends with `alarm_0230` written |
| R2-R4 branch matrix | three runs forcing first / second / third options with `ygg.Story.Choose` | every flag in §3.2 is written by at least one run; every conditional line plays at least once (log `Story [VO]`); the journal page has the matching lines |
| R5 sick body | start with the `ams` flag (or the illness model) and low hydration | the headache choice appears; the heel-to-toe wobble shows (sway measured); `load_light` changes the pack split |
| R6 no realm | `ygg.Realm.Force none` | the chapter still ends; D5 is skipped cleanly; no beat waits forever |
| R7 fast | `-Fast` | finishes; no stuck beat; used for quick checks after edits |

### 9.3 Measured checks (pass / fail, before the director plays)

1. **Pacing:** no gap over 6 minutes between a decision, an event or a discovery (from the monitor's event log).
2. **Camera** (`Chapter1_Camera_Spec.md` §11): nudges ≤ 2; every cut behind closed eyelids; every NPC spawn behind lids or
   black; eye ≥ 120 cm whenever walking faster than 0.6 m/s; no freeze over 100 ms with the eyes open; every lease
   released (no `lease … expired` warnings except planned ones).
3. **Body:** SpO₂ at rest at Base Camp within ±3 of the target the director approves (section 6); the hydration drain
   outside shelter in the Hungry Ghost wind is 2.5× the drain in the kitchen tent (two 60 s samples); fatigue at 02:30 is
   higher on the `water_hale` branch than on the others.
4. **NPCs:** no sliding (`ygg.Feet.Slide`), feet on the ground (`ygg.GroundProbe`), nobody inside a tent or the altar,
   nobody standing on the route line.
5. **Performance** at the puja with every NPC present vs the empty camp: GPU cost ≤ +2.0 ms; VRAM peak < 10.5 GB; at least
   60 fps at 1080p High on the laptop (the A4 budget). If it fails: fewer close NPCs, grooms off, LODs.
6. **Hand-off:** the save at the end holds the alarm, loads and carry flags; Chapter 5 starts from it.
7. **Review shots** for the director (story_shots.ps1): the arrival frame, the kitchen, the medical tent, the pole going up
   with the sun line, the tsampa cloud, the plan board, the Hungry Ghost wind, Nima in the night doorway. **The director
   judges every look; the robot only measures.**

---

## 10. Risks and open questions

### 10.1 For the director

| # | Question | Default if no answer |
|---|---|---|
| Q1 | **Which chapter list is real?** `Chapter_Design_v1.md` (17 chapters, Ch4 = Base Camp) or Chomolungma (12 chapters, Ch4 = the Western Cwm, the puja in Ch2)? | build this as the Base Camp chapter; only the number changes |
| Q2 | Was Dr. Hale the doctor on the father's team last season, with his last call at 02:10 from the Balcony (canon)? | yes |
| Q3 | The father's name is **Mingma Sherpa** (it is on the stone in `build_chapter1_world.py`). Keep it? | keep |
| Q4 | The **Hungry Ghost wind** at Base Camp, forced on a first play (random on replays)? | yes |
| Q5 | Show that clients eat in the dining tent and the Sherpas in the kitchen tent? | yes, softly (Elena comes to the kitchen if she likes you) |
| Q6 | Small mechanical effects for blessed items (sanity), or story only? | story only until a Sherpa reader says effects are respectful |
| Q7 | **Meshy credits**: 420 (must) + 181 (nice), about 690 with retries. About 7 are left | wait |
| Q8 | The lama speaks Sherpa language with subtitles, or only chants? | one short line with subtitles |
| Q9 | Puja at dawn (`Chapter_Design_v1.md`) or mid-morning when the sun reaches camp (more realistic [To verify])? | mid-morning, 08:30 |
| Q10 | Alarm options: is 05:30 allowed (Chapter 5's "start with the light")? | allowed, Nima trust −5 |
| Q11 | Optional ladder practice at Base Camp: good warm-up, or does it take away Chapter 5's first-ladder moment? | keep optional; Chapter 5 shortens its lesson if done |

### 10.2 For a Sherpa cultural reader [Cultural check]

| # | What we show | What we believe | How sure |
|---|---|---|---|
| 1 | No Sherpa enters the Icefall before the expedition's puja | widely reported in climbing literature | fairly sure [To verify] |
| 2 | The altar (lhap-so) is built of stones by the expedition's Sherpas, with a pole for the flag lines | spelling and name vary | unsure of the name |
| 3 | The lama picks an auspicious day | common practice | fairly sure |
| 4 | Juniper burned as a smoke offering (*sang*) | common practice | sure |
| 5 | Axes, crampons, harnesses and boots placed at the altar to be blessed | widely reported | sure |
| 6 | Offerings: tsampa, rice, beer, whisky, biscuits, fruit, torma | the torma's shape and colour | torma unsure |
| 7 | Rice tossed during the chant; tsampa thrown at the end with "Lha gyalo!" three times; tsampa smeared on cheeks for long life | order and meaning | fairly sure, check the order |
| 8 | A red protection cord (sungdi) and a khata given by the lama | spelling | fairly sure |
| 9 | The goddess Miyolangsangma, one of the Five Sisters of Long Life, lives on Chomolungma; the puja asks her permission and forgiveness for crampons and axes | attributes vary by source | fairly sure |
| 10 | Sherpa Buddhism is mostly of the Nyingma school; the puja lama in monastic robes (or a lay lama?) | which one fits | unsure |
| 11 | Hungry ghosts (*preta*, *yi dwags*) suffer endless thirst; offerings of burnt tsampa (*sur*) and water feed them; Nima's grandmother's saying | Nima's line must not invent a folk term | unsure, check |
| 12 | "Om mani padme hum" murmured; walking clockwise around the altar | | sure |
| 13 | The lama's words to Sangye: *"For Mingma's son. Go slowly, come back."* | whether a lama would say this, and in which language | unsure |
| 14 | No real names: the lama, the dead Icefall doctor, the neighbour team and all companies are fictional; no Red Cross emblem | | rule |

**Reading list for this check** (to read, not to copy): Jamling Tenzing Norgay, *Touching My Father's Soul* (2001): a
Sherpa son climbs Everest after his father, with the puja and Miyolangsangma. Sherry B. Ortner, *Life and Death on Mt.
Everest* (1999): the anthropology of Sherpas and mountaineering.

### 10.3 Coordination with the other chapter designs (written at the same time)

1. Chapter 3 must end at `bc_gate` (leg 1 at 2,970 m) at about 17:00 on day 3, with Dr. Hale and Nima present, in black or
   with the NPCs already spawned.
2. Chapter 5 reads `alarm_*`, `carry_*`, `load_light`, `ladder_practice`, `water_given`, `water_hale`, `knows_icefall`.
3. `Chapter_Design_v1.md` puts Chapter 5's juniper fire at the Icefall entrance. In real practice the Sherpas burn juniper
   at their **own camp altar** before leaving in the morning [To verify]. Chapter 5 could open at our altar at 02:30
   instead; the hand-off does not change.
4. Cast roles: in Chomolungma, Marek, Elena and Ji-woo are Sangye's own team. Chapter 5's "other team" beats (Ji-woo
   overtaking, Marek irritated) should use the neighbouring team, or be rewritten for the own team.
5. If Chapter 2 introduces the realms (Chapter 1 v2 says it does), the Hungry Ghost wind here is the second realm, not the
   first. If not, `c4_wind_warn` needs one taught line (Nima explains the wheel).

### 10.4 Risks

| Risk | Likelihood | Effect | Mitigation |
|---|---|---|---|
| The chapter list conflict (Q1) | high | work on the wrong chapter number | the slot design works in both lists; only the number and day change |
| Cultural mistakes in the puja (the most visible scene) | medium | disrespect to a living religion | §10.2 check before release; fictional people only; real chants only with consent; no invented terms |
| A hub chapter with no walking feels slow | medium | boredom (the director's 2026-11-14 concern) | six decisions in 40 minutes; the gasp and the wind; the 6-minute rule measured |
| Crowd performance at the puja | high | low FPS, VRAM over 12 GB | ≤ 8 close characters, grooms off beyond 10 m, LODs; measured in R1 |
| Systems not built yet (illness, acclimatisation by nights, fatigue, camp stocks, trust, the plan screen, the realm look) | certain | parts are flags only | Phase 1 plays with choices, flags and `ygg.Survival.Set`; Phase 2 adds the systems; every beat has a fallback |
| Tent interiors missing (medical, Sangye's) | high | the inside scenes look empty | shared with Chapter 1's open item; build once |
| Meshy credits (about 7 left) | certain | no new characters | the director decides; fallbacks: tinted reuse of `porter_a` for the lama and the cook (looks weaker) |
| Too much canon in Dr. Hale's log | low | narrows later chapters | one line, director decision Q2, with a neutral fallback answer |
| The summit is not visible from real Base Camp | low | a wrong view if a beat needs it | nothing needs it; the compass only points |

---

## Appendix: source documents

| Document | Used for |
|---|---|
| `Docs/Development/Chapter_Design_v1.md` §1-5 (Chapter 4 "Base Camp", Chapters 3 and 5 hand-offs, systems §4) | the slot, the decisions, the systems, the calendar |
| `Docs/Development/Chapter1_v2_Design.md` | format, cast, the story so far, camera rules |
| `Docs/Development/Chomolungma_Chapter1.md` Part 1 | the 12-chapter list; the puja and Hungry Ghost sentence |
| `Docs/Development/Chapter1_Camera_Spec.md` §4, §6, §7 | postures, leases, nudges, reflexes, blinks, inspect, the data API |
| `Tools/Story/chapter1_story.py` | story machine keys, the flags of Chapter 1, the journal page format |
| `Tools/RoutePlanning/route.json`, `place_camps.py`, `base_camp_ground.py`, `Saved/Compare/ygg_info.json` | Base Camp landmark, the altar, leg 1 and leg 2 positions |
| `Tools/Meshy/assets.py` | the prompt house style |
| `Tools/Thesis/physio_model.py` | SpO₂ targets at 5,274 m; Grocott 2009 at 8,400 m |
| `Content/Story/` (AI, PH, Photos, MetaHumans, Cast.ygs), `Content/Audio/` | existing assets and sounds |
| Memory: chomolungma-direction, climb-design-decisions, story-machine-chapter1, terrain-georeference, art-direction, audio identity, meshy-pipeline, nine-realms-planets | direction, rules, formulas, prices |
