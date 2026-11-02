import json, math, sys
import numpy as np
from PIL import Image, ImageDraw, ImageFilter
Z=12; S=256*2**Z
def merc(lat,lon): return ((lon+180)/360*S, (1-math.log(math.tan(math.radians(lat))+1/math.cos(math.radians(lat)))/math.pi)/2*S)
cx,cy=merc(28.008,86.921)
DX,DY=504.5-1.07,504.5-2.57          # fitted to summit/Gorak Shep/Dingboche
R=2048; k=R/1009.0                    # mask px per heightmap px
G=json.load(open('glacier/glaciers.json'))
img=Image.new('L',(R,R),0); dr=ImageDraw.Draw(img)
hole=Image.new('L',(R,R),0); dh=ImageDraw.Draw(hole)
for g in G:
    for P in g['polys']:
        def pix(ring): return [((merc(la,lo)[0]-cx+DX)*k,(merc(la,lo)[1]-cy+DY)*k) for lo,la in ring]
        dr.polygon(pix(P[0]),fill=255)
        for inner in P[1:]: dh.polygon(pix(inner),fill=255)
m=np.array(img,np.float32)*(1-np.array(hole,np.float32)/255)
Image.fromarray(m.astype(np.uint8)).filter(ImageFilter.GaussianBlur(2.5)).save('glacier/T_GlacierMask.png')
# preview over the heightmap hillshade
h=np.array(Image.open(r'E:/One_Drive_File/OneDrive/Desktop/游戏设计内容/28_008_86_921_12_1009_1009_16bit.png'),np.float32)
gy,gx=np.gradient(h); sh=np.clip(0.5+(gx-gy)/400,0,1)
base=np.stack([sh]*3,-1)
mm=np.array(Image.fromarray(m.astype(np.uint8)).resize((1009,1009)),np.float32)[...,None]/255
prev=base*(1-mm*0.6)+np.array([0.3,0.7,1.0])*mm*0.6
im=Image.fromarray((prev*255).astype(np.uint8)); d=ImageDraw.Draw(im)
import json as J
for L in J.load(open(r'F:/UE_project/YggdrasilsPeak/Tools/RoutePlanning/route.json')):
    pts=[((p[0]+504000)/3374.24,(p[1]+504000)/3374.24) for p in L['pts']]
    d.line(pts,fill=(255,60,60),width=2)
for n,(c,r) in {'EBC':(293.7,531.5),'Summit':(517,568),'Gorak Shep':(235.8,594.1),'Dingboche':(241.7,881.5)}.items():
    d.ellipse((c-4,r-4,c+4,r+4),fill=(255,220,0)); d.text((c+6,r-6),n,fill=(255,255,0))
im.crop((150,350,750,950)).resize((1200,1200)).save('glacier/preview.png')
print('glacier px fraction %.3f'%(m.mean()/255))
