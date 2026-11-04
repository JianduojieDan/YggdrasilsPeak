// M_EverestDistant: ~90% snow, steepest faces rock, plus distance haze toward the sky colour.
float3 p = WP;
float3 toP = p - Cam;
float dist = length(toP);
// value noise (no sines - they make stripes)
float2 q = p.xy * 0.00012;
float2 i0 = floor(q); float2 f = frac(q); f = f*f*(3.0-2.0*f);
float h00 = frac(sin(dot(i0,              float2(127.1,311.7)))*43758.5453);
float h10 = frac(sin(dot(i0+float2(1,0),  float2(127.1,311.7)))*43758.5453);
float h01 = frac(sin(dot(i0+float2(0,1),  float2(127.1,311.7)))*43758.5453);
float h11 = frac(sin(dot(i0+float2(1,1),  float2(127.1,311.7)))*43758.5453);
float nn = lerp(lerp(h00,h10,f.x), lerp(h01,h11,f.x), f.y) - 0.5;
float2 q2 = p.xy * 0.0007;
float2 j0 = floor(q2); float2 g = frac(q2); g = g*g*(3.0-2.0*g);
float k00 = frac(sin(dot(j0,              float2(269.5,183.3)))*43758.5453);
float k10 = frac(sin(dot(j0+float2(1,0),  float2(269.5,183.3)))*43758.5453);
float k01 = frac(sin(dot(j0+float2(0,1),  float2(269.5,183.3)))*43758.5453);
float k11 = frac(sin(dot(j0+float2(1,1),  float2(269.5,183.3)))*43758.5453);
float n2 = lerp(lerp(k00,k10,g.x), lerp(k01,k11,g.x), g.y) - 0.5;

float nz = abs(N.z); // ring mesh is mirrored (scale y=-1): normals arrive flipped
// rock only where steep: cutoff ~ steepest 10%
float rockMask = 1.0 - smoothstep(RockCut - 0.06, RockCut + 0.06, nz + nn*0.10 + n2*0.06);
float3 snowCol = float3(0.86,0.88,0.92) * (0.94 + nn*0.08 + n2*0.05);
float3 rockCol = lerp(float3(0.035,0.032,0.03), float3(0.09,0.08,0.07), saturate(n2+0.5));
// rock faces still carry some snow dusting
rockCol = lerp(rockCol, snowCol, 0.06);
float3 col = lerp(snowCol, rockCol, rockMask);

float haze = HazeMax * (1.0 - exp(-max(dist - HazeStart, 0.0) / HazeDist));
Haze = haze;
float3 d = toP / max(dist, 1.0);
d.z = max(d.z, 0.04);
Dir = normalize(d);
return col * (1.0 - haze);
