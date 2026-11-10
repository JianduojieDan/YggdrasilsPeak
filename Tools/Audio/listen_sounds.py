# Listen to every sound the game uses, one by one, and judge it by ear. Double-click Tools/Audio/listen_sounds.bat.
#
# For each group it first plays a short DEMO the way the game uses it (a walk on snow, breathing getting harder, the
# wheel spinning and landing, clipping into the rope...), then every sound on its own. Loops play continuously (their
# seam comes after ~4 s and every lap) until you press a key. Your verdicts go to Docs/Development/Sound_Review.md
# (and Saved/AudioWork/review.json, so you can stop and continue later).
#
# Keys:  Enter / G = good, next    B = bad (type what is wrong, or just Enter)    U = unsure
#        R = play again            N = add a note    P = previous    D = demo again    S = skip the rest of this group
#        Q = save and quit
# Plays the exact files imported into the game (Saved/AudioWork/out), at their own level (not the in-game mix volume).
import os, sys, json, time, random, msvcrt, winsound, hashlib
import numpy as np
from scipy.io import wavfile

ROOT = "F:/UE_project/YggdrasilsPeak"
OUT = f"{ROOT}/Saved/AudioWork/out"
TMP = f"{ROOT}/Saved/AudioWork/listen_tmp"
REVIEW_JSON = f"{ROOT}/Saved/AudioWork/review.json"
REVIEW_MD = f"{ROOT}/Docs/Development/Sound_Review.md"
SR = 48000
os.makedirs(TMP, exist_ok=True)

# Game moment, where it plays, what it was made from -- in the order you would meet them on the mountain
GROUPS = [
    ("Step_Snow", "Walking on snow", "every footstep on snow (and the steps that follow you)", "boots-in-snow recordings, one crunch each"),
    ("Step_Ice", "Walking on ice / hard crust", "every footstep on bare ice, crust and serac sides", "crampons recording, one step each"),
    ("Breath_Phrase", "Breathing", "whole phrases of real breathing back to back; harder phrases and louder with effort", "breath sequence: its 6 natural stretches kept whole, soft to hard"),
    ("Wind_Calm", "Wind: calm air (loop)", "always under everything, strongest in calm weather", "natural wind recording, a clean 90 s stretch"),
    ("Wind_Gusty", "Wind: gusty (loop)", "moderate to strong wind", "mountain wind with gusts"),
    ("Wind_Blizzard", "Wind: blizzard (loop)", "gales and storms (Jotunheim)", "blizzard recording, a clean 2 min stretch"),
    ("Wind_Whistle", "Wind: high whistle (loop)", "only high up (5,600 m+) in strong wind", "wind in cactus spines on a mountain top"),
    ("Wind_Gust_", "Single gust", "now and then on top of strong wind", "ice-spell blizzard blast"),
    ("Thunder", "Thunder", "a few seconds after a lightning flash in the Jotunheim storm", "the 4 thunders without rain, each starting at the strike"),
    ("Avalanche", "Distant avalanche", "every 4-10 minutes above 4,900 m, from a direction", "the 5 avalanches, far-away filtered"),
    ("Ice_Crack", "Ice cracking", "warning cracks around you in serac zones", "ice_break_2 single snaps + deep glacier cracks"),
    ("Ice_Groan", "Ice groaning", "the serac zone straining before a collapse", "glacier breaking, slowed to 0.6x"),
    ("Serac_Collapse", "Serac collapse", "a tower of ice falls", "glacier breaking + close avalanche"),
    ("Carabiner_Click", "Carabiner click", "unclipping from the fixed rope", "Timeline_1"),
    ("Carabiner_ClipIn", "Clip-in - CHOOSE", "clipping into the fixed rope (a whole carabiner gesture, then the rope pulls tight). Mark GOOD the ones that sound right; the others are removed", "Timeline_1, whole gestures"),
    ("Rope_Tighten", "Rope pulls tight", "just after you clip in", "rope tighten sequence"),
    ("Rope_Slide", "Rope sliding", "moving while clipped (the loop), plus fast / slow slides", "rope sliding recordings"),
    ("Wheel_Tick", "Weather wheel ticks", "one tick each time a divider passes the pointer", "wheel spin recording"),
    ("Realm_Landing", "Wheel landing per weather (by danger)", "the wheel spins and lands on each weather in turn, harmless -> risky -> deadly; the more dangerous, the more intense the landing. Judge each pairing", "your approved landings C, E, H, K, L, M; mapping in Config/RealmLooks.json \"Landing\""),
]

# The nine weathers as the player reads them (RealmSky.cpp GInfo): realm label, name, danger 0 harmless / 1 risky / 2 deadly
REALMS = [("Midgard", "QUIET SKY", 0), ("Niflheim", "VEIL OF MIST", 1), ("Muspelheim", "EMBERFALL", 1), ("Jotunheim", "WINTER'S VEIL", 2),
          ("Alfheim", "PEARL LIGHT", 0), ("Vanaheim", "JADE TIDE", 0), ("Svartalfheim", "SILVER ECLIPSE", 2), ("Hel", "STARFALL FROST", 1),
          ("Asgard", "GOLDEN HUSH", 0)]
DANGER = ("harmless", "risky", "deadly")

def realm_items():
    looks = json.load(open(f"{ROOT}/Config/RealmLooks.json", encoding="utf8"))
    items = []
    for label, name, danger in sorted(REALMS, key=lambda r: r[2]):
        snd = looks.get(label, {}).get("Landing", "UI_Chime_01")
        items.append({"name": f"Landing:{label}", "folder": "UI", "sound": snd, "loop": False, "seconds": 0,
                      "title": f"{name} ({DANGER[danger]}) -> {snd.replace('UI_Chime_', '')}"})
    return items

def item_file(m):
    return f"{OUT}/{m['folder']}/{m.get('sound', m['name'])}.wav"

def item_hash(m):
    h = hashlib.md5(open(item_file(m), "rb").read()).hexdigest()
    return hashlib.md5((m["sound"] + h).encode()).hexdigest() if "sound" in m else h


# ------------------------------------------------------------------ audio helpers
def read(path):
    sr, a = wavfile.read(path)
    a = a.astype(np.float32) / 32767.0
    return a if a.ndim == 2 else np.stack([a, a], axis=1)

def write(path, a):
    wavfile.write(path, SR, (np.clip(a, -1, 1) * 32767).astype(np.int16))
    return path

def pitched(a, ratio):
    n = int(len(a) / ratio)
    x = np.linspace(0, len(a) - 1, n)
    return np.stack([np.interp(x, np.arange(len(a)), a[:, c]) for c in range(2)], axis=1).astype(np.float32)

def mix(events, seconds):
    """events: (start_s, clip, gain)"""
    out = np.zeros((int(seconds * SR) + SR, 2), np.float32)
    for t, c, g in events:
        i = int(t * SR); j = min(len(out), i + len(c))
        out[i:j] += c[: j - i] * g
    p = np.abs(out).max()
    return out * (0.9 / p) if p > 0.9 else out

def play(path, loop=False):
    flags = winsound.SND_FILENAME | winsound.SND_ASYNC | (winsound.SND_LOOP if loop else 0)
    winsound.PlaySound(path, flags)

def stop():
    winsound.PlaySound(None, 0)

def wait_key(seconds=None):
    """Returns the key pressed, or None when `seconds` ran out (the sound finished)."""
    t0 = time.time()
    while seconds is None or time.time() - t0 < seconds:
        if msvcrt.kbhit():
            k = msvcrt.getwch()
            if k in ("\x00", "\xe0"):
                msvcrt.getwch(); continue
            return k
        time.sleep(0.02)
    return None


# ------------------------------------------------------------------ demos: the sounds the way the game uses them
def files(prefix):
    man = []
    for mf in ("manifest.json", "manifest_synth.json"):
        p = f"{OUT}/{mf}"
        if os.path.exists(p):
            man += json.load(open(p, encoding="utf8"))
    return [m for m in man if m["name"].startswith(prefix)]

def clips(prefix):
    return [read(f"{OUT}/{m['folder']}/{m['name']}.wav") for m in files(prefix)]

def demo(prefix):
    rng = random.Random(3)
    if prefix in ("Step_Snow", "Step_Ice"):
        cs = clips(prefix); ev, t, last = [], 0.3, -1
        for _ in range(12):                                   # a walk: never the same step twice, small pitch changes
            i = rng.randrange(len(cs))
            if i == last: i = (i + 1) % len(cs)
            last = i
            ev.append((t, pitched(cs[i], rng.uniform(0.92, 1.08)), rng.uniform(0.75, 1.0)))
            t += rng.uniform(0.52, 0.6)
        return write(f"{TMP}/demo_{prefix}.wav", mix(ev, t + 1)), "a walk of 12 steps, as in the game"
    if prefix == "Breath_Phrase":
        cs = clips(prefix); ev, t = [], 0.3
        for k, i in enumerate(range(len(cs))):                # the game: phrases back to back, effort rising soft -> hard
            e = k / max(1, len(cs) - 1)
            ev.append((t, cs[i], 0.35 + 0.65 * e))
            t += len(cs[i]) / SR + rng.uniform(0.05, 0.4)
        return write(f"{TMP}/demo_Breath.wav", mix(ev, t + 1.0)), "all phrases back to back, effort rising (as the game plays them)"
    if prefix == "Carabiner_ClipIn":
        ev, t = [], 0.3
        for c in clips("Carabiner_ClipIn"):                   # each version alone (review round 2: no rope pull after it)
            ev.append((t, c, 0.8))
            t += max(1.6, len(c) / SR + 1.0)
        return write(f"{TMP}/demo_ClipIn.wav", mix(ev, t)), "every clip-in version in order (1, 2, 3...), alone, as the game now plays it"
    if prefix == "Ice_Crack":
        cr, gr, co = clips("Ice_Crack"), clips("Ice_Groan"), clips("Serac_Collapse")
        ev = [(0.3, gr[0], 0.5), (2.5, cr[0], 0.8), (4.0, cr[5] if len(cr) > 5 else cr[-1], 0.8), (5.5, cr[-1], 0.9), (7.5, co[0], 1.0)]
        return write(f"{TMP}/demo_Serac.wav", mix(ev, 21.0)), "a serac zone: groan, warning cracks, then the collapse"
    if prefix == "Wind_Calm":
        # the game's blend (MountainAudioComponent::UpdateWind) while the wind rises 2 -> 10 over 40 s, at 6,000 m
        beds = [clips(n)[0] for n in ("Wind_Calm", "Wind_Gusty", "Wind_Blizzard", "Wind_Whistle")]
        L = 40 * SR; w = np.linspace(2, 10, L); ss = lambda a, b, x: np.clip((x - a) / (b - a), 0, 1) ** 2 * (3 - 2 * np.clip((x - a) / (b - a), 0, 1))
        alt = ss(5600, 7600, 6000.0)
        gains = [0.6 + (0.2 - 0.6) * ss(3, 7, w), ss(2.5, 5.5, w) * (1 - 0.5 * ss(7, 9.5, w)) * 0.8, ss(6, 9.5, w) * 0.95, ss(4, 7.5, w) * alt * 0.7]
        out = np.zeros((L, 2), np.float32)
        for b, g in zip(beds, gains):
            seg = np.tile(b, (L // len(b) + 1, 1))[:L]
            out += seg * g[:, None]
        out *= np.minimum(1, np.arange(L) / SR)[:, None]
        return write(f"{TMP}/demo_Wind.wav", out * (0.9 / max(0.9, np.abs(out).max()))), "the wind rising from calm to a storm over 40 s (all four loops blending)"
    if prefix == "Wheel_Tick":
        return wheel_demo(clips("UI_Chime_01")[:1], "demo_Wheel"), "the wheel spinning, slowing and landing (with the game's current landing sound)"
    return None, None

def wheel_demo(chime, name):
    """The wheel exactly as the game plays it (SurvivalHUDWidget::TickWheel): a tick per divider passing the pointer,
    ticks at 0.3-0.45, the landing sound at 0.8 just after the stop. Returns the written file."""
    rng = random.Random(3)
    cs = clips("Wheel_Tick")
    sector, spin, slow = 14.0, 1.0, 0.8; stop_t = spin + slow; rnd = sector * 9
    speed = 2 * rnd / (spin + slow / 3)
    def turn(T):
        left = speed * (spin - T) + speed * slow / 3 if T < spin else speed * slow / 3 * (1 - min(1, max(0, (T - spin) / slow))) ** 3
        after = max(0.0, T - stop_t)
        return left + (0.8 * np.exp(-after * 7) * np.sin(after * 20) if after > 0 else 0)
    ev, div = [], int(np.floor(turn(0) / sector + 0.5))
    for f in range(1, int((stop_t + 0.3) * 120)):
        T = f / 120; d = int(np.floor(turn(T) / sector + 0.5))
        if d != div:
            fast = min(1, max(0, 1 - T / stop_t))
            ev.append((0.3 + T, pitched(rng.choice(cs), rng.uniform(0.97, 1.03) * (1 + 0.06 * fast)), 0.45 - 0.15 * fast))
        div = d
    tail = 3.5
    if chime:
        ev.append((0.3 + stop_t + 0.05, chime[0], 0.8)); tail = len(chime[0]) / SR + 0.5
    return write(f"{TMP}/{name}.wav", mix(ev, 0.3 + stop_t + tail))


# ------------------------------------------------------------------ review
def load_review():
    try:
        return json.load(open(REVIEW_JSON, encoding="utf8"))
    except Exception:
        return {}

def save_review(rv):
    json.dump(rv, open(REVIEW_JSON, "w", encoding="utf8"), indent=1, ensure_ascii=False)
    lines = ["# Sound review (by ear)", "", "Written by Tools/Audio/listen_sounds.py. good / bad / unsure + your notes.", ""]
    for prefix, title, where, src in GROUPS:
        items = [m for m in (realm_items() if prefix == "Realm_Landing" else files(prefix)) if m["name"] in rv]
        if not items: continue
        lines += [f"## {title}", f"_{where}_", "", "| Sound | Verdict | Note |", "|---|---|---|"]
        for m in items:
            r = rv[m["name"]]
            lines.append(f"| {m.get('title', m['name'])} | {r.get('verdict', '')} | {r.get('note', '')} |")
        if f"demo:{prefix}" in rv:
            lines.append(f"| (demo) | {rv[f'demo:{prefix}'].get('verdict', '')} | {rv[f'demo:{prefix}'].get('note', '')} |")
        lines.append("")
    open(REVIEW_MD, "w", encoding="utf8").write("\n".join(lines))

def loop_preview(m):
    """The loop rotated so its seam comes 4 s in; it then repeats exactly as in the game."""
    a = read(f"{OUT}/{m['folder']}/{m['name']}.wav")
    k = max(0, len(a) - 4 * SR)
    return write(f"{TMP}/loop_{m['name']}.wav", np.concatenate([a[k:], a[:k]]))

def ask(text):
    stop()
    try:
        return input(text).strip()
    except EOFError:
        return ""

def fhash(path):
    return hashlib.md5(open(path, "rb").read()).hexdigest()

def approved(rv, name, path):
    """Already marked good, and the file has not changed since (entries from before hashes count as unchanged)."""
    r = rv.get(name)
    return bool(r) and r.get("verdict") == "good" and r.get("hash", fhash(path)) == fhash(path)

def approved_item(rv, m):
    r = rv.get(m["name"])
    if not r or r.get("verdict") != "good":
        return False
    return r.get("hash", item_hash(m)) == item_hash(m) if "sound" not in m else r.get("hash") == item_hash(m)

def review_item(rv, name, path, seconds, loop, label, hpath=None, h=None):
    h = h or fhash(hpath or path)
    while True:
        play(path, loop)
        print(f"    {label}", "(looping - press a key to rate)" if loop else f"({seconds:.1f} s)")
        k = wait_key(None if loop else seconds + 0.25)
        if k is None:
            k = wait_key(None)                                # finished: wait for the verdict
        stop()
        k = k.lower()
        if k in ("\r", "g"): rv[name] = {**rv.get(name, {}), "verdict": "good", "hash": h}; return "next"
        if k == "b": rv[name] = {"verdict": "bad", "note": ask("    what is wrong? > "), "hash": h}; return "next"
        if k == "u": rv[name] = {**rv.get(name, {}), "verdict": "unsure", "hash": h}; return "next"
        if k == "n": rv[name] = {**rv.get(name, {}), "note": ask("    note > ")}; continue
        if k == "r": continue
        if k in ("p", "s", "q", "d"): return k


def main():
    if "--build-only" in sys.argv:                            # check the demos and loop previews without playing
        for prefix, title, *_ in GROUPS:
            p, label = demo(prefix)
            rv = load_review()
            its = realm_items() if prefix == "Realm_Landing" else files(prefix)
            ask_n = sum(1 for m in its if not approved_item(rv, m))
            dq = "to judge" if p and not approved(rv, f"demo:{prefix}", p) else "approved"
            print("%-16s %2d sounds, %2d to judge  demo: %s" % (prefix, len(its), ask_n, ("%.1f s (%s) - %s" % (len(read(p)) / SR, dq, label)) if p else "-"))
            if prefix == "Realm_Landing":
                for m in its: print("    ", m["title"])
            for m in its:
                if m["loop"]: print("    loop preview", m["name"], "%.0f s" % (len(read(loop_preview(m))) / SR))
        return
    rv = load_review()
    print("YGGDRASIL'S PEAK - SOUND CHECK")
    print("Every sound the game uses. Each group starts with a demo of how the game plays it, then each sound alone.")
    print("Loops keep playing (their seam comes after ~4 s, then every lap) until you press a key.")
    print("Keys: Enter=good  B=bad  U=unsure  R=again  N=note  P=previous  D=demo again  S=skip group  Q=save+quit")
    redo = "--all" not in sys.argv
    if redo:
        print("Only NEW or CHANGED sounds (and ones not yet marked good) are played. Start with --all to hear everything.\n")
    gi = 0
    while gi < len(GROUPS):
        prefix, title, where, src = GROUPS[gi]
        realm = prefix == "Realm_Landing"                     # virtual items: the wheel landing on each weather
        items = realm_items() if realm else files(prefix)
        if not items:
            gi += 1; continue
        dpath, dlabel = demo(prefix)
        compare = False
        if redo:
            items = [m for m in items if not approved_item(rv, m)]
            want_demo = bool(dpath) and not approved(rv, f"demo:{prefix}", dpath)
            if not items and not want_demo:
                print(f"  ok  {title} - approved, unchanged")
                gi += 1; continue
        else:
            want_demo = bool(dpath)
        print(f"\n=== {title} ({len(items)} sound{'s' if len(items) != 1 else ''} to judge) ===")
        print(f"    plays: {where}\n    from:  {src}")
        action = None
        if want_demo:
            a = read(dpath)
            action = review_item(rv, f"demo:{prefix}", dpath, len(a) / SR, False, f"DEMO - {dlabel}")
        if action == "q": break
        if action == "s": gi += 1; continue
        if action == "p": gi = max(0, gi - 1); continue
        i = 0
        while i < len(items):
            m = items[i]
            orig = item_file(m)
            secs, label = m["seconds"], m.get("title", m["name"])
            if realm:                                          # the landing heard where it lives: after a wheel spin
                path = wheel_demo([read(orig)], f"wheel_{m['name'].replace(':', '_')}")
                secs, label = len(read(path)) / SR, f"wheel lands on {m['title']}"
            else:
                path = loop_preview(m) if m["loop"] else orig
            done = f"  [{rv[m['name']]['verdict']}]" if m["name"] in rv and "verdict" in rv[m["name"]] else ""
            print(f"  {i + 1}/{len(items)}  {m.get('title', m['name'])}{done}")
            action = review_item(rv, m["name"], path, secs, m["loop"], label, orig, item_hash(m))
            save_review(rv)
            if action == "next": i += 1
            elif action == "p": i = max(0, i - 1)
            elif action == "d" and dpath:
                review_item(rv, f"demo:{prefix}", dpath, len(read(dpath)) / SR, False, f"DEMO - {dlabel}")
            elif action in ("s", "q"): break
        if action == "q": break
        gi += 1
    stop()
    save_review(rv)
    print(f"\nSaved: {REVIEW_MD}\nTell Claude when you are done; it reads that file and fixes what you marked.")
    ask("Press Enter to close.")

if __name__ == "__main__":
    main()
