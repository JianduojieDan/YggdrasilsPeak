# Offline (python -I): ripstop nylon normal map for the down jackets, made by us (CC0). One 512 px tile = 12 mm:
# two 6 mm cells, raised reinforcement threads 0.35 mm wide, a faint plain weave between. DirectX green (UE).
import numpy as np
from PIL import Image
N = 512; mm = N / 12.0
y, x = np.mgrid[0:N, 0:N].astype(np.float32)
def ridge(c):                       # distance to the nearest grid thread (every 6 mm) -> raised profile
    d = np.abs(((c / mm) + 3.0) % 6.0 - 3.0) * mm      # px from the thread centre line
    w = 0.35 * mm / 2
    return np.clip(1.0 - (d / (w * 1.6)) ** 2, 0, 1)
h = np.maximum(ridge(x), ridge(y)) * 1.0
h += 0.12 * (np.sin(x / mm * 2 * np.pi / 0.25) * np.sin(y / mm * 2 * np.pi / 0.25))   # 0.25 mm weave
rng = np.random.default_rng(3); h += 0.05 * rng.standard_normal((N, N)).astype(np.float32)
gy, gx = np.gradient(h)
s = 2.2
nx, ny = -gx * s, gy * s            # DirectX: green flipped
nz = np.ones_like(nx)
L = np.sqrt(nx * nx + ny * ny + nz * nz)
img = np.stack([(nx / L) * 0.5 + 0.5, (ny / L) * 0.5 + 0.5, (nz / L) * 0.5 + 0.5], -1)
Image.fromarray((img * 255).astype(np.uint8)).save("F:/UE_project/YggdrasilsPeak/Art/Blender/Export/Garments/T_Ripstop_N.png")
print("ripstop ok")
