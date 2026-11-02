import osmium, json, sys
W,S,E,N = 86.74,27.85,87.10,28.17
out=[]
class H(osmium.SimpleHandler):
    def area(self, a):
        t = a.tags
        if t.get('natural') != 'glacier': return
        polys=[]
        for o in a.outer_rings():
            ring=[(n.lon,n.lat) for n in o]
            inn=[[(n.lon,n.lat) for n in i] for i in a.inner_rings(o)]
            polys.append([ring]+inn)
        xs=[p[0] for P in polys for p in P[0]]; ys=[p[1] for P in polys for p in P[0]]
        if not xs or max(xs)<W or min(xs)>E or max(ys)<S or min(ys)>N: return
        out.append({'id':a.orig_id(),'name':t.get('name:en') or t.get('name',''),'glacier_type':t.get('glacier:type',''),'polys':polys})
h=H(); h.apply_file(sys.argv[1], locations=True)
json.dump(out, open(sys.argv[2],'w'))
print(len(out)); print(sorted(set(o['name'] for o in out if o['name'])))
