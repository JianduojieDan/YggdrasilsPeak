// Asgard's rings: fine ringlets, a wide open gap round the planet, one bright main band, thin dark lanes further out, deep gold fading to bronze.
// Hidden where the planet stands in front (translucent planets write no depth). rgb + a. R = planet radius in cm.
float r = length(UV * 2.0 - 1.0);
float inner = smoothstep(0.42, 0.43, r) * (1.0 - smoothstep(0.93, 1.0, r));
float x = saturate((r - 0.42) / 0.58);
float fine = 0.5 + 0.28 * sin(r * 520.0) + 0.22 * sin(r * 1270.0 + 1.3);
float lanes = 0.5 + 0.5 * sin(r * 96.0 + sin(r * 31.0) * 3.0);
lanes = smoothstep(0.2, 0.85, lanes);
float gap = 1.0 - 0.9 * smoothstep(0.015, 0.0, abs(x - 0.34));
float mainBand = smoothstep(0.02, 0.09, x) * (1.0 - smoothstep(0.22, 0.32, x));
float outerFade = pow(1.0 - x, 1.5);
float a = inner * gap * (mainBand * 0.72 + outerFade * 0.5 * lanes) * lerp(0.35, 1.0, fine);
float3 c = lerp(float3(1.0, 0.72, 0.36), float3(0.5, 0.25, 0.09), x);
c = lerp(c, float3(1.0, 0.86, 0.6), mainBand * lanes * 0.7);
float3 v = normalize(WP - Cam);
float3 toP = WP - Ctr;
float3 perp = toP - v * dot(toP, v);
a *= 1.0 - step(0.0, dot(toP, v)) * step(length(perp), R);
a = saturate(a * Fade);
return float4(c * Bright * 1.1 * a, a);
