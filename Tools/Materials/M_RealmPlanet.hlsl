// M_RealmPlanet - the nine realm planets of Yggdrasil's Peak (Docs/ArtDirection/realms, Nine Realms brief).
// One unlit sky material, one branch per realm. The planet is a sphere ~5000 km away; everything is shaded from the
// world-space direction (no UVs needed except Midgard's map). Lit by the real sun (crescents and terminators follow
// the time of day) plus a little fill so a far side never reads as a hole in the sky.
// Inputs: WP (world pos), Ctr (object centre), Cam (camera pos), SunDir (towards the sun), Sky (sky colour behind
// the pixel), Tex (Midgard daymap), Realm (0..9), Spin (rad), Bright, WashLow (how much it fades into the day sky
// low on the mountain), Relief (surface bump strength), AP (sky atmosphere aerial perspective at the pixel: rgb inscatter, a transmittance)
struct RealmFx {
  float H(float3 p){ p=frac(p*0.3183099+0.1); p*=17.0; return frac(p.x*p.y*p.z*(p.x+p.y+p.z)); }
  float VN(float3 p){ float3 i=floor(p), f=frac(p); f=f*f*(3.0-2.0*f);
    return lerp(lerp(lerp(H(i),H(i+float3(1,0,0)),f.x),lerp(H(i+float3(0,1,0)),H(i+float3(1,1,0)),f.x),f.y),
                lerp(lerp(H(i+float3(0,0,1)),H(i+float3(1,0,1)),f.x),lerp(H(i+float3(0,1,1)),H(i+1.0),f.x),f.y),f.z); }
  float Fbm(float3 p){ float s=0,a=0.5; for(int o=0;o<6;o++){ s+=a*VN(p); p=p*2.03+float3(1.7,9.2,3.1); a*=0.5; } return s; }
  // distance to the nearest / second nearest cell point (cracks, craters)
  float2 Cell(float3 p){ float3 i=floor(p), f=frac(p); float d1=8, d2=8;
    for(int z=-1;z<=1;z++) for(int y=-1;y<=1;y++) for(int x=-1;x<=1;x++){
      float3 g=float3(x,y,z); float3 o=float3(H(i+g),H(i+g+19.1),H(i+g+47.3));
      float d=length(g+o-f); if(d<d1){d2=d1;d1=d;} else if(d<d2){d2=d;} }
    return float2(d1,d2); }
  float3 Spin(float3 v, float a){ float s,c; sincos(a,s,c); return float3(c*v.x-s*v.y, s*v.x+c*v.y, v.z); }
};
RealmFx r;

float3 N = normalize(WP - Ctr);
float3 V = normalize(Cam - WP);
float3 L = normalize(SunDir);
// planet frame: the point facing the viewer is at longitude Spin, latitude Tilt (radians). Build an orthonormal
// basis from the view: F = towards viewer, Up = world up projected, Rt = right. Then express N in it and rotate.
float3 Fv = normalize(Cam - Ctr);
float3 Uv = normalize(float3(0, 0, 1) - Fv * Fv.z);
float3 Rv = cross(Uv, Fv);
float3 nl = float3(dot(N, Fv), dot(N, Rv), dot(N, Uv));        // x towards viewer, y right, z up
float sT, cT; sincos(Tilt, sT, cT);
nl = float3(nl.x * cT - nl.z * sT, nl.y, nl.x * sT + nl.z * cT); // tip so latitude Tilt sits at the centre
float sS, cS; sincos(Spin, sS, cS);
float3 P = float3(nl.x * cS - nl.y * sS, nl.x * sS + nl.y * cS, nl.z);   // longitude Spin at the centre
float ndl = dot(N, L);
float lit = smoothstep(-0.06, 0.30, ndl);
float facing = saturate(dot(N, V));
float rim = pow(1.0 - facing, 3.0);
// sun direction projected onto the disk as seen: where a crescent / the eclipse bead sits
float3 Lp = normalize(L - V * dot(L, V) + 1e-4);
int R = (int)round(Realm);
float3 A = 0.5; float3 E = 0.0; float amb = 0.10; float wash = WashLow;
Op = 1.0;

if (R == 0) {            // Midgard: ocean blue, curling white cloud bands, a fine blue atmosphere on the sunlit edge
  // world frame: X east, Y SOUTH (UE), Z up. Planet north pole = +Z. Longitude grows eastward = toward -Y seen from +X,
  // so lon = atan2(-P.y, P.x); u = lon/2pi + 0.5 puts Greenwich at u 0.5 like the equirect map.
  float lat = asin(clamp(P.z, -1.0, 1.0)), lon = -atan2(P.y, P.x);
  float3 earth = Tex.SampleLevel(TexSampler, float2(lon / 6.2831853 + 0.5, 0.5 - lat / 3.1415927), 0).rgb;
  float3 q = P * 2.5; float3 w = float3(r.Fbm(q + 1.3), r.Fbm(q + 7.9), r.Fbm(q + 4.4));
  float3 w2 = float3(r.Fbm(P * 5.0 + w * 3.0), r.Fbm(P * 5.0 + w * 3.0 + 3.3), r.Fbm(P * 5.0 + 6.1));
  float c = r.Fbm(P * 2.0 + w2 * 1.8);
  // curling bands: thin streaks where the warped field crosses a level, plus a few thicker storm patches
  float streak = 1.0 - smoothstep(0.0, 0.06, abs(c - 0.5));
  float cloud = saturate(streak * 0.55 + smoothstep(0.66, 0.76, c) * 0.7) * smoothstep(0.38, 0.58, r.Fbm(P * 1.2 + 4.0));
  cloud *= 0.75 + 0.25 * r.Fbm(P * 14.0);    // soft, not speckled
  A = lerp(earth * float3(0.9, 0.95, 1.05), float3(0.95, 0.96, 0.98), cloud * 0.85);
  E = float3(0.30, 0.55, 1.0) * rim * (lit * 1.4 + 0.15);
  amb = 0.06;
}
else if (R == 1) {       // Niflheim: pale ivory, matte frozen surface, shallow blue-gray fractures, frost layers on the lower rim
  float n = r.Fbm(P * 5.0);
  A = float3(0.82, 0.78, 0.69) * (0.78 + 0.32 * n) * (0.85 + 0.3 * r.Fbm(P * 18.0));
  float2 c = r.Cell(P * 5.0 + n * 0.8);
  float crack = 1.0 - smoothstep(0.0, 0.035, c.y - c.x);
  A = lerp(A, float3(0.42, 0.50, 0.60), crack * 0.8);
  // the lower rim (towards world-down, near the limb): uneven terraces of white ice catching the light
  float low = smoothstep(0.15, -0.45, P.z + (r.Fbm(P * 4.0) - 0.5) * 0.35);          // the lower face, ragged edge
  float terr = frac((P.z + r.Fbm(P * 5.0) * 0.22 + r.Fbm(P * 17.0) * 0.05) * 11.0);
  float ledge = smoothstep(0.55, 0.9, terr);
  float3 frost = float3(0.97, 0.97, 0.96) * (0.70 + 0.35 * ledge) * (0.85 + 0.3 * r.Fbm(float3(P.xy * 30.0, P.z * 4.0)));
  A = lerp(A, frost, low);
  A *= 1.0 - 0.45 * low * (1.0 - ledge);     // shadowed undersides between the ice layers
  E = float3(0.85, 0.88, 0.95) * rim * lit * 0.35;
  amb = 0.07; wash = WashLow * 0.6;
}
else if (R == 2) {       // Muspelheim: charcoal-red crust, ONE dominant diagonal glowing fissure (orange-white core -> copper)
  float n = r.Fbm(P * 4.0), m = r.Fbm(P * 11.0 + 3.0);
  A = lerp(float3(0.06, 0.018, 0.012), float3(0.34, 0.07, 0.03), smoothstep(0.35, 0.8, n)) * (0.7 + 0.5 * m);
  float3 F = normalize(float3(0.15, 0.62, 0.62));    // diagonal across the visible face (top-left -> bottom-right)
  float d = dot(P, F) + (r.Fbm(P * 5.0 + 9.0) - 0.5) * 0.22 + (r.Fbm(P * 20.0) - 0.5) * 0.06;
  float core = 1.0 - smoothstep(0.0, 0.010, abs(d));
  float glow = 1.0 - smoothstep(0.0, 0.07, abs(d));
  // a few subdued side cracks branching from it
  float2 c = r.Cell(P * 9.0 + n); float side = (1.0 - smoothstep(0.0, 0.02, c.y - c.x)) * (1.0 - smoothstep(0.04, 0.30, abs(d)));
  A *= 1.0 - glow * 0.4;
  E = float3(1.0, 0.35, 0.06) * (glow * glow * 2.2 + side * 0.35) + float3(1.0, 0.8, 0.5) * core * 6.0;
  amb = 0.08; wash = 0.0;
}
else if (R == 3) {       // Jötunheim: layers of slate, broad mineral bands at an angle, pale seams; a storm bank hides the lower edge
  float3 T = normalize(float3(0.1, 0.45, 0.85));     // bands tilted across the face
  float n = r.Fbm(P * 3.5);
  float s = dot(P, T) * 6.0 + n * 1.2 + r.Fbm(P * 14.0) * 0.25;
  float band = frac(s);
  float3 slate = lerp(float3(0.05, 0.055, 0.065), float3(0.24, 0.25, 0.28), smoothstep(0.2, 0.8, r.Fbm(P * 6.0 + floor(s) * 3.1)));
  float seam = smoothstep(0.88, 0.92, band) * (1.0 - smoothstep(0.94, 0.98, band)) * smoothstep(0.35, 0.6, r.Fbm(P * 7.0 + 3.0));   // broken seams
  A = lerp(slate, float3(0.55, 0.56, 0.58), seam * 0.8);
  A *= 0.8 + 0.4 * smoothstep(0.0, 1.0, frac(s * 0.5));    // alternating darker / paler layers
  // storm bank: dark rolling cloud over the lower part of the disk
  float billow = r.Fbm(N * 5.0 + 2.0) + 0.5 * r.Fbm(N * 13.0 + 7.0);
  float storm = smoothstep(0.15, -0.25, P.z + (billow - 0.75) * 0.6);   // lower part of the disk (planet frame z = up)
  A = lerp(A, lerp(float3(0.05, 0.055, 0.07), float3(0.30, 0.31, 0.34), smoothstep(0.6, 1.1, billow)), storm);
  E = float3(0.7, 0.75, 0.85) * rim * lit * 0.3 * (1.0 - storm);
  amb = 0.10;
}
else if (R == 4) {       // Alfheim: pale enough to vanish into the day sky; a narrow mother-of-pearl crescent on one edge
  float n = r.Fbm(P * 4.0);
  float bands = r.Fbm(float3(P.xy * 2.0, P.z * 9.0) + n * 1.4);   // a pale pearl world with slow cloud bands under the film
  A = lerp(float3(0.42, 0.45, 0.58), float3(0.86, 0.84, 0.92), smoothstep(0.3, 0.72, bands)) * (0.8 + 0.3 * n);
  float edge = pow(1.0 - facing, 2.2) * smoothstep(-0.1, 0.8, dot(N, Lp));
  float hue = (1.0 - facing) * 3.0 + n * 1.5;
  float3 pearl = 0.6 + 0.4 * cos(6.2831853 * (hue + float3(0.0, 0.33, 0.67)));
  float film = r.Fbm(P * 3.0 + r.Fbm(P * 6.0) * 2.0);
  float3 swirl = 0.55 + 0.45 * cos(6.2831853 * (film * 2.5 + float3(0.0, 0.33, 0.67)));
  E = pearl * edge * 1.6 + swirl * smoothstep(0.35, 0.85, film) * 0.1 * (0.4 + 0.6 * pow(1.0 - facing, 1.5)) + float3(1, 1, 1) * pow(1.0 - facing, 16.0) * 0.5;
  Op = 1.0;
  amb = 0.2; wash = 0.0;
}
else if (R == 5) {       // Vanaheim: deep jade gas giant - slow marbled currents folding into each other, one pale storm eye
  float3 w1 = float3(r.Fbm(P * 1.6), r.Fbm(P * 1.6 + 5.2), r.Fbm(P * 1.6 + 9.7));
  float3 w2 = float3(r.Fbm(P * 3.1 + w1 * 2.5), r.Fbm(P * 3.1 + w1 * 2.5 + 2.8), r.Fbm(P * 3.1 + 6.6));
  float lat = asin(clamp(P.z, -1.0, 1.0));
  float flow = r.Fbm(float3(P.xy * 1.2, lat * 5.0) + w2 * 2.4);
  float swirl = sin(lat * 9.0 + flow * 9.0);
  float cur = smoothstep(-0.3, 0.6, swirl);
  A = lerp(float3(0.008, 0.055, 0.04), float3(0.05, 0.25, 0.17), cur);
  A = lerp(A, float3(0.34, 0.56, 0.44), smoothstep(0.9, 0.995, swirl) * 0.55);    // pale foam where currents meet
  float3 eyeC = normalize(float3(0.75, -0.45, -0.25));
  float ed = length(P - eyeC);
  float eye = 1.0 - smoothstep(0.0, 0.14, ed + (r.Fbm(P * 14.0) - 0.5) * 0.04);
  A = lerp(A, lerp(float3(0.12, 0.40, 0.28), float3(0.70, 0.88, 0.72), smoothstep(0.3, 0.9, eye)), eye);
  E = float3(0.25, 0.95, 0.55) * rim * 0.18;
  amb = 0.06;
}
else if (R == 6) {       // Svartalfheim: nearly black face; a thin continuous silver ring; one bright bead where light escapes
  float n = r.Fbm(P * 6.0);
  A = float3(0.006, 0.0065, 0.009) * (0.5 + 1.1 * n);   // auto exposure at altitude lifts any grey: keep it near black
  float ring = pow(1.0 - facing, 34.0) * (0.65 + 0.7 * r.Fbm(N * 7.0)) + pow(1.0 - facing, 9.0) * 0.05;
  float bead = pow(saturate(dot(N, Lp)), 200.0) * pow(1.0 - facing, 10.0);     // on the very edge, towards the sun
  E = float3(0.92, 0.94, 1.0) * ring * 6.0 + float3(1.0, 0.95, 0.85) * bead * 25.0;
  amb = 1.0; lit = 0.0; wash = 0.0;
}
else if (R == 7) {       // Hel, the Long Night Star: deep indigo, almost black, a short silver crescent on one side
  float n = r.Fbm(P * 5.0);
  float vein = (1.0 - smoothstep(0.0, 0.018, abs(r.Fbm(P * 3.0 + n * 1.4) - 0.5))) * smoothstep(0.35, 0.65, r.Fbm(P * 2.1 + 5.0));
  A = lerp(float3(0.010, 0.008, 0.032), float3(0.05, 0.04, 0.13), vein * 0.5 + n * 0.45);
  float cres = pow(1.0 - facing, 3.0) * smoothstep(0.55, 0.98, dot(N, Lp));   // a short crescent on one side only
  E = float3(0.80, 0.84, 0.95) * cres * 1.2 + float3(0.45, 0.4, 0.95) * vein * 0.015;
  amb = 1.0; lit = 0.0; wash = 0.0;
}
else if (R == 8) {       // Asgard: pale gold, soft shadow, craters, a brighter upper rim
  float n = r.Fbm(P * 3.0);
  A = lerp(float3(0.62, 0.42, 0.20), float3(0.88, 0.76, 0.54), smoothstep(0.3, 0.75, r.Fbm(float3(P.xy * 1.5, P.z * 7.0) + n))) * (0.85 + 0.25 * n);
  A *= lerp(float3(1, 1, 1), float3(1.0, 0.85, 0.7), 1.0 - lit);
  float2 c = r.Cell(P * 4.0 + 2.0);
  float crater = smoothstep(0.20, 0.17, c.x) * smoothstep(0.10, 0.16, c.x);
  float floorC = smoothstep(0.17, 0.12, c.x);
  A = A * (1.0 - floorC * 0.1) + float3(0.12, 0.09, 0.04) * crater;
  E = float3(1.0, 0.80, 0.45) * pow(1.0 - facing, 2.0) * saturate(N.z * 1.5 + 0.4) * 1.6;
  amb = 0.08;
}
else {                   // 9: Hel's trail - a broken river of light: few bright knots, many dim points, empty stretches
  float3 q = WP / 2.0e6;
  float star = r.H(floor(q * 90.0));
  float knot = smoothstep(0.975, 1.0, star) * 4.0 + smoothstep(0.80, 0.86, star) * 0.6;
  float gaps = smoothstep(0.35, 0.6, r.Fbm(q * 3.0));
  Op = saturate(knot * gaps);
  return float3(0.85, 0.9, 1.0) * knot * gaps * Bright;
}

// weight: a solid body, not a disc. Bumped normal from the surface pattern catches the sun at the terminator,
// limb darkening rounds the sphere, a crisp day/night line, very little fill on the night side.
// each realm its own relief pattern and scale (seeded by the realm index), so no two share the same shadow blotches
float3 Q = P * (6.0 + Realm * 1.7) + Realm * 13.1;
float e = 0.04;
float3 gN = float3(r.Fbm(Q + float3(e,0,0)) - r.Fbm(Q - float3(e,0,0)),
                   r.Fbm(Q + float3(0,e,0)) - r.Fbm(Q - float3(0,e,0)),
                   r.Fbm(Q + float3(0,0,e)) - r.Fbm(Q - float3(0,0,e)));
float3 Nb = normalize(N - gN * Relief * 0.35);
// the relief only matters near the terminator; the day side stays clean
float litB = lerp(smoothstep(-0.02, 0.22, dot(N, L)), smoothstep(-0.02, 0.22, dot(Nb, L)), 1.0 - smoothstep(0.15, 0.5, dot(N, L)));
float limb = lerp(0.45, 1.0, pow(facing, 0.55));
float ambW = amb < 0.99 ? amb * 0.45 : amb;           // emissive-only realms (amb 1) keep their own light
float lt = amb < 0.99 ? litB : lit;
float3 col = A * (ambW + (1.0 - ambW) * lt) * limb * Bright + E * Bright;
// the engine adds the sky's aerial perspective on top of opaque surfaces (inscatter + transmittance at this pixel);
// looking up through thin air the transmittance stays high, so pre-compensate: the planet then shows its own colour
float Tr = 1.0; AP = float4(0, 0, 0, 1);   // translucent, fog off: no aerial perspective is added any more
float Elev = 6326.3 + (Cam.z - 129770.0) / 100.0;   // camera elevation, m
// low on the mountain the thicker air washes the planet into the day sky (Midgard: recognisable only high up)
float w = wash * saturate((8200.0 - Elev) / 3500.0);
col = lerp(col, Sky, w);
return max((col - AP.rgb * (1.0 - w)) / Tr, 0.0);
