# Reads one recorded play session and writes what happened in it (user 2026-11-13: "monitor me play the game, so you
# know exactly what's going on"). Input: a Saved/Monitor/<date_time>/ folder written by the game (UYggSessionMonitor)
# and by Tools/Monitor/system_monitor.ps1. Output in the same folder:
#   report.md      smoothness, every big hitch with its likely cause, sky vs ground, turning, places, realms, menus,
#                  input, the climb, the computer (CPU/GPU/other programs), the log
#   timeline.png   FPS, frame cost by thread, GPU load and altitude over the session, hitches marked
#   views.png      frame time by where the camera looks and by how fast it turns
#   python -I Tools/Monitor/analyze_session.py [session folder | latest]
# Charts follow the dataviz method: one measure per panel on a shared time axis (no second y-scale), thin lines,
# recessive grid, ink-coloured text, categorical slots in fixed order.
import bisect, collections, csv, glob, os, re, statistics, sys
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt

ROOT = 'F:/UE_project/YggdrasilsPeak/'
SURFACE, INK, INK2, GRID = '#fcfcfb', '#0b0b0b', '#52514e', '#e4e3df'
S1, S2, S3, S4 = '#2a78d6', '#eb6834', '#1baf7a', '#eda100'      # categorical slots 1-4 (reference palette, light)
CRIT = '#d03b3b'                                                  # status: critical (hitch marks)


def num(v):
    try:
        return float(v)
    except (TypeError, ValueError):
        return float('nan')


def load(path, text=()):
    if not os.path.exists(path):
        return []
    csv.field_size_limit(10 ** 9)
    with open(path, encoding='utf-8-sig', newline='', errors='replace') as f:
        rows = list(csv.DictReader(f))
    for r in rows:
        for k in list(r.keys()):
            if k is None:
                del r[k]
            elif k not in text:
                r[k] = num(r[k])
    return rows


def pct(vals, p):
    if not vals:
        return float('nan')
    s = sorted(vals)
    return s[min(len(s) - 1, int(len(s) * p))]


def med(vals):
    return statistics.median(vals) if vals else float('nan')


def fmt_t(t):
    return f'{int(t // 60)}:{int(t % 60):02d}'


def main():
    arg = sys.argv[1] if len(sys.argv) > 1 else 'latest'
    if arg == 'latest':
        dirs = sorted(d for d in glob.glob(ROOT + 'Saved/Monitor/*') if os.path.isdir(d))
        if not dirs:
            sys.exit('no sessions in Saved/Monitor')
        arg = dirs[-1]
    D = arg.rstrip('/\\')
    name = os.path.basename(D)
    info = {}
    if os.path.exists(D + '/session.txt'):
        for line in open(D + '/session.txt', encoding='utf-8-sig', errors='replace'):
            k, _, v = line.partition(' ')
            v = v.strip()
            if k in ('cvar', 'setting'):
                kk, _, vv = v.partition(' = ')
                info.setdefault(k, {})[kk.strip()] = vv.strip()
            else:
                info[k] = v
    F = load(D + '/frames.csv')
    S = load(D + '/seconds.csv', text=('map', 'state', 'realm'))
    E = load(D + '/events.csv', text=('type', 'detail'))
    Y = load(D + '/system.csv', text=('gpu_pstate', 'gpu_slowdown', 'top_others'))
    epoch0 = num(info.get('epoch'))
    for r in Y:
        r['t'] = r['epoch'] - epoch0 if epoch0 == epoch0 else float('nan')
    out = []
    w = out.append

    # ---------------------------------------------------------------- what counts as play
    # not paused, not in a menu, not the first 6 s after a level appears (loading), not a frame slowed by the monitor's
    # own screenshot (hitch column 2)
    level_t = [e['t'] for e in E if e['type'] == 'level']

    def warm(t):
        return any(0.0 <= t - lt < 6.0 for lt in level_t)

    play = [f for f in F if f['paused'] == 0 and f['cursor'] == 0 and f['t'] > 3.0 and f['hitch'] != 2 and not warm(f['t'])]
    s_t = [s['t'] for s in S]

    def alt_at(t):
        if not S:
            return float('nan')
        return S[min(len(S) - 1, bisect.bisect_left(s_t, t))]['alt_m']
    ft = [f['dt_ms'] for f in play]
    dur = F[-1]['t'] if F else 0.0
    w(f'# Play session {name}\n')
    w(f'Recorded {info.get("started", "?")}, {fmt_t(dur)} long, {len(F):,} frames ({len(play):,} in play, the rest paused or in menus).  ')
    w(f'Viewport {info.get("viewport", "?")}; {info.get("gpu", "?")}; {info.get("cpu", "?")}; RAM {info.get("ram", "?")}.\n')
    cv = info.get('cvar', {})
    st = info.get('setting', {})
    keys = ['r.VSync', 't.MaxFPS', 'r.ScreenPercentage', 'r.NGX.DLSS.Enable', 'r.Streamline.DLSSG.Enable', 'r.DynamicGlobalIlluminationMethod',
            'r.ReflectionMethod', 'r.Shadow.Virtual.Enable']
    w('Settings at start: ' + ', '.join(f'{k} {cv[k]}' for k in keys if k in cv) + '.  ')
    if st:
        w('Game settings: ' + ', '.join(f'{k} {v}' for k, v in st.items()) + '.\n')

    # ---------------------------------------------------------------- smoothness headline
    w('\n## Smoothness (in play)\n')
    if ft:
        fps = 1000.0 / med(ft)
        low1 = 1000.0 / pct(ft, 0.99)
        low01 = 1000.0 / pct(ft, 0.999)
        under50 = sum(1 for v in ft if v > 20.0) / len(ft) * 100
        h33 = sum(1 for v in ft if v > 33.4)
        h50 = sum(1 for v in ft if v > 50)
        h100 = sum(1 for v in ft if v > 100)
        mins = max(1e-6, sum(ft) / 60000.0)
        pres = [f['fps_presented'] for f in play if f['fps_presented'] > 0]
        w(f'- Rendered frames: median {fps:.1f} fps ({med(ft):.2f} ms), 1% low {low1:.1f} fps, 0.1% low {low01:.1f} fps, worst frame {max(ft):.0f} ms.')
        if pres:
            w(f'- Shown on screen with DLSS frame generation: median {med(pres):.0f} fps (what the player sees).')
        w(f'- Frames slower than 50 fps: {under50:.1f}% of play.')
        w(f'- Hitches: {h33} frames over 33 ms ({h33 / mins:.1f} a minute), {h50} over 50 ms, {h100} over 100 ms.')
        g = med([f['game_ms'] for f in play]); r = med([f['render_ms'] for f in play]); p = med([f['gpu_ms'] for f in play])
        rh = med([f['rhi_ms'] for f in play])
        bound = max((p, 'the GPU'), (r, 'the render thread'), (g, 'the game thread'))[1]
        w(f'- Median cost per frame: game thread {g:.1f} ms, render thread {r:.1f} ms, RHI {rh:.1f} ms, GPU {p:.1f} ms -> limited by {bound}.')
        w(f'- Draw calls median {med([f["draws"] for f in play]):,.0f} (max {max(f["draws"] for f in play):,.0f}), triangles median {med([f["prims_k"] for f in play]) / 1000:.1f} M.')
        vs = cv.get('r.VSync', '?')
        if vs not in ('0', '?'):
            w(f'- VSync is on: rendered fps cannot rise above the monitor rate; drops show as steps (60 -> 30/40/45...).')

    # ---------------------------------------------------------------- hitches with context
    ev_by_type = collections.defaultdict(list)
    for e in E:
        ev_by_type[e['type']].append(e)
    hitch_ev = ev_by_type.get('hitch', [])
    shots_t = [e['t'] for e in ev_by_type.get('screenshot', [])]
    ui_t = sorted(e['t'] for e in E if e['type'] in ('ui', 'focus', 'settings', 'level', 'realm', 'death', 'camp'))
    ft_t = [f['t'] for f in F]
    sys_t = [y['t'] for y in Y]

    def near(times, t, before, after=0.0):
        i = bisect.bisect_left(times, t - before)
        return i < len(times) and times[i] <= t + after

    def frames_between(a, b):
        i, j = bisect.bisect_left(ft_t, a), bisect.bisect_right(ft_t, b)
        return F[i:j]

    def sys_at(t):
        if not Y:
            return None
        i = min(len(Y) - 1, max(0, bisect.bisect_left(sys_t, t)))
        return Y[i] if abs(Y[i]['t'] - t) < 3 else None

    causes = collections.Counter()
    rows = []
    load_hitches = []
    for e in hitch_ev:
        t = e['t']
        d = e['detail']
        m = re.match(r'([\d.]+) ms \(game ([\d.]+), render ([\d.]+), gpu ([\d.]+); ([a-z ]+)\)(.*)', d)
        if not m:
            continue
        ms, gms, rms, pms, bnd, logs = float(m[1]), float(m[2]), float(m[3]), float(m[4]), m[5], m[6]
        if warm(t):
            load_hitches.append(ms)
            continue
        tags = []
        low = logs.lower()
        if 'forcing update for all mesh draw commands' in low:
            tags.append('mesh draw commands rebuilt (sky light capture / scene change)')
        if 'pso' in low or 'shader' in low:
            tags.append('shader / PSO compile')
        if 'garbage' in low or 'collectgarbage' in low:
            tags.append('garbage collection')
        before = frames_between(t - 0.35, t)
        at = before[-1] if before else None
        if at and at['loading']:
            tags.append('asset loading')
        prev = frames_between(t - 1.5, t - 1.0)
        if at and prev and at['streaming'] - prev[-1]['streaming'] > 20:
            tags.append(f'streaming burst ({prev[-1]["streaming"]:.0f} -> {at["streaming"]:.0f} wanted)')
        turn = max((f['turn_dps'] for f in before), default=0.0)
        if turn > 90:
            tags.append(f'turning {turn:.0f} deg/s')
        if near(shots_t, t, 0.4, 0.05):
            tags.append('monitor screenshot')
        if near(ui_t, t, 0.6, 0.1):
            tags.append('menu / window / realm change')
        y = sys_at(t)
        if y:
            if y['cpu_core_max'] > 97:
                tags.append(f'a CPU core at {y["cpu_core_max"]:.0f}%')
            if y['editors'] > 0 or y['other_games'] > 0:
                tags.append('Unreal editor or another game running')
            if y['hard_faults_s'] > 500:
                tags.append(f'paging to disk ({y["hard_faults_s"]:.0f} pages/s)')
            if y['gpu_slowdown'] and any(k in y['gpu_slowdown'] for k in ('thermal', 'power', 'hw-slowdown')):
                tags.append(f'GPU slowed ({y["gpu_slowdown"]})')
        if not tags:
            tags.append('unexplained')
        for tg in tags:
            causes[re.sub(r'\s*\(.*|\s\d+.*', '', tg)] += 1
        rows.append((ms, t, bnd, gms, rms, pms, tags, at, logs))

    w('\n## Hitches and what was happening\n')
    if rows:
        w('Likely causes (one hitch can have several):  ')
        w(', '.join(f'{k} {v}' for k, v in causes.most_common()) + '\n')
        w('| time | frame | slow side | game / render / GPU ms | camera | likely cause |')
        w('|---|---|---|---|---|---|')
        for ms, t, bnd, gms, rms, pms, tags, at, logs in sorted(rows, reverse=True)[:30]:
            cam = f'{alt_at(t):.0f} m, pitch {at["pitch"]:.0f}, speed {at["speed_ms"]:.1f} m/s' if at else ''
            w(f'| {fmt_t(t)} | {ms:.0f} ms | {bnd} | {gms:.1f} / {rms:.1f} / {pms:.1f} | {cam} | {"; ".join(tags)} |')
        logged = [(t, logs) for ms, t, *_, logs in sorted(rows, reverse=True)[:12] if logs.strip()]
        if logged:
            w('\nLog lines around the biggest hitches:')
            for t, logs in logged:
                w(f'- {fmt_t(t)}: ' + logs.strip(' |')[:400])
    else:
        w('No hitches in play.')
    if load_hitches:
        w(f'\n{len(load_hitches)} more slow frames while a level was loading (first 6 s after it appeared, worst {max(load_hitches):.0f} ms): not counted above.')

    # ---------------------------------------------------------------- where the camera looks / how fast it turns
    def bucket_table(title, keyf, buckets):
        w(f'\n## {title}\n')
        w('| | frames | median fps | 1% low | median GPU ms | median render ms |')
        w('|---|---|---|---|---|---|')
        res = []
        for label, lo, hi in buckets:
            sel = [f for f in play if lo <= keyf(f) < hi]
            if len(sel) < 30:
                continue
            m = med([f['dt_ms'] for f in sel])
            w(f'| {label} | {len(sel):,} | {1000 / m:.1f} | {1000 / pct([f["dt_ms"] for f in sel], 0.99):.1f} | {med([f["gpu_ms"] for f in sel]):.2f} | {med([f["render_ms"] for f in sel]):.2f} |')
            res.append((label, sel))
        return res

    look = bucket_table('Looking up, level or down (camera pitch)', lambda f: f['pitch'],
                        [('down (< -20 deg)', -91, -20), ('level (-20..20)', -20, 20), ('up (20..50)', 20, 50), ('sky (> 50)', 50, 91)])
    turnb = bucket_table('Turning speed', lambda f: f['turn_dps'],
                         [('still (< 15 deg/s)', 0, 15), ('slow (15-60)', 15, 60), ('normal (60-180)', 60, 180), ('fast (180-400)', 180, 400), ('flick (> 400)', 400, 1e9)])

    # ---------------------------------------------------------------- places, realms, weather
    w('\n## Places\n')
    cells = collections.defaultdict(list)
    for f in play:
        cells[(round(f['x_m'] / 100), round(f['y_m'] / 100))].append(f)
    worst = sorted(((med([f['dt_ms'] for f in v]), k, v) for k, v in cells.items() if len(v) > 120), reverse=True)[:8]
    if worst:
        w('Slowest 100 m squares (at least 2 s spent there):')
        w('| square (x, y m) | altitude | time there | median fps | median GPU ms |')
        w('|---|---|---|---|---|')
        for m, k, v in worst:
            w(f'| {k[0] * 100}, {k[1] * 100} | {med([alt_at(f["t"]) for f in v[::30]]):.0f} m | {sum(f["dt_ms"] for f in v) / 1000:.0f} s | {1000 / m:.1f} | {med([f["gpu_ms"] for f in v]):.2f} |')
    sp = [s for s in S if s['paused'] == 0 and s['cursor'] == 0 and s['fps'] > 0]
    if sp:
        by = collections.defaultdict(list)
        for s in sp:
            by[s['realm']].append(s['fps'])
        w('\nFPS by realm sky shown: ' + ', '.join(f'{k} {med(v):.0f} ({len(v)} s)' for k, v in sorted(by.items(), key=lambda kv: -len(kv[1]))) + '.  ')
        storm = [s['fps'] for s in sp if s.get('storm', 0) > 0.3]
        calm = [s['fps'] for s in sp if s.get('storm', 0) <= 0.3]
        if storm:
            w(f'In storms {med(storm):.0f} fps ({len(storm)} s), calm {med(calm):.0f} fps.')

    # ---------------------------------------------------------------- menus, focus, input
    w('\n## Menus, window and input\n')
    paused = sum(1 for s in S if s['paused'])
    menu = sum(1 for s in S if s['cursor'] and not s['paused'])
    unfocused = sum(1 for s in S if s.get('focus', 1) == 0)
    w(f'Paused {paused} s, other screens with the mouse cursor {menu} s, game window not active {unfocused} s.  ')
    uis = ev_by_type.get('ui', [])
    if uis:
        w('Screens over time: ' + '; '.join(f'{fmt_t(e["t"])} {e["detail"] or "(none)"}' for e in uis[:40]) + '.  ')
    kd = collections.Counter(e['detail'] for e in ev_by_type.get('key_down', []))
    if kd:
        w('Keys and buttons pressed: ' + ', '.join(f'{k} {v}' for k, v in kd.most_common(20)) + '.  ')
    md = sum(abs(f['mouse_dx']) + abs(f['mouse_dy']) for f in F)
    w(f'Mouse moved {md:,.0f} px in total.')
    sets = ev_by_type.get('settings', [])
    if sets:
        w(f'\nSettings changed {len(sets)} times; last state: {sets[-1]["detail"][:600]}')

    # ---------------------------------------------------------------- the climb
    w('\n## The climb\n')
    if S:
        alts = [s['alt_m'] for s in S if s['alt_m'] > 0]
        if alts:
            w(f'Altitude {alts[0]:.0f} m at start, {alts[-1]:.0f} m at the end, highest {max(alts):.0f} m.  ')
        o2 = [s['spo2'] for s in S if s['spo2'] > 0]
        if o2:
            w(f'SpO2 lowest {min(o2):.0f}%, heart rate highest {max(s["hr_bpm"] for s in S):.0f} bpm, core lowest {min(s["core_c"] for s in S if s["core_c"] > 0):.1f} C, health lowest {min(s["health"] for s in S):.0f}.  ')
        sc = collections.Counter(s['state'] for s in S if s['paused'] == 0)
        w('Time spent: ' + ', '.join(f'{k} {v} s' for k, v in sc.most_common()) + '.')
    for t in ('death', 'camp', 'realm', 'level'):
        if ev_by_type.get(t):
            w(f'\n{t.capitalize()} events: ' + '; '.join(f'{fmt_t(e["t"])} {e["detail"]}' for e in ev_by_type[t][:30]))
    msgs = ev_by_type.get('message', [])
    if msgs:
        w(f'\nMessages the game showed ({len(msgs)}): ' + '; '.join(f'{fmt_t(e["t"])} "{e["detail"][:80]}"' for e in msgs[:25]))

    # ---------------------------------------------------------------- the computer
    w('\n## The computer\n')
    if Y:
        w(f'Game process CPU median {med([y["game_cpu"] for y in Y]):.0f}% of all cores (max {max(y["game_cpu"] for y in Y):.0f}%), '
          f'busiest core median {med([y["cpu_core_max"] for y in Y]):.0f}%, whole CPU median {med([y["cpu_total"] for y in Y]):.0f}%.  ')
        w(f'GPU load median {med([y["gpu_util"] for y in Y]):.0f}%, VRAM up to {max(y["vram_mb"] for y in Y) / 1024:.1f} of {Y[0]["vram_total_mb"] / 1024:.1f} GB, '
          f'GPU up to {max(y["gpu_temp"] for y in Y):.0f} C and {max(y["gpu_power_w"] for y in Y):.0f} W, clock median {med([y["gpu_clock"] for y in Y]):.0f} MHz.  ')
        w(f'Game memory up to {max(y["game_ram_mb"] for y in Y) / 1024:.1f} GB (private {max(y["game_private_mb"] for y in Y) / 1024:.1f} GB), free RAM lowest {min(y["ram_free_mb"] for y in Y) / 1024:.1f} GB, '
          f'paging peaks {max(y["hard_faults_s"] for y in Y):.0f} pages/s.  ')
        slow = collections.Counter(y['gpu_slowdown'] for y in Y if y['gpu_slowdown'])
        if slow:
            w('GPU slow-down reasons (share of samples): ' + ', '.join(f'{k} {v * 100 / len(Y):.0f}%' for k, v in slow.most_common(5)) + '.  ')
        ed = sum(1 for y in Y if y['editors'] > 0 or y['other_games'] > 0)
        if ed:
            w(f'**An Unreal editor or a second game was running for {ed * 100 / len(Y):.0f}% of the session** (shares the GPU and CPU).  ')
        others = collections.Counter()
        for y in Y:
            for part in (y['top_others'] or '').split():
                n, _, v = part.rpartition(':')
                others[n] += num(v)
        if others:
            w('Other programs using CPU (sum of % samples): ' + ', '.join(f'{k} {v:.0f}' for k, v in others.most_common(8)) + '.')
    else:
        w('No system.csv (the system monitor was not running).')

    # ---------------------------------------------------------------- log
    w('\n## Engine log\n')
    if os.path.exists(D + '/log.txt'):
        lines = open(D + '/log.txt', encoding='utf-8', errors='replace').read().splitlines()
        rep = collections.Counter()
        last = ''
        for ln in lines:
            m = re.match(r'[\d.]+\s+\(previous line repeated (\d+) more times\)', ln)
            if m:
                rep[last] += int(m[1])
                continue
            body = re.sub(r'^[\d.]+\s+', '', ln)
            last = re.sub(r'(?<![A-Za-z\d])\d+(\.\d+)?', '#', body)[:200]
            rep[last] += 1
        warn = sum(v for k, v in rep.items() if 'warning' in k.lower() or 'error' in k.lower())
        w(f'{len(lines):,} lines kept, {warn:,} warnings/errors. Most frequent:')
        for k, v in rep.most_common(15):
            w(f'- {v:,} x  {k}')

    with open(D + '/report.md', 'w', encoding='utf-8') as f:
        f.write('\n'.join(out) + '\n')

    # ---------------------------------------------------------------- charts
    plt.rcParams.update({'font.size': 9, 'axes.edgecolor': GRID, 'axes.labelcolor': INK2, 'xtick.color': INK2, 'ytick.color': INK2,
                         'axes.facecolor': SURFACE, 'figure.facecolor': SURFACE, 'axes.grid': True, 'grid.color': GRID, 'grid.linewidth': 0.6})
    if S:
        T = [s['t'] / 60 for s in S]
        panels = [('FPS (rendered)', [('average', [s['fps'] for s in S], S1), ('slowest frame', [s['fps_min'] for s in S], S2)])]
        if F:
            per = collections.defaultdict(lambda: ([], [], []))
            for f in F:
                b = per[int(f['t'])]
                b[0].append(f['game_ms']); b[1].append(f['render_ms']); b[2].append(f['gpu_ms'])
            ks = sorted(per)
            panels.append(('frame cost, ms (median per second)', [('game thread', [med(per[k][0]) for k in ks], S1), ('render thread', [med(per[k][1]) for k in ks], S2),
                                                                  ('GPU', [med(per[k][2]) for k in ks], S3)], [k / 60 for k in ks]))
        if Y:
            panels.append(('GPU load %', [('GPU', [y['gpu_util'] for y in Y], S1)], [y['t'] / 60 for y in Y]))
        panels.append(('altitude, m', [('altitude', [s['alt_m'] for s in S], S1)]))
        fig, axes = plt.subplots(len(panels), 1, figsize=(12, 2.2 * len(panels)), sharex=True)
        for ax, pnl in zip(axes, panels):
            title, series = pnl[0], pnl[1]
            xs = pnl[2] if len(pnl) > 2 else T
            for lab, ys, col in series:
                ax.plot(xs[:len(ys)], ys, color=col, linewidth=1.2, label=lab)
            ax.set_title(title, loc='left', color=INK, fontsize=10)
            if len(series) > 1:
                ax.legend(loc='upper right', frameon=False, fontsize=8)
            for lt in level_t:                                    # level loads, left out of the numbers
                ax.axvspan(lt / 60, (lt + 6) / 60, color=GRID, alpha=0.7, linewidth=0)
        # the loading seconds read thousands of fps or seconds-long frames: scale the panels to play
        pf = [s['fps'] for s in S if not warm(s['t']) and s['fps'] > 0]
        if pf:
            axes[0].set_ylim(0, max(70, pct(pf, 0.99) * 1.15))
        if F and play:
            axes[1].set_ylim(0, max(20, pct([max(f['game_ms'], f['render_ms'], f['gpu_ms']) for f in play], 0.995) * 1.3))
        axes[0].set_xlim(0, dur / 60)
        for ms, t, *_ in rows:
            if ms > 50:
                axes[0].axvline(t / 60, color=CRIT, linewidth=0.6, alpha=0.5)
        for t in shots_t:                                         # the monitor's own screenshots cost one slow frame each
            axes[0].axvline(t / 60, color=INK2, linewidth=0.6, linestyle=':', alpha=0.6)
        axes[-1].set_xlabel('minutes (red: hitches over 50 ms; dotted: monitor screenshot; grey: level loading, not counted)')
        fig.tight_layout()
        fig.savefig(D + '/timeline.png', dpi=110)
        plt.close(fig)
    if look or turnb:
        fig, axes = plt.subplots(1, 2, figsize=(12, 3.4))
        for ax, res, title in ((axes[0], look, 'frame time by camera pitch'), (axes[1], turnb, 'frame time by turning speed')):
            labels = [r[0].split(' (')[0] for r in res]
            data = [[f['dt_ms'] for f in r[1]] for r in res]
            if data:
                ax.boxplot(data, showfliers=False, widths=0.5, medianprops={'color': S1, 'linewidth': 2}, boxprops={'color': INK2}, whiskerprops={'color': INK2}, capprops={'color': INK2})
                ax.set_xticks(range(1, len(labels) + 1), labels)
                ax.plot(range(1, len(data) + 1), [pct(d, 0.99) for d in data], 'o', color=S2, markersize=6, label='1% slowest')
                ax.legend(frameon=False, fontsize=8)
            ax.set_title(title + ' (ms, lower is better)', loc='left', color=INK, fontsize=10)
        fig.tight_layout()
        fig.savefig(D + '/views.png', dpi=110)
        plt.close(fig)
    # screenshots: full-size PNGs (5-8 MB each) become 1280-wide JPEGs (~0.2 MB), enough to see the scene
    try:
        from PIL import Image
        for png in glob.glob(D + '/shots/*.png'):
            im = Image.open(png).convert('RGB')
            im.thumbnail((1280, 1280))
            im.save(png[:-4] + '.jpg', quality=82)
            os.remove(png)
    except Exception as ex:
        print('screenshots not converted:', ex)
    print('report:', D + '/report.md')


if __name__ == '__main__':
    main()
