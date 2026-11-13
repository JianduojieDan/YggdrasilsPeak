# Runs inside the editor: the O(1) "support" measure vs the exact vertex measure on random small stones.
import random
import unreal
exec(open('F:/UE_project/YggdrasilsPeak/Tools/Placement/ground_lib.py').read())
rocks = [a for a in acts if a.get_actor_label() == 'Rocks_Route'][0]
rng = random.Random(7)
for c in rocks.get_components_by_class(unreal.InstancedStaticMeshComponent):
    if c.get_instance_count() < 20000: continue
    d = []
    for i in rng.sample(range(c.get_instance_count()), 300):
        q, s, L = tr_parts(c.get_instance_transform(i, True))
        ms = measure_support(c.static_mesh, q, s[0], L)
        me = measure_exact([(c.static_mesh, (q, s, L))])
        if ms and me:
            d.append((ms['pen'], me['pen'], me['height']))
    diffs = sorted(abs(a - b) for a, b, h in d)
    agree = sum(1 for a, b, h in d if (a < 0) == (b < 0))
    print(c.static_mesh.get_name(), 'n', len(d), 'pen support vs exact: median |diff| %.1f cm, p90 %.1f; same floating verdict %d/%d' % (diffs[len(diffs) // 2], diffs[int(len(diffs) * .9)], agree, len(d)))
    print('  samples (support, exact, height):', [(round(a), round(b), round(h)) for a, b, h in d[:8]])
