# Yggdrasil's Peak — Development Plan (thesis edition)

Version 2.0 · written 2026-11-13 · replaces the 5 October 2026 plan (deleted; still in git history).
The project is the user's senior thesis. This plan gets a **thesis version** done by the defense in May 2027.
The commercial (Steam) version comes after graduation and is only listed (section 8), not scheduled.

---

## 0. How to use this plan (Claude: read this first, every session)

1. Read this file, then the status board (section 3).
2. Take the first block whose status is **Ready** (all its dependencies **Done**). One block at a time.
3. Work it until its **Done when** is met. Collect the **Evidence** (numbers, screenshots, logs).
4. Commit, push code and docs (section 9), set the block to **Done** on the board, add one line to the log (section 10).
5. Ask the user only for what the block lists under **You**, batched into one message, as late as possible.
6. Something new comes up (a bug, an idea, a user request)? Put it in the backlog (section 11) or add a block.
   Work outside the plan only when the user asks for it.
7. Estimates are in **Claude working days** (one full session). Re-estimate a block if it runs 50% over, and say so.

Status words: **Ready** · **Waiting** (on a dependency or on the user) · **In progress** · **Done** · **Later**.

---

## 1. Goal, deadlines, thesis

| Date | Milestone | What must exist | Internal deadline |
|---|---|---|---|
| 2026-12-17/18 | **Interim check** (30–40% shown to supervisor and university) | Phase A done | **2026-12-14** |
| 2027-03 (date TBD) | **Title demonstration** | Phase B done | **2027-03-01** |
| 2027-04 (date TBD) | **Pre-defense** | Phase C done | **2027-04-05** |
| 2027-05 (date TBD) | **Defense**, graduation end of May | Phase D done | **2027-05-03** |

Fill in the real university dates when the user has them; then move the internal deadlines to 3 days before each.

**Thesis topic** (supervisor, 2026-10-26):
- Main: a **physiology model** for high-altitude climbing (thin air, cold, exhaustion) and its evaluation with a
  **"virtual climber"**.
- Foundation: the **geodata pipeline** that turned a real elevation model (DEM) into a playable 1:1 Everest.
- Plus: **an approach using AI** (proposed here: the virtual climber is an AI agent whose climbing strategy is
  optimised by search; to confirm with the supervisor).
- Also: keep a **cloud backup**; related games to study: **Steep** and **PEAK**.

**Draft research questions** (the user and supervisor decide the final wording):
- RQ1: Can a real-time physiology model (SpO₂, heart rate, core temperature, exertion, acclimatisation) driven by real
  altitude reproduce known high-altitude patterns?
- RQ2: Does an automated virtual climber running that model produce outcomes consistent with real Everest expedition
  statistics (for example the effect of bottled oxygen, or of time spent above 8,000 m)?
- RQ3: Do players find the physiology feedback believable, and does it change their decisions (rest, descend, use oxygen)?

**The thesis build** = two playable stretches of the real route plus the evaluation tools:
1. Dingboche → Everest Base Camp → Khumbu Icefall → Camp 1 (grounded survival, the icefall).
2. South Col → Summit (the death zone), reached by a camp jump.
3. The physiology model, data logger, virtual climber, comparison with real data, player study.

---

## 2. Roles

**Claude (about 95%)**: all code, content, art integration, tools, builds, testing, measuring, screenshots, video,
figures, data analysis, and drafts of the technical text.

**You (about 5%)**: play and judge as a real player (every visual and feel decision is yours), one short test session
per block with the checklist in section 12, approve downloads and anything needing an account or payment, meet the
supervisor, and answer the few questions marked **DECIDE**.

Honest limits, outside the 5%:
- **The thesis text is academically yours.** I can draft technical sections and make every figure, within your
  university's rules on AI use; you write, edit and own the final text. Check those rules early (block A1).
- **The player study needs you in the room** (recruiting 10–20 people, running sessions: about 1–2 days in March).
- Logins, payments and legal acceptances are always yours.

---

## 3. Status board

| ID | Block | Phase | Days | Depends on | Status |
|---|---|---|---|---|---|
| A0 | Cloud backup of the whole project | A | 0.5 | — | Ready (needs your cloud space) |
| A1 | Thesis scope page and supervisor questions | A | 0.5 | — | Ready |
| A2 | Physiology model v1: write-up, calibration, acclimatisation | A | 4 | A1 | Waiting |
| A3 | Physiology data logger and charts | A | 1.5 | — | Ready |
| A4 | Demo route 1: Dingboche → Camp 1 | A | 6 | — | Ready |
| A5 | Demo route 2: South Col → Summit | A | 2 | — | Ready |
| A6 | Demo blockers: floating on slopes, compass in storms, backpack | A | 3 | — | Ready |
| A7 | Body feedback finish: heartbeat sound, breathing, comfort setting | A | 1 | — | Ready |
| A8 | Interim package: build, video, figures, report drafts | A | 3 | A2–A7 | Waiting |
| A9 | Buffer and fixes | A | 2 | — | — |
| B1 | Virtual climber (the AI approach) | B | 9 | A2, A3 | Waiting |
| B2 | Comparison with real expedition and physiology data | B | 4 | B1 | Waiting |
| B3 | Climbing feel: rope ascending, ladders, self-arrest | B | 7 | A4 | Waiting |
| B4 | Camps, rest and acclimatisation loop; night and headlamp | B | 4 | A2 | Waiting |
| B5 | Sound and music pass on the demo route | B | 5 | A4, A5 | Waiting |
| B6 | Player study preparation | B | 2 | A3 | Waiting |
| B7 | Visual polish of the demo route | B | 5 | A4, A5 | Waiting |
| B8 | Title-demonstration package | B | 2 | B1–B7 | Waiting |
| C1 | Run the player study | C | 3 | B6, B8 | Waiting |
| C2 | Study analysis | C | 3 | C1 | Waiting |
| C3 | Simple summit ending | C | 5 | A5 | Waiting |
| C4 | Thesis figures, appendices, technical drafts | C | 4 | B2, C2 | Waiting |
| C5 | Pre-defense package | C | 2 | C1–C4 | Waiting |
| D1 | Fixes from pre-defense feedback | D | 5 | C5 | Waiting |
| D2 | Final build, video, presentation material, archive | D | 4 | D1 | Waiting |

**Capacity**: Phase A 23.5 days for 31 calendar days to the internal deadline. Phase B 38 days over about 10 weeks
(holidays included). Phase C 17, Phase D 9. Total about 88 Claude days. My throughput is limited by usage limits
and by your test sessions, not by calendar days: keep test sessions short and quick to schedule.

---

## 4. Phase A — Interim (now → 2026-12-14)

Goal: show a working thesis core. The physiology model is written down and calibrated, it records data, two route
stretches play cleanly, and there is a build, a video and figures for the report.

### A0 · Cloud backup of the whole project
- **Goal**: the project survives the laptop dying. GitHub only holds code (LFS quota full); 14 GB of Content exists
  only on this disk.
- **Work**: `Tools/Backup/backup_project.ps1` mirrors Content, Art, Imports, Docs, Source, Tools, Config, Build and the
  .uproject (about 15 GB) to a cloud-synced folder, by default `E:\One_Drive_File\OneDrive\YggdrasilsPeak_Backup`.
  Leave out what can be rebuilt or downloaded again: Intermediate, Saved, DerivedDataCache, Binaries, Downloads,
  the NVIDIA plugins, .git. The script logs the size, file count and time. Run it weekly and before every milestone.
- **Done when**: the backup folder matches the source (file count and size), and the cloud client shows it synced.
- **Evidence**: the log line; a screenshot of the synced folder.
- **You**: **DECIDE** which cloud (OneDrive needs about 20 GB free; the free plan has 5 GB). 1 minute.
- **Days**: 0.5 · **Depends on**: —

### A1 · Thesis scope page and supervisor questions
- **Goal**: one page your supervisor can approve, so nothing outside it gets built before May.
- **Work**: `Docs/Thesis/Thesis_Scope.md`: working title, the research questions, the thesis build (section 1), the
  evaluation plan, what is out of scope until after graduation, and the questions for the supervisor:
  1. What exactly is due on 17–18 December: a report, a demo or both, and in what format?
  2. Is the virtual climber with an optimised strategy acceptable as "the AI approach"?
  3. Does the evaluation need human players? Is ethics approval needed?
  4. What are the university's rules on AI-assisted work, and how should it be disclosed?
- **Done when**: the page exists; you have taken it to the supervisor; the answers are written into it.
- **You**: the supervisor meeting (your time, not counted in the 5%).
- **Days**: 0.5 · **Depends on**: —

### A2 · Physiology model v1: write-up, calibration, acclimatisation
- **Goal**: the model is documented, explainable and calibrated against published data. This is the thesis core.
- **Work**:
  - `Docs/Thesis/Physiology_Model.md`: every variable, unit, equation, parameter and its source. Code:
    `Source/YggdrasilsPeak/Survival/SurvivalComponent.cpp` (oxygen, cold, stamina, damage), `SurvivalConfig.h`
    (parameters), `ExpeditionComponent.cpp` UpdateScreenEffect (heart rate, body effects), `RealmSky.cpp`
    GetAirTemperature (air).
  - Replace the linear "thin air" term with barometric pressure from altitude → inspired oxygen → SpO₂, keeping the
    effort and slope terms. Calibrate to published SpO₂-by-altitude values for unacclimatised and acclimatised climbers.
  - **Acclimatisation**: a slow state that grows with time spent high (faster while resting at a camp), shifts the
    SpO₂ curve, decays lower down, and is saved in the save game.
  - Bottled oxygen as a flow-dependent lowering of the "felt" altitude, calibrated against published values.
  - Heart rate (added 2026-11-13): document it and calibrate it against published data.
  - Console sweep `ygg.Physio.Sweep` → CSV of SpO₂ and heart rate at every camp altitude: resting or walking, with
    or without oxygen, acclimatised or not.
- **Done when**: the write-up has references for every number; the sweep matches the calibration targets within the
  tolerance stated in the write-up; normal play still works (no instant deaths, the climb is possible).
- **Evidence**: sweep CSV and chart; a short play log; the commit.
- **You**: nothing required (reading the write-up is optional).
- **Days**: 4 · **Depends on**: A1 (scope confirmed; can start before the supervisor answers)

### A3 · Physiology data logger and charts
- **Goal**: every play session produces data and charts. These become the results figures.
- **Work**:
  - `ygg.Log 1`: once per second to `Saved/Telemetry/session_<date>_<time>.csv`: time, position, altitude, speed,
    slope, effort state, SpO₂, heart rate, core temperature, stamina, food, water, sanity, bottle state, sheltered,
    realm, air temperature, and events (death, camp, clue, summit).
  - `Tools/Thesis/plot_session.py`: altitude profile with physiology over time, SpO₂ against altitude, and a summary
    table.
  - On in development builds by default, off in builds for players (setting or console variable).
- **Done when**: a 10-minute session produces its CSV and charts with no manual steps.
- **Evidence**: example CSV and charts in `Saved/Review`.
- **You**: —
- **Days**: 1.5 · **Depends on**: —

### A4 · Demo route 1: Dingboche → Base Camp → Icefall → Camp 1
- **Goal**: the first stretch plays from start to end without anything broken, ugly or confusing.
- **Work**:
  - An automated walker along `Tools/RoutePlanning/route.json` with ground traces and screenshots every 200 m, to
    find floating objects, holes, falls through the ground, dead ends and missing guidance. Fix what it finds with
    the placement tools in `Tools/Placement`.
  - Icefall: extend the test patch (`Tools/Art/place_icefall_patch.py`, about 70% of the reference look) over the
    whole stretch the route crosses; ladders and crevasses playable; collision right.
  - Guidance: rope, flags, clues and the HUD's next-camp column agree.
  - Performance: at least 60 fps at 1080p High on the laptop at the worst spot (`Tools/Art/perf_run.ps1` at 5 spots).
  - Time a full walk; target 15–25 minutes.
- **Done when**: the automated walker and one real play both reach Camp 1 without getting stuck; the FPS numbers
  meet the budget; there is a screenshot sheet of the route.
- **Evidence**: walker log, perf numbers, screenshot sheet.
- **You**: one test play (about 25 min, checklist in section 12) and the visual call on the icefall.
- **Days**: 6 · **Depends on**: —

### A5 · Demo route 2: South Col → Summit (the death zone)
- **Goal**: the death zone plays and shows the physiology at its strongest.
- **Work**: camp jump to the South Col (`ygg.Expedition.Go`); route and fixed ropes to the summit checked; the body
  effects and the death-zone message play; the summit is reached and the World Tree is visible; a short summit beat
  (camera hold, subtitle, no cinematic); the way back down works.
- **Done when**: summit reachable in 20 minutes or less with bottled oxygen, and fatal without it if you stay too
  long; screenshots of each stage.
- **You**: one test play (about 20 min).
- **Days**: 2 · **Depends on**: —

### A6 · Demo blockers
- **Goal**: nothing in the demo looks broken.
- **Work**:
  - Character floating on steep slopes (backlog #7, #24): feet onto the ground (foot IK or mesh offset).
  - Compass unreadable in storms (#66).
  - Backpack screen: you called it "bad"; make two different proposals as mockups and build the one you pick.
  - The pink cube at spawn is your marker: **DECIDE** keep or remove for the demo.
- **Done when**: each is fixed and shown before/after; you accept the backpack.
- **You**: pick a backpack proposal; answer about the cube.
- **Days**: 3 · **Depends on**: —

### A7 · Body feedback finish
- **Goal**: the death-zone feel built on 2026-11-13 is complete and comfortable.
- **Work**: a heartbeat sound (needs a download, your OK first) tied to the heart rate; breathing tied to SpO₂ and
  heart rate (`MountainAudioComponent`); a setting **Body effects: Full / Reduced / Off** (warnings stay in the HUD);
  check the shiver, sway and pulse in motion.
- **Done when**: sounds play at the right moments; the setting works; you have heard and felt it.
- **You**: OK the download; judge the sound by ear (`Tools/Audio/listen_sounds.bat`).
- **Days**: 1 · **Depends on**: —

### A8 · Interim package
- **Goal**: everything for 17–18 December.
- **Work**:
  - A packaged build, tested from the start to Camp 1 and from the South Col to the summit.
  - A 3–5 minute video recorded with ffmpeg from the game window (scripted route, body states, HUD, data charts).
  - Figures: pipeline diagram (DEM → terrain → route tiles), physiology model diagram, session charts (A3),
    sweep charts (A2).
  - Drafts of the technical parts of the interim report (system overview, pipeline, physiology model, first results)
    for you to rewrite in your own words, following the university's AI rules.
- **Done when**: the build runs on a clean start; the video, figures and drafts are in `Docs/Thesis/Interim/`.
- **You**: watch the video; present to the supervisor.
- **Days**: 3 · **Depends on**: A2–A7

### A9 · Buffer and fixes
- **Goal**: absorb what goes wrong. Untouched buffer moves to Phase B.
- **Days**: 2

---

## 5. Phase B — Title demonstration (2026-12-19 → 2027-03-01)

Goal: the evaluation exists (virtual climber plus comparison with real data), the demo route feels like climbing,
and the player study is ready to run.

### B1 · Virtual climber (the AI approach)
- **Goal**: an automated climber that uses the same physiology model and climbs the real route many times with
  different strategies. This answers RQ2 and is the AI part.
- **Work**:
  - `AVirtualClimber`: an AI-controlled climber with the same `USurvivalComponent`, following the route and camps.
  - A strategy (policy) with parameters: pace, rest thresholds (for example rest when SpO₂ is below X), bottled oxygen
    use, acclimatisation rotations ("climb high, sleep low"), turnaround time.
  - **Two modes**: in the engine (real terrain, sped-up time) for credibility, and a fast simulation of the same
    equations for thousands of runs. The fast mode is checked against the engine mode.
  - Strategy optimisation: an evolutionary search over the policy parameters (explainable, cheap) to find the best
    strategy for a given goal (summit and survive).
  - One CSV row per run: success, total time, time above 8,000 m, lowest SpO₂, cause of death.
- **Done when**: 1,000+ runs per strategy give stable statistics; the two modes agree within a stated tolerance; the
  optimised strategy beats the hand-made ones.
- **You**: —
- **Days**: 9 · **Depends on**: A2, A3

### B2 · Comparison with real expedition and physiology data
- **Goal**: show where the model matches reality and where it does not (RQ1, RQ2).
- **Work**: collect published aggregate statistics and measurements with full citations in
  `Docs/Thesis/References.md`; `Tools/Thesis/compare.py` makes the comparison figures and tables (for example
  success and death rates with and without bottled oxygen; SpO₂ by altitude against measured values).
- **Done when**: every comparison has a cited source and an honest discussion of the differences.
- **You**: check with the supervisor that the sources are acceptable.
- **Days**: 4 · **Depends on**: B1

### B3 · Climbing feel on the demo route
- **Goal**: the route feels like climbing, not walking.
- **Work**: rope ascending on steep fixed ropes with an ascender rhythm (Lhotse Face style, used on the demo
  stretches); ladder crossings with balance and wobble; a slip that can be stopped with the ice axe; every action
  feeds effort into the physiology model.
- **Done when**: each action works start to end, can fail with a clear cause, and recovers; you have played it.
- **You**: one test play; feel decisions.
- **Days**: 7 · **Depends on**: A4

### B4 · Camps, rest and acclimatisation loop; night and headlamp
- **Goal**: the physiology creates decisions (RQ3).
- **Work**: resting at a camp builds acclimatisation and recovers; going high then sleeping low pays off; the summit
  push starts at night with a headlamp; the stove turns snow into water at camps.
- **Done when**: a climber who acclimatises does measurably better (logger plus virtual climber); you understand
  why without reading the code.
- **You**: one test play.
- **Days**: 4 · **Depends on**: A2

### B5 · Sound and music pass on the demo route
- **Goal**: no placeholder sounds on the demo route; music that carries the feeling (epic, poetic, fate).
- **Work**: replace the placeholders on both stretches (`Docs/Development/Sound_Map.md`); 3–4 music cues (calm,
  tension, death zone, summit) with a small music system; a mix pass.
- **Done when**: you have listened to everything and accepted it.
- **You**: OK each download; judge by ear.
- **Days**: 5 · **Depends on**: A4, A5

### B6 · Player study preparation
- **Goal**: the study can run in March without surprises.
- **Work**: protocol, questionnaire (presence/immersion, perceived physiology, decisions), consent text, telemetry
  tags for study sessions, an analysis script that runs on the study data.
- **Done when**: a dry run with test data produces the results tables.
- **You**: ethics or approval paperwork if the university needs it.
- **Days**: 2 · **Depends on**: A3

### B7 · Visual polish of the demo route
- **Goal**: the demo route looks finished.
- **Work**: the open visual issues on those stretches (glacier look, shadows, snow), one dawn sky moment, and a
  performance check after every change.
- **Done when**: you accept the screenshot sheet; FPS still meets the budget.
- **You**: visual calls.
- **Days**: 5 · **Depends on**: A4, A5

### B8 · Title-demonstration package
- **Work**: build, video, figures, slide content.
- **Done when**: everything is in `Docs/Thesis/TitleDemo/`.
- **You**: present.
- **Days**: 2 · **Depends on**: B1–B7

---

## 6. Phase C — Pre-defense (2027-03 → 2027-04-05)

### C1 · Run the player study
- **Work**: study builds, session scripts, automatic data collection (telemetry and questionnaire), backups.
- **Done when**: data from all participants is stored and checked.
- **You**: recruit 10–20 players and run the sessions (about 1–2 days; outside the 5%).
- **Days**: 3 · **Depends on**: B6, B8

### C2 · Study analysis
- **Work**: statistics, figures and tables answering RQ3; honest limitations.
- **Days**: 3 · **Depends on**: C1

### C3 · Simple summit ending
- **Goal**: the game ends with meaning, without the full three cinematic endings.
- **Work**: at the World Tree, one choice scene (text, camera, music), then the end and credits.
- **Done when**: reachable from the South Col; you have played it.
- **You**: **DECIDE** which single choice (from the original setting's three endings) to use.
- **Days**: 5 · **Depends on**: A5

### C4 · Thesis figures, appendices, technical drafts
- **Work**: final figures, data appendices, drafts of the method and results chapters for you to write up.
- **Days**: 4 · **Depends on**: B2, C2

### C5 · Pre-defense package
- **Work**: build, video, slide content.
- **Days**: 2 · **Depends on**: C1–C4

---

## 7. Phase D — Defense (2027-04 → 2027-05-03)

### D1 · Fixes from pre-defense feedback
- **Days**: 5 · **Depends on**: C5

### D2 · Final build, video, presentation material, archive
- **Work**: final build; final video; presentation material; a full backup and an archive of every thesis artefact.
- **Days**: 4 · **Depends on**: D1

---

## 8. Later — after graduation (the commercial version)

Not scheduled before May. From the 2026-11-13 audit (game about 35% done by effort; 155–235 working days to a
commercial PC release):
- Story: opening village, the priest, the parents' story through chapters 3–4 (frozen creatures, gravity anomaly,
  gold dust, the dragon), the summit temple, the **three cinematic endings**, NPC dialogue.
- World: set pieces for the Western Cwm, Lhotse Face, Yellow Band and Geneva Spur; the rest of the route's content.
- Systems: rockfall and avalanche gameplay; rappel and skiing; equipment, durability and injuries; map and journal log.
- Release: localisation (Chinese and English), Steam features (achievements, cloud saves), store page and trailer,
  console ports (needs a company, dev kits and certification).

---

## 9. Working rules (standing, from the user and from experience)

- **Looks and feel are the user's call.** I verify by measurement and screenshots; the user decides what looks or
  feels right. Never call a visual done without looking at it at full size.
- **Aesthetics first.** Anything added must look good, not just be placed correctly.
- **Performance first.** Laptop target, Steam audience. Budget: at least 60 fps at 1080p High at the worst spot of the
  demo route. Measure before and after any heavy change (`Tools/Art/perf_run.ps1`). No brute-force fixes.
- **Downloads** only with the user's OK, into `F:\UE_project\YggdrasilsPeak\Downloads\<pack>\`, never C:. Name the
  file, source and size when asking.
- **Sounds**: examine every sound for its scene, cut sequences into single pieces and loops, and let the user judge by
  ear before it goes in.
- **C++ builds** need the editor and game closed: `E:\UE5.8\UE_5.8\Engine\Build\BatchFiles\Build.bat
  YggdrasilsPeakEditor Win64 Development -Project=F:\UE_project\YggdrasilsPeak\YggdrasilsPeak.uproject -WaitMutex
  -NoHotReload`. I save, close and relaunch the editor myself.
- **Editor Python**: `python -I Tools/RoutePlanning/uepy.py <script>`; never put ".py" in a script's first comment lines
  (the editor then treats the text as a path).
- **Screenshots**: `Tools/Art/shot.ps1` (numbers as `-Name:value`; `-UI` for the HUD), `Tools/Art/review_shots.ps1`.
  Wait for Nanite streaming before judging "floating".
- **Git**: commit everything on local `main`; code and docs reach GitHub only through the worktree
  `F:\UE_project\YggdrasilsPeak_github` (robocopy Source, Config, Docs, Tools and the .uproject, then
  `git -C <worktree> push origin github-code:main`). **Never push `main`** (LFS quota full).
- **Voice input**: the user dictates; misheard words happen. Ask when a word matters.
- **Play-test reports** from the user go into the backlog (section 11) with the date; fix them inside the block they
  belong to.
- After every block: status board, log line, commit, push, memory note if something non-obvious was learned.

---

## 10. Log

One line per finished block: date · ID · what was delivered · evidence path · commit.

- 2026-11-13 · plan · Development plan v2.0 written; old plan deleted.

---

## 11. Backlog (reported issues and ideas, sorted into blocks)

| # | Date | Item | Block |
|---|---|---|---|
| 7, 24 | 2026-11-03 / 11-06 | Character floats on steep snow slopes | A6 |
| 66 | 2026-11-13 | Compass unreadable in storms | A6 |
| — | 2026-11-13 | "Bad backpack": pack screen needs a new arrangement | A6 |
| — | 2026-11-13 | No heartbeat sound | A7 |
| 12, 32, 33 | 2026-11-03 / 11-07 | Glacier and icefall look | A4, B7 |
| 5, 13, 18, 25 | 2026-10/11 | Hard, stepped shadows on ridges and the summit | B7 |
| 26–31 | 2026-11-07 | Camp props and tents part buried or floating; tent collision | A4 (route 1), A5 (route 2) |
| — | 2026-11-13 | 21 icon textures in /Game/UI/HUD/Icons6 unused (halos, tree, mountain, snow, wind, summit) | Delete with the user's OK |

---

## 12. Your test sessions (about 15–25 minutes each)

When a block asks for a test, I send: the build or the editor map to open, where to start (camp jump command), what to
look at, and these questions. Answer in a few words, voice is fine.

1. Did anything look broken, floating, ugly or out of place? Where (a screenshot helps)?
2. Did you always know where to go?
3. Did anything feel wrong to control?
4. Did the body (breathing, heartbeat, view, HUD) match what you expected at that moment?
5. One thing you would change first.
