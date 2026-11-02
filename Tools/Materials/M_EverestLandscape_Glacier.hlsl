// Glacier surfaces, after SnowSnakes. Mask = T_GlacierMask: real glacier outlines (OpenStreetMap natural=glacier,
// rasterised by Tools/RoutePlanning/glac_mask.py onto the heightmap grid: uv = (world xy + 504000) / (3374.24 * 1009)).
// Outside the mask everything passes through unchanged, so the snow and rock look elsewhere is untouched.
// Zones by elevation (game elevation, see SurvivalConfig): debris-covered tongue below ~5350 m (grey moraine rubble
// with patchy snow and dark ice faces), bare ice ~5350-6000 m (Base Camp / Icefall), snow-covered glacier above
// (only steep ice shows through). Explicit mips everywhere: ray-tracing hit shaders have no derivatives.
struct GlacierFx {
  float H1(float2 p){ return frac(sin(dot(p,float2(127.1,311.7)))*43758.5453); }
  float VN(float2 p){ float2 i=floor(p), f=frac(p); f=f*f*(3.0-2.0*f);
    return lerp(lerp(H1(i),H1(i+float2(1,0)),f.x),lerp(H1(i+float2(0,1)),H1(i+1.0),f.x),f.y); }
  float Fbm(float2 p){ return 0.5*VN(p)+0.3*VN(p*2.13+17.3)+0.2*VN(p*4.71+5.1); }
  float2 Rot(float2 v, float a){ float s,c; sincos(a,s,c); return float2(c*v.x-s*v.y, s*v.x+c*v.y); }
  float2 G2(float2 i){ float a=H1(i)*6.2831853; return float2(cos(a),sin(a)); }
  // quintic gradient noise, -0.5..0.5-ish, smooth derivatives (value noise shows its grid when used for normals)
  float GN(float2 p){ float2 i=floor(p), f=frac(p); float2 u=f*f*f*(f*(f*6.0-15.0)+10.0);
    float a=dot(G2(i),f), b=dot(G2(i+float2(1,0)),f-float2(1,0)), c=dot(G2(i+float2(0,1)),f-float2(0,1)), d=dot(G2(i+1.0),f-1.0);
    return lerp(lerp(a,b,u.x),lerp(c,d,u.x),u.y); }
  float GF(float2 p){ return GN(p)+0.5*GN(Rot(p,1.7)*2.03+7.1)+0.25*GN(Rot(p,4.1)*4.11+3.3); }
  // thin ridged lines (crevasse cracks / veins): 1 on the line, 0 away from it
  float Lines(float2 p, float w){ float v=VN(p); return 1.0-smoothstep(0.0,w,abs(v-0.5)); }
};
GlacierFx gf;

NrmOut = N;
RghOut = Rgh;
float2 uv = (WP.xy + 504000.0) / (3374.24 * 1009.0);
float g = Mask.SampleLevel(MaskSampler, uv, 0).r * (1.0 - smoothstep(0.35, 0.65, Rock));
if (g < 0.005) return Col;

float3 n = normalize(float3(VN.xy, VN.z * 1.774)); // landscape normal arrives unscaled (2x landscape ratio)
float elev = 6326.3 + (WP.z - 129770.0) / 100.0;
float dist = length(WP - Cam);
float wob = (gf.Fbm(WP.xy / 9000.0) - 0.5) * 300.0;      // ragged zone borders, not contour lines
// terrain sits ~85 m below the real elevations around Base Camp (34 m/px DEM), so the real ~5350 m transition is at ~5270
float debris = 1.0 - smoothstep(5170.0, 5330.0, elev + wob);
float bare = (1.0 - debris) * (1.0 - smoothstep(5850.0, 6100.0, elev + wob));
float steep = 1.0 - smoothstep(0.72, 0.92, n.z);

// ---- moraine rubble (two scales, rotated against each other so the tiling never lines up)
float2 p1 = WP.xy / 450.0;
float2 p2 = gf.Rot(WP.xy, 1.13) / 1700.0 + 0.37;
float mip1 = clamp(log2(dist / 140.0), 0.0, 11.0);
float mip2 = clamp(log2(dist / 530.0), 0.0, 11.0);
float3 c1 = MorC.SampleLevel(MorCSampler, p1, mip1).rgb;
float3 c2 = MorC.SampleLevel(MorCSampler, p2, mip2).rgb;
float mixA = smoothstep(0.25, 0.75, gf.Fbm(WP.xy / 1300.0 + 3.3));
float2 p3 = gf.Rot(WP.xy, 2.41) / 6500.0 + 0.71;
float3 c3 = MorC.SampleLevel(MorCSampler, p3, clamp(log2(dist / 2000.0), 0.0, 11.0)).rgb;
float far = smoothstep(1200.0, 6000.0, dist);                       // beyond ~20 m the 4.5 m tile shows as a grid: fade to big scales
float3 mc = lerp(lerp(c1, c2, mixA), lerp(c2, c3, 0.6), far);
mc = lerp(dot(mc, float3(0.3, 0.59, 0.11)).xxx, mc, 0.2) * float3(0.98, 0.97, 0.96); // Khumbu granite is grey
mc *= lerp(0.75, 1.15, gf.Fbm(WP.xy / 6000.0 + 8.1));                                // big light/dark patches
float3 t1 = MorN.SampleLevel(MorNSampler, p1, mip1).xyz * 2.0 - 1.0;
float3 t2 = MorN.SampleLevel(MorNSampler, p2, mip2).xyz * 2.0 - 1.0;
float2 tn = lerp(t1.xy, gf.Rot(t2.xy, -1.13), mixA) * float2(1.0, -1.0) * (1.0 - far * 0.7); // DirectX green
float3 mN = normalize(float3(n.xy + tn * 0.9, n.z));
float mR = lerp(MorA.SampleLevel(MorASampler, p1, mip1).g, MorA.SampleLevel(MorASampler, p2, mip2).g, mixA);

// ---- bare glacier ice: blue-white, dirt bands across the flow, hairline cracks
float bands = gf.Fbm(float2(WP.x / 2200.0, WP.y / 9000.0) + 2.0);
// cracks: short straight hairlines across the flow (glacier flows roughly along +x here), staggered and broken
float2 cq = gf.Rot(WP.xy, 0.15) / float2(170.0, 1.0) ;
float row = floor(cq.x);
float cs = frac(cq.x) - 0.5 + (gf.VN(float2(row, WP.y / 900.0)) - 0.5) * 0.5;      // slight wobble along the crack
float cr = (1.0 - smoothstep(0.004, 0.02, abs(cs))) * step(0.55, gf.VN(float2(row * 3.7, WP.y / 700.0)))
         * smoothstep(0.45, 0.6, gf.Fbm(WP.xy / 2500.0)) * (1.0 - smoothstep(2500.0, 6000.0, dist));
// old glacier ice is grey-blue, not white: weathered milky surface, clear blue where it is polished
float clear = smoothstep(0.55, 0.8, gf.Fbm(WP.xy / 900.0 + 1.7));
float3 ic = lerp(float3(0.62, 0.70, 0.76), float3(0.42, 0.62, 0.78), clear);
ic *= lerp(0.85, 1.08, gf.Fbm(WP.xy / 2600.0 + 9.2));
ic = lerp(ic, float3(0.45, 0.45, 0.44), smoothstep(0.55, 0.8, bands) * 0.55);          // dirt bands (ogives)
float grit = smoothstep(0.55, 0.9, dot(MorC.SampleLevel(MorCSampler, WP.xy / 220.0, clamp(log2(dist / 70.0), 0.0, 11.0)).rgb, 0.33.xxx));
ic = lerp(ic, float3(0.36, 0.35, 0.34), grit * 0.55 * (1.0 - clear));                   // wind-blown grit in the milky ice
ic = lerp(ic, float3(0.16, 0.34, 0.52), cr * 0.85);                                     // cracks: deep blue
// hummocks (metres) as normal relief, from smooth gradient noise
float e = 30.0;
float2 q0 = WP.xy / 1100.0;
float h0 = gf.GF(q0), hx = gf.GF(q0 + float2(e / 1100.0, 0)), hy = gf.GF(q0 + float2(0, e / 1100.0));
float2 slope = float2(hx - h0, hy - h0) / e * 70.0;
// fine weathered texture near the camera (borrowed from the rubble normal map, weak)
float2 mt = MorN.SampleLevel(MorNSampler, gf.Rot(WP.xy, 0.8) / 160.0, clamp(log2(dist / 50.0), 0.0, 11.0)).xy * 2.0 - 1.0;
float near = 1.0 - smoothstep(1500.0, 5000.0, dist);
float3 iN = normalize(float3(n.xy - slope + mt * float2(1.0, -1.0) * lerp(0.25, 0.08, clear) * near, n.z));
float iR = lerp(0.32, 0.08, clear);

// ---- where the old snow stays: hollows and flats keep the existing snow surface (Col/N/Rgh)
// crisp, ragged edges: a fine noise breaks the border so rubble pokes through the snow
float patchN = gf.Fbm(WP.xy / 1400.0 + 11.0) + (gf.Fbm(WP.xy / 90.0 + 3.0) - 0.5) * 0.16 + (gf.VN(WP.xy / 25.0) - 0.5) * 0.05;
float snowKeepDebris = smoothstep(0.63, 0.655, patchN + (n.z - 0.9) * 0.6);
float snowKeepBare = smoothstep(0.55, 0.575, patchN + (n.z - 0.85) * 0.8);

// debris tongue: rubble, with dark old ice on its steep faces
float3 dCol = lerp(mc, float3(0.32, 0.38, 0.44), steep * 0.6);
float3 dN = normalize(lerp(mN, iN, steep * 0.6));
float dR = lerp(mR, 0.3, steep * 0.6);
// bare zone: ice, with some rubble streaks (medial moraines) along the flow
float med = smoothstep(0.62, 0.75, gf.Fbm(float2(WP.x / 7000.0, WP.y / 1800.0) + 6.0));
float3 bCol = lerp(ic, mc, med * 0.7);
float3 bN = normalize(lerp(iN, mN, med * 0.7));
float bR = lerp(iR, mR, med * 0.7);
// snow-covered upper glacier: only steep ice shows
float3 uCol = ic * 0.95;

float aD = g * debris * (1.0 - snowKeepDebris);
float aB = g * bare * (1.0 - snowKeepBare);
float aU = g * (1.0 - debris - bare) * steep * 0.6;
float3 outC = Col;
outC = lerp(outC, dCol, aD);
outC = lerp(outC, bCol, aB);
outC = lerp(outC, uCol, aU);
float3 outN = N;
outN = normalize(lerp(outN, dN, aD));
outN = normalize(lerp(outN, bN, aB));
outN = normalize(lerp(outN, iN, aU));
NrmOut = outN;
RghOut = lerp(lerp(lerp(Rgh, dR, aD), bR, aB), 0.25, aU);
return max(outC, 0.0);
