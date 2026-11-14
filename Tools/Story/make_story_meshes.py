# Offline (python -I): writes Saved/Story/*.glb for chapter 1: expedition dome tents (2-person, mess), a khata.
# Units: metres in glTF (UE imports x100). glTF axes (x, y up, z) -> UE (x, z, y): we build in UE-like (x, y, z up) and swap.
import json, struct, os, math
import numpy as np
OUT = r"F:/UE_project/YggdrasilsPeak/Saved/Story"; os.makedirs(OUT, exist_ok=True)

def normals(P, F):
    N = np.zeros_like(P)
    v = np.cross(P[F[:, 1]] - P[F[:, 0]], P[F[:, 2]] - P[F[:, 0]])
    for k in range(3): np.add.at(N, F[:, k], v)
    return N / np.maximum(np.linalg.norm(N, axis=1, keepdims=True), 1e-9)

def write_glb(name, prims):
    """prims: list of (P[n,3] UE-style metres z-up, UV[n,2], F[m,3])"""
    blobs, views, accs, gprims = [], [], [], []
    off = 0
    def add(arr, target, kind, minmax=False):
        nonlocal off
        b = arr.tobytes(); blobs.append(b)
        views.append({"buffer": 0, "byteOffset": off, "byteLength": len(b), "target": target}); off += len(b)
        while off % 4: blobs.append(b"\0"); off += 1
        a = {"bufferView": len(views) - 1, "componentType": 5126 if arr.dtype == np.float32 else 5125, "count": len(arr), "type": kind}
        if minmax: a["min"] = arr.min(0).tolist(); a["max"] = arr.max(0).tolist()
        accs.append(a); return len(accs) - 1
    for i, (P, UV, F) in enumerate(prims):
        N = normals(P, F)
        Pg = P[:, [0, 2, 1]].astype(np.float32) * np.array([1, 1, -1], np.float32)   # z-up -> y-up, keep handedness
        Ng = N[:, [0, 2, 1]].astype(np.float32) * np.array([1, 1, -1], np.float32)
        Fg = F[:, [0, 2, 1]].astype(np.uint32).ravel()                                # winding flips with the mirror
        ip = add(Pg, 34962, "VEC3", True); inn = add(Ng, 34962, "VEC3"); iu = add(UV.astype(np.float32), 34962, "VEC2"); ii = add(Fg, 34963, "SCALAR")
        gprims.append({"attributes": {"POSITION": ip, "NORMAL": inn, "TEXCOORD_0": iu}, "indices": ii, "material": i})
    g = {"asset": {"version": "2.0"}, "scene": 0, "scenes": [{"nodes": [0]}], "nodes": [{"mesh": 0, "name": name}],
         "meshes": [{"primitives": gprims}], "materials": [{"name": f"Slot{i}"} for i in range(len(prims))],
         "buffers": [{"byteLength": off}], "bufferViews": views, "accessors": accs}
    J = json.dumps(g).encode(); J += b" " * ((4 - len(J) % 4) % 4)
    B = b"".join(blobs)
    with open(f"{OUT}/{name}.glb", "wb") as f:
        f.write(struct.pack("<III", 0x46546C67, 2, 28 + len(J) + len(B)))
        f.write(struct.pack("<II", len(J), 0x4E4F534A) + J + struct.pack("<II", len(B), 0x004E4942) + B)
    print("wrote", name, sum(len(p[0]) for p in prims), "verts")

def grid_faces(nu, nv, wrap_u=True):
    F = []
    for j in range(nv - 1):
        for i in range(nu if wrap_u else nu - 1):
            a = j * nu + i; b = j * nu + (i + 1) % nu; c = a + nu; d = b + nu
            F += [[a, c, b], [b, c, d]]
    return np.array(F, np.int64)

def tube(path, radius, sides=6):
    path = np.asarray(path, float); n = len(path)
    P, UV = [], []
    for i in range(n):
        t = path[min(i + 1, n - 1)] - path[max(i - 1, 0)]; t /= np.linalg.norm(t) + 1e-9
        a = np.cross(t, [0, 0, 1.0]);
        if np.linalg.norm(a) < 1e-3: a = np.cross(t, [1.0, 0, 0])
        a /= np.linalg.norm(a); b = np.cross(t, a)
        for k in range(sides):
            ang = 2 * math.pi * k / sides
            P.append(path[i] + radius * (math.cos(ang) * a + math.sin(ang) * b)); UV.append([k / sides, i / (n - 1)])
    return np.array(P), np.array(UV), grid_faces(sides, n)

def merge(parts):
    P, UV, F, base = [], [], [], 0
    for p, uv, f in parts:
        P.append(p); UV.append(uv); F.append(f + base); base += len(p)
    return np.vstack(P), np.vstack(UV), np.vstack(F)

def dome_tent(name, a, b, H, expo, vest, sag, guy):
    nth, ns = 72, 18
    P, UV = [], []
    def surf(th, s):
        c, si = math.cos(th), math.sin(th)
        r = (abs(c) ** expo / a ** expo + abs(si) ** expo / b ** expo) ** (-1.0 / expo)
        between = math.sin(2 * (th - math.pi / 4)) ** 2               # 0 on the pole lines (diagonals), 1 mid-panel
        r *= 1.0 - sag * between * math.sin(math.pi * s)
        r += vest * max(0.0, math.cos(th)) ** 3 * s ** 1.6               # vestibule out the front (+x)
        hz = math.sin(s * math.pi / 2) ** 0.8
        z = H * math.cos(s * math.pi / 2) ** 1.1 * (1.0 - 0.04 * between * math.sin(math.pi * s))
        return np.array([r * hz * c, r * hz * si, z])
    for j in range(ns):
        s = 0.02 + 0.98 * j / (ns - 1)
        for i in range(nth):
            th = 2 * math.pi * i / nth
            P.append(surf(th, s)); UV.append([i / nth, s])
    fabric = (np.array(P), np.array(UV), grid_faces(nth, ns))
    # a cap over the very top
    poles, cords = [], []
    for th0 in (math.pi / 4, 3 * math.pi / 4):
        arch = [surf(th0 + (0 if s >= 0 else math.pi), abs(s)) * 1.012 for s in np.linspace(-1, 1, 41)]
        poles.append(tube(arch, 0.007))
    for th0 in (math.pi / 4, 3 * math.pi / 4, 5 * math.pi / 4, 7 * math.pi / 4):
        top = surf(th0, 0.45)
        stake = np.array([math.cos(th0), math.sin(th0), 0]) * (np.linalg.norm(top[:2]) + guy)
        cords.append(tube(np.linspace(top, stake, 12), 0.0025, 4))
        cords.append(tube([stake + [0, 0, 0.12], stake - [0, 0, 0.05]], 0.006, 5))   # the stake
    write_glb(name, [fabric, merge(poles), merge(cords)])

def khata(name):
    # a 1.5 m white scarf tied round a stone: a loop that hangs down on two sides, fringes at the ends
    nu, nv = 60, 6
    P, UV = [], []
    for i in range(nu):
        u = i / (nu - 1)
        x = (u - 0.5) * 0.9
        z = -0.55 * (1 - math.cos(math.pi * (u - 0.5) * 2)) / 2 - 0.08 * math.sin(u * 13)   # hangs down at both ends
        y = 0.05 * math.sin(u * 9)
        for j in range(nv):
            w = (j / (nv - 1) - 0.5) * 0.28
            P.append([x, y + w * 0.3, z + w * 0.15]); UV.append([u, j / (nv - 1)])
    write_glb(name, [(np.array(P), np.array(UV), grid_faces(nv, nu, False)[:, [0, 1, 2]] if False else _strip_faces(nu, nv))])

def _strip_faces(nu, nv):
    F = []
    for i in range(nu - 1):
        for j in range(nv - 1):
            a = i * nv + j; b = a + 1; c = a + nv; d = c + 1
            F += [[a, c, b], [b, c, d]]
    return np.array(F, np.int64)

dome_tent("SM_Tent_Dome", 1.15, 0.8, 1.12, 2.4, 0.55, 0.07, 0.9)
dome_tent("SM_Tent_Mess", 2.5, 2.0, 2.25, 3.2, 0.0, 0.05, 1.4)
khata("SM_Khata")
