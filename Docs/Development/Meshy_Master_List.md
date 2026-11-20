# Meshy master asset list (Chapters 2-7)

Version 1.0 · 2026-11-20 · written by Claude for the director. **Nothing here is sent or bought.** Buying credits is
the director's money and the director's decision.

Sources: the asset sections of `Chapter2_Design.md` ... `Chapter7_Design.md`, `Tools/Meshy/assets.py` (house style),
memory note `meshy-pipeline`, the folder list of `Content/Story/AI`, `Downloads/polyhaven/CATALOG.md`.

## 0. How to read this

| Item | Rule used here |
|---|---|
| Chapter numbers | the numbers of the design files. **Warning:** `Chapter3_Design.md` and `Chapter5_Design.md` both design "The Icefall" (two different chapter lists). Their assets are merged here, so nothing is bought twice. |
| Price of a prop | concept image 3 + textured image-to-3D 30 = **33** (36 with the better concept model `nano-banana-2`) |
| Price of a character | concept 6 + model 30 + rig 5 = **41** (a failed rig is refunded) |
| Price of an animation | **3** per action. Bought once on one rigged character, then retargeted in Unreal to all others (the IK retarget pipeline already exists). Whether Meshy's animation library has each action is [To verify]; Mixamo is the free fallback. |
| Retries | round 1 had 0 failures in 32 models, so plan **about 10%** extra, not 20% |
| Prompt | the `what=` text only; `PROP_STYLE` / `CHAR_STYLE` in `assets.py` wrap it. Full prompts are in the chapter files. |
| Priority | **A** = the chapter does not work without it · **B** = nice, or a free/assembled fallback exists · **C** = later, or almost certainly replaced for free |
| Colours | colour variants (other teams' tents, jackets) are made in Unreal, never generated again |

### 0.1 Already made (35 folders in `Content/Story/AI`, all reused, 0 credits)

`barrel_blue` `bedding` `butter_lamp` `choktse` `chorten_a/b/c` `cook_set` `copper_drum` `daypack` `doko` `duffel`
`dung_heap` `father_stone` `glacier_glasses` `headlamp` `khata_folded` `khata_tied` `meat_slats` `offering_bowl`
`porter_load` `stove_set` `stream_hole` `tarchen` `tent_dome` `tent_kitchen` `tent_mess` `tent_toilet` `water_bottle`
· characters: `sangye` `father` `father_sit` `dolma` `nima_ai` `marek_ai` `elena_ai`.

Credits now: **about 7** (morning report 2026-11-20). `meshy.py` has **FLOOR = 300** (it stops when the balance would
drop below 300). With a 1,000 purchase only about 700 can be spent unless the floor is lowered (decision D1).

## 1. The master table (68 items, deduplicated)

"Merged from" lists the ids used in the chapter files, so each chapter's text can be traced.

### 1.1 Characters (rigged, 41 each)

| id | One-line prompt (`what=`) | Ch. | Pri | Cr | Merged from / reuse notes |
|---|---|---|---|---|---|
| `sangye_climb` | a Sherpa man of about 22, short black hair, red down jacket with a darker patch on the left elbow, black harness with slings and carabiners, dark grey softshell trousers, thin black liner gloves, black-and-orange insulated double boots with steel crampons | 3 5 6 | A | 41 | = `sangye_climb` (3), `sangye_icefall` (5), `sangye_high` (6). The player body above Base Camp |
| `nima_climb` | a Sherpa guide of about 45, weathered face, little stubble, faded mustard-yellow down jacket with charcoal shoulder panels, white helmet with headlamp, glacier glasses on the helmet, harness with carabiners and a jumar, worn grey softshell trousers, black double boots with crampons, mittens on wrist cords | 3 5 6 | A | 41 | = `nima_climb`, `nima_icefall`, `nima_high` |
| `climber_icefall` | a mountaineer in a faded orange down jacket and black insulated trousers, harness, white helmet with headlamp, neck gaiter over the nose and dark goggles hiding the face, black double boots with crampons, big black mittens | 3 4 5 6 7 | A | 41 | = `climber_icefall` (5), `icefall_sherpa` (3). Faceless, so one model is every other team, rope fixer and client: recolour in Unreal |
| `hale` | a Norwegian woman doctor of about 52, fair weather-reddened skin, ash-blond and grey hair tied back under a dark blue beanie, reading glasses on a cord, slate-blue down jacket over a grey fleece, dark softshell trousers, insulated brown boots, stethoscope in a pocket | 4 7 (voice 2 6) | A | 41 | = `hale_ai` (4), `hale` (7). Ch4's prompt does not say man or woman; Ch7 says woman (decision D4) |
| `lama` | a Sherpa Buddhist lama of about 60, calm weathered face, close-cropped grey hair, maroon monastic robes with a saffron sleeveless shirt, maroon shawl over the left shoulder, thick maroon down jacket open over the robes, old brown trekking boots | 4 | A | 41 | = `lama_ai`. [Cultural check] before sending |
| `pasang_cook` | a Sherpa expedition cook of about 50, round friendly sun-darkened face, short black-grey hair, black beanie, old red down vest over a faded navy fleece, stained cotton apron, dark track trousers, worn trekking shoes | 4 7 | A | 41 | = `pasang_ai` (4), `pasang` (7). Name clash with Ch2's guide Pasang (decision D3) |
| `diki` | a Sherpa woman of about 40 who runs a lodge, red cheeks, long black braid under a faded floral headscarf, dark maroon wrap dress over a pink blouse, striped rainbow apron, open grey fleece, black shoes | 2 | A | 41 | [Cultural check]: the striped apron is worn by married women |
| `trekker` | a European man of about 26 on a budget trek, pale sunburnt face, fair stubble, messy blond hair under a cheap grey beanie, thin light-blue jacket over a grey hoodie, black tights under baggy shorts, trail-running shoes | 2 | A | 41 | the HACE stranger ("Jonas"); needs `anim_stumble` |
| `becker` | a German man of about 57, pale skin, sunburnt nose, cracked lips, grey stubble, tired eyes, faded dark blue down jacket over grey fleece, black insulated trousers with harness, grey beanie, yellow-and-black double boots | 6 | A | 41 | = `becker_ai` (Paul). Free fallback: the freed MetaHuman `MH_Marek_Bruce` |
| `elena_climb` | `elena_ai` text with a white helmet and headlamp, new grey harness, new grey-and-teal mountaineering boots with crampons | 3 5 6 | B | 41 | = `elena_climb`, `elena_high`. Cheaper: attach `climbing_helmet` + `crampons` to the existing `elena_ai` sockets |
| `marek_climb` | `marek_ai` text with an orange helmet and headlamp, black harness, black double boots with crampons | 3 5 6 | B | 41 | = `marek_climb`, `marek_high`. Same socket fallback |
| `jiwoo_climb` | a slim Korean man of about 25, dark blue helmet with headlamp, new bright blue down jacket, action camera on the shoulder strap, harness, black softshell trousers, black double boots with crampons | 3 5 6 | B | 41 | = `jiwoo_climb`, `jiwoo_high`. Ji-woo is a MetaHuman today |
| `ang_dorje` | a Sherpa Icefall doctor around 60, deeply lined sunburnt face, grey stubble, glacier glasses on a faded cap, old patched one-piece down suit open with arms tied round the hips, harness with carabiners and a hammer, heavy boots | 4 7 | B | 41 | = `icefall_doctor` (4, aged 35), `ang_dorje` (7, aged 60): pick one age. Fallback: aged porter MetaHuman |
| `pasang_guide` | a Sherpa climbing guide of about 55, deeply lined face, grey stubble, black beanie, faded red-and-black down jacket shiny at the cuffs, black softshell trousers, harness, brown leather mountaineering boots | 2 | B | 41 | Ch2's `pasang`. Fallback: re-dressed MetaHuman (Ch2 says so) |
| `sherpa_climber_b` | a young Sherpa guide of about 28, faded blue hooded down jacket, black softshell trousers, grey beanie, brown leather mountaineering boots | 4 | C | 41 | recolour `climber_icefall` or `nima_climb` instead |

### 1.2 Props (33 each)

| id | One-line prompt (`what=`) | Ch. | Pri | Cr | Merged from / reuse notes |
|---|---|---|---|---|---|
| `lodge_main` | a two-storey Khumbu trekking lodge of rough grey dry-laid stone, small blue wooden windows with many panes, faded blue corrugated tin roof weighted with rocks, solar panels and a black water tank on the roof, stone terrace, no signboard | 2 | A | 33 | the chapter's main place. Meshy scales everything to ~1.9 m: rescale to 14 m on import. Test item |
| `sang_burner` | a Sherpa juniper incense burner: a whitewashed stone-and-clay altar about 1.2 m tall with a smoke-blackened fire bowl on top, ash and green juniper sprigs | 2 | A | 33 | smoke = Niagara |
| `lhapso_altar` | a square Sherpa puja altar of stacked flat granite stones, 1.6 m tall, sooty fire niche in front, flat offering shelf on top, a socket for a tall pole, on glacier rubble | 2 3 4 7 | A | 33 | replaces the Base Camp altar dressing (Ch3 asks for "a better one") |
| `juniper_sang` | a bundle of fresh dwarf juniper branches with blue-grey berries tied with a cloth strip, one end charred and smouldering on a flat stone, grey ash, a few rice grains | 2 3 4 5 | A | 33 | = `juniper_offering` (3), `juniper_bundle` (4), `juniper_sang` (5). Put on Poly Haven `stone_fire_pit` |
| `phone_rugged` | a modern smartphone in a scuffed black rugged rubber case, screen dark and blank, a thin wrist cord | 2 3 5 | A | 33 | Ji-woo's phone (the video). Screen = a texture |
| `crampons` | a pair of steel 12-point crampons with long front points, orange anti-balling plates, black nylon straps with buckles, side by side, scratched and dull | 3 4 5 6 | A | 33 | = `crampons_pair` (3), `crampons` (5). Also the socket prop for B characters and part of `gear_pile`. Test item |
| `ice_anchor` | a steel ice screw drilled into blue ice, bent hanger, short faded red sling, steel carabiner, faded blue fixed rope through it, frost on the metal | 3 5 | A | 33 | = `ice_anchor` (3 and 5); place along `AFixedRope` |
| `carabiner_sling` | a climber's safety leash: short grey sewn sling, two steel locking carabiners, a mechanical ascender with a dark grey handle, worn | 5 6 | A | 33 | the clip-in close-up |
| `climbing_helmet` | a scuffed white mountaineering helmet with a black headlamp strapped round it, chin strap hanging open | 3 5 6 | A | 33 | socket prop for Marek, Elena, Ji-woo (saves 3 × 41) |
| `load_pack` | a 70-litre expedition rucksack in faded red, packed full, two orange oxygen cylinders strapped upright on the back, coiled rope under the lid, yellow foam mat on the side, dusty | 3 5 | A | 33 | = `load_pack` (3), `expedition_pack` (5). Sangye's carry |
| `radio_vhf` | a rugged black handheld VHF radio, stubby rubber antenna, worn belt clip, scratched, on a short orange cord | 3 4 6 7 | A | 33 | = `radio_handset` (3), `radio_vhf` (6). Every radio call |
| `puja_offerings` | a puja offering spread on a low wooden tray with a white cloth: brass bowl of roasted barley flour, bowl of rice, plain beer cans, an unlabelled bottle, biscuits, apples, oranges | 4 | A | 33 | also covers Ch2's `tsampa_offering` |
| `gamow_bag` | a portable hyperbaric chamber: a 2.1 m red-and-grey coated nylon tube with a clear window at the head, half inflated on a foam mat, a foot pump with a hose | 4 | A | 33 | |
| `med_station` | an expedition doctor's station: folding aluminium table, open grey hard case of medicine boxes, blood-pressure cuff, finger oximeter, stethoscope, small oxygen cylinder with regulator beside it, no cross symbols | 4 6 7 | A | 33 | = `med_station` (4), `medical_kit` (7). Add Poly Haven `medical_box`; the cot from Ch7 = a separate Fab/PH item |
| `oximeter` | a small fingertip pulse oximeter, matte grey plastic clip, dark blank display, short black lanyard, scuffed | 4 6 | A | 33 | the hand-held close-up in Ch6. Display numbers = a texture |
| `plan_board` | a portable whiteboard on a folding easel on glacier rubble, blank and slightly smudged, marker tray with three markers, a few round magnets | 4 7 | A | 33 | = `plan_whiteboard` (4), `plan_board` (7). The drawing is a decal (can change) |
| `hanging_stove` | a compact high-altitude canister stove hanging from three thin steel cables: tall insulated aluminium pot with lid on a burner, squat grey gas canister under it, soot, frost, no logos | 6 | A | 33 | `stove_set` is a kerosene stove: wrong at 6,000 m |
| `double_boots` | a pair of high-altitude double boots, stiff yellow-and-black insulated shells with gaiter zip, grey foam inner boots half pulled out, frost on the shells | 5 6 | A | 33 | the 04:30 boots scene |
| `father_notebook` | a cheap Nepali school exercise notebook, soft blue faded water-stained cover, curled corners, a rubber band round it, inside a clear zip bag, no readable text | 7 | A | 33 | pages = textures |
| `lodge_annex` | a single-storey stone hut, faded green tin roof weighted with rocks, one small blue window, thin black stove pipe, dung patties stacked against a wall | 2 | B | 33 | a recoloured `lodge_main` part can stand in |
| `mani_stones` | a low pile of flat grey stones carved with Tibetan mantras painted faded white and ochre, worn so the letters cannot be read, lichen | 2 | B | 33 | free: Poly Haven `rock_07` + carved decal |
| `yak_stand` | a shaggy black-and-white yak standing, head low, long belly hair, curved horns, wooden pack saddle with two empty sacks, red wool tassel | 2 | B | 33 | static (Meshy rigs humanoids only). Check Fab for a free yak first |
| `torma_plate` | three barley-and-butter torma cakes on a round brass plate, red cones about 25 cm tall with small white butter discs | 4 | B | 33 | [Cultural check] before sending: shapes differ by ritual |
| `kitchen_counter` | a long low dry-stone wall topped with stone slabs and a plywood board, two dented pressure cookers, kerosene burner under a big aluminium pot, stacked steel plates and cups, two rice sacks | 4 7 | B | 33 | free: assemble from `cook_set`, `stove_set`, PH pots, `small_lpg_tank`, crates, `stone_wall` texture |
| `camp_table_chairs` | a long aluminium folding camp table with flowered plastic cloth and folding chairs in faded blue, thermoses and enamel mugs on it, dusty | 4 7 | B | 33 | = `camp_table_chairs` (4), `mess_table_set` (7). Free first: PH `folding_wooden_stool` + Fab table |
| `radio_table` | an expedition radio station on a folding table: black VHF base radio with hand mic, two handhelds in a charger, car battery, cables, a paper log book | 7 | B | 33 | free: assemble from `radio_vhf` + table + battery |
| `gear_pile` | ice axes, crampons, a harness with carabiners and a coiled orange rope leaning against a low stone wall | 4 | B | 33 | free: assemble from `crampons`, `climbing_helmet`, the existing ice axes and rope coil |
| `old_o2_bottle` | an old 1990s orange steel oxygen cylinder, paint chipped pale, brass valve, no regulator, frost, lying on its side | 3 | B | 33 | free: existing `SM_O2_Bottle` + Poly Haven `rusty_metal_04` |
| `figure8` | a scratched steel figure-eight descender with a locking carabiner on a short length of blue fixed rope | 6 | B | 33 | Fab first |
| `route_wand` | a thin 1.2 m bamboo marker wand with a small faded red-orange nylon flag, slightly bent, icy at the foot | 5 | B | 33 | a cylinder + cloth card is almost as good |
| `tent_highcamp` | a two-person four-season dome tent in faded yellow on snow, grey poles outside the fly, door zipped, snow along the skirt, guy lines to buried stakes, clean fabric | 6 | B | 33 | free: snow material on `tent_dome` |
| `ladder_bundle` | two aluminium ladder sections lashed side by side with old blue-and-white rope, snow in the rungs, a rope coil and four snow pickets tied on top | 7 | B | 33 | free: existing ladder sections + `SM_SnowPicket` + rope |
| `solar_panel` | two dusty solar panels on a wooden frame on rubble, a car battery and black cables at the foot | 4 | B | 33 | Fab first |
| `satellite_dish` | a small grey satellite dish on a tripod weighted with stones, a cable running away, dusty | 4 | B | 33 | Fab first |
| `yak_lie` | a shaggy black yak lying on stony ground, legs folded, curved horns, red tassel | 2 | C | 33 | only if `yak_stand` looks good |
| `tsampa_offering` | a small open cloth bag of roasted barley flour beside a wooden bowl with butter and juniper sprigs | 2 | C | 33 | covered by `puja_offerings` |
| `phone_old` | a cheap old button mobile phone, cracked screen corner, blue tape on the battery cover | 7 | C | 33 | can stay hidden in Nima's glove |
| `o2_rack` | a wooden rack of twelve orange oxygen bottles in two rows, taped numbers, regulators and masks in a crate | 7 | C | 33 | free: 12 × `SM_O2_Bottle` + PH `wooden_crate_02` |
| `tent_mess_torn` | the yellow mess dome after a storm: one panel torn and flapping, a bent pole, snow drifted inside | 7 | C | 33 | free: hidden-panel trick on `tent_mess` |
| `cache_tin` | a battered square steel cache box, chipped olive paint, frost in the seams, red tape across the lid | 5 | C | 33 | free: retinted PH `medical_box` |
| `snow_pot` | a dented aluminium pot filled with chunks of hard snow, the lid leaning against it | 6 | C | 33 | free: PH `pot_enamel_01` + snow material |
| `journal_book` | a small worn hardback notebook, faded red cloth cover, elastic band, pencil stub, wavy pages | 6 | C | 33 | |
| `med_pouch` | a small red zipped first-aid pouch lying open, foil blister packs, a roll of white tape, no text | 6 | C | 33 | |
| `gas_canister` | a squat 450 g gas canister, plain grey-blue steel, a small dent, frost on the lower half | 6 | C | 33 | **exists**: `SM_Food_GasCanister` (`/Game/Props`); also inside `hanging_stove` |

### 1.3 Animations (3 each, bought once, retargeted to every character)

| id | Action | Ch. | Pri | Cr | Notes |
|---|---|---|---|---|---|
| `anim_sit_cross` | sit cross-legged, idle breathing | 4 6 7 | A | 3 | try `father_sit` first (Ch4 says so); buy only if it does not fit |
| `anim_kneel` | kneel on both knees, idle | 2 4 | A | 3 | puja, sang fire |
| `anim_stumble` | drunk-like stumbling walk, grabbing for support | 2 6 | A | 3 | HACE (the trekker, Paul). Mixamo fallback |
| `anim_cook_stir` | standing cooking loop, stirring a pot | 4 7 | A | 3 | Pasang |
| `anim_chant_drum` | seated chanting with a hand drum | 4 | B | 3 | the lama |
| `anim_hammer` | kneeling hammering (pickets) | 4 | B | 3 | |
| `anim_lash` | lashing a ladder with rope | 4 7 | B | 3 | |
| `anim_ladder_cross` | walking across a horizontal ladder, careful steps | 3 5 | B | 3 | Ch3 risk: NPC feet slide on ladders |
| `anim_throw` | underhand throw (tsampa) | 4 | B | 3 | |

## 2. Totals

### 2.1 Per priority (before retries)

| Priority | Characters | Props | Animations | Items | Credits | With 10% retries |
|---|---|---|---|---|---|---|
| **A** | 9 (369) | 19 (627) | 4 (12) | 32 | **1,008** | ≈ 1,110 |
| **B** | 5 (205) | 15 (495) | 5 (15) | 25 | **715** | ≈ 790 |
| **C** | 1 (41) | 10 (330) | 0 | 11 | **371** | ≈ 410 |
| **All** | 15 | 44 | 9 | 68 | **2,094** | ≈ 2,300 |

The chapter files alone added up to about 2,900 with their duplicates; merging saves about **800 credits**.

### 2.2 Per chapter

"Used by" counts every item the chapter shows (shared items count in each). "New spend" counts an item only in the
first chapter that needs it, in file order 2 → 7 (this is the money if chapters are built in order).

| Chapter (design file) | A used by | B used by | C used by | **A new spend** | A cumulative |
|---|---|---|---|---|---|
| 2 The Hidden Mountain (Lobuche) | 253 | 140 | 66 | **253** | 253 |
| 3 The Icefall (12-chapter list) | 387 | 159 | 0 | **288** | 541 |
| 4 Base Camp | 470 | 251 | 41 | **294** | 835 |
| 5 The Icefall (v1 list) | 387 | 159 | 33 | **66** | 901 |
| 6 First Night High | 434 | 189 | 132 | **74** | 975 |
| 7 Thick Air | 294 | 176 | 99 | **33** | 1,008 |

If Chapter 3 and Chapter 5 are one chapter (decision D2), Chapter 5's new spend moves into Chapter 3: nothing changes in
the total.

## 3. Recommended purchase: the offered 1,000 credits

Balance after buying: about **1,007**. The A list is 1,008, so 1,000 does **not** cover all of A plus retries.
Two free swaps make it fit: `becker` → MetaHuman `MH_Marek_Bruce` (−41), `oximeter` folded into `med_station`
close-ups (−33). A then costs **934**, leaving **73** for retries. Lower `FLOOR` in `meshy.py` from 300 to about 50
for this round (D1).

| Step | What | Credits | Running total |
|---|---|---|---|
| 0 | Buy 1,000 (director). Check with `meshy.py balance` | | 1,007 left |
| 1 | **Test three** (house rule): `sangye_climb` (gear on a character), `crampons` (thin metal), `lodge_main` (14 m building) | 107 | 107 |
| 2 | Director looks at the three. If one fails, fix the prompt before step 3 | 0 | |
| 3 | Chapter 2 rest: `diki`, `trekker`, `sang_burner`, `lhapso_altar`, `juniper_sang`, `phone_rugged`, `anim_kneel`, `anim_stumble` | 220 | 327 |
| 4 | Icefall (Ch3 + Ch5): `nima_climb`, `climber_icefall`, `ice_anchor`, `carabiner_sling`, `climbing_helmet`, `load_pack`, `radio_vhf`, `double_boots` | 280 | 607 |
| 5 | Base Camp (Ch4): `hale`, `lama`, `pasang_cook`, `puja_offerings`, `gamow_bag`, `med_station`, `plan_board`, `anim_sit_cross` (only if `father_sit` fails), `anim_cook_stir` | 261 | 868 |
| 6 | Ch6 + Ch7: `hanging_stove`, `father_notebook` | 66 | 934 |
| | Reserve for retries | ≈ 73 | 1,007 |

One batch per step (the director prefers one approval per batch). Meshy deletes files after 3 days: download every
batch the same day (`meshy.py` does this).

**Next purchase (later):** about **800** buys all of B (or about 1,200 for B + C). Cheapest B first: the
animations (15), then `elena/marek/jiwoo_climb` only if the socket props look wrong.

## 4. Free instead of Meshy (Poly Haven CC0 already downloaded, Fab, or assembly)

Check these **before** sending anything (director's rule: download quality assets first).

| Meshy id | Free replacement | Source | Saves |
|---|---|---|---|
| `gas_canister` | `SM_Food_GasCanister` | `/Game/Props` (exists) | 33 |
| `old_o2_bottle` | `SM_O2_Bottle` + `rusty_metal_04` texture | exists + PH | 33 |
| `o2_rack` | 12 × `SM_O2_Bottle` + `wooden_crate_02` | exists + PH | 33 |
| `mani_stones` | `rock_07` scaled up + carved-mantra decal, `stone_wall` texture | PH | 33 |
| `kitchen_counter` | `cook_set`, `stove_set`, PH brass/enamel pots, `small_lpg_tank`, `plastic_crate_02`, `stone_wall` | exists + PH | 33 |
| `camp_table_chairs` | `folding_wooden_stool` + a free table from Fab | PH + Fab | 33 |
| `med_station` (part) | `medical_box` beside it | PH | 0 (adds detail) |
| `cache_tin` | `medical_box`, retinted olive | PH | 33 |
| `snow_pot` | `pot_enamel_01` + snow material | PH (in `/Game/Story/PH`) | 33 |
| `juniper_sang` (base) | `stone_fire_pit` under the bundle | PH | 0 (adds detail) |
| `gear_pile` | `crampons` + `climbing_helmet` + existing axes + rope coil | assembly | 33 |
| `radio_table` | `radio_vhf` + Fab table + battery; `portable_generator` nearby | assembly + PH | 33 |
| `ladder_bundle` | existing ladder sections + `SM_SnowPicket` + rope | assembly | 33 |
| `tent_highcamp` | `tent_dome` with a clean snow-camp material | material | 33 |
| `tent_mess_torn` | hidden torn panel on `tent_mess` | material/mesh trick | 33 |
| `tsampa_offering` | inside `puja_offerings` | Meshy A item | 33 |
| `becker` | freed MetaHuman `MH_Marek_Bruce` | exists | 41 |
| `pasang_guide` | re-dressed MetaHuman | exists | 41 |
| `sherpa_climber_b`, `icefall_sherpa` | recoloured `climber_icefall` / `nima_climb` | Meshy A item | 41 each |
| `elena/marek/jiwoo_climb` | `climbing_helmet` + `crampons` on head/foot sockets | Meshy A items | 123 |
| `figure8`, `solar_panel`, `satellite_dish`, `yak_stand` | look on Fab (free) first | Fab | 33 each |
| barrels, crates, generator, binoculars, lpg tank | `Barrel_02/03`, crates, `portable_generator`, `binoculars`, `small_lpg_tank` | PH (no Meshy item needed) | — |

**Not Meshy at all:** the tent interior (missing since Chapter 1; needed in Ch4, 6, 7 three times): Meshy makes closed
objects, so look on Fab / Megascans or build an inner tent in Blender. Smoke, spindrift, avalanche clouds = Niagara.
Plan-board drawing, notebook pages, phone screen, name tape = textures/decals.

## 5. Decisions for the director

| # | Decision | Default if no answer |
|---|---|---|
| D1 | Buy 1,000 credits, and lower `meshy.py` FLOOR from 300 to about 50 for this round? | wait |
| D2 | `Chapter3_Design.md` and `Chapter5_Design.md` are both "The Icefall": which one stays? (assets are already merged) | Ch3 (12-chapter list) |
| D3 | Two people called Pasang: Ch2's guide (55) and Ch4/7's cook (40-50). Rename one? | the cook keeps "Pasang" |
| D4 | Dr. Hale: woman (Ch7) or not stated (Ch4)? | woman |
| D5 | Ang Dorje, the Icefall doctor: about 35 (Ch4) or about 60 (Ch7)? | 60 |
| D6 | Accept the two swaps (`becker` → MetaHuman, `oximeter` into `med_station`) so A fits in 1,000? | yes |
| D7 | Helmet and crampons as socket props on Elena, Marek, Ji-woo instead of three new characters (−123)? | yes, test first |
