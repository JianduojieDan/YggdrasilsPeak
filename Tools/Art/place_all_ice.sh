#!/bin/bash
# Re-places every ice actor (pinnacles below EBC, the icefall stretches, the old test patch) with place_icefall_patch.py.
cd /f/UE_project/YggdrasilsPeak; Y="$TEMP/ygg"
for st in "Glacier_Pinnacles_A 405 430" "Glacier_Pinnacles_B 430 455" "Glacier_Pinnacles_C 455 503"; do set -- $st
  echo "{\"label\": \"$1\", \"route_from\": $2, \"route_to\": $3, \"half_width_m\": 160, \"seed\": $2, \"mode\": \"pinnacles\", \"clusters\": 22}" > $Y/icepatch.json
  echo -n "$1: "; python -I Tools/RoutePlanning/uepy.py Tools/Art/place_icefall_patch.py 2>&1 | grep -a "placed\|rror" | head -1; done
n=0; for st in "506 514" "514 522" "522 530" "530 538" "538 546" "546 554" "554 561" "567 574" "574 581" "581 588" "588 596"; do set -- $st; n=$((n+1))
  echo "{\"label\": \"Icefall_S$(printf %02d $n)\", \"route_from\": $1, \"route_to\": $2, \"half_width_m\": 75, \"outer_m\": 230, \"seed\": $1}" > $Y/icepatch.json
  echo -n "S$n: "; python -I Tools/RoutePlanning/uepy.py Tools/Art/place_icefall_patch.py 2>&1 | grep -a "placed\|rror" | head -1; done
echo '{"label": "Icefall_TestPatch", "route_from": 561, "route_to": 567, "half_width_m": 60, "outer_m": 230, "seed": 11}' > $Y/icepatch.json
echo -n "TestPatch: "; python -I Tools/RoutePlanning/uepy.py Tools/Art/place_icefall_patch.py 2>&1 | grep -a "placed\|rror" | head -1
