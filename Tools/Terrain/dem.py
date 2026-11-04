# DEM mosaic from the downloaded Terrarium tiles + the world <-> web-mercator mapping of base_map.
# The playable heightmap is a web-mercator zoom-12 pixel grid (33.742 m/px at 28 N), 1009 px centred on 28.008N 86.921E;
# landscape world x = -504000 + col*3374.24 cm (row 0 = north = -Y). World z_cm = (elev_m - 6326.3)*100 + 129770.
import math, os, numpy as np
from PIL import Image
TILES = r"F:/UE_project/YggdrasilsPeak/Downloads/terrain_aws_2026-11-03"
LAT, LON = 28.008, 86.921
PX_CM = 3374.24
def gpx(lat, lon, z):   # global pixel
    n = 256 * 2 ** z
    return ((lon + 180) / 360 * n,
            (1 - math.log(math.tan(math.radians(lat)) + 1 / math.cos(math.radians(lat))) / math.pi) / 2 * n)
CX12, CY12 = gpx(LAT, LON, 12)
OFF = [0.0, -1.5]       # fitted 2026-11-03: median |error| 3.4 m over 399 landscape traces
def world_to_g(x, y, z):  # world cm -> global pixel at zoom z
    s = 2 ** (z - 12)
    col = (x + 504000) / PX_CM; row = (y + 504000) / PX_CM
    return ((CX12 - 504 + col + OFF[0]) * s, (CY12 - 504 + row + OFF[1]) * s)
def g_to_world(gx, gy, z):
    s = 2 ** (z - 12)
    return ((gx / s - CX12 + 504 - OFF[0]) * PX_CM - 504000, (gy / s - CY12 + 504 - OFF[1]) * PX_CM - 504000)
def elev_to_z(e): return (e - 6326.3) * 100 + 129770
class Mosaic:
    def __init__(self, z):
        self.z = z; d = f"{TILES}/{z}"
        xs, ys = zip(*[tuple(map(int, f[:-4].split("_"))) for f in os.listdir(d)])
        self.x0, self.y0 = min(xs), min(ys)
        W, H = (max(xs) - self.x0 + 1) * 256, (max(ys) - self.y0 + 1) * 256
        self.h = np.zeros((H, W), np.float32)
        for f in os.listdir(d):
            tx, ty = map(int, f[:-4].split("_"))
            a = np.asarray(Image.open(f"{d}/{f}").convert("RGB"), np.float32)
            self.h[(ty - self.y0) * 256:(ty - self.y0 + 1) * 256, (tx - self.x0) * 256:(tx - self.x0 + 1) * 256] = a[..., 0] * 256 + a[..., 1] + a[..., 2] / 256 - 32768
    def sample(self, gx, gy):   # bilinear, global pixel coords (pixel centres at +0.5)
        u = np.asarray(gx) - self.x0 * 256 - 0.5; v = np.asarray(gy) - self.y0 * 256 - 0.5
        u = np.clip(u, 0, self.h.shape[1] - 1.001); v = np.clip(v, 0, self.h.shape[0] - 1.001)
        i, j = np.floor(u).astype(int), np.floor(v).astype(int); fu, fv = u - i, v - j
        H = self.h
        return (H[j, i] * (1 - fu) * (1 - fv) + H[j, i + 1] * fu * (1 - fv) + H[j + 1, i] * (1 - fu) * fv + H[j + 1, i + 1] * fu * fv)
