# Glacier pass 2 (2026-11-11, user: "more random, more distinct from the snow"; dark lines in the icefall floor).
# Same edits on the landscape and the detail-tile Glacier sources, then push_glacier.py puts them into the materials.
#  - no more hairline cracks (the thin dark-blue lines on the icefall floor)
#  - ice clearly blue-grey, not snow white; more bare ice, less snow lying on it
#  - broken into irregular plates (Voronoi, 22 m and 6.5 m): each plate its own tint and tilt, catching the light differently
R = "F:/UE_project/YggdrasilsPeak/Tools/Materials"
EDITS = [
("GlacierFx gf;\n",
 """GlacierFx gf;
struct IcePlates {
  float2 H2(float2 p){ return frac(sin(float2(dot(p,float2(127.1,311.7)),dot(p,float2(269.5,183.3))))*43758.5453); }
  // nearest cell of a jittered grid: xy = that cell's two random numbers, z = distance to its centre
  float3 Cell(float2 p){ float2 i=floor(p), f=frac(p); float best=8.0; float2 id=i;
    for(int y=-1;y<=1;y++) for(int x=-1;x<=1;x++){ float2 g=float2(x,y); float2 r=g+H2(i+g)-f; float d=dot(r,r); if(d<best){best=d; id=i+g;} }
    return float3(H2(id+7.3), sqrt(best)); }
};
IcePlates ipl;
"""),
("""ic = lerp(ic, float3(0.10, 0.30, 0.55), cr * 0.8);                                     // cracks: deep blue
""",
 """// broken plates of ice, each with its own tint and tilt (no crack lines: they read as dark wires on the floor)
float3 pl1 = ipl.Cell(WP.xy / 2200.0), pl2 = ipl.Cell(gf.Rot(WP.xy, 0.9) / 650.0 + 3.1);
ic *= lerp(0.84, 1.10, pl1.x) * lerp(0.93, 1.05, pl2.y);
ic = lerp(ic, ic * float3(0.86, 0.95, 1.06), smoothstep(0.55, 0.9, pl1.y));          // some plates bluer
float2 plateTilt = (pl1.xy - 0.5) * 0.22 + (pl2.xy - 0.5) * 0.09;
"""),
("""ic = lerp(float3(0.60, 0.77, 0.90), float3(0.88, 0.93, 0.97), saturate(scanT * 0.6 + (1.0 - clear) * 0.55));""",
 """ic = lerp(float3(0.42, 0.63, 0.80), float3(0.70, 0.80, 0.88), saturate(scanT * 0.6 + (1.0 - clear) * 0.55));   // blue-grey: never snow white"""),
("""slope -= scanN * 0.55;
""",
 """slope -= scanN * 0.55;
slope += plateTilt;
"""),
("""ic = lerp(ic, float3(0.16, 0.34, 0.52), cr * 0.6);
""", ""),
("""float snowKeepBare = smoothstep(0.44, 0.56, patchN + (n.z - 0.85) * 0.8);   // more bare ice""",
 """float snowKeepBare = smoothstep(0.54, 0.64, patchN + (n.z - 0.85) * 0.8);   // much more bare ice (2026-11-11: read as snow)"""),
]


def patch(s):
    s = s.replace("\r\n", "\n")
    for a, b in EDITS:
        if a not in s:
            print("  (not present, skipped):", a[:50].replace("\n", " "))
            continue
        assert s.count(a) == 1, a[:50]
        s = s.replace(a, b)
    return s


for f in ["M_EverestLandscape_Glacier.hlsl", "M_EverestDetailTile_Glacier.hlsl"]:
    src = open(f"{R}/{f}", encoding="utf8").read()
    if "IcePlates" in src:
        print(f, "already patched"); continue
    open(f"{R}/{f}", "w", encoding="utf8", newline="\n").write(patch(src))
    print(f, "patched")
