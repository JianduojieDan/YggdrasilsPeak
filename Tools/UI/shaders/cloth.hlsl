// M_UI_Cloth (UI domain): the pack screen's ground - a dark canvas tarp in a pool of lamplight, lit from the upper
// left, fading into the dark at its edges. Inputs: UV, Aspect, Light (0..1, the screen fading in). Returns rgb + alpha.
float2 p = float2(UV.x * Aspect, UV.y);
// canvas weave: warp and weft threads, faint (7 px threads at 1080p, no shimmer)
float2 q = p * 150.0;
float wv = abs(sin(q.x * 3.14159)) * abs(sin(q.y * 3.14159 + 1.57));
float check = frac((floor(q.x) + floor(q.y)) * 0.5) * 2.0;
float weave = lerp(abs(sin(q.y * 3.14159)), abs(sin(q.x * 3.14159)), check) * 0.6 + wv * 0.4;
// slow variation and a few soft creases
float n = 0.0, amp = 0.5;
float2 r = p * 2.6;
for (int o = 0; o < 4; o++)
{
    float2 i = floor(r), f = frac(r);
    float2 u = f * f * (3.0 - 2.0 * f);
    float a = frac(sin(dot(i, float2(127.1, 311.7))) * 43758.5453);
    float b = frac(sin(dot(i + float2(1, 0), float2(127.1, 311.7))) * 43758.5453);
    float c = frac(sin(dot(i + float2(0, 1), float2(127.1, 311.7))) * 43758.5453);
    float d = frac(sin(dot(i + float2(1, 1), float2(127.1, 311.7))) * 43758.5453);
    n += amp * lerp(lerp(a, b, u.x), lerp(c, d, u.x), u.y);
    r = r * 2.1 + 3.7;
    amp *= 0.5;
}
float crease = 1.0 - abs(sin(p.x * 5.3 + p.y * 2.1 + n * 4.0)) * 0.12;
// the pool of light, a little left of centre, brighter towards the lamp at the upper left
float2 cen = float2(0.42 * Aspect, 0.5);
float dd = length((p - cen) / float2(0.62 * Aspect * 0.55, 0.56));
float pool = smoothstep(1.05, 0.1, dd);
float key = saturate(1.15 - length(p - float2(0.15 * Aspect, 0.05)) * 0.75);
float3 dark = float3(0.012, 0.015, 0.02);
float3 lit = float3(0.10, 0.092, 0.082);
float3 col = lerp(dark, lit, pool * (0.55 + 0.45 * key));
col *= (0.86 + 0.14 * weave) * (0.88 + 0.24 * n) * crease;
float alpha = saturate(0.55 + pool * 0.6) * Light;
return float4(col, alpha);
