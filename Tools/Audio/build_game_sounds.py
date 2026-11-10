# Turns the user's sound recordings (/Game/Game_Design/sound_effect, exported to Saved/AudioWork/raw by
# Tools/Audio/export_sounds.py inside the editor) into the pieces the game plays:
#   single footsteps / cracks / clicks / ticks cut at their onsets (tight start, tail ending at the recording's own
#   noise floor or before a second event), whole breath cycles, seamless click-free loops for the wind beds and the
#   rope slide, trimmed and faded thunder and avalanches. 48 kHz, 16-bit WAV, into Saved/AudioWork/out/<Folder>/<Name>.wav
#   plus out/manifest.json (folder, name, loop, seconds). Check with Tools/Audio/inspect_sounds.py, import with
#   Tools/Audio/import_game_sounds.py (in the editor).
#   python -I Tools/Audio/build_game_sounds.py
import os, json, glob, shutil
import numpy as np
from scipy.io import wavfile
from scipy.signal import resample_poly, butter, sosfiltfilt, find_peaks
from scipy.ndimage import median_filter

ROOT = "F:/UE_project/YggdrasilsPeak/Saved/AudioWork"
RAW, OUT = f"{ROOT}/raw", f"{ROOT}/out"
SR = 48000
manifest = []


# ---------------------------------------------------------------- basics
def load(rel):
    path = glob.glob(f"{RAW}/{rel}")[0]
    sr, a = wavfile.read(path)
    if a.dtype.kind == "i":
        a = a.astype(np.float32) / np.iinfo(a.dtype).max
    a = a.astype(np.float32)
    if a.ndim == 1:
        a = a[:, None]
    if sr != SR:
        g = np.gcd(sr, SR)
        a = resample_poly(a, SR // g, sr // g, axis=0).astype(np.float32)
    return a                                   # (n, ch)

def mono(a):
    return a.mean(axis=1, keepdims=True)

def filt(a, kind, hz, order=4):
    sos = butter(order, hz, btype=kind, fs=SR, output="sos")
    return sosfiltfilt(sos, a, axis=0).astype(np.float32)

def fade(a, fin=0.005, fout=0.05):
    a = a.copy(); n = len(a)
    i, o = min(int(fin * SR), n // 2), min(int(fout * SR), n // 2)
    if i: a[:i] *= np.linspace(0, 1, i)[:, None] ** 2
    if o: a[-o:] *= (np.linspace(1, 0, o)[:, None]) ** 2
    return a

def peak_norm(a, db=-1.0):
    p = np.abs(a).max()
    return a * (10 ** (db / 20) / p) if p > 0 else a

def rms(a):
    return float(np.sqrt((a ** 2).mean()) + 1e-12)

def rms_norm(a, db, ceiling=-1.0):
    a = a * (10 ** (db / 20) / rms(a))
    p = np.abs(a).max(); c = 10 ** (ceiling / 20)
    return a * (c / p) if p > c else a        # never clip: loud transients pull the whole sound down

def sec(a, t0, t1=None):
    return a[int(t0 * SR): None if t1 is None else int(t1 * SR)]

def envelope_db(m, hop_s=0.005):
    hop = max(1, int(SR * hop_s)); n = len(m) // hop
    e = np.sqrt((m[:n * hop].reshape(n, hop) ** 2).mean(axis=1) + 1e-12)
    return 20 * np.log10(e), hop

def noise_floor_db(a):
    """The recording's own quiet level (between events), 20 ms RMS."""
    e, _ = envelope_db(mono(a)[:, 0], 0.02)
    return float(np.percentile(e, 8))

def save(folder, name, a, loop=False):
    os.makedirs(f"{OUT}/{folder}", exist_ok=True)
    a = np.clip(a, -1, 1)
    wavfile.write(f"{OUT}/{folder}/{name}.wav", SR, (a * 32767).astype(np.int16) if a.shape[1] > 1 else (a[:, 0] * 32767).astype(np.int16))
    manifest.append({"folder": folder, "name": name, "loop": loop, "seconds": round(len(a) / SR, 2), "channels": int(a.shape[1])})


# ---------------------------------------------------------------- cutting
def onsets(a, min_gap=0.25, rise_db=9.0, floor_rel=-30.0):
    """Onset times (s): the envelope jumps rise_db within 30 ms and is within floor_rel of the loudest point."""
    e, hop = envelope_db(mono(a)[:, 0])
    k = max(1, int(0.03 * SR / hop))
    jump = e - np.concatenate([np.full(k, e[0]), e[:-k]])
    loud = e > e.max() + floor_rel
    pk, _ = find_peaks(np.where(loud, jump, -99), height=rise_db, distance=int(min_gap * SR / hop))
    out = []
    for p in pk:
        q = p
        while q > 0 and e[q - 1] < e[q] and p - q < k * 2:
            q -= 1
        out.append(q * hop / SR)
    return out

def tighten(c, floor_db, pre=0.003, start_rel=-24.0, second_rise=10.0, tail_over_floor=4.0, tail_rel=-42.0):
    """Trim one hit to the sound itself: start a few ms before it reaches (peak + start_rel); end where it falls to the
    recording's noise floor (or tail_rel under the peak), or at the trough before a second event rises again."""
    e, hop = envelope_db(mono(c)[:, 0], 0.002)
    top = int(np.argmax(e)); peak = e[top]
    first = int(np.argmax(e > peak + start_rel))
    s = max(0, first * hop - int(pre * SR))
    end_level = max(floor_db + tail_over_floor, peak + tail_rel)
    end = len(e)
    trough, trough_i = peak, top
    for i in range(top, len(e)):
        if e[i] < trough:
            trough, trough_i = e[i], i
        if e[i] <= end_level:
            end = i; break
        if trough < peak - 15 and e[i] > trough + second_rise:     # a second event after the first decayed
            end = trough_i; break
    return c[s: end * hop + int(0.004 * SR)], peak, e

def attack_ok(c, rise=15.0):
    """A real hit climbs at least `rise` dB within its first 30 ms (fragments and tails do not)."""
    e, _ = envelope_db(mono(c)[:, 0], 0.002)
    w = e[: max(2, int(0.03 * SR / int(SR * 0.002)))]
    return w.max() - w[0] >= rise

def peak_early(c, within=0.12):
    """A step / pull is loudest right at its start; a clip whose peak comes late began on some other small sound."""
    e, hop = envelope_db(mono(c)[:, 0], 0.005)
    return int(np.argmax(e)) * hop / SR <= within

def cut_hits(a, starts, max_len, min_len=0.05, next_guard=0.012, attack=15.0, **kw):
    """One clip per onset, tightened to the hit; returns (clean_score, clip) where clean = how far it decays."""
    floor = noise_floor_db(a)
    clips = []
    for i, t in enumerate(starts):
        t0 = max(0.0, t - 0.02)
        t_next = starts[i + 1] - next_guard if i + 1 < len(starts) else len(a) / SR
        c = sec(a, t0, min(t0 + max_len, t_next))
        if len(c) < int(0.04 * SR):
            continue
        c, peak, _ = tighten(c, floor, **kw)
        if len(c) < min_len * SR or not attack_ok(c, attack):
            continue
        tail = mono(c)[-int(0.01 * SR):, 0]
        clean = peak - 20 * np.log10(np.sqrt((tail ** 2).mean()) + 1e-12)
        clips.append((clean, c))
    return clips

def best(clips, n, min_clean=10.0):
    """Keep the n cleanest (well decayed) hits."""
    return [c for s, c in sorted([x for x in clips if x[0] >= min_clean], key=lambda x: -x[0])[:n]]


# ---------------------------------------------------------------- loops
def hf_clicks(a, rel=10.0, max_ms=40):
    """Short sharp events (knocks, mic handling, ticks): >5 kHz energy rel dB over its 1 s running median."""
    h = filt(mono(a), "highpass", 5000)[:, 0]
    e, hop = envelope_db(h, 0.005)
    sp = e - median_filter(e, size=201)
    out, i = [], 0
    while i < len(e):
        if sp[i] > rel:
            j = i
            while j < len(e) and sp[j] > rel - 6: j += 1
            if (j - i) * 5 <= max_ms * 2: out.append((i * hop, j * hop))
            i = j
        else:
            i += 1
    return out

def declick(a):
    """Patch each short click with the audio just before it (crossfaded), so it cannot repeat every lap."""
    a = a.copy()
    for s, e in hf_clicks(a):
        pad = int(0.01 * SR); s0, e0 = max(0, s - pad), min(len(a), e + pad); L = e0 - s0
        src = a[max(0, s0 - L): max(0, s0 - L) + L]
        if len(src) < L: continue
        w = np.hanning(L)[:, None]
        a[s0:e0] = a[s0:e0] * (1 - w) + src * w
    return a

def cleanest_window(a, length, step=1.0):
    """Start time of the window with the fewest sharp events and the steadiest loudness."""
    clicks = np.array([s / SR for s, _ in hf_clicks(a, rel=8.0, max_ms=200)])
    e, _ = envelope_db(mono(a)[:, 0], 0.1)
    w = int(length / 0.1); st = int(step / 0.1)
    best_t, best_v = 0.0, 1e9
    for i in range(0, max(1, len(e) - w), st):
        t0 = i * 0.1
        seg = e[i:i + w]
        v = (np.percentile(seg, 95) - np.percentile(seg, 20)) + 3.0 * int(((clicks >= t0) & (clicks < t0 + length)).sum())
        if v < best_v: best_t, best_v = t0, v
    return best_t

def make_loop(a, length, xfade):
    """Seamless loop of `length` s: the xfade s after the loop end are equal-power blended into its start."""
    L, X = int(length * SR), int(xfade * SR)
    seg = a[: L + X].copy()
    t = np.linspace(0, np.pi / 2, X)[:, None]
    head = seg[:X] * np.sin(t) + seg[L:L + X] * np.cos(t)
    out = seg[:L].copy(); out[:X] = head
    return out


# ---------------------------------------------------------------- the game's sounds
def blobs(a, rel_db=-50.0, min_gap=0.12, min_len=0.25, max_len=1.2):
    """Whole sound events between silences: (t0, t1) runs where the 10 ms level is within rel_db of the loudest,
    gaps shorter than min_gap joined. Used where one event has several parts (a step: foot down, roll, lift off)."""
    e, hop = envelope_db(mono(a)[:, 0], 0.01)
    on = e > e.max() + rel_db
    runs, i = [], 0
    while i < len(on):
        if on[i]:
            j = i
            while j < len(on) and (on[j] or on[j:j + int(min_gap / 0.01)].any()):
                j += 1
            if min_len <= (j - i) * 0.01 <= max_len:
                runs.append((i * 0.01, j * 0.01))
            i = j
        else:
            i += 1
    return runs

def footsteps():
    # Snow, walking (user review 2026-11-11: the old clips kept the crunch but lost the foot leaving the snow):
    # each clip is now the WHOLE step, foot down -> roll -> lift off, up to the silence before the next step.
    # Only the Nox_Sound boots recording: its steps stand apart; the second recording's run together over hiss.
    a = filt(mono(load("footsteps/613849*.wav")), "highpass", 70)
    for i, (t0, t1) in enumerate(blobs(a, rel_db=-50, min_gap=0.12, min_len=0.3, max_len=1.0), 1):
        save("Footsteps", "Step_Snow_%02d" % i, rms_norm(fade(sec(a, max(0, t0 - 0.01), t1 + 0.04), 0.002, 0.06), -20))
    # Crampons on hard ice: the points biting (several spikes in one step is right), tail to the noise floor
    a = filt(mono(load("crampon/*.wav")), "highpass", 90)
    for i, c in enumerate(best(cut_hits(a, onsets(a, min_gap=0.3), 0.32, min_len=0.12), 14), 1):
        save("Footsteps", "Step_Ice_%02d" % i, rms_norm(fade(c, 0.002, 0.04), -19))

def ice():
    # Sharp cracks (ice_break), deep cracks (Glacier_Ice_Breaking 3-5), groans (slowed glacier breaking), collapses
    n = 0
    for rel in ("ice_break/ice_break_2.wav", "ice_break/ice_break_1.wav"):
        a = mono(load(rel))
        for c in best(cut_hits(a, onsets(a, min_gap=0.35, rise_db=8), 1.6, min_len=0.08, tail_rel=-36), 5, min_clean=8):
            n += 1
            gain = -6.0 if n == 1 else 0.0                    # user review: Ice_Crack_01 too loud (5-6 dB over the others)
            save("Ice", "Ice_Crack_%02d" % n, peak_norm(fade(c, 0.001, 0.08), -2 + gain))
    for rel in ("glacier/Glacier_Ice_Breaking_3.wav", "glacier/Glacier_Ice_Breaking_4.wav", "glacier/Glacier_Ice_Breaking_5.wav"):
        n += 1; save("Ice", "Ice_Crack_%02d" % n, peak_norm(fade(mono(load(rel)), 0.002, 0.6), -1.5))
    g = 0
    for rel, spans in (("glacier/Glacier_Ice_Breaking.wav", ((0.0, 4.5), (4.5, 9.0))), ("glacier/Glacier_Ice_Breaking_2.wav", ((0.0, 4.0), (4.0, 8.0)))):
        a = mono(load(rel))
        for t0, t1 in spans:
            slow = resample_poly(sec(a, t0, t1), 5, 3, axis=0).astype(np.float32)   # played 0.6x: deeper, slower
            g += 1; save("Ice", "Ice_Groan_%02d" % g, peak_norm(fade(filt(slow, "lowpass", 2500), 0.08, 0.8), -3))
    save("Ice", "Serac_Collapse_01", peak_norm(fade(mono(load("glacier/Glacier_Ice_Breaking.wav")), 0.002, 2.0), -1))
    save("Ice", "Serac_Collapse_02", peak_norm(fade(mono(load("glacier/Glacier_Ice_Breaking_2.wav")), 0.002, 2.0), -1))
    # (Serac_Collapse_03 from snow_avalanche_4 removed: user review "sounds like wind, not suitable")
    # Distant avalanches: a long roar far away (air takes the highs), 26 s; the slow swell in is intended
    for i, (rel, t0) in enumerate((("snow_avalanche/snow_avalanche_3*.wav", 7.0), ("snow_avalanche/snow_avalanche_5.wav", 1.0),
                                   ("snow_avalanche/snow_avalanche_1.wav", 0.0), ("snow_avalanche/snow_avalanche_2.wav", 0.0),
                                   ("snow_avalanche/snow_avalanche_4.wav", 1.0)), 1):
        a = filt(sec(load(rel), t0, t0 + 26.0), "lowpass", 3200)
        save("Ice", "Avalanche_%02d" % i, rms_norm(fade(a, 2.0, 7.0), -22))

def weather():
    # Thunder: the four without rain on concrete, each starting at its strike (the game times it after the flash)
    for i, (rel, t1_len) in enumerate((("thunder/840628*.wav", 13.5), ("thunder/712962*.wav", 10.5),
                                       ("thunder/399656*.wav", 17.0), ("thunder/194364*.wav", 26.0)), 1):
        a = load(rel)
        t = [x for x in onsets(a, min_gap=1.0, rise_db=10, floor_rel=-8)][0]       # the main strike, not the lead-in
        save("Weather", "Thunder_%02d" % i, peak_norm(fade(sec(a, max(0, t - 0.03), t + t1_len), 0.01, 4.0), -1))
        print("  Thunder_%02d strike at %.1f s" % (i, t))
    # Wind beds: seamless loops from the cleanest stretch (no knocks / handling), clicks patched, one loudness
    beds = (("Wind_Calm", "wind/488360*.wav", 90.0, 35), ("Wind_Gusty", "wind/760726*.wav", 84.0, 35),
            ("Wind_Blizzard", "wind/558664*.wav", 120.0, 35), ("Wind_Whistle", "wind/156414*.wav", 75.0, 120))
    for name, rel, length, hp in beds:
        a = filt(load(rel), "highpass", hp)
        t0 = 0.0 if name == "Wind_Gusty" else cleanest_window(a, length + 4.0)
        seg = declick(sec(a, t0, t0 + length + 4.0))
        save("Weather", name, rms_norm(make_loop(seg, length, 4.0), -20 if name != "Wind_Whistle" else -24), loop=True)
        print("  %s from %.0f s, %d clicks patched" % (name, t0, len(hf_clicks(sec(a, t0, t0 + length + 4.0)))))
    save("Weather", "Wind_Gust_01", peak_norm(fade(load("wind/636085*.wav"), 0.2, 3.0), -3))

def body():
    # Breathing (user review 2026-11-11: single breaths with gaps between "is not how people breathe"):
    # keep the recording's own phrases WHOLE -- six stretches of real breathing (4-8 s, several breaths each, natural
    # rhythm), cut only at the silences between them. Sorted soft -> hard; one gain for all so their natural loudness
    # differences stay. The game plays them back to back, harder phrases with more effort.
    a = filt(mono(load("breath/*.wav")), "highpass", 80)
    e, hop = envelope_db(a[:, 0], 0.05)
    quiet = e < np.percentile(e, 5) + 10
    phrases, start, run = [], None, 0
    for i, q in enumerate(quiet):
        if not q:
            if start is None: start = i
            run = 0
        else:
            run += 1
            if start is not None and run >= 12:                   # 0.6 s of silence ends a phrase
                phrases.append((start * 0.05, (i - run + 1) * 0.05)); start = None
    if start is not None:
        phrases.append((start * 0.05, len(e) * 0.05))
    clips = [fade(sec(a, max(0, t0 - 0.05), t1 + 0.15), 0.03, 0.25) for t0, t1 in phrases if t1 - t0 >= 2.0]
    clips.sort(key=rms)
    # user review round 2: by ear the loudest phrase is phase 5 and the one before it is phase 6 (the player cannot get
    # enough oxygen, utterly exhausted) -- loudness is not exhaustion, so the last two swap
    if len(clips) >= 6:
        clips[4], clips[5] = clips[5], clips[4]
    g = 10 ** (-2 / 20) / max(np.abs(c).max() for c in clips)
    for i, c in enumerate(clips, 1):
        save("Body", "Breath_Phrase_%02d" % i, c * g)
    print("  breath phrases", ["%.1fs" % (len(c) / SR) for c in clips])

def gear():
    a = filt(mono(load("Carabiner_clip/Timeline_1.wav")), "highpass", 150)
    for i, c in enumerate(best(cut_hits(a, onsets(a, min_gap=0.2), 0.45, min_len=0.03, second_rise=30), 6, min_clean=8), 1):
        save("Gear", "Carabiner_Click_%02d" % i, peak_norm(fade(c, 0.001, 0.02), -3))   # click-clack kept whole
    # Whole clip gestures (gate opens, clips, closes, rattles): Timeline_1 holds four, between silences. User review:
    # the clip-in made of one click + a rope pull was not right; these are the candidates for the real clip-in.
    for i, (t0, t1) in enumerate(blobs(a, rel_db=-45, min_gap=0.2, min_len=0.15, max_len=2.0), 1):
        save("Gear", "Carabiner_ClipIn_%02d" % i, peak_norm(fade(sec(a, max(0, t0 - 0.01), t1 + 0.05), 0.002, 0.05), -3))
    a = mono(load("Carabiner_clip/rope_tighten_sequence_1.wav"))
    for i, c in enumerate(best([x for x in cut_hits(a, onsets(a, min_gap=0.4, rise_db=7), 1.0, min_len=0.12, attack=6) if peak_early(x[1], 0.2)], 6, min_clean=8), 1):
        if i == 2:
            continue                                           # user review: Rope_Tighten_02 "wrong"
        save("Gear", "Rope_Tighten_%02d" % i, peak_norm(fade(c, 0.002, 0.08), -3))
    a = mono(load("Carabiner_clip/rope_silding_1.wav"))
    t0 = cleanest_window(sec(a, 0.9, 4.9), 3.6, step=0.1) + 0.9
    save("Gear", "Rope_Slide_Loop", rms_norm(make_loop(declick(sec(a, t0, t0 + 4.2)), 3.4, 0.8), -24), loop=True)
    save("Gear", "Rope_Slide_Fast", peak_norm(fade(mono(load("Carabiner_clip/rope_silding_fast_1.wav")), 0.005, 0.3), -4))
    save("Gear", "Rope_Slide_Slow", peak_norm(fade(mono(load("Carabiner_clip/rope_silding_slow_1.wav")), 0.005, 0.3), -4))

def ui():
    a = mono(load("wheel_spin/*.wav"))
    clips = best(cut_hits(a, onsets(a, min_gap=0.08, rise_db=8), 0.12, min_len=0.03, pre=0.001), 6, min_clean=6)
    for i, c in enumerate(clips, 1):
        save("UI", "Wheel_Tick_%02d" % i, peak_norm(fade(c, 0.0005, 0.015), -6))
    print("  ticks", len(clips))


if __name__ == "__main__":
    if os.path.isdir(OUT):
        shutil.rmtree(OUT)                     # only our own generated output folder
    for step in (footsteps, ice, weather, body, gear, ui):
        print(step.__name__); step()
    json.dump(manifest, open(f"{OUT}/manifest.json", "w"), indent=1)
    from collections import Counter
    print(Counter(m["folder"] for m in manifest))
