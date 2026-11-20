# Morning report, 2026-11-20 (overnight chapter 1 redo)

Timed log: `Docs/Development/2026-11-19-overnight.md`. For your professor: the shareable page https://claude.ai/artifact/XW3T4J6ZKG3CN3auz1mpfW
(private until you share it from its Share menu; source `Docs/Development/Chapter1_v2_Design.md`)
(the full chapter 1 v2 design, written for a reader outside the project) and `Docs/Development/Chapter1_Camera_Spec.md`
(the technical camera spec).

## What changed tonight

**Camera, the biggest part.**
- New camera system: `Survival/YggViewRig`. It is the only thing allowed to move the view.
- **The 360-degree vertical spin is fixed at the root.** The story used to blend your up/down angle the long way round
  (0..360). It can no longer write your view at all: it can only give small nudges, which always take the shortest way
  and which you can resist with the mouse.
- **Knee-height camera:** the dispatch bug (`lock` was read as `look`) is fixed, so the tent now really holds you.
- **Snow sink:** reaches the eye only up to 10 cm (it used to be 35 cm).
- **Eye in front of the neck:** the eye now sits 16 cm in front of the body, so looking down no longer puts the camera
  inside your own jacket collar.
- **Real postures:** lying, sitting up in the bag, kneeling, crawling out.
- **Gentler breathing and steps:** this needs your judgement.

**Your height:** Sangye is now 1.9 m (`ygg.Player.HeightCm 190`; the eye sits at 174 cm).

**People.**
- **Nima, Marek and Elena are new AI characters** with realistic clothes. Marek was rigged in Blender on Nima's
  skeleton, because Meshy's rigger rejected him.
- **Ji-woo, Pema and the porters keep their MetaHuman faces.** Their clothes were rebuilt with real thickness and
  lined, rolled hems, collars that hug the neck, and drape and folds.
- **Walking:**
  - Feet now really step. The cause was MetaHuman foot-IK aiming at bones the skeleton does not have.
  - Teammates leave boot prints in the snow.
  - Followers walk your footsteps in single file, match your pace, and ignore camera turns.

**Camp and world.**
- The camp stands in a dry-stone walled field like the real Dingboche fields: a stone altar with flags, a trail chorten, gear by the doors, trodden lanes. There is no smoke yet (no smoke effect exists in the project).
- Prayer flags everywhere are now real printed lungta: wind horse, four animals, faded cotton, frayed edges. They
  replace the old fake shader. Strings hang edge to edge with a capped sag.
- The mess dome is regenerated without windows, so the jagged window rims are gone.
- The headlamp is soft now (35 lumens, wide fade) instead of a burnt white disc.

**Tools, so I can see problems myself:**
- **Camera flight recorder:** `Saved/Monitor/incidents/`. It saves the last 6 s and names the culprit whenever the
  camera spins, rolls, sits low or hitches.
- **Robot player:** `powershell -File Tools/Bot/run_bot.ps1 -Fast -YggCamRec`. It plays chapter 1 by itself and
  writes a report. It finished the whole chapter (25 beats) every run tonight.

**Later in the night (after the design page).**
- **Waking in the tent:** the start spot moved so the door (left) and the duffel (right, below the eye) are both in view.
- **End of the prologue fall:** the camera now snaps back in a single cut behind closed eyes, so the fast turn is gone.
- **Flag line from the altar to the mess tent:** it now runs level with the dome top, so it no longer hangs like a
  vertical column.
- **Floating on slopes (Development Plan A6):**
  - On walkable slopes the boots sit 0-7 cm above the snow, and on the route the visible ground matches collision
    within 1-5 cm.
  - The real float was sliding on slopes steeper than 46°: the legs stayed in the in-air pose, up to 1.4 m high. The
    feet are now fitted to the slope.
  - Testing with BugItGo also made you float, because it is a no-gravity cheat. That case is now recovered.
  - Off the route, the simplified collision is up to 50 cm off the visible ground (the RAM trade-off from 11-10).
    That is your call.
  - Evidence: Saved/Review/floating_A6/.
- **Robot run:** 24 of 25 beats, no camera spins or rolls. The duffel beat was skipped, which also happened before
  tonight's changes.

**Development Plan A5, South Col -> Summit demo (done; your test play is still open).**
- **Summit robot:** it climbs the route from the South Col by itself. Run it with
  `Tools/Bot/run_bot.ps1 -Chapter summit -SummitArgs "fast"`.
- **Summit moment:** an 8-second hold, a gentle turn toward the World Tree, and one subtitle: "Nothing above you now
  but the tree, and the sky." The wording is a placeholder for you to judge.
- **With bottled oxygen:** the summit takes 20:39 walking and 18:46 sprinting. The target is 20 minutes, so it is met
  only when sprinting. You decide if that is fine.
- **Without oxygen:** the climber collapses from lack of oxygen at 8,552 m after 21 minutes, before the top.
- **Fixed:**
  - Two World Tree root pieces near the summit had an invisible collision shape up to 11 m tall that blocked the route.
    They got a proper collision shape and were moved 7-12 m off the route.
  - The summit now only counts if you are on the surface, not inside the mountain under it.
- **Pictures:** Saved/Review/A5/. The summit picture shows a bright green chequered tent at the summit camp, which
  looks like a missing material (being checked). Your body is at about 33 °C there, so the frost and colour fringe are
  strong.

## Please look at (your eye)
1. Walk around the camp. Do the camera, the breathing and the steps feel right? Look down at your feet. Turn fast.
2. The team at the camp: Nima, Marek and Elena up close; Ji-woo and Pema's new clothes.
3. Your teammates following you on the trail.
4. The prayer flags and the camp from a distance.
5. Walk on a steep slope (or slide down one) and look at your feet.
6. The summit demo: `ygg.Expedition.Go` to the South Col, climb with oxygen (about 20 min).
7. Streaks when turning. Frame generation is now capped at 2x under a 120 fps limit; I could not reproduce the smear
   in engine screenshots, because it lives in generated frames.

## Still open
- Lure sounds (tent flap, laboured breath, flag gust, ridge wind) need picking by ear.
- Ji-woo, Pema and the porters as AI characters, if you prefer them after seeing tonight's clothes. That needs Meshy
  credits: 7 left, about 41 per character.
- A few staging asserts in the world builder are still unmet: the tent view angles and the porter entrance.
- AI-made Tibetan script on the prayer flags needs a cultural check before release.

## After you woke up (Ultracode hour, about 05:00-05:50 UTC)

About 30 agents ran in parallel while the A4 agent used the editor. Everything below is committed and pushed.

**New documents (Docs/Development/):**
- Chapter designs: Chapter2 ... Chapter7_Design.md.
- Specs: B3_Climbing_Spec.md, B4_Camps_Rest_Spec.md, B5_Sound_Plan.md (lists only; you pick by ear),
  B7_Visual_Review.md (top 10 visual fixes), Physiology_Game_Integration_Spec.md.
- Voice_Recording_Script_Ch1.md: every line, how to say it, file names, a home recording guide.
- Cultural_Check.md: the AI Tibetan script on the flags is a risk; replace it with a verified mantra.
- Design_Study_Steep_PEAK.md: the study your supervisor asked for.
- Meshy_Master_List.md: every new asset, with credits, and what 1,000 credits buys.
- Systems_Backlog.md: the systems still missing, in build order.
- Review_2026-11-20.md: 22 real bugs from tonight's code, each checked by a second agent. Not fixed yet.

**Data and tools:**
- Chapter 2 story data: 54 beats, and its checks pass. The chapter 2 world builder is untested because it needs the
  editor.
- Poly Haven assets: 510 MB of free textures, rocks, crates, a ladder and HDRIs in Downloads/polyhaven, listed in
  CATALOG.md.

**Chapter 1:**
- **Duffel beat:** the robot is not the cause. After the cut into the tent, physics pushes the 1.9 m body sideways out
  of the tent, so the duffel is out of reach. A fix is written as `Docs/Development/patches/chapter1_tent_wake_pin.patch`
  but not applied.
- **Crawl check:** fixed. The tent has no real door hole, only a painted door, so the check now tests that.

**A4 (Dingboche -> Camp 1, partly done):**
- New walker `ygg.Bot.Route`.
- Icefall path cleared: 347 ice pieces moved, and the invisible collision shells replaced.
- The HUD now names the correct next camp.
- The green summit tents are fixed.
- Still open: a clean full walk, the FPS number at the worst spot, and the contact sheet.
- For your call:
  - 10 tile seams with a 1-1.7 m step in their collision (fixing them needs a terrain rebuild)
  - the serac zones kill a walker who doesn't sprint
  - the whole route takes about 3 hours to walk, against a 15-25 minute target

**Decisions for you:**
1. Which chapter list is real: the old 17-chapter Chapter_Design_v1 or the newer 12-chapter Chomolungma list?
   Tonight's chapter designs use the v1 numbering.
2. Meshy credits: each chapter needs about 250-450 for its must-have assets. See Meshy_Master_List.md.
3. Usage lesson: Fable has its own weekly bar, but it also takes about 5x Opus from the 5-hour bar.

**First things next session:**
1. Fix review bug #1: since tonight's slope fix, walking can keep the swinging boot pinned to the ground.
2. Fix the other review bugs, including "story assets not cooked in a packaged build".
3. Apply the duffel patch and rebuild.
4. Finish A4.
