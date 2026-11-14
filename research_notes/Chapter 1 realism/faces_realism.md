# Faces: realism notes for Chapter 1 (Khumbu, Dingboche ~4,300 m, late Oct/Nov)

Written 2026-11-14 (overnight research pass). Covers what faces, hair, hands and eyes look like on people who
live or work at 3,500-5,000 m in the Khumbu, and how to get that look out of the **in-engine MetaHuman Creator
of the UE 5.8 install this project uses** (`E:\UE5.8\UE_5.8`).

How much to trust each point:
- **[HIGH]**: peer-reviewed paper, Epic documentation, or read directly from the UE 5.8 engine source/data.
- **[MED]**: credible secondary source (medical society guidance, outfitter kit lists, consistent reporting).
- **[LOW]**: my inference, or a single informal source. Check these against reference photos before using them.

Starting values given for sliders are **untested first guesses**, not measured. Every one has to be checked
by screenshot under in-game light (UDS sky, -10 C clear day, plus the warm house interior).

---

## 0. Biggest realism problems in the current cast (read this first)

Taken from `Content/Story/Cast.ygs`, the groom folders under `Content/Story/MetaHumans/<Name>/<Name>/Grooms`,
and the preset sheet `Saved/Compare/faces/_sheet.png`:

| Character | Now | Problem | Fix |
|---|---|---|---|
| **Nima** (older Sherpa sirdar) | `Beard_L_Full` + `Mustache_L_Full`, hat = true, so no head hair | A long full beard is unusual for Sherpa or Tibetan men. Facial hair in East Asian-ancestry men is on average much sparser (see 2.4). That beard is the most "wrong-looking" detail in the cast. | `Mustache_S_Stubble` or `Mustache_S_Uneven` plus `Beard_S_Stubble`/`Beard_S_Uneven`, or nothing. Keep it thin and patchy at the chin, with some grey (Whiteness). |
| **Nima, Pema, Marek, Ji-woo, porter** | `hat: true` means DressAs drops the Hair groom | A beanie on a bald scalp shows **no hair at the nape, temples or sideburns**. That reads as CG or "bald man in a hat". Real people always show hair under the hat edge. | Keep a **short** groom under hats: `Hair_S_BuzzCut`, `Hair_S_Clean`, `Hair_S_Casual`, `Hair_S_BrushCut`. For women, use `Hair_S_LowPonytail` or `Hair_S_PulledBack` so the hair comes out below the hat (section 4.6). |
| **Pema** (Sherpa woman ~50) | hat = true, no hair. `Hair_M_BobCurly` and `Hair_S_Pixie` are sitting in her folder. | Older Sherpa women overwhelmingly keep **long** hair, tied back or braided ([LOW-MED], 2.3). A curly bob or pixie would be wrong. | `Hair_S_PulledBack` / `Hair_S_LowPonytail` now. Later, a custom long single or double braid groom (no built-in one exists). |
| **Porter A** (preset Kelvin) | `Hair_S_CurlyFade`, `Goatee_S_ChinStrap`, `Mustache_S_Curly`. Pale skin on the sheet. | Nepali highland porters almost all have straight black hair. A curly fade, a chinstrap and a curly moustache read as a Western fashion look. The face is far too pale for someone who carries loads in the sun every day. | Straight short groom (`Hair_S_Messy`, `Hair_S_Casual`, `Hair_S_BuzzCut`) with Melanin ~0.9. At most stubble. Skin U much darker, cheek and nose accents up (section 6). |
| **Elena** (Spanish client) | `Eyelashes_L_ThickCurl` | Reads as mascara or false lashes. Wrong for a trekker on day ~6. | `Eyelashes_S_Fine` or `Eyelashes_L_SlightCurl`. |
| **Marek** (Slovak leader) | `Beard_M_MuttonChops` + `Mustache_S_Horseshoe` | A stylised "biker" look ([LOW]: a style judgement, not a fact). A leader weeks into a season more plausibly has a short, untidy full beard. | `Beard_S_Full` or `Beard_M_Stubble` + `Mustache_S_Full`, with a little Whiteness at the chin. |
| **Dolma** (girl, 8) | Not built yet | MetaHuman Creator only ships adult body types. Epic has no child support (2.7, 4.8). | Third-party child MetaHumans, or a custom approach. Decide before any work starts. |
| **Everyone** | No peach fuzz slot chosen | Without vellus hair, skin in backlight or rim light looks like plastic. | Add `Peachfuzz_M_Thin` (the only built-in one) to every character. |

Changing **Face Texture Index** (wrinkles, stubble, marks) on these characters needs the rig removed first,
because they are already auto-rigged ([HIGH], section 4.1). Skin tone, accents, freckles, roughness, makeup,
eyes and grooms can all be changed on a rigged character without re-rigging.

---

## 1. The environment that shapes these faces

- **UV rises with altitude.** WHO's *Global Solar UV Index: A Practical Guide* (2002) gives UV radiation
  +10-12 % per 1,000 m [HIGH]. The UIAA Medical Commission gives about +4 % per 300 m [HIGH]. Dingboche at
  ~4,300 m therefore gets roughly 40-50 % more UV than sea level at the same sun angle.
  - WHO guide: https://www.who.int/publications/i/item/9241590076 (archived PDF:
    https://webarchive.library.unt.edu/web/20130216222323/http://www.who.int/uv/publications/en/GlobalUVI.pdf)
  - UIAA MedCom Standard No. 20, *Eye Problems on Expeditions* (2020 v2.4):
    https://www.theuiaa.org/documents/mountainmedicine/Engl_UIAA_MedCom_Rec_No_20_Eye_problems_2020_V2-4.pdf
- **Snow reflects UV back up.** UIAA says snow reflects 80 % of UV [HIGH]. Singh 2017 says "up to 90 %",
  against 9-17 % for grass [HIGH]. Reflected light **burns places that are normally shaded: under the chin,
  inside the nostrils, inside the ears** (Singh 2017) [HIGH]. This is the one sunburn pattern that most
  separates "mountain" from "beach".
- **Expedition UV doses are huge.** A 2007 north-side Everest study (15 expeditioners, 10 Sherpas) measured
  peak doses above **100 minimal erythema doses**. Melanin-haemoglobin in exposed skin rose by a median
  +83.6 % in expeditioners and +24.7 % in Sherpas. The study also recorded dehydration and subcutaneous lipid
  loss. The Sherpas' **unexposed** skin got lighter over the trip [MED: I read the abstract through a mirror,
  so check the numbers against the journal].
  Wilderness & Environmental Medicine 2013, doi:10.1016/j.wem.2013.05.007. Registry: https://clinicaltrials.gov/study/NCT00685438
- **Cold, dry and windy.** Humidity at altitude is 14-20 %, which dries skin (xerosis), scales the scalp
  and erodes the lips (actinic cheilitis) [HIGH, Singh 2017].
- **Smoke indoors.** Traditional Khumbu homes burn wood or dung on open fires or stoves. Indoor CO was more
  than 50 % higher in private homes than in lodges, and **having no chimney** was the main factor (Duo et
  al. 2016, four Sherpa villages, 2,500-3,900 m) [HIGH].
  https://sfera.unife.it/handle/11392/2356498 . Smoke is also a listed irritant for pterygium (Maharjan
  2014). Expect red-rimmed or watery eyes on older women who cook over a fire [LOW-MED: inference].

Main review used throughout: **Singh GK. "High Altitude Dermatology." Indian J Dermatol 2017;62(1):59-65**,
doi:10.4103/0019-5154.198050. https://pmc.ncbi.nlm.nih.gov/articles/PMC5286755

---

## 2. Sherpa residents (Nima, Pema, Dolma, porters)

### 2.1 Skin tone and the tan pattern
- Sherpas are of Tibetan origin. Nepali dermatology clinics see mostly **Fitzpatrick IV-V** skin: in one
  Kathmandu series 60.4 % were type V, in another 40.5 % were type IV [HIGH for the clinics, LOW as a figure
  for Sherpas specifically: those are mixed-ethnicity Kathmandu patients].
  https://nepjol.info/index.php/JKMC/article/download/53551/40357/159726 ·
  https://nepjol.info/index.php/NJDVL/article/view/45068/36309
- **Where they tan.** Singh 2017 lists the face, the "V" of the neck and the backs of the hands [HIGH].
  Highland farmers and herders build up **pigmentation, skin thickening and deep furrows**, most of all on
  the face and the back of the neck ("cutis rhomboidalis nuchae") [HIGH]. Covered skin (chest, upper arms,
  forehead under a hat) stays clearly lighter, as the Sunnybrook study saw in Sherpas' unexposed skin.
- For MetaHuman this means the **face should be darker and warmer than the neck and body skin**. MetaHuman
  uses one tone for face and body, so you need either an accent or texture trick (section 5) or clothing
  that hides the joins (high collars, scarves, chuba).

### 2.2 Ruddy cheeks: "Plateau Red Face"
- In long-term highland residents the cheeks go a distinct **ruddy colour from telangiectasia**, small
  dilated vessels in a fine mesh. It can spread to the **bridge of the nose, around the mouth, the ear edges
  and the conjunctivae** [HIGH]. Prevalence was **32.9 % at 4,300 m (Maduo, mostly Tibetan nomads)** against
  15.7 % at 2,260 m. In Maduo those with it were *younger* (mean 34 vs 44 years). The authors suspect hypoxia,
  cold, wind and UV, but the cause is unknown.
  Ma L et al., *High Alt Med Biol* 2015;16(2):147-153. https://pmc.ncbi.nlm.nih.gov/articles/PMC4490706
- Singh 2017: hyperpigmentation and telangiectasia **start in childhood** and peak around the 3rd decade.
  In the 5th-6th decades thickening and furrowing take over [HIGH]. So **Dolma (8) and a young porter should
  have redder cheeks than Nima. Nima has more furrows and brown pigment and less bright red.**
- Caveat [MED]: Sherpas and Tibetans have **lower haemoglobin** than Andean highlanders (EPAS1/EGLN1
  adaptation), so polycythaemic "plethoric" purple-red faces are less typical for them. Keep the red
  **local to the cheeks and nose and brick-toned**, not an all-over flush.
  https://www.ncbi.nlm.nih.gov/pmc/articles/PMC5241213/
- Not every highlander has it (about a third at 4,300 m). Give it to some characters and not others.

### 2.3 Hair
- **Colour:** straight, black to very dark brown [MED, general descriptions]. Grey arrives as salt and
  pepper. Long sun exposure can bleach the ends reddish-brown [LOW: general hair photochemistry, not sourced
  for Sherpas].
- **Men today:** mostly short, practical cuts, often under a beanie or cap [LOW: from contemporary press
  photos of guides, no study]. Historically men wore a **long pigtail, often wound round the head**: a 1953
  NZ newspaper describes Sherpas wearing their hair long in a pigtail wound around the head [MED, period
  press]. https://paperspast.natlib.govt.nz/newspapers/CHP19530718.2.20 . **Nima's father's generation**
  (flashbacks, photos on the wall) could wear the pigtail. Nima himself would have short hair.
- **Women:** long hair kept up, tied back or braided. Sherpa women braid each other's hair with **coloured
  threads worked into long braids** (wedding photo essay) [LOW-MED, one informal source].
  https://medium.com/variantbio/nepalese-sherpa-wedding-a-photoblog-261871d25fef . Tibetan sources describe
  red string at the braid root, one plait for unmarried girls and two for married women [LOW: this is Tibet,
  not documented for Sherpas]. https://www.tibettravel.org/tibetan-people/tibetan-womens-headdress.html .
  Festive Khumjung dress includes a **gold headband** with coral, turquoise and zi beads (SOAS archive
  caption) [MED]. https://library.soas.ac.uk/Record/PP_MS_19.08.01.SHER.178
- **Headwear:** wool or acrylic beanies, expedition-gift fleece and caps are everywhere. Some men wear
  felt or cowboy-style hats [MED, factsanddetails citing NatGeo 1992].
  https://factsanddetails.com/south-asia/Nepal/Ethnic_Groups_and_Minorities_Nepal/entry-7840.html .
  The hair under a hat is **compressed and flattened, with a crease line** when the hat comes off.

### 2.4 Facial hair
- Comparative studies find East Asian men much less hairy than Europeans: chest hair scored 0.8 vs 3.0 on a
  0-4 scale (Chinese vs Caucasian) [HIGH for that comparison]. In a second study body hairiness was
  significantly lower in East Asians [HIGH]. Tibeto-Burman men are not measured directly, so treating
  Sherpas the same way is an inference [MED].
  https://pure.psu.edu/en/publications/clinical-and-biochemical-parameters-of-androgen-action-in-normal- ·
  https://pmc.ncbi.nlm.nih.gov/articles/PMC6587453
- In practice: most Sherpa men are **clean-shaven or have a thin moustache and a few chin hairs**. Older men
  may have a wispy moustache or chin tuft [LOW-MED, photo observation]. **Avoid full, dense beards for
  Sherpa characters.**

### 2.5 Hands
- Backs of the hands are tanned like the face (Singh 2017) [HIGH]. Xerosis and chapping from dry cold air
  [HIGH]. Koilonychia (spoon nails) is reported in Ladakhis at 3,800 m, and Beau's lines after altitude
  exposure [HIGH, minor detail].
- Working hands (porters, field and household work for women): thick skin, darker knuckles, **dirt in nail
  folds and creases, short broken nails**, cracked fingertips [LOW-MED, inference from occupation plus
  xerosis].
- In the editor: Skin > Hands & Feet has `PalmLightness`, `PalmTint`, `PalmCavityDarkness` (0-0.5) and
  fingernail tint, roughness and metallic [HIGH, engine source]. Push **PalmCavityDarkness toward 0.3-0.5**
  for grime in the palm creases. Make fingernails rougher (0.6-0.8) and slightly yellow-grey.

### 2.6 Eyes
- **Iris:** dark brown, almost universal [MED].
- **UV damage to the eye is common:** cataract is the main cause of preventable blindness in mountain Nepal.
  The Ridley Eye Foundation estimates cataract at 75-80 % of it, with 15-20 % of camp visitors having
  cataracts [MED, charity data]. https://ridleyeyefoundation.org/ref-nepal
- **Pterygium** is a triangular fibrovascular growth on the white of the eye, usually on the nasal side. It
  affects **10.1 %** of adults in Upper Mustang (n = 1,319), rises linearly with age (peaks at 66-75) and is
  higher in farmers and construction workers. UV is the main cause; wind, dust and smoke contribute [HIGH].
  Maharjan IM et al., *Nepal J Ophthalmol* 2014;6(11):65-70.
  https://nepjol.info/index.php/NEPJOPH/article/view/10774/8749 . (A web summary quoting "39.6 %" did not
  match this paper's text, so ignore that figure.) **For Nima (older sirdar), a slightly yellowed, more
  veined sclera fits; a small nasal pterygium in one eye would be authentic.**
- **Porters:** about half of 1,084 porters at a 2,860 m eye camp had some eye condition (refractive error,
  dry eye, infection, glaucoma, cataract) [HIGH]. https://nepjol.info/index.php/NEPJOPH/article/view/17529
- **Sunglasses habits:** UIAA stresses porters are as exposed as trekkers and urges leaders to give them
  sunglasses, which implies many go without [HIGH]. Snow blindness shows as **red, gritty, watering eyes
  with swollen lids, squinting, and conjunctiva red where it was exposed**, appearing hours after exposure
  [HIGH, UIAA]. Sherpa guides on the mountain wear glacier glasses or goggles. In the village, older people
  often wear cheap dark glasses or none [LOW].

### 2.7 Children (Dolma)
- Redness and pigmentation start in the first decade (Singh 2017) [HIGH], so: **ruddy, wind-chapped
  cheeks**, chapped lips, a slightly runny nose, tanned face, a dirty-fingered child's hands [LOW for the
  last three: common in photos, not a study].
- MetaHuman Creator has no official child body or face. The default body system is built on adult
  proportions; child MetaHumans on Fab use modified plugins or custom meshes [MED].
  https://forums.unrealengine.com/t/athian-games-metahuman-child-characters/2701955 ·
  https://dev.epicgames.com/documentation/metahuman/metahuman-creator-body-params-tool-in-unreal-engine

### 2.8 Crow's feet and squint lines
- Years of squinting into snow glare plus UV breakdown of collagen deepen lateral canthal lines (crow's feet)
  and the frown lines between the brows [MED: dermatology consensus as summarised by AAD-referencing sources].
  In very sun-exposed faces the folds of the crow's feet can be **paler than the surrounding tan**, because
  the skin inside the fold was not exposed while squinting [LOW: a commonly seen "squint-line" look, not
  sourced].
- MetaHuman: choose a Face Texture Index with **High** wrinkles for Nima and **Medium** for Pema (4.1). To get
  pale crease lines you need a texture override (section 5).

---

## 3. Clients and leader (Marek, Elena, Ji-woo)

**When.** Dingboche is reached on day 5-6 of a standard Lukla start, with an acclimatisation day there
(Lukla -> Phakding -> Namche x2 -> Tengboche -> Dingboche x2) [MED, outfitter itineraries:
https://www.intrepidtravel.com/us/nepal/everest-base-camp-trek-144944 , https://www.kimkim.com/c/everest-base-camp-trek].
So the clients have had **about one week** of high sun. They look new, not haggard. Marek, the leader, may
have been in Nepal for weeks, so he looks more weathered.

What to show:
- **Early sunburn:** pink-red **nose tip and bridge, upper cheeks, lower lip, ears, and under the chin**
  (snow albedo) [HIGH for sites, Singh 2017/UIAA]. Peeling on the nose tip after day 4-5 [LOW-MED].
- **Glasses tan line:** a lighter band around the eyes where sunglasses sit, with sharper lines at the
  temples from the arms. It builds over weeks. Faint on Elena and Ji-woo at day 6, clear on Marek [LOW-MED].
- **Lips:** dry, cracked, paler and less saturated, sometimes **cold sores**. In skiers with a history of
  sun-triggered herpes labialis, 12 % reactivated within a week at altitude, median 3.5 days after exposure
  [HIGH, ski study; reference via https://scilit.net/publications/703fb0ba228b10c8997ca8420340fba8].
  Lip balm gives a **glossy lower lip** in the specular [LOW].
- **Sunscreen:** a slight sheen on the cheeks and nose. Mineral (zinc) sunscreen leaves a **whitish cast**
  on the nose and cheekbones [LOW: common, no study].
- **Kit on the face:** Everest kit lists ask for a Buff or bandana for the neck and face, glacier glasses
  with side shields (two pairs), SPF lip balm and sunscreen [MED].
  https://MADISONMOUNTAINEERING.COM/wp-content/uploads/2019/08/EVR-2023-Mount-Everest-Expedition-gear-list.pdf .
  So: Buff round the neck or pulled up to the nose, glasses pushed onto the hat or hanging on a cord.
- **Cold and cough:** the Khumbu cough is common (22-42 % of trekkers in Everest-region surveys) [MED].
  Expect a **red, chapped nostril rim and upper lip** from wiping [LOW].
- **Hair:** unwashed (lodge showers are cold or extra cost), flattened by the hat, women's hair tied back in
  a ponytail or braid [LOW]. Men get **1-week stubble** unless they shave [LOW].
- **Later in the climb, not Chapter 1:** weight loss of ~7 % body mass over an Everest expedition
  (Caudwell Xtreme Everest: -7.3 ± 4.9 kg at 8 weeks) gives hollow cheeks [HIGH]. Superficial frostbite
  or frostnip shows as **waxy white or grey patches on cheeks and nose, then red, blisters and peeling**
  [HIGH, Mayo Clinic: https://www.mayoclinic.org/health/frostbite/DS01164]. Keep these for the higher
  chapters as visible progression.
- **Ji-woo (Korean):** Korean outdoor gear does include full-coverage UV masks and visors (retail
  listings) [LOW]. Don't lean on it as a stereotype. A Buff or sun mask that she pulls down for the camera
  is a believable personal touch.
- **Elena (Spanish):** Mediterranean skin is often Fitzpatrick III, tans rather than burns, and dark hair
  and brown eyes are most common [LOW, general]. Her preset (Vivian) has **green eyes and very pale skin**.
  Skin U is already raised to 0.36. Consider brown or hazel irises.
- **Marek (Slovak):** Central European, likely Fitzpatrick II-III, so he **burns** first. Red nose, cheeks
  and neck, a deep goggle tan, crow's feet with paler creases, a weathered short beard [LOW].

---

## 4. MetaHuman Creator (UE 5.8): the controls that matter

Sources: Epic docs (linked per section) plus the 5.8 engine headers I read for this note:
`Engine/Plugins/MetaHuman/MetaHumanCharacter/Source/MetaHumanCharacter/Public/MetaHumanCharacterSkin.h`,
`...Makeup.h`, `...Eyes.h`, `MetaHumanDefaultPipeline/Public/Item/MetaHumanDefaultGroomPipeline.h`,
and `MetaHumanCharacterEditor/Private/Tools/MetaHumanCharacterEditorSkinTool.cpp`.

All of these can be scripted from `Tools/Story/mh_cast`. The character holds `skin_settings`,
`eyes_settings` and `makeup_settings`. Commit them with the editor subsystem's `commit_skin_settings`,
`commit_eyes_settings` and `commit_makeup_settings` (all `BlueprintCallable` in 5.8) [HIGH, engine source].
mh_cast already does this for skin U.

### 4.1 Skin (Materials > Skin) [HIGH]
Docs: https://dev.epicgames.com/documentation/metahuman/skin-material-tools
- **Skin Tone U/V** (0-1). The picker is a generated skin-tone chart. In this project U was used as
  light (0) to dark (1); V is the second axis of the chart (undertone). Check visually; the axes aren't
  documented. **Safe on rigged characters.**
- **Face Texture Index** picks the synthesised face texture (wrinkles, stubble shadow, marks) from **153**
  textures (`TS-1.3-F ... nchr-153`). **Body Texture Index** has 0-8.
  - **Use Texture Index Filters** with attributes **Face Wrinkles** (Low/Medium/High), **Face Stubble**
    (None/Low/Medium/High) and **Face Marks** (Low/Medium/High), then **Face Filter Index**.
  - **RIG-RESTRICTED:** Face/Body Texture Index, the filter controls and *Texture Position Offset* can only
    be edited while the character is **Unrigged** (`CanEditRigRestrictedProperties()` returns
    `RigState == Unrigged`). Every cast member is already auto-rigged. Changing the wrinkle texture means
    removing the rig, changing it, and re-requesting the auto-rig (one cloud job at a time, 161-590 s each,
    needs Epic sign-in).
  - From `Content/Optional/TextureSynthesis/TS-1.3-F_UE_res-1024_nchr-153/texture_attributes.json` (153
    entries; I'm **assuming entry i = Face Texture Index i**, so check one in the editor):
    - Wrinkles **High**, stubble None: 20, 47, 48, 50, 52, 55, 69, 70, 90, 109, 120, 121, 127, 151 (Pema candidates)
    - Wrinkles High, stubble Low: 11, 19, 51, 53, 54, 71, 140 (**Nima candidates**: old, with a thin beard shadow)
    - Wrinkles High, stubble Med/High: 4, 21
    - Wrinkles Medium, stubble None: 10, 14, 22, 24, 40, 42, 62, 68, 73, 76, 100, 138, 141
    - Wrinkles Medium, stubble Low: 3, 115, 143 (Marek candidates)
    - Wrinkles Low, stubble Med/High: 15, 23, 27, 28, 30, 31, 32, 57, 65, 72, 77, 79, 98, 104, 108, 110, 111,
      117, 124, 133, 134, 142, 148 (young porter or male client with stubble)
  - Prefer the **Low or None stubble** textures for Sherpa men. A heavy blue-black beard shadow fights the
    sparse-facial-hair look.
- **Roughness** is a **multiplier clamped to 0.85-1.15, default 1.06**. Higher is more matte. Dry, wind-burnt
  highland skin should go **1.10-1.15**; sweaty clients after a climb 1.00-1.05. It only scales the baked
  roughness map, so the T-zone stays shinier relative to the rest, which is correct.
- **Hands & Feet:** PalmLightness (default 0.75; 5.8 changed the look, so 0 restores 5.7), PalmTint,
  PalmCavityDarkness (0-0.5), and fingernail and toenail tint, intensity, roughness and metallic.
- **Texture Sources** (2k/4k/8k per group) and **Texture Override**: supply your own Face base colour,
  normal, cavity and animated maps, or Body base colour, normal and cavity. 5.8 also adds **Material
  Overrides** (`TextureMaterialOverrides.bEnableMaterialOverrides`, per-slot face, body, teeth and eye MICs)
  with **"Inherit UI Parameters and Source Textures"**, which applies the Skin, Eyes, Makeup and Teeth UI
  params on top of your own material [HIGH, engine header]. That is the cleanest route to dirt and hat-line
  tans (section 5).

### 4.2 Freckles [HIGH]
- Off until you choose a **Mask** (`Type1/Type2/Type3`). Then **Density** (count, not area), **Strength**
  (opacity, default 0.2), **Saturation** (0.6), **Tone Shift** (0.65).
- Use: low-density, low-saturation masks as **solar lentigines and sun spots** on Nima and Pema
  (Density 0.3-0.4, Strength 0.15-0.25, Saturation 0.3-0.4). Darker, browner spots read as sun damage rather
  than "cute freckles" [LOW: starting values]. Melasma, the patchy brown malar pigment common in sun-exposed
  Nepali skin (malar pattern predominant in one Nepali male series [MED]), **can't be made with freckles**.
  It needs a texture override.

### 4.3 Accents: regional redness [HIGH for controls]
- **8 regions:** Scalp, Forehead, Nose, UnderEye, Cheeks, Lips, Chin, Ears. Each has **Redness, Saturation,
  Lightness** (0-1, default 0.5). Always symmetric.
  Docs: https://dev.epicgames.com/documentation/metahuman/skin-material-tools ("blush, sunburn, dark circles")
- This is the main tool for **Plateau Red Face, windburn and client sunburn**. Starting points [LOW]:
  - *Highland ruddy cheeks* (Dolma, porter, some Sherpa adults): Cheeks R 0.68-0.78, S 0.55-0.62, L 0.44-0.47.
    Nose R 0.60-0.65. Ears R 0.60 (ear edges are listed in PRF).
  - *Older weathered* (Nima): Cheeks R 0.60, S 0.48, L 0.43 (browner, less pink). Nose R 0.58, L 0.45.
    Forehead L 0.46.
  - *Client sunburn at ~day 6* (Elena, Ji-woo): Nose R 0.68-0.72, S 0.58. Cheeks R 0.60. Ears R 0.62.
    Chin R 0.56 (snow albedo). Lips S 0.42, L 0.54 (dry, pale).
  - *Marek (fair, burns)*: Nose R 0.75, Cheeks R 0.68, Ears 0.66, Chin 0.60, Forehead 0.56.
  - *Tired at altitude*: UnderEye L 0.44-0.46, R 0.52 (dark circles from poor sleep at altitude) [LOW].
- Keep changes under about ±0.25 from default. Big values go blotchy and "makeup-y".

### 4.4 Makeup, used as a weathering tool [HIGH for controls]
Docs: https://dev.epicgames.com/documentation/metahuman/makeup-material-tools
- **Foundation** (`bApplyFoundation`, Color, Intensity 0.5, **Roughness 0.65**, **Concealer 0.57**): covers
  the cheeks, with concealer under the eyes. Little use for weathering, and it makes skin look cleaner.
  **Leave it off.** One exception: a very low-intensity, slightly **darker, warmer** foundation colour can
  deepen the facial tan relative to the body. Foundation only covers the cheek and face zone, so it partly
  fakes a "face darker than neck" tan [LOW: test whether the coverage mask reaches forehead and nose].
- **Blush** (types `Angled, Apple, LowSweep, HighCurve`; Color, Intensity 0.4, Roughness 0.6): `Apple` or
  `HighCurve` with a **brick red** (not pink) colour at **Intensity 0.15-0.3** and Roughness 0.8-0.9 (matte)
  adds windburn on top of the Accents. Never glossy [LOW].
- **Lips** (`Natural, Hollywood, Cupid`; Color, Roughness 0.25, Opacity 0.7, **Metalness 1.0 by default**):
  for **lip balm** use `Natural` with a colour close to the lip colour, low opacity and Roughness 0.2-0.35,
  and **set Metalness to 0**, because the default of 1.0 would look like metallic lipstick [HIGH that the
  default is 1.0; LOW that it reads well]. For chapped lips, leave lips makeup off and rely on Accents (Lips
  S down, L up) plus a rough texture.
- **Eyes makeup** (types incl. `ThinLiner, SoftSmokey, PandaSmudge`...): keep **None** for everyone. The
  exception is a very faint `SoftSmokey` in a grey-brown at low opacity to fake **smoke-reddened, tired
  lids** on Pema [LOW].

### 4.5 Eyes (Materials > Eyes) [HIGH for controls]
Docs: https://dev.epicgames.com/documentation/metahuman/eye-material-tools
- Iris: `Pattern` (Iris001-009), PrimaryColorU/V, SecondaryColorU/V, ColorBlend, BlendMethod
  (Radial/Structural), ShadowDetails, **LimbalRingSize 0.6-0.85**, LimbalRingSoftness 0.02-0.15,
  LimbalRingColor, **GlobalSaturation 0-4 (default 2.0)**, GlobalTint.
- Sclera: **`bUseCustomTint`**. When off, the sclera tint is **derived from skin tone**. Also Tint,
  TransmissionSpread, TransmissionColor, **VascularityIntensity 0-2 (default 1.0)**, **VascularityCoverage
  0.1-0.4 (default 0.2)**, Rotation. Cornea: Size 0.145-0.185, LimbusSoftness, LimbusColor. Pupil:
  Dilation 0.85-1.2, Feather.
- Use:
  - Sherpa characters: dark brown iris, **GlobalSaturation down to ~1.2-1.5** (default 2.0 can look
    "contact-lens" vivid), a darker limbal ring.
  - Nima and older porters: `bUseCustomTint` on, a slightly **warm off-white/yellowish** sclera,
    VascularityIntensity 1.3-1.6, Coverage 0.25-0.3 (UV and smoke-exposed eyes) [LOW].
  - Pema: Vascularity a little up (smoke) [LOW].
  - Clients: default, or Vascularity 1.2 for altitude dryness. Snow-blind NPC (later chapter): Intensity
    near 2.0, Coverage 0.4, with swollen and red lids from the UnderEye accent.
  - Pupil Dilation small (0.9) in bright snow scenes, larger indoors. The runtime eye MIs (`MI_EyeL_Baked`,
    `MI_EyeR_Baked`) can be driven at runtime [HIGH, Epic materials doc:
    https://dev.epicgames.com/documentation/metahuman/metahuman-materials-and-textures].

### 4.6 Grooms (Hair & Clothing) [HIGH for the list and params]
Docs: https://dev.epicgames.com/documentation/metahuman/hair-and-clothing-tools ·
https://dev.epicgames.com/documentation/en-us/metahuman/hair-overrides
- Wardrobe slots: Hair, Eyebrows, Eyelashes, Mustache, Beard, **Peachfuzz**. Overrides are per costume and
  don't change the original asset.
- **Built-in grooms in this 5.8 install** (`MetaHumanCharacter/Content/Optional/Grooms/GroomAssets`):
  - Hair: `Hair_S_BuzzCut, Hair_S_Clean, Hair_S_Casual, Hair_S_BrushCut, Hair_S_Messy, Hair_S_RecedeMessy,
    Hair_S_HairLoss, Hair_S_BaldingStubble, Hair_S_SlickBack, Hair_S_SideSweptFringe, Hair_S_PulledBack,
    Hair_S_LowPonytail, Hair_S_Updo, Hair_S_UpdoBraids, Hair_S_UpdoBuns, Hair_M_TwistedBraids, Hair_L_Straight,
    Hair_M_BobStraight, Hair_M_Layered` and others (curly, afro, cornrow and mohawk styles that don't suit
    this cast).
  - Mustache: `Mustache_S_Stubble, Mustache_M_Stubble, Mustache_L_Stubble, Mustache_S_PencilThin,
    Mustache_S_Uneven, Mustache_S_Full`...
  - Beard: `Beard_S_Stubble, Beard_M_Stubble, Beard_S_Uneven, Beard_S_Full, Beard_S_PencilThin`...
    Goatees: `Goatee_L_Scraggly` (a thin wispy chin tuft can suit an old Sherpa man if Density is low).
  - Eyelashes: `Eyelashes_S_Sparse, Eyelashes_S_Fine, Eyelashes_S_Thin, Eyelashes_L_SlightCurl` (natural);
    `L_Curl, L_ThickCurl` (makeup look).
  - Peachfuzz: `Peachfuzz_M_Thin` (only one).
  - **No long single or double rear braid** exists. For Pema's braid you'd need a custom groom (Blender
    hair curves export as an Alembic groom, or a Fab braid groom) [MED].
- **Hair colour params** (`UMetaHumanDefaultGroomPipelineMaterialParameters`): **Melanin** (0 white to 1
  black, default 0.16), **Redness**, **Roughness** (0.25), **Whiteness** (share of grey hairs),
  **Lightness** (brightness of the greys), DyeColor, plus Ombre (tips), Highlights and Regions.
  - Sherpa hair: Melanin 0.85-0.95, Redness 0.05-0.12, Roughness 0.35-0.45 (dry, unwashed hair is less
    glossy).
  - Nima: Whiteness 0.3-0.45, Lightness 0.6.
  - Pema: Whiteness 0.15-0.25.
  - Optional sun-bleached ends: Ombre Melanin ~0.6, Redness 0.25, low Intensity [LOW].
  - Eyelashes have their own Melanin, Redness, SaltAndPepper and Lightness (defaults 0.3, 0.28, 0.20). Raise
    Melanin to ~0.8 for Sherpa characters, or lashes look light brown [LOW].
- **Hair under hats:**
  - Don't remove the Hair groom for hat wearers. Use a **short** groom so hair shows at the nape, ears and
    temples.
  - Strands poking through a beanie are the usual failure. Options: a tighter/shorter groom; the
    project-wide or per-character "Always Use Hair Cards" (cards clip less visibly than strands and cost less
    on a laptop); or an inflated hat mesh. Check by screenshot from behind and at 3/4.
  - Long-haired women under hats: `Hair_S_LowPonytail` or `Hair_S_PulledBack`, so the tail comes out below
    the hat.
  - [MED] There's no automatic hat-flattens-hair feature in the docs I found.

### 4.7 Teeth (Materials > Teeth & Eyelashes)
- Not researched in depth. Older rural highlanders often have **worn, yellowed or missing teeth**; clients
  have white, even teeth [LOW]. Check the teeth tools in a later pass.

### 4.8 Children
- No official child support (see 2.7). Dolma needs Fab child MetaHumans (check licence and 5.8 compatibility)
  or a non-MetaHuman approach [MED].

---

## 5. Avoiding the "too clean CG" look

**What the shipped runtime material can do [HIGH, read from the asset strings, verify in the editor].**
- After assembly, the cast's face uses `MI_Face_Skin_Baked_LOD*` with parent **`M_skin_unified_baked`**
  (`Content/Story/MetaHumans/Common/Lookdev_UHM/Skin/Materials`). It exposes only **global** adjustments:
  - BaseColorAdjust, Roughness Adjust, Spec Adjust, Scatter, Normal, Fuzz, Micro Skin, Concavity/Convexity
    multipliers.
  - Epic's doc also says global adjustments only:
    https://dev.epicgames.com/documentation/metahuman/metahuman-materials-and-textures
- The **editor-preview** parent **`M_skin_unified`** (engine `Lookdev_UHM/Skin/Materials`) has much more:
  - static switches **`Use Dirt`** (Dirt Mask, Dirt Color, Dirt Roughness/Specular Value/Multiply, Dirt
    Normal), **`Use Wet`** (Wet Mask, Wet Roughness/Specular, Wet Normal Strength), `Use Blood`, `Use Makeup`,
    **`Use Region Adjustments`**.
  - per-region Basecolor/Roughness/Specular/Normal/Scatter for **Nasal Tip, Nasal Bridge, Temporal Cheeks,
    Oral Lip Upper/Lower, Orbital Crowsfeet, Orbital Eyebags, Frontal Main, Auricular**, etc.
  - **None of that exists in the baked parent**, so it is lost at assembly unless you carry it over.

**Ways to get dirt, wear and asymmetric sun damage into the game, best first:**
1. **5.8 Material Overrides with "Inherit UI Parameters and Source Textures"** (section 4.1).
   - Make a face MIC (a child of a duplicate of the baked parent that has a dirt and region layer added),
     set it as the override and keep the UI params.
   - Lets you add: a tiling **grime mask** (nail folds, nose creases, hairline, neck); a **hat-line or
     glasses tan** mask (lighter forehead band and eye band); asymmetric sunburn; **chapped-lip roughness and
     normal**; and per-region roughness so the cheeks are matte while the nose tip and lips are shinier.
   - [MED: the feature exists in the 5.8 header; I haven't tested how assembly handles it.]
2. **Texture Override** of Face base colour, normal and cavity: paint the sun damage, melasma, pale crow's-feet
   crease lines and broken capillaries into the albedo (a pterygium goes in the eye/sclera material, not the skin) (in Substance or Krita, using the downloaded 4k
   source textures as the base). Simple and robust; costs one texture set per character [HIGH that the
   feature exists].
3. **Post-assembly material edit** on the generated `MI_Face_Skin_Baked_*` (or a duplicate of
   `M_skin_unified_baked`) with a dirt layer. Simple, but a rebuild from Creator overwrites it, so script it
   in mh_cast or import_garments [LOW risk note].
4. **DBuffer decals** for soot on hands or a smudge on a cheek. Cheap and per-shot, but they swim on animated
   faces. Use them only on hands and clothing edges [LOW].

**Other anti-CG rules** (most [LOW]: established CG lookdev practice; check each by screenshot):
- **Peach fuzz on** (`Peachfuzz_M_Thin`). It catches rim light, and at -10 C in low sun the fuzz is visible.
- **Roughness:** matte overall (1.10-1.15 for Sherpas). Spec highlights only on the nose tip, lips (balm)
  and under the eyes. A uniformly glossy face is the clearest CG giveaway. A UE 5.6 forum report describes
  MetaHumans going too glossy, with no fix posted, so verify in game, not only in the Creator viewport:
  https://forums.unrealengine.com/t/metahuman-looks-glossy-in-metahuman-creator-5-6/2597347
- **Asymmetry:** Accents are always symmetric. Real sun damage isn't: the driver-side effect, the side facing
  the slope, one eye with pterygium. Break the symmetry with the texture override or decals.
- **Colour variation:** faces darker and warmer than necks. Ears and nose redder. Under-chin *pinker* on
  clients (albedo burn). Upper forehead lighter under the hat line.
- **Lips:** not saturated pink. Highland lips are often darker or bluish-brown and dry, with fine cracks.
- **Eyes:** lower GlobalSaturation, more vascularity on outdoor workers, small pupils in snow light, wet
  line and lacrimal fluid kept (MetaHuman default).
- **Hair:** lower hair roughness default looks shampoo-clean; raise roughness, add flyaways (messy
  variants), and show the **hat-compressed** shape when the hat is off.
- **Hands:** cavity darkness, rough dull nails, darker knuckles, grime.
- **Clothes touching the face:** Buff or scarf edges, glasses on the forehead, beanie down to the eyebrows.
  These break the silhouette and hide the face-body tone seam.
- **Performance** (laptop/Steam target): every option above is texture or material-level. No extra strands
  beyond peach fuzz; prefer cards for hat wearers. Skin LODs drop SSS and animated maps below High quality
  anyway (Epic doc), so test the look at **Medium** settings too.

---

## 6. Per-character starting recipes (first guesses, verify by screenshot)

| | Skin U (light 0 to dark 1) | Roughness | Face tex (filter) | Accents (R/S/L) | Freckles | Eyes | Grooms |
|---|---|---|---|---|---|---|---|
| **Nima** (~55-65, sirdar) | darker than preset Aoi, ~+0.1 | 1.12 | Wrinkles High / Stubble Low (needs unrig) | Cheeks .60/.48/.43, Nose .58/.5/.45, Ears .58 | Type2, D .35 S .2 Sat .35 (sun spots) | dark brown, Sat 1.3, custom warm sclera, Vasc 1.5/0.28 | Hair_S_Clean or S_RecedeMessy, Mel .9 White .4; Mustache_S_Uneven or none; Beard_S_Stubble or Goatee_L_Scraggly at low density; Eyebrows_M_Messy (grey via Whiteness); Peachfuzz |
| **Pema** (~50) | about like preset Lani or slightly darker | 1.12 | Wrinkles Med-High / Stubble None | Cheeks .66/.55/.45, Nose .58, UnderEye L .45 | Type1 low (lentigines); melasma needs texture | dark brown, Sat 1.4, Vasc 1.3 (smoke) | Hair_S_PulledBack or S_LowPonytail (custom braid later), Mel .92 White .2; Eyebrows sparse (not L_Shaded); Peachfuzz |
| **Porter A** (young) | much darker than preset Kelvin (+0.25 to 0.35) | 1.10 | Wrinkles Low / Stubble None-Low | Cheeks .72/.6/.45, Nose .62, Ears .6 | none | dark brown, Vasc 1.2 | Hair_S_Messy or S_Casual (straight), Mel .92; no chinstrap or curly moustache; Peachfuzz |
| **Dolma** (8) | n/a until a child base exists | 1.08 | n/a | Cheeks .75/.62/.46 (chapped red), Lips S .42 L .53 | none | dark brown | two braids or a ponytail |
| **Marek** (40s) | preset Bruce, slightly tanned | 1.06 | Wrinkles Medium / Stubble Low | Nose .75, Cheeks .68, Ears .66, Chin .60, Forehead .56 | Type3 light (sun freckles on fair skin) | default | Hair_S_BrushCut; Beard_S_Full or M_Stubble + Mustache_S_Full, a little Whiteness at the chin |
| **Elena** | 0.36 (set) | 1.04 | Low wrinkles | Nose .70/.58, Cheeks .60, Chin .56, Lips S .42 L .54 | optional Type1 very low | brown or hazel instead of green | Hair_S_LowPonytail (tied back); Eyelashes_S_Fine; Lips Natural, balm with Metalness 0 |
| **Ji-woo** | preset Aera, a touch warmer | 1.04 | Low wrinkles | Nose .66, Cheeks .58, Chin .55 | none | default dark brown, Sat 1.5 | needs a Hair groom under the hat (LowPonytail or BobStraight); Eyelashes_L_SlightCurl OK |

(Absolute U/V values aren't shown on the chart and the presets' current U isn't recorded here. Read
`skin_settings.skin.u/.v` from each `MH_*` asset first and adjust relative to that.)

---

## 7. Screenshot checklist for each iteration

For each character, take a close-up (head and shoulders) in three lights: in-game sun on snow, overcast,
and the warm house interior. Also one 3/4 back view with the hat on.
1. Is the face visibly **darker and warmer than the neck and hands**? Is there a seam at the collar?
2. Are the cheeks ruddy in a **brick** tone (not pink blush), and only on some characters?
3. Are the nose tip, ears and under-chin redder on clients?
4. Is the skin **matte** overall, with highlights only on nose, lips and under-eyes? No full-face sheen?
5. Is hair visible **under the hat edge** at the nape, sideburns and temples? Any strands poking through
   the hat?
6. Sherpa men: **sparse** facial hair? No full beard?
7. Eyes: iris not neon? Whites not paper-white on outdoor workers? Pupils small in snow light?
8. Lips: dry and dull, not glossy or saturated (except a balm sheen on clients)?
9. Hands: grime in creases, dull nails?
10. Peach fuzz visible in backlight?
11. Compare side by side with at least 2 real reference photos per character type, from news agencies,
    museum archives, or the Wellcome Collection CC-BY Khumbu set
    (https://wellcomecollection.org/works/ctgufgad).

---

## 8. Uncertainty and gaps

- No study measured **Sherpa** skin tone, hair practice or facial hair directly. Those points are inferred
  from Tibetan/Qinghai, Ladakh, Kathmandu-clinic and East Asian data. Marked [LOW]/[MED] above.
- Women's braids and thread customs: only informal or Tibet-wide sources. Better sources to check: Sherry
  Ortner (*Sherpas Through Their Rituals*), Fürer-Haimendorf (*The Sherpas of Nepal*, 1964), the SOAS
  Fürer-Haimendorf photo archive (https://digital.soas.ac.uk), and the Wellcome Collection Khumbu set.
- The Face Texture Index mapping (attribute list entry i = texture index i) is assumed. Check one value in
  the editor before using the lists.
- Material Overrides "Inherit UI Parameters" is new in 5.8. I read it from the header and haven't tested it.
- Slider starting values are untested guesses. The user judges the visuals; this note supplies facts and
  ranges.

## 9. Sources (all accessed 2026-11-14)

**Epic / engine**
- MetaHuman docs, Skin Material Tools: https://dev.epicgames.com/documentation/metahuman/skin-material-tools
- Makeup Material Tools: https://dev.epicgames.com/documentation/metahuman/makeup-material-tools
- Eye Material Tools: https://dev.epicgames.com/documentation/metahuman/eye-material-tools
- Hair & Clothing Tools: https://dev.epicgames.com/documentation/metahuman/hair-and-clothing-tools
- Hair Overrides: https://dev.epicgames.com/documentation/en-us/metahuman/hair-overrides
- MetaHuman Materials and Textures: https://dev.epicgames.com/documentation/metahuman/metahuman-materials-and-textures
- Body Params tool: https://dev.epicgames.com/documentation/metahuman/metahuman-creator-body-params-tool-in-unreal-engine
- Engine source (local, UE 5.8): `MetaHumanCharacterSkin.h`, `MetaHumanCharacterMakeup.h`, `MetaHumanCharacterEyes.h`,
  `MetaHumanCharacter.h`, `MetaHumanDefaultGroomPipeline.h`, `MetaHumanCharacterEditorSkinTool.cpp/.h`,
  `SMetaHumanCharacterEditorSkinToolView.cpp`, `TextureSynthesis/TS-1.3-F_UE_res-1024_nchr-153/texture_attributes.json`,
  `Lookdev_UHM/Skin/Materials/M_skin_unified(.uasset)`, project `Common/Lookdev_UHM/Skin/Materials/M_skin_unified_baked`.
- Forum (gloss issue, no fix): https://forums.unrealengine.com/t/metahuman-looks-glossy-in-metahuman-creator-5-6/2597347
- Child MetaHumans (third party): https://forums.unrealengine.com/t/athian-games-metahuman-child-characters/2701955

**Medicine / science**
- Singh GK. High Altitude Dermatology. Indian J Dermatol 2017;62(1):59-65. https://pmc.ncbi.nlm.nih.gov/articles/PMC5286755
- Ma L et al. VEGF in "Plateau Red Face". High Alt Med Biol 2015;16(2):147-153. https://pmc.ncbi.nlm.nih.gov/articles/PMC4490706
- Everest UV dosimetry and skin (Sunnybrook, 2007 expedition). Wilderness Environ Med 2013. doi:10.1016/j.wem.2013.05.007; https://clinicaltrials.gov/study/NCT00685438
- WHO. Global Solar UV Index: A Practical Guide (2002). https://www.who.int/publications/i/item/9241590076
- UIAA MedCom Standard No. 20, Eye Problems on Expeditions (2020). https://www.theuiaa.org/documents/mountainmedicine/Engl_UIAA_MedCom_Rec_No_20_Eye_problems_2020_V2-4.pdf
- Maharjan IM et al. Pterygium in Upper Mustang. Nepal J Ophthalmol 2014;6(11):65-70. https://nepjol.info/index.php/NEPJOPH/article/view/10774/8749
- Ocular morbidity among porters at high altitudes. Nepal J Ophthalmol 2017. https://nepjol.info/index.php/NEPJOPH/article/view/17529
- Ridley Eye Foundation, High Altitude Cataract Project. https://ridleyeyefoundation.org/ref-nepal
- Duo B et al. Indoor air pollution in Sherpa villages (2016). https://sfera.unife.it/handle/11392/2356498
- Sherpa/Tibetan EPAS1 and low haemoglobin. https://www.ncbi.nlm.nih.gov/pmc/articles/PMC5241213/
- Androgen action, Caucasian vs Chinese (chest hair 3.0 vs 0.8). https://pure.psu.edu/en/publications/clinical-and-biochemical-parameters-of-androgen-action-in-normal-
- East Asian vs Euroamerican hairiness. https://pmc.ncbi.nlm.nih.gov/articles/PMC6587453
- Nepali melasma / Fitzpatrick series: https://nepjol.info/index.php/JKMC/article/download/53551/40357/159726 ·
  https://nepjol.info/index.php/NJDVL/article/view/45068/36309 · https://pmc.ncbi.nlm.nih.gov/articles/PMC11404921/
- Recurrent herpes labialis in skiers: https://scilit.net/publications/703fb0ba228b10c8997ca8420340fba8
- Caudwell Xtreme Everest body composition (7.3 ± 4.9 kg loss by 8 weeks): via https://eprints.soton.ac.uk/410618/3/main.pdf
- Mayo Clinic, Frostbite: https://www.mayoclinic.org/health/frostbite/DS01164

**Ethnographic / press / outfitters**
- 1953 NZ press on Sherpa pigtails: https://paperspast.natlib.govt.nz/newspapers/CHP19530718.2.20
- SOAS, Khumjung festive dress caption: https://library.soas.ac.uk/Record/PP_MS_19.08.01.SHER.178
- Sherpa wedding photo essay (braids with threads): https://medium.com/variantbio/nepalese-sherpa-wedding-a-photoblog-261871d25fef
- Tibetan women's braid customs (Tibet, not Sherpa): https://www.tibettravel.org/tibetan-people/tibetan-womens-headdress.html
- Facts and Details, Sherpa life (hats, clothes; cites NatGeo 1992): https://factsanddetails.com/south-asia/Nepal/Ethnic_Groups_and_Minorities_Nepal/entry-7840.html
- Madison Mountaineering Everest gear list 2023: https://MADISONMOUNTAINEERING.COM/wp-content/uploads/2019/08/EVR-2023-Mount-Everest-Expedition-gear-list.pdf
- EBC itineraries: https://www.intrepidtravel.com/us/nepal/everest-base-camp-trek-144944 · https://www.kimkim.com/c/everest-base-camp-trek
- Wellcome Collection, 1986 Khumbu porter photos (CC BY 4.0): https://wellcomecollection.org/works/ctgufgad
