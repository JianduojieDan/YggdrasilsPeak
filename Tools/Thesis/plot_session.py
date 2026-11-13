# Charts one thesis data-log session (Development_Plan.md A3).
#   python -I Tools/Thesis/plot_session.py [path/to/session_*.csv | latest]
# Out (Saved/Review): telemetry_<session>_timeline.png  - altitude, SpO2, heart rate, core temperature, stamina over time
#                     telemetry_<session>_spo2_alt.png  - SpO2 against altitude by effort
#                     telemetry_<session>_summary.txt   - duration, distance, extremes, time above 8,000 m, events
# Style (dataviz method): one measure per panel on a shared time axis (never two y-scales), thin 2 px lines, recessive
# grid, text in ink colours, series colour = categorical slot 1; the scatter uses three effort groups (all-pairs cap).
import csv, glob, os, sys
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt

ROOT = 'F:/UE_project/YggdrasilsPeak/'
SURFACE, INK, INK2, GRID = '#fcfcfb', '#0b0b0b', '#52514e', '#e4e3df'
S1, S2, S3 = '#2a78d6', '#eb6834', '#1baf7a'          # categorical slots 1-3 (validated all-pairs, light mode)
DAMAGE_SPO2, COLD_WARN = 52.0, 35.0                    # SurvivalConfig: OxygenDamageBelow, ColdWarningEnter


def load(path):
    with open(path, encoding='utf-8') as f:
        rows = list(csv.DictReader(f))
    for r in rows:
        for k, v in r.items():
            if k in ('state', 'realm', 'events'):
                continue
            try:
                r[k] = float(v)
            except (TypeError, ValueError):
                r[k] = float('nan')
    return rows


def style(ax, title, unit):
    ax.set_facecolor(SURFACE)
    for s in ('top', 'right', 'left'):
        ax.spines[s].set_visible(False)
    ax.spines['bottom'].set_color(GRID)
    ax.grid(axis='y', color=GRID, linewidth=0.8)
    ax.tick_params(colors=INK2, labelsize=9, length=0)
    ax.set_title(f'{title}  ({unit})', loc='left', fontsize=10, color=INK, pad=4)


def main():
    arg = sys.argv[1] if len(sys.argv) > 1 else 'latest'
    if arg == 'latest':
        files = sorted(glob.glob(ROOT + 'Saved/Telemetry/session_*.csv'), key=os.path.getmtime)
        if not files:
            sys.exit('no session files in Saved/Telemetry')
        arg = files[-1]
    rows = load(arg)
    if len(rows) < 2:
        sys.exit(f'{arg}: too few rows ({len(rows)})')
    name = os.path.splitext(os.path.basename(arg))[0].replace('session_', '')
    out = ROOT + 'Saved/Review/telemetry_' + name
    t = [r['t_s'] / 60.0 for r in rows]
    events = [(r['t_s'] / 60.0, r['events']) for r in rows if r['events'] and r['events'] not in ('teleport',)]

    # ---- timeline: five panels, one measure each, shared time axis
    hr_top = max(140.0, max(r['hr_bpm'] for r in rows) + 10.0)
    panels = [('Altitude', 'm', 'alt_m', None, None), ('SpO\u2082', '%', 'spo2', (DAMAGE_SPO2, 'hypoxia damage'), (40, 101)),
              ('Heart rate', 'bpm', 'hr_bpm', None, (40, hr_top)), ('Core temperature', '\u00b0C', 'core_c', (COLD_WARN, 'cold warning'), (32, 37.5)),
              ('Stamina', '%', 'stamina', None, (0, 101))]
    fig, axes = plt.subplots(len(panels), 1, figsize=(10, 10.5), sharex=True, facecolor=SURFACE)
    for ax, (title, unit, key, ref, lim) in zip(axes, panels):
        style(ax, title, unit)
        if lim:
            ax.set_ylim(*lim)
        ax.plot(t, [r[key] for r in rows], color=S1, linewidth=2, solid_capstyle='round')
        if ref:
            ax.axhline(ref[0], color=INK2, linewidth=1, linestyle=(0, (4, 3)))
            ax.text(t[-1], ref[0], f' {ref[1]}', color=INK2, fontsize=8, va='center', ha='left')
        for et, _ in events:
            ax.axvline(et, color=GRID, linewidth=1, zorder=0)
    # events: numbered on the top panel, spelled out in a key under the figure
    top = axes[0]
    lo, hi = top.get_ylim()
    key = []
    for i, (et, text) in enumerate(events, 1):
        top.text(et, hi, str(i), fontsize=8, color=INK2, ha='center', va='bottom')
        label = text.split(' | ')[0]
        key.append(f'{i}  {et:.1f} min  ' + ((label[:70] + '\u2026') if len(label) > 71 else label))
    axes[-1].set_xlabel('time (min)', color=INK2, fontsize=9)
    fig.suptitle(f'Session {name}', x=0.06, ha='left', fontsize=12, color=INK)
    rows_key = max(1, (len(key) + 1) // 2)
    fig.tight_layout(rect=(0, 0.032 * rows_key + 0.01, 1, 0.97))
    for j, line in enumerate(key):
        fig.text(0.06 + 0.47 * (j // rows_key), 0.032 * rows_key - 0.032 * (j % rows_key) - 0.002, line, fontsize=7.5, color=INK2, va='top')
    fig.savefig(out + '_timeline.png', dpi=150, facecolor=SURFACE)
    plt.close(fig)

    # ---- SpO2 against altitude, by effort (three groups: the all-pairs cap for a scatter)
    groups = [('resting', S1, ('rest',)), ('walking / on the rope', S2, ('walk', 'rope')), ('sprinting / climbing', S3, ('sprint', 'climb'))]
    fig, ax = plt.subplots(figsize=(8, 5.5), facecolor=SURFACE)
    style(ax, 'SpO\u2082 against altitude', '%')
    for label, col, states in groups:
        pts = [(r['alt_m'], r['spo2']) for r in rows if r['state'] in states]
        if pts:
            ax.scatter([p[0] for p in pts], [p[1] for p in pts], s=36, color=col, edgecolor=SURFACE, linewidth=1, label=f'{label} ({len(pts)} s)')
    ax.axhline(DAMAGE_SPO2, color=INK2, linewidth=1, linestyle=(0, (4, 3)))
    ax.set_xlabel('altitude (m)', color=INK2, fontsize=9)
    leg = ax.legend(frameon=False, fontsize=9, loc='lower left')
    for txt in leg.get_texts():
        txt.set_color(INK)
    fig.tight_layout()
    fig.savefig(out + '_spo2_alt.png', dpi=150, facecolor=SURFACE)
    plt.close(fig)

    # ---- summary
    dist = sum(r['speed_ms'] * (rows[i]['t_s'] - rows[i - 1]['t_s']) for i, r in enumerate(rows) if i > 0) / 1000.0
    above = sum(rows[i]['t_s'] - rows[i - 1]['t_s'] for i in range(1, len(rows)) if rows[i]['alt_m'] >= 8000.0)
    lines = [f'session        {name}', f'file           {arg}', f'rows           {len(rows)}',
             f'duration       {rows[-1]["t_s"] / 60:.1f} min', f'distance       {dist:.2f} km (walked, teleports excluded)',
             f'altitude       {min(r["alt_m"] for r in rows):.0f} - {max(r["alt_m"] for r in rows):.0f} m',
             f'lowest SpO2    {min(r["spo2"] for r in rows):.1f} %', f'highest HR     {max(r["hr_bpm"] for r in rows):.0f} bpm',
             f'lowest core    {min(r["core_c"] for r in rows):.2f} C', f'above 8,000 m  {above / 60:.1f} min',
             f'states         ' + ', '.join(f'{s} {sum(1 for r in rows if r["state"] == s)}' for s in sorted({r["state"] for r in rows})),
             'events:'] + [f'  {et:6.2f} min  {text}' for et, text in events]
    open(out + '_summary.txt', 'w', encoding='utf-8').write('\n'.join(lines) + '\n')
    print('\n'.join(lines[:11]))
    print('charts:', out + '_timeline.png', out + '_spo2_alt.png')


if __name__ == '__main__':
    main()
