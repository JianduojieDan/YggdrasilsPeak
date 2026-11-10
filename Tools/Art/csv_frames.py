"""Frame-time summary of an Unreal CSV profile (last half only: capture starts at frame 1, during warm-up).
python csv_frames.py <csv> <name>"""
import csv, sys, statistics

csv.field_size_limit(10**9)
rows = list(csv.reader(open(sys.argv[1], newline='', encoding='utf-8', errors='replace')))
head = rows[0]
fi, gi = head.index('FrameTime'), head.index('GPUTime') if 'GPUTime' in head else None
ft, gpu = [], []
for r in rows[1:]:
    try:
        ft.append(float(r[fi]))
        if gi is not None: gpu.append(float(r[gi]))
    except (ValueError, IndexError):
        pass
ft = sorted(ft[len(ft) // 2:]); gpu = sorted(gpu[len(gpu) // 2:])
med = statistics.median(ft); p95 = ft[int(len(ft) * 0.95)]
print(f'{sys.argv[2]} : frames {len(ft)}  median {med:.2f} ms ({1000 / med:.0f} fps)  p95 {p95:.2f} ms'
      + (f'  GPU median {statistics.median(gpu):.2f} ms' if gpu else ''))
