# Chomolungma · Chapter 6 "First Night High" · Design v1

Version 1.0 · 2026-11-20 · written by Claude (designer) for the director (the author). It turns the one-page sketch of
Chapter 6 in `Chapter_Design_v1.md` (§5, "Chapter 6 · First Night High") into a design that can be built. It uses the
format of `Chapter1_v2_Design.md` and the camera words of `Chapter1_Camera_Spec.md`.

**How to read this.** Section 1 is the chapter in one page. Section 2 is what it receives from Chapter 5 and gives to
Chapter 7. Section 3 is the chapter beat by beat. Sections 4-10 are how it is made. Section 11 is the robot test.
Section 12 is risks and questions.

**Labels used below** (the same as Chapter 1 v2, plus two checks)

| Label | Meaning |
|---|---|
| **[Built]** | exists in the project today |
| **[To build]** | planned, not started |
| **[Proposed]** | a new idea in this document; the director decides |
| **[VO]** | a spoken line; the director records the voices himself |
| **[To verify]** | a fact or position we believe but have not checked |
| **[Cultural check]** | Sherpa or Tibetan Buddhist detail to check with a Khumbu Sherpa reader before release |
| **[Medical check]** | altitude-medicine detail to check against a good source (for example the Wilderness Medical Society guidelines) |

**Words used often**

| Word | Plain meaning |
|---|---|
| Beat | one small story step: "when X happens, do Y" |
| Story machine | our code that plays the beats from a data file (`.ygs`) |
| Rig | the camera code: the only thing allowed to move the player's view |
| Lease | a hold on the camera or the body that a beat takes, with a time limit, and that is always given back |
| Nudge | a slow, gentle turn of the view that gives way to the mouse; the last way to guide the eye |
| Posture | a body state with its own eye height: stand, sit, lie, kneel, crawl |
| Rotation | going up the mountain to sleep high, then coming back down to recover |
| AMS | acute mountain sickness: headache, sick stomach, bad sleep after going up too fast |
| HACE | fluid on the brain at altitude: confusion and an unsteady walk; it kills if the person does not go down |
| Periodic breathing | at altitude, sleeping breath slows, stops for a few seconds, then comes back in gasps |
| Arm wrap | going down a fixed rope by wrapping it round one arm and leaning back |
| Rappel | sliding down a rope through a metal brake (a figure-eight) held in one hand |
| SpO₂ | blood oxygen saturation in percent, the number on a finger oximeter |

---

## 0. Which chapter this is

This document follows the **17-chapter list of `Chapter_Design_v1.md`**, as the task asks: Chapter 6 = "First Night
High", game day 5 at Camp 1 to day 6 at Base Camp. Every hand-off to Chapters 5 and 7 is kept exactly as v1 describes it.

It is written for the **Chomolungma story** (2026-11-14): Sangye's father died high on Everest last season; Nima
climbed with him and hides how he died; the weather realms are the Six Realms; the team of Marek, Elena and Ji-woo is
Sangye's own employer. Where v1 still says "the other team", this document reads "Marek's team".

**Note for the director:** the 12-chapter list in `Chomolungma_Chapter1.md` has no chapter of this name. Its content
falls between "3 · The Icefall" (load to Camp 1) and "4 · The Valley of Silence" (Camp 2 and down). Its "6 · The
Changed Ice" is v1's Chapter 8. See question D1 in §12.

---

## 1. One-page summary

### What the chapter is

Sangye has just climbed the Khumbu Icefall for the first time and dropped his first load at **Camp 1 (about 6,130 m)**.
Now he must **live through his first night above 6,000 m** and **get down safely**. By day the tent is an oven. A client
of Marek's team cannot walk a straight line, and Sangye must say what he sees to the man who pays him. On the evening
radio Sangye can tell Dr. Hale the truth about his numbers, or lie. In the dark Nima tells him one small thing about his
father. At three in the morning Sangye wakes up without air, alone, while everyone sleeps, and his father's compass
points up the valley instead of north. At 04:30 his boots are frozen. Then he goes down the Icefall on the ropes,
facing the drop he climbed in the dark, and this time he is the one who holds the ladder for someone else.

### The feeling

**Quiet awe, with fear in the body and a mystery in the dark.** The chapter is the first time the mountain feels
*inside* Sangye: the heat, the headache, the breath that stops in the night. Tension comes first (D2: tension over
poetry): every scene carries a decision about water, the body, the truth, or another man's life. The awe sits in two
places only: the Cwm opening towards the blue Lhotse Face at noon, and the compass at three in the morning. The sense of
fate comes from the forecast: a dark realm is coming in three days, and everybody hears it.

**Not alone.** Nima is always within reach. Marek's team shares the camp, other expeditions' tents glow at night, Dr.
Hale is on the radio, and Base Camp's cooks wait at the bottom. The one stretch of real solitude is chosen on purpose:
**awake alone at 03:00, gasping** (v1 §6).

### The shape

| Part | Minutes | Clock | Tension (1-5) | Company |
|---|---|---|---|---|
| The tent: water from snow | 0-3 | day 5, 10:00-11:30 | 2 | Nima |
| The oven | 3-4 | 11:30-12:00 | 3 | Nima, Ji-woo outside |
| A walk into the Cwm (optional) | 4-8 | 12:00-12:40 | 3 | Nima |
| Paul cannot walk a line | 8-11 | 15:30 | 4 | Marek's team |
| The headache | 11-12 | 17:30 | 3 | Nima |
| The radio call | 12-14 | 19:00 | 3 | Dr. Hale (voice), Nima |
| Dark: Nima about the father | 14-15 | 20:00 | 3 | Nima |
| **03:00: no air, alone** (the peak) | 15-17 | day 6, 03:00 | 4 | alone (Nima asleep) |
| 04:30: frozen boots, the cold stove | 17-20 | 04:30-05:00 | 3 | Nima |
| Down the Icefall on the ropes | 20-44 | 05:00-07:20 | 4 | Nima (and Paul, if he goes down) |
| **Thick air: Base Camp** (the end) | 44-45 | 07:30 | 2 | crowd |

### Length

**45 minutes** (v1: walk 25, other 20). The Cwm walk is optional, so a player who rests is about 41 minutes. The descent
is 4.75 km on the map (legs 3 and 2 down); 25 minutes needs an average of about 3.2 m/s with v1's faster descent on the
ropes (§7.7). The robot player times it before the director plays (§11).

### How it serves the thesis

This is the **acclimatisation night**, the heart of the physiology model (plan blocks A2 and B4). In one chapter the
player feels the whole loop: climb high, the body struggles at rest, sleep is broken by periodic breathing, SpO₂ dips in
the night, and the next morning the body is a little better adapted. The flight recorder logs SpO₂ every second through
the night, so the chapter produces a thesis chart by itself (§11). A 10-minute slice (beats C6-07 to C6-11) would make a
strong demo of the sleep and acclimatisation model on demo route 1, which already ends at Camp 1 (plan block A4).

---

## 2. Hand-offs (exactly as `Chapter_Design_v1.md`)

### 2.1 In from Chapter 5 · The Icefall

| Item | Value |
|---|---|
| Place | Camp 1, in Sangye and Nima's tent ("Drop the load; the tent", v1 Ch 5 minute 70) |
| Clock | game day 5, 10:00 |
| Camp 1 stock | 3 oxygen bottles [Measured: `place_camps.py` camp 4, `o2=3`] plus the load carried in Chapter 5 |
| Body (default if no save) | fatigue about 55 of 100; water about 45%; stamina low; adapted altitude about 5,300 m (the Base Camp nights); SpO₂ about 70 at rest (the model decides) |
| Flags read | `c5_collapse_behind` / `c5_collapse_ahead` (the serac collapse near the top of the Icefall), `c5_cave` (the ice-cave cache), `c5_ownline` (left Nima's line), Nima's trust. Names are proposals: align them with Chapter 5's story file |
| Taught before | clipping at anchors and ladders (Chapter 5); the heel-to-toe test, the night gasp of periodic breathing, and the 19:00 check (Chapter 4's design); the radio and SpO₂ (Chapter 1). From the puja (Chapter 4): the red protection cord (sungdi) at Sangye's and Nima's necks |

### 2.2 Out to Chapter 7 · Thick Air

| Item | Value |
|---|---|
| Place | Base Camp, the kitchen tent (breakfast) |
| Clock | game day 6, about 07:30 (early start) or about 11:30 (late start) |
| Forecast | two calm days, then **a dark realm on day 8** (v1: "a dark realm in three days"). Chapter 7's night storm at Base Camp is this realm |
| Dr. Hale | wants Sangye's numbers in person (she can find a radio lie there) |
| Flags set | see §3.2 |
| Body | adapted altitude raised by the night (§7.6); fatigue partly recovered; any frostbite risk on the toes |
| Camp 1 stock | gas used, bottles left (2 or 3) |
| Ends on the question | ***Three days to rest. A dark realm is coming.*** (v1, unchanged) |

### 2.3 Suggestions for the neighbours (not changes)

- **Chapter 7:** if Paul Becker went up (`becker_up` or `becker_silent`), a helicopter could pass over Base Camp on day 7
  carrying him down from Camp 2. If the player told the Icefall doctors about the collapse (`told_doctors`), it fits
  Chapter 7's decision 3 (help them rebuild).
- **Chapter 8:** v1's stretcher team in the Icefall could carry Paul if he went up and the body model made him worse.

---

## 3. The chapter, beat by beat

### 3.1 Beat list

Marks (v1 §1.5): **F** fixed (always happens), **S** systemic (the systems produce it), **C** chosen (the player starts
it). Minutes are targets; the clock is game time.

| Id | Min | Clock | Beat | Mark |
|---|---|---|---|---|
| C6-01 | 0 | 10:00 | The tent | F |
| C6-02 | 1 | 10:00-11:30 | Water first, then snow (the stove) | F |
| C6-03 | 3 | 11:30 | The oven | S |
| C6-04 | 4 | 12:00 | Climb high, sleep low (the Cwm walk) | C |
| C6-05 | 8 | 15:30 | Paul cannot walk a line | C |
| C6-06 | 11 | 17:30 | The drum (headache) | S |
| C6-07 | 12 | 19:00 | Numbers, please (radio and forecast) | F |
| C6-08 | 14 | 20:00 | Headlamps off (boots, Nima, the father) | F/C |
| C6-09 | 15 | 03:00 | No air (alone) | S |
| C6-10 | 16 | 03:10 | Day 5 / 49 (journal by headlamp) | F |
| C6-11 | 17 | 04:30 | Frozen boots, sleeping gas | F |
| C6-12 | 20 | 05:00 | Arm wrap | F |
| C6-13 | 24 | 05:20 | Ladders, looking down; now you hold | S |
| C6-14 | 27 | 05:40 | Serac zone 2 | S |
| C6-15 | 31 | 06:00 | Rappel | F |
| C6-16 | 34 | 06:20 | The Football Field from above, the big step | F |
| C6-17 | 37 | 06:40 | Down through the old ice (traffic, Icefall doctors, serac zone 1) | S/C |
| C6-18 | 44 | 07:30 | Thick air: Base Camp | F |

### 3.2 Choices and flags

| Beat | Choice (on screen) | Flags | Default on timeout | What it changes |
|---|---|---|---|---|
| C6-04 | Walk up the Cwm / Rest in the tent | `cwm_walk` / `cwm_rest` | rest | walk: better adaptation tonight, heat and water cost, crevasse risk off the rope |
| C6-05 | He goes down with us at dawn / Ask Dr. Hale tonight / You're the leader, Marek / Say nothing | `becker_down` / `becker_hale` / `becker_up` / `becker_silent` | say nothing | Paul's path; Marek's and Elena's trust; who walks down with you |
| C6-05 | (optional) [E] Ask him to walk a line | `becker_tested` | — | the player sees the sign with his own eyes |
| C6-06 | Take a painkiller / Tell Dr. Hale tonight / Drink and wait | `painkiller` / `headache_told` / `headache_wait` | drink and wait | painkiller hides the headache cue; the illness keeps growing unseen |
| C6-07 | Read the true numbers / Say "eighty-one, ninety-five" / Say "fine" and nothing else | `radio_truth` / `radio_lie` / `radio_vague` | read the truth | Dr. Hale's advice and trust; found out in person in Chapter 7 |
| C6-08 | Liners into the sleeping bag / Leave the boots in the vestibule | `liners_bag` / `liners_out` | vestibule | frozen boots at 04:30, cold toes on the descent |
| C6-08 | Ask about my father / Let him sleep | `asked_father` / `let_sleep` | let him sleep | Nima's small piece about the father (two levels by trust) |
| C6-09 | (optional) [E] Breathe from a bottle | `o2_c1` | — | Camp 1 stock −1 bottle (meant for the South Col); less adaptation tonight |
| C6-11 | Go now, on frozen ice / Wait for the sun | `start_early` / `start_late` | go now | serac clock on the descent; cold; Dr. Hale's order |
| C6-17 | Wait clipped / Unclip and pass outside | `pass_safe` / `pass_risky` | wait | 10 s unroped by a crevasse lip, or two minutes of cold |
| C6-17 | Tell them about the collapse / Say nothing | `told_doctors` / — | say nothing | Chapter 7 hook (only if Chapter 5 had the collapse) |

Seven real decisions with later consequences (pillar 2 asks for at least three). None is obviously right.

### 3.3 The beats in detail

Each beat says **When** (its trigger), **Does / Sees / Hears**, **Says** (lines, all **[VO]**, drafts for the director),
**Camera** (rig requests), and **Ends**.

---

#### C6-01 · The tent (F)

- **When:** chapter start (Chapter 5's last beat), or `ygg.Story.Start Chapter6` from a test. Under black, `cut` in the
  start beat.
- **Does:** nothing yet; looks around.
- **Sees:** orange-yellow light through the fabric; frost melting in drops on the inner wall; Nima kneeling in the
  vestibule with the hanging stove; the three oxygen bottles lying in the snow outside the door, orange caps.
- **Hears:** almost nothing. The Cwm has no wind at this hour. A far serac crack. A zip in another tent.
- **Says:** Nima: *"Boots off. Feet in the bag. Now we make water."* Then, nodding at the bottles: *"Those three are
  for the Col. Not for sleeping."*
- **Camera:** `blink wake` (2.8 s), `pose sit` (snap, inside the blink) facing the door, `hold true` (Move lease; the
  tent scenes keep you seated). Sleeping-bag prop shown.
- **Ends:** when Nima holds out the pot.

#### C6-02 · Water first, then snow (F)

- **When:** after C6-01.
- **Does:** [E] takes snow from the stuff sack at the door and feeds the pot, four or five times; drinks from the first
  thermos.
- **Sees:** the blue flame under the pot; snow slumping into grey slush; steam on the tent wall; the gas canister on the
  pack screen going from full to three-quarters.
- **Hears:** the stove's roar; snow hissing on hot metal; the thermos filling.
- **Says:** Nima: *"Water first, then snow. Or the pot burns."* Later: *"Four litres a day up here. More in the sun."*
- **Caption (once):** "[E] Add snow". The pack screen shows: one thermos = a quarter hour of gas.
- **Camera:** stays seated. `hands` shows the snow sack, then the thermos.
- **Clock:** glides 10:00 → 11:30 over about 90 s while two thermoses fill (`timeto`).
- **Ends:** two thermoses full. Water +2 thermoses; gas −1/4 cartridge (two eighths, v1 §4.4).

#### C6-03 · The oven (S)

- **When:** clock ≥ 11:30 (the sun clears the Nuptse ridge on the tent).
- **Does:** [E] on the back zip opens the second door (air flows through), or does nothing. May drink.
- **Sees:** the light on the fabric jumps from soft to hard; the whole tent glows; body temperature on the HUD climbs
  towards 38.5 °C (heat, v1 §4.1). Outside, Ji-woo filming the Cwm with his phone.
- **Hears:** your breath getting heavier; the fabric ticking in the heat; a drip somewhere.
- **Says:** Nima: *"Open both doors. Lie still. Drink."* Ji-woo, at the door: *"Sangye! Say something for the people at
  home!"* (no choice; the player can wave by looking at him; Ji-woo laughs and moves on).
- **Camera:** `exposure +0.3` for 6 s (the glare, Exposure lease), then steady. `breath` heavier while hot (the body
  layer does this by itself).
- **Ends:** 60 s, or when the door is open (heat falls faster).

#### C6-04 · Climb high, sleep low (C)

- **When:** clock ≥ 12:00 and C6-03 ended.
- **Says (before):** Nima: *"We can walk a little way up the Cwm. Climb high, sleep low. Or we rest. Your choice."*
- **Choice:** Walk up the Cwm / Rest in the tent.
- **If walk:** Sangye crawls out of the tent door (Sit → Crawl → stand, as in Chapter 1), puts on glasses, clips into the
  leg-4 rope. He walks **325 m up the Cwm to a bamboo wand with a red flag** (leg 4 at 15%, about 6,190 m, +60 m).
  - **Sees:** the Cwm opening east between the Nuptse wall on the right and the Everest West Shoulder on the left; at
    its end, 5 km away, the **Lhotse Face**, blue ice up to the sky; 320 m ahead, the 4.4 m ladder of Chapter 9 (seen,
    not crossed); smooth snow on both sides of the rope.
  - **Hears:** your own breath, loud; boots in soft snow; the silence of the Cwm.
  - **Says:** Nima: *"Glasses."* (if they are off in the glare). At the wand: *"Stay on the rope. The Cwm eats people
    who walk where it is smooth."* Then: *"Next time, Camp 2. Today we only look."*
  - **Body:** water loss ×2 (noon sun, v1 §4.1); stamina recovery ×0.5; body temperature up. Off the rope the eight
    hidden crevasses of leg 4 are deadly [Measured: they exist].
  - **Camera:** look trigger on the Lhotse Face at the wand: companion head turn (Nima looks at it) at T0+3 s, a lure
    (a far ice crack from the face) at 7 s. **No nudge planned**; one nudge is held in reserve here (§4).
  - **Ends:** back at the tent door; crawl in (the rig's Crawl and Sit).
- **If rest:** `blink sleep` → 15:30 inside the blink (`time`). Fatigue a little down; water down by the heat.

#### C6-05 · Paul cannot walk a line (C)

- **When:** clock ≥ 15:30 (a blink skips the afternoon after C6-04 if needed).
- **Hears first:** Elena's voice at the door (sound before sight): *"Sangye? Can you come? It's Paul. Something's
  wrong."*
- **Does:** crawls out; walks 15 m to the next tent.
- **Sees:** **Paul Becker** (a client of Marek's team, §6) sitting on a duffel by his tent, one boot half laced, his
  fingers fumbling the lace again and again. Marek arrives from the other side.
- **Says:**
  - Paul: *"I'm fine. Just slow. What... what day is it?"*
  - Marek: *"He's tired. Everybody is tired at six thousand. Tomorrow we go to Camp 2, as planned. He has paid for this
    three times."*
  - Elena: *"He asked me the day twice. I'm a doctor, Sangye, but down there. Up here you know more than me. What do you
    see?"* (if the director drops "Elena is a doctor": *"You grew up with this. What do you see?"*)
  - Nima, quietly behind Sangye: *"Sangye. Say what you see."*
- **Optional:** [E] "Ask him to walk a line". Sangye lays his ice axe in the snow; Paul tries to walk heel to toe along
  it (the test from Dr. Hale's tent in Chapter 4). He wobbles at the third or fourth step and steps off. How badly comes
  from his body model (§7.8); in the robot run he always fails at step 4.
- **Choice:** He goes down with us at dawn / Ask Dr. Hale tonight / You're the leader, Marek / Say nothing (16 s).
- **Replies:**
  - Down: Marek: *"You work for me, Sangye. Remember that."* A pause. *"...Fine. Dawa goes down with you."* Elena:
    *"Thank you."* Marek trust −, Elena trust +, Nima trust +.
  - Ask Dr. Hale: Marek: *"Fine. Let the doctor decide."* (resolved at C6-07)
  - Leader: Marek: *"Good man."* Elena looks away. Marek trust +, Elena trust −.
  - Say nothing: Marek: *"Camp 2 tomorrow, then."* Nima says nothing.
- **Camera:** `npc_mode present` for Elena at the door (she comes into your view; you are not turned to her). The line
  walk is staged in front of you. No nudge.
- **Ends:** Paul back in his tent; Sangye back in his own (blink to 17:30 inside the tent).

#### C6-06 · The drum (S)

- **When:** clock ≥ 17:30 and Sangye's AMS index (§7.8) is at least "mild". For the design, the first night guarantees a
  mild headache: real first nights at 6,000 m almost always bring one [Medical check].
- **Sees / hears:** the heartbeat in the ears; a soft pulse of darkness at the edge of the view with each beat (the
  existing body effects, not a camera move).
- **Caption:** "Your head is pounding."
- **Says:** Nima: *"Head?"*
- **Choice:** Take a painkiller / Tell Dr. Hale tonight / Drink and wait.
  - **Painkiller:** the headache cue goes away for 6 game hours. The AMS index keeps growing unseen. If the player also
    lies on the radio and starts late, mild HACE signs can show on the ladders (§7.8).
  - **Tell Dr. Hale:** the honest path; her advice comes at C6-07.
  - **Drink and wait:** the cue stays; water −1 thermos (drinks it), hydration +.
- **Camera:** none (seated, Move lease held).
- **Ends:** at once.

#### C6-07 · Numbers, please (F)

- **When:** clock 19:00 (the blink from C6-06 lands here if needed).
- **Hears:** radio squelch (`Story_Radio_Squelch`, exists), static.
- **Does:** clips the finger oximeter on ([E], `inspect`); its screen shows the live numbers, for example **69 · 112**.
- **Says:**
  - Dr. Hale (radio): *"Camp 1, Base Camp. Good evening. Numbers, please. Sangye first."*
  - **Choice (the report):** Read the true numbers ({spo2}, {pulse}) / Say "eighty-one, ninety-five" / Say "fine" and
    nothing else.
  - Dr. Hale, after the truth: *"{spo2}. That's normal for a first night up there. Drink a litre before you sleep. If
    your head gets worse, you wake Nima and you come down. In the dark if you must."*
  - If `headache_told`: *"And take half a Diamox from the kit tonight. Tell me how you are at seven."* [Medical check:
    acetazolamide for mild AMS; never show a dose on screen]
  - After the lie: *"Eighty-one? Lucky you. Keep drinking."*
  - After "fine": *"Fine is not a number, Sangye. ... All right. Seven tomorrow, numbers."* If the player heard the
    father's last call in Chapter 4 (Dr. Hale's log: "fine, can see the way", 54% at the Balcony; flag name to agree
    with Chapter 4), she adds after a pause: *"Your father said 'fine' too."*
  - If the player lied at the Base Camp check (`med_lied`, Chapter 4) and lies again now, nothing is said tonight; the
    two lies are counted together when she measures him in person (Chapter 7).
  - Nima reads his own: *"Seventy-six. Eighty-four."*
  - If `becker_hale`: Dr. Hale: *"And Paul. Marek, put him on."* Paul's voice, slow, the words joined. Dr. Hale: *"Paul
    goes down at first light. Not up. That is not a suggestion, Marek."* → sets `becker_down`, flag `becker_down_hale`.
  - **The forecast** (it sharpens Chapter 4's "something I don't like coming from the west"): *"Forecast. Two calm
    days. Then the thing from the west arrives, on the eighth. A deep low. Wind on the summit over a hundred kilometres
    an hour, eighty at Base Camp. I want everybody down by the seventh."*
  - After the static, Nima, quietly: *"The eighth. The old people say on such days the cold hells breathe out."*
    [Cultural check: a folk way of speaking we must not invent; which realm Chapter 7 uses, see D7]
- **Sees:** the forecast card on the HUD (the existing realm wheel symbol): day 6 calm, day 7 calm, day 8 a dark realm.
- **Camera:** `inspect` on the oximeter (Inspect lease, closed by the player); then seated.
- **Ends:** radio off. Dr. Hale's trust: + for truth, − for "fine"; the lie has no effect *now* (it is found later).

#### C6-08 · Headlamps off (F/C)

- **When:** after C6-07; clock jumps to 20:00 inside a short blink.
- **Does:** lies down (`pose lie`, played over 2.5 s, not snapped); chooses where the boots go.
- **Choice 1:** Liners into the sleeping bag / Leave the boots in the vestibule. (Real practice: climbers sleep with the
  inner boots in the bag so they are not frozen in the morning.)
- **Sees:** Nima's headlamp on the tent roof; he clicks it off. Darkness. Across the camp, other expeditions' tents glow
  orange and go dark one by one.
- **Choice 2 (in the dark):** Ask about my father / Let him sleep.
- **Says:**
  - Ask (always): Nima: *"Your father did not talk up here. In the village he never stopped. Up here, nothing. He said
    the mountain listens."*
  - Ask, and Nima's trust ≥ 60: *"Only once he talked all night. Our last night. To someone outside the tent."* A long
    pause. *"There was nobody outside the tent."* [Director decides: how much Nima gives here; this is v1's "first small
    piece about his father's silence"]
  - Let him sleep: Nima: *"Sleep, bhai."* [Cultural check: "bhai", younger brother in Nepali, between Sherpas]
- **Camera:** posture Lie; Move lease held. Then `blink sleep` (2.5 s). Clock → 03:00 inside the closed eyes.
- **Ends:** eyes closed.

#### C6-09 · No air (S) · the peak

- **When:** clock 03:00, eyes closed.
- **Hears:** your breathing slows, softer, softer, and **stops**. The heartbeat (`Story_Heartbeat`, exists) climbs in the
  silence for about 10 s. Wind touches the fabric.
- **Then:** a gasp. The eyes open (`blink open` 0.3 s, no wake flutter: this is a shock). The body jerks up to sitting
  (`pose sit` 1.6 s: the body explains the move).
- **Builds on Chapter 4** (its design: the 01:40 gasp at Base Camp; holding still for 6 s lets the breath settle; Nima
  says through the wall "Breathe slow. It passes."). Here, 850 m higher, the pause is longer and holding still must
  last **10 s**. Moving makes the gasps return. **Nobody says it passes**: Nima sleeps.
- **Sees:** darkness; starlight through the fabric; frost from your breath glittering on the inner wall; Nima a dark
  shape, asleep.
- **Hears:** your panting; Nima's slow snoring; the camp asleep. Then, far away, an avalanche roars down the Nuptse wall.
- **Does (optional):**
  - [Q] holds up the father's compass: the needle does not point north. It points up the Cwm (towards the summit, about
    east) and trembles. A faint warm glint lies on the brass, with no light to make it. (The myth: quiet, deniable.)
  - [E] on the oxygen bottles in the vestibule: "Breathe from a bottle". Nima said they are for the Col. If taken: the
    shaking stops within a minute; Camp 1 stock −1; tonight's adaptation gain ×0.8 (§7.6). [Proposed: cut if the
    director wants bottled oxygen first used high.]
- **Says:** nobody. This is the one solitude of the chapter.
- **Camera:** `breath` override (Breath lease, ≤10 s, chained up to 3 times) for the panting; `attention −3°` for 0.4 s
  at the gasp; `reflex 10°` towards the avalanche (counts as a body reaction, not a nudge).
- **Ends:** when the player switches on the headlamp (goes to C6-10) or lies down (C6-10 still runs: he cannot sleep).

#### C6-10 · Day 5 / 49 (F)

- **When:** after C6-09.
- **Does:** reads.
- **Sees:** his headlamp on (`light`); the journal on his knees; the page writes itself in Sangye's hand from what
  happened (`page`, head "Day {day} / 49").
- **Page lines (drafts; `if` = the flag):**
  1. "Camp 1, {alt} metres. The tent was an oven all day."
  2. `cwm_walk`: "Nima and I walked up the Cwm to a red wand. Lhotse filled the end of the valley."
  3. `cwm_rest`: "We stayed in the tent and made water. Nima does it like a prayer."
  4. `becker_down` (not `becker_down_hale`): "Paul could not walk a straight line. I said he must go down. Marek did not
     like it."
  5. `becker_down_hale`: "I let Dr. Hale decide about Paul. She sent him down."
  6. `becker_up`: "Marek says Paul is only tired. I agreed."
  7. `becker_silent`: "Paul asked what day it was. I said nothing."
  8. `painkiller`: "My head was a drum. I took a pill and it stopped. I told nobody."
  9. `headache_told`: "I told Dr. Hale about my head. Drink, she said, and come down if it gets worse."
  10. `radio_lie`: "On the radio I said eighty-one. The clip said {spo2}."
  11. `radio_truth`: "On the radio I read my numbers: {spo2}. Dr. Hale says that is normal up here."
  12. `asked_father`: "Nima says my father did not talk up here. He said the mountain listens."
  13. `asked_father` and the second level: "His last night he talked to someone outside the tent. There was nobody."
  14. always: "At three I woke without air. Nima slept. The compass pointed up the valley, not north."
  15. `o2_c1`: "I breathed from a bottle meant for the Col. Two left."
  16. always: "At half past four we go down."
- **Camera:** seated; Hands lease on the journal. `blink sleep` after the page. Clock → 04:30 inside the closed eyes.
- **Ends:** eyes closed.

#### C6-11 · Frozen boots, sleeping gas (F)

- **When:** clock 04:30.
- **Hears:** Nima's watch alarm, muffled under his bag; his zip.
- **Says:** Nima: *"Up. The ice is asleep. We go before it wakes."*
- **Does:**
  1. **Boots:** hold [F] to force the feet in. `liners_bag`: 1.5 s. `liners_out`: 4 s, with a creak of frozen plastic;
     the toes start cold (frostbite risk on the descent).
  2. **The stove does not light:** click, click, a sputter. Nima: *"Gas sleeps in the cold. Wake it in your jacket."*
     Hold [F] 5 s with the canister inside the jacket (`hands`: the canister against the chest). Then it lights. Tea:
     body temperature +0.5 °C, water +.
  3. **The choice:** Go now, on frozen ice / Wait for the sun.
     - Nima first: *"Now is better. The seracs sleep until the sun."* If Paul is going down: *"Paul is slow. If we wait,
       we cross the seracs in the heat."*
     - Go now: start 05:00; dark until about 05:45 in the Icefall [To verify on our sky]; cold fingers at every clip.
     - Wait: `blink sleep`, clock → 08:30; start 09:00; warm, light; the serac clock runs ×1 and ×1.7 from 11:00
       [Measured]; if Dr. Hale said "come down at first light" this disobeys her (her trust −).
  4. Outside: kneels to strap the crampons (`pose kneel1`, hold [F] 2 s each).
  5. Nima touches the red protection cord the lama tied at the puja (Chapter 4) and murmurs a mantra (*Om mani padme
     hum*) before he clips in. [Cultural check: whether a Sherpa would do this at the top of the Icefall]
- **Camera:** Sit → Crawl → stand at the door; Kneel1 for the crampons.
- **Ends:** standing at the rope; Nima clips in. If `becker_down`: Paul and Dawa come out of their tent, Paul short-roped
  to Dawa.

#### C6-12 · Arm wrap (F)

- **Where:** from Camp 1 down leg 3. First passes **yesterday's collapse** (if `c5_collapse_behind` or
  `c5_collapse_ahead`): fresh blue blocks across the route near leg 3 ≈ 0.8-0.9, yesterday's footprints going under them.
  Nima: *"Yesterday we were here. Look at it."* Then the first rope stretch steeper than 20° (the builder finds it on
  route.json; expected near ladder 5, leg 3 ≈ 0.60-0.65).
- **Does:** watches Nima go first (staging: he is in front of you, in view), then wraps the rope and goes down. Hold
  [Brake] to slow; let go to run. Too fast on ice = a slip: you swing on the rope and stop (`knock`), stamina −, a bruise.
- **Says:** Nima: *"Rope round your arm. Lean back. Let it run. Your feet only walk."*
- **Camera:** walking camera; the Knock takeover only on a slip (15° in 0.14 s, the existing cap).
- **Ends:** foot of the slope.

#### C6-13 · Ladders, looking down; now you hold (S)

- **Where:** ladder 6 (2.8 m, leg 3 at 76%) and ladder 5 (4.0 m, leg 3 at 58%) [Measured].
- **Sees:** going down, the crevasse is in front of your feet, not behind: blue walls into black.
- **Does:** crosses ladder 6 as in Chapter 5 (×0.45 speed, sway in wind). At **ladder 5 the roles turn**: Sangye crosses
  first, then holds the hand line tight for the one behind: hold [F] while they cross. This is the hand-hold input of
  Chapter 1 (Dolma's hand), now Sangye giving the hold.
- **Says:** Nima: *"Now you hold for me."* If Paul is there: Dawa: *"Hold it tight. He is heavy."*
- **Camera:** none. If [F] is let go, the line goes slack and the crosser stops and waits (never falls: a fair rule).
- **Ends:** everyone across.

#### C6-14 · Serac zone 2 (S)

- **Where:** leg 3 at about 47%, just below the checkpoint "Icefall - halfway" (leg 3 at 50%) [Measured].
- **Sees:** the red pickets either side; the leaning towers. At 05:40 the serac clock runs ×0.7; at 09:40, ×1.
- **Does:** crosses within the zone's 30 s warning (exists). If Paul is slow, Sangye waits on the safe side and watches
  him come through.
- **Says:** Nima to Paul: *"Paul. Don't stop here."* (Nima's line from Chapter 5, now for someone else.)
- **Ends:** the checkpoint saves.

#### C6-15 · Rappel (F)

- **Where:** the ice step on leg 3 at 0.15-0.11 (slope 35-41° [Measured, route.json]); then ladder 4 (3.2 m, leg 3 at
  22%).
- **Does:** Nima clips Sangye's figure-eight onto the rope. Sangye turns to face the slope and goes down backwards. Hold
  [Brake] to stop; ease it to slide. Too fast = slip (as C6-12).
- **Says:** Nima: *"Figure eight. Right hand is your brake. Never let go of the brake."*
- **Camera:** **new rig state "Rappel"** (§4.2): the body turns to face the slope over 0.6 s (the player sees the rope
  come into his hands, so the turn is explained); the player can look down over either shoulder.
- **Ends:** foot of the step; ladder 4 crossed.

#### C6-16 · The Football Field from above, the big step (F)

- **Where:** the Icefall camp (leg 2 end / leg 3 start, about 5,700 m), then the steep step below it (leg 2 at
  0.89-1.00, slopes 30-52° [Measured]).
- **Does:** a short stop to drink; then the second rappel **without Nima's words** (he only watches: the "conclude" step
  of the teaching ladder).
- **Sees:** the flat snow where they ate yesterday at sunrise; late start: water running on the seracs, drips.
- **Hears:** ice groans (`Ice_Groan_*`, exist), louder on a late start.
- **Ends:** foot of the step.

#### C6-17 · Down through the old ice (S/C)

- **Where:** ladder 3 (2.6 m, leg 2 at 84%), an arm-wrap stretch (leg 2 at 0.76-0.67, 25-30°), ladder 2 (3.6 m, 66%),
  **serac zone 1** (leg 2 at about 56%), ladder 1 (3.0 m, 45%) [all Measured].
- **Events:**
  - **Traffic:** a line of another expedition's climbers coming up the rope. Choice: Wait clipped / Unclip and pass
    outside (the Chapter 1 lesson of the mountain side, again: the outside is a crevasse lip; 10 s unroped).
  - **The Icefall doctors** (the Sherpas who keep the route, carrying a spare ladder) near ladder 1: *"Nima-dai! How is
    the route?"* Choice (only if Chapter 5 had the collapse): Tell them about the collapse / Say nothing.
  - **Serac zone 1** timed by the start: 06:40 (×0.7) on the early start; about 10:40 on the late start, close to the
    ×1.7 hour, later still with Paul.
- **Ends:** the glacier flattens below ladder 1.

#### C6-18 · Thick air: Base Camp (F) · the end

- **Where:** the last 1 km of flat glacier to Base Camp (5,277 m).
- **Feels:** the breath audibly eases (`breath` easing; SpO₂ climbs towards about 78-80, the model decides). After a
  night at 6,130 m, Base Camp air feels thick. This is the reward, felt in the body.
- **Hears (before sight):** the generator; a pressure cooker whistling in the kitchen tent; prayer flags
  (`Story_Flags_Loop`, exists); voices.
- **Says:** Pasang, the head cook (Chapter 4's design), at the kitchen tent: *"Sangye! Nima-dai! Chiya, chiya!"* (tea)
  [Cultural check: how a Sherpa cook greets a young climbing Sherpa]. Dr. Hale, at the kitchen door, in person:
  *"Welcome down. Eat first. Then my tent: I want your numbers in person."*
- **Does:** sits on the stone bench in the kitchen tent (`pose sit`), where the Sherpas eat, with sweet milk tea.
- **Ends:** `blink sleep` → black → title **CHOMOLUNGMA · Chapter 6 · First Night High** → caption ***"Three days to
  rest. A dark realm is coming."*** → end.

### 3.4 The teaching ladder

One new thing per scene, in four steps (introduce, develop, twist, conclude), taught by a person.

| Mechanic | Introduce | Develop | Twist | Conclude | Taught by |
|---|---|---|---|---|---|
| Stove and water | Nima lights it | feed snow, fill thermoses | 04:30: cold gas will not light | warm it in the jacket; hot tea | Nima |
| Heat | the sun on the tent | body temperature climbs | the noon Cwm doubles water loss | open doors, rest, drink | Nima |
| Radio check | 19:00, she asks numbers | the oximeter shows the truth | you can lie | her answer depends; the lie is remembered | Dr. Hale |
| Sleeping high | (Chapter 4: the gasp at Base Camp; hold still) | at 6,130 m the gasp is longer; hold still 10 s | alone, nobody says "it passes"; the bottle tempts | 04:30: a better number at rest | the body |
| Arm wrap and rappel | Nima goes first | brake and run | the steep step: too fast is a slip | the big step without words | Nima |
| Ladders down | (Chapter 5) | looking down | now you hold for another | everyone across | Nima |

### 3.5 Pacing check (rule: no stretch over 6 minutes without a decision, an event or a discovery)

Descent distances from the route (2D, map). Minutes at the v1 average of about 3.2 m/s on the ropes, 2.3 m/s on the flat.

| From | To | Distance | Minutes | OK? |
|---|---|---|---|---|
| Camp 1 | collapse debris / ladder 6 | 586 m | 3.1 | yes |
| ladder 6 | ladder 5 (hold) | 440 m | 2.3 | yes |
| ladder 5 | serac zone 2, checkpoint | 270 m | 1.4 | yes |
| checkpoint | ladder 4 (the wood fibre is passed at 1.64 km) | 680 m | 3.5 | yes |
| ladder 4 | rappel step | 170 m | 0.9 | yes |
| rappel step | Icefall camp, big step | 370 m | 1.9 | yes |
| big step | ladder 3 | 370 m | 1.9 | yes |
| ladder 3 | ladder 2, traffic | 420 m | 2.2 | yes |
| ladder 2 | serac zone 1, ladder 1, Icefall doctors | 490 m | 2.6 | yes |
| ladder 1 | Base Camp | **1,040 m** | **5.5-7.5** | **just**: filled by the Base Camp sounds and the breath easing; if the robot measures over 6 min, move the Icefall doctors here |

The tent half has an event every 1-3 minutes by design.

### 3.6 The questions it leaves

What did the father talk to on his last night? Why does the compass point up the valley? Will Paul be all right? Did
Dr. Hale believe the numbers? And the one the chapter ends on: a dark realm in three days.

---

## 4. Camera: what the chapter asks of the rig

The story never moves the view itself (camera spec R1). It asks the rig, with the vocabulary of the camera spec §7.

### 4.1 The chapter's camera budget

| Item | Count |
|---|---|
| Nudges | **0 planned**, 1 in reserve (the Lhotse Face at the Cwm wand, C6-04). The validator holds Chapter 6 to 1 |
| Reflexes (10°, the player finishes the turn) | 2: the avalanche at 03:00; a serac crack within 60 m in a zone (systemic) |
| Jumps in time, all behind closed eyes, no teleport | up to 7: the afternoon (→ 15:30), → 17:30, → 19:00, → 20:00, → 03:00, → 04:30, and 04:45 → 08:30 on a late start |
| Teleports | 1, the chapter start under black (`cut`) |
| Body takeovers | Knock on a rope slip (only if too fast) |
| Cinematic shots / view locks | 0 / 0 |

### 4.2 Postures and leases used

| Posture / lease | Where | Notes |
|---|---|---|
| Sit + Move hold | the tent: C6-01/02/03, C6-06/07, C6-09/10; the Base Camp kitchen tent | released before every walk |
| Crawl (with `exit`) | out of and into the tent door | the Chapter 1 tent exit, reused |
| Lie | C6-08 (played down over 2.5 s) | sleeping-bag prop |
| Kneel1 | crampons at 04:30; the canister (optional) | |
| Lids | sleep, wake, the 03:00 gasp (open 0.3 s, no flutter) | |
| Breath override | C6-09 panting (≤10 s, up to 3 in a row); C6-18 easing | |
| Attention | −3° for 0.4 s at the gasp | |
| Exposure | +0.3 EV for 6 s in the oven | |
| Inspect | the oximeter (C6-07); optional: the father's Icefall photo at 03:00 | |
| Hands | snow sack, thermos, canister, compass, journal | P1 hands from the camera spec |
| **Rappel (new rig state)** [To build] | C6-15, C6-16 | body faces up-slope, turned over 0.6 s; yaw ±110° around up-slope so the player can look down over a shoulder; eye 20 cm further back (leaning out); the watchdog returns to Walk when unclipped or at the foot of the rope. **For the rig owner, not the story** |

**Eye heights** follow the rig's posture table, scaled to Sangye's current height (1.9 m, eye 174 cm,
`ygg.Player.HeightCm`), not the 156 cm of the first spec.

---

## 5. Places

### 5.1 Georeference (from `terrain-georeference` and route.json)

The world is real Everest at 1:1. Pixel (col, row) of the 1009 px heightmap (33.742 m per pixel) to world centimetres:

```
x_cm = -504000 + col * 3374.24        (+X = east)
y_cm = -504000 + row * 3374.24        (row 0 = north, so +Y = south)
elevation_m = z_cm / 100 + 5028.4     (route.json z; same as png = (z - 129770) * 128 / 951.4 + 32768,
                                       elevation = (png + 52342) / 13.4538)
lat ≈ 28.008 - (row - 504) * 33.742 / 110950
lon ≈ 86.921 + (col - 504) * 33.742 / 98286
```

Check: the summit pixel (517, 568) gives 27.9885 N 86.9255 E; the real summit is 27.9881 N 86.9253 E (about 50 m off).
The lat/lon are for orientation only; the world positions are what the builder uses.

### 5.2 The places of Chapter 6 (route.json, 2026-11-20)

Positions on the planned route line (z not yet traced on today's ground; the builder traces every place, §8).

| Place | Route position | x, y (cm) | Elevation | Pixel | ≈ lat, lon | What stands there |
|---|---|---|---|---|---|---|
| **Camp 1** | leg 3 end / leg 4 start | 794,739, 1,465,059 | **6,130 m** | 384.9, 583.6 | 27.9838 N, 86.8801 E | 4 Fab tents in two terraces, 3 oxygen bottles, an axe [Built]; to add §5.3 |
| Cwm wand (turnaround) | leg 4 at 15% (325 m) | 824,114, 1,480,060 | 6,191 m | 393.6, 588.0 | 27.9825 N, 86.8831 E | a bamboo wand with a red flag on the rope [To build] |
| Cwm ladder (seen only) | leg 4 at 30% | 854,333, 1,489,544 | 6,221 m | 402.6, 590.8 | 27.9816 N, 86.8862 E | the 4.4 m ladder of Chapter 9 [Built] |
| Ladder 6 (2.8 m) | leg 3 at 76% | 773,894, 1,411,370 | 6,043 m | 378.7, 567.6 | 27.9886 N, 86.8780 E | [Built] |
| Ladder 5 (4.0 m), the hold | leg 3 at 58% | 754,739, 1,371,934 | 5,966 m | 373.0, 556.0 | 27.9922 N, 86.8760 E | [Built] |
| Checkpoint "Icefall - halfway" | leg 3 at 50% | 754,739, 1,352,398 | 5,908 m | 373.0, 550.2 | 27.9940 N, 86.8760 E | [Built] |
| Serac zone 2 | leg 3 at ≈47% | 751,667, 1,345,987 | 5,881 m | 372.1, 548.3 | 27.9945 N, 86.8757 E | red pickets [Built] |
| Wood fibre (passed) | leg 3 at 0.8 km | 723,048, 1,331,368 | 5,758 m | 363.6, 543.9 | — | [Built] |
| Ladder 4 (3.2 m) | leg 3 at 22% | 698,790, 1,323,059 | 5,702 m | 356.5, 541.5 | 27.9966 N, 86.8703 E | [Built] |
| Rappel step 1 | leg 3 at 0.15-0.11 | ≈ 676,739, 1,329,059 | 5,650-5,666 m | — | — | 35-41° ice step; rappel anchor [To build] |
| Icefall camp ("Football Field") | leg 2 end / leg 3 start | 657,014, 1,343,409 | 5,704 m | 344.1, 547.5 | 27.9948 N, 86.8661 E | 2 tents, 2 bottles [Built] |
| Rappel step 2 (the big step) | leg 2 at 0.89-1.00 | ≈ 659,014, 1,339,409 | 5,589-5,704 m | — | — | 30-52° (Chapter 5 may reshape it; v1's "too steep to stand" point) [To build: anchor] |
| Ladder 3 (2.6 m) | leg 2 at 84% | 657,745, 1,316,139 | 5,547 m | 344.3, 539.4 | 27.9972 N, 86.8662 E | [Built] |
| Ladder 2 (3.6 m) | leg 2 at 66% | 632,053, 1,286,448 | 5,394 m | 336.7, 530.6 | 27.9999 N, 86.8636 E | [Built] |
| Serac zone 1 | leg 2 at ≈56% | 613,155, 1,275,409 | 5,328 m | 331.1, 527.4 | 28.0009 N, 86.8616 E | [Built] |
| Ladder 1 (3.0 m), Icefall doctors | leg 2 at 45% | 587,728, 1,275,409 | 5,321 m | 323.5, 527.4 | 28.0009 N, 86.8590 E | 2 Icefall doctors with a spare ladder [To build] |
| **Base Camp** | leg 2 start | 487,014, 1,289,409 | **5,277 m** | 293.7, 531.5 | 27.9996 N, 86.8488 E | mess and kitchen, altar, flags [Built] |

**Far landmarks** (for framing and the compass; [To verify] on our terrain with a line-of-sight trace):

| Landmark | ≈ lat, lon (real) | ≈ x, y (cm) | From Camp 1 |
|---|---|---|---|
| Lhotse summit (8,516 m) | 27.9617 N, 86.9333 E | 1,317,000, 1,710,000 | 5.8 km, at the end of the Cwm (game yaw ≈ 25°, along leg 4) |
| Everest summit (8,849 m) | 27.9881 N, 86.9253 E | 1,240,500, 1,412,600 | 4.5 km, bearing about east (yaw ≈ −7°); probably hidden by the West Shoulder [To verify]. The compass points here |
| Pumori (7,161 m) | 28.0147 N, 86.8281 E | 283,500, 1,122,400 | about 6 km north-west, back across the Khumbu valley. **The tent door should frame it** (a far, recognisable shape, the art direction's "notch" idea) |

### 5.3 What to add at Camp 1 [To build]

- **Sangye and Nima's tent** at real size (about 2.2 × 1.4 m, 1.1 m tall), **with an interior** (inner tent, two mats,
  two bags, the stove hanging in the vestibule, boots, the snow sack). The door looks west to Pumori; the back door looks
  east up the Cwm. Placed on a flat dug platform with a low snow wall on the windward side.
- **Paul's tent** 15 m away, door towards Sangye's, with 5 m of flat packed snow between them for the line walk.
- **Marek's tent, Elena and Ji-woo's tent, a Sherpa team tent** near by.
- **8-12 tents of other expeditions** further off, instanced, lit orange at dusk (they make the night "not alone").
- **The cache:** gas canisters and food under a tarp held by snow blocks; the 3 bottles by Sangye's door.
- **Route wands** (bamboo, red flags) along leg 4 to the turnaround.
- **The collapse debris** near leg 3 ≈ 0.8-0.9, if Chapter 5 placed it (shared state, §2.1).

Camp 1's route point slopes 15-17° [Measured]: the builder must find or flatten a platform for the tents (the camp-pass
tools already do this for other camps).

### 5.4 Light by time of day

| Scene | Clock | Light | Art-direction note |
|---|---|---|---|
| Tent at 10:00 | morning | sun not yet on the tent; soft light through fabric | stillness |
| The oven | 11:30-15:00 | hard sun, the fabric glows; outside deep blue sky, white glare | "midday: startling detail" |
| The Cwm walk | noon | Lhotse Face blue ice under a dark blue sky; the Nuptse wall in sharp shadow lines | "detail follows attention" |
| Paul | 15:30 | low sun on the Cwm, long tent shadows | |
| Radio | 19:00 | dusk; violet sky; warm light only on the Lhotse summit | target image 03 (violet dusk) |
| 03:00 | night | starlight, no moon assumed; blue frost glints; one warm glint on the compass | the only myth moment |
| 04:30 | before dawn | headlamps; sky deep blue over Pumori | "dawn = warm far, blue near" |
| Descent, early | 05:00-07:30 | blue shade in the Icefall; apricot on Pumori and the high ridges | |
| Descent, late | 09:00-11:30 | full sun on the seracs; dripping ice; glare | |
| Base Camp | 07:30 | sun reaching the camp; kitchen steam | |

---

## 6. Cast and their states

| Who | Role in Chapter 6 | States used | How made |
|---|---|---|---|
| **Sangye** (22) | you | sit, lie, crawl, stand, kneel1, rappel; headache, cold toes, heat | Meshy character [Built]; **high-camp kit needed** (§9.2) |
| **Nima** (about 45) | teacher, then quiet; the stove; the father's piece | sit (stove), lie and sleep (snoring), lead, follow, wait at an anchor, hold the ladder; trust | `nima_ai` [Built]; high-camp kit needed |
| **Marek Kovač** | leader of Sangye's team; pushes the schedule | stand, talk; goes up to Camp 2 at 07:00 day 6 with the clients | `marek_ai` [Built]; high-camp kit needed |
| **Elena Ruiz** | worried for Paul; asks Sangye's opinion | present at the door, talk | `elena_ai` [Built]; high-camp kit needed |
| **Ji-woo Han** | films; a moment of light company | walk, film (phone up), idle | MetaHuman with rebuilt garments [Built]; high-camp kit needed |
| **Paul Becker** [Proposed] | a client of Marek's team, 57, German, third Everest attempt; early HACE | sit (fumbling), the wobbling line walk, slow walk short-roped | new (§9.2); fallback: the freed MetaHuman `MH_Marek_Bruce` with hat, buff and sleeping bag |
| **Dawa** [Proposed] | a Sherpa of Marek's team; takes Paul down if he goes | follow, short-rope | climber model, face covered (goggles, buff) |
| **Dr. Ingrid Hale** | Base Camp doctor; the radio call; in person at the end | voice; at the mess door at C6-18 | voice; figure at a distance |
| **Icefall doctors** (2) | keep the route; news | carry a ladder, one line | climber model, helmets, faces covered |
| **Other climbers** | traffic on the ropes; tents at night | route followers going up | climber model, suits, masks off but goggles on |
| **Pasang** (head cook) | Base Camp welcome | idle, one line | from Chapter 4's design (`pasang_ai`) |
| **The father** | present only as the compass, the journal, Nima's words | — | — |

**Paul's body model.** Paul runs the same physiology model as the player (the virtual climber, block B1), started with a
high AMS/HACE index. His fate after the chapter comes from the model, not a script. **[Director decides]** whether a
client can die in Act II (v1 question 4 about tone).

**Trust values touched:** Nima (C6-05, C6-11), Marek (C6-05), Elena (C6-05), Dr. Hale (C6-07, C6-11).

---

## 7. Gameplay and physiology hooks

Each hook says what exists today and what the chapter needs. Numbers marked "model" come from the physiology model
(`Docs/Thesis/Physiology_Model.md`, block A2), not from this document.

| # | Hook | Today | Chapter 6 needs |
|---|---|---|---|
| 7.1 | **Oxygen (SpO₂)** | physiology model: pressure → saturation, effort widens the gap, shelter +4 [Built] | live values in lines (`{spo2}`, `{pulse}`); a night dip during sleep (about 60-65 in the pauses, model); the oximeter `inspect` reads the live number |
| 7.2 | **Bottled oxygen** | a bottle acts as 2,500 m lower; auto-on below 58 [Built] | optional use at 03:00 from the Camp 1 stock (C6-09); stock −1 |
| 7.3 | **Heat** | not built | v1 §4.1: in the Cwm sun towards 38.5 °C, water ×2, stamina recovery ×0.5; open doors lowers it in the tent |
| 7.4 | **Cold** | body temperature drifts to a target by altitude and night [Built] | night −20 °C outside; the tent and bag keep you safe; frozen boots → cold toes (frostbite risk index, v1 §4.9) |
| 7.5 | **Water and the stove** | camps refill for free; the stove does nothing [Built] | v1 §4.4: one thermos = 15 game minutes and 1/8 of a cartridge; the cold canister will not light until warmed |
| 7.6 | **Acclimatisation** | rises 5 m per real second resting in a camp; sleep gives nothing [Built] | v1 §4.3: rises **only in sleep**, 150-250 m per night towards (highest point in 48 h − 300 m), **plus a bonus when you slept lower than you climbed that day** (the Cwm walk, about +50 m) [Proposed]; ×0.8 sleeping on a bottle. Shown as a better resting number at 04:30 |
| 7.7 | **Fatigue** | not built | v1 §4.1: +4 per game hour awake; sleep removes it, **×0.7 at Camp 1** [Proposed]; the 03:00 waking costs sleep quality |
| 7.8 | **AMS and HACE** | not built (SpO₂ thresholds only) | an AMS index from time spent above the adapted altitude; a guaranteed mild headache on this first night; painkiller hides the cue only; HACE signs (ladder sway +50%, a wobble in the walk) only if the index grows high (painkiller + lie + late start + the Cwm walk). Paul runs the same model |
| 7.9 | **Fixed ropes and clipping** | auto clip within 4 m [Built] | v1 §4.8 manual clipping at anchors (if built by then; the chapter works with auto clip) |
| 7.10 | **Descending** | as walking, no bonus | v1 §4.8: arm wrap below 35° and rappel on steep ice, ×2 speed with a brake input, too fast = a slip; the new rig state (§4.2) |
| 7.11 | **Ladders** | ×0.45, sway, a fall kills [Built] | the hold (C6-13): while [F] is held the crosser moves; let go and they wait |
| 7.12 | **Serac zones** | clock ×0.7 before 08:00, ×1.7 from 11:00 to 18:00; 30 s warning [Built] | nothing new; the start time (C6-11) and Paul's speed decide the exposure |
| 7.13 | **Weather and realms** | realm wheel and warning; realms random, 10-12 min [Built] | the 19:00 forecast card: day 8 a dark realm (v1 §4.7). **During the chapter only calm or harmless realms** (Act II rule); a fog realm on the Cwm walk is the worst case [Proposed] |
| 7.14 | **Sleep** | `Rest()` in a camp shelter: to 04:30 if afternoon or later [Built] | the story drives the jumps (`time` inside blinks); the night's log for the journal and the thesis |
| 7.15 | **Death and recovery** | death screen, return to the last camp or checkpoint [Built] | deaths possible: a ladder fall, a serac zone, a crevasse off the rope on the Cwm walk, a fall while unclipped passing traffic. Return: Camp 1 (before the halfway checkpoint) or the checkpoint |
| 7.16 | **Company and sanity** | sanity drains with altitude and night [Built] | v1 §4.1: Nima within 25 m restores; the radio call +3; the 03:00 solitude is the low point |

**Minimum version** (to play the chapter before every system exists):
1. **Story only:** postures, blinks, time jumps, lines, choices, the journal page; auto clip; today's ladders and serac
   zones; the descent walked at today's speed (it will run about 34 min instead of 25).
2. **Plus small code:** live numbers in lines, the stove as a timed task, a simple AMS index, the ladder hold.
3. **Full:** arm wrap and rappel (block B3), acclimatisation from nights and fatigue (B4, A2), Paul on the virtual
   climber (B1), trust.

---

## 8. Story machine: files and new keys

| File | What |
|---|---|
| `Tools/Story/chapter6_story.py` [To build] | writes `Content/Story/Chapter6.ygs`; same `beat()` helper and `validate()` as chapter 1; Chapter 6 nudge cap 1 |
| `Tools/Story/build_chapter6_world.py` [To build] | traces every place in §5.2-5.3 onto the ground; writes `Content/Story/Chapter6_places.ygs`; checks the key frames: the tent door frames Pumori; the Lhotse Face centred from the Cwm wand; Paul's tent in view from Sangye's door |
| `Content/Story/Cast.ygs` [Built] | add `becker` and `dawa` looks |

**New keys the chapter needs** [To build; names are proposals for the story-machine owner]

| Key | Kind | Use |
|---|---|---|
| `{spo2}`, `{pulse}`, `{alt}`, `{day}` in `say` / `choice` / `page` text | text tokens | the live numbers on the radio and in the journal |
| `task` | action | a timed interaction with a prompt and effects, e.g. `{"task": "melt", "prompt": "[E] Add snow", "presses": 4, "gives": {"water": 2, "gas": -0.25}}` |
| `stat` | action | set or add a survival value, e.g. `{"stat": "ams", "add": 20}`, `{"stat": "toes_cold", "add": 15}` |
| `stock` | action | change a camp stock, e.g. `{"stock": "camp1", "o2": -1}` |
| `when.stat` | condition | e.g. `{"stat": "ams", ">=": 20}` for C6-06 |
| `when.clock` | condition | game time reached, e.g. `{"clock": 1130}` |
| `trust` | action | `{"trust": "marek", "add": -10}` |
| `npc_mode "sleep"` | NPC state | lie, breathe, snore (Nima at night) |
| `npc_mode "shortrope"` | NPC state | follow 2 m behind another NPC (Paul behind Dawa) |
| `holdline` | action | the ladder hold: waits while [F] is held and moves the crosser |

**Sketch** (the night peak, in the chapter 1 style):

```python
beat("c6_noair", {"after": "c6_lamps_off", "delay": 0.5},
     {"blink": "closed", "t": 0, "during": [{"time": 300}]},          # 03:00 behind closed eyes
     {"breath": 0.1, "t": 6}, {"wait": 6},                              # the breath stops (Breath lease)
     {"sound": snd("Story/Story_Heartbeat"), "vol": 0.7, "id": "hb"}, {"wait": 4},
     {"blink": "open", "t": 0.3}, {"pose": "sit", "t": 1.6},           # the gasp: the body sits up
     {"attention": -3, "t": 0.4, "wait": False}, {"breath": 1.8, "t": 9},
     {"npc_mode": "nima", "mode": "sleep"},
     {"wait": 8}, {"sound": snd("Ice/Avalanche_03"), "at": "c6_nuptse_wall", "vol": 0.6},
     {"reflex": "c6_nuptse_wall", "deg": 10},
     {"compass": {"tremble": 1, "toward": "summit", "t": 20}})
```

---

## 9. Assets

### 9.1 What exists (checked in `Content/Story`, `Content/Audio`, route tools, 2026-11-20)

| Need | Existing asset | Note |
|---|---|---|
| Sleeping bags and mats | `/Game/Story/AI/bedding` | red bag on yellow mat |
| Headlamp | `/Game/Story/AI/headlamp` | |
| Water bottle, thermos | `/Game/Story/AI/water_bottle`; Poly Haven `modified_thermos`, `plastic_thermos` (`/Game/Story/PH`) | |
| Glacier glasses | `/Game/Story/AI/glacier_glasses` | |
| Duffels, daypack | `/Game/Story/AI/duffel`, `daypack` | Paul sits on a duffel |
| Dome tent | `/Game/Story/AI/tent_dome` | dusty trek tent: needs a snow-camp material (no dust) or a new model (§9.2) |
| Camp 1 tents, bottles, axe | Fab props by `place_camps.py` camp 4 | |
| Ladders, ropes, pickets, serac zones, checkpoint | route actors | |
| Team | `nima_ai`, `marek_ai`, `elena_ai`, `sangye`; MetaHumans Ji-woo, porters; freed `MH_Marek_Bruce`, `MH_Nima_Aoi` | all in trek clothes |
| Sounds | `Story_Stove_Loop`, `Story_Radio_Squelch`, `Story_Radio_Static_Loop`, `Story_Heartbeat`, `Story_Flags_Loop`; `Gear/Rope_Slide_*`, `Gear/Carabiner_*`; `Ice/Avalanche_*`, `Ice_Crack_*`, `Ice_Groan_*`, `Serac_Collapse_*`; `Body/Breath_Phrase_*`; `Weather/Wind_*` | the stove loop may be the kerosene stove: check by ear |
| Not suitable | `/Game/Story/AI/stove_set` | a kerosene stove: right for the trek, wrong at 6,000 m |

### 9.2 New assets as Meshy prompts (house style of `Tools/Meshy/assets.py`)

Each line is the `what=` text; `PROP_STYLE` or `CHAR_STYLE` wraps it. Prices (memory note `meshy-pipeline`): a prop is
about 33-36 credits (concept image 3-6 + image-to-3D 30); a character about 38-41 with the rig.

**Chapter 6's own props (must)**

```python
dict(id="hanging_stove", kind="prop", size=0.45, tris=8000,
     what="a compact high-altitude canister stove system hanging from three thin steel cables: a tall insulated aluminium pot with a lid on a burner, a squat grey gas canister screwed underneath, soot on the burner, frost on the canister, no logos"),
dict(id="gas_canister", kind="prop", size=0.15, tris=2000,
     what="a squat 450-gram isobutane-propane gas canister in plain grey-blue painted steel with no print, a small dent, frost on the lower half"),
dict(id="oximeter", kind="prop", size=0.07, tris=2500,
     what="a small fingertip pulse oximeter, a matte grey plastic clip with a dark blank display window, a short black lanyard, scuffed"),
dict(id="double_boots", kind="prop", size=0.40, tris=12000,
     what="a pair of high-altitude double mountaineering boots, stiff yellow-and-black insulated outer shells with an integrated gaiter zip, the grey foam inner boots pulled half out, frost on the shells, no logos"),
dict(id="radio_vhf", kind="prop", size=0.20, tris=3000,
     what="a rugged black handheld VHF radio with a stubby rubber antenna and a worn belt clip, scratched, tied to a short orange cord"),
dict(id="figure8", kind="prop", size=0.15, tris=3000,
     what="a scratched steel figure-eight rappel descender with a locking carabiner clipped through it, on a short length of blue fixed rope"),
```

**The new character (must)**

```python
dict(id="becker_ai", kind="char", size=1.80, tris=50000,
     what="a German man of about 57 on his third Everest expedition, pale skin with a sunburnt nose and lips cracked by the cold, a grey stubble beard, tired unfocused eyes, wearing a faded dark blue down jacket over a grey fleece, black insulated climbing trousers with a harness, a grey knit beanie, and yellow-and-black high-altitude double boots; real worn technical fabric, no logos"),
```

**Optional props**

```python
dict(id="snow_pot", kind="prop", size=0.25, tris=4000,
     what="a dented aluminium cooking pot filled to the brim with chunks of clean hard snow, its lid leaning against it"),
dict(id="journal_book", kind="prop", size=0.18, tris=3000,
     what="a small worn hardback notebook with a faded red cloth cover held shut by an elastic band, a stub of pencil under the band, pages wavy from damp"),
dict(id="med_pouch", kind="prop", size=0.20, tris=4000,
     what="a small red zipped first-aid pouch lying open, foil blister packs of tablets and a roll of white tape inside, no text and no logos"),
dict(id="tent_highcamp", kind="prop", size=2.2, tris=25000,
     what="a two-person four-season geodesic dome tent in faded yellow standing on snow, grey poles outside the fly, a vestibule with the door zipped shut, snow piled along the skirt, guy lines tied to buried snow stakes, clean fabric with no dust"),
```

**Shared with Chapter 5 and every chapter above Base Camp: the high-camp kit** (buy once, whoever builds first). The
team's Meshy characters wear trek clothes. Above Base Camp real climbers wear harnesses, helmets on the Icefall, double
boots and crampons. Five characters: `sangye_high`, `nima_high`, `marek_high`, `elena_high`, `jiwoo_high`. Prompts:
the existing `what` texts, with the boots changed to "yellow-and-black high-altitude double boots with steel crampons",
plus "a climbing harness with a figure-eight and two carabiners, a white helmet clipped to the pack strap".

**Download first, Meshy only as fallback** (memory rule: download quality assets):
- **Tent interior** (inner tent, mats, the stove hanging in the vestibule). Shared with Chapter 1 (its open item 9).
  Check Fab free and Megascans first. Meshy makes closed objects, so an interior is a poor fit.
- **Oxygen mask and regulator** (only if the 03:00 bottle stays). Check `/Game/Fab` first.

**Credit estimate**

| Group | Items | Credits |
|---|---|---|
| Must: props | 6 × ≈33 | ≈ 200 |
| Must: Paul | 1 × ≈41 | ≈ 41 |
| Retries (+20%) | | ≈ 50 |
| **Chapter 6 must, total** | | **≈ 290** |
| Optional props | 4 × ≈33 | ≈ 130 |
| High-camp kit (shared with Chapter 5) | 5 × ≈41 | ≈ 205 |
| **Everything** | | **≈ 625** |

On 2026-11-20 about 7 credits were left; the director offered about 1,000 more. Buying credits is the director's money:
show this list, test three models first, as before. Cheapest path: `becker_ai` → the freed MetaHuman; `tent_highcamp`
→ a snow material on the existing tent; `radio_vhf` and `figure8` → Fab, if suitable free models exist.

---

## 10. Sound needs (list only; the director picks every sound by ear)

The rule (memory `feedback-audio-identity-fate`): game-wide and story cues are deep, slow and fateful, never light or
"app-like". Diegetic sounds (a stove, a watch) stay real and small.

| Scene | Sound | Have? |
|---|---|---|
| Stove | canister stove roar; snow hissing on hot metal; simmer; a thermos filling; the cold canister failing (piezo click, sputter) | stove loop exists (check if it is the right stove) |
| Oven | fabric ticking in heat; drips; the silence of the Cwm; a far serac crack in the warm afternoon | crack exists |
| Cwm walk | boots in soft snow; heavy breath; far ice cracks from the Lhotse Face | partly |
| Paul | his laboured, slow speech (the director's voice work); boot lace fumbling | VO |
| Radio | squelch, static, Dr. Hale; another team's chatter in the background | squelch/static exist |
| The forecast card | one deep, dark cue for "a dark realm is coming" (from the shortlisted families: K brass, L choir, M bowed drone) | candidates exist |
| Night | Sangye's breath slowing, stopping, the gasp (the director records it, or `Breath_Phrase_*`); heartbeat; Nima snoring; frost crackle on the fabric; a gust; a distant avalanche | heartbeat, avalanche, wind exist |
| The compass at 03:00 | nothing, or a very faint low drone under the heartbeat | to choose |
| 04:30 | a muffled digital watch alarm; frozen boot shell creaking; crampon straps | new |
| Descent | rope through the arm and through a figure-eight (friction squeal); carabiners; crampons on hard ice; aluminium ladder creak under crampons; ice groans in the late start | rope slides, carabiners, groans exist |
| Traffic | other climbers' voices, a "namaste" passing | new / VO |
| Base Camp | generator hum; pressure cooker whistle; kitchen; flags; far helicopter | flags exist |
| Chapter end | the title cue (the same family as Chapter 1's end) | to choose |

---

## 11. Robot-player test plan

Tools [Built]: `Tools/Bot/run_bot.ps1`, `ygg.Story.Auto 1`, the flight recorder (`Saved/Monitor/`), camera incidents
(`Saved/Monitor/incidents/`), `story_shots.ps1`.

### 11.1 Runs

| Run | Choices | Purpose |
|---|---|---|
| R1 default | every default (rest, say nothing, drink and wait, truth, vestibule, let him sleep, go now, wait clipped) | finishes; time; clock reaches about 07:30 |
| R2 careful | walk the Cwm, Paul down, tell Dr. Hale, truth, liners in the bag, ask about the father, go now, wait | Paul's descent path; Paul slows serac exposure; Nima's second level at high trust |
| R3 risky | walk the Cwm, Marek decides, painkiller, lie, vestibule, take a bottle, wait for the sun, pass outside | the collision case: HACE signs on the ladders; late serac clock; Dr. Hale's order broken |
| R4 Dr. Hale | ask Dr. Hale; truth | the radio resolves Paul (`becker_down_hale`) |
| R5 no Cwm, no tests | rest, never press optional prompts | shortest path; still no 6-minute gap |
| R6 slip | rappel with no brake | the slip takeover fires, releases, and the walk returns; no death on a clipped rope |

The robot needs new skills: hold [Brake] on rope sections steeper than 35°, release on gentler ones; the ladder hold; the
stove and canister hold keys; the line-walk prompt.

### 11.2 What is measured (pass marks)

| Check | Pass |
|---|---|
| Finishes | R1-R6 reach the end title |
| Length | R1 45 min ±25% (34-56 min); descent part 25 min ±25% |
| Pacing | no stretch over 6 minutes without a beat, choice or event (session monitor) |
| Clock | R1 Base Camp at 07:30 ±30 game min; R3 at 11:30 ±45 |
| Serac exposure | seconds spent in serac zones while their clock is ×1.7: R1 = 0, R3 > 0 and logged |
| Physiology (thesis) | SpO₂ logged every second through the night; the lowest night value below the 19:00 value; adapted altitude higher at 04:30 than at 20:00; R3's gain smaller (bottle) |
| Heat | R2 body temperature peak on the Cwm walk higher than R1's in the tent |
| Water and gas | gas used matches thermoses made (1/8 each) |
| Paul | each choice gives the planned path; his model state logged |
| Camera (camera spec §11) | nudges ≤ 1; every time jump with eyes ≥ 99% closed; no walking with the eye below 120 cm; the Rappel state always returns to Walk; no freeze over 100 ms with the eyes open; no roll or spin incidents |
| Performance | night tent (one headlamp, shadowless) and Camp 1 with 6-8 NPCs: frame time logged; no hitch when NPCs appear (they spawn behind closed eyes) |
| Lines | every `[VO]` line logged as `Story [VO] who: text` for the director's recording list |

### 11.3 Review shots for the director (`story_shots.ps1`)

Tent and stove; the oven; the Cwm wand view; Paul on the duffel; the oximeter; the dark at 03:00 with the compass; the
journal page; frozen boots; the first rappel; holding the ladder; Base Camp arrival. The director judges the look
(memory rule: I verify by measurement, the director makes the visual call).

---

## 12. Risks and open questions

### 12.1 For the director

| # | Question | Default if no answer |
|---|---|---|
| D1 | **Numbering:** this follows v1's 17 chapters. In the 12-chapter Chomolungma list this night sits between chapters 3 and 4. Keep the night as its own chapter, or fold it into "The Valley of Silence"? | its own chapter, as v1 |
| D2 | **Paul Becker:** a new client with early HACE. Keep him? Can he die later, from the model? | keep; he can be evacuated, not die, in Act II |
| D3 | **Elena a doctor?** v1 says yes; Chapter 1 v2 makes her a first-timer of 35. Her line has both versions | doctor |
| D4 | **How much Nima says in the dark** (the father talked to "someone outside the tent" on the last night, only at trust ≥ 60) | as written |
| D5 | **The 03:00 bottle:** keep the temptation, or keep bottled oxygen for the death zone only? | keep, if the mask exists |
| D6 | **The compass at 03:00:** a faint glint, or no light at all (only the needle)? | faint glint |
| D7 | **Which realm is the "dark realm" on day 8?** Chapter 4's design already uses the Hungry Ghost wind at Base Camp on day 4, so Nima names the **Cold Hells** here (the 12-chapter list brings them with the second rotation, which follows Chapter 7). Chapter 7 must agree | Cold Hells |
| D8 | **Meshy credits:** ≈290 for the chapter's must-haves, ≈625 with the shared high-camp kit | wait for your OK |
| D9 | **Descent length:** keep 25 min (needs v1's ×2 rope speed), or let it run about 34 min at today's speed? | measure first |

### 12.2 Cultural checks (a Khumbu Sherpa reader, before release)

- How Nima would speak of a coming storm as a realm, and whether "the cold hells breathe out" (the cold hells of the
  hell realm) sounds right in his mouth, or is a belief we would be inventing.
- The red protection cord (sungdi) from the puja and touching it before a descent; murmuring *Om mani padme hum* at the
  top of the Icefall.
- Address words: "bhai" (Nima to Sangye), "dai" ("Nima-dai"), "chiya" (tea) from a Sherpa cook.
- Whether a Sherpa would say "the mountain listens", and how Sherpas speak of the goddess of the mountain
  (Miyolangsangma) at a high camp. Not used in a line yet; add only after the check.
- Nothing in this chapter shows a religious object on screen except the cord; keep it that way until checked.

### 12.3 Medical checks

- Periodic breathing: is a stop of about 10 s, then a gasp, a fair picture of a first night at 6,100 m?
- A mild AMS headache as near-certain on a first night at Camp 1.
- Painkillers for an AMS headache are normal care. The game's message is "the pill takes the pain, not the illness;
  keep watching". Make sure no line says painkillers are wrong.
- Acetazolamide (Diamox) for mild AMS and for periodic breathing: Dr. Hale names it, never a dose.
- The heel-to-toe (tandem) walk as the field sign of HACE; descent as the cure.
- Night SpO₂ dips: the model's numbers against published sleep studies at altitude (thesis block B2).

### 12.4 Facts to verify on our terrain

- Is the Everest summit visible from Camp 1? (Probably not: the West Shoulder.) The compass works either way.
- Pumori in view from the tent door; the Lhotse Face from the Cwm wand.
- Real Camp 1 moves every season; ours sits where the Cwm flattens at the top of the Icefall (6,130 m).
- When light reaches the Icefall at 05:00-06:00 in our sky (Ultra Dynamic Sky settings).

### 12.5 Risks

| Risk | Likelihood | Effect | Mitigation |
|---|---|---|---|
| 20 minutes inside a tent with no interior model | high | looks cheap at close range | the interior is the first asset to find (download first); the camera's soft look limits; most tent beats are lit by one lamp |
| Arm wrap, rappel and the Rappel rig state not built | high | the descent is plain walking | the minimum version plays without them; block B3 builds them |
| Descent runs long (about 34 min at today's speed) | medium | chapter 54 min (inside +25%) | measure with the robot; trim the traffic event first |
| Systems missing (stove, heat, AMS, fatigue, night acclimatisation, trust) | high | decisions have no teeth | the minimum version list (§7); each system lands in its plan block |
| Paul's storyline feels like a lecture | medium | tension turns to preaching | Paul is shown, not explained; the player's own eyes in the line walk; Marek is reasonable, not a villain |
| Cultural mistakes | medium | disrespect to a living faith | the checks in §12.2; no religious object or text without a reader |
| Medical mistakes | medium | false teaching | §12.3; Dr. Hale never gives doses |
| Camp 1 slope 15-17° under the tents | medium | tents tilt, NPCs float | a flat platform from the camp tools; trace every place |
| NPC count at Camp 1 (6-8) on a 12 GB laptop | medium | frame drops, VRAM | climber-model figures with covered faces for background; MetaHumans only close; NPCs spawn behind closed eyes |
| Credits run out | high | missing kit | the cheap path in §9.2; shared kit with Chapter 5 bought once |
| Chapters 5 and 7 change while written in parallel | medium | broken hand-offs | §2 copies v1 exactly; anything new is in §2.3 as a suggestion only |

---

## Appendix: source documents

| Document | Used for |
|---|---|
| `Docs/Development/Chapter_Design_v1.md` §2-8 | the chapter sketch, calendar, budget, systems, cast, tension curve, passing places again |
| `Docs/Development/Chapter1_v2_Design.md` | format, pillars, cast as it stands, teaching ladder style |
| `Docs/Development/Chapter1_Camera_Spec.md` §2, §4, §6, §7 | rules, postures, leases, story camera tools, data keys |
| `Docs/Development/Chomolungma_Chapter1.md` Part 1 | the 12-chapter list (numbering note, §0) |
| `Docs/Development/Chapter4_Design.md` (written in parallel, 2026-11-20) | continuity: the Hungry Ghost wind on day 4, the first night gasp, the sungdi cord, Pasang, Dr. Hale and the father's last call, the forecast "from the west" |
| `Tools/RoutePlanning/route.json`, `place_camps.py`, `place_slice_content.py` | positions, slopes, Camp 1 contents, checkpoints |
| `Tools/Story/chapter1_story.py` | the story-machine keys and the validator |
| `Tools/Meshy/assets.py` | prompt house style |
| `Source/YggdrasilsPeak/Survival/SurvivalConfig.h`, `ExpeditionComponent.h` (read only) | the physiology model's values, `Rest()` |
| Memory notes: chomolungma-direction, climb-design-decisions, story-machine-chapter1, terrain-georeference, art-direction, feedback-audio-identity-fate, meshy-pipeline, nine-realms-planets | direction, georeference, art, sound, assets |
