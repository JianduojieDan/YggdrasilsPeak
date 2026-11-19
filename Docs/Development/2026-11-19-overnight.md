# Overnight 2026-11-19 → 20

Rules (user, 2026-11-14): at 95% of the 5-hour limit stop and sleep until the reset; at 98% weekly, or when done,
commit + push + memory, then shut the PC down. Time every item. Leave all visual judgement to the user.

| # | Item | Start | End | Notes |
|---|---|---|---|---|
| 0 | Usage check: 5-hour 95% (resets 17:00 UTC), weekly 55% | 16:39 UTC | 17:02 UTC | slept until the reset |
| 0b | Window 2 start: 5-hour 0%. Resumed camera workflow (3/6 agents cached) + new debug/doc workflow | 17:03 UTC | | |
| 0c | Debug system (flight recorder, robot player) + Chapter1_v2_Design.md written (not compiled yet) | 17:03 UTC | 17:39 UTC | 4 agents |
| 1a | Camera spec implemented (rig, story, body, survival, settings, data) + build-fix + 18 review fixes; build OK | 17:03 | 18:21 UTC | workflow, resumed |
| 2a | World rebuilt (65 places), story validated; builder now run via a small stub (big script broke remote exec) | 18:21 | 18:51 | |
| 2b | Robot player run 1: whole chapter finished by itself (25 beats, 46 min); recorder: no spins/flips, 8x low eye | 18:52 | 19:40 | |
| 2c | Low eye root cause: snow sink 30-35 cm applied fully to the eye -> capped at 10 cm (SinkEyeMaxCm) | 19:41 | 20:05 | run 2 confirmed cause |
| 5a | Prayer flags: real printed lungta textures (Meshy image, 6 credits) on M_PrayerFlag = every flag on the map; strings edge to edge | 19:45 | 20:10 | |
| 6a | NPC walk (glide), NPC snow prints, follow redesign -> one agent, building + testing itself | 20:12 | | running |
| 6b | NPC glide root cause (MetaHuman foot IK aims at missing ik_foot bones) -> walk blend space; NPC prints; trail-following followers | 20:12 | 20:50 | agent |
| 6c | Team as AI characters: Nima, Elena (Meshy rig), Marek (Meshy rig failed -> rigged in Blender on Nima's skeleton) | 20:40 | 21:40 | 105 credits |
| 6d | Garments for Ji-woo/Pema/porters/Marek: thickness + rolled lined hems, hugging collars, drape/folds | 20:55 | 21:45 | agent |
| 4a | Mess dome regenerated without windows (33 credits), warmer tint | 20:20 | 21:50 | |
| 2d | Eye inside the jacket collar when looking down -> eye 16 cm in front of the body (ygg.View.EyeFwd); headlamp 35 lm soft | 22:00 | 22:35 | |
| 2e | Regression robot run: chapter finished, low-eye incidents gone (2 harmless left) | 22:35 | 23:20 | |
| 7a | Commits 520a4c3 (local) / 5957c6e (GitHub); rule: every commit pushed to GitHub | 23:25 | 23:30 | |
| 5b | Camp beauty pass (walled field, altar + juniper smoke, kitchen smoke, gear life) | 23:31 | | agent running |
| 1 | Resume the camera implementation workflow (wf_b07cd78d-946) | | | |
| 2 | Rebuild chapter 1 world + story, functional run (CamTest, Auto) | | | |
| 3 | #4 shadow smear under the flag cords | | | |
| 4 | #7 mess-tent window edges | | | |
| 5 | #6 realistic prayer flags everywhere, prettier camp | | | |
| 6 | #5 NPC walk, snow prints, clothes | | | |
| 7 | Commit + push + memory, shut down | | | |
