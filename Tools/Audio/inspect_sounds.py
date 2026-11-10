# Examines every sound made by build_game_sounds.py (Saved/AudioWork/out) before it goes into the game:
#   one-shots: how many separate events are inside (a footstep / click / tick / crack must hold exactly one), whether
#              the start is late or the tail is chopped, clipping;
#   loops:     the seam (end -> start level and spectrum jump), and single loud events baked into the bed (a branch
#              snapping in the wind loop would repeat every lap).
# Writes contact sheets (spectrogram + loudness line per sound) to Saved/AudioWork/inspect/<group>.png and prints flags.
#   python -I Tools/Audio/inspect_sounds.py [group-prefix ...]
import os, sys, json
import numpy as np
from scipy.io import wavfile
from scipy.signal import find_peaks, spectrogram
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt

ROOT = "F:/UE_project/YggdrasilsPeak/Saved/AudioWork"
OUT, INS = f"{ROOT}/out", f"{ROOT}/inspect"
os.makedirs(INS, exist_ok=True)

def load(folder, name):
    sr, a = wavfile.read(f"{OUT}/{folder}/{name}.wav")
    a = a.astype(np.float32) / 32767
    return sr, (a if a.ndim == 1 else a.mean(axis=1))

def env_db(m, sr, hop_s):
    hop = max(1, int(sr * hop_s)); n = len(m) // hop
    return 20 * np.log10(np.sqrt((m[:n * hop].reshape(n, hop) ** 2).mean(axis=1) + 1e-12)), hop

def events(m, sr, rise=9.0, floor_rel=-24.0, gap=0.06):
    e, hop = env_db(m, sr, 0.004)
    k = max(1, int(0.025 * sr / hop))
    jump = e - np.concatenate([np.full(k, e[0]), e[:-k]])
    loud = e > e.max() + floor_rel
    pk, _ = find_peaks(np.where(loud, jump, -99), height=rise, distance=max(1, int(gap * sr / hop)))
    return [p * hop / sr for p in pk]

def check(m, sr, loop, single):
    flags = []
    pk = 20 * np.log10(np.abs(m).max() + 1e-9)
    if pk > -0.3: flags.append("near clipping %.1f dB" % pk)
    e, hop = env_db(m, sr, 0.01)
    if loop:
        w = int(0.25 * sr)
        a, b = m[-w:], m[:w]
        jump = abs(20 * np.log10((np.sqrt((a ** 2).mean()) + 1e-9) / (np.sqrt((b ** 2).mean()) + 1e-9)))
        sa, sb = np.abs(np.fft.rfft(a * np.hanning(w))), np.abs(np.fft.rfft(b * np.hanning(w)))
        spec = float(np.mean(np.abs(np.log10(sa + 1e-6) - np.log10(sb + 1e-6))))
        sample_jump = abs(float(m[-1] - m[0]))
        if jump > 3: flags.append("seam level jump %.1f dB" % jump)
        if spec > 0.35: flags.append("seam spectrum jump %.2f" % spec)
        if sample_jump > 0.05: flags.append("seam click %.2f" % sample_jump)
        med = np.median(e)
        spikes = np.where(e > med + 12)[0]
        if len(spikes): flags.append("loud events in bed at %s s" % ", ".join("%.1f" % (s * 0.01) for s in spikes[::max(1, len(spikes) // 5)][:5]))
        return flags, None
    ev = events(m, sr)
    if single and len(ev) > 1: flags.append("%d events inside: %s s" % (len(ev), ", ".join("%.2f" % t for t in ev)))
    if not ev: flags.append("no clear attack")
    start = np.argmax(e > e.max() - 20) * 0.01
    if start > 0.04: flags.append("starts late %.2f s" % start)
    if e[-1] > e.max() - 20: flags.append("tail chopped (end only %.0f dB under peak)" % (e.max() - e[-1]))
    return flags, ev

def sheet(group, items):
    n = len(items); cols = 6 if n > 6 else n; rows = (n + cols - 1) // cols
    fig, axes = plt.subplots(rows, cols, figsize=(3.2 * cols, 2.3 * rows), squeeze=False)
    for ax in axes.flat: ax.axis("off")
    for ax, (name, sr, m, flags, loop) in zip(axes.flat, items):
        ax.axis("on")
        f, t, S = spectrogram(m, sr, nperseg=1024 if len(m) > 4096 else 256, noverlap=None)
        ax.pcolormesh(t, f / 1000, 10 * np.log10(S + 1e-12), shading="auto", cmap="magma", vmin=-110, vmax=-30)
        ax.set_ylim(0, 16); ax.set_yticks([0, 4, 8, 12, 16]); ax.tick_params(labelsize=6)
        e, hop = env_db(m, sr, max(0.002, len(m) / sr / 400))
        tt = np.arange(len(e)) * hop / sr
        ax2 = ax.twinx(); ax2.plot(tt, e, color="cyan", lw=0.6); ax2.set_ylim(-80, 0); ax2.tick_params(labelsize=5)
        ax.set_title(("%s  %.2fs" % (name, len(m) / sr)) + ("\n" + "; ".join(flags)[:60] if flags else ""), fontsize=6.5, color="red" if flags else "black")
    fig.tight_layout()
    path = f"{INS}/{group}.png"; fig.savefig(path, dpi=80); plt.close(fig)
    return path

SINGLE = ("Step_", "Carabiner_Click", "Wheel_Tick", "Rope_Tighten")
if __name__ == "__main__":
    want = sys.argv[1:]
    man = json.load(open(f"{OUT}/manifest.json"))
    groups = {}
    for x in man:
        g = x["name"].rstrip("0123456789").rstrip("_") if x["name"][-1].isdigit() else ("Loops" if x["loop"] else "Other")
        if want and not any(x["name"].startswith(w) or g.startswith(w) for w in want): continue
        groups.setdefault(g, []).append(x)
    for g, xs in groups.items():
        items = []
        for x in xs:
            sr, m = load(x["folder"], x["name"])
            flags, ev = check(m, sr, x["loop"], x["name"].startswith(SINGLE))
            items.append((x["name"], sr, m, flags, x["loop"]))
            if flags: print("  %-20s %s" % (x["name"], " | ".join(flags)))
        print("%-16s %2d sounds -> %s" % (g, len(items), sheet(g, items)))
