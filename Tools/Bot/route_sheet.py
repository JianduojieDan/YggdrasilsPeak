# Contact sheet of a route robot run (ygg.Bot.Route): every checkpoint picture of the run in one JPEG, labelled with the
# distance along the route, the leg, the height and the issues found in the 200 m before it.
#   python -I Tools/Bot/route_sheet.py [run folder | latest] [out.jpg] [--cols 8] [--width 320]
# Default out: Saved/Review/A4/route1_sheet.jpg. Reads <run>/points.csv and the report's issue table.
import csv, os, re, sys
from PIL import Image, ImageDraw, ImageFont

PROJ = r"F:/UE_project/YggdrasilsPeak"
args, opts, av = [], {}, sys.argv[1:]
while av:
    a = av.pop(0)
    if a.startswith("--") and av: opts[a] = av.pop(0)
    else: args.append(a)
cols = int(opts.get("--cols", 8)); width = int(opts.get("--width", 320))
run = args[0] if args else "latest"
if run == "latest":
    root = os.path.join(PROJ, "Saved", "Bot")
    runs = sorted((d for d in os.listdir(root) if d.startswith("route_")), key=lambda d: os.path.getmtime(os.path.join(root, d)))
    run = os.path.join(root, runs[-1])
out = args[1] if len(args) > 1 else os.path.join(PROJ, "Saved", "Review", "A4", "route1_sheet.jpg")
shots = os.path.join(PROJ, "Saved", "Review", "A4")

pts = list(csv.DictReader(open(os.path.join(run, "points.csv"), encoding="utf-8")))
issues = []
rep = os.path.join(run, "report.md") if os.path.exists(os.path.join(run, "report.md")) else os.path.join(run, "progress.md")
for line in open(rep, encoding="utf-8"):
    m = re.match(r"\| [\d:]+ \| (\d+) \| \d+ \| [\d.]+ \| (\w+) \|", line)
    if m: issues.append((float(m.group(1)), m.group(2)))

tiles = []
prev = -1e9
for p in pts:
    f = os.path.join(shots, p["picture"])
    if not p["picture"] or not os.path.exists(f): continue
    s = float(p["s_m"])
    kinds = sorted({k for (a, k) in issues if prev < a <= s})
    prev = s
    tiles.append((f, s, int(p["leg"]), float(p["elev_m"]), kinds, float(p["fps"])))
if not tiles: sys.exit("no pictures for " + run)
im0 = Image.open(tiles[0][0]); h = int(width * im0.height / im0.width)
rows = (len(tiles) + cols - 1) // cols
lab = 34
sheet = Image.new("RGB", (cols * width, rows * (h + lab)), (16, 18, 22))
try: font = ImageFont.truetype("arial.ttf", 13)
except OSError: font = ImageFont.load_default()
d = ImageDraw.Draw(sheet)
for i, (f, s, leg, elev, kinds, fps) in enumerate(tiles):
    x, y = (i % cols) * width, (i // cols) * (h + lab)
    sheet.paste(Image.open(f).convert("RGB").resize((width, h), Image.LANCZOS), (x, y))
    d.text((x + 4, y + h + 2), f"{s:,.0f} m  leg {leg}  {elev:,.0f} m a.s.l.  {fps:.0f} fps", fill=(230, 230, 230), font=font)
    if kinds: d.text((x + 4, y + h + 17), " ".join(kinds), fill=(255, 150, 90), font=font)
os.makedirs(os.path.dirname(out), exist_ok=True)
sheet.save(out, quality=85)
print(out, len(tiles), "pictures", sheet.size)
