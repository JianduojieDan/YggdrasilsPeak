# Summarise the last ProfileGPU dump (graphics pipeline) in UE logs: passes with inclusive time >= MIN ms.
#   python -I gpuprof.py MIN log1 [log2 ...]   (several logs -> side-by-side columns, matched by path of names)
import re, sys
MIN = float(sys.argv[1])
logs = sys.argv[2:]
row = re.compile(r"LogRHI: Display:\s+┃(.*?)┃(.*?)┃( *)(\S.*?)\s*┃\s*$")
ms = re.compile(r"([0-9.]+) ms")
def parse(path):
    lines = open(path, encoding="utf-8", errors="replace").read().splitlines()
    start = max(i for i, l in enumerate(lines) if "GPU Profile for Frame" in l and "Graphics pipeline" in l)
    out, stack = [], []
    for l in lines[start:]:
        m = row.search(l)
        if not m:
            if out and "LogRHI" not in l:
                break
            continue
        incl = ms.findall(m.group(2))
        if not incl:
            continue
        d = len(m.group(3))
        while stack and stack[-1][0] >= d:
            stack.pop()
        stack.append((d, m.group(4)))
        out.append((re.sub(r"Frame \d+", "Frame", "/".join(n for _, n in stack)), d, m.group(4), float(incl[-1])))
    return out
tables = [parse(p) for p in logs]
maps = [{k: t for k, _, _, t in tab} for tab in tables]
print("  " + "  ".join(f"{p.split('_')[-1].split('.')[0][:9]:>9}" for p in logs) + "  pass")
for key, d, name, t in tables[0]:
    vals = [mp.get(key) for mp in maps]
    if max(v or 0 for v in vals) < MIN:
        continue
    print("  " + "  ".join(f"{v:9.2f}" if v is not None else f"{'-':>9}" for v in vals) + "  " + " " * max(0, d // 3 - 1) + name[:80])
