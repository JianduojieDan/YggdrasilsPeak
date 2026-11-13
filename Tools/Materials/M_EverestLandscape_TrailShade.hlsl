// Lighting and shade for the trail the displacement cuts (same height field as TrailDisp).
struct TrailHelpers {
  float H(float2 p){ return frac(sin(dot(p, float2(127.1, 311.7))) * 43758.5453); }
  float N(float2 p){ float2 i = floor(p), f = frac(p); f = f * f * (3.0 - 2.0 * f);
    return lerp(lerp(H(i), H(i + float2(1, 0)), f.x), lerp(H(i + float2(0, 1)), H(i + float2(1, 1)), f.x), f.y); }
  float Fbm(float2 p){ return 0.5 * N(p) + 0.3 * N(p * 2.13 + 17.3) + 0.2 * N(p * 4.71 + 5.1); }
  // 1 where the glacier is rubble or bare ice (no snow to sink into), 0 on snow
  float GlacierHard(Texture2D M, SamplerState MS, float3 wp, float3 vn){
    float g = M.SampleLevel(MS, (wp.xy + 504000.0) / (3374.24 * 1009.0), 0).r;
    if (g < 0.005) return 0.0;
    float elev = 6326.3 + (wp.z - 129770.0) / 100.0;
    float wob = (Fbm(wp.xy / 9000.0) - 0.5) * 300.0;
    float debris = 1.0 - smoothstep(5170.0, 5330.0, elev + wob);
    float bare = (1.0 - debris) * (1.0 - smoothstep(5850.0, 6100.0, elev + wob));
    float nz = normalize(float3(vn.xy, vn.z * 1.774)).z;   // landscape normal arrives unscaled (2x landscape ratio)
    float patchN = Fbm(wp.xy / 1400.0 + 11.0) + (Fbm(wp.xy / 90.0 + 3.0) - 0.5) * 0.16 + (N(wp.xy / 25.0) - 0.5) * 0.05;
    float keepD = smoothstep(0.63, 0.655, patchN + (nz - 0.9) * 0.6);
    float keepB = smoothstep(0.55, 0.575, patchN + (nz - 0.85) * 0.8);
    return saturate(g * (debris * (1.0 - keepD) + bare * (1.0 - keepB)));
  }

  // Height (cm) of the trail at world xy from the softened trail map (R = print, G = wide spread).
  // Deep holes with smooth, scooped walls; the snow they pushed out lies beside them in soft, rounded banks.
  // Only large, gentle variation (20-30 cm) - fine noise here turned into slabs and spikes at this depth.
  float Height(Texture2D T, SamplerState S, float2 uv, float2 wp, float MaxCm, float RimCm){
    float2 s = T.SampleLevel(S, uv, 0).rg;
    float d = smoothstep(0.0, 1.0, s.r);
    float bank = saturate(s.g * 1.5 - s.r * 1.3);
    bank = bank * bank * (3.0 - 2.0 * bank);
    float hole = -MaxCm * d * (0.93 + 0.07 * N(wp / 18.0 + 41.0));
    return hole + bank * RimCm * (0.75 + 0.5 * N(wp / 25.0));
  }
};
TrailHelpers th;
NrmOut = N;
if (Area.w < 0.5) return Col;
float2 uv = (WP.xy - Area.xy) / Area.z + 0.5;
float2 edge = abs(uv - 0.5);
// no trench shade on rock, glacier rubble or bare ice (it painted a blue channel across the moraine, 2026-11-11)
float fade = (1.0 - smoothstep(0.40, 0.48, max(edge.x, edge.y))) * (1.0 - Rock) * (1.0 - th.GlacierHard(Mask, MaskSampler, WP, VN));
if (fade <= 0.0) return Col;
float t = 1.0 / Area.z;            // uv per cm
float e = 2.5;                      // cm between gradient samples (was 4: the walls read soft up close)
float h0 = th.Height(Trail, TrailSampler, uv, WP.xy, 18.0, 7.0);
float hx = th.Height(Trail, TrailSampler, uv + float2(e * t, 0), WP.xy + float2(e, 0), 18.0, 7.0);
float hy = th.Height(Trail, TrailSampler, uv + float2(0, e * t), WP.xy + float2(0, e), 18.0, 7.0);
float2 g = float2(hx - h0, hy - h0) / e * fade;
// 2026-11-13 (user: the footprints are good, make them more beautiful) - shading only, the shape stays:
//  - the walls a little crisper in the light (gradient x1.25), the pressed floor packed and fine-grained,
//  - the pushed-out lip crumbly and a touch brighter (fresh snow turned up), cold shade gathering in the deep corners
float2 s = Trail.SampleLevel(TrailSampler, uv, 0).rg;
float depth = saturate(-h0 / 18.0) * fade;
float floorM = smoothstep(0.55, 0.92, s.r) * fade;                      // where the boot pressed the snow flat
float lip = saturate(s.g * 1.5 - s.r * 1.3) * fade;                    // the snow pushed out beside the hole
float wall = saturate(length(g) * 1.6) * (1.0 - floorM);                // steep sides
float2 grain = float2(th.N(WP.xy / 0.9) - th.N(WP.xy / 0.9 + float2(5.3, 0.0)), th.N(WP.xy / 0.9 + float2(0.0, 7.1)) - th.N(WP.xy / 0.9 + float2(2.9, 3.3)));
float2 crumb = float2(th.N(WP.xy / 2.6 + 11.0) - 0.5, th.N(WP.xy / 2.6 + 23.0) - 0.5);
float2 detail = grain * 0.10 * floorM + crumb * 0.35 * lip * smoothstep(0.15, 0.6, lip);
NrmOut = normalize(N - float3(g * 1.25 + detail, 0.0));
// deep holes fill with cold blue shade (light scattered inside the snow), deepest at the bottom and in the corners
float3 c = Col * lerp(float3(1, 1, 1), float3(0.70, 0.83, 1.0), depth);
c *= lerp(1.0, 0.86, wall * depth);                                      // the inner walls hold a little shadow
c *= 1.0 + 0.05 * lip;                                                   // turned-up snow catches the light
c = lerp(c, c * float3(0.96, 0.98, 1.0), floorM * 0.5);                  // packed floor: denser, a shade cooler
return c;
