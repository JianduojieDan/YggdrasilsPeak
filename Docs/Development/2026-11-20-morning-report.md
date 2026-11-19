# Morning report, 2026-11-20 (overnight chapter 1 redo)

Timed log: `Docs/Development/2026-11-19-overnight.md`. For your professor: `Docs/Development/Chapter1_v2_Design.md`
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
- Prayer flags everywhere are now real printed lungta: wind horse, four animals, faded cotton, frayed edges. They
  replace the old fake shader. Strings hang edge to edge with a capped sag.
- The mess dome is regenerated without windows, so the jagged window rims are gone.
- The headlamp is soft now (35 lumens, wide fade) instead of a burnt white disc.

**Tools, so I can see problems myself:**
- **Camera flight recorder:** `Saved/Monitor/incidents/`. It saves the last 6 s and names the culprit whenever the
  camera spins, rolls, sits low or hitches.
- **Robot player:** `powershell -File Tools/Bot/run_bot.ps1 -Fast -YggCamRec`. It plays chapter 1 by itself and
  writes a report. It finished the whole chapter (25 beats) every run tonight.

## Please look at (your eye)
1. Walk around the camp. Do the camera, the breathing and the steps feel right? Look down at your feet. Turn fast.
2. The team at the camp: Nima, Marek and Elena up close; Ji-woo and Pema's new clothes.
3. Your teammates following you on the trail.
4. The prayer flags and the camp from a distance.
5. Streaks when turning. Frame generation is now capped at 2x under a 120 fps limit; I could not reproduce the smear
   in engine screenshots, because it lives in generated frames.

## Still open
- Lure sounds (tent flap, laboured breath, flag gust, ridge wind) need picking by ear.
- Ji-woo, Pema and the porters as AI characters, if you prefer them after seeing tonight's clothes. That needs Meshy
  credits: 7 left, about 41 per character.
- A few staging asserts in the world builder are still unmet: the tent view angles and the porter entrance.
- AI-made Tibetan script on the prayer flags needs a cultural check before release.
