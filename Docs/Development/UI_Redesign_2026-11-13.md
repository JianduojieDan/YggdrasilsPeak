# UI redesign — 2026-11-13

User's verdict on menus v2: right track, but it reads "AI generated". Death effects not good enough. Backpack, HUD
and menus don't fit together. Redo all of it; the user reviews at the end.

## Why v2 read as AI-made

- The "premium template" kit: hairline frames, letter-spaced small capitals everywhere, an eyebrow line over every
  title, gold accent bars, beads on lines, numbered options (01 02 03), film grain and vignette on everything.
- Poetic micro-copy on every surface ("THE MOUNTAIN WAITS", "THE CLIMBER'S KIT", "THOSE WHO MADE THE WAY").
- Dashboard tropes: three stat columns with tiny tracked labels under big light numerals.
- Effects drawn as clip-art vectors (asterisk snowflakes, line icicles, line cracks) over a static picture.
- Three unrelated styles: editorial menus, thin-outline HUD, paper-cutout backpack.

## The idea: light, not lines

The art direction says the sky is the centre of the game: cloud gaps send light across the mountain; warm light only
on the summit and the upper ridges, near snow stays blue. The UI takes its whole language from that.

- **Selection is light.** Nothing gets a box, bar or bead. The chosen word is lit: shadowed words are cold blue-grey,
  the lit one is warm snow-white with a soft apricot glow behind it, as if a cloud gap opened on it.
- **Backgrounds are weather.** Behind a menu the world goes soft and darkens under drifting cloud shadow (a slow noise
  veil), darkest where the words stand.
- **One voice.** Alegreya (the book serif already in the project) for everything you read, in sentence case, with its
  old-style figures. No tracked capitals. Roboto only inside key caps.
- **Few words.** A title, the options, one quiet line where it says something real. No eyebrows, no numbering, no
  headers and footers, no grain.
- **Effects transform the world, not a picture of it.** Death screens are post-process materials on the live scene
  (frost growing on the lens, the crevasse closing over the sky, snow burying the view...), the words animate on top.

## Palette (sRGB)

| Role | Hex | Use |
| --- | --- | --- |
| Snow | #EEE8DC | lit text, titles |
| Mist | #A9B4C2 | unlit options, quiet lines |
| Shade | #6F7B8C | hints, disabled |
| Ink | #0A1019 | veils (deep night blue, never black) |
| Sun | #F4B884 | the lit word's glow, the one number that matters |
| Ember | #E07A66 | danger (low oxygen, freezing), sparingly |
| Ice | #9CC7E8 | cold states |

## Screens

- **Start**: the captured views drift as now; the title alone, large; options below; Continue says where you are.
- **Pause**: the place you are as the title ("Dingboche"), height and time as one serif line; the route of camps on the
  right as names on a thread, the camp you reached lit.
- **Death**: one word, large, animated by the cause; the cause line; one serif line of facts; three options. The world
  behind turns into the cause (post-process, M_PP_Fate).
- **Summit**: the same layout in warm light.
- **Settings**: tabs as serif words; rows lit like options; sliders as a soft line with a sunlit fill.
- **HUD**: the same type and colours; altitude is the hero (serif numerals); condition as soft lines that only speak up
  when they change or run low.
- **Backpack**: a kit flat-lay instead of paper cut-outs: your gear rendered from the game's own meshes, laid on dark
  cloth, lit from one side; the thing you point at is lit like a chosen option; its page in the menus' type.
