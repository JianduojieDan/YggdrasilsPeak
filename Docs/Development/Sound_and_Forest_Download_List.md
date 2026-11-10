# Sound and forest download list (2026-11-10)

Put downloads in the project's `Downloads/` folder (never C:). For every sound, keep the page link and licence name: the game ships on Steam, so the licence must allow commercial use (CC0 is easiest; CC-BY is fine if we credit the author; avoid "non-commercial" and "no derivatives").

Format: WAV, 44.1 or 48 kHz, 16 or 24 bit. Loops must loop cleanly (no click at the join). Mono for point sounds (steps, cracks), stereo for ambience beds.

## Sounds

Today the game uses Ultra Dynamic Sky's generic wind and snow, plus placeholder sounds I synthesized (ice cracks, groans, booms, serac collapse, whispers, footsteps on ice/rubble/ladder). The ones marked **replaces placeholder** are the most important.

### A. Wind and weather beds (loops, 1–3 min each)
| # | What | Notes |
|---|---|---|
| A1 | High mountain wind, light, steady | Main bed for calm weather. No birds, no traffic, no trees rustling. |
| A2 | High mountain wind, strong gusts | Gusts rising and falling, for ridges and the South Col. |
| A3 | Blizzard / gale, roaring | For the Jotunheim storm. Dense, scary, but no thunder baked in. |
| A4 | Wind whistling past rock edges or a tent | Thin, high, occasional. |
| A5 | Near silence, very low air ("room tone" of a still glacier) | For the mist and eclipse realms. |
| A6 | Low distant rumble / earth drone | For Muspelheim (fire). |
| A7 | Soft glassy shimmer or ice chimes, very quiet | For Alfheim / frost realms. Must not sound like a wind chime in a garden. |
| A8 | Very low hum / sub drone, warm | For Vanaheim / Asgard. |

### B. One-shots for weather
| # | What | Notes |
|---|---|---|
| B1 | Distant thunder rolls ×4–6 | Long tails. |
| B2 | Close thunder crack ×3 | |
| B3 | Ember crackle / small fire pops | For Muspelheim; sparse. |

### C. Mountain and ice (replaces placeholder)
| # | What | Notes |
|---|---|---|
| C1 | Ice cracking, sharp ×6–10 | Glacier or frozen lake cracks. |
| C2 | Ice groan / creak, deep ×4–6 | Glacier moving. |
| C3 | Distant avalanche / serac collapse ×3–4 | Long rumble. |
| C4 | Distant mountain boom ×3 | Single deep boom, echo. |
| C5 | Snow bridge collapse / snow slump ×2–3 | Crevasse fall. |
| C6 | Falling ice chips / small debris ×4 | |

### D. Player body (replaces placeholder)
| # | What | Notes |
|---|---|---|
| D1 | Footsteps on crunchy dry snow, walking ×8+ | Boots, not sneakers. |
| D2 | Footsteps on crunchy snow, running ×8+ | |
| D3 | Crampons on hard ice ×8 | Metal points biting ice. |
| D4 | Boots on rock / scree ×8 | |
| D5 | Boots on an aluminium ladder ×6 | Metal ringing, for crevasse ladders. |
| D6 | Heavy breathing, calm and exhausted ×2–3 loops | Male or neutral; for altitude and stamina. |
| D7 | Oxygen mask breathing / regulator hiss ×2 | For O2 bottles. |
| D8 | Down jacket / clothing rustle ×4 | Movement layer. |
| D9 | Body fall into snow ×2 | |

### E. Gear
| # | What | Notes |
|---|---|---|
| E1 | Carabiner clip in / out ×4 each | For fixed ropes. |
| E2 | Rope slide through hand or jumar ×3 | |
| E3 | Ice axe swing and strike into ice ×4 | |
| E4 | Tent zip open / close ×2 | |
| E5 | Prayer flags flapping in wind (loop) | For camps. |

### F. Mind and story (replaces placeholder)
| # | What | Notes |
|---|---|---|
| F1 | Unclear whispers, eerie ×6 | Must not be understandable words. |
| F2 | Distant footsteps in snow behind you ×4 | "Something follows you." |
| F3 | Low heartbeat loop | Fear and altitude. |

### G. Interface
| # | What | Notes |
|---|---|---|
| G1 | Fine ratchet ticks, slowing down | For the weather wheel spin. |
| G2 | One soft, elegant stop / chime | Wheel result. |
| G3 | Soft UI confirm / pickup ×2 | Pickups, compass. |

Places to search: Sonniss GDC bundles (free, royalty-free, made for games), BigSoundBank, Freesound (filter by CC0), Kenney UI Audio (CC0), Mixkit (read its licence).

## Forest (Fab)

You said altitude limits don't apply (fantasy world), so trees can grow lower down the valley and around Dingboche. Look for a Himalayan / alpine look: **conifers in snow**, not a green summer forest.

What to look for:
| # | What | Notes |
|---|---|---|
| T1 | Conifer trees (pine, fir or spruce), **snow-covered versions** | 3–5 species or variants, heights 8–30 m. Most important. |
| T2 | Same conifers without snow (or lightly dusted) | For lower, sunnier slopes. |
| T3 | Dead or broken trees, stumps, fallen logs | Mood and variety near the tree line. |
| T4 | Low alpine shrubs (juniper, rhododendron bushes) | Tree line edge, between rocks. |
| T5 | Dry alpine grass / tussock patches | Valley floor around Dingboche. |
| T6 | Optional: birch trees (bare, winter) | Lighter accents. |

Must have (for the laptop RAM/FPS target):
- **Nanite support**, or proper LODs and impostors / billboards for distant trees.
- **Wind animation** (pivot painter or similar) if possible.
- Textures at 2K is enough (we shrink 4K/8K anyway).
- UE5 version listed as supported (5.4 or newer is safest).
- Free, or a licence you're happy with.

Good search words on Fab: "snow pine", "winter forest", "snowy conifer", "alpine trees", "Megascans Nordic forest" (Quixel's Nordic Forest set is a strong match), "fir tree nanite".

Add them in the editor with the Fab plugin as before; tell me the folder names and I'll set up materials, scatter and draw distances.
