// Lighting and a little blue shade for the trench the trail displacement cuts (same height field as TrailDisp).
struct TrailHelpers {
  float Height(Texture2D T, SamplerState S, float2 uv, float MaxCm, float RimCm, float Texel){
    float c = T.SampleLevel(S, uv, 0).r;
    float r = 0.0; float o = 30.0 * Texel;
    r += T.SampleLevel(S, uv + float2( o, 0), 0).r; r += T.SampleLevel(S, uv + float2(-o, 0), 0).r;
    r += T.SampleLevel(S, uv + float2(0,  o), 0).r; r += T.SampleLevel(S, uv + float2(0, -o), 0).r;
    r *= 0.25;
    return -c * MaxCm + saturate(r - c) * RimCm;
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
float e = 6.0;                      // cm between gradient samples
float h0 = th.Height(Trail, TrailSampler, uv, 24.0, 8.0, t);
float hx = th.Height(Trail, TrailSampler, uv + float2(e * t, 0), 24.0, 8.0, t);
float hy = th.Height(Trail, TrailSampler, uv + float2(0, e * t), 24.0, 8.0, t);
float2 g = float2(hx - h0, hy - h0) / e * fade;
NrmOut = normalize(N - float3(g.x, g.y, 0.0));
// the trench floor and walls are in their own shade: a little bluer, a little darker
float depth = saturate(-h0 / 24.0) * fade;
return Col * lerp(float3(1, 1, 1), float3(0.86, 0.92, 1.0), depth * 0.8);
