# Usage budget (week 2026-11-20 06:00 UTC -> 2026-11-27 06:00 UTC)

Goal: spend the weekly limit evenly until Friday's reset, never hit the 5-hour wall mid-task, and run as many helpers in
parallel as the budget allows.

## The numbers (2026-11-20 11:48 UTC)

- Weekly, all models: 1 % used, resets 2026-11-27 06:00 UTC (6 d 18 h = 162 h left).
- Weekly, Fable: 0 % used (separate bar; Fable also counts heavily on the 5-hour bar).
- 5-hour window: 5 % used, resets 13:20 UTC.
- Plan: **Max** (the user confirmed with a screenshot; the session tool's "Pro" label is wrong).
- Two free limit resets on the account (only the user presses them):
  - Full reset, expires 2026-12-11 04:00 GMT+6. Worth up to one whole week if pressed when a week's bar is nearly
    full. It expires before the Dec 17/18 interim, so the best use is the week of Dec 4-11, or any week that runs out early.
  - Fable reset, expires 2027-03-09: refills the 5-hour limit and the Fable allowance and gives back what Fable used of
    the week (not the whole week). Good after a planned Fable-heavy day.
- The budget below assumes neither reset is used.

## The model

- Reserve R = 6 % of the week (last-day fixes, emergencies). Spendable S = 99 - 6 = 93 %.
- Days left D = 6.75. Daily allowance a = S / D = **13.8 % of the week per day**.
- The line to stay under: weekly used at time t <= 1 % + 13.8 % x (days since 2026-11-20 11:48 UTC).
  Checkpoints: Sat 21 12:00 UTC <= 15 %, Sun 22 <= 29 %, Mon 23 <= 43 %, Tue 24 <= 57 %, Wed 25 <= 70 %, Thu 26 <= 84 %,
  Fri 27 06:00 <= 94 % (the reserve is what is left).
- Conversion k = weekly % per 5-hour %. First reading today: 5 % of a window ~ 1 % of the week -> k ~ 0.20 (rough:
  the weekly bar rounds to whole percent, so k is 0.1-0.3 until more readings). Then one full 5-hour window ~ 20 % of
  the week, and the daily allowance ~ **69 % of one 5-hour window per day** (about 35 % per window if two windows a day).
- Helper cost (5-hour bar, per agent per minute, measured 2026-11-20 on Max): Opus high 0.10-0.15, Fable ~0.65.
  Sonnet and Haiku: to be measured. With k ~ 0.2: ten Opus-high helpers for an hour ~ 70 % of a window ~ 14 % of the
  week = one day's allowance.

## Running rules

1. Before a big fan-out: read usage, compute the day's remaining allowance, size the helper count to fit.
2. Ahead of the line -> fewer helpers, cheaper tiers. Behind it -> more helpers.
3. 5-hour window at 90 % -> finish running helpers, start no new ones until the reset.
4. Fable: only for level-5 tasks, at most 1-2 at a time, with a usage reading before and after.
5. Every reading goes in the ledger; k and the per-tier rates are re-fitted from it.

## Ledger

| UTC time | 5-hour % | weekly % | Fable % | what was running |
|---|---|---|---|---|
| 2026-11-20 11:48 | 5 | 1 | 0 | main Opus + 3 helpers (2 Sonnet high, 1 Opus medium) on the 22 review fixes |
| 2026-11-20 11:51 | 7 | 1 | 0 | build done; robot 4-min test; 2 Opus-high investigators + 1 Sonnet-medium starting |
| 2026-11-20 12:01 | 15 | 2 | 0 | 4 Opus high + 1 Opus medium + 2 Sonnet helpers over ~10 min, main xhigh; k so far ~0.1-0.2 |
| 2026-11-20 15:49 | 8 | 5 | 0 | main xhigh + 1 Opus-high helper (follow, 19 min); #68 tests, follow build/test/commit e1948f99 |
