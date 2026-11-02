# Procedural foley for the climber's footsteps where the project has no recorded sound (placeholders until real
# recordings): crampons biting into glacier ice, boots on moraine rubble, crampons on an aluminium ladder rung.
#   python Tools/Audio/synth_footsteps.py [out_dir]      -> Step_Ice_01..05.wav, Step_Rubble_01..05.wav, Step_Ladder_01..05.wav
# 44.1 kHz mono 16-bit. Each variation has its own seed, so re-running gives the same files.
import numpy as np, os, sys, wave
from scipy.signal import butter, sosfilt

SR = 44100
OUT = sys.argv[1] if len(sys.argv) > 1 else os.path.join(os.path.dirname(__file__), "..", "..", "Art", "Audio", "Footsteps")
os.makedirs(OUT, exist_ok=True)


def band(x, lo, hi, order=4):
    return sosfilt(butter(order, [lo, hi], btype="band", fs=SR, output="sos"), x)


def low(x, hi, order=4):
    return sosfilt(butter(order, hi, btype="low", fs=SR, output="sos"), x)


def env(n, attack, decay):
    t = np.arange(n) / SR
    return np.minimum(t / max(attack, 1e-4), 1.0) * np.exp(-np.maximum(t - attack, 0) / decay)


def place(buf, x, at):
    i = int(at * SR)
    j = min(len(buf), i + len(x))
    buf[i:j] += x[: j - i]


def write(name, x):
    x = x / (np.max(np.abs(x)) + 1e-9) * 0.89
    fade = int(0.01 * SR)
    x[-fade:] *= np.linspace(1, 0, fade)
    with wave.open(os.path.join(OUT, name), "wb") as w:
        w.setnchannels(1); w.setsampwidth(2); w.setframerate(SR)
        w.writeframes((x * 32767).astype(np.int16).tobytes())


def crampon_bite(rng, n, bright=1.0):
    """Steel points stabbing ice: a very short bright click, then a brittle scrape of ice chips."""
    click = band(rng.standard_normal(n), 2500 * bright, min(14000 * bright, 20000)) * env(n, 0.0005, 0.006)
    chips = band(rng.standard_normal(n), 1800, 9000) * env(n, 0.002, 0.035) * 0.35
    grains = np.zeros(n)
    for _ in range(rng.integers(6, 14)):           # flakes breaking off
        g = int(rng.uniform(0.002, 0.05) * SR)
        if g < n - 200:
            grains[g:g + 200] += band(rng.standard_normal(200), 3000, 12000) * env(200, 0.0003, 0.001) * rng.uniform(0.1, 0.4)
    return click + chips + grains


def boot_thump(rng, n, lo=60, hi=260, decay=0.045):
    return low(rng.standard_normal(n), hi) * env(n, 0.004, decay) * 2.5 + np.sin(2 * np.pi * lo * np.arange(n) / SR) * env(n, 0.003, decay) * 0.4


def step_ice(seed):
    rng = np.random.default_rng(seed)
    buf = np.zeros(int(0.45 * SR))
    n = int(0.25 * SR)
    place(buf, boot_thump(rng, n, decay=0.03) * 0.5, 0.0)
    place(buf, crampon_bite(rng, n), 0.004)                                 # front points
    place(buf, crampon_bite(rng, n, 0.8) * rng.uniform(0.5, 0.8), rng.uniform(0.05, 0.085))   # heel points
    return buf


def step_rubble(seed):
    rng = np.random.default_rng(seed)
    buf = np.zeros(int(0.5 * SR))
    n = int(0.3 * SR)
    place(buf, boot_thump(rng, n, hi=320, decay=0.06), 0.0)
    # stones knocking and grinding: many short pitched clicks, denser at the start, plus a gritty scrape
    for _ in range(rng.integers(40, 70)):
        t = abs(rng.normal(0.0, 0.06))
        g = int(rng.uniform(0.002, 0.012) * SR)
        f = rng.uniform(900, 4500)
        k = np.arange(g) / SR
        stone = np.sin(2 * np.pi * f * k) * np.exp(-k / rng.uniform(0.0015, 0.006)) * rng.uniform(0.05, 0.3)
        place(buf, stone, t)
    place(buf, band(rng.standard_normal(n), 700, 6000) * env(n, 0.01, 0.07) * 0.25, 0.005)
    return buf


def step_ladder(seed):
    rng = np.random.default_rng(seed)
    buf = np.zeros(int(0.9 * SR))
    n = int(0.8 * SR)
    t = np.arange(n) / SR
    # aluminium rung / rail: a few inharmonic modes (thin-walled tube), short and damped by the boot, a hollow body
    base = rng.uniform(380, 520)
    ring = np.zeros(n)
    for ratio, amp, dec in ((1.0, 1.0, 0.16), (2.76, 0.6, 0.11), (5.40, 0.35, 0.07), (8.93, 0.2, 0.04), (13.3, 0.1, 0.025)):
        ring += amp * np.sin(2 * np.pi * base * ratio * rng.uniform(0.985, 1.015) * t + rng.uniform(0, 6.28)) * np.exp(-t / dec)
    hit = ring * env(n, 0.0008, 0.5) * 0.6
    place(buf, boot_thump(rng, n, lo=90, hi=400, decay=0.05) * 0.7, 0.0)
    place(buf, hit + crampon_bite(rng, n, 1.1) * 0.6, 0.003)
    # the whole span flexes and rattles in its lashings a moment later
    rattle = np.zeros(n)
    for _ in range(rng.integers(3, 7)):
        k = int(rng.uniform(0.04, 0.16) * SR)
        r = band(rng.standard_normal(600), 1200, 6000) * env(600, 0.0003, 0.002) * rng.uniform(0.05, 0.2)
        rattle[k:k + 600] += r
    place(buf, rattle, 0.0)
    return buf


for i in range(1, 6):
    write(f"Step_Ice_{i:02d}.wav", step_ice(100 + i))
    write(f"Step_Rubble_{i:02d}.wav", step_rubble(200 + i))
    write(f"Step_Ladder_{i:02d}.wav", step_ladder(300 + i))
print("written to", os.path.abspath(OUT))
