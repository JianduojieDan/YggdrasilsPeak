// M_PP_Body (post process, after tonemapping): what the body feels, on the view while it lasts (2026-11-13, "a death zone
// you can feel"). Inputs 0..1 from UExpeditionComponent::UpdateScreenEffect:
//   Hyp     thin air: the view tunnels in; outside the tunnel it smears towards the centre, splits and drains
//   Beat    the heartbeat's thump (lub-dub), tightening the tunnel a little on each beat
//   Frost   cold: frost grows over the lens from the edges (the death screen's Frozen fate, held by the body's cold)
//   Exhaust the view swims and softens
//   Faint   near collapse: the sight greys out and comes back in slow waves
// T = seconds, Aspect = width / height. Each effect works only where it shows (centre pixels skip the frost and the
// tunnel); with every input at 0 the C++ sets the blendable's weight to 0 and the pass is not drawn at all.
// Helpers (noise, cells, needles, scene lookup) are the death screen's, from pp_fate.hlsl.
struct FBody
{
    float H(float2 p) { return frac(sin(dot(p, float2(127.1, 311.7))) * 43758.5453); }
    float2 H2(float2 p) { return frac(sin(float2(dot(p, float2(127.1, 311.7)), dot(p, float2(269.5, 183.3)))) * 43758.5453); }
    float N(float2 p)
    {
        float2 i = floor(p), f = frac(p);
        float2 u = f * f * (3.0 - 2.0 * f);
        return lerp(lerp(H(i), H(i + float2(1, 0)), u.x), lerp(H(i + float2(0, 1)), H(i + float2(1, 1)), u.x), u.y);
    }
    float F(float2 p)
    {
        float n = 0.0, a = 0.5;
        for (int o = 0; o < 5; o++) { n += a * N(p); p = p * 2.02 + float2(7.3, 3.1); a *= 0.5; }
        return n;
    }
    // x = distance to the nearest cell point, y = to the second nearest
    float2 V(float2 p)
    {
        float2 i = floor(p), f = frac(p);
        float d1 = 8.0, d2 = 8.0;
        for (int y = -1; y <= 1; y++)
        {
            for (int x = -1; x <= 1; x++)
            {
                float2 g = float2(x, y);
                float d = length(g + H2(i + g) - f);
                if (d < d1) { d2 = d1; d1 = d; } else if (d < d2) { d2 = d; }
            }
        }
        return float2(d1, d2);
    }
    // straight segments between random heights: the edge of fractured ice
    float NL(float x, float seed) { float i = floor(x), f = frac(x); return lerp(H(float2(i, seed)), H(float2(i + 1.0, seed)), f); }
    // the nearest cell point: x = its distance, yz = the offset from it to here
    float3 VO(float2 p)
    {
        float2 i = floor(p), f = frac(p);
        float3 best = float3(8.0, 0.0, 0.0);
        for (int y = -1; y <= 1; y++)
        {
            for (int x = -1; x <= 1; x++)
            {
                float2 g = float2(x, y);
                float2 o = f - (g + H2(i + g));
                float d = length(o);
                if (d < best.x) { best = float3(d, o); }
            }
        }
        return best;
    }
    // ice needles: in every cell a thin straight crystal, pointing roughly along Dir (away from the edge it grew from)
    float Needles(float2 p, float2 Dir, float Spread, float Branch)
    {
        float2 i = floor(p), f = frac(p);
        float n = 0.0;
        for (int y = -1; y <= 1; y++)
        {
            for (int x = -1; x <= 1; x++)
            {
                float2 g = float2(x, y);
                float2 r = H2(i + g + 17.0);
                float2 c = g + r;
                // the main crystals point in from the edge; branches leave them at 60 degrees, as ice grows
                float side = r.x < 0.5 ? -1.0 : 1.0;
                float ang = atan2(Dir.y, Dir.x) + lerp((r.x - 0.5) * Spread, side * 1.047 + (r.y - 0.5) * 0.35, Branch);
                float2 d = float2(cos(ang), sin(ang));
                float len = 0.6 + r.y * 1.1;
                float2 q = f - c;
                float t = clamp(dot(q, d), 0.0, len);
                float dist = length(q - d * t);
                n = max(n, (1.0 - smoothstep(0.0, 0.035, dist)) * (1.0 - t / len * 0.7));
            }
        }
        return n;
    }
    float3 S(float2 uv) { return SceneTextureLookup(ViewportUVToBufferUV(saturate(uv)), 14, false).rgb; }
    float E(float x) { x = saturate(x); return x * x * (3.0 - 2.0 * x); }
    float L(float3 c) { return dot(c, float3(0.299, 0.587, 0.114)); }
};
FBody b;
float2 uv = UV;
float2 asp = float2(Aspect, 1.0);
float2 p = (uv - 0.5) * asp;
float d = length(p);
float t = T;

// exhaustion: a slow wobble and a softer view
float ex = saturate(Exhaust);
float2 wob = ex * 0.0022 * float2(sin(t * 1.7 + uv.y * 5.0), cos(t * 1.3 + uv.x * 4.0));
float3 col = b.S(uv + wob);
if (ex > 0.01)
{
    float r = 0.0018 * ex;
    float3 soft = (b.S(uv + wob + float2(r, 0)) + b.S(uv + wob - float2(r, 0)) + b.S(uv + wob + float2(0, r * Aspect)) + b.S(uv + wob - float2(0, r * Aspect))) * 0.25;
    col = lerp(col, soft, ex * 0.6);
}

// thin air: the tunnel, tightening with each heartbeat
float hy = saturate(Hyp);
float beat = saturate(Beat);
if (hy > 0.01)
{
    float rad = lerp(1.25, 0.52, hy) - 0.05 * beat * hy;
    float tun = smoothstep(rad - 0.42, rad, d);
    if (tun > 0.003)
    {
        float2 dir = d > 0.0001 ? p / d : float2(0.0, 0.0);
        float2 du = float2(dir.x / Aspect, dir.y);
        float r = 0.010 * tun * hy;
        float3 bl = 0.0;
        for (int k = 1; k <= 4; k++) { bl += b.S(uv - du * r * (k / 4.0)); }
        bl *= 0.25;
        float3 split = float3(b.S(uv + du * r * 0.35).r, bl.g, b.S(uv - du * r * 0.6).b);
        float3 edge = lerp(bl, split, 0.5);
        float lu = b.L(edge);
        edge = lerp(edge, float3(lu, lu, lu), 0.55 * hy);
        col = lerp(col, edge, saturate(tun * 1.2));
        col *= 1.0 - tun * (0.5 + 0.25 * beat) * hy;
    }
}

// cold: frost over the lens from every edge
float fr = saturate(Frost);
if (fr > 0.01)
{
    float dx = min(uv.x, 1.0 - uv.x) * Aspect;
    float dy = min(uv.y, 1.0 - uv.y);
    float edgeD = min(dx, dy);
    float grow = 0.02 + 0.30 * fr;
    if (edgeD - grow < 0.20)
    {
        float warp = b.F(p * 2.5 + 3.0);
        float cover = smoothstep(grow, grow - 0.22, edgeD + (warp - 0.5) * 0.28 + (b.N(p * 22.0) - 0.5) * 0.05);
        if (cover > 0.002)
        {
            float2 inward = dx < dy ? float2(uv.x < 0.5 ? 1.0 : -1.0, 0.0) : float2(0.0, uv.y < 0.5 ? 1.0 : -1.0);
            // finer than the death screen's: no big needles (at full cold they read as scratches on glass)
            float needles = b.Needles(p * 17.0 + 3.1, inward, 0.9, 1.0) * 0.5 + b.Needles(p * 38.0 + 7.7, inward, 0.9, 1.0) * 0.35 + b.Needles(p * 80.0 + 1.9, inward, 1.2, 1.0) * 0.2;
            float2 v = b.V(p * 110.0);
            float grain = saturate(1.0 - (v.y - v.x) * 3.0);
            float frost = saturate(needles * 0.85 + grain * 0.15 + 0.12) * cover;
            float2 g = (float2(b.N(p * 9.0 + 1.3), b.N(p * 9.0 + 7.9)) - 0.5) * 0.02 * cover;
            float r = 0.010 * cover;
            float3 bl = 0.0;
            for (int k2 = 0; k2 < 6; k2++) { float a = k2 * 1.0472; bl += b.S(uv + g + float2(cos(a) / Aspect, sin(a)) * r); }
            bl /= 6.0;
            float3 under = lerp(col, lerp(bl, float3(0.78, 0.84, 0.92), 0.18) * float3(0.88, 0.94, 1.0), cover);
            float2 gc = floor(uv * asp * 300.0);
            float glint = step(0.992, b.H(gc)) * pow(0.5 + 0.5 * sin(t * 2.2 + b.H(gc + 5.0) * 40.0), 8.0) * frost;
            col = under * (1.0 - frost * 0.12) + frost * float3(0.84, 0.92, 1.0) * 0.42 + glint * 1.2;
        }
    }
    // the cold takes a little warmth out of the whole view
    float lc = b.L(col);
    col = lerp(col, lc * float3(0.80, 0.90, 1.0), 0.18 * fr);
}

// near collapse: grey waves
float fa = saturate(Faint);
if (fa > 0.01)
{
    float wave = 0.5 + 0.5 * sin(t * 0.7) * sin(t * 0.29 + 1.3);
    float k3 = fa * (0.35 + 0.65 * wave);
    float lf = b.L(col);
    col = lerp(col, float3(lf, lf, lf) * 0.45, k3 * 0.75);
}
return col;
