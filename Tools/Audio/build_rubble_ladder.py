# Real-recording footsteps for rubble/rock and the crevasse ladders (2026-11-11: the user rated the synthesized
# placeholders "extremely bad"). Sources (free, downloaded to Downloads/, decoded to Saved/AudioWork/dl by ffmpeg):
#   - BigSoundBank "Footsteps on gravels 2" (#1117) and "4" (#3216): boots walking on gravel -> single steps for moraine rubble
#   - OpenGameArt "footsteps on different surfaces" metal set, derived from "boots on aluminum ladder 01" by Eelke
#     (freesound 462598) -> ladder rungs, layered with the user's approved crampon steps (Step_Ice) so crampon points bite the rung
# Rules from the user's reviews: a step is a WHOLE event (foot down + roll + lift off) cut between silences, its peak in
# the first 0.12 s; never cut at a "second event".
#   python -I Tools/Audio/build_rubble_ladder.py  -> Saved/AudioWork/out/Footsteps/Step_Rubble_01..08.wav, Step_Ladder_01..06.wav
import os, glob, random
import numpy as np
from scipy.io import wavfile
from scipy.signal import butter, sosfilt

ROOT = "F:/UE_project/YggdrasilsPeak"
DL = f"{ROOT}/Saved/AudioWork/dl"
OUT = f"{ROOT}/Saved/AudioWork/out/Footsteps"
SR = 48000


def read(p):
    sr, a = wavfile.read(p)
    assert sr == SR, (p, sr)
    a = a.astype(np.float32) / 32768.0
    return a if a.ndim == 2 else np.stack([a, a], 1)


def write(name, a):
    wavfile.write(f"{OUT}/{name}.wav", SR, (np.clip(a, -1, 1) * 32767).astype(np.int16))


def env(a, ms=12):
    m = np.abs(a).mean(1)
    k = int(SR * ms / 1000)
    return np.convolve(m, np.ones(k) / k, "same")


def rms(a):
    return float(np.sqrt(np.mean(a ** 2)) + 1e-12)


def fades(a, fin=0.006, fout=0.05):
    a = a.copy()
    n1, n2 = int(fin * SR), min(len(a) // 2, int(fout * SR))
    a[:n1] *= np.linspace(0, 1, n1)[:, None]
    a[-n2:] *= np.linspace(1, 0, n2)[:, None] ** 1.5
    return a


def steps_in(a, max_len=0.65):
    """Whole step events between silences: each envelope peak, from where it rises out of the quiet before it to the
    quiet after it (gravel steps here fall to ~3% of their peak between steps)."""
    from scipy.signal import find_peaks
    e = env(a)
    pk, _ = find_peaks(e, distance=int(0.3 * SR), prominence=np.percentile(e, 95) * 0.4)
    out = []
    for n, p in enumerate(pk):
        lo = e[p] * 0.08
        s = p
        while s > 0 and e[s] > lo and p - s < int(0.06 * SR):
            s -= 1
        nxt = pk[n + 1] if n + 1 < len(pk) else len(e) - 1
        j = p + int(0.12 * SR)
        while j < nxt and e[j] > e[p] * 0.05 and j - s < max_len * SR:
            j += 1
        dur = (j - s) / SR
        if not (0.18 <= dur <= max_len) or (p - s) > 0.12 * SR or e[j] > e[p] * 0.08:
            continue
        pre = float(e[max(0, s - int(0.08 * SR)):s].mean()) if s > 0 else 1.0
        out.append((float(e[p]) / (pre + 1e-6) * min(1.0, float(e[p]) / 0.03), a[s:j]))
    return out


def lead_trim(a):
    e = env(a, 4)
    k = int(np.argmax(e > e.max() * 0.05))
    return a[max(0, k - int(0.004 * SR)):]


os.makedirs(OUT, exist_ok=True)
snow = [read(p) for p in sorted(glob.glob(f"{OUT}/Step_Snow_*.wav"))]
target = float(np.mean([rms(s) for s in snow]))
peak_target = float(np.mean([np.abs(s).max() for s in snow]))

# ---- rubble: the cleanest whole gravel steps of both recordings, spread over the files
cands = []
for p in sorted(glob.glob(f"{DL}/bsb/*.wav")):
    ev = steps_in(read(p))
    print(os.path.basename(p), "steps found", len(ev))
    cands += ev
cands.sort(key=lambda c: -c[0])
picked = []
for score, seg in cands:
    if len(picked) == 8:
        break
    # not two near-identical steps: different length or loudness shape
    if all(abs(len(seg) - len(q)) > 0.03 * SR or abs(rms(seg) - rms(q)) > 0.1 * rms(q) for q in picked):
        picked.append(seg)
for k, seg in enumerate(picked, 1):
    seg = fades(seg)
    # moraine rubble under a heavy mountain boot: a touch more low body than garden gravel
    low = sosfilt(butter(2, 220, btype="low", fs=SR, output="sos"), seg, axis=0)
    seg = seg + low * 0.6
    seg *= (target * 1.12) / rms(seg)
    seg *= min(1.0, peak_target * 1.25 / np.abs(seg).max())
    write(f"Step_Rubble_{k:02d}", seg)
    print(f"Step_Rubble_{k:02d}  {len(seg)/SR:.2f} s")

# ---- ladder: boots on an aluminium ladder, with crampon points biting the rung (the user's approved crampon steps)
ice = [lead_trim(read(p)) for p in sorted(glob.glob(f"{OUT}/Step_Ice_*.wav"))]
metal = [lead_trim(read(p)) for p in sorted(glob.glob(f"{DL}/metal/*.wav"))]
metal.sort(key=lambda m: -np.abs(m).max())
rng = random.Random(7)
for k, m in enumerate(metal[:6], 1):
    c = ice[rng.randrange(len(ice))]
    n = max(len(m), len(c))
    mix = np.zeros((n, 2), np.float32)
    mix[:len(m)] += m / (np.abs(m).max() + 1e-6)
    hi = sosfilt(butter(2, 1500, btype="high", fs=SR, output="sos"), c, axis=0)   # only the points' bite, not the ice body
    mix[:len(c)] += hi / (np.abs(hi).max() + 1e-6) * 0.45
    mix = fades(mix[: int(0.7 * SR)], 0.003, 0.08)
    mix *= (target * 1.05) / rms(mix)
    mix *= min(1.0, peak_target * 1.3 / np.abs(mix).max())
    write(f"Step_Ladder_{k:02d}", mix)
    print(f"Step_Ladder_{k:02d}  {len(mix)/SR:.2f} s")
