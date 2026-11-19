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
]

BY_ID = {a["id"]: a for a in ASSETS}


def concept_prompt(a):
    return (CHAR_STYLE if a["kind"] == "char" else PROP_STYLE).format(what=a["what"])
