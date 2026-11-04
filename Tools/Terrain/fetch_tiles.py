# Downloads AWS Terrarium elevation tiles (free, public) around Everest into Downloads/terrain_aws_2026-11-03/<z>/<x>_<y>.png
import math, os, sys, urllib.request, concurrent.futures as cf
OUT = r"F:/UE_project/YggdrasilsPeak/Downloads/terrain_aws_2026-11-03"
LAT, LON = 28.008, 86.921
def tile(lat, lon, z):
    n = 2 ** z
    x = (lon + 180) / 360 * n
    y = (1 - math.log(math.tan(math.radians(lat)) + 1 / math.cos(math.radians(lat))) / math.pi) / 2 * n
    return x, y
def fetch(z, radius_km):
    cx, cy = tile(LAT, LON, z)
    m_per_tile = 40075016.7 * math.cos(math.radians(LAT)) / 2 ** z
    r = int(math.ceil(radius_km * 1000 / m_per_tile)) + 1
    jobs = [(z, x, y) for x in range(int(cx) - r, int(cx) + r + 1) for y in range(int(cy) - r, int(cy) + r + 1)]
    os.makedirs(f"{OUT}/{z}", exist_ok=True)
    def get(j):
        z, x, y = j
        p = f"{OUT}/{z}/{x}_{y}.png"
        if os.path.exists(p) and os.path.getsize(p) > 0: return 0
        urllib.request.urlretrieve(f"https://s3.amazonaws.com/elevation-tiles-prod/terrarium/{z}/{x}/{y}.png", p)
        return 1
    with cf.ThreadPoolExecutor(16) as ex: n = sum(ex.map(get, jobs))
    print(z, len(jobs), "tiles,", n, "new")
fetch(9, 330)   # ~270 m/px: outer background to ~330 km
fetch(11, 90)   # ~67 m/px: ring + normal map detail out to ~90 km
