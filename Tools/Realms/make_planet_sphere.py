# Smooth UV sphere for the realm planets (radius 1 m = 100 cm, 256 x 128 segments) -> Saved/Realms/SM_RealmSphere.glb
# The planet material shades from the world-space position, so only the silhouette needs the resolution.
import numpy as np, os, sys
sys.path.insert(0, os.path.join(os.path.dirname(__file__), "..", "Terrain"))
from build_far_terrain import write_glb
OUT = r"F:/UE_project/YggdrasilsPeak/Saved/Realms"; os.makedirs(OUT, exist_ok=True)
nu, nv = 256, 128
u = np.linspace(0, 2 * np.pi, nu + 1); v = np.linspace(0, np.pi, nv + 1)
U, V = np.meshgrid(u, v)
X, Y, Z = np.sin(V) * np.cos(U), np.sin(V) * np.sin(U), np.cos(V)
P = np.stack([X, Z, Y], -1).reshape(-1, 3).astype(np.float32)      # glTF Y-up; UE maps (x, y, z) -> (x, z, y)
idx = np.arange((nv + 1) * (nu + 1)).reshape(nv + 1, nu + 1)
a, b, c, d = idx[:-1, :-1].ravel(), idx[:-1, 1:].ravel(), idx[1:, :-1].ravel(), idx[1:, 1:].ravel()
F = np.concatenate([np.stack([a, b, c], -1), np.stack([b, d, c], -1)]).astype(np.uint32)
write_glb(f"{OUT}/SM_RealmSphere.glb", P, F)
print(len(P), "verts", len(F), "tris")
