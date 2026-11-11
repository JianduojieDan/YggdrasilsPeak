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
struct IcePlates {
  float2 H2(float2 p){ return frac(sin(float2(dot(p,float2(127.1,311.7)),dot(p,float2(269.5,183.3))))*43758.5453); }
  // nearest cell of a jittered grid: xy = that cell's two random numbers, z = distance to its centre
  float3 Cell(float2 p){ float2 i=floor(p), f=frac(p); float best=8.0; float2 id=i;
    for(int y=-1;y<=1;y++) for(int x=-1;x<=1;x++){ float2 g=float2(x,y); float2 r=g+H2(i+g)-f; float d=dot(r,r); if(d<best){best=d; id=i+g;} }
    return float3(H2(id+7.3), sqrt(best)); }
};
IcePlates ipl;
struct MorHexFx {
// ---- detail tiles: moraine sampled in randomly rotated / offset hexagonal cells (no visible 4.5 m repeat).
// The same function runs in the displacement node, so the raised stones line up with the visible ones.
float2 MH2(float2 p){ return frac(sin(float2(dot(p,float2(127.1,311.7)),dot(p,float2(269.5,183.3))))*43758.5453); }
float2 MRot(float2 v, float a){ float s,c; sincos(a,s,c); return float2(c*v.x-s*v.y, s*v.x+c*v.y); }
void MHex(float2 uv, out float3 w, out float2 u1, out float2 u2, out float2 u3, out float3 ang)
{
  float2 s = float2(uv.x - uv.y * 0.57735027, uv.y * 1.15470054) * 1.6;
  float2 b = floor(s), f = frac(s); float z = 1.0 - f.x - f.y;
  float2 v1, v2, v3;
  if (z > 0) { w = float3(z, f.y, f.x); v1 = b; v2 = b + float2(0, 1); v3 = b + float2(1, 0); }
  else { w = float3(-z, 1.0 - f.y, 1.0 - f.x); v1 = b + 1.0; v2 = b + float2(1, 0); v3 = b + float2(0, 1); }
  ang = float3(MH2(v1 + 3.7).x, MH2(v2 + 3.7).x, MH2(v3 + 3.7).x) * 6.2831853;
  u1 = MRot(uv, ang.x) + MH2(v1); u2 = MRot(uv, ang.y) + MH2(v2); u3 = MRot(uv, ang.z) + MH2(v3);
}
};
MorHexFx mhx;

NrmOut = N;
RghOut = Rgh;
float2 uv = (WP.xy + 504000.0) / (3374.24 * 1009.0);
float g = Mask.SampleLevel(MaskSampler, uv, 0).r * (1.0 - smoothstep(0.35, 0.65, Rock));
float tr = saturate(TileRock);  // detail tiles: rubble map baked into UV0.x
if (g < 0.005 && tr < 0.005) return Col;

float3 n = normalize(N); // detail tiles: the VN input is not a usable normal on static meshes, use the surface normal // landscape normal arrives unscaled (2x landscape ratio)
float elev = 6326.3 + (WP.z - 129770.0) / 100.0;
float dist = length(WP - Cam);
float wob = (gf.Fbm(WP.xy / 9000.0) - 0.5) * 300.0;      // ragged zone borders, not contour lines
// terrain sits ~85 m below the real elevations around Base Camp (34 m/px DEM), so the real ~5350 m transition is at ~5270
float debris = 1.0 - smoothstep(5050.0, 5230.0, elev + wob);
float bare = (1.0 - debris) * (1.0 - smoothstep(6450.0, 6800.0, elev + wob));   // ice shows through the Icefall and up the Cwm
float steep = 1.0 - smoothstep(0.72, 0.92, n.z);

// ---- moraine rubble (two scales, rotated against each other so the tiling never lines up)
float2 p1 = WP.xy / 450.0;
float2 p2 = gf.Rot(WP.xy, 1.13) / 1700.0 + 0.37;
// explicit mips (no derivatives in RT hit shaders) went by distance only: on slopes seen at a low angle the rubble
// was undersampled and smeared into streaks - also blur with the grazing angle
float graze = 0.5 * log2(1.0 / max(abs(dot(normalize(WP - Cam), n)), 0.12));
float mip1 = clamp(log2(dist / 140.0) + graze, 0.0, 11.0);
float mip2 = clamp(log2(dist / 530.0) + graze, 0.0, 11.0);
// biplanar: projected straight down on flat ground, from the side on slopes (straight-down projection stretched the
// rubble into downslope streaks on the valley walls)
float3 an = abs(n);
float wTop = smoothstep(0.6, 0.8, an.z);
float2 side = (an.x > an.y ? WP.yz : WP.xz) * float2(1.0, -1.0);
float3 hw; float2 hu1, hu2, hu3; float3 ha; mhx.MHex(p1, hw, hu1, hu2, hu3, ha);
float3 hc1 = MorC.SampleLevel(MorCSampler, hu1, mip1).rgb, hc2 = MorC.SampleLevel(MorCSampler, hu2, mip1).rgb, hc3 = MorC.SampleLevel(MorCSampler, hu3, mip1).rgb;
float3 hb3 = pow(max(hw, 0.0), 3.0) * (0.25 + float3(dot(hc1, 0.333.xxx), dot(hc2, 0.333.xxx), dot(hc3, 0.333.xxx)));
hb3 /= max(hb3.x + hb3.y + hb3.z, 1e-5);
float3 c1 = hc1 * hb3.x + hc2 * hb3.y + hc3 * hb3.z;
float3 c2 = MorC.SampleLevel(MorCSampler, p2, mip2).rgb;
if (wTop < 0.999)
{
  float m1 = clamp(log2(dist / 140.0), 0.0, 11.0), m2 = clamp(log2(dist / 530.0), 0.0, 11.0);
  c1 = lerp(MorC.SampleLevel(MorCSampler, side / 450.0, m1).rgb, c1, wTop);
  c2 = lerp(MorC.SampleLevel(MorCSampler, gf.Rot(side, 1.13) / 1700.0 + 0.37, m2).rgb, c2, wTop);
}
float mixA = smoothstep(0.25, 0.75, gf.Fbm(WP.xy / 1300.0 + 3.3));
float2 p3 = gf.Rot(WP.xy, 2.41) / 6500.0 + 0.71;
float3 c3 = MorC.SampleLevel(MorCSampler, p3, clamp(log2(dist / 2000.0) + graze, 0.0, 11.0)).rgb;
float far = smoothstep(1200.0, 6000.0, dist);                       // beyond ~20 m the 4.5 m tile shows as a grid: fade to big scales
float3 mc = lerp(lerp(c1, c2, mixA), lerp(c2, c3, 0.6), far);
mc = lerp(dot(mc, float3(0.3, 0.59, 0.11)).xxx, mc, 0.2) * float3(0.98, 0.97, 0.96); // Khumbu granite is grey
mc *= lerp(0.75, 1.15, gf.Fbm(WP.xy / 6000.0 + 8.1));                                // big light/dark patches
mc = pow(saturate(mc), 1.35) * float3(0.92, 0.96, 1.02);   // 2026-11-11: Khumbu moraine is dark cool granite, the texture read as tan sand
float3 t1 = float3(mhx.MRot(MorN.SampleLevel(MorNSampler, hu1, mip1).xy * 2.0 - 1.0, -ha.x) * hb3.x
                + mhx.MRot(MorN.SampleLevel(MorNSampler, hu2, mip1).xy * 2.0 - 1.0, -ha.y) * hb3.y
                + mhx.MRot(MorN.SampleLevel(MorNSampler, hu3, mip1).xy * 2.0 - 1.0, -ha.z) * hb3.z, 1.0);
float3 t2 = MorN.SampleLevel(MorNSampler, p2, mip2).xyz * 2.0 - 1.0;
float2 tn = lerp(t1.xy, gf.Rot(t2.xy, -1.13), mixA) * float2(1.0, -1.0) * (1.0 - far * 0.7); // DirectX green
float3 mN = normalize(float3(n.xy + tn * 0.9, n.z));
float mR = lerp(MorA.SampleLevel(MorASampler, hu1, mip1).g * hb3.x + MorA.SampleLevel(MorASampler, hu2, mip1).g * hb3.y + MorA.SampleLevel(MorASampler, hu3, mip1).g * hb3.z, MorA.SampleLevel(MorASampler, p2, mip2).g, mixA);

// ---- bare glacier ice: blue-white, dirt bands across the flow, hairline cracks
float bands = gf.Fbm(float2(WP.x / 2200.0, WP.y / 9000.0) + 2.0);
// cracks: short straight hairlines across the flow (glacier flows roughly along +x here), staggered and broken
float2 cq = gf.Rot(WP.xy, 0.15) / float2(170.0, 1.0) ;
float row = floor(cq.x);
float cs = frac(cq.x) - 0.5 + (gf.VN(float2(row, WP.y / 900.0)) - 0.5) * 0.5;      // slight wobble along the crack
float cr = (1.0 - smoothstep(0.004, 0.02, abs(cs))) * step(0.55, gf.VN(float2(row * 3.7, WP.y / 700.0)))
         * smoothstep(0.45, 0.6, gf.Fbm(WP.xy / 2500.0)) * (1.0 - smoothstep(2500.0, 6000.0, dist));
// glacier ice (2026-11-11 rewrite: the old teal-green blotches read as mould): weathered milky white over large calm
// areas of polished pale-blue ice; deep blue only in the cracks and on fresh steep faces
float clear = smoothstep(0.5, 0.85, gf.Fbm(WP.xy / 2600.0 + 1.7));
float3 ic = float3(0.86, 0.91, 0.95);
// real ice: the scanned ice cliff's own colour and surface, at two scales turned against each other so no tile repeats
float2 iu1 = gf.Rot(WP.xy, 0.7) / 1100.0;
float2 iu2 = gf.Rot(WP.xy, 2.3) / 4300.0 + 0.31;
float im1 = clamp(log2(dist / 340.0) + graze, 0.0, 11.0), im2 = clamp(log2(dist / 1300.0) + graze, 0.0, 11.0);
float3 sc1 = IceC.SampleLevel(IceCSampler, iu1, im1).rgb, sc2 = IceC.SampleLevel(IceCSampler, iu2, im2).rgb;
float iceMix = smoothstep(0.3, 0.7, gf.Fbm(WP.xy / 2100.0 + 2.2));
float3 scan = lerp(sc1, sc2, lerp(iceMix, 1.0, smoothstep(1500.0, 7000.0, dist)));
float scanL = dot(scan, float3(0.3, 0.59, 0.11));
// colour from the reference photos: the scan's detail only modulates milky white against pale polished blue
float scanT = smoothstep(0.05, 0.5, scanL);
ic = lerp(float3(0.42, 0.63, 0.80), float3(0.70, 0.80, 0.88), saturate(scanT * 0.2 + (1.0 - clear) * 0.7));   // blue-grey: never snow white
ic *= lerp(0.94, 1.03, gf.Fbm(WP.xy / 3000.0 + 9.2));
ic = lerp(ic, float3(0.62, 0.64, 0.66), smoothstep(0.62, 0.85, bands) * 0.22 * smoothstep(5900.0, 5500.0, elev)); // ogives: soft grey arcs, lower ice only
float grit = smoothstep(0.6, 0.92, dot(MorC.SampleLevel(MorCSampler, WP.xy / 220.0, clamp(log2(dist / 70.0), 0.0, 11.0)).rgb, 0.33.xxx));

// broken plates of ice, each with its own tint and tilt (no crack lines: they read as dark wires on the floor)
// soft random patches only (2026-11-11: Voronoi plates drew straight seams and dashed lines up close)
float pn1 = gf.Fbm(WP.xy / 1800.0 + 21.0), pn2 = gf.Fbm(gf.Rot(WP.xy, 0.9) / 520.0 + 3.1);
ic *= lerp(0.86, 1.08, pn1) * lerp(0.94, 1.04, pn2);
ic = lerp(ic, ic * float3(0.86, 0.95, 1.06), smoothstep(0.55, 0.8, gf.Fbm(WP.xy / 3500.0 + 5.5)));
float2 plateTilt = float2(0.0, 0.0);
// hummocks (metres) as normal relief, from smooth gradient noise: large swells, plus the sharp ribs and scallops of melting ice
float e = 30.0;
float2 q0 = WP.xy / 1100.0;
float h0 = gf.GF(q0), hx = gf.GF(q0 + float2(e / 1100.0, 0)), hy = gf.GF(q0 + float2(0, e / 1100.0));
float2 slope = float2(hx - h0, hy - h0) / e * 110.0;
float2 qR = gf.Rot(WP.xy, 0.5) / 260.0;
float r0 = abs(gf.GF(qR)), rx = abs(gf.GF(qR + float2(e / 260.0, 0))), ry = abs(gf.GF(qR + float2(0, e / 260.0)));
// (ribs from abs(noise) removed 2026-11-11: their creases drew thin straight lines up close)
float2 sn1 = IceN.SampleLevel(IceNSampler, iu1, im1).xy * 2.0 - 1.0, sn2 = IceN.SampleLevel(IceNSampler, iu2, im2).xy * 2.0 - 1.0;
float2 scanN = lerp(gf.Rot(sn1, -0.7), gf.Rot(sn2, -2.3), iceMix) * float2(1.0, -1.0);
// (tiled ice-scan normal removed 2026-11-11: its straight crack features drew the long lines on the glacier)
slope += plateTilt;
// fine weathered texture near the camera (borrowed from the rubble normal map, weak)
float2 mt = MorN.SampleLevel(MorNSampler, gf.Rot(WP.xy, 0.8) / 160.0, clamp(log2(dist / 50.0), 0.0, 11.0)).xy * 2.0 - 1.0;
float near = 1.0 - smoothstep(1500.0, 5000.0, dist);
float3 iN = normalize(float3(n.xy - slope, n.z));
float iR = lerp(0.55, 0.3, clear);   // weathered ice is not a mirror; lower values flashed metallic under a low sun
iR = lerp(iR, 0.62, smoothstep(0.3, 0.7, scanL));

// ---- where the old snow stays: hollows and flats keep the existing snow surface (Col/N/Rgh)
// crisp, ragged edges: a fine noise breaks the border so rubble pokes through the snow
float patchN = gf.Fbm(WP.xy / 3600.0 + 11.0) + (gf.Fbm(WP.xy / 420.0 + 3.0) - 0.5) * 0.06;   // big calm patches, not camouflage
float snowKeepDebris = smoothstep(0.68, 0.74, patchN + (n.z - 0.9) * 0.6);   // 2026-11-03: was 0.63-0.655, read as paper cut-outs
float snowKeepBare = smoothstep(0.54, 0.64, patchN + (n.z - 0.85) * 0.8);   // much more bare ice (2026-11-11: read as snow)

// debris tongue: rubble, with dark old ice on its steep faces
// hummocks: the rubble is darker in the hollows and lighter on the crests (tens of metres)
float hum = gf.GF(WP.xy / 1800.0 + 2.9);
mc *= lerp(0.78, 1.08, saturate(hum * 0.9 + 0.5));
// ice cliffs in the debris: dirty grey-blue walls striped by old layers, paler bands between
float strata = gf.VN(float2(WP.z / 45.0, gf.Fbm(WP.xy / 3000.0) * 4.0));
float3 cliff = lerp(float3(0.36, 0.42, 0.48), float3(0.66, 0.74, 0.80), smoothstep(0.3, 0.8, strata));
float3 dCol = lerp(mc, cliff, steep * 0.75);
float3 dN = normalize(lerp(mN, iN, steep * 0.75));
float dR = lerp(mR, 0.3, steep * 0.75);
// bare zone: ice, with some rubble streaks (medial moraines) along the flow
// medial moraines stop below the Icefall: higher up they read as a blurred dark smear on the snow (the dark patch near Camp 1)
float med = smoothstep(0.62, 0.75, gf.Fbm(float2(WP.x / 7000.0, WP.y / 1800.0) + 6.0)) * smoothstep(5750.0, 5500.0, elev + wob);
float3 bCol = lerp(ic, mc, med * 0.7);
float3 bN = normalize(lerp(iN, mN, med * 0.7));
float bR = lerp(iR, mR, med * 0.7);
// snow-covered upper glacier: only steep ice shows
float3 uCol = ic * 0.95;

// rubble/ice textures are projected flat (xy): on the valley walls they smear into streaks - keep them to the
// glacier floor and let the snow/rock surface (triplanar) carry the steeper sides
float floorOnly = smoothstep(0.55, 0.75, n.z);   // biplanar rubble holds up on moderate slopes now
float aD = g * debris * (1.0 - snowKeepDebris) * floorOnly;
float aB = g * bare * (1.0 - snowKeepBare) * lerp(0.35, 1.0, floorOnly);
float aU = g * (1.0 - debris - bare) * steep * 0.6;
// detail tiles: rubble shows through the snow where the rubble map says so. The stones decide the exact edge:
// dark stones poke through, the light gaps between them fill with snow, so the border follows the rubble at
// sub-metre scale (no 1 m grid steps) and breaks up into patches and specks like the South Col photos.
float lumM = dot(mc, float3(0.333, 0.333, 0.333));
float wob2 = (gf.Fbm(WP.xy / 350.0) - 0.5) * 0.5 + (gf.Fbm(WP.xy / 90.0 + 5.0) - 0.5) * 0.35;
float cover = tr + wob2 - (lumM - 0.45) * 0.35;
float aT = smoothstep(0.42, 0.58, cover) * smoothstep(0.5, 0.78, n.z);
// rubble tone: mottled light and dark over tens of metres, slightly warm grey-brown (South Col photo)
float3 mcT = mc * lerp(0.62, 1.08, gf.Fbm(WP.xy / 2500.0 + 4.2)) * float3(0.97, 0.98, 1.0);
aD = max(aD, aT);
dCol = lerp(dCol, mcT, aT); dN = normalize(lerp(dN, mN, aT)); dR = lerp(dR, mR, aT);
// frozen melt ponds in the flat hollows of the debris tongue: turquoise ice with a pale frozen rim (Khumbu photos)
float pondN = gf.Fbm(WP.xy / 11000.0 + 4.4) + (gf.Fbm(WP.xy / 900.0) - 0.5) * 0.03;
float pond = smoothstep(0.745, 0.75, pondN) * smoothstep(0.985, 0.995, n.z) * debris;
float rim = 1.0 - smoothstep(0.75, 0.77, pondN);
float3 pCol = lerp(float3(0.20, 0.52, 0.58), float3(0.72, 0.88, 0.90), rim * 0.8);
float aP = g * pond * floorOnly;
float3 outC = Col;
outC = lerp(outC, dCol, aD);
outC = lerp(outC, bCol, aB);
outC = lerp(outC, uCol, aU);
outC = lerp(outC, pCol, aP);
float3 outN = N;
outN = normalize(lerp(outN, dN, aD));
outN = normalize(lerp(outN, bN, aB));
outN = normalize(lerp(outN, iN, aU));
outN = normalize(lerp(outN, n, aP));
NrmOut = outN;
RghOut = lerp(lerp(lerp(lerp(Rgh, dR, aD), bR, aB), 0.25, aU), 0.12, aP);
return max(outC, 0.0);
