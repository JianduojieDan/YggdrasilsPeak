# B7 · Visual review of the demo route

Date: 2026-11-20. Block B7 in `Development_Plan.md` ("the demo route looks finished").
This is a review only. Nothing in the game was changed.

## How this review was made

- I looked at about 75 of the roughly 250 pictures, as contact sheets plus some full-size frames:
  - `Saved/Review/A4/`: the route robot, Dingboche to the top of the icefall. That run was still going when I looked.
  - `Saved/Review/A5/`: the summit robot, South Col to the summit.
  - `Saved/Compare/shot_story_*.png`: the chapter 1 camp. I used the newest pictures, from 11-19 and 11-20.
  - `Saved/Review/floating_A6/`.
- Every picture path below is relative to `F:/UE_project/YggdrasilsPeak/`.
- I judged each picture against the art direction: **quiet awe, cloud light**. That means:
  - layered ridges that fade to blue;
  - drifting cloud gaps that throw light patches on far faces;
  - snow that is blue in shade, not grey;
  - warm light only high up;
  - a realistic base, where myth enters quietly;
  - clear shapes far away and touchable detail near.
  (The reference images are in `Docs/ArtDirection/refs/01-04`.)
- **Limits of the pictures.** Keep these in mind before you act on any item:
  - The route robot walks at 4x game speed. That makes motion blur about 4x stronger than a player would see.
  - Both robots often look at the ground. So the views of the summit ridge show snow, not the ridge.
  - The A4 run had not reached Camp 1 yet (its last picture is at 5,725 m). The Camp 1 notes therefore use old
    pictures from 11-02 and 11-12.
  - Per the "camera above terrain" rule, any "camera under the ground" item must be checked before it is fixed.
- **Priority.** **P1** breaks immersion. **P2** is noticeable. **P3** is polish.
- **Budgets for every fix.** 60 fps rendered on the laptop (RTX 5070 Ti, 12 GB VRAM).
  - Measure before and after each change with `Tools/Art/perf_run.ps1`. Use the usual spots: Dingboche, icefall,
    glacier, summit, Cwm, storm.
  - Downloaded textures: 2k at most, compressed, shared between materials where possible.
  - No new hand-made models. Prefer, in this order: settings, then materials, then free downloaded assets.

**Today's frame budget, from the robot logs.** About 60 fps rendered almost everywhere. The 120 fps on the counter is
frame generation. The 1% low was 57 fps. The summit's GPU time is 8.9 ms. The slowest stretch was 58 fps at 5,274 to
5,325 m, near Base Camp and the icefall approach. So there is little spare room. Most fixes below cost nothing or
save time; the cloud fix is the only one that needs a real measurement.

---

## 1. Whole route (seen in many areas)

### What reads well (keep)
- The HUD is quiet and readable, and it stays out of the picture.
- The frost crackle at the screen edges is a good idea, as long as it stays at the edges (see G1).
- Boot prints, the snow trench and the step puffs. You liked these before; leave them alone.

### G1 · The body effects are too strong: colour split, smear and frost — **P1**
- **Seen in:**
  - `Saved/Review/A4/route_20261120_094757_L0-3_10000m.png` (5,015 m, body 35.9 °C, colour fringe on every edge,
    smeared ground)
  - `Saved/Review/A4/route_20261120_094757_L0-3_11000m.png`, `..._11400m.png`, `..._leg0_start.png`
  - `Saved/Review/A5/summit_20261120_084131_o2_3_climb_8600.png` (33.2 °C: frost covers the whole frame, not
    just the edges)
  - `Saved/Review/A5/summit_20261120_084131_o2_4_summit_beat.png` (33.6 °C: the summit picture itself has a red/blue
    fringe on every flag, rope and branch)
- **What breaks:** from about 5,000 m on, the whole trail looks blurry and "broken lens". At the summit, the most
  important picture of the demo is the most damaged one.
- **Likely cause:** `M_PP_Body` (the body screen effect) adds a tunnel, a radial smear and a colour split.
  - The robot logs show oxygen saturation (SpO2) is already about 78% at 5,000 m. That is normal for a trekker there,
    yet the split and smear are clearly on.
  - Frost reaches full strength at 33.2 °C, and its mask covers the centre of the screen.
  - The robot's 4x game speed adds extra motion blur on top of this.
- **Fix idea (settings and material, no cost):**
  1. First find which input does it. Test with `ygg.Body.Hypoxia 0` and then `ygg.Body.Exhaust 0` at the same spot.
  2. Make the colour split start only below about 70% SpO2. Cap it at about 1.5 px at 1080p.
  3. Let the smear come only in short pulses with the heartbeat, not all the time.
  4. Give the frost a radial mask: the outer 25% of the screen only. The centre stays clear even at 33 °C.
  5. Add a "clarity moment": during story beats (the 8-second summit hold, dialogue) fade all body effects to about
     30%.
  6. Check the camera's own post-process too. Its "scene fringe" (UE's built-in chromatic aberration) should be 0.

### G2 · No clouds and no cloud light on the trail — **P1**
- **Seen in:**
  - `Saved/Review/A4/route_20261120_094757_L0-3_01000m.png`, `..._02000m.png`, `..._03000m.png`, `..._04000m.png`
  - `Saved/Review/A4/route_20261120_104624_L1-3_02000m.png`
- **What breaks:** the trail is under an empty deep-blue noon sky the whole way. The sky is the centre of the art
  direction ("cloud gaps send light patches across distant faces"), and here it is missing. Everything has the same
  hard white value, so the valley reads as small.
- **Likely cause:** the weather is "Clear", which turns off the volumetric clouds. The time of day stays near noon.
- **Fix idea (settings):**
  - For the demo, use a partly cloudy setting: Ultra Dynamic Sky cloud coverage about 2 to 3, cloud shadows on, slow
    cloud speed. Then light patches drift over the far faces.
  - Add a thin low cloud or fog layer in the valleys below the trail, for "ridges fading layer by layer".
  - Set a time of day per stretch: early morning at Dingboche, late morning on the moraine, midday at Base Camp.
  - Keep the cirrus seen at the summit. It already looks right.
  - **Measure the cost.** Volumetric clouds are the one fix here that can cost about 1 ms. Ultra Dynamic Sky has a
    cloud quality setting if it does.

### G3 · "Dalmatian" mountains: round dark rock blots on the snow — **P1**
- **Seen in:**
  - `Saved/Review/A4/route_20261120_094757_L0-3_01000m.png`, `..._02000m.png`, `..._03000m.png`, `..._11800m.png`
  - `Saved/Review/A5/summit_20261120_084131_o2_6_summit_view_down.png` (dark blots with grainy edges on the far snow)
- **What breaks:** mid-distance peaks look like cow-spotted toys instead of real Himalayan faces. Real faces show rock
  ribs, dark strata bands and snow gullies that run downhill.
- **Likely cause:** the landscape material picks rock by slope. On the coarse 17 to 34 m terrain grid, only single
  steep facets pass the slope test, so the rock becomes isolated round blots. The grainy edge is a dithered blend that
  TSR (the upscaler) cannot smooth at distance.
- **Fix idea (material, plus one baked texture):**
  - Beyond about 500 m, use a softer slope ramp. Add a curvature term (ridges show rock, hollows keep snow) and a
    downhill streak term (gullies).
  - Add faint dark strata bands, as in the reference images. Make rock blue-grey in shade, not black.
  - Optional: bake one slope/curvature/flow mask from the DEM (elevation data) with the existing
    `Tools/Terrain/dem.py`. One 4k single-channel texture is about 11 MB of VRAM, and no geometry is added.

### G4 · Spindrift reads as flying paper or birds in a calm, clear sky — **P2**
- **Seen in:**
  - `Saved/Review/A4/route_20261120_094757_L0-3_06000m.png` (white flecks high in the blue sky)
  - `Saved/Review/A4/route_20261120_094757_L0-3_08000m.png` (dark sticks flying across the sky)
- **Likely cause:** spindrift streaks spawn high above the ground and in calm weather. In shade they turn brown or
  dark. This is the same family as old to-dos #3 and #8.
- **Fix idea:**
  - No spindrift in calm "Clear" weather.
  - Spawn it only near the ground and only in wind.
  - Fade it by height above the ground and by distance.
  - Use a downloaded soft snow-particle flipbook, lit, never darker than the sky behind it.

### G5 · The camera ends up under the ground (half the screen black) — **P1, check first**
- **Seen in:**
  - `Saved/Review/A4/route_20261120_094757_L0-3_05001m.png` ("A hard landing", lower half black)
  - `Saved/Review/A4/route_20261120_094757_L0-3_07000m.png`, `..._08000m.png` (the underside of the ground is
    visible)
  - `Saved/Review/A4/route_20261120_094757_L0-3_06000m.png` (the eye sits at snow level, and the near ground is a
    grey smear)
- **What breaks:** if a player sees this even once, the world turns into a stage set.
- **Likely cause (not proven):** one of these:
  - after a fall or slide, the camera's near plane dips below the visible tile;
  - the visible tile surface and its collision disagree (tile displacement vs the collision mesh);
  - a robot-only effect.
- **Fix idea:**
  1. Check the robot report columns "boots-visible" and "boots-collision" between 4,740 and 4,950 m.
  2. Check the camera flight recorder (`Saved/Monitor/incidents/`).
  3. If it is real, add a small sphere sweep for the eye that keeps it about 20 cm above the *visible* surface.
  This overlaps with A4/A6, so it belongs to whoever owns those blocks.

### G6 · The subtitle veil is a stack of hard grey bands, and the HUD sits under it — **P1**
- **Seen in:**
  - `Saved/Review/A5/summit_20261120_084131_o2_4_summit_beat.png`
  - `Saved/Compare/shot_story_team.png`, `Saved/Compare/shot_story_nimaclose.png`
- **What breaks:** the veil shows 4 or 5 visible steps of grey across the bottom quarter of the screen. It looks like a
  render bug, and it breaks the "light, not lines" UI language. The altitude numbers show through it.
- **Likely cause:** the veil is drawn as several flat rectangles with rising alpha, instead of one smooth gradient.
- **Fix idea (small UI change):**
  - Draw one soft gradient: a texture or a material with smooth alpha and no hard top edge, kept narrower.
  - Fade the HUD out while a subtitle is on screen.

### G7 · Snow plains are flat, overexposed white — **P2**
- **Seen in:**
  - `Saved/Review/A4/route_20261120_094757_L0-3_11400m.png`, `..._11800m.png`
  - `Saved/Review/A4/route_20261120_104624_L1-3_03200m.png` to `..._03800m.png`
  - `Saved/Review/A5/summit_20261120_084131_o2_1_southcol.png`
- **What breaks:** big areas sit almost at pure white with no texture. The eye has nothing at the feet to rest on
  ("tracks give scale" in the art direction).
- **Likely cause:**
  - exposure tuned for the sky;
  - snow albedo close to 1.0;
  - flat noon light;
  - the snow detail was made "bigger units" on request, so little small shape is left.
- **Fix idea (settings and material):**
  - Exposure compensation about -0.3 to -0.5 EV at high noon.
  - Snow albedo about 0.85.
  - Add wind features on the open plains only: sastrugi (wind-carved ridges), wind crust, ripples, a light blue tint
    in hollows.
  - Keep what you liked: the soft snow shadows and the rock/snow blend.

---

## 2. Dingboche camp (4,340 m, chapter 1)

### What reads well (keep)
- The warm, low morning sun with long blue shadows (`Saved/Compare/shot_story_camp.png`, `shot_story_tentext.png`).
  This is the best light on the route so far.
- The orange dome tents have believable wrinkles. The camp props are well dressed.
- The chortens and the mani stone ("Mingma Sherpa 1978–2026") are strong quiet story beats
  (`Saved/Compare/shot_story_chortens.png`, `shot_story_stone.png`).
- The warm glow inside the tent (`Saved/Compare/shot_story_afterslip.png`).
- The night prologue: a tiny figure on a huge slope under the stars (`Saved/Compare/shot_story_fatherfar.png`).

### D1 · The photo floats above the duffel in the first shot of the game — **P1**
- **Seen in:** `Saved/Compare/shot_story_tentduffel.png`, `Saved/Compare/shot_story_tent.png`
- **What breaks:** the photo is a flat, rigid white card about 10 cm above the bag, with a hard shadow. It is the
  first close-up a player sees.
- **Likely cause:** it is placed at a fixed height, not seated on the bag.
- **Fix idea:** trace down onto the bag and place it there. Give it a slight bend and a soft edge, and lower its
  roughness contrast.

### D2 · Trees in a camp above the treeline ("Alpine ski resort") — **P1, your call**
- **Seen in:**
  - `Saved/Review/A4/route_20261120_094757_L0-3_00004m.png`
  - `Saved/Compare/shot_story_camp.png`, `shot_story_dolma.png`, `shot_story_tentext.png`
- **What breaks:** real Dingboche (4,410 m) is above the treeline. It has stone-walled fields, juniper scrub and dry
  grass, and no conifers. The tall pines make it read as the Alps.
- **Likely cause:** the forest scatter has no height limit (the same as item T1).
- **Fix idea:** see T1. **Question for you:** do you want a few trees here on purpose, for warmth? If not, remove
  them.

### D3 · Fully white camp — **P2, your call**
- **What breaks:** in November, Dingboche is mostly brown and dry, with patchy snow. A brown start that turns white as
  you climb would give the route a colour story ("start low, climb into the white").
- **Fix idea:** a landscape material zone below about 4,600 m with dry grass and patchy snow. Use free downloaded
  grass and soil textures.

### D4 · Night is a pure saturated blue — **P3**
- **Seen in:** `Saved/Compare/shot_story_headlamp35.png`, `shot_story_prologue.png`, `shot_story_fatherfar.png`
- **Fix idea:** make moonlight blue-grey with low saturation. Real night snow is almost colourless. Use colour
  grading or the moon light colour; this is a settings change.

### D5 · Tent interior and sleeping bags look like smooth plastic — **P3**
- **Seen in:** `Saved/Compare/shot_story_tent.png`, `shot_story_afterslip.png`
- **Fix idea:**
  - Tent: a fabric weave normal map, seam lines, and light that shines through the fabric.
  - Sleeping bags: higher roughness and a quilting normal map. Downloaded fabric textures at 1k are enough.

### D6 · The "stream" reads as a hole in the snow — **P3**
- **Seen in:** `Saved/Compare/shot_story_stream.png`. A stream should be a line with flowing water, not a round
  crater.
- **Fix idea:** a long, thin, partly frozen channel. A downloaded "frozen creek" material on a spline is enough.

---

## 3. Trail, Dingboche to Gorak Shep (4,340–5,160 m)

### What reads well (keep)
- The valley composition at 1,000–3,000 m: the route leads off to one side, with peaks at the end of the valley
  (`Saved/Review/A4/route_20261120_094757_L0-3_02000m.png`). It just needs sky and haze (G2, G3).

### T1 · Pines and dead grey trunks up to 5,000 m — **P1**
- **Seen in:**
  - `Saved/Review/A4/route_20261120_094757_L0-3_03000m.png`, `..._04000m.png` (green broadleaf trees at 4,640 m)
  - `Saved/Review/A4/route_20261120_094757_L0-3_06000m.png`, `..._08000m.png` (bare grey poles at 4,940 m),
    `..._11400m.png` (trees at Gorak Shep, 5,154 m)
- **What breaks:** in the Khumbu valley, trees end at about 3,900–4,100 m. Above that there is only low scrub, then
  bare moraine. The dead trunks read as burnt poles.
- **Likely cause:** the forest scatter (built 11-11) has no height cut-off.
- **Fix idea (settings, and it saves frame time):**
  - Cut off trees above about 4,000 m in the scatter or PCG height filter.
  - In their place, below about 4,800 m: low juniper and dwarf rhododendron scrub plus dry grass tufts, from free
    Fab/Quixel/Poly Haven assets.

### T2 · The yellow fixed rope runs along the whole trek — **P1, your call**
- **Seen in:**
  - `Saved/Review/A4/route_20261120_094757_L0-3_00200m.png`, `..._01000m.png`
  - `Saved/Review/A4/route_20261120_094757_L0-3_10000m.png` (the HUD says "on the rope" at 5,015 m)
  - `Saved/Compare/shot_story_c_porter.png`
- **What breaks:** a bright, perfectly straight yellow line runs for 11 km of walking trail. It reads like a laser
  guide. Real treks have no ropes; fixed ropes start in the icefall.
- **Likely cause:** one rope line was laid along the whole route, for guidance and clipping in.
- **Fix idea:**
  - On the trek legs (Dingboche to Base Camp), remove the rope. Guide with a worn path instead: a path mask from
    `route.json` in the landscape material, plus cairns, mani stones and flag lines at the turns.
  - From the icefall up, make the rope look real:
    - faded real colours, not saturated yellow;
    - a braided normal map;
    - sag between anchors;
    - partly buried, with snow on top.
- **Question for you:** does the trek need the rope for gameplay or guidance?

### T3 · The moraine ground looks smeared — **P2, check first**
- **Seen in:** `Saved/Review/A4/route_20261120_094757_L0-3_09000m.png`, `..._10000m.png`, `..._11000m.png`
- **Likely cause:** three things add up: low texture resolution on the detail tiles (old to-do #38), the body smear
  (G1) and the robot's 4x motion blur.
- **Fix idea:**
  1. Look at it again at 1x speed with body effects off.
  2. If it is still soft, use a downloaded scanned moraine/scree material (2k) with a detail normal.
  3. Add a little more small rock, only within about 30 m of the path.

---

## 4. Everest Base Camp (5,364 m) and the glacier below it

### What reads well (keep)
- From Base Camp, the icefall at 300–800 m is the closest look to your reference photos so far: a dense blue-white
  jumble below dark rock (`Saved/Review/A4/route_20261120_104624_L1-3_04000m.png`,
  `Saved/Review/A4/route_20261120_110212_L2-3_01000m.png`).
- The canvas mess tents, the flags on the pole and the stone altar (`Saved/Review/A4/route_20261120_104624_L1-3_03000m.png`).

### B1 · Base Camp stands on clean white snow — **P2**
- **Seen in:** `Saved/Review/A4/route_20261120_104624_L1-3_03000m.png`, `Saved/Review/A4/route_20261120_110212_L2-3_00000m.png`
- **What breaks:** the real Base Camp sits on grey rubble on top of the glacier. It has many tents and prayer-flag
  lines radiating from the puja altar. A few tents on a white plain feel empty and generic.
- **Fix idea:**
  - A rubble zone in the landscape material under the camp.
  - More tents as cheap instances, in mixed colours.
  - Flag lines that radiate from one central pole above the altar. This is the iconic Base Camp image.

### B2 · The ice pinnacles below Base Camp look like clean crystals — **P2**
- **Seen in:**
  - `Saved/Review/A4/route_20261120_104624_L1-3_02000m.png`, `..._02800m.png`
  - `Saved/Review/A4/route_20261120_104624_L1-3_03000m.png`
- **What breaks:** tall, smooth, pale-blue needles stand straight on flat snow. Real Khumbu glacier towers are wider
  and dirty. They stand among grey rubble and melt pools.
- **Fix idea (material and placement script):**
  - Shorter and wider pinnacles.
  - Dirt bands and sun cups (melt hollows) in `M_SeracIce`.
  - A rubble floor around them, using free Megascans glacier/rubble textures.

---

## 5. Khumbu Icefall (5,400–5,725 m)

### What reads well (keep)
- The density, and the narrow winding path between the blocks (`Saved/Review/A4/route_20261120_110212_L2-3_02000m.png`).
- Snow pillows on the tops of the blocks.

### I1 · Up close, the ice looks like soft white marshmallow — **P2 (B7 names "glacier look")**
- **Seen in:**
  - `Saved/Review/A4/route_20261120_104624_L1-3_04400m.png`, `..._04800m.png`
  - `Saved/Review/A4/route_20261120_110212_L2-3_01600m.png`, `..._02200m.png`
  - `Saved/Review/A4/route_20261120_094213_L3-3_00002m.png`
- **What breaks:** the fracture faces are white and rounded. In the reference photos they are crisp, glassy and blue,
  darker in shade. White blocks on white snow give almost no contrast. The icefall should feel dangerous; here it
  feels soft.
- **Likely cause:**
  - The snow cover in `M_IcefallIce` reaches too far down the steep faces.
  - The pre-smoothing in the block builder rounded the edges.
  - The shadow sides are grey, not blue.
- **Fix idea (material first, no new geometry):**
  - Snow only where the surface is flatter than about 35°.
  - Fresh fracture faces in deep blue, with a fake depth look (darker and bluer at steep view angles) and sharp
    specular glints.
  - Edge wear in the normal map, for crisp edges.
  - Fine fracture lines and thin dirt layers.
  - A trodden path on the floor, with crevasse lines.
- **Note:** you have not judged the v7 test patch yet. Show this as a before/after first.

### I2 · The falling-ice event looks cheap — **P2**
- **Seen in:** `Saved/Review/A4/route_20261120_110212_L2-3_01200m.png` ("The ice above you groans – MOVE!")
- **What breaks:**
  - The falling block is a flat, camouflage-textured card.
  - There is no powder cloud.
  - It falls from open sky onto a flat snowfield, with no serac above.
- **Fix idea:**
  - Use the icefall block meshes and `M_IcefallIce` for the falling chunks.
  - Add a soft powder burst (Niagara, lit soft sprites) and a small camera shake.
  - Trigger the event only under real seracs.

### I3 · The flags at the icefall camp are oversized and very saturated — **P3**
- **Seen in:** `Saved/Review/A4/route_20261120_094213_L3-3_00002m.png`
- **Fix idea:** check whether the new printed lungta flags (11-20) have replaced these too. If not, use the same
  ones, at real size (about 30 x 25 cm).

---

## 6. Camp 1 (6,065 m) and the Western Cwm

**There is no fresh picture.** The A4 run had not got here yet. These notes use old pictures:
`Saved/Compare/Camp1_SnowWall0_fix2.png` and `Camp1_SnowPile1_fix2.png` (11-02), `Saved/Compare/cwm_a1.png` (11-03)
and `Saved/Backdrops/raw_cwm.png` (11-12).

### What reads well (keep)
- The Cwm in deep blue shade under a bright face (`Saved/Compare/cwm_a1.png`). That is the "quiet awe" mood. The
  Cwm, the "valley of silence", is the best place on the route for it.

### C1 · Lime-green tent tops — **P2, check**
- **What breaks:** the tents have the same bright green as the summit tent (S1).
- **Fix idea:** check whether the Camp 1 and Camp 2 tents also use the raw Fab green ripstop material. If so, give
  them the camp fabric (`MI_CampOchre` / `MI_CampRust`).

### C2 · Smooth white "egg" snow piles — **P2**
- **What breaks:** the snow piles and walls are smooth ellipsoids with no texture. They read as eggs, not drifts.
- **Fix idea:** wind-shaped drift forms (sastrugi texture, blended into the ground), or remove them.

### C3 · The Cwm floor is a smooth snow sheet — **P2**
- **Fix idea:** crevasse lines with ladders, and wind features. Then get a robot picture looking up at the Lhotse face.

---

## 7. South Col (7,906 m)

### What reads well (keep)
- The line of figures and tents, small in a wide space (`Saved/Review/A5/summit_20261120_084131_o2_1_southcol.png`).

### SC1 · The South Col is a soft white snowfield — **P2**
- **Seen in:** `Saved/Review/A5/summit_20261120_084131_o2_1_southcol.png`, `Saved/Review/A5/summit_20261120_062932_o2_1_southcol.png`
- **What breaks:** the real South Col is a windswept saddle of bare rock and blue polished ice, with little soft snow.
  Here it is a smooth white plain, and the oxygen bottles stand upright like black posts.
- **Fix idea:**
  - A material zone for the col: blue wind-polished ice, dark scree, thin snow in the lee.
  - Wind streaks on the ground.
  - Tents moving in the wind (vertex wind in the material, which is cheap).
  - Bottles lying in groups, half buried.

---

## 8. Summit ridge (8,000–8,800 m)

### What reads well (keep)
- The giant shadows of the World Tree falling across the snow (`Saved/Review/A5/summit_20261120_084131_o2_3_climb_8600.png`).
  This is the myth entering quietly, which is exactly the direction.

### R1 · The ridge cannot be judged from these pictures — **P2, needs new pictures**
- The robot looks straight down at the snow and the rope (`..._2_deathzone.png`, `..._3_climb_8300.png`).
- The few ridge shapes visible elsewhere are soft and pillowy (`..._6_summit_view_down.png`). The real ridge is
  narrow, with cornices hanging over the east side and rock steps.
- **Fix idea:**
  1. Have the summit robot take pictures at eye level, looking forward along the ridge.
  2. Then, if needed, add downloaded cornice and snow-ridge meshes at a few key points (the Hillary Step area). Add a
     wind-carved snow material on the ridge. No hand modelling.

### R2 · The rope lies on the snow as a dirty, flat strip — **P2**
- **Seen in:** `Saved/Review/A5/summit_20261120_084131_o2_3_climb_8600.png`, `Saved/Review/A5/summit_20261120_090548_o2_fast_3_climb_8600.png`
- **Fix idea:** the same rope look as in T2: braided, real colours, small sag, with snow on top in places.

---

## 9. Summit (8,849 m)

### What reads well (keep)
- **The view down is the strongest picture on the route** (`Saved/Review/A5/summit_20261120_084131_o2_6_summit_view_down.png`):
  - a warm band at the horizon;
  - cirrus wisps;
  - the curve of the far terrain;
  - the World Tree's shadow lying on the ridge below.
- The descent views (`..._7_descent.png`, `..._8_descent_end.png`): wide, layered and calm.
- The summit end screen over a blurred view (`..._5_summit_screen.png`).

### S1 · The bright green chequered tent — **P1, probably fixed, check**
- **Seen in:** `Saved/Review/A5/summit_20261120_084131_o2_4_summit_beat.png` (right edge, and a second one at the left)
- **Cause (known):** the summit camp tents used the raw Fab green ripstop material. Its tiny pattern turns into a
  lime-green checker.
- **Fix:** already done in commit `fbcec150` (the tents now use `MI_CampOchre` / `MI_CampRust`). Take a new summit
  picture to confirm, and check C1 at the same time.

### S2 · Frost and colour fringe on the summit picture — **P1**
- At 33.6 °C body temperature, the summit picture has a strong colour split on every edge and a blue-grey wash. See
  G1 for the fix: the "clarity moment" during the 8-second summit hold.

### S3 · Close up, the World Tree trunk reads as a grey rock wall — **P2, your call**
- **Seen in:** `Saved/Review/A5/summit_20261120_084131_o2_4_summit_beat.png`, `..._6_worldtree.png`, `..._6_worldtree_up.png`
- **What breaks:** the bark looks smeared and low-detail, with bright jagged white patches. From up close you cannot
  tell it is a tree. Your art direction wants the myth to be quiet and easy to miss. The tree's *shadow* does that
  very well; the trunk filling the frame does not.
- **Fix idea:**
  - A downloaded scanned bark material (2k, triplanar), with rime frost and snow on the upper sides.
  - Frame the summit turn so the tree reads as a silhouette against the cloud.
  - Let the upper trunk fade into cloud.
- **Question for you:** how literal should the tree be at the summit?

### S4 · Small props: the snow "egg", black gear "cards", and ropes hanging like wires in the sky — **P2**
- **Seen in:**
  - `Saved/Review/A5/summit_20261120_084131_o2_4_summit_beat.png` (a smooth white dome; black slots along the snow
    line)
  - `Saved/Review/A5/summit_20261120_084131_o2_6_summit_view_down.png` and `..._7_descent.png` (long, perfectly
    straight lines across the sky)
- **Fix idea:**
  - Replace the dome with a drift shape, or remove it.
  - Seat the gear in the snow, with some snow on it. Check that it is lit and not pure black.
  - Ropes and flag lines should sag and follow the snow. No line should cross the open sky without a pole.

### S5 · Fine horizontal stripes on the far mountains — **P2**
- **Seen in:** `Saved/Review/A5/summit_20261120_084131_o2_6_summit_view_down.png` (the middle band of the picture)
- **Likely cause (not proven):** banding in the far-terrain normal map or height data, or shadow aliasing on the far
  ring.
- **Fix idea:**
  1. Check the `M_FarTerrain` normal map format and source precision. Add a little dither noise.
  2. Compare one capture with the far ring's shadow casting off.
  3. Keep near and far terrain looking the same, as you asked on 11-03.

### S6 · The far view is white everywhere — **P2**
- **Seen in:** the same picture, and `..._7_descent.png`.
- **What breaks:** the real summit view has brown Tibetan plateau to the north and a sea of cloud in the southern
  valleys. Here everything is snow, which hides the height. Your reference images show valley cloud seas.
- **Fix idea:**
  - A snow line by height and slope direction in the far-terrain material: brown below about 5,500 m on the north.
  - A low cloud or fog band over the southern valleys (Ultra Dynamic Sky low clouds or height fog). This also helps
    G2.

### S7 · The white disc in the sky reads as a flat coin — **P3**
- **Seen in:** `Saved/Review/A5/summit_20261120_084131_o2_4_summit_beat.png` (upper left; probably a realm planet)
- You liked the planets, so this is only a check. Make sure this one has the "heavy" look: limb darkening (darker
  toward its edge) and a terminator (the line between its lit and dark sides).

---

## 10. Floating on slopes (A6 pictures)

### F1 · Bare hands in the snow at altitude — **P2**
- **Seen in:** `Saved/Review/floating_A6/after_slide_46deg.png`, `Saved/Review/floating_A6/after_walk_36deg.png`
- **What breaks:** bare skin on a 46° snow slope. No climber does this, and players will notice at once.
- **Fix idea:** mittens or gloves on the first-person arms, as a downloaded glove asset or a material swap on the
  hands.

### F2 · Rock patches on the slope look like flat beige paint, and the cliff texture is stretched — **P2**
- **Seen in:** `Saved/Review/floating_A6/after_slide_46deg.png`, `Saved/Review/floating_A6/before_slide_46deg.png`
- **Fix idea:**
  - Darker blue-grey rock instead of sandy beige.
  - Triplanar mapping on steep faces (the same family as old to-do #38).
  - A height-based snow/rock blend, so the snow sits *on* the rock and is not painted beside it.

---

## 11. Not new, or already fixed (no action)

- `Saved/Review/A5/summit_20261120_062932_o2_4_summit_beat.png` and `..._6_summit_view_down.png`: an orange or brown
  full-screen view. That early run counted the summit while the robot was *inside* the mountain. Fixed on 11-20.
- `Saved/Review/floating_A6/before_bugitgo_walk_stuck.png`: the stretched body. That was the no-gravity BugItGo case,
  fixed on 11-20.
- These are still open and still part of B7, from the issue log: hard stepped shadows on the ridges and summit (#5,
  #13, #18, #25), the dirty triangle near Camp 1 (#28), a shadow line that moves with the camera (#30), blocky
  footprint shadows (#68), and rock treated as snow by the trail (#69). These pictures did not show them clearly, so
  they need their own test spots.

---

## 12. B7 also asks for one dawn sky moment

**My suggestion: sunrise on the summit push, between the South Col and the Balcony (8,400 m).**
- Real climbers leave the col at night and meet the dawn on the Balcony. So this moment is true to life.
- The art direction's dawn: tiny warm apricot areas far away while the near snow stays blue.
- Everest casts a famous pyramid-shaped shadow onto the morning haze to the west. It can be faked cheaply with one dark
  translucent wedge in the haze.
- Add a cloud sea below (S6), and the World Tree's cirrus above the summit.
- **Cost:** a time-of-day key on the route (about 04:30 at the col to 06:00 at the Balcony) and one wedge mesh. No new
  heavy assets.
- **Your call:** this moment, or dawn when leaving Dingboche?

---

## Top 10, in order

The order weighs how much each item breaks immersion against how cheap it is. Each item names the issues it covers.

1. **Calm the body effects** (G1, S2). Start the colour split later, keep the frost at the edges only, and add a
   "clarity moment" at the summit and in dialogue. Settings only; it improves every picture above 5,000 m and the
   summit picture.
2. **One soft subtitle veil, and hide the HUD while a subtitle shows** (G6). A small UI change, seen in every story beat.
3. **Fix the obvious prop bugs in the two most important frames** (S1 check, D1, S4). The first tent shot: the
   floating photo. The summit beat: confirm the tent fix, the snow egg, the black gear, the wire ropes.
4. **Check, then stop, the camera going under the ground** (G5). Check first; it is P1 if real.
5. **Bring back clouds and cloud light** (G2, with S6). A partly cloudy demo weather, a time of day per stretch, and
   valley cloud layers. Measure the fps.
6. **Remove the trees above about 4,000 m and add low scrub** (T1, D2). A settings change that also saves frame time.
   Dingboche is your call (question 2).
7. **Fix the "dalmatian" rock blots on the mountains** (G3). A material change, plus an optional baked mask (about
   11 MB).
8. **Decide on the trek rope, and make the real ropes look real** (T2, R2). Needs your decision first.
9. **Give the snow plains value and texture** (G7). Exposure, albedo and wind features, keeping the snow you liked.
10. **Icefall close-up look and the falling-ice event** (I1, I2). The blue fracture faces, crisp edges, powder burst.
    This is the "glacier look" B7 names. Show a before/after first.

After the top 10: Base Camp on rubble (B1), the glacier pinnacles (B2), the South Col material (SC1), gloves (F1), the
far-terrain stripes (S5), and the dawn moment (section 12).

## Questions for you

1. **Trek rope.** Does the trek from Dingboche to Base Camp need the rope for gameplay? If not, I suggest a worn path
   with cairns instead.
2. **Trees.** Do you want any trees at Dingboche on purpose? The real treeline is about 4,000 m.
3. **Dingboche colour.** Should Dingboche stay fully white, or become brown and patchy, which is more true to
   November?
4. **The World Tree.** How literal should it be at the summit: a trunk you stand next to, or mostly a shadow, a
   silhouette and cloud?
5. **Dawn.** Should the dawn moment be on the summit push (my pick) or when leaving Dingboche?
