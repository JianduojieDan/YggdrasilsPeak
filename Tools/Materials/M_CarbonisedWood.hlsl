// Carbonised giant wood fibre. Diff / Nrm = bark texture objects (long fibres along V), WP world position (cm),
// VN world vertex normal. Side projections only use (horizontal, height) so the grain always runs vertically;
// the top projection fades in only on flat tops. Returns charcoal colour; NrmOut is a world-space normal.
// SampleGrad with per-component ddx/ddy: plain Sample is not allowed in the path-tracing hit shader, and UE only
// zeroes ddx() there when it is written per component.
float3 n = normalize(VN);
float3 w = pow(abs(n), 4.0); w /= max(w.x + w.y + w.z, 1e-5);
const float S = 1.0 / 260.0;                      // one texture repeat = 2.6 m
float2 uvX = float2(WP.y, -WP.z * 0.5) * S;       // stretched along the height: fibres as long as the formation
float2 uvY = float2(WP.x, -WP.z * 0.5) * S;
float2 uvZ = WP.xy * S;
float3 cX = Diff.SampleGrad(DiffSampler, uvX, float2(ddx(uvX.x), ddx(uvX.y)), float2(ddy(uvX.x), ddy(uvX.y))).rgb, cY = Diff.SampleGrad(DiffSampler, uvY, float2(ddx(uvY.x), ddx(uvY.y)), float2(ddy(uvY.x), ddy(uvY.y))).rgb, cZ = Diff.SampleGrad(DiffSampler, uvZ, float2(ddx(uvZ.x), ddx(uvZ.y)), float2(ddy(uvZ.x), ddy(uvZ.y))).rgb;
float l = dot(cX * w.x + cY * w.y + cZ * w.z, float3(0.3, 0.59, 0.11));
// charcoal: almost black, warm brown in the fibre ridges, a few ashy grey flecks where the texture is light
float3 col = lerp(float3(0.006, 0.005, 0.0045), float3(0.03, 0.022, 0.016), smoothstep(0.05, 0.35, l));
col = lerp(col, float3(0.09, 0.088, 0.085), smoothstep(0.55, 0.8, l) * 0.4);   // ash (snow light made the old values read as brown bark)
Rough = lerp(0.92, 0.65, smoothstep(0.1, 0.4, l));  // a dull sheen on the fibre ridges, like charcoal
float3 tX = Nrm.SampleGrad(NrmSampler, uvX, float2(ddx(uvX.x), ddx(uvX.y)), float2(ddy(uvX.x), ddy(uvX.y))).xyz * 2.0 - 1.0;
float3 tY = Nrm.SampleGrad(NrmSampler, uvY, float2(ddx(uvY.x), ddx(uvY.y)), float2(ddy(uvY.x), ddy(uvY.y))).xyz * 2.0 - 1.0;
float3 tZ = Nrm.SampleGrad(NrmSampler, uvZ, float2(ddx(uvZ.x), ddx(uvZ.y)), float2(ddy(uvZ.x), ddy(uvZ.y))).xyz * 2.0 - 1.0;
tX.y = -tX.y; tY.y = -tY.y; tZ.y = -tZ.y;          // DirectX green
float k = 1.6;                                     // deep grooves
float3 nb = n + (w.x * float3(0.0, tX.x, tX.y) * sign(n.x) + w.y * float3(tY.x, 0.0, tY.y) * sign(n.y) + w.z * float3(tZ.x, tZ.y, 0.0)) * k;
NrmOut = normalize(nb);
return col;
