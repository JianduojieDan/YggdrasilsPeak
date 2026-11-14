// M_StoryProp (2026-11-14, Chomolungma chapter 1): one material for every hand-built story prop, picked by Kind.
// Inputs: UV, WP (world position), N (world vertex normal), Kind, Col (tint), T (time), Rock (rock colour texture),
// Photo (picture texture). Outputs: return = base colour; Rough, Metal, Emis, Nrm (WORLD-space normal, the material has
// tangent-space normals off). Patterns on walls run in world space so any box size tiles at real scale.
struct SP {
  float H(float2 p){ return frac(sin(dot(p, float2(127.1, 311.7))) * 43758.5453); }
  float N1(float2 p){ float2 i = floor(p), f = frac(p); f = f*f*(3.0-2.0*f);
    return lerp(lerp(H(i), H(i+float2(1,0)), f.x), lerp(H(i+float2(0,1)), H(i+float2(1,1)), f.x), f.y); }
  float Fbm(float2 p){ return 0.5*N1(p) + 0.3*N1(p*2.07+13.1) + 0.2*N1(p*4.13+7.7); }
};
SP sp;
float3 n = normalize(N);
// wall-plane coordinates: h along the surface horizontally, v up (floors/roofs: the two horizontal axes)
float3 ax = abs(n);
float2 pl = ax.z > 0.7 ? WP.xy : float2(dot(WP.xy, normalize(float2(-n.y, n.x) + 1e-4)), WP.z);
float3 c = Col; Rough = 0.8; Metal = 0.0; Emis = 0.0; Nrm = n;
int k = (int)round(Kind);
if (k == 0) {            // dry-stacked stone wall: irregular courses, dark gaps, rock detail from the scan texture
  float rowH = 21.0;
  float row = floor(pl.y / rowH);
  float off = sp.H(float2(row, 3.1)) * 40.0;
  float len = 28.0 + 26.0 * sp.H(float2(row, 7.7));
  float cx = floor((pl.x + off) / len);
  float2 f = float2(frac((pl.x + off) / len), frac(pl.y / rowH));
  float edge = min(min(f.x, 1.0 - f.x) * len, min(f.y, 1.0 - f.y) * rowH);
  float gap = 1.0 - smoothstep(0.8, 2.4, edge);
  float3 rk = Rock.SampleLevel(RockSampler, pl / 160.0, 0).rgb;
  rk = lerp(dot(rk, float3(0.333, 0.333, 0.333)).xxx, rk, 0.35) * float3(1.1, 0.96, 0.8) * 0.62;   // smoke-warm grey-brown
  float tone = 0.55 + 0.5 * sp.H(float2(cx, row)) + 0.15 * (sp.Fbm(pl / 9.0) - 0.5);
  c = lerp(rk * tone, float3(0.035, 0.03, 0.026), gap * 0.9);
  float bump = sp.Fbm(pl / 6.0);
  float2 g = float2(sp.Fbm(pl / 6.0 + float2(0.5, 0)) - bump, sp.Fbm(pl / 6.0 + float2(0, 0.5)) - bump);
  float2 tilt = (float2(sp.H(float2(cx, row + 0.5)), sp.H(float2(cx + 0.5, row))) - 0.5) * 0.35;   // every stone sits at its own angle
  g = g * 1.2 + tilt;
  Nrm = normalize(n + 0.9 * (ax.z > 0.7 ? float3(g, 0) : float3(normalize(float2(-n.y, n.x)) * g.x, g.y)) - n * gap * 0.3);
  Rough = 0.92;
} else if (k == 1) {     // old timber planks: seams every 16 cm, grain along the plank, smoke-dark
  float seam = 1.0 - smoothstep(0.0, 0.06, min(frac(pl.y / 16.0), 1.0 - frac(pl.y / 16.0)));
  float plank = floor(pl.y / 16.0);
  float grain = sp.Fbm(float2(pl.x / 40.0, pl.y * 0.9 + plank * 9.0)) * 0.6 + 0.4 * sin(pl.x * 0.35 + sp.Fbm(pl * 0.2) * 6.0);
  c = Col * (0.75 + 0.25 * grain) * (0.85 + 0.3 * sp.H(float2(plank, 1.3))) * (1.0 - 0.6 * seam);
  Rough = 0.75;
} else if (k == 2) {     // corrugated tin roof, painted blue, rusted where water runs
  float w = sin(pl.x * 6.2831 / 7.6);
  Nrm = normalize(n + float3(normalize(float2(-n.y, n.x) + 1e-4) * cos(pl.x * 6.2831 / 7.6) * 0.35, 0));
  float rust = smoothstep(0.55, 0.85, sp.Fbm(float2(pl.x / 30.0, pl.y / 140.0)));
  c = lerp(Col * (0.9 + 0.1 * w), float3(0.28, 0.12, 0.05), rust * 0.7);
  Rough = lerp(0.45, 0.9, rust); Metal = lerp(0.6, 0.1, rust);
} else if (k == 3) {     // Tibetan rug: border bands, a field of knots, a centre medallion
  float2 q = abs(UV - 0.5) * 2.0;
  float bx = max(q.x, q.y);
  float3 red = float3(0.33, 0.04, 0.03), saff = float3(0.6, 0.33, 0.05), navy = float3(0.03, 0.05, 0.16), ivory = float3(0.55, 0.5, 0.4);
  c = red;
  c = lerp(c, navy, step(0.86, bx));
  c = lerp(c, saff, step(0.9, bx) * (1.0 - step(0.94, bx)));
  float2 r = frac(UV * float2(9.0, 6.0)) - 0.5;
  c = lerp(c, saff, step(abs(r.x) + abs(r.y), 0.18) * step(bx, 0.8));
  float med = length((UV - 0.5) * float2(1.0, 1.4));
  c = lerp(c, navy, step(med, 0.17)); c = lerp(c, ivory, step(med, 0.1) * step(0.07, med));
  c *= 0.85 + 0.15 * sp.N1(UV * 400.0);
  Rough = 0.95;
} else if (k == 4) {     // hammered copper
  float hm = sp.N1(pl * 0.9);
  c = float3(0.75, 0.38, 0.2) * (0.8 + 0.3 * hm);
  Nrm = normalize(n + (hm - 0.5) * 0.25);
  Metal = 1.0; Rough = 0.35;
} else if (k == 5) {     // blackened iron
  c = float3(0.035, 0.033, 0.03) * (0.8 + 0.4 * sp.Fbm(pl / 8.0)); Metal = 0.7; Rough = 0.6;
} else if (k == 6) {     // khata silk: white, a faint woven pattern
  c = float3(0.82, 0.8, 0.74) * (0.94 + 0.06 * sin(UV.x * 900.0) * sin(UV.y * 900.0));
  Rough = 0.35;
} else if (k == 7) {     // butter-lamp flame / ember glow, flickering
  float fl = 0.8 + 0.2 * sin(T * 11.0 + WP.x) * sin(T * 7.3 + WP.y);
  c = float3(0.0, 0.0, 0.0); Emis = Col * fl * 40.0; Rough = 1.0;
} else if (k == 8) {     // a photograph: the picture inside a white border, a little yellowed
  float2 m = abs(UV - 0.5);
  float inside = step(max(m.x, m.y), 0.44);
  float3 pic = Photo.SampleLevel(PhotoSampler, (UV - 0.06) / 0.88, 0).rgb;
  c = lerp(float3(0.78, 0.74, 0.64), pic * float3(1.0, 0.95, 0.85), inside);
  Rough = 0.55;
} else if (k == 9) {     // expedition tent nylon: panels, seams, wrinkles, a sheen
  float pan = frac(UV.x * 6.0);
  float seam = 1.0 - smoothstep(0.0, 0.015, min(pan, 1.0 - pan));
  float wr = sp.Fbm(float2(UV.x * 60.0, UV.y * 14.0));
  c = Col * (0.88 + 0.16 * wr) * (1.0 - 0.35 * seam);
  float2 g = float2(sp.Fbm(float2(UV.x * 60.0 + 0.3, UV.y * 14.0)) - wr, sp.Fbm(float2(UV.x * 60.0, UV.y * 14.0 + 0.3)) - wr);
  Nrm = normalize(n + float3(g * 0.8, 0.0));
  Rough = 0.5;
} else if (k == 10) {    // open water under ice: black-blue, glassy, shivering
  float rp = sin(WP.x * 0.3 + T * 2.0) * sin(WP.y * 0.27 - T * 1.7);
  c = float3(0.004, 0.012, 0.02); Nrm = normalize(n + float3(rp * 0.05, rp * 0.04, 0)); Rough = 0.04;
} else if (k == 12) {    // painted cloth (thangka-like): rings and a square palace, mineral colours
  float2 p = (UV - 0.5) * 2.0;
  float r = length(p), s = max(abs(p.x), abs(p.y));
  c = float3(0.05, 0.12, 0.1);
  c = lerp(c, float3(0.55, 0.32, 0.06), step(abs(r - 0.82), 0.04));
  c = lerp(c, float3(0.4, 0.05, 0.04), step(s, 0.55) * step(0.47, s));
  c = lerp(c, float3(0.06, 0.14, 0.35), step(s, 0.47));
  c = lerp(c, float3(0.62, 0.55, 0.38), step(r, 0.16));
  c *= (0.85 + 0.15 * sp.N1(UV * 300.0)) * 0.55;   // old, smoke-darkened cloth
  Rough = 0.9;
} else {                 // plain, rough
  c = Col * (0.9 + 0.1 * sp.Fbm(pl / 20.0));
}
return c;
