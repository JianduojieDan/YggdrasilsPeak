
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
  float2 Xf(float2 uv, float2 q, float2 o){ return Rot(uv*exp2(lerp(-0.5,0.5,q.y)), q.x*6.2831853)+o; }
  float2 Xd(float2 d, float2 q){ return Rot(d*exp2(lerp(-1.5,1.5,q.y)), q.x*6.2831853); }
  // rock UV multiplier per cell: 90% normal rock (0.75..1.35), 10% big rock (UV x0.18..0.33 = 3..5.5x bigger)
  float SR(float2 q){ return q.y<0.9 ? lerp(0.75,1.35,q.y/0.9) : lerp(0.18,0.33,(q.y-0.9)*10.0); }
  float2 XfR(float2 uv, float2 q, float2 o){ return Rot(uv*SR(q), q.x*6.2831853)+o; }
  float2 XdR(float2 d, float2 q){ return Rot(d*SR(q), q.x*6.2831853); }
  void Cell(float2 uv, out float3 w, out float2 o1, out float2 o2, out float2 o3, out float2 q1, out float2 q2, out float2 q3){
    float2 s=float2(uv.x,-0.57735027*uv.x+1.15470054*uv.y)*2.4;
    float2 b=floor(s); float2 f=frac(s); float z=1.0-f.x-f.y;
    float2 v1,v2,v3;
    if(z>0){ w=float3(z,f.y,f.x); v1=b; v2=b+float2(0,1); v3=b+float2(1,0); }
    else   { w=float3(-z,1.0-f.y,1.0-f.x); v1=b+1.0; v2=b+float2(1,0); v3=b+float2(0,1); }
    o1=Hash(v1); o2=Hash(v2); o3=Hash(v3); q1=Hash(v1+7.31); q2=Hash(v2+7.31); q3=Hash(v3+7.31);
  }
  float3 Blend(float3 w, float3 h){ float3 b=pow(max(w,0.0),4.0)*(0.3+h); return b/max(b.x+b.y+b.z,1e-6); }
};
HexTile hx;
// ---- wind ripples (2026-11-14): integer-hashed hex cells (~2.8 m) over the ripple map, each with a random offset,
// a +-0.15 rad turn and +-12 % size. The colour node and the tile displacement node run the same code, so the
// raised crests and their shading line up.
struct WindRip {
  float2 H2(float2 p){ int2 i=int2(floor(p)); uint h=asuint(i.x)*0x8da6b343u+asuint(i.y)*0xd8163841u;
    h^=h>>15; h*=0x2c1b3c6du; h^=h>>12; h*=0x297a2d39u; h^=h>>15; return float2(float(h&0xffffu),float(h>>16))/65536.0; }
  float2 R(float2 v, float a){ float s,c; sincos(a,s,c); return float2(c*v.x-s*v.y, s*v.x+c*v.y); }
  void Cells(float2 u, out float3 w, out float2 o1, out float2 o2, out float2 o3, out float3 a, out float3 k){
    float2 s=float2(u.x,-0.57735027*u.x+1.15470054*u.y)*1.08;
    float2 b=floor(s); float2 f=frac(s); float z=1.0-f.x-f.y; float2 v1,v2,v3;
    if(z>0){ w=float3(z,f.y,f.x); v1=b; v2=b+float2(0,1); v3=b+float2(1,0); }
    else   { w=float3(-z,1.0-f.y,1.0-f.x); v1=b+1.0; v2=b+float2(1,0); v3=b+float2(0,1); }
    o1=H2(v1); o2=H2(v2); o3=H2(v3);
    float2 q1=H2(v1+7.0), q2=H2(v2+7.0), q3=H2(v3+7.0);
    a=(float3(q1.x,q2.x,q3.x)-0.5)*0.3; k=exp2((float3(q1.y,q2.y,q3.y)-0.5)*0.35);
  }
  float3 Bl(float3 w, float3 h){ float3 b=pow(max(w,0.0),4.0)*(0.3+h); return b/max(b.x+b.y+b.z,1e-6); }
};
WindRip wr;
struct MorHexFx {
// ---- detail tiles: moraine sampled in randomly rotated / offset hexagonal cells (no visible 4.5 m repeat).
// The same function runs in the displacement node, so the raised stones line up with the visible ones.
uint MIH(float2 p){ int2 i = int2(floor(p)); uint h = asuint(i.x) * 0x8da6b343u + asuint(i.y) * 0xd8163841u;
  h ^= h >> 15; h *= 0x2c1b3c6du; h ^= h >> 12; h *= 0x297a2d39u; h ^= h >> 15; return h; }
float2 MH2(float2 p){ uint h = MIH(p); return float2(float(h & 0xffffu), float(h >> 16)) / 65536.0; }
float2 MRot(float2 v, float a){ float s,c; sincos(a,s,c); return float2(c*v.x-s*v.y, s*v.x+c*v.y); }
void MHex(float2 uv, out float3 w, out float2 u1, out float2 u2, out float2 u3, out float3 ang)
{
  float2 s = float2(uv.x - uv.y * 0.57735027, uv.y * 1.15470054) * 1.6;
  float2 b = floor(s), f = frac(s); float z = 1.0 - f.x - f.y;
  float2 v1, v2, v3;
  if (z > 0) { w = float3(z, f.y, f.x); v1 = b; v2 = b + float2(0, 1); v3 = b + float2(1, 0); }
  else { w = float3(-z, 1.0 - f.y, 1.0 - f.x); v1 = b + 1.0; v2 = b + float2(1, 0); v3 = b + float2(0, 1); }
  ang = float3(MH2(v1 + 3.7).x, MH2(v2 + 3.7).x, MH2(v3 + 3.7).x) * 6.2831853;
  u1 = MRot(uv, ang.x) + MH2(v1); u2 = MRot(uv, ang.y) + MH2(v2); u3 = MRot(uv, ang.z) + MH2(v3);
}
};
MorHexFx mhx;

float3 n=hx.SN(float3(N.xy,N.z*1.0),float3(0,0,1)); // landscape normal arrives unscaled; 1.774 = xy/z scale (1687.12/951.4, 2x landscape)
float lf=hx.Fbm(WP.xy/30000.0); // value noise, no fixed wavelength (sine waves made visible stripes)
float lf2=hx.Fbm(WP.xy/6000.0+31.7);
float nzv=n.z+(lf-0.5)*0.10+(lf2-0.5)*0.05;
float slopeM=1.0-smoothstep(Cutoff-Soft,Cutoff+Soft,nzv);
// detail tiles: rubble is laid over the snow by the glacier pass (TileRock), steep rock still comes from slope here
float3 w=pow(abs(n),4.0); w=w/max(w.x+w.y+w.z,1e-6);
float wa[3]={w.x,w.y,w.z};
float2 pc[3]={WP.yz,WP.xz,WP.xy};
float rt=1.0/RockTile; float st=1.0/SnowTile;

float rockH=0,snowH=0;
[unroll] for(int i=0;i<3;i++){
  float2 uvR=pc[i]*rt; float3 hw; float2 o1,o2,o3,q1,q2,q3; hx.Cell(uvR,hw,o1,o2,o3,q1,q2,q3);
  float3 hh=float3(RockH.SampleLevel(RockHSampler,hx.XfR(uvR,q1,o1),1).r,RockH.SampleLevel(RockHSampler,hx.XfR(uvR,q2,o2),1).r,RockH.SampleLevel(RockHSampler,hx.XfR(uvR,q3,o3),1).r);
  rockH+=dot(hh,hx.Blend(hw,hh))*wa[i];
  float2 uvS=pc[i]*st; float3 sw; float2 p1,p2,p3,r1,r2,r3; hx.Cell(uvS*1.8+53.1,sw,p1,p2,p3,r1,r2,r3); // same cells as the colour node (2026-11-14)
  float3 sh3=float3(SnowH.SampleLevel(SnowHSampler,hx.Xf(uvS,r1,p1),3).r,SnowH.SampleLevel(SnowHSampler,hx.Xf(uvS,r2,p2),3).r,SnowH.SampleLevel(SnowHSampler,hx.Xf(uvS,r3,p3),3).r); // mip 3 = 8 mm
  snowH+=dot(sh3,hx.Blend(sw,sh3))*wa[i];
}
float hb=slopeM*1.6-0.3+(rockH-snowH)*0.6;
float rm=smoothstep(0.35,0.65,hb);
RockMask=rm; // vertex-safe copy of the rock mask (no ddx) for UDS snow masking
// rock: full relief up and down around the ground (as the user liked); snow: none (SnowDisplacementCm 0 - snow bumps caused sky streaks + FPS drop)
// rock relief biased upward (0.35 instead of 0.5): collision is the flat heightmap, so deep dips made the player float above rock
float d=lerp((snowH-0.5)*DispSnow,(rockH-0.35)*DispRock,rm);
// displacement runs in landscape local space, where sideways is 3.5x longer: damp it on steep faces
d*=lerp(0.55,1.0,n.z*n.z);
// detail tiles: the rubble stands up - same hex-tiled moraine as the visible layer, light stones up, dark gaps down
float trD = saturate(TileRock) * smoothstep(0.5, 0.78, n.z);
if (trD > 0.001)
{
  float3 hw; float2 hu1, hu2, hu3; float3 ha; mhx.MHex(WP.xy / 450.0, hw, hu1, hu2, hu3, ha);
  float3 l3 = float3(dot(MorC.SampleLevel(MorCSampler, hu1, 1).rgb, 0.333.xxx), dot(MorC.SampleLevel(MorCSampler, hu2, 1).rgb, 0.333.xxx), dot(MorC.SampleLevel(MorCSampler, hu3, 1).rgb, 0.333.xxx));
  float3 hb3 = pow(max(hw, 0.0), 3.0) * (0.25 + l3); hb3 /= max(hb3.x + hb3.y + hb3.z, 1e-5);
  float lum = dot(l3, hb3);
  d += (lum - 0.45) * TileRubbleCm * trD;
}
// wind ripples stand up too (2026-11-14): same cells and height as the colour node, read at mip 3 (1.2 cm), within
// ~35 m of the player only (Area = the trail map around the player); further out they are below a pixel. Height is
// not scaled with the cell size: the colour node's slopes grow with it instead, so the two match.
if (Area.w > 0.5)
{
  float rpE=6326.3+(WP.z-129770.0)/100.0;
  float rpHard=saturate(smoothstep(6600.0,7600.0,rpE+(hx.Fbm(WP.xy/9000.0+3.3)-0.5)*600.0)*0.9+(1.0-smoothstep(0.72,0.92,n.z))*0.55);
  float ripK=smoothstep(0.80,0.93,n.z)*w.z*smoothstep(0.40,0.66,hx.Fbm(WP.xy/1600.0+61.3))*lerp(0.75,1.0,rpHard)
            *(1.0-smoothstep(2500.0,4000.0,length(WP.xy-Area.xy)))*(1.0-rm)*(1.0-saturate(TileRock));
  if (ripK>0.003)
  {
    float rpt=1.0/RipTile; const float A0=-1.2208;
    float2 rpU=wr.R(WP.xy*rpt,A0); float3 rpW,rpA,rpK3; float2 rpO1,rpO2,rpO3; wr.Cells(rpU,rpW,rpO1,rpO2,rpO3,rpA,rpK3);
    float3 rpH3=float3(RipH.SampleLevel(RipHSampler,wr.R(rpU,rpA.x)*rpK3.x+rpO1,3).r,
                       RipH.SampleLevel(RipHSampler,wr.R(rpU,rpA.y)*rpK3.y+rpO2,3).r,
                       RipH.SampleLevel(RipHSampler,wr.R(rpU,rpA.z)*rpK3.z+rpO3,3).r);
    float3 rpB=wr.Bl(rpW,rpH3); float rpV=rsqrt(max(dot(rpB,rpB),0.2));
    float ripH=saturate(0.5+(dot(rpH3,rpB)-0.5)*rpV);
    d+=(ripH-0.5)*3.0*1.25*ripK; // map 0..1 = -1.5..+1.5 cm; x1.25 as the shading
  }
}
return saturate(0.5+d/400.0);
