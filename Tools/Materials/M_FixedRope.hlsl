// Fixed rope sheath (kernmantle braid) on SM_FixedRope_Piece: UV.x around the rope, UV.y along one piece (~50 cm).
// Inputs: UV, Base (rope colour), Tracer (fleck colour), Age (0 new .. 1 old, faded and dirty), Rnd (per instance),
//         NWS (world normal), WP (world position, cm), Mode (0 = colour+roughness, 1 = tangent normal).
// 16 strand pairs around, crossing diagonally; the strand on top alternates (over-under); every 8th strand is a tracer.
float2 p = float2(UV.x * 16.0, UV.y * 64.0);
float a = p.x + p.y, b = p.x - p.y;
float fa = frac(a), fb = frac(b);
float ca = floor(a), cb = floor(b);
bool topA = fmod(abs(ca + cb), 2.0) < 1.0;
float ha = sin(fa * 3.14159), hb = sin(fb * 3.14159);
float h = topA ? ha : hb;
if (Mode > 0.5)
{
    float d = topA ? 3.14159 * cos(fa * 3.14159) : 3.14159 * cos(fb * 3.14159);
    float2 g = topA ? float2(d, d) : float2(d, -d);
    return float4(normalize(float3(-g * 0.22 * (1.0 - 0.5 * Age), 1.0)), 0.0);   // both paths return float4 (one code, two nodes)
}
float strand = topA ? ca : cb;
float tracer = (fmod(abs(strand), 8.0) < 1.0) ? 1.0 : 0.0;
float3 col = lerp(Base.rgb, Tracer.rgb, tracer * step(0.25, h));
// age: sun-bleached, greyer, a little dirt in the braid; per-rope variation
float rv = frac(Rnd * 7.31);
col = lerp(col, dot(col, float3(0.3, 0.5, 0.2)) * float3(1.02, 1.0, 0.96), Age * (0.55 + 0.25 * rv));
col *= lerp(1.0, 0.82 + 0.12 * rv, Age);
col *= 0.62 + 0.38 * h;                                   // the gaps between strands are darker
// rime on the upper side, patchy along the rope
float n = frac(sin(dot(floor(WP.xy / 7.0), float2(12.9898, 78.233))) * 43758.5453);
float rime = saturate((NWS.z - 0.35) * 2.0) * saturate((n - 0.45) * 3.0) * (0.35 + 0.5 * Age);
col = lerp(col, float3(0.86, 0.9, 0.95), rime * (0.6 + 0.4 * (1.0 - h)));
float rough = lerp(0.68, 0.85, Age) + 0.1 * (1.0 - h);
return float4(col, lerp(rough, 0.55, rime));
