// M_IceScan: Khumbu ice from the photos. The scan decides only light and dark; colour comes from depth: saturated blue in deep hollows,
// pale blue on the faces, near white on top, and snow settled on whatever faces up. Inputs: Col (scan colour), VN (world normal).
float l = dot(Col, float3(0.3, 0.59, 0.11));
float t = smoothstep(0.02, 0.32, l);   // mostly pale; real blue only in the darkest cracks
float3 deep = float3(0.20, 0.47, 0.60), mid = float3(0.66, 0.82, 0.89), top = float3(0.93, 0.96, 0.98);
float3 c = t < 0.5 ? lerp(deep, mid, t * 2.0) : lerp(mid, top, (t - 0.5) * 2.0);
float snow = smoothstep(0.3, 0.7, VN.z);
c = lerp(c, float3(0.92, 0.95, 0.98), snow);
Rough = lerp(0.6, 0.9, snow);
return c;
