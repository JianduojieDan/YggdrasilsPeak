# Glacier pass 3 (2026-11-11 user screenshots): repeating pattern, white speckle and turquoise blobs on the moraine,
# thin straight lines and dark strokes on the ice. Same edits on both Glacier sources (landscape + detail tile).
R = "F:/UE_project/YggdrasilsPeak/Tools/Materials"
EDITS = [
# thin straight lines: the "ribs" were abs(noise) creases -> razor lines in the lighting
("slope += float2(rx - r0, ry - r0) / e * 16.0 * (1.0 - smoothstep(2500.0, 9000.0, dist));",
 "// (ribs from abs(noise) removed 2026-11-11: their creases drew thin straight lines up close)"),
# the tiled ice scan repeated: much weaker in both normal and colour
("slope -= scanN * 0.55;", "slope -= scanN * 0.18;"),
("saturate(scanT * 0.6 + (1.0 - clear) * 0.55)", "saturate(scanT * 0.2 + (1.0 - clear) * 0.7)"),
# dark grit strokes on the ice: gone
("ic = lerp(ic, float3(0.52, 0.52, 0.52), grit * 0.18 * (1.0 - clear) * smoothstep(5900.0, 5500.0, elev));", ""),
# white speckle: the snow-patch border noise was too fine; fewer, bigger patches of snow on the moraine
("float patchN = gf.Fbm(WP.xy / 1400.0 + 11.0) + (gf.Fbm(WP.xy / 90.0 + 3.0) - 0.5) * 0.16 + (gf.VN(WP.xy / 25.0) - 0.5) * 0.05;",
 "float patchN = gf.Fbm(WP.xy / 1400.0 + 11.0) + (gf.Fbm(WP.xy / 160.0 + 3.0) - 0.5) * 0.07;"),
("float snowKeepDebris = smoothstep(0.61, 0.68, patchN + (n.z - 0.9) * 0.6);", "float snowKeepDebris = smoothstep(0.68, 0.74, patchN + (n.z - 0.9) * 0.6);"),
# ponds: rare and larger, not turquoise confetti
("float pondN = gf.Fbm(WP.xy / 5200.0 + 4.4) + (gf.VN(WP.xy / 300.0) - 0.5) * 0.04;", "float pondN = gf.Fbm(WP.xy / 11000.0 + 4.4) + (gf.Fbm(WP.xy / 900.0) - 0.5) * 0.03;"),
("float pond = smoothstep(0.700, 0.705, pondN) * smoothstep(0.975, 0.992, n.z) * debris;", "float pond = smoothstep(0.745, 0.75, pondN) * smoothstep(0.985, 0.995, n.z) * debris;"),
("float rim = 1.0 - smoothstep(0.705, 0.74, pondN);", "float rim = 1.0 - smoothstep(0.75, 0.77, pondN);"),
# tiles only: stones poking through snow made white specks everywhere
("float cover = tr + wob2 - (lumM - 0.45) * 0.9;", "float cover = tr + wob2 - (lumM - 0.45) * 0.35;"),
]
for f in ["M_EverestLandscape_Glacier.hlsl", "M_EverestDetailTile_Glacier.hlsl"]:
    s = open(f"{R}/{f}", encoding="utf8").read().replace("\r\n", "\n")
    for a, b in EDITS:
        if a in s:
            s = s.replace(a, b)
        else:
            print(f, "skip:", a[:55])
    open(f"{R}/{f}", "w", encoding="utf8", newline="\n").write(s)
    print(f, "done")
