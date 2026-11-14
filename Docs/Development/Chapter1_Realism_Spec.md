# Chapter 1 realism spec: boots, cast, house

Written 2026-11-14 (overnight). An implementation spec for the technical artist. It turns the four research notes into
lines you can build and tick off. Every look choice here is traced to a source or marked as unverified.

**Research notes this spec is built on** (full source lists and quotes are in these files):
- `research_notes/Chapter 1 realism/boots_models.md`: free footwear models, licences, boot dimensions
- `research_notes/Chapter 1 realism/clothing_authentic.md`: what each character really wears
- `research_notes/Chapter 1 realism/faces_realism.md`: skin, hair, eyes and MetaHuman 5.8 controls
- `research_notes/Chapter 1 realism/house_kitchen.md`: the Khumbu Sherpa house and kitchen

**Tags used on every line**
| Tag | Meaning |
|---|---|
| **[S]** | Stated by a cited source (URL given here or in the notes file) |
| **[P]** | Seen in a dated, located photo. One real example, not a rule |
| **[O]** | Plausible, but no source confirms it. Check against photos before relying on it |
| **[E]** | An estimated number. No source measured it |
| **[D]** | A design or style choice. Not a realism claim |

**Priority:** **P1** = wrong today, fix first. **P2** = an authenticity cue that is missing. **P3** = polish.

**Colours** are given as sRGB hex (to judge by eye) plus the **linear** RGB tuple to paste into
`Tools/Blender/garments.py` `OUTFITS` and `Tools/Blender/sherpa_house.py` `slot(...)`. Both scripts' tints end up in
`LinearColor` material parameters, so they are linear values. Keep dark fabrics at linear 0.02 or above
(about sRGB #282828); darker reads as a CG black hole [D].

---

## 0. Decisions to make before building (blocking or near-blocking)

| # | Decision | Why it matters | Default used in this spec |
|---|---|---|---|
| 0.1 | **Season.** The research brief says late autumn (Oct/Nov). `Docs/Development/Chomolungma_Chapter1.md` §2.1 says "Day 1 of 49, **spring**". | Spring changes the fields (above 3,500 m there is one crop cycle, roughly April to October [S, Onlinekhabar via `house_kitchen.md` §5], so in spring fields are being prepared or sown, not harvested), the festivals (Mani Rimdu at Tengboche is mid-Oct to mid-Nov [S] and falls outside a spring window), and what is stockpiled in the yard. Clothing should barely change, since both seasons have frozen dawns and strong midday sun [O: spring temperatures were not researched]. | **Late autumn**, as briefed. Lines that depend on the season are marked **(season)** |
| 0.2 | **Is Pema married?** `Docs/Development/Chapter1_Design.md` names "Uncle Pasang, Pema's husband". `Chomolungma_Chapter1.md` does not mention him. | Only married women wear the front apron (pangden) [S: Sherpa-authored sources]. No source says whether widows keep it. | **Married**: front pangden plus back apron. If the script makes her a widow, ask a Sherpa consultant |
| 0.3 | **How to build Dolma (8).** MetaHuman Creator ships adult bodies only; Epic has no child support [S]. Today she is a tinted placeholder at `scale 0.62` in `Content/Story/Chapter1.ygs`. | A uniformly scaled adult has adult proportions. An 8-year-old's head is about 1/6 of her height, against about 1/7.5 for an adult [E, general anthropometry]. Dolma holds the player's hand, so she is seen close. | Evaluate a Fab child MetaHuman first (check licence and 5.8 compatibility: https://forums.unrealengine.com/t/athian-games-metahuman-child-characters/2701955). Never ship the scaled adult |
| 0.4 | **Free Sketchfab account?** The best Sherpa-guide boot (the "Old Lowa Gore-Tex Boot", CC BY) and the 4K versions of every Objaverse scan need one. | The no-login Objaverse copies have 1024 px textures only [S, measured]. | No account: use the Objaverse copies (§1.2). Creating the account is the user's own action |
| 0.5 | **One storey or two.** The classic house has two storeys, with the byre below and the family upstairs; 73% of 223 Thame-valley buildings surveyed in the 2020s were single-storey [S]. | Exterior silhouette and the door/stair logic. | Keep **one storey** (a defensible kitchen house); a byre storey is optional (§3.1) |
| 0.6 | **Face Texture Index** (wrinkles, stubble) can only change on an **unrigged** character [S, engine source]. All six are auto-rigged. | Un-rig, change, re-request the rig: Epic cloud, one job at a time (161-590 s), needs the user's Epic sign-in. | **Phase 2.** Everything else in §2 works on the rigged characters |
| 0.7 | **Rotate the house.** Door and windows belong on the same long, sunny, downhill façade (§3). The builder now turns the house so the end-wall door "looks at the camp" (`build_chapter1_world.py` line 221). | Moving the door changes `RY`, the collision shell, the story places and the shot list. | Do it (P1), then regenerate places (§3.4) |

---

## 1. Boots

### 1.1 Who wears what in Chapter 1 (realism rule first)

- **Clients and the leader wear medium-weight hiking boots on the trek.** Double or 8,000 m boots stay in the duffels.
  IMG's autumn EBC list asks for "medium-weight waterproof hiking boots" and lists plastic or double boots only under
  the Lobuche climb add-on [S: https://www.mountainguides.com/pdf/Everest-Trek-Autumn-Gear.pdf]. Madison Mountaineering
  separates trek boots, camp boots ("Sorel Caribou") and summit boots ("La Sportiva Olympus Mons Cube")
  [S: https://madisonmountaineering.com/wp-content/uploads/2019/08/EVR-2023-Mount-Everest-Expedition-gear-list.pdf].
- **Today Marek and Ji-woo wear yellow `mountain` boots** (`garments.py`, tint `(0.85, 0.55, 0.04)` = sRGB #EDC438). That
  is wrong twice: summit boots on the trail, and La Sportiva's signature yellow. **P1.**
- **Sherpa men wear trekking shoes or boots; older village women wear sneakers or walking shoes** [S/P]. Traditional
  wool boots with hide soles are festival or heritage items only [S].
- **Porters wear cheap trainers or canvas shoes**: "thin clothing and poor shoes, including canvas shoes"
  [S: https://kathmandupost.com/art-culture/2025/12/12/porters-keep-tourists-moving-but-who-looks-after-them]. Pheriche,
  7 Nov 2022: black low trainers [P: https://commons.wikimedia.org/wiki/File:Carrying_Culture_-_The_Strength_of_a_Nepali_Porter,_Pheriche,_Nepal.jpg].
- **The only 8,000 m boots in Chapter 1** are on the father in the prologue (the Balcony, 8,400 m) and in luggage or props.

### 1.2 Download list (all no-login, licence checked in `boots_models.md` §4)

Download each model into its own new folder under `F:/UE_project/YggdrasilsPeak/Downloads/boots_2026-11-15/<id>/`, never
on C:. Downloads are untrusted data: inspect them with Blender (`--factory-startup`) or `python -I`, never run anything
from inside the folder.

URL prefixes: **GSO** = `https://fuel.gazebosim.org/1.0/GoogleResearch/models/<NAME>/1/<NAME>.zip` (OBJ + one 4096 px
colour PNG, metres, one shoe per model, CC BY 4.0, "Copyright 2020 Google LLC"). **OBJV** =
`https://huggingface.co/datasets/allenai/objaverse/resolve/main/glbs/<path>` (GLB, 1024 px textures, licence of the
original Sketchfab author).

| Wearer | Model (id) | Exact download | Licence (credit) | Tris / textures | Logos to remove | Notes |
|---|---|---|---|---|---|---|
| **Nima** | Garmont Hiking Boot (`garmont`) | OBJV `000-132/c46df86e358f489cacd09020cfdf7515.glb` | CC BY 4.0 (tyjajack, https://sketchfab.com/3d-models/garmont-hiking-boot-c46df86e358f489cacd09020cfdf7515) | 200,000 / 1×1024 colour | GARMONT wordmark on the side | Worn leather-and-fabric mid boot, dirty. 5/5 realism (thumbnail judgement) |
| **Marek** | Left Leather Boot, Danner (`danner`) | OBJV `000-120/783e6e237f3e4d6faa8538ce5001a143.glb` | CC BY 4.0 (jorgen_nelson, https://sketchfab.com/3d-models/left-leather-boot-783e6e237f3e4d6faa8538ce5001a143) | 99,999 / 1×1024 | Yellow Vibram octagon on the heel | Heavily worn brown leather, deep lug sole. **Left boot only: mirror it** |
| **Elena** | Salomon OUTline Mid GTX, women's (`salomon_outline`) | OBJV `000-073/5e24610ab910434b8e40b2a83199e98d.glb` | CC BY 4.0 (inciprocal.com, https://sketchfab.com/3d-models/salomon-outline-gtx-boot-5e24610ab910434b8e40b2a83199e98d) | 24,852 / 4 PBR maps at 1024 | Salomon logos | A modern women's mid hiker, 29.7 × 11.1 × 17.0 cm. Keep it clean and new: she is a first-timer [O] |
| **Ji-woo** | Fairbanks Snow Boot, Columbia (`fairbanks`) | OBJV `000-082/b39f6aaeb97549afb76a44375e93245b.glb` | CC BY 4.0 (inciprocal.com, https://sketchfab.com/3d-models/fairbanks-snow-boot-b39f6aaeb97549afb76a44375e93245b) | 19,836 / 4 maps at 1024 | Columbia marks | A tan/orange winter mid hiker, 29.4 × 11.3 × 16.1 cm. **Recolour to her palette** (§2.6). Alternative: GSO `TERREX_FAST_X_GTX` (low, 36,824 tris, 4K; adidas stripes to remove) |
| **Pema** | Reebok DMX MAX PLUS RAINWALKER (`gso_dmx`) | GSO `Reebok_DMX_MAX_PLUS_RAINWALKER` | CC BY 4.0 (Google LLC, Google Scanned Objects) | 27,446 / 4096 colour | Reebok marks | All-black walking shoe. Photos show older Khumbu women in sneakers [P]. **Alternative with no logo work:** "Priest Karol Wojtyla's sports shoes", **CC0**, OBJV `000-107/678f96cfadc741a3b260548eeb75954a.glb` (118,620 tris, old brown leather, red laces) |
| **Dolma** | Merrell Kid's Boot (`merrell_kid`) | OBJV `000-031/bbb0764dd0fb4f88b7e6acd6f62d6947.glb` | CC BY 4.0 (inciprocal.com, https://sketchfab.com/3d-models/merrell-kids-boot-bbb0764dd0fb4f88b7e6acd6f62d6947) | 22,110 / 4 maps at 1024 | Merrell marks | Purple/pink child's hiker with Velcro, 22.0 × 8.8 × 12.8 cm. A trekker hand-me-down is plausible [O]. Alternatives: GSO `GIRLS_DECKHAND` (canvas); Poly Haven `rubber_boots` (CC0, embossed logo) scaled down as "small rubber boots" |
| **Porter A** | Reebok HIMARA LTR (`gso_himara`) | GSO `Reebok_HIMARA_LTR` | CC BY 4.0 (Google LLC) | 28,674 / 4096 | Large Reebok vector logo | A synthetic trainer. **Recolour to black/dark grey** (Pheriche 2022 photo [P]) and dirty it heavily |
| **Porter B** | Men's Bahama in Khaki Oyster, Sperry (`gso_bahama`) | GSO `Mens_Bahama_in_Khaki_Oyster_xU2jeqYwhQJ` | CC BY 4.0 (Google LLC) | 20,600 / 4096 | Small Sperry mark | Canvas shoe [S: "canvas shoes"]. Alternative: `Mens_Striper_Sneaker_in_White_rnp8HUli59Y` dirtied to grey-brown |
| Props: camp boots in tent vestibules | Sorel Boot (`sorel`) | OBJV `000-020/9db6ee3d0f0b4dc7afb2b4791c153059.glb` | CC BY 4.0 (mikkoh, https://sketchfab.com/3d-models/sorel-boot-9db6ee3d0f0b4dc7afb2b4791c153059) | 98,120 / 1024 | Sorel marks | A pac boot: rubber shell, leather upper, felt cuff. One pair per client tent [S: "Camp Boots"]. Decimate to about 8k for a prop |
| Props: gear pile | Boots and crampons (`boots_crampons`) | OBJV `000-015/a8fda8369fc94d7da5984ca35dfeb7e8.glb` | CC BY 4.0 (robinschwartz, https://sketchfab.com/3d-models/boots-and-crampons-a8fda8369fc94d7da5984ca35dfeb7e8) | 358,846 / 1024 | none seen | Fused to a ground patch: **prop only**. Cut the patch, decimate to about 30k |
| **Optional (account)** | Old Lowa Gore-Tex Boot | https://sketchfab.com/3d-models/old-lowa-gore-tex-boot-2021-free-417531d1bfcd4fc39f9cedbc1c85963e | CC BY (inciprocal.com) | 19,900 faces / PBR 2k | LOWA logo | The best single match for a Sherpa guide's leather boot (a LOWA Tibet scan, 29.9 × 11.1 × 21.9 cm). Use it for Nima or Marek if the user makes an account (decision 0.4) |

**Do not use:** anything CC BY-NC or BY-NC-SA, Sketchfab "Standard"/"Editorial" licences, the Poly Pizza / Kenney /
OpenGameArt low-poly boots (stylised), or the Smithsonian ski boots (wrong silhouette) [S].

### 1.3 Fitting pipeline for scanned boots (applies to every row above)

The current boots are lofted round cross-sections ("flower pots": no heel counter, toe spring, tongue or laces). Replace
them with the fitted scans. Build this as a new `boots=("scan", "<glb or obj path>", recolour)` option in `garments.py`
`make_boots`, so the boot joins `SKM_G_<outfit>` and `AYggStoryNPC::DressAs` needs no change.

1. **Clean.** Delete scan bases and ground patches. Apply transforms. Work in metres (GSO is already metres) [S].
2. **Orient.** Sole flat on Z = 0. Toe along the character's foot axis (`foot_<side>` to `ball_<side>`, as the current
   `make_boots` computes `fwd`). Heel above the heel of the body mesh.
3. **Scale to the character's real foot.** Measure the MetaHuman foot on the body mesh (heel back to longest toe tip) = Lf.
   - Inner length = Lf + 15 mm, because a last is 13-17 mm longer than the foot [S: https://en.wikipedia.org/wiki/Paris_point].
   - Outsole length = inner length + 20-30 mm [E, from 280 mm last vs 295-305 mm outsole in `boots_models.md` §7.2].
   - Scale uniformly to that outsole length. Width correction may be at most ±5% [D], or the shape stops looking real.
   - Cross-check: EU 42 is built for a 265 mm foot and EU 38 for a 240 mm foot [S: https://en.wikipedia.org/wiki/Shoe_size].
4. **Clearance test.** Every body vertex of the foot, and of the calf below the boot top, must be inside the boot with at
   least 4 mm gap (BVH inside test). If any fails, scale up in 1% steps [D].
5. **Reduce.** Decimate to **8-12k triangles per boot** with an edge loop at the ball-of-foot flex line and at the ankle
   [S: `boots_models.md` §6]. Bake normal and AO from the full scan at 2048 px.
6. **Weights.** Behind the flex line: `foot_<side>` 1.0. From 1.5 cm in front of it: `ball_<side>` 1.0, with a 3 cm blend
   across the line. Shaft: blend from 0 at the ankle bone to 0.35-0.5 `calf_<side>` at the collar [E: starting values;
   check ankle flex in the walk cycle].
7. **Mirror** the single shoes (all GSO, and the Danner): duplicate, scale X by −1, recalculate normals. Offset the dirt
   mask on one side so the pair is not a perfect mirror [D].
8. **Textures.**
   - Paint out every logo in the albedo, and in the normal map where it is embossed (clone stamp in Krita or GIMP).
     CC BY 4.0 does not license trademarks (s.2(b)(2)) [S: https://creativecommons.org/licenses/by/4.0/legalcode.en].
   - Add wear for NPCs: dust rising 3-6 cm up from the sole, creases at the flex line (normal), scuffed toe caps, and
     water or salt lines on leather [E].
   - Elena keeps clean boots. Recolour Ji-woo's and Porter A's by HSV shift in the albedo.
9. **Material.** A new `M_ScanBoot` (albedo, normal, roughness and AO; leather roughness 0.55-0.75, rubber 0.85-0.95 [E])
   with a dust-amount parameter. Name the slot `<outfit>_boots_scan`, and extend `Tools/Story/import_garments.py` to read it.
10. **Credit** every CC BY model (template in §5). For GSO the author line is "Google LLC (Google Scanned Objects)". For
    Objaverse, credit the original Sketchfab author and also cite Objaverse (ODC-By, arXiv 2212.08051) [S].

### 1.4 Procedural boots (when no model exists, or as a fallback): build from a real last, not a loft

**Anatomy every procedural boot must have** (the parts missing from today's boots):
1. A **side-profile last curve** with a heel stack, a forefoot stack (the difference is the drop) and **toe spring**.
2. A **plan outline** that is asymmetric (straighter on the inner side), narrow at the waist, with a rounded toe box.
3. A **vertical heel counter**: a flat, near-vertical back, with the sole overhanging the heel by 5-8 mm [E].
4. A separate **tongue** panel with gussets, plus a padded **collar** set lower at the Achilles than at the ankle bones.
5. **Lacing** hardware: eyelets, a locking hook and speed hooks; laces as curves.
6. A **rand or welt** step where the sole meets the upper (5-12 mm overhang), plus **lugs** under the sole.
7. Bake a high-detail version (stitching, flex creases, leather grain from the CC0 textures below) down to 8-12k
   triangles [S: `boots_models.md` §7.6].

CC0 textures for procedural boots: Poly Haven `brown_leather`, `fabric_leather_01/02`, `scuba_suede`; ambientCG
`Leather001-039`, `Rubber001-004` [S].

**(a) Leather mid trekking boot** (fallback for Nima, Marek, Elena if the scans fail). Measured values are the LOWA
Renegade GTX Mid at men's US 9 (about EU 42) from RunRepeat's lab [S: https://www.runrepeat.com/lowa-renegade-gtx-mid].

| Part | EU 42 (men) | EU 38 (women) |
|---|---|---|
| Outsole length | 295-305 mm [E] | 268-277 mm [E] |
| Forefoot width (ball) | ~110 mm [S] | 98-101 mm [E] |
| Heel width | 85-88 mm [S] | 76-79 mm [E] |
| Waist width (under the arch) | 75-80 mm [E] | 68-72 mm [E] |
| Heel stack / forefoot stack | 36-38 / 21-24 mm [S] | 34-36 / 20-22 mm [E] |
| Drop | 12-17 mm [S] | same |
| Lug depth / rubber under lugs | ~5 mm / 2-3 mm [S] | ~4.5-5 mm |
| Toe spring | ~15 mm [S, via a search snippet of Shoemaker's Academy: re-check] | 13-15 mm |
| Shaft height from the arch | ~127 mm (5 in) [S, retailer] | 115-140 mm [E] |
| Ground to top of tongue | 170-220 mm [S, scan sizes] | 155-175 mm |
| Toe-box inside height / width | 28.4 / 95.8 mm [S] | ~26 / ~86 mm [E] |
| Tongue padding | 11-14 mm [S] | ~12 mm |
| Collar padding | 15-20 mm [E] | same |
| Lacing | 4-5 pairs of eyelets or webbing loops, 1 locking hook at the ankle flex line, 2-3 pairs of open speed hooks [O, from scan thumbnails] | same |
| Rubber toe cap | rises 20-30 mm at the toe [E] | same |

**(b) 8,000 m triple boot with integrated gaiter** (the father in the prologue; duffel and gear props). No usable free
model exists [S]. Build it from these values (all EU 42):

| Part | Value |
|---|---|
| Gaiter top above the arch | 380-400 mm [E, scaled from Scarpa Phantom 8000 "16.5 in from arch" at EU 46: https://www.outlandusa.com/p/scarpa-phantom-8000-blackorange-46] |
| Ground to top of gaiter | 410-440 mm [E] |
| Sole stack heel / forefoot / drop | 35 / 24 / 11 mm [S, Olympus Mons Pro: https://www.bergzeit.us/p/la-sportiva-olympus-mons-pro-shoes/3014382/] |
| Outsole length / forefoot width | 310-325 mm / 115-125 mm [E: no manufacturer figure found] |
| Toe spring | 18-25 mm [E, unverified: measure a side-on product photo] |
| Lug depth | 6-8 mm [E: no figure for 8,000 m boots; hiking boots are 5 mm and the Nepal Evo is 8.5 mm] |
| Crampon welts | a toe and heel ledge for automatic crampons, about 8-10 mm deep [S that they exist; E for depth] |
| Closures | a diagonal waterproof zip up the gaiter, a Velcro or BOA panel over the instep, a pull loop at the back [S, product descriptions] |
| Construction | thick EVA/rubber sole, broad rubber rand all round, a removable insulated inner boot (only seen if a boot is open on a prop) [S] |
| Colour | **non-signature**: charcoal gaiter #2B2D31 (0.024, 0.026, 0.031), slate-blue panels, black rand #1E1E1E. **Never** yellow/black (La Sportiva Olympus Mons) or orange/black (Scarpa Phantom) [S product names]. Check red/black against Millet's Everest boots before using it [O] |

**(c) Canvas sneaker** (fallback for porters). Converse Chuck 70 High, RunRepeat [S: https://runrepeat.com/converse-chuck-70-high]:
heel stack 26.2 mm, forefoot 17.6 mm, outsole rubber 7.4 mm thick, forefoot / heel width 103 / 74 mm, tongue
essentially unpadded (1.3 mm), 6-8 metal eyelets, rubber foxing tape about 30-35 mm tall at the toe [E], toe spring about
5 mm [S, via the same Shoemaker's Academy snippet].

---

## 2. Characters

### 2.0 Rules that apply to the whole cast

**Looks per scene.** The chapter runs from dawn in the camp, along the trail at midday, to dusk at the Thukla memorials
(about 4,830 m) and night in the tent (`Chomolungma_Chapter1.md` §2.1). Real layering changes with the hour, so build
up to three looks per character as separate `Cast.ygs` entries (for example `elena` and `elena_trail`) and select them
with the existing `look` field in `Chapter1.ygs`:

| Look | Time | What changes [source] |
|---|---|---|
| **A. Dawn camp** | Dingboche, sunrise, about −5 to −10 °C [E from Namche Oct 2 °C low via lapse rate; S for the range "−5 °C to −10 °C"] | Down jackets on for everyone, beanies, light gloves; sunglasses pushed up on the hat or hanging on a cord |
| **B. Trail, midday** | about +3 to +8 °C in sun [E] | Down jacket in the daypack; base layer plus fleece or softshell; **sunglasses ON for everyone** [S: "good, dark, polarized sunglasses" required]; sun hat or cap; buff [S] |
| **C. Dusk/night** | memorials, tent | Down jackets back on, beanies; in the tent, down booties or camp boots [S: gear lists] |

Pema and Dolma appear only at dawn, so they need look A only. **Build look A for everyone first** (P1); B and C are P2.

**Everyone, everywhere:**
- **No real brand logos** on any garment, boot, pack or device (legal, and the project rule) [S for trademarks].
- **Local people's gear is faded, mismatched and a few seasons old**, never a matching new catalogue set [S: Thamel is
  "virtually the entire supply... cheap counterfeits"; Sherpa guides often cannot afford brands; Everest Outfit was founded for that reason].
- **Hair shows under every hat.** `Cast.ygs` has **no Hair groom** for Nima, Pema, Marek, Ji-woo or porter_a, so
  their beanies sit on bald scalps (a bare nape and temples read as CG) [S current state]. Give every character a short
  Hair groom (per character below), set the `CAST` table's hide-hair flag in `Tools/Story/mh_cast.py` to `False`, and
  rebuild. Use **hair cards** for hat wearers ("Always Use Hair Cards"): less poke-through and cheaper on the laptop target [S: `faces_realism.md` §4.6].
- **Peach fuzz** `Peachfuzz_M_Thin` on every character (the only built-in one). Without it skin looks like plastic in rim light [S/D].
- **Skin roughness**: multiplier 1.10-1.15 for Sherpa and porter skin (dry, wind-burnt), 1.04-1.06 for clients. The engine
  clamps the range to 0.85-1.15, default 1.06 [S, engine source].
- **Accents** (8 regions, Redness / Saturation / Lightness, default 0.5): stay within ±0.25 of default, or the face goes
  blotchy and reads as make-up [S/D]. All accents are symmetric. Break the symmetry later with a texture override (phase 2).
- **Lips makeup**: if used as balm, set **Metalness 0**. The default is 1.0, which reads as metallic lipstick [S, engine default].
- **Irises**: lower GlobalSaturation from the default 2.0 to 1.2-1.5. The default looks like coloured contact lenses [S/D].
- **Material fade and wear** (new `M_Garment` parameters, P2):
  - Sun fade: blend the tint 15-25% toward a light grey on surfaces that face upwards (shoulders, hood, cap tops) for local people's gear [O].
  - Dust: a gradient from the hem up to about 25 cm on trousers and up to 6 cm on boots [O].
  - Abrasion: lighter, rougher patches at knees, seat, elbows and the pack-strap line on the shoulders [O].
  - Strength: Nima and porters 1.0, Marek 0.7, Pema 0.6, Dolma 0.8, Elena 0.2, Ji-woo 0.1 [D].

**Shared accessory meshes to build** (none exist yet). Search Poly Haven, ambientCG and GSO first; model only if nothing fits:

| Item | Dimensions | Used by | Tag |
|---|---|---|---|
| Wraparound sport sunglasses / glacier glasses with side shields | frame width 135-145 mm, lens height 40-50 mm, temples 135-145 mm, side shields about 25 × 20 mm | everyone in look B; pushed up on the hat in A | [S that they are worn; E for size] |
| Retainer cord for glasses | 2-3 mm cord | Nima, Marek | [O] |
| Sungdi blessing cord | 2 mm knotted cord, loop about 40-46 cm, one knot at the front | Nima | [S practice; E size; O knot position] |
| Buff / neck gaiter | tube around the neck, can be pulled up over the nose | all clients, porters, Marek | [S] |
| Handheld radio | about 60 × 35 × 120 mm, antenna 80-170 mm, clipped to a chest strap | Nima, Marek | [O] |
| Digital outdoor watch | 45-50 mm case, black resin strap | Nima, Marek | [O] |
| Daypack 30-50 L | client 40-50 L, Marek 30-40 L | clients, Marek | [S 40-50 L on IMG list] |
| Trekking poles | 110-135 cm | clients, Marek | [S poles on list; E length] |
| Action camera on chest harness | camera about 62 × 45 × 33 mm; harness of 25-38 mm webbing | Ji-woo | [O] |
| Phone gimbal | about 28 cm folded | Ji-woo | [O] |
| Expedition duffels | 90-120 L, about 80-90 cm long × 40-45 cm diameter; yellow, blue, orange, red | porters (2-3 roped), camp | [P: Namche bridge 2017; E size] |
| Doko basket (conical bamboo) | 70-90 cm tall, top 50-60 cm, base 25-30 cm in diameter | porter B, villagers | [S it exists; E size] |
| Namlo tumpline | flat strap 3-5 cm wide across the forehead, over a folded cloth | porters | [S; E width] |
| Tokma T-stick | about 1.0-1.2 m, a T-shaped head to rest the load on | porters | [S; E length] |
| Khata | existing `SM_Khata.glb` | Pema (to give), shrine | [S: given at farewells and blessings] |

---

### 2.1 Nima: older Sherpa sirdar, climbed with the father

**Basis.** Sherpa men have worn Western clothes day to day for decades [S: Wellcome 1986, https://identity.wellcomecollection.org/works/ddxn2b8v];
men who work on expeditions wear Western high-altitude gear [S: Gale 2008, https://www.everyculture.com/wc/Mauritania-to-Nigeria/Sherpas.html].
His wardrobe mixes client hand-me-downs, the Nepali brand Everest Outfit and Thamel counterfeits
[S: https://english.onlinekhabar.com/everest-outfit-affordable-made-in-nepal-climbing-gear-by-and-for-mountaineers.html].

**Garments (`OUTFITS["nima"]`)**
| Piece | Spec | Colour sRGB (linear) | Fabric id | Tag | Today → change |
|---|---|---|---|---|---|
| Down jacket, hooded | mid-weight, a few seasons old, sun-faded on the shoulders and hood, one small patch or burn mark on a sleeve | faded mustard #AC8640 (0.413, 0.238, 0.051) | nylon | [S expedition colours; O wear] | Today bright mustard #E7AD3F: lower the saturation, add fade (P2) |
| Jacket yoke / panels | charcoal, not pure black | #2E2E30 (0.027, 0.027, 0.030) | nylon | [D] | Today (0.025, 0.025, 0.03): fine |
| Mid layer, seen at the collar | fleece collar showing 2-3 cm above the jacket collar | slate grey #3F4447 (0.050, 0.058, 0.063) | fleece | [S: fleece or down-vest mid layer] | **Add** (P2): it is the layering cue |
| Trousers | synthetic softshell trekking trousers, worn lighter at the knees | #2B2B2D (0.024, 0.024, 0.026) | softshell | [O] | Fine; add abrasion (P2) |
| Hat, look A | knit beanie with pompom | faded maroon #6E2A2C (0.156, 0.023, 0.025) | knit | [O] | Today brighter red #A2383F: darken (P3) |
| Hat, look B | baseball cap, sun-bleached, no logo | faded dark navy #3A4352 (0.042, 0.056, 0.084) | softshell | [O] | New (P2) |
| Boots | Garmont scan (§1.2) | from the scan | scan | [S] | **P1**: replace the lofted boot |

**Accessories**
- **Sungdi**: 1-2 thin, faded, knotted cords at the throat, half hidden under the collar. Colours: faded red #A3392F (0.366, 0.041, 0.028) and/or gold #C9A13B (0.584, 0.356, 0.044) [S practice: Lama Geshe's "knotted gold string" (https://www.nationalgeographic.com/adventure/article/dispatch-april-4-text); RMI 2025 "a sungdi, a string we wear around our neck" (https://www.rmiguides.com/blog/2025/03/16/everest_base_camp_trek_grom_team_receive_blessing_in_tengboche); O that Nima wears one]. P2.
- **Glacier glasses** pushed up on the beanie (look A), worn (look B) [S]. P2.
- Handheld radio on a chest strap [O]. Cheap black digital watch, left wrist [O]. Wrist mala optional [O, no source]. P3.

**Hair, face and skin (MetaHuman; preset Aoi)**
| Setting | Value | Tag |
|---|---|---|
| Hair groom | `Hair_S_Clean` or `Hair_S_RecedeMessy` (cards under the hat) | [S grooms exist; LOW men today wear short hair] |
| Hair colour | Melanin 0.9, Redness 0.05-0.1, Roughness 0.4, **Whiteness 0.4**, Lightness 0.6 | [S controls; LOW values] |
| Beard / moustache | **Remove `Beard_L_Full` + `Mustache_L_Full`** (P1). Use `Mustache_S_Uneven` or none, plus `Beard_S_Stubble` or a low-density `Goatee_L_Scraggly`, or nothing. Whiteness 0.4 | [MED: East Asian-ancestry men have much sparser facial hair (chest hair 0.8 vs 3.0 on a 0-4 scale); Sherpas not measured directly] |
| Eyebrows / lashes | keep `Eyebrows_M_Messy`, Whiteness 0.35; `Eyelashes_S_Sparse`, lash Melanin 0.8 | [LOW] |
| Skin U | about +0.1 darker than the Aoi preset (read the current `skin_settings.skin.u` first) | [LOW] |
| Roughness | 1.12 | [LOW] |
| Accents | Cheeks R .60 / S .48 / L .43 (browner, less pink: older skin furrows and pigments more than it reddens); Nose R .58, L .45; Ears R .58; Forehead L .46 | [S pattern: Singh 2017, https://pmc.ncbi.nlm.nih.gov/articles/PMC5286755; LOW values] |
| Freckles | Type2, Density .35, Strength .2, Saturation .35 (sun spots) | [LOW] |
| Eyes | dark brown, GlobalSaturation 1.3; `bUseCustomTint` on, warm off-white sclera, VascularityIntensity 1.5, Coverage 0.28; optionally a small nasal pterygium in one eye (texture) | [S: pterygium in 10.1% of Upper Mustang adults, rising with age, https://nepjol.info/index.php/NEPJOPH/article/view/10774/8749] |
| Hands | PalmCavityDarkness 0.4, nail roughness 0.7 | [LOW] |
| Face Texture Index (phase 2) | wrinkles High, stubble Low: 11, 19, 51, 53, 54, 71 or 140 (check in the editor that list entry i = index i) | [S list; assumption flagged] |

**Must be true**
- [ ] Outdoor gear, not a chuba; faded and slightly mismatched.
- [ ] Short black-and-grey hair visible at the nape and temples under the beanie.
- [ ] Sparse facial hair at most.
- [ ] Face visibly darker and warmer than the neck; matte skin.
- [ ] Glasses on him (on the hat at dawn, on the face at midday).
- [ ] At least one sungdi cord visible at the throat in close-ups.
- [ ] Trekking boots with heel, toe spring, tongue and laces.

**Must not appear**
- [ ] Chuba, fur robe or monk-like clothes in camp.
- [ ] A long full beard.
- [ ] A brand-new matching kit, or logos.
- [ ] Big turquoise or coral necklaces.
- [ ] An 8,000 m down suit or double boots at 4,300 m.
- [ ] A khata worn as an everyday scarf (it belongs only to the blessing or farewell moment) [S].

---

### 2.2 Aunt Pema: Sherpa woman, about 50, walks up from the village at dawn

**Basis.** Older Khumbu women commonly wear the long dark wrap dress and striped apron **with modern layers on top**.
- Pangboche, 21 Oct 2009 [P: https://commons.wikimedia.org/wiki/File:Pangboche,_Sherpa_senior_woman,_Nepal.jpg]: bright multicolour headscarf, dark patterned knit cardigan, long dark skirt, narrow multi-stripe apron, white sneakers.
- Phurte, 12 Oct 2009 [P: https://commons.wikimedia.org/wiki/File:Bhote_Koshi_Valley,_Phurte_village,_Woman,_Nepal.jpg]: cream knit hat, mauve padded jacket, long dark skirt, striped front apron.

**Garment names** (spellings vary between sources) [S: Wikipedia citing Lhakpa Norbu Sherpa 2008; Ang Diku Sherpa, https://www.musamasala.com/blog/blog-post-title-two-5nmj4-7rpwj]:
- **angi** (aangi, engi): the sleeveless wrap dress, worn over the **honju** blouse
- **pangden**: the front apron
- **gyabtil**: the back apron
- **kyetig**: the embossed silver buckle
- **kara**: the sash

**Today**: a maroon long-sleeved chuba with **gold brocade collar and cuffs**, a red sash, a front apron only, and a dark-red
scarf (`garment_pema_*.png`). Change it as below. **P1** for the dress colour and trim, the jacket on top, and the back apron.

**Garments (`OUTFITS["pema"]`)**
| Piece | Spec | Colour sRGB (linear) | Fabric id | Tag |
|---|---|---|---|---|
| Angi (dress) | **sleeveless** wrap dress, ankle-length (hem 3-8 cm above the ground [E]), wrapped and belted, plain, **no gold brocade**. Today's factory fabric, not homespun | very dark brown-charcoal #2E2826 (0.027, 0.021, 0.019) | wool | [S/P dark plain; S factory fabric] |
| Honju (blouse) | long sleeves; at dawn hidden under the jacket, so only a few cm at the collar if at all | colour unknown: use a muted dark teal or maroon synthetic | brocade or linen | [O: no source on colours, open question] |
| Padded jacket over the angi | hip-length synthetic padded jacket (thinner baffles than a down jacket), closed at dawn; covers the top of the aprons | mauve #8C5A7A (0.262, 0.102, 0.195) | nylon | [P Phurte]. Alternative: a dark patterned knit cardigan [P Pangboche] |
| Pangden (front apron) | narrow; waist to near the hem; **many-coloured horizontal stripes**, not plain red. Keep `make_pangden` but desaturate 15% and add a dust multiply | existing T_Pangden palette, faded | pangden | [S/P] |
| Gyabtil (back apron) | **add**: wraps the back and sides of the skirt, waist to about 10 cm above the hem [E]. Use a second stripe texture with a muted, darker palette | base #3A2F2A (0.042, 0.028, 0.023) | pangden variant | [S that it exists and that married and unmarried women both wear it; O for its pattern: check the SOAS/Wellcome photos] |
| Kyetig (buckle) | an embossed round or oval silver buckle at the front waist, 5-7 cm [E] | metal: base about (0.95, 0.93, 0.88), metallic 1, roughness 0.45 (tarnished) | metal | [S exists; E size] |
| Kara (sash) | woven sash at the waist, mostly hidden by the jacket | dark red #7A2A22 (0.195, 0.023, 0.016) | linen | [S exists; O colour] |
| Headscarf | bright multicolour floral, tied at the nape (keep `make_hat` scarf style). Hair shows at the forehead edge and the knot | pink base #C2577F (0.539, 0.095, 0.212) with green #4E8A4A (0.076, 0.254, 0.068) | floral | [P Pangboche]. Alternative: cream knit hat [P Phurte] |
| Shoes | black walking shoes (GSO DMX scan) or white sneakers | scan | scan | [P] |

**Accessories**
- Small gold earrings [O: daily-wear amounts are not documented].
- A folded white **khata** in her hand for the farewell [S: khata given at farewells; D: story staging].
- Optional: a thermos [O].

**Hair, face and skin (preset Lani)**
| Setting | Value | Tag |
|---|---|---|
| Hair groom | `Hair_S_PulledBack` or `Hair_S_LowPonytail` (cards), so the tail shows below the scarf. Later a custom long braid groom (none is built in) | [LOW-MED long hair, tied back or braided] |
| Hair colour | Melanin 0.92, Roughness 0.4, Whiteness 0.15-0.25 | [LOW] |
| Do not use | `Hair_M_BobCurly` and `Hair_S_Pixie`, which are already in her folder | [LOW-MED] |
| Eyebrows | sparser than `Eyebrows_M_Wide`, not `L_Shaded` | [LOW] |
| Skin U | about the Lani preset or slightly darker | [LOW] |
| Roughness | 1.12 | [LOW] |
| Accents | Cheeks R .66 / S .55 / L .45 (brick, not pink), Nose R .58, UnderEye L .45 | [S Plateau Red Face pattern; LOW values] |
| Freckles | Type1, low (lentigines). Melasma would need a texture override (phase 2) | [LOW] |
| Eyes | dark brown, Saturation 1.4, VascularityIntensity 1.3 (cooking smoke) | [S smoke as an eye irritant; LOW values] |
| Makeup | none. Optionally a very faint grey-brown `SoftSmokey` at low opacity for smoke-reddened lids | [LOW] |
| Face Texture Index (phase 2) | wrinkles Medium-High, stubble None, e.g. 20, 47, 48, 50 or 52 | [S list] |

**Must be true**
- [ ] Long plain **dark** dress to the ankles.
- [ ] **Two aprons**: a striped front pangden (married, decision 0.2) and a back apron.
- [ ] A modern jacket or cardigan on top at dawn.
- [ ] Headscarf or knit hat, with hair visible at the edge.
- [ ] Sneakers or walking shoes.
- [ ] Small earrings only.
- [ ] Hands with dark palm creases (PalmCavityDarkness 0.4).

**Must not appear**
- [ ] A **nose ring** (no Sherpa dress source mentions one; nose ornaments belong to other hill groups) [O, flagged].
- [ ] Coral, turquoise, amber or dzi necklaces, gold headbands or a gau (festival dress) [S].
- [ ] Gold brocade edging on an everyday dress [P: plain in both photos].
- [ ] Traditional felt boots [S: festival only].
- [ ] A sari or kurta (lowland Nepali dress).
- [ ] A Tibetan nomad sheepskin, a one-sleeve-off chuba, or Kham/Amdo red hair tassels.
- [ ] A plain red apron.

---

### 2.3 Dolma: Sherpa girl, 8

**Basis.** The 1986 record says girls always wore traditional dress [S], but that is 40 years old. Today's evidence for
Western children's clothes is indirect only: a 2009 Namche photo of a young woman in a red fleece
[P: https://commons.wikimedia.org/wiki/File:Namche_Bazaar,_Homestay,_Nepal.jpg]. Traditional dress belongs to festivals
(Losar, Dumji, Mani Rimdu) [S]. Confidence: low-medium.

**Build first** (decision 0.3). Height about 120-128 cm [E: WHO median for 8-year-old girls is about 127 cm, from memory, so
verify; local children may be shorter, unverified].

**Garments (new outfit `dolma`)**
| Piece | Spec | Colour sRGB (linear) | Fabric id | Tag |
|---|---|---|---|---|
| Puffer jacket | child's down/synthetic puffer, sleeves 2-4 cm too long (a hand-me-down), dusty, a little worn | faded fuchsia #B84A78 (0.479, 0.068, 0.188) | nylon | [O colours; O hand-me-down] |
| Trousers | track pants or leggings, 1-2 side stripes (**never three stripes**, an adidas trademark), knees dusty | navy #232C45 (0.017, 0.025, 0.060) | softshell | [O] |
| Hat | knit hat **with ear flaps** and ties (as written in `Chapter1_Design.md` Scene 2), striped | red #A8322D (0.392, 0.032, 0.026), teal #2C7A7B (0.025, 0.195, 0.198), yellow #D6A834 (0.672, 0.392, 0.034) | knit | [D design doc; O realism] |
| Footwear | Merrell kid's scan, or small rubber boots | scan | scan | [O] |

**Face (child base, when it exists):**
- Accents: Cheeks R .75 / S .62 / L .46 (chapped red); Lips S .42, L .53 [S: highland redness starts in childhood; LOW values].
- Slightly runny nose, dirty fingers [O].
- Two braids or a ponytail; Melanin 0.92.
- Dark brown eyes.

**Must be true**
- [ ] Child proportions (head about 1/6 of height), not a scaled adult.
- [ ] Red, wind-chapped cheeks.
- [ ] Clothes a little dusty and slightly too big.
- [ ] Hair visible under the ear-flap hat.

**Must not appear**
- [ ] A **front apron** (it marks a married woman) [S].
- [ ] Adult jewellery.
- [ ] Spotless clothes.
- [ ] Festival dress (unless a festival scene is written).
- [ ] The current tinted `scale 0.62` placeholder in a shipped build.

---

### 2.4 Marek Kovač: Slovak expedition leader, 40s

**Basis.** Experienced leaders wear well-used, good technical layers in muted colours [O: a style call, not a sourced
fact]. Northfinder is the largest Slovak outdoor-clothing maker [S, self-reported]. Keep everything logo-free.

**Today**: olive shell, black softshell trousers, dark beanie, **yellow mountain boots** (P1), mutton chops with a
horseshoe moustache (P1).

**Garments (`OUTFITS["marek"]`; look A = dawn, look B = trail)**
| Piece | Spec | Colour sRGB (linear) | Fabric id | Tag |
|---|---|---|---|---|
| A: insulated hooded jacket | synthetic or down belay-style hoody, well used, abrasion at the cuffs | dark olive #4A4F37 (0.068, 0.078, 0.038) | nylon | [O] |
| B: softshell jacket | worn, pack-strap abrasion on the shoulders | slate #4D555B (0.074, 0.091, 0.105) | softshell | [O] |
| Trousers | softshell, lighter at the knees and seat | charcoal #2F3033 (0.028, 0.030, 0.033) | softshell | [O] |
| Hat | A: charcoal beanie #3A3B3D (0.042, 0.044, 0.047). B: sun-bleached cap #5A6475 (0.102, 0.127, 0.178), no logo | | knit / softshell | [O] |
| Boots | Danner scan, mirrored (§1.2). **Not mountaineering boots** | scan | scan | [S] |

**Accessories:** glacier glasses with side shields on a retainer cord [S kit lists]; radio in a chest harness [O];
GPS/altimeter watch [O]; poles; 30-40 L pack [O].

**Hair, face and skin (preset Bruce)**
| Setting | Value | Tag |
|---|---|---|
| Hair | `Hair_S_BrushCut` (cards under the beanie) | [D] |
| Facial hair | **replace** `Beard_M_MuttonChops` + `Mustache_S_Horseshoe` with `Beard_S_Full` or `Beard_M_Stubble` + `Mustache_S_Full`; a little Whiteness at the chin | [LOW: style judgement; weeks into a season] |
| Accents | fair skin that burns: Nose R .75, Cheeks .68, Ears .66, Chin .60 (snow-reflected UV), Forehead .56 | [S burn sites: under the chin, nostrils and ears burn from snow reflection, Singh 2017; LOW values] |
| Freckles | Type3, light | [LOW] |
| Roughness | 1.06 | [LOW] |
| Glasses tan | a clear lighter band round the eyes (texture override, phase 2) | [LOW-MED] |
| Face Texture Index (phase 2) | wrinkles Medium, stubble Low: 3, 115 or 143 | [S list] |

**Must be true:** muted, used technical kit; hiking boots; glasses on a cord; sunburn on the nose, ears and under the chin;
a short, untidy beard.

**Must not appear:** double or 8,000 m boots on the trail [S]; yellow/black or orange/black boots; a brand-new kit;
logos; a stylised "biker" beard.

---

### 2.5 Elena Ruiz: Spanish first-time client

**Basis.** Many trekkers rent down jackets and down trousers in Thamel for under $1 a day; older ones can be "light on down".
Big-brand gear sold in Thamel is almost all counterfeit [S: https://www.switchbacktravel.com/nepal/buying-renting-trekking-gear-kathmandu].
Clients reach Dingboche around day 5-6 from Lukla, so they show **early** sun damage, not a weathered look [S itineraries].

**Today**: rust-orange shell, grey softshell trousers, dark `mountain` boots, **no hat**, `Hair_S_BobLayered`,
`Eyelashes_L_ThickCurl`.

**Garments (`OUTFITS["elena"]`)**
| Piece | Spec | Colour sRGB (linear) | Fabric id | Tag |
|---|---|---|---|---|
| A: rented down jacket | **one size too big** (inflate 10-15% beyond fit, sleeves 3 cm long), faded, slightly flat baffles ("light on down") | faded red #9E3D34 (0.342, 0.047, 0.034) | nylon | [S rental; O fit/fade] |
| Mid layer at the collar | newer fleece | teal #2F6F73 (0.028, 0.159, 0.171) | fleece | [O] |
| Trousers | newer softshell trekking trousers, clean | mid grey #55575B (0.091, 0.095, 0.105) | softshell | [S softshell trousers on list] |
| Hat | A: knit beanie, teal #2E6B6E (0.027, 0.147, 0.156). B: **wide-brim sun hat** (brim 7-8 cm [E]), stone #B5A98C (0.462, 0.397, 0.262) | | knit / linen | [S sun hat on list] |
| Buff | patterned blue-grey, round the neck | | knit | [S] |
| Boots | Salomon OUTline scan, **kept clean and new** | scan | scan | [S hiking boots; O new] |

**Accessories:** 40-50 L daypack, trekking poles [S]; sunglasses in look B [S].

**Hair, face and skin (preset Vivian, skin U 0.36 already set)**
| Setting | Value | Tag |
|---|---|---|
| Hair | `Hair_S_LowPonytail` (tied back; shows below the beanie) instead of `Hair_S_BobLayered` | [LOW] |
| Lashes | `Eyelashes_S_Fine` instead of `L_ThickCurl` (which reads as mascara) | [LOW] |
| Eyes | brown or hazel instead of the preset's green (optional) | [LOW: brown is most common in Spain; green exists] |
| Accents (day-6 sunburn) | Nose R .70 / S .58, Cheeks R .60, Ears R .62, **Chin R .56**, Lips S .42 / L .54 (dry, pale) | [S burn sites; LOW values] |
| Lip balm | Lips makeup `Natural`, low opacity, Roughness 0.2-0.35, **Metalness 0** | [S default trap; LOW look] |
| Roughness | 1.04 | [LOW] |

**Must be true:** a slightly baggy, faded rented down jacket over newer kit; new-looking boots; a pink nose, ears and
under-chin; hair tied back and visible; a sun hat on the trail.

**Must not appear:** jeans; a cotton hoodie as the main layer; thick curled lashes or make-up; mountaineering boots on the trail.

---

### 2.6 Ji-woo Han: Korean client who films everything

**Basis.** Korean hikers are known for expensive head-to-toe branded outfits [S: AFP 2013 via https://arabnews.com/news/461562].
Older hikers' gear is "vividly colored"; younger hikers lean to muted gorpcore [S: https://www.koreaherald.com/article/3263841].
Korean brands sell UV face masks and arm sleeves, but **no source confirms Korean trekkers in Nepal dress this way** [S products; O Nepal].
Preset Aera (a woman).

**Palette by age** [S/D]: if she is in her 20s-30s, a **muted coordinated** set (below); if 40s or older, the same pieces in
vivid coordinated colours (for example coral with teal).

**Garments (`OUTFITS["jiwoo"]`)**: the one client whose kit may look new and expensive, still with no logos.
| Piece | Spec | Colour sRGB (linear) | Fabric id | Tag |
|---|---|---|---|---|
| Down jacket | matching set with the trousers, new | navy #24324F (0.018, 0.032, 0.078) | nylon | [S coordinated] |
| Trousers | same family | navy-black #262C3C (0.019, 0.025, 0.045) | softshell | [S] |
| Hat | A: cream beanie #D9D4C7 (0.694, 0.658, 0.571), as today. B: cream bucket or brimmed hat #D2CCBC (0.644, 0.604, 0.503) | | knit / linen | [O] |
| Face cover | buff or UV mask, pulled down to the chin when she films herself | cream or navy | knit | [O] |
| Gloves | matching navy | | softshell | [S gloves on list] |
| Boots | Fairbanks scan **recoloured** to navy/grey | scan | scan | [S mid boots] |

**Accessories:** action camera on a chest harness; phone gimbal in hand; power bank [O].

**Hair, face and skin:**
- **Add a Hair groom** (`Hair_S_LowPonytail` or `Hair_M_BobStraight`, cards); today she has none under the beanie (P1).
- Keep `Eyelashes_L_SlightCurl`. Check that `Eyebrows_L_Shaded` does not read as drawn-on make-up [D].
- Accents: Nose R .66, Cheeks .58, Chin .55. Iris Saturation 1.5. Roughness 1.04 [LOW].

**Must be true:** a visibly coordinated outfit; filming kit on her body; hair visible; early sunburn.

**Must not appear:** logos (the brand look must come from cut and colour, not marks); a spotless studio look on her face;
yellow mountain boots.

---

### 2.7 Porters (porter_a; porter_b planned)

**Basis.**
- Most porters on the trail are **not Sherpa**: they come from the lower eastern hills (named examples: Dahal, Magar,
  Tamang, Kulung) [S: https://kathmandupost.com/national/2024/06/08/lugging-loads-for-a-living-on-treacherous-everest-trails;
  https://www.himalayanclub.org/hj/64/11/lowland-porters-in-the-solu-khumbu/].
- They are under-dressed on a budget [S].
- Donated Western jackets often do not fit smaller porters [S: https://keepnepal.org/program-category/porters-welfare-program].
- Trekking porters carry 30 kg officially and 35-40 kg in practice, usually stacked, roped client duffels on a namlo [S].

**Today**: a maroon fleece, navy beanie, khaki trousers and trainers. The face is preset Kelvin with a **curly fade, chinstrap
goatee and curly moustache**, and pale skin (P1).

**Porter A garments (Pheriche, 7 Nov 2022 photo [P])**
| Piece | Spec | Colour sRGB (linear) | Fabric id | Tag |
|---|---|---|---|---|
| Puffer jacket | cheap, **oversized** (sleeves 3-5 cm past the wrist, hem low), slightly shiny nylon (roughness 0.35-0.45), **hood up over a folded cloth** | black #2A2A2C (0.023, 0.023, 0.025) | nylon | [P; S oversized donated jackets] |
| Head cloth | folded beige cloth under the hood; the namlo runs over it | #B8A88A (0.479, 0.392, 0.254) | linen | [P; S namlo over cloth] |
| Face | buff or mask over the mouth and nose (dust) | grey #5A5A5A (0.102, 0.102, 0.102) | knit | [P] |
| Trousers | olive cargo trousers **rolled or cut to mid-calf**, dark socks showing | #4B4E35 (0.070, 0.076, 0.036) | softshell | [P] |
| Shoes | black low trainers (HIMARA scan, recoloured, dirty) | scan | scan | [P/S] |
| Sunglasses | cheap, or none | | | [S: UIAA urges leaders to give porters sunglasses, which implies many go without] |
| Load | 2-3 client duffels (yellow/blue/orange/red) roped in a stack on the namlo; T-stick tokma in hand | | | [S/P] |

**Porter B (when built)**: faded red knock-off puffer #8E2D2A (0.270, 0.026, 0.023), grey hoodie under it #6B6B6E
(0.147, 0.147, 0.156), navy track pants #2A3550 (0.023, 0.036, 0.080) with 1-2 stripes, khaki canvas shoes (Bahama scan), cap or beanie; a doko with lodge supplies
(kerosene can, LPG cylinder, crates) and a tokma [S/P/O]. Include some older men and the occasional woman porter across the chapter [S].

**Face and hair (porter_a)**
- **Re-pick the preset** from `Saved/Compare/faces/_sheet.png`: a lean South or East Asian male face, 20-40 [D].
- If Kelvin stays: replace `Hair_S_CurlyFade` with straight `Hair_S_Messy` / `Hair_S_Casual` / `Hair_S_BuzzCut` at Melanin 0.92;
  **remove** `Goatee_S_ChinStrap` and `Mustache_S_Curly` (at most `Mustache_S_Stubble`) [MED].
- Skin: no lighter than Nima's [LOW]. Roughness 1.10. Accents: Cheeks R .72 / S .6 / L .45, Nose .62, Ears .6.
  Eyes: dark brown, Vascularity 1.2 [S: about half of porters at an eye camp had an eye condition].
- Hands: PalmCavityDarkness 0.5, short dull nails.

**Must be true:** cheap, ill-fitting warmth; a buff over the mouth; rolled trousers; cheap shoes; a namlo with a cloth
under it; a real load; non-Sherpa as written (in subtitles or UI, never "our Sherpa" for a porter) [S: Project Himalaya warns against it].

**Must not appear:** high-end technical gear; rucksacks with hip belts carrying the load; a curly fade, chinstrap or curly moustache;
pale skin; "Sherpa" as his job title.

### 2.8 New `garments.py` / pipeline features this section needs (checklist for the TA)

| # | Feature | For | Priority |
|---|---|---|---|
| G1 | `boots=("scan", path, recolour)`: load, fit and weight a scanned boot (§1.3) | everyone | P1 |
| G2 | `chuba(sleeveless=True, trim=None)` = angi; jacket layered **over** the angi bodice, with the hem at the hip | Pema | P1 |
| G3 | back apron (`apron_back=True`, its own stripe texture) | Pema | P1 |
| G4 | Hair groom for every hat wearer (`mh_cast` hide-hair flag False, select short grooms, rebuild, rewrite `Cast.ygs`) | Nima, Pema, Marek, Ji-woo, porter | P1 |
| G5 | Facial-hair swaps (Nima, Marek, porter) and Elena's lashes and hair | | P1 |
| G6 | mid-layer collar ring (fleece showing above the jacket collar) | Nima, Elena, Marek | P2 |
| G7 | `fit=1.1-1.15` oversize factor plus long sleeves | Elena (rented), porters, Dolma | P2 |
| G8 | hood-up option, head cloth, rolled trouser hems (mid-calf) | porter A | P2 |
| G9 | cap with brim, wide-brim sun hat, bucket hat, ear-flap knit hat | look B; Dolma | P2 |
| G10 | buff, glasses, sungdi, radio, watch, camera harness (shared meshes, §2.0) | as listed | P2 |
| G11 | `M_Garment` sun-fade, dust and abrasion parameters | all | P2 |
| G12 | looks B and C as separate `Cast.ygs` entries selected by `look` | clients, Marek, Nima | P2 |
| G13 | skin, eyes and makeup sliders scripted in `mh_cast` (`commit_skin_settings`, `commit_eyes_settings`, `commit_makeup_settings`, all BlueprintCallable in 5.8 [S]) | all | P2 |
| G14 | Face Texture Index and texture or material overrides for dirt, hat-line and glasses tan, asymmetric sun damage (5.8 "Material Overrides" with "Inherit UI Parameters", untested) | all | Phase 2 |

---

## 3. House exterior and kitchen interior

**Basis.** Sestini & Somigli, *Sherpa Architecture* (UNESCO 1978, 1975 survey)
[S: http://web.mit.edu/scot/public/oed/Sherpa%20Architecture%20-%20Valerio%20Sestini,%20Anzo%20Somigli%20(9232016125).pdf];
Stonor, *The Sherpa and the Snowman* (1955, eyewitness 1954)
[S: https://archive.org/download/in.ernet.dli.2015.149169/2015.149169.The-Sherpa-And-The-Snowman_text.pdf]; Frontiers in
Built Environment 2023 (223 Thame-valley buildings)
[S: https://www.frontiersin.org/journals/built-environment/articles/10.3389/fbuil.2023.1086008/full].

Files: `Tools/Blender/sherpa_house.py` (visual house; frame: +X = today's door wall, +Y = window wall, metres),
`Tools/Story/build_chapter1_world.py` (collision shell `L_, W_, H_, T_ = 600, 460, 240, 40` cm, furniture placement, `RY`),
`Content/Story/Chapter1_places.ygs`, `Tools/Story/story_shots.ps1`.

### 3.1 Exterior checklist

| # | Item | Target | Tag | Today | Action | Pri |
|---|---|---|---|---|---|---|
| X1 | Orientation | All doors and windows on the **long, sunny, downhill façade** (S or SE). **Back wall blind**, dug into the slope | [S: Sestini; Frontiers; Wellcome 1986] | Door on the +X gable end; window on +Y | Put the door on the +Y wall. Rotate the house (`RY`) so +Y faces S-SE and downhill (south from the terrain georeference). Bury the lower 0.5-1.5 m of the −Y wall in the slope where the terrain allows | P1 |
| X2 | Door | Heavy plank leaf on wooden pins, frame set into the wall, a hood of stone slabs above. 0.9-1.0 m wide × 1.7-1.8 m tall, threshold 15-20 cm | [S construction; E size] | 1.0 × 1.9 m on +X | `DOOR` onto +Y, at the +X end of the façade, at least 0.3 m from the inner corner; lower it to 1.75 m; add a stone-slab hood | P1 |
| X3 | Windows | **2-3** on the front wall. Heavy wooden frame joined without nails, a **lattice counter-frame** (squares and oblongs), glass panes, a small hood over the top. Each 0.7-0.8 m wide × 0.8-0.9 m tall, sill 0.75-0.85 m above the floor (just above the bench) | [S count and construction; E size] | One window, 0.9 × 0.7 m, sill 1.0 m, cross bars | Two windows between the hearth and the door. Example with the interior x range −2.6 to 2.6 m: W1 x −0.55 to 0.20, W2 x 0.40 to 1.15, door x 1.40 to 2.30 (adjust to the props) | P1 |
| X4 | Wall thickness | 0.6 m (traditional walls are 0.6-1.0 m; 0.635 m measured at Hilajung). Show deep window reveals | [S] | 0.40 m | `T = 0.6`. Grow the outer size to **6.4 × 5.0 m** so the interior stays 5.2 × 3.8 m and the furniture stays put. Builder: `L_, W_, T_ = 640, 500, 60` | P1 |
| X5 | Corners and openings | **Larger squared quoins** at corners and opening edges: 0.45-0.7 m long, 0.2-0.3 m tall, alternating long and short | [S: Sestini; E size] | Interlocking coursed stones, no quoins | Add a quoin pass in the stone generator | P2 |
| X6 | Wall finish | Dry or clay-mortared rubble. Older faces partly coated in **ochre-brown mud plaster** (30-60% of the front, weathered off at the base and edges) | [S plaster; E share] | Bare dry stone | Add a plaster layer, ochre #8A6A48 (0.254, 0.144, 0.065), on the front façade | P2 |
| X7 | Stone colour | Light-to-mid grey (leucogranite, banded gneiss), grey-brown weathering, rust stains, pale-green, orange and black lichen; darker near the ground and under the eaves | [S geology; E look] | Mid grey #6C6A67 (0.15, 0.145, 0.135) with lichen | OK. Vary between #8E8A84 (0.270, 0.254, 0.231) and #6E6A64 (0.156, 0.144, 0.127); add rust streaks | P3 |
| X8 | Roof form | Gable (ridge) roof, 15-25°; eaves are the roofing running past the wall. **Never flat (Tibetan)** | [S gable; E pitch] | 18° in Blender, 14° in the collision shell | Keep 18°; set the builder pitch to 18° for consistency | P3 |
| X9 | Roof material | The family house may keep **boards weighted with stones** (documented into the 1970s) plus a few tin sheets. Most neighbouring houses and lodges: **corrugated galvanised iron** (about 88% of buildings today), unpainted, rusting, green, blue or red, often with stones holding the sheets | [S CGI share; E colours; E stones on tin] | Board-and-tin mix | Keep. Make neighbouring buildings mostly CGI | P2 |
| X10 | Frame colour | Vividly painted frames are documented. **Blue** matches the user's reference photos; no source gives blue a meaning | [S vivid; O hue] | Blue #2C65AD | Keep. **Never explain the blue as symbolic** in game text. Fade and chip the paint on the sill and lower door | P3 |
| X11 | Not Khumbu | The wide **black trapezoid window surround** is central-Tibetan and Ladakhi, not documented for Khumbu | [S absence] | none | Keep it out | P1 (guard) |
| X12 | Yard | A small yard in front of the door, of flat stones or beaten earth, for drying and sorting the harvest | [S] | Steps only | Add a 3-4 m deep yard in front of the +Y façade | P2 |
| X13 | Firewood | Winter firewood stacked in the yard and against the walls | [S] | Stack against the −Y wall | Move the main stack to the yard or an end wall; keep a small one by the door | P2 |
| X14 | Dung fuel | Yak-dung patties drying on the ground and stored in heaps (the main fuel above the treeline). Patties stuck on walls are only weakly sourced for Khumbu: use sparingly or not at all | [S ground and heaps; weak for walls] | none | Add one dung heap plus a patch of patties drying in the yard | P2 |
| X15 | Fields **(season)** | Small fields of barley and potato behind dry-stone walls, right up to the house. Oct/Nov: harvested, bare brown soil and stubble. If spring: tilled or being sown | [S fields; S crop cycle; E look] | Snow plinth | Add 2-3 walled plots at the downhill side | P2 |
| X16 | Prayer flags | A tall pole with a prayer banner (tarchen) near the house; printed flags faded by the monsoon | [S poles at houses; E fade] | none | Add one 4-6 m pole [E] | P2 |
| X17 | Solar | Dingboche has had solar lighting since at least 2014. A small panel (0.5-1 m²) on the roof or a pole, with a cable into the house | [S solar; E panel] | none | Add | P2 |
| X18 | Grid power | Whether the Amadablam mini hydro reached Dingboche by Nov 2026 is unconfirmed | [S plan; unknown status] | none | No dense power lines. At most one service cable | P3 |
| X19 | Village context | Dingboche has about 50 houses, mostly converted to guesthouses, so lodges should dominate the surroundings | [S, undated article] | — | Neighbouring buildings mostly 1-2-storey lodges with CGI roofs | P3 |
| X20 | Storeys (decision 0.5) | Optional: a byre ground floor 1.7-1.9 m clear, with the living floor above reached by an inside stair | [S Stonor; E height] | One storey | Only if the exterior must read as the classic family home | P3 |

### 3.2 Kitchen and living-room checklist

**Layout** (Stonor 1954; Sestini 1975) [S]:
- A broad low bench with a long low table runs **under the windows**.
- The **hearth sits near one end of that window wall**, with the owner's seat and the guest's seat of honour beside it.
- The **wall opposite the windows** carries long dark shelves of shining copper and brass water vessels: the family's
  wealth on display ("the number and size... showed wealth and social prestige", Stonor).

| # | Item | Target | Tag | Today (`build_chapter1_world.py`) | Action | Pri |
|---|---|---|---|---|---|---|
| K1 | Bench + low table | **Under the windows** on +Y. Bench seat 30-40 cm high, 60-80 cm deep; Tibetan pile rugs on it (khaden size, about 90 × 180 cm) | [S position; E size] | Two `painted_wooden_bench` on the −Y wall | Move the benches, rugs and cushions to +Y under the windows | P1 |
| K2 | Copper and brass wall | The **−Y wall** (opposite the windows): a 3-4 m run of long dark shelves. **Large wide-bellied copper water drums** (40-60 cm in diameter, 50-70 cm tall [E]) on the floor or bottom shelf, plus brass pots and ladles, dents and dark patina | [S; E sizes] | A brass shelf on −Y over the benches (right wall); dresser and painted shelves on +Y (window wall) | Keep the −Y shelves and extend them to 2-3 tiers; move the dresser and shelves off the window wall; **model 3 copper drums** (none in the downloads) | P1 |
| K3 | Hearth | Near one end of the window wall. A mud stove with hood and pipe is the right present-day choice; an open stone-slab hearth with no chimney fits an older household | [S] | Stove at (−0.95, +0.95) m, near +Y at the −X end | **Keep** the position. Put the owner's cushion on the bench end by the stove, the guest seat next to it | P2 |
| K4 | Soot | Heaviest above and behind the stove; ceiling boards and beams black near it; glossy black rafters | [S] | Soot gradient added on 2026-11-14 | Check by screenshot; push it darker within 1 m of the hood | P2 |
| K5 | Over the fire | Yak-meat strips on wooden slats hung from the beams over the stove (about 1.6-1.9 m up); chhurpi cheese cubes on strings; a kettle or pot always on the stove | [S meat, kettle; E photo-common cheese] | Pot and kettle on the stove | Add the slats with meat and 2-3 chhurpi strings | P2 |
| K6 | Butter-tea churn (dongmo) | Wood, 2-4 brass bands, about **82 cm tall × 13 cm in diameter**; plunger rod about 1.2× the barrel | [S: V&A O56609, https://api.vam.ac.uk/v2/object/O56609; E rod] | 0.84 m × about 0.17 m | Slim to 0.13 m | P2 |
| K7 | Shrine | On the back (−X) end wall, away from the door: a painted altar cabinet (chösham), a row of **seven small water bowls**, butter lamp(s), a thangka, family photos | [S lhakhang in every house; S cabinet; general Tibetan Buddhist practice for 7 bowls] | Painted cabinet, 7 `brass_goblet_03`, 2 `brass_diya_lantern`, Shakyamuni thangka, photos, khata | Replace the goblets with **small brass bowls** (7-9 cm across [E]); replace the **diya lanterns** (an Indian/Hindu lamp form [O]) with footed brass butter lamps (8-12 cm tall [E]) | P1 |
| K8 | Father's photo | A framed photo of the father **on the altar with a khata draped over the frame** | [S: The National 2025, a climber son's photo with a khata in a Lukla kitchen] | `Photo_Icefall` on the bench; khata high on the altar wall | Add the framed photo with a khata on the altar; the bench photo can stay as the story object | P2 |
| K9 | Low table | Long low table, 30-40 cm high, 80-150 cm long, 35-50 cm deep; plain planked and smoke-darkened, or Tibetan painted panels | [S museum dims: LACMA, Rubin; E style] | `chinese_tea_table` (a Chinese design) | Replace with a choktse-style table: model it, or search Poly Haven first | P2 |
| K10 | Modern items | Chinese thermos flasks (already standard in 1975), **pressure cooker**, steel and aluminium pots, a plastic jerrycan, an **LPG cylinder or kerosene can**, an **LED bulb or solar lantern** with a cable, a wall calendar, summit photos or certificates | [S thermos, pressure cooker, LPG/kerosene, solar; E the rest] | 2 thermoses, enamel pot, kerosene-style lantern | Add a pressure cooker, `propane_tank` (already downloaded, in the CAMP list), `plastic_jerrycan`, one LED bulb on a cable, a calendar | P2 |
| K11 | Expedition containers | 1-2 old expedition jerry-cans or metal boxes among the copper (a 1950s status habit; here, the father's) | [S Stonor] | Father's orange duffel | Add `metal_jerrycan` / `old_military_crate` on the copper shelf | P3 |
| K12 | Rugs | Scarlet, dark blue and saffron Tibetan pile rugs on the bench; coarser rugs on the floor | [S scarlet; E others] | `SM_TibetanRug` (dark blue, lotus, key border) | Keep; add one scarlet rug | P3 |
| K13 | Walls, floor | Clay plaster, ochre-brown, soot-black above the fire; carefully chosen floorboards with carpets | [S] | Present | Keep | — |
| K14 | Light | Warm low firelight, 1-2 LED bulbs or a solar lantern, a butter lamp at the shrine, cool daylight from small south windows | [S/E] | Point lights plus a window spot | Re-aim the window light after X1-X3 | P2 |
| K15 | Thangkas | Two public-domain 18th-century museum thangkas | — | Present | Keep. A modest family more often has newer painted or printed images [O]; add one printed image or poster | P3 |
| K16 | Seating | Bench and floor seating; no chairs in 1954. Plastic chairs today belong in lodges | [S 1954; E today] | — | No Western dining chairs in the family room | P3 |
| K17 | Butter-tea blender | Electric blenders are used for butter tea where there is power, but this is **not verified** for Khumbu homes | [S general; unverified locally] | — | Leave out | — |

### 3.3 Words on screen
Sources give Tibetan (*thab*, *mdong mo*, *choktse*) or Nepali (*chulo*, *agenu*, *gagri*) forms, not confirmed Sherpa
ones. **No on-screen Sherpa word ships without a Sherpa speaker's check** [S: `house_kitchen.md` §7].

### 3.4 After moving the door (dependency list)
1. `sherpa_house.py`: `L, W, T`, `DOOR` onto +Y, `WIN` as two windows, quoins, plaster. Rebuild, then run `Tools/Story/import_house.py`.
2. `build_chapter1_world.py`: `L_, W_, T_`; the door gap and steps in the collision shell move from `xf` (+X) to the +Y wall;
   two window gaps; `RY` rotated by about 90° so the +Y façade faces S-SE (not "door looks at the camp"); furniture per K1-K11.
3. Regenerate the places copy inside `Chapter1.ygs` after the world build (the 2026-11-14 gotcha: the kitchen shot landed on a hillside).
4. Update `room_start`, `room_out` and the door-related beats; update `story_shots.ps1` coordinates for `house`, `housedoor`, `houseback`.

---

## 4. Common mistakes to avoid

**People and dress**
1. Putting every Sherpa man in a chuba. Men have worn Western clothes for decades [S].
2. Dressing Sherpas as Tibetan nomads: sheepskin, fur trim, one sleeve hanging off, Kham/Amdo red hair tassels [S/O].
3. Dressing them as lowland Nepalis: saris, kurtas, Dhaka topi caps, nose rings [O].
4. A front apron on girls or unmarried women; a plain red apron instead of many-coloured stripes [S].
5. Festival jewellery (coral, turquoise, amber, dzi, gold headbands, gau) as everyday wear [S].
6. Spotless, matching, new kit on locals [S].
7. Porters as "Sherpas", or as either barefoot or fully kitted pros [S].
8. Clients in double or 8,000 m boots on the trek; Sherpas in yak-hide boots outside a festival [S].
9. No sunglasses at midday at 4,300 m [S].
10. A khata worn all day like a scarf [S].
11. Monk robes or ritual objects on lay villagers [O].
12. Real logos, including three stripes, swooshes, the Vibram octagon and brand-signature boot colours (La Sportiva yellow/black, Scarpa orange/black) [S].

**Faces and hair**
13. Long full beards on Sherpa men [MED].
14. Bald scalps under hats (the current state for five characters) [S current].
15. Curly fades, chinstraps or curly moustaches on Nepali highlanders; pale porters [MED].
16. An all-over pink flush instead of local, brick-toned cheek and nose redness (Sherpas have low haemoglobin, so no plethoric face) [S/MED].
17. Glossy, uniformly shiny skin; neon irises; paper-white sclera on outdoor workers; metallic lip "balm" [S/D].
18. Mascara-like lashes or make-up on trekkers [LOW].
19. The same face weathering on everyone. Clients at day 6 are *sunburnt*, not weathered; Marek (weeks in Nepal) is more weathered; frostnip and hollow cheeks belong to later chapters [S].
20. A uniformly scaled adult as the child [S].

**Boots and gear**
21. Lofted round "flower-pot" boots with no heel counter, toe spring, tongue or laces [S user report].
22. Branded scans shipped with their logos still on (CC BY does not license trademarks) [S].
23. Mirrored shoes with identical dirt on both feet [D].

**House**
24. A door on the end wall, openings on the back wall, or one tiny window [S].
25. Walls 0.4 m thick with flush windows (no reveal) [S].
26. Benches against the back wall and copper on the window wall: the layout inverted [S].
27. A museum room: no thermos, LPG, LED light, pressure cooker or plastic [S].
28. Tibetan black trapezoid window surrounds; flat roofs [S].
29. Symbolic explanations for the blue paint in game text [S: none exists].
30. Hindu objects (diya lamps, Ganesh images) in a Buddhist Sherpa shrine [O].
31. Rice drying in the yard. Sestini's general text mentions it, but rice does not grow at Khumbu altitudes; use barley and potatoes [O].

**Process**
32. Judging the look in the Blender workbench or the MetaHuman viewport only. Check in game, under UDS sun, overcast and house light, and at **Medium** quality too (skin LODs drop SSS below High) [S/D].
33. Unsourced Sherpa words on screen [S].

---

## 5. Verification loop and credits

**Each iteration, per character** (the user's brief: screenshot every character and correct it):
1. Head-and-shoulders close-ups in three lights: snow sun, overcast, warm house interior. One **3/4 back view with the hat on**, and one full-body shot with the boots in frame.
   - `story_shots.ps1` already has `nimaclose`, `auntclose`, `people`, `team`, `room`, `house`, `housedoor`, `houseback`.
   - **Add** `marekclose`, `elenaclose`, `jiwooclose`, `porterclose`, `dolmaclose`, plus a back view and a feet view for each.
2. Run the checklist in `faces_realism.md` §7 (face darker than neck, brick cheeks, matte skin, hair under the hat, sparse
   facial hair, eyes, lips, hands, peach fuzz), then the character's *Must be true* / *Must not appear* boxes above.
3. Compare side by side with the dated reference photos:
   - Pangboche woman 2009: https://commons.wikimedia.org/wiki/File:Pangboche,_Sherpa_senior_woman,_Nepal.jpg
   - Phurte woman 2009: https://commons.wikimedia.org/wiki/File:Bhote_Koshi_Valley,_Phurte_village,_Woman,_Nepal.jpg
   - Pheriche porter 2022: https://commons.wikimedia.org/wiki/File:Carrying_Culture_-_The_Strength_of_a_Nepali_Porter,_Pheriche,_Nepal.jpg
   - Lobuche Sherpa guide 2007: https://commons.wikimedia.org/wiki/File:Lobuche-24-Sherpa_Lodge-Guide-2007-gje.jpg
   - Namche homestay kitchen 2009: https://commons.wikimedia.org/wiki/File:Namche_Bazaar,_Homestay,_Nepal.jpg
   - Wellcome Khumbu 1986 set (CC BY 4.0): https://wellcomecollection.org/works/ctgufgad
4. Measure what can be measured (boot length vs foot, clearance, albedo values, triangle counts). Leave the final visual call to the user.

**Credits register** (one line per CC BY asset, in the game credits):
`"<Title>" by <Author> (<source URL>), licensed CC BY 4.0 (https://creativecommons.org/licenses/by/4.0/). Modified: retopologised, logos removed, retextured.`
GSO author = "Google LLC (Google Scanned Objects)". Objaverse files: credit the Sketchfab author, and cite Objaverse
(ODC-By). CC0 assets (Poly Haven, the Wojtyla shoes) need no credit, but list them anyway [S].

---

## 6. Open questions (for a Khumbu Sherpa consultant before the title demo)
- Does a widowed Sherpa woman keep the front pangden? (no source)
- What is on the back apron (gyabtil) of an everyday dress: stripes, plain or brocade? (sources only say "striped aprons")
- Typical honju blouse colours for a woman of about 50 today.
- Do older Khumbu women braid coloured yarn into their hair? (Tibetan practice is documented; Sherpa practice is not)
- Is there a school in Dingboche, and where would Dolma study? (unverified)
- Sherpa words for stove, churn, low table and shrine.
- Season (decision 0.1).
