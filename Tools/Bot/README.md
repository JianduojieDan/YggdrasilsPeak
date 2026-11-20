# Robot player

The standalone game plays a chapter by itself and writes a report: story flow, places where the climber gets stuck,
the camera under abuse, FPS. Code: `Source/YggdrasilsPeak/Survival/YggPlayBot.h/.cpp` (`UYggPlayBot`, a world subsystem).

## Run it

Close the editor first (one heavy process at a time), then:

```
powershell -File Tools/Bot/run_bot.ps1                      # chapter 1, normal walk
powershell -File Tools/Bot/run_bot.ps1 -Fast                # t.MaxFPS 0, walks at sprint
powershell -File Tools/Bot/run_bot.ps1 -Extra "ygg.Bot.CapMin 30, ygg.Bot.Camera 0" -TimeoutMin 45
```

The script starts `UnrealEditor.exe <uproject> /Game/LevelPrototyping/base_map -game -windowed -ResX=1280 -ResY=720
-nosplash -NoSound -unattended -YggNoSave -YggMonitor -YggCamRec -log=Bot.log -ExecCmds="ygg.Later 3 ygg.Bot.Run chapter1"`, waits
for the game to quit (or `-TimeoutMin`, default 95), puts the save back, and prints the report path, the summary and
beat table, and the end of the report. Exit code 0 = the chapter finished. `-YggCamRec` keeps the camera flight
recorder (`ygg.CamRec`, Saved/Monitor/incidents/) on although `-YggNoSave` would switch it off; `-NoCamRec` drops it.
The bot's own look input is reported to the recorder (`UYggPlayBot::GetLookFeed`), so its sweeps and down / up looks are
not taken for spins; the recorder still flags a pitch past +-90 or roll whatever the bot does.

In a running game (or PIE) the console command works too: `ygg.Bot.Run chapter1 [fast]`, `ygg.Bot.Status`,
`ygg.Bot.Stop` (final report, no quit).

## What it does

- Starts the story: `ygg.Story.Start Chapter1` with `ygg.Story.Auto 1` (choices, cards, inspects, press prompts and
  hand-holds answer themselves) and `ygg.Story.Debug 1` (so the log says when a beat is done).
- Reads `Content/Story/<Chapter>.ygs` (+ its `places_file`). The walk target is the first beat, in file order, that has
  not fired, whose `after` beats are done, and that waits `near` a place or a person. It walks there with
  `AddMovementInput` until it is inside 60% of the beat's radius. A beat that wants a posture (sit, kneel, crawl) is left
  to the story, except that a lying climber gets a held move input until he sits up (the rig's Lie -> Sit; the input
  stops the frame the wanted posture is reached); `posture: stand` is reached by walking (that is what stands the
  climber up). Crawling, it steers to the `tent_out` / `tent_door` place first (the rig stands up only at the exit).
  No stuck teleports while lying, sitting or kneeling.
- Path: a straight line first. After 8 s without progress it follows `Tools/RoutePlanning/route.json` leg 0 instead
  (`ygg.Bot.Path 1` = the route whenever the target is more than 60 m away). After 10 s without progress it also
  sidesteps for 2 s every 6 s.
- Stuck: no progress for `ygg.Bot.StuckSec` (25 s) -> logged, teleported 15 m ahead along the route (ground traced,
  camera-cut flag set, the view rig's `HardReset` while walking with open eyes).
- Per-beat cap: a beat that has been the target for `ygg.Bot.BeatCapMin` (6 min) -> teleport to its place, then 90 s,
  then skipped. Also skipped: a beat that wants a posture while the climber walks (20 s), a beat it stands in reach of
  without it firing (2 min), and a beat the walk went past (a later beat that does not wait for it fired 30 s ago).
- Looking: aims at a beat's `look` target when the beat's other conditions hold. Otherwise it looks where it walks,
  plus camera stress (`ygg.Bot.Camera 1`): a 270-540 degree yaw sweep at 360-720 deg/s every 25-45 s, straight down
  then straight up (held 1.2 s each) every 70-110 s, and small random jitter (on 8-15 s, off 4-8 s). Only
  `AddYawInput` / `AddPitchInput`, scaled back to degrees: the view rig stays the one writer of the view.
- Ends at the chapter end (`Story: <chapter> finished`), after `ygg.Bot.CapMin` (75) minutes of game time, or after
  `ygg.Bot.StallMin` (15) minutes without any beat. Then writes the report and quits (`ygg.Bot.Quit 1`).

Story progress is read from the story's own log lines (`Story: beat <id>`, `Story: beat <id> done`, `... fires without
the look`, `Story: <chapter> finished`, the teleport put-back warning) through a log output device the bot registers:
the story subsystem has no public getter for its beats.

## The report

`Saved/Bot/<yyyyMMdd_HHmmss>/report.md` (or under `%LOCALAPPDATA%/UnrealEngine/5.8/Saved/Bot/` when the game writes
there): result, beats fired, the beat table (game and real time, place, player position, distance from the place, FPS
since the previous beat, notes: skipped and why, fired without the look, cap teleport), stuck events and what the bot did,
FPS (average, median, 1% and 0.1% lows, worst frame, frames over 50 / 100 / 250 ms, the 12 worst hitches with place),
camera (sweeps, down/up, aim time, pitch reached, time posed and with the eyelids shut, eye height range while walking,
the view rig's audit at the end), the CamRec incidents (the `CamRec: INCIDENT` log lines of this run, the first 30
listed, plus the folder count of `Saved/Monitor/incidents/<session>`, the newest session folder written since the run
began), story and view warnings
grouped, and the event log. `progress.md` is the same report rewritten every 60 s while it runs; `done.txt` holds the
result line.

## Console variables

| cvar | default | |
|---|---|---|
| `ygg.Bot.CapMin` | 75 | total cap, minutes of game time |
| `ygg.Bot.StuckSec` | 25 | seconds without progress before a 15 m teleport |
| `ygg.Bot.BeatCapMin` | 6 | minutes a beat may be the walk target |
| `ygg.Bot.StallMin` | 15 | minutes without a beat before the run ends |
| `ygg.Bot.Camera` | 1 | camera stress on / off |
| `ygg.Bot.Path` | 0 | 0 straight first, route when stuck; 1 route when far |
| `ygg.Bot.Rig` | 1 | read the view rig (0 = never touch it) |
| `ygg.Bot.HardReset` | 1 | a bot teleport calls the rig's `HardReset` |
| `ygg.Bot.Quit` | 1 | quit when the run ends |
| `ygg.Bot.Seed` | 0 | camera stress seed (0 = clock; the report names it) |

## The death-zone run (A5: South Col -> Summit)

```
powershell -File Tools/Bot/run_bot.ps1 -Chapter summit -ResX 1920 -ResY 1080 -TimeoutMin 50 -SummitArgs "down 300"   # bottled oxygen, walk
powershell -File Tools/Bot/run_bot.ps1 -Chapter summit -Fast                                                         # sprints when stamina allows
powershell -File Tools/Bot/run_bot.ps1 -Chapter summit -SummitArgs "noo2 stay 15"                                     # no oxygen, then stand at the top
```

`ygg.Bot.Summit [fast] [noo2] [stay <min>] [down <m>] [cap <min>] [from <m>]` (`UYggSummitBot`, `Source/YggdrasilsPeak/Survival/YggSummitBot.*`;
`from 2110` = testing a stretch: teleport that far along leg 7 after the South Col, the times are then not the demo's; when stuck it
names what blocks the capsule, and each teleport names what it landed on):
camp jump to the South Col (`UExpeditionComponent::TeleportToCamp`, as `ygg.Expedition.Go`), then route.json leg 7 by pure
pursuit 7 m ahead (the fixed rope lies on that line). Oxygen: a full bottle on at the start plus one spare, or with `noo2`
none at all (taken away again if the camp's pickups hand some over). At the summit it waits for the summit beat
(`Content/Story/Summit.ygs`) and the summit screen, closes the screen ("Stay at the summit"), looks at the World Tree, then
stands (`stay`) or walks back down `down` metres (300). Every 0.25 s: off the route (> 8 m for 3 s = lost), clipped to the
rope or not (off-rope stretches by metres along the leg), falling / sliding over 0.75 s, the floor gap, under the ground
(a trace from 30 m above hits over the capsule bottom), stuck (sidestep + jump after 8 s, a 10 m teleport after 30 s).
`ygg.GroundProbe A5_up_<m>` every 100 m (visible surface vs collision, `Saved/Review/ground_probe.csv`). The body: SpO2,
heart rate, health, stamina, core, bottle, the body effects on the view and the death-zone message (from the log).

Writes `Saved/Bot/summit_<time>_<o2|noo2>[_fast]/report.md` (times, rope, ground, body by 100 m bands, the summit beat,
issues, frames, pictures), `samples.csv` (one row per second) and pictures `Saved/Review/A5/<run>_<stage>.png`
(1 South Col, 2 death zone, 3 climb 8300 / 8600, 4 summit beat, 5 summit screen, 6 World Tree / up / view down, 7-8 descent,
death). `ygg.Bot.Summit.Stop` ends it with a report, `ygg.Bot.Summit.Quit 0` keeps the game open afterwards.

Gotcha (2026-11-20): the climber stuck at 2,140 m and 2,160-2,172 m was the World Tree root meshes' single auto convex hull
(an invisible blob over the arch, 0-11 m over the snow; the teleport landed on top of it). Traces with complex collision
(`clear_route_corridor.py`, any editor trace with trace_complex=True) do not see it; check with simple collision. Fixed by a
convex decomposition (15 and 24 hulls) on SM_wjvpcilfa_tier_0 and xiciaat_tier_1.
