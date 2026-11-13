# Heartbeat one-shots for the climber's body: candidates for the user to judge BY EAR (2026-11-14). Nothing goes into the game.
#
# The game would play ONE beat ("lub-dub") per heart beat at the current rate (48-188 bpm, ExpeditionComponent::
# UpdateScreenEffect), so every file is a single beat that must still read as one beat at 180 bpm (333 ms apart).
#
#   python -I Tools/Audio/make_heartbeat.py all              synth + cut every recording in Downloads/Heartbeat + previews + README
#   python -I Tools/Audio/make_heartbeat.py synth [--halves] synth_calm / synth_strained / synth_deathzone .wav
#                                                            (--halves: also lub and dub alone in halves/, see README)
#   python -I Tools/Audio/make_heartbeat.py cut <source> <file> [n]   <source>_beat_1..n.wav out of a downloaded recording
#   python -I Tools/Audio/make_heartbeat.py previews         preview_<name>_70.wav / _150.wav: 10 s of each one-shot at that rate
#   python -I Tools/Audio/make_heartbeat.py readme           README.md (sources, licences, what each file measures) + listen .bat
#   python -I Tools/Audio/make_heartbeat.py info <file>      length, peak, lub->dub gap... of one file
#   python -I Tools/Audio/make_heartbeat.py listen [--all]   play each candidate at 70 bpm, at 150 bpm and alone; rate it by ear
#                                                            (double-click Saved/AudioWork/Heartbeat/listen_heartbeats.bat)
# Output: Saved/AudioWork/Heartbeat, 48 kHz 16-bit mono WAV, peak -3 dBFS. Downloads are untrusted data: they are only
# decoded (ffmpeg as a plain process, no shell), never run or extracted.
import os, sys, json, glob, time, wave, shutil, hashlib, subprocess
import numpy as np
from scipy.io import wavfile
from scipy.signal import butter, sosfilt, sosfiltfilt, find_peaks, resample_poly

ROOT = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", ".."))
OUT = os.path.join(ROOT, "Saved", "AudioWork", "Heartbeat")
DL = os.path.join(ROOT, "Downloads", "Heartbeat")
SR = 48000
PEAK = 10 ** (-3 / 20)                                        # -3 dBFS
AUDIO_EXT = (".wav", ".flac", ".ogg", ".oga", ".mp3")
PREVIEW_BPM = (70, 150)                                       # resting / slow walk, and a hard climb in thin air
REVIEW_JSON = os.path.join(OUT, "review.json")
REVIEW_MD = os.path.join(OUT, "Review.md")

# Free recordings that need no login (checked 2026-11-14). key = the folder in Downloads/Heartbeat and the name prefix.
# "pick": the 3-5 proposed for download; the others are alternates.
SOURCES = [
    {"key": "commons_bradycardia", "pick": True, "title": "Athletic bradycardia (a resting heart at 42 bpm)",
     "author": "Athletearrhythmia, Wikimedia Commons (own work, 2024)",
     "page": "https://commons.wikimedia.org/wiki/File:Athletic_bradycardia_wav.wav",
     "url": "https://upload.wikimedia.org/wikipedia/commons/5/59/Athletic_bradycardia_wav.wav",
     "file": "Athletic_bradycardia_wav.wav", "size": "1,930,796 bytes (1.84 MB)",
     "licence": "CC0 1.0 Universal public domain dedication (commercial use, no attribution)",
     "about": "a real heart, 20 s, 42 bpm: long quiet gaps, so clean single beats are easy to cut"},
    {"key": "commons_mitral150", "pick": True, "title": "Heartbeat, mitral valve, 150 bpm",
     "author": "ezwa (PDSounds #158, 2007), on Wikimedia Commons",
     "page": "https://commons.wikimedia.org/wiki/File:Heartbeat_mitral_valve_150_bpm.ogg",
     "url": "https://upload.wikimedia.org/wikipedia/commons/2/2b/Heartbeat_mitral_valve_150_bpm.ogg",
     "file": "Heartbeat_mitral_valve_150_bpm.ogg", "size": "87,196 bytes (85 KB)",
     "licence": "Public domain (released by the author worldwide, no conditions)",
     "about": "a real heart after exercise, listened to at the mitral valve (stethoscope spot), 11 s; low-bitrate ogg (62 kbps)"},
    {"key": "bigsoundbank_1929", "pick": True, "title": "Heart Beat #4 (slow)",
     "author": "Joseph Sardin, BigSoundBank #1929 (studio foley, Neumann KM184, 48 kHz 24-bit mono, 47 s)",
     "page": "https://bigsoundbank.com/heart-beat-4-s1929.html",
     "url": None, "file": "(FLAC from the page's format menu)", "size": "FLAC 2.6 MB (WAV 6.5 MB)",
     "licence": "CC0 (public domain): free and royalty-free, attribution not required",
     "about": "a designed (foley) heartbeat recorded in a studio: made to sound heavy rather than clinical. The site answers "
              "scripts with HTTP 403, so it has to be saved from a browser (format menu -> FLAC -> OK)"},
    {"key": "oga_bart", "pick": True, "title": "Heartbeat sounds (slow, dry)",
     "author": "bart, OpenGameArt (2010)",
     "page": "https://opengameart.org/content/heartbeat-sounds",
     "url": "https://opengameart.org/sites/default/files/heartbeat_slow_0.wav",
     "file": "heartbeat_slow_0.wav", "size": "635,216 bytes (620 KB)",
     "licence": "CC0",
     "about": "made from scratch in FL Studio (synthetic, game-style), a loop; the dry file, not *_reverb"},
    {"key": "oga_independent", "pick": True, "title": "Heartbeat (single sound)",
     "author": "Independent.nu, submitted by qubodup, OpenGameArt (2009)",
     "page": "https://opengameart.org/content/heartbeat-single-sound",
     "url": "https://opengameart.org/sites/default/files/heartbeat.mp3_.flac",
     "file": "heartbeat.mp3_.flac", "size": "68,646 bytes (67 KB)",
     "licence": "CC0",
     "about": "one 'bum bum' beat; a FLAC made from an MP3, so the top end may be soft"},
    {"key": "commons_wilfredor", "pick": False, "title": "Human heart rate",
     "author": "Wilfredor, Wikimedia Commons (own work, 2025)",
     "page": "https://commons.wikimedia.org/wiki/File:Human_heart_rate.flac",
     "url": "https://upload.wikimedia.org/wikipedia/commons/a/a4/Human_heart_rate.flac",
     "file": "Human_heart_rate.flac", "size": "2,906,072 bytes (2.77 MB)",
     "licence": "CC0 1.0 Universal public domain dedication",
     "about": "72 s of a real heart; how it was recorded is not stated"},
    {"key": "commons_maxeffort", "pick": False, "title": "10 min max heart rate (over 180 bpm)",
     "author": "Athletearrhythmia, Wikimedia Commons (own work)",
     "page": "https://commons.wikimedia.org/wiki/File:10_min_max_heart_rate_wav.wav",
     "url": "https://upload.wikimedia.org/wikipedia/commons/e/e4/10_min_max_heart_rate_wav.wav",
     "file": "10_min_max_heart_rate_wav.wav", "size": "9,595,902 bytes (9.15 MB)",
     "licence": "CC0 1.0 Universal public domain dedication",
     "about": "an athlete's heart in a maximum effort, over 180 bpm: the real sound of a heart at its limit (big file)"},
]

# lub = the first, stronger thump (the heart squeezing, valves closing); dub = the shorter, higher second one.
# Shapes: (start Hz, end Hz, pitch-fall time s, decay time s). The pitch falls fast: a chest settling, not a drum.
VARIANTS = {
    "calm": dict(seed=11, gap=0.135, lub=(95, 48, 0.022, 0.050), dub=(110, 58, 0.016, 0.034), dub_gain=0.58,
                 drive=1.5, knock=0.38, knock_ratio=2.9, click=0.14, click_lp=480, pressure=0.0, muffle=0,
                 about="a resting heart: round and soft, the dub clearly lighter, the longest gap"),
    "strained": dict(seed=23, gap=0.100, lub=(130, 55, 0.014, 0.040), dub=(150, 66, 0.011, 0.028), dub_gain=0.78,
                     drive=2.0, knock=0.45, knock_ratio=3.1, click=0.20, click_lp=800, pressure=0.0, muffle=0,
                     about="hard climbing: tighter and harder, a quick gap, the dub nearly as strong, a firmer knock"),
    "deathzone": dict(seed=37, gap=0.128, lub=(82, 38, 0.030, 0.050), dub=(96, 46, 0.020, 0.040), dub_gain=0.66,
                      drive=2.4, knock=0.32, knock_ratio=2.6, click=0.12, click_lp=340, pressure=0.22, muffle=900,
                      about="8,000 m: heavier and lower, slower to die away, muffled as if through pressure in the ears, "
                            "a dull rush of blood behind it"),
}


# ------------------------------------------------------------------ audio helpers
def hb_read(path):
    """Any audio file -> float mono at 48 kHz. ffmpeg decodes it (no shell); plain WAVs fall back to scipy."""
    exe = shutil.which("ffmpeg")
    if exe:
        r = subprocess.run([exe, "-nostdin", "-hide_banner", "-v", "error", "-i", path, "-map", "0:a:0", "-ac", "1",
                            "-ar", str(SR), "-f", "f32le", "pipe:1"], capture_output=True, timeout=120)
        if r.returncode == 0 and r.stdout:
            return np.frombuffer(r.stdout, np.float32).astype(np.float64)
        print("  ffmpeg could not decode", path, r.stderr.decode("utf8", "replace")[:300])
    if not path.lower().endswith(".wav"):
        raise SystemExit(f"need ffmpeg on PATH to read {path}")
    sr, a = wavfile.read(path)
    if a.dtype == np.uint8:
        a = (a.astype(np.float64) - 128) / 128
    elif a.dtype.kind == "i":
        a = a.astype(np.float64) / float(np.iinfo(a.dtype).max + 1)   # 24-bit arrives left-justified in int32
    else:
        a = a.astype(np.float64)
    if a.ndim == 2:
        a = a.mean(axis=1)
    if sr != SR:
        g = np.gcd(sr, SR)
        a = resample_poly(a, SR // g, sr // g)
    return a

def hb_write(path, x):
    wavfile.write(path, SR, np.round(np.clip(x, -1, 1) * 32767).astype(np.int16))
    return path

def hb_seconds(path):
    with wave.open(path, "rb") as w:                          # header only; our files are 16-bit PCM
        return w.getnframes() / w.getframerate()

def hb_md5(path):
    return hashlib.md5(open(path, "rb").read()).hexdigest()

def hb_env(n, attack, decay):
    """Raised-cosine attack (never a click), then an exponential decay."""
    t = np.arange(n) / SR
    a = np.where(t < attack, 0.5 - 0.5 * np.cos(np.pi * np.clip(t / attack, 0, 1)), 1.0)
    return a * np.exp(-np.maximum(t - attack, 0.0) / decay)

def hb_fades(x, fade_in, fade_out):
    x = x.copy()
    i, o = int(fade_in * SR), min(len(x), int(fade_out * SR))
    if i > 0:
        x[:i] *= 0.5 - 0.5 * np.cos(np.linspace(0, np.pi, i))
    if o > 0:
        x[-o:] *= 0.5 + 0.5 * np.cos(np.linspace(0, np.pi, o))
    return x

def hb_normalise(x):
    p = np.abs(x).max()
    return x * (PEAK / p) if p > 0 else x

def hb_trim_tail(x, floor_db=-54.0, keep=0.02, max_len=0.7):
    """Drop the tail once it has died below floor_db, keep a short fade, never longer than max_len."""
    lvl = np.abs(x)
    above = np.nonzero(lvl > lvl.max() * 10 ** (floor_db / 20))[0]
    end = min(len(x), (above[-1] if len(above) else len(x)) + int(keep * SR), int(max_len * SR))
    return x[:end]


# ------------------------------------------------------------------ synthesis
def hb_thump(rng, shape, v, gain):
    f0, f1, tau_f, decay = shape
    n = int((decay * 7 + 0.02) * SR)                          # long enough to die below -60 dB
    t = np.arange(n) / SR
    f = f1 + (f0 - f1) * np.exp(-t / tau_f)
    ph = 2 * np.pi * np.cumsum(f) / SR                        # starts at phase 0: no step at the start
    body = np.sin(ph) * hb_env(n, 0.006, decay)
    body += v["knock"] * np.sin(ph * v["knock_ratio"]) * hb_env(n, 0.003, decay * 0.45)   # a higher partial (~150-250 Hz) so laptop speakers still carry it
    body = np.tanh(v["drive"] * body) / np.tanh(v["drive"])   # soft saturation: weight and warmth, no edge
    nz = sosfilt(butter(2, v["click_lp"], fs=SR, output="sos"), rng.standard_normal(n))
    nz *= hb_env(n, 0.002, 0.016)                             # the soft, felt start of the thump (valves), low-passed
    nz /= np.abs(nz).max() + 1e-9
    return gain * (body + v["click"] * nz)

def hb_synth_parts(v):
    """Lub and dub placed in one beat, unnormalised: (beat, lub alone, dub alone)."""
    rng = np.random.default_rng(v["seed"])
    lub = hb_thump(rng, v["lub"], v, 1.0)
    dub = hb_thump(rng, v["dub"], v, v["dub_gain"])
    lead, gi = int(0.004 * SR), int(v["gap"] * SR)            # a few ms of silence so no fade ever touches the attack
    n = max(lead + len(lub), lead + gi + len(dub))
    beat, lub_only, dub_only = np.zeros(n), np.zeros(n), np.zeros(n)
    lub_only[lead:lead + len(lub)] += lub
    dub_only[lead:lead + len(dub)] += dub
    beat[lead:lead + len(lub)] += lub
    beat[lead + gi:lead + gi + len(dub)] += dub
    if v["pressure"] > 0:                                     # death zone: a dull rush of blood that swells with the lub
        rush = sosfilt(butter(2, [30, 160], btype="band", fs=SR, output="sos"), rng.standard_normal(n))
        rush *= hb_env(n, 0.04, 0.07) * v["pressure"] / (np.abs(rush).max() + 1e-9)
        beat += rush
        lub_only += rush
    hp = butter(2, 25, btype="high", fs=SR, output="sos")     # nothing below 25 Hz: it only eats headroom
    lp = butter(2, v["muffle"], fs=SR, output="sos") if v["muffle"] else None
    out = []
    for x in (beat, lub_only, dub_only):
        x = sosfilt(hp, x)
        if lp is not None:
            x = sosfilt(lp, x)                                # muffled, as if heard through pressure in the ears
        out.append(x)
    return out

def hb_synth(halves=False):
    os.makedirs(OUT, exist_ok=True)
    for name, v in VARIANTS.items():
        beat, lub, dub = hb_synth_parts(v)
        beat = hb_fades(hb_trim_tail(beat), 0.0, 0.04)
        p = hb_write(os.path.join(OUT, f"synth_{name}.wav"), hb_normalise(beat))
        print("  wrote", os.path.relpath(p, ROOT))
        if halves:                                            # same scale for both, so the dub stays lighter than the lub
            os.makedirs(os.path.join(OUT, "halves"), exist_ok=True)
            lub, dub = hb_fades(hb_trim_tail(lub), 0.0, 0.04), hb_fades(hb_trim_tail(dub), 0.0, 0.04)
            s = PEAK / max(np.abs(lub).max(), np.abs(dub).max())
            for part, x in (("lub", lub), ("dub", dub)):
                p = hb_write(os.path.join(OUT, "halves", f"synth_{name}_{part}.wav"), x * s)
                print("  wrote", os.path.relpath(p, ROOT))


# ------------------------------------------------------------------ cutting single beats out of a recording
def hb_events(x):
    """Peaks of the low-band envelope = the heart sounds. Returns (envelope, peak sample indices)."""
    b = sosfiltfilt(butter(4, [20, 400], btype="band", fs=SR, output="sos"), x)
    env = np.maximum(sosfiltfilt(butter(2, 30, fs=SR, output="sos"), np.abs(b)), 0.0)
    ref = np.percentile(env, 99.5) + 1e-12
    pk, _ = find_peaks(env, height=0.12 * ref, distance=int(0.06 * SR), prominence=0.08 * ref)
    return env, pk

def hb_pair(env, pk):
    """Group the heart sounds into beats: [(lub peak, dub peak)], plus a line saying how."""
    if len(pk) < 3:
        return [], 0.0, "fewer than 3 heart sounds found"
    t, a = pk / SR, env[pk]
    d = np.diff(t)
    period = float(np.median(t[2:] - t[:-2]))                 # two sounds per beat
    e, o = float(np.median(d[0::2])), float(np.median(d[1::2]))
    # Below ~110 bpm the lub->dub gap (systole) is clearly the shorter one. Faster, the two gaps become alike, and the lub
    # is told by being the louder sound (true where these were recorded: the chest, the apex / mitral spot).
    if period > 0.55 and max(e, o) / max(min(e, o), 1e-6) > 1.25:
        par, how = (0 if e < o else 1), "timing"
    else:
        par, how = (0 if a[0::2].mean() >= a[1::2].mean() else 1), "loudness"
    systole = float(np.median(d[par::2]))
    pairs, k = [], par
    while k + 1 < len(pk):                                    # a sound that does not fit (noise, a missed dub) re-syncs
        g = d[k]
        ok = abs(g - systole) <= 0.35 * systole and 0.05 <= g <= 0.45
        if how == "loudness":
            ok = ok and a[k] >= 0.8 * a[k + 1]
        if ok:
            pairs.append((int(pk[k]), int(pk[k + 1])))
            k += 2
        else:
            k += 1
    return pairs, systole, f"{60 / period:.0f} bpm, lub told by {how}, lub->dub {systole * 1000:.0f} ms, {len(pairs)} beats"

def hb_cut(key, path, count=3):
    """Cut the `count` cleanest single beats of a recording into <key>_beat_N.wav."""
    os.makedirs(OUT, exist_ok=True)
    x = hb_read(path)
    xh = sosfiltfilt(butter(2, 20, btype="high", fs=SR, output="sos"), x)
    env, pk = hb_events(x)
    pairs, systole, how = hb_pair(env, pk)
    print(f"  {key}: {len(x) / SR:.1f} s, {how}")
    pk_list = [int(p) for p in pk]
    floor_all = float(np.percentile(env, 10)) + 1e-12
    beats = []
    for lub, dub in pairs:
        al, ad = env[lub], env[dub]
        prev = [p for p in pk_list if p < lub]
        nxt = [p for p in pk_list if p > dub]
        lo = prev[-1] + int(0.03 * SR) if prev else 0           # never reach back into the previous dub
        hi = nxt[0] - int(0.03 * SR) if nxt else len(x)         # nor into the next lub
        i = lub
        while i > max(lo, lub - int(0.08 * SR)) and env[i] > 0.08 * al:
            i -= 1
        start = max(lo, i - int(0.010 * SR))
        j, lim = dub, min(hi, dub + int(0.25 * SR), len(x))
        while j < lim - 1 and env[j] > 0.06 * ad:
            j += 1
        end = min(lim, j + int(0.02 * SR))
        if start < int(0.02 * SR) or end > len(x) - int(0.02 * SR) or end - start < int(0.12 * SR):
            continue                                            # cut off by the file's edge
        if np.abs(x[start:end]).max() >= 0.999:
            continue                                            # clipped in the recording
        before = env[max(0, start - int(0.15 * SR)):start]
        floor = float(np.median(before)) if len(before) > int(0.03 * SR) else floor_all
        snr = 20 * np.log10(al / max(floor, 1e-12))
        score = snr - 20 * abs((dub - lub) / SR - systole) / max(systole, 1e-6)   # clean, and a typical lub->dub
        beats.append((score, start, end, snr, (dub - lub) / SR))
    if not beats:
        print("  no clean beat found - cut it by hand")
        return []
    pick = sorted(sorted(beats, reverse=True)[:count], key=lambda b: b[1])
    for old in glob.glob(os.path.join(OUT, f"{key}_beat_*.wav")):   # a re-cut with fewer beats leaves no stale file
        os.remove(old)
    written = []
    for n, (score, start, end, snr, gap) in enumerate(pick, 1):
        seg = hb_normalise(hb_fades(xh[start:end], 0.003, 0.025))
        p = hb_write(os.path.join(OUT, f"{key}_beat_{n}.wav"), seg)
        written.append(p)
        print(f"    {os.path.basename(p)}: at {start / SR:.2f} s, {(end - start) / SR * 1000:.0f} ms, lub->dub {gap * 1000:.0f} ms, "
              f"{snr:.0f} dB above the noise")
    return written

def hb_downloads():
    """[(key, path)] for every audio file in Downloads/Heartbeat/<key>/ (archives and anything else are ignored)."""
    found = []
    for folder in sorted(glob.glob(os.path.join(DL, "*"))):
        if not os.path.isdir(folder):
            continue
        files = sorted(f for f in glob.glob(os.path.join(folder, "*")) if f.lower().endswith(AUDIO_EXT) and os.path.isfile(f))
        key = os.path.basename(folder)
        for f in files:
            stem = "".join(c if c.isalnum() else "_" for c in os.path.splitext(os.path.basename(f))[0])
            found.append((key if len(files) == 1 else f"{key}_{stem}", f))
    return found


# ------------------------------------------------------------------ measuring, previews
def hb_measure(path):
    """What a one-shot looks like on the waveform: length, peak, lub->dub gap, dub level, lub pitch, when it is over."""
    x = hb_read(path)
    env, pk = hb_events(x)
    m = {"length_ms": len(x) / SR * 1000, "peak_db": 20 * np.log10(np.abs(x).max() + 1e-12)}
    if len(pk):
        lub = int(pk[0])
        m["gap_ms"] = (pk[1] - pk[0]) / SR * 1000 if len(pk) > 1 else None
        m["dub_db"] = 20 * np.log10(env[pk[1]] / env[pk[0]]) if len(pk) > 1 else None
        m["sounds"] = len(pk)
        on = max(0, lub - int(0.02 * SR))
        seg = x[on:on + int(0.1 * SR)]
        spec = np.abs(np.fft.rfft(seg * np.hanning(len(seg)), SR))   # 1 Hz bins
        m["lub_hz"] = float(20 + np.argmax(spec[20:400]))
        over = np.nonzero(env > env.max() * 10 ** (-30 / 20))[0]
        m["over_ms"] = (over[-1] - on) / SR * 1000 if len(over) else None
    spec = np.abs(np.fft.rfft(x)) ** 2
    f = np.fft.rfftfreq(len(x), 1 / SR)
    m["above150"] = float(spec[f > 150].sum() / max(spec.sum(), 1e-12) * 100)   # roughly what a laptop speaker can play
    return m

def hb_note(m):
    words = []
    if m.get("lub_hz"):
        words.append("very low" if m["lub_hz"] < 50 else "low" if m["lub_hz"] < 80 else "mid-low" if m["lub_hz"] < 130 else "fairly high")
    g = m.get("gap_ms")
    if g:
        words.append("tight lub-dub" if g < 110 else "lub-dub ~0.1 s" if g < 160 else "wide lub-dub (real heart timing)"
                     if g < 400 else "dub far behind")
    if m.get("dub_db") is not None:
        words.append("dub nearly as strong" if m["dub_db"] > -2.5 else "dub lighter" if m["dub_db"] > -8 else "dub faint")
    if m.get("over_ms") and m["over_ms"] > 333:
        words.append("still sounding after 333 ms: overlaps itself at 180 bpm")
    if m["above150"] < 10:
        words.append("almost all below 150 Hz: felt on headphones, thin on laptop speakers")
    return ", ".join(words)

def hb_candidates():
    """The one-shots to judge: synth first, then the cut beats."""
    synth = [os.path.join(OUT, f"synth_{n}.wav") for n in VARIANTS if os.path.exists(os.path.join(OUT, f"synth_{n}.wav"))]
    cuts = sorted(glob.glob(os.path.join(OUT, "*_beat_*.wav")))
    return synth + cuts

def hb_preview_path(path, bpm):
    return os.path.join(OUT, f"preview_{os.path.splitext(os.path.basename(path))[0]}_{bpm}.wav")

def hb_previews():
    for path in hb_candidates():
        a = hb_read(path)
        for bpm in PREVIEW_BPM:
            n = 10 * SR
            out = np.zeros(n + len(a))
            t = 0.25
            while t < 9.9:                                    # exactly in time: the sample is judged, not the timing
                i = int(t * SR)
                out[i:i + len(a)] += a
                t += 60.0 / bpm
            out = hb_fades(out[:n], 0.0, 0.05)
            p = np.abs(out).max()
            if p > PEAK:                                      # overlapping tails at 150 bpm may add up
                out *= PEAK / p
            hb_write(hb_preview_path(path, bpm), out)
        print("  previews for", os.path.basename(path))


# ------------------------------------------------------------------ README and the double-click listener
def hb_source_of(name):
    for s in SOURCES:
        if name.startswith(s["key"] + "_beat_") or name.startswith(s["key"] + "_"):
            return s
    return None

def hb_about(path):
    name = os.path.splitext(os.path.basename(path))[0]
    if name.startswith("synth_"):
        v = VARIANTS.get(name[6:])
        return f"synthesised ({v['about']})" if v else "synthesised"
    s = hb_source_of(name)
    return f"cut from {s['title']} - {s['licence']}" if s else "cut from a downloaded recording"

def hb_readme():
    os.makedirs(OUT, exist_ok=True)
    have = {k for k, _ in hb_downloads()}
    L = ["# Heartbeat candidates", "",
         "One-shot heartbeats for the climber's body. **One file = one beat (\"lub-dub\")**: the game would play one per heart "
         "beat at the current heart rate (48-188 bpm, the same heart that drives the screen pulse). Nothing here is in the "
         "game yet. They are for you to judge by ear.", "",
         "## How to listen", "",
         "Double-click `listen_heartbeats.bat` in this folder. Each candidate plays 10 s at **70 bpm** (resting, a slow walk), "
         "then 10 s at **150 bpm** (a hard climb in thin air), then **one beat alone**.", "",
         "Keys: Enter = good, B = bad (type why), U = unsure, R = again, 1 / 2 / 3 = replay 70 bpm / 150 bpm / alone, "
         "N = note, P = previous, Q = save and quit. Your verdicts go to `Review.md` here. Start it with `--all` to hear the "
         "ones you already marked good. Or open any `preview_*.wav` in a player.", "",
         "## Candidates", "",
         "| File | Made from | Length | Peak | Lub -> dub | Dub vs lub | Lub pitch | Over by (-30 dB) | Above 150 Hz | How it looks |",
         "|---|---|---|---|---|---|---|---|---|---|"]
    cands = hb_candidates()
    for p in cands:
        m = hb_measure(p)
        f = lambda k, fmt: (fmt % m[k]) if m.get(k) is not None else "-"
        L.append(f"| `{os.path.basename(p)}` | {hb_about(p)} | {m['length_ms']:.0f} ms | {m['peak_db']:.1f} dBFS | "
                 f"{f('gap_ms', '%.0f ms')} | {f('dub_db', '%+.1f dB')} | {f('lub_hz', '~%.0f Hz')} | {f('over_ms', '%.0f ms')} | "
                 f"{m['above150']:.0f}% | {hb_note(m)} |")
    if not cands:
        L.append("| (none yet: run `python -I Tools/Audio/make_heartbeat.py all`) | | | | | | | | | |")
    L += ["", "Columns: *Lub -> dub* = time between the two thumps; *Over by* = when the beat has faded 30 dB below its peak "
          "(at 180 bpm the next beat starts at 333 ms); *Above 150 Hz* = share of the energy a laptop speaker can actually "
          "play (headphones get all of it). These are measurements only; the verdict is your ear.", "",
          "Previews: `preview_<file>_70.wav` and `preview_<file>_150.wav`, 10 s each, beats exactly in time.", "",
          "## Recordings (free, no login)", "",
          "Proposed for download (5, about 5.3 MB together) and two alternates, all CC0 / public domain (free for a "
          "commercial game, no credit needed). Files marked **not downloaded** wait for your go-ahead. Each goes into its "
          "own folder `Downloads/Heartbeat/<key>/`; then `python -I Tools/Audio/make_heartbeat.py all` cuts the 3 cleanest "
          "single beats out of each (`<key>_beat_1..3.wav`), makes their previews, adds them to the listener and rewrites "
          "this file.", "",
          "| Key | Recording | Licence | Size | Status | Notes |", "|---|---|---|---|---|---|"]
    for s in SOURCES:
        status = "downloaded" if any(k == s["key"] or k.startswith(s["key"] + "_") for k in have) else \
                 ("**not downloaded**" if s["pick"] else "alternate, not downloaded")
        L.append(f"| `{s['key']}` | [{s['title']}]({s['page']}) - {s['author']} | {s['licence']} | {s['size']} | {status} | {s['about']} |")
    L += ["", "Direct files (PowerShell, from any folder; Wikimedia may answer 429 when asked too fast - wait a minute):", "", "```powershell"]
    for s in SOURCES:
        folder = os.path.join(DL, s["key"]).replace("/", "\\")
        if s["url"]:
            L.append(f"# {s['key']} ({s['size']}){'' if s['pick'] else '  - alternate'}")
            L.append(f"New-Item -ItemType Directory -Force \"{folder}\" | Out-Null")
            L.append(f"curl.exe -L --fail -A \"YggdrasilsPeak-audio/1.0\" -o \"{folder}\\{s['file']}\" \"{s['url']}\"")
        else:
            L.append(f"# {s['key']}: save it from the page in a browser ({s['page']}, format menu -> FLAC) into {folder}")
    L += ["```", "",
          "## Format and how they were made", "",
          "- 48 kHz, 16-bit, mono WAV, peak -3 dBFS, no reverb. Cut beats: 20 Hz high-pass, 3 ms fade-in, 25 ms fade-out, "
          "taken from the start of the lub to the end of the dub, the cleanest 3 of each recording (least noise before the "
          "beat, a typical lub->dub gap, not clipped).",
          "- Synth: each thump is a 40-130 Hz body whose pitch falls fast, a higher partial and soft saturation (so laptop "
          "speakers still carry it), and a low-passed noise touch at the start; the dub is shorter, higher and lighter. "
          "The death-zone one adds a low rush of blood and is low-passed at 900 Hz (muffled).",
          "- `halves/synth_<name>_lub.wav` and `_dub.wav` (made by `synth --halves`, same level ratio as the full beat): "
          "the game could fire them separately at the moments the screen pulse already uses (ExpeditionComponent: lub at 0.06, "
          "dub at 0.26 of the beat), so the lub->dub gap would follow the heart rate like a real heart and the visual pulse.",
          "- A real heart's lub->dub gap is ~300 ms at rest and ~200 ms near 180 bpm. A beat cut from a resting recording "
          "keeps its ~300 ms gap, so played at 150-180 bpm its dub lands almost on the next lub; that is why the synth ones "
          "use a fixed 95-135 ms gap (or the halves above).", "",
          "Rebuild everything: `python -I Tools/Audio/make_heartbeat.py all` (Tools/Audio/make_heartbeat.py).", ""]
    open(os.path.join(OUT, "README.md"), "w", encoding="utf8").write("\n".join(L))
    bat = ["@echo off",
           "rem Listen to the heartbeat candidates and rate them by ear (Tools/Audio/make_heartbeat.py listen). Double-click to start.",
           "chcp 65001 >nul",
           "title Yggdrasil's Peak - heartbeat check",
           "python -I \"%~dp0..\\..\\..\\Tools\\Audio\\make_heartbeat.py\" listen %*",
           "if errorlevel 1 pause", ""]
    open(os.path.join(OUT, "listen_heartbeats.bat"), "w", encoding="ascii", newline="\r\n").write("\n".join(bat))
    print("  wrote", os.path.relpath(os.path.join(OUT, "README.md"), ROOT), "and listen_heartbeats.bat")

def hb_wait(seconds):
    import msvcrt
    t0 = time.time()
    while seconds is None or time.time() - t0 < seconds:
        if msvcrt.kbhit():
            k = msvcrt.getwch()
            if k in ("\x00", "\xe0"):
                msvcrt.getwch()
                continue
            return k.lower()
        time.sleep(0.02)
    return None

def hb_ask(text):
    import winsound
    winsound.PlaySound(None, 0)
    try:
        return input(text).strip()
    except EOFError:
        return ""

def hb_save_review(rv):
    json.dump(rv, open(REVIEW_JSON, "w", encoding="utf8"), indent=1, ensure_ascii=False)
    L = ["# Heartbeat review (by ear)", "", "Written by `make_heartbeat.py listen`. good / bad / unsure + your notes.", "",
         "| Candidate | Verdict | Note |", "|---|---|---|"]
    for name, r in rv.items():
        L.append(f"| {name} | {r.get('verdict', '')} | {r.get('note', '').replace('|', '/')} |")
    open(REVIEW_MD, "w", encoding="utf8").write("\n".join(L) + "\n")

def hb_rate(rv, name, steps, h):
    """steps: [(wav, label)] played in turn; any key stops the sound. Returns 'next', 'p' or 'q'."""
    import winsound
    queue = steps
    while True:
        k = None
        for path, label in queue:
            print(f"      > {label}")
            winsound.PlaySound(path, winsound.SND_FILENAME | winsound.SND_ASYNC)
            k = hb_wait(hb_seconds(path) + 0.3)
            if k:
                break
        winsound.PlaySound(None, 0)
        if k is None:
            k = hb_wait(None)
        queue = []
        if k in ("\r", "g"):
            rv[name] = {**rv.get(name, {}), "verdict": "good", "hash": h}
            return "next"
        if k == "b":                                          # keep a note typed with N before
            why, before = hb_ask("      what is wrong? > "), rv.get(name, {}).get("note", "")
            rv[name] = {"verdict": "bad", "note": "; ".join(n for n in (before, why) if n), "hash": h}
            return "next"
        if k == "u":
            rv[name] = {**rv.get(name, {}), "verdict": "unsure", "hash": h}
            return "next"
        if k == "n":
            rv[name] = {**rv.get(name, {}), "note": hb_ask("      note > ")}
        elif k == "r":
            queue = steps
        elif k in ("1", "2", "3") and int(k) <= len(steps):
            queue = [steps[int(k) - 1]]
        elif k in ("p", "q"):
            return k

def hb_listen(everything):
    try:
        rv = json.load(open(REVIEW_JSON, encoding="utf8"))
    except Exception:
        rv = {}
    items = hb_candidates()
    if not items:
        print("No candidates yet. Run: python -I Tools/Audio/make_heartbeat.py all")
        hb_ask("Press Enter to close.")
        return
    if any(not os.path.exists(hb_preview_path(p, b)) for p in items for b in PREVIEW_BPM):
        hb_previews()
    if not everything:
        items = [p for p in items if not (rv.get(os.path.basename(p), {}).get("verdict") == "good"
                                          and rv[os.path.basename(p)].get("hash") == hb_md5(p))]
    print("YGGDRASIL'S PEAK - HEARTBEAT CHECK")
    print("Each candidate: 10 s at 70 bpm, 10 s at 150 bpm, then one beat alone.")
    print("Keys: Enter=good  B=bad  U=unsure  R=again  1/2/3=70 bpm/150 bpm/alone  N=note  P=previous  Q=save+quit")
    if not everything:
        print("Only new or changed candidates (and ones not yet marked good). Start with --all to hear everything.")
    i, action = 0, None
    while i < len(items):
        p = items[i]
        name = os.path.basename(p)
        done = f"  [{rv[name]['verdict']}]" if rv.get(name, {}).get("verdict") else ""
        print(f"\n  {i + 1}/{len(items)}  {name}{done}\n      {hb_about(p)}")
        steps = [(hb_preview_path(p, 70), "70 bpm (resting, a slow walk)"),
                 (hb_preview_path(p, 150), "150 bpm (a hard climb, thin air)"), (p, "one beat alone")]
        action = hb_rate(rv, name, steps, hb_md5(p))
        hb_save_review(rv)
        if action == "next":
            i += 1
        elif action == "p":
            i = max(0, i - 1)
        elif action == "q":
            break
    hb_save_review(rv)
    print(f"\nSaved: {REVIEW_MD}\nTell Claude when you are done; it reads that file.")
    hb_ask("Press Enter to close.")


def main():
    args = [a for a in sys.argv[1:] if not a.startswith("--")]
    cmd = args[0] if args else "all"
    if cmd == "synth":
        hb_synth("--halves" in sys.argv)
    elif cmd == "cut":
        if len(args) < 3:
            raise SystemExit("usage: make_heartbeat.py cut <source_name> <file> [count]")
        hb_cut(args[1], args[2], int(args[3]) if len(args) > 3 else 3)
    elif cmd == "previews":
        hb_previews()
    elif cmd == "readme":
        hb_readme()
    elif cmd == "info":
        for f in args[1:]:
            m = hb_measure(f)
            print(os.path.basename(f), {k: (round(v, 1) if isinstance(v, float) else v) for k, v in m.items()}, "-", hb_note(m))
    elif cmd == "listen":
        hb_listen("--all" in sys.argv)
    elif cmd == "all":
        hb_synth("--halves" in sys.argv)
        for key, path in hb_downloads():
            hb_cut(key, path)
        hb_previews()
        hb_readme()
    else:
        raise SystemExit(__doc__ or "unknown command: " + cmd)

if __name__ == "__main__":
    main()
