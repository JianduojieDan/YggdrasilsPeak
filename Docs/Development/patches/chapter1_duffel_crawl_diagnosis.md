# Chapter 1: duffel beat skipped, crawl assert fails (diagnosis, 2026-11-20)

Two problems in Sangye's tent at the start of chapter 1. Short version:

1. **Duffel skipped.** This is not the robot. The climber does not stay where the story puts him. Right after the cut
   to `tent_start`, the physics pushes his capsule 1.2-1.7 m to the side (once out past the tent wall). From there the
   duffel is 2.4 m away and the beat's reach is 1.6 m, so the beat can never fire, for the robot or for a person.
2. **Crawl assert.** The tent model has no real door opening. The door is only painted on a closed shell. The old check
   looked for a gap that does not exist, so it failed from every spot. The check now tests what matters: the crawl line
   leaves the tent through the painted door, not through a wall or the roof.

## 1. The duffel beat

### What the runs show

| Run (local time) | Climber after the cut (tent frame, cm) | Duffel distance | Duffel beat |
|---|---|---|---|
| 23:51, 00:41 (before commit 8bfe81d7) | (-20, -62), standing ~38 cm up | 1.5 m | fired |
| 02:40 (1.9 m capsule, old tent_start) | (-68, -128), standing 71 cm up | 2.4 m | skipped |
| 05:13 (1.9 m capsule, tent_start (14, -15)) | (-55, -172), outside the side wall | 2.4 m | skipped |

Tent frame: x points to the door, y to the left, origin = the ground under the tent's centre. The side wall is at
y = +-125 cm. The duffel is at (48, +40), 45 cm up.

From `Saved/Monitor/20261120_051338/frames.csv` (the camera, every frame):

- The cut puts the climber at tent_start (log: "teleport ok, now at 340118, 2468204").
- In the next 2-3 frames he moves about 1 m sideways and 44 cm up **with speed 0**. Speed 0 means it is not walking or
  falling: it is the physics pushing the capsule out of something it overlaps ("depenetration").
- Then he slides slowly (0.2-0.5 m/s) for about 2 s and stops at (-51, -169), outside the tent wall.
- Eyelids are shut the whole time, so the player never sees it. He opens his eyes outside the tent fabric.

### Why it is not the robot or the beat data

- The story fires `duffel` only if the capsule centre is within 160 cm of `tent_duffel` (3D,
  `YggStory.cpp` line 1879), the posture is sit, the duffel is in an 18 degree cone and [E] is pressed.
- At tent_start the distance is 84.5 cm: inside the reach. The beat data is fine.
- From where the climber really ends up it is 243 cm. The robot's rule is the same as the story's (it only aims when in
  reach), so it waits 30 s and skips. A person could not fix it either: sitting, a move key starts the crawl out.

### What pushes him (not proven yet: needs the editor)

- The capsule is now about 2.2 m tall (half height ~110 cm, measured from two runs: actor height minus the floor under
  the eye). The story's teleport puts the capsule centre 100 cm over the place, so its bottom starts **10 cm under the
  tent floor**. This probably came with "player height 1.9 m" (commit 8bfe81d7, 02:17); the last good runs (23:51,
  00:41) were before it.
- In the 23:51 run, if the capsule was still the C++ default (half 96), the climber ended standing 37.7 cm up: exactly
  the top of Sangye's sleeping bag (37.3 cm). So the sleeping bag may block the player, although the builder spawns it
  with `collide=False`. Something there blocks the capsule.
- Other candidates: the duffel, the father's bag, the dome itself (only if it has pawn collision: the capsule is
  taller than the 1.66 m dome), or something under the floor.

### What I changed (data / Python)

- `Tools/Story/chapter1_story.py` -> `Content/Story/Chapter1.ygs`: the tent_start teleport now has `"up": 112`, so the
  capsule starts on the floor instead of 10 cm inside the ground. Regenerated with `python -I`: validate OK, the only
  change in the .ygs is that one line.
- `Tools/Story/build_chapter1_world.py`: a new staging assert, "nothing blocks the waking climber's capsule at
  tent_start". It runs last, puts the player's real capsule (read from BP_FirstPersonCharacter; fallback r 42 / half
  110) at tent_start, and lists every actor that blocks a pawn there, by name and tags. Its next run names the culprit.

### What needs a Source change: `chapter1_tent_wake_pin.patch` (not applied; `git apply --check` passes)

- `YggStory.cpp` CutTo: the capsule bottom never starts inside the ground (`up` is raised to half height + 2 cm, for
  every cut).
- CutTo logs every component that blocks the capsule at the cut spot ("Story: cut tent_start: ... overlaps X").
- A posture right after a cut (lie, sit, kneel) **pins** the climber to the cut spot: movement off (MOVE_None, so no
  gravity and no push), put back if he drifts more than 15 cm. Walking or crawling hands control back. Stop() also
  releases it. Switch: `ygg.Story.PinPosed 0` = old behaviour.
- `YggPlayBot`: the skip note now gives the numbers ("2.4 m from tent_duffel, reach 1.6 m"). A new warning in the event
  log when the climber is more than 30 cm from where a story cut put him, 3 s later, while posed.

Known risk of the pin: if something really blocks the capsule at tent_start, the push can still happen when the crawl
starts (movement comes back on). The real fix is to remove that blocker; the new assert and log name it.

## 2. The crawl assert

- `Art/Meshy/prep_report.json`: `tent_dome` has `"openings": []` and one hole filled. The prep script looked for
  see-through openings and found none.
- I checked offline on `Art/Meshy/tent_dome.glb` (scaled x1.8, the way the builder places it): every line from inside
  the tent to tent_out meets the closed end wall twice (its inner and outer surface), for every offset from -30 to +30
  cm and from both tent_start spots. At crawl height the end wall is at x = 144 cm (35 cm up) and x = 122 cm (68 cm up).
- So "passes the door gap" could never pass. The crawl works in the game only because Sangye's tent has no pawn
  collision: the climber goes through the painted door.

**New check:** "crawl line tent_start -> tent_out leaves through the painted door". It traces the crawl line both ways
(inside -> tent_out and tent_out -> inside) at 35 and 68 cm. Both first fabric hits must be on the door panel: on the
door end (x >= 87 cm, where the straight side walls stop), within +-45 cm of the axis, and not higher than 126 cm.

Offline result with the same rule: from tent_start (14, -15) with tent_out on the axis it passes (crossings at y -7 and
-8 cm). A line out through the side wall or the roof fails, so the check still means something.

## Needs the editor / a build to confirm

1. Re-run the builder. Expect the new crawl assert to pass. Read the capsule assert: what blocks the capsule at
   tent_start?
2. Remove that blocker (collision off), or tell me what it is.
3. Apply the patch, build, run the robot (`Tools/Bot/run_bot.ps1`). Expect: `duffel` fired, no "cut put him" warning,
   and the tent opening shot from inside the tent.

## Open questions for you

- The painted door's size (+-45 cm wide, 1.26 m high) is my estimate. No geometry marks it. Please check by eye.
- `tent_door` sits at the bounding-box tip (x = 198 cm). The real end wall is at x = 122-164 cm. This only moves the
  "face the door" direction by about 3 degrees, so I left it.
