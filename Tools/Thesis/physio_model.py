# Physiology model comparison (Development_Plan.md A2): the game's current SpO2 rule against a physiology-based model,
# with published reference points. Writes Saved/Review/physio_spo2_altitude.png and Saved/Review/physio_model.csv.
#   python -I Tools/Thesis/physio_model.py
#
# Proposed model (all standard equations; the parameters marked CALIBRATE are fitted to published data in A2):
#   1. Barometric pressure (West 1996, J Appl Physiol 81:1850):   PB = exp(6.63268 - 0.1112 h - 0.00149 h^2)  [Torr, h km]
#   2. Inspired O2:                                                PIO2 = 0.2093 (PB - 47)
#   3. Alveolar gas equation:                                      PAO2 = PIO2 - PACO2 / R,  R = 0.8
#   4. Arterial O2:                                                PaO2 = PAO2 - AaD   (AaD: alveolar-arterial gap, CALIBRATE)
#   5. Blood pH from CO2 (respiratory alkalosis):                  pH = 7.40 + k (40 - PaCO2), k = 0.008 acute, 0.0049 acclimatised
#   6. Virtual PO2 (Severinghaus 1979 temperature/pH/CO2 shift):   PO2v = PaO2 * 10^(0.024 (37 - T) + 0.40 (pH - 7.40) + 0.06 log10(40 / PaCO2))
#   7. Saturation (Severinghaus 1979, J Appl Physiol 46:599):      SaO2 = 100 / (23400 / (PO2v^3 + 150 PO2v) + 1)
#   Acclimatisation = how far the breathing has adapted: PACO2 moves from the acute curve to the acclimatised curve.
import math, csv
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt

ROOT = 'F:/UE_project/YggdrasilsPeak/'
SURFACE, INK, INK2, GRID = '#fcfcfb', '#0b0b0b', '#52514e', '#e4e3df'
S1, S2, S3 = '#2a78d6', '#eb6834', '#1baf7a'

# ---- the game's current rule (SurvivalConfig.h defaults, SurvivalComponent.cpp): resting, no bottle
DROP_START, SUMMIT, REST_AT_SUMMIT, SHELTER_BONUS = 4500.0, 8849.0, 58.0, 8.0


def game_spo2(alt_m, sheltered=False):
    thin = min(max((alt_m - DROP_START) / (SUMMIT - DROP_START), 0.0), 1.0)
    return min(100.0, 100.0 - thin * (100.0 - REST_AT_SUMMIT) + (SHELTER_BONUS if sheltered else 0.0))


# ---- the proposed model
R, AAD_REST, T_CORE = 0.8, 5.0, 37.0               # AaD at rest: CALIBRATE (exercise widens it)
# alveolar CO2 (Torr) by altitude (km). Acclimatised: 13.3 at 8.4 km is the arterial value measured by Grocott et al.
# 2009 (NEJM 360:140) and 7.5 on the summit is West et al. 1983 (J Appl Physiol 55:678); the points between are
# ESTIMATES to CALIBRATE. Acute (just arrived): a smaller fall, ESTIMATES to CALIBRATE.
# West's 7.5 on the summit is an extreme single-climber value; used as is, saturation would RISE above 8.4 km, so the
# summit point is an ESTIMATE (12.2) that keeps saturation falling with height until A2 calibrates it.
PACO2_ACCL = [(0.0, 40.0), (3.0, 33.0), (4.5, 27.5), (5.3, 24.0), (6.4, 20.0), (7.2, 17.0), (8.0, 14.5), (8.4, 13.3), (8.85, 12.2)]
PACO2_ACUTE = [(0.0, 40.0), (3.0, 36.0), (4.5, 32.5), (5.3, 30.0), (6.4, 27.0), (7.2, 25.0), (8.0, 23.0), (8.85, 21.0)]


def interp(table, x):
    if x <= table[0][0]:
        return table[0][1]
    for (x0, y0), (x1, y1) in zip(table, table[1:]):
        if x <= x1:
            return y0 + (y1 - y0) * (x - x0) / (x1 - x0)
    return table[-1][1]


def model(alt_m, acclim=1.0, aad=AAD_REST):
    h = alt_m / 1000.0
    pb = math.exp(6.63268 - 0.1112 * h - 0.00149 * h * h)
    pio2 = 0.2093 * (pb - 47.0)
    paco2 = interp(PACO2_ACUTE, h) + (interp(PACO2_ACCL, h) - interp(PACO2_ACUTE, h)) * acclim
    pao2 = max(pio2 - paco2 / R, 1.0)
    pa = max(pao2 - aad, 1.0)
    k = 0.008 + (0.0049 - 0.008) * acclim
    ph = 7.40 + k * (40.0 - paco2)
    pv = pa * 10 ** (0.024 * (37.0 - T_CORE) + 0.40 * (ph - 7.40) + 0.06 * math.log10(40.0 / paco2))
    sat = 100.0 / (23400.0 / (pv ** 3 + 150.0 * pv) + 1.0)
    return dict(pb=pb, pio2=pio2, paco2=paco2, pao2=pao2, pao2_art=pa, ph=ph, spo2=sat)


# ---- published reference points (only values we can cite; more are added in A2 from the literature and the user's papers)
REFS = [(8400.0, 54.0, 'Grocott et al. 2009, NEJM: arterial SaO\u2082 at 8,400 m, acclimatised, breathing air')]

CAMPS = [('Dingboche', 4410), ('Base Camp', 5364), ('Camp 1', 6065), ('Camp 2', 6400), ('Camp 3', 7160), ('South Col', 7906), ('Summit', 8849)]


def main():
    alts = list(range(0, 8901, 50))
    rows = []
    for a in alts:
        acc, acu = model(a, 1.0), model(a, 0.0)
        rows.append(dict(alt_m=a, game_rest=game_spo2(a), game_camp=game_spo2(a, True), model_acclimatised=acc['spo2'],
                         model_acute=acu['spo2'], pb_torr=acc['pb'], pio2=acc['pio2'], pao2_art_accl=acc['pao2_art'], ph_accl=acc['ph']))
    with open(ROOT + 'Saved/Review/physio_model.csv', 'w', newline='', encoding='utf-8') as f:
        w = csv.DictWriter(f, fieldnames=list(rows[0].keys()))
        w.writeheader()
        for r in rows:
            w.writerow({k: (f'{v:.2f}' if isinstance(v, float) else v) for k, v in r.items()})

    fig, ax = plt.subplots(figsize=(10, 6.2), facecolor=SURFACE)
    ax.set_facecolor(SURFACE)
    for s in ('top', 'right', 'left'):
        ax.spines[s].set_visible(False)
    ax.spines['bottom'].set_color(GRID)
    ax.grid(axis='y', color=GRID, linewidth=0.8)
    ax.tick_params(colors=INK2, labelsize=9, length=0)
    x = [r['alt_m'] for r in rows]
    ax.plot(x, [r['game_rest'] for r in rows], color=S2, linewidth=2, label='game now (resting, outside a camp)')
    ax.plot(x, [r['model_acclimatised'] for r in rows], color=S1, linewidth=2, label='physiology model, acclimatised')
    ax.plot(x, [r['model_acute'] for r in rows], color=S3, linewidth=2, label='physiology model, just arrived')
    for a, v, note in REFS:
        ax.scatter([a], [v], s=64, color=INK, zorder=5)
        ax.annotate('Grocott 2009', (a, v), textcoords='offset points', xytext=(-8, -16), ha='right', fontsize=8, color=INK)
    for name, a in CAMPS:
        ax.axvline(a, color=GRID, linewidth=1, zorder=0)
        ax.text(a, 101.5, name, rotation=0, fontsize=7.5, color=INK2, ha='center', va='bottom')
    ax.set_xlim(3500, 8900)
    ax.set_ylim(30, 101)
    ax.set_xlabel('altitude (m)', color=INK2, fontsize=9)
    ax.set_title('Resting SpO\u2082 against altitude  (%)', loc='left', fontsize=11, color=INK, pad=22)
    leg = ax.legend(frameon=False, fontsize=9, loc='lower left')
    for t in leg.get_texts():
        t.set_color(INK)
    fig.tight_layout()
    fig.savefig(ROOT + 'Saved/Review/physio_spo2_altitude.png', dpi=150, facecolor=SURFACE)
    print('camp        game  game(camp)  model-accl  model-acute   PB Torr  PaO2 accl')
    for name, a in CAMPS:
        m1, m0 = model(a, 1.0), model(a, 0.0)
        print(f'{name:10s} {game_spo2(a):5.1f}   {game_spo2(a, True):6.1f}     {m1["spo2"]:6.1f}      {m0["spo2"]:6.1f}     {m1["pb"]:6.1f}   {m1["pao2_art"]:5.1f}')
    m = model(8400, 1.0)
    print(f'check 8,400 m acclimatised: PaO2 {m["pao2_art"]:.1f} Torr (Grocott 24.6), SaO2 {m["spo2"]:.1f}% (Grocott 54.0), pH {m["ph"]:.2f}')


if __name__ == '__main__':
    main()
