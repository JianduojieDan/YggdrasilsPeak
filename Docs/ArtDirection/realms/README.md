# Nine Realms — prototype status (2026-11-04)

Reference images: `01_Midgard.png` … `09_Asgard.png` (from the Nine Realms brief). Comparison sheets: `realm_sheet_latest2.jpg`, summit view: `summit_all_nine_v2.png`.

## How to try it

In Play mode, open the console (`~`) and type:

| Command | What you see |
|---|---|
| `ygg.Realm Midgard` | blue Earth, Asia facing the summit |
| `ygg.Realm Niflheim` | ivory frost planet; **white fog fills the valleys** |
| `ygg.Realm Muspelheim` | charcoal-red planet with one glowing fissure; **copper sunlight** on exposed faces; **glowing ember flakes** drift around you |
| `ygg.Realm Jotunheim` | giant slate planet with a storm bank; **heavy cloud, ice-blue flashes, thunder 3–5 s later** |
| `ygg.Realm Alfheim` | small soap-film planet, nearly invisible in daylight; **pale prism bow up the Western Cwm, only within 120 m of one spot just past Camp 1** |
| `ygg.Realm Vanaheim` | marbled jade gas giant; **green/violet aurora** (best at night) |
| `ygg.Realm Svartalfheim` | small black disk with a silver ring; **eclipse: sunlight drops to ~6 %** and returns |
| `ygg.Realm Hel` | indigo planet with its broken river-of-light trail; **thin blue-white starfall frost** |
| `ygg.Realm Asgard` | giant gold planet; **golden light pillar over Camp 2** |
| `ygg.Realm all` | the summit payoff: all nine together |
| `ygg.Realm route` | prototype: the realm changes at each route landmark (0 Midgard, 1 Niflheim, 2 Vanaheim, 3 Muspelheim, 4 Alfheim, 5 Asgard, 6 Jotunheim, 7 Svartalfheim, 8 all). Order is a placeholder, not a design decision; Hel isn't on it |
| `ygg.Realm none` | clear sky, everything restored |

Planets fade in/out over ~6 s and stay centred on the player.

## Mood and effects

One table in `RealmSky.cpp` (`LookOf`) sets each realm's sun, sky-light (shadow) colour, fog, colour grade and per-second effects on cold, stamina and sanity. Changes glide over about 10 s. In route mode Hel arrives when sanity drops below 30 and leaves above 40. All values are easy-to-change defaults.

## Sizes (angular diameter, single sighting)

Giants: Jötunheim 55°, Asgard 46° · Middle: Niflheim 34°, Vanaheim 30°, Midgard 26°, Muspelheim 24°, Hel 20° · Small: Alfheim 16°, Svartalfheim 11°.

## Not built yet

- Hel: the parents' object
- Final route order for the realms (current order is a placeholder)
- Asgard pillar verified live from Camp 1 (`Saved/Compare/pie_asgard2.png`); still a little too narrow / white

## Files

`Tools/Realms/` (sphere, placement, sheets, skygate), `Tools/Materials/M_RealmPlanet.hlsl` + `make_realm_planets.py`, `M_LightShaft.hlsl`, `M_RealmMote.hlsl`, `M_PrismBow.hlsl`, `make_realm_fx_materials.py`, `Tools/Realms/make_realm_particles.py` (run over MCP), `place_prism_bow.py`, C++ `Source/YggdrasilsPeak/Environment/RealmSky.*`.
