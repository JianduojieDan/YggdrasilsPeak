"""Summarise NPC follow behaviour from a log with ygg.Story.NPCDebug 1 (NPCDbg lines) - #8 follow check.
   python -I follow_report.py Saved/Logs/Follow.log"""
import re
import sys
from datetime import datetime

LINE = re.compile(r"^\[(\d{4}\.\d\d\.\d\d-\d\d\.\d\d\.\d\d:\d{3})\].*?NPCDbg (\S+) (.*)$")
MARK = re.compile(r"^\[(\d{4}\.\d\d\.\d\d-\d\d\.\d\d\.\d\d:\d{3})\].*FOLLOWTEST (\w+)")
KV = re.compile(r"(\w+)=(-?[\d.]+)")


def ts(s):
    return datetime.strptime(s, "%Y.%m.%d-%H.%M.%S:%f").timestamp()


rows, marks, skips = {}, [], {}
t0 = None
for raw in open(sys.argv[1], encoding="utf-8", errors="replace"):
    m = MARK.match(raw)
    if m:
        marks.append((ts(m.group(1)), m.group(2)))
        continue
    m = LINE.match(raw)
    if not m:
        continue
    t = ts(m.group(1))
    t0 = t if t0 is None else t0
    who, rest = m.group(2), m.group(3)
    if rest.startswith("trail skip"):
        skips[who] = skips.get(who, 0) + 1
        continue
    kv = {k: float(v) for k, v in KV.findall(rest)}
    if "stopped" in kv:
        rows.setdefault(who, []).append((t, kv))

if not rows:
    print("no NPCDbg lines (story did not reach the follow beat?)")
    sys.exit(0)
start = marks[0][0] if marks else t0
print("markers:", ", ".join(f"{n}@{t - start:.0f}s" for t, n in marks))
for who, r in rows.items():
    r = [(t, kv) for t, kv in r if t >= start - 1]
    if not r:
        continue
    mode = r[-1][1].get("mode", -1)
    stops = sum(1 for a, b in zip(r, r[1:]) if a[1]["stopped"] == 0 and b[1]["stopped"] == 1)
    starts = sum(1 for a, b in zip(r, r[1:]) if a[1]["stopped"] == 1 and b[1]["stopped"] == 0)
    walking = [kv for _, kv in r if kv["stopped"] == 0 and kv.get("pace", 0) > 60]
    errs = [kv.get("err", 0) for kv in walking]
    steer_flips = sum(1 for a, b in zip(r, r[1:]) if a[1].get("steer", 0) * b[1].get("steer", 0) < 0
                      or (a[1].get("steer", 0) == 0) != (b[1].get("steer", 0) == 0))
    yaw_jumps = 0
    for a, b in zip(r, r[1:]):
        if a[1]["stopped"] == 1 and b[1]["stopped"] == 1:
            d = abs((b[1].get("yaw", 0) - a[1].get("yaw", 0) + 180) % 360 - 180)
            yaw_jumps += d > 45
    dists = [kv["dist"] for _, kv in r]
    vel_walk = [kv["vel"] for kv in walking]
    print(f"{who:6s} mode={mode:.0f} lines={len(r)} stops={stops} starts={starts} skips={skips.get(who, 0)} "
          f"dist {min(dists):.0f}-{max(dists):.0f} | walking: err {min(errs, default=0):.0f}..{max(errs, default=0):.0f} "
          f"vel {min(vel_walk, default=0):.0f}-{max(vel_walk, default=0):.0f} | steer changes {steer_flips} | "
          f"standing yaw jumps>45 {yaw_jumps}")
# reaction to the player's stops and starts: per marker window, when each follower stopped / restarted
print("timeline (s from first marker): who stopped(+)/started(-)")
ev = []
for who, r in rows.items():
    for a, b in zip(r, r[1:]):
        if a[1]["stopped"] != b[1]["stopped"] and b[0] >= start - 1:
            ev.append((b[0] - start, who, "+" if b[1]["stopped"] == 1 else "-"))
print("  " + "  ".join(f"{t:.1f}{s}{w}" for t, w, s in sorted(ev)))
