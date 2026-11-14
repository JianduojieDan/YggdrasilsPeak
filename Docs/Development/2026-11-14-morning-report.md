# Morning report — night of 2026-11-14

Pictures of everything visible: `Saved/Review/morning_2026-11-14.jpg` (title, pack, tents, packaged game, snow
before/after). Every item's timing is in `Docs/Development/2026-11-14-overnight.md`. Everything is committed on local
`main` and the code and docs are pushed to GitHub.

## Done (all 15 items on the list)

| # | Item | Result | Checked by |
|---|---|---|---|
| 1 | **Freezes (2-5 s)** | Cause: the drive with the engine and the project (ORICO NVMe, E: and F:) stalls on writes, about 0.5 s every 10 minutes and sometimes 1-3.6 s. The game wrote files on its main thread, so it froze with the drive. Now nothing in the game touches the disk during play, and loads that used to happen mid-play are done at start. | Same kind of 5-minute session: **1% low 33 -> 66 fps, hitches 14 -> 1.3 a minute, worst frame 76 ms**. The drive stalled twice (212 and 580 ms) and the game did not freeze. |
| 2 | **Title animation** | Any key: the title glides from the centre to the top left and shrinks (1.5 s, smooth, no bounce), then the quote and the buttons come in one by one. A key or click skips it. No replay when you come back from Settings or Credits. Background untouched. | Six frames frozen at fixed moments. Real key presses tested. |
| 3 | **Pack screen** | Your pick: grouped list on the left, the big picture and description on the right, in the HUD's style. | In-game screenshot. |
| 4 | **Clean-up** | Pink cube removed and the spawn point set onto the ground (it was 13.5 m up). 15 unused HUD icons deleted (6 the HUD still draws are kept). StartMenu folder gone: the start map and game instance moved to `UI/Menu`, the old Blueprint menu deleted. | Packaged game starts on the new map. |
| 5 | **Heartbeat** | 13 candidates: 3 I made, 10 cut from free CC0 / public-domain recordings (5 downloaded, 5.6 MB). Each has a 70 and 150 bpm preview. **Listen: `Saved/AudioWork/Heartbeat/listen_heartbeats.bat`**. Nothing is in the game yet. | Your ears. |
| 6 | **Snow** | Diagnosis: the snow had almost no detail between 1 cm and 2 m, the snow texture's grain was mixed in at only 9 %, and it had no height at all (the rock has real geometry). Changes: our own crust grain texture (made by a script, no download), crisp grain near your feet, real 3 cm height on the route tiles, small shadows in the cracks, sharper texture reads with DLSS, sharper ground at a low angle. | Before/after shots. **Your call.** |
| 7 | **Feet on steep slopes** | The hips drop so the downhill boot reaches the slope, and the uphill knee bends. This applies to your own legs and to the body seen in third person and in the shadow. | Measured on a 36° slope: **both boots 5.3 cm into the snow (target)**, hips down 17 cm. |
| 8 | **Route check without walking** | `ygg.RouteCheck` jumps every 200 m. The whole route (148 points, 27.7 km) takes **8 minutes**. No holes, guidance everywhere. It found **14 tents floating 42-109 cm** at Base Camp, Camp 1, 2, 3 and the South Col. They are now lowered onto the snow. | Side screenshot of a tent. |
| 9 | **Start-up stutters** | Covered by item 1: sounds, menu pieces, HUD icons, ghosts and so on are loaded before play. | No frame over 100 ms in play. |
| 10 | **Performance** | Worst spot on the route (start of the Icefall) ran at **39 fps** at 1080p High. Cause: terrain "tessellation" took 17 of 23 ms. It now follows the quality setting (High uses slightly larger micro-triangles): **67 fps**. Epic is unchanged. | Measured runs. |
| 11 | **Oxygen reading** | Real air-pressure model instead of "100 % up to 4,500 m". Resting: Dingboche 84 %, Base Camp 79, Camp 2 73, South Col ~54, summit ~44. With a bottle it stays above 66 everywhere. Getting used to altitude ("adapted altitude") rises while you are high, fastest resting in a camp. It is saved with your game. Without a bottle, walking uphill above ~8,250 m hurts you. `ygg.Physio 0` brings the old rule back. | In-game table (`ygg.Physio.Sweep`). **Play it and tell me if it feels right.** |
| 12 | **Breathing** | Your recorded breathing now follows the blood oxygen, not the altitude: a bottle calms it, and hard work high up makes it heavier. | Code. Listen in play. |
| 13 | **Packaged build** | `F:\UE_project\Builds\2026-11-14\Windows` (run `YggdrasilsPeak\Binaries\Win64\YggdrasilsPeak.exe`). 2,552 packages, 0 cook warnings, 3.4 GB in memory on the mountain. | Started on both maps. |
| 14 | **Code review** | Three independent reviewers. Fixed: the pack swallowing clicks under the death screen, the eye dropping twice as far as the hips on slopes, the route check could overwrite your save, death-zone oxygen tuned on the wrong values. | Rebuilt and re-tested. |
| 15 | This report | | |

## What needs your eye (in this order)

1. **Snow near your feet** (Dingboche in the sun is the best place). Crust plates and fine cracks now show near you. If they look like cracked ground instead of snow, tell me and I will soften them or change their shape. The ripples (5-50 cm wind ripples) are made and imported but not in yet; they come next if you like the direction.
2. **Title animation and pack screen**: start the game normally.
3. **Oxygen and breathing**: climb or camp-jump (`ygg.Expedition.Go 7` = South Col) and see whether the numbers and the breathing feel right.
4. **Heartbeat**: pick one or two from the listener.
5. **Feet**: on a steep slope, look at your shadow; the boots should now touch it.

## Two mistakes I made (both fixed, nothing lost)

- I ran the old ground tool with automatic fixing. It measured wrong tonight and lowered 83,893 rocks and ice blocks by about 5 m. I noticed right away and put every single one back exactly. The route check's pictures show the rocks where they belong. That tool is now marked "do not auto-fix" until I know why it measured wrong.
- I broke my own screenshot script for one run. For a few seconds it sent key presses (Enter, Tab, Esc and two typed console lines) to the Claude app, which was in front. Nothing reached this conversation and no work stopped. The script now only sends keys when the game window is in front. That safety check stopped it twice afterwards, so it works.

## For you to decide

- **The ORICO drive** (E: engine, F: project) is the one that stalls. Your Samsung (C:/D:) stays under 0.2 s. Moving the project (or at least `Saved`) to D:, or updating the ORICO firmware, would help the editor too. That is your call: hardware and system settings are yours.
- The 4 Poly Haven models (about 25 MB) for nicer pack items are still waiting for a yes or no.

## How the night was organised (what I learned)

- One heavy job on the laptop at a time; agents wrote code in parallel in the cloud, then **one** build for a whole batch (38 s). The first batch of 12 agents took 68 minutes; the build, title, pack and feet tests then took 15 minutes in total.
- Testing in the standalone game was faster than the editor. The route check replaced hours of walking with 8 minutes.
- What cost time: my two mistakes (7 minutes), material text with Windows line endings (1 minute), an editor that refuses renames of things named in the config (6 minutes to work around).

## Morning addendum (06:01-06:25, after the usage reset; game and editor were closed)

- **Snow wind ripples are in** (diagnosis step P2): 8-25 cm crests, 1-2 cm high, lying across the prevailing wind,
  on gentle snow only, in fields with calm smooth snow between them. On the route tiles they have real height within
  ~35 m of you, like the rock. Our own ripple texture, no download. Script: `Tools/Materials/snow_ripples_p2c.py`
  (+ `_p2d` tuning).
- **Crust cracks softer**: in shade they drew dark lines like dried mud (the risk I mentioned above). Their near-feet
  boost went from 1.6-1.9x to 1.25-1.45x (`Tools/Materials/snow_crust_p2e.py`), and a sign error in the crust's
  lighting is fixed (it lit cracks like ridges, against the real height).
- Pictures: `Saved/Review/snow_ripples_2026-11-14.jpg` (last night vs now, sun and shade). **Your call**: if the
  ripples are too strong, too busy or in the wrong places, tell me and it is one number each.
