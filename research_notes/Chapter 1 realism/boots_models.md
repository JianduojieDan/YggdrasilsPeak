# Boots: free 3D models you can download without an account, plus modelling references

Chomolungma, Chapter 1 (Dingboche, about 4,300 m, late autumn). Research date: 2026-11-14.
Scope: free footwear models that download **without login**, with a licence that allows a **commercial game**
(CC0 preferred, CC-BY acceptable). Three needs: (1) leather trekking boots for Sherpa guides and the older village woman,
(2) brand-neutral mountaineering boots for clients, (3) cheap canvas or synthetic trainers for a porter.

Method: I queried each site's public API or search where one exists, read licence fields from the API or metadata
rather than from listings, checked that each download URL works without a session (a 4-byte range request that returned
`PK` or `glTF`), and read triangle counts and texture sizes from the file headers (OBJ header lines, GLB JSON chunk, PNG/JPEG
headers). I judged realism from the official thumbnails at 1920 px. **I have not opened these models in Blender or UE.**

---

## 1. Bottom line

* **Sources with no realistic footwear:** Poly Haven (one model only: green rubber wellies), ambientCG (no footwear models at all),
  Kenney and Poly Pizza (stylised low-poly only), OpenGameArt CC0 (low-poly game props only). None of these has a realistic leather boot,
  mountaineering boot or trainer.
* **Two good no-login sources:**
  1. **Google Scanned Objects (GSO)** on Gazebo Fuel. These are clean scans of real retail shoes: **254 models in the "Shoe" category**,
     **CC BY 4.0**, about 16k-42k triangles, **one 4096x4096 colour texture** each. One shoe per model, so you mirror it for the pair.
     Every model is branded (Timberland, adidas, Reebok, Sperry, Cole Haan...).
  2. **Objaverse 1.0** on Hugging Face (Allen Institute for AI). This is a mirror of Sketchfab models under Creative Commons licences.
     The dataset is not gated, so **no login** is needed. It includes good **photogrammetry scans of worn leather boots** (CC-BY, plus one CC0).
     **Caveat:** in the Objaverse GLBs I probed, **every texture had been downscaled to 1024x1024**. The full-resolution textures are only on
     Sketchfab, and Sketchfab downloads need an account.
* **Smithsonian Open Access** offers direct CC0 3D downloads without login, but the only footwear I found as CC0 3D was **ski boots** and **stage boots**.
  Neither is usable as a mountaineering boot. My search was partial (see section 3.6).
* **(1) Leather trekking boots:** realistic and usable. The best no-login options are listed in section 4.1.
  The overall best match, the **"Old Lowa Gore-Tex Boot" (a LOWA Tibet scan, 19.9k triangles, PBR textures)**, is **Sketchfab-only, so it needs a free account**.
* **(2) Mountaineering boots:** **no clean, brand-neutral, wearable no-login model exists** as far as I could find.
  The only real one is a CC-BY scan of boots with crampons fused onto a ground patch, which is usable only as a prop.
  **Model these procedurally.** Dimensions are in section 7.
  **Realism check:** on the trek through Dingboche, clients would normally wear **hiking boots**, with **insulated camp boots in camp**.
  The 8000 m boots travel in duffels (section 2). Chapter 1 may need them only as luggage or props.
* **(3) Porter trainers:** realistic and usable from GSO, which has canvas sneakers and cheap-looking synthetic trainers (section 4.3).
  Logos must be painted out.
* **Legal note on all branded scans:** CC licences do **not** license trademarks. CC BY 4.0 s.2(b)(2) says
  "Patent and trademark rights are not licensed under this Public License." Remove or alter logos (Timberland tree, adidas stripes,
  Reebok vector, Vibram octagon, Salomon/Garmont/Caterpillar wordmarks) before shipping.

---

## 2. Realism context: who wears what in Chapter 1 (checked against sources)

| Who | What the sources say | Source |
|---|---|---|
| Expedition clients on the approach trek | Madison Mountaineering's Everest gear list separates **"Hiking Boots/Shoes, comfortable boots or shoes for the trek to base camp"**, **"Camp Boots, comfortable boots for wearing in camp, recommended: Sorel Caribou Boot"**, lodge slippers, optional down booties, and the summit **"Mountaineering Boots, recommended: La Sportiva Olympus Mons Cube"** | Madison Mountaineering EVR gear list PDF (doc code EVR.2022.04): https://madisonmountaineering.com/wp-content/uploads/2019/08/EVR-2023-Mount-Everest-Expedition-gear-list.pdf |
| Summit boots in general | Operators' lists name the Scarpa Phantom 8000, La Sportiva Olympus Mons and Millet Everest (search summary of several operator lists; only Madison's was opened) | e.g. https://climbingthesevensummits.com/wp-content/uploads/2021/01/Everest-Gear-List.pdf |
| Porters | A Republica report describes porters "wearing flip-flops, a t-shirt and holding a plastic bag to keep off the rain". The Kathmandu Post (2025) says many porters work "in thin clothing and poor shoes" | https://myrepublica.nagariknetwork.com/news/porter-protection-takes-priority ; https://kathmandupost.com/art-culture/2025/12/12/porters-keep-tourists-moving-but-who-looks-after-them |
| Nepali-made cheap shoes | **Goldstar** (Nepal, founded 1970) makes cheap, durable PU/PVC trainers and is culturally iconic. **I found no source that says Khumbu porters wear Goldstar specifically**, so treat that as unverified | https://www.arabnews.pk/node/1247476 ; https://www.b360nepal.com/going-for-gold |

Implication for the model list: clients and guides in Chapter 1 most plausibly wear **mid-height trekking boots** on the trail and **pac-style
camp boots** (rubber shell plus leather upper, like the Sorel Caribou) in camp. 8000 m boots appear only as luggage or props.
The other topic notes cover Sherpa and villager footwear in depth; this file covers the 3D assets.

---

## 3. Source-by-source results

### 3.1 Poly Haven (CC0, no login)
* The public API https://api.polyhaven.com/assets?t=models lists 522 models. Filtering ids, names, tags and categories for
  shoe/boot/sneaker/trainer/footwear returns **one model: `rubber_boots`**. Its category from `/info` is
  "Apparel & Personal Items/Clothing & Footwear/Footwear", the only model in that category.
  * Page: https://polyhaven.com/a/rubber_boots
  * Direct download: https://dl.polyhaven.org/file/ph-assets/Models/gltf/2k/rubber_boots/rubber_boots_2k.gltf (also 1k/4k/8k; FBX at
    https://dl.polyhaven.org/file/ph-assets/Models/fbx/2k/rubber_boots/rubber_boots_2k.fbx; also .blend and USD)
  * The API reports polycount 35,004, max textures 8192x8192, and these extra maps: dirty_diff, dirty_rough, dirty_nor, dirty_mask
  * Description from the API: "deep green rubber wellies, glossy shafts with black rims and toe guards, chunky tread soles... mud splashes, embossed logo".
  * Licence: CC0 (Poly Haven). **Relevance:** villagers do wear rubber boots, but these are western garden wellies. At most a background prop.
* **No leather, mountaineering or trainer models on Poly Haven.**
* Useful **CC0 textures** for procedural boots: `brown_leather`, `fabric_leather_01`, `fabric_leather_02`, `leather_white`,
  `leather_red_02/03`, `scuba_suede` (all 8K) from https://api.polyhaven.com/assets?t=textures

### 3.2 ambientCG (CC0, no login)
* All 34 ambientCG 3D models (API `https://ambientcg.com/api/v2/full_json?type=3d-model`) are food, fruit, a stick and a stump. **No footwear.**
  Note: the API's `type=3DModel` filter is silently ignored; `type=3d-model` is the value that works.
* Useful CC0 **materials**: Leather001-Leather039, Rubber001-Rubber004. Example page: https://ambientcg.com/view?id=Leather037

### 3.3 Kenney (CC0)
* Only stylised low-poly packs (Mini Characters etc.). **No realistic footwear.** https://kenney.nl/assets

### 3.4 OpenGameArt (CC0 filter, no login)
* The advanced search with the "3D Art" and "CC0" filters for boot/boots/shoe/shoes/sneaker/footwear returns only low-poly game props:
  * "Sneakers" by Teh_Bucket: "1.5k triangles mesh + texture", CC0. https://opengameart.org/content/sneakers ; download https://opengameart.org/sites/default/files/sneakers.zip
  * "Lowpoly Shoe" by Lyricsz, CC0. https://opengameart.org/content/lowpoly-shoe ; download https://opengameart.org/sites/default/files/sneakers_0.zip
  * "Hats / Clothing / Props" by Teh_Bucket, CC0, includes `work_boots.zip` (FBX plus texture). https://opengameart.org/content/hats-clothing-props ; https://opengameart.org/sites/default/files/work_boots.zip
* Realism: low. Not suitable for close NPC shots.

### 3.5 Poly Pizza (no login stated)
* "Boots" by Isa Lousberg, "Public Domain (CC0)", FBX/GLTF: https://poly.pizza/m/7XCvej7wZU
* "Boots" by Poly by Google, "Creative Commons Attribution", OBJ/GLTF: https://poly.pizza/m/7HbqG8RwRcA
* Also Trainer, Sneakers, Cowboy boots, Rubber boot. **All are stylised low-poly**, so not usable for a realistic look.

### 3.6 Smithsonian Open Access 3D (CC0 where marked, no login for the files)
* 3d.si.edu pages serve a bot-verification page to scripts, so I could not read them. I used the Open Access API (`api.si.edu`, public
  `DEMO_KEY`) instead. Records with a `3d_voyager` media item expose direct file URLs.
* **CC0 footwear with 3D found (partial search: the DEMO_KEY hit its rate limit after about 2,000 of the 3,865 "boots" hits;
  queries for "shoes" and "sneakers" were not completed):**
  * **Ski boots worn by Seba Johnson** (NMAAHC), `access: CC0`. Files:
    `https://3d-api.si.edu/content/document/3d_package:d8c630ca-4ebc-11ea-b77f-2e728ce88125/resources/ski-boots-worn-by-seba-johnson-150k-4096.glb`
    (13.6 MB, 150k faces, 4096 textures; full-res OBJ, 150k OBJ and glTF zips are also listed). These are 1980s plastic alpine **ski** boots
    (black/yellow, buckles). **Wrong silhouette for mountaineering boots.** Useful only as a reference for plastic shell, buckle and liner materials.
  * **Costume boots for the Wizard in The Wiz** (NMAAHC), CC0. Stage costume, not relevant.
* Search engines also list 3d.si.edu scans of "athletic shoes found in the Sonoran Desert", "boots worn by muralist Judy Baca" and
  "blue sneakers worn by Wellington Webb". **I could not verify their licences.** They did not appear as CC0 3D in the part of the API I searched.
  Treat them as "usage conditions may apply" until checked on the object page in a normal browser.

### 3.7 Sketchfab (**download requires a free account**)
* Sketchfab's Download API guidelines say downloading requires an authenticated Sketchfab account: https://api.sketchfab.com/developers/download-api/guidelines
* Its public search API (no login, used for discovery) finds many CC-BY and a few CC0 footwear scans. The best candidates are listed in section 4.
  Several of them are **also in Objaverse**, which gives a no-login route at 1K textures.

### 3.8 Google Scanned Objects (Gazebo Fuel), the best clean no-login source
* Owner `GoogleResearch` on Gazebo Fuel: 1,033 models. Each `metadata.pbtxt` has a `categories { first: "Shoe" }` field. **254 are "Shoe".**
* Licence, from each model's files (`model.config` / `model.mtl` / OBJ header): "This work is licensed under the Creative Commons Attribution 4.0
  International License", with "Copyright 2020 Google LLC." The API field `license_name` is "Creative Commons Attribution 4.0 International".
* Format: zip containing `meshes/model.obj` + `model.mtl`, `materials/textures/texture.png` (**4096x4096 colour only**, no normal/roughness maps),
  `model.sdf`, `model.config`, `metadata.pbtxt` and 5 thumbnails. Units are **metres**. **Single shoe** per model.
* Download (verified without login, returns a zip): `https://fuel.gazebosim.org/1.0/GoogleResearch/models/<NAME>/1/<NAME>.zip`
* Page (renders in a browser): `https://app.gazebosim.org/GoogleResearch/fuel/models/<NAME>`. JSON: `https://fuel.gazebosim.org/1.0/GoogleResearch/models/<NAME>`
* Collection: https://app.gazebosim.org/GoogleResearch/fuel/collections/Scanned%20Objects%20by%20Google%20Research . Paper: https://arxiv.org/abs/2204.11918
* Look: clean, new retail products under even studio light. Accurate shape and colour, **no wear or dirt** (you would add that in Substance or Blender),
  laces fused to the mesh, flat albedo with no fine normal detail.

### 3.9 Objaverse 1.0 (Hugging Face), a no-login mirror of Sketchfab CC models
* https://huggingface.co/datasets/allenai/objaverse . The Hugging Face API reports `gated: false`.
* Licence statement from the README: "The use of the dataset as a whole is licensed under the ODC-By v1.0 license. Individual objects in
  Objaverse are all licensed as creative commons distributable objects, and may be under the following licenses: CC-BY 4.0... CC-BY-NC 4.0...
  CC-BY-NC-SA 4.0... CC-BY-SA 4.0... CC0 1.0... The metadata will provide the license for each object."
  For each object below I checked the **current Sketchfab licence via the Sketchfab API**. **Use only CC0 or CC-BY objects**
  (NC is not allowed commercially; SA would force the same licence on your derivative).
* Download pattern: `https://huggingface.co/datasets/allenai/objaverse/resolve/main/glbs/<folder>/<uid>.glb`. Folder and uid come from
  `object-paths.json.gz` in the same repo.
* **Texture caveat (measured):** every Objaverse GLB I probed (24 footwear files) has **1024x1024** textures, even where the Sketchfab original
  is 4K or 8K. Geometry is the full Sketchfab mesh. Attribution goes to the **original Sketchfab author**.
* Not every Sketchfab model is in Objaverse. The snapshot is from about 2022 and some uploads are missing:
  "Old Lowa Gore-Tex Boot" and "LOWA Zephyr" are **absent**.

---

## 4. Candidate list (every link checked; triangle counts and textures read from the files)

Legend: **NL** = no login needed. **SF** = Sketchfab, account needed. Realism scores are my judgement from 1920 px thumbnails: 5 = indistinguishable from a photo.

### 4.1 Need (1): leather trekking or hiking boots (Sherpa guides, Nima, Aunt Pema)

| # | Model | Source / licence (quoted) | Download | Format, tris, textures | Realism and fit for role |
|---|---|---|---|---|---|
| A | **Priest Karol Wojtyla's sports shoes** (a pair; Cardinal Karol Wojtyla Archdiocesan Museum, Krakow; Malopolska Virtual Museums) | Sketchfab licence **"CC0 Public Domain"**. Object page: https://sketchfab.com/3d-models/priest-karol-wojtyas-sports-shoes-678f96cfadc741a3b260548eeb75954a | **NL** via Objaverse: https://huggingface.co/datasets/allenai/objaverse/resolve/main/glbs/000-107/678f96cfadc741a3b260548eeb75954a.glb (5.2 MB) | GLB, **118,620 tris**, 1024 colour + 1024 PNG map (higher on SF) | **5/5.** Old brown leather, ankle-height lace-up mountain shoes. Stitched welt, cleated rubber sole, scuffed toes, red laces. **Best no-login CC0 match for an older person (Aunt Pema, Nima).** Mid-century style. Needs decimation to about 20-30k. No brand logos visible. |
| B | **Garmont Hiking Boot** by tyjajack | "CC Attribution". https://sketchfab.com/3d-models/garmont-hiking-boot-c46df86e358f489cacd09020cfdf7515 . Description: "8,192 x 8,192... Delighting: Yes", real size "12.25"x4.5"x7"" | **NL** Objaverse: https://huggingface.co/datasets/allenai/objaverse/resolve/main/glbs/000-132/c46df86e358f489cacd09020cfdf7515.glb (7.3 MB) | GLB, **200,000 tris**, one 1024 colour texture (8K on SF) | **5/5.** Leather and fabric mid hiking boot, dirty and well worn, rubber rand, lugged sole. **GARMONT wordmark on the side must be removed.** Good for a Sherpa guide. |
| C | **Left Leather Boot** (Danner, worn) by jorgen_nelson | "CC Attribution". https://sketchfab.com/3d-models/left-leather-boot-783e6e237f3e4d6faa8538ce5001a143 | **NL** Objaverse: .../glbs/000-120/783e6e237f3e4d6faa8538ce5001a143.glb (4.1 MB) | GLB, **99,999 tris**, one 1024 colour texture | **5/5.** Heavily worn brown leather work/hunting boot with deep Vibram lug sole. **Yellow Vibram octagon on the heel must be removed.** Left boot only, so mirror it. A good Sherpa-guide or porter "old boots" look. |
| D | **Bota de montana MUY USADA** ("very used mountain boot") by antonio.tao | "CC Attribution". https://sketchfab.com/3d-models/bota-de-montana-muy-usada-445f4524a69242fca6fa99805e95c3a0 | **NL** Objaverse: .../glbs/000-083/445f4524a69242fca6fa99805e95c3a0.glb (0.9 MB) | GLB, **22,355 tris**, 1024 colour texture | **3/5.** iPhone 12 Pro LiDAR scan. Convincingly filthy and worn, but soft geometry and a scan base under the sole needs cutting. Background use. |
| E | **Utlaningspjaxa 1935-1955** (Stockholm City Museum; scan Christer Lindqvist, retopology Niclas Ekholm) | "CC Attribution". https://sketchfab.com/3d-models/utlaningspjaxa-1935-1955-42b619c45e054c88a3b3940f0b6c5665 | **NL** Objaverse: .../glbs/000-010/42b619c45e054c88a3b3940f0b6c5665.glb (10.3 MB) | GLB, 209,927 tris, 1024 colour texture | **4/5.** A 1935-55 Swedish leather ski/winter boot (pjaxa). Plain dark-brown leather lace-up. A good **period** reference, e.g. for a flashback to the hero's father's era. |
| F | **Old Lowa Gore-Tex Boot (2021 / FREE)** by inciprocal.com. Description: "Lowa Mens Tibet Gore-Tex Leather Boot... Vibram Masai outsole... 29.9 x 11.1 x 21.9 cm (245 micrometers per texel @ 2k)" | "CC Attribution". https://sketchfab.com/3d-models/old-lowa-gore-tex-boot-2021-free-417531d1bfcd4fc39f9cedbc1c85963e | **SF only (account)**. Not in Objaverse | Sketchfab: 19,900 faces, PBR at 2k | **5/5. Best single match for a Sherpa guide's leather trekking boot:** a classic brown full-leather trekking boot, used, game-ready topology. Needs a free Sketchfab account. LOWA logo on the side. |
| G | **Timberland Men's Earthkeepers Newmarket 6-Inch Cupsole Boot** (GSO) | CC BY 4.0 (Google LLC 2020) | **NL**: https://fuel.gazebosim.org/1.0/GoogleResearch/models/Timberland_Mens_Earthkeepers_Newmarket_6Inch_Cupsole_Boot/1/Timberland_Mens_Earthkeepers_Newmarket_6Inch_Cupsole_Boot.zip | OBJ, **16,976 tris**, 4096 colour | **4/5 as a scan** but **wrong style**: a red nubuck lifestyle boot with a white cupsole. You would recolour and age it. Timberland marks. |
| H | **MARTIN WEDGE LACE BOOT** (Cole Haan, GSO). Description: "Lace boot in waterproof leather upper... Leather welt. Vibram bottom" | CC BY 4.0 | **NL**: .../models/MARTIN_WEDGE_LACE_BOOT/1/MARTIN_WEDGE_LACE_BOOT.zip | OBJ, **27,128 tris**, 4096 colour | **4/5.** A black leather lace-up boot on a wedge sole. Plausible for **Aunt Pema** if the wedge is flattened. Very clean and new, so it needs wear. |
| I | **Colton Wntr Chukka** (Cole Haan, GSO) | CC BY 4.0 | **NL**: .../models/Colton_Wntr_Chukka_y4jO0I8JQFW/1/Colton_Wntr_Chukka_y4jO0I8JQFW.zip | OBJ, 26,766 tris, 4096 colour | 4/5. A low brown winter chukka. Village or lodge footwear at most. |
| J | **Scout Boots - 3d Scan** by yaskebasi. Description: "my old trekking boots after an hike" | "CC Attribution". https://sketchfab.com/3d-models/scout-boots-3d-scan-7676464926a3465aba75196cafd38ad6 | **NL** Objaverse: .../glbs/000-003/7676464926a3465aba75196cafd38ad6.glb | GLB, 266,951 tris, one 1024 texture | 2/5. A pair of grey trekking boots fused to a scan base. Rough. Reference only. |

Other CC-BY leather boots on Sketchfab (account needed; not checked in Objaverse): "Brown Hiking Boot" (mrfunnysheep, 211,616 faces),
"Hiking boot" (mrfunnysheep, 25,000 faces, normal and AO maps), "Rocky Hiking Boot" (scbenoit, 49,999), "WANDERSTIEFEL - HIKINGBOOT" (majme, 201,226),
"Merrel MOAB 3 Boot" (weedlog, 457,600), "LOWA Zephyr Gore-Tex Hiking Boots" (oldMediaArt, 500,000), "Hiking Boots LOWPOLY" (evan4129, 6,280,
"Quad topology and a 4k color map"), "Winter Hiking Boots" (antoncla, 18,956, diffuse plus normal, "Diffuse has not yet been delighted").
Discovery query: `https://api.sketchfab.com/v3/search?type=models&q=hiking%20boot&downloadable=true&license=by`

### 4.2 Need (2): mountaineering boots (stiff, high, insulated, brand-neutral)

**Result: no usable no-login model.** What exists:

| Model | Source / licence | Download | Format, tris, textures | Verdict |
|---|---|---|---|---|
| **Boots and crampons** by robinschwartz | "CC Attribution", no description. https://sketchfab.com/3d-models/boots-and-crampons-a8fda8369fc94d7da5984ca35dfeb7e8 | **NL** Objaverse: .../glbs/000-015/a8fda8369fc94d7da5984ca35dfeb7e8.glb (11.7 MB) | GLB, 358,846 tris, one 1024 texture | A pair of grey leather/synthetic **single-layer mountaineering boots with strap-on crampons**, fused together on a scanned ground patch. **Prop only** (gear pile, drying rack). Not wearable without a full rebuild. |
| Ski boots worn by Seba Johnson | Smithsonian **CC0** | **NL** (section 3.6) | GLB 150k/4K | Alpine ski boots, wrong object. Material reference for plastic shells and liners only. |
| Salomon OUTline Mid GTX (women's) by inciprocal.com | "CC Attribution". https://sketchfab.com/3d-models/salomon-outline-gtx-boot-5e24610ab910434b8e40b2a83199e98d . Description: "29.7 x 11.1 x 17.0 cm" | **NL** Objaverse: .../glbs/000-073/5e24610ab910434b8e40b2a83199e98d.glb (4.5 MB) | GLB, **24,852 tris**, 4 maps at 1024 (4K on SF) | **Not a mountaineering boot**, but a realistic **modern women's mid hiking boot**: **good for Elena on the trek**. Salomon logos to remove. |
| Salomon X RAISE (men's low hiking shoe) by inciprocal.com | "CC Attribution". https://sketchfab.com/3d-models/salomon-x-raise-hiking-e876d6429ed847c884f45d66e9be2d3c | **NL** Objaverse: .../glbs/000-000/e876d6429ed847c884f45d66e9be2d3c.glb | GLB, 24,708 tris, 4 maps at 1024 | A modern low hiking shoe for Marek or Ji-woo around the lodge. |
| adidas TERREX FAST X GTX / TERREX FAST R (GSO) | CC BY 4.0 | **NL**: .../models/TERREX_FAST_X_GTX/1/TERREX_FAST_X_GTX.zip ; .../TERREX_FAST_R/1/TERREX_FAST_R.zip | OBJ 36,824 / 39,346 tris, 4096 colour | Low hiking shoes, clean and new. adidas stripes to remove. Client trek shoes. |

**Camp boots for clients** (realistic for Dingboche evenings, per Madison's "Camp Boots... Sorel Caribou"):

| Model | Licence | Download | Format, tris | Notes |
|---|---|---|---|---|
| **Sorel Boot** by mikkoh ("Boot shot with Trnio") | "CC Attribution". https://sketchfab.com/3d-models/sorel-boot-9db6ee3d0f0b4dc7afb2b4791c153059 | **NL** Objaverse: .../glbs/000-020/9db6ee3d0f0b4dc7afb2b4791c153059.glb (5.0 MB) | GLB, 98,120 tris, 4 materials, 1024 textures | **4/5.** A classic pac boot: black rubber shell, brown leather upper, laces, felt-liner cuff. Exactly the camp-boot type the gear lists name. |
| **Fairbanks Snow Boot** (Columbia) by inciprocal.com. Description: "29.4 x 11.3 x 16.1 cm" | "CC Attribution". https://sketchfab.com/3d-models/fairbanks-snow-boot-b39f6aaeb97549afb76a44375e93245b | **NL** Objaverse: .../glbs/000-082/b39f6aaeb97549afb76a44375e93245b.glb | GLB, 19,836 tris, 4 maps at 1024 | 5/5 scan. A tan/orange winter hiker. Columbia marks to remove. |

### 4.3 Need (3): cheap canvas or synthetic trainers (porter); also the child (Dolma)

| Model | Licence | Download (NL unless noted) | Format, tris, textures | Notes |
|---|---|---|---|---|
| **Men's Striper Sneaker in White** (Sperry, GSO) | CC BY 4.0 | .../models/Mens_Striper_Sneaker_in_White_rnp8HUli59Y/1/Mens_Striper_Sneaker_in_White_rnp8HUli59Y.zip | OBJ 27,882 tris, 4096 | A **white canvas low-top**, vulcanised sole with a navy stripe. With grime and a recolour, this is a convincing cheap canvas porter shoe. Small logo. |
| **Women's Canvas Bahama in Black** (Sperry, GSO) | CC BY 4.0 | .../models/Womens_Canvas_Bahama_in_Black/1/Womens_Canvas_Bahama_in_Black.zip | OBJ 15,574 tris, 4096 | A black canvas lace-up with white sole. Boat-shoe lacing detail. Small. |
| **Men's Bahama in Khaki Oyster** (Sperry, GSO) | CC BY 4.0 | .../models/Mens_Bahama_in_Khaki_Oyster_xU2jeqYwhQJ/1/Mens_Bahama_in_Khaki_Oyster_xU2jeqYwhQJ.zip | OBJ 20,600 tris, 4096 | A khaki canvas shoe. A good porter colourway once dirtied. |
| **Reebok HIMARA LTR** (GSO) | CC BY 4.0 | .../models/Reebok_HIMARA_LTR/1/Reebok_HIMARA_LTR.zip | OBJ 28,674 tris, 4096 | Despite the name, a **grey/white synthetic running trainer with orange accents**: the "cheap synthetic trainer" look. Large Reebok vector logo to remove. |
| Reebok CLASSIC JOGGER / Reebok VERSA TRAIN (GSO) | CC BY 4.0 | .../models/Reebok_CLASSIC_JOGGER/1/Reebok_CLASSIC_JOGGER.zip (20,090 tris) ; .../Reebok_VERSA_TRAIN/1/Reebok_VERSA_TRAIN.zip | OBJ, 4096 | Retro or synthetic trainers. Logos to remove. |
| Reebok DMX MAX PLUS RAINWALKER (GSO) | CC BY 4.0 | .../models/Reebok_DMX_MAX_PLUS_RAINWALKER/1/Reebok_DMX_MAX_PLUS_RAINWALKER.zip | OBJ 27,446 tris, 4096 | An all-black walking shoe. Plausible for an older villager (Aunt Pema) or a lodge worker. |
| Unused Blue Vans Shoe by thesidekick | "CC Attribution". https://sketchfab.com/3d-models/unused-blue-vans-shoe-96baa4684df7415ba8ba87d39bd1c2ee | Objaverse .../glbs/000-093/96baa4684df7415ba8ba87d39bd1c2ee.glb | GLB 77,987 tris, 3 maps at 1024 | A canvas skate shoe. Vans side stripe is a trademark, so remove it. |
| Dirty Nike Moonglide - Photogrammetry by monokultur | "CC Attribution". https://sketchfab.com/3d-models/dirty-nike-moonglide-photogrammetry-66f0f196877a4c6ca390e5b5f275e43f | Objaverse .../glbs/000-144/66f0f196877a4c6ca390e5b5f275e43f.glb (12.9 MB) | GLB 113,444 tris, 6 maps at 1024 | A **genuinely worn and dirty** pair of running trainers. Nike swoosh to remove. |
| Converse Chuck Taylor 2 by mario.krajcir | "CC Attribution". https://sketchfab.com/3d-models/converse-chuck-taylor-2-65e43a34c69941acb3ad18297346f3d5 | Objaverse .../glbs/000-136/65e43a34c69941acb3ad18297346f3d5.glb (43.5 MB) | GLB 388,485 tris, 1024 | A dark canvas low-top. Heavy mesh. Converse marks. |
| old shoe photogrammetry (low poly) by deathcow ("found in a field") | "CC Attribution". https://sketchfab.com/3d-models/old-shoe-photogrammetry-low-poly-834b6b22df6e4925a94b2517201fe702 | Objaverse .../glbs/000-054/834b6b22df6e4925a94b2517201fe702.glb | GLB **5,539 tris**, 2 maps at 1024 | A destroyed white trainer on a ground patch. A discarded-shoe prop on the trail. |
| **Merrell Kid's Boot** by inciprocal.com. Description: "Merrell Kid's Chameleon 7 Waterproof Hiking Berry/Coral Boot... 22.0 x 8.8 x 12.8 cm" | "CC Attribution". https://sketchfab.com/3d-models/merrell-kids-boot-bbb0764dd0fb4f88b7e6acd6f62d6947 | Objaverse .../glbs/000-031/bbb0764dd0fb4f88b7e6acd6f62d6947.glb | GLB 22,110 tris, 4 maps at 1024 | A purple/pink child's hiker with velcro. **Option for Dolma** (a hand-me-down from trekkers is plausible, but unverified). |
| GIRLS DECKHAND (Reef, GSO). Description: "Hemp and canvas upper... Vulcanized construction" | CC BY 4.0 | .../models/GIRLS_DECKHAND/1/GIRLS_DECKHAND.zip | OBJ 18,796 tris, 4096 | A girl's canvas shoe. Another option for Dolma. |

(GSO download prefix: `https://fuel.gazebosim.org/1.0/GoogleResearch/models/`. Objaverse prefix: `https://huggingface.co/datasets/allenai/objaverse/resolve/main/glbs/`.)

---

## 5. Licence and attribution checklist

* **CC0** (Poly Haven, the Wojtyla shoes, Smithsonian CC0, OpenGameArt CC0): no attribution required. Crediting is still good practice.
* **CC BY 4.0** (GSO, most Sketchfab/Objaverse picks): credit the creator, link the licence, link the source, and **say if you modified it**
  (CC BY 4.0 s.3(a)(1)). A template for the game credits:
  `"<Title>" by <Author> (<source URL>), licensed CC BY 4.0 (https://creativecommons.org/licenses/by/4.0/). Modified: retopologised, logos removed, retextured.`
  For GSO the author line is "Google LLC (Google Scanned Objects)". For Objaverse files, credit the **Sketchfab author**, not Objaverse.
  Objaverse itself is ODC-By, so also cite Objaverse (arXiv 2212.08051) if you redistribute files from it.
  Note: the "Kid Boots" Sketchfab page text says CC BY **3.0** while the Sketchfab licence label says CC Attribution. That model is not recommended here.
* **Trademarks:** CC BY 4.0 legal code s.2(b)(2): "Patent and trademark rights are not licensed under this Public License."
  https://creativecommons.org/licenses/by/4.0/legalcode.en . Remove logos and distinctive brand marks (stripes, tree, swoosh, Vibram octagon) in the texture,
  and alter distinctive trade-dress shapes where they are recognisable.
* **Never use** CC-BY-NC or CC-BY-NC-SA objects. Sketchfab's "Standard"/"Editorial" licences are not CC either.

---

## 6. Practical notes on turning scans into MetaHuman boots (not verified in this project yet)

* Scans are static, closed meshes: 100k-360k triangles for most Sketchfab scans, 16-42k for GSO. Plan to **retopologise** to about 6-12k triangles per boot,
  with edge loops at the **ball-of-foot flex line** (toe bone) and the ankle. Skin to `foot_l/r` and `ball_l/r`, then bake the scan to normal and AO maps.
* GSO models are **single shoes**: mirror for the other foot and break the symmetry in the texture.
* Objaverse files carry only **1024 textures**. Fine for NPC boots at normal camera distance, soft in close-ups. For 4K, get the original from Sketchfab with an account.
* GSO textures are clean and new: add dust, mud, scuffs, salt lines and creasing at the flex line. The CC0 leather and rubber textures listed in 3.1 and 3.2 help here.
* Remove the scanned ground patches (Boots and crampons, Bota de montana, old shoe, Scout Boots).

---

## 7. Modelling references for procedural boots in Blender (EU 42 men's, EU 38 women's)

Needed for the mountaineering boot (no model exists), and useful for any boot you want brand-neutral.
**Measured** = from a lab or manufacturer source. **Estimate** = my derivation, flagged with the reasoning.

### 7.1 Size to length basics

| Quantity | EU 42 (men) | EU 38 (women) | Source / note |
|---|---|---|---|
| Foot length a size is made for | **265 mm** | **240 mm** | Wikipedia "Shoe size" Euro to Mondopoint table: EUR 42 = Mondopoint 265, EUR 38 = 240 (https://en.wikipedia.org/wiki/Shoe_size). A LOWA chart via Bergfreunde also gives EU 42 = 265 mm (https://www.bergfreunde.eu/lowa-size-chart/) |
| Last length (Paris point = 2/3 cm) | **280 mm** (42 x 6.667) | **253 mm** (38 x 6.667) | Wikipedia: "the shoe size is the length of the last, expressed in Paris points or 2/3 cm"; the last is longer than the foot "by 4/3 cm (13 mm) to 5/3 cm (17 mm)". https://en.wikipedia.org/wiki/Paris_point |
| One EU size step | 6.67 mm in length | | Paris point; Scarpa also states 1 EU size = 6.6 mm |
| Internal length measured in real boots | 269.7 mm (LOWA Renegade GTX Mid); lab average 270.8 mm; range 255-281 mm across 1,000+ shoes in one size | (no lab data; scale x 0.906) | RunRepeat tests **men's US 9 (about EU 42-42.5)**: https://www.runrepeat.com/lowa-renegade-gtx-mid ; https://runrepeat.com/testing-methodology |

### 7.2 Leather mid trekking boot (Sherpa guide, client on trek)

| Part | EU 42 men | EU 38 women | Basis |
|---|---|---|---|
| Outsole length (toe to heel, outside) | **295-305 mm** | **268-277 mm** | Estimate: last 280 mm plus toe bumper and heel overhang. Cross-check with scan sizes given by inciprocal and tyjajack (sizes not stated): LOWA Tibet **299** x 111 x 219 mm; Garmont 311 x 114 x 178 mm; Wolverine Harrison 307 x 110 x 202 mm; Salomon women's OUTline Mid **297** x 111 x 170 mm. Women's = men's x 0.905 |
| Outsole width, forefoot (ball) | **~110 mm** (Renegade 110.6; lab average 112.3) | ~98-101 mm (estimate x 0.89-0.91) | RunRepeat midsole width, forefoot |
| Outsole width, heel | **~85-88 mm** (Renegade 84.8; average 88.0) | ~76-79 mm (estimate) | RunRepeat midsole width, heel |
| Waist width (under the arch) | ~75-80 mm | ~68-72 mm | **Estimate** from scan proportions. Verify on photos |
| Heel stack (ground to footbed at 12% of length) | **36-38 mm** (Renegade 37.8; average of 44 boots 36.0) | ~34-36 mm (estimate) | RunRepeat |
| Forefoot stack (at 75% of length) | **~21-24 mm** (Renegade 20.6; average 23.5) | ~20-22 mm | RunRepeat |
| Heel-to-toe drop | 12-17 mm (average 12.4; Renegade 17.2) | same | RunRepeat |
| Lug depth | **~5 mm** (Renegade 5.1; Moab 3 Mid 5.0; average 4.2) | ~4.5-5 mm | RunRepeat |
| Outsole rubber under the lugs | ~2-3 mm (Renegade 2.2; average 2.7) | same | RunRepeat |
| Toe spring (toe tip lifted off the ground) | **~15 mm** | ~13-15 mm | Shoemaker's Academy glossary (via search snippet; page blocked automated reads): stiff hiking boot about 15 mm vs a casual slip-on about 5 mm. Stiffer sole means more toe spring. https://shoemakersacademy.com/glossary/toe-spring/ |
| Shaft height measured from the arch (inside) | **~127 mm** (5 in, Renegade GTX Mid, men's US 8) | ~115-140 mm (retailers list 4.75-5.5 in for women's Renegade variants: uncertain) | Zappos/Desertcart listings for the LOWA Renegade GTX Mid (https://zappos.com/p/lowa-renegade-gtx-mid/product/7605665) |
| Overall height, ground to top of tongue | **170-220 mm** | ~155-175 mm | Scan heights above: 178 (Garmont), 219 (LOWA Tibet, a taller trekking boot), 170 (Salomon women's mid) |
| Toe box internal height | 28.4 mm (Renegade; average 27.6) | ~26 mm (estimate) | RunRepeat |
| Toe box internal width (widest) | 95.8 mm (Renegade; average 94.2) | ~86 mm (estimate) | RunRepeat |
| Tongue padding thickness | 13.8 mm (Renegade; average 10.9) | ~12 mm | RunRepeat |
| Insole thickness | 4.5 mm (average 5.9) | same | RunRepeat |
| Weight per boot | 597 g (Renegade, US M9) | ~500 g | RunRepeat |
| Lacing | Typical layout: 4-5 pairs of eyelets or webbing loops on the forefoot and instep, 1 **locking hook** at the ankle flex line, 2-3 pairs of open **speed hooks** up the shaft | same | **Observation** from the scanned boots' thumbnails (LOWA Tibet, Garmont). Verify on product photos |
| Collar | Padded collar about 15-20 mm thick, scalloped lower at the Achilles than at the ankle bones; finger loop on the heel tab (Renegade "Finger loop") | same | Thickness is an **estimate**; heel tab from RunRepeat |
| Rand / toe cap | Rubber toe cap rising about 20-30 mm at the toe and wrapping the forefoot | same | **Estimate / observation** |

Women's lasts are generally narrower (heel and forefoot) and lower volume. Outdoor Life, citing REI experts: "women's feet tend to be narrower
than men's, so women's boots are generally narrower too" (https://www.outdoorlife.com/gear/best-hiking-boots-for-women/). The women's figures above
are **proportional estimates** (length x 0.905, width x 0.89-0.91), not lab measurements.

### 7.3 Stiff single-layer mountaineering boot (B3/C3 class, e.g. La Sportiva Nepal Evo GTX)

| Part | Value | Source |
|---|---|---|
| Heel stack / forefoot stack / drop | **48 / 32 / 16 mm** | La Sportiva official spec, "Stack Height (mm) - Heel: 48... Forefoot: 32 Drop (mm): 16" (seen via search; the site returns 403 to automated reads): https://www.lasportiva.com/en/boots-mountaineering-nepal-evo-gtx-man-zfms060 . Same figures on https://www.outdoorway.gr/product/19948/La-Sporiva-Nepal-Evo-Gore-Tex |
| Lug height | **8.5 mm** | La Sportiva (same page, via search snippet) |
| Weight | 1012 g per boot (La Sportiva figure); about 2080 g per pair at EU 42 (retailer) | La Sportiva USA; Galaxus/retailer |
| Height class | "Mid-calf" (REI, men's) | https://www.rei.com/product/733952 |
| Construction cues | Welted toe and heel for automatic crampons, fully rigid midsole, thick rubber rand all round, leather upper, insulated lining | La Sportiva and REI descriptions |
| Toe spring | **Estimate 18-25 mm** (a rigid sole needs more rocker than the 15 mm of a stiff hiking boot) | Inference from the Shoemaker's Academy principle. **Unverified.** Measure from a side-on product photo |

### 7.4 8000 m "triple" boot with integrated gaiter (what clients carry for the summit)

| Part | Value | Source |
|---|---|---|
| Gaiter/shaft height from the arch | **~419 mm (16.5 in) at EU 46** (Scarpa Phantom 8000). **EU 42 estimate: 380-400 mm** (scaled by length) | Outland USA listing: "Shaft measures approximately 16.5" from arch" (size 46): https://www.outlandusa.com/p/scarpa-phantom-8000-blackorange-46 |
| Ground to top of gaiter, EU 42 | **Estimate 410-440 mm** (shaft plus arch height above ground with a thick sole) | Estimate |
| Sole stack (Olympus Mons Pro) | Heel **35 mm**, forefoot **24 mm**, drop **11 mm** | Bergzeit spec: https://www.bergzeit.us/p/la-sportiva-olympus-mons-pro-shoes/3014382/ |
| Sole materials | Olympus Mons Pro: "Vibram Olympus... EVA foam with rubber insert in the toe area". Olympus Mons Cube S: Vibram LITEBASE plus Vibram MORFLEX expanded EVA. Phantom 8000: "Vibram Zero Gravity" | Bergzeit; Outland USA |
| Closures | Olympus Mons Pro: zip, BOA, Quicklace, Velcro. Olympus Mons Cube S: **dual BOA** with "a corner zip that opens fully". Phantom 8000: waterproof zip on the integrated gaiter, laces and Velcro | Bergzeit; Outland USA (https://www.outlandusa.com/p/la-sportiva-olympus-mons-cube-yellowblack-41) |
| Inner boot | Removable insulated inner boot (PrimaLoft Gold, aerogel, PE/EVA). Cube S: "two removable thermal booties" | Bergzeit; Outland USA |
| Gaiter fabric | Olympus Mons Pro: single-layer heat-sealed fabric with TPU film. Phantom 8000 Thermic HD: Schoeller plus PrimaLoft Gold plus HDry membrane plus "YKK wp zip" | Bergzeit pages |
| Weight | Olympus Mons Pro **1880 g/pair at EU 42**; Phantom 8000 Thermic HD **2900 g/pair at EU 42**; Olympus Mons Cube 1490 g per half pair at size 44 (Mont) vs 1226 g per boot at size 44 (UKClimbing): **conflicting** | Bergzeit; https://mont.com.au/products/la-sportiva-olympus-mons-cube ; UKClimbing forum post |
| Colours (product names) | La Sportiva Olympus Mons: **yellow/black**. Scarpa Phantom 8000: **orange/black** | Product listing names (Outland USA, Trekitt) |
| Crampon fit | Fully automatic crampons (toe and heel welts) | Bergzeit |
| Outsole length, EU 42 | **Estimate 310-325 mm** (bulky shell and insulation add length; no manufacturer figure found) | **Unverified estimate** |
| Forefoot width, EU 42 | **Estimate 115-125 mm** | **Unverified estimate** |
| Sizes | Olympus Mons Cube: Euro 38-50 in half sizes, so an **EU 38 exists** for Elena | Search summary of retailer listings |

**Brand-neutral design guidance:** combine the generic features (tall gaiter with a diagonal waterproof zip, broad rubber rand, thick EVA/rubber sole with
toe and heel welts, BOA or Velcro panel, pull loop) in a non-signature colour, e.g. dark red/black or grey/blue. The yellow/black or orange/black
blocking reads instantly as one specific brand.

### 7.5 Canvas sneaker (porter)

| Part | Value (Converse Chuck 70 High, RunRepeat, US M9) | Note |
|---|---|---|
| Heel / forefoot stack, drop | 26.2 / 17.6 / 8.6 mm | https://runrepeat.com/converse-chuck-70-high |
| Outsole thickness | 7.4 mm (thick vulcanised rubber; average 5.1) | |
| Internal length | 280.1 mm (Converse runs long; average 272.2) | |
| Midsole width, forefoot / heel | 103.0 / 73.6 mm | narrower than a hiking boot |
| Tongue padding | 1.3 mm (essentially unpadded canvas) | |
| Weight | 482 g | |
| Chuck 70 Low | heel 24.3, forefoot 16.7, drop 7.6 mm | RunRepeat via search snippet |
| Construction cues | Canvas upper, rubber foxing tape around the sole (about 30-35 mm tall at the toe, estimate), rubber toe cap, 6-8 metal eyelets | Observation from the GSO Striper/Bahama scans; foxing height is an **estimate** |

### 7.6 Suggested Blender procedural approach (brief)
1. Draw the **last** as a side-profile curve and a plan curve, using the numbers above: last length, toe spring, heel stack and the forefoot/heel widths.
   Loft or skin it to a volume and use that as the inner surface.
2. **Sole:** offset the last's bottom outline by 5-12 mm for the welt/rand overhang. Extrude to the heel and forefoot stacks, bend it with the toe-spring curve,
   and scatter **lugs** with Geometry Nodes (5 mm hiking, 8.5 mm mountaineering; chevron forefoot lugs, a heel brake block).
3. **Upper:** shrinkwrap panels onto the last plus an ease offset of 3-6 mm. Cut panels along seam curves (vamp, quarters, collar, tongue) and solidify them
   (leather 2-3 mm, padded collar 15-20 mm).
4. **Hardware:** instance eyelets and hooks along a lacing curve. Laces are curve objects with a twisted rope profile.
5. Bake the high-detail version (stitch rows, wrinkles at the flex line, leather grain from the CC0 textures) down to a low-poly game mesh of about 8-12k triangles.

---

## 8. Uncertainties and items not verified

* I judged realism from thumbnails only. No model has been imported or inspected in Blender or UE yet.
* Objaverse texture resolution: I measured 1024 on all 24 GLBs probed. I assume the rest of Objaverse is the same, but did not check exhaustively.
* The licence reported for Objaverse picks is the current Sketchfab licence (via the API). The snapshot-time licence is in Objaverse's per-folder metadata,
  which I did not open. CC licences are irrevocable, so a later change would not remove rights already granted.
* The Smithsonian search was incomplete because of API rate limits. A normal-browser check of 3d.si.edu for footwear could still turn up more CC0 shoes.
* The shoe sizes of the scanned boots (LOWA Tibet 29.9 cm etc.) are not stated by the scanners, so they serve only as proportion checks.
* There is no manufacturer sole-length figure for 8000 m boots. The outsole length and width in 7.4 are estimates.
* La Sportiva and Shoemaker's Academy pages block automated reads. Their figures come from search-engine extracts of those pages. Spot-check them in a browser.
* That porters use Goldstar shoes is plausible but **unverified**.

## 9. Source URLs (main)
* Poly Haven API: https://api.polyhaven.com/assets?t=models ; https://api.polyhaven.com/info/rubber_boots ; https://api.polyhaven.com/files/rubber_boots
* ambientCG API: https://ambientcg.com/api/v2/full_json?type=3d-model
* OpenGameArt: https://opengameart.org/content/sneakers ; https://opengameart.org/content/lowpoly-shoe ; https://opengameart.org/content/hats-clothing-props
* Poly Pizza: https://poly.pizza/m/7XCvej7wZU ; https://poly.pizza/m/7HbqG8RwRcA
* Smithsonian Open Access API: https://api.si.edu/openaccess/api/v1.0/search ; Voyager package: https://3d-api.si.edu/voyager/3d_package:d8c630ca-4ebc-11ea-b77f-2e728ce88125
* Google Scanned Objects: https://fuel.gazebosim.org/1.0/GoogleResearch/models ; paper https://arxiv.org/abs/2204.11918
* Objaverse: https://huggingface.co/datasets/allenai/objaverse ; paper https://arxiv.org/abs/2212.08051
* Sketchfab Download API guidelines (login required): https://api.sketchfab.com/developers/download-api/guidelines
* CC BY 4.0 legal code: https://creativecommons.org/licenses/by/4.0/legalcode.en
* RunRepeat lab data: https://www.runrepeat.com/lowa-renegade-gtx-mid ; https://runrepeat.com/merrell-moab-3-mid-gtx ; https://runrepeat.com/converse-chuck-70-high ; methodology https://runrepeat.com/testing-methodology
* Shoe sizing: https://en.wikipedia.org/wiki/Shoe_size ; https://en.wikipedia.org/wiki/Paris_point ; https://www.bergfreunde.eu/lowa-size-chart/
* Mountaineering boot specs: https://www.lasportiva.com/en/boots-mountaineering-nepal-evo-gtx-man-zfms060 ; https://www.bergzeit.us/p/la-sportiva-olympus-mons-pro-shoes/3014382/ ; https://www.bergzeit.us/p/scarpa-phantom-8000-thermic-hd-boots/3012397/ ; https://www.outlandusa.com/p/scarpa-phantom-8000-blackorange-46 ; https://www.outlandusa.com/p/la-sportiva-olympus-mons-cube-yellowblack-41 ; https://www.rei.com/product/733952
* Gear lists and porters: Madison Mountaineering PDF (above); https://myrepublica.nagariknetwork.com/news/porter-protection-takes-priority ; https://kathmandupost.com/art-culture/2025/12/12/porters-keep-tourists-moving-but-who-looks-after-them
