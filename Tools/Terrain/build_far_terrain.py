# Builds the two far-terrain meshes + their world-space normal maps from the real DEM (Downloads/terrain_aws_*):
#   FarRing        34 km playable square -> 150 km square, 200 m grid, normal map from zoom-11 DEM (~67 m)
#   FarBackground  ring -> 660 km square, 1 km grid, normal map from zoom-9 DEM (~270 m)
# Earth curvature is built in (drop = max(r-17 km,0)^2 / 2R from the playable centre) so far ranges sink below the
# horizon like they really do. Under the playable landscape the ring is pushed down (hidden); at the landscape edge it
# sits 15 m under it so no seam shows. Output: Saved/FarTerrain/*.glb + *_N.png (vertex coords relative to CENTRE).
import numpy as np, math, os
from PIL import Image
import dem
OUT = r"F:/UE_project/YggdrasilsPeak/Saved/FarTerrain"; os.makedirs(OUT, exist_ok=True)
CENTRE = np.array([1196616.96, 1196616.96])            # playable square centre (cm)
HALF_PLAY = 504 * dem.PX_CM                               # 17.0 km
R_EARTH = 6371e5
def heights(M, X, Y):
    gx, gy = dem.world_to_g(X, Y, M.z)
    return dem.elev_to_z(M.sample(gx, gy))
def curvature(X, Y):
    r = np.hypot(X - CENTRE[0], Y - CENTRE[1])
    return np.maximum(r - 1.7e6, 0) ** 2 / (2 * R_EARTH)
def write_glb(path, P, F):
    import json, struct
    pb, ib = P.tobytes(), F.tobytes()
    gl = {"asset": {"version": "2.0"}, "scene": 0, "scenes": [{"nodes": [0]}], "nodes": [{"mesh": 0, "name": os.path.basename(path)[:-4]}],
          "meshes": [{"name": os.path.basename(path)[:-4], "primitives": [{"attributes": {"POSITION": 0}, "indices": 1}]}],
          "buffers": [{"byteLength": len(pb) + len(ib)}],
          "bufferViews": [{"buffer": 0, "byteOffset": 0, "byteLength": len(pb), "target": 34962},
                          {"buffer": 0, "byteOffset": len(pb), "byteLength": len(ib), "target": 34963}],
          "accessors": [{"bufferView": 0, "componentType": 5126, "count": len(P), "type": "VEC3", "min": P.min(0).tolist(), "max": P.max(0).tolist()},
                        {"bufferView": 1, "componentType": 5125, "count": F.size, "type": "SCALAR"}]}
    js = json.dumps(gl).encode(); js += b" " * (-len(js) % 4)
    bin_ = pb + ib; bin_ += bytes(-len(bin_) % 4)
    with open(path, "wb") as f:
        f.write(struct.pack("<III", 0x46546C67, 2, 12 + 8 + len(js) + 8 + len(bin_)))
        f.write(struct.pack("<II", len(js), 0x4E4F534A)); f.write(js)
        f.write(struct.pack("<II", len(bin_), 0x004E4942)); f.write(bin_)
def build(name, M, half, step, hole_half, tex, normal_M):
    n = int(round(2 * half / step)) + 1
    t = np.linspace(-half, half, n)
    X, Y = np.meshgrid(CENTRE[0] + t, CENTRE[1] + t)       # row = +y
    Z = heights(M, X, Y) - curvature(X, Y)
    # distance outside the playable square (cm, <0 inside)
    dout = np.maximum(np.abs(X - CENTRE[0]), np.abs(Y - CENTRE[1])) - HALF_PLAY
    if name == "FarRing":
        Z -= np.where(dout < -100000, 30000, np.interp(dout, [-100000, 0, 200000, 300000], [30000, 1500, 1500, 0]))
    else:
        Z -= np.interp(dout, [hole_half - HALF_PLAY - 2e5, hole_half - HALF_PLAY + 3e5], [4000, 0])
    # faces: skip quads fully inside the hole
    keep = np.ones((n - 1, n - 1), bool)
    if hole_half:
        cx = (t[:-1] + t[1:]) / 2
        CX, CY = np.meshgrid(cx, cx)
        keep = np.maximum(np.abs(CX), np.abs(CY)) > hole_half
    idx = np.arange(n * n).reshape(n, n) + 1
    a, b, c, d = idx[:-1, :-1][keep], idx[:-1, 1:][keep], idx[1:, :-1][keep], idx[1:, 1:][keep]
    # glTF (metres, Y-up). UE imports glTF (x, y, z) as (x, z, y) in cm (checked: peak vertex vs DEM) -> write (X, Z, Y).
    P = np.stack([X - CENTRE[0], Z, Y - CENTRE[1]], -1).reshape(-1, 3) / 100.0
    F = np.concatenate([np.stack([a, b, c], -1), np.stack([b, d, c], -1)]) - 1
    write_glb(f"{OUT}/{name}.glb", P.astype(np.float32), F.astype(np.uint32))
    # world-space normal map over the mesh's square (u = +x, v = +y)
    T = np.linspace(-half, half, tex)
    TX, TY = np.meshgrid(CENTRE[0] + T, CENTRE[1] + T)
    TZ = heights(normal_M, TX, TY) - curvature(TX, TY)
    sp = 2 * half / (tex - 1)
    gy, gx = np.gradient(TZ, sp)
    N = np.stack([-gx, -gy, np.ones_like(gx)], -1); N /= np.linalg.norm(N, axis=-1, keepdims=True)
    Image.fromarray(np.uint8(np.clip((N * 0.5 + 0.5) * 255 + 0.5, 0, 255))).save(f"{OUT}/T_{name}_N.png")
    print(name, n, "x", n, "verts", int(keep.sum()) * 2, "tris", "z range m", Z.min() / 100, Z.max() / 100)
if __name__ == "__main__":
    z11, z9 = dem.Mosaic(11), dem.Mosaic(9)
    build("FarRing", z11, 7.5e6, 20000, 0, 4096, z11)
    build("FarBackground", z9, 3.3e7, 100000, 7.3e6, 4096, z9)
