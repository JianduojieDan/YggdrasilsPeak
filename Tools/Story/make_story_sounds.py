# Offline (python -I): synthesises the chapter 1 sounds the project did not have -> Saved/Story/Audio/*.wav (48 kHz mono).
# Loops are cross-faded so they repeat without a click. The author judges every sound by ear later.
import os, wave, math
import numpy as np
SR = 48000
OUT = r"F:/UE_project/YggdrasilsPeak/Saved/Story/Audio"; os.makedirs(OUT, exist_ok=True)
rng = np.random.default_rng(7)

def save(name, x, loop=False):
    x = np.asarray(x, float)
    if loop:                                  # cross-fade the tail into the head
        n = SR // 2
        f = np.linspace(0, 1, n)
        x[:n] = x[:n] * f + x[-n:] * (1 - f)
        x = x[:-n]
    x = x / (np.max(np.abs(x)) + 1e-9) * 0.7
    with wave.open(f"{OUT}/{name}.wav", "wb") as w:
        w.setnchannels(1); w.setsampwidth(2); w.setframerate(SR)
        w.writeframes((x * 32767).astype(np.int16).tobytes())
    print(name, round(len(x) / SR, 1), "s")

def lp(x, a):            # one-pole low-pass, a = 0..1 (smaller = darker)
    y = np.empty_like(x); s = 0.0
    for i, v in enumerate(x): s += a * (v - s); y[i] = s
    return y

def band(x, lo, hi):     # crude band-pass via FFT
    X = np.fft.rfft(x); f = np.fft.rfftfreq(len(x), 1 / SR)
    X[(f < lo) | (f > hi)] = 0
    return np.fft.irfft(X, len(x))

t = lambda sec: np.arange(int(SR * sec)) / SR

# a stream running under ice: band noise with bubbling amplitude
T = t(12.5); n = rng.standard_normal(len(T))
bub = 0.6 + 0.4 * np.abs(band(rng.standard_normal(len(T)), 2, 9)) * 6
save("Story_Stream_Loop", band(n, 400, 5200) * bub + 0.4 * band(n, 120, 600), loop=True)

# a kerosene stove roaring: dark rushing noise with a slow flutter
T = t(10.5); n = rng.standard_normal(len(T))
save("Story_Stove_Loop", band(n, 60, 900) * (0.85 + 0.15 * np.sin(2 * math.pi * 3.1 * T)) + 0.3 * band(n, 900, 3000), loop=True)

# radio: squelch burst, crackle, a soft carrier hiss, then the click
T = t(1.6); n = rng.standard_normal(len(T))
crack = (rng.random(len(T)) > 0.9985) * rng.standard_normal(len(T)) * 6
env = np.minimum(1, T / 0.02) * np.minimum(1, (1.6 - T) / 0.15)
x = band(n, 900, 3800) * 0.5 * env + crack
x[:300] += np.sin(2 * math.pi * 1600 * T[:300]) * 0.8
save("Story_Radio_Squelch", x)
T = t(8.5); n = rng.standard_normal(len(T))
save("Story_Radio_Static_Loop", band(n, 700, 4200) * 0.3 + (rng.random(len(T)) > 0.999) * rng.standard_normal(len(T)) * 3, loop=True)

# prayer flags snapping in the wind: fluttering cloth (AM noise) with irregular snaps
T = t(11.5); n = rng.standard_normal(len(T))
flut = 0.5 + 0.5 * np.sin(2 * math.pi * (7 + 3 * np.sin(2 * math.pi * 0.21 * T)) * T)
snaps = np.zeros(len(T))
for s in rng.uniform(0, 11, 26):
    i = int(s * SR); k = np.arange(min(2400, len(T) - i)); snaps[i:i + len(k)] += np.exp(-k / 300) * rng.standard_normal(len(k))
save("Story_Flags_Loop", band(n, 300, 4000) * flut * 0.5 + band(snaps, 200, 6000) * 1.5, loop=True)

# the golden plume: a low, slow drone (two detuned fifths and a far choir-like shimmer), 14 s, swelling and fading
T = t(14.0); env = np.sin(np.pi * T / 14.0) ** 1.5
x = sum(np.sin(2 * math.pi * f * T + p) for f, p in [(55, 0), (55.4, 1), (82.4, 2), (110, 0.5), (164.8, 1.7)])
x += 0.25 * band(rng.standard_normal(len(T)), 600, 1400) * (0.5 + 0.5 * np.sin(2 * math.pi * 0.3 * T))
save("Story_Plume_Drone", x * env)

# heartbeat for the peak (lub-dub, 72 bpm), 10 s
T = t(10.0); x = np.zeros(len(T))
for b in np.arange(0.2, 10, 60 / 72):
    for d, a in ((0, 1.0), (0.24, 0.6)):
        i = int((b + d) * SR); k = np.arange(min(5000, len(T) - i)); x[i:i + len(k)] += a * np.sin(2 * math.pi * 48 * k / SR) * np.exp(-k / 900)
save("Story_Heartbeat", lp(x, 0.2))
