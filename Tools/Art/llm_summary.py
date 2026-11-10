"""Summarise an Unreal LLM CSV: peak MB per tag, largest first (python llm_summary.py <csv> [top N])."""
import csv, sys

csv.field_size_limit(10**9)
path = sys.argv[1]
top = int(sys.argv[2]) if len(sys.argv) > 2 else 40
with open(path, newline='', encoding='utf-8', errors='replace') as f:
    rows = list(csv.reader(f))
head, data = rows[0], [r for r in rows[1:] if r and r[0].replace('.', '', 1).isdigit()]
peak = {}
for r in data:
    for k, v in zip(head, r):
        try:
            x = float(v)
        except ValueError:
            continue
        peak[k] = max(peak.get(k, 0.0), x)
last = dict(zip(head, data[-1])) if data else {}
print(f'{len(data)} samples; values MB (peak / last)')
for k, v in sorted(peak.items(), key=lambda kv: -kv[1])[:top]:
    print(f'{k:40s} {v:9.1f} {float(last.get(k, 0) or 0):9.1f}')
