// Alfheim's prism bow on a flat disc: red outside, violet inside, broken up by drifting mist; a faint secondary bow outside.
float r = length(UV * 2.0 - 1.0);
float x = saturate((r - 0.70) / 0.14);                         // 0 inner (violet) .. 1 outer (red)
float3 spec = saturate(float3(1.5 - abs(x * 4.0 - 3.3), 1.5 - abs(x * 4.0 - 2.0), 1.5 - abs(x * 4.0 - 0.7)));
float band = smoothstep(0.0, 0.25, x) * smoothstep(1.0, 0.75, x);
float x2 = saturate((r - 0.88) / 0.10);                        // secondary: reversed order, much fainter
float3 spec2 = saturate(float3(1.5 - abs(x2 * 4.0 - 0.7), 1.5 - abs(x2 * 4.0 - 2.0), 1.5 - abs(x2 * 4.0 - 3.3)));
float band2 = smoothstep(0.0, 0.3, x2) * smoothstep(1.0, 0.7, x2) * 0.22;
float2 q = UV * 7.0 + float2(T * 0.012, T * 0.007);
float mist = 0.5 + 0.5 * sin(q.x * 3.1 + sin(q.y * 2.3)) * sin(q.y * 2.7 + sin(q.x * 1.9 + T * 0.03));
mist = lerp(0.45, 1.0, mist);
float3 c = spec * band + spec2 * band2;
c = lerp(c, dot(c, float3(0.33, 0.33, 0.33)).xxx, 0.4);          // pale, a mist bow rather than a painted one
return c * mist * Bright * Fade * 0.12;   // RealmSky drives Bright at 2 for a material without an instance override
