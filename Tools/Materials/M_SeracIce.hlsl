// Serac glacier ice. Crack = Ice004 colour (bright = white fracture lines / milky ice, dark = clear ice),
// VN = world normal, WP = world position, Obj = the serac's pivot (at its base).
// Clear ice is blue, fractures and weathered skin are white, the base is deeper blue; fresh snow rests on upward
// faces. Glow = a faint blue light from inside so shaded sides read as ice (light scattering through it), not stone.
float l = dot(Crack, float3(0.3, 0.59, 0.11));
float milky = smoothstep(0.1, 0.42, l);              // mostly weathered white skin; blue only where it is clear
float3 clearIce = float3(0.50, 0.69, 0.84);
float3 whiteIce = float3(0.84, 0.88, 0.92);
float3 ice = lerp(clearIce, whiteIce, milky);
// deeper blue low down, where the serac is old and compressed
float h = saturate((WP.z - Obj.z) / 1200.0);
ice = lerp(ice, float3(0.16, 0.42, 0.70), (1.0 - smoothstep(0.0, 0.3, h)) * 0.4 * (1.0 - milky));
// neighbouring seracs differ a little; a faint dirt layer now and then
float s = frac(sin(dot(Obj.xy, float2(0.0129898, 0.078233))) * 43758.5453);
ice *= lerp(0.94, 1.04, s);
float band = smoothstep(0.88, 0.96, frac(WP.z / 420.0 + s));
ice = lerp(ice, float3(0.55, 0.57, 0.58), band * 0.3);
// snow on the upward faces
float snow = smoothstep(0.5, 0.8, normalize(VN).z);
Rough = lerp(lerp(0.06, 0.28, milky), 0.85, snow);
Glow = float3(0.012, 0.03, 0.055) * (1.0 - milky * 0.5) * (1.0 - snow);
return lerp(ice, float3(0.90, 0.92, 0.95), snow);
