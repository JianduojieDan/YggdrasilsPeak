// Asgard's rings: fine ringlets close round the planet, one broad bright main band, a narrow dark division, lanes further out, deep gold fading to bronze.
// (2026-11-11 user: too faint, too thin, too far from the planet -> starts at 1.15 radii, main band twice as wide, nearly opaque.)
// Hidden where the planet stands in front (translucent planets write no depth). rgb + a. R = planet radius in cm.
float r = length(UV * 2.0 - 1.0);
float inner = smoothstep(0.33, 0.34, r) * (1.0 - smoothstep(0.95, 1.0, r));
float x = saturate((r - 0.33) / 0.67);
float fine = 0.5 + 0.28 * sin(r * 520.0) + 0.22 * sin(r * 1270.0 + 1.3);
float lanes = 0.5 + 0.5 * sin(r * 96.0 + sin(r * 31.0) * 3.0);
lanes = smoothstep(0.2, 0.85, lanes);
float gap = 1.0 - 0.85 * smoothstep(0.012, 0.0, abs(x - 0.52));
float mainBand = smoothstep(0.0, 0.05, x) * (1.0 - smoothstep(0.42, 0.5, x));
float outerFade = pow(1.0 - x, 1.2);
float a = inner * gap * (mainBand * 0.95 + outerFade * 0.6 * lanes) * lerp(0.55, 1.0, fine);
float3 c = lerp(float3(1.0, 0.72, 0.36), float3(0.5, 0.25, 0.09), x);
c = lerp(c, float3(1.0, 0.86, 0.6), mainBand * lanes * 0.7);
float3 v = normalize(WP - Cam);
float3 toP = WP - Ctr;
float3 perp = toP - v * dot(toP, v);
a *= 1.0 - step(0.0, dot(toP, v)) * step(length(perp), R);
a = saturate(a * Fade);
return float4(c * Bright * 1.4 * a, a);
