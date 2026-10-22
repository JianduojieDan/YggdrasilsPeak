// Pushes the snow down where the trail map says something went through it (Ghost of Yotei-style displacement buffer).
// Base = existing displacement output (0.5 = none, 1 unit = 200 cm), Rock = rock mask (no trench in rock),
// Area = (centre X, centre Y, side cm, on/off), Trail = trail map (R: 0..1).
struct TrailHelpers {
  float Height(Texture2D T, SamplerState S, float2 uv, float MaxCm, float RimCm, float Texel){
    float c = T.SampleLevel(S, uv, 0).r;
    // ring of samples ~30 cm out: snow pushed sideways out of the trench piles up beside it
    float r = 0.0; float o = 30.0 * Texel;
    r += T.SampleLevel(S, uv + float2( o, 0), 0).r; r += T.SampleLevel(S, uv + float2(-o, 0), 0).r;
    r += T.SampleLevel(S, uv + float2(0,  o), 0).r; r += T.SampleLevel(S, uv + float2(0, -o), 0).r;
    r *= 0.25;
    return -c * MaxCm + saturate(r - c) * RimCm;
  }
};
TrailHelpers th;
if (Area.w < 0.5) return Base;
float2 uv = (WP.xy - Area.xy) / Area.z + 0.5;
float2 edge = abs(uv - 0.5);
float fade = 1.0 - smoothstep(0.40, 0.48, max(edge.x, edge.y));
if (fade <= 0.0) return Base;
float h = th.Height(Trail, TrailSampler, uv, 24.0, 8.0, 1.0 / Area.z) * fade * (1.0 - Rock);
return Base + h / 200.0;
