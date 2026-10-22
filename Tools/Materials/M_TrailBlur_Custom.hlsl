// Softens the raw trail map (sharp boot stamps, 2.5 cm texels) for the landscape:
// R = print depth with rounded walls (~10 cm blur), G = wide spread (~30 cm) used for the banks of snow
// pushed out beside the trail. Texel = 1 / map size.
float t = Texel;
// R: ~10 cm soft blur (centre + two rings) so the deep walls slope instead of standing vertical
float r = Trail.SampleLevel(TrailSampler, UV, 0).r * 0.2;
[unroll] for (int k = 0; k < 8; k++) {
  float a = k * 0.7853982;
  float2 d = float2(cos(a), sin(a)) * t;
  r += Trail.SampleLevel(TrailSampler, UV + d * 2.0, 0).r * 0.06 + Trail.SampleLevel(TrailSampler, UV + d * 4.0, 0).r * 0.04;
}
float g = 0.0;
[unroll] for (int i = 0; i < 12; i++) {
  float a = i * 0.5235988;
  float2 d = float2(cos(a), sin(a)) * t;
  g += Trail.SampleLevel(TrailSampler, UV + d * 5.0, 0).r * 0.6 + Trail.SampleLevel(TrailSampler, UV + d * 11.0, 0).r * 0.4;
}
g /= 12.0;
return float3(r, max(g, r), 0);
