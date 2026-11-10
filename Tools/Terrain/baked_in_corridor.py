# Which old baked rocks lie inside the detail corridor (they sat on the old landscape); writes baked_remove.json
import json, numpy as np, os
from scipy.spatial import cKDTree
R = 'F:/UE_project/YggdrasilsPeak'
route = np.array([p[:2] for l in json.load(open(R + '/Tools/RoutePlanning/route.json')) for p in l['pts']]) / 100.0
dense = np.concatenate([np.linspace(a, b, max(2, int(np.hypot(*(b - a)) / 3)), endpoint=False) for a, b in zip(route[:-1], route[1:])])
t = cKDTree(dense)
d = json.load(open(R + '/Saved/DetailTiles/route/baked_rocks.json'))
out = {k: [i for i, (x, y) in enumerate(v) if t.query([x / 100, y / 100])[0] < 590] for k, v in d.items()}
json.dump(out, open(R + '/Saved/DetailTiles/route/baked_remove.json', 'w'))
print({k: len(v) for k, v in out.items()})
