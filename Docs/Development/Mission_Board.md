# Mission board (2026-11-10)

Everything you download goes into the project's **`F:\UE_project\YggdrasilsPeak\Downloads\`** folder (one sub-folder per pack), never C:. Fab items: add them in the editor with the Fab plugin as before, then tell me the folder names they landed in.

---

## YOUR MISSIONS

### Mission 1: Sounds (full list with numbers: `Docs/Development/Sound_and_Forest_Download_List.md`, section "Sounds")
The game ships on Steam, so each sound must allow **commercial use**. CC0 is easiest; CC-BY is fine (we credit the author). Avoid "non-commercial" and "no derivatives". Please keep the page link and licence name for each one (a text file in the same folder is enough).

Format: WAV, 44.1 or 48 kHz. Loops must join without a click. Most wanted first:

| Priority | What | Describe it like this when you search | Why |
|---|---|---|---|
| 1 | **Mountain wind, light** (loop, 1-3 min) | steady high-altitude wind, no birds, no trees, no traffic | The bed under almost all play time |
| 2 | **Blizzard / gale** (loop) | roaring storm wind with gusts, *no thunder baked in* | Jotunheim storm |
| 3 | **Boots on crunchy dry snow**, walking + running (8+ steps each) | heavy boots, not sneakers, dry cold snow squeak/crunch | Every step you take |
| 4 | **Crampons on hard ice** (8 steps) | metal points biting into ice | Icefall, Lhotse Face |
| 5 | **Ice cracking / glacier groan** (6-10 + 4-6) | sharp cracks; deep slow creaks | Icefall danger; replaces my synthesized placeholders |
| 6 | **Distant avalanche / serac collapse** (3-4) | long rumble, far away | Icefall set piece |
| 7 | **Heavy breathing, calm + exhausted** (loops) | altitude breathing, not sports panting | Body / mind system |
| 8 | **Carabiner clip, rope sliding** (4 + 3) | metal click, rope through hand/jumar | Fixed ropes |
| 9 | **Fine ratchet ticks slowing + one soft chime** | like a wheel of fortune slowing, elegant | The weather wheel |
| 10 | Thunder (far + close), whispers, distant footsteps behind you, heartbeat, tent zip, prayer flags flapping | see the list file | Atmosphere, story |

Good places: **Sonniss GDC bundles** (free, made for games), **BigSoundBank**, **Freesound** (filter by CC0), **Kenney UI Audio** (CC0). Read Mixkit's licence before using it.

### Mission 2: Forest (Fab)
Fantasy world, so trees may grow lower in the valleys around Dingboche and Pheriche. Look: **Himalayan / alpine conifers in snow**, not a green summer forest.

| What | Details |
|---|---|
| **Snow-covered conifers** (most important) | Pine / fir / spruce, 3-5 variants, 8-30 m tall, with snow on the branches |
| Same trees without snow | For sunnier lower slopes |
| Dead trees, stumps, fallen logs | Near the tree line, for mood |
| Low alpine shrubs (juniper, rhododendron) | Between rocks at the tree line |
| Dry alpine grass / tussocks | Valley floor |

Must have for performance: **Nanite** (or good LODs/billboards), **wind animation** if possible, textures 2K is enough, UE 5.4+ listed. Search words: `snow pine`, `winter forest`, `snowy conifer`, `alpine trees`, `Megascans Nordic forest`, `fir tree nanite`.

### Mission 3: Ice for the Icefall (Fab / Quixel Megascans) - NEW
I will build the ice blocks myself as **solid closed meshes** (no more hollow backs). What I need from you is **real ice surfaces** to put on them. Please get these from Fab (Quixel Megascans surfaces are free on Fab):

| What | What it must look like | Used for |
|---|---|---|
| **Blue glacier ice / ice wall** surface | Translucent blue-white ice with **layers / bands** and vertical streaks, like the cut face of a serac | Vertical faces of seracs and blocks |
| **Compacted old snow / firn** surface | White-grey, slightly dirty, wind-polished, not fluffy | Tops of the ice blocks, snow between blocks |
| **Dirty / weathered glacier ice** surface | Grey-white ice with dust, small cracks, some debris | Lower Icefall, block bottoms |
| **Ice rubble / broken ice chunks** surface (optional) | Crushed ice pieces | Debris at the foot of towers |

For each: download **2K** (4K is fine, I shrink it), with **Albedo, Normal, Roughness, Displacement/Height** maps. Search: `glacier ice`, `ice wall`, `blue ice`, `firn`, `compacted snow`, `dirty ice`.

Optional, only if they are SOLID: Megascans **3D ice boulders / ice chunks** (photogrammetry meshes). Before downloading, rotate the 3D preview and check the **back and the bottom are closed**. **Do NOT download Gaussian-splat scans** (the hollow ones we deleted).

### Mission 4: Play-test later
When the Icefall test patch is ready I will give you one combined list of everything to check in a single play session (old play-test items + today's fixes: boots on snow/rock, footprints on the new terrain, HUD, realms).

---

## MY MISSIONS (in this order)

1. **Icefall from scratch, done carefully**:
   - Research the real Khumbu Icefall (photos, how seracs and blocks form, size, spacing, colours) and pick **one reference photo**.
   - Build **solid** ice blocks with the Blender pipeline, arranged by glacier logic (rows across the flow, towers with rubble at their feet, snow in the gaps, crevasses), with hidden faces truly closed.
   - Make **one 100 m test patch** beside the route, compare it with the reference **from walking height**, and keep improving until it is **70-80% like the photo**. Only then fill the route.
   - Until your ice surfaces arrive I use the Icelandic Glacier surface already in the project.
2. **Combined play-test list** for you (Mission 4).
3. Waiting for your go (on the to-do list): Display Settings menu (footprints Off / Normal / High), real height for footprints, close-up rock detail + blocky gravel texture.
4. When your downloads are in: wire sounds into the game, scatter the forest.
