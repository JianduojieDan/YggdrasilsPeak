import json,numpy as np,collections,sys
from scipy.interpolate import LinearNDInterpolator
D='F:/UE_project/YggdrasilsPeak/Saved/DetailTiles/route'
idx=json.load(open(D+'/tiles.json')); tr=json.load(open('C:/Users/danji/AppData/Local/Temp/ygg/trout.json'))
by=collections.defaultdict(list)
for r in tr: by[r[0]].append(r)
def load(p,piv):
    V=[]
    with open(p) as f:
        for line in f:
            if line.startswith('v '):
                s=line.split(); V.append((float(s[1]),float(s[2]),float(s[3])))
            elif line.startswith('f'): break
    V=np.array(V); return V[:,0]/100+piv[0], -V[:,1]/100+piv[1], V[:,2]/100+piv[2]
allr=[]; hits=collections.Counter()
for n,rows in by.items():
    piv=idx[n]; X,Y,Z=load(f'{D}/{n}.obj',piv)
    for r in rows: hits['tile' if r[4] and r[4].startswith('DetailTile') else r[4]]+=1
    P=np.array([(r[1]/100,r[2]/100,r[3]/100) for r in rows if r[4] and r[4].startswith('DetailTile')])
    if not len(P): continue
    m=np.zeros(len(X),bool)
    for px,py,_ in P: m|=(abs(X-px)<3)&(abs(Y-py)<3)
    zr=LinearNDInterpolator(np.c_[X[m],Y[m]],Z[m])(P[:,0],P[:,1])
    allr+=list(P[:,2]-zr)
e=np.array(allr); e=e[~np.isnan(e)]
print(dict(hits),'n',len(e),'median abs %.3f p95 %.3f max %.3f mean %.3f'%(np.median(abs(e)),np.percentile(abs(e),95),abs(e).max(),e.mean()))
