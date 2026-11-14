# Offline (python -I): downloads Poly Haven models (CC0) as glTF + textures into Downloads/polyhaven_models_2026-11-14/<id>/.
#   python -I Tools/Story/fetch_polyhaven_models.py [res=2k] [id id ...]
import json, os, sys, urllib.request
OUT = "F:/UE_project/YggdrasilsPeak/Downloads/polyhaven_models_2026-11-14"
ROOM = ["barrel_stove", "painted_wooden_cabinet", "painted_wooden_cabinet_02", "painted_wooden_shelves", "painted_wooden_bench",
        "painted_wooden_table", "painted_wooden_stool", "chinese_tea_table", "brass_pot_01", "brass_pot_02", "brass_pan_01",
        "brass_vase_01", "brass_vase_02", "brass_vase_03", "brass_vase_04", "brass_goblets", "brass_diya_lantern",
        "brass_candleholders", "metal_jug", "jug_01", "pot_enamel_01", "plastic_thermos", "modified_thermos", "ceramic_pot",
        "Lantern_01", "carved_wooden_plate"]
CAMP = ["plastic_jerrycan", "metal_jerrycan", "propane_tank", "old_military_crate", "plastic_crate_01", "Barrel_01"]
args = sys.argv[1:]
res = args.pop(0) if args and args[0].endswith("k") else "2k"
ids = args or ROOM + CAMP
def get(url): return urllib.request.urlopen(urllib.request.Request(url, headers={"User-Agent": "asset-fetch"})).read()
for i in ids:
    d = f"{OUT}/{i}"
    if os.path.exists(f"{d}/{i}.gltf"): print("have", i); continue
    try:
        f = json.loads(get(f"https://api.polyhaven.com/files/{i}"))
        g = f["gltf"][res]["gltf"] if res in f["gltf"] else f["gltf"]["1k"]["gltf"]
        os.makedirs(d, exist_ok=True)
        open(f"{d}/{i}.gltf", "wb").write(get(g["url"]))
        for rel, inc in g.get("include", {}).items():
            p = os.path.join(d, rel); os.makedirs(os.path.dirname(p), exist_ok=True); open(p, "wb").write(get(inc["url"]))
        print("got", i, sum(os.path.getsize(os.path.join(r, x)) for r, _, fs in os.walk(d) for x in fs) // 1024, "KB")
    except Exception as e:
        print("FAILED", i, e)
open(f"{OUT}/LICENSE.txt", "w").write("Poly Haven (polyhaven.com) models, CC0 1.0 public domain.\n")
