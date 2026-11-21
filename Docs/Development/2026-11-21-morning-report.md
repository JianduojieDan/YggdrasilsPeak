# Morning report, 2026-11-21

Overnight work on chapter 1. I checked everything below myself, with measurements, robot runs and screenshots. How it
looks is your call. Before/after pictures are in **`Saved/Compare/morning_2026-11-21/`**. The file names say what each
one shows.

## Status by to-do item

| # | What | Status |
|---|---|---|
| 84 | NPCs floating while following | **Fixed, verified.** Cause: the retargeted walk lifts the Meshy bodies as they speed up, about 0.2 cm per cm/s, and above about 150 cm/s the pose breaks. The sole fitting now subtracts that lift. The followers' pace is capped at 150 cm/s. Follow test: 0 frames more than 5 cm off the ground, across all 5 NPCs. |
| 80 / 83 | Hair through hats / Pema's scarf | **Done.** Pema is now a complete Meshy character (`pema_body`), with no separate hair or scarf, so nothing pokes through. Ji-woo wears a plain knit beanie fitted to her head (fixed sizes, not measured from the face; the measured ones came out huge). |
| 79 | MetaHuman heads on the Meshy bodies (Marek, Nima, Elena) | **Tried, not finished, reverted.** What I found: the Meshy head bone points 90° off from the MetaHuman one. The retargeted walk makes the head and chest bones jump at speed, so the head floated now and then. The neck seam stayed visible. I put them back to their complete Meshy looks, which are stable (no floating, heads attached; close-ups `npc_*_front.png`). The code for a face on another body stays in `YggStory` (unused) for a later try. |
| 81 | Ji-woo's outfit | **Done.** MetaHuman techwear outfit plus the cream beanie (`npc_jiwoo_front.png`). |
| 82 | Coat flap between the legs | **Found, not fixed.** It is the coat hem splitting open between the legs while walking. The skin weights pull the hem to each leg. You can see it on Pema in `followers_coat_hem_split.png`. Fix: in Blender, move the hem's weights from the thighs to the hips. This is next. |
| 78 | Stone walls hollow / too neat | **Rebuilt.** Uses the four free Megascans field-wall scans. Lines are bowed, there are slumps and gaps, and the corners are jittered: 215 pieces, 9 gaps, 50 slumped. |
| 6 / 4 | Prayer flags | New cloth flags with their own colours on every string. The AI-made writing is kept, as you said. |
| 86 | Fire in the fire ring | **Done.** Built on its own stone ring on the snow, 4 m from the prayer-flag pole: yak-dung patties, three flame cards (a painted flame, pushed upward by a moving noise, with flicker), faint smoke, and a warm flickering light (`fire_2_after.png`, `fire_4_wide_with_wall.png`). Gorak Shep has the same fire. Note: the stone pile at the foot of the pole is the base that holds the pole up (tarchen), so I did not put the fire there. |
| 75 / 7 | Mess dome texture | **Fixed, verified.** UE turned this one model's JPEG texture into a different, scrambled image, even when re-imported. The dome now uses Meshy's PNG, and its clean yellow/grey triangle panels show (`mess_dome_1_before.png` → `mess_dome_2_after.png`). I checked all 55 Meshy models the same way, and every other one matches its source texture. |
| 85 | Tent interior empty | **Done.** Added a load pack, a climbing helmet, double boots, crampons, a maroon beanie, and glacier glasses on the duffel. |
| 77 | Getting out of the tent | **Done: posture hints.** "Hold [key] to sit up" and then "crawl out", plus the crawl. I tried a cool light at the door too, but this tent fabric does not block light: it lit the snow outside with a hard straight edge, and at short reach it showed as a round hot spot on the outside (`tent_door_light_*.png`). It is **off** now. |
| 76 | Settings apply-with-confirm | **Done.** You change a setting, then Apply asks you to confirm, and Keep/Revert works. Self-test `ygg.Settings.Test.Pending` passes 16/16. |
| 73 | Lines on the camp ground | **Still open.** The ground's collision is perfectly flat. At the tent at dawn I found no repeating lines. The only hard straight edge I found was my own door light, now off. I need the spot and time of your screenshot to chase it further. |
| 74 | Halos / patches around characters | **Still open, not reproduced.** In the follow test, switching off local exposure, Lumen screen traces, short-range AO, SSAO and Lumen GI one by one showed nothing around the followers. |

## Robot run

Chapter 1 robot run (fast): every beat it reached fired in order: dawn in the tent, the duffel, Dolma calls, crawling
out, the farewell, the job, and the climb up out of camp. I stopped it on the walk to the stream, about 4 game minutes
in, to free the computer for your light fixes. Its notes are in `Saved/Bot/20261121_055046/progress.md`.

## Assets

- **Fab** (free, your library): 22 items in `Downloads/fab_2026-11-21`. `SOURCES.md` lists the licences. Six are CC-BY
  and need a line in the credits: snow_fence (Punttulanbro), walkie (Waffle1434), smartphone (dimension Dazzle),
  whiteboard (Ewan Lejkowski), ifak (Barnus Model's), beanie (Viktor™).
- **Meshy**: 21 concepts, 17 models, 7 rigs, 5 flag prints. Balance 492 credits. Nothing was generated after the asset
  round.

## What needs your eye

1. The fire: its size, the flame look, and how bright it is at dawn (`fire_*`).
2. The mess dome with its real panels (`mess_dome_2_after.png`).
3. The five NPCs in their complete Meshy looks, and Ji-woo's outfit and beanie (`npc_*`).
4. The walls and flags, walking through the camp.
5. Getting out of the tent with only the posture hints, now that the door light is off.

## Not done tonight

- #82 coat hem (diagnosed above), #73 and #74 (not reproduced).
- Extras I did not reach: NPC life (look-at, blinking, breathing), the imperfection pass, performance numbers, the
  tile-seam rebuild, and the sound list.
- Three staging checks in the world builder fail: porter_start, the night stove, and the fly-out clearance. I believe
  they were failing before tonight's changes, but I did not check.

## Morning fixes (your notes after waking)

| What you said | What changed |
|---|---|
| The tent interior is too shiny; put a light inside and leave some shadows | The centred ceiling light (every wall the same distance, so evenly bright like daylight, with one hard shadow) is now a small lamp hung in the door half of the tent, shining down and back across the bags. It has soft shadows and little shine. The floor and the back corners fall into shade (`tent_light_1_before.png` → `tent_light_2_after.png`). |
| The compass is too shiny; it looks like the flashlight itself | The brass is worn satin (rougher, a bit darker) and the dial is an aged cream instead of near-white. The tent lamp no longer lights the held compass directly (lighting channels), and under your headlamp the compass takes only a soft warm spill with no glare (`compass_tent_*`, `headlamp_compass_night_*`). |
| The flashlight is very powerful | Headlamp 35 → 6 lumens, with a soft wide centre instead of a 4° hot spot, and less shine. At 35 and even 15 lm it clipped to a white disc; at 6 the snow shows inside the pool (`headlamp_night_*`). Console: `ygg.Story.HeadlampLm`, `ygg.Compass.Wear`, `ygg.Compass.LampSpill`. |

## NPC coats (#82), fixed after your "fix the NPC problem"

| What | Result |
|---|---|
| Root cause | Every Meshy NPC's retargeter (`RTG_Manny_to_<id>`) ran Unreal's default **Root Motion** step. It puts Manny's floor-level root motion on the target's top bone, and a Meshy skeleton has no root bone (its top bone is the pelvis). So in every walk/jog clip the pelvis sat on the floor while the legs and spine stayed up, and everything skinned to the pelvis (hips, crotch, coat bottom) was dragged toward the ground. That made Elena's jacket look knee-long and torn, put the triangle between Nima's legs, and is likely part of the old "body lifts with speed" (#84). In Blender the models were fine; only the game's clips were wrong. |
| Fix | Root Motion step off in all 13 Meshy retargeters (our NPCs are moved by code, not root motion) and the clips re-baked in place (`Tools/Story/npc_retarget_fix.py`). Pelvis in the walk is back at its height (Elena 85 cm vs 89 cm standing; it was 0). `ai_retarget.py` now turns the step off for future characters. |
| Second fix | Meshy's auto-rig added stray bones (`Bone_NNN`) to Elena, Pema, Hale and Nima's climbing version: e.g. Elena's shoulders were skinned to bones hanging off her forearms. Their weights now sit on the standard bones the vertices lie on (`Tools/Blender/meshy_clean_rig.py`, re-import `Tools/Story/reimport_clean_rigs.py`). Skeleton unchanged. |
| Checked | The game's exact poses rendered in Blender (`Tools/Blender/ue_pose_preview.py` from `Tools/Story/dump_npc_pose.py`): Elena, Nima, Marek, Pema clean in walk and jog (`npc_coat_game_pose_fixed.png`). In the game, the follow scene: Elena's jacket back to hip length, no flap, boots on the snow (`npc_coat_1_before_2_after.png`; tool `Tools/Story/hem_test.ps1`). Not checked: a full robot run, Nima in the game (he was not in view), the chapter 2 characters in the game. |
