# Glacier surface rewrite (2026-11-11, user: "still very, very bad"). Applies the same edits to the landscape Glacier
# node source (Tools/Materials/M_EverestLandscape_Glacier.hlsl) and to the detail-tile copy (its own hex-sampled
# moraine), writes both to Tools/Materials/, then (inside the editor) pushes them into the two materials.
#   python -I glacier_rewrite_2026_11_11.py <tile_glacier_dump.hlsl>      -> patch files
#   uepy.py push_glacier.py                                                -> into the materials
import sys
ROOT = "F:/UE_project/YggdrasilsPeak/Tools/Materials"
EDITS = [
("""// old glacier ice is grey-blue, not white: weathered milky surface, clear blue where it is polished
float clear = smoothstep(0.55, 0.8, gf.Fbm(WP.xy / 900.0 + 1.7));
float3 ic = lerp(float3(0.68, 0.78, 0.84), float3(0.36, 0.60, 0.78), clear);
ic *= lerp(0.88, 1.08, gf.Fbm(WP.xy / 2600.0 + 9.2));
ic = lerp(ic, float3(0.50, 0.52, 0.53), smoothstep(0.62, 0.85, bands) * 0.35 * smoothstep(5900.0, 5500.0, elev)); // dirt bands (ogives), lower ice only
float grit = smoothstep(0.55, 0.9, dot(MorC.SampleLevel(MorCSampler, WP.xy / 220.0, clamp(log2(dist / 70.0), 0.0, 11.0)).rgb, 0.33.xxx));
ic = lerp(ic, float3(0.40, 0.40, 0.40), grit * 0.3 * (1.0 - clear) * smoothstep(5900.0, 5500.0, elev));   // wind-blown grit in the milky ice
ic = lerp(ic, float3(0.16, 0.34, 0.52), cr * 0.85);                                     // cracks: deep blue
""",
"""// glacier ice (2026-11-11 rewrite: the old teal-green blotches read as mould): weathered milky white over large calm
// areas of polished pale-blue ice; deep blue only in the cracks and on fresh steep faces
float clear = smoothstep(0.5, 0.85, gf.Fbm(WP.xy / 2600.0 + 1.7));
float3 ic = float3(0.86, 0.91, 0.95);
"""),
("""// colour from the reference photos, not the dirty Icelandic scan: its detail decides white or blue, blue only where it is darkest
float scanT = smoothstep(0.05, 0.5, scanL);
ic = lerp(float3(0.30, 0.60, 0.74), float3(0.90, 0.95, 0.98), scanT);
ic = lerp(ic, float3(0.16, 0.34, 0.52), cr * 0.6);
""",
"""// colour from the reference photos: the scan's detail only modulates milky white against pale polished blue
float scanT = smoothstep(0.05, 0.5, scanL);
ic = lerp(float3(0.60, 0.77, 0.90), float3(0.88, 0.93, 0.97), saturate(scanT * 0.6 + (1.0 - clear) * 0.55));
ic *= lerp(0.94, 1.03, gf.Fbm(WP.xy / 3000.0 + 9.2));
ic = lerp(ic, float3(0.62, 0.64, 0.66), smoothstep(0.62, 0.85, bands) * 0.22 * smoothstep(5900.0, 5500.0, elev)); // ogives: soft grey arcs, lower ice only
float grit = smoothstep(0.6, 0.92, dot(MorC.SampleLevel(MorCSampler, WP.xy / 220.0, clamp(log2(dist / 70.0), 0.0, 11.0)).rgb, 0.33.xxx));
ic = lerp(ic, float3(0.52, 0.52, 0.52), grit * 0.18 * (1.0 - clear) * smoothstep(5900.0, 5500.0, elev));
ic = lerp(ic, float3(0.10, 0.30, 0.55), cr * 0.8);                                     // cracks: deep blue
"""),
("""float3 dCol = lerp(mc, float3(0.32, 0.38, 0.44), steep * 0.6);
float3 dN = normalize(lerp(mN, iN, steep * 0.6));
float dR = lerp(mR, 0.3, steep * 0.6);
""",
"""// hummocks: the rubble is darker in the hollows and lighter on the crests (tens of metres)
float hum = gf.GF(WP.xy / 1800.0 + 2.9);
mc *= lerp(0.78, 1.08, saturate(hum * 0.9 + 0.5));
// ice cliffs in the debris: dirty grey-blue walls striped by old layers, paler bands between
float strata = gf.VN(float2(WP.z / 45.0, gf.Fbm(WP.xy / 3000.0) * 4.0));
float3 cliff = lerp(float3(0.36, 0.42, 0.48), float3(0.66, 0.74, 0.80), smoothstep(0.3, 0.8, strata));
float3 dCol = lerp(mc, cliff, steep * 0.75);
float3 dN = normalize(lerp(mN, iN, steep * 0.75));
float dR = lerp(mR, 0.3, steep * 0.75);
"""),
("""float3 outC = Col;
""",
"""// frozen melt ponds in the flat hollows of the debris tongue: turquoise ice with a pale frozen rim (Khumbu photos)
float pondN = gf.Fbm(WP.xy / 5200.0 + 4.4) + (gf.VN(WP.xy / 300.0) - 0.5) * 0.04;
float pond = smoothstep(0.700, 0.705, pondN) * smoothstep(0.975, 0.992, n.z) * debris;
float rim = 1.0 - smoothstep(0.705, 0.74, pondN);
float3 pCol = lerp(float3(0.20, 0.52, 0.58), float3(0.72, 0.88, 0.90), rim * 0.8);
float aP = g * pond * floorOnly;
float3 outC = Col;
"""),
("""outC = lerp(outC, uCol, aU);
""",
"""outC = lerp(outC, uCol, aU);
outC = lerp(outC, pCol, aP);
"""),
("""outN = normalize(lerp(outN, iN, aU));
""",
"""outN = normalize(lerp(outN, iN, aU));
outN = normalize(lerp(outN, n, aP));
"""),
("""RghOut = lerp(lerp(lerp(Rgh, dR, aD), bR, aB), 0.25, aU);""",
"""RghOut = lerp(lerp(lerp(lerp(Rgh, dR, aD), bR, aB), 0.25, aU), 0.12, aP);"""),
]


def patch(src):
    s = src.replace("\r\n", "\n")
    for a, b in EDITS:
        assert s.count(a) == 1, ("not found once", a[:60], s.count(a))
        s = s.replace(a, b)
    return s


if __name__ == "__main__":
    land = open(f"{ROOT}/M_EverestLandscape_Glacier.hlsl", encoding="utf8").read()
    open(f"{ROOT}/M_EverestLandscape_Glacier.hlsl", "w", encoding="utf8", newline="\n").write(patch(land))
    tile = open(sys.argv[1], encoding="utf8").read()
    open(f"{ROOT}/M_EverestDetailTile_Glacier.hlsl", "w", encoding="utf8", newline="\n").write(patch(tile))
    print("patched both")
