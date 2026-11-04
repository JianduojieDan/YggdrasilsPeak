# Writes Saved/Realms/SM_PrayerFlag.glb: a 1 m cloth grid (16 x 12) lying like /Engine/BasicShapes/Plane, v = 0 on the edge tied to the cord.
import json, struct, numpy as np, os
OUT = r"F:/UE_project/YggdrasilsPeak/Saved/Realms"; os.makedirs(OUT, exist_ok=True)
nx, ny = 16, 12
u, v = np.meshgrid(np.linspace(0, 1, nx + 1), np.linspace(0, 1, ny + 1))
P = np.stack([u - 0.5, np.zeros_like(u), v - 0.5], -1).reshape(-1, 3).astype(np.float32)   # glTF (x, y, z) -> UE (x, z, y)
N = np.tile(np.array([0, 1, 0], np.float32), (len(P), 1))
UV = np.stack([u, v], -1).reshape(-1, 2).astype(np.float32)
i = np.arange((ny + 1) * (nx + 1)).reshape(ny + 1, nx + 1)
a, b, c, d = i[:-1, :-1].ravel(), i[:-1, 1:].ravel(), i[1:, :-1].ravel(), i[1:, 1:].ravel()
F = np.concatenate([np.stack([a, c, b], -1), np.stack([b, c, d], -1)]).astype(np.uint32)
blobs = [P.tobytes(), N.tobytes(), UV.tobytes(), F.tobytes()]; views = []; off = 0
for x, t in zip(blobs, (34962, 34962, 34962, 34963)):
    views.append({"buffer": 0, "byteOffset": off, "byteLength": len(x), "target": t}); off += len(x)
g = {"asset": {"version": "2.0"}, "scene": 0, "scenes": [{"nodes": [0]}], "nodes": [{"mesh": 0, "name": "SM_PrayerFlag"}],
     "meshes": [{"primitives": [{"attributes": {"POSITION": 0, "NORMAL": 1, "TEXCOORD_0": 2}, "indices": 3}]}],
     "buffers": [{"byteLength": off}], "bufferViews": views,
     "accessors": [{"bufferView": 0, "componentType": 5126, "count": len(P), "type": "VEC3", "min": P.min(0).tolist(), "max": P.max(0).tolist()},
                   {"bufferView": 1, "componentType": 5126, "count": len(N), "type": "VEC3"},
                   {"bufferView": 2, "componentType": 5126, "count": len(UV), "type": "VEC2"},
                   {"bufferView": 3, "componentType": 5125, "count": F.size, "type": "SCALAR"}]}
J = json.dumps(g).encode(); J += b" " * (-len(J) % 4); B = b"".join(blobs); B += b"\0" * (-len(B) % 4)
open(f"{OUT}/SM_PrayerFlag.glb", "wb").write(struct.pack("<III", 0x46546C67, 2, 28 + len(J) + len(B)) + struct.pack("<II", len(J), 0x4E4F534A) + J + struct.pack("<II", len(B), 0x004E4942) + B)
print(len(P), "verts")
