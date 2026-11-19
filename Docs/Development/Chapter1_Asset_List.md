# Chapter 1 · 3D assets to generate

Written 2026-11-19 for the AI asset pipeline. Built from `Chomolungma_Chapter1.md` §2.8, `Chapter1_Realism_Spec.md`,
`research_notes/NPC clothing realism/DECISION.md` and what `Tools/Story/build_chapter1_world.py` places today.

**Priority:** **P1** = the chapter looks fake or broken without it (today it is a stand-in). **P2** = authenticity, seen
up close. **P3** = nice to have, or only needed if a system gets built.

---

## Before you generate: what each export needs

So I can import and place every asset without a second round:

1. **Format:** GLB or FBX, one asset per file, saved under `F:/UE_project/YggdrasilsPeak/Downloads/ai_assets/<id>/`.
2. **PBR textures, de-lit:** base colour, normal, roughness (plus metallic where there is metal). The base colour must
   have **no baked light or shadow**. Many generators bake light in by default; pick their "PBR" or "de-light" option.
3. **Texture size:** 2k for hero assets (tents, memorial stones, characters), 1k for small props.
4. **Triangles:** small props 1-5k, medium props 5-15k, tents 15-30k, characters 40-60k. Static props can be denser,
   because they render with Nanite; characters cannot.
5. **No logos, text or brand look-alikes.** Never ask for a copy of a named product.
6. **Characters and clothes:** A-pose, arms about 45° down (the MetaHuman pose), feet on the ground. Clothes as
   separate pieces if the tool allows it (jacket, trousers, hat).
7. **Reference photos:** the dated photos in `Chapter1_Realism_Spec.md` §5 are good for clothing and building
   references. Never use a real person's face as input.

Real sizes are in the tables, so the scale in the file does not matter: I scale on import.

---

## One decision first: how the people are made

| Who | Recommendation | Why |
|---|---|---|
| Nima, Marek, Elena, Ji-woo, Pema, porters | **Keep the MetaHuman faces and bodies; generate only the clothes** (row 14). I fit and skin them to each body. | The faces are already good and can animate. AI faces are worse up close (Nima's face in the headlamp light is the last shot). |
| Dolma (8), the father, Sangye's first-person body | **Generate whole characters** (rows 11-13). I rig them to the UE skeleton with the IK Retargeter, as with the climber. | MetaHuman has no child body. The father is only seen far away in the dark. Sangye is only seen as arms, legs and a shadow. |

If you would rather generate the whole cast as AI characters, row 14 becomes six full characters instead of outfits.

---

## P1 · Must have

### Camps (Dingboche camp and the night camp)
Today every tent is one code-built dome, only recoloured.

| # | Asset | What it must look like | Real size | Notes |
|---|---|---|---|---|
| 1 | **Two-person expedition dome tent** | Geodesic 4-season dome, crossing poles, a vestibule at the door, guy lines, snow flaps at the base. Used and a little dirty, faded at the top. Colours: orange, yellow, red, green | floor 2.2 × 1.4 m, vestibule 0.8 m, 1.1 m tall | 3-4 colour variants, door closed and door open. **Needs a real inside** (inner tent, mats visible): the last scene is inside one |
| 2 | **Mess (dining) dome** | Large geodesic dome tent, one big door, a few windows, guy lines, the base weighted with rocks | 5-6 m across, 2.5-3 m tall | 1, maybe 2 colours |
| 3 | **Kitchen tent** | Tall frame or ridge tent: canvas or blue tarp, smoke-stained near the top, sides tied up, a stone wall at the base | 4 × 3 m, 2.2-2.5 m tall | 1 |
| 4 | **Toilet tent** | Narrow, tall pop-up tent, dull blue or green | 1.2 × 1.2 × 2.0 m | 1 |
| 5 | **Expedition duffel bags** | 90-120 L waterproof duffels, packed lumpy, straps and handles, scuffed. Yellow, blue, orange, red | 85 × 42 cm across | 4 colours. The orange one, patched, is the father's duffel in the kitchen (today it is a cylinder) |
| 6 | **Blue expedition barrel** | Blue plastic drum, black screw-clamp ring lid, scuffed, a strap around it | 60 L: 0.39 m across, 0.62 m tall | 1, plus a 120 L size if easy |
| 7 | **Sleeping bag and sleeping mat** | Mummy down bag, half unzipped, rumpled; closed-cell foam mat | bag 2.1 m, mat 1.8 × 0.5 m | For the night tent interior, 2 of each |

### The memorials at Thukla (the chapter's peak)
Today these are stacked stone slabs on a cliff rock, and the father's name is floating 3D text.

| # | Asset | What it must look like | Real size | Notes |
|---|---|---|---|---|
| 8 | **Memorial chortens** | Stepped dry-stone chortens and cairns of the Thukla pass: grey granite, lichen, some with a carved stone or metal plaque, some with a stick for prayer flags on top | 1-3 m tall | **3-4 variants** of different sizes. I scatter about 22 of them and string the flags |
| 9 | **The father's memorial stone** | A new stone with fresh, sharp-edged cuts, one flat smooth face for the name, set on a small stone base | about 0.6 × 0.3 × 0.8 m | Leave the face **blank**: I add the name as a decal, so it can change |
| 10 | **Khata (white silk scarf)** | Thin white silk with a woven pattern, fringed ends. Three states: folded flat in two hands; draped around a neck; tied around a stone | 1.5-2 m long, 30-40 cm wide | Replaces my code-built one. The neck state is seen when the player looks down |

### People

| # | Asset | What it must look like | Real size | Notes |
|---|---|---|---|---|
| 11 | **Dolma, 8-year-old Sherpa girl** (whole character) | Child proportions (head about 1/6 of her height), red wind-chapped cheeks, two braids. A faded fuchsia child's puffer a size too big (hand-me-down), navy track pants with 1-2 side stripes (never three), a striped knit hat with ear flaps (red, teal, yellow), small hiking boots. Dusty | 1.20-1.28 m | Rigged. She holds the player's hand. Full spec: Realism Spec §2.3 |
| 12 | **The father in an 8,000 m down suit** (whole character) | One-piece expedition down suit (red, the twin of Sangye's jacket colour), hood down, **no oxygen mask**, triple boots with a built-in gaiter in charcoal and slate blue, a harness. Sherpa man, about 48 | 1.68 m | Seen only far away, at night, sitting in the snow. Rigged (sits and turns his head) |
| 13 | **Sangye, the player's body** (whole character, first person) | Young Sherpa man, about 22. **Red down jacket patched at the elbow** (his father bought two), dark softshell trousers, thin gloves, mid trekking boots | 1.70 m | Seen as hands, sleeves, legs and a shadow. Replaces the current climber look. Rigged to the climber skeleton |
| 14 | **Seven outfits for the MetaHumans** | See the outfit table below | fitted to each body | Clothes only. Boots stay the fitted scans |
| 15 | **Porter load** | 2-3 duffels from row 5 stacked and roped together, a namlo strap (flat 4 cm band) that goes over the forehead, on a folded cloth | about 90 cm tall | Hangs on a porter's back. Porters carry loads on the trail |

**Outfits for row 14** (look A: dawn in camp. Full colours and fabrics in `Chapter1_Realism_Spec.md` §2.1-2.7)

| Who | Pieces |
|---|---|
| Nima | faded mustard hooded down jacket, a few seasons old · dark softshell trousers · faded maroon knit beanie with pompom |
| Marek | dark olive insulated hooded jacket, well used · charcoal softshell trousers · charcoal beanie |
| Elena | faded red down jacket **a size too big** (rented) · mid-grey trekking trousers · teal beanie |
| Ji-woo | matching navy puffer and trousers, new and expensive-looking · cream beanie · buff |
| Porter A | cheap oversized black puffer, **hood up over a folded beige head cloth** · olive cargo trousers rolled to mid-calf · grey buff over the mouth |
| Porter B | faded red cheap puffer over a grey hoodie · navy track pants · cap |
| Aunt Pema | long **sleeveless** dark wrap dress to the ankles (angi), no gold trim · mauve padded jacket over it · **striped front apron** (pangden) and a darker back apron · woven sash with a round silver buckle · bright floral headscarf tied at the nape |

---

## P2 · Authenticity up close

### Camp life

| # | Asset | What it must look like | Real size | Notes |
|---|---|---|---|---|
| 16 | **Tokma T-stick** | Porter's wooden walking stick with a T-shaped head to rest the load on | 1.0-1.2 m | |
| 17 | **Doko basket** | Conical woven bamboo basket with lodge supplies inside (kerosene can, crates) | 80 cm tall, 55 cm top, 28 cm base | Porter B |
| 18 | **Night camp stove set** | Kerosene or gas camp stove with a big dented aluminium pot and a sooty kettle on it | stove 30 cm across | Today: cylinders. The stove "roars" in the night scene |
| 19 | **Camp kitchen set** | Big aluminium pots, a pressure cooker, a two-burner gas stove on a stand | pots 30-50 cm | For the kitchen tent. The pressure cooker also goes in the house kitchen |
| 20 | **Folding camp chairs and a camp table** | Simple folding chairs, a folding aluminium table | seat 45 cm high | For the mess tent |
| 21 | **Solar panel on a stand** | Small panel on a metal frame, dusty, with a cable | 1 × 0.6 m | Camp, and one on the house roof |

### The Sherpa house kitchen (open items from the realism research)

| # | Asset | What it must look like | Real size | Notes |
|---|---|---|---|---|
| 22 | **Copper water drums** | Big wide-bellied copper water vessels, dents, dark patina, brass bands | 40-60 cm across, 50-70 cm tall | 3 variants. The family's wealth on the shelf wall; none exist in the downloads |
| 23 | **Small brass offering bowls** | Plain brass water bowls for the altar | 7-9 cm across | Replaces the goblets (a row of 7) |
| 24 | **Brass butter lamp** | Footed Tibetan butter lamp, brass, wax-stained | 8-12 cm tall | Replaces the Indian diya lamps (wrong religion) |
| 25 | **Low Tibetan table (choktse)** | Long low wooden table, painted panels or plain smoke-darkened planks | 35 cm high, 120 × 45 cm | Replaces the Chinese tea table |
| 26 | **Standing photo frame** | Simple wooden frame on a stand | 15 × 20 cm photo | The father's photo on the altar, with the khata over it |
| 27 | **Meat and cheese over the stove** | Strips of dried yak meat hanging on wooden slats; dried cheese cubes (chhurpi) on strings | slats 1 m | Hangs from the beams |
| 28 | **Yak-dung fuel** | A heap of dried dung patties, plus a patch of round patties drying flat on the ground | heap 1 m across, patties 20 cm | The yard |
| 29 | **Prayer pole (tarchen)** | Tall wooden pole with a long vertical prayer banner, faded | 4-6 m | Next to the house |

### Accessories (small, attached to the people)

| # | Asset | What it must look like | Real size | Notes |
|---|---|---|---|---|
| 30 | **Glacier glasses** | Wraparound sport sunglasses with side shields, plus a plain pair | 140 mm wide | Worn, or pushed up on the hat |
| 31 | **Buff / neck gaiter** | Tube scarf around the neck; one pulled up over the nose | | Clients, porters, Marek |
| 32 | **Handheld radio** | Small walkie-talkie with antenna, on a chest strap | 6 × 3.5 × 12 cm | Nima, Marek |
| 33 | **Daypacks** | 30-50 L trekking packs, 2-3 styles and colours, logo-free | | Clients and Marek. **Sangye's pack too** (demo blocker A6 is still waiting for your pick) |
| 34 | **Trekking poles** | Telescopic poles with straps | 110-135 cm | Clients, Marek |
| 35 | **Headlamp** | Headlamp with elastic band | | Nima in the night tent (his face in its light is the last shot) |
| 36 | **Water bottle** | 1 L wide-mouth bottle, in an insulated sleeve | 9 × 21 cm | Filled at the frozen stream; one of the load choices |
| 37 | **Ji-woo's filming kit** | Action camera on a chest harness; a phone gimbal | camera 6 × 4.5 × 3 cm | "Films everything" |

### Trail

| # | Asset | What it must look like | Real size | Notes |
|---|---|---|---|---|
| 38 | **Frozen stream hole** | A shelf of clear and white ice around an opening with dark running water underneath | 1.5 m across | Today: two flat discs. The player kneels and fills the bottle here |

---

## P3 · Nice to have, or only if a system is built

| # | Asset | What it must look like | Real size | Notes |
|---|---|---|---|---|
| 39 | **Father's things for an inspect view** | Prayer beads (108 beads); summit certificate in a plastic sleeve; three photo prints; Dolma's folded crayon drawing; the folded red down jacket with the elbow patch | hand-sized | **Only if the inspect view is built.** Today these are text cards |
| 40 | **First-aid kit** | Red zipped pouch | 20 × 14 × 8 cm | A load choice |
| 41 | **Fingertip pulse oximeter** | Small clip-on oximeter | 6 × 3.5 × 3 cm | The morning radio check |
| 42 | **Khumbu lodges** | Stone teahouses with blue-painted window frames and corrugated tin roofs (green, blue, red, rusty), 1-2 storeys, exterior only | 10-14 × 6-8 m | 2-3 variants. Dingboche is mostly lodges, seen from the house door |
| 43 | **Dry-stone field walls** | Modular wall pieces for the potato and barley plots | segments 2-3 m, 1 m tall | Around the house |
| 44 | **Mani stones and a mani wall** | Flat stones carved with a mantra, painted, piled on a long low wall; a small trail chorten | wall 4 m × 1.2 m | Along the trail. The carving needs a check by a Tibetan reader before release |
| 45 | **Yak with loads and a bell** | Domestic yak, long hair, a wooden saddle with loads, a bell at the neck | 1.4 m at the shoulder | Rigged and animated: the hardest item. Today porters stand in for the yak train |

**Upgrade only if they look fake in your shots:** the butter-tea churn, the mud stove and the rug in the kitchen, and
the stove, thermos and gas canister in `Content/Props`. I built all of these in code.

---

## Not on this list

- **Already good:** terrain, rocks, snow, sky; the prayer-flag lines; the rope and clips; the brass compass; the
  oxygen bottle; the fitted boot scans; the Sherpa house; the Poly Haven kitchen props (cabinets, shelves, brass pots,
  thermoses, barrel stove, jerrycans, propane tank, crates).
- **Not 3D:** the photos (renders of our own characters), the thangkas, the wall calendar, the father's name on the
  stone, the golden plume (a material card).
