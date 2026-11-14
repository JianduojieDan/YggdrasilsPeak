# Offline (python -I): downloads Poly Haven texture sets (CC0) as T_<id>_diff/nor/rough/disp.jpg (DirectX normals, for UE).
#   python -I Tools/Story/fetch_polyhaven_textures.py <out folder name> <res> id id ...
import json, os, sys, urllib.request
pack, res, ids = sys.argv[1], sys.argv[2], sys.argv[3:]
OUT = f"F:/UE_project/YggdrasilsPeak/Downloads/{pack}"
os.makedirs(OUT, exist_ok=True)
def get(url): return urllib.request.urlopen(urllib.request.Request(url, headers={"User-Agent": "asset-fetch"})).read()
for i in ids:
    try:
        f = json.loads(get(f"https://api.polyhaven.com/files/{i}"))
        for key, tag in (("Diffuse", "diff"), ("nor_dx", "nor"), ("Rough", "rough"), ("Displacement", "disp")):
            if key not in f: continue
            r = res if res in f[key] else sorted(f[key])[0]
            fmt = "jpg" if "jpg" in f[key][r] else ("png" if "png" in f[key][r] else None)
            if not fmt: continue
            path = f"{OUT}/T_{i}_{tag}.jpg" if fmt == "jpg" else f"{OUT}/T_{i}_{tag}.png"
            if os.path.exists(path): continue
            open(path, "wb").write(get(f[key][r][fmt]["url"]))
        print("got", i)
    except Exception as e:
        print("FAILED", i, e)
open(f"{OUT}/LICENSE.txt", "w").write("Poly Haven (polyhaven.com) textures, CC0 1.0 public domain.\n")
