# Chapter 1 assets for the Meshy pipeline (round 1). See Docs/Development/Chapter1_Asset_List.md.
# Colours and variants (tent colours, duffel colours) are made in Unreal, not generated again.
# Each entry: id, kind ("prop" or "char"), text for the concept image, real size in metres (largest side, or height for
# characters), target triangle count for the 3D model.

PROP_STYLE = ("Photoreal product photograph of {what}. A single object, isolated, the whole object fully visible and "
              "centred in the frame, three-quarter view from slightly above, on a plain light grey seamless studio "
              "background, soft even diffuse light, no harsh shadows. Authentic, used and weathered, as seen on the "
              "Everest trek in the Nepal Himalaya. No text, no logos, no people.")

CHAR_STYLE = ("Photoreal full-body character reference photo of {what}. Standing in an A-pose: arms straight and 45 "
              "degrees down from the body, palms facing in, feet slightly apart. Facing the camera, front view, the "
              "whole body visible from head to boots, plain light grey studio background, soft even light. Realistic "
              "skin and fabric, used and weathered clothes. No props, no text, no logos.")

TEX_STYLE = ("A flat, straight-on, evenly lit photograph of {what}. The cloth fills the whole frame edge to edge, no "
             "background, no shadows, no perspective, no folds, high detail of the cotton weave and the ink.")

ASSETS = [
    # ---------------------------------------------------------------- camp
    dict(id="tent_dome", kind="prop", size=2.2, tris=25000,
         what="a two-person four-season geodesic expedition dome tent in faded orange, grey poles crossing outside the "
              "fly, a vestibule at the front with a closed zipped door, a dark grey snow skirt along the base, no guy "
              "lines and no stakes. The upper fabric is sun-faded and fairly clean; light dry grey-brown dust on the "
              "lower part, uneven from 5 to 20 cm high, higher at the corners and around the door, a few faint water "
              "stains on the side"),
    dict(id="tent_mess", kind="prop", size=6.0, tris=45000,
         what="a large geodesic expedition dining dome tent about 6 metres across, faded yellow and grey triangular fabric "
              "panels with taut smooth fabric between the seams, one large zipped door, NO windows at all, the base weighted "
              "with a ring of rocks, no guy lines, sun-faded top, light dry dust low on the walls"),
    dict(id="tent_kitchen", kind="prop", size=4.0, tris=20000,
         what="a tall rectangular expedition kitchen tent of heavy faded blue tarp over a metal frame, ridge roof, "
              "smoke-stained grey near the top, one side rolled up, standing on a low dry-stone wall, dusty"),
    dict(id="tent_toilet", kind="prop", size=2.0, tris=8000,
         what="a narrow tall pop-up toilet tent of dull blue nylon, square footprint, a closed zipped door, dusty "
              "base, no guy lines"),
    dict(id="duffel", kind="prop", size=0.85, tris=6000,
         what="a large 100-litre waterproof expedition duffel bag in yellow coated fabric, packed full and lumpy, "
              "compression straps, carry handles and end handles, scuffed and dusty, lying on its side"),
    dict(id="barrel_blue", kind="prop", size=0.62, tris=4000,
         what="a blue plastic 60-litre expedition drum with a black screw clamp-ring lid, scuffed and dusty, a grey "
              "cargo strap around its middle"),
    dict(id="bedding", kind="prop", size=2.1, tris=8000,
         what="a rumpled red mummy down sleeping bag, half unzipped, lying on a yellow closed-cell foam sleeping mat"),
    dict(id="porter_load", kind="prop", size=0.9, tris=10000,
         what="a Nepali porter's load standing upright on its own: three large expedition duffel bags in yellow, "
              "blue and red stacked on top of each other and tied together with thin rope, a flat woven forehead "
              "strap (namlo) looped from the rope, a folded beige cloth on the strap"),
    dict(id="doko", kind="prop", size=0.8, tris=10000,
         what="a conical woven bamboo doko basket used by Nepali porters, wide at the top and narrow at the base, "
              "filled with a metal kerosene can and small wooden crates, a flat woven strap attached"),
    dict(id="stove_set", kind="prop", size=0.5, tris=8000,
         what="a brass kerosene pressure camp stove with a large dented aluminium cooking pot on it and a sooty "
              "aluminium kettle beside it"),
    dict(id="cook_set", kind="prop", size=1.0, tris=10000,
         what="an expedition camp kitchen set: a two-burner gas stove on a low metal stand, large dented aluminium "
              "cooking pots stacked beside it and a pressure cooker"),
    # ---------------------------------------------------------------- the memorials at Thukla
    dict(id="chorten_a", kind="prop", size=2.0, tris=20000,
         what="a Buddhist memorial chorten for a climber who died on Everest, built of stacked dry grey granite "
              "stones in stepped square tiers, about 2 metres tall, patches of pale green and orange lichen, a carved "
              "stone plaque set into the front face, at the Thukla pass in Nepal"),
    dict(id="chorten_b", kind="prop", size=1.2, tris=15000,
         what="a small rough memorial cairn of stacked flat grey granite stones, about 1.2 metres tall, lichen "
              "patches, a thin wooden stick on top with a few faded prayer-flag scraps"),
    dict(id="chorten_c", kind="prop", size=3.0, tris=25000,
         what="a large stepped stone memorial chorten about 3 metres tall, old whitewash mostly weathered off to grey "
              "granite, a small metal plaque on the front, a wooden pole on top for prayer flags"),
    dict(id="father_stone", kind="prop", size=0.8, tris=10000,
         what="a freshly cut grey granite memorial stone with sharp clean edges, one large flat smooth blank face "
              "with no inscription, set on a small base of stacked rough stones, about 80 cm tall"),
    dict(id="khata_folded", kind="prop", size=0.4, tris=6000,
         what="a white silk khata, a long Tibetan ceremonial scarf of very thin, slightly sheer silk with a subtle "
              "woven pattern, folded into a long flat strip about 40 cm long and 12 cm wide, thin like paper, not "
              "thick like a towel, the fringed ends hanging loose over one edge"),
    dict(id="khata_tied", kind="prop", size=1.2, tris=6000,
         what="a white silk khata, a Tibetan ceremonial scarf with a subtle woven pattern, tied in a loop with a "
              "simple knot as if around a 30 cm post, the loop empty, the long fringed ends hanging down and curling "
              "in the wind"),
    # ---------------------------------------------------------------- the Sherpa house kitchen
    dict(id="copper_drum", kind="prop", size=0.6, tris=6000,
         what="a large traditional Sherpa copper water vessel with a wide round belly and a short neck, dented, dark "
              "aged patina with brass bands around it"),
    dict(id="offering_bowl", kind="prop", size=0.08, tris=1500,
         what="a small plain polished brass Tibetan offering water bowl, 8 cm across"),
    dict(id="butter_lamp", kind="prop", size=0.12, tris=2500,
         what="a footed Tibetan brass butter lamp on a short stem with a round cup, wax-stained and tarnished"),
    dict(id="choktse", kind="prop", size=1.2, tris=6000,
         what="a long low Tibetan wooden table (choktse) with painted floral side panels, smoke-darkened and worn, "
              "120 cm long and 35 cm high"),
    dict(id="meat_slats", kind="prop", size=1.0, tris=8000,
         what="strips of dried dark yak meat hanging over a horizontal wooden slat, with two strings of small hard "
              "dried white cheese cubes (chhurpi) hanging beside them"),
    dict(id="dung_heap", kind="prop", size=1.0, tris=8000,
         what="a low heap of dried round yak dung patties stacked as cooking fuel, brown and grey, dry and cracked"),
    dict(id="tarchen", kind="prop", size=5.0, tris=6000,
         what="a tall wooden Buddhist prayer pole with a long faded vertical cloth prayer banner with woodblock-printed "
              "patterns, on a small base of stacked stones"),
    # ---------------------------------------------------------------- accessories
    dict(id="glacier_glasses", kind="prop", size=0.14, tris=3000,
         what="a pair of wraparound mountaineering glacier sunglasses with dark lenses and leather side shields"),
    dict(id="headlamp", kind="prop", size=0.2, tris=3000,
         what="an outdoor headlamp with a black elastic head strap forming a loop"),
    dict(id="water_bottle", kind="prop", size=0.21, tris=2000,
         what="a 1-litre wide-mouth plastic water bottle in a padded grey insulated sleeve, dusty"),
    dict(id="daypack", kind="prop", size=0.6, tris=8000,
         what="a 40-litre trekking backpack in faded blue and grey, padded hip belt, compression straps, dusty, "
              "standing upright"),
    # ---------------------------------------------------------------- trail
    dict(id="stream_hole", kind="prop", size=1.5, tris=10000,
         what="a patch of frozen mountain stream about 1.5 metres across: a rough shelf of clear and white ice with "
              "an opening in the middle showing dark running water underneath, frosty gravel around the edge"),
    # ---------------------------------------------------------------- people (whole characters, rigged)
    dict(id="dolma", kind="char", size=1.25, tris=40000,
         what="an 8-year-old Sherpa girl from the Nepal Himalaya with realistic child proportions, a round face with "
              "red wind-chapped cheeks and two black braids, wearing a faded fuchsia puffer jacket one size too big "
              "with the sleeves past her wrists, navy track pants with two thin white side stripes, a striped red, "
              "teal and yellow knit hat with ear flaps, and small purple hiking boots; her clothes slightly dusty"),
    dict(id="father", kind="char", size=1.68, tris=50000,
         what="a Sherpa man of about 48 from Nepal with a weathered face and short black and grey hair, wearing a red "
              "one-piece 8000-metre expedition down suit with the hood down, a climbing harness at the waist, and "
              "charcoal and slate-blue high-altitude mountaineering boots with built-in gaiters; no oxygen mask"),
    dict(id="sangye", kind="char", size=1.70, tris=50000,
         what="a Sherpa man of about 22 from Nepal with short black hair, wearing a red down jacket with a darker "
              "patch sewn on the left elbow, dark grey softshell trekking trousers, thin black gloves and brown "
              "mid-height trekking boots"),
    # 2026-11-20 night: whole AI characters for the main team (the MetaHuman garments looked bad; user wants polish tonight)
    dict(id="nima_ai", kind="char", size=1.68, tris=50000,
         what="a Sherpa mountain guide of about 45 from Nepal with a weathered sun-darkened face, short black hair with some "
              "grey, a little stubble, wearing a faded mustard-yellow hooded down jacket a few seasons old with charcoal "
              "shoulder panels, a slate-grey fleece collar showing at the neck, dark grey softshell trekking trousers worn "
              "lighter at the knees, a faded maroon knit beanie, glacier glasses pushed up on the beanie, and dark brown "
              "leather trekking boots; real worn fabric with stitching and creases, no logos"),
    dict(id="marek_ai", kind="char", size=1.80, tris=50000,
         what="a Slovak expedition leader in his late 40s, fair sunburnt skin on the nose and cheeks, short brown hair and a "
              "short untidy beard, wearing a well-used dark olive insulated hooded jacket with abrasion at the cuffs, "
              "charcoal softshell trekking trousers, a charcoal knit beanie, glacier glasses with side shields on a cord "
              "around the neck, and brown leather hiking boots; real worn technical fabric, no logos"),
    dict(id="elena_ai", kind="char", size=1.66, tris=50000,
         what="a Spanish woman of about 35 on her first Himalayan trek, light olive skin with a slightly sunburnt nose, "
              "dark brown hair tied back in a low ponytail showing under a teal knit beanie, wearing a faded red rented down "
              "jacket one size too big with sleeves slightly too long, a teal fleece collar at the neck, mid-grey trekking "
              "trousers and clean new grey-and-teal hiking boots; real fabric texture, no logos"),

    # ================================================================ round 2 (2026-11-21, new account, 1,250 credits)
    # Only items with NO free Fab / Megascans / Poly Haven replacement (user rule). Characters get MetaHuman heads later
    # (head swap), so every person is bare-headed with hair tied up off the shoulders and the collar open round the neck;
    # hats and helmets are separate props.
    dict(id="pema_body", kind="char", size=1.58, tris=50000,
         what="a Sherpa woman of about 45 from the Khumbu in Nepal, black hair pulled back tightly into a small low bun, nothing "
              "on her head, wearing a traditional dark maroon wool chuba wrap dress to the ankles, a pink silk blouse with the "
              "collar open at the neck, a striped rainbow pangden apron with thin horizontal stripes of red, green, yellow, blue "
              "and white tied at the waist, an old grey zip fleece open over it, a woven belt and black flat cloth shoes; worn "
              "everyday clothes, dust at the hem, real wool and silk texture, no logos"),
    dict(id="sangye_climb", kind="char", size=1.70, tris=50000,
         what="a Sherpa man of about 22 from Nepal, short black hair, bare head, wearing a red down jacket with a darker patch "
              "on the left elbow, collar open at the neck, a black climbing harness with slings and carabiners, dark grey "
              "softshell trousers, thin black liner gloves and black-and-orange insulated double mountaineering boots with "
              "steel crampons; used and dusty, no logos"),
    dict(id="nima_climb", kind="char", size=1.68, tris=50000,
         what="a Sherpa mountain guide of about 45 from Nepal, short black hair with some grey, bare head, a faded mustard-yellow "
              "down jacket a few seasons old with charcoal shoulder panels, collar open at the neck, a climbing harness with "
              "carabiners and a mechanical ascender, worn grey softshell trousers, black double mountaineering boots with "
              "steel crampons, black mittens hanging on wrist cords; dirty and worn, plain unbranded fabric with no logo, no text and no badge anywhere on the jacket"),
    dict(id="climber_icefall", kind="char", size=1.78, tris=50000,
         what="a mountaineer in a faded orange down jacket and black insulated trousers, a climbing harness, a white climbing "
              "helmet with a headlamp, a black neck gaiter pulled over the nose and dark goggles hiding the whole face, black "
              "double mountaineering boots with crampons, big black mittens; dirty and worn, no logos"),
    dict(id="hale", kind="char", size=1.68, tris=50000,
         what="a Norwegian woman doctor of about 52, fair weather-reddened skin, ash-blond and grey hair tied back in a short low "
              "bun, bare head, reading glasses on a cord tucked into the jacket, a slate-blue down jacket open over a grey "
              "fleece with the collar open at the neck, dark softshell trousers, insulated brown boots, a stethoscope in a "
              "jacket pocket; used and dusty, no logos"),
    dict(id="lama", kind="char", size=1.66, tris=50000,
         what="an elderly Sherpa Buddhist lama of about 60 from Nepal, calm weathered face, close-cropped grey hair, bare head, "
              "maroon monastic robes with a saffron sleeveless shirt, a maroon shawl over the left shoulder, a thick maroon "
              "down jacket open over the robes, old brown trekking boots; real wool texture, dust at the hem"),
    dict(id="pasang_cook", kind="char", size=1.65, tris=50000,
         what="a Sherpa expedition cook of about 50 from Nepal, round sun-darkened face, short black-grey hair, bare head, an old "
              "red down vest over a faded navy fleece with the collar open, a stained white cotton apron, dark track "
              "trousers, worn trekking shoes; dirty and used, no logos"),
    dict(id="diki", kind="char", size=1.56, tris=50000,
         what="a Sherpa woman of about 40 who runs a trekking lodge in the Khumbu, red wind-chapped cheeks, black hair pinned up "
              "in a bun, nothing on her head, a dark maroon wrap dress over a pink blouse with the collar open, a striped "
              "rainbow pangden apron, an open grey fleece, black shoes; everyday worn clothes, no logos"),
    dict(id="lodge_main", kind="prop", size=14.0, tris=60000,
         what="a two-storey Khumbu trekking lodge in the Nepal Himalaya, walls of rough grey dry-laid stone, small blue-painted "
              "wooden windows with many small panes, a low-pitched faded blue corrugated tin roof weighted down with rocks, "
              "two solar panels and a black plastic water tank on the roof, a wooden door, a stone terrace in front, no "
              "signboard; the whole building visible"),
    dict(id="lhapso_altar", kind="prop", size=1.6, tris=20000,
         what="a square Sherpa puja altar (lhapso) about 1.6 metres tall built of stacked flat grey granite stones without "
              "mortar, a soot-blackened fire niche in the front, a flat stone offering shelf on top, a short wooden pole "
              "socket in the middle of the top, standing on grey glacier rubble"),
    dict(id="sang_burner", kind="prop", size=1.2, tris=15000,
         what="a Sherpa juniper incense burner: a whitewashed stone-and-clay altar about 1.2 metres tall, square, slightly "
              "tapering, with a smoke-blackened fire bowl on top full of grey ash and a few green juniper sprigs, the "
              "whitewash stained and flaking"),
    dict(id="crampons", kind="prop", size=0.35, tris=12000,
         what="a pair of steel 12-point mountaineering crampons with long front points, orange anti-balling plates under the "
              "soles, black nylon straps with metal buckles, lying side by side, scratched and dull metal"),
    dict(id="ice_anchor", kind="prop", size=0.3, tris=8000,
         what="a steel tubular ice screw drilled into a small block of blue glacier ice, a bent metal hanger, a short faded "
              "red sewn sling clipped to it with a steel carabiner, frost on the metal"),
    dict(id="carabiner_sling", kind="prop", size=0.6, tris=8000,
         what="a climber's safety leash: a short grey sewn nylon sling with two steel locking carabiners and a mechanical "
              "rope ascender with a dark grey rubber handle, worn and scratched, lying flat"),
    dict(id="climbing_helmet", kind="prop", size=0.3, tris=8000,
         what="a scuffed white mountaineering helmet with a black headlamp strapped round it, the chin strap hanging open"),
    dict(id="load_pack", kind="prop", size=0.85, tris=15000,
         what="a 70-litre expedition rucksack in faded red, packed full, two orange oxygen cylinders strapped upright on the "
              "back, a coiled rope under the lid strap, a yellow foam mat strapped on the side, dusty, standing upright"),
    dict(id="puja_offerings", kind="prop", size=0.8, tris=15000,
         what="a Sherpa puja offering spread on a low wooden tray covered with a white cloth: a brass bowl of roasted barley "
              "flour, a bowl of rice, plain unlabelled beer cans, an unlabelled bottle, a packet of biscuits, apples and "
              "oranges"),
    dict(id="gamow_bag", kind="prop", size=2.1, tris=12000,
         what="a portable hyperbaric chamber for altitude sickness: a 2.1 metre long red-and-grey coated nylon tube with a "
              "clear round window at the head end, half inflated, lying on a foam mat, a foot pump with a hose beside it"),
    dict(id="hanging_stove", kind="prop", size=0.5, tris=10000,
         what="a compact high-altitude canister stove hanging from three thin steel cables: a tall insulated aluminium pot "
              "with a lid on a small burner, a squat grey gas canister under it, soot and frost, no logos"),
    dict(id="double_boots", kind="prop", size=0.4, tris=12000,
         what="a pair of high-altitude mountaineering double boots: stiff yellow-and-black insulated shells with a gaiter "
              "zip, grey foam inner boots half pulled out, frost on the shells, standing side by side"),
    dict(id="flag_blue", kind="tex", size=0.3, tris=0,
         what="a single rectangular Tibetan lungta prayer flag of thin blue cotton cloth, block-printed in black ink: a wind "
              "horse carrying a flaming jewel in the centre, a tiger, a snow lion, a garuda and a dragon in the four corners, "
              "and rows of worn, faded block-printed lines around them so smudged that no letter can be read; sun-faded, "
              "frayed edges, a narrow folded hem along the top"),
    dict(id="flag_white", kind="tex", size=0.3, tris=0,
         what="a single rectangular Tibetan lungta prayer flag of thin white cotton cloth, block-printed in black ink: a wind "
              "horse carrying a flaming jewel in the centre, a tiger, a snow lion, a garuda and a dragon in the four corners, "
              "and rows of worn, faded block-printed lines around them so smudged that no letter can be read; sun-faded, "
              "frayed edges, a narrow folded hem along the top"),
    dict(id="flag_red", kind="tex", size=0.3, tris=0,
         what="a single rectangular Tibetan lungta prayer flag of thin red cotton cloth, block-printed in black ink: a wind "
              "horse carrying a flaming jewel in the centre, a tiger, a snow lion, a garuda and a dragon in the four corners, "
              "and rows of worn, faded block-printed lines around them so smudged that no letter can be read; sun-faded, "
              "frayed edges, a narrow folded hem along the top"),
    dict(id="flag_green", kind="tex", size=0.3, tris=0,
         what="a single rectangular Tibetan lungta prayer flag of thin green cotton cloth, block-printed in black ink: a wind "
              "horse carrying a flaming jewel in the centre, a tiger, a snow lion, a garuda and a dragon in the four corners, "
              "and rows of worn, faded block-printed lines around them so smudged that no letter can be read; sun-faded, "
              "frayed edges, a narrow folded hem along the top"),
    dict(id="flag_yellow", kind="tex", size=0.3, tris=0,
         what="a single rectangular Tibetan lungta prayer flag of thin yellow cotton cloth, block-printed in black ink: a wind "
              "horse carrying a flaming jewel in the centre, a tiger, a snow lion, a garuda and a dragon in the four corners, "
              "and rows of worn, faded block-printed lines around them so smudged that no letter can be read; sun-faded, "
              "frayed edges, a narrow folded hem along the top"),
    dict(id="knit_beanie", kind="prop", size=0.25, tris=6000,
         what="a plain cuffed knit wool beanie hat in neutral grey, ribbed knit with a folded-up cuff, no pompom, no logo, "
              "slightly worn and pilled, shaped as if worn on a head"),
]
ROUND2 = [a["id"] for a in ASSETS[ASSETS.index(next(x for x in ASSETS if x["id"] == "pema_body")):]]

BY_ID = {a["id"]: a for a in ASSETS}


def concept_prompt(a):
    return {"char": CHAR_STYLE, "tex": TEX_STYLE}.get(a["kind"], PROP_STYLE).format(what=a["what"])
