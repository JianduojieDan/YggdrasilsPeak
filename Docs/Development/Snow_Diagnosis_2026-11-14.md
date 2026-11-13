# Snow surface: why it reads blurry, and the fix plan (2026-11-14)

User, 2026-11-14: "the rock surface is very detailed, with real height information ... the snow surface in the player's
perspective is so bad because it is blurring, not clear compared to the rock surface. Make it realistic."

This is a read-only investigation: no file other than this one was changed, nothing was built or run. Material values
were read out of the saved `.uasset` files (name table + tagged properties), the shader code was compared byte for byte
with the copies in `Tools/Materials`, and engine behaviour was checked in the UE 5.8 source at `E:\UE5.8\UE_5.8`.

## 0. Short answer

The snow is not blurry because of a low-resolution texture. Its texture is 2048 px and tiles more finely than the rock
texture does. It reads blurry because **the snow carries almost no detail between 1 cm and 2 m**, and none at all as
real height:

1. Up close, the snow texture's grain is mixed in at only **9 %** on the soft snow where the player spends most of the
   game (all of the route below ~6,600 m). The rock texture's normal is used at **125 %**.
2. The snow **colour texture is sampled but thrown away** (dead code). Colour, AO and roughness are flat procedural
   values that only vary over 6-17 m.
3. The procedural forms that are left are all **1.8 m to 27 m wide** and only a few degrees steep. The comment says this
   was on purpose ("no finer detail - small-scale bumps everywhere read as noise", 2026-10-22, "bigger units").
4. The snow has **no height at all** (`SnowDisplacementCm` = 0 in both materials, 1 m tile mesh). The rock the user loves
   is mostly the **scanned Fab rock meshes** on the route (8K photogrammetry, real Nanite geometry). Even the rock
   *texture* on the tiles has no displacement there (`RockDisplacementCm` = 0 in `M_EverestDetailTile`).
5. Making it worse: the user played at DLSS Quality (67 %). Our Custom nodes sample with `SampleGrad` and so **miss the
   engine's upscaling mip bias** (about -0.9 mip at 67 %). Anisotropic filtering is **4x** on High, which blurs flat ground
   seen at a low angle, but not rock faces that face the camera.

Evidence from the user's own session (monitor `Saved/Monitor/20261114_001904`, 4,334 m, Dingboche):
- `shots/0095.1_hitch.jpg` (eye level): the bottom third of the frame (the snow 1-15 m away) is one even white with no
  texture at all. Rock on the far peak and the trees are crisp.
- `shots/0185.8_hitch.jpg` (looking straight down at the snow by the rope): flat grey-white, zero grain. The rope next
  to it is sharp, so this is the material, not the upscaler.
- `shots/0140.0_timer.jpg` (looking down at the trench): the trench walls carry detail. The untouched snow around it is
  smooth.

The plan (section 3) keeps the calm large forms the user chose for the far view. It adds tactile detail only where the
eye can resolve it: cm grain within a few metres, 5-50 cm wind ripples and crust within ~10-15 m, and real
displacement of a few cm on the route tiles, so the sun throws small crisp shadows the way it does on the rock.

---

## 1. Diagnosis, ordered by likely impact

Line numbers refer to `Tools/Materials/M_EverestLandscape_Custom0.hlsl` (identical to the landscape asset's
`MaterialExpressionCustom_0`). The detail tile's colour node (`M_EverestDetailTile`, `Custom_0`) is the same code with
three differences: line 59 uses `N.z*1.0` instead of `*1.774`, a comment line is inserted after line 62 (so tile line =
landscape line + 1 below it), and the ledge line 137/138 is multiplied by `(1.0-saturate(TileRock)*0.8)`. **There is no
repo copy of the tile colour node.** A copy extracted from the asset is at
`C:\Users\danji\AppData\Local\Temp\claude\9e42861fdb5f9175\294420b5-ce33-488b-98cf-7669721ce32d\scratchpad\tile\code_1.hlsl`
(scratch, will not persist). It should be saved as `Tools/Materials/M_EverestDetailTile_Custom0.hlsl` before any edit.

Saved parameter values (read from the assets):

| param | M_EverestLandscape | M_EverestDetailTile |
|---|---|---|
| RockSlopeCutoff / RockSoftness | 0.7 / 0.06 | 0.7 / 0.06 |
| RockTileCm / **SnowTileCm** | 2500 / **600** | 2500 / **600** |
| SnowOnLedges / NormalStrength (NStr) | 0.7 / 1.0 | 0.7 / 1.0 |
| RockDisplacementCm (DispRock) | 340 | **0** (no value saved = class default 0) |
| **SnowDisplacementCm (DispSnow)** | **0** | **0** |
| TileRubbleCm | - | 6 |
| DisplacementScaling magnitude / centre | 0.4204 (x Z scale 951.4 = 400 cm) / 0.5 | 400 / 0.5 |
| bEnableTessellation | true | true |
| Shading model | Default Lit | Default Lit |

The DispRock and DispSnow pins of the tile displacement node are wired to export objects 37 and 38. These are
`RockDisplacementCm` and `SnowDisplacementCm`: the nine scalar parameters are exports 31-39 in asset order, and Cutoff,
Soft, RockTile, SnowTile, Ledge and NStr decode to 31-36 as expected. Decoded from the binary; **check in the editor.**

Textures: snow = `/Game/AzielArts_Material/Textures/ClumpSnow/ClumpSnow_{Color,Height,Normal}`, **2048 x 2048**. Rock =
`AzielArts .../Rock/Rock_{Color,Displacement,NormalDX,ORM}`, 2048 x 2048. `ClumpSnow_ORM` and `ClumpSnow_ORD` exist and
are unused.

### D1. Nothing at the scale the eye resolves at 2-20 m (biggest cause)

The player's view in the session: viewport 2560 x 1600, DLSS Quality 67 %, so the internal image is about 1715 x 1072,
with a 90-degree FOV and the eye about 1.65 m above the snow. The shader's own pixel footprint `fp` (line 158, cm per
pixel on the ground) comes out at roughly:

| distance | fp (67 %) | fp (native) | smallest feature that reads (~4 px) |
|---|---|---|---|
| 2 m | 0.6 cm | 0.4 cm | ~2-3 cm |
| 5 m | 2.5 cm | 1.7 cm | ~10 cm |
| 10 m | 8 cm | 6 cm | ~30 cm |
| 20 m | 30 cm | 21 cm | ~1.2 m |

What the snow has at each scale today:

| scale | source | strength |
|---|---|---|
| 0.1-1.1 cm texels (clumps ~2-10 cm) | ClumpSnow normal, hex-tiled, lines 99-127 | x0.09 on soft snow, x0.30 on hard snow (line 156) |
| **5-50 cm** | **nothing** | - |
| 1.8 m ridges, patchy | `Sast` (lines 53-55, 152-154) | x4 gradient, x0.22 on soft snow, x0.2-1 patch mask |
| 3-9 m | `Lump` / `Clump` (lines 50-51, 159-163) | slopes of ~1-3 degrees |
| 10-27 m | `Pil` (line 48, 148-150) | slopes of ~2-4 degrees |

The **5-50 cm band** is what makes snow look sharp from 3 to 15 m away: wind ripples, crust plates, crumbs, sun cups.
It is completely empty. The 1.8-27 m forms are only a few degrees steep, so they shade as soft gradients. Everything
from the boots to about 15 m therefore reads as smooth gradient: "blurry". This matches `0095.1_hitch.jpg`.

`hard` (line 146) = smoothstep(6600, 7600 m) plus a slope term. It is **0 from Dingboche to Camp 2 on gentle ground**, so
the weakest settings apply there, on most of the route (the user's session was at 4,334 m).

### D2. Snow texture detail is turned almost off; colour, AO and roughness are flat

- Line 156: `micro = lerp(n, snowW, lerp(0.09, 0.30, hard))`: 9-30 % of a texture normal whose own strength is
  `sNStr` = NStr x 0.25..1.7 (about 1.0 typically, line 76). Rock: `tx *= NStr*1.25` (line 93), full strength.
  `Tools/Materials/snow_crisp.py` raised 0.05 to 0.09 on 2026-11-13, which is still invisible (see `0185.8_hitch.jpg`).
- Lines 106 and 127: `sc` / `snowC` (the ClumpSnow colour) are accumulated and **never used**. Line 182 uses `sCol`, a
  constant colour (lines 168-173) varied only by the 6-17 m noise fields `sm1`/`sm3` and the metre-scale `hollow`. The
  layer-B colour `scB` (line 118) is dead too. The compiler strips these samples, so they cost nothing today.
- Line 174: roughness 0.80-0.93, varied only at 6-17 m. Line 192: AO = `lerp(1.0, 0.9, hollow)`, metre-scale only.
  Rock: per-cell tint `CT`, ORM AO and ORM roughness (lines 87-88, 191-192).
- Why it was turned down: user report #1 (2026-10-22) was a regular grid of small round bumps from this **ClumpSnow**
  texture ("clump" snow = round lumps, which is the wrong character for wind-packed Himalayan snow). The per-cell scale
  range 0.35-2.8 (line 12, `Xf`: an 8x spread) also mixes grain sizes into blotches. The texture is the problem, so
  turning the strength down hid all detail along with it.

### D3. No height on snow; the rock the user loves has real geometry

- `Tools/Materials/M_EverestDetailTile_Displacement.hlsl:72-74`: `d = lerp((snowH-0.5)*DispSnow, (rockH-0.35)*DispRock, rm)`
  with DispSnow = 0. The comment says snow bumps "caused sky streaks + FPS drop". That was on the landscape (17 m
  triangles, hardware ray tracing on). The tiles are hidden from ray tracing and have 1 m triangles.
- The tile mesh is a 1 m grid. `Tools/Terrain/build_route_terrain.py:101-106` gives snow only 18 cm at 40 m, 3 cm at
  14 m and a rare 6 cm wind saw every 5 m. Vertex normals are smooth at 1 m.
- Rock on the tiles: DispRock = 0 as well, so only rubble (`TileRubbleCm` 6) and the trail move geometry. The "very
  detailed rock with real height" is the route rock scatter (`Tools/Terrain/editor/rocksroute.py`: Fab
  `stone_photogrammetry` 8192 px textures, `FREE_Stone_-_8K_Textures` 8192 px, `alaskan_cliff_rock_1` 2048 px, Megascans
  `Rocky_Snow_Pile`), with Nanite meshes. Up close they have real edges, so they get crisp VSM micro-shadows. The snow
  gets none.
- Tessellation already runs on the tiles: `bEnableTessellation` is on, magnitude 400 cm, `r.Nanite.DicingRate=4`
  (`Config/DefaultEngine.ini:75`). Nanite dices every tile triangle down to about 4 px whatever the displacement value
  is. So **snow displacement costs mostly shader time, not new geometry.**

### D4. Upscaling softens our Custom-node textures more than the engine's own ones

- Session: `r.ScreenPercentage 67`, DLSS on, frame generation on (`report.md` lines 6-7). Upscaling "Auto" = 75 %, which
  the automatic quality can lower to 50 % (`Source/YggdrasilsPeak/Environment/YggQualitySubsystem.cpp:19,23`).
- With a temporal upscaler the engine sets `View.MaterialTextureMipBias = -log2(1/fraction) - 0.3` (UE 5.8
  `Renderer/Private/SceneVisibility.cpp:5338`, `r.ViewTextureMipBias.Offset` = -0.3): -0.88 at 67 %, -1.3 at 50 %.
  Normal TextureSample nodes add it in generated code (`HLSLMaterialTranslator.cpp:7404-7421`). A texture's own sampler
  (`SnowNrmSampler` etc.) does not include it.
- Our `SampleGrad(..., ddx, ddy)` calls in both terrain Custom nodes therefore pick mips about 0.9 levels blurrier than
  the engine would at 67 %, for both rock and snow. Snow has so little detail that this is enough to lose it. The fix is
  free: multiply the gradients by `View.MaterialTextureDerivativeMultiply` (= 2^bias; `SceneView.h:978`).

### D5. 4x anisotropic filtering hurts flat ground, not rock faces

- `sg.TextureQuality 2` (High, the session value) sets `r.MaxAnisotropy=4` (`Engine/Config/BaseScalability.ini:768`).
  Texture-asset samplers with MaxAnisotropy 0 use that value (`RHIUtilities.h:699-705`).
- At eye height, snow 10 m away is seen at about 9 degrees: an anisotropy ratio of about 6. Past about 6 m the snow
  textures blur along the view direction. Rock faces facing the camera are not affected. This is a real snow-versus-rock
  asymmetry. Epic (`sg 3`) sets 8.

### D6. Lighting choices that flatten what little detail there is

- Lines 188-190 ("shadow-terminator fix"): where the sun grazes (dot(n, sun) < 0.30), the sun-facing part of the normal
  is removed. That cuts the normal-detail contrast exactly in low, raking sun, when real snow shows the most texture.
  It was made for the landscape's **17 m** triangles. The tile copy inherited it, but tiles are 1 m.
- Default Lit at roughness 0.80-0.93: almost no forward-scattered sheen when looking toward the sun. Line 174 says 0.6
  read as metallic at low sun, so it should not go below about 0.68 on large areas. The landscape left Subsurface on
  2026-10-22 because of VSM stripes on 17 m triangles; that reason does not apply to 1 m tiles.
- Sparkle (lines 176-180): 1.2 % of 1.6 cm world cells get a random facet with roughness 0.08. It fades out by fp
  1.0-3.2 cm, about 3-5 m at 67 %. Random normals rarely line up with the sun-eye half vector, so few glints actually
  flash. `hx.Hash` is `frac(sin(...))` on world-scale cell ids: the same precision issue as #55. Use an integer hash.

### D7. Blowing-snow streaks erase the surface where they pass (secondary)

`Tools/Materials/M_EverestLandscape_SnowSnakes.hlsl:22-23` flattens the normal by up to 80 % and whitens it by up to 45 %
inside the streak bands. Strength rises with altitude (`SpindriftComponent.h:50,53`: +35 % from 5,200 m to 8,000 m).
These are literally moving blurred bands. Test with `ygg.Snow.Spindrift 0`.

### D8. Checked and probably not the cause

- **Texture resolution / tiling:** snow texel 0.1-0.8 cm (layer A, 600 cm tile x per-cell 0.35-2.8) and 1.1 cm
  (layer B), versus rock ~1.2 cm. The snow is denser.
- **Streaming:** the landscape asks for full resolution of every texture its material uses (texel factor 0.75 x component
  size, `LandscapeRender.cpp:4281+`). Tiles have degenerate UV0 (`u` = rubble, `v` = 0), so they add no requests, and the
  engine's unknown-ref rule loads recently drawn textures in full (`AsyncTextureStreaming.cpp:314-344`). Pool is 3000 MB.
  Still worth one A/B with `r.Streaming.FullyLoadUsedTextures 1`. Note that **TextureQuality Low** sets
  `r.Streaming.MipBias=16`, which would blur everything; that was the 2026-11-13 "Low/50 %" bug.
- **TrailBlur:** `M_TrailBlur_Custom.hlsl` only blurs the trail render target (R/G), not the surface.
- **Depth of field:** `RealmSky.cpp:646` forces focal distance 0. Body effects were off in the session.
- **Motion blur:** the session shows `r.MotionBlurQuality 5`, but `r.DefaultFeature.MotionBlur=False`
  (`DefaultEngine.ini:99`) and nothing in our code sets `MotionBlurAmount`. Rule it out with one A/B
  (`r.MotionBlurQuality 0`) while turning.
- Side note from `0140.0_timer.jpg`: the trench walls show regular ribbing (vertical striations). This is probably the
  2.5 cm trail-map texels under bilinear `SampleLevel` plus tessellation. A B-spline (bicubic) read of the trail height in
  TrailDisp/TrailShade would smooth it. That is a footprint issue, not part of this item.

---

## 2. What real high-altitude snow looks like (references) and what we lack

Project references: `Docs/ArtDirection/terrain_refs/03_snow_slope_trench_rope.jpg` shows crisp crumbled snow in the boot
trench with hard little shadows, smooth wind-packed snow beside it with fine texture, and clear blue shade.
`05_snow_ridge_cornice_scoops.jpg` shows big smooth wind forms with flutings and scoops, and blue shadow in the scoops.
The art direction asks for the same split: "detail follows attention: clear silhouettes far, tactile detail near",
"the few metres at your feet".

| feature | real scale | where on the route | in our snow now |
|---|---|---|---|
| grain / crust texture, crumbs | grains 1-3 mm, crust texture 1-3 cm, crumbs 2-10 cm | everywhere, read within ~3 m | texture at 9-30 %, wrong (clumpy) character |
| wind ripples | 5-20 cm wavelength, 0.5-2 cm high | soft snow, valleys, Western Cwm | **none** |
| wind crust plates, broken edges | 20-80 cm plates, 1-3 cm steps | exposed ground, Camp 2 and up | **none** |
| sastrugi | 10 cm-1 m high, wavelengths of metres to ~20 m [1] | Lhotse face, South Col, ridge | 1.8 m spacing, ~6 cm, normal only, patchy |
| sun cups | 20-80 cm across, 2-50 cm deep [5] | melting snow on the lower glacier (EBC-Camp 1) | **none** |
| penitentes | 1-4 m (field) [5] | lower Khumbu ice | not on the snow (glacier node and meshes) |
| drift swells | 3-30 m | everywhere | yes (Pil, Lump, Clump) |
| glints | single crystals, flash as you move | sunlit, near | few, random, within ~4 m |
| forward-scattered sheen at grazing angles | strong toward the sun on wind-packed snow [2][3] | sunlit slopes ahead | almost none (roughness 0.8-0.93) |
| blue in cavities and shade | cm-scale in crumbs and ripples, dm in holes | everywhere in low sun | metre-scale `hollow` and trench only |
| albedo | fresh ~0.9, older/wind-packed lower, very low variance [6] | - | 0.86-0.97, fine |

Glints in games: Wang and Bowles 2016 (a jittered sparkle grid, used in a shipped game; the paper also cites Journey's
noisy-normal glints) [4]. Our cheap version (P1-e) does the "noisy normal pointed at the half vector" idea.

Suggested reference set for the user: 4-6 eye-level photos of Everest-route snow at 1-20 m (Western Cwm, Lhotse face
crust, Khumbu sun cups), plus the Wikimedia Commons sastrugi category [7], in `Docs/ArtDirection/snow_refs/`. Downloading
needs the user's OK.

---

## 3. Change plan

Rules carried over: keep the far view calm (the user's 2026-10-22 "bigger units"); keep what the user likes: soft
large-scale shadows on snow, the rock/snow border blend (the `rm` path stays untouched), trail depth, footprints, step
snow. Never touch rock settings without asking (memory: "user furious about rock changes"). Every step below only adds
snow detail, gated by distance. The rock gets sharper only from P0-1/P0-2, which are filtering fixes. Do the tile
material first, judge it, then port it to the landscape.

### P0: free or nearly free fixes (no new assets)

**P0-1 Upscaling mip bias for Custom nodes** (both terrain colour nodes, plus the glacier nodes' `SampleLevel` mips).
After line 72:
```hlsl
// DLSS/TSR render fewer pixels; the engine sharpens its own texture reads by View.MaterialTextureMipBias, a Custom
// node's SampleGrad never gets it - scale our gradients the same way (x0.54 at 67 %, x1 at native without upscaler)
float dmul=View.MaterialTextureDerivativeMultiply;
```
Then use `gx[i]*rt*dmul`, `gy[i]*rt*dmul` (line 80) and `gx[i]*st*dmul`, `gy[i]*st*dmul` (line 99). In the glacier nodes,
add `log2(dmul)` to every hand-made mip (`mip1`, `mip2`, `im1` ...). Cost: about 0, slightly more texture bandwidth.

**P0-2 Anisotropy 8 on High, 16 on Epic.** New `Config/DefaultScalability.ini` (not owned by me):
`[TextureQuality@2] r.MaxAnisotropy=8` and `[TextureQuality@3] r.MaxAnisotropy=16`. Sampler states pick it up on a
scalability apply. Verify that a mid-session change takes effect; otherwise it needs a restart. Expected +0.1-0.3 ms in
ground-heavy views at 1715 x 1072.

**P0-3 Tile only: drop the 17 m terminator hack.** Tile Custom_0, landscape line 189 (+1 on the tile):
`float term=0.0;` (keep it on the landscape). Check for VSM stripes at low sun (#5, #30) at the test views. Revert if
stripes appear.

**P0-4 Blowing snow keeps the surface.** `M_EverestLandscape_SnowSnakes.hlsl:22`: `amount * 0.8` becomes
`amount * 0.35`. Keep the whitening.

### P1: near-field detail from what is already there (still ClumpSnow, so expect some clump look until P2)

**P1-a Grain strength by footprint.** Move `fp` (line 158) above line 156, then:
```hlsl
// near the eye the snow's own grain shows (photos: crisp crust and crumbs at your feet); beyond ~10 m it fades back to
// the calm large forms the user chose on 2026-10-22. fp = cm per pixel on the ground.
float nearK=1.0-smoothstep(1.5,8.0,fp);
float3 micro=hx.SN(lerp(n,hx.SN(snowW,n),lerp(lerp(0.09,0.30,hard),lerp(0.50,0.70,hard),nearK)),n);
```
**P1-b Cavity shade and AO at cm scale** (after line 173, using `snowH` from the loop):
```hlsl
float cav=saturate((0.52-snowH)*3.0)*nearK;          // low spots of the snow's own height field
sCol*=lerp(float3(1,1,1),float3(0.86,0.92,1.0),cav*0.6); // light scattered inside the snow: a little blue, a little dark
```
Line 192: `AO=lerp(lerp(1.0,0.9,hollow)*lerp(1.0,0.75,cav),rockM.r,rm);`

**P1-c Roughness by micro relief** (after line 175): crests polished by the wind, hollows matte, only on hard snow:
`sRough=lerp(sRough,max(sRough-0.14,0.68),saturate((snowH-0.5)*3.0)*nearK*hard);`

**P1-d Narrow the per-cell scale range** (line 12, `Xf`/`Xd`): `exp2(lerp(-1.5,1.5,q.y))` becomes
`exp2(lerp(-0.5,0.5,q.y))`. A real surface has one grain size; the 8x spread made the blotches.

**P1-e Glints that actually flash.** Replace lines 176-180, and move `sunL` (line 188) above them:
```hlsl
// glints: a few ice facets per cm² face exactly half-way between the eye and the sun, so they really flash; which ones
// flash re-rolls as the angle changes, so they twinkle as you walk. Only where a pixel is under ~1.5 cm (TSR-stable).
float3 Vw=Parameters.CameraVector;                    // pixel -> eye (or wire CameraVectorWS into a new input pin)
float3 Hs=normalize(sunL+Vw);
uint2 gi=uint2(int2(floor(WP.xy/0.7)));               // 7 mm cells; integer hash (sin-hash breaks on world-size ids, #55)
uint gh=(gi.x*0x8da6b343u)^(gi.y*0xd8163841u)^(uint(floor(Hs.x*18.0)+64.0)*0x2c1b3c6du)^(uint(floor(Hs.y*18.0)+64.0)*0x297a2d39u);
gh^=gh>>15; gh*=0x2c1b3c6du; gh^=gh>>12;
float spark=step(0.985,float(gh&0xffffu)/65536.0)*step(0.55,dot(Hs,n))*(1.0-smoothstep(0.6,1.6,fp))*(1.0-ice);
sN=hx.SN(lerp(sN,Hs,spark),n);
sRough=lerp(sRough,0.06,spark);
```
Shadows still apply, because the light pass does the lighting. Cost: a handful of ALU.

### P2: a real snow texture set (ask the user to download; all CC0)

Downloads go to `F:\UE_project\YggdrasilsPeak\Downloads\snow_2026-11-14\` (project rule: never C:).

| set | what it is | why | download |
|---|---|---|---|
| **Poly Haven `snow_02`** (Rob Tuytel) | photo scan, 2 m x 2 m, "soft powdery snow, smooth uneven drifts, fine frosty granularity, subtle ridges" | main near grain (2K = ~1 mm/texel) with matching height and normal | 2k PNG: Diffuse 5.1 MB, nor_dx 8.5 MB, Rough 1.6 MB, AO 2.5 MB, Displacement 6.9 MB (~25 MB); 4k ~90 MB |
| **ambientCG `Snow008A`** | procedural "dunes, waves" | the missing 5-50 cm band: wind ripples on wind-packed snow | 2K-JPG zip 23.9 MB |
| ambientCG `Snow009A` | procedural "dunes" | second ripple variant (breaks repetition) | 2K-JPG zip 24.0 MB |
| ambientCG `Snow014` (optional) | photogrammetry, "smooth, soft" | calm powder in valleys if `snow_02` reads too busy | 2K-JPG zip 24.1 MB |
| Poly Haven `snow_field_aerial` (optional) | 80 m x 80 m aerial scan | macro height/normal only (its colour is muddy) | 2k only |

Already in Content and unused (mesh UV atlases, not tileable, so useful only as look references): Megascans
`Snow_Embankment_uhvjae0fa` and `Snow_Pile_xgxhcjxqx` (sources 2048 px). Also unused: `Icelandic_Glacier .../T_ucnkcgkn_4K_H`
(4096 px height) and ORM.

Import (editor script, new `Tools/Materials/snow_detail_2026_11_14.py`): `T_SnowFine_N` (snow_02 nor_dx, TC_Normalmap/BC5),
`T_SnowFine_HRA` (R = height, G = roughness, B = AO packed into one TC_Masks texture, sRGB off), `T_SnowRipple_N` /
`T_SnowRipple_H` (Snow008A; 009A as a second cell set). Max 2048, group World / WorldNormalMap. Colour maps are not
needed: snow albedo varies by a few % at most, which the height-driven cavity tint covers.

Shader:
- Layer A (lines 99-115): swap the ClumpSnow objects for `T_SnowFine_N` / `T_SnowFine_HRA`.
  `SnowTileCm` = **200** (the scan's real size). Drop the `*1.8` in line 100 (cells then about 0.4x the tile, ~85 cm, so
  each cell shows a large piece of the scan and fewer blend seams). Narrow scale
  as in P1-d. Read roughness and AO from HRA into `sRough` / AO near the camera (`nearK`).
- Layer B (lines 117-122): replace "same texture 3.7x larger" with the **wind-ripple** texture, on the top plane only
  (`i==2`). It is rotated across the prevailing westerly like `Sast` (`hx.Rot(p,0.35)`), with a 250-400 cm tile, blended
  in by `hard`, `sm2` patches and a footprint fade to about 30 m (`1-smoothstep(10,40,fp)`).
- This needs new Custom inputs (`RipN`, `RipH`, `RipTile`). Rewriting a Custom node's input list drops all its wires, so
  reconnect every one (same pattern as `Tools/Materials/update_trail_shade.py`).

### P3: real height for snow on the route tiles (the "like the rock" part)

`Tools/Materials/M_EverestDetailTile_Displacement.hlsl`:
- Lines 65-67: read the new fine height (and the ripple height on the top plane) at a fixed **mip 3** (2 m / 2048 px means
  mip 3 = 8 mm, matching the 1-2 cm micro-triangles near the eye at DicingRate 4), not mip 0.
- Line 74: `d = lerp(sD + ripD, (rockH-0.35)*DispRock, rm)` with `sD = (snowH-0.5)*DispSnow` and **SnowDisplacementCm =
  3** (±1.5 cm grain). `ripD = (ripH-0.5) * lerp(2.0, 6.0, hard) * ripMask` gives ±1-3 cm ripples. `hard` and `ripMask`
  are recomputed here from `WP.z` and `n` (cheap) so they match the colour node. Keep line 76 (steep damping) and the
  rubble block.
- **Do not fade by camera distance with `CameraPositionWS`.** In VSM shadow passes the "camera" is the light's view, so
  geometry and its shadow would disagree. Keep the displacement world-space only. If far crawling shows, fade around
  `MPC_SnowTrail.TrailArea.xy` (it follows the player) with a 25-40 m radius instead.
- Matching normals: Nanite displacement moves the geometry (and VSM shadows) but does not re-derive the shading normal.
  The colour node's normal must come from the same height maps at matching strength (snow_02's nor_dx corresponds to
  its Displacement; calibrate by eye).
- Collision: the change is under 3 cm against a 2 m collision mesh. Boots sink 20 cm anyway.
- Optional cost item: tile displacement magnitude **400 to 64** (centre 0.5 = ±32 cm, enough for trench -18/+7, rubble
  ±3, snow ±5). This tightens the Nanite cluster bounds used for culling and VSM page marking. It needs `/400.0` changed
  in the tile displacement return (line 87) and a **tile-only copy of TrailDisp** (`M_EverestLandscape_TrailDisp.hlsl:44`
  is shared with the landscape, whose 400 cm range must stay). This also removes lead (a) of #56 (render surface ±2 m).
- VSM: `r.Shadow.Virtual.Nanite.AllowTessellationDirectional` defaults to 1, so the displaced snow throws its own small
  sun shadows: the crisp micro-shadowing the scanned rocks have. The large soft shadows the user likes are unaffected.

### P4: optional lighting

- Tile material only: try MSM_Subsurface again with low opacity (0.15) and the old blue SSS colour (0.42, 0.62, 0.9) x
  (1-rm). On 1 m tiles the 17 m-triangle stripe cause (#5) should not apply. Revert if stripes come back.
  Cost +0.1-0.3 ms.
- Sun contact shadows (a small "Contact Shadow Length" on the directional light) make micro-shadows from the new
  displacement very cheaply. Per the "UDS settings first" rule, check whether Ultra Dynamic Sky exposes it on its sun.

### Low-end switch

Feed a `QualitySwitch` node (Low = 0, others = 1) into a new `Detail` input on both snow Custom nodes. Multiply
`nearK`, glints, ripples and `DispSnow` by it. No C++ needed: in UE 5.8 `sg.EffectsQuality` sets `r.MaterialQualityLevel`
(`[EffectsQuality@0]` = Low, `BaseScalability.ini:804`). It follows the tier, or the menu's "Effects" override
(`GfxEffects`). Alternatively, an MPC scalar set in `YggQualitySubsystem::ApplyTierExtras` (C++, not my file).

---

## 4. GPU cost and how to keep it cheap

Baseline: the session median was **GPU 13.6 ms (GPU-bound), 54 fps rendered** at 2560 x 1600, DLSS Quality and frame
generation (`report.md`). Target: **net ≤ +0.3 ms**. All numbers below are estimates; measure each step (section 5).

| change | estimate |
|---|---|
| P0-1 derivative multiply | +0.0-0.1 ms (sharper mips = more bandwidth) |
| P0-2 anisotropy 4 to 8 (High) | +0.1-0.3 ms in ground-heavy views |
| P0-3 / P0-4 / P1 a-d | ~0 (ALU on samples already taken) |
| P1-e glints | < 0.03 ms |
| P2 texture swap (HRA packed, ripples on the top plane only) | +0.05-0.15 ms; VRAM +16-25 MB |
| P3 snow displacement (tessellation already active) | +0.2-0.5 ms (displacement shader per micro-vertex + VSM re-raster on invalidation) |
| P4 SSS on tiles | +0.1-0.3 ms |

Savings to fund it, in the same Custom nodes:
- **Skip the triplanar planes that do not contribute.** The loop at line 79 is `[unroll]` and samples all three planes
  for both rock and snow (~60 texture reads per pixel) even on flat ground, where `w.x`, `w.y` ≈ 0. Threshold the weights
  (`w = max(w-0.02, 0)`, renormalise) and wrap each plane in `if (wa[i] > 0)`. `SampleGrad` is legal in branches and
  `gx`/`gy` are computed outside. Expected **-0.2 to -0.6 ms**. Rock's look is unchanged (weights under 2 % are invisible).
- **Replace the finite-difference noise** (`Pil`, `Lump`, `Clump`, `Sast` evaluated 3x each = ~30 simplex/value-noise
  calls, lines 148-163) with one baked tiling macro-normal texture read at 2-3 scales: **-0.2 to -0.4 ms**.
  Optional, and the look must be matched.
- Tile displacement magnitude 400 to 64 (P3): unknown gain, worth one measurement.

---

## 5. Test views and procedure

Use `Tools/Art/shot.ps1` with **`-NoGhost`** so the climber falls onto the ground and the camera sits at true eye
height. Z is the route point plus 3 m. Use the same command before and after. For the user's own setup add
`-ResX 2560 -ResY 1600`.

Common part of every command:
`-NoGhost -ResX 2560 -ResY 1600 -Set "Graphics=2;Upscaling=2;Upscaler=0;FrameGen=0;GfxTextures=0" -Extra "ygg.Realm.Force none, ygg.Snow.Spindrift 0, ygg.Body.Hypoxia 0, ygg.Body.Frost 0, ygg.Body.Exhaust 0"`

| name | BugItGo x y z pitch yaw | elevation | why |
|---|---|---|---|
| S1 Dingboche (session spot) | `338533 2468724 -68990 -15 172` | ~4,340 m | where the user played; `hard` = 0, worst case; compare with `0095.1_hitch.jpg` |
| S2 Dingboche, looking down | `338533 2468724 -68990 -70 172` | ~4,340 m | grain at 1-3 m (compare `0185.8_hitch.jpg`) |
| S3 lower valley | `231733 1792724 -7072 -15 -63` | ~4,955 m | gentle soft snow, scattered stones (rock vs snow in one frame) |
| S4 below EBC, glacier snow | `533014 1285409 27457 -12 -26.6` (Ghost, known view from #55) | ~5,300 m | glacier snow patches, pinnacles |
| S5 Western Cwm | `941082 1497544 131952 -15 0` | ~6,345 m | wide flat soft snow, the most typical play view |
| S6 Camp 2 east | `1013070 1516502 145100 -15 63` | ~6,480 m | used before (16 runs in logs); camp props and rocks for scale |
| S7 Lhotse face, lower | `1118911 1640421 176719 -15 -14` | ~6,790 m | steeper, `hard` rising: ripples and crust |
| S8 Lhotse face, upper | `1204948 1590655 231374 -10 31` | ~7,340 m | hard wind-packed snow, 36-degree slope |
| S9 South Col / Camp 4 | `1286948 1584654 283481 -15 -150` | ~7,906 m | hard snow, rubble border, maximum blowing snow |

Example:
`powershell -File Tools/Art/shot.ps1 snow_S5_before 941082 1497544 131952 -15 0 -NoGhost -ResX 2560 -ResY 1600 -Set "Graphics=2;Upscaling=2;Upscaler=0;FrameGen=0;GfxTextures=0" -Extra "ygg.Realm.Force none, ygg.Snow.Spindrift 0, ygg.Body.Hypoxia 0, ygg.Body.Frost 0, ygg.Body.Exhaust 0"`
writes `Saved/Compare/shot_snow_S5_before.png`. Route z near the summit is pre-RaiseSummit (memory), so there are no
summit views here. If a `-NoGhost` start lands inside a tile, add 3 m.

Isolate each cause before changing the shader (one variable per shot, at S1 and S5):
- Upscaler: `Upscaling=1` (Native/DLAA) against `Upscaling=2` (67 %). If Native is much sharper, D4 matters.
- Anisotropy: `GfxTextures=4` (Epic, 8x) against `0` (High, 4x). Detail 6-20 m away should sharpen (D5).
- Streaming (rule-out): add `r.Streaming.FullyLoadUsedTextures 1`. No change is expected.
- Blowing snow: `ygg.Snow.Spindrift 0` against `-1` at S8/S9 (D7).
- Motion blur (rule-out): `r.MotionBlurQuality 0` while turning (needs a hand play-test).
- Tessellation share of GPU: `perf_run.ps1` with `-Extra "r.Nanite.Tessellation 0"` against default (also shows what P3
  adds on top).

Performance: `powershell -File Tools/Art/perf_run.ps1 S5 941082 1497544 131952 -15 0 0 -Frames 3000` (and S1) before
and after each phase. Judge the last half of the CSV (capture starts at frame 1; UE 5.8 writes the CSV under
`%LOCALAPPDATA%\UnrealEngine\5.8\Saved\Profiling\CSV`). Run once with `-Extra "ProfileGPU"` for the BasePass,
Nanite (VisBuffer / tessellation) and ShadowDepths split.

The user makes the visual call (memory: "I verify by measurement, user judges visuals"). Show S1, S2, S5 and S8
before/after side by side, at 100 % crops of the bottom third of the frame.

---

## 6. Uncertain or unverified

- DispRock/DispSnow wiring to RockDisplacementCm/SnowDisplacementCm on the tile was decoded from the binary (export
  order); confirm in the editor. If DispRock is wired elsewhere, the rock on tiles may have relief I did not count.
- All cost numbers are estimates; nothing was run.
- `View.MaterialTextureDerivativeMultiply` inside a Custom node: the member exists in UE 5.8 (`SceneView.h:978`) and the
  engine's own generated code uses `View.` the same way, but it has not been compiled here. In a Nanite programmable
  raster or displacement context `View` is available; the colour node is a pixel/shading context.
- `Parameters.CameraVector` in the colour Custom node: standard in pixel contexts. If the compile complains (for
  example in a Lumen card capture context), add an input pin wired from the existing `CameraVectorWS` expression.
- `r.MaxAnisotropy` changed at runtime may need sampler recreation (restart) to show.
- Poly Haven "dimensions 2000 x 2000" is taken to be millimetres (2 m), as usual for their scans; ambientCG lists no
  physical size for Snow008A/009A. The tile sizes above are starting points to set by eye.
- Whether Subsurface (P4) brings back the VSM stripes on tiles is unknown until tried.

## 7. Files the fix would touch (none edited by this investigation)

- `Tools/Materials/M_EverestDetailTile_Custom0.hlsl`: new file (save the tile node first; extracted copy in scratch, see
  section 1). Then P0-1, P0-3, P1, P2 edits.
- `Tools/Materials/M_EverestLandscape_Custom0.hlsl`: P0-1 (and P1/P2 once the tiles are approved).
- `Tools/Materials/M_EverestDetailTile_Displacement.hlsl`: P3.
- `Tools/Materials/M_EverestLandscape_SnowSnakes.hlsl`: P0-4.
- `Tools/Materials/M_EverestLandscape_Glacier.hlsl`, `M_EverestDetailTile_Glacier.hlsl`: P0-1 mip offsets (optional).
- New tile-only TrailDisp copy, only if the magnitude goes from 400 to 64.
- New `Tools/Materials/snow_detail_2026_11_14.py`: import textures, push code, add/re-wire Custom inputs, set
  SnowTileCm 200 / SnowDisplacementCm 3, add a QualitySwitch. Saves `Content/Materials/M_EverestDetailTile.uasset`
  (later `M_EverestLandscape.uasset`). New textures in `/Game/Environment/Snow/Textures/`.
- New `Config/DefaultScalability.ini`: P0-2.
- Optional: `Source/YggdrasilsPeak/Environment/YggQualitySubsystem.cpp` (MPC detail scalar per tier) if the
  QualitySwitch route is not used.

## Sources

1. Sastrugi scales: [Seasonal changes in snow surface roughness at Summit, Greenland (Annals of Glaciology)](https://cambridge.org/core/services/aop-cambridge-core/content/view/D4DDCF9E6EF3FB88E21A90878580664C/S0260305500257959a.pdf/seasonal-changes-in-snow-surface-roughness-characteristics-at-summit-greenland-implications-for-snow-and-firn-ventilation.pdf); [Japan Ministry of the Environment, sastrugi](https://www.env.go.jp/nature/nankyoku/kankyohogo_en/kankyo_en/hogo/kokunai/houritsu/keikan/sastrugi.html)
2. Snow BRDF forward peak: [Dumont et al. 2010, ACP](https://acp.copernicus.org/articles/10/2507/2010/); [Ball et al. 2015](https://epic2-clone.awi.de/37955/1/Ball_et_al_2015.pdf)
3. Sastrugi BRDF (forward peak along the sastrugi): [Effect of surface roughness on remote sensing of snow albedo (Annals of Glaciology)](https://core-prod.cambridgecore.org/core/journals/annals-of-glaciology/article/effect-of-surface-roughness-on-remote-sensing-of-snow-albedo/329B068F136E8FA00D9550CAF3162990)
4. Real-time sparkle: [Wang & Bowles 2016, A Robust and Flexible Real-Time Sparkle Effect](https://hal.archives-ouvertes.fr/hal-01327604); [Wang, Deng, Holzschuch 2020](https://hal-univ-tlse3.archives-ouvertes.fr/INRIA/hal-03156230)
5. Sun cups / penitentes: [Suncup (Wikipedia)](https://en.wikipedia.org/wiki/Suncup_(snow)); [Physics Today, penitentes](https://physicstoday.aip.org/backscatter/penitentes); [Penitentes on the Khumbu Glacier (SummitPost)](https://summitpost.org/penitentes-on-the-khumbu-glacier-central-himalayas/331364)
6. Albedo: [Grenfell & Maykut, optical properties of ice and snow in the Arctic Basin](https://www.cambridge.org/core/services/aop-cambridge-core/content/view/EE9874935EE49EB944960B83BC53397C/S0022143000021122a.pdf/the-optical-properties-of-ice-and-snow-in-the-arctic-basin.pdf); [Antarctic snow albedo](https://resolve.cambridge.org/core/services/aop-cambridge-core/content/view/BA15776866760EF2AEBC7C5657F79297/S0022143000009011a.pdf/on-the-albedo-of-snow-in-antarctica-a-contribution-to-iago.pdf)
7. Photo references: [Wikimedia Commons, Category:Sastrugi](https://commons.wikimedia.org/wiki/Category:Sastrugi)
8. Textures (CC0): [Poly Haven snow_02](https://polyhaven.com/a/snow_02) (API: api.polyhaven.com/files/snow_02); [ambientCG Snow008A](https://ambientcg.com/view?id=Snow008A), [Snow009A](https://ambientcg.com/view?id=Snow009A), [Snow014](https://ambientcg.com/view?id=Snow014); [Poly Haven snow_field_aerial](https://polyhaven.com/a/snow_field_aerial)
