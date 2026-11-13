// M_UI_CloudVeil (UI domain): the shadow of drifting cloud over the world behind a menu, darkest where the words
// stand (Left), thinner towards the other side (Right). Inputs: UV, T (s), Aspect, Left, Right, Drift, Ink, Light.
// Returns rgb + alpha.
float2 p = float2(UV.x * Aspect, UV.y);
float n = 0.0, amp = 0.55;
float2 q = p * 1.25 + float2(T * 0.011, -T * 0.0035);
for (int o = 0; o < 4; o++)
{
    float2 i = floor(q), f = frac(q);
    float2 u = f * f * (3.0 - 2.0 * f);
    float a = frac(sin(dot(i, float2(127.1, 311.7))) * 43758.5453);
    float b = frac(sin(dot(i + float2(1, 0), float2(127.1, 311.7))) * 43758.5453);
    float c = frac(sin(dot(i + float2(0, 1), float2(127.1, 311.7))) * 43758.5453);
    float d = frac(sin(dot(i + float2(1, 1), float2(127.1, 311.7))) * 43758.5453);
    n += amp * lerp(lerp(a, b, u.x), lerp(c, d, u.x), u.y);
    q = q * 2.03 + float2(3.1, 1.7) + T * 0.004 * (o + 1);
    amp *= 0.5;
}
float side = lerp(Left, Right, smoothstep(0.0, 0.85, UV.x));
float vig = 0.14 * pow(abs(UV.y * 2.0 - 1.0), 2.0) + 0.10 * pow(abs(UV.x * 2.0 - 1.0), 3.0);
float alpha = saturate(side + vig + (n - 0.5) * Drift);
// where the cloud is thin the veil carries a little cold light, where it is thick it is ink
float3 col = lerp(Ink, Light, saturate((0.55 - n) * 2.2) * 0.35);
return float4(col, alpha);
