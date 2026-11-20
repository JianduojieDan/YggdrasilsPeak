# B5 · Sound and music plan for the two demo routes

Written 2026-11-20. Block B5 in `Docs/Development/Development_Plan.md`. This is the **plan only**. Nothing is
downloaded, picked or hooked into the game yet.

**Goal of B5**: no placeholder sounds on the demo routes, and 3–4 music cues that carry the feeling: epic, poetic,
a sense of fate.

**Rules for the whole block**
- **You pick every sound by ear.** I prepare candidates, cut them, check them, and put them in front of you in the
  game. I never choose. Nothing goes into the real game before you mark it good.
- **Placeholder** here means: any sound on the route that you have not yet marked good *in its scene*. A sound I made
  myself (synthesized) is not a placeholder if you approved it (the wheel landing sounds are an example).
- **Downloads**: one list, one OK from you, before anything is downloaded. Files go to the project's `Downloads/`,
  never to C:.
- **Approved files stay byte-identical.** I checksum them before and after every rebuild.
- **Examine first, then hook.** Every sound is cut into whole events or clean loops and checked before you hear it.

---

## 1. What exists today

All game sounds live in `/Game/Audio/<Folder>` (`Content/Audio`). The code loads them by name.

| Folder | Assets | Where they come from | Your verdict | B5 status |
|---|---|---|---|---|
| Body | `Breath_Phrase_01–06` | your recording | good (05 = phase 6, 06 = phase 5) | keep |
| Footsteps | `Step_Snow_01–11` | your recording (Nox boots in snow) | good | keep |
| Footsteps | `Step_Ice_01–14` | your crampons recording | good | keep |
| Footsteps | `Step_Rubble_01–08` | downloaded recording (gravel) | good in game | keep |
| Footsteps | `Step_Ladder_01–06` | downloaded ladder recording + your crampon steps | good in game | keep |
| Gear | `Carabiner_ClipIn_01–05`, `Carabiner_Click_01–06`, `Rope_Slide_Loop/Fast/Slow` | your recordings | good in game | keep |
| Gear | `Rope_Tighten_01, 03–06` | your recording | good, but not used ("rope pull is not correct") | keep, unused |
| Ice | `Avalanche_01–05`, `Ice_Crack_01–08`, `Ice_Groan_01–04`, `Serac_Collapse_01–02` | your recordings | good (`Ice_Crack_01` "too loud") | keep |
| Ice | `Mountain_Boom_01–03` | made by me (synthesized) | good in game | **ask**: keep, or find a real one? |
| Mind | `Whisper_01–06` | made by me (synthesized) | 01 good, 02–06 unsure | **replace or remake 02–06** |
| Story | `Story_Flags_Loop`, `Story_Stream_Loop`, `Story_Stove_Loop`, `Story_Radio_Squelch`, `Story_Radio_Static_Loop`, `Story_Plume_Drone`, `Story_Heartbeat` | made by me for chapter 1 (stand-ins) | not judged yet (not in the sound tour) | **placeholders** |
| UI | `Wheel_Tick_01–06`, `UI_Chime_C/E/H/K/L/M`, `UI_Chime_01` | your recording / made by me | good | keep |
| Weather | `Wind_Calm`, `Wind_Gusty`, `Wind_Blizzard`, `Wind_Whistle`, `Wind_Gust_01`, `Thunder_01–04` | your recordings | good | keep |
| Classes | `SC_Effects`, `SC_Ambience`, `SC_Voices`, `SC_Interface` | sound classes for the volume sliders | — | needs a **Music** class (see section 6) |

Also, outside the game:
- **Heartbeat candidates** (block A7): `Saved/AudioWork/Heartbeat/`, with `listen_heartbeats.bat`. Waiting for your ear.
- **Music**: nothing exists. No music component, no music slider.

What the code already does well (keep it):
- Four wind loops blended by the weather's wind (0–10) and the storm. The whistle comes in only above 5,600 m in strong
  wind. Single gusts above wind 6. Inside a tent the wind is muffled (a low-pass filter: it cuts the high sounds).
- Breathing gets harder and louder with effort. Silent at rest down low.
- Footsteps follow the animation (rhythm error under 11 ms) and change with the surface.
- Thunder is darker and quieter with distance. Avalanches come from a direction.

Three small problems found while reading:
1. The `Story` folder has **no sound class**, so the volume sliders do not touch those sounds.
2. The whispers sit in `SC_Voices`. If we duck the mix under voices (section 6), whispers would duck it too. They need
   their own class.
3. There is no **Music** volume slider in the settings.

---

## 2. The sound identity: fate

From your reviews (wheel landing rounds 1–4):
- Deep, resonant, slow to fade. A dark key with space around it (a stone-hall echo).
- Never light, bright, short or "app notification" style.
- No thunder under interface or music cues ("why a lightning sound?").
- When you ask for variety, I change the **instrument family**, not only the pitch.
- Present every candidate **in context** (the full scene first, then the sound alone).

Added for B5:
- **One key world.** The wheel bells are in D. All music is built around D too, so a wheel landing never clashes with
  the music.
- **One small motif for fate.** A three-note figure that appears three times: faintly in the gold plume (chapter 1),
  slowly in the death zone, and in full at the summit. The player feels it come back without knowing why.
- **Silence is a tool.** The chapter 1 design says: "Music: almost none." Default = no music. Music is a rare event,
  so it means something when it comes.
- **Respect the mountain's culture** (the Chomolungma direction). Tibetan instruments (long horns, bells, cymbals) may
  be used as distant colour, never as decoration. No recorded prayer chant with words. Any use gets a cultural check
  before release.

---

## 3. Route 1: Dingboche → Base Camp → Icefall → Camp 1

Route legs 0–3 in `Tools/RoutePlanning/route.json`. Walk target 15–25 minutes. Camp numbers are the ones
`ygg.Expedition.Go <n>` uses.

**Bed** = always-on background loop. **Spots** = single events. **Have** = exists. IDs like E5 are in section 5.

### 3.1 Dingboche (4,410 m, camp 0) — the start, chapter 1's village
- **Bed**: calm valley wind (`Wind_Calm`, have). The river far below (N1; today `Story_Stream_Loop`, stand-in).
- **Spots**: prayer flags at the camp (E5; today `Story_Flags_Loop`, stand-in). Yak bells passing (N2, missing:
  "sound before sight"). The stove inside (`Story_Stove_Loop`, stand-in). Radio (`Story_Radio_*`, stand-ins).
- **Body**: frozen ground and stones (`Step_Rubble`, have). No breathing at rest.
- **Music**: none. Only the gold-plume sting in chapter 1 (`Story_Plume_Drone`, stand-in), which carries the fate
  motif for the first time.

### 3.2 The trail to Thukla and the memorials (4,600–4,830 m, leg 0)
- **Bed**: wind rising from calm to gusty on the pass (have). The Khumbu river near the Thukla bridge (N1).
- **Spots**: flags snapping at the memorial stones (E5). Distant avalanches start above 4,900 m (have).
- **Body**: `Step_Rubble` (have). Breathing starts on the steep Thukla climb (have).
- **Music**: **deliberate silence**. The chapter 1 text: "The wind is the only sound."

### 3.3 Lobuche → Gorak Shep (4,900–5,164 m, end of leg 0, camp 1)
- **Bed**: wind (have).
- **Spots**: distant avalanches (have). Rare far calls of choughs or ravens (N3, optional: these birds really live this
  high, and one far call makes the silence bigger).
- **Body**: `Step_Rubble`, breathing (have).
- **Music**: none.

### 3.4 Everest Base Camp (5,364 m, end of leg 1, camp 2)
- **Bed**: wind (have). Camp life far off: voices in several languages, never words you can follow (N4).
  Optional realism: a generator hum (N5) and a far helicopter (N6) — see open question 4.
- **Spots**: tent zips (E4). Tents moving in the wind (N7). Flags at the altar (E5). Juniper fire at the altar
  (B3 crackle, quiet). Radio squelch (stand-in). A low creak of the glacier under the camp (`Ice_Groan`, have, very
  quiet: a new use, so you judge it).
- **Body**: `Step_Rubble` on the moraine, `Step_Ice` on bare glacier (have).
- **Music**: **M1 "The Valley" (calm)** — once, at the first full view of the Icefall from the camp. About 60–90 s,
  then wind only.

### 3.5 Khumbu Icefall (5,400–5,900 m, legs 2–3, camp 3 inside it)
- **Bed**: still glacier air: almost silence with faint ice ticks (A5, missing). Wind lower inside the maze (have; see
  "wind exposure" in section 6). Optional: water trickling at midday when the ice melts (N8).
- **Spots**:
  - Two serac zones: groans, warning cracks, collapse (have).
  - Small random cracks between the zones (`Ice_Crack`, have, quieter: new use).
  - Falling ice chips (C6, missing).
  - Hidden crevasse: a snow bridge breaking (C5, missing; today it borrows `Ice_Crack_03`).
  - Avalanches off the West Shoulder and Nuptse (have).
  - The ice cave off leg 3: a quiet, close "room" of ice with drips (N9, optional).
- **Body and gear**: crampons on ice (`Step_Ice`), ladders (`Step_Ladder`), fixed rope clip-in, click and slide
  (have). Ice axe into ice (E3, missing). Down jacket rustle (D8, missing). Falling into snow (D9, missing).
  Heartbeat when fear or heart rate is high, for example on a ladder (F3 / A7).
- **Music**: **M2 "The Ice" (tension)** — starts at the first ladder or the first serac zone. A low drone with no beat
  and a slowly rising line. It goes silent 2 s before any serac warning, so the ice speaks alone, and comes back about
  20 s later. It ends at Camp 1. Never longer than about 6 minutes without a gap of silence.

### 3.6 Camp 1 (6,065 m, camp 4) — the end of route 1, the door to the Western Cwm
- **Bed**: very quiet wind (have, low exposure). Chapter 4 is called "The Valley of Silence": the silence is the sound.
- **Spots**: avalanches off the Nuptse and Lhotse walls (have). The deep boom inside the mountain over the Cwm
  (`Mountain_Boom`, synthesized; see open question 7). Tent zip (E4) and tent fabric (N7). Oxygen bottles clinking at
  the camp store (N10).
- **Body**: `Step_Snow`, breathing (have).
- **Music**: a short **M1 reprise** on arrival (relief, 30–40 s), or silence. Your call (open question 2).

---

## 4. Route 2: South Col → Summit (the death zone)

Reached by the camp jump `ygg.Expedition.Go 7`. Route leg 7. About 20 minutes with bottled oxygen.

### 4.1 South Col, Camp 4 (7,906 m, camp 7)
- **Bed**: the strongest wind on the route: gusty to blizzard, whistle and single gusts (have). The Col is a wind
  funnel, so its wind exposure is high (section 6).
- **Spots**: tents flapping hard (N7, strong version). Loose oxygen bottles knocking on stones (N10). Flags (E5).
- **Body and gear**: oxygen mask and regulator hiss (D7, missing). Hard breathing (have). Heartbeat (F3 / A7).
  Jacket rustle (D8). Steps on rock (`Step_Rubble`) and blue ice (`Step_Ice`) (have).
- **Music**: none at the Col. M3 starts at 8,000 m.

### 4.2 The Triangular Face to the Balcony (8,000–8,400 m)
- **Bed**: wind (have). Spindrift: fine snow hissing over the slope and the jacket (N11, missing; the look exists).
- **Spots**: fixed rope (have). Rope ascender clicks (E2 / N12, shared with block B3). A bottle change: click and hiss
  (N10 + D7).
- **Body**: hardest breathing phrases, heartbeat, crampons on snow and ice (have / A7).
- **Music**: **M3 "Above 8,000" (death zone)** — starts with the death-zone message at 8,000 m. The fate motif, slow and
  very low. It has 2–3 layers; the lower your SpO₂ (oxygen in the blood), the more layers and the darker it gets.

### 4.3 South Summit, the cornice traverse, the Hillary Step (8,750–8,790 m)
- **Bed**: an exposed ridge: strong wind and whistle, wide in stereo (wind from both sides) (have, wider mix).
- **Spots**: crampons scraping on rock at the Step (N13, missing). Carabiner moves (have).
- **Body**: the hearing is most muffled here (section 6); breath and heart are close and loud.
- **Music**: M3 thins to its drone only, so you hear the danger. Silence at the Hillary Step itself.

### 4.4 Summit (8,849 m, camp 8)
- **Bed**: wind with whistle (have). Prayer flags on the summit pole, snapping hard (E5, strong version).
- **Music**: **M4 "The Tree" (summit)** — the one full fate moment, during the summit beat (camera hold, subtitle,
  the World Tree in view). 60–90 s, then only the wind. This is where the motif plays in full.

### 4.5 Going down, fainting and death
- **Going down**: wind and body only. The M3 drone may return faintly below the South Summit (your call).
- **Fainting** (`ygg.Body.Faint`): the world fades for a few seconds, the heartbeat stays, then the world comes back.
- **Death by hypoxia** (without oxygen, for example 8,552 m at 21:09 in the robot test): the music stops. Over about
  10 s the world sounds go dull and quiet, the breathing fades, the heartbeat slows and stops. Two seconds of silence,
  then the death screen. No new sound is needed except the heartbeat.

### 4.6 Weather layers (both routes, any place)

The weather wheel can bring any of nine weathers. Each adds a layer on top of the wind. (The names may change with the
Six Realms re-rooting; the moods stay.)

| Weather | Danger | Extra layer | Status |
|---|---|---|---|
| Quiet Sky | harmless | none | — |
| Pearl Light | harmless | faint glassy shimmer (A7) | missing |
| Jade Tide | harmless | very low warm hum (A8) | missing |
| Golden Hush | harmless | warm low hum, soft wind (A8) | missing |
| Veil of Mist | risky | near silence; distant sounds muffled (A5) | missing |
| Starfall Frost | risky | sparse thin shimmer and ice ticks (A7) | missing |
| Emberfall | risky | low distant rumble (A6) and soft crackle (B3) | missing |
| Winter's Veil | deadly | blizzard and thunder | have |
| Silver Eclipse | deadly | near-total silence; only your breath (A5) | missing |

---

## 5. Missing sounds and music, by priority

**Must** = needed for "no placeholder on the demo route". **Should** = makes a place feel real. **Could** = nice.
IDs from `Sound_and_Forest_Download_List.md` are kept; new ones start with N.

| ID | What | Where | Priority | Today | Where candidates can come from |
|---|---|---|---|---|---|
| M1 | Music "The Valley" (calm) | Base Camp, Camp 1 | Must | nothing | see section 7 |
| M2 | Music "The Ice" (tension) | Icefall | Must | nothing | section 7 |
| M3 | Music "Above 8,000" (death zone) | 8,000 m to summit | Must | nothing | section 7 |
| M4 | Music "The Tree" (summit) | summit beat | Must | nothing | section 7 |
| F3 | Heartbeat | everywhere (body) | Must | `Story_Heartbeat` stand-in | A7 candidates already cut |
| D7 | Oxygen mask and regulator hiss | route 2 | Must | nothing | download or recording, or I make it |
| E5 | Prayer flags in wind (calm and strong) | camps, memorials, summit | Must | `Story_Flags_Loop` stand-in | download or recording |
| N1 | River / stream | Dingboche, Thukla | Must | `Story_Stream_Loop` stand-in | download |
| C5 | Snow bridge collapse | hidden crevasses | Must | borrows `Ice_Crack_03` | download, or cut from your ice recordings |
| F1 | Whispers 02–06 | sanity (any place) | Must | synthesized, unsure | remake, or a voice recording by you |
| A5 | Still glacier air, near silence | Icefall; Mist, Eclipse | Must | nothing | download or I make it |
| E4 | Tent zip | camps | Should | nothing | download or recording |
| N7 | Tent fabric in wind (soft and hard) | camps, South Col | Should | nothing | download |
| N10 | Oxygen bottles: clink, valve click | camps, route 2 | Should | nothing | download or recording |
| D8 | Down jacket rustle | movement | Should | nothing | download or recording |
| E3 | Ice axe into ice and snow | Icefall (and B3) | Should | nothing | download, or cut from crampon takes |
| C6 | Falling ice chips | Icefall | Should | nothing | cut from your ice recordings |
| N2 | Yak bells | Dingboche, trail | Should | nothing | download |
| N4 | Far camp voices, no clear words | Base Camp | Should | nothing | download, or voices by you |
| N11 | Spindrift hiss | route 2, storms | Should | nothing | download or I make it |
| D9 | Body falling into snow | falls | Should | nothing | download |
| N13 | Crampons scraping rock | Hillary Step | Should | nothing | download or recording |
| A6, A7, A8, B3 | Weather layers (rumble, shimmer, hum, crackle) | weathers | Should | nothing | I make them (planned in `Sound_Map.md`) |
| N3 | Far bird calls (choughs, ravens) | Gorak Shep, Base Camp | Could | nothing | download |
| N5, N6 | Generator hum, far helicopter | Base Camp | Could | nothing | download (open question 4) |
| N8 | Melt water trickle | Icefall at midday | Could | nothing | download |
| N9 | Ice cave room and drips | ice cave off leg 3 | Could | nothing | download |
| N12 / E2 | Rope ascender clicks | steep ropes (B3) | Could (B3) | nothing | download or recording |

Also: `Mountain_Boom_01–03` (synthesized, rated good) and the chapter 1 stand-ins `Story_Stove_Loop`,
`Story_Radio_Squelch`, `Story_Radio_Static_Loop` and `Story_Plume_Drone` need your verdict in their scenes.

---

## 6. Mix rules

These are starting values. You judge the result by ear; I change numbers, not your picks.

### 6.1 Loudness and order
- **Peak** never above −1 dBFS, measured in the worst case (blizzard + thunder + avalanche + music + voice). Today's
  worst case without music is −0.6 dBFS, so music must sit low or the others step back when it plays.
- **Order, loudest first**: spoken voice → your own body (steps, breath, heart) → spot sounds → wind → music. The one
  exception is the summit cue M4, which may come to the front.
- **Your own steps and breath are never covered**, not even in a blizzard. They are your feedback. The wind may cover
  distant sounds.

### 6.2 Ducking under voice
Ducking = other sounds get quieter for a moment so a voice is clear.
- When a spoken line or a radio call plays: music −9 dB, wind −6 dB, spot sounds −3 dB, breath −4 dB.
  Fade down in 0.2 s, back up 0.8 s after the line ends.
- Built without code: a passive sound mix on the `SC_Voices` class (it applies itself while a voice plays).
- Whispers move to their own class (`SC_Mind`), so they do not trigger the duck.
- The wheel landing is a fate moment too: music −6 dB for 4 s when the wheel lands.
- Until your recorded voices exist, this is tested with the radio sounds.

### 6.3 Altitude thins the high end
Thin air carries sound less well: at the summit the air pressure is about a third of sea level. Cold, dry air also
takes away high sounds over distance. Hypoxia (too little oxygen) dulls the senses. The game uses this as a gentle,
felt cue, not as a simulation.
- **World sounds** (wind, spot sounds, far events) go through one shared low-pass filter whose cut-off follows the
  altitude. Starting values: open up to 5,000 m, about 14 kHz at 6,000 m, 11 kHz at 7,000 m, 9 kHz at 8,000 m, 8 kHz
  at the summit.
- **Not filtered**: your breath, heartbeat, close steps, voices, interface, music.
- **Hypoxia**: when SpO₂ falls below about 75%, the cut-off drops further (towards 4 kHz at 60%) and the world gets
  2–6 dB quieter. "The world goes far away; your body comes close."
- **Body effects setting**: Full = as above; Reduced = half the hypoxia effect; Off = no hypoxia effect (the gentle
  altitude filter stays).
- **Distance**: far events (avalanches, booms) lose more of their high end with distance (air absorption in the
  attenuation settings), as thunder already does.

### 6.4 Wind by weather and by place
- **By weather** (exists): the four wind loops blend by the weather's wind 0–10 and the storm; whistle above 5,600 m in
  strong wind; single gusts above 6.
- **By place** (new): a wind exposure number per stretch. Starting values: Dingboche valley 0.7, Icefall maze 0.6,
  Camp 1 / Western Cwm 0.5, South Col 1.3, summit ridge 1.2 with the whistle wider in stereo.
- **Weather layers** (section 4.6) sit on top of the wind.
- **Never fully silent outside**: when the wind drops, the near-silence bed (A5) takes over, so the quiet itself is
  heard.

### 6.5 Shelter and mask
- **In a tent**: all world sounds are muffled (today only the wind is). The tent fabric is louder inside.
- **Oxygen mask on**: breath gets duller and the regulator hiss joins on each breath in (D7). Mask off: raw, harsher
  breath.

### 6.6 Music rules
- Default is silence. Each cue plays at most once per visit to its place.
- At least 3 minutes of silence between cues (death and summit excepted).
- Fades of 4–8 s. Death cuts the music in 0.5 s.
- Music never starts within 5 s of a big spot sound (avalanche, serac collapse, thunder), and stops before a serac
  warning.

### 6.7 Laptop budget
- At most about 48 sounds at once. Limits per group: own steps 2, cracks 3, groans 2, gusts 1, avalanches 1,
  whispers 1, music 2 loops (more only during a cross-fade).
- Long loops (wind, music, river) stream from disk instead of sitting in RAM (the game is already at 10–12 GB).
- Place sounds (river, flags, camp voices) play only when you are within a few hundred metres.

---

## 7. Music: the four cues and how they are made

| Cue | Where | Length | Feel | Three candidate families to audition |
|---|---|---|---|---|
| M1 "The Valley" | first view of the Icefall at Base Camp; Camp 1 reprise | 60–90 s, once | awe, wide, still | (a) low choir pad (the family of chime L); (b) bowed drone with one low bell (M + C); (c) a soft, low brass swell (the family of K) |
| M2 "The Ice" | the Icefall | 2–3 min loop, 2 layers | held breath, no beat | (a) bowed drone with a slowly rising cluster; (b) hummed voices with no words; (c) bowed metal and low strings |
| M3 "Above 8,000" | 8,000 m to the summit | 3–4 min loop, 3 layers by SpO₂ | fate, beyond human limits, cold | (a) deep male voices (no words) with a bell at long gaps; (b) bowed drone with a very distant long horn (soft: J was "too heavy" up close); (c) a slow brass cluster |
| M4 "The Tree" | the summit beat | 60–90 s, once | the most epic and poetic moment: awe, release, the father | (a) full choir with brass (L + K); (b) bowed drone with the bells of the Norns blooming (M + E); (c) the H direction (bell and distant horns, "so epic") grown into a cue |

No drum that sounds like a heartbeat: the real heartbeat must stay readable.

**The small music system** (built after your picks):
- One music component. One cue at a time, with cross-fades.
- A cue = 1–3 loops (layers) and an intensity value 0–1 that sets the layer volumes (for M3: from SpO₂ and altitude).
- Triggers come from data, not code: a place, an altitude line, a body state, a story beat (a new `music` action for
  the `.ygs` story files), the summit, death.
- Debug commands (to be added): `ygg.Music.Log 1`, `ygg.Music.Play <cue>`, `ygg.Music.Stop`.
- A **Music** sound class and a **Music** slider in the settings.

**Where the music comes from** is your decision (open question 1). The options:
1. I make it, the same way as the wheel sounds you liked (K, L, M): synthesis layered with real recordings. Free,
   fast, and it fits the bells already in the game.
2. Free music downloads (only licences that allow use in a released game).
3. You compose, record, or commission it.
4. An AI music service. Costs credits (your money) and needs a licence check.

---

## 8. Your listening checklist

**How a session works**
- Before each session I prepare 2–4 candidates per missing sound (3 different instrument families per music cue),
  already cut and checked.
- They go into the in-game sound tour as new groups. Each group plays its **scene first** (the sound in context, over
  the live wind), then each candidate alone.
- Start Play, open the console, type the command. Keys: **G** good, **B** bad, **U** unsure (each moves on),
  **Left/Right** step, **Up** next group, **Down** play again.
- Console: `snd` (whole tour), `snd new` (only new or changed), `snd <group number or word>`, `snd note <words>` (a
  note on the last sound you rated), `snd bad <words>`, `snd 0` (end).
- Your verdicts land in `Docs/Development/Sound_Review_InGame.md`. I fix exactly what you mark.
- Mark **good** on every candidate you like. If you like more than one, I ask which wins.
- Helpful commands: `ygg.Audio.Log 1` (prints the wind mix, effort and rope every 2 s), `ygg.Expedition.Go <camp>`
  (jump to a camp), `ygg.Snow.StepSurface 1–4` (force snow, ice, rubble, ladder steps; 0 = real),
  `ygg.Snow.AutoWalk 0.5` (walk by itself).

| # | Session | About | Where and how | What you judge |
|---|---|---|---|---|
| 1 | Dingboche and the trail | 20 min | `ygg.Expedition.Go 0`, walk up the trail | river (N1), yak bells (N2), prayer flags calm (E5), stove and radio stand-ins, the memorial silence |
| 2 | Base Camp | 20 min | `ygg.Expedition.Go 2` | camp voices (N4), tent zip (E4), tent fabric (N7), juniper fire (B3), glacier creak under the camp, optional generator / helicopter (N5, N6), birds (N3); **M1** candidates at the Icefall view |
| 3 | The Icefall and Camp 1 | 25 min | `ygg.Expedition.Go 3`, then `4`; tour groups `snd 4` (ladder), `snd 10` (boom), `snd 11` (serac zone); `ygg.Boom`, `ygg.Avalanche` | still glacier air (A5), ice chips (C6), snow bridge collapse (C5), ice axe (E3), `Mountain_Boom` keep or replace, small random cracks; **M2** candidates on a ladder and near a serac zone |
| 4 | Body and mind | 20 min | best at the South Col (`ygg.Expedition.Go 7`); `ygg.Sanity 20` for low sanity, `snd 12` (whispers); heartbeat files via `Saved/AudioWork/Heartbeat/listen_heartbeats.bat` | heartbeat (F3 / A7), oxygen mask (D7), jacket rustle (D8), fall into snow (D9), whispers 02–06 (F1), the hypoxia filter at three strengths |
| 5 | South Col to the summit | 25 min | `ygg.Expedition.Go 7`; storm with `ygg.Realm Jotunheim` and `ygg.Storm.Strike` | tents flapping hard (N7), bottles (N10), spindrift (N11), crampons on rock (N13), summit flags strong (E5); **M3** and **M4** candidates |
| 6 | Weather layers | 15 min | `ygg.Realm <name>` for each weather (Midgard, Niflheim, Muspelheim, Jotunheim, Alfheim, Vanaheim, Svartalfheim, Hel, Asgard) | A5, A6, A7, A8, B3 against the wind |
| 7 | Final mix | 45 min | everything hooked in. Route 1 walked (or hands-free: `ygg.Bot.Route.Quit 0`, then `ygg.Bot.Route noshots`); route 2 climbed (or `ygg.Bot.Summit.Quit 0`, then `ygg.Bot.Summit`; `ygg.Bot.Summit noo2` to hear the hypoxia death) | balance only: ducking, the altitude filter, music levels, wind by place. Tell me in words or with `snd note` |

Sessions 1–6 can happen in any order and on different days. Session 7 comes last.

---

## 9. Steps after you pick

1. **Download list**: one message with every file (name, source, size, licence) for your OK. Then download to
   `Downloads/`.
2. **Cut and check**: a new build script in `Tools/Audio/` (same rules as `build_game_sounds.py`: whole events, clean
   loops, peak in the first 0.12 s for steps). Contact sheets with `inspect_sounds.py`. Checksums of approved files
   before and after.
3. **Tour groups**: add the candidates to `SoundTourComponent` as new groups (camp, ice, body, route 2, weather,
   music) so you can judge them in the game. Then sessions 1–6.
4. **Import the winners**: `import_game_sounds.py` into `/Game/Audio/` (new folders `Camp` and `Music`). Loops set to
   loop, long loops set to stream.
5. **Classes**: add `SC_Music` and `SC_Mind`; give the `Story` folder its classes; add the Music slider to the
   settings and to the settings self-test.
6. **Place sounds on the route**: a placement script in `Tools/RoutePlanning/` (like `place_rumble.py`) puts river,
   flags, camp voices and tent sounds at their places, each playing only near the player.
7. **Music system**: the music component, the trigger data, and the `music` story action.
8. **Mix**: the voice duck, the shared altitude filter for world sounds, hypoxia muffling tied to the Body effects
   setting, wind exposure per place, the tent muffle for all world sounds, the oxygen-mask breath layer.
9. **Measure**: worst-case peak (must stay under −1 dBFS), audio thread cost, and RAM, on the laptop.
10. **Session 7** with you. Fix exactly what you mark. Update `Sound_Map.md`, then commit.

**Overlap with other blocks**: the heartbeat is block A7. Ascender clicks, ladder wobble and the ice-axe self-arrest
are block B3. Night and headlamp ambience is block B4. B5 uses their sounds but does not build those actions.

---

## 10. Open questions for you

1. **Music source**: I make it (like K, L, M), free downloads, you compose or commission it, or an AI service (costs
   credits)?
2. **How much music**: four cues, each once per route, silence everywhere else? And at Camp 1: a short M1 reprise or
   silence?
3. **Licences**: you said they need no checking while the game is free. Shall I still write down each sound's source
   and licence (two minutes per sound), in case of a later Steam release?
4. **Real Base Camp noise**: the real camp has generators, helicopters and crowds. Include them (more real, more
   tension) or keep the quiet awe?
5. **Tibetan colour in the music**: is a very distant long horn or bell welcome, or should the music avoid Tibetan
   instruments altogether?
6. **Your own recordings**: would you record some of the missing sounds yourself (tent zip, jacket rustle, oxygen
   bottle, whispers in your voice), as you did for the 33 files you gave me?
7. **`Mountain_Boom` and whispers 02–06**: keep my synthesized versions if they pass in their scene, or always look for
   real recordings first?
8. **Hypoxia muffling**: on by default (Body effects: Full), or start at Reduced?
