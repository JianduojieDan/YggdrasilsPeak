// Sun-bleached, wind-worn tent fabric left on the mountain for twenty years. Col = the fabric's own base colour,
// VN = world normal, WP = world position. UV bleaching fades the dye towards a pale grey-beige, grit settles in
// the folds and snow dust rests on upward faces.
float l = dot(Col, float3(0.3, 0.59, 0.11));
float3 c = lerp(Col, float3(l, l, l), 0.5);                  // most of the dye is gone
c = lerp(c, float3(0.46, 0.44, 0.39), 0.25);                 // bleached towards dusty canvas
// grit / stains in blotches (cheap hash noise on world position, 40 cm cells, smoothed)
float2 g = WP.xy / 40.0; float2 i = floor(g), f = frac(g); f = f * f * (3.0 - 2.0 * f);
#define H(p) frac(sin(dot(p, float2(127.1, 311.7))) * 43758.5453)
float n = lerp(lerp(H(i), H(i + float2(1, 0)), f.x), lerp(H(i + float2(0, 1)), H(i + float2(1, 1)), f.x), f.y);
c *= lerp(0.72, 1.0, smoothstep(0.25, 0.7, n));
float snow = smoothstep(0.55, 0.9, normalize(VN).z) * smoothstep(0.35, 0.6, n);
Rough = lerp(0.85, 0.9, snow);
return lerp(c, float3(0.86, 0.88, 0.9), snow * 0.8);
