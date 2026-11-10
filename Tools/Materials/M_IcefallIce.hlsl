// M_IcefallIce - the solid icefall blocks (Tools/Blender/icefall_blocks.py), v1 2026-11-10.
// Reference: Downloads/icefall_refs (walking height "Atravesando el Khumbu Icefall"): snow pillows on every upward face,
// broken faces white-cyan with pale blue-green where the ice is clear, bluer low down and in the shade, never grey stone.
// World-space everywhere (blocks are tilted and scaled at random): triplanar Ice004 crack detail, snow by world normal.
// Inputs: WP world pos (cm), VN vertex normal (world), Obj object pivot (block base, cm), IceC/IceN Ice004 colour/normal,
//         SnowC/SnowN the terrain's ClumpSnow colour/normal (so block snow matches the ground snow).
// Outputs: return = base colour, Rough, Nrm (WORLD space normal: material Tangent Space Normal is off), Glow (emissive).
struct IceFx {
  float H(float3 p){ p=frac(p*0.3183099+0.1); p*=17.0; return frac(p.x*p.y*p.z*(p.x+p.y+p.z)); }
  float VN3(float3 p){ float3 i=floor(p), f=frac(p); f=f*f*(3.0-2.0*f);
    return lerp(lerp(lerp(H(i),H(i+float3(1,0,0)),f.x),lerp(H(i+float3(0,1,0)),H(i+float3(1,1,0)),f.x),f.y),
                lerp(lerp(H(i+float3(0,0,1)),H(i+float3(1,0,1)),f.x),lerp(H(i+float3(0,1,1)),H(i+1.0),f.x),f.y),f.z); }
  float3 W(float3 n){ float3 w = pow(abs(n), 4.0); return w / (w.x + w.y + w.z); }
  // triplanar colour and a UDN-blended world normal
  float3 TriC(Texture2D T, SamplerState S, float3 p, float3 w){
    return Texture2DSample(T, S, p.yz).rgb * w.x + Texture2DSample(T, S, p.xz).rgb * w.y + Texture2DSample(T, S, p.xy).rgb * w.z; }
  float3 TriN(Texture2D T, SamplerState S, float3 p, float3 n, float3 w, float k){
    float3 tx = Texture2DSample(T, S, p.yz).xyz * 2 - 1, ty = Texture2DSample(T, S, p.xz).xyz * 2 - 1, tz = Texture2DSample(T, S, p.xy).xyz * 2 - 1;
    float3 nx = float3(0, tx.y, tx.x) * sign(n.x), ny = float3(ty.x, 0, ty.y) * sign(n.y), nz = float3(tz.x, tz.y, 0) * sign(n.z);
    return normalize(n + (nx * w.x + ny * w.y + nz * w.z) * k); }
};
IceFx f;
float3 n = normalize(VN);
float3 w = f.W(n);
float3 pIce = WP / 650.0;           // Ice004 every ~6.5 m: broad milky / clear variation, not a cell pattern
float3 pSnow = WP / 180.0;
// snow on upward faces, its edge broken up a little in world space (never a clean line)
float edge = f.VN3(WP / 90.0) * 0.6 + f.VN3(WP / 23.0) * 0.4;
float snow = smoothstep(0.3, 0.52, n.z + (edge - 0.5) * 0.25);
// wind-plastered snow on the sides (the reference blocks are white almost all over; blue shows only on fresh breaks)
float plaster = smoothstep(0.55, 0.8, f.VN3(WP / 140.0) * 0.65 + f.VN3(WP / 37.0) * 0.35) * smoothstep(-0.35, 0.2, n.z);
snow = max(snow, plaster * 0.6);
// --- ice
float3 crack = f.TriC(IceC, IceCSampler, pIce, w);
float l = dot(crack, float3(0.3, 0.59, 0.11));
float milky = smoothstep(0.0, 0.3, l) * 0.75 + 0.25;              // mostly weathered white skin; clear ice only in soft patches
float3 clearIce = float3(0.66, 0.84, 0.90);                       // pale blue-green, as in the reference's fresh breaks
float3 whiteIce = float3(0.88, 0.92, 0.95);
float3 ice = lerp(clearIce, whiteIce, milky);
// fresh breaks: steep faces show more clear blue-green ice, the way the reference's broken faces do
float steep = 1.0 - smoothstep(0.15, 0.6, abs(n.z));
ice = lerp(ice, float3(0.58, 0.80, 0.88), steep * smoothstep(0.35, 0.75, f.VN3(WP / 300.0)) * 0.55);
float h = saturate((WP.z - Obj.z) / 900.0);                       // low on the block: older, denser, bluer
ice = lerp(ice, float3(0.45, 0.66, 0.80), (1.0 - smoothstep(0.0, 0.35, h)) * 0.3 * (1.0 - milky));
float s = frac(sin(dot(Obj.xy, float2(0.0129898, 0.078233))) * 43758.5453);   // neighbours differ a little
ice *= lerp(0.95, 1.03, s);
// --- snow (the ground's own ClumpSnow, slightly brighter on the pillows)
float3 snowC = f.TriC(SnowC, SnowCSampler, pSnow, w) * float3(1.02, 1.03, 1.05);
snowC = lerp(float3(0.9, 0.92, 0.95), snowC, 0.6);
// --- normals
float3 nIce = f.TriN(IceN, IceNSampler, pIce, n, w, 0.3);
float3 nSnow = f.TriN(SnowN, SnowNSampler, pSnow, n, w, 0.35);
Nrm = normalize(lerp(nIce, nSnow, snow));
Rough = lerp(lerp(0.28, 0.5, milky), 0.82, snow);
// light scattering through the ice: the shaded side glows faintly blue, so it reads as ice, not stone
Glow = float3(0.006, 0.016, 0.026) * (1.0 - milky * 0.6) * (1.0 - snow);
return lerp(ice, snowC, snow);
