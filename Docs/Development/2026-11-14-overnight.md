# Overnight 2026-11-14 — work log and timings

The user asked for every item's time to be written down, and for a workflow that puts the least strain on the laptop
while saving time. Times are local (UTC+6). "Machine" = minutes the laptop spent building/compiling/running.

## How tonight is organised (decided before starting)

What strains the laptop: the C++ build (all cores, 3-8 min), the editor (10+ GB RAM, shader compile workers on all
cores), a game run (GPU), packaging (cook, 30-60 min), Blender. What does not: agents reading and writing code
(they run in the cloud), Python analysis, the browser.

1. **One heavy process at a time.** Never editor + game + build together. Close each when done.
2. **Batch the C++.** Agents write the code for several items in parallel (separate files each), a second agent
   checks each change against the engine headers, then ONE build for the whole batch instead of one per item.
3. **Batch the editor.** All asset work (cube, icons, StartMenu move, materials) in one editor session, by remote
   Python, while the agents are writing code. Then close the editor before the build.
4. **Test in the standalone game, not the editor** (`-game -windowed 1280x720 -unattended`): faster to start, less RAM
   and GPU. Several checks per launch (menus, pack, route) via console commands and existing scripts.
5. **No walking for route checks**: a console command jumps point to point every 200 m.
6. **Packaging once**, at the end.
7. A disk probe (4 KB/s, `Tools/Monitor/disk_probe.ps1`) runs all night to catch the drive that stalls.

## Timings

| Item | Start | End | Total | Machine | Notes |
|---|---|---|---|---|---|
| Setup + plan + freeze analysis | 01:07 | 01:13 | 6 min | 0 | disk stalls on the ORICO NVMe (E: engine, F: project) = 100% busy, ~0.1 MB/s, other programs stalled too |
| Agents write batch 1 (7 in parallel) | 01:13 | | | 0 | freezes, title, pack, feet, route check, heartbeat, snow diagnosis |
| Clean-up in the editor (one session) | 01:14 | 01:20 | 6 min | 6 (editor) | cube gone, PlayerStart onto the ground (was 13.5 m up), 15 unused icons deleted (6 the HUD draws kept), StartMenu -> UI/Menu (L_MainMenu, BP_YggGameInstance); editor refused renames of assets named in config: had to change config first, then copy + replace references |
| Heartbeat candidates | 01:13 | 01:33 | 20 min | 1 | agent synthesised 3 + found 7 free recordings but would not download (approval was in my chat, not its brief); I downloaded 5 (5.6 MB, CC0/PD) and cut 10 beats; listener Saved/AudioWork/Heartbeat/listen_heartbeats.bat |
| Agents batch 1 finished (12 agents, all reviewed) | 01:13 | 02:21 | 68 min | 0 | longest: feet + route-check reviews; 3.9 M agent tokens |
| ONE build for the whole batch | 02:21 | 02:22 | 38 s | 0.6 | succeeded first try (freezes, title, pack, feet, route check, StartMenu paths) |
| Title glide test (6 frozen frames) | 02:23 | 02:25 | 2 min | 2 | works: centre -> top-left, quote, staggered buttons; background untouched |
| Pack + feet tests | 02:25 | 02:35 | 10 min | 8 (4 runs) | pack = pack_hud_2; feet 36 deg snow: both boots -5.3 cm (target), hips -17 cm; boots not in view looking down even with the old path (camera geometry, not a regression) |
| Snow P0+P1 in the editor | 01:46 | 01:50 | 4 min | 3 | first patch half-applied (CRLF in node text) -> fixed in 1 min; both materials compile clean |
| Snow detail textures (agent) | 01:47 | 02:20 | 33 min | 0.5 | own procedural crust + ripple maps, no download |
| Route check, whole route | 02:35 | 02:44 | 9 min | 8 | 148 points / 27.7 km in 8.1 min (walking would take hours): no holes, guidance everywhere, 14 tents 42-109 cm up, ~80 rocks near Dingboche (64 only measured roughly by bounds) |
| Snow P2 (crust maps, tile height, glints) + P2b (stronger near) | 02:45 | 02:57 | 12 min | 6 (2 editor sessions + 4 shots) | P2 alone was too faint (only visible contrast-stretched) -> P2b exaggerates near slopes 1.6-1.9x + crack AO |
| 14 floating tents seated | 02:52 | 02:57 | 5 min | 1 | lowered by measured gap + 3 cm, side shot: shadows attached |
| Monitored 5-min session (real keys) | 03:04 | 03:10 | 6 min | 5 | 76 fps median, 1% low 66, worst 76 ms, 1.3 hitches/min (before: 1% low 33, 14/min, 2-5 s freezes); disk stalled 212 + 580 ms during play without freezing the game |
| MISTAKE: menu_shots launch line broken by my edit | 02:59 | 03:03 | 4 min | 0 | the null window passed the old check and the script's keys (Enter, Tab, Esc, two typed console lines) went to the Claude app in front; fixed: keys only when the game window is in front |
| MISTAKE: ground pass with APPLY | 03:10 | 03:13 | 3 min | 2 | it lowered 83,893 rock/ice instances by a median ~5 m (wrong measurement; the route check and its pictures show them seated); all put back exactly within 2 min (new tool for instance undo) |
| Performance pass | 03:14 | 03:23 | 9 min | 7 (6 perf runs + build) | worst route spot (Icefall start) 39 fps at 1080p High: tessellation 17 of 23 GPU ms; dicing by tier (High 8 px) -> 67 fps |
| Oxygen physiology model | 03:24 | 03:37 | 13 min | 4 (2 builds + 2 test runs) | first test: a camp jump to the South Col arrived unacclimatised (33.9 %, instant damage) -> camp jumps/continues adapt to the camp, trekker floor 0.5 |
| Breathing follows SpO2 | 03:39 | 03:41 | 2 min | 0 | existing recorded phrases only |
| Packaged build | 03:38 | 03:42 | 4 min | 4 | Development Win64 -> F:\UE_project\Builds\2026-11-14 (15 min last time; the cook reused the cache); 2,552 packages, 0 cook warnings; packaged game runs on both maps, 3.4 GB resident |
| Loader warnings in the packaged game | 03:43 | 03:45 | 2 min | 1 (build) | numbered sound sets / backdrops asked for files that do not exist |
| Push code + docs to GitHub | 03:45 | 03:47 | 2 min | 0 | worktree robocopy + push (56 files) |
| Independent code review (3 agents) | 03:42 | 03:59 | 17 min | 0 | threads/IO: none; UI: 1 (pack under death page); body: 3 (doubled eye drop, death-zone tuning on wrong values, route check could write the save) |
| Review fixes + verify | 03:59 | 04:05 | 6 min | 2 (build + sweep run) | menu_shots could not get the game window in front twice -> used a start-up command instead of keys |
| Morning report | 04:05 | 04:10 | 5 min | 0 | Docs/Development/2026-11-14-morning-report.md + Saved/Review/morning_2026-11-14.jpg |
| Snow "before" shots + references | 01:21 | 01:27 | 6 min | 3 (2 game runs) | first two shots were under the ground (route.json z is not ground height) |
