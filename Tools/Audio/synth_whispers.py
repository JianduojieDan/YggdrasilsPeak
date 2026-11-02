# Procedural whispers for the sanity system (placeholders): breathy noise shaped into syllables, each with its
# own vowel formants, a slow phrase envelope and a cold echo - unintelligible, almost words.
#   Whisper_01..06.wav -> Art/Audio/Mind/.   python Tools/Audio/synth_whispers.py
import numpy as np, os, wave
from scipy.signal import butter, sosfilt, fftconvolve
SR = 44100
OUT = os.path.join(os.path.dirname(__file__), "..", "..", "Art", "Audio", "Mind")
os.makedirs(OUT, exist_ok=True)
FORMANTS = [(700, 1220), (390, 2300), (300, 870), (610, 1900), (440, 1020), (500, 1500)]   # rough vowel pairs
def band(x, lo, hi): return sosfilt(butter(2, [lo, hi], btype="band", fs=SR, output="sos"), x)
def whisper(seed):
    rng = np.random.default_rng(seed)
    syl = rng.integers(4, 9)
    parts = []
    for _ in range(syl):
        n = int(rng.uniform(0.09, 0.22) * SR)
        f1, f2 = FORMANTS[rng.integers(len(FORMANTS))]
        noise = rng.standard_normal(n)
        v = band(noise, f1 * 0.8, f1 * 1.25) * 1.0 + band(noise, f2 * 0.85, f2 * 1.2) * 0.7 + band(noise, 4000, 8000) * rng.uniform(0.1, 0.5)
        t = np.linspace(0, 1, n)
        env = np.sin(np.pi * t) ** rng.uniform(0.6, 1.6)
        if rng.random() < 0.4:   # a consonant hiss at the start
            k = int(0.03 * SR); v[:k] += band(rng.standard_normal(k), 3000, 9000) * 1.5
        parts.append(v * env)
        parts.append(np.zeros(int(rng.uniform(0.02, 0.12) * SR)))
    x = np.concatenate(parts)
    x *= np.sin(np.pi * np.linspace(0, 1, len(x))) ** 0.5
    ir = rng.standard_normal(int(1.2 * SR)) * np.exp(-np.arange(int(1.2 * SR)) / SR / 0.3)
    wet = fftconvolve(x, ir)[: len(x) + len(ir)]
    y = np.zeros(len(wet)); y[: len(x)] = x
    y = y + wet / (np.max(np.abs(wet)) + 1e-9) * np.max(np.abs(x)) * 0.45
    y = y / (np.max(np.abs(y)) + 1e-9) * 0.7
    f = int(0.02 * SR); y[-f:] *= np.linspace(1, 0, f)
    return y
for i in range(1, 7):
    with wave.open(os.path.join(OUT, f"Whisper_{i:02d}.wav"), "wb") as w:
        w.setnchannels(1); w.setsampwidth(2); w.setframerate(SR)
        w.writeframes((whisper(50 + i) * 32767).astype(np.int16).tobytes())
print("whispers written")
