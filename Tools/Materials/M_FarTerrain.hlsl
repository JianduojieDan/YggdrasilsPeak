// M_FarTerrain (FarRing + FarBackground): same snow as M_EverestLandscape (sCol 0.90/0.93/0.97, rough ~0.88), lit by the
// real sun + sky, no fake haze (the sky atmosphere does the distance). Detail comes from a baked world-space normal map
// of the real DEM, so coarse geometry still shows sharp ridges and rock steps.
// Inputs: WP (world pos), Tex (normal map object), Origin (xy of map corner, cm), Size (cm), Centre (playable centre xy)
float2 uv = (WP.xy - Origin) / Size;
float3 n = Texture2DSampleLevel(Tex, TexSampler, uv, 0).xyz * 2.0 - 1.0;
n.z = sqrt(saturate(1.0 - dot(n.xy, n.xy)));          // BC5: z rebuilt
n = normalize(n);
// elevation in metres, curvature drop added back
float r = length(WP.xy - Centre);
float drop = pow(max(r - 1.7e6, 0.0), 2.0) / (2.0 * 6371e5);
float e = (WP.z + drop - 129770.0) / 100.0 + 6326.3;
// cheap value noise
float2 q = WP.xy * 0.00004;
float2 i0 = floor(q); float2 f = frac(q); f = f*f*(3.0-2.0*f);
float4 hv = frac(sin(float4(dot(i0, float2(127.1,311.7)), dot(i0+float2(1,0), float2(127.1,311.7)),
                            dot(i0+float2(0,1), float2(127.1,311.7)), dot(i0+float2(1,1), float2(127.1,311.7)))) * 43758.5453);
float nn = lerp(lerp(hv.x, hv.y, f.x), lerp(hv.z, hv.w, f.x), f.y) - 0.5;
// snow holds where it is flat enough and high enough
float snowLine = 5050.0 + nn * 500.0 + (n.y < 0 ? 150.0 : -150.0);     // north-facing (-Y = north) keeps snow lower
float high = smoothstep(snowLine - 150.0, snowLine + 150.0, e);
float flat = smoothstep(RockCut - 0.07, RockCut + 0.05, n.z + nn * 0.08);
float snow = high * flat;
// rock: dark strata bands like the Yellow Band / Lhotse face
float band = frac(e / 85.0 + nn * 1.7);
float3 rock = lerp(float3(0.055, 0.05, 0.047), float3(0.13, 0.115, 0.10), smoothstep(0.35, 0.65, band));
rock = lerp(rock, rock * float3(1.25, 1.05, 0.8), saturate(nn + 0.5) * 0.6);
// below the snow: moraine / plateau browns, valley floors greener-dark in the south
float3 low = lerp(float3(0.20, 0.17, 0.135), float3(0.28, 0.235, 0.18), saturate(nn * 2.0 + 0.5));
low = lerp(float3(0.085, 0.095, 0.06), low, smoothstep(3200.0, 4300.0, e));
float3 ground = lerp(low, rock, smoothstep(0.85, 0.7, n.z));
float3 sCol = lerp(float3(0.90, 0.93, 0.97), float3(0.84, 0.88, 0.94), saturate(nn + 0.5)) ;
// partial snow dusting on rock near the snow line
float3 col = lerp(ground, sCol, saturate(snow + high * 0.12));
Rough = lerp(0.8, 0.88, snow);
Nrm = n;
return col;
