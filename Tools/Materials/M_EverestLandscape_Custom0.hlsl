
struct HexTile {
  float H1(float2 p){ return frac(sin(dot(p,float2(127.1,311.7)))*43758.5453); }
  float VN(float2 p){ float2 i=floor(p); float2 f=frac(p); f=f*f*(3.0-2.0*f);
    return lerp(lerp(H1(i),H1(i+float2(1,0)),f.x),lerp(H1(i+float2(0,1)),H1(i+1.0),f.x),f.y); }
  float Fbm(float2 p){ return 0.5*VN(p)+0.3*VN(p*2.13+17.3)+0.2*VN(p*4.71+5.1); }
  float2 Hash(float2 p){ return frac(sin(float2(dot(p,float2(127.1,311.7)),dot(p,float2(269.5,183.3))))*43758.5453); }
  float2 Rot(float2 v, float a){ float s,c; sincos(a,s,c); return float2(c*v.x-s*v.y, s*v.x+c*v.y); }
  // safe normalize: Nanite-tessellated triangles can deliver a zero/NaN normal -> fall back instead of NaN (black triangles)
  float3 SN(float3 v, float3 f){ float l=dot(v,v); return (l>1e-12 && l<1e12) ? v*rsqrt(l) : f; }
  // per-cell UV transform: random rotation (full circle) and scale (0.35..2.8, log-uniform)
  float2 Xf(float2 uv, float2 q, float2 o){ return Rot(uv*exp2(lerp(-1.5,1.5,q.y)), q.x*6.2831853)+o; }
  float2 Xd(float2 d, float2 q){ return Rot(d*exp2(lerp(-1.5,1.5,q.y)), q.x*6.2831853); }
  // rock UV multiplier per cell: 90% normal rock (0.75..1.35), 10% big rock (UV x0.18..0.33 = 3..5.5x bigger)
  float SR(float2 q){ return q.y<0.9 ? lerp(0.75,1.35,q.y/0.9) : lerp(0.18,0.33,(q.y-0.9)*10.0); }
  float2 XfR(float2 uv, float2 q, float2 o){ return Rot(uv*SR(q), q.x*6.2831853)+o; }
  float2 XdR(float2 d, float2 q){ return Rot(d*SR(q), q.x*6.2831853); }
  // per-cell brightness/warmth so neighbouring (esp. big) rocks never look alike
  float3 CT(float2 q){ float b=frac(q.x*13.7+q.y*5.1); float t=frac(q.x*3.3+q.y*17.9); return lerp(0.8,1.15,b)*lerp(float3(0.94,0.98,1.05),float3(1.06,1.0,0.92),t); }
  void Cell(float2 uv, out float3 w, out float2 o1, out float2 o2, out float2 o3, out float2 q1, out float2 q2, out float2 q3){
    float2 s=float2(uv.x,-0.57735027*uv.x+1.15470054*uv.y)*2.4;
    float2 b=floor(s); float2 f=frac(s); float z=1.0-f.x-f.y;
    float2 v1,v2,v3;
    if(z>0){ w=float3(z,f.y,f.x); v1=b; v2=b+float2(0,1); v3=b+float2(1,0); }
    else   { w=float3(-z,1.0-f.y,1.0-f.x); v1=b+1.0; v2=b+float2(1,0); v3=b+float2(0,1); }
    o1=Hash(v1); o2=Hash(v2); o3=Hash(v3); q1=Hash(v1+7.31); q2=Hash(v2+7.31); q3=Hash(v3+7.31);
  }
  float3 Blend(float3 w, float3 h){ float3 b=pow(max(w,0.0),4.0)*(0.3+h); return b/max(b.x+b.y+b.z,1e-6); }
  // soft pillowy swells of deep snow (~10 m and ~27 m across), height 0..1.6
  float Pil(float2 p){ return VN(p/1000.0)*0.6+VN(p/2700.0+17.0); }
  // softer, smaller lumps: 2-6 m mounds and 30-60 cm powder clumps (height 0..1 each)
  float Lump(float2 p){ return VN(p/250.0+3.1)*0.6+VN(p/600.0+8.7)*0.4; }
  float Clump(float2 p){ return VN(p/35.0+1.3)*0.6+VN(p/60.0+4.4)*0.4; }
  // wind-packed ripples (sastrugi) across the prevailing westerly: gentle windward rise, short steep lee face, in patches; 0..1
  float Sast(float2 p){ float2 r=Rot(p,0.35); float w=r.y/180.0+VN(r/1500.0)*3.0+VN(r/400.0)*0.6;
    float s=frac(w); float ridge=s<0.75?s/0.75:(1.0-s)/0.25;
    return ridge*ridge*smoothstep(0.35,0.7,VN(r*float2(1.0/2500.0,1.0/700.0)+9.1)); }
};
HexTile hx;

float3 n=hx.SN(float3(N.xy,N.z*1.774),float3(0,0,1)); // landscape normal arrives unscaled; 1.774 = xy/z scale (1687.12/951.4, 2x landscape)
float lf=hx.Fbm(WP.xy/30000.0); // value noise, no fixed wavelength (sine waves made visible stripes)
float lf2=hx.Fbm(WP.xy/6000.0+31.7);
float nzv=n.z+(lf-0.5)*0.10+(lf2-0.5)*0.05;
float slopeM=1.0-smoothstep(Cutoff-Soft,Cutoff+Soft,nzv);
float3 w=pow(abs(n),4.0); w=w/max(w.x+w.y+w.z,1e-6);
float wa[3]={w.x,w.y,w.z};
float2 pc[3]={WP.yz,WP.xz,WP.xy};
float rt=1.0/RockTile; float st=1.0/SnowTile;

float2 n2[3]={n.yz,n.xz,n.xy}; float nax[3]={n.x,n.y,n.z};
float sg[3]={n.x<0?-1.0:1.0,n.y<0?-1.0:1.0,n.z<0?-1.0:1.0};
float2 gx[3]={float2(ddx(pc[0].x),ddx(pc[0].y)),float2(ddx(pc[1].x),ddx(pc[1].y)),float2(ddx(pc[2].x),ddx(pc[2].y))};
float2 gy[3]={float2(ddy(pc[0].x),ddy(pc[0].y)),float2(ddy(pc[1].x),ddy(pc[1].y)),float2(ddy(pc[2].x),ddy(pc[2].y))};
// snow variety fields: which of two snow scales shows, how bumpy it is (wind-packed vs. loose)
float sm1=hx.Fbm(WP.xy/1700.0+7.7); float sm2=hx.Fbm(WP.xy/5200.0+91.3); float sm3=hx.Fbm(WP.xy/600.0+41.9);
float bigMix=smoothstep(0.35,0.65,sm2);
float sNStr=NStr*lerp(0.25,1.7,smoothstep(0.2,0.8,sm1*0.7+sm3*0.3));
float3 rockC=0,rockW=0,rockM=0; float rockH=0;
float3 snowC=0,snowW=0; float snowH=0;
[unroll] for(int i=0;i<3;i++){
  float2 uvR=pc[i]*rt; float2 dX=gx[i]*rt; float2 dY=gy[i]*rt;
  float3 hw; float2 o1,o2,o3,q1,q2,q3; hx.Cell(uvR,hw,o1,o2,o3,q1,q2,q3);
  float2 ua=hx.XfR(uvR,q1,o1), ub=hx.XfR(uvR,q2,o2), uc=hx.XfR(uvR,q3,o3);
  float2 xa=hx.XdR(dX,q1), xb=hx.XdR(dX,q2), xc=hx.XdR(dX,q3);
  float2 ya=hx.XdR(dY,q1), yb=hx.XdR(dY,q2), yc=hx.XdR(dY,q3);
  float3 hh=float3(RockH.SampleGrad(RockHSampler,ua,xa,ya).r,RockH.SampleGrad(RockHSampler,ub,xb,yb).r,RockH.SampleGrad(RockHSampler,uc,xc,yc).r);
  float3 bw=hx.Blend(hw,hh);
  float3 rc=RockCol.SampleGrad(RockColSampler,ua,xa,ya).rgb*hx.CT(q1)*bw.x+RockCol.SampleGrad(RockColSampler,ub,xb,yb).rgb*hx.CT(q2)*bw.y+RockCol.SampleGrad(RockColSampler,uc,xc,yc).rgb*hx.CT(q3)*bw.z;
  float3 rm3=RockORM.SampleGrad(RockORMSampler,ua,xa,ya).rgb*bw.x+RockORM.SampleGrad(RockORMSampler,ub,xb,yb).rgb*bw.y+RockORM.SampleGrad(RockORMSampler,uc,xc,yc).rgb*bw.z;
  // normals were sampled in rotated UV space: rotate them back by -angle before blending
  float2 tx=hx.Rot(RockNrm.SampleGrad(RockNrmSampler,ua,xa,ya).rg*2.0-1.0,-q1.x*6.2831853)*bw.x
           +hx.Rot(RockNrm.SampleGrad(RockNrmSampler,ub,xb,yb).rg*2.0-1.0,-q2.x*6.2831853)*bw.y
           +hx.Rot(RockNrm.SampleGrad(RockNrmSampler,uc,xc,yc).rg*2.0-1.0,-q3.x*6.2831853)*bw.z;
  tx*=NStr*1.25; tx.x*=(i==2)?-sg[i]:sg[i];
  float tz=sqrt(saturate(1.0-dot(tx,tx)));
  float3 tn=float3(tx+n2[i],tz*nax[i]);
  float3 wn=(i==0)?tn.zxy:((i==1)?float3(tn.x,tn.z,tn.y):tn);
  rockC+=rc*wa[i]; rockW+=wn*wa[i]; rockH+=dot(hh,bw)*wa[i]; rockM+=rm3*wa[i];
  // snow layer A: small hex cells (1.8x denser than rock), random offset/rotation/size per cell
  float2 uvS=pc[i]*st; float2 sX=gx[i]*st; float2 sY=gy[i]*st;
  float3 sw; float2 p1,p2,p3,r1,r2,r3; hx.Cell(uvS*1.8+53.1,sw,p1,p2,p3,r1,r2,r3);
  float2 sa=hx.Xf(uvS,r1,p1), sb=hx.Xf(uvS,r2,p2), sc3=hx.Xf(uvS,r3,p3);
  float2 ka=hx.Xd(sX,r1), kb=hx.Xd(sX,r2), kc=hx.Xd(sX,r3);
  float2 la=hx.Xd(sY,r1), lb=hx.Xd(sY,r2), lc=hx.Xd(sY,r3);
  float3 sh3=float3(SnowH.SampleGrad(SnowHSampler,sa,ka,la).r,SnowH.SampleGrad(SnowHSampler,sb,kb,lb).r,SnowH.SampleGrad(SnowHSampler,sc3,kc,lc).r);
  float3 sbw=hx.Blend(sw,sh3);
  float3 sc=SnowCol.SampleGrad(SnowColSampler,sa,ka,la).rgb*sbw.x+SnowCol.SampleGrad(SnowColSampler,sb,kb,lb).rgb*sbw.y+SnowCol.SampleGrad(SnowColSampler,sc3,kc,lc).rgb*sbw.z;
  float2 sx=hx.Rot(SnowNrm.SampleGrad(SnowNrmSampler,sa,ka,la).rg*2.0-1.0,-r1.x*6.2831853)*sbw.x
           +hx.Rot(SnowNrm.SampleGrad(SnowNrmSampler,sb,kb,lb).rg*2.0-1.0,-r2.x*6.2831853)*sbw.y
           +hx.Rot(SnowNrm.SampleGrad(SnowNrmSampler,sc3,kc,lc).rg*2.0-1.0,-r3.x*6.2831853)*sbw.z;
  float shA=dot(sh3,sbw);
  // snow layer B: same texture 3.7x larger, rotated 1.1 rad, blended in by large noise patches
  float2 uvB=hx.Rot(uvS*0.27,1.1)+float2(0.43,0.71); float2 bX=hx.Rot(sX*0.27,1.1); float2 bY=hx.Rot(sY*0.27,1.1);
  float3 scB=SnowCol.SampleGrad(SnowColSampler,uvB,bX,bY).rgb;
  float shB=SnowH.SampleGrad(SnowHSampler,uvB,bX,bY).r;
  float2 sxB=hx.Rot(SnowNrm.SampleGrad(SnowNrmSampler,uvB,bX,bY).rg*2.0-1.0,-1.1);
  float bm=saturate(bigMix+(shB-shA)*0.8-0.4+0.4*bigMix);
  sc=lerp(sc,scB,bm); sx=lerp(sx,sxB,bm); float shS=lerp(shA,shB,bm);
  sx*=sNStr; sx.x*=(i==2)?-sg[i]:sg[i];
  float sz=sqrt(saturate(1.0-dot(sx,sx)));
  float3 sn=float3(sx+n2[i],sz*nax[i]);
  float3 swn=(i==0)?sn.zxy:((i==1)?float3(sn.x,sn.z,sn.y):sn);
  snowC+=sc*wa[i]; snowW+=swn*wa[i]; snowH+=shS*wa[i];
}
// large-scale rock variation (hundreds of metres): brightness and warm/cool shift
float mv=hx.Fbm(WP.xy/45000.0+11.1);
rockC*=lerp(0.65,1.3,saturate((mv-0.5)*2.2+0.5));
rockC=lerp(rockC*float3(0.92,0.97,1.04),rockC*float3(1.08,1.0,0.88),lf2);
float3 rN=hx.SN(rockW,n);
float hb=slopeM*1.6-0.3+(rockH-snowH)*0.6;
float rm=smoothstep(0.35,0.65,hb);
// snow settles on the rock's up-facing ledges (bumps from its texture, low spots first), with a dark lip just below each snow edge
float lup=rN.z+(lf2-0.5)*0.15+(0.5-rockH)*0.25;
float ledge=saturate(smoothstep(0.62,0.80,lup)*Ledge*1.3);
float lip=smoothstep(0.48,0.62,lup)*(1.0-smoothstep(0.62,0.80,lup));
rockC*=lerp(1.0,0.72,lip*rm);
rm=saturate(rm-ledge);

// ---- Snow (2026-10-22): deep soft snow low and in flat valleys, hard wind-packed snow and ice high and on steep ground ----
float elev=6326.3+(WP.z-129770.0)/100.0;
float zn=hx.Fbm(WP.xy/9000.0+3.3);
float hard=saturate(smoothstep(6600.0,7600.0,elev+(zn-0.5)*600.0)*0.9+(1.0-smoothstep(0.72,0.92,n.z))*0.55);
// big soft swells (all snow), flattened where the wind packs it
float e=40.0;
float p0=hx.Pil(WP.xy), px=hx.Pil(WP.xy+float2(e,0)), py=hx.Pil(WP.xy+float2(0,e));
float2 gP=float2(px-p0,py-p0)/e*lerp(140.0,50.0,hard);
// sastrugi ripples on hard snow only (6 cm tall)
float es=20.0;
float s0=hx.Sast(WP.xy), sx1=hx.Sast(WP.xy+float2(es,0)), sy1=hx.Sast(WP.xy+float2(0,es));
float2 gS=float2(sx1-s0,sy1-s0)/es*6.0*hard;
// grain from the snow texture is kept faint: deep powder looks smooth, wind crust a bit rougher
float3 micro=hx.SN(lerp(n,hx.SN(snowW,n),lerp(0.22,0.55,hard)),n);
// soft lumps and clumps, strongest in deep powder; clumps fade out where a pixel covers more than ~3 cm (no shimmer far away)
float fp=length(float2(ddx(WP.x),ddx(WP.y)))+length(float2(ddy(WP.x),ddy(WP.y)));
float el=10.0, ec=4.0;
float l0=hx.Lump(WP.xy), lx=hx.Lump(WP.xy+float2(el,0)), ly=hx.Lump(WP.xy+float2(0,el));
float c0=hx.Clump(WP.xy), cx=hx.Clump(WP.xy+float2(ec,0)), cy=hx.Clump(WP.xy+float2(0,ec));
float2 gL=float2(lx-l0,ly-l0)/el*lerp(60.0,18.0,hard);
float2 gC=float2(cx-c0,cy-c0)/ec*lerp(3.5,1.2,hard)*(1.0-smoothstep(1.5,4.0,fp));
float3 sN=hx.SN(micro-float3(gP.x+gS.x+gL.x+gC.x,gP.y+gS.y+gL.y+gC.y,0.0),n);
// colour: clean white powder, blue-grey crust, blue ice patches on steep high ground; blue in the hollows
// bare blue ice only where it is both high and steep (wind strips the snow), in soft-edged patches
float ice=(1.0-smoothstep(0.6,0.85,n.z))*smoothstep(7000.0,7800.0,elev)*smoothstep(0.45,0.8,hx.Fbm(WP.xy/4000.0+5.0))*0.8;
float3 sCol=lerp(float3(0.90,0.93,0.97),float3(0.80,0.86,0.93),hard);
sCol=lerp(sCol,float3(0.66,0.78,0.89),ice);
float hollow=saturate((0.55-snowH)*1.6)*0.45+saturate(0.6-p0*0.6)*0.35*(1.0-hard)+saturate(0.45-l0)*0.4*(1.0-hard);
hollow=saturate(hollow+saturate(rm*(1.0-rm)*4.0)*0.5); // blue edge where snow meets rock: the side of the snow layer
sCol*=lerp(float3(1,1,1),float3(0.84,0.91,1.0),hollow);
sCol*=lerp(0.95,1.03,sm3*0.5+sm1*0.5);
float sRough=lerp(lerp(0.86,0.93,sm1),lerp(0.58,0.68,sm3),hard);
sRough=lerp(sRough,0.22,ice);
// sparkle: a few ice-crystal facets per square decimetre catch the sun, near the camera only
float2 gc=floor(WP.xy/1.6); float2 gh=hx.Hash(gc+0.37);
float spark=step(0.988,gh.x)*(1.0-smoothstep(0.6,1.6,fp))*(1.0-ice);
sRough=lerp(sRough,0.08,spark);
sN=hx.SN(lerp(sN,hx.SN(float3((gh.y-0.5)*0.8,(frac(gh.y*7.3)-0.5)*0.8,1.0),n),spark),n);

float3 col=lerp(sCol,rockC,rm);
col*=lerp(0.96,1.03,lf);
Normal=hx.SN(lerp(sN,rN,rm),n);
Roughness=lerp(sRough,rockM.g,rm);
AO=lerp(lerp(1.0,0.9,hollow),rockM.r,rm);
RockMask=rm;
SnowOut=sCol*lerp(0.96,1.03,lf); // fed to UDS so its weather-snow layer looks like ours
return max(col,0.0);
