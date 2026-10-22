// Lighting and shade for the trail the displacement cuts (same height field as TrailDisp).
struct TrailHelpers {
  float H(float2 p){ return frac(sin(dot(p, float2(127.1, 311.7))) * 43758.5453); }
  float N(float2 p){ float2 i = floor(p), f = frac(p); f = f * f * (3.0 - 2.0 * f);
    return lerp(lerp(H(i), H(i + float2(1, 0)), f.x), lerp(H(i + float2(0, 1)), H(i + float2(1, 1)), f.x), f.y); }
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
float fade = (1.0 - smoothstep(0.40, 0.48, max(edge.x, edge.y))) * (1.0 - Rock);
if (fade <= 0.0) return Col;
float t = 1.0 / Area.z;            // uv per cm
float e = 4.0;                      // cm between gradient samples
float h0 = th.Height(Trail, TrailSampler, uv, WP.xy, 18.0, 7.0);
float hx = th.Height(Trail, TrailSampler, uv + float2(e * t, 0), WP.xy + float2(e, 0), 18.0, 7.0);
float hy = th.Height(Trail, TrailSampler, uv + float2(0, e * t), WP.xy + float2(0, e), 18.0, 7.0);
float2 g = float2(hx - h0, hy - h0) / e * fade;
NrmOut = normalize(N - float3(g.x, g.y, 0.0));
// deep holes fill with cold blue shade (light scattered inside the snow), deepest at the bottom
float depth = saturate(-h0 / 18.0) * fade;
return Col * lerp(float3(1, 1, 1), float3(0.70, 0.83, 1.0), depth);
