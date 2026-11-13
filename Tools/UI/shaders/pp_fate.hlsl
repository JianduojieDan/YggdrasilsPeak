// M_PP_Fate (post process, after tonemapping): the live world behind a death screen turns into what took you.
// Inputs: UV (viewport uv), SceneTex (PostProcessInput0, only so the scene textures are bound), Fate (1..7),
// T (s since death), Amount (0..1 fade in), Aspect. Returns the new colour.
// Fates: 1 Frozen, 2 Breathless, 3 Fallen, 4 Buried, 5 Parched, 6 Starved, 7 Lost.
struct FFate
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
FFate f;
float2 uv = UV;
float t = max(T, 0.0);
float A = saturate(Amount);
float2 asp = float2(Aspect, 1.0);
float3 col = f.S(uv);
int fate = (int)(Fate + 0.5);

if (fate == 1)
{
    // FROZEN: frost grows over the lens from every edge - fine ice needles at three scales pointing in from the edge
    // they grew from, over a milky frosting; the world under it blurs and goes cold; a few crystals catch the light.
    float2 p = (uv - 0.5) * asp;
    float dx = min(uv.x, 1.0 - uv.x) * Aspect;
    float dy = min(uv.y, 1.0 - uv.y);
    float edge = min(dx, dy);
    float grow = 0.05 + 0.40 * f.E(t / 5.5);
    float warp = f.F(p * 2.5 + 3.0);
    float cover = smoothstep(grow, grow - 0.26, edge + (warp - 0.5) * 0.30 + (f.N(p * 22.0) - 0.5) * 0.05) * A;
    // which way the crystals grow: inward from the nearest edge
    float2 inward = dx < dy ? float2(uv.x < 0.5 ? 1.0 : -1.0, 0.0) : float2(0.0, uv.y < 0.5 ? 1.0 : -1.0);
    float needles = f.Needles(p * 8.0, inward, 0.9, 0.0) * 0.6 + f.Needles(p * 19.0 + 3.1, inward, 0.9, 1.0) * 0.4 + f.Needles(p * 43.0 + 7.7, inward, 0.9, 1.0) * 0.28;
    float2 v = f.V(p * 110.0);
    float grain = saturate(1.0 - (v.y - v.x) * 3.0);
    float frost = saturate(needles * 0.9 + grain * 0.12 + 0.2) * cover;
    float2 g = (float2(f.F(p * 7.0 + 1.3), f.F(p * 7.0 + 7.9)) - 0.5) * 0.02 * cover;
    float r = 0.012 * cover;
    float3 blur = 0.0;
    for (int k2 = 0; k2 < 8; k2++)
    {
        float a = k2 * 0.785398;
        blur += f.S(uv + g + float2(cos(a), sin(a) * Aspect) * r);
    }
    blur /= 8.0;
    float3 cold = lerp(col, f.L(col) * float3(0.70, 0.85, 1.0), 0.6 * A);
    float3 under = lerp(cold, lerp(blur, float3(0.78, 0.84, 0.92), 0.3) * float3(0.85, 0.92, 1.0), cover);
    float2 gc = floor(uv * float2(Aspect, 1.0) * 300.0);
    float glint = step(0.992, f.H(gc)) * pow(0.5 + 0.5 * sin(t * 2.2 + f.H(gc + 5.0) * 40.0), 8.0) * frost;
    col = under * (1.0 - frost * 0.15) + frost * float3(0.84, 0.92, 1.0) * 0.55 + glint * 1.2;
    col *= lerp(1.0, 0.82, A * (1.0 - cover));
}
else if (fate == 2)
{
    // BREATHLESS: the view tunnels in with each gasp and lets go slowly; the edges smear, split and drain.
    float ph = frac(t / 3.4);
    float br = ph < 0.25 ? f.E(ph / 0.25) : 1.0 - f.E((ph - 0.25) / 0.75);
    float2 p = (uv - 0.5) * asp;
    float d = length(p);
    float rad = lerp(1.0, 0.32, f.E(t / 7.0)) + 0.08 * br;
    float tunnel = smoothstep(rad, rad - 0.30, d);
    float ca = 0.014 * (1.0 - tunnel) * A;
    float2 dir = p / max(d, 0.0001);
    float3 c = float3(f.S(uv + dir * ca).r, f.S(uv).g, f.S(uv - dir * ca).b);
    float3 sm = 0.0;
    for (int k = 1; k <= 5; k++) { sm += f.S(uv - p * 0.022 * k * (1.0 - tunnel)); }
    sm /= 5.0;
    float3 drained = f.L(sm) * float3(0.60, 0.64, 0.86);
    float3 m = lerp(drained, lerp(c, f.L(c) * float3(0.80, 0.84, 1.0), 0.5), tunnel);
    m *= lerp(0.08, 1.0, tunnel) * (0.86 + 0.20 * br);
    col = lerp(col, m, A);
}
else if (fate == 3)
{
    // FALLEN: a jolt as you land, then walls of blue glacier ice close over the last sliver of sky above you. The
    // walls' edges break in straight facets like fractured ice; light comes through them blue near the gap.
    float shake = exp(-t * 5.0) * 0.014;
    float2 su = uv + float2(sin(t * 83.0), cos(t * 71.0)) * shake;
    float close = f.E((t - 0.2) / 2.8);
    float hw = lerp(0.62, 0.075, close);
    float x = (uv.x - 0.5) * Aspect;
    float jagL = (f.NL(uv.y * 5.0, 1.0) - 0.5) * 0.26 + (f.NL(uv.y * 23.0, 2.0) - 0.5) * 0.05;
    float jagR = (f.NL(uv.y * 5.0, 7.0) - 0.5) * 0.26 + (f.NL(uv.y * 23.0, 8.0) - 0.5) * 0.05;
    float lean = (0.5 - uv.y) * 0.16 * close;
    float l = -hw + jagL + lean, r = hw + jagR - lean;
    float inSlit = smoothstep(l - 0.003, l + 0.003, x) * smoothstep(r + 0.003, r - 0.003, x);
    float3 sky = 0.0;
    for (int k = 0; k < 6; k++) { sky += f.S(su + float2(0.0, -0.013 * k * (1.0 - close * 0.6))); }
    sky /= 6.0;
    sky = lerp(sky, f.L(sky) * float3(0.86, 0.93, 1.0), 0.45) * lerp(1.0, 1.12, close);
    float wx = x < (l + r) * 0.5 ? (l - x) : (x - r);
    float d = max(wx, 0.0);
    float bands = 0.5 + 0.5 * sin(uv.y * 34.0 + f.F(float2(x * 3.0, uv.y * 2.0)) * 5.0);
    float streak = f.F(float2(x * 10.0, uv.y * 0.9 + t * 0.05));
    float3 deep = float3(0.006, 0.013, 0.03);
    float3 glacier = float3(0.06, 0.20, 0.33) * (0.7 + 0.3 * bands) * (0.75 + 0.5 * streak);
    float3 ice = lerp(deep, glacier, exp(-d * 6.0));
    ice += float3(0.55, 0.80, 0.98) * exp(-d * 80.0) * 0.6;
    col = lerp(f.S(su), lerp(ice, sky, inSlit), A * smoothstep(0.0, 0.25, t));
}
else if (fate == 4)
{
    // BURIED: the avalanche's debris rises over the view - rounded clumps of snow of every size, lit from the gap
    // above, darker the deeper they lie; powder hangs in the last of the air; the light in the gap slowly goes.
    float2 p = uv * asp;
    float level = 0.8 * f.E(t / 3.0);
    float top = 1.0 - level + (f.F(float2(p.x * 1.6, t * 0.06)) - 0.5) * 0.24 + (f.N(float2(p.x * 9.0, 2.0)) - 0.5) * 0.035;
    float snowM = smoothstep(top - 0.006, top + 0.025, uv.y);
    float2 lightDir = normalize(float2(-0.4, -1.0));
    float clumps = 0.0;
    float scale = 5.0, wsum = 0.0, wgt = 1.0;
    for (int o = 0; o < 3; o++)
    {
        float3 c = f.VO(p * scale + o * 5.3 + (f.F(p * 2.0 + o) - 0.5) * 0.6);
        float ball = saturate(1.0 - c.x * 1.25);
        float lit = saturate(0.55 + dot(normalize(c.yz + 0.0001), -lightDir) * 0.45 * ball);
        clumps += lit * wgt; wsum += wgt; wgt *= 0.55; scale *= 2.3;
    }
    clumps /= wsum;
    float grain = f.N(p * 260.0);
    float depth = saturate((uv.y - top) * 1.7);
    float3 snow = lerp(float3(0.58, 0.66, 0.78), float3(0.94, 0.96, 1.0), clumps) * (0.94 + 0.08 * grain);
    snow *= lerp(1.0, 0.36, depth);
    snow = lerp(snow, snow * float3(0.75, 0.85, 1.0), depth);
    float rim = exp(-abs(uv.y - top) * 70.0) * 0.3;
    float haze = saturate(0.45 + level * 0.4 + (f.F(p * 3.0 + float2(t * 0.05, -t * 0.03)) - 0.5) * 0.5) * 0.7;
    float3 blur = (f.S(uv + float2(0.004, 0)) + f.S(uv - float2(0.004, 0)) + f.S(uv + float2(0, 0.006)) + f.S(uv - float2(0, 0.006))) * 0.25;
    float3 air = lerp(blur, float3(0.74, 0.80, 0.88), haze);
    float flakes = step(0.9965, f.H(floor((uv + float2(t * 0.02, -t * 0.22)) * float2(Aspect, 1.0) * 180.0))) * 0.6 * (1.0 - snowM);
    float3 m = lerp(air + flakes, snow, snowM) + rim * float3(0.9, 0.95, 1.0);
    m *= lerp(1.0, 0.55, f.E((t - 2.5) / 6.0));
    col = lerp(col, m, A);
}
else if (fate == 5)
{
    // PARCHED: heat shimmer, the colour bleached to dust; a dry crust creeps in from the edges, cracking into
    // irregular plates with finer cracks between, their edges curling dark; the middle stays clear.
    float2 p = uv * asp;
    float w = f.F(float2(p.x * 6.0, p.y * 3.0 + t * 0.6));
    float2 wob = float2(sin(uv.y * 60.0 + t * 3.1), cos(uv.x * 40.0 + t * 2.3)) * 0.0022 + (w - 0.5) * 0.0045;
    float3 c = f.S(uv + wob * A);
    float lu = f.L(c);
    c = lerp(c, float3(lu * 1.08, lu * 0.86, lu * 0.62), 0.75 * A);
    c = 1.0 - (1.0 - c) * lerp(1.0, 0.82, A);
    float edge = min(min(uv.x, 1.0 - uv.x) * Aspect, min(uv.y, 1.0 - uv.y));
    float grow = 0.05 + 0.22 * f.E(t / 6.0);
    float crust = smoothstep(grow, grow - 0.22, edge + (f.F(p * 2.5) - 0.5) * 0.24) * A;
    float2 wp = (float2(f.F(p * 2.0 + 4.0), f.F(p * 2.0 + 8.0)) - 0.5) * 0.7;
    float2 v1 = f.V(p * 16.0 + wp);
    float2 v2 = f.V(p * 42.0 + wp * 2.0 + 5.0);
    float c1 = saturate(1.0 - (v1.y - v1.x) * 34.0);
    float c2 = saturate(1.0 - (v2.y - v2.x) * 30.0) * smoothstep(0.35, 0.65, f.F(p * 5.0));
    float curl = 1.0 - smoothstep(0.0, 0.16, v1.y - v1.x);
    float3 clay = float3(0.46, 0.33, 0.22) * (0.85 + 0.3 * f.F(p * 40.0));
    c = lerp(c, clay, crust * 0.5);
    c *= 1.0 - (max(c1, c2 * 0.75) * 0.8 + curl * 0.15) * crust;
    float vig = smoothstep(0.35, 0.95, length((uv - 0.5) * asp));
    c *= lerp(float3(1, 1, 1), float3(0.55, 0.38, 0.25), vig * 0.7 * A);
    col = c;
}
else if (fate == 6)
{
    // STARVED: the colour drains out, the edges dim, the sight fades and comes back, weaker each time.
    float drain = f.E(t / 7.0) * A;
    float3 b = (f.S(uv + float2(0.002, 0)) + f.S(uv - float2(0.002, 0)) + f.S(uv + float2(0, 0.003)) + f.S(uv - float2(0, 0.003))) * 0.25;
    float3 c = lerp(col, b, drain * 0.8);
    float lu = f.L(c);
    c = lerp(c, float3(lu, lu, lu) * float3(0.95, 0.95, 0.93), drain);
    c = lerp(c, float3(0.42, 0.42, 0.42), drain * 0.35);
    float fade = 0.5 + 0.5 * sin(t * 0.9) * sin(t * 0.37);
    c *= lerp(1.0, 0.70 + 0.2 * fade, drain);
    float vig = smoothstep(0.25, 0.9, length((uv - 0.5) * asp));
    c *= 1.0 - vig * 0.6 * drain;
    col = c;
}
else if (fate == 7)
{
    // LOST: a whiteout swallows the world; spindrift streams sideways through it.
    float2 p = uv * asp;
    float k = f.E(t / 4.0) * A;
    float fog = f.F(p * float2(1.5, 3.0) + float2(-t * 0.35, 0.0));
    float streaks = f.F(p * float2(0.8, 22.0) + float2(-t * 1.6, 0.0));
    float3 b = (f.S(uv + float2(0.005, 0)) + f.S(uv - float2(0.005, 0)) + f.S(uv + float2(0, 0.007)) + f.S(uv - float2(0, 0.007))) * 0.25;
    float white = saturate(k * (0.6 + 0.45 * fog) + streaks * 0.2 * k);
    col = lerp(lerp(col, b, k), float3(0.86, 0.88, 0.90), white);
}
return col;
