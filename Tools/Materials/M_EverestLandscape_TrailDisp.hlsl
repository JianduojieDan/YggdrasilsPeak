// Pushes the snow down where the trail map says something went through it (Ghost of Yotei / Wukong-style displacement).
// Base = existing displacement output (0.5 = none, 1 unit = 200 cm), Rock = rock mask (no trench in rock),
// Area = (centre X, centre Y, side cm, on/off), Trail = softened trail map (R: print depth, G: wide spread).
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
if (Area.w < 0.5) return Base;
float2 uv = (WP.xy - Area.xy) / Area.z + 0.5;
float2 edge = abs(uv - 0.5);
float fade = 1.0 - smoothstep(0.40, 0.48, max(edge.x, edge.y));
if (fade <= 0.0) return Base;
float h = th.Height(Trail, TrailSampler, uv, WP.xy, 18.0, 7.0) * fade * (1.0 - Rock);
return Base + h / 200.0;
