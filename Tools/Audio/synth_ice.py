# Procedural ice and mountain sounds (placeholders until real recordings), same pipeline as synth_footsteps.py:
#   Ice_Crack_01..05   - a sharp snap with the descending "pew" chirp that cracking ice makes (dispersion in the ice)
#   Ice_Groan_01..03   - slow creaking groan of a serac under strain (low resonances, amplitude-modulated)
#   Serac_Collapse_01..02 - cracks, then a roaring rumble of ice blocks falling and breaking
#   Mountain_Boom_01..03  - a deep impact inside the mountain (sub-bass thud, long rolling tail)
# 44.1 kHz mono 16-bit -> Art/Audio/Ice/.   python Tools/Audio/synth_ice.py
import numpy as np, os, wave
from scipy.signal import butter, sosfilt, fftconvolve

SR = 44100
OUT = os.path.join(os.path.dirname(__file__), "..", "..", "Art", "Audio", "Ice")
os.makedirs(OUT, exist_ok=True)


def band(x, lo, hi, o=4): return sosfilt(butter(o, [lo, hi], btype="band", fs=SR, output="sos"), x)
def low(x, hi, o=4): return sosfilt(butter(o, hi, btype="low", fs=SR, output="sos"), x)
def high(x, lo, o=4): return sosfilt(butter(o, lo, btype="high", fs=SR, output="sos"), x)
def env(n, a, d):
    t = np.arange(n) / SR
    return np.minimum(t / max(a, 1e-4), 1) * np.exp(-np.maximum(t - a, 0) / d)


def write(name, x, peak=0.89):
    x = x / (np.max(np.abs(x)) + 1e-9) * peak
    f = int(0.02 * SR); x[-f:] *= np.linspace(1, 0, f)
    with wave.open(os.path.join(OUT, name), "wb") as w:
        w.setnchannels(1); w.setsampwidth(2); w.setframerate(SR)
        w.writeframes((x * 32767).astype(np.int16).tobytes())


def reverb(x, seconds=1.5, wet=0.35, seed=0):
    """Cheap mountain echo: a decaying noise impulse response convolved in."""
    rng = np.random.default_rng(seed)
    n = int(seconds * SR)
    ir = rng.standard_normal(n) * np.exp(-np.arange(n) / SR / (seconds / 5))
    ir = low(ir, 3000)
    y = fftconvolve(x, ir)[: len(x) + n] / np.sqrt(n)
    out = np.zeros(len(y)); out[: len(x)] = x
    return out * (1 - wet) + y / (np.max(np.abs(y)) + 1e-9) * np.max(np.abs(x)) * wet


def chirp(n, f0, f1, curve=3.0):
    t = np.arange(n) / SR
    T = n / SR
    f = f1 + (f0 - f1) * np.exp(-t / (T / curve))       # falls fast, then lingers low
    return np.sin(2 * np.pi * np.cumsum(f) / SR)


def crack(seed):
    rng = np.random.default_rng(seed)
    n = int(1.6 * SR)
    snap = high(rng.standard_normal(n), 1500) * env(n, 0.0004, 0.012)
    pew = chirp(n, rng.uniform(2800, 4200), rng.uniform(180, 320), rng.uniform(2.5, 4)) * env(n, 0.001, rng.uniform(0.18, 0.3)) * 0.5
    pew2 = chirp(n, rng.uniform(1800, 2600), rng.uniform(120, 200), 3) * env(n, 0.001, 0.12) * 0.25
    x = snap + pew + np.roll(pew2, int(rng.uniform(0.01, 0.04) * SR))
    return reverb(x, 1.2, 0.3, seed)


def groan(seed):
    rng = np.random.default_rng(seed)
    n = int(3.5 * SR)
    t = np.arange(n) / SR
    base = rng.uniform(70, 110)
    f = base * (1 + 0.25 * np.sin(2 * np.pi * rng.uniform(0.2, 0.5) * t + rng.uniform(0, 6)))
    tone = np.sin(2 * np.pi * np.cumsum(f) / SR) + 0.4 * np.sin(2 * np.pi * np.cumsum(f * 2.7) / SR)
    stick = (np.sin(2 * np.pi * rng.uniform(9, 16) * t) > 0.3).astype(float)          # stick-slip creak
    stick = low(stick, 200)
    noise = band(rng.standard_normal(n), 60, 400) * 0.6
    x = (tone * 0.6 + noise) * (0.4 + 0.6 * stick) * np.sin(np.pi * t / t[-1]) ** 0.7
    return reverb(x, 2.0, 0.4, seed)


def collapse(seed):
    rng = np.random.default_rng(seed)
    n = int(7.0 * SR)
    t = np.arange(n) / SR
    x = np.zeros(n)
    for k in range(3):                                   # the cracks that start it
        c = crack(seed * 10 + k)[: int(1.5 * SR)]
        i = int(k * 0.25 * SR); x[i: i + len(c)] += c * 0.8
    brown = np.cumsum(rng.standard_normal(n)); brown -= low(brown, 5)
    roar = low(brown, 400) / (np.max(np.abs(brown)) + 1e-9)
    shape = np.clip((t - 0.6) / 0.8, 0, 1) * np.exp(-np.maximum(t - 2.5, 0) / 1.6)
    x += roar * shape * 3.0
    for _ in range(140):                                 # blocks hitting and breaking
        i = int(rng.uniform(0.8, 5.0) * SR); m = int(rng.uniform(0.02, 0.12) * SR)
        if i + m < n:
            x[i: i + m] += band(rng.standard_normal(m), rng.uniform(150, 600), rng.uniform(1200, 5000)) * env(m, 0.002, m / SR / 4) * rng.uniform(0.1, 0.6)
    return reverb(x, 2.5, 0.35, seed)


def boom(seed):
    rng = np.random.default_rng(seed)
    n = int(6.0 * SR)
    t = np.arange(n) / SR
    f = rng.uniform(28, 40) * np.exp(-t / 2.5) + 22
    thud = np.sin(2 * np.pi * np.cumsum(f) / SR) * env(n, 0.015, 0.9)
    body = low(rng.standard_normal(n), 120) * env(n, 0.02, 1.6) * 4
    rumble = low(np.cumsum(rng.standard_normal(n)), 80); rumble /= np.max(np.abs(rumble)) + 1e-9
    x = thud + body * 0.5 + rumble * np.clip(t / 0.3, 0, 1) * np.exp(-t / 2.2) * 0.8
    return reverb(x, 3.5, 0.45, seed)


for i in range(1, 6):
    write(f"Ice_Crack_{i:02d}.wav", crack(10 + i))
for i in range(1, 4):
    write(f"Ice_Groan_{i:02d}.wav", groan(20 + i), 0.7)
    write(f"Mountain_Boom_{i:02d}.wav", boom(40 + i))
for i in range(1, 3):
    write(f"Serac_Collapse_{i:02d}.wav", collapse(30 + i))
print("written to", os.path.abspath(OUT))
