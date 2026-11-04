// Worn cotton: weave, sun-faded blotches, a faint printed block of script, torn irregular edges. v = 0 at the cord. rgb + a = opacity mask.
float n = sin(UV.x * 17.0 + Rand * 9.0) * sin(UV.y * 13.0 + Rand * 3.0) + 0.5 * sin(UV.x * 41.0 - UV.y * 33.0 + Rand);
float weave = 0.95 + 0.05 * sin(UV.x * 320.0) * sin(UV.y * 420.0);
float3 c = lerp(Col, float3(0.62, 0.6, 0.55), 0.18 + 0.14 * n) * weave;
float2 q = abs(UV - float2(0.5, 0.46));
float rows = step(0.45, frac(UV.y * 11.0)) * step(0.35, frac(UV.x * 23.0 + floor(UV.y * 11.0) * 0.37 + Rand * 5.0));
float print = rows * step(q.x, 0.34) * step(q.y, 0.3);
c *= 1.0 - 0.32 * print;
float tear = 0.035 * (0.5 + 0.5 * sin(UV.y * 53.0 + Rand * 11.0)) + 0.02 * (0.5 + 0.5 * sin(UV.y * 131.0 + Rand * 3.0));
float tearB = 0.1 * (0.5 + 0.5 * sin(UV.x * 37.0 + Rand * 7.0)) + 0.05 * (0.5 + 0.5 * sin(UV.x * 97.0 + Rand)) + 0.02;
float m = step(tear, UV.x) * step(tear, 1.0 - UV.x) * step(tearB, 1.0 - UV.y);
return float4(c, m);
