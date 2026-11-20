# Design study: what Chomolungma can learn from Steep, PEAK and four other games

Written 2026-11-20 · draft 1 · for the thesis supervisor's request (2026-10-26: "study Steep and PEAK").
Related blocks in `Development_Plan.md`: **B3** climbing feel, **B4** camps loop, **A7** body feedback,
**B6** player study. Proposals here are **[Proposed]** until the user decides.

**How this was made.** I read reviews, community wikis, developer interviews and one GDC write-up (sources are
listed under each game and again at the end). I have **not** played these games myself. Facts about the games come
from those sources. Opinions about what fits Chomolungma are mine. The user should check the key points by playing
(see "Open questions").

**Words used here**
- *Stamina*: how much hard effort the body can still give right now.
- *Affliction*: a bad state (cold, hunger, injury) that lowers what the body can do.
- *Diegetic*: shown inside the game world (a breath, trembling hands), not on a screen overlay (HUD).
- *Asynchronous*: other players leave things in your world, but you never meet them live.
- *Checkpoint*: the place the game sends you back to after a failure.
- *Within-subject test*: every player tries both versions (A and B), in a swapped order for half of them.

---

## 0. The short answer

1. **PEAK's best idea is one stamina bar with many causes.** Hunger, cold, weight and injury each take a coloured
   piece of the same bar, so the player sees *why* they are weak. Chomolungma's physiology (SpO₂, cold, exhaustion,
   food, water) can be shown the same way. This is the single most useful thing to borrow.
2. **PEAK also shows that company works when people *depend* on each other**, not when they only walk side by side.
   For us this means Nima (the partner) must be able to help the player and need help, with a real cost.
3. **Steep treats the mountain with respect and makes it a place to read.** Binoculars, a map of your own tracks,
   and an instant, honest report after a crash. We can adapt all three in a realistic way.
4. **Jusant and Cairn show how to make climbing feel physical**: Jusant with a two-hand rhythm and anchors as
   checkpoints, Cairn with a body that trembles and pants instead of a health bar. Cairn also shows the danger:
   slow limb-by-limb climbing would make a 14-hour climb tiring in the wrong way.
5. **Death Stranding proves that walking can be the game** when the ground, the load and balance matter, and when
   traces of other people (ladders, ropes) appear in your world. That fits a Sherpa who carries loads and walks on
   ropes and ladders that other people fixed.

---

## 1. Our starting point

What makes Chomolungma different from all the games below:

| Topic | Chomolungma today |
|---|---|
| World | The real Everest route at 1:1 scale, Dingboche to the summit. A full climb must last at least 14 hours. |
| Player | Sangye, a young Sherpa, in first person with a visible body (arms, ice axe, legs). |
| Body | A physiology model: SpO₂ (blood oxygen), heart rate, core temperature, stamina, food, water, health (`Docs/Thesis/Physiology_Model.md`). Bottled oxygen. A death zone above 8,000 m. |
| Body on screen | Tunnel vision, lens frost, swimming view, faint waves, shiver, heartbeat camera pulse; HUD altimeter and air temperature; setting "Body effects: Full / Reduced / Off". |
| Climbing | Fixed ropes with clip-in (steeper ground allowed while clipped), icefall ladders, camps. Rope ascending, ladder balance and self-arrest are planned (B3). |
| Company | Single player. Company levels per chapter: crowd, partner (Nima), voice only (Dr. Hale on radio), alone. |
| Design rule | Tension over poetry: survival numbers and decisions matter most. Solitude is allowed in parts, not the whole climb. |
| Thesis | RQ3 asks: do players find the physiology believable, and does it change their decisions (rest, descend, use oxygen)? |

The six questions used for every game:
**(1) climbing and walking feel, (2) stamina, (3) camera, (4) social or co-op presence, (5) failure,
(6) how altitude and cold are shown.**

---

## 2. Steep (Ubisoft Annecy, 2016)

**What it is.** An open-world winter sports game set in the Alps around Mont Blanc, later with Alaska (Denali),
Japan and Korea. You ski, snowboard, paraglide and fly a wingsuit. All players share one online world. Metacritic
about 72 ("mixed or average"); about 10 million players by 2019 (Wikipedia).

**Core mechanics, by question**
1. *Feel*: the sports are the core. The physics sit between realistic and arcade (IGN, via Wikipedia). You can
   move on foot, but walking is only a way to reach a start point.
2. *Stamina*: the sources describe no stamina, cold or altitude system. The body is never the limit.
3. *Camera*: first or third person; GoPro-style angles in races.
4. *Social*: one shared world; you can see and collide with other players (collisions can be switched off). You can
   share replays and set your own line down the mountain as a challenge for others. Your movement is recorded and
   shown on the mountain map.
5. *Failure*: a crash costs little. You retry at once, and the game shows the g-force your body took.
6. *Altitude and cold*: shown only through scenery: wind, snow, huge views. In "Mountain Stories" a narrator speaks
   *as the mountain* and tells its history (PopMatters, GamingTrend).

**What works and why**
- *The mountain as a place you respect, not an enemy.* PopMatters argues that Steep makes the mountain feel like a
  teacher that pushes you to improve. Its dangers are "passive": rocks, drops, trees. This is close to our art
  direction ("quiet awe").
- *Reading the mountain.* "Mountain view" and binoculars let you discover places by looking. Looking becomes a
  game action.
- *Your own tracks on the map.* Seeing the line you took turns a session into a story you can review.
- *Failure that teaches.* The g-force number after a crash tells you *what* happened in one figure.

**What does not fit a realistic 1:1 Everest**
- Fast travel to any "drop zone" would remove the walk that is the point of our game.
- Trick scores, medals and arcade physics clash with "tension over poetry".
- Crashes without cost: on Everest a fall or a slow mistake must matter.
- Reviewers criticised the lack of direction and the always-online rule (Wikipedia). We need a clear goal on every
  stretch and an offline game.

**Sources**: [Wikipedia: Steep](https://en.wikipedia.org/wiki/Steep_(video_game)) ·
[PopMatters: "Steep Loves Its Mountains"](https://www.popmatters.com/steep-loves-its-mountains-2495397348.html) ·
[GamingTrend preview](https://gamingtrend.com/previews/steep-preview-skys-the-limit/) ·
[Official site](https://steep.ubisoft.com)

---

## 3. PEAK (Aggro Crab and Landfall, 2025)

**What it is.** A co-op climbing game for 1 to 4 players. Scouts crash on an island and must climb to its peak.
Made in a one-month game jam in Seoul (PC Gamer); over 10 million copies sold (GamesRadar). The island changes every
day at 17:00 UTC. Tone: bright and comic.

**Core mechanics, by question**
1. *Feel*: almost any surface can be climbed by holding a button. The climbing itself is simple; the challenge is
   managing stamina (Aftermath).
2. *Stamina*: **one bar** that climbing, sprinting and lunging use. Afflictions fill the same bar from the end and
   shrink the usable part. Each has its own colour and symbol: hunger (yellow), injury (red), weight of carried items,
   cold, heat, poison, drowsiness, curse (PEAK wiki "How to play"; Millman). Some afflictions fade by themselves
   (cold, hunger, poison); others stay until treated (injury needs bandages; weight stays while you carry).
3. *Camera*: first person with the scout's hands in view (my understanding; the sources I read do not describe the
   camera, so check by playing). Friends are easy to recognise by their custom colours (Millman).
4. *Social*: proximity voice chat (you hear friends only when they are near). Real mutual help:
   - *Helping hand*: with empty hands you can pull up a friend who is climbing below you.
   - *Boost*: a friend stands on you and you jump; they rise without spending stamina.
   - *Rope spools* placed by one player help everyone who follows.
   - Food and healing items can be used on friends.
   - *Carry a friend* who passed out; this pauses their death timer but costs you a lot of weight.
   - Anyone who climbs more than 160 m ahead of the second-highest player is chased by a monster (the old
     scoutmaster) and thrown back towards the group (Millman). The game punishes leaving friends behind.
5. *Failure*: falls add injury. At zero stamina you pass out; friends have limited time to save you. After that you
   become a ghost. An "Ancient Statue" at each campfire can bring ghosts back, but they come back hungry and cursed.
   If everyone arrives alive, the statue gives a rare item instead (Millman). Rising fog adds cold and works as a
   soft time limit; it can be turned off (Aftermath, wiki). Harder "Ascent" levels raise fall damage, hunger speed,
   and end with permadeath.
6. *Altitude and cold*: the island is a stack of zones (shore, tropics, alpine, mesa, caldera...), each with its own
   danger. The alpine zone brings blizzards that add cold; higher difficulty adds cold nights. Campfires separate the
   zones and are the safe points.

**What works and why**
- *One bar, many causes.* The player always sees *why* they are weak, and every choice (eat the unknown berry? carry
  the rope?) becomes a trade on the same bar. The GDC write-up calls the stamina bar the game's "story engine".
- *Dependence makes company matter.* Because a friend can pull you up the last metre, staying together is the best
  plan, not just a social rule. The game gives in-world reasons too: the scouts' "Rule 0: never abandon a friend".
- *Failure with a cost, not a full stop.* Passing out, then a short rescue window, then a costly revive keeps
  everyone playing and creates stories.
- *Zones and campfires give rhythm*: danger, then a safe point, then a new kind of danger.

**What does not fit a realistic 1:1 Everest**
- Climbing any surface: the real route is fixed ropes, ladders and walking, not free rock climbing.
- The comic tone, monsters and magic items.
- A new random mountain every day: our mountain is real and fixed.
- PEAK is about 4 human friends; ours is single-player with computer-controlled companions. The help must come from
  Nima and other climbers, not from friends on voice chat.

**Sources**: [PEAK wiki: Peak (game)](https://peak.wiki.gg/wiki/Peak_(game)) ·
[PEAK wiki: How to play](https://peak.wiki.gg/wiki/How_to_play) ·
[Aftermath impressions](https://aftermath.site/peak-climbing-impressions-review) ·
[Z. Millman, "Is This Peak?", GDC Game Narrative Review 2026 (student review, PDF)](https://media.gdcvault.com/gdc2026/GNR/Papers/Zac+Millman+-+GDCReview_2026_Peak_Millman.pdf) ·
[PC Gamer: made in a 4-week jam](https://www.pcgamer.com/games/sim/peak-devs-say-the-hit-comedy-climber-was-pitched-in-a-swedish-hot-tub-and-developed-in-a-frantic-4-week-korean-game-jam-we-brought-our-computers-to-an-airbnb-in-hongdae-and-locked-tf-in-for-a-month) ·
[PC Gamer: developer interview on "friendslop"](https://pcgamer.com/games/adventure/peak-boss-doesnt-mind-us-calling-it-friendslop-why-not-make-a-game-where-the-point-is-to-hang-out-with-your-friends) ·
[GamesRadar: sales](https://gamesradar.com/games/co-op/after-a-few-months-of-work-led-to-11-million-copies-sold-on-steam-peak-devs-embrace-what-many-companies-refuse-to-learn-were-not-going-to-continually-have-a-graph-go-up)

---

## 4. Jusant (Don't Nod, 2023)

**What it is.** A short (about 5–6 hours) single-player climbing game up a huge tower in a world that lost its water.
Game Informer 9/10.

**Core mechanics, by question**
1. *Feel*: each shoulder button grips with one hand; you climb by alternating left and right. Reviewers call it
   natural and realistic without being hard to control (Game Informer).
2. *Stamina*: gripping drains a stamina meter; resting at certain spots gives some back. Rushing without pauses makes
   you fall to the end of your rope (Well Played).
3. *Camera*: third person; looking down at what you climbed and up at what remains is a big part of the feeling.
4. *Social*: no other people. Their absence is the story: letters, wall paintings, abandoned homes. A small creature
   (Ballast) is your only company.
5. *Failure*: a rope always holds you. Up to three pitons (metal spikes) act as anchors and checkpoints; a slip sends
   you back only to the last one. Reviewers felt lost progress was their own fault for poor piton choice. Digital
   Trends notes there is hardly any fail state.
6. *Altitude and cold*: height through views, wind and changing zones; no cold.

**What works and why**
- *Rhythm in the hands.* Two buttons for two hands turns climbing into a physical rhythm. Our planned ascender
  ("jumar") rhythm on fixed ropes can work the same way.
- *Anchors as checkpoints you choose.* The player decides where safety is; failure feels fair.
- *Absence as story.* Letters and empty homes tell what happened without cutscenes. This fits Sangye's search for his
  father.

**What does not fit**
- No real danger. Everest must be able to kill you, mostly slowly and through decisions.
- Free placement of pitons anywhere: on Everest the anchors are fixed by the rope teams; you clip to them.

**Sources**: [Game Informer review](https://www.gameinformer.com/review/jusant/reaching-the-summit) ·
[Well Played review](https://www.well-played.com.au/jusant-review/) ·
[Digital Trends review](https://www.digitaltrends.com/gaming/jusant-review-xbox-series-x/)

---

## 5. Cairn (The Game Bakers, 2026)

**What it is.** A single-player mountain climbing game with survival. The climber, Aava, climbs a huge peak called
Kami. Well reviewed (PC Gamer 91, via search), with complaints about clarity.

**Core mechanics, by question**
1. *Feel*: you place each hand and foot one at a time. An automatic mode picks the least-strained limb; a manual mode
   gives precision (Noobfeed). Some holds that look solid break. One reviewer compared it to *QWOP* (a famously hard
   limb-control game) (Dawn).
2. *Stamina*: stamina, hunger, thirst, temperature and tiredness all affect performance. Health and exhaustion are
   shown mainly through the body: heavy breathing, trembling limbs, sounds, not a big HUD (Noobfeed, Dawn).
3. *Camera*: third person; the rock fills the screen and makes Aava look small. You can zoom out to see your route on
   the mountain.
4. *Social*: mostly alone. A small robot (Climbot) helps. Other climbers appear through what they left: broken gear,
   empty camps, last messages, and a body found early in the game (Dawn, Noobfeed).
5. *Failure*: falls are sudden and hard. Too many falls or no food means death and a restart at the last save; one
   mistake can cost 20–30 minutes (GamesCreed, via search). Pitons stop falls but are limited and can break.
   Difficulty modes: Explorer (can rewind a bad move), Alpinist (standard), Free Solo (very hard); survival can be
   switched off.
6. *Altitude and cold*: weather, wind, breaking ice, night camps and cooking; meals give short boosts such as cold
   protection. You can set up camp and wait out bad weather.

**What works and why**
- *The body tells you its state.* Trembling and panting are readable without numbers. Reading these signs is the
  skill. This is exactly the goal of our body effects (A7).
- *Camp as a decision.* Stop and wait out the storm, or push on. Food as a short-term tool, not only a bar to refill.
- *Difficulty modes that change rules, not just numbers* (rewind on/off).

**What does not fit**
- Limb-by-limb control: right for a 5-hour rock climb, too slow for a 14-hour walk-and-rope climb. Reviewers already
  called it slow and tedious at times.
- Reviewers found stamina feedback hard to read on the wall (Ladies Gamers). A body-only display needs a backup.
- Losing 20–30 minutes on one mistake is too harsh for a climb of 14+ hours.

**Sources**: [Noobfeed review](https://www.noobfeed.com/reviews/cairn-review) ·
[Dawn review](https://www.dawn.com/news/1980987) ·
[The Outerhaven review](https://www.theouterhaven.net/cairn-review-survive-the-climb/) ·
[Top Gear review](https://www.topgear.com/car-news/gaming/cairn-review-2026-has-already-peaked-spellbinding-climbing-game) ·
[Ladies Gamers review](https://ladiesgamers.com/cairn-review/) ·
[GamesCreed review](https://www.gamescreed.com/reviews/cairn-review)

---

## 6. Death Stranding (Kojima Productions, 2019): walking as the game

**What it is.** A third-person game about a porter, Sam, who carries cargo across a broken country on foot.

**Core mechanics, by question**
1. *Feel*: the weight and packing of the load change how Sam moves. On rough ground or in rivers he loses balance;
   the player leans and braces with the shoulder triggers. Turning too fast on bad ground makes him stumble and drop
   cargo (Den of Geek, Wikipedia).
2. *Stamina*: a stamina gauge and a "patience" gauge. Steep ground (over 45°) and moving water drain patience; when it
   runs out, Sam falls or is swept away (Samurai Gamers guide).
3. *Camera*: third person behind the shoulder.
4. *Social*: asynchronous. Ladders, climbing ropes, bridges, signs and lost cargo left by other players appear in
   your world; you give each other "likes". You never meet (Wikipedia; God is a Geek).
5. *Failure*: mostly a slow loss: damaged cargo, stumbling, time lost. Not instant death.
6. *Altitude and cold*: snowy mountain stretches feel hard through slope, deep snow and load, not through a body model.
   A scanner marks safe, rough, steep and impassable ground.

**What works and why**
- *Walking becomes a skill* when the ground matters: every slope and stream is a small decision.
- *Load as a trade-off.* More cargo means more value and more danger.
- *Traces of other people* make a lonely world feel shared. A ladder left by a stranger is a gift.
- A Sherpa's real job is carrying loads up to camps on fixed ropes and ladders laid by others. Death Stranding is the
  closest game to that fantasy.

**What does not fit**
- The scanner overlay: on Everest the player should read the snow, ice and rope with their own eyes.
- Fetch quests: reviewers called parts of the game bloated and slow (Polygon, via Wikipedia). Carrying must stay a
  choice with meaning, not a chore.
- Placing your own ladders anywhere: the icefall route is fixed by the "icefall doctors" (the Sherpa team that sets
  the ladders each season).

**Sources**: [Wikipedia: Death Stranding](https://en.wikipedia.org/wiki/Death_Stranding) ·
[Den of Geek review](https://www.denofgeek.com/games/death-stranding-review) ·
[Samurai Gamers: terrain guide](https://samurai-gamers.com/death-stranding/surveying-sensor-terrain-guide) ·
[God is a Geek: beginner tips](https://www.godisageek.com/2019/11/10-beginners-tips-for-reconnecting-america-in-death-stranding) ·
[GamesRadar: connections and community](https://www.gamesradar.com/death-stranding-makes-you-understand-the-value-of-connections-and-community-projects)

---

## 7. Two short notes

**A Short Hike (Adam Robinson-Yu, 2019).** A small, gentle hike to a summit. Stamina is a row of golden feathers used
for climbing, sprinting and gliding; finding more feathers lets you climb higher. There is one simple goal: reach the
peak. The developer drew on his own hiking trips (Wikipedia). *Lesson*: one clear goal, ideally one you can see, pulls
the player forward. Everest's summit and the notch landmark can do the same. *Not for us*: stamina as a collectable
upgrade.
[Wikipedia](https://en.wikipedia.org/wiki/A_Short_Hike)

**Insurmountable (ByteRockers' Games, 2021).** A map-based mountain-climbing roguelike (each run starts fresh). It
models oxygen and body warmth: oxygen use rises sharply in the death zone, cold drains warmth, a tent has limited uses
and sleeping in the open is very bad. *Lesson*: altitude and cold as resource trade-offs work even in a simple game.
*Not for us*: map-based play and run-based restarts.
[Softpedia](https://www.softpedia.com/reviews/games/pc/insurmountable-review-535224.shtml) ·
[TechRaptor](https://techraptor.net/gaming/reviews/insurmountable-game-review) ·
[Finger Guns](https://fingerguns.net/reviews/2021/05/06/insurmountable-review-pc-peak-practice/)

---

## 8. Side-by-side comparison

| Question | Steep | PEAK | Jusant | Cairn | Death Stranding | **Chomolungma (goal)** |
|---|---|---|---|---|---|---|
| Feel | sports physics | hold to climb anything | two hands, rhythm | limb by limb | balance + load | walk, clip, ascend, cross ladders |
| Stamina | none | one bar, coloured causes | bar, rest spots | many needs, shown by the body | stamina + patience | physiology model (SpO₂, cold, effort) |
| Camera | 1st or 3rd | 1st, hands (to check) | 3rd | 3rd, zoom-out | 3rd | 1st, visible body |
| Company | shared online world | 4 friends who depend on each other | absence, letters | absence, traces, robot | traces of strangers | partner, radio, other teams, alone in parts |
| Failure | instant retry + g-force | pass out → rescue → costly revive | rope, last piton | hard falls, last save | slow loss | graded, readable, decision-based |
| Altitude / cold | scenery only | cold zone, fog | height only | weather, food, camp | slope, snow, load | real altitude, air temperature, body effects |

---

## 9. Synthesis: borrow, adapt, avoid

**Borrow** = take the idea almost as it is. **Adapt** = keep the idea but change its form to fit a realistic Everest.
**Avoid** = do not use, and why.

| # | Idea | From | Verdict | Block | What it would be in Chomolungma |
|---|---|---|---|---|---|
| 1 | One stamina bar; each cause takes a coloured piece of the maximum | PEAK | **Borrow** | A7, HUD | Low SpO₂, cold, hunger/thirst, load and exhaustion each take a labelled piece of the stamina bar. Bottled oxygen visibly gives a piece back. |
| 2 | The body shows its state: trembling, panting | Cairn | **Borrow** | A7 | Already partly built (shiver, sway, tunnel). Add: hands and ice axe tremble at low stamina, breathing sound tied to SpO₂ and heart rate (A7 work item), heartbeat sound. Keep the HUD as backup (Cairn's clarity problem). |
| 3 | Two-hand rhythm | Jusant | **Adapt** | B3 | Ascender rhythm on steep fixed ropes: one input slides the jumar up, the other steps. A bad rhythm costs extra effort, which feeds the physiology. |
| 4 | Anchors as checkpoints | Jusant | **Adapt** | B3 | On fixed ropes, a slip stops at the last anchor you clipped through. You choose when to clip and when to rest at an anchor; skipping clips is faster but riskier. |
| 5 | Balance with load on rough ground | Death Stranding | **Adapt** | B3, B4 | Ladder crossings sway more with a heavy pack, high fatigue or wind (already listed in `Chapter_Design_v1.md`). Two inputs to steady yourself, no scanner overlay. |
| 6 | Helping hand and carry a friend, with a cost | PEAK | **Adapt** | B3, story | Nima can catch a slip or short-rope you down when close (Chapter_Design_v1). The player can help too: carrying or guiding a weak client costs stamina and time. This is the Balcony choice (save Elena or go up) made playable. |
| 7 | Do not leave your team behind | PEAK (160 m rule) | **Adapt** | story, B4 | No monster. Instead: climbing far ahead of Nima loses his warnings and his help, and the radio crackles. A real reason to stay close. |
| 8 | Pass out → rescue window → costly revive | PEAK | **Adapt** | B4, failure | Graded failure: stumble → slip (self-arrest chance) → injury (lowers max stamina, like PEAK) → collapse (Nima or a team brings you down: time and acclimatisation lost) → death (restart at last camp). |
| 9 | Campfires between zones | PEAK | **Adapt** | B4 | Camps between the real zones (Khumbu valley, icefall, Western Cwm, Lhotse Face, death zone). Camps are safe points, save points and decision points. |
| 10 | Wait out the weather at camp; food as short boosts | Cairn | **Borrow** | B4 | Stove melts snow for water; hot food warms the core for a while; waiting costs days of the weather window. |
| 11 | Fog as a soft time limit | PEAK | **Adapt** | B4, A5 | The real time limit: turnaround time and the weather window. Show a turnaround clock; ignoring it is allowed but dangerous. |
| 12 | Binoculars and reading the mountain | Steep | **Adapt** | B4, B7 | From Base Camp or camps, watch the route, the weather and other teams' headlamps through binoculars (real climbers do this). No fast travel. |
| 13 | Your own tracks on the map, crash report | Steep | **Adapt** | B6, A3 | After a failure or a summit, a one-page "body report": your route with SpO₂, core temperature and heart rate along it, and markers ("oxygen empty 12:40"). Uses the A3 logger. |
| 14 | Traces of other people | Death Stranding, Steep, Cairn, Jusant | **Adapt** | B1, B6, story | Footprints, headlamps, dropped bottles and fixed ropes left by other teams. The other teams can come from virtual-climber runs (B1). Letters and photos about Sangye's father, Jusant-style. |
| 15 | Difficulty modes that change rules | Cairn, PEAK Ascents | **Adapt** | later | For example: Trekker (forgiving, can rewind a fall), Sherpa (standard), Without oxygen (hard). Not needed for the thesis build. |
| 16 | A summit you can see from the start | A Short Hike | **Borrow** | B7 | Keep the summit and landmarks in view on long walks to pull the player forward. |
| 17 | The mountain speaks | Steep | **Ask first** | story | Chomolungma is a goddess in a living religion. A narrator voice "as the mountain" may be disrespectful. Safer: the father's sayings, prayer flags, Nima's stories. |
| 18 | Free climbing on any surface | PEAK | **Avoid** | — | The real route is ropes, ladders and walking. |
| 19 | Limb-by-limb control | Cairn | **Avoid** | — | Too slow for 14+ hours; reviewers found it tedious. |
| 20 | Fast travel anywhere, trick scores | Steep | **Avoid** | — | Removes the walk and its decisions. Camp jumps stay only as a chapter or demo tool. |
| 21 | Death without cost or no death | Steep, Jusant | **Avoid** | — | Everest must be able to kill you. |
| 22 | Overlay scanner for ground | Death Stranding | **Avoid** | — | The player should read snow and ice by eye (our snow and footprints work supports this). |
| 23 | Comic tone, monsters, magic items | PEAK | **Avoid** | — | Clashes with the story of Sangye and his father. |
| 24 | Carrying as a fetch-quest chore | Death Stranding | **Avoid** | — | Load ferrying only where it creates a real decision. |
| 25 | Bodies of real climbers as props | Cairn (a body as a story beat) | **Avoid** | — | Real bodies on Everest are real people. Only fictional characters (the father). |

**By block, in one line each**
- **B3 climbing feel**: #3 ascender rhythm, #4 anchors as checkpoints, #5 ladder balance, #6 Nima catches you,
  #8 slip → self-arrest → injury.
- **B4 camps loop**: #9 camps as zone boundaries, #10 stove and waiting, #11 turnaround clock, #12 binoculars,
  #8 rescue with a cost.
- **A7 body feedback**: #1 one bar with causes, #2 trembling hands, breathing and heartbeat sound.
- **B6 player study**: #13 body report, #14 traces, and the five experiments below.

---

## 10. Five experiments for the player study (B6, run in C1)

**Common design.** 10–20 players, one session of about 45–60 minutes each. Each experiment compares two versions
(A and B) of the same short stretch (6–8 minutes each). Within-subject: every player tries both versions; half see A
first, half see B first, so learning does not decide the result. We can probably run **2 or 3** of the five in one
session; choose in B6.

**Measures used in several experiments**
- Telemetry from the A3 logger: SpO₂, heart rate, core temperature, stamina, speed, rests, oxygen use, turnarounds.
- A short presence questionnaire, for example the **IPQ** (Igroup Presence Questionnaire, 14 items, free:
  http://www.igroup.org/pq/ipq/index.php), or parts of the **GEQ** (Game Experience Questionnaire,
  IJsselsteijn, de Kort and Poels 2013).
- The **SSQ** (Simulator Sickness Questionnaire, Kennedy et al. 1993) where camera effects are involved.
- Our own 1–7 items: "I understood why my body felt weak", "The body felt believable", "I felt alone", "I felt bored".

### E1 · One bar with causes vs separate numbers (PEAK → A7, RQ3)
- **Question**: Does showing causes as pieces of one stamina bar help players understand their body and act sooner?
- **A**: today's HUD (separate values). **B**: one stamina bar where SpO₂, cold, food/water, load and exhaustion take
  labelled pieces.
- **Stretch**: Camp 3 → South Col, or the icefall test patch.
- **Measures**: three short pauses per run with one question, "Why are you slow right now?" (multiple choice, % right);
  time from SpO₂ falling below a set level to resting or using oxygen; the "I understood" item.
- **Expect**: B gives more correct answers and faster correct actions.
- **Cost**: one new HUD widget (about 0.5–1 day).

### E2 · Feel it vs read it (Cairn → A7, RQ3)
- **Question**: Do the body effects make the physiology more believable and change decisions, and at what comfort
  cost?
- **A**: Body effects Full. **B**: Body effects Off (HUD warnings stay).
- **Stretch**: South Col → Balcony (death zone).
- **Measures**: "believable" item, IPQ, SSQ; number of rests, oxygen use, lowest SpO₂ reached, turnarounds.
- **Expect**: A is rated more believable and players rest or use oxygen earlier; SSQ may rise a little.
- **Cost**: almost nothing; the setting already exists. **The cheapest experiment; run it for sure.**

### E3 · With a partner vs alone (PEAK → story, climb design rule D4)
- **Question**: Does a partner who can help (and give advice) reduce boredom and change safety decisions?
- **A**: Nima climbs with the player: waits, warns ("your breathing is fast, sit down"), catches one slip.
  **B**: the same stretch alone.
- **Stretch**: an icefall section or Camp 2 → Camp 3.
- **Measures**: "alone" and "bored" items, IPQ; distance to the partner over time; how often advice is followed;
  rests, falls, time.
- **Expect**: A is less boring and safer. Risk to watch: players may follow Nima blindly (less agency).
- **Cost**: needs a basic partner. The existing route walker (`ygg.Bot.Route`) could be the start of one.

### E4 · Body report after failure vs plain death screen (Steep → B6, RQ3)
- **Question**: Does a short body report after a failure help players explain what went wrong and climb differently
  next time?
- **A**: plain death or turnaround screen. **B**: one-page body report (route, SpO₂ / core temperature / heart rate
  over time, markers for key moments).
- **Stretch**: South Col → summit with limited oxygen, so most players fail once (a planned failure; tell players
  failure is normal and expected).
- **Measures**: "In your own words, why did you fail?" (answers scored against the logged cause); changes in the
  second attempt (earlier oxygen, pace, turnaround); frustration item.
- **Expect**: B gives more correct explanations and more changed strategies.
- **Cost**: about 1 day (the data already exists in the logger).
- **Strongest link to RQ3** ("does the physiology change decisions?").

### E5 · Traces of other climbers vs an empty route (Death Stranding, Steep → B1, B6)
- **Question**: Do traces of other climbers (from virtual-climber runs) make the route feel less lonely and change
  the player's pace and risk?
- **A**: empty route. **B**: footprints, distant headlamps, a dropped oxygen bottle, and one ghost climber ahead whose
  pace comes from a virtual-climber run.
- **Stretch**: Western Cwm or the summit ridge at night.
- **Measures**: "alone" item, IPQ; pace compared with the ghost; risk (time above 8,000 m, lowest SpO₂); "the other
  climbers felt real" item.
- **Expect**: B feels less lonely; some players will copy the ghost's pace, good or bad. This links the AI part (B1)
  to the player experience.
- **Cost**: medium; needs B1 outputs and a way to place traces.

**Reserve idea**: turnaround clock shown vs hidden (PEAK fog → B4). Do players turn back on time when they can see the
clock?

**Recommendation**: E2 and E4 for sure (cheap and directly about RQ3). Then E1 if the HUD change is done, or E3 if Nima
exists by March.

---

## 11. Draft for the thesis related-work section (plain; for the user to rewrite)

> Games about mountains handle the body in very different ways. *Steep* (Ubisoft, 2016) recreates real Alpine peaks
> but has no model of the body: the mountain is a playground, and failure is an instant retry with a g-force readout.
> *PEAK* (Aggro Crab and Landfall, 2025) puts every limit of the body into one stamina bar. Hunger, cold, injury and
> carried weight each take a visible part of it, so players always see why they are weak. Its co-op design makes
> players depend on each other: they pull, boost and carry their friends. *Jusant* (Don't Nod, 2023) and *Cairn*
> (The Game Bakers, 2026) make climbing physical through hand and limb control; Cairn shows exhaustion through the
> climber's breathing and trembling instead of numbers. *Death Stranding* (Kojima Productions, 2019) shows that
> walking itself can be the game when load, balance and terrain matter, and that traces left by other players make a
> lonely world feel shared. None of these games uses a physiology model based on real altitude, and none is set on a
> real mountain at 1:1 scale with realistic oxygen and cold. This thesis takes the legibility of PEAK's stamina bar
> and Cairn's bodily feedback, and connects them to a physiology model driven by real altitude. A player study then
> tests whether this feedback is believable and changes decisions.

For the academic version, add sources on game feel (S. Swink, *Game Feel*, 2009), failure in games (J. Juul,
*The Art of Failure*, 2013) and presence (the IPQ papers). Reviews and wikis are fine for describing the games, but
the supervisor may want fewer of them.

---

## 12. Open questions for the user

1. **Play PEAK for one session?** The supervisor named it, it is cheap, and my notes come only from reviews. One hour,
   ideally with a friend, would let you judge points 1, 6 and 8 yourself. (Your money and account, your call.)
2. **The mountain's voice (#17)**: is a narrator "as the mountain" acceptable in a game about a living religion, or
   should this stay with the father's sayings and Nima's stories?
3. **Help from Nima (#6)**: may Nima physically save the player (catch a slip, short-rope down), or does that feel
   too easy for a realistic game?
4. **Failure cost (#8)**: restart at the last camp, or "rescued with a cost" (time and acclimatisation lost) as the
   normal outcome, and death only after that?
5. **Study choice**: agree with E2 + E4 as the core of the March study?

---

## All sources

Steep: https://en.wikipedia.org/wiki/Steep_(video_game) · https://www.popmatters.com/steep-loves-its-mountains-2495397348.html ·
https://gamingtrend.com/previews/steep-preview-skys-the-limit/ · https://steep.ubisoft.com

PEAK: https://peak.wiki.gg/wiki/Peak_(game) · https://peak.wiki.gg/wiki/How_to_play ·
https://aftermath.site/peak-climbing-impressions-review ·
https://media.gdcvault.com/gdc2026/GNR/Papers/Zac+Millman+-+GDCReview_2026_Peak_Millman.pdf ·
https://www.pcgamer.com/games/sim/peak-devs-say-the-hit-comedy-climber-was-pitched-in-a-swedish-hot-tub-and-developed-in-a-frantic-4-week-korean-game-jam-we-brought-our-computers-to-an-airbnb-in-hongdae-and-locked-tf-in-for-a-month ·
https://pcgamer.com/games/adventure/peak-boss-doesnt-mind-us-calling-it-friendslop-why-not-make-a-game-where-the-point-is-to-hang-out-with-your-friends ·
https://gamesradar.com/games/co-op/after-a-few-months-of-work-led-to-11-million-copies-sold-on-steam-peak-devs-embrace-what-many-companies-refuse-to-learn-were-not-going-to-continually-have-a-graph-go-up

Jusant: https://www.gameinformer.com/review/jusant/reaching-the-summit · https://www.well-played.com.au/jusant-review/ ·
https://www.digitaltrends.com/gaming/jusant-review-xbox-series-x/

Cairn: https://www.noobfeed.com/reviews/cairn-review · https://www.dawn.com/news/1980987 ·
https://www.theouterhaven.net/cairn-review-survive-the-climb/ ·
https://www.topgear.com/car-news/gaming/cairn-review-2026-has-already-peaked-spellbinding-climbing-game ·
https://ladiesgamers.com/cairn-review/ · https://www.gamescreed.com/reviews/cairn-review

Death Stranding: https://en.wikipedia.org/wiki/Death_Stranding · https://www.denofgeek.com/games/death-stranding-review ·
https://samurai-gamers.com/death-stranding/surveying-sensor-terrain-guide ·
https://www.godisageek.com/2019/11/10-beginners-tips-for-reconnecting-america-in-death-stranding ·
https://www.gamesradar.com/death-stranding-makes-you-understand-the-value-of-connections-and-community-projects

Others: https://en.wikipedia.org/wiki/A_Short_Hike ·
https://www.softpedia.com/reviews/games/pc/insurmountable-review-535224.shtml ·
https://techraptor.net/gaming/reviews/insurmountable-game-review ·
https://fingerguns.net/reviews/2021/05/06/insurmountable-review-pc-peak-practice/

Study tools: IPQ http://www.igroup.org/pq/ipq/index.php · GEQ: IJsselsteijn, W. A., de Kort, Y. A. W., Poels, K.
(2013), *The Game Experience Questionnaire*, TU Eindhoven · SSQ: Kennedy, R. S., Lane, N. E., Berbaum, K. S.,
Lilienthal, M. G. (1993), *International Journal of Aviation Psychology* 3(3), 203–220.

**Caveat on sources**: most game facts come from press reviews and community wikis, which can be out of date after
patches. The PEAK GDC document is a student narrative review, not a developer talk. Details marked "via search" were
seen in search summaries only and should be checked before the thesis cites them.
