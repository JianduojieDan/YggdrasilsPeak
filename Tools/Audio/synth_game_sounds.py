# Sounds the game needs that none of the user's recordings cover, made here from scratch (free, ours):
#   the weather wheel landing. User review: it must feel like FATE being decided -- epic, heavy, poetic, not a light UI
#   chime. Round 2 kept only C (low rune bell); E-I build on it. UI_Chime_01 (what the game plays) = the chosen one.
# Writes Saved/AudioWork/out/<Folder>/<Name>.wav + out/manifest_synth.json; import with Tools/Audio/import_game_sounds.py.
#   python -I Tools/Audio/synth_game_sounds.py
import os, json
import numpy as np
from scipy.io import wavfile
from scipy.signal import fftconvolve, butter, sosfilt, sosfiltfilt

OUT = "F:/UE_project/YggdrasilsPeak/Saved/AudioWork/out"
SR = 48000
manifest = []
rng = np.random.default_rng(7)
# rejected by the user (round 1: 01 "not good enough"; round 2: A, B, D) -- still generated in the same order so the
# random phases of the approved C stay byte-identical, but not written
DROP = {"UI_Chime_Glass", "UI_Chime_A", "UI_Chime_B", "UI_Chime_D",
        "UI_Chime_F", "UI_Chime_G", "UI_Chime_I",   # round 3: F "too much", G "why a lightning sound", I "too similar"
        "UI_Chime_J", "UI_Chime_N"}                 # round 4: J "too heavy", N "not good"
# Approved landings (C, E, H, K, L, M) are used per realm by danger: Config/RealmLooks.json "Landing".
CHOSEN = "UI_Chime_C"          # the game's landing sound (written again as UI_Chime_01) until the user picks another

def save(folder, name, a, loop=False):
    if name in DROP:
        return
    if name == CHOSEN:
        save(folder, "UI_Chime_01", a)
    os.makedirs(f"{OUT}/{folder}", exist_ok=True)
    a = np.clip(a, -1, 1)
    wavfile.write(f"{OUT}/{folder}/{name}.wav", SR, (a * 32767).astype(np.int16))
    manifest.append({"folder": folder, "name": name, "loop": loop, "seconds": round(len(a) / SR, 2), "channels": int(a.shape[1]) if a.ndim > 1 else 1})

def bell(f0, seconds, partials):
    t = np.arange(int(seconds * SR)) / SR
    out = np.zeros((len(t), 2))
    for ratio, amp, decay in partials:
        for ch, detune in ((0, 0.9993), (1, 1.0007)):                  # a hair apart left/right: shimmer, not chorus
            ph = rng.uniform(0, 2 * np.pi)
            out[:, ch] += amp * np.exp(-t / decay) * np.sin(2 * np.pi * f0 * ratio * detune * t + ph)
    attack = np.minimum(1.0, t / 0.006) ** 2                          # soft mallet, no click
    return out * attack[:, None]

def chime():
    # glass bell on A5 (880 Hz): partial ratios of a small bell, upper partials die first
    a = bell(880.0, 2.6, [(1.0, 1.0, 0.9), (2.01, 0.42, 0.45), (2.76, 0.25, 0.3), (5.40, 0.08, 0.12), (8.93, 0.03, 0.06)])
    a += 0.35 * bell(1318.5, 2.6, [(1.0, 1.0, 0.7), (2.01, 0.3, 0.3)]) * (np.arange(len(a)) / SR > 0.09)[:, None]   # a fifth above, a moment later
    fade = np.ones(len(a)); fade[-int(0.4 * SR):] = np.linspace(1, 0, int(0.4 * SR)) ** 2
    a *= fade[:, None]
    a *= 10 ** (-6 / 20) / np.abs(a).max()
    save("UI", "UI_Chime_Glass", a)                                   # the first chime (rejected), kept for rng order

def reverb(a, seconds=2.4, wet=0.28, seed=11):
    """A soft room: decaying noise impulse response that gets darker as it fades, convolved; wet/dry mix."""
    from scipy.signal import fftconvolve, butter, sosfilt
    r = np.random.default_rng(seed)
    n = int(seconds * SR); t = np.arange(n) / SR
    ir = r.standard_normal((n, 2)) * np.exp(-t / (seconds / 6.9))[:, None]
    lo = sosfilt(butter(2, 2500, fs=SR, output="sos"), ir, axis=0)
    mixk = np.clip(t / seconds * 1.6, 0, 1)[:, None]               # bright early, dark late
    ir = ir * (1 - mixk) + lo * mixk
    ir /= np.sqrt((ir ** 2).sum(axis=0, keepdims=True))
    tail = np.stack([fftconvolve(a[:, c], ir[:, c]) for c in range(2)], axis=1)
    out = np.zeros_like(tail); out[:len(a)] = a
    return out * (1 - wet) + tail * wet

def finish(a, peak_db=-6.0, tail=0.6):
    f = np.ones(len(a)); k = int(tail * SR); f[-k:] = np.linspace(1, 0, k) ** 2
    a = a * f[:, None]
    return a * (10 ** (peak_db / 20) / np.abs(a).max())

def pluck(f0, seconds, bright=0.5):
    """Karplus-Strong string: a warm plucked note (kalimba / harp like)."""
    n = int(seconds * SR); p = int(SR / f0)
    buf = rng.uniform(-1, 1, p) * np.hanning(p) ** bright
    out = np.zeros(n)
    for i in range(n):
        out[i] = buf[i % p]
        buf[i % p] = 0.996 * 0.5 * (buf[i % p] + buf[(i + 1) % p])
    return np.stack([out, out], axis=1)

def chime_versions():
    # A  singing bowl: low, slow, beating partials -- calm, a little mystical
    a = bell(392.0, 4.5, [(1.0, 1.0, 1.6), (1.003, 0.6, 1.5), (2.71, 0.35, 0.9), (2.716, 0.25, 0.85), (5.15, 0.10, 0.4), (8.17, 0.04, 0.2)])
    a *= np.minimum(1, np.arange(len(a)) / (0.03 * SR))[:, None]
    save("UI", "UI_Chime_A", finish(reverb(a, 2.8, 0.25), -7, 1.0))
    # B  glass, two notes rising a fifth (E5 -> B5): light and elegant
    a = bell(659.3, 3.2, [(1.0, 1.0, 0.8), (2.0, 0.2, 0.35), (3.0, 0.06, 0.15)])
    b = bell(987.8, 3.2, [(1.0, 1.0, 0.9), (2.0, 0.18, 0.35), (3.0, 0.05, 0.15)])
    k = int(0.14 * SR); a[k:] += 0.8 * b[:-k]
    save("UI", "UI_Chime_B", finish(reverb(a, 2.4, 0.3), -7, 0.8))
    # C  low rune bell (A3 with the hum, minor third and fifth of a real bell): weighty, "the sky has decided"
    a = bell(220.0, 5.0, [(0.5, 0.5, 2.2), (1.0, 1.0, 1.8), (1.19, 0.45, 1.3), (1.5, 0.35, 1.1), (2.0, 0.4, 0.9), (2.74, 0.18, 0.5), (3.0, 0.12, 0.4)])
    save("UI", "UI_Chime_C", finish(reverb(a, 3.0, 0.3), -7, 1.2))
    # D  plucked arpeggio (D5 F5 A5, a soft minor chord): warm, like a small kantele / kalimba
    a = np.zeros((int(3.6 * SR), 2))
    for i, f in enumerate((587.3, 698.5, 880.0)):
        k = int(i * 0.075 * SR); p = pluck(f, 3.6, 0.7)[: len(a) - k]
        a[k:k + len(p)] += p * (0.9 - 0.1 * i)
    save("UI", "UI_Chime_D", finish(reverb(a, 2.4, 0.3), -7, 0.8))

# ---------------------------------------------------------------- round 3: the landing as FATE (all in D, a dark key)
def hall(a, seconds=6.5, wet=0.45, predelay=0.035, seed=21):
    """A large stone hall: dense stereo tail where the lows ring longest and the highs die first, early reflections."""
    r = np.random.default_rng(seed)
    n = int(seconds * SR); t = np.arange(n) / SR
    noise = r.standard_normal((n, 2))
    bands = ((sosfilt(butter(2, 300, fs=SR, output="sos"), noise, axis=0), seconds / 4.5),
             (sosfilt(butter(2, [300, 2500], btype="band", fs=SR, output="sos"), noise, axis=0), seconds / 6.0),
             (sosfilt(butter(2, 2500, btype="high", fs=SR, output="sos"), noise, axis=0), seconds / 11.0))
    ir = sum(b * np.exp(-t / tau)[:, None] for b, tau in bands)
    ir *= np.minimum(1, t / 0.08)[:, None] ** 0.5                       # the tail swells in, no hard edge
    for d, g in ((0.011, 0.5), (0.019, 0.4), (0.029, 0.33), (0.043, 0.25)):
        ir[int(d * SR), 0] += g * 3; ir[int(d * 1.07 * SR), 1] += g * 3
    ir /= np.sqrt((ir ** 2).sum(axis=0, keepdims=True))
    pd = int(predelay * SR)
    tail = np.stack([fftconvolve(a[:, c], ir[:, c]) for c in range(2)], axis=1)
    out = np.zeros((len(tail) + pd, 2)); out[:len(a)] += a * (1 - wet); out[pd:pd + len(tail)] += tail * wet
    return out

def place(dst, src, t0, gain=1.0):
    k = int(t0 * SR); n = min(len(src), len(dst) - k)
    dst[k:k + n] += src[:n] * gain

def great_bell(f0, seconds):
    """A big cast bell: hum (an octave down, rings longest), prime, minor-third tierce, quint, nominal, upper partials,
    plus the metal strike itself."""
    a = bell(f0, seconds, [(0.5, 0.75, 5.5), (1.0, 1.0, 3.8), (1.19, 0.5, 2.7), (1.5, 0.38, 2.3), (2.0, 0.45, 1.8),
                           (2.52, 0.2, 1.0), (3.0, 0.15, 0.8), (4.07, 0.08, 0.45), (5.4, 0.04, 0.25)])
    t = np.arange(len(a)) / SR
    strike = sosfilt(butter(2, 2500, btype="high", fs=SR, output="sos"), rng.standard_normal((len(a), 2)), axis=0) * np.exp(-t / 0.018)[:, None]
    return a + 0.18 * strike

def sub_impact(seconds=2.0, f_end=41.0):
    """Felt more than heard: a deep thump whose pitch falls as it dies."""
    t = np.arange(int(seconds * SR)) / SR
    f = f_end + 22 * np.exp(-t / 0.08)
    x = np.sin(2 * np.pi * np.cumsum(f) / SR) * np.exp(-t / 0.75) * np.minimum(1, t / 0.004)
    return np.stack([x, x], axis=1)

def war_drum(seconds=2.6):
    """A great frame drum: skin slap + a body that drops from ~98 Hz to 48 Hz."""
    t = np.arange(int(seconds * SR)) / SR
    f = 48 + 50 * np.exp(-t / 0.06)
    body = np.sin(2 * np.pi * np.cumsum(f) / SR) * np.exp(-t / 0.55)
    skin = sosfilt(butter(2, [150, 900], btype="band", fs=SR, output="sos"), rng.standard_normal(len(t))) * np.exp(-t / 0.05) * 0.6
    x = (body + skin) * np.minimum(1, t / 0.002)
    return np.stack([x, x * 0.97], axis=1)

def horn(f0, seconds, swell=1.2, decay=3.2):
    """A long wooden / bronze horn (lur) far away: rich harmonics, brass formants, slow swell, slight vibrato."""
    t = np.arange(int(seconds * SR)) / SR
    vib = 1 + 0.003 * np.sin(2 * np.pi * 4.6 * t) * np.minimum(1, t / 1.0)
    ph = 2 * np.pi * f0 * np.cumsum(vib) / SR
    x = sum((1.0 / n ** 1.25) * np.sin(n * ph + rng.uniform(0, 2 * np.pi)) for n in range(1, 40) if n * f0 < 5000)
    x = x + 0.8 * sosfiltfilt(butter(2, [450, 900], btype="band", fs=SR, output="sos"), x) + 0.4 * sosfiltfilt(butter(2, [1100, 1500], btype="band", fs=SR, output="sos"), x)
    x = sosfiltfilt(butter(2, 2600, fs=SR, output="sos"), x)
    env = np.where(t < swell, (t / swell) ** 2 * (3 - 2 * t / swell), np.exp(-(t - swell) / decay))
    x = x * env / np.abs(x).max()
    return np.stack([x, x], axis=1)

def fate_versions():
    D3 = 146.83
    # E  the bell of the Norns: one great bell in a stone hall, a deep thump under the strike
    a = np.zeros((int(9 * SR), 2))
    place(a, great_bell(D3, 9.0), 0.0); place(a, sub_impact(), 0.0, 0.55)
    save("UI", "UI_Chime_E", finish(hall(a, 7.0, 0.42)[: int(10.5 * SR)], -3, 3.5))
    # F  fate drum: two drum beats (doom - doom, like a heart) with the great bell on the first
    a = np.zeros((int(9 * SR), 2))
    place(a, war_drum(), 0.0, 1.0); place(a, war_drum(), 0.42, 0.55)
    place(a, great_bell(D3, 8.5), 0.0, 0.6)
    save("UI", "UI_Chime_F", finish(hall(a, 6.5, 0.4)[: int(10.5 * SR)], -3, 3.5))
    # G  the sky answers: the great bell, then the user's own approved thunder rolls in far away beneath it
    a = np.zeros((int(10 * SR), 2))
    place(a, great_bell(D3, 9.5), 0.0); place(a, sub_impact(), 0.0, 0.4)
    a = hall(a, 6.0, 0.35)[: len(a)]
    sr, th = wavfile.read(f"{OUT}/Weather/Thunder_04.wav")
    th = th.astype(np.float32) / 32767
    th = th if th.ndim == 2 else np.stack([th, th], axis=1)
    roll = sosfiltfilt(butter(2, 900, fs=SR, output="sos"), th[int(0.8 * SR): int(9.3 * SR)], axis=0)
    roll *= np.minimum(1, np.arange(len(roll)) / (0.8 * SR))[:, None]
    roll *= 0.45 * np.sqrt((a ** 2).mean()) / (np.sqrt((roll ** 2).mean()) + 1e-9)
    place(a, roll, 0.6)
    save("UI", "UI_Chime_G", finish(a, -3, 2.5))
    # H  horn of fate: the great bell, and two distant horns answer in an open fifth (D2 + A2) -- ancient, Nordic
    a = np.zeros((int(10 * SR), 2))
    place(a, great_bell(D3, 9.5), 0.0, 0.85); place(a, sub_impact(), 0.0, 0.4)
    place(a, horn(73.42, 7.5), 0.25, 0.32); place(a, horn(110.0, 7.2), 0.45, 0.22)
    save("UI", "UI_Chime_H", finish(hall(a, 7.0, 0.5)[: int(10.5 * SR)], -3, 3.5))
    # I  the approved C, heavier: the same low rune bell a fourth lower (D3), with the thump and the big hall
    a = np.zeros((int(9 * SR), 2))
    place(a, bell(D3, 8.0, [(0.5, 0.5, 3.3), (1.0, 1.0, 2.7), (1.19, 0.45, 2.0), (1.5, 0.35, 1.7), (2.0, 0.4, 1.4), (2.74, 0.18, 0.8), (3.0, 0.12, 0.6)]), 0.0)
    place(a, sub_impact(), 0.0, 0.5)
    save("UI", "UI_Chime_I", finish(hall(a, 6.5, 0.4)[: int(10.5 * SR)], -3, 3.5))

# ---------------------------------------------------------------- round 4: new DIRECTIONS (not more bells)
# User, round 3: C and E good, H "so epic", I "give me more samples with different direction". Own generator so C, E, H
# above stay byte-identical. All in D, all with weight and space.
r4 = np.random.default_rng(404)

def additive(f0, t, n_max=60, tilt=1.0, freq_mod=None, top=6000.0):
    """Harmonic tone: partial n at n*f0 with amplitude 1/n^tilt, random phases; optional per-sample freq factor."""
    fm = freq_mod if freq_mod is not None else np.ones_like(t)
    ph = 2 * np.pi * f0 * np.cumsum(fm) / SR
    return sum((1.0 / n ** tilt) * np.sin(n * ph + r4.uniform(0, 2 * np.pi)) for n in range(1, n_max + 1) if n * f0 < top)

def bp(x, lo, hi, order=2):
    return sosfiltfilt(butter(order, [lo, hi], btype="band", fs=SR, output="sos"), x, axis=0)

def lp(x, hz, order=2):
    return sosfiltfilt(butter(order, hz, fs=SR, output="sos"), x, axis=0)

def swell_env(t, attack, decay, hold=0.0):
    a = np.clip(t / attack, 0, 1); a = a * a * (3 - 2 * a)
    return np.where(t < attack + hold, a, np.exp(-(t - attack - hold) / decay))

def stereo(l, r=None):
    return np.stack([l, l if r is None else r], axis=1)

def dungchen(f0, seconds, detune=1.0):
    """Tibetan long horn: very low, a lip-buzz growl (half-frequency amplitude wobble), scoops up into the note."""
    t = np.arange(int(seconds * SR)) / SR
    scoop = (1 - 0.035 * np.exp(-t / 0.3)) * detune
    x = additive(f0, t, 80, 0.9, scoop, 5000)
    x = x * (1 + 0.45 * np.sin(np.pi * f0 * np.cumsum(scoop) / SR))           # growl
    x = x + 1.2 * bp(x, 250, 650) + 0.5 * bp(x, 900, 1400)
    x = lp(x, 2800) + 0.08 * lp(r4.standard_normal(len(t)), 1200) * np.abs(x).max()
    x = x * swell_env(t, 0.9, 3.2, 0.8)
    return x / np.abs(x).max()

def brass_chord(freqs, seconds):
    """Cinematic low brass: bright harmonics, brass formants, a little saturation, quick swell, long fall."""
    t = np.arange(int(seconds * SR)) / SR
    x = np.zeros(len(t))
    for i, f in enumerate(freqs):
        v = additive(f * (1 + r4.uniform(-0.002, 0.002)), t, 50, 0.8, 1 + 0.002 * np.sin(2 * np.pi * (4.8 + i * 0.3) * t), 7000)
        x += v / np.abs(v).max()
    x = x + 0.9 * bp(x, 500, 1100) + 0.4 * bp(x, 1300, 2000)
    x = np.tanh(1.6 * x / np.abs(x).max())
    x = lp(x, 3500) * swell_env(t, 0.28, 2.6, 0.9)
    return x / np.abs(x).max()

VOWEL_OO = ((330, 70, 1.0), (750, 90, 0.5), (2400, 140, 0.12))     # formant Hz, bandwidth, gain ("ooh")
def choir(freqs, seconds, voices=4):
    """Wordless choir: per voice a breathy pulse source (vibrato, slow drift) through vowel formants; voices detuned."""
    t = np.arange(int(seconds * SR)) / SR
    L = np.zeros(len(t)); R = np.zeros(len(t))
    for f in freqs:
        for v in range(voices):
            cents = r4.uniform(-9, 9)
            vib = 1 + 0.006 * np.sin(2 * np.pi * r4.uniform(4.6, 5.6) * t + r4.uniform(0, 6)) + 0.002 * np.sin(2 * np.pi * 0.3 * t + r4.uniform(0, 6))
            src = additive(f * 2 ** (cents / 1200), t, 40, 1.6, vib, 5000) + 0.03 * r4.standard_normal(len(t))
            voice = sum(g * bp(src, fc - bw, fc + bw) for fc, bw, g in VOWEL_OO)
            pan = r4.uniform(0.2, 0.8)
            L += voice * (1 - pan); R += voice * pan
    env = swell_env(t, 1.3, 3.0, 1.0)
    out = stereo(L * env, R * env)
    return out / np.abs(out).max()

def bowed(f0, seconds):
    """A low bowed string / tagelharpa drone: sawtooth body, bow noise, brightness that opens and closes."""
    t = np.arange(int(seconds * SR)) / SR
    x = additive(f0, t, 70, 1.0, 1 + 0.003 * np.sin(2 * np.pi * 5.1 * t) * np.clip(t / 1.5, 0, 1), 6000)
    bow = bp(r4.standard_normal(len(t)), 800, 3000) * 0.05
    bright = 0.35 + 0.65 * swell_env(t, 1.5, 2.5, 0.5)
    x = lp(x, 900) * (1 - bright) + lp(x, 3200) * bright + bow
    x = x * swell_env(t, 0.6, 3.5, 1.2)
    return x / np.abs(x).max()

def gong(f0, seconds):
    """A great temple gong: dense inharmonic partials; the low ones speak at once and sink a little in pitch, the high
    ones BLOOM in after the strike (the gong's swell) and shimmer -- unlike a bell, which only decays."""
    t = np.arange(int(seconds * SR)) / SR
    L = np.zeros(len(t)); R = np.zeros(len(t))
    for k in range(70):
        ratio = 1.0 + k * 0.37 + r4.uniform(-0.15, 0.15)
        f = f0 * ratio
        if f > 7000: break
        glide = 1 - 0.012 * (1 - np.exp(-t / 1.5)) if ratio < 3 else 1.0
        bloom = 0.0 if ratio < 2.5 else min(1.6, 0.25 * ratio)                 # seconds until it peaks
        env = (np.clip(t / max(bloom, 0.003), 0, 1) ** 1.5) * np.exp(-np.maximum(0, t - bloom) / (4.5 / (1 + 0.15 * ratio)))
        amp = 1.0 / ratio ** 0.9
        for ch, d in ((0, 0.9995), (1, 1.0005)):
            s = amp * env * np.sin(2 * np.pi * f * d * np.cumsum(glide) / SR + r4.uniform(0, 6.3))
            if ch == 0: L += s
            else: R += s
    out = stereo(L, R)
    return out / np.abs(out).max()

def direction_versions():
    D2, A2, D3, F3, A3, D4 = 73.42, 110.0, 146.83, 174.61, 220.0, 293.66
    secs = 10.5
    # J  Tibetan long horns (dungchen): two great horns in near-unison, growling, far across the valley; a soft gong
    a = np.zeros((int(secs * SR), 2))
    place(a, stereo(dungchen(D2, 9.5, 1.0)), 0.0, 0.55); place(a, stereo(dungchen(D2, 9.3, 1.004)), 0.12, 0.45)
    place(a, gong(D3 / 2, 9.0), 0.0, 0.18)
    save("UI", "UI_Chime_J", finish(hall(a, 6.5, 0.5)[: int(secs * SR)], -3, 3.5))
    # K  cinematic brass: one huge low D-minor brass chord swelling in over a drum thump
    a = np.zeros((int(secs * SR), 2))
    place(a, stereo(brass_chord((D2, A2, D3, F3), 9.0)), 0.0, 0.8); place(a, sub_impact(), 0.0, 0.6); place(a, war_drum(), 0.0, 0.45)
    save("UI", "UI_Chime_K", finish(hall(a, 6.0, 0.42)[: int(secs * SR)], -3, 3.5))
    # L  voices of the Norns: a slow wordless choir chord (D minor), as if fate were sung
    a = np.zeros((int(secs * SR), 2))
    place(a, choir((D3, F3, A3, D4), 9.5), 0.0, 0.9); place(a, sub_impact(), 0.0, 0.25)
    save("UI", "UI_Chime_L", finish(hall(a, 7.0, 0.55)[: int(secs * SR)], -3, 3.5))
    # M  bowed drone: a low bowed string in an open fifth, one soft frame-drum beat -- dark, ancient, patient
    a = np.zeros((int(secs * SR), 2))
    place(a, stereo(bowed(D2, 9.5)), 0.0, 0.6); place(a, stereo(bowed(A2, 9.3)), 0.25, 0.4); place(a, war_drum(), 0.0, 0.5)
    save("UI", "UI_Chime_M", finish(hall(a, 6.0, 0.45)[: int(secs * SR)], -3, 3.5))
    # N  temple gong: one great gong whose shimmer blooms after the strike
    a = np.zeros((int(secs * SR), 2))
    place(a, gong(D2, 10.0), 0.0, 1.0); place(a, sub_impact(), 0.0, 0.35)
    save("UI", "UI_Chime_N", finish(hall(a, 6.5, 0.4)[: int(secs * SR)], -3, 3.5))

if __name__ == "__main__":
    chime()
    chime_versions()
    fate_versions()
    direction_versions()
    json.dump(manifest, open(f"{OUT}/manifest_synth.json", "w"), indent=1)
    print("made", [m["name"] for m in manifest])
