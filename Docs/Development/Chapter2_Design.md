# Chomolungma · Chapter 2 "The Hidden Mountain" · Design v1

Version 1.0 · 2026-11-20 · written by Claude (designer) for the director (the author). Everything here is
**[Proposed]** until the director says yes. Format and level of detail follow `Chapter1_v2_Design.md`.

**How to read this.** Section 0 says where the chapter sits and why it is shorter on the map than in
`Chapter_Design_v1.md`. Sections 1-3 are the chapter itself (summary, the emotional line, the beats). Sections 4-7 are
the world, the people, the systems and the build. Sections 8-11 are testing, assets, sound and voices. Section 12 is
risks and questions.

**Labels used below**

| Label | Meaning |
|---|---|
| **[Built]** | exists in the project today |
| **[To build]** | planned, not started |
| **[New key]** | a story-machine field that does not exist yet (small C++ change) |
| **[Measured]** | read from the elevation model (DEM), `route.json` or `Chapter1_places.ygs` (method in the appendix) |
| **[VO]** | a spoken line; the director records the voices himself |
| **[To verify]** | a fact we believe but have not checked against a good source |
| **[Cultural check]** | must be checked by a Sherpa / Tibetan Buddhist reader before release |
| **F / S / C** | beat type from `Chapter_Design_v1.md` §1.5: **F**ixed (always), **S**ystemic (the systems make it), **C**hosen (the player starts it) |

**Words used often**

| Word | Plain meaning |
|---|---|
| Beat | one story step: "when X happens, do Y" (one entry in the `.ygs` file the story machine plays) |
| Rig | the camera code; the only thing allowed to move the player's view |
| Lease | a time-limited hold the story takes on the body or camera (hold still, pace, hands, inspect...); it is always given back |
| Nudge | a slow, resistible turn of the view toward something; the last resort, at most 3 per chapter |
| Reflex | a small 10° head turn toward a sudden loud sound; the player finishes the turn |
| Posture | stand, lie, sit, crawl, kneel1 (one knee), kneel2 (both knees): each has its own eye height |
| HACE | high-altitude cerebral oedema: the brain swells; the person stumbles, gets confused, and can die in hours |
| AMS | acute mountain sickness: headache, nausea, bad sleep; can grow into HACE |
| Ablation valley | the long flat trough between a glacier's side moraine and the mountainside; Lobuche sits in one |

---

## 0. Where this chapter sits

### 0.1 Numbering (please read)

The orchestrator asked for **Chapter 2 as in `Chapter_Design_v1.md`**: "The Memorial Pass", Dingboche → Lobuche,
ending with a night at Lobuche. Chapter 3 ("Their Tent") then starts **at Lobuche at dawn on game day 3**.

Since v1 was written, **Chapter 1 v2 already walks the first half of that chapter**: Dingboche → the Thukla memorials,
the porters and the "mountain side" rule, the cloud, the father's stone, and a night camp just below the memorials. So
this Chapter 2 is the **second half of the old v1 Chapter 2**, plus its Lobuche night:

| | v1 Chapter 2 (2026-11-14) | This document |
|---|---|---|
| Map | Dingboche → Lobuche, 11.5 km leg | Thukla night camp → Lobuche, **1.9 km** walked + a 2 km optional hike |
| Memorials | the peak of the chapter | already done in Chapter 1; passed again in morning light |
| Sick trekker at Lobuche (HACE) | decision 2 | kept: the central decision |
| Push on to Gorak Shep | decision 1 | **dropped**: it would break the hand-off to Chapter 3. Replaced by "climb high, sleep low" |
| Yaks on a ledge | decision 3 | kept as a **test without words** of the rule Chapter 1 taught |
| Cloud on the pass | decision 4 | kept as the afternoon cloud on the hike |
| Parents / village stone | v1 story | replaced by the Chomolungma story (father Mingma, body still on the mountain) |

**Conflict to settle (Q1, §12):** `Chomolungma_Chapter1.md` Part 1 lists 12 chapters where Chapter 2 is "The Puja"
(Lobuche, Gorak Shep, Base Camp). This document follows the v1 numbering as instructed.

**Found at the end of this work:** the parallel `Chapter3_Design.md` (same day) chose the **12-chapter** numbering. It
designs "The Icefall" and expects to receive Sangye **at Base Camp, Day 5, 02:30**, after the puja and ladder training.
The two documents still fit one calendar (day 2 Lobuche, day 3 Lobuche → Gorak Shep → Base Camp, day 4 puja, day 5
Icefall). If the director keeps the 12-chapter list, this document becomes **the first part of "The Puja" (Day 2)**,
and a second part (Days 3-4: Lobuche → Gorak Shep → Base Camp, the puja at the lhapso, ladder training, Dr. Hale in
person) is still to be designed to reach `Chapter3_Design.md` §0.2. Nothing here blocks that: the Lobuche night, its
flags and the frozen-bottle setup carry into it unchanged.

### 0.2 Hand-off in and out

| | State |
|---|---|
| **In** (from Chapter 1) | Night of day 1, the team's tents just below the Thukla memorials. Ch1 flags: `promise / bringhome / silent`, `tea / water2 / firstaid / mala`, `elena_load / elena_water / elena_push`, `money / forhim / quiet`, `safe / risky`, `read_photo`. Nima's last line: *"Your father did not fall."* |
| **Out** (to Chapter 3, exactly as v1 says) | Night of day 2, **Lobuche**, sleep and the journal page *"Day 2 / 49"*. Chapter 3 opens **at Lobuche at dawn, game day 3, with frozen water bottles** (this chapter sets up whose bottle is frozen: §3, beat B20). Marek's plan from Ch1 still holds: *"Lobuche tomorrow, Base Camp the day after."* |
| Shared place | The **Lobuche anchor** in §4 must be used by Chapter 3 too. |

---

## 1. One-page summary

### What the chapter is

Day 2. Sangye wakes in the tent below his father's memorial stone with Nima's words still in his ears. Nima burns
juniper at the stone at dawn and will not talk about the father "in front of him". The team climbs out of the Thukla
hollow into the long flat valley beside the Khumbu glacier. All day **the mountain hides**: Everest is behind the
Nuptse wall, and the father's compass points into rock. At Lobuche, a cluster of stone lodges at **4,934 m**, a young
trekker who came up too fast falls ill with HACE in the evening, and Sangye must choose what his own medicine, his own
sleep and his duty to the team are worth. Late at night, on the dining-room bench where Sherpa staff sleep, the quiet
client Ji-woo confesses that he has a video from last season: Sangye's father, **sitting** in the snow at the Balcony,
without his mask.

### The feeling

**Quiet awe with dread underneath.** The morning is grief and ritual. The day is company, work and thin air. The
evening is a stranger's body failing in front of you. The night is one image you cannot unsee. The sense of fate: the
mountain shows itself only when it wants, and a man who sits down up there does not get up.

### The shape

| Part | Minutes | Tension (1-5) | Company |
|---|---|---|---|
| Dawn: the tent, the juniper smoke at the stone | 0-12 | 2 | alone, then Nima |
| Breaking camp, past the memorials | 12-17 | 2 | the team |
| The valley to Lobuche | 17-38 | 2-3 | the team; Nima ahead or Elena behind (your choice) |
| Lobuche at midday | 38-44 | 1 | crowd (lodge, trekkers, porters) |
| Afternoon: the hike up the moraine, or rest | 44-58 | 2-3 | Nima and Ji-woo, or the lodge |
| Dusk: snow, the offering, the radio | 58-64 | 2 | crowd |
| **The stranger (HACE): the peak** | 64-74 | 4-5 | Nima, Diki, the trekker; alone with him on one branch |
| **Night on the bench: the end** | 74-80 | 3 → 5 | Ji-woo, Nima in the dark |

### Length

**About 70 minutes** (65 on the shortest branch, about 82 if you walk the trekker down). The walk is short (1.9 km
[Measured]); the time comes from the lodge, the hike and the night, not from padding. The robot player times every
branch before the director plays (§11).

### How it serves the thesis

The chapter shows the physiology model **on other bodies first**: Elena's AMS (headache, SpO₂ in the 70s), a stranger's
HACE (stumbling, confusion), and the effect of "climb high, sleep low" on Sangye's own adaptation. The player learns the
signs he will later see in himself. The flight recorder logs every second as in Chapter 1.

---

## 2. Purpose in the arc and the emotional line

### 2.1 Purpose

| Job | How this chapter does it |
|---|---|
| Keep the mystery moving without answering it | Nima refuses to talk at the stone; a stranger who knew the father shows Nima is hiding something; the video gives one hard fact: the father **sat down**. How he died stays open (Nima tells half the truth only much later) |
| Teach the body through others | Elena's AMS, the stranger's HACE, the heel-to-toe test, water and cold at night |
| Make the player's kindness cost something | the medicine, the night walk, or a stranger left to chance; the result returns in Chapter 4 (Dr. Hale reports) |
| Grow the company | Ji-woo gets a face (the witness), Elena gets a bond, Diki the lodge keeper is the first warm stranger |
| Root the mythic layer in real practice | the morning **sang** (juniper smoke for the gods) and the evening **sur** (burnt flour for the hungry dead) [Cultural check]. The second plants the Hungry Ghost realm of the Six Realms without any fantasy on screen |
| Hand off cleanly | Lobuche, night, journal; the frozen-bottle setup for Chapter 3's first beat |

### 2.2 The emotional line

| Moment | Feeling | Rule it serves |
|---|---|---|
| Frost, an empty bag beside you, juniper smoke above | grief, stillness | quiet awe: "I could stand here" |
| Nima: *"Not here. Not in front of him."* | a secret, close by | tension > poetry: the quiet moment carries a question |
| The valley opens; the compass points into the Nuptse wall | awe, then unease | "what am I standing in front of?" |
| A stranger calls you "Mingma's son" and speaks to Nima in Sherpa | suspicion | the secret has other keepers |
| The cloud comes up the valley on the hike | fear vs. wanting the view | a real choice with weather as the dice |
| The stranger cannot walk a straight line | dread, responsibility | the body as the source of drama (Cairn) |
| The phone's light on Ji-woo's face | dread | peak-end: one designed end |
| *"People who fall do not sit down."* | fate | every night ends on a question |

**One myth moment at most, and here none on screen.** Chapter 1 had the gold plume. Chapter 2 stays fully credible;
the strangeness is only in the father's posture on a phone screen, which matches the prologue. (Credible first, mythic
later: `Chapter_Design_v1.md` pillar 5.)

### 2.3 The question it leaves

*If he did not fall, why was he sitting?* (or, if Sangye did not look: *What is in that video?*). Plus: will the
stranger live? Is the bottle frozen? 47 days remain.

---

## 3. The beats

Minutes are targets from the chapter start. Walking assumes 2.3 m/s (Chapter 1's measured speed) plus stops.
Trigger keys are the story machine's existing `when` keys (`start, after, delay, near + r (cm), look + cone/dwell, lure,
nudge, companion, fire, press + prompt, posture, flag, noflag, any`) unless marked **[New key]**.

### 3.1 Overview

| # | id | Min | Where | Trigger (when) | What happens, in one line | Type |
|---|---|---|---|---|---|---|
| B01 | `wake2` | 0 | night tent | `start` | eyes open lying in the bag at 06:10; frost; Nima's bag is empty; a man murmuring a mantra outside | F |
| B02 | `bottle_slush` | 1 | night tent | `after wake2`, `look bottle seen` or `press Interact` | your bottle is half ice: you left it outside the bag | C |
| B03 | `sang_seen` | 3 | tent door | `after wake2`, `near tent_out r 300` | outside: blue dawn, a thin line of juniper smoke at the father's stone, 150 m up | F |
| B04 | `sang_offer` | 5 | father's stone | `near nima r 300` (or `fire` after 90 s: Nima comes down) | Nima at the fire; hold F to add juniper; a three-way talk choice | C |
| B05 | `break_camp` | 8 | night camp | `after sang_offer` | porters pack, milk tea, Marek's brief, the morning radio check | F |
| B06 | `pass_stone` | 13 | memorials | `near memorial r 1200` | the khata you tied yesterday, frozen stiff in the first sun; keep the chortens on your right | S |
| B07 | `climb_out` | 15 | S0 → S1 | `near s1_climb r 2000` | 90 m up out of the hollow; the rest-step is yours now (no line unless you sprint dry) | S |
| B08 | `valley_open` | 18 | S1 | `near s1 r 600`, `look lobuche_far` | the valley opens; Lobuche far ahead; the compass needle points into the Nuptse wall | F |
| B09 | `nima_or_elena` | 20 | S2 | `near s2 r 800` | Nima walks ahead alone, Elena falls behind: **you choose by walking** | C |
| B10 | `porters_down` | 26 | narrow spot ≈ S3 | `near ev_narrow r 2500` | porters come down a narrow stretch; **no prompt**: the rule from Chapter 1, now alone | S |
| B11 | `pasang` | 31 | between S4 and S5 | `near ev_pasang r 1800` | a descending Sherpa knew your father; he talks to Nima in Sherpa; the Icefall rumour | F |
| B12 | `lobuche` | 37 | Lobuche | `near lobuche r 4000` | dogs, a lodge radio, tin roofs; Diki: *"Tashi delek!"* | F |
| B13 | `refill_lunch` | 39 | lodge | `near diki r 400` | boiled water, dal bhat, Marek checks Elena's AMS score | S |
| B14 | `afternoon` | 44 | lodge terrace | `after refill_lunch`, `delay 20` | Marek: who walks up the moraine? **hike or rest** | C |
| B14a | `hike` | 45-58 | Lobuche → knoll (+118 m) | `flag hike` | Ji-woo asks about your father; the cloud climbs the valley: go down, or wait for a gap | C/S |
| B14b | `rest` | 45-55 | dining room | `flag rest` | tea and cards with porters; Elena; the solo trekker arrives, too fast | C |
| B15 | `snow` | 56 | lodge | `after afternoon`, `delay 600` or hike return | light snow; Diki lights the dung stove | S |
| B16 | `sur` | 58 | lodge burner | `after snow`, `look sur_burner` or `near diki_burner r 500` | Diki burns tsampa smoke for the hungry dead; your father's two plates | C |
| B17 | `radio_dusk` | 61 | dining room | `after snow`, `near dining r 600` | Dr. Hale's evening numbers | F |
| B18 | `stranger` | 64 | dining room | `after radio_dusk`, `delay 40` | the trekker is ill: the heel-to-toe test; **the decision** | S/C |
| B19a | `walk_down` | 68-76 | Lobuche → S5 | `flag walkdown` | at night in the snow, you hold his arm and walk him down 400 m | C |
| B19b | `dex` | 68 | dining room | `flag gavedex` | the pills, tea, an hour later he can hold his cup | C |
| B19c | `diki_o2` | 68 | dining room | `flag leftdiki` | Diki's orange oxygen bottle; a helicopter at first light, maybe | C |
| B20 | `bench` | 74 | dining-room bench | after B19x | staff sleep on the benches; Nima: your bottle, inside the bag | F/C |
| B21 | `video` | 76 | bench | `after bench`, `delay 25` | Ji-woo's phone: the father at the Balcony. Show / delete / turn away | F/C |
| B22 | `end2` | 79 | bench | `after video` | lie down, snow on the tin roof, sleep, the journal page, the title | F |

Fixed-only beats: 8 of 27 rows (30%), under the one-third limit of v1 §1.5.

### 3.2 Beat by beat

Lines are drafts in plain English for the director to rewrite and record. Sangye speaks only when the player picks a
line, as in Chapter 1. "(in Sherpa)" lines are shown as *[in Sherpa]* with no translation.

#### B01 `wake2` · The tent, 06:10 (F)

- **Trigger:** `start`. Behind closed eyes: `time 610`, `teleport night_tent_lie` (cut), `pose lie snap`, `npc_remove`
  everyone, then `blink wake`.
- **Sees:** grey-blue light through orange fabric; frost on the inner wall; Nima's sleeping bag rolled and empty; your
  bottle on the floor by the door. Caption: *Day 2 / 49 · Thukla, 4,830 m*.
- **Hears:** the stove roaring outside; far flags; a man's low voice murmuring *Om mani padme hum* (the lure for B03).
- **Camera:** posture Lie (eye 32 cm) → first key press → Sit (78 cm); Move hold until sitting; then crawl out along
  the door line exactly as Chapter 1 (`pose crawl exit tent_out`). Lids lease for the opening blink only.
- **Ends:** the player stands outside (B03).

#### B02 `bottle_slush` · The bottle (C)

- **Trigger:** `after wake2`, `look bottle_ch2` with `seen`, or `press Interact` + prompt *[E] Pick up*.
- **Does:** inspect the bottle; shake it; ice knocks inside. Caption: *Half ice. You left it outside your bag.*
  Hydration +5 (you drink the slush).
- **Why:** the first step of the frozen-bottle ladder (introduce here, develop at night in B20, pay off in Chapter 3).
- **Camera:** Inspect lease (closes on [E] or after 20 s).

#### B03 `sang_seen` · Juniper smoke at dawn (F)

- **Trigger:** `after wake2`, `near tent_out r 300`; `look sang_fire cone 14 dwell 0.35`, `lure` = the murmured mantra
  at `sang_fire`, `lure_at 7`, `nudge true` (**nudge 1 of 3**), `fire 15`.
- **Sees:** the camp still in blue shadow; above, on the memorial ridge, one straight line of blue-white smoke in still
  air; Nima's dark shape at the father's stone. Far west, the tops of **Taboche and Cholatse** catch the first apricot
  light [Measured: both visible from the memorials]. "Dawn = warm far, blue near" (art direction).
- **Ends:** the player walks up (C) or does nothing (after 90 s Nima comes down, and B04 plays at the camp edge
  instead).

#### B04 `sang_offer` · At the stone (C)

- **Trigger:** `after sang_seen`, `near nima r 300`, or `delay 90` (fallback at camp).
- **Does:** `holdkey F 2.0` with prompt *Hold [F] to add juniper*; the hands appear with green sprigs; the smoke
  thickens and smells sweet (sound: crackle). Your khata from yesterday hangs frozen stiff on the stone.
- **Camera:** optional `pose kneel1 t 0.9 face sang_fire auto` (eye 108 cm), stands on move. Hands lease (≤ 60 s).
- **Lines [VO]:**
  - **Nima:** *"Every morning he did this. A little smoke for her, so she lets us pass."*
  - **Choice** (timeout 16 s, default "Say nothing"): *"What did you mean last night?"* / *"Teach me the prayer."* /
    *Say nothing.* → flags `ask_nima` / `learn_mani` / `quiet2`.
  - ask → **Sangye:** *"What did you mean last night?"* · **Nima** (looks at the stone, a long pause): *"Not here. Not
    in front of him."* ... *"Walk with me today."*
  - learn → **Sangye:** *"Teach me the prayer."* · **Nima:** *"Om mani padme hum. Slowly. With me."* (both say it,
    Sangye's voice under his) · **Nima:** *"He said it fast. Like a man running for a bus."*
  - silence → **Nima:** *"Good. She hears enough talk."*
- **Cultural notes:** sang is offered to the gods and local protectors, not to the dead; so "for her" (the mountain
  goddess). [Cultural check: whether a Sherpa would burn sang at a memorial stone, or only at a lhapso altar or a
  household burner; the words "she lets us pass".]

#### B05 `break_camp` · Tea and the brief (F)

- **Trigger:** `after sang_offer` (or its fallback).
- **Sees/hears:** porters roping duffels to their head straps; the cook pouring milk tea from a kettle; Ji-woo filming
  the camp; Marek with a map.
- **Lines [VO]:**
  - **Marek:** *"Short day. Lobuche by lunch, then a little walk up and down again. Climb high, sleep low. Elena, the
    head?"*
  - **Elena** (`elena_push`): *"Like someone is in there with a hammer. I was sick in the night. Sorry."*
  - **Elena** (otherwise): *"Better. Still a small drum."* (to Sangye) *"Thanks for yesterday."*
  - **Dr. Hale (radio):** *"Morning, Thukla. Numbers, please... Sangye, eighty-five. Good."*
    + (`elena_push`) *"Elena, seventy-four. Take the Diamox now, and walk behind Nima, slowly."*
    + (otherwise) *"Elena, eighty. Drink."*
  - **Ji-woo** (lens on Sangye): *"Say something for the camera?"* · **Nima** (pushes the lens down, gently):
    *"Later."*
- **Note:** Dr. Hale's numbers should come from the live model when possible: **[New key]** a `{spo2:player}` token in
  `say` text. Until then, fixed numbers by flags (as in Chapter 1).

#### B06 `pass_stone` · The memorials in morning light (S)

- **Trigger:** `near memorial r 1200`.
- **Sees:** the field of chortens and flags in low gold light; the khata on the father's stone, white and frozen.
- **Systemic rule:** Buddhists pass chortens and mani stones keeping them on the **right** (clockwise)
  [Cultural check]. If the player passes a chorten on its wrong side, **Nima:** *"Other side. Keep them on your
  right."* (once). Needs **[New key]** `side`: which side of a tagged object the player passed.
- **If the player stops at the stone for more than 20 s, Nima:** *"He is not in the stone. Come."*
- **Compass:** if raised, the needle trembles toward the summit, which is **hidden behind Nuptse by over 400 m**
  [Measured, §4.3]: it points into dark rock.

#### B07 `climb_out` · Out of the hollow (S)

- **Where:** S0 → S1: the memorials lie in a hollow; the valley floor is about **90 m higher, 230 m away**
  [Measured, DEM]. The path zigzags (about 400 m walked).
- **Does:** breathing grows loud; the rest-step from Chapter 1 now works without a word (the "develop" step).
- **Only if** stamina hits empty: **Nima:** *"Step, breathe, step. You know this."*

#### B08 `valley_open` · The hidden mountain (F)

- **Trigger:** `near s1 r 600`, `look lobuche_far cone 20` or `fire 10` (no nudge).
- **Sees:** a step opens a valley (art direction): a long, flat, grey-brown trough; on the right, the steep grey wall of
  the glacier's side moraine, hiding the glacier; on the left, the slopes of Lobuche peak; far ahead, tin roofs. Ahead
  right, the dark Nuptse wall, backlit by the morning sun with a bright rim.
- **Lines [VO]:** **Nima:** *"Lobuche."* · (if the compass is raised) **Nima:** *"She is behind Nuptse. Up here she shows
  herself only when she wants."*
- **Hears:** behind the moraine wall, the glacier groans now and then (sound before sight: you never see it today).

#### B09 `nima_or_elena` · Ahead or behind (C, chosen by walking)

- **Trigger:** `near s2 r 800`. Staging only, no prompt: Nima lengthens his stride and walks ahead alone (a gap opens:
  an invitation); Elena slows with Marek behind (`elena_push`: she stops, hands on knees). One caption only if the
  player stands still for 10 s: *Nima is far ahead. Elena is falling behind.*
- **The choice is made by where you walk** (two beats with `any`): reach Nima within 60 s → `nima_walk`; stay within
  15 m of Elena for 45 s → `elena_walk`. Neither → `elena_walk` by default after 90 s (Marek calls you back).
- **Nima branch (walk-and-talk, Nima at your right shoulder, `talk_side`):**
  - **Nima** (if `ask_nima`): *"I know what I said."* (60 m of silence, then:)
  - **Nima:** *"Your father hummed when he climbed. Always the same song. A very bad singer. On the Lhotse Face you
    could hear him two ropes away."* ... *"On the mountain he was the best of us. Remember that, whatever you hear."*
  - flag `nima_memory`. Cost: Elena is worse at Lobuche (−3 SpO₂ in the dusk radio), and Marek: *"Where was my
    Sherpa?"* (Marek trust −1).
- **Elena branch:**
  - **Elena:** *"You don't have to babysit me."* Sangye walks the rest-step beside her; she copies the rhythm.
  - **Elena:** *"Step, breathe, step... Nima taught you that?"* · **Choice** (timeout 10, default silence):
    *"My father did."* / *"Nima did."* / *Say nothing.* → `taught_father / taught_nima / -`.
  - This is the "conclude" step of the rest-step ladder: **the player now teaches it** (competence).
  - flag `elena_helped`; Elena +3 SpO₂ at dusk; at Lobuche, **Nima:** *"You stayed with her. Good. That is the job."*

#### B10 `porters_down` · The rule, alone (S)

- **Where:** `ev_narrow`, the narrowest stretch between S2 and S4 with a drop on one side, found by the world builder
  (steepest side slope within 2 m of the path) [To build].
- **Trigger:** `near ev_narrow r 2500`; a porter's whistle first, then three porters with empty dokos (and two resting
  yaks if the yak props are made, §9) coming down.
- **No prompt, no Nima line before.** If the player stands on the mountain side when they pass, the lead porter nods:
  *"Namaste, dai."* If he is on the drop side: the Chapter 1 `knock` (Knock lease, 15° in 0.14 s), `pose kneel1`,
  health −10, then **Nima** (or Marek if Nima is behind): *"The mountain side. Yesterday you knew it."*
- **Needs [New key]** `side` (mountain or valley side of the path at the moment the porter passes).
- **Camera:** reflex 10° toward the whistle if it comes from 30-120° off-centre.

#### B11 `pasang` · A man who knew him (F)

- **Trigger:** `near ev_pasang r 1800` (between S4 and S5, ≈ 1.4 km along the path). Two climbers from another team and their Sherpa, **Pasang**,
  coming down to Pheriche to recover in thicker air (normal during rotations).
- **Lines [VO]:**
  - **Pasang** (stops, stares): *"You are Mingma's son. You have his face."* He takes Sangye's hands in both of his.
  - **Pasang** *[in Sherpa]* to Nima, a question. **Nima** *[in Sherpa]*, two short words, hard. Pasang lets go.
  - **Pasang** (English, to Sangye): *"The Icefall is bad this year. A kitchen boy from Base Camp died there last week.
    The doctors moved the ladders twice. Go slowly in there."*
- **Staging:** Ji-woo, filming, lowers the camera at the name "Mingma" and looks at his phone (the seed for B21).
- **[Cultural check]:** some Tibetan and Sherpa families avoid saying a dead person's name, at least during the 49 days
  after death. The father died last season (a year ago): would Pasang say "Mingma" aloud to his son? If not, the line
  becomes *"You are his son. You have his face."* and Ji-woo learns the name from the memorial stone instead.
  The two-hand greeting also needs a check.

#### B12 `lobuche` · Lobuche at midday (F)

- **Trigger:** `near lobuche r 4000`; sound first: dogs barking, Nepali pop music from a lodge kitchen radio.
- **Sees:** six or seven stone lodges with blue and green tin roofs weighted with rocks; solar panels; flag cords from
  the roof corners; yak dung drying on a wall; trekkers on a terrace with their boots off in the sun; two yaks resting.
  No readable lodge names (no real businesses).
- **Lines [VO]:** **Diki** (at her door): *"Tashi delek! Nima-dai! Come, come, the tea is ready."*
- **Clock:** `timeto 1030` over the last 300 m.

#### B13 `refill_lunch` · Water, dal bhat, the AMS check (S)

- **Does:** hold [E] at Diki's kettle to fill the bottle with boiled water (hydration). Lunch: dal bhat at the long
  table. Marek runs the Lake Louise questions (the standard AMS score: headache, stomach, tiredness, dizziness,
  each 0-3) with Elena.
- **Lines [VO]:**
  - **Diki:** *"Boiled water. Fifty rupees, but for Nima's boy, nothing."* [To verify: today's price]
  - **A porter** (grinning, at the table): *"Dal bhat power, twenty-four hour!"* [Cultural check: a common trek joke]
  - **Marek:** *"Headache, one to three?"* · **Elena:** (`elena_push` and not `elena_helped`) *"Three."* (otherwise)
    *"One. Maybe two."*
  - **Marek** (if three): *"Then you rest this afternoon, and if it is worse tonight we talk about going down."*
- **Sets:** `elena_sick` = (`elena_push` and not `elena_helped`) or (Nima branch and Elena score ≥ 3). Chapter 3 reads it.
  **Elena never leaves the team** (she is needed in Chapter 11): "going down" is only threatened.

#### B14 `afternoon` · Climb high, sleep low (C)

- **Trigger:** `after refill_lunch`, `delay 20`.
- **Lines [VO]:** **Marek:** *"Who walks up the moraine with Nima? One hour. Up, look, back. It helps the night."*
- **Choice** (timeout 16, default "Rest"): *"I'll go up."* / *"I'll rest here."* → `hike` / `rest`.

#### B14a `hike` · The moraine knoll (C/S)

- **Where:** Lobuche → the **moraine knoll, 5,051 m, +118 m, 1.0 km north-north-east** [Measured: the only knoll
  100-260 m above Lobuche within 1.5 km]. A small cairn with flags on top (existing chorten_b).
- **Sees from the top:** the Khumbu glacier stretching north toward Base Camp; the Nuptse wall 7 km east; the tip of
  Pumori to the north on the edge of sight [Measured: within 6 m in the DEM; check in the editor]. **Everest stays
  hidden** (by about 310 m [Measured]). This is also Chapter 3's way north: the player sees tomorrow's path.
- **Lines [VO]:** **Ji-woo** (filming the view, then lowering the camera): *"Can I ask about your father?"* ·
  **Choice** (timeout 12, default "Not today"): *"Ask."* / *"Not today."* → `jiwoo_talk` / `jiwoo_no`.
  - ask → **Ji-woo:** *"What was he like?"* · **Sangye** (`nima_memory`): *"He sang when he climbed. Badly."*
    (otherwise) *"He was always leaving. Every spring."* · Ji-woo goes quiet and looks at his phone.
- **The cloud (systemic):** at 14:00 a grey veil climbs the valley from Thukla, seen 3-4 km away before it arrives
  (art direction). `look cloud_front` with `nudge {pitchmax 4}` (**nudge 2 of 3**) if missed. **Nima:** *"Down now."*
  - Walk down with Nima → safe; a little snow on the way.
  - Stay at the cairn → Nima waits 30 m below, impatient: *"Sangye!"* After 60 s, a coin toss **[New key]** `roll 0.5`:
    **gap:** a hole opens and sunlight sweeps across the Nuptse face (art direction target image 02, the one quiet-awe
    picture of the chapter; flag `gap_seen`); **no gap:** the cloud swallows you, snow, cold exposure 0.6 for 5 min,
    and the path down is found by the compass.
- **Body:** adaptation rises while you are higher (the model already does this: `AdaptMovingMetersPerSecond`), so the
  hike gives a real, measured benefit tonight and tomorrow; it costs stamina and water. [To verify in calibration
  block A2: the benefit must be visible, but not huge.]

#### B14b `rest` · The lodge in the afternoon (C)

- **Does:** tea by the window; porters play cards (Callbreak) [Cultural check]; warmth and sanity +; sit (posture Sit).
- **Lines [VO]:**
  - **Elena** (`elena_helped`): *"How do you say thank you, in your language?"* · **Sangye:** *"Thuk-je-che."*
    [Cultural check: Sherpa spelling and pronunciation] · **Elena** tries it, badly; the porters laugh kindly.
  - **The solo trekker** arrives, red-faced, no guide, too light a jacket: *"Came up from Pheriche this morning. Piece
    of cake!"* (Pheriche 4,371 m → Lobuche 4,934 m: +560 m in a day [To verify Pheriche height]; the classic mistake.)
  - **Choice** (timeout 10, default silence): *"Go slower. Drink."* / *Say nothing.* → `warned`.
  - warned → **Trekker:** *"Yeah, yeah. Thanks, mate."* (He remembers it in B18.)

#### B15 `snow` · The afternoon snow (S)

- **Trigger:** `delay 600` after B14 or the hike's return. `timeto 1600`. Light snow starts (the spring pattern: clear
  mornings, cloud and snow in the afternoon [To verify]). Diki lights the dung stove in the dining room; the room
  fills with trekkers, socks hanging over the stove pipe.

#### B16 `sur` · Smoke for the hungry ones (C)

- **Trigger:** `after snow`, `look sur_burner` through the window or `near diki_burner r 500`. Optional.
- **Sees:** dusk, snow falling; Diki at a small whitewashed burner outside, sprinkling flour and butter on embers; a
  sweet, heavy smoke.
- **Does:** `holdkey F 1.5` to sprinkle tsampa if you join her. Sanity +.
- **Lines [VO]:**
  - **Diki:** *"Sur. For the hungry ones who wander. They cannot eat, only smell. So we give them smoke."*
  - **Diki:** *"Your father ate here every spring. Always two plates of dal bhat. Always he paid for the porters' tea."*
- **Why:** it roots the **Hungry Ghost realm** (the first of the Six Realms the game will use) in a real practice.
  [Cultural check: the sur offering among Sherpas: who performs it, at what time of day, for whom (the dead in the 49
  days, wandering spirits), and the words.] If the Six Realms re-skin of the weather wheel exists by then, the
  forecast shows the Hungry Ghost sign for the first time tonight; otherwise nothing [To build, depends on the re-skin].

#### B17 `radio_dusk` · Evening numbers (F)

- **Lines [VO]:** **Dr. Hale (radio):** *"Evening, Lobuche. Numbers..."* Sangye: 84 (hike: 86). Elena: 76 if
  `elena_sick`, else 81 (+3 with `elena_helped`, −3 on the Nima branch). *"Base Camp tomorrow, then. I'll have the
  kettle on."*

#### B18 `stranger` · The heel-to-toe test: the peak (S/C)

- **Trigger:** `after radio_dusk`, `delay 40`. The trekker (if `rest`, you already know his face) sits slumped in a
  corner; he cannot lift his cup; he laughs at nothing; he is sick in a bucket (sound only, off-screen).
- **Lines [VO]:**
  - **Diki** (to Nima and Sangye): *"That boy. He is not right."*
  - **Nima:** *"Sangye. Ask him to walk."*
  - Prompt *[E] Ask him to walk the line*. **Sangye:** *"Stand up. Walk heel to toe, along this board."* The trekker
    tries: he sways, steps off, grabs a chair (Mixamo "drunk walk" style clip; HACE looks like drunkenness).
  - **Trekker** (`warned`): *"You told me... go slower. I'm fine. I'm just... tired."* (otherwise) *"I'm fine. Just
    tired. Is it... is it night?"*
  - **Nima** (quietly): *"His brain is swelling. If he sleeps here tonight, maybe he does not wake."*
  - Optional: radio Dr. Hale (prompt *[R] Call Base Camp*): *"Dexamethasone, eight milligrams, if you have it. Oxygen
    if the lodge has a bottle. And get him down. Down is the medicine."* [To verify against the Wilderness Medical
    Society altitude guidelines]
- **The decision** (timeout 20, default = the third option):
  - with `firstaid`: *"Give him my dexamethasone."* / *"I'll walk him down tonight."* / *"Leave him with Diki."*
  - without `firstaid`: *"I'll walk him down tonight."* / *"Leave him with Diki."* + caption *You did not pack the
    first-aid kit.* (two `choice` actions with `if` / `ifnot`: no new code)
  - → `gavedex` / `walkdown` / `leftdiki`.
- **Ji-woo** lifts his camera; **Nima:** *"Not this."* He lowers it.

| Choice | What it costs you | What happens to him | Later |
|---|---|---|---|
| Give the dexamethasone | the kit's only altitude medicine (`firstaid_used`) | improves in an hour; flown out at first light; survives | Chapter 4: Dr. Hale thanks you. A later chapter needs that medicine and it is gone (v1 rule: "the medicine given on day 2 is missing on day 22") |
| Walk him down tonight | 1.5 h of sleep, cold, stamina; Marek trust −1 (*"You are paid to look after this team"*); Nima trust +1 | survives | Chapter 3 starts tired (`tired`, stamina cap 80% until noon) |
| Leave him with Diki | nothing now | lodge oxygen; helicopter at first light if the weather allows; **survives or dies by chance** (seeded at chapter start; 65% survive, to tune) | Chapter 3 may hear the helicopter at dawn (`heli_dawn`); Chapter 4: Dr. Hale tells his fate |

No option is obviously right (v1 pillar 2). The medicine choice is the cheapest tonight and the most expensive later.

#### B19a `walk_down` · At night, in the snow (C)

- **Does:** headlamp on (soft, shadowless: the Chapter 1 light policy); `hand trekker` (hold [F] to hold his arm: the
  Dolma input returns for the third time, after Dolma's hand and the khata); Pace lease 0.8 m/s; 400 m down the valley
  to `ev_handover` (between S5 and S4), where a lodge porter from Thukla, called by Diki's phone, meets you with a
  second light.
- **Company:** Nima comes **only if** not `elena_sick`; otherwise **Nima:** *"I stay with Elena. Go. Keep his arm."*
  and you are alone with the stranger (solitude as a tool).
- **Lines [VO]:** **Trekker** (confused, now and then): *"Is this the way to the airport?"* · *"My mum's waiting. She
  worries."* · *"Why is it so dark?"* · **Porter** (at the handover): *"I take him. Go, dai, you are cold."*
- **Ends:** `blink sleep` behind closed eyes → `time 2340`, back on the bench (B20). Body temperature −1.5 °C, stamina
  30, flag `tired`.

#### B19b `dex` / B19c `diki_o2`

- **dex:** hold [E] to give the tablets; Diki brings tea; `timeto 2130`; the trekker can hold his cup. **Diki:** *"In
  the morning, the helicopter."*
- **diki_o2:** Diki fits an orange oxygen bottle and mask from her storeroom (existing oxygen props). **Diki:** *"I
  have done this before. In the morning, the helicopter. If it can fly."*

#### B20 `bench` · The bench (F/C)

- **Trigger:** after B19x. Behind closed eyes (`blink`): `teleport bench_sangye`, `pose sit`, `prop sleeping_bag`.
  The dining room is dark; the trekkers have gone to their rooms; guides and porters sleep on the benches round the
  cooling stove (normal for lodge staff [To verify]).
- **Lines [VO]:** **Nima** (from the next bench): *"Your bottle. Inside the bag, or it is a stone by morning."*
- **Does:** press [E] on the bottle within 20 s → `bottle_in`; otherwise `bottle_out`. **Chapter 3's first beat reads
  this flag** ("Lobuche at dawn, frozen bottles").

#### B21 `video` · The phone (F/C)

- **Trigger:** `after bench`, `delay 25`. Ji-woo comes in from the rooms, the phone's light on his face, and sits on the
  end of Sangye's bench.
- **Lines [VO]:**
  - **Ji-woo:** *"Sangye... I have to tell you something. Before we came, I watched every video of the route. There is
    one from last May. At the Balcony."* ... *"Today that man said the name. People wrote it under the video."* ...
    *"I'm sorry. I didn't know he was your father."*
  - **Choice** (timeout 16, default "Say nothing"): *"Show me."* / *"Delete it."* / *Say nothing.* →
    `video_seen` / `video_deleted` / `video_unseen`.
  - **show:** `inspect phone_video` (the phone, its screen a still: headlamp light, a figure in a red down suit sitting
    by the fixed rope, facing up the ridge, mask off). The clip's sound: wind, crampons, a man's voice in English,
    *"Don't look. Keep moving."* Eight seconds. Then **Ji-woo:** *"I'm sorry."* Nima, awake on the next bench, gets
    up and walks out into the snow without a word (the door, the cold air).
  - **delete:** Ji-woo deletes it in front of him. *"Done. It's gone."* · **Nima** (from the dark): *"Good."*
  - **silence:** **Ji-woo:** *"I'll keep it. If you ever want..."* He leaves. Nima turns to the wall.
- **Camera:** Inspect lease (closes after the clip, or on [E]); Attention lease −3° for 4 s after it (as Chapter 1's
  night). No nudge, no FOV beat.
- **The still image:** a render of the existing `father_sit` model at the Balcony in headlamp light, made in the
  engine (no new credits). It must match the prologue exactly.
- **[Cultural check]:** how a Sherpa family feels about images of their dead, and bodies left on the mountain being
  filmed. The scene treats filming as shameful (Ji-woo apologises; deleting is offered).

#### B22 `end2` · Sleep and the page (F)

- `pose lie`; the stove ticks as it cools; snow whispers on the tin roof; `blink sleep`; `fade 1`; `day 2`.
- **The journal page** (head *Day {day} / 49*), lines by flag:
  - *"Nima burned juniper at his stone at dawn."* (always)
  - `ask_nima`: *"I asked him what he meant. 'Not in front of him,' he said."* · `learn_mani`: *"He taught me the prayer.
    He says my father said it too fast."*
  - `nima_memory`: *"My father sang when he climbed. Badly. Nima says he was the best of them."*
  - `elena_helped`: *"I walked with Elena. I taught her the step."*
  - `risky2`: *"I stood on the wrong side of the porters again."*
  - *"A man who knew him spoke to Nima in Sherpa. Nima would not translate."* (always)
  - `hike` + `gap_seen`: *"From the moraine I saw Nuptse burn in the sun. Not her. She hides."*
  - `gavedex`: *"I gave the boy my medicine."* · `walkdown`: *"I walked a stranger down in the dark. My legs are
    stone."* · `leftdiki`: *"I left the sick boy with Diki. I don't know if he will live."*
  - `video_seen`: *"In Ji-woo's video he is sitting. Facing up. People who fall do not sit down."*
  - `video_deleted`: *"Ji-woo had a video of him. I made him delete it. I see it anyway."*
  - `video_unseen`: *"Ji-woo has a video of him. I did not look."*
- `title "CHOMOLUNGMA" sub "Chapter 2 · The Hidden Mountain"`; still black; `end` (Chapter 3 opens at dawn).

### 3.3 The teaching ladder (continued from Chapter 1)

| Mechanic | Ch1 step | Ch2 step | Next |
|---|---|---|---|
| Rest-step | introduce, develop, twist | **conclude**: works without a word; on the Elena branch you teach it | used everywhere |
| Mountain side | introduce with a prompt | **conclude**: no prompt at all (B10) | yaks/porters later are pure systems |
| Water | stream, "drink before you are thirsty" | boiled water at the lodge; the slushy bottle | — |
| Frozen bottle | — | **introduce** (B02), **develop** (B20 choice) | **twist** in Ch3 dawn |
| Hand hold [F] | Dolma's hand; the khata | the juniper; the stranger's arm | Nima holding your rope (later) |
| Talk choices, silence | introduced | used three times | — |
| Reading other bodies | Elena's headache | Elena's AMS score; the heel-to-toe test (HACE) | the player's own body higher up |
| Clockwise around chortens | (mani wall, if built in Ch1) | B06 | Base Camp puja |

### 3.4 Pacing check (rule: no stretch over 6 minutes without a decision, an event or a discovery)

| From | To | Distance [Measured] | Walking at 2.3 m/s | OK? |
|---|---|---|---|---|
| night camp | memorials (B06) | 150 m | 1.1 min | yes |
| memorials | top of the climb (B08) | ≈ 400 m walked (230 m straight, 90 m up) | ≈ 4 min with the rest-step | yes |
| B08 | ahead/behind (B09) | ≈ 290 m | 2.1 min | yes |
| B09 | porters (B10) | ≈ 330 m (S2 → S3) | 2.4 min | yes |
| B10 | Pasang (B11) | ≈ 560 m (S3 → 1.4 km) | 4.1 min | yes |
| B11 | Lobuche (B12) | ≈ 520 m | 3.8 min | yes |
| Lobuche | knoll top (hike) | 1.0 km, +118 m | ≈ 9 min uphill | **just**: Ji-woo's question fires halfway if not yet asked |

---

## 4. Places

### 4.1 Coordinates

Formulas (from `terrain-georeference`): `x = -504000 + col × 3374.24`, `y = -504000 + row × 3374.24` (cm, north = −Y),
`png = 13.4538 × elev_m − 52342`, `z = (png − 32768) × 951.4 / 128 + 129770` (cm). Latitude/longitude by a local flat
projection checked against Dingboche and Gorak Shep (within 2 px). Heights marked "traced" come from
`Chapter1_places.ygs` (on the detail tiles); the others from the 34 m DEM, which can be ±20-30 m off on slopes.

| Place | What stands there | Pixel (col, row) | World x, y, z (cm) | Height | Lat / lon |
|---|---|---|---|---|---|
| Night camp (`night_tent`, Ch1) | 3 domes, stove, duffels [Built] | 195.0, 756.0 | 153 986, 2 046 836, −20 047 | 4,828 m traced | 27.9313 N, 86.8147 E |
| Memorials (`memorial`, Ch1; leg 0 at 5,800 m) | chortens, flags [Built] | 196.6, 751.9 | 159 495, 2 032 962, −18 460 | 4,844 m traced | 27.9325 N, 86.8153 E |
| Father's stone (Ch1) | stone, khata, sang fire **[To build: a ring of flat stones + embers]** | 197.3, 751.3 | 161 885, 2 031 110, −18 046 | 4,847 m traced | 27.9327 N, 86.8155 E |
| S1 top of the climb (`s1`) | the valley reveal (B08) | 192.5, 746.5 | 145 541, 2 014 870, −11 507 | 4,913 m DEM | 27.9342 N, 86.8139 E |
| S2 (`s2`) | ahead/behind (B09) | 187.0, 740.0 | 126 983, 1 992 938, −10 851 | 4,920 m | 27.9361 N, 86.8120 E |
| S3 | `ev_narrow` searched here (B10) | 183.5, 731.0 | 115 173, 1 962 569, −10 249 | 4,926 m | 27.9389 N, 86.8108 E |
| S4 | `ev_pasang` lies 180 m past S4 (B11) | 182.0, 720.0 | 110 112, 1 925 453, −12 858 | 4,900 m | 27.9422 N, 86.8103 E |
| S5 | `ev_handover` (B19a): 70 m south of S5, 400 m from Lobuche | 182.5, 709.0 | 111 799, 1 888 336, −12 108 | 4,907 m | 27.9456 N, 86.8104 E |
| **Lobuche anchor** (`lobuche`) | 6-7 lodges, Diki's lodge with the dining room, a burner, flags, resting yaks | **183.0, 699.2** | **113 486, 1 855 269, −9 487** | **4,934 m** DEM (real Lobuche ≈ 4,940 m) | **27.9486 N, 86.8106 E** [To verify on a map] |
| Moraine knoll (`knoll`) | small cairn with flags (chorten_b) | 195.0, 672.0 | 153 977, 1 763 489, 2 290 | 5,051 m | 27.9569 N, 86.8147 E |

The story path S0-S6 is **1.92 km** [Measured]. All world z values are DEM estimates: the world builder traces the
real ground, as in Chapter 1.

### 4.2 Why Lobuche is not on leg 0

- `route.json` leg 0 was planned by A* over the height grid. Between the memorials and Gorak Shep it runs **on the
  Khumbu glacier** itself; the real trail runs in the ablation valley to the west. Seen on the satellite image that came
  with the DEM [Measured overlay, scratch image].
- The real Lobuche sits in that valley, **972 m west of leg 0** (nearest leg-0 point: 7,589 m along, 4,937 m). Its DEM
  height (4,934 m) matches the real village (≈ 4,940 m), which supports the coordinates.
- **The detail tiles stop at 400-545 m from leg 0** (`Tools/Terrain/build_route_terrain.py`, CORE_M 400, FADE_M 545).
  From S2 on, the path and Lobuche stand on the coarse landscape only.
- **Recommendation:** add the story path and Lobuche (and the knoll) as a **spur** to the corridor source of the route
  terrain builder: about 6-10 new 500 m tiles. RAM must be measured before and after (game peak today 10-12 GB).
- **Fallback, not recommended:** put "Lobuche" inside the existing corridor. The nearest corridor ground is on the
  glacier, which is wrong, and it breaks the rule "never invent a position".
- **Chapter 3 note:** the real trail north from Lobuche passes the knoll and rejoins leg 0 around 8.3-8.9 km. The
  Chapter 3 designer decides; this chapter's hike already shows that way.
- Leg 0's fixed rope (if still visible on the trek) must stay hidden through Chapters 1-3: there are no fixed ropes on
  the trek in reality.

### 4.3 What can be seen [Measured: line of sight over the DEM with earth curvature]

| From | Everest summit | Nuptse | Pumori | Taboche, Cholatse |
|---|---|---|---|---|
| Night camp / memorials | hidden by 410-430 m | top on the edge | top on the edge | visible (dawn light, B03) |
| Valley (S2-S3) | hidden by 320-360 m | visible | edge | visible behind you |
| Lobuche | hidden by 330 m | visible, 7.7 km east | edge (13 m) | visible |
| Moraine knoll | hidden by 310 m | visible, 7.1 km | edge (6 m) | — |

Everest is hidden all day, as in reality: that is the chapter's title. "Edge" = within ±25 m in a 34 m DEM; check in the
editor before staging a shot on it.

### 4.4 Light and time

| Beat | Clock | Light | Art-direction note |
|---|---|---|---|
| B01-B04 | 06:10-06:50 | blue camp, apricot on Taboche/Cholatse tops | "dawn = warm far, blue near" |
| B06-B08 | 07:40-08:30 | low sun behind Nuptse: a dark wall with a bright rim | the hidden mountain |
| B09-B12 | 08:30-10:30 | deep blue sky, crisp detail, footprints give scale | "midday" |
| B14a | 13:00-14:30 | the veil climbing from Thukla; a cloud gap on Nuptse (if lucky) | target image 02 |
| B15-B17 | 15:00-18:30 | snow, sky and snow close in value; warm windows | "overcast" |
| B18-B22 | 19:30-23:40 | dung-stove glow, headlamps (shadowless), one phone screen | the phone is the last light |

---

## 5. Cast and states

| Who | Role in Ch2 | Look / asset | States and flags |
|---|---|---|---|
| **Sangye** (you) | | `sangye_ai` [Built] | reads all Ch1 flags; writes the flags in §3 |
| **Nima** | the secret, the ritual | `nima_ai` [Built] | sang at dawn; ahead in the valley; shaken by the video (the only time); trust ±1 by choices [To build: a trust value] |
| **Marek Kovač** | authority | `marek_ai` [Built] | trust −1 on the Nima branch and on `walkdown` |
| **Elena Ruiz** | care | `elena_ai` [Built] | `elena_sick` (scripted: slower, stops, lines); `elena_helped`; never leaves the team |
| **Ji-woo Han** | the witness | MetaHuman `jiwoo` [Built] | films; the seed at Pasang; the hike talk; the video |
| **Dr. Hale** | radio | voice only | morning and dusk checks; HACE advice |
| **Porters** (3) | the trail's workers | MetaHuman `porter_a` [Built] + tints | B05, B10, B14b, B19a handover |
| **Diki** (≈ 40, Lobuche lodge keeper, Sherpa) | the first warm stranger | **new** Meshy character | knew the father; sur at dusk; oxygen bottle |
| **The solo trekker** (≈ 26; placeholder name "Jonas", Q5) | the stranger | **new** Meshy character | fine → too fast → HACE; fate seeded at start (`trekker_lives`) |
| **Pasang** (≈ 55, another team's Sherpa) | a man who knew him | **new** Meshy character (or a re-dressed MetaHuman) | one encounter; speaks Sherpa to Nima |
| Two climbers with Pasang, 3-4 trekkers at Lobuche | background | existing looks with new tints | idle, sitting, walking |
| Yaks (2) | life at the lodge | **new** static Meshy props (no animation) | standing, lying |

**Performance:** Lobuche can hold 10+ people at once. Use Meshy characters (lighter) for background, MetaHumans only
where already built, LODs and hiding out of view; measure (12 GB VRAM laptop, `feedback-vram-12gb`).

---

## 6. Gameplay and physiology hooks

| System | In this chapter | Exists? |
|---|---|---|
| **SpO₂** (oxygen saturation) | radio checks morning and dusk; the model's acclimatised resting value at Lobuche is about 81% (between Dingboche 84 and Base Camp 79, from `SurvivalConfig.h`) | [Built]; live numbers in `say` need **[New key]** `{spo2:player}` |
| **Acclimatisation** | the hike raises the adapted altitude ("climb high, sleep low"); a bad night (walk-down) does not lower it | [Built] (`StartAdaptedMeters 4400`, adapt rates); check the hike's benefit is visible but modest (A2) |
| **Water** | slushy bottle at dawn; boiled water at Lobuche; the bottle in or out of the bag tonight | hydration [Built]; **frozen bottle state** [To build]: a flag-driven item state that Ch3 reads |
| **Cold** | dawn ≈ −10 °C, the hike cloud, the night walk; `ygg.Survival.Cold` | [Built]; temperatures [To verify: Lobuche spring nights] |
| **Fatigue / stamina** | the climb out of the hollow, the hike, the night walk; `tired` carries to Ch3 | stamina [Built]; carry-over cap [To build] |
| **AMS (Elena)** | Lake Louise score, Diamox, SpO₂ in the 70s | scripted NPC states [To build]; no AMS system for the player yet (v1 §4.1, later) |
| **HACE (stranger)** | heel-to-toe test, confusion, stumbling walk, treatment choices | scripted NPC + a stumbling clip [To build] |
| **Medicine** | `firstaid` from Ch1 decides the dexamethasone option; `firstaid_used` | flags [Built]; inventory item [To build] |
| **Sanity** | sang +, sur +, tea with porters +, the video − | SanityComponent [Built] |
| **Oxygen bottles** | only Diki's, for the stranger | props [Built] |
| **Ropes and ladders** | **none**: there are none on the trek; ladder training is at Base Camp (Chapter 4) | — |
| **Company** | crowd → team → alone (only on the walk-down branch) → crowd → bench | — |

**Flags to Chapter 3** (the save must carry them from file to file [To build if missing]): `bottle_in / bottle_out`,
`tired`, `firstaid_used`, `heli_dawn`, `trekker_lives` (secret until Ch4), `hike`, `gap_seen`, `elena_sick`,
`elena_helped`, `nima_memory`, `video_seen / video_deleted / video_unseen`, `jiwoo_talk`, `warned`, plus every Ch1 flag.

### 6.1 Camera budget (vocabulary of `Chapter1_Camera_Spec.md`; the rig is the only camera writer)

| Item | Count / use |
|---|---|
| Nudges (max 3, ≥ 60 s apart, only after 12 s of not looking) | 2: the sang smoke (B03), the cloud veil (B14a); 1 spare, unused |
| Reflexes (10°, sudden sound 30-120° off-centre) | porters' whistle (B10), Pasang's call (B11), the door when Nima walks out (B21) |
| Knock | only if on the drop side at B10 |
| Postures | Lie (B01, B22), Sit (B01, B14b, B20, B21), Crawl (B01 exit), Kneel1 (sang fire, optional) |
| Leases | Move (lying/sitting), Inspect (bottle, phone), Hands (juniper, the stranger's arm), Pace (0.8 m/s walk-down), Lids (blinks), Attention (−3° after the video), Breath (after a knock) |
| Cuts, all behind closed eyes | 3: start (B01), back from the walk-down (B19a), onto the bench (B20) |
| FOV beat, cinematic shots, view locks | none |

---

## 7. How it is built (story machine and world)

| Piece | What | Status |
|---|---|---|
| `Tools/Story/chapter2_story.py` | writes `Content/Story/Chapter2.ygs`; same `validate()` as Chapter 1 (nudges ≤ 3, teleports behind closed eyes, no removed keys) | [To build] |
| `Tools/Story/build_chapter2_world.py` | traces the story path, Lobuche, the knoll and the event spots; places lodges, props, flags; checks key frames (B08 reveal: Lobuche within ±20° ahead; B03: the smoke visible from the tent door) → `Chapter2_places.ygs` | [To build] |
| Chapter chaining | Chapter 1's `end` starts Chapter 2; flags and the body state carry over; a save at the break | [To build] |
| New keys | `side` (B06, B10), `roll` (B14a, the trekker's fate), `{spo2:player}` token (radio) | **[New key]** ×3 |
| Lodge interior | Diki's dining room: reuse the Blender Sherpa house shell (`Content/Story/House`) widened, plus Poly Haven furniture already imported (benches, tables, barrel stove) | [To build] |
| Smoke | one cheap smoke effect for sang, sur and chimneys (none exists in the project) | [To build] |
| Terrain spur | §4.2 | [To build] |

---

## 8. Robot-player test plan

**Tool:** `Tools/Bot/run_bot.ps1 -Chapter chapter2 -Fast -YggCamRec` [To build: the `chapter2` option; `summit` exists]
with `ygg.Story.Auto 1`, under the flight recorder. Start state: the end of Chapter 1, or a preset of Ch1 flags
**[To build if missing]** (for example `ygg.Story.Start Chapter2 flags=firstaid,elena_push`). Choices are forced with
`ygg.Story.Choose`; walking choices (B09, B14a cloud, B20 bottle) by bot waypoints.

**Every run must reach, in order:** `wake2 → sang_seen → sang_offer → break_camp → pass_stone → climb_out → valley_open →
nima_or_elena → porters_down → pasang → lobuche → refill_lunch → afternoon → (hike | rest) → snow → radio_dusk →
stranger → (walk_down | dex | diki_o2) → bench → video → end2`. (`bottle_slush` and `sur` are optional; the bot visits
them on runs 1 and 4.)

| Run | Preset / forced choices | Checks beyond "all beats reached" |
|---|---|---|
| 1 | no Ch1 flags; silence everywhere (defaults: Elena branch, rest, leave with Diki, bottle out, video unseen) | shortest time; every default resolves |
| 2 | `firstaid`; ask Nima; Nima branch; hike, go down at once; give dex | dex option present; `firstaid_used` set; Marek line on the Nima branch |
| 3 | no `firstaid`; walk down; `elena_push` (so Nima stays) | dex option absent with its caption; Pace lease 0.8 m/s; hand hold works; alone with the trekker; blink back to the bench; `tired` set |
| 4 | `elena_push`; Elena branch; hike, wait at the cairn; `roll` forced both ways (2 sub-runs) | `gap_seen` only on gap; cold applied on no-gap; compass path back found |
| 5 | bot stands on the drop side at B10 | knock fires once; no knock on the mountain side (separate pass) |
| 6 | jump to `video` ×3 (show, delete, silence) | phone inspect opens and closes; Nima leaves on "show"; journal lines match |
| 7 | full run with `ygg.Story.Debug` | the journal page shows exactly the lines of the flags set; flags written to the save for Chapter 3 |

**Measured on every run:** total minutes per branch (target 65-82); the longest stretch without an event (≤ 6 min);
stuck time on the zigzag climb and lodge doors (< `StuckSec`); followers within 25 m while walking and never > 3 m off
the path; nudges ≤ 3, no eyes-open teleport, no lease expiry warnings, eye ≥ 120 cm when moving; frame time and hitches
at Lobuche with all NPCs (no freeze > 100 ms with eyes open); RAM peak. Review shots for the director with
`story_shots.ps1 -Only ch2_sang,ch2_reveal,ch2_lobuche,ch2_knoll,ch2_dining,ch2_phone` [To build: the shot list].

---

## 9. Assets

### 9.1 What exists (Content/Story and Content/Audio) and is reused

| Need | Existing asset |
|---|---|
| Night camp, tents, bedding, stove, duffels | `AI/tent_dome`, `AI/bedding`, `AI/stove_set`, `AI/cook_set`, `AI/duffel`, `AI/barrel_blue` (all placed in Ch1) |
| Memorials, father's stone, khata | `AI/chorten_a/b/c`, `AI/father_stone`, `AI/khata_tied` |
| Porter loads | `AI/porter_load`, `AI/doko` |
| Bottle, headlamp, daypack, glasses | `AI/water_bottle`, `AI/headlamp`, `AI/daypack`, `AI/glacier_glasses` |
| Lodge life | `AI/dung_heap`, `AI/tarchen`, `AI/copper_drum`, `AI/butter_lamp`, `AI/offering_bowl`, `AI/choktse`, `AI/meat_slats` |
| Dining room furniture and kitchen | `PH/painted_wooden_bench`, `_table`, `_stool`, `_cabinet(_02)`, `_shelves`, `PH/barrel_stove` (the dung stove), `PH/plastic_thermos`, `PH/modified_thermos`, `PH/metal_jug`, `PH/jug_01`, `PH/pot_enamel_01`, brass pots, `PH/Lantern_01`, jerrycans, crates, `PH/propane_tank` |
| Lodge shell and interior | `House/SM_House_*`, `House/SM_MudStove`, `House/SM_TibetanRug`, `House/SM_Dongmo` (butter-tea churn) |
| Wall pictures | `Photos/T_Thangka_MountMeru`, `T_Thangka_Shakyamuni` |
| The phone still | rendered in-engine from `AI/father_sit` at the Balcony (new texture, no credits) |
| Cast | `nima_ai`, `marek_ai`, `elena_ai`, `sangye_ai`, MetaHumans `jiwoo`, `porter_a` (`Cast.ygs`) |
| Sounds | `Audio/Story/Story_Stove_Loop`, `Story_Radio_Squelch`, `Story_Radio_Static_Loop`, `Story_Flags_Loop`, `Story_Stream_Loop`, `Story_Heartbeat`; `Audio/Ice/Ice_Groan_01-04` (glacier behind the moraine: candidates); `Audio/Weather/Wind_*`; `Audio/Footsteps/*` |

### 9.2 New: Meshy prompts (house style of `Tools/Meshy/assets.py`)

Ready to paste into `ASSETS` (I did not edit that file). Props ≈ 33 credits each (concept + textured model),
characters ≈ 41 each (concept + model + rig). Add 20% for retries.

```python
    # ---------------------------------------------------------------- chapter 2: Lobuche
    dict(id="lodge_main", kind="prop", size=14.0, tris=60000,
         what="a two-storey trekking lodge in the Khumbu, Nepal, built of rough grey dry-laid stone, small wooden windows painted blue with many small panes, a shallow roof of faded blue corrugated tin weighted with rocks, two solar panels and a black plastic water tank on the roof, a wooden door with a painted frame, a short stone terrace in front, dusty and weathered, no signboard and no text"),
    dict(id="lodge_annex", kind="prop", size=8.0, tris=30000,
         what="a single-storey Himalayan stone hut with a faded green corrugated tin roof weighted with rocks, one small blue wooden window, a thin black stove chimney pipe, a stack of dried yak-dung patties against one wall, dusty"),
    dict(id="sang_burner", kind="prop", size=1.2, tris=10000,
         what="a Sherpa juniper incense burner standing outside a lodge: a small whitewashed stone and clay altar about 1.2 metres tall with an open fire bowl on top blackened by smoke, ash and a few green juniper sprigs in the bowl"),
    dict(id="mani_stones", kind="prop", size=1.5, tris=20000,
         what="a low pile of flat grey stones carved with Tibetan Buddhist mantras, the carved letters painted faded white and ochre, shallow and worn so the letters cannot be read, patches of lichen"),
    dict(id="phone_rugged", kind="prop", size=0.16, tris=3000,
         what="a modern smartphone in a scuffed black rugged rubber case, the screen dark and blank, a thin wrist cord"),
    dict(id="yak_stand", kind="prop", size=2.2, tris=20000,
         what="a shaggy black and white Himalayan yak standing still with its head low, long hair hanging from its belly, curved horns, a wooden pack saddle with two empty sacks, a red wool tassel at its neck, dusty"),
    dict(id="yak_lie", kind="prop", size=2.0, tris=20000,
         what="a shaggy black Himalayan yak lying down resting on stony ground with its legs folded under it, long belly hair spread on the ground, curved horns, a red wool tassel at its neck, dusty"),
    dict(id="tsampa_offering", kind="prop", size=0.35, tris=5000,
         what="a small open cloth bag of pale roasted barley flour (tsampa) beside a worn wooden bowl holding a lump of yellow butter and a few dried juniper sprigs"),
    dict(id="diki", kind="char", size=1.55, tris=50000,
         what="a Sherpa woman of about 40 who runs a trekking lodge in the Khumbu, Nepal, weathered sun-darkened face with red cheeks, black hair in a long braid under a faded floral headscarf, wearing a long dark maroon sleeveless wrap dress over a pink blouse, a striped woven apron in faded rainbow colours at the front, an open grey fleece jacket over it, and black rubber-soled shoes; real worn fabric, no logos"),
    dict(id="trekker", kind="char", size=1.82, tris=50000,
         what="a European man of about 26 on a budget trek in the Himalaya, pale sunburnt face, a few days of fair stubble, messy blond hair under a cheap grey beanie, wearing a thin light-blue synthetic jacket too light for the cold over a grey cotton hoodie, black running tights under baggy shorts, and low trail-running shoes; pale and tired; no logos"),
    dict(id="pasang", kind="char", size=1.65, tris=50000,
         what="a Sherpa climbing guide of about 55 from Nepal, a deeply lined dark face, grey stubble, short grey hair under a black knit beanie, wearing a faded red and black down jacket worn shiny at the cuffs, black softshell trousers, a climbing harness at the waist and heavy brown leather mountaineering boots; real worn fabric, no logos"),
```

| Set | Items | Credits (≈, before retries) |
|---|---|---|
| **Minimal** | lodge_main, sang_burner, phone_rugged, diki, trekker | 3 × 33 + 2 × 41 = **181** |
| **Recommended** | minimal + lodge_annex, mani_stones, yak_stand, pasang | 181 + 3 × 33 + 41 = **321** |
| **Full** | recommended + yak_lie, tsampa_offering | 321 + 2 × 33 = **387** |

Notes:
- The current Meshy balance is unknown here (167 on 2026-11-19, then three characters were made the next night). Check
  it with `meshy.py balance` before any plan. Buying credits is the director's money.
- Meshy's rigger is for humanoids, so the yaks stay static [To verify]. Animated yaks only if a free Fab/Megascans yak
  appears.
- Pasang can be a re-dressed MetaHuman instead (−41 credits), at the cost of another MetaHuman on screen.
- Diki's dress needs the same care as Aunt Pema's (the striped apron is worn by married women [Cultural check]).
- The stumbling walk: a "drunk walk" style clip from Mixamo (as other Mixamo clips were used) [To find].

---

## 10. Sound needs (the director chooses every sound by ear; this is a list, not a pick)

Identity rule (`feedback-audio-identity-fate`): deep, slow, with space; never light or "app-like". Music: almost none.

| # | Need | Where | Have a candidate? |
|---|---|---|---|
| 1 | a low murmured mantra, one male voice (the director's own voice?) | B01, B03 (lure) | no |
| 2 | juniper crackle on embers, small and dry | B04 | no |
| 3 | still dawn air, very faint far flags | B01-B04 | `Story_Flags_Loop` (distance version) |
| 4 | ice knocking in a plastic bottle | B02 | no |
| 5 | camp morning: porters talking, a kettle, rope pulled tight | B05 | partly (`Rope_Tighten_*`) |
| 6 | glacier groan behind a moraine wall, far and muffled | B08 onward | `Ice/Ice_Groan_01-04` (muffled versions needed) |
| 7 | small rockfall from the moraine wall (systemic danger cue) | valley | no |
| 8 | a porter's whistle; empty dokos creaking | B10 | no |
| 9 | yak bells, slow (only if the yaks are made) | Lobuche | no |
| 10 | dogs barking far off; Nepali pop from a small radio in a lodge kitchen | B12 | no (licensing: must be free or made) |
| 11 | lodge room tone: cups, benches, trekkers murmuring in several languages | B13-B18 | no |
| 12 | dung stove lit and burning; cooling ticks at night | B15, B22 | `Story_Stove_Loop` (+ ticks) |
| 13 | snow on a tin roof, soft | B15, B22 | no |
| 14 | the afternoon cloud arriving: wind changing colour | B14a | `Wind_Gusty`, `Wind_Calm` |
| 15 | sur: flour and butter hissing on embers | B16 | no |
| 16 | radio (exists) | B05, B17, B18 | `Story_Radio_*` |
| 17 | retching, muffled, off-screen | B18 | no |
| 18 | night walk: boots in new snow, a stranger's uneven steps, breath | B19a | `Step_Snow_*`, breath |
| 19 | the phone clip's sound: wind, crampons, a man's voice *"Don't look. Keep moving."* | B21 | no (a recorded line) |
| 20 | a door, cold wind rushing in | B21 | no |
| 21 | one low drone or nothing under the phone clip | B21 | `Story_Plume_Drone`, or silence |
| 22 | a helicopter, far, at dawn (for Chapter 3, `heli_dawn`) | Ch3 start | no |

---

## 11. Voice script summary

About **75 spoken lines** (each logged as `Story [VO] who: text`): Nima ≈ 22, Diki ≈ 7, Ji-woo ≈ 9, Elena ≈ 8,
Marek ≈ 6, Dr. Hale ≈ 8, the trekker ≈ 8, Pasang ≈ 3 (+ Sherpa lines), porters ≈ 3, Sangye ≈ 10 (choice echoes).
Sherpa-language lines need a native speaker or a Nepali fallback (Q6).

---

## 12. Risks and open questions

### 12.1 Risks

| Risk | Likelihood | Effect | What we do |
|---|---|---|---|
| Lobuche and the path stand off the detailed terrain (972 m from leg 0) | certain without work | coarse 34 m ground under the village | terrain spur (§4.2); measure RAM |
| Lodge buildings and the dining room look fake | medium | the chapter's main place looks cheap | Meshy exteriors + the Blender house shell + Poly Haven interior; the director judges screenshots before the play-test |
| Too many people at Lobuche for the laptop | medium | stutter, VRAM | Meshy characters for background, LODs, hide out of view; measure |
| Flags do not carry from Chapter 1's file to Chapter 2's | unknown | choices forgotten | check `YggStory`; add chaining and a save at the break |
| The night walk in the dark (pathing, snow, headlamp) | medium | the trekker gets stuck; dark screen | waypoints like the Chapter 1 porters; shadowless headlamp; bot run 3 |
| Cultural mistakes (sang, sur, names of the dead, greetings, Sherpa lines, filming the dead) | medium | disrespect to a living culture | every item in §12.3 checked by a Sherpa reader before release; unreadable script on mani stones |
| Medical mistakes (HACE signs, dexamethasone dose, descent) | low-medium | wrong lessons | check against the Wilderness Medical Society guidelines [To verify] |
| The video beat feels exploitative | medium | the director or players dislike it | offered as a choice, filming shown as shameful; alternative endings in Q3 |
| The parallel Chapter 3 draft starts at Base Camp on Day 5 (12-chapter list), not at Lobuche on Day 3 (v1) | certain | Days 3-4 are designed by nobody yet | Q1; whoever designs Days 3-4 uses the Lobuche anchor in §4.1 and this chapter's flags |
| No AMS system for the player yet | certain | Elena's sickness is scripted | fine for this chapter; v1 §4.1 later |
| Real Lobuche coordinates are from memory | low | small offset | DEM height and the satellite overlay agree; check on a map |

### 12.2 Questions for the director

| # | Question | Default if no answer |
|---|---|---|
| Q1 | Numbering: Chapter 2 = Thukla → Lobuche (this, from v1), or "The Puja" from the 12-chapter list (which `Chapter3_Design.md` follows)? If the 12-chapter list: this becomes part 1 of "The Puja" and Days 3-4 still need a design | the 12-chapter list, with this as part 1 |
| Q2 | Title: "The Hidden Mountain"? Others: "Sur", "The Stranger", "Day Two" | The Hidden Mountain |
| Q3 | The end: (A) Ji-woo's video (this document), (B) Nima: *"The report says he fell. I signed it."*, (C) something gentler | A |
| Q4 | Lobuche at its real place with a terrain spur (recommended), or inside the existing corridor (on the glacier)? | real place |
| Q5 | The trekker: name and nationality; can "leave him with Diki" end in his death? | "Jonas", German; yes, by chance |
| Q6 | Sherpa-language lines: in Sherpa (who speaks it?), in Nepali, or English only? | Nepali with *[in Sherpa]* subtitles until a speaker is found |
| Q7 | Meshy credits for Chapter 2: minimal (≈ 181), recommended (≈ 321) or full (≈ 387)? | wait |
| Q8 | Static yaks at Lobuche, or no yaks? | static |
| Q9 | Sangye sleeps on the dining-room bench (real for staff) or in a tent by the lodge? | bench |
| Q10 | Should the hike's cloud gap be chance (as designed) or always shown once? | chance |

### 12.3 Cultural and factual checks before the script is final

- **Sang** (juniper offering): who burns it, where (memorial stone, lhapso altar, household burner), the morning
  timing, and Nima's words *"so she lets us pass"*.
- **Sur** (burnt tsampa offering): its use among Sherpas, the time of day, who it is for (the dead in their 49 days,
  wandering hungry spirits), and Diki's words.
- **Saying the name of the dead** a year after death (B11, the video).
- How a Sherpa family feels about **photos and videos of their dead**, and bodies left on the mountain.
- Walking **clockwise** around chortens and mani stones; the mantra *Om mani padme hum*.
- Greetings: *Tashi delek*, *dai*, *Nima-dai*, holding hands in both hands; *thank you* in Sherpa (spelling of
  *thuk-je-che*).
- Lodge life: staff sleeping in the dining room, when the dung stove is lit, boiled-water prices, *"Dal bhat power,
  twenty-four hour"*, Callbreak.
- Diki's dress: the maroon wrap dress and the striped apron of married women.
- Medicine: HACE signs, the heel-to-toe test, dexamethasone 8 mg, lodge oxygen, helicopters at first light only.
- Facts: Lobuche's position and height; Pheriche's height; spring weather at Lobuche (afternoon snow, night
  temperatures); whether Everest can be glimpsed from anywhere on this stretch (our DEM says no).

---

## Appendix: method and sources

| What | How |
|---|---|
| Route distances and heights | `Tools/RoutePlanning/route.json` leg 0 (369 points, `[x, y, z_cm, slope]`), heights from z by the formula in §4.1 (Dingboche check: 4,335 m = the measured camp ground) |
| Real-place pixels | lat/lon → pixel with 2,912.4 px per degree of longitude and 3,277.1 px per degree of latitude around 28.008 N, 86.921 E; checked: Dingboche 243.5 / 883 vs 241.7 / 881.5, Gorak Shep 236.3 / 594.3 vs 235.8 / 594.1 |
| Heights off the route | the 16-bit DEM `28_008_86_921_12_1009_1009_16bit.png` (33.742 m/px), bilinear |
| Lobuche's place in the valley | the satellite image that came with the DEM (`..._albedo_imagery.png`), with leg 0 and the story path drawn on it |
| Line of sight | DEM samples every 11 m, eye 1.7 m, earth curvature with refraction (0.87) |
| Chapter 1 places | `Content/Story/Chapter1_places.ygs` (traced by `Tools/Story/build_chapter1_world.py`) |
| Story keys and Chapter 1 flags | `Tools/Story/chapter1_story.py` |
| Camera vocabulary | `Docs/Development/Chapter1_Camera_Spec.md` §2, §4.3-4.4, §6, §7 |
| Chapter list and arc | `Docs/Development/Chapter_Design_v1.md` §1.5, §5 (Chapters 2 and 3); `Chomolungma_Chapter1.md`; `Chapter1_v2_Design.md` |
| Physiology values | `Source/YggdrasilsPeak/Survival/SurvivalConfig.h` (acclimatised resting SpO₂, adaptation rates) |
