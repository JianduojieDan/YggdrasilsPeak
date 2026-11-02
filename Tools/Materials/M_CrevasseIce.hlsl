// Crevasse lip ice: M_SeracIce's look (Crack = Ice004 colour, VN world normal, WP world position) plus the slot:
// the walls glow blue just under the crest and fall away to near-black about 4 m down, the same colour as
// M_CrevasseAbyss at the bottom, so the slot reads as bottomless. CrestZ = world height of this lip's crest.
float l = dot(Crack, float3(0.3, 0.59, 0.11));
float milky = smoothstep(0.1, 0.42, l);
float3 ice = lerp(float3(0.50, 0.69, 0.84), float3(0.84, 0.88, 0.92), milky);
float band = smoothstep(0.88, 0.96, frac(WP.z / 420.0));
ice = lerp(ice, float3(0.55, 0.57, 0.58), band * 0.3);
float3 n = normalize(VN);
float depth = CrestZ - WP.z;                                   // cm below the crest
float wall = 1.0 - smoothstep(0.35, 0.6, n.z);                 // steep faces only
float deep = smoothstep(40.0, 330.0, depth) * wall;
// first the light coming through the ice turns it saturated blue, then it goes dark
float3 blue = float3(0.10, 0.32, 0.55);
ice = lerp(ice, blue, smoothstep(0.0, 0.35, deep) * 0.8);
ice = lerp(ice, float3(0.004, 0.012, 0.025), smoothstep(0.35, 0.95, deep));
// wind-packed snow on top, faintly textured so the ramps are not flat white
float snow = smoothstep(0.5, 0.8, n.z);
float3 snowCol = float3(0.90, 0.92, 0.95) * lerp(0.92, 1.0, smoothstep(0.2, 0.6, l));
Rough = lerp(lerp(0.06, 0.28, milky), 0.85, snow);
Glow = float3(0.012, 0.03, 0.055) * (1.0 - milky * 0.5) * (1.0 - snow) * (1.0 - smoothstep(0.2, 0.7, deep));
return lerp(ice, snowCol, snow);
