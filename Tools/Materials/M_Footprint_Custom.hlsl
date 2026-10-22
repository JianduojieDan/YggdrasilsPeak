// One boot print, drawn procedurally in decal UV. DC = decal colour RGB (R: 1 = left foot), DA = its alpha (depth 0..1: 1 = deep powder,
// ~0.35 = hard crust), Life = DecalLifetimeOpacity. The dent comes from the normals (lit side / shaded side follow the real sun);
// the colour only adds a little blue on the floor and a soft shade on the inner wall, so it matches the snow around it.
struct BootPrint {
  float Ell(float2 p, float2 c, float2 r){ float2 d=(p-c)/r; return 1.0-dot(d,d); }
  // > 0 inside the outline, < 0 outside: wide ball at the front, narrow waist, round heel
  float Sole(float2 p){ return max(max(Ell(p,float2(0.5,0.28),float2(0.40,0.25)),Ell(p,float2(0.5,0.74),float2(0.32,0.20))),Ell(p,float2(0.5,0.52),float2(0.26,0.22))); }
  // height: a soft bowl (deep powder) with a pushed-up rim; tread lugs only show on hard snow (Hard = 1 - depth)
  float H(float2 p, float Hard){ float s=Sole(p); float inside=smoothstep(0.0,0.35,s);
    float rim=smoothstep(-0.55,-0.05,s)*(1.0-smoothstep(-0.05,0.12,s));
    float lug=step(0.55,frac(p.y*11.0))*smoothstep(0.15,0.35,s)*Hard;
    return -inside+lug*0.08+rim*0.35; }
};
BootPrint bp;
float2 p = LengthAlongU > 0.5 ? UV.yx : UV.xy;   // p.x across the boot, p.y toe (0) to heel (1)
if (DC.r > 0.5) p.x = 1.0 - p.x;                 // left foot is the mirror image
float depth = DA;
float hard = saturate((1.0 - depth) * 1.5);
float s = bp.Sole(p);
float inside = smoothstep(0.0, 0.35, s);
float rim = smoothstep(-0.55, -0.05, s) * (1.0 - smoothstep(-0.05, 0.12, s));
// normal from the height field: print is ~14 cm wide, ~32 cm long, up to ~10 cm deep in powder
float e = 0.01;
float h0 = bp.H(p, hard);
float dhx = (bp.H(p + float2(e, 0), hard) - h0) / (e * 14.0);
float dhy = (bp.H(p + float2(0, e), hard) - h0) / (e * 32.0);
float k = 20.0 * depth * NormalSign;
float2 g = LengthAlongU > 0.5 ? float2(dhy, dhx) : float2(dhx, dhy);
if (DC.r > 0.5 && LengthAlongU < 0.5) g.x = -g.x;
if (DC.r > 0.5 && LengthAlongU > 0.5) g.y = -g.y;
NrmOut = normalize(float3(-g.x * k, -g.y * k, 1.0));
float wall = saturate(inside * (1.0 - inside) * 4.0);
float3 snow = float3(0.90, 0.93, 0.97);
float3 col = lerp(snow, float3(0.86, 0.91, 0.98), inside * 0.5);
col = lerp(col, float3(0.78, 0.86, 0.97), wall * 0.35 * depth);
col = lerp(col, float3(0.95, 0.97, 1.0), rim * 0.4);
Rough = 0.9;
Op = saturate(inside + rim * 0.8) * Life;
return col;
