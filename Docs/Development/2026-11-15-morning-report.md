# Morning report, 2026-11-15 (overnight chapter 1 realism)

Full timed log: Docs/Development/2026-11-14-overnight-2.md (items 1-19). Screens: Saved/Compare/shot_story_*.png.
Committed on local main and the code pushed to GitHub (worktree github-code -> origin/main). Editor and game closed.

## What changed
- **Sherpa home** rebuilt from your two photos: dry-stone walls in courses, greyer stones, muted blue door/window
  frames, a board-and-tin roof weighted with stones. The kitchen has smoke-darkened mud plaster, a darker clay stove
  with real downloaded pots, a hood, dongmo churns and rugs, and a red-lacquered altar cabinet (it was chipped white).
  Soft fire and window light: the hard pale wedges on the wall were the hood's shadow from a point-sized fire light.
- **Boots**: the round "flower pots" are gone. Every person wears a real 3D-scanned shoe, fitted to their own foot:
  Garmont boots for Nima, Salomon for Marek and Elena, adidas Terrex for Ji-woo, cheap trainers for Pema and the porter.
  All six are CC BY with credits in Docs/Credits; brand logos must be painted out before a commercial release.
- **Pema** now looks like a Sherpa woman of her age: a knit cardigan over the chuba, a blouse collar, the striped
  pangden apron in front plus the back apron, a sash with a silver buckle over the cardigan, and a printed cotton
  headscarf (the plain one read as a knit cap).
- **A real bug that touched every garment**: each outfit had three UV channels, and UE read only the first, which was
  empty for most pieces. So the apron was a flat orange slab and wool or knit showed no texture. Fixed for all six.
- **Porter**: looser cargo trousers, a flat rolled cuff (it read as a rubber ring), trail dirt on the shoes.
- **NPC bug**: the porters on the trail fell out of the world (shoved 42 m off their spot at spawn). They now stay
  visible on the trail; they still move in small hops instead of walking smoothly there (open).

## Please look at
1. Pema (story_shots c_pema): outfit, scarf, overall look.
2. The kitchen mood and the red altar (room), and the house colour outside (housedoor).
3. The boots in game (c_nima shows Nima's Garmont boots).

## Still open
- Porters advance in hops on that trail stretch (something there blocks walking; the ground there is coarse collision, not the detail tiles).
- Brand logos on the shoe scans. Dolma (child) and the father are not MetaHumans yet (no child body in the Creator).
- Optional from the research: thicker house walls and the door on the long side.
- I did not shut the PC down: the shutdown rule was from the night before.
