# Cultural Check · Sherpa and Tibetan Buddhist content

Version 1.0 · 2026-11-20 · written by Claude for the director. It is for a review with a knowledgeable person **before any
public showing** (title demo, trailer, Steam page, release).

**Why this file exists.** Chomolungma shows a living people and a living religion. Pillar P4 of the Chapter 1 design says:
real place, real people, respectfully. This file lists every Sherpa or Buddhist item in Chapter 1, plus a few planned for
later chapters. For each one it says what the game does today, what good sources say, how risky it is, and what to do.

**Read this first**
- I am not a Sherpa and not a Buddhist teacher. Most sources below are expedition reports, encyclopedias and travel
  guides. A few are scholarly. Where sources disagree, this file says so.
- "Correct" below means "what our sources say". Practice differs between villages, families and lamas. **The reviewer
  has the last word** on every row.
- Nothing in the game was changed. This is a checklist only.
- What I checked myself: the story file `Content/Story/Chapter1.ygs`, the world builder
  `Tools/Story/build_chapter1_world.py`, the asset list, and two of the flag textures (`Art/PrayerFlags/T_PrayerFlag_Blue.png`
  and `_Red.png`), which I looked at directly.

**Risk levels**

| Level | Meaning |
|---|---|
| **High** | Wrong or fake on a sacred object that every player sees, or something that could deeply hurt people. Fix before any public showing. |
| **Medium** | A knowledgeable player would likely notice, or we have no good source yet. Ask the reviewer. |
| **Low** | Already right, or a small detail. Polish when there is time. |

Sources are numbered [S1], [S2] ... and listed with links in section 8.

**Words used below**

| Word | Plain meaning |
|---|---|
| Lungta | "wind horse": the small square prayer flags on a horizontal string |
| Darchog / tarchen | a tall pole with one long vertical prayer banner |
| Chorten | a Buddhist stupa: a sacred structure of stone or plaster. Many memorials at Thukla are built like small chortens |
| Mani stone / mani wall | stones carved with a prayer, piled into a long low wall along a trail |
| Khata | a white ceremonial scarf, usually silk |
| Puja | here: the blessing ceremony a lama performs at Base Camp before the climb |
| Sang | burning juniper as an offering; the smoke purifies |
| Bardo | in Tibetan Buddhism, the state between death and the next birth |
| Mantra | a short sacred phrase, said or written as a prayer |

---

## 1. The five most important actions

| # | Action | Risk |
|---|---|---|
| 1 | **Replace the print on every prayer flag.** The AI-made print shows fake Tibetan writing and the wrong corner animals. Until a Tibetan reader checks new text, use a print with no letters. Details in section 4. | **High** |
| 2 | **Check the father's plaque**, "MINGMA SHERPA 1978 - 2026". Mingma Sherpa is a very common real name. Make sure it does not match a real person who died on Everest. | Medium |
| 3 | **Ask whether a trekking camp at Dingboche would have a prayer mast with lines radiating out, and a stone altar.** Our sources describe both only at Base Camp, raised during the puja. | Medium |
| 4 | **Decide how to frame the father's spirit and the "49 days".** In Tibetan Buddhism the time between death and rebirth is at most 49 days. The father died last season, so more than 49 days ago. | Medium |
| 5 | **Decide the subtitle "Goddess Mother of the World".** It is the popular translation of Chomolungma, but scholars question it. | Medium |

---

## 2. Checklist: Chapter 1

| # | Item in game | Where | What we do now | What is correct (per sources) | Risk | Action |
|---|---|---|---|---|---|---|
| 1 | Prayer flag colours (lungta) | camp, trail chorten, memorials | The world builder repeats blue, white, red, green, yellow along every line (`flag_line` in `build_chapter1_world.py`) | Sets of five in this fixed order: blue (sky, space), white (air, wind), red (fire), green (water), yellow (earth) [S1][S2] | Low | Keep. Never break the order inside one line. |
| 2 | **Print on the flags** | every flag | One Meshy image, tinted five times (`Art/PrayerFlags/T_PrayerFlag_*.png`). The wind horse with a flaming jewel is good. The four corner animals look like lions, a tiger and a bull: **no garuda, no dragon**. The text blocks **look like Tibetan but are not readable Tibetan** | The wind horse carries the wish-fulfilling jewel in the centre. Corners: garuda top left, dragon top right, tiger and snow lion at the bottom (sources differ on which side). Mantras and prayers fill the space between [S1][S3][S4][S5] | **High** | Replace. See section 4. |
| 3 | How the flags hang | everywhere | Flags hang from a curved cord; sag is capped at 45 cm. Director (2026-11-19): they float and look fake. Earlier, some memorial lines sank into the ground | Each flag is sewn along its top edge to a cord. Lines run diagonally from high to low between high points: poles, roofs, stupas, passes. **Flags never touch the ground.** Old flags are left up and new ones go up beside them [S1] | Medium | Add a clearance check to the world builder: no flag or cord touches ground, rock or tent. Mix faded old lines with a few bright new ones. No flag may ever lie on the ground, not even as debris. |
| 4 | Prayer pole (tarchen) with lines radiating out | in front of the Dingboche camp | A Meshy pole with a vertical banner. Five flag lines run from its top to the mess dome and to poles. A khata is tied on the pole | Vertical banners on poles stand by houses, on hills, on cairns and roofs [S1]. A mast with flags radiating "in five directions" is described **at Base Camp, raised during the puja** [S12][S13]. We found no source for such a mast at a trekking camp | Medium | Ask the reviewer. Option: tie the camp's flags to village things (a house roof, a lodge pole, the trail chorten) and keep the radiating mast for Base Camp after the puja (Chapter 2). Also check the banner's print: if it has AI-made text, it needs the same fix as row 2. |
| 5 | Stone altar ("lhapso") and juniper hearth | beside the pole at camp | A 1.2 m cairn model (`chorten_b`) used as an altar, with a ring of 7 flat stones in front for burning juniper (no smoke yet) | At Base Camp the Sherpa team builds a stone chorten as the altar for the puja. Juniper is burned there as an offering [S12]. Juniper smoke pleases the mountain gods and purifies the climbers [S11]. **The word "lhapso" did not appear in any source we found.** Related Tibetan words are lhatho and labtse: shrines and cairns for local gods [S31] | Medium | Ask the reviewer for the right word, and whether a trekking camp would build an altar at all. If not, move it to Base Camp (Chapter 2). At Dingboche, a family burning juniper at its own house may be truer [To verify]. |
| 6 | Juniper smoke | camp (planned) | Not built: there is no smoke system yet | Juniper branches are burned as an incense offering, often in the morning [S11][S12] | Low | When it is built: juniper branches on stones, thin white smoke, at dawn. Not incense sticks (those look Hindu or Chinese). |
| 7 | Trail chorten | where the trail leaves the camp fields | The 2 m **memorial** chorten model with a plaque (`chorten_a`), placed so the walker keeps it on the right going out | Buddhists walk clockwise round sacred objects, so a chorten or mani wall stays on your right [S7][S8] | Low | The side is right. But a village trail chorten is usually whitewashed, not a memorial with a plaque. The asset list (#44) already asks for a whitewashed one: swap the model. If the path splits round it, walkers going both ways can keep it on their right (common in trail photos; confirm with the reviewer). |
| 8 | Mani wall (proposed, to fill the 8-minute gap) | last climb to the pass | Not built. Nima would correct you if you pass on the wrong side | Mani stones carry "Om mani padme hum" in Tibetan script, usually as raised letters, often painted: om white, ma green, ni yellow, pad light blue, me red, hum dark blue. Pass from the left, so the wall is on your right [S6][S7] | **High** if the carving is AI-made; Low if verified | Carve only the verified string in section 4. Nima's correction: gentle, once, no penalty. The player cannot pick up, kick or stand on mani stones. |
| 9 | Khata at the farewell | camp edge | Aunt Pema "ties the white khata round your neck"; Sangye sees it on his jacket | A khata given at parting wishes a safe journey. White stands for a sincere heart [S9]. It is offered with both hands; to an equal or a younger person it is put round the neck [S10] | Low | Change "ties" to "puts" in both caption lines: it is draped, not knotted. Animate it with both hands. |
| 10 | Khata on the father's stone | Thukla memorials | Hold F to tie it; caption: "You tie your khata to his name." | Khatas are offered at temples and are tied to cairns, stupas and sacred rocks [S9]. Scarves on the Thukla memorials are common in photos [To verify] | Low | Keep. Ask the reviewer whether tying it round the stone or laying it over the stone is more normal. "To his stone" may read better than "to his name". |
| 11 | Memorial chortens | Thukla | About 22 Meshy stone chortens and cairns in 3 kinds, flag lines between them. Proposed rules: no real names, kneel only at the father's stone, nothing can be climbed or knocked over | Over 100 stone memorials to climbers and Sherpas stand on the pass; famous ones include Scott Fischer's. A Khumbu Sherpa view: new memorials should be "simple, environmentally friendly, and made with stone only" [S22] | Medium | Keep the stone-only look. Do not copy any real, well-known memorial. Plaques unreadable or fictional. Make the proposed rules real: no climbing collision, no physics, no footprint decals on them. |
| 12 | Father's plaque | father's stone | A text actor: "MINGMA SHERPA / 1978 - 2026", in English | Inscriptions at Thukla vary; one quoted inscription is in English [S22]. Mingma is a very common Sherpa name [To verify] | Medium | Search the Himalayan Database and 2025-2026 news for a real Mingma Sherpa who died on Everest. If anything matches, change the name or drop the years. Ask the reviewer whether a Sherpa family would add a Tibetan line (for example the mantra in section 4). |
| 13 | Father's spirit, the Bardo, "Day 1 / 49" | prologue, journal | The dead father speaks in the prologue. The journal counts 49 days. Later chapters call the ghosts "souls in the Bardo" | After death there are 49 days "between lives", then rebirth [S15][S16]. Families call lamas at once, cremate the body, pray every 7 days, and hold a 49-day ceremony (gyewa) [S14][S15]. Sherpas say they believe in the spirits of the dead and want rites for those who died on Everest [S18]. They do not like to leave bodies on the mountain [S17] | Medium | The father died "last season", so more than 49 days ago. Do not present the game's 49 days as Buddhist teaching. Ask the reviewer: how does a Sherpa family see a death with no body? Does a spirit that stays near its body fit their belief? |
| 14 | The father's frozen body | prologue, 8,400 m | He sits in the snow in a red down suit, no oxygen mask | Many bodies remain on Everest. They are real people with living families [S17] | Medium | Do not copy any real body (pose, clothes, place). No gore. Never let the player search or take things from a body. |
| 15 | The gold plume (the myth moment) | dusk at the memorials | The summit plume glows gold for 11 s; only Sangye sees it; no god is named | Miyolangsangma, a goddess of generosity, lives on Everest. She is one of the Five Long-Life Sisters [S19]. Not all Sherpas link Everest to her [S11]. She is shown with a mongoose that spits jewels [S14], or riding a golden tigress [S19] | Low | Keep it subtle and unnamed in Chapter 1. Before she is ever shown, get the reviewer's OK on her form. |
| 16 | Title subtitle | the fly-out title, the journal | "CHOMOLUNGMA · Goddess Mother of the World" | Britannica gives "Goddess Mother of the World" [S21]. Wikipedia gives "holy mother" [S20]. Reinhard (1987) rejects the popular reading and gives "goddess" plus "valley" [S11]. Ward lists other readings [S21] | Medium | Ask the reviewer. Options: keep it; use "Holy Mother"; or show no subtitle. |
| 17 | The father's prayer beads (mala) | duffel; an optional item to carry | Inspect it; carry it as one choice | A sacred object in daily use | Low | Never dropped or thrown in any animation; kept in a pocket or worn. |
| 18 | Forms of address and language | Dolma's lines | "Sangye-dai" | "Dai" is Nepali for "older brother". Sherpa is its own Tibetic language, mostly spoken [S25] | Low | Ask a Sherpa speaker what a Sherpa girl calls an older cousin. For open question D9: if people speak a local language, a Sherpa family at home speaks Sherpa, not Nepali. |
| 19 | Names | Sherpa characters | Sangye, Nima, Dolma, Pema, Mingma | All are used as Sherpa or Tibetan names. Nima and Mingma are also day-of-the-week names [To verify] | Low | The reviewer confirms the names fit one Dingboche family and village. |
| 20 | The word "Sherpa" | captions, menus, store page | Sangye is a Sherpa; his job is "climbing Sherpa" | Sherpa is an ethnic group, not a job. Sherpa writers ask that the two be kept apart [S23][S24] | Low | Never call the porters "sherpas" unless they are Sherpa. In marketing, write "Sherpa people", never "a sherpa" as a job title. |
| 21 | Aunt Pema's dress | farewell | Planned: a long sleeveless wrap dress, a striped front apron, a sash with a silver buckle, a floral headscarf | A sleeveless dress (angi) over a blouse; striped aprons held by a silver buckle; the front apron (pangden) is worn only by married women [S32] | Medium | If Pema is a widow, ask whether she still wears the front apron (already an open question in the v2 design). |
| 22 | Dignity of the Sherpa characters | whole chapter | Nima is the expert and has his own judgement; porters are real workers on the trail | Sherpa writers object to Sherpas being shown only as helpers in other people's stories [S24] | Medium | Keep Nima the expert in every scene; the clients never "save" him. Give the porters faces and one line, not only a role as obstacles. |

---

## 3. Later chapters and parked assets (check before building them)

| Item | Where | What is planned | What sources say | Risk | Action |
|---|---|---|---|---|---|
| Base Camp puja | Chapter 2 | A lama blesses climbers and axes at the stone altar; juniper smoke, flags, rice. v1 idea: what you have blessed gives small "sanity" and "trust" effects | A lama comes from a nearby monastery (for example Pangboche). The day is chosen from the Tibetan calendar. Sherpas build a stone chorten as the altar. Juniper and food are offered. A mast of flags goes up in all directions. People throw tsampa (barley flour) and put it on each other's cheeks; rice is thrown [S12][S13]. Expedition blogs say Sherpas will not climb before it [To verify] | Medium | Build it the way the sources describe. The lama is a respected person, never comic. **Do not turn blessings into number bonuses.** Effects on relationships (Nima's trust) are fine. |
| Six Realms as weather | Chapters 2-10 | Hungry Ghosts, Cold Hells, Demigods' wind as weather events | The Six Realms are a teaching about rebirth, not about weather | Medium to High | Show them as Sangye's visions, not as teaching. Never as enemies to kill. Reviewer sign-off before building. |
| Wish-Fulfilling Tree on the summit | Chapter 12 | The golden tree, "give him back" or "let him go" | A tree from Buddhist cosmology (Mount Meru) [To verify] | Medium | Reviewer sign-off on the idea and its look. |
| House altar: butter lamps, 7 offering bowls, low table (choktse), father's photo with a khata | Sherpa house (parked in v2) | Meshy brass bowls and lamp, a painted low table | Seven water bowls are filled in the morning and emptied in the evening; they are not left empty and upright [S30]. The Buddha image sits highest on the altar [S29] | Low now | If the house returns: bowls in a straight row, filled; lamp below the Buddha image; the father's photo lower than any Buddha image [To verify]. |
| Thangkas (painted scrolls) in the house | Sherpa house (parked) | 2D images | Deity paintings follow strict rules of form and colour | **High** if AI-made | Use licensed photos of real thangkas, or none. Never an AI-made deity. |
| Chanting or ritual audio | puja, monastery | not chosen yet | Sound libraries often sell "Tibetan chant" with unknown words | Medium | Use only recordings whose content and licence are known, ideally recorded with permission. |

---

## 4. The AI-made Tibetan script on the flags

### 4.1 What we have

- All five flag textures come from **one Meshy image** (`Tools/Materials/make_prayer_flag.py`, note of 2026-11-20). They
  differ only in colour.
- The **wind horse** carrying a flaming jewel in the centre is a fair match to real flags.
- The **four corner animals do not match** the tradition. They look like two lions, a tiger and a bull. A real flag has a
  garuda (a bird) top left and a dragon top right [S3][S5].
- The **writing looks Tibetan but is not real Tibetan text**. Some shapes are close to Tibetan letters, many are not, and
  the lines do not form words. This is what AI image tools do with any script.
- The v2 design rule says "unreadable script until a Tibetan reader checks". The current print breaks that rule in a bad
  way: it is not plainly unreadable, it **looks like real writing and invites reading**.

### 4.2 Why it is High risk

- The text on a prayer flag is sacred. Sources ask that flags be treated with respect, never put on the ground or used
  as clothing [S1].
- A Tibetan or Sherpa player will see nonsense on a sacred object in the very first minutes, on every flag in the game.
  This is the same mistake as fake Chinese characters on a sign, but on a prayer.
- It would also undercut the game's main promise: the real mountain, seen through Sherpa eyes.

### 4.3 Recommendation

| Option | What | When |
|---|---|---|
| **B (now, interim)** | A print with **no letters at all**: the wind horse and soft, worn ink blocks where the text would be. Real flags fade; from a few metres this reads as a faded print. Nothing can be misread. | before any public screenshot |
| **A (recommended)** | Redraw the print: keep the wind horse; put the four traditional animals in the corners (or their names, as some flags do [S4]); fill the text area with **"Om mani padme hum"** in rows, set in a real Tibetan font. A Tibetan reader checks the final images. | after the reviewer is found |
| C (most authentic) | Real lungta carry long prayers, often a long dharani of the victory-banner deity [S4]. Scan a real woodblock-printed flag bought from a Tibetan or Sherpa maker. Only with the reviewer's OK and after checking that we may use the design. | later, if wanted |

"Om mani padme hum" is the mantra of Avalokiteshvara (Chenrezig), the bodhisattva of compassion. It is written on prayer
flags, prayer wheels and mani stones [S6]. It is the safest choice because it is short, very common and easy to verify.

### 4.4 The verified string

| Form | Text |
|---|---|
| Tibetan (Unicode) | ཨོཾ་མ་ཎི་པདྨེ་ཧཱུྃ |
| With the end mark (shad) | ཨོཾ་མ་ཎི་པདྨེ་ཧཱུྃ། |
| Escaped, for code | `ཨོཾ་མ་ཎི་པདྨེ་ཧཱུྃ` (end mark: `།`) |
| Latin | Om mani padme hum (Oṃ maṇi padme hūṃ) |
| Devanagari (for comparison) | ॐ मणि पद्मे हूँ |

Code points, syllable by syllable (checked with Python's `unicodedata`; the string is already in normal form NFC):

| Syllable | Code points | Unicode names |
|---|---|---|
| ཨོཾ om | U+0F68 U+0F7C U+0F7E | LETTER A, VOWEL SIGN O, SIGN RJES SU NGA RO |
| ་ (dot between syllables, "tsheg") | U+0F0B | MARK INTERSYLLABIC TSHEG |
| མ ma | U+0F58 | LETTER MA |
| ཎི ni | U+0F4E U+0F72 | LETTER NNA, VOWEL SIGN I |
| པདྨེ padme | U+0F54 U+0F51 U+0FA8 U+0F7A | LETTER PA, LETTER DA, SUBJOINED LETTER MA, VOWEL SIGN E |
| ཧཱུྃ hum | U+0F67 U+0F71 U+0F74 U+0F83 | LETTER HA, VOWEL SIGN AA, VOWEL SIGN U, SIGN SNA LDAN |
| ། (end mark) | U+0F0D | MARK SHAD |

**One variant to ask about.** Wikipedia also prints the last syllable with U+0F82 (ཧཱུྂ, a slightly different mark on
top) [S6]. Both forms are in use. The reviewer picks one.

### 4.5 How to make the texture

1. **Font.** Noto Serif Tibetan or Jomolhari. Both use the SIL Open Font License 1.1, so commercial use is allowed; put
   the licence text in the game's legal credits [S28]. Do not use Microsoft Himalaya (it ships with Windows; it is not
   ours to put in a texture we sell without checking its licence). The Qomolangma fonts are for non-commercial use only
   [S28].
2. **Shaping.** Tibetan letters stack. In པདྨེ the small "ma" sits under the "da". A tool without "complex text shaping"
   shows boxes or loose letters. Set the text in a tool that shapes Tibetan (a web browser, Inkscape, LibreOffice), export
   a PNG, and compare it with the Wikipedia image of the mantra [S6].
3. **Bake it into the texture** (ink colour, a little blur, fade). Do not use live Unreal text on the flags.
4. **Direction.** The text runs left to right on the printed face. Check that the flag mesh's UVs are not mirrored. On the
   back, a faint mirror image is fine: ink soaks through cotton.
5. **Faded is fine, wrong is not.** Old flags are pale and torn. Small or faint letters are realistic.
6. **Mani stones** use the same string, one syllable per colour if painted (section 2, row 8).
7. **Sign-off.** The reviewer approves the final PNG files, not only the plan.

---

## 5. General rules: things to avoid

These come from the sources, from criticism of earlier games, and from the project's own pillar P4.

1. **No fake script anywhere**: flags, banners, mani stones, plaques, signs, UI. Either verified text or no text.
2. **Respect sacred objects in the game's systems.** The player cannot climb, sit on, kick, break or loot chortens, mani
   stones, flags, altars or bodies. No physics on them. No footprint or blood decals on them.
3. **Sacred things never on the ground or on the body as decoration.** No prayer flag as a bandana, a rag or a doormat [S1].
4. **Do not mix in other religions by mistake.** The asset list already replaced Indian diya lamps and a Chinese tea
   table. Keep checking incense sticks, statues and lamps.
5. **No god, lama or monk as an enemy, a joke or a shop.** No blessing that works like a power-up.
6. **No "mystic Shangri-La".** Sherpa life today has phones, solar panels and down jackets. The realism spec already
   does this. Far Cry 4's mixed Nepal-Tibet country was criticised by a Nepali reviewer for claiming a culture it did not
   get right [S26].
7. **Respect the real dead.** No real names on memorials. No copies of real memorials or real bodies on the mountain.
8. **Mark the myth as myth.** Invented things (the gold plume, the Wish-Fulfilling Tree on the summit) should be seen by
   Sangye only, not shown as real Sherpa ritual. Never invent a ritual and present it as real.
9. **Work with the community, and pay them.** *Never Alone* (2014) was made with about 40 Iñupiat elders, storytellers and
   writers, who had a say in all parts of the game and are credited in it [S27].
10. **Credit the reviewers** (with their permission), and say clearly in the credits what is fiction.

---

## 6. Questions for the reviewer

1. Is the new flag print correct: animals, their corners, the mantra, its spelling (U+0F83 or U+0F82)?
2. Would a foreign expedition's trekking camp at Dingboche have a prayer mast and a stone altar? What is the right word
   for the altar ("lhapso" is in our design; we could not confirm it)?
3. How does a Sherpa family mourn a man whose body is still on the mountain? What do lamas do without a body?
4. Can the father's spirit stay near his body more than 49 days after death, in Sherpa belief, without contradicting the
   teaching? How should the game say it?
5. Is "Goddess Mother of the World" an acceptable subtitle?
6. Would a Sherpa family's memorial at Thukla carry English, Nepali or Tibetan text? Is tying a khata to it normal?
7. What does a Sherpa girl call her older cousin (instead of the Nepali "dai")?
8. Does a widowed Sherpa woman wear the front apron (pangden)?
9. Is it acceptable to show Miyolangsangma at all in later chapters? If so, how?
10. Is the Six Realms idea acceptable as Sangye's visions?
11. Anything in the chapter that a Sherpa player would find wrong or hurtful that this file did not list?

---

## 7. Suggested reviewers (kinds of people and organisations)

No personal contacts are listed. Check that each organisation still exists and how it prefers to be contacted.

| Who | What they can check | Where to look |
|---|---|---|
| A Khumbu Sherpa with expedition experience (climbing Sherpa, sirdar, or expedition cook) | camp life, the puja, juniper, the memorials, how Nima talks and behaves | Sherpa climbing schools and associations, for example the Khumbu Climbing Center (Phortse) or the Nepal Mountaineering Association |
| A lama or monk from a Khumbu monastery | the puja, death rites, the Bardo framing, Miyolangsangma, what may be shown at all | monasteries of the Khumbu, for example Tengboche or Pangboche |
| A Sherpa family, ideally an older woman from the Khumbu | the farewell, the khata, Pema's dress, forms of address, names | Sherpa community associations in Nepal and abroad (for example in Kathmandu or New York) |
| A Tibetan script reader or calligrapher | every piece of script: flags, mani stones, plaque | university Tibetan studies departments; Tibetan cultural centres; the Library of Tibetan Works and Archives (Dharamsala) |
| A Sherpa or Himalayan anthropologist | representation as a whole, the word "Sherpa", the clients-and-Sherpas story | university Himalayan studies programmes |
| A Himalayan art specialist | iconography: the flag animals, any deity, thangkas | Himalayan Art Resources; museum education departments with Himalayan collections |

**How to ask**
- Offer a fee and a credit. Do not ask for free work.
- Send screenshots, the spoken lines and captions, and this file. Give them time.
- Write each decision into this file, with the date and who decided.
- One reviewer cannot speak for everyone. For the title demo, aim for at least one Sherpa reviewer and one script reader.

---

## 8. Sources

"Read" means I opened the page. "Search only" means I saw it in search results and did not read it in full. Expedition
blogs and travel sites describe what their writers saw; they are not religious authorities.

| # | Source | Type | Used for |
|---|---|---|---|
| S1 | Wikipedia, "Prayer flag" · https://en.wikipedia.org/wiki/Prayer_flag | encyclopedia · read | colours, lungta and darchog, printed content, hanging, respect |
| S2 | New World Encyclopedia, "Prayer flag" · https://www.newworldencyclopedia.org/entry/Prayer_flag | encyclopedia · search only | colour order left to right |
| S3 | Tsewang Gyalpo Arya, "A short note on Lungta, a Tibetan prayer flag and its origin", Tibet Policy Institute, 10 Jan 2021 · https://tibetpolicy.net/a-short-note-on-lungta-a-tibetan-prayer-flag-and-its-origin/ | Tibetan institute · read | wind horse, the four animals and their corners |
| S4 | Jeff Watt, "Himalayan Buddhist Art 101: Prayer Flags, Part 2", Tricycle, 18 Apr 2013 · https://tricycle.org/trikedaily/himalayan-buddhist-art-101-prayer-flags-part-2/ | art historian · read | the four animals, names instead of pictures, the long dharani text |
| S5 | Tibetan Nuns Project, "The Meaning of Windhorse Tibetan Prayer Flags" · https://tnp.org/the-meaning-of-windhorse-tibetan-prayer-flags/ | Tibetan nonprofit · search only | corner positions |
| S6 | Wikipedia, "Om mani padme hum" · https://en.wikipedia.org/wiki/Om_mani_padme_hum | encyclopedia · read | Tibetan spelling, Avalokiteshvara, use on flags and stones |
| S7 | Wikipedia, "Mani stone" · https://en.wikipedia.org/wiki/Mani_stone | encyclopedia · read | mani walls, colours of the syllables, passing from the left |
| S8 | Wikipedia, "Circumambulation" · https://en.wikipedia.org/wiki/Circumambulation | encyclopedia · read | clockwise walking in Buddhism |
| S9 | Wikipedia, "Khata" · https://en.wikipedia.org/wiki/Khata | encyclopedia · read | meaning, white, farewells, tied to cairns and stupas |
| S10 | Great Tibet Tour, "Khata and related etiquette" · https://www.greattibettour.com/tibetan-culture/khata.html | travel company · search only | both hands, round the neck for equals and younger people |
| S11 | Johan Reinhard, "The Sacred Himalaya", American Alpine Journal, 1987 · https://publications.americanalpineclub.org/articles/12198712300/The-Sacred-Himalaya | anthropologist · read | the name Chomolungma, the Five Sisters, juniper, offerings, Sherpa feelings about climbing |
| S12 | National Geographic, Everest expedition dispatch on the puja · https://www.nationalgeographic.com/adventure/article/dispatch-april-12-text | expedition report · read | lama from Pangboche, stone chorten altar, juniper, flag mast, tsampa, rice, Tibetan calendar |
| S13 | RMI Guides, "Mt. Everest climbers listen to the prayers and chants of Sherpa during puja", 7 Apr 2015 · https://www.rmiguides.com/blog/2015/04/07/mt._everest_climbers_listen_to_the_prayers_and_chants_of_sherpa_during_puja | guide company · read | flags radiating in five directions, tsampa on cheeks |
| S14 | Broughton Coburn, "Why Sherpas will still climb Everest, even after deadly avalanche", National Geographic, 22 Apr 2014 · https://www.nationalgeographic.com/travel/article/140421-sherpas-mount-everest-climbing-risks-nepal | journalist · read | the 49-day gyewa ceremony, Miyolangsangma, the mountain can be "polluted" |
| S15 | F. Klatzel, "The Wheel of Life" (Sherpa story, part 3), Berg Adventures · https://www.bergadventures.com/v3_main/Sherpa-Story3.php | Khumbu writer · read | Sherpa death rites: lamas, cremation, tsatsa, prayers every 7 days, 49 days |
| S16 | Wikipedia, "Bardo" · https://en.wikipedia.org/wiki/Bardo | encyclopedia · read | the intermediate state, "seven times seven days" |
| S17 | Associated Press (via Fox News), "Death on Everest leads to risky effort to recover bodies", 27 May 2016 · https://www.foxnews.com/world/death-on-everest-leads-to-risky-effort-to-recover-bodies | news agency · read | a Sherpa guide: "we do not like to leave behind bodies"; cremation for the peace of the soul |
| S18 | Kedar Man Singh, "Everest is haunted by spirits", IOL, about 2004 · https://iol.co.za/news/world/2004-06-15-everest-is-haunted-by-spirits/ | news · read | Sherpa belief in spirits of the dead; call for rites |
| S19 | Wikipedia, "Miyolangsangma" · https://en.wikipedia.org/wiki/Miyolangsangma | encyclopedia · read | the goddess, generosity, golden tigress |
| S20 | Wikipedia, "Mount Everest" (name section) · https://en.wikipedia.org/wiki/Mount_Everest | encyclopedia · read | Qomolangma, "holy mother" |
| S21 | Michael Ward, "The name of the world's highest peak", Himalayan Journal 53 · https://www.himalayanclub.org/hj/53/3/the-name-of-the-worlds-highest-peak/ ; Britannica · https://www.britannica.com/print/article/197160 | journal; encyclopedia · search only | other readings of the name; "Goddess Mother of the World" |
| S22 | Heaven Himalaya, "Thukla Pass and Scott Fischer's memorial" · https://heavenhimalaya.com/thukla-pass-and-scott-fischers-memorial/ | trekking company · read | over 100 memorials, an English inscription, "made with stone only" |
| S23 | David Leveille, "When you call someone a Sherpa, what exactly does that mean?", The World, 25 Apr 2014 · https://theworld.org/stories/2014/04/25/when-you-call-someone-sherpa-what-exactly-does-mean-2 | radio news · read | Sherpa as a people, not a job |
| S24 | Kathmandu Post column, "Sherpas should step out of the shadows", 25 Feb 2021 · https://kathmandupost.ekantipur.com/columns/2021/02/25/sherpas-should-step-out-of-the-shadows | Nepali newspaper · search only | Sherpa voices on how they are shown |
| S25 | Wikipedia, "Sherpa language" · https://en.wikipedia.org/wiki/Sherpa_language | encyclopedia · read | Sherpa is a Tibetic language, mostly spoken |
| S26 | Prazon Parajuli, "Far from perfect", Kathmandu Post, 25 Feb 2015 · https://kathmandupost.com/art-entertainment/2015/02/25/far-from-perfect | Nepali review of Far Cry 4 · read | criticism of a game's claimed Nepali culture |
| S27 | Game Developer, postmortem of *Never Alone* (E-Line Media and Upper One Games) · https://gamedeveloper.com/audio/postmortem-e-line-media-and-upper-one-games-i-never-alone-i- | industry · search only | community collaboration as a model |
| S28 | Fontsource, "Noto Serif Tibetan" · https://fontsource.org/fonts/noto-serif-tibetan/about ; Fontsource, "Jomolhari" · https://fontsource.org/fonts/jomolhari/about ; DigitalTibetan, "Tibetan fonts" · https://digitaltibetan.github.io/DigitalTibetan/docs/tibetan_fonts.html | font listings · search only | OFL 1.1 licences; Qomolangma fonts non-commercial |
| S29 | Rubin Museum, "What are Tibetan Buddhist shrine rooms?" · https://rubinmuseum.org/what-are-tibetan-buddhist-shrine-rooms/ | museum · search only | home shrine layout |
| S30 | Termatree, "The seven water bowl offering" · https://www.termatree.com/blogs/termatree/the-seven-water-bowl-offering | shop blog · search only | water bowls filled in the morning, not offered empty |
| S31 | Background on lhatho and labtse (shrines for local gods): Ladakh Studies paper (Dollfus) · https://ladakhstudies.org/wp-content/uploads/2023/07/rrol8_11_dollfus_ocred_compressed.pdf ; "On local mountain deities" · https://halshs.archives-ouvertes.fr/halshs-00717941v1/file/On_local_mountains_deities.pdf | scholarly · search only | related words for the altar |
| S32 | Khumbule, "Traditional Sherpa fashion" · https://khumbule.com/?p=261 | Khumbu website · search only | angi, striped aprons, pangden for married women, silver buckle |

**Project files read:** `Docs/Development/Chapter1_v2_Design.md`, `Chomolungma_Chapter1.md`, `Chapter_Design_v1.md`,
`Chapter1_Asset_List.md`, `Chapter1_Asset_Prompt.txt`, `Content/Story/Chapter1.ygs`, `Tools/Story/build_chapter1_world.py`,
`Tools/Materials/make_prayer_flag.py`, `Art/PrayerFlags/T_PrayerFlag_Blue.png` and `_Red.png`.
