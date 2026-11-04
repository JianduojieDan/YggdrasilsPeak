#!/usr/bin/env bash
# Captures each realm alone (single layout, big) from the summit and builds a side-by-side sheet with its reference
# image: Docs/ArtDirection/realms/realm_sheet_<tag>.jpg.   realm_sheet.sh <tag> [realm names...]
cd "$(dirname "$0")/../.."
TAG=${1:-v1}; shift
NAMES=${@:-Midgard Niflheim Muspelheim Jotunheim Alfheim Vanaheim Svartalfheim Hel Asgard}
TMP="${TMPDIR:-/tmp}/ygg_realm.py"
for N in $NAMES; do
  printf 'LAYOUT="single"\nREALM="%s"\nAZ=-95\nEL=30\nSIZE=None\nFOV=100.0\nexec(open(r"F:/UE_project/YggdrasilsPeak/Tools/Realms/place_realms.py").read())\nVIEWS=[]\nVIEWS_ABS=[("realm_%s",1240508,1412617,382663,-95,26)]\nOUT_TAG="_%s"\nexec(open(r"F:/UE_project/YggdrasilsPeak/Tools/RoutePlanning/capture_scene.py").read())\n' "$N" "$N" "$TAG" > "$TMP"
  PYTHONIOENCODING=utf8 python -I Tools/RoutePlanning/uepy.py "$TMP" 2>&1 | grep -E "captured|Error" | tail -1
done
python - "$TAG" $NAMES <<'EOF'
import sys
from PIL import Image, ImageDraw
tag, names = sys.argv[1], sys.argv[2:]
refs = {n: f for n, f in [("Midgard","01_Midgard"),("Niflheim","02_Niflheim"),("Muspelheim","03_Muspelheim"),("Jotunheim","04_Jotunheim"),
        ("Alfheim","05_Alfheim"),("Vanaheim","06_Vanaheim"),("Svartalfheim","07_Svartalfheim"),("Hel","08_Hel"),("Asgard","09_Asgard")]}
W, H = 640, 360
sheet = Image.new("RGB", (W * 2 + 30, (H + 30) * len(names) + 10), (18, 20, 26)); d = ImageDraw.Draw(sheet)
for i, n in enumerate(names):
    y = 10 + i * (H + 30)
    d.text((10, y), f"{n}: reference | game", fill=(220, 220, 230))
    sheet.paste(Image.open(f"Docs/ArtDirection/realms/{refs[n]}.png").convert("RGB").resize((W, H)), (10, y + 16))
    sheet.paste(Image.open(f"Saved/Compare/realm_{n}_{tag}.png").convert("RGB").resize((W, H)), (W + 20, y + 16))
sheet.save(f"Docs/ArtDirection/realms/realm_sheet_{tag}.jpg", quality=85)
print("sheet", sheet.size)
EOF
