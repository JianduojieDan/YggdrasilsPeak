# Chomolungma · Chapter 7 "Thick Air" · Design v1

Version 1.0 · 2026-11-20 · written by Claude (designer and technical lead) for the director (the author).
It turns the half page "Chapter 7 · Thick Air" in `Chapter_Design_v1.md` (section 5) into a design that can be built,
in the format of `Chapter1_v2_Design.md`. Nothing here is built yet. Nothing here is approved until the director says so.

> **Read this first: which "Chapter 7"?**
> Two chapter lists exist. This document follows the task it was given: **Chapter 7 of `Chapter_Design_v1.md`
> (17 chapters) = "Thick Air"**, the rest days at Base Camp between rotation 1 and rotation 2.
> In the newer 12-chapter list (`Chomolungma_Chapter1.md`, Part 1) the same place in the story is **Chapter 5 "Rest
> Days"** ("clients quarrel, the radio crackles with other teams, and Sangye finds his father's last diary pages in
> the expedition's store tent"). The two are the same slot in the arc, so this design covers both: it keeps every
> hand-off of v1's Chapter 7 and it contains the diary pages of the 12-list.
> In the 12-list, "Chapter 7" is a different chapter ("The Lhotse Face"). **If that one was meant, this document is the
> wrong chapter and only its systems sections can be reused.** See open question Q1.

**How to read this.** Sections 1-3 are the chapter. Sections 4-9 are how it is made. Sections 10-11 are how it is
tested and what needs a decision.

**Labels**

| Label | Meaning |
|---|---|
| **[Built]** | exists in the project today |
| **[To build]** | planned here, not started |
| **[Proposed]** | a new idea in this document; the director decides |
| **[From v1]** | taken from `Chapter_Design_v1.md` without change |
| **[Measured]** | read from `Tools/RoutePlanning/route.json` or the terrain georeference today |
| **[To measure]** | a position the world builder must trace on the real ground before it is used (rule of 2026-11-02: never invent a position or a height) |
| **[VO]** | a spoken line; the director records the voices |
| **[Check]** | culturally or factually uncertain; must be checked with a Sherpa or Tibetan reader, or a good source, before release |
| **[Open]** | the director decides (section 11) |

**F / S / C** on a beat: **F** fixed (always happens), **S** systemic (comes out of the game's systems and the
forecast), **C** chosen (only if the player chooses it).

---

## 1. One-page summary

### 1.1 What the chapter is

Sangye is back at **Everest Base Camp (5,274 m)** after his first night above 6,000 m. For three days the expedition
rests. His body heals in the "thick" air, the clients quarrel, and the mountain does not rest at all: one night the
Icefall collapses where the team would have stood an hour later, one night a storm tries to take the mess tent, and
one noon an avalanche cloud rolls over the camp. In the store tent, in a barrel with his father's name on it, Sangye
finds his father's notebook from last season. The chapter ends with the forecast: the team goes back up at three in
the morning.

There is no route walking in this chapter. It is the game's breath between two Icefall crossings.

### 1.2 Purpose

1. **Let the player recover, and let him see the recovery in numbers.** Three morning oximeter checks: the resting
   SpO₂ climbs and the resting pulse falls. This is the physiology model (the thesis core) shown as a small daily
   ritual, and it is the only chapter where the numbers get *better*.
2. **Change the Icefall off screen.** Chapter 8 needs a collapsed section and a new double ladder. The player must
   hear it fall and, if he chooses, help rebuild it.
3. **Move the father's story one step.** The notebook: the father also saw the gold light, and Nima did not.
4. **Make Base Camp a lived place** (decision D4, "not alone for the whole climb"): cooks, clients, the doctor, the
   Icefall doctors, Nima's family on the phone. Company level: **crowd**.
5. **Load the next chapter.** The load plan, the forecast, the alarm at 02:30.

### 1.3 The emotional line

**A held breath.** The art direction asks for quiet awe; the director's rule is tension over poetry; the sound rule
asks for a sense of fate. A rest chapter can easily become only "poetry". So the rule of this chapter is:

> **The body rests. The mountain does not. Every quiet scene is cut short by the mountain.**

| Step | The player should feel | How |
|---|---|---|
| 1 | relief | waking warm, sun through the tent, the number on the finger is better than yesterday |
| 2 | belonging, and its limits | serving tea in the mess tent; the clients argue over his head |
| 3 | fate, the first time | the Icefall falls at 01:40. "At three, eight of ours would have been under it." |
| 4 | usefulness, or guilt | help the Icefall doctors, or sleep; both cost something |
| 5 | fear without a monster | the storm night: holding a tent pole with the cooks while the sky fights |
| 6 | awe | the avalanche cloud at noon, then ice dust falling in sunlight, in total silence |
| 7 | **the peak: fate, the second time** | the father's notebook: he saw the gold too |
| 8 | **the end: dread and will** | the forecast, the alarm, the ice creaking under the tent floor |

**Quiet awe** lives in the pauses (the dust in the sun, the camp at blue hour with forty lit tents).
**Tension** comes from three decisions that each cost recovery, trust or knowledge, and from the fact that the player
never knows which quiet scene will be interrupted.
**Fate** comes from near misses by the clock (the collapse), from the forecast naming the day, and from the notebook:
the son is seeing what the father saw.

### 1.4 The shape

Three rest days, each with one fixed scene, one event from the mountain, and one choice.

| Part | Minutes | Tension (1-5) | Company |
|---|---|---|---|
| Day 7 · morning: waking, the doctor, the mess tent | 0-9 | 1 → 2 | crowd |
| Day 7 · the choice: stay, or go down to Dingboche | 9-10 | 2 | Nima |
| Night 7 · the Icefall collapses | 10-12 | 4 | alone in the tent, then the whole camp |
| Day 8 · the Icefall doctors (help or rest) | 12-18 | 2-3 | the doctors, or alone |
| Day 8 · dusk: Nima's call home | 18-20 | 1 | Nima |
| Night 8 · the storm in the mess tent | 20-24 | 4 | the cooks |
| Day 9 · noon: the avalanche cloud | 24-26 | 4 → 1 | crowd |
| Day 9 · the store tent: loads, and **the notebook (the peak)** | 26-30 | 3 | Nima |
| Day 9 · evening: the forecast, the tent, **the end** | 30-33 | 3 | crowd, then alone |

### 1.5 Length

**About 28-33 minutes** on a first play. v1 budgets 25 minutes; the notebook scene from the 12-list adds about 4.
If the director wants 25, cut in this order: the doctor's log (beat `c7_log`, optional anyway), the flag line after
the avalanche cloud (`c7_flags`), then shorten the mess-tent quarrel to one exchange. The Dingboche branch (section
3.3) is about the same length.

### 1.6 Hand-offs (kept exactly as `Chapter_Design_v1.md` describes them)

| | From v1 | What this chapter does with it |
|---|---|---|
| **In, from Chapter 6** | Chapter 6 ends at Base Camp at breakfast, game day 6, 07:30, on the question *"Three days to rest. A dark realm is coming."* The 19:00 radio call at Camp 1 forecast "a dark realm in three days" | The chapter opens on the next waking, day 7. The dark realm arrives on night 8 (two nights later, inside the "three days") |
| **In, flags** | Chapter 6's decisions: the radio check answered honestly or with a lie; the painkiller; the time of the descent; the answer about the other team's sick client | Read as flags (names to be aligned with `Chapter6_Design.md` when it exists; working names in section 6.4) |
| **Out, to Chapter 8** | Chapter 8 starts game day 10, 03:00, at the Icefall entrance, with a heavier load. "A section near the Popcorn field collapsed and the Icefall doctors re-routed it with a tied double ladder... If the player helped the Icefall doctors in Chapter 7, they know the new line and the double ladder has a guide rope." | The collapse is heard on night 7. Helping sets `c7_helped_doctors`. The chapter ends on day 9 with the alarm set for 02:30 |
| **Out, the question** | *"Rotation 2: Camp 2, and a look at the Lhotse Face."* | The last radio line and the end card |
| **Out, state** | the body recovered; adapted altitude; camp stocks after the load plan; trust with Nima and Dr. Hale | Section 6 |

### 1.7 What changed from v1's half page, and why [Proposed]

v1 was written before the game became *Chomolungma* (2026-11-14). The story layer is re-rooted here; the structure and
the three decisions are unchanged.

| v1 | This design | Why |
|---|---|---|
| "the parents" | the father | the Chomolungma story |
| Dr. Hale's tent: the Eddas; she sees the palm mark; her certainty cracks | Dr. Hale's tent: last season's medical log. The father's last radio reading from 8,400 m is a number her science says is impossible. Her certainty cracks | The Norse layer is gone. The crack now comes from the physiology numbers the player has learned to read |
| "a realm storm" | the storm is the existing storm realm in code, named in the story as the Demigods' wind [Open, Check] | the Six Realms |
| going down to Dingboche "costs two extra days, and the window may pass" | going down fits inside the three days; it costs the Base Camp scenes, the chance to help the doctors, and some of Nima's trust | Chapter 8 must still start on day 10 (the hand-off is kept exact). Open question Q4 keeps v1's version available |
| the player chooses how many rest days | the days are fixed at three; Marek sets the schedule; the player chooses how Sangye uses them | Sangye is now an employee, a junior climbing Sherpa, not the expedition's owner |
| (not in v1) | the father's notebook in the store tent | from the 12-list's "Rest Days" |
| (not in v1) | Sangye works on rest days: tea, loads | rest days are for clients. This is the Sherpa view the game is about [Check] |

---

## 2. Design rules for this chapter

The five pillars of Chapter 1 v2 still hold (eyes on a body; teach in the moment, by a person; spaces and objects tell
the story; real place and real people, respectfully; tension over poetry). Rules special to a rest chapter:

| # | Rule | What it rules out |
|---|---|---|
| R1 | **No stretch longer than 6 minutes without a decision, an event or a discovery** (the project's pacing rule). Here the longest planned gap is about 4 minutes | "walk around camp until something happens" |
| R2 | **Sound before sight, always.** Every event of the mountain is heard 1-3 seconds before it is seen (real distances: the collapse is 1.3 km away, about 4 s of sound travel) | sudden visual scares |
| R3 | **Days pass behind closed eyes.** Each night is a blink (sleep, wake). No calendar screen with buttons | a "skip day" menu |
| R4 | **The numbers get better, and the player sees why.** One oximeter reading each morning | hidden recovery |
| R5 | **Nobody of the cast dies here** [From v1]. The mountain shows its hand and takes nothing, yet | cheap shock |
| R6 | **Sangye is staff.** He pours the tea; he does not sit at the clients' table unless asked | a hero's welcome |
| R7 | **The story never turns the head.** Only the view rig moves the view (camera spec rule R1). People, sound and light pull the gaze | camera grabs |

---

## 3. The beat list

### 3.1 How to read the tables

- **id**: the beat's name in `Content/Story/Chapter7.ygs` (written by a new `Tools/Story/chapter7_story.py`).
- **Trigger**: the story machine's `when` (start / after + delay / near a place / look / flag / posture).
- **Camera**: only the vocabulary of `Chapter1_Camera_Spec.md` section 7: `pose`, `hold` (a Move lease), `blink`,
  `nudge`, `reflex`, `lure`, `inspect`, `holdkey`, `knock`, `pace`, `caption`. A **lease** is something the story
  holds for a limited time and must give back; a **nudge** is a slow, weak turn of the view that gives way to the
  mouse; a **posture** is a real body state (lie, sit, kneel, crawl, stand).
- Minutes are targets for the **Base Camp branch** (the default). The Dingboche branch is in 3.3.
- Every line is a first draft for the director's ear. Sherpa and Nepali words are marked [Check].

### 3.2 The beats, Base Camp branch

#### Day 7 · "The number on the finger"

| id | Trigger | The player does | Sees and hears | Lines | Camera | |
|---|---|---|---|---|---|---|
| `c7_wake` | start (after Chapter 6's end blink) | lies in his sleeping bag; the first key press sits him up | 07:10. Sun through yellow tent fabric, frost melting and dripping from the inner wall. Under the floor the glacier ticks and creaks. Far off, a cook sings. Caption: **Day 7 · Base Camp, 5,274 m**. The oximeter on his finger: **SpO₂ 81, pulse 94** (numbers come from the model, section 6.1) | none. A small caption once: *"[Q] Oximeter"* only if the player does not look at his hand in 10 s | `blink wake`; `pose lie` → sit on input (as Chapter 1's dawn). No hold | F |
| `c7_crawl_out` | posture sit + move input | crawls out, stands | standing up is the reveal [as in Chapter 1]: the whole camp on the grey moraine, forty tents of other teams along the glacier, the Icefall as a white frozen river above, flags. Breath fog. Nima sits on a barrel, sorting carabiners | **[VO] Nima:** *"You slept eleven hours. Good. That is the work today: sleeping."* then, dry: *"After the tea."* | `pose crawl` → stand, played. Staging: Nima sits where the standing view lands, 25° right | F |
| `c7_hale` | near `hale_tent` | walks in, sits on the cot; holds out his hand | The medical tent: a folding cot, an oxygen cylinder, a laptop, a small heater. **Dr. Hale in person for the first time**: 50s, Norwegian, fleece and reading glasses. She clips her own oximeter on him, listens to his chest | **[VO] Hale:** *"Sit. Finger."* ... *"Eighty-one. Yesterday at Camp 1 you told me seventy-four."* **If `c6_lied_spo2`:** she looks at him over her glasses: *"My instrument and your voice do not agree. Up there I only have your voice. Remember that."* **Else:** *"Good. You tell me the truth, I keep you alive. That is the whole contract."* Then: *"Three days. Eat, drink four litres, sleep. Your blood is building. Let it."* | `pose sit` at the cot, `hold` (Move lease) only while the oximeter is on (max 40 s, auto release); `npc_mode present` for Hale; no nudge | F |
| `c7_mess` | near `mess_door`, after `c7_hale` | takes the big kettle from the cook; walks along the table and pours ([E] at each of three cups) | The mess dome, warm and loud. Marek, Elena, Ji-woo at the table; Ji-woo's camera on its tripod. The cook, **Pasang**, hands him the kettle without a word. Elena has a dry cough | **[VO] Ji-woo:** *"Three days? I feel fine. We lose the weather sitting here."* **[VO] Marek:** *"You feel fine because you are sitting."* **[VO] Elena:** (coughs) *"Let him be young, Marek."* **[VO] Marek**, to Sangye, not unkind: *"You were at Camp 1. Tell him how the night was."* | staging: all three inside a 60° arc from the door. `press` prompts at the cups. No hold | F |
| `c7_mess_choice` | after Marek's question | answers while he pours (a choice waits 8 s; silence is an answer) | **Choice A:** *"I woke up drowning. Every hour."* (honest) · **B:** *"It was fine."* · **C:** say nothing, pour the tea | **A →** Ji-woo is quiet; **[VO] Elena:** *"Thank you. Nobody says it."* (flag `c7_honest_mess`; Marek's goodwill +) **B →** **[VO] Ji-woo:** *"See?"* Marek looks at Sangye a moment too long (Nima's trust −, small) **C →** **[VO] Pasang**, low, taking the kettle back: *"Good. Not our table."* [Check] | choice on screen while moving | C |
| `c7_plan` | near `stores_front`, after `c7_mess` | talks with Nima at the load pile; makes **decision 1** | Nima ties a load. Behind him the valley drops away south, layer after layer of blue ridges: down there is home | **[VO] Nima:** *"Three days. Marek does not need us until the loads go. Your aunt's kitchen is one long day down. Thick air. You would sleep like a child."* (beat) *"Or stay. There is always work here. And the ice is talking."* **Choice: Stay at Base Camp · Go down to Dingboche.** If he stays: **[VO] Nima:** *"Good. Then carry this."* | walk-and-talk; `follow talk_side` for Nima | C |
| `c7_camp_free` | after `c7_plan` (stay) | free time, about 2 minutes of light: may walk the camp | Small things to find (each a caption, no card): the puja altar with its flags and the blessed axes; another team's radio playing Nepali pop; a row of oxygen bottles being checked, each hiss a different note; the Icefall doctors' tent with coiled rope and ladder sections outside. The light goes: `timeto` 18:30 over 90 s. The camp at blue hour: forty tents glowing from inside | (barks only) **[VO] Nima:** *"Go to bed. You are no use to me tired."* at 18:30 | none | S |
| `c7_collapse` | after `c7_camp_free` + sleep blink; game time 01:40 | wakes lying in the dark; sits up; may crawl out | **Sound first:** a crack like a rifle, then a roar that does not stop, 9 seconds long, from the Icefall (1.3 km away: the sound comes about 4 s after the first flash of sliding ice, but it is night, so there is only sound). The tent floor shakes. Then silence. Then, one by one, headlamps come on across the whole camp. Outside: a grey dust cloud standing over the middle of the Icefall in the moonlight. A radio on a barrel crackles | **[VO] radio, an Icefall doctor (Nepali accent):** *"All teams, all teams. The Popcorn has gone. Count your people. Count your people now."* **[VO] Marek**, shouting from his tent: *"Kovač, all in camp!"* **[VO] Nima**, beside Sangye, looking up, very quiet: *"At three, eight of ours would have been under it."* (beat) *"Go back to sleep, if you can."* | `blink wake`, `pose lie`; reflex is off while lying. Outside: `lure` at `icefall_dust` (the dust cloud), then **nudge 1** as the fallback (max 30° yaw, 8° pitch) | F |

#### Day 8 · "The ice doctors"

| id | Trigger | The player does | Sees and hears | Lines | Camera | |
|---|---|---|---|---|---|---|
| `c7_wake2` | sleep blink; 07:00 | sits up, reads the oximeter | **SpO₂ 83, pulse 88.** Caption: **Day 8**. A raven on the tent ridge shadow | none | `blink wake`, `pose lie` | F |
| `c7_doctors_ask` | near `doctors_tent` or after 60 s outside | listens; makes **decision 3** | Two Icefall doctors (older Sherpas, faded suits, faces burnt dark) lash ladder sections together. One, **Ang Dorje** [Check: name], counts rope coils | **[VO] Ang Dorje:** *"You are Tashi's son."* [Open: the father's name] (beat) *"He carried for us, when he was your age. We need backs today. Ladders to the foot of the ice. You don't climb. You carry."* **[VO] Nima:** *"It is your rest day. Nobody will think less."* **Choice: Carry for them · Rest.** | staging; no hold | C |
| `c7_carry` | choice *Carry* | carries a ladder bundle (18 kg, the walk slows) from the camp to the doctors' cache, **450 m up the route** toward the Icefall; then helps lash a double ladder: [hold F] to pull the rope tight, three times | The moraine path, rough. The Icefall grows until it fills the sky. At the cache: the doctors lay two ladders end to end, overlap them by two rungs, and lash them. **Ang Dorje points with his whole arm:** the old line (a grey scar, a ladder hanging broken in the hole) and the new line he will fix: left of the scar, over one long crevasse | **[VO] Ang Dorje:** *"Two ladders, one rope around the join. It will bend in the middle. It always bends. You do not stop on the bend."* (pointing) *"The old way is gone. We go left of the grey. One long step over the big one. I will put a hand line on it. For you."* **[VO] Sangye** may ask: *"Does it frighten you?"* → *"Every morning. That is how I am old."* | `pace` lease 1.1 m/s with the bundle (keep); `holdkey` F ×3 (the hand-hold input returns); look trigger on `new_line` with `companion` = Ang Dorje (he points), `lure` = ice ticking, **nudge 2** as fallback | C |
| `c7_rest` | choice *Rest* | goes back to the tent; sleeps through the morning (blink); wakes at 13:00 | The day from inside the tent: the sun's patch moving across the fabric, voices passing. In the afternoon he sees the doctors come back, small and slow, on the moraine | **[VO] Nima**, later: *"They put a new ladder over the big one. I don't know where. We will see it by headlamp."* | `blink sleep` / `wake`; `pose lie` | C |
| `c7_call` | 17:30, near `phone_rock` (Nima walks there; the player follows or is called) | stands near Nima on the one rock where the phone has a signal; may make a call himself | Evening light, warm only on the high ridges. Nima stands on a boulder, phone held high, then to his ear. From the small speaker, a child's voice, too fast, in Sherpa. He laughs, a sound the player has not heard from him. Clouds are building over the Lho La, dark and fast | **[VO] Nima** (in Sherpa, subtitled only in part): *"...yes. Yes, I am eating. ...No. Not this year. Tell her I said so."* He hangs up and stands a moment. **[VO] Nima:** *"She wants a pink bicycle. In Pangboche. There is not one flat metre in Pangboche."* Then he holds the phone out: *"Your aunt?"* **Choice: Call her (tell the truth) · Call her ("It is easy, aunt") · Not today.** If called: **[VO] Pema** (speaker): *"You are thin. I can hear it."* / **[VO] Dolma:** *"Did you see him? Did you see uncle?"* The line cuts in a gust | staged at 6-8 m so he overhears; no hold; the storm cloud is found by light and sound, not by a nudge | F + C |
| `c7_storm` | 21:30, after `c7_call` + supper blink; the storm realm starts (forced by the story, as forecast in Chapter 6) | in the mess tent with the cooks: [hold F] on a pole through three gust waves; between the second and third, **a choice** | The tent walls slam in and out. The gas lamp swings; shadows jump. Lightning lights the fabric from outside; thunder 3-5 s later [Built: storm strikes with delayed thunder]. Pots fall. Pasang and the kitchen boy hold the other poles. On the windward side one guy line has pulled out: the wall there bulges in | **[VO] Pasang:** *"Pole! Take the pole!"* ... (between gusts, grinning, frightened) *"The lha-ma-yin are fighting again. Let them. Hold."* [Check] **Choice (8 s): Run out and re-tie the line · Keep holding.** **Run:** 25 s outside in the storm: knee-deep spindrift, the line whipping, [hold F] to tie; body temperature drops, hands shake; he comes back white with snow; the tent holds (flag `c7_tent_saved`). **Hold:** the wall tears; snow pours in; they save the stove and the food but the dome is torn (flag `c7_tent_torn`) | `holdkey` F (max 8 s each, ×3); between waves the hands shake (Hands lease). No view lock. Proposed new rig input `buffet` (section 5) | S + C |
| `c7_storm_end` | storm realm ends (about 4 min of play; the rest of the realm passes in the sleep blink) | sits on a barrel; Pasang pushes a cup into his hands | Silence. Snow hissing on the fabric. The stove's blue flame. Everyone breathing | **[VO] Pasang:** *"Tea."* **If `c7_tent_saved`:** *"You tie like your father. Too many turns."* **If torn:** *"Tomorrow we sew. Tonight we drink."* | `pose sit`; no hold | F |

#### Day 9 · "The notebook"

| id | Trigger | The player does | Sees and hears | Lines | Camera | |
|---|---|---|---|---|---|---|
| `c7_wake3` | sleep blink; 08:00 | sits up, reads the oximeter; crawls out | **SpO₂ 85, pulse 82.** Caption: **Day 9**. Outside: 20 cm of new snow on everything, a hard blue sky, every sound soft. Cooks sewing the dome, or shaking snow from it | **[VO] Nima:** *"Eighty-five? Now you are a Sherpa again."* | `blink wake`, `pose lie` → crawl → stand | F |
| `c7_cloud` | 11:40, the player outside and more than 15 m from any tent door (else it waits, max 3 min, then fires anyway) | has **about 20 seconds**: run into a tent, or crouch behind the altar or a big boulder, or stand and watch | **Sound first:** a deep crack high on the Nuptse wall to the south-east, then a long tearing roar. A white line falls down the dark wall, hits the glacier and becomes a cloud that grows toward the camp, silent now, taller than the tents by ten times. Then the wind hits first, then white: 15 seconds of nothing but white and roar. Then it is past. **Then the awe:** ice dust hangs in the air and falls slowly in full sunlight, glittering, in complete silence. Every tent, every person, every prayer flag is white | **[VO] someone, far:** *"Avalanche! Inside! Inside!"* **[VO] Nima**, close: *"Down! Behind the stones!"* After: people stand up one by one, laughing the way people laugh when they are not dead. **[VO] Ji-woo:** *"Tell me you got that. Tell me somebody got that."* **[VO] Elena**, coughing, to Sangye: *"Does it do that often?"* **[VO] Sangye** (auto): *"My father said it only does it to say hello."* | `reflex` 10° toward `nuptse_face` at the crack (the player finishes the turn). **Unsheltered:** `knock` (to one knee, Knock lease ≤ 2 s), view whites out by fog, not by a fade. **Sheltered:** nothing is taken from the player. No nudge | S |
| `c7_flags` | after `c7_cloud`, near `altar` | may re-string the flag line the blast tore from the altar pole: [hold F] | The five-colour line lies across the snow; Pasang is already lifting one end, keeping the flags off the ground | **[VO] Pasang:** *"Not on the ground. Lift."* [Check] **[VO] Nima:** *"Before we climb, it goes up again."* | `holdkey` F 3 s | C |
| `c7_stores` | 14:00, near `stores_tent` (Nima calls him) | **decision 2**, the load plan: with Nima, picks which **two of four** loads go up first with the Sherpa team: **oxygen for Camp 3** · **gas and food for Camp 2** · **a spare tent and rope for Camp 2** · **the medical kit and a sleeping-oxygen bottle for Camp 2** | The store tent: dim, blue light through the fabric, rows of barrels, oxygen bottles in a rack, labels in marker pen. A plan board on an easel with the rotation drawn in Marek's hand | **[VO] Nima:** *"Marek chooses the days. We choose the backs. Twelve loads, eight men. Two go first. Which?"* After the choice he reacts with one line each, for example **oxygen:** *"For the clients' lungs. Good for business."* **gas and food:** *"A stove that burns is worth two bottles."* | the choice is made on the plan board ([E]); no hold | C |
| `c7_barrel` | after `c7_stores`; look trigger on `father_barrel` (lure: a barrel lid ticking in the wind; companion: none, Nima has his back turned) | finds a blue barrel at the back with old tape on the lid; [E] opens it; kneels | Tape, sun-faded, in marker: the expedition's name, last year's date, and a name: **TASHI** [Open: the father's name]. Inside: a regulator, mitts stiff with old sweat, a thermos, and at the bottom a school notebook in a zip bag | none | `pose kneel1` at the barrel; `press` Interact | F |
| `c7_notebook` | [E] on the notebook | holds it and turns the pages with the mouse: **three pages**, the last one torn out | His father's handwriting [Check: script and language, section 11.3]; the subtitle gives the meaning. **Page 1:** load lists, a child's drawing of a yak pressed between the pages (Dolma's). **Page 2:** *"The plume was gold again this evening. I asked Nima. He saw wind. I did not ask twice."* **Page 3:** *"Sangye wants to come next year. He is strong and he is not ready. A strong boy listens to his legs. On this mountain you must listen to something else."* Then the stub of a torn page, and on the inside cover, pressed hard: *"If she calls, I will not say no."* | **[VO] Father** (reading voice, low, close): the three quoted lines above. Then silence. The heartbeat rises (the one `fovbeat` of the chapter) | `inspect` (Inspect lease; blocks until closed; the mouse turns the pages, as the photos in Chapter 1). `fovbeat` once | F, **the peak** |
| `c7_torn` | notebook closed; Nima has turned round | may ask about the torn page | Nima stands in the door, a black shape against the white outside. He looks at the notebook, not at Sangye | **Choice: "A page is gone." · "He saw the gold. So did I." · Say nothing, put it in the jacket.** **Page →** **[VO] Nima:** *"Some pages are not for reading at night."* **Gold →** a long pause. **[VO] Nima:** *"Then do not tell Marek. And do not tell the doctor."* (flag `c7_told_nima_gold`) **Silence →** **[VO] Nima:** *"Keep it dry."* In all three he goes out first | no hold; Nima `npc_mode present` at 3 m, back-lit | C |
| `c7_log` | optional: near `hale_tent` between `c7_torn` and 18:30, only if Dr. Hale's trust is not low | sits; she opens last season's log on the laptop | A table of numbers. One row is marked in yellow. Time 02:10, place "Balcony 8,400", **SpO₂ 94**, no oxygen | **[VO] Hale:** *"You have his eyes. I should have said so on the first day."* (beat) *"His last radio check. Ninety-four, at the Balcony, without a mask. You know what your finger read at Camp 1. At 8,400 that number does not exist. I wrote 'instrument fault'."* (beat) *"I have used that instrument for nine years."* **Choice: "What do you think it was?" → "I think I do not know. I do not like not knowing."** · **Say nothing.** | `pose sit`; `inspect` of the laptop screen (no back side) | C |
| `c7_forecast` | 19:00, radio call in the comms tent (or over the handset wherever he is) | listens | The whole team around the radio table; Hale reads from a printout; outside, the sky is green and clear and very cold | **[VO] Hale:** *"Summit wind falling for three days. Tonight minus nineteen here. The Icefall doctors say the new line is open: one double ladder at the Popcorn. No traffic before four."* **[VO] Marek:** *"Then we go at three. Rotation two: Camp 2, and a look at the Lhotse Face. Breakfast at two. Sleep."* **[VO] Ji-woo:** *"Finally."* **[VO] Elena** coughs, and says nothing | staging; no hold | F |
| `c7_end` | in his tent, posture lie, after `c7_forecast` | sets the alarm (02:30, fixed); reads the journal page; lies down | Headlamp on the notebook beside his own journal. The journal page writes itself from his choices (section 6.5). He turns the light off. Dark. The glacier creaks under his back; far up, once, the Icefall cracks | **[VO] Nima**, through the tent wall: *"Sangye. Sleep."* (beat) *"The ice does not."* Black. Title: **CHOMOLUNGMA · Chapter 7 · Thick Air**. Under it: *Rotation 2: Camp 2, and a look at the Lhotse Face.* | `pose lie`, `hold "end"` (legal only in the last beat); `blink sleep` | F, **the end** |

### 3.3 The Dingboche branch (decision 1 = "Go down") [From v1, adapted]

Rule 2.7 of v1 allows a stretch already walked, below Base Camp, to be travelled in a short condensed scene. So the
14.6 km down and up are **not** walked again.

| id | Replaces | What happens | Lines | Camera |
|---|---|---|---|---|
| `c7_down` | `c7_camp_free` | **The descent in four eyelid cuts**, about 90 s: (1) Gorak Shep's lodges in afternoon light; (2) the Thukla memorials: he passes his father's stone, **the khata he tied in Chapter 1 is still there**, stiff with frost; (3) the first grass, the first juniper; (4) the Dingboche field walls at dusk. With each cut the breath is quieter and the oximeter caption reads higher: 84 · 87 · 89 · 91 | none; only breath, boots, and at the stone the wind in the flags | 4 × `blink` with `teleport` inside (`during`), each to a traced place facing along the trail; he walks 10-15 s in each |
| `c7_kitchen` | `c7_collapse` | **Aunt Pema's kitchen** [Built: the house assets from Chapter 1 v1, kept]. Stove, brass, the altar shelf with a butter lamp. Dolma will not let go of his sleeve. He eats; he falls asleep sitting up | **[VO] Pema:** *"You came down. You promised to come down. This does not count."* (she almost smiles) **[VO] Dolma:** *"Did you see him?"* **Choice: "Not yet." · "I saw where he went." · say nothing** | `pose sit` on the bench; Dolma's hand: `holdkey` F (the Chapter 1 input) |
| `c7_home_day` | Day 8 | He wakes at noon: **SpO₂ 92, pulse 71.** The best numbers of the whole game. Pema lights the butter lamp for the father [Check]. The phone: Nima, from the phone rock | **[VO] Nima** (phone): *"The fall came down in the night. Nobody under it. And a storm is coming tonight. Stay low. Sleep. Come up tomorrow."* | free in the house and the yard |
| `c7_up` | Day 9 morning to 16:30 | **The climb back in three eyelid cuts.** The numbers fall again: 89 · 86 · 84. Base Camp at 16:30 under new snow: a torn mess dome, flags re-strung, everyone with a story he was not part of | **[VO] Pasang:** *"You missed the lha-ma-yin. They asked for you."* **[VO] Ji-woo** shows him the avalanche cloud on his camera's small screen | 3 × `blink` with `teleport` |
| then | | `c7_stores` → `c7_barrel` → `c7_notebook` → `c7_torn` → `c7_forecast` → `c7_end` exactly as in 3.2 (no `c7_log`: there is no time) | | |

**What the branch costs and gives** is in section 6.3. He cannot help the Icefall doctors, so Chapter 8's new line is
unknown and has no hand line.

### 3.4 Pacing check (rule: no gap over 6 minutes)

| From | To | Planned gap | OK? |
|---|---|---|---|
| start | Dr. Hale | 2 min | yes |
| Dr. Hale | mess tent choice | 3 min | yes |
| mess tent | decision 1 | 2 min | yes |
| decision 1 | the collapse | 2-3 min (free time; the light change is the clock) | yes |
| the collapse | decision 3 | 2 min | yes |
| decision 3 | Nima's call | 6 min (Carry: full of small steps) / 1.5 min (Rest) | yes |
| the call | the storm choice | 3 min | yes |
| the storm | the avalanche cloud | 3 min | yes |
| the cloud | the load plan | 2 min | yes |
| the load plan | the notebook | 1 min | yes |
| the notebook | the forecast | 2-4 min | yes |

The session monitor measures the real gaps (section 10).

### 3.5 Decisions and what they change

| # | Decision | Options | Cost now | Changes later |
|---|---|---|---|---|
| 1 [From v1] | Rest at Base Camp, or go down to Dingboche | stay / go down | going down: no Base Camp events, no help to the doctors, Nima's trust −, a long walk back (fatigue) | better recovery (section 6.3); the journal; the khata at the stone; Pema and Dolma remember in the ending |
| 2 [From v1] | The load plan | 2 of 4 loads go up first | none | the stocks at Camp 2 and Camp 3 in Chapters 9-10 (v1 4.4): oxygen, or gas and food, or a spare tent, or medicine |
| 3 [From v1] | Help the Icefall doctors | carry / rest | carry: that day's fatigue recovery is halved; 18 kg for 450 m at 5,290 m | `c7_helped_doctors`: in Chapter 8 the new line is known (safer) and the double ladder has a hand line; "the doctors owe you" |
| 4 [Proposed] | The storm: run out, or hold | run / hold | run: body temperature −1.0 to −1.5 °C, cold hands for 10 game hours | tent saved: hot breakfast, full recovery on night 8. Tent torn: cold breakfast, recovery ×0.8, the camp's food stock −1 day |
| small | the mess table · the call home · the torn page · the doctor's log | | none | trust (Nima, Dr. Hale, Marek), journal lines, what Nima and Dr. Hale say in later chapters |

No option is "obviously right" (pillar 2 of v1): resting properly is good for the body and bad for Chapter 8;
helping is the opposite.

---

## 4. The camera in this chapter

The view rig (`Survival/YggViewRig`) is the only writer of the view. The story asks; the rig decides.

### 4.1 Budget

| Item | Count | Where |
|---|---|---|
| Nudges (fallback only, after lure and companion) | **at most 2** | the dust over the Icefall (`c7_collapse`); the new line (`c7_carry`) |
| Reflexes (10° toward a sudden sound; the player finishes) | 2 | the avalanche crack (`c7_cloud`); the first thunder if he is outside (`c7_storm`) |
| Body takeovers | 1, only if unsheltered | `knock` in the avalanche blast |
| Jumps in place or time, all behind closed eyes | 6 sleeps and wakes (Base Camp branch); +7 travel cuts (Dingboche branch) | nights; the condensed walk |
| Heartbeat FOV push (`fovbeat`) | 1 | the notebook |
| Cinematic shots | 0 | |
| View locks (Look lease) | 0 | |

### 4.2 Postures used

| Posture | Where |
|---|---|
| Lie | every waking; the end |
| Crawl | out of the tent, three mornings |
| Sit | Dr. Hale's cot; the barrel after the storm; the aunt's bench |
| Kneel on one knee | the father's barrel; the avalanche blast (knock) |
| Standing | everything else |

### 4.3 Leases

| Lease | Beat | Max | Released by |
|---|---|---|---|
| Move (`hold`) | `c7_hale` while the oximeter is on | 40 s | the beat's end |
| Move (`hold "end"`) | `c7_end` | to the chapter end | chapter end (validator: only in the last beat) |
| Inspect | `c7_notebook`, `c7_log` | until closed (cap 300 s) | the player |
| Pace | `c7_carry` (1.1 m/s with the bundle, `keep`) | until the cache | `pace false` at the cache |
| Hands | the storm, between gusts (shaking) | 60 s | the beat's end |
| Knock | `c7_cloud`, unsheltered | 2 s | automatic |
| FovBeat | `c7_notebook` | 12 s | automatic |
| Lids | every sleep | 8 s closed | automatic |

### 4.4 Shot notes (key frames the world builder must check)

| Frame | Rule |
|---|---|
| Standing up outside the tent (`c7_crawl_out`) | the Icefall fills the upper third, 20-40° left of the tent's exit line; Nima 25° right at 5-6 m; at least six other-team tents in frame |
| The dust over the Icefall (`c7_collapse`) | from 3 m outside the tent door the dust place is within 45° of the exit line and 8-14° up, clear of tents |
| Nima on the phone rock (`c7_call`) | the player's stop is 6-8 m from him, with Nima against the sky and the Lho La cloud behind him |
| The avalanche (`c7_cloud`) | from the altar, the Nuptse face is in clear view; no tent closer than 4 m in that direction |
| The store tent (`c7_barrel`) | the father's barrel is the only one whose lid catches the light from the door |
| The notebook (`c7_notebook`) | held at reading distance; the page fills 60% of the height; the tape with the name stays readable in the lower corner |

### 4.5 One proposed addition to the rig [Proposed, To build]

`buffet`: wind pushing the body in the storm and in the avalanche blast, as a small body-explained motion (at most
1.5° at no more than 20°/s, irregular, never a steady roll, off under "Head motion: Off"). Without it the storm is
carried by the tent walls, the lamp and the sound only, which is acceptable. It must go through the rig, never
through the story.

---

## 5. Places

### 5.1 Coordinates

World coordinates are Unreal centimetres on `base_map`. Height = (z − 129,770) / 100 + 6,326 m. Latitude and longitude
come from the terrain georeference (heightmap centre 28.008° N, 86.921° E, 33.742 m per pixel); they are **our
terrain's** coordinates and can differ from a map by a few hundred metres.

| Place | World x, y, z (cm) | Height | Lat, Lon | Source |
|---|---|---|---|---|
| **Base Camp**, the camp landmark (end of leg 1) | 487,646 · 1,289,409 · 24,899 | 5,277 m (v1: ≈ 5,274; the label says 5,364) | 27.9997° N, 86.8489° E | [Measured] route.json |
| Start of leg 2 (the way to the Icefall) | 487,014 · 1,289,409 · 24,885 | 5,277 m | 27.9997° N, 86.8488° E | [Measured] |
| **The doctors' cache** (leg 2 at 450 m) | 531,117 · 1,286,357 · 26,689 | 5,295 m | 27.9999° N, 86.8533° E | [Measured] on the route line; the flat spot beside it [To measure] |
| **The Popcorn field** (leg 2 at 56%, the serac zone that collapses) | 617,057 · 1,275,409 · 30,752 | 5,336 m | 28.0009° N, 86.8620° E | [Measured]; 1.3 km from camp |
| The Icefall camp (end of leg 2) | 657,014 · 1,343,409 · 67,598 | 5,704 m | 27.9948° N, 86.8661° E | [Measured]; seen, not visited |
| Nuptse summit (the direction of the avalanche) | 886,041 · 1,671,156 · (off the route) | 7,861 m in reality | 27.9653° N, 86.8894° E | computed from real coordinates; 5.5 km from camp, to the south-east. **The face that releases** must be the nearest slope steeper than 45° in that direction on our terrain [To measure] |
| Lho La (where the storm cloud builds) | 744,512 · 1,143,273 | about 6,000 m | 28.0128° N, 86.8750° E | computed; 3.0 km north-east |
| Pumori (background, sunset light) | 283,562 · 1,122,158 | 7,161 m in reality | 28.0147° N, 86.8281° E | computed; 2.6 km north-west |
| **Dingboche**, the camp (start of leg 0) | 339,733 · 2,468,724 · −69,388 | 4,334 m (label 4,410) | 27.8935° N, 86.8338° E | [Measured] |
| **Thukla memorials** (leg 0 at 5.8 km) | 156,870 · 2,036,451 · −20,661 | 4,822 m | 27.9324° N, 86.8152° E | [Measured] on the route line; the father's stone is in `Chapter1_places.ygs` [Built] |
| Gorak Shep (end of leg 0) | 291,733 · 1,500,724 · 12,954 | 5,158 m | 27.9806° N, 86.8289° E | [Measured] |
| Pangboche (Nima's home; never visited, only named) | off the playable area | 3,985 m in reality | 27.857° N, 86.794° E | real coordinates |

### 5.2 Base Camp sub-places [To measure]

A new `Tools/Story/build_chapter7_world.py` places these around the camp landmark by tracing the ground, tags every
actor `Ch7`, writes `Content/Story/Chapter7_places.ygs`, and checks the key frames of section 4.4. Today's camp
[Built]: 9 tents in an arc, a mess tent, a store tent, 4 oxygen bottles, a stone puja altar at the pole
(`place_camps.py`, kit 2), on stone rubble over ice (`base_camp_ground.py`).

| Place name | Rule for placing it |
|---|---|
| `sangye_tent`, `tent_door`, `tent_out` | the last dome of the arc, door toward the Icefall |
| `nima_seat` | a barrel 5-6 m from `tent_out`, 25° right of the exit line |
| `hale_tent`, `hale_cot` | a medical tent (white, with a red cross flag [Check: what marks a clinic tent at Base Camp]) 30-50 m from the arc, toward the trail |
| `mess_door`, `mess_table`, `mess_pole_a/b/c`, `guy_line` | the existing mess dome; three pole spots inside; the loose guy line on the windward (north-east) side |
| `stores_tent`, `stores_front`, `father_barrel`, `plan_board` | the existing store tent; the father's barrel at the back, in the door's light |
| `comms_table` | under the mess dome's awning or its own small tent, with the antenna mast |
| `altar`, `flag_pole` | the existing puja altar |
| `doctors_tent` | 60-100 m from the arc on the Icefall side, ladders and rope outside |
| `phone_rock` | the highest moraine boulder or hump within 150 m of the camp pole, at least 4 m above the camp, with a clear line to the south (down-valley, where the signal comes from) |
| `icefall_dust` | a point 120-200 m above the Popcorn field coordinates (the dust cloud's centre) |
| `new_line`, `old_scar` | two points at the Popcorn field, 60-90 m apart, both visible from `doctors_cache`. They must match the Chapter 8 Icefall state (section 9, Q8) |
| `doctors_cache` | a flat spot within 15 m of leg 2 at 450 m |
| `nuptse_face` | from Nuptse's direction, the release point on the nearest steep face; `cloud_path` = 5 points from there across the glacier to the camp |
| `shelter_1..n` | every tent door, the altar's lee side, and 3-4 boulders taller than 1.4 m: the places that count as shelter in `c7_cloud` |
| other teams' camps | 3 clusters of 8-14 tents (instanced domes in other colours) along the moraine, 150-500 m away, lit from inside at night; no collision work, no people up close |

### 5.3 Dingboche branch places

`kitchen` (the house interior from Chapter 1 v1, kept in the project but not placed today; it must be placed in or
beside the walled field at the Dingboche camp, and **must have a way out that cannot trap the player**, the fault that
removed it from Chapter 1) · `gorak_cut`, `thukla_cut`, `juniper_cut`, `fields_cut` (the four descent cuts, on leg 0
and leg 1, each facing down the trail; the Thukla cut 12 m before the father's stone so the khata is walked past).

### 5.4 Art direction and light

The yardstick is the director's art direction: quiet awe; attention moving between the few metres at the feet and the
far view; a sky that carries the light; blue snow in shade; warm light only on the high ridges.

| Scene | Clock | Light | Note |
|---|---|---|---|
| Waking, day 7 | 07:10 | sun through yellow fabric; frost dripping | stillness |
| Camp, morning | 07:30 | the camp still in blue shade; the Icefall's top and Pumori already lit | "dawn = warm far, blue near" |
| Blue hour | 18:30 | forty tents glowing from inside against blue snow | the "inhabited mountain" image of this chapter |
| The collapse | 01:40 | moonlight; the dust cloud pale grey; headlamps coming on one by one | sound carries it |
| The phone rock | 17:30 | warm on the ridges only; a dark cloud over the Lho La | weather seen far off before it arrives |
| The storm | 21:30 | one gas lamp inside (swinging); lightning through the fabric | no outside view needed except in the "run out" choice |
| After the storm, day 9 | 08:00 | hard blue sky, new snow, every edge soft | "midday: startling detail" |
| The avalanche cloud | 11:40 | white-out by fog, then **ice dust in full sun** | the awe moment; the dust is the picture to get right |
| The store tent | 14:00 | dim blue light through fabric; one bright door | the barrel in the door's light |
| The forecast | 19:00 | green, clear, very cold dusk | "the weather is good" must look frightening, not friendly |
| The end | 21:00 | headlamp on paper, then dark | |

---

## 6. Gameplay and physiology hooks

### 6.1 The body in this chapter (the thesis core, shown as recovery)

| What | In this chapter | Status |
|---|---|---|
| **Resting SpO₂ at 5,277 m** | read each morning; it rises day by day as the adapted altitude catches up (v1: about 76-79 at rest on arrival in Chapter 4; after the night at Camp 1, about 81 → 83 → 85). **The numbers in the beat tables are examples. The game shows what the model gives.** If the model gives a flat line, the model's acclimatisation term needs attention, not the script | model [Built]; nightly adaptation [To build, v1 4.3] |
| **Resting pulse** | falls each morning (about 94 → 88 → 82): a real sign of acclimatisation, and easy to read | heart rate [Built]; resting trend [To build] |
| **Fatigue** (v1 4.1, new index) | sleep at Base Camp removes it at ×1.0 (it was ×0.6 or less at Camp 1). The chapter starts high and ends near zero, unless the player works | [To build] |
| **Adapted altitude** (v1 4.3) | rises only while sleeping, toward (highest point in the last 48 game hours − 300 m). After Camp 1 (6,131 m) the target is about 5,830 m; three nights at 150-250 m per night bring it there or close. This is what makes rotation 2 possible | [To build]; `ygg.Physio.Adapt` exists for tests |
| **Health** | regenerates when fed and watered [Built] | |
| **Hydration and food** | "four litres" (Dr. Hale). At Base Camp the kitchen gives food and tea, so supply is not a pressure here; the only cost is the torn tent (decision 4) | camp stock rules [To build, v1 4.4] |
| **Body temperature** | the storm: inside the tent nothing happens (shelter blocks every realm effect [Built]); the "run out" choice costs about 1.0-1.5 °C in 25 s of storm and never reaches the damage line (33 °C). The avalanche blast, unsheltered: −1.0 °C, a 60 s cough, one knee | storm cold [Built]; blast [To build] |
| **Sanity** | company restores it (v1 4.1: Nima within 25 m, each radio call). The call home gives a clear step up. The notebook lowers it a little: it is not comfort | company gain [To build] |
| **Elena's cough** | a dry cough in every scene she is in, a little worse on day 9. It is the common Base Camp cough, or it is not. Nobody says. It prepares her collapse later in the story | sound only |

### 6.2 Systems this chapter needs

| System | What it does here | Status |
|---|---|---|
| Story machine (`YggStory`) | plays `Chapter7.ygs`; all beat actions above exist in the camera-spec vocabulary | [Built]; chapter data [To build] |
| **Rest night** (`rest` action) [Proposed] | one action: closes the eyes, moves the clock to the wake time, applies sleep to fatigue, health and adapted altitude with a quality factor (altitude, a warm meal, a torn tent), bumps the day, opens the eyes | [To build]; it is the same thing every later camp night needs |
| Morning oximeter | the reading on the finger on waking; a caption with SpO₂ and pulse | oximeter [Built]; the waking caption [To build] |
| Realm weather, forced by the story | `ygg.Realm.Force` holds a realm [Built]. The storm realm (in code today: Jotunheim, shown to the player as "Winter's Veil" [To verify the display name]) gives blizzard, lightning and delayed thunder | [Built]; the story name is [Open] |
| Distant collapse | the existing distant-avalanche sound (`ygg.Avalanche`) plus a new long roar, a ground shake (through the rig), and a dust plume card over the Icefall (like Chapter 1's summit plume card) | sound partly [Built]; plume and shake [To build] |
| **Avalanche powder cloud** [From v1] | a cloud that travels `cloud_path` in about 20 s; white-out by fog (the realm look's fog cut-off already does white-outs); wind; shelter test; afterwards slow glitter particles (the spindrift and mote systems exist) and a snow-dusted look on tents and people (a material parameter by tag) | parts [Built]; the travelling cloud [To build] |
| **Tent-holding event** [From v1] | three gust waves; `holdkey` F per wave; wall bulge by material or simple animation; the guy-line choice | [To build] |
| Load plan board | choose 2 of 4 loads; writes camp stocks for later chapters | [To build] (shared with v1 Chapter 4's plan board if that is built first) |
| NPCs in person | Dr. Hale (new), Pasang the cook, a kitchen boy, two Icefall doctors, plus the Chapter 1 cast. They need `sit` and simple work idles (sorting, lashing, sewing) | cast pipeline [Built]; new people and idles [To build] |
| Condensed travel | blink + teleport + a short walk, repeated (all existing actions) | [Built] |
| Journal page | written from this chapter's flags (6.5) | [Built] in Chapter 1; lines [To build] |
| Session monitor and robot player | section 10 | [Built] |

### 6.3 Base Camp against Dingboche (decision 1) [Proposed numbers, placeholders until the model is calibrated]

| | Stay at Base Camp (5,277 m) | Go down to Dingboche (4,334 m) |
|---|---|---|
| Resting SpO₂ | about 81 → 85 | about 91-92 at home |
| Fatigue removed per night | ×1.0 | ×1.5 (thick air heals faster [From v1]) |
| Health | normal regeneration | ×1.5 |
| Adapted altitude | rises each night toward about 5,830 m | does not rise for two nights; falls about 50 m per day if he is more than 1,000 m below it [From v1]. With an adapted altitude near 5,400-5,500 m, Dingboche is about 1,100 m below: about −100 m |
| The walk back up (943 m of height, condensed) | — | arrives with fatigue +15 |
| Trust | Nima + if he works | Nima − (small): "the loads did not sort themselves" |
| Knowledge of Chapter 8's Icefall | possible (decision 3) | none |
| Story | the collapse, the storm, the cloud, the call | the khata at the stone, the aunt, Dolma, the best numbers of the game |

The honest summary for the player, said by Nima in `c7_plan`: down there he will feel wonderful and his blood will
learn nothing.

### 6.4 Flags

**Read (working names, to be aligned with the Chapter 1-6 documents):** `promise` / `bring_home` (Chapter 1's
promise to the aunt), `c6_lied_spo2`, `c6_painkiller`, `c6_descent_late`, `c6_sick_client_answer`.

**Written:** `c7_went_down` · `c7_honest_mess` · `c7_helped_doctors` · `c7_called_home` (+ `c7_call_truth`) ·
`c7_tent_saved` / `c7_tent_torn` · `c7_sheltered` (the avalanche cloud) · `c7_flags_raised` · `c7_loads` (two of:
`o2_c3`, `gasfood_c2`, `tent_c2`, `med_c2`) · `c7_diary_read` (always) · `c7_asked_torn_page` ·
`c7_told_nima_gold` · `c7_saw_log`.

### 6.5 The journal page (examples of lines by flag)

*"Day 9. Base Camp. 85 on the finger this morning. Nima says I am a Sherpa again."* ·
`c7_helped_doctors`: *"I carried ladders for the ice doctors. The old one says the new ladder bends in the middle and
that I must not stop there."* · `c7_tent_torn`: *"The wind took the mess tent's wall. We drank tea in the snow."* ·
`c7_went_down`: *"The khata is still on his stone."* · always: *"I found his notebook. He saw it too."*

### 6.6 Failure and edges

| Case | What happens |
|---|---|
| The player walks up the route toward the Icefall alone | Nima calls after 200 m (*"Not today. Not alone."*), again at 350 m; beyond the doctors' cache (450 m) without the `c7_carry` beat the way is closed by the doctors' rope and a doctor who turns him round. No invisible wall |
| The player stays outside in the storm | the realm's cold applies as in the open [Built]. At the cold warning (35 °C) Pasang shouts from the door; at 34 °C the cooks pull him in (failure forward, v1 4.11): a blink, the barrel, tea, and `c7_tent_torn` |
| The player ignores the avalanche warning | he is knocked to one knee and coughs. It cannot kill (rule R5) |
| The player never goes to a trigger place | each fixed beat has a clock fallback: Nima fetches him (`npc_mode present`), or the radio calls |
| Death is not possible in this chapter by design. If a system kills him anyway (a fall from a boulder), the normal death screen returns him to the Base Camp checkpoint with the day's state [Built] | |

---

## 7. Cast states

| Who | Where and when | State and look | Made how | Status |
|---|---|---|---|---|
| **Sangye** | everywhere | camp clothes: red down jacket (Chapter 1), no harness, no helmet, camp boots. His hands are seen often: the kettle, the pole, the rope, the notebook | Meshy character | [Built] |
| **Nima** | `nima_seat` (sorting gear), the stores, the phone rock, beside Sangye at the collapse and the cloud | calm, dry, watching Sangye more than before. He **laughs** once (the phone). He **turns away** once (the torn page). Trust moves by the choices | Meshy character | [Built] |
| **Dr. Ingrid Hale** | her tent (day 7 morning; optional day 9), the comms table (day 9, 19:00) | **in person for the first time.** Fleece, reading glasses on a cord, a watch on the inside of her wrist. Brisk, dry; day 9: unsure for the first time | **new character** | [To build] |
| **Marek Kovač** | mess tent; shouts at the collapse; the forecast | on a schedule; fair. Decides the day | Meshy character | [Built] |
| **Elena Ruiz** | mess tent; after the cloud; the forecast | **a dry cough** in every scene, slightly worse each day; warm to Sangye; if he helped her in Chapter 1 she says so once | Meshy character | [Built]; cough sounds needed |
| **Ji-woo Han** | mess tent; after the cloud; the forecast | bored, impatient, filming. After the cloud: thrilled, which is the wrong reaction, and he half knows it | MetaHuman + clothes | [Built] |
| **Pasang**, the cook [Proposed, Check: name] | mess tent, the storm, the altar | 40s, loud, kind, superstitious and joking about it. He is the one who says what Sangye's place is | reuse a porter MetaHuman with an apron and a wool hat, or a new Meshy character | [To build] |
| **The kitchen boy** | the storm (holds a pole), background | 16-17, frightened, says nothing | reuse the second porter look | [To build] |
| **Ang Dorje**, Icefall doctor [Check: name] | the doctors' tent, the cache | 55-60, slow, exact, burnt dark by sun. Knew the father as a young man | covered-face climber model in a faded suit is not enough here (he talks, close up): reuse a porter MetaHuman aged up, or a new Meshy character | [To build] |
| A second Icefall doctor | lashing ladders; no lines | | the climber model with hood and glasses | [Built] |
| **Aunt Pema, Dolma** | on the phone (voice); in person only in the Dingboche branch | Pema: relieved and stern. Dolma: one question | MetaHuman / Meshy | [Built] |
| **The father** | never seen. His handwriting and his **reading voice** | | voice only | VO |
| Other teams | 150-500 m away: small figures, lights, radio voices, a pop song | | instanced tents, the climber model far away, sound | partly [Built] |

**Limits for the cast.** The laptop has 12 GB of graphics memory: **at most five MetaHuman-quality people in view at
once** (the mess tent is the worst case: Marek, Elena, Ji-woo, Pasang, plus Nima at the door). Everyone loads in the
background at the chapter start and appears only behind closed eyes (camera spec, NPC preload).

---

## 8. Assets

### 8.1 Existing, reused

| Asset | From |
|---|---|
| Base Camp: 9 dome tents, mess tent, store tent, puja altar, flag lines (printed lungta), oxygen bottles, rubble ground | camp kit, Meshy round 1, the flag rebuild of 2026-11-19 |
| Meshy props: `tent_dome`, `tent_mess`, `tent_kitchen`, `tent_toilet`, `duffel`, `barrel_blue`, `bedding`, `porter_load`, `doko`, `stove_set`, `cook_set`, `butter_lamp`, `offering_bowl`, `choktse`, `tarchen`, `headlamp`, `water_bottle`, `daypack`, `khata_tied`, `father_stone`, the three chortens | `/Game/Story/AI/` |
| Poly Haven models (CC0): thermoses, enamel pot, jerrycans, propane tank, crates, lantern, brass pots | `/Game/Story/PH/` |
| The aunt's kitchen (walls, stove, shelves, thangkas) | `/Game/Story/House/` (kept, not placed) |
| Fixed-rope coil meshes, ladders (the Icefall ladders), ice axes | the Icefall slice |
| Characters: Sangye, Nima, Marek, Elena (Meshy); Ji-woo, Pema, porters (MetaHuman); Dolma, the father (Meshy) | the cast |
| Effects: spindrift, storm lightning and thunder, realm fog white-out, distant avalanche sound | RealmSky, MountainAudioComponent |
| A tent interior (inner tent, mat, sleeping bag) | **missing since Chapter 1** (its open item 9); this chapter needs it three times. Not counted below |

### 8.2 New, as one-line Meshy prompts

Prices: a prop is about **33 credits** (concept image 3 + textured model 30); a character is about **41** (concept 6
+ model 30 + rigging 5). **Only 7 credits are left** (morning report, 2026-11-20). Everything below needs a purchase,
which is the director's money and the director's decision (Q7).

| # | id | Kind | Size | One-line prompt | Credits | Need |
|---|---|---|---|---|---|---|
| 1 | `hale` | char | 1.70 m | A Norwegian woman doctor in her early fifties at Everest Base Camp, lean weathered face, grey-blonde hair in a short ponytail, reading glasses on a cord, navy fleece jacket over a grey wool sweater, dark soft-shell trousers, worn approach shoes, a stethoscope in one pocket, A-pose, photorealistic, no logos | 41 | must |
| 2 | `father_notebook` | prop | 0.21 m | A cheap Nepali school exercise notebook, soft blue paper cover faded and water-stained, corners curled, a rubber band around it, inside a clear zip bag, a few loose page edges showing, no readable text, photorealistic | 33 | must |
| 3 | `ladder_bundle` | prop | 2.4 m | Two aluminium ladder sections lashed together side by side with old blue and white climbing rope, scratched and dented, snow in the rungs, a coil of rope and four snow pickets tied on top, carried as one load, photorealistic | 33 | must (decision 3) |
| 4 | `radio_table` | prop | 1.6 m | An expedition radio station on a folding camp table: a black VHF base radio with a hand microphone on a coiled cord, two handheld radios in a charger, a car battery, a small solar charge controller, cables, a paper log book and a pen, weathered, no logos, photorealistic | 33 | must (the forecast) |
| 5 | `medical_kit` | prop | 2.0 m | The inside of a Base Camp medical tent as one set: a folding aluminium camp cot with a grey blanket, a green medical oxygen cylinder with a regulator and mask on a stand, an open hard case of medicine boxes, a small fan heater, a closed laptop on a crate, photorealistic, no logos, no readable text | 33 | must |
| 6 | `plan_board` | prop | 1.8 m | A white dry-erase board on a wooden easel at a mountain base camp, smudged, with hand-drawn lines, circles and arrows in blue and red marker and small magnets, unreadable handwriting, a rag hanging from one corner, photorealistic | 33 | must (decision 2) |
| 7 | `mess_table_set` | prop | 3.0 m | A long folding expedition mess table with a plastic flowered tablecloth, eight folding camp chairs, steel thermoses, enamel mugs, a tin of biscuits, sauce bottles and a roll of toilet paper used as napkins, photorealistic, no logos | 33 | should (the mess tent is empty inside today) |
| 8 | `tent_mess_torn` | prop | 6.0 m | The same yellow expedition mess dome tent after a storm: one wall panel torn open and flapping, a pole bent, snow drifted inside against the tear, guy lines slack, photorealistic | 33 | optional (only for `c7_tent_torn`; a hidden-panel trick on the existing dome is the free fallback) |
| 9 | `pasang` | char | 1.65 m | A Sherpa expedition cook in his forties, round friendly sun-darkened face, thin moustache, a knitted wool hat with ear flaps, an old faded green down jacket with a stained cotton apron tied over it, track trousers, rubber boots, A-pose, photorealistic, no logos | 41 | should (fallback: a porter MetaHuman with an apron) |
| 10 | `ang_dorje` | char | 1.66 m | A Sherpa Icefall doctor around sixty, deeply sun-burnt lined face, grey stubble, glacier glasses pushed up on a faded cap, an old patched one-piece down suit open to the waist with the arms tied round the hips, a worn harness with carabiners and a hammer, heavy mountaineering boots, A-pose, photorealistic, no logos | 41 | should (fallback: an aged porter MetaHuman) |
| 11 | `phone_old` | prop | 0.13 m | A cheap old button mobile phone with a cracked screen corner and a strip of blue tape holding the battery cover, photorealistic, no logos | 33 | optional (it can stay hidden in Nima's glove) |
| 12 | `o2_rack` | prop | 1.5 m | A wooden rack holding twelve orange high-altitude oxygen bottles lying in two rows, each with a strip of tape and a marker number, regulators and masks in a crate beside it, photorealistic, no logos | 33 | optional (single bottles exist) |

| Group | Credits |
|---|---|
| Must (1-6) | **206** |
| Should (7, 9, 10) | 115 |
| Optional (8, 11, 12) | 99 |
| **All** | **420**, plus about 15% for retries: **about 480** |

**Not from Meshy:** the tape with the father's name, the notebook's pages, the log on the laptop and the plan-board
drawing are **decals and textures** made by hand so the words can change (and be checked). Other teams' tents are
colour variants made in Unreal. The avalanche cloud, the dust plume and the ice glitter are effects.

**Free option to check first** (rule: download before building): Poly Haven and Fab free items for a camp table,
folding chairs, a whiteboard and a radio set; if good ones exist, items 4, 6 and 7 cost nothing.

---

## 9. Sound needs (a list only; the director picks every sound by ear)

The identity rule: deep, resonant, slow to die away, a sense of fate; never light or "app-like"; no thunder under
interface sounds.

**The mountain**
1. The Icefall collapse at night: a rifle crack, then a 9-second roar with a long tail and an echo off the valley walls
2. The silence after it (room tone of a cold night; one dog-like bark of a raven is optional)
3. The glacier under the tent floor: ticks, creaks, one deep groan (three variants; they return in `c7_end`)
4. One far Icefall crack for the last second of the chapter
5. The avalanche on Nuptse: a high crack, a tearing roar, the wind front arriving, the white-out roar, and the stop
6. Ice dust falling in silence: almost nothing, a faint glassy hiss
7. Storm: wind on tent fabric (slamming in and out), three gust swells, snow hiss on fabric, thunder near and far
   (exists; check it against the tent setting), a guy line whipping
8. A rising evening wind over the Lho La before the storm

**The camp**
9. Camp bed at blue hour: stoves, zips, far voices in several languages, a generator, a far radio with a Nepali pop
   song [Check: licence; a made-up song is safer]
10. Cook singing while working (a short Sherpa or Nepali work song) [Check]
11. Mess tent: kettle, tea pouring, enamel mugs, chairs, pots falling in the storm
12. Radio: squelch, crackle, "all teams" call tone; the handset click
13. Oxygen bottles being tested: short hisses at different pitches
14. Ladder sections clanking; rope pulled tight and creaking; a hammer on a picket, far
15. Sewing the tent (needle through ripstop), snow shaken off fabric
16. The barrel: lid ring, lid ticking in the wind (the lure for `c7_barrel`), zip bag, paper pages
17. The phone: signal beeps, a tiny speaker voice (a child), the line cutting
18. The oximeter's small double beep (deep and soft, not a toy) [Open: is a beep right at all, or silent]

**Bodies**
19. Elena's dry cough: 6-8 variants in three strengths
20. Breath and effort carrying the ladder bundle (exists; check the load variant)
21. Coughing ice dust (Sangye, 60 s, fading)
22. Nima's laugh on the phone

**Story cues**
23. The notebook: a low held tone under the father's reading voice, and the heartbeat rise (the `fovbeat`)
24. Day change (each waking): one deep, soft cue in the family of the approved landing sounds (C, E, H)
25. The end card: the chapter's closing cue, in the same family
26. Dingboche branch: the kitchen stove, a butter lamp being lit, yak bells far off, the river, thick-air silence

**Voices (the director records)**: Sangye, Nima, Dr. Hale, Marek, Elena, Ji-woo, Pasang, Ang Dorje, the father
(reading), Pema, Dolma, Nima's daughter (phone), a radio voice, far shouts. Every line is logged as `Story [VO]`.

---

## 10. Robot-player test plan

The robot player (`UYggPlayBot`, `Tools/Bot/run_bot.ps1`) plays a chapter by itself under the session monitor and the
camera flight recorder. For this chapter:

### 10.1 What must exist first [To build]

| Item | Note |
|---|---|
| `Tools/Story/chapter7_story.py` → `Content/Story/Chapter7.ygs`, with the same `validate()` as Chapter 1 | the validator's limits for this chapter: nudges ≤ 2, `fovbeat` ≤ 1, every teleport and every NPC spawn inside a blink, `hold "end"` only in the last beat |
| `Tools/Story/build_chapter7_world.py` → `Chapter7_places.ygs` | places traced, key frames checked (section 4.4) |
| A chapter start state | `ygg.Story.Start Chapter7` must put the climber at Base Camp (as `ygg.Expedition.Go 2`), on day 7, with a body like the end of Chapter 6: adapted altitude about 5,400 m (`ygg.Physio.Adapt`), fatigue high, and default Chapter 6 flags |
| A way to choose the branch | today `ygg.Story.Auto 1` answers by itself. Needed: a bot setting that forces the answers of a run, for example `ygg.Bot.Answers "c7_plan=stay,c7_doctors=carry,c7_storm=run"` [Proposed] |
| Time compression for waits | the bot's fast mode must also shorten `timeto` waits and sleeps, or the free-time beats stall it |

### 10.2 The runs

`powershell -File Tools/Bot/run_bot.ps1 -Chapter chapter7 -Fast -YggCamRec -Extra "<answers>"` (editor closed: one
heavy process at a time).

| Run | Answers | Proves |
|---|---|---|
| A · "worker" | stay · honest at the table · carry · call home · run out · shelter · any loads · ask about the page · see the log | the longest path; every beat fires |
| B · "sleeper" | stay · silence · rest · no call · hold · stand in the open · silence everywhere · no log | the shortest path; the unsheltered knock; the torn tent |
| C · "home" | go down | the Dingboche branch: 7 travel cuts, the kitchen (and its exit), the return, the shared ending |
| D · "wanderer" | stay; then the bot ignores targets for the first minutes: walks up the route toward the Icefall, stands outside in the storm | the soft boundaries and the failure-forward path (section 6.6); every fixed beat still fires by its clock fallback |
| E · camera abuse | run A with `ygg.Bot.Camera 1` (fast sweeps, straight down and up) | no spins, no roll, no low eye while walking |

### 10.3 Pass conditions (measured, from the bot report and the monitor)

**Story**
- every F beat fires in every run; the chapter ends with `Story: Chapter7 finished`; exit code 0
- real time at normal speed: 25-35 minutes (run A), 20-28 (run B), 25-35 (run C)
- no gap longer than 6 minutes without a beat, a choice or an event (monitor)
- the out-flags are set and consistent (`c7_helped_doctors` only in run A; `c7_went_down` only in C; `c7_diary_read` always)

**Camera** (the Chapter 1 checks that apply)
- nudges ≤ 2; no view change the player did not cause beyond 20°/s yaw, 8°/s pitch
- eye never below 120 cm while moving faster than 0.6 m/s; every cut behind closed eyes
- no lease expires by its time limit (log line `Rig: lease ... expired`): zero lines
- no camera incident files in `Saved/Monitor/incidents/`

**Body** (the thesis data; from the per-second log)
- resting SpO₂ at Base Camp on waking: inside the model's expected band for 5,277 m, and **not lower** on day 9 than on day 7
- resting pulse on waking: day 9 lower than day 7
- fatigue at the end lower than at the start in runs B and C; run A higher than B at the end of day 8
- adapted altitude: rises in A and B; does not rise in C
- in run C: SpO₂ at Dingboche higher than any Base Camp reading
- body temperature: never below 34 °C in any run; in run B the avalanche blast costs about 1 °C; in the storm inside the tent it does not move
- health never falls in runs A-C

**Performance**
- no freeze over 100 ms with the eyes open; NPC loading only behind closed eyes
- the mess tent in the storm (five people, lamp, lightning): the real frame rate is not lower than the Chapter 1 camp scene by more than 15%
- the avalanche cloud and the ice dust: no frame under the project's floor during the white-out; particle count logged
- graphics memory peak logged against the 12 GB limit

**Review shots** (`story_shots.ps1`, jump to a beat, one screenshot each, for the director's eye): standing up
outside the tent · Dr. Hale close · the mess table · blue hour · the dust over the Icefall · the new line from the
cache · Nima on the phone rock · the storm inside · the avalanche at 5 s, 15 s, and the ice dust · the barrel · the
notebook pages · Nima in the door · the forecast · (branch) the khata at the stone, the kitchen.

### 10.4 What the robot cannot judge

Whether the storm is frightening, whether the dust is beautiful, whether Nima's lines land, whether the sounds carry
fate. Those are the director's, after the measured checks pass.

---

## 11. Risks and open questions

### 11.1 For the director

| # | Question | Default if no answer |
|---|---|---|
| Q1 | **Is this the right Chapter 7?** v1's "Thick Air" (this document) or the 12-list's "The Lhotse Face"? And which list is the real one now: 17 chapters or 12? | this document stands as the rest-days chapter under either number |
| Q2 | The **father's name**. Chapter 1 puts a name on the stone as a decal; the tape on the barrel and Ang Dorje's line need the same one ("Tashi" is only a placeholder here) | use the name on the stone |
| Q3 | **The Six Realms name of the storm.** Proposed: the Demigods' wind (the cook's "lha-ma-yin are fighting"), first heard here from far below and met in full at the South Col. Or leave the storm unnamed here | unnamed in captions; the cook's line stays if it passes the check |
| Q4 | **Dingboche: inside the three days** (this design) **or two extra days** as v1 said (then Chapter 8 starts on day 12 and the weather window can be lost)? | inside the three days |
| Q5 | **The father's "impossible" SpO₂ 94 at the Balcony** as Dr. Hale's crack: good, or does it give away too much before Nima speaks? | keep, optional beat |
| Q6 | **The notebook's three lines** (the gold plume; "not ready"; "If she calls, I will not say no"). Is "she" (the mountain as the Goddess Mother) the right word for the father to use? | keep as drafts for the voice session |
| Q7 | **Meshy credits:** 7 left. Must-haves cost about 206, everything about 480 with retries | buy nothing until the free downloads are checked; then the must-haves |
| Q8 | **Where exactly is Chapter 8's new line?** `new_line` and `old_scar` must be the same points Chapter 8 uses | take them from `Chapter8_Design.md` when it exists |
| Q9 | Length: 28-33 minutes, or cut to v1's 25? | measure first |
| Q10 | The day count: v1's calendar says days 7-9; Chapter 1 shows "Day 1 / 49". Are these the same count? | yes, one count |
| Q11 | Should the oximeter beep? | silent |
| Q12 | Should the player be able to refuse the tea job (sit down with the clients)? | no (rule R6) |

### 11.2 Risks

| Risk | Likelihood | Effect | Mitigation |
|---|---|---|---|
| The numbering mismatch (Q1) means this is the wrong chapter | medium | wasted design | flagged at the top; sections 5-10 are reusable for any Base Camp chapter |
| A rest chapter is boring (the director's own 2026-11-14 concern) | medium | players quit here | three mountain events, three decisions, a 4-minute maximum gap; cut list b of v1 can shrink it to one scene |
| The systems it stands on do not exist yet: fatigue, nightly adaptation, camp stocks, trust | high | recovery is invisible; decisions have no weight | the chapter still plays as story without them; build the `rest` action and the morning oximeter first, they serve every later camp |
| The avalanche cloud looks like plain fog | medium | the awe moment fails | build and judge it alone first (a test level shot at the altar) before anything else in the chapter |
| The mess tent storm looks cheap without cloth motion | medium | the fear moment fails | sound and light carry it; wall bulge by a few blend shapes or a material offset; judge by a 20 s test clip |
| Five realistic people in one tent on a 12 GB card | medium | stutter, texture loss | 1k cloth textures, preload behind closed eyes, the kitchen boy and second doctor as the light climber model |
| The Dingboche kitchen traps the player again | medium | the Chapter 1 fault returns | one wide door, the bot's run C must walk out; or play the scene seated only |
| Cultural mistakes (section 11.3) | medium | disrespect to living people and a living religion | every item marked [Check]; a Khumbu Sherpa consultant before the title demo |
| The Icefall collapse reads as the real 2014 disaster (sixteen Sherpas died in the Icefall) or the 2015 Base Camp avalanche (many died in camp) | medium | hurtful, careless | nobody dies; no date; no real names; the lines are about luck and work, not spectacle; ask the consultant whether to keep both events |
| Thesis time: chapter work crowds out the physiology core | medium | a weak interim | this chapter is not needed for the December interim; build it after Phase A |
| Voices: about 60 lines, 13 speakers, two languages | medium | a long recording job for the director | lines are short; the father's reading voice and Nima first |

### 11.3 Culturally uncertain: to check before any of it is final

1. **Sherpas working on rest days** and not sitting at the clients' table; the cook's "Not our table." True of many
   expeditions, but it is a judgement about class and must be said the way a Sherpa would say it.
2. **Names:** Pasang, Ang Dorje, and the father's name. Sherpa names follow the day of birth; common, but check the
   combinations and that none points at a real, known person (Ang Dorje is a real, well-known climber's name: likely
   to change).
3. **"Icefall doctors"**: the real team is appointed by the Sagarmatha Pollution Control Committee. Use the role, no
   real names, no real logos.
4. **"lha-ma-yin"** (Tibetan for the demigods, asura) said by a Sherpa cook as a joke about a storm. Is this something
   a Sherpa would say, and is joking about it acceptable?
5. **The Six Realms as weather** in general, and the Demigods' war over the Wish-Fulfilling Tree in particular (in
   Buddhist cosmology the demigods fight the gods over a tree whose roots are in their realm and whose fruit is in
   the gods'). It fits the game's summit tree very well, which is exactly why it needs a careful check.
6. **Prayer flags on the ground** after the avalanche blast: the belief that they must not touch the ground, and who
   may re-string them.
7. **The mountain as "she"** (Miyolangsangma, the goddess who lives on Chomolungma) in the father's private writing.
8. **The father's handwriting:** which language and script would a Khumbu Sherpa of about 48 write a private notebook
   in? (Sherpa has no single standard script; Nepali in Devanagari is likely; some would write in English.) Until it
   is checked, the page shows handwriting that cannot be read and the subtitle carries the meaning.
9. **The butter lamp for the dead father** in the aunt's kitchen, a season after his death (Dingboche branch), and
   whether the 49 days of the Bardo can still be the game's day count when he died last season.
10. **The puja altar**: touching it, crouching behind it for shelter in the avalanche. If that is disrespectful, the
    altar is removed from the shelter list.
11. **Mobile signal at Base Camp** and the "phone rock": true in recent years (patchy), but check the season and
    the direction.
12. **Nima's family in Pangboche**, his daughter's wish, and Sherpa spoken on the phone: the words must be written
    by a Sherpa speaker; until then the phone speech is subtitled in part only and mixed low.
13. **Avalanches reaching Base Camp from Nuptse:** powder blasts do reach the camp from the walls around it; which
    wall on our terrain can do it believably must be measured, and the line "it only does it to say hello" must not
    sound careless to people who lost family in 2015.
14. **The medical details:** "four litres", resting SpO₂ in the low to mid 80s at 5,300 m after a week, a falling
    resting pulse, the Base Camp cough. Believed correct; check against the thesis sources.

---

## Appendix: sources

| Document | Used for |
|---|---|
| `Docs/Development/Chapter_Design_v1.md` §5 (Chapters 6, 7, 8), §2.3, §2.7, §3, §4 | the chapter's slot, hand-offs, decisions, scenes, systems, calendar |
| `Docs/Development/Chomolungma_Chapter1.md` Part 1 | the 12-chapter list; "Rest Days" and the diary pages |
| `Docs/Development/Chapter1_v2_Design.md` | format, labels, pillars, cast, pipeline, risks |
| `Docs/Development/Chapter1_Camera_Spec.md` §4.4, §6, §7 | leases, nudge, reflex, postures, the story data vocabulary, the validator |
| `Tools/RoutePlanning/route.json`, `place_camps.py` | every [Measured] coordinate; today's Base Camp layout |
| `Tools/Bot/README.md` | the robot player |
| `Tools/Meshy/assets.py` | existing AI assets and prices |
| `Docs/Development/2026-11-20-morning-report.md` | what is built today; credits left |
| memory notes: Chomolungma direction, climb design decisions, story machine, terrain georeference, art direction, audio identity, Meshy pipeline, realm planets | direction, rules, tools |
