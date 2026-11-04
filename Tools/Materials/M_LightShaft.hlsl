// M_LightShaft (Asgard's Skygate pillar): additive, unlit, on a cylinder. Soft golden beam: wide faint glow, brighter
// centre, fades at both ends; slow ray striations and a few drifting motes.
// Inputs: UV (u around, v 0..1 along), V (|view . normal|: 1 face-on centre, 0 at the silhouette), T (time), Bright
float a = saturate(V);
float glow = pow(a, 3.0) * 0.3;                      // soft wide body
float core = pow(a, 6.0) * 0.45;                      // brighter middle, no hard bar
float along = smoothstep(0.0, 0.15, UV.y) * smoothstep(1.0, 0.7, UV.y);
float rays = 0.7 + 0.3 * sin(UV.x * 37.0 + T * 0.25) * sin(UV.x * 13.0 - T * 0.15);
float m = frac(sin(dot(floor(float2(UV.x * 50.0, UV.y * 120.0 - T * 0.5)), float2(12.9898, 78.233))) * 43758.5453);
float motes = smoothstep(0.992, 1.0, m) * 1.5 * a;
return float3(1.0, 0.62, 0.26) * ((glow * 1.4 + core * 0.6) * rays + motes) * along * Bright;
