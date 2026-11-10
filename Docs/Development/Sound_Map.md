# Sound map (2026-11-11)

Your recordings are in `/Game/Game_Design/sound_effect`. They were examined, cut and looped by `Tools/Audio/build_game_sounds.py` and imported into `/Game/Audio/...`. The code loads them by name.

**How the sounds were checked**
- `Tools/Audio/inspect_sounds.py` draws contact sheets (spectrogram + loudness) into `Saved/AudioWork/inspect/`.
- Every single sound holds exactly one event and starts with it. Each tail ends at the recording's own noise floor.
- Every loop comes from a clean stretch, with knocks and clicks patched and no audible seam.

## What plays where

| Game moment | Sounds (`/Game/Audio/...`) | Cut from | Code |
|---|---|---|---|
| Walking on snow | `Footsteps/Step_Snow_01-11` | Nox_Sound boots-in-snow recording, each the WHOLE step (foot down, roll, lift off) | SnowTrailComponent |
| Walking on ice / hard crust | `Footsteps/Step_Ice_01-14` | crampons recording (single steps) | SnowTrailComponent |
| Steps that follow you (mind) | `Step_Snow_01-08` | same | SanityComponent |
| Serac zone warning cracks | `Ice/Ice_Crack_01-08` | ice_break_2 (sharp single snaps) + Glacier_Ice_Breaking 3-5 (deep) | SeracCollapseZone |
| Serac zone groans | `Ice/Ice_Groan_01-04` | Glacier_Ice_Breaking 1-2 played at 0.6x speed, low-passed | SeracCollapseZone |
| Serac collapse | `Ice/Serac_Collapse_01-02` | Glacier_Ice_Breaking 1-2 | SeracCollapseZone |
| Distant avalanche (every 4-10 min above 4,900 m) | `Ice/Avalanche_01-05` | the five avalanches (your "good" one is first), far-away filtered, 26 s | MountainAudioComponent |
| Jotunheim thunder | `Weather/Thunder_01-04` | the 4 thunders without rain-on-concrete, each starting at its strike | RealmSky |
| Wind | `Weather/Wind_Calm`, `Wind_Gusty`, `Wind_Blizzard`, `Wind_Whistle` (loops) | natural wind, mountain gusts, blizzard (a clean 2-min stretch), cactus-spine wind | MountainAudioComponent: blended by wind 0-10 and storm; whistle only high up; muffled in a tent |
| Single strong gusts | `Weather/Wind_Gust_01` | ice-spell blizzard blast | MountainAudioComponent (wind above 6) |
| Breathing | `Body/Breath_Phrase_01-06` | breath sequence: its 6 natural stretches kept whole (4.6-8 s), soft to hard | MountainAudioComponent: phrases back to back, harder and louder with effort, silent at rest down low |
| Clip into the fixed rope | `Gear/Carabiner_ClipIn_01-05` (whole gestures, you choose), then `Rope_Tighten_01,03-06` | Timeline_1 gestures, rope_tighten_sequence | MountainAudioComponent |
| Unclip | `Gear/Carabiner_Click_01-06` | Timeline_1 single clicks | MountainAudioComponent |
| Moving clipped to the rope | `Gear/Rope_Slide_Loop` | rope_sliding_1 (clean loop) | MountainAudioComponent |
| Weather wheel spin | `UI/Wheel_Tick_01-06` (one tick per divider passing the pointer) | wheel_spin_calm_1 | SurvivalHUDWidget |
| Weather wheel lands | per weather, more intense = more dangerous: harmless Quiet Sky `UI_Chime_C` (rune bell); Pearl Light, Jade Tide, Golden Hush `UI_Chime_L` (choir) / risky Veil of Mist, Starfall Frost `UI_Chime_E` (bell of the Norns); Emberfall `UI_Chime_H` (bell + horns) / deadly Winter's Veil `UI_Chime_K` (brass + drum); Silver Eclipse `UI_Chime_M` (bowed drone + drum) | made by me (`Tools/Audio/synth_game_sounds.py`), chosen by you over 4 rounds | SurvivalHUDWidget via `ARealmSky::GetLandingSound`; change any pairing with `"Landing"` in `Config/RealmLooks.json` (no rebuild) |

**Not used:**
- `169159` (thunder with water dripping on concrete) and `511348` (rain and thunder). There is no rain at this altitude.
- The rain-hiss lead-in of `712962`; only its strike is used.

The weather system's own wind sound is turned off in play, so the two never double. Its rain and thunder are unchanged.

## Still missing (I will make these myself, free)

| ID | What | Plan |
|---|---|---|
| A5 | Near silence, still-glacier air (mist / eclipse realms) | very quiet filtered noise bed |
| A6 | Low distant rumble drone (Muspelheim) | layered low noise + sub sine, slow swells |
| A7 | Glassy shimmer / ice chimes (Alfheim) | soft high sine partials, random sparkle |
| A8 | Warm low hum (Vanaheim / Asgard) | sine chord with slow beating |
| B3 | Ember crackle (Muspelheim) | random filtered pops |
| C6 | Falling ice chips | short pitched-up cuts of the cracks + tinkles |
| D4 | Boots on rock / scree (still my old placeholder) | crunch from the snow steps + gravel clicks |
| D5 | Crampons on aluminium ladder (still placeholder) | crampon step + metal ring |
| D7 | Oxygen mask / regulator hiss | breath-shaped filtered noise |
| D8 | Down jacket rustle | band-passed noise bursts |
| D9 | Body falling into snow | low thump + snow crunch layer |
| E3 | Ice axe strike into ice | sharp crack cut + metal ping |
| E4 | Tent zip | fast tick train with a filter sweep |
| E5 | Prayer flags flapping | wind-modulated cloth flutter |
| F1 | Whispers (still placeholder) | formant-filtered noise, never real words |
| F3 | Heartbeat loop | two-thump low pulses |
| G3 | UI pickup / confirm | soft click + short bell |

## Test it

- `ygg.Audio.Log 1`: prints the wind mix, effort and rope level every 2 s.
- `ygg.Realm Jotunheim`: storm, wind beds blend to blizzard, wheel ticks + chime, thunder.
- `ygg.Avalanche`: a distant avalanche now.
- Walk on snow, ice and the fixed rope (click when clipping, rope sliding while moving). Climb high to hear breathing.
