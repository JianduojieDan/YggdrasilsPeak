// Blowing powder streaming over the snow ("snow snakes"), after TrailShade.
// Wind = MPC_Wind.WindDir (xy direction the wind blows to, z strength 0..1), Anchor = MPC_Wind.WindAnchor
// (xy frame anchor cm, zw scroll cm; SpindriftComponent keeps both continuous). Streaks = T_SnowStreaks (tiles).
// VN = landscape vertex normal (arrives without the landscape's z scale, hence the 3.548).
NrmOut = N;
if (Wind.z < 0.01 || Rock > 0.98) return Col;
float2 d = normalize(Wind.xy + float2(1e-5, 0.0));
float2 q = WP.xy - Anchor.xy;
float2 p = float2(dot(q, d), dot(q, float2(-d.y, d.x))) - Anchor.zw;
// long thin ribbons along the wind, broken up by a shorter second layer (tile sizes match SpindriftComponent)
// explicit mip (ray tracing hit shaders have no derivatives): coarser with distance
float dist = length(WP - Cam);
float mip = clamp(log2(dist / 1500.0), 0.0, 6.0);
float s1 = Streaks.SampleLevel(StreaksSampler, p / float2(3200.0, 800.0), mip).r;
float s2 = Streaks.SampleLevel(StreaksSampler, p / float2(1600.0, 400.0) + float2(0.37, 0.61), mip).g;
float band = saturate((s1 - 0.56) * 4.0) * (0.45 + 0.55 * saturate((s2 - 0.42) * 3.0));
// only on gentle snow, only near the player (beyond ~120 m the tiling would show)
float3 vn = normalize(float3(VN.xy, VN.z * 3.548));
float gentle = smoothstep(0.72, 0.9, vn.z);
float fade = saturate(1.0 - (dist - 4000.0) / 8000.0) * saturate((dist - 60.0) / 200.0);
float amount = band * Wind.z * gentle * fade * (1.0 - Rock);
// moving powder hides the surface detail and is a touch whiter
NrmOut = normalize(lerp(N, float3(0, 0, 1), amount * 0.8));
return lerp(Col, float3(0.97, 0.985, 1.0), amount * 0.45);
