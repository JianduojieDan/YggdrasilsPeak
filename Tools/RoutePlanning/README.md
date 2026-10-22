# Route planning scripts

Editor-side Python used to build the climbing route in `base_map`. Each script runs inside the
running editor through Python remote execution (`uepy.py <script>`; needs `bRemoteExecution=True`).
Scripts read their inputs from globals set by a small wrapper, e.g.:

```python
ROUTE_JSON = r"F:/UE_project/YggdrasilsPeak/Tools/RoutePlanning/route.json"
exec(open(r"F:/UE_project/YggdrasilsPeak/Tools/RoutePlanning/place_ropes.py").read())
```

| Script | Inputs | Does |
|---|---|---|
| `plan_route.py` | `OUT_JSON`, `LEGS` (None = all) | A* over a 20 m traced height grid between consecutive `RouteLandmark`s; avoids slopes over 50 deg and rocks; writes `route.json` |
| `place_ropes.py` | `ROUTE_JSON` | Replaces all `AFixedRope` actors with one rope per leg (4 m points, 30 cm above the snow, stops 12 m from camps) |
| `place_camps.py` | `ASSETS_PY`, `ONLY` (camp indices or None) | Rebuilds the Fab props around the chosen camps (actors tagged `CampProp`) |
| `place_pickups.py` | none | Turns decorative oxygen bottles (`*_O2_*`) into `AOxygenPickup` actors; run after `place_camps.py` |
