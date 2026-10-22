// Rock with a snow cap on its up-facing parts. NW = rock normal (normal map) in world space, VN = vertex normal in world space.
struct RockSnowHash { float H1(float2 p){ return frac(sin(dot(p,float2(127.1,311.7)))*43758.5453); } };
RockSnowHash rsh;
float n1=rsh.H1(floor(WP.xy/40.0)); float n2=rsh.H1(floor(WP.xy/40.0)+float2(1,0)); float n3=rsh.H1(floor(WP.xy/40.0)+float2(0,1)); float n4=rsh.H1(floor(WP.xy/40.0)+1.0);
float2 f=frac(WP.xy/40.0); f=f*f*(3.0-2.0*f);
float vn=lerp(lerp(n1,n2,f.x),lerp(n3,n4,f.x),f.y);
// how much the surface faces up: mostly the rock's real bumps, a bit of the overall shape, broken up by noise
float up=NW.z*0.65+VN.z*0.35+(vn-0.5)*0.22;
float cap=smoothstep(CapStart,CapStart+0.18,up)*Coverage;
// the cap edge: a thin blue-shaded band (the side of the snow layer) and a soft shadow on the rock just below it
float rim=saturate(cap*(1.0-cap)*4.0);
float under=smoothstep(CapStart-0.2,CapStart,up)*(1.0-cap);
float3 rock=Col.rgb*lerp(1.0,0.72,under);
float3 snow=float3(0.90,0.93,0.97)*lerp(float3(1,1,1),float3(0.80,0.88,1.0),rim*0.8);
Rough=lerp(MR.g,0.9,cap);
// tangent-space normal: snow is smooth, so it mostly loses the rock's bumps
NrmOut=normalize(lerp(Nrm,float3(0,0,1),cap*0.85));
AO=lerp(lerp(1.0,0.8,under),0.95,cap);
return lerp(rock,snow,cap);
