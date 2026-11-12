"""GPU time per render pass from an Unreal CSV profile run with r.GPUCsvStatsEnabled 1 (last half of the capture).
python csv_gpu.py <csv> <name> [top]"""
import csv, sys, statistics
csv.field_size_limit(10**9)
rows = list(csv.reader(open(sys.argv[1], newline='', encoding='utf-8', errors='replace')))
head = rows[0]
data = rows[1:]
data = data[len(data) // 2:]
top = int(sys.argv[3]) if len(sys.argv) > 3 else 18
res = []
for i, h in enumerate(head):
    if not h.startswith('GPU/') or h in ('GPU/Total',):
        continue
    vals = []
    for r in data:
        try: vals.append(float(r[i]))
        except (ValueError, IndexError): pass
    if vals:
        res.append((statistics.median(vals), h))
res.sort(reverse=True)
print(sys.argv[2], 'GPU passes (median ms):')
for v, h in res[:top]:
    print('  %6.2f  %s' % (v, h))
